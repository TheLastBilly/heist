#include "renderer.h"
#include "fmath.h"

const char*
color_to_str(struct color_t *c)
{
    static char buff[1024] = {0};
    
    snprintf(buff, 1024,
        "c->r = %f, c->g = %f, c->b = %f", 
        c->r, c->g, c->b
    );

    return buff;
}

void 
make_scene(struct color_t * ai, struct scene_t *s)
{
    *s = (struct scene_t){0};
    s->shadow_bias = 0.01;
    s->ai = *ai;
}

// Initialize camera with identity matrix
void
make_camera(struct pv_t *look_at, struct pv_t *up, struct pv_t *orig, struct camera_t *c)
{
    matrix_t m, tm;
    struct pv_t b;

    cross_pv(look_at, up, &b);

    memset(c, 0, sizeof(*c));

    m[0][0] = b.x; 
    m[0][1] = b.y; 
    m[0][2] = b.z; 

    m[1][0] = up->x; 
    m[1][1] = up->y; 
    m[1][2] = up->z; 

    m[2][0] = look_at->x; 
    m[2][1] = look_at->y; 
    m[2][2] = look_at->z; 
 
    m[3][0] = 0; 
    m[3][1] = 0; 
    m[3][2] = 0; 

    m[0][3] = m[1][3] = m[2][3] = 0.0;
    m[3][3] = 1.0;

    // make_identity_matrix(m);

    make_translation_matrix(orig->x, orig->y, orig->z, tm);
    mxm(tm, m, c->transform);
    inv_matrix(m, c->inv_transform);
}

// Transform camera matrix.
// Also get the inverse transformation while you are at it
void
transform_camera(struct camera_t *c, matrix_t m, struct camera_t *r)
{
    matrix_t b;

    mxm(m, c->transform, b);
    inv_matrix(b, r->inv_transform);
    copy_matrix(b, r->transform);
}

// Allocates space for framebuffer and saves
// pointer, width and height into fb
void
new_framebuffer(int height, int width, struct framebuffer_t *fb)
{
    fb->pixels = (pixel_t *)calloc(height * width, sizeof(pixel_t));
    fb->height = height;
    fb->width = width;
}

// Frees allocated memory new_framebuffer and clears
// fb
void
free_framebuffer(struct framebuffer_t *fb)
{
    free(fb->pixels);
    fb->pixels = NULL;
    fb->height = 0;
    fb->width = 0;
}

void
make_distant_light(struct pv_t *pv, struct pv_t *direction, struct color_t *id, struct color_t *is, struct light_t *light)
{
    light->orig = *pv;
    light->direction = *direction;
    light->type = DISTANT_LIGHT;

    light->is = *is;
    light->id = *id;

    normalize_pv(&light->direction, &light->direction);
}

void
make_local_light(struct pv_t *pv, struct color_t *id, struct color_t *is, struct light_t *light)
{
    light->orig = *pv;
    light->type = LOCAL_LIGHT;

    light->is = *is;
    light->id = *id;
}


void
make_area_light(struct pv_t *pv, struct light_t *light)
{
    light->orig = *pv;
    light->type = AREA_LIGHT;
}

void 
compute_light(struct pv_t *pv, struct pv_t *normal, struct light_t *light, struct light_params_t *params)
{
    switch(light->type)
    {
        case LOCAL_LIGHT:
            {
                substract_pv(pv, &light->orig, &params->direction);
                normalize_pv(&params->direction, &params->direction);
                scale_pv(&params->direction, -1.0, &params->inv_direction);
            }
            break;
            
        case DISTANT_LIGHT:
            scale_pv(&light->direction, -1.0, &params->inv_direction);
            params->direction = light->direction;
            break;

        default:
            break;
    }

    params->id = light->id;
    params->is = light->is;
}
