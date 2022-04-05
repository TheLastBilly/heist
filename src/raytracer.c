#include "raytracer.h"
#include "fmath.h"

#include <assert.h>

#define SMALL_F         0.000000001

struct ray_t
{
    struct pv_t orig, dir, inv_dir;
    short dir_sign[3];
    bool primary_ray;

    bool indirect;

    struct gobject_t *object;

    int depth;
};

struct hit_info_t
{
    float u,v,w;

    struct pv_t normal, hit_point;
    struct gobject_t* object;
};

static void
make_ray(struct pv_t *orig, struct pv_t *dir, struct ray_t *r)
{
    r->orig = *orig;
    r->orig.w = 1.0;
    normalize_pv(dir, &r->dir);
    scale_pv(dir, -1.0, &r->inv_dir);
    r->indirect = false;
    r->depth = 0;
    r->primary_ray = false;
    /* inverse_pv(dir, &r->inv_dir); */
    /* r->dir_sign[0] = (r->inv_dir.x < 0); */
    /* r->dir_sign[1] = (r->inv_dir.y < 0); */
    /* r->dir_sign[2] = (r->inv_dir.z < 0); */
}

static struct raytracer_opts_t
DEFAULT_OPTS =
{
    .fov    = RAYTRACER_DEFAULT_FOV,
    .aa     = 0,

    .depth  = RAYTRACER_DEFAULT_DEPTH
};

static struct gparams_t
DEFAULT_OBJECT_PARAMS = {0};

static void
ray_intersect_point(struct ray_t *r, float t, struct pv_t *v)
{
    scale_pv(&r->dir, t, v);
    add_pv(v, &r->orig, v);

    v->w = 1.0;
}

static float
ray_intersect(struct ray_t *ray, struct gobject_t *object, struct hit_info_t *info)
{
    struct hit_info_t dinfo;
    struct pv_t *orig, *dir;
    float normal_dot_dir = 0.0;
    float t0 = -1.0, t1 = -1.0, a = 0.0, b = 0.0, c = 0.0;
    struct pv_t pv = {0.0};

    if(info == NULL)
        info = &dinfo;

    orig = &ray->orig;
    dir = &ray->dir;

    switch(object->type)
    {
        case GEOMETRY_SPHERE:
            substract_pv(orig, &object->center, &pv);
            /* inf("orig: %s", pv_to_str(orig)); */
            /* inf("cent: %s", pv_to_str(&object->center)); */
            /* inf("pv:   %s\n", pv_to_str(&pv)); */

            a = dot_product(dir, dir);
            b = 2.0 * dot_product(dir, &pv);
            c = dot_product(&pv, &pv) - object->radius2;
            if(!solve_quadratic(a, b, c, &t0, &t1)) 
                return -1.0;

            if(t0 > t1)
                SWAP(t0, t1, float);
            if(t0 < 0.0)
            {
                t0 = t1;
                if(t0 < 0.0)
                    return -1.0;
            }

            ray_intersect_point(ray, t0, &info->hit_point);
            substract_pv(&info->hit_point, &object->center, &pv);
            normalize_pv(&pv, &info->normal);

            /* inf("t0 = %f\t\tt1 = %f", t0, t1); */
            // scale_pv(&pv, object->radius, &pv);
            // add_pv(&pv, orig, &pv);
            break;

        case GEOMETRY_DISK:
        case GEOMETRY_PLANE:
            normal_dot_dir = dot_product(&object->normal, dir);
            if(fabs(normal_dot_dir) < SMALL_F)
                return -1.0;
            
            substract_pv(&object->center, orig, &pv);
            t0 = dot_product(&pv, &object->normal)/dot_product(dir, &object->normal);
            if(t0 < 0.0)
                return t0;
            
            info->normal = object->normal;
            info->object = object;
            ray_intersect_point(ray, t0, &info->hit_point);
            pv = info->hit_point;

            // Only go further if the object is a disk
            if(object->type == GEOMETRY_PLANE)
                break;

            // Get the radious of the intersct point
            if(magnitude_pv(&pv) > object->radius)
                return -1.0;
            return t0;
            break;
        case GEOMETRY_TRIANGLE:
            {
                struct pv_t p, c;
                normal_dot_dir = dot_product(&object->normal, dir);

                // Check if dir and normal are orthogonal
                if(fabs(normal_dot_dir) < SMALL_F)
                    return -1.0;

                // Check for single-sided triangle.
                if(normal_dot_dir > 0.0 && object->single_sided)
                    return -1.0;

                // Find t
                t0 = (
                    (-dot_product(&object->normal, orig) + dot_product(&object->normal, &object->edges[0])) /
                    normal_dot_dir
                );
                if(t0 < 0.0)
                    return t0;

                // Inside out test
                // (Note: ac, cb and ba were precalculated on form_triangle)
                ray_intersect_point(ray, t0, &pv);

                // p = (Q-A)
                substract_pv(&pv, &object->edges[0], &p);
                cross_pv(&object->ba, &p, &c);
                if(dot_product(&c, &object->normal) < 0)
                    return -1.0;

                // p = (Q-B)
                substract_pv(&pv, &object->edges[1], &p);
                cross_pv(&object->cb, &p, &c);
                if((info->u = dot_product(&c, &object->normal)) < 0)
                    return -1.0;

                // p = (Q-C)
                substract_pv(&pv, &object->edges[2], &p);
                cross_pv(&object->ac, &p, &c);
                if((info->v = dot_product(&c, &object->normal)) < 0)
                    return -1.0;

                info->u /= object->area;
                info->v /= object->area;
                info->w = 1 - info->u - info->v;
                info->normal = object->normal;
                info->object = object;
                ray_intersect_point(ray, t0, &info->hit_point);
            }
            break;
        default:
            break;
    }
    
    return t0;
}

static float
raytrace(struct ray_t *ray, struct scene_t *scene, struct camera_t *camera, struct color_t *color, struct hit_info_t *ext_info);

struct lights_t
{
    struct color_t diffuse_light, specular_light, ambient_light;
};

static void
generate_random_direction(struct pv_t *normal, struct pv_t *r)
{
    float m2 = 0.0;

    while (true) {
        *r = PV(rrandf(-1.0f,1.0f), rrandf(-1.0f,1.0f), rrandf(-1.0f,1.0f));
        m2 = magnitude_pv(r);
        if (m2 > 1) continue;

        normalize_pv(r,r);
        if(dot_product(r, normal) <= 0.0)
            scale_pv(r, -1.0, r);

        return;
    }
}

static struct lights_t
compute_lights(struct ray_t *ray, float t, struct scene_t *scene, struct gobject_t *object, struct camera_t *camera, struct hit_info_t *info, struct gparams_t *gparams)
{
    size_t i = 0, s = 0;
    float shadow_rt = 0.0, diffuse_intensity = 0.0, specular_intensity = 0.0;
    struct ray_t shadow_ray = {{0.0}};
    struct light_t *light = NULL;
    struct light_params_t params = {0};
    struct ray_t light_ray;
    struct color_t tmp_color = {0.0}, diffuse_color = {0.0}, specular_color = {0.0}, ambient_color = {0.0};
    struct pv_t *normal = NULL, hit_point = {0.0}, shadow_orig = {0.0}, ps = {0.0};
    
    normal = &info->normal;
    /* normal->z *= -1.0; */
    hit_point = info->hit_point;
    ambient_color = COLOR(1.0, 1.0, 1.0);
    for(i = 0; i < scene->light_count; i++)
    {
        light = &scene->lights[i];
        if(light->type == AREA_LIGHT)
            continue;
        // inf("orig: %s", pv_to_str(&hit_point));
        compute_light(&hit_point, normal, light, &params);
        
        // Figure out if hitpoint is on the shadows
        shadow_orig = hit_point;
        substract_pv(&light->orig, &shadow_orig, &shadow_ray.dir);
        normalize_pv(&shadow_ray.dir, &shadow_ray.dir);
        scale_pv(&shadow_ray.dir, scene->shadow_bias, &shadow_orig);
        add_pv(&shadow_orig, &hit_point, &shadow_orig);
        make_ray(&shadow_orig, &shadow_ray.dir, &shadow_ray);

        shadow_rt = raytrace(&shadow_ray, scene, camera, NULL, NULL);

        // If the object the shadow ray touched is not from light,
        // The this is a shadow
        if(shadow_rt > 0.0 && shadow_rt < INFINITY)
            continue;

        // If global illumination hasn't been specified, use
        // PhonShader
        add_pv(&shadow_ray.dir, &ray->inv_dir, &ps);
        normalize_pv(&ps, &ps);

        specular_intensity = dot_product(&ps, normal);
        specular_intensity = MAX(0.0, specular_intensity);
        specular_intensity = pow(specular_intensity, gparams->pc);

        // Calculate diffuse light
        diffuse_intensity = MAX(0.0, dot_product(normal, &params.inv_direction)) * M_PI;
        tmp_color = scale_color(params.id, diffuse_intensity);
        diffuse_color = add_color(diffuse_color, tmp_color);

        // Calculate specular light
        tmp_color = scale_color(params.is, specular_intensity);
        specular_color = add_color(specular_color, tmp_color);
    }

    struct color_t indirect_diffuse = {0.0}, indirect_specular = {0.0}, indirect_ambient = {0.0};
    hit_point = info->hit_point;

    light_ray.orig = hit_point;
    scale_pv(normal, scene->shadow_bias, &light_ray.orig);
    add_pv(&light_ray.orig, &hit_point, &light_ray.orig);

    for(s = 0; scene->has_area_light && s < scene->light_count; s++)
    {
        for(i = 0; i < scene->area_light_n; i++)
        {
            light = &scene->lights[s];
            if(light->type != AREA_LIGHT)
                continue;

            float t = 0.0, intensity = 0.0;
            struct pv_t rand_buffer, ps;
            struct gparams_t *params;
            struct hit_info_t info;

            struct color_t diffuse_temp = {0.0}, specular_temp = {0.0}, ambient_temp = {0.0};

            rand_buffer = PV(
                rrandf(light->xlimits[0], light->xlimits[1]), 
                rrandf(light->ylimits[0], light->ylimits[1]), 
                rrandf(light->zlimits[0], light->zlimits[1])
            );

            substract_pv(&rand_buffer, &light_ray.orig, &light_ray.dir);
            normalize_pv(&light_ray.dir, &light_ray.dir);
            
            make_ray(&light_ray.orig, &light_ray.dir, &light_ray);
            light_ray.indirect = true;

            light_ray.object = NULL;
            t = raytrace(&light_ray, scene, camera, NULL, &info);
            if(light_ray.object != NULL)
                params = (struct gparams_t *)light_ray.object->param;

            if(t < 0.0 || t == INFINITY || !params->emits)
                continue;
            
            // Calculate diffuse light
            intensity = MAX(0.0, dot_product(normal, &light_ray.dir)) * M_PI;
            diffuse_temp = scale_color(params->id, intensity);

            add_pv(&light_ray.dir, &ray->inv_dir, &ps);
            normalize_pv(&ps, &ps);

            intensity = dot_product(&ps, normal);
            intensity = MAX(0.0, intensity);
            intensity = pow(intensity, gparams->pc);

            // Calculate specular light
            specular_temp = scale_color(params->is, intensity);

            indirect_diffuse = add_color(indirect_diffuse, diffuse_temp);
            indirect_specular = add_color(indirect_specular, specular_temp);
            indirect_ambient = add_color(indirect_ambient, ambient_temp);
        }

        indirect_diffuse = scale_color(indirect_diffuse, (1.0f/scene->area_light_n));
        indirect_specular = scale_color(indirect_specular, (1.0f/scene->area_light_n));
        indirect_ambient = scale_color(indirect_ambient, (1.0f/scene->area_light_n));
    
        diffuse_color = add_color(diffuse_color, indirect_diffuse);
        specular_color = add_color(specular_color, indirect_specular);
        // lights.ambient_light = add_color(lights.ambient_light, indirect_ambient);

        indirect_diffuse = COLOR(0.0f,0.0f,0.0f);
        indirect_specular = COLOR(0.0f,0.0f,0.0f);
        indirect_ambient = COLOR(0.0f,0.0f,0.0f);
    }

    // Apply defuse coefficient to light
    diffuse_color = scale_color(diffuse_color, gparams->kd);

    // Apply specular coefficient to light
    specular_color = scale_color(specular_color, gparams->ks);

    return (struct lights_t) {.diffuse_light = diffuse_color, .specular_light = specular_color, .ambient_light = scale_color(ambient_color, gparams->ka)};
}

static struct color_t
shade(struct ray_t *ray, float t, struct scene_t *scene, struct gobject_t *object, struct camera_t *camera, struct hit_info_t *info, struct gparams_t *gparams)
{
    struct lights_t lights;
    struct color_t color = {0.0};
    struct color_t diffuse_color = {0.0}, specular_color = {0.0}, ambient_color = {0.0};

    if(gparams->emits)
        return add_color(gparams->is, gparams->id);
    else
        lights = compute_lights(ray, t, scene, object, camera, info, gparams);

    diffuse_color = multiply_color(lights.diffuse_light, gparams->dc);
    specular_color = multiply_color(lights.specular_light, gparams->sc);
    ambient_color = multiply_color(lights.ambient_light, gparams->ac);
    
    color = add_color(diffuse_color, color);
    color = add_color(color, specular_color);
    color = add_color(color, ambient_color);

    return color;
}

static float
raytrace(struct ray_t *ray, struct scene_t *scene, struct camera_t *camera, struct color_t *color, struct hit_info_t *ext_info)
{
    float t = 0.0, ct = 0.0;
    size_t i = 0, s = 0;
    struct hit_info_t info = {0}, b_info = {0};
    struct gobject_t *intersecting = NULL;
    struct gparams_t *param;

    t = INFINITY;

    // Do all the primitives 
    // inf("objects seen: %ld", scene->objects_count);
    for(i = 0; i < scene->objects_count; i++)
    {
        ct = ray_intersect(ray, &scene->objects[i], &b_info);
        /* inf("[%ld] t = %f, ct = %f", i, t, ct); */
        if(ct > 0.0 && ct < t)
        {
            info = b_info;
            t = ct;
            intersecting = &scene->objects[i];
        }
    }

    // Now do all the meshes
    for(i = 0; i < scene->mesh_count; i++)
    {
        for(s = 0; s < scene->meshes[i]->triangle_count; s++)
        {
            ct = ray_intersect(ray, &scene->meshes[i]->triangles[s], &b_info);

            if(ct > 0.0 && ct < t)
            {
                info = b_info;
                t = ct;
                intersecting = &scene->meshes[i]->triangles[s];
            }
        }
    }

    ray->object = intersecting;
    // If color = NULL, just return the distnace
    if(color != NULL)
    {
        // If you hit anything...
        if(intersecting != NULL)
        {
            // Check to see if that color has an object
            if(intersecting->param == NULL)
            {
                param = &DEFAULT_OBJECT_PARAMS;
            }
            else
                param = (struct gparams_t *)intersecting->param;

            // Apply light to color
            
            if(scene->global_illumination && ray->primary_ray && ray->depth < scene->max_depth)
            {
                struct ray_t child_ray;
                struct pv_t direction;
                float n_dor_dir = 0.0;
                struct color_t child_lights = {0.0};
                struct hit_info_t child_info = {0};
                
                generate_random_direction(&info.normal, &direction);
                make_ray(&info.hit_point, &direction, &child_ray);
                child_ray.depth = ray->depth + 1;
                child_ray.primary_ray = true;
                n_dor_dir = dot_product(&info.normal, &direction);

                raytrace(&child_ray, scene, camera, &child_lights, &child_info);
                child_lights = scale_color(child_lights, n_dor_dir);

                *color = shade(ray, t, scene, intersecting, camera, &info, param);
                // child_lights = scale_color(child_lights, 1.0f / (2.0f * (float)M_PI));
                *color = add_color(*color, child_lights);
            }
            else
            {
                *color = shade(ray, t, scene, intersecting, camera, &info, param);
                *color = clamp_color(*color);
            }
        }
        else 
            *color = camera->background_color;
    }

    if(ext_info != NULL)
        *ext_info = info;
    
    return t;
}

void
raytracer_render(struct scene_t *scene, struct camera_t *camera,  struct framebuffer_t *fb)
{
    size_t aa = 0;
    struct ray_t ray;
    struct color_t color_buffer;
    struct pv_t orig = {0.0}, dir = {0.0};
    float ratio = 0.0, scale = 0.0, aa_scale = 0.0;
    struct raytracer_opts_t *opts = NULL;
    
    ratio =1.0/( (float)fb->width/(float)fb->width);

    // Get parameters from camera
    if(camera->opts == NULL)
        opts = &DEFAULT_OPTS;
    else
        opts = (struct raytracer_opts_t *)camera->opts;

    scale = tan(deg2rad(opts->fov/2.0));
    transform_pv(camera->inv_transform, &orig, &orig);

    if(scene->samples < 1)
        scene->samples = 1;

    // This macro will create a ray object with an origin at (0,0)
    // and a direction pointing to the pixel on screen specified by
    // xc and yc. The end result will be saved on 'ray'
#define FORM_RAY(xc, yc)                                                        \
    orig = PV(0.0, 0.0, 0.0);                                                   \
    dir.x = ((2.0 * ((xc) +0.5)/(float)fb->width) - 1.0)*scale;                 \
    dir.y = (1.0 - 2.0 * ((float)(yc) + 0.5)/(float)fb->height)*scale*ratio;    \
    dir.z = 1.0;                                                                \
    dir.w = 0.0;                                                                \
    transform_pv(camera->transform, &dir, &dir);                                \
    transform_pv(camera->transform, &orig, &orig);                              \
    normalize_pv(&dir, &dir);                                                   \
                                                                                \
    make_ray(&orig, &dir, &ray);                                                \
    ray.primary_ray = true;                                                     \
    ray.depth = opts->depth;

#define LOG_RAYS_PRINT()                                                        \
    rc++;                                                                       \
    printf("\r[%lu out of %lu] (%f%%)", rc, max_rc,                             \
            ((float)rc/(float)max_rc)*100.0);

#ifdef LOG_RAYS
    size_t rc = 0;
    const size_t max_rc = fb->height * fb->width;
    inf("Rays shot: ");
#endif

    aa = opts->aa;
    
    if(aa > 0)
    {
#ifdef LOG_RAYS
    const size_t max_rc = fb->height * fb->width * opts->aa;
#endif

        aa_scale = 1.0/aa;
        for(size_t y = 0; y < fb->height; y++)
        {
            for(size_t x = 0; x < fb->width; x++)
            {
                color_buffer = (struct color_t){0.0};
                for(size_t a = 0; a < aa; a++)
                {
                    FORM_RAY(x + randf(), y + randf())
                    struct color_t global_ilumn_buffer = {0.0};
                    for(size_t s = 0; s < scene->samples; s++)
                    {
                        struct color_t c;
                        raytrace(&ray, scene, camera, &c, NULL);
                        global_ilumn_buffer = add_color(global_ilumn_buffer, c);
                    }
                    global_ilumn_buffer = scale_color(global_ilumn_buffer, 1/scene->samples);
                    
                    color_buffer = add_color(global_ilumn_buffer, color_buffer);

#ifdef LOG_RAYS
                LOG_RAYS_PRINT()
#endif
                }
                
                color_buffer = scale_color(color_buffer, aa_scale);
                fb->pixels[y * fb->width + x] = color_to_pixel(color_buffer); 
            }
        }
    }

    // With no Anti-Aliasing
    else
    {
        for(size_t y = 0; y < fb->height; y++)
        {
            for(size_t x = 0; x < fb->width; x++)
            {
                FORM_RAY(x, y)
                struct color_t global_ilumn_buffer = {0.0};
                for(size_t s = 0; s < scene->samples; s++)
                {
                    struct color_t c;
                    raytrace(&ray, scene, camera, &c, NULL);
                    global_ilumn_buffer = add_color(global_ilumn_buffer, c);
                }
                global_ilumn_buffer = scale_color(global_ilumn_buffer, 1/scene->samples);
                fb->pixels[y * fb->width + x] = color_to_pixel(global_ilumn_buffer); 

#ifdef LOG_RAYS
                LOG_RAYS_PRINT()
#endif
            }
        }
    }

    // Just so the next log won't overlap the ray count
#ifdef LOG_RAYS
    printf("\n");
#endif


    // Gotta clean after myself!
#undef LOG_RAYS_PRINT
#undef LOG_RAYS
}
