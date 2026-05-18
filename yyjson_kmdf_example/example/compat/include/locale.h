#ifndef YYJSONK_LOCALE_H
#define YYJSONK_LOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LC_ALL
#define LC_ALL 0
#endif

struct lconv {
    char *decimal_point;
};

char *__cdecl setlocale(int category, const char *locale);
struct lconv *__cdecl localeconv(void);

#ifdef __cplusplus
}
#endif

#endif
