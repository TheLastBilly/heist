#include "script.h"
#include "raytracer.h"
#include "wavefront.h"

#include <sys/types.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>

#define fatal(fmt, ...)                                 { err("[pc: %d] " fmt, pc, __VA_ARGS__); exit(1); }
#define fatal_na(fmt)                                   fatal(fmt "%s", "")

// I'll be using static allocations for the submission
// but in the future I'll make this dynamic
#define MAX_INSTRUCTION_BUFFER              100
#define MAX_LINE_BUFFER                     256
#define MAX_INSTRUCTION_BUFFER              100
#define MAX_VARIABLE_BUFFER                 100
#define MAX_MESH_COUNT                      10
#define MAX_LIGHT_COUNT                     10

#define DEFAULT_WIDTH                       640
#define DEFAULT_HEIGHT                      480

static int instruction_count = 0;
static int mesh_count = 0;
static int variable_count = 0;
static int light_count = 0;
static int object_count = 0;

static int shot_count = 0;

static char *project_name = NULL;

// Program counter, or rather line counter?
// keeps track of the current instruction being run
static int pc = 0;

struct variable_t variable_buffer[MAX_VARIABLE_BUFFER] = {0};
struct instruction_t instruction_buffer[MAX_INSTRUCTION_BUFFER] = {0};

struct mesh_t mesh_buffer[MAX_MESH_COUNT] = {0};
struct mesh_t *mesh_pointer_buffer[MAX_MESH_COUNT] = {0};

struct gobject_t object_bufer[MAX_MESH_COUNT] = {0};
struct gparams_t mesh_gparams_buffer[MAX_MESH_COUNT] = {0};
struct gparams_t object_gparams_buffer[MAX_MESH_COUNT] = {0};

struct light_t light_buffer[MAX_LIGHT_COUNT] = {0};

struct camera_t camera = {0};
struct scene_t scene = {0};
struct raytracer_opts_t camera_opts = {0};

static int height = DEFAULT_HEIGHT, width = DEFAULT_WIDTH;

static struct gparams_t default_gparams = {0};

static 
void run_instruction(struct instruction_t *instruction);

void
script_run_file(const char * file_path)
{
    struct pv_t look_at = {0}, up = {0}, origin = {0};
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
        
        else if(sscanf(line_buffer, "create %s %s (%f %f %f)", inst.type, inst.name, &inst.x, &inst.y, &inst.z) == 5)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s (%f %f)", inst.type, inst.name, &inst.x, &inst.y) == 4)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s %f", inst.type, inst.name, &inst.t) == 3)
            inst.command = INSTRUCTION_CREATE;
        else if(sscanf(line_buffer, "create %s %s", inst.type, inst.name) == 2)
            inst.command = INSTRUCTION_CREATE;

        else if(sscanf(line_buffer, "color %s (%f %f %f)", inst.name, &inst.x, &inst.y, &inst.z) == 4)
            inst.command = INSTRUCTION_COLOR;

        // Same but take variable names as arguments
        else if(sscanf(line_buffer, "color %s ( %s %s %s )", inst.name, inst.var[0], inst.var[1], inst.var[2]) == 4)
            inst.command = INSTRUCTION_COLOR;
        
        else if(sscanf(line_buffer, "load %s %s", inst.type, inst.name) == 2)
            inst.command = INSTRUCTION_LOAD;
        
        else if(sscanf(line_buffer, "translate %s (%f %f %f)", inst.name, &inst.x, &inst.y, &inst.z) == 4)
            inst.command = INSTRUCTION_TRANSLATE;
        else if(sscanf(line_buffer, "rotate %s (%f %f %f %f)", inst.name, &inst.x, &inst.y, &inst.z, &inst.t) == 5)
            inst.command = INSTRUCTION_ROTATE;
        
        // Same but take variable names as arguments
        else if(sscanf(line_buffer, "translate %s (%s %s %s)", inst.name, inst.var[0], inst.var[1], inst.var[2]) == 4)
            inst.command = INSTRUCTION_TRANSLATE;
        else if(sscanf(line_buffer, "rotate %s (%s %s %s %s)", inst.name, inst.var[0], inst.var[1], inst.var[2], inst.var[3]) == 5)
            inst.command = INSTRUCTION_ROTATE;

        else if(sscanf(line_buffer, "assign %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_ASSIGN;
        else if(sscanf(line_buffer, "add %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_ADD;
        else if(sscanf(line_buffer, "substract %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_SUB;
        else if(sscanf(line_buffer, "equals %s %f", inst.name, &inst.t) == 2)
            inst.command = INSTRUCTION_EQUALS;
        
        else if(sscanf(line_buffer, "tag %s", inst.name) == 1)
            inst.command = INSTRUCTION_TAG;
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

    // Initialize camera
    camera_opts.aa = 0;
    camera_opts.depth = 1;
    camera_opts.fov = 90.0f;

    up = PV(0.0f, 1.0f, 0.0f);
    look_at = PV(0.0f, 0.0f, -1.0f);
    origin = PV(0.0f, 0.0f, 0.0f);

    make_camera(&look_at, &up, &origin, &camera);
    camera.background_color = COLOR(0.5f, 0.5f, 0.5f);
    camera.opts = (void *)&camera_opts;

    // Initialize scene
    scene.objects = object_bufer;
    scene.lights = light_buffer;

    for(int i = 0; i < MAX_MESH_COUNT; i++)
        mesh_pointer_buffer[i] = &mesh_buffer[i];
    scene.meshes = mesh_pointer_buffer;
    
    scene.shadow_bias = 0.001;
    scene.area_light_n = 5;
    scene.global_illumination = false;

    // Set default parameters for objects
    default_gparams.ac = COLOR(1.0f, 1.0f, 1.0f);
    default_gparams.dc = COLOR(0.0f, 0.0f, 0.0f);
    default_gparams.sc = COLOR(1.0f, 1.0f, 1.0f);

    default_gparams.pc = 100;

    default_gparams.ka = 0;
    default_gparams.kd = 1;
    default_gparams.ks = 1;

    // Run the script
    for(pc =0; pc < instruction_count; pc++)
    {
        scene.objects_count = object_count;
        scene.mesh_count = mesh_count;
        scene.light_count = light_count;
        run_instruction(&instruction_buffer[pc]);
    }
    
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

void 
apply_color(char *name, struct color_t *color)
{
    int i = 0;

    if((i = find_mesh(name)) >= 0)
        mesh_gparams_buffer[i].ac = 
        mesh_gparams_buffer[i].dc = *color;

    else if((i = find_object(name)) >= 0)
        object_gparams_buffer[i].ac = 
        object_gparams_buffer[i].dc =  *color;
        
    else
        fatal("cannot find object %s", name);
}

static 
void run_instruction(struct instruction_t *instruction)
{
    int t = 0;
    matrix_t matrix;
    struct color_t id, is, color;
    struct framebuffer_t fb = {0};
    struct pv_t pv = PV(0.0f, 0.0f, 0.0f);
    struct gparams_t *mesh_gparams, *object_gparams;
    struct wavefront_t wf = {0};

    float *values[4] = {&instruction->x, &instruction->y, &instruction->z, &instruction->t};

    // And because you can never be too sure...
    char name[SCRIPT_MAX_TEXT_BUFFER + 128] = {0};

    mesh_gparams = &mesh_gparams_buffer[mesh_count];
    object_gparams = &object_gparams_buffer[mesh_count];

    for(size_t i = 0; i < 4; i++)
    {
        if(strlen(instruction->var[i]) > 0)
            t = find_variable(instruction->var[i]);
        else
            continue;
        
        if(t < 0)
            fatal("cannot find variable %s", instruction->var[i]);
        
        *values[i] = variable_buffer[i].value;
    }

    switch (instruction->command)
    {
    case INSTRUCTION_NAME:
        project_name = instruction->name;
        break;

    case INSTRUCTION_CREATE:
        {
            if(strcmp(instruction->type, "rectangle") == 0)
            {
                if(instruction->x == 0 && instruction->y == 0)
                    wrn("creating rectangle \"%s\" with empty dimensions", instruction->name);

                *mesh_gparams = default_gparams;

                new_rectangle_mesh_wh(instruction->x, instruction->y, &mesh_buffer[mesh_count]);
                mesh_buffer[mesh_count].triangles[0].param = (void *)mesh_gparams;
                mesh_buffer[mesh_count].triangles[0].param = (void *)mesh_gparams;

                mesh_gparams->name = instruction->name;

                mesh_count++;
            }
            
            else if(strcmp(instruction->type, "sphere") == 0)
            {
                if(instruction->t == 0)
                    wrn("creating sphere \"%s\" with no radius", instruction->name);

                pv = PV(0.0f, 0.0f, 0.0f);
                *object_gparams = default_gparams;

                make_sphere(&pv, instruction->t, &object_bufer[object_count]);
                object_bufer[object_count].param = (void *)object_gparams;

                object_gparams->name = instruction->name;

                object_count++;
            }

            else if(strcmp(instruction->type, "light") == 0)
            {
                pv = PV(0.0f, 0.0f, 0.0f);

                is = COLOR(instruction->x, instruction->y, instruction->z);
                id = scale_color(is, 0.5);

                make_local_light(&pv, &id, &is, &light_buffer[light_count]);
                light_buffer[light_count].name = instruction->name;

                light_count++;
            }

            else if(strcmp(instruction->type, "variable") == 0)
                variable_buffer[variable_count++].name = instruction->name;
            
            else
                fatal("cannot create object \"%s\" of unknown type \"%s\"", instruction->name, instruction->type);

            inf("%s %s was created", instruction->type, instruction->name);
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
        // inf("translating %s by (%f %f %f)", instruction->name, instruction->x, instruction->y, instruction->z);

        make_translation_matrix(instruction->x, instruction->y, instruction->z, matrix);
        apply_transform(instruction->name, matrix);
        break;

    case INSTRUCTION_ROTATE:
        // inf("rotating camera %s by %f degrees on normal (%f %f %f)", instruction->name, instruction->t, instruction->x, instruction->y, instruction->z);
        pv = PV(instruction->x, instruction->y, instruction->z);
        normalize_pv(&pv, &pv);

        make_rotation_matrix(instruction->t, &pv, matrix);
        apply_transform(instruction->name, matrix);
        break;
    
    case INSTRUCTION_RENDER:
        if(project_name == NULL)
            fatal_na("cannot render shot without a project name");
        
        inf("taking frame %d", shot_count);

        snprintf(name, sizeof(name), "%s_%d.ppm", project_name, shot_count++);

        new_framebuffer(height, width, &fb);
        raytracer_render(&scene, &camera, &fb);
        ppm_save(name, fb.pixels, height, width);

        free_framebuffer(&fb);
        break;
    
    case INSTRUCTION_COLOR:
        color = COLOR(instruction->x, instruction->y, instruction->z);
        apply_color(instruction->name, &color);
        break;

    case INSTRUCTION_ASSIGN:
        t = find_variable(instruction->name);
        if(t < 0)
            fatal("cannot find variable \"%s\"", instruction->name);

        variable_buffer[t].value = instruction->t;
        break;
    
    case INSTRUCTION_EQUALS:
        t = find_variable(instruction->name);
        if(t < 0)
            fatal("cannot find variable \"%s\"", instruction->name);
        if(variable_buffer[t].value != instruction->t)
            pc += 1;
        break;

    case INSTRUCTION_ADD:
        t = find_variable(instruction->name);
        if(t < 0)
            fatal("cannot find variable \"%s\"", instruction->name);

        variable_buffer[t].value += instruction->t;
        break;

    case INSTRUCTION_SUB:
        t = find_variable(instruction->name);
        if(t < 0)
            fatal("cannot find variable \"%s\"", instruction->name);

        variable_buffer[t].value -= instruction->t;
        break;
    
    case INSTRUCTION_GOTO:
        t = find_tag(instruction->name);
        if(t < 0)
            fatal("cannot find tag \"%s\"", instruction->name);
        
        pc = t;
        break;

    default:
        break;
    }
}