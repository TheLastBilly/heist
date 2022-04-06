#ifndef GEOMETRY_H__
#define GEOMETRY_H__

#include <math.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef M_PI
#define	M_PI		                ((double)3.14159265358979323846)
#endif

// // Returns p^2
// // Because p*p takes too long to write
// #define pow2(p)                     (p)*(p)

// Converts angle from degrees to radiants
#define deg2rad(t)                  ((t * M_PI)/180.0)

#define PV(a, b, c)                 (struct pv_t){.x = a, .y = b, .z = c, .w = 1.0}
#define SPHERE(v, r)                (struct gobject_t){.type = GEOMETRY_SPHERE, .center = v, .radius = r, .radius2 = pow2(r)}
#define DISK(c, n, r)               (struct gobject_t){.type = GEOMETRY_DISK, .center = c, .normal = n, .radius = r, .radius2 = pow2(r)}
#define PLANE(c, n)                 (struct gobject_t){.type = GEOMETRY_PLANE, .center = c, .normal = n}
#define TRIANGLE(a, b, c, n)        (struct gobject_t){.type = GEOMETRY_TRIANGLE, .edges[0] = a, .edges[1] = b, .edges[2] = c, .normal = n}

#define MIN(a,b)                    (((a)<(b))?(a):(b))
#define MAX(a,b)                    (((a)>(b))?(a):(b))

typedef uint32_t pixel_t;

typedef float matrix_t[4][4];
typedef float column_t[4][1];

struct pv_t
{ float x,y,z,w; };

enum gobject_type_t
{
    GEOMETRY_TRIANGLE,
    GEOMETRY_SPHERE,
    GEOMETRY_PLANE,
    GEOMETRY_DISK,
};

struct gobject_t
{
    enum gobject_type_t type;
    struct pv_t center;

    bool emitter;

    struct pv_t normal;
    union
    {
        struct
        {
            float radius;
            float radius2;
        };

        struct
        {
            struct pv_t edges[3], ba, cb, ac;
            bool single_sided;
            float area;
        };
    };

    void *param;
};
#define triangle_t gobject_t
#define sphere_t gobject_t
#define plane_t gobject_t

struct mesh_t
{
    struct triangle_t *triangles;
    size_t triangle_count;
    matrix_t transform, inv_transform;

    struct gobject_t bounding_sphere;
    bool has_bounding_sphere;
};

const char* matrix_to_str(matrix_t m);
const char* column_to_str(column_t c);
const char* pv_to_str(struct pv_t *p);

void copy_matrix(matrix_t s, matrix_t d);
void clear_matrix(matrix_t m);

float magnitude_pv(struct pv_t *v);

void reflect_pv(struct pv_t *i, struct pv_t *n, struct pv_t *r);
void substract_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r);
void add_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r);
void scale_pv(struct pv_t * p, float s, struct pv_t *r);
void divide_pv(struct pv_t *p, float s, struct pv_t *r);
void multiply_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r);
void normalize_pv(struct pv_t *v, struct pv_t *r);
void inverse_pv(struct pv_t* p, struct pv_t *r);
void cross_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r);
float dot_product(struct pv_t *a, struct pv_t *b);

void inv_matrix(matrix_t m, matrix_t r);
void mxc(matrix_t m, column_t c, column_t r);
void mxm(matrix_t m, matrix_t c, matrix_t r);

void make_identity_matrix(matrix_t m);
void make_translation_matrix(float x, float y, float z, matrix_t m);
void make_scaling_matrix(float x, float y, float z, matrix_t m);
void make_rotation_matrix(float t, struct pv_t *u, matrix_t m);

void transform_pv(matrix_t m, struct pv_t *p, struct pv_t *r);
void transform_pv_arr(matrix_t m, struct pv_t *src, struct pv_t *dst, size_t s);

void make_triangle(struct pv_t *a, struct pv_t *b, struct pv_t *c, struct triangle_t *t);
void transform_triangle(struct triangle_t *t, matrix_t m, struct triangle_t *r);

void make_sphere(struct pv_t *center, float radius, struct sphere_t *s);

void make_plane(struct pv_t *center, struct pv_t *normal, struct plane_t * plane);

void new_mesh(struct mesh_t *m, size_t c);
void free_mesh(struct mesh_t *m);

void transform_object(struct gobject_t *g, matrix_t t, struct gobject_t *r);
void transform_mesh(struct mesh_t *m, matrix_t t, struct mesh_t *r);

void new_rectangle_mesh(struct pv_t *a, struct pv_t *b, struct pv_t *c, struct pv_t *d, struct mesh_t *m);
void new_rectangle_mesh_wh(float width, float height, struct mesh_t *m);

#endif
