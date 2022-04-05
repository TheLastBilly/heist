#include "script.h"
#include "raytracer.h"
#include "wavefront.h"

#include <sys/types.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>

#define fatal(fmt, ...)                              { err("[pc: %d] " fmt, pc, __VA_ARGS__); exit(1); }

// I'll be using static allocations for the submission
// but in the future I'll make this dynamic
#define MAX_INSTRUCTION_BUFFER              100
#define MAX_LINE_BUFFER                     256
#define MAX_INSTRUCTION_BUFFER              100
#define MAX_VARIABLE_BUFFER                 100
#define MAX_MESH_COUNT                      10
#define MAX_LIGHT_COUNT                     10

static int instruction_count = 0;
static int mesh_count = 0;
static int variable_count = 0;
static int light_count = 0;
static int object_count = 0;

static char *project_name = NULL;

// Program counter, or rather line counter?
// keeps track of the current instruction being run
static int pc = 0;

struct variable_t variable_buffer[MAX_VARIABLE_BUFFER] = {0};
struct instruction_t instruction_buffer[MAX_INSTRUCTION_BUFFER] = {0};

struct mesh_t mesh_buffer[MAX_MESH_COUNT] = {0};
struct gobject_t object_bufer[MAX_MESH_COUNT] = {0};
struct gparams_t mesh_gparams_buffer[MAX_MESH_COUNT] = {0};
struct gparams_t object_gparams_buffer[MAX_MESH_COUNT] = {0};

struct light_t light_buffer[MAX_LIGHT_COUNT] = {0};

struct camera_t camera;
struct scene_t scene;

static 
void run_instruction(struct instruction_t *instruction);

void
script_run_file(const char * file_path)
{
    size_t line_count = 0;
    char line_buffer[MAX_LINE_BUFFER] = {0};

    // First parse the script file
    if(file_path == NULL)
        return;
    
    FILE * fp = fopen(file_path, "r");
    if(fp == NULL)
        fatal("cannot open file: %s", file_path);

    instruction_count = 0;
    struct instruction_t inst = {0};
    
    // The line_count++ is just there to make sure the line count always increases
    while(fgets(line_buffer, sizeof(line_buffer), fp) && instruction_count < MAX_INSTRUCTION_BUFFER && line_count++ >= 0)
    {
        if(strlen(line_buffer) < 2)
            continue;
        
        // In case this is a comment
        if(line_buffer[0] == '#')
            continue;

        // Quick trick to remove trailing new line
        // This makes it easier to debug any bad lines
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;

        // Clear the instruction buffer
        memset(&inst, 0, sizeof(inst));

        // I know there are a billion ways to do this more efficiently, but
        // I just want this prototype to work for now
        if(sscanf(line_buffer, "name %s", inst.name) == 1)
            inst.command = INSTRUCTION_NAME;
        else if(sscanf(line_buffer, "create %s %s", inst.type, inst.name) == 2)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s (%f %f)", inst.type, inst.name, &inst.x, &inst.y) == 4)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s (%f %f %f)", inst.type, inst.name, &inst.x, &inst.y, &inst.z) == 5)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s %f", inst.type, inst.name, &inst.t) == 4)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "load %s %s", inst.type, inst.name) == 2)
            inst.command = INSTRUCTION_LOAD;
        else if(sscanf(line_buffer, "translate %s (%f %f %f)", inst.name, &inst.x, &inst.y, &inst.z) == 4)
            inst.command = INSTRUCTION_TRANSLATE;
        else if(sscanf(line_buffer, "rotate %s (%f %f %f %f)", inst.name, &inst.t, &inst.x, &inst.y, &inst.z) == 5)
            inst.command = INSTRUCTION_ROTATE;
        else if(sscanf(line_buffer, "assign %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_ASSIGN;
        else if(sscanf(line_buffer, "tag %s", inst.name) == 1)
            inst.command = INSTRUCTION_TAG;
        else if(sscanf(line_buffer, "add %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_ADD;
        else if(sscanf(line_buffer, "equals %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_EQUALS;
        else if(sscanf(line_buffer, "goto %s", inst.name) == 1)
            inst.command = INSTRUCTION_GOTO;
        else if(strcmp("render", line_buffer) == 0)
            inst.command = INSTRUCTION_RENDER;
        else if(strcmp("exit", line_buffer) == 0)
            inst.command = INSTRUCTION_EXIT;
        else
            fatal("line %ld: %s", line_count, line_buffer);

        memcpy(&instruction_buffer[instruction_count++], &inst, sizeof(struct instruction_t));
        line_buffer[0] = '\0';
    }
    fclose(fp);

    // Run the script
    for(pc =0; pc < instruction_count; pc++)
        run_instruction(&instruction_buffer[pc]);
    
    for(int i = 0; i < mesh_count; i++)
        free_mesh(&mesh_buffer[i]);
}

static inline struct pv_t
vertex_to_pv(struct wvertex_t *v)
{
    return (struct pv_t){.x = v->x, .y = v->y, .z = v->z, .w = 1.0};
}

// Allocates an array of gobject_ts and saves the faces from wf to it as triangles
static size_t
allocate_objects_from_wavefront(const struct wavefront_t *wf, struct mesh_t *mesh)
{
    size_t i = 0;
    new_mesh(mesh, wf->faces.used);
    struct pv_t a, b, c;
    for(i = 0; i < wf->faces.used; i++)
    {
        a = vertex_to_pv(&wf->vertices.data[wf->faces.data[i].v[0]]);
        b = vertex_to_pv(&wf->vertices.data[wf->faces.data[i].v[1]]);
        c = vertex_to_pv(&wf->vertices.data[wf->faces.data[i].v[2]]);
        make_triangle(&a, &b, &c, &mesh->triangles[i]);
        mesh->triangles[i].single_sided = true;
        mesh->triangles[i].param = NULL;
    }
    return wf->faces.used;
}

int
find_mesh(char *name)
{
    for(int i = 0; i < mesh_count; i++)
    {
        struct gparams_t *param = (struct gparams_t *)mesh_buffer[i].triangles[0].param;
        if(strcmp(param->name, name) == 0)
            return i;
    }

    return -1;
}

int
find_object(char *name)
{
    for(int i = 0; i < object_count; i++)
    {
        struct gparams_t *param = (struct gparams_t *)object_bufer[i].param;
        if(strcmp(param->name, name) == 0)
            return i;
    }

    return -1;
}

int
find_light(char *name)
{
    for(int i = 0; i < light_count; i++)
    {
        if(strcmp(light_buffer[i].name, name) == 0)
            return i;
    }

    return -1;
}

int
find_variable(char *name)
{
    for(int i = 0; i < variable_count; i++)
    {
        if(strcmp(variable_buffer[i].name, name) == 0)
            return i;
    }

    return -1;
}

int
find_tag(char *name)
{
    for(int i = 0; i < instruction_count; i++)
    {
        struct instruction_t *instruction = &instruction_buffer[i];
        if(instruction->command != INSTRUCTION_TAG)
            continue;
        
        if(strcmp(instruction->name, name))
            return i;
    }

    return -1;
}

void
apply_transform(char *name, matrix_t matrix)
{
    int i = 0;

    if((i = find_mesh(name)) >= 0)
        transform_mesh(&mesh_buffer[i], matrix, &mesh_buffer[i]);

    else if((i = find_object(name)) >= 0)
        transform_object(&object_bufer[i], matrix, &object_bufer[i]);

    else if((i = find_light(name)) >= 0)
        transform_pv(matrix, &light_buffer[i].orig, &light_buffer[i].orig);

    else if(strcmp(name, "camera") == 0)
        transform_camera(&camera, matrix, &camera);
    else
        fatal("cannot find object %s", name);
}

static 
void run_instruction(struct instruction_t *instruction)
{
    int t = 0;
    matrix_t matrix;
    struct color_t id, is;
    struct pv_t pv = PV(0.0f, 0.0f, 0.0f);
    struct gparams_t *mesh_gparams, *object_gparams;
    struct wavefront_t wf = {0};

    mesh_gparams = &mesh_gparams_buffer[mesh_count];
    object_gparams = &object_gparams_buffer[mesh_count];

    switch (instruction->command)
    {
    case INSTRUCTION_NAME:
        project_name = instruction->name;
        break;

    case INSTRUCTION_CREATE:
        {
            if(strcmp(instruction->type, "rectangle") == 0)
            {
                new_rectangle_mesh_wh(instruction->x, instruction->y, &mesh_buffer[mesh_count]);
                mesh_buffer[mesh_count].triangles[0].param = (void *)mesh_gparams;
                mesh_buffer[mesh_count].triangles[0].param = (void *)mesh_gparams;

                mesh_gparams->name = instruction->name;

                mesh_count++;
            }
            
            else if(strcmp(instruction->type, "sphere") == 0)
            {
                pv = PV(0.0f, 0.0f, 0.0f);

                make_sphere(&pv, instruction->t, &object_bufer[object_count]);
                object_bufer[object_count].param = (void *)object_gparams;

                object_gparams->name = instruction->name;

                object_count++;
            }

            else if(strcmp(instruction->type, "light") == 0)
            {
                pv = PV(0.0f, 0.0f, 0.0f);
                id = is = COLOR(instruction->x, instruction->y, instruction->z);

                make_local_light(&pv, &id, &is, &light_buffer[light_count]);
                light_buffer[light_count].name = instruction->name;

                light_count++;
            }

            else if(strcmp(instruction->type, "variable") == 0)
                variable_buffer[variable_count++].name = instruction->name;
        }
        
        break;
    case INSTRUCTION_LOAD:
        t = wavefront_parse_file(instruction->type, &wf);
        if(t < 0)
            fatal("cannot load file %s", instruction->type);

        allocate_objects_from_wavefront(&wf, &mesh_buffer[mesh_count]);
        wavefront_destroy(&wf);

        for(size_t i = 0; i < mesh_buffer[mesh_count].triangle_count; i++)
            mesh_buffer[mesh_count].triangles[i].param = (void *)mesh_gparams;

        mesh_gparams->name = instruction->name;

        mesh_count++;
        break;
    
    case INSTRUCTION_TRANSLATE:
        make_translation_matrix(instruction->x, instruction->y, instruction->z, matrix);
        apply_transform(instruction->name, matrix);
        break;
    case INSTRUCTION_ROTATE:
        pv = PV(instruction->x, instruction->y, instruction->z);
        normalize_pv(&pv, &pv);

        make_rotation_matrix(instruction->t, &pv, matrix);
        apply_transform(instruction->name, matrix);
        break;
    default:
        break;
    }
}