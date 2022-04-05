// DISCLAIMER: This code was extracted from an older project of mine
// so sorry for any inconsistencies regarding code quality. Like, it
// works, but I was spending too much time removing it's dependencies
// from the other files so I didn't spend too much time refactoring it

#include "wavefront.h"

#define ABS(X)                          ((X) < 0 ? (-X) : (X))
#define SWAP(X, Y, T)                   do { T s = X; X = Y; Y = s; } while(0)

void
wavefront_init(struct wavefront_t *wf)
{
    *wf = (struct wavefront_t){{0}};
}

// I don't really remember why I used custom parsers... but I don't feel like
// touching this bit of the code now
static inline double 
str_to_double(const char * str, size_t size)
{
    double sign = 1.0, val = 0.0, mult = 1.0;
    int index = 0;
    if(str[index] == '-')
    {
        sign = -1.0;
        index++;
    }

    bool dot_found = false;
    for(int i = index; i < size; i++)
    {
        if(str[i] == '.')
        {
            dot_found = true;
            continue;
        }
        else if(str[i] < '0' || str[i] > '9')
            break;

        if(dot_found)
        {
            mult *= 10.0;
            val += ((double)(str[i] - 48))/mult;
        }
        else
        {
            val *= 10.0;
            val += ((double)(str[i] - 48));
        }
    }

    return val * sign;
}

static inline 
int32_t str_to_int32(const char * str, size_t size)
{
    int32_t val = 0;
    int index = 0, sign = 1;
    if(str[index] == '-')
    {
        sign = -1;
        index++;
    }

    for(int i = index; i < size; i++)
    {
        if(str[i] < '0' || str[i] > '9')
            break;
        val *= 10;
        val += (int32_t)(str[i] - 48);
    }

    return val * sign;
}

static inline void
wavefront_parse_vertex_(const char * line, struct wvertex_t *v)
{
    int next_index = 2;
    size_t size = strlen(line);

    v->x = str_to_double(&line[next_index++], size);
    
    for(; next_index < size && line[next_index] != ' '; next_index++) ;
    v->y = str_to_double(&line[++next_index], size);
    
    for(; next_index < size && line[next_index] != ' '; next_index++) ;
    v->z = str_to_double(&line[++next_index], size);
}

static inline void
wavefront_parse_face_(const char * line, struct wface_t *f)
{
    int next_index = 2;
    size_t size = strlen(line);

    f->v[0] = (size_t)str_to_int32(&line[next_index++], size) -1;
    
    for(; next_index < size && line[next_index] != ' '; next_index++) ;
    f->v[1] = (size_t)str_to_int32(&line[++next_index], size) -1;
    
    for(; next_index < size && line[next_index] != ' '; next_index++) ;
    f->v[2] = (size_t)str_to_int32(&line[++next_index], size) -1;
}

int 
wavefront_parse_file(const char * file_path, struct wavefront_t *wf)
{
    if(file_path == NULL)
        return -1;

    FILE * fp = fopen(file_path, "r");
    if(fp == NULL)
        return -1;

    //Reserve space for the vertices and faces
    //Do a cheap attempt to figure out how much memory you'll need for the file
    fseek(fp, 0, SEEK_END);
    size_t prealloc_size = ftell(fp)/30;    //I just looked for the biggest line on a random .obj, and that's how many chatacters it had so...
    fseek(fp, 0, SEEK_SET);
    
    reserve_wvertex_list_t(&wf->vertices, prealloc_size);
    reserve_wface_list_t(&wf->faces, prealloc_size);

    // Get vertices and faces count
    char line_buffer[256] = {0};

    struct wvertex_t v;
    struct wface_t f;
    
    while(fgets(line_buffer, sizeof(line_buffer), fp))
    {
        uint16_t id = 
            (0x00ff & (uint16_t)(line_buffer[1])) | 
            (0xff00 & (uint16_t)(line_buffer[0] << 8));

        switch (id)
        {
        case 0x5620: // 'V '
        case 0x7620: // 'v '
            wavefront_parse_vertex_(line_buffer, &v);
            append_wvertex_list_t(&wf->vertices, &v);
            break;
        case 0x4620: // 'F '
        case 0x6620: // 'f '
            wavefront_parse_face_(line_buffer, &f);
            append_wface_list_t(&wf->faces, &f);
            break;
        
        default:
            continue;
        }
    }

    shrink_wvertex_list_t(&wf->vertices);
    shrink_wface_list_t(&wf->faces);

    fclose(fp);
    
    return 0;
}

void 
wavefront_destroy(struct wavefront_t * wf)
{
    free_wface_list_t(&wf->faces);
    free_wvertex_list_t(&wf->vertices);

    memset(wf, 0, sizeof(struct wavefront_t));
}

