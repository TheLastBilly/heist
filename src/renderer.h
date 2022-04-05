#ifndef RENDERER_H__
#define RENDERER_H__

#include <string.h>
#include <stdlib.h>

#include "ppm.h"
#include "geometry.h"

#define RGB(R,G,B)              PPM_RGB(R,G,B) 
#define RGB_R(rgb)              (((uint32_t)(rgb) & 0x00ff0000) >> 16)
#define RGB_G(rgb)              (((uint32_t)(rgb) & 0x0000ff00) >> 8)
#define RGB_B(rgb)              (((uint32_t)(rgb) & 0x000000ff))

#define COLOR(x, y, z)          (struct color_t){.r = (x), .g = (y), .b = (z)}

#define add_color(x, y)         (struct color_t){.r = (x).r + (y).r, .g = (x).g + (y).g, .b = (x).b + (y).b}
#define substract_color(x, y)   (struct color_t){.r = (x).r - (y).r, .g = (x).g - (y).g, .b = (x).b - (y).b}
#define multiply_color(x, y)    (struct color_t){.r = (x).r * (y).r, .g = (x).g * (y).g, .b = (x).b * (y).b}
#define scale_color(x, m)       (struct color_t){.r = (x).r * m, .g = (x).g * m, .b = (x).b * m}       
#define clamp_color(x)          (struct color_t){.r = saturate((x).r), .g = saturate((x).g), .b = saturate((x).b)}       
#define saturate(x)             ((x) > 1.0f ? 1.0f : (x < 0.0l ? 0.0f : x))

#define pixel_to_color(p)       (struct color_t){.r = RGB_R(p)/255, .g = RGB_G(p)/255, .b = RGB_B(p)/255}
#define color_to_pixel(c)       RGB((int)(255*saturate(c.r)), (int)(255*saturate(c.g)), (int)(255*saturate(c.b)))

struct color_t
{
    float r, g, b;
};

struct gparams_t
{
    struct color_t ac, sc, dc, is, id;
    float ka, kd, ks;
    float pc;

    char *name;

    bool emits;
};


enum light_type_t
{
    DISTANT_LIGHT,
    AREA_LIGHT,
    LOCAL_LIGHT
};

struct light_params_t
{
    struct pv_t direction, inv_direction;

    struct color_t id, is;
};

struct light_t
{
    enum light_type_t type;
    struct pv_t orig, direction;

    struct gobject_t *objects;
    size_t object_count;

    struct color_t id, is;

    char *name;

    float xlimits[2], ylimits[2], zlimits[2];
};

struct scene_t
{
    struct gobject_t *objects;
    size_t objects_count;

    struct mesh_t **meshes;
    size_t mesh_count;

    struct light_t *lights;
    size_t light_count;

    float shadow_bias;
    struct color_t ai;

    int area_light_n;
    int max_depth;

    int samples;

    bool global_illumination;
    bool has_area_light;
};

struct framebuffer_t
{
    int height, width;
    pixel_t *pixels;
};

struct camera_t
{
    matrix_t transform, inv_transform;
    struct color_t background_color;

    // Used to pass camera options
    // to renderer(s)
    void *opts;
};


const char* color_to_str(struct color_t *c);

void make_scene(struct color_t *ai, struct scene_t *scene);

void make_camera(struct pv_t *look_at, struct pv_t *up, struct pv_t *orig, struct camera_t *c);
void transform_camera(struct camera_t *c, matrix_t m, struct camera_t *r);

void new_framebuffer(int height, int width, struct framebuffer_t *fb);
void free_framebuffer(struct framebuffer_t *fb);

void make_distant_light(struct pv_t *pv, struct pv_t *direction, struct color_t *id, struct color_t *is, struct light_t *light);
void make_local_light(struct pv_t *pv, struct color_t *id, struct color_t *is, struct light_t *light);
void make_area_light(struct pv_t *orig, struct light_t *light);

void compute_light(struct pv_t *pv, struct pv_t *normal, struct light_t *light, struct light_params_t *params);
// void make_directional_light(struct pv_t *pv, struct pv_t *normal, struct light_t *light);

#endif
