#ifndef WAVEFRONT_H__
#define WAVEFRONT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "geometry.h"
#include "utilities.h"

struct wface_t 
{
    size_t v[3];
};
struct wvertex_t
{
    float x, y, z;
};
DYNAMIC_ARRAY(wface_list_t, struct wface_t)
DYNAMIC_ARRAY(wvertex_list_t, struct wvertex_t)

struct wavefront_t
{
    wface_list_t faces;
    wvertex_list_t vertices;
};

void wavefront_init(struct wavefront_t *wf);
int wavefront_parse_file(const char * file_path, struct wavefront_t *wf);
void wavefront_destroy(struct wavefront_t * wf);

#endif
