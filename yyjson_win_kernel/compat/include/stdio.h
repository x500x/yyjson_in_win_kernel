#ifndef YYJSONK_STDIO_H
#define YYJSONK_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int errno_t;
typedef struct yyjsonk_FILE FILE;

#ifndef EOF
#define EOF (-1)
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

extern FILE *stdout;
extern FILE *stderr;

FILE *__cdecl fopen(const char *path, const char *mode);
errno_t __cdecl fopen_s(FILE **file, const char *path, const char *mode);
size_t __cdecl fread(void *buf, size_t size, size_t count, FILE *file);
size_t __cdecl fread_s(void *buf, size_t buf_size, size_t elem_size, size_t count, FILE *file);
size_t __cdecl fwrite(const void *buf, size_t size, size_t count, FILE *file);
int __cdecl fclose(FILE *file);
int __cdecl fseek(FILE *file, long offset, int origin);
long __cdecl ftell(FILE *file);
int __cdecl fflush(FILE *file);
int __cdecl printf(const char *fmt, ...);
int __cdecl fprintf(FILE *file, const char *fmt, ...);
int __cdecl sprintf(char *buf, const char *fmt, ...);
int __cdecl snprintf(char *buf, size_t len, const char *fmt, ...);
int __cdecl vsnprintf(char *buf, size_t len, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif
