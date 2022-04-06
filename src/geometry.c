#include <stdio.h>

#include "utilities.h"
#include "geometry.h"

#include "fmath.h"

// Debug functions I use to display the contents of
// some structures. No need to pay much attention to
// these tho
const char*
matrix_to_str(matrix_t m)
{
    static char buff[1024] = {0};

    snprintf(buff, 1024, 
        "[%f]\t[%f]\t[%f]\t[%f]\n"
        "[%f]\t[%f]\t[%f]\t[%f]\n"
        "[%f]\t[%f]\t[%f]\t[%f]\n"
        "[%f]\t[%f]\t[%f]\t[%f]",
        m[0][0], m[0][1], m[0][2], m[0][3], 
        m[1][0], m[1][1], m[1][2], m[1][3], 
        m[2][0], m[2][1], m[2][2], m[2][3], 
        m[3][0], m[3][1], m[3][2], m[3][3]
    );

    return buff;
}

const char*
column_to_str(column_t c)
{
    static char buff[1024] = {0};
    
    snprintf(buff, 1024,
        "c[0][0] = %f, c[1][0] = %f, c[2][0] = %f, c[3][0] = %f",
        c[0][0],c[1][0],c[2][0],c[3][0]
    );

    return buff;
}

const char*
pv_to_str(struct pv_t *p)
{
    static char buff[1024] = {0};
    
    snprintf(buff, 1024,
        "p->x = %f, p->y = %f, p->z = %f, p->w = %f", 
        p->x, p->y, p->z, p->w
    );

    return buff;
}

// Copy the contents of matrix d to s
void
copy_matrix(matrix_t s, matrix_t d)
{
    int r = 0;

    for(r = 0; r < 4; r++)
    {
        d[r][0] = s[r][0];
        d[r][1] = s[r][1];
        d[r][2] = s[r][2];
        d[r][3] = s[r][3];
    }
}

// Sets all the contents of a matrix to 0.0
void
clear_matrix(matrix_t m)
{
    matrix_t b = {{0.0}};
    copy_matrix(b, m);
}

// r = a - b
void
substract_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    r->w = 0.0;
}

// Returns a + b
void
add_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    r->w = 0.0;
}

// r = p * s 
void
scale_pv(struct pv_t * p, float s, struct pv_t *r)
{
    r->x = p->x * s;
    r->y = p->y * s;
    r->z = p->z * s;
}

// r = p / s
void
divide_pv(struct pv_t *p, float s, struct pv_t *r)
{
    scale_pv(p, 1.0/s, r);
}

// r = a * b
void
multiply_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r)
{
    r->x = a->x * b->x;
    r->y = a->y * b->y;
    r->z = a->z * b->z;
}

// Returns ||v||
float
magnitude_pv(struct pv_t *v)
{
    return sqrtf(pow2((v)->x) + pow2((v)->y) + pow2((v)->z));
}

// r = v/(||v||)
void
normalize_pv(struct pv_t *v, struct pv_t *r)
{
    float m = 0.0;

    m = magnitude_pv(v);
   
    r->x = v->x/m;    
    r->y = v->y/m;    
    r->z = v->z/m;    
}

// r = 1/p
// Returns the inverse of p.
// I had to get a little experimental with this, so I hope
// it works. Essentially this will check if any of the 
// components on the vector is equal to 0, and if they are,
// the resulting value on r will be equal to INFINITY
void inverse_pv(struct pv_t* p, struct pv_t *r)
{
#define csign(x) (((x) >= 0) - ((x) < 0))
    r->x = (p->x == 0 ? csign(p->x) * INFINITY : 1.0/p->x);
    r->y = (p->y == 0 ? csign(p->y) * INFINITY : 1.0/p->y);
    r->z = (p->z == 0 ? csign(p->z) * INFINITY : 1.0/p->z);
#undef csgin
}

float
dot_product(struct pv_t *a, struct pv_t *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void
cross_pv(struct pv_t *a, struct pv_t *b, struct pv_t *r)
{
    r->x = a->y * b->z - a->z * b->y;
    r->y = a->z * b->x - a->x * b->z;
    r->z = a->x * b->y - a->y * b->x;
}

// Okay, so everyone seems to 
// Checks if pointvector is a point
// Just in case I need to add other criteria to this
// later
static inline bool 
is_point(struct pv_t *p) 
{
    return p->w > 0;
}

// Converts a pv_t to a column_t
static inline void
pv_to_column(struct pv_t *p, column_t c)
{
    c[0][0] = p->x;
    c[1][0] = p->y;
    c[2][0] = p->z;
    c[3][0] = p->w;   
}

// Converts a column_t to a pv_t
static inline void
column_to_pv(column_t c, struct pv_t *p)
{
    p->x = c[0][0];
    p->y = c[1][0];
    p->z = c[2][0];
    p->w = c[3][0];
}

// Yeah... I don't have time to worry about 
// how to calculate the inverse of a matrix.
// Like, I know how to do this, but I was spending
// too much time writing my own method so f*ck it,
// I asked San Google for some help with this one.
//
// Taken from 
// https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
// (I would have referenced the original source for the function
// but I couldn't find it)
//
// Also, I made this inlined since it's only going to be used
// in one place. So essentially, this would be the same as 
// wiriting this function on inv_matrix, but I wanted to separate
// this from inv_matrix since I didn't write this myself.
inline static bool 
_inv_matrix(const float m[16], float r[16])
{
    float inv[16], det;
    int i;

    inv[0] = 
        m[5]  * m[10] * m[15] - 
        m[5]  * m[11] * m[14] - 
        m[9]  * m[6]  * m[15] + 
        m[9]  * m[7]  * m[14] +
        m[13] * m[6]  * m[11] - 
        m[13] * m[7]  * m[10];

    inv[4] = 
        -m[4]  * m[10] * m[15] + 
        m[4]  * m[11] * m[14] + 
        m[8]  * m[6]  * m[15] - 
        m[8]  * m[7]  * m[14] - 
        m[12] * m[6]  * m[11] + 
        m[12] * m[7]  * m[10];

    inv[8] = 
        m[4]  * m[9] * m[15] - 
        m[4]  * m[11] * m[13] - 
        m[8]  * m[5] * m[15] + 
        m[8]  * m[7] * m[13] + 
        m[12] * m[5] * m[11] - 
        m[12] * m[7] * m[9];

    inv[12] = 
        -m[4]  * m[9] * m[14] + 
        m[4]  * m[10] * m[13] +
        m[8]  * m[5] * m[14] - 
        m[8]  * m[6] * m[13] - 
        m[12] * m[5] * m[10] + 
        m[12] * m[6] * m[9];

    inv[1] =
        -m[1]  * m[10] * m[15] + 
        m[1]  * m[11] * m[14] + 
        m[9]  * m[2] * m[15] - 
        m[9]  * m[3] * m[14] - 
        m[13] * m[2] * m[11] + 
        m[13] * m[3] * m[10];

    inv[5] = 
        m[0]  * m[10] * m[15] - 
        m[0]  * m[11] * m[14] - 
        m[8]  * m[2] * m[15] + 
        m[8]  * m[3] * m[14] + 
        m[12] * m[2] * m[11] - 
        m[12] * m[3] * m[10];

    inv[9] = 
        -m[0]  * m[9] * m[15] + 
        m[0]  * m[11] * m[13] + 
        m[8]  * m[1] * m[15] - 
        m[8]  * m[3] * m[13] - 
        m[12] * m[1] * m[11] + 
        m[12] * m[3] * m[9];

    inv[13] = 
        m[0]  * m[9] * m[14] - 
        m[0]  * m[10] * m[13] - 
        m[8]  * m[1] * m[14] + 
        m[8]  * m[2] * m[13] + 
        m[12] * m[1] * m[10] - 
        m[12] * m[2] * m[9];

    inv[2] = 
        m[1]  * m[6] * m[15] - 
        m[1]  * m[7] * m[14] - 
        m[5]  * m[2] * m[15] + 
        m[5]  * m[3] * m[14] + 
        m[13] * m[2] * m[7] - 
        m[13] * m[3] * m[6];

    inv[6] = 
        -m[0]  * m[6] * m[15] + 
        m[0]  * m[7] * m[14] + 
        m[4]  * m[2] * m[15] - 
        m[4]  * m[3] * m[14] - 
        m[12] * m[2] * m[7] + 
        m[12] * m[3] * m[6];

    inv[10] = 
        m[0]  * m[5] * m[15] - 
        m[0]  * m[7] * m[13] - 
        m[4]  * m[1] * m[15] + 
        m[4]  * m[3] * m[13] + 
        m[12] * m[1] * m[7] - 
        m[12] * m[3] * m[5];

    inv[14] = 
        -m[0]  * m[5] * m[14] + 
        m[0]  * m[6] * m[13] + 
        m[4]  * m[1] * m[14] - 
        m[4]  * m[2] * m[13] - 
        m[12] * m[1] * m[6] + 
        m[12] * m[2] * m[5];

    inv[3] = 
        -m[1] * m[6] * m[11] + 
        m[1] * m[7] * m[10] + 
        m[5] * m[2] * m[11] - 
        m[5] * m[3] * m[10] - 
        m[9] * m[2] * m[7] + 
        m[9] * m[3] * m[6];

    inv[7] = 
        m[0] * m[6] * m[11] - 
        m[0] * m[7] * m[10] - 
        m[4] * m[2] * m[11] + 
        m[4] * m[3] * m[10] + 
        m[8] * m[2] * m[7] - 
        m[8] * m[3] * m[6];

    inv[11] = 
        -m[0] * m[5] * m[11] + 
        m[0] * m[7] * m[9] + 
        m[4] * m[1] * m[11] - 
        m[4] * m[3] * m[9] - 
        m[8] * m[1] * m[7] + 
        m[8] * m[3] * m[5];

    inv[15] = 
        m[0] * m[5] * m[10] - 
        m[0] * m[6] * m[9] - 
        m[4] * m[1] * m[10] + 
        m[4] * m[2] * m[9] + 
        m[8] * m[1] * m[6] - 
        m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        r[i] = inv[i] * det;

    return true;
}

// r = inv(m)
// If m doesn't have an inverse, then inv_matrix
// will act just like copy_matrix
void
inv_matrix(matrix_t m, matrix_t r)
{
    bool s;

    s = _inv_matrix((const float *)m, (float *)r);
    if(s)
        return;
    else
        copy_matrix(m, r);
}

// r = m * x
// Matrix times column operation. Do not use
// c as r as well. This will cause problems
void
mxc(matrix_t m, column_t c, column_t r)
{
    r[0][0] = m[0][0] * c[0][0] + m[0][1] * c[1][0] + m[0][2] * c[2][0] + m[0][3] * c[3][0];
    r[1][0] = m[1][0] * c[0][0] + m[1][1] * c[1][0] + m[1][2] * c[2][0] + m[1][3] * c[3][0];
    r[2][0] = m[2][0] * c[0][0] + m[2][1] * c[1][0] + m[2][2] * c[2][0] + m[2][3] * c[3][0];
    r[3][0] = m[3][0] * c[0][0] + m[3][1] * c[1][0] + m[3][2] * c[2][0] + m[3][3] * c[3][0];
}

// r = m * c
// Matrix times column operation. Do not use
// c as r as well. This will cause problems
void
mxm(matrix_t m, matrix_t c, matrix_t r)
{
    unsigned char i = 0;

    // I know I didn't NEED to use a loop here, but this is due in
    // a week
    for(i = 0; i < 4; i++)
    {
        r[0][i] = m[0][0] * c[0][i] + m[0][1] * c[1][i] + m[0][2] * c[2][i] + m[0][3] * c[3][i];
        r[1][i] = m[1][0] * c[0][i] + m[1][1] * c[1][i] + m[1][2] * c[2][i] + m[1][3] * c[3][i];
        r[2][i] = m[2][0] * c[0][i] + m[2][1] * c[1][i] + m[2][2] * c[2][i] + m[2][3] * c[3][i];
        r[3][i] = m[3][0] * c[0][i] + m[3][1] * c[1][i] + m[3][2] * c[2][i] + m[3][3] * c[3][i];
    }
}

// m = Im
void
make_identity_matrix(matrix_t m)
{
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;

    m[0][1] = m[0][2] = m[0][3] = 
    m[1][0] = m[1][2] = m[1][3] = 
    m[2][0] = m[2][1] = m[2][3] = 
    m[3][0] = m[3][1] = m[3][2] = 0.0; 
}

// m = T(x, y, z)
void
make_translation_matrix(float x, float y, float z, matrix_t m)
{
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;

    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;

    m[0][1] = 0.0;
    m[0][2] = 0.0;

    m[1][0] = 0.0;
    m[1][2] = 0.0;

    m[2][0] = 0.0;
    m[2][1] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
}

// m = S(x, y, z)
void
make_scaling_matrix(float x, float y, float z, matrix_t m)
{
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
    m[3][3] = 1.0;

    m[0][1] = 0.0;
    m[0][2] = 0.0;
    m[0][3] = 0.0;

    m[1][0] = 0.0;
    m[1][2] = 0.0;
    m[1][3] = 0.0;

    m[2][0] = 0.0;
    m[2][1] = 0.0;
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
}

// m = R(t, u)
// t => Thetha: Angle for the rotation
// u => Unit Vector: Normal vector indicating the rotation axis(es)
void
make_rotation_matrix(float t, struct pv_t *u, matrix_t m)
{
    float x2 = .0, y2 = .0, z2 = .0, sint = .0, cost = .0, d = .0;

    x2 = pow2(u->x);
    y2 = pow2(u->y);
    z2 = pow2(u->z);
    
    d = deg2rad(t); 
    cost = cos(d);
    sint = sin(d);

    m[0][0] = x2 + cost*(1-x2);
    m[0][1] = u->x * u->y * (1-cost) - u->z*sint;
    m[0][2] = u->x * u->z * (1-cost) + u->y*sint;
    m[0][3] = 0.0;

    m[1][0] = u->x * u->y * (1-cost) + u->z*sint;
    m[1][1] = y2 + cost*(1-y2);
    m[1][2] = u->y * u->z * (1-cost) - u->x*sint;
    m[1][3] = 0.0;

    m[2][0] = u->x * u->z * (1-cost) - u->y*sint;
    m[2][1] = u->y * u->z * (1-cost) + u->x*sint;
    m[2][2] = z2 + cost*(1-z2);
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
}

// Apply transform matrix m to pv
void
transform_pv(matrix_t m, struct pv_t *p, struct pv_t *r)
{
    column_t a, c;
    
    pv_to_column(p, c);
    mxc(m, c, a);
    column_to_pv(a, r);
}

// Multiply m by all the pv_t on src and save
// the results on dst
void
transform_pv_arr(matrix_t m, struct pv_t *src, struct pv_t *dst, size_t s)
{
    column_t a, c;

    while(s > 0)
    {
        s--;
        pv_to_column(&src[s], c);
        mxc(m, c, a);
        column_to_pv(a, &dst[s]);
    }
}

// Reflects i around normal n
void
reflect_pv(struct pv_t *i, struct pv_t *n, struct pv_t *r)
{
    struct pv_t p = {0.0};

    scale_pv(n, dot_product(i, n) * 2.0, &p);
    substract_pv(i, &p, r);
}

void
make_sphere(struct pv_t *center, float radius, struct sphere_t *s)
{
    s->radius = radius;
    s->radius2 = pow2(radius);
    s->center = *center;

    s->type = GEOMETRY_SPHERE;

    s->param = NULL;
}

void
make_plane(struct pv_t *center, struct pv_t *normal, struct plane_t * plane)
{
    plane->center = *center;
    plane->normal = *normal;

    plane->type = GEOMETRY_PLANE;
}

void 
make_triangle(struct pv_t *a, struct pv_t *b, struct pv_t *c, struct triangle_t *t)
{
    // I know I could have used ac instead... but
    // I also want to be able to understand this in
    // the future
    struct pv_t ca, cp;

    t->edges[0] = *a;
    t->edges[1] = *b;
    t->edges[2] = *c;

    // Use for intersection check
    substract_pv(b, a, &t->ba);
    substract_pv(c,b, &t->cb);
    substract_pv(a,c, &t->ac);

    substract_pv(c, a, &ca);

    cross_pv(&t->ba, &ca, &cp);
    normalize_pv(&cp, &t->normal);
    t->area = dot_product(&cp, &t->normal);

    t->type = GEOMETRY_TRIANGLE;
}

void
transform_triangle(struct triangle_t *t, matrix_t m, struct triangle_t *r)
{
    transform_pv(m, &t->edges[0], &t->edges[0]);
    transform_pv(m, &t->edges[1], &t->edges[1]);
    transform_pv(m, &t->edges[2], &t->edges[2]);

    make_triangle(&t->edges[0], &t->edges[1], &t->edges[2], r);
}

void 
new_mesh(struct mesh_t *m, size_t c)
{
    matrix_t i;

    m->triangles = (struct triangle_t *)calloc(c, sizeof(struct triangle_t));
    m->triangle_count = c;

    make_identity_matrix(i);
    copy_matrix(i, m->transform);
    copy_matrix(i, m->inv_transform);
}

void 
free_mesh(struct mesh_t *m)
{
    if(m->triangles != NULL)
        free(m->triangles);
        
    m->triangles = NULL;
    m->triangle_count = 0;
}

void 
transform_mesh(struct mesh_t *m, matrix_t t, struct mesh_t *r)
{
    size_t i = 0;
    matrix_t b;

    for(; i < m->triangle_count; i++)
        transform_triangle(&m->triangles[i], t, &r->triangles[i]);

    mxm(t, m->transform, b);
    inv_matrix(b, r->inv_transform);
    copy_matrix(b, r->transform);
}

void transform_object(struct gobject_t *g, matrix_t t, struct gobject_t *r)
{
    switch (g->type)
    {
    case GEOMETRY_SPHERE:
        transform_pv(t, &g->center, &r->center);
        break;
    case GEOMETRY_TRIANGLE:
        transform_triangle(g, t, r);
    default:
        break;
    }
}

void
new_rectangle_mesh(struct pv_t *a, struct pv_t *b, struct pv_t *c, struct pv_t *d, struct mesh_t *m)
{
    float radius = 0.0;
    struct pv_t p = {0};

    new_mesh(m, 2);
    make_triangle(a, b, c, &m->triangles[0]);
    make_triangle(c, d, a, &m->triangles[1]);
    m->triangles[0].single_sided = m->triangles[1].single_sided = false;

    m->has_bounding_sphere = true;

    // substract_pv(b, a, &p);
    // radius = magnitude_pv(&p)/2;
    // normalize_pv(&p, &p);
    // scale_pv(&p, radius, &p);
    // add_pv(a, &p, &p);
    // make_sphere(&p, radius, &m->bounding_sphere);
}

void 
new_rectangle_mesh_wh(float width, float height, struct mesh_t *m)
{
    struct pv_t a, b, c;

    a = b = c = PV(0.0, 0.0, 0.0);
    new_mesh(m, 2);

    a.x = width;
    c.z = -height;
    make_triangle(&b, &a, &c, &m->triangles[0]);

    b.z = c.z;
    b.x = a.x;
    make_triangle(&a, &b, &c, &m->triangles[1]);

    m->triangles[0].single_sided = m->triangles[1].single_sided = false;
}
