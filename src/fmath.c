#include "fmath.h"

bool
solve_quadratic(float a, float b, float c, float *x1, float *x2)
{
    float d = 0.0, q = 0.0;

    d = pow2(b) - 4.0 * a * c;

    // No imaginary numbers pls
    if(d < 0)
        return false;

    // Easy
    else if(d == 0.0)
        *x1 = *x2 = (-0.5 * b/a);

    // Frick
    else
    {
        q = b> 0.0 ? -0.5 * (b + sqrt(d)) : -0.5 * (b - sqrt(d));
        *x1 = q/a;
        *x2 = c/q;
    }
    if(*x1 > *x2)
        SWAP(*x1, *x2, float);

    return true;
}

