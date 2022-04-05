#include "ppm.h"

int ppm_save(const char * file_name, const ppm_pixel_t pixels[], uint height, uint width)
{
    size_t len = 0;
    FILE *fp = NULL;
    unsigned char* buffer = NULL;

    if(file_name == NULL || pixels == NULL || height < 1 || width < 1)
        return PPM_BAD_PARAMETERS;
    len = height * width;
    buffer = (unsigned char *)calloc(sizeof(unsigned char), len*3 + PPM_HEADER_LEN);

    fp = fopen(file_name, "w+");
    if(fp == NULL)
        return errno;

    fprintf(fp, "P6 %u %u 255\n", width, height);
    for(size_t i = 0; i < len; i++) 
    {
        size_t s = i*3;
        buffer[s]       = (uint8_t)((pixels[i] & 0x00ff0000) >> 16);
        buffer[s + 1]   = (uint8_t)((pixels[i] & 0x0000ff00) >> 8);
        buffer[s + 2]   = (uint8_t)((pixels[i] & 0x000000ff));
    }   
    fwrite(buffer, len*3, sizeof(*buffer), fp);
    fclose(fp);
    free(buffer);

    return 0;
}
