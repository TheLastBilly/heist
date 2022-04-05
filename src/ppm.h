#ifndef PPM_H__
#define PPM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <errno.h>

#ifndef uint
#define uint            unsigned int
#endif

#define PPM_HEADER_LEN  128
#define PPM_RGB(R,G,B)  \
    (uint32_t)(((((uint32_t)(R))&0xff) << 16) | ((((uint32_t)(G)) & 0xff) << 8) | (((uint32_t)(B)) & 0xff))

typedef uint32_t ppm_pixel_t;

enum
{
    PPM_BAD_PARAMETERS,
};
extern int errno;

int
ppm_save(const char * file_name, const ppm_pixel_t pixels[], uint height, uint width);

#endif
