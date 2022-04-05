#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "utilities.h"

#define pow2(x)                             (x*x)
#define randf()                             ((float)rand()/(float)(RAND_MAX + 1.0))
#define rrandf(min, max)                    (min + (max - min)*randf())

bool solve_quadratic(float a, float b, float c, float *x1, float *x2);
