#ifndef YYJSONK_ASSERT_H
#define YYJSONK_ASSERT_H

#include "yyjsonk_runtime.h"

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) do { \
    if (!(expr)) { \
        yyjsonk_fail_assert(#expr, __FILE__, __LINE__, NULL); \
    } \
} while (0)
#endif

#endif
