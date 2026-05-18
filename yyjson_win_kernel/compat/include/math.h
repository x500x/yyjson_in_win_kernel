#ifndef YYJSONK_MATH_H
#define YYJSONK_MATH_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static __inline double yyjsonk_math_inf(void)
{
    unsigned long long bits = 0x7ff0000000000000ull;
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static __inline double yyjsonk_math_nan(void)
{
    unsigned long long bits = 0x7ff8000000000000ull;
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

#ifndef HUGE_VAL
#define HUGE_VAL (yyjsonk_math_inf())
#endif

#ifndef INFINITY
#define INFINITY (yyjsonk_math_inf())
#endif

#ifndef NAN
#define NAN (yyjsonk_math_nan())
#endif

#ifdef __cplusplus
}
#endif

#endif
