#ifndef RAYTRACER_H__
#define RAYTRACER_H__

#include "renderer.h"
#include "geometry.h"

#define RAYTRACER_DEFAULT_FOV   90.0
#define RAYTRACER_DEFAULT_DEPTH 12

struct raytracer_opts_t
{
    float fov;
    int aa;

    int depth;
};

void raytracer_render(struct scene_t *scene, struct camera_t *camera,  struct framebuffer_t *fb);

#endif
