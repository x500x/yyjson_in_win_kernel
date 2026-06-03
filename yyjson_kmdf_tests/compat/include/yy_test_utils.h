/*==============================================================================
 * Kernel-compatible utilities for yyjson upstream tests.
 *============================================================================*/

#ifndef yy_test_utils_h
#define yy_test_utils_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#include "yyjsonk_test_support.h"

/* warning suppress for tests */
#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wunused-function"
#   pragma clang diagnostic ignored "-Wunused-parameter"
#   pragma clang diagnostic ignored "-Wunused-variable"
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#   pragma GCC diagnostic ignored "-Wunused-parameter"
#   pragma GCC diagnostic ignored "-Wunused-variable"
#elif defined(_MSC_VER)
#   pragma warning(disable:4100)
#   pragma warning(disable:4101)
#endif

#ifndef yy_has_builtin
#   ifdef __has_builtin
#       define yy_has_builtin(x) __has_builtin(x)
#   else
#       define yy_has_builtin(x) 0
#   endif
#endif

#ifndef yy_has_attribute
#   ifdef __has_attribute
#       define yy_has_attribute(x) __has_attribute(x)
#   else
#       define yy_has_attribute(x) 0
#   endif
#endif

#ifndef yy_has_include
#   ifdef __has_include
#       define yy_has_include(x) __has_include(x)
#   else
#       define yy_has_include(x) 0
#   endif
#endif

#ifndef yy_inline
#   if _MSC_VER >= 1200
#       define yy_inline __forceinline
#   elif defined(_MSC_VER)
#       define yy_inline __inline
#   elif yy_has_attribute(always_inline) || __GNUC__ >= 4
#       define yy_inline __inline__ __attribute__((always_inline))
#   elif defined(__clang__) || defined(__GNUC__)
#       define yy_inline __inline__
#   elif defined(__cplusplus) || (__STDC__ >= 1 && __STDC_VERSION__ >= 199901L)
#       define yy_inline inline
#   else
#       define yy_inline
#   endif
#endif

#ifndef yy_noinline
#   if _MSC_VER >= 1200
#       define yy_noinline __declspec(noinline)
#   elif yy_has_attribute(noinline) || __GNUC__ >= 4
#       define yy_noinline __attribute__((noinline))
#   else
#       define yy_noinline
#   endif
#endif

#ifndef yy_likely
#   if yy_has_builtin(__builtin_expect) || __GNUC__ >= 4
#       define yy_likely(expr) __builtin_expect(!!(expr), 1)
#   else
#       define yy_likely(expr) (expr)
#   endif
#endif

#ifndef yy_unlikely
#   if yy_has_builtin(__builtin_expect) || __GNUC__ >= 4
#       define yy_unlikely(expr) __builtin_expect(!!(expr), 0)
#   else
#       define yy_unlikely(expr) (expr)
#   endif
#endif

#define yy_assert(expr) do { \
    if (!(expr)) { \
        yyjsonk_fail_assert(#expr, __FILE__, __LINE__, NULL); \
    } \
} while(false)

#define yy_assertf(expr, ...) do { \
    if (!(expr)) { \
        yyjsonk_fail_assert(#expr, __FILE__, __LINE__, __VA_ARGS__); \
    } \
} while(false)

#if yy_has_include("yy_xctest.h")
#   include "yy_xctest.h"
#   define yy_test_case(name) \
        void name(void)
#else
#   define yy_test_case(name) \
    void name(void); \
    int main(int argc, const char * argv[]) { \
        (void)argc; \
        (void)argv; \
        name(); \
        return 0; \
    } \
    void name(void)
#endif

#define yy_nelems(x)  (sizeof(x) / sizeof((x)[0]))
#define yy_min(a, b) ((a) < (b) ? (a) : (b))
#define yy_max(a, b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

typedef float       f32;
typedef double      f64;
typedef int8_t      i8;
typedef uint8_t     u8;
typedef int16_t     i16;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;
typedef int64_t     i64;
typedef uint64_t    u64;
typedef size_t      usize;

double __cdecl strtod(const char *nptr, char **endptr);
float __cdecl strtof(const char *nptr, char **endptr);
long long __cdecl strtoll(const char *nptr, char **endptr, int base);
unsigned long long __cdecl strtoull(const char *nptr, char **endptr, int base);

void yy_rand_reset(u64 seed);
u32 yy_rand_u32(void);
u32 yy_rand_u32_uniform(u32 bound);
u32 yy_rand_u32_range(u32 min, u32 max);
u64 yy_rand_u64(void);
u64 yy_rand_u64_uniform(u64 bound);
u64 yy_rand_u64_range(u64 min, u64 max);
f32 yy_rand_f32(void);
f32 yy_rand_f32_range(f32 min, f32 max);
f64 yy_rand_f64(void);
f64 yy_rand_f64_range(f64 min, f64 max);

#define YY_DIR_SEPARATOR '\\'
#define YY_MAX_PATH 4096

#if yy_has_attribute(sentinel)
__attribute__((sentinel))
#endif
bool yy_path_combine(char *buf, const char *path, ...);
bool yy_path_remove_last(char *buf, const char *path);
bool yy_path_get_last(char *buf, const char *path);
bool yy_path_append_ext(char *buf, const char *path, const char *ext);
bool yy_path_remove_ext(char *buf, const char *path);
bool yy_path_get_ext(char *buf, const char *path);
bool yy_path_exist(const char *path);
bool yy_path_is_dir(const char *path);
char **yy_dir_read(const char *path, int *count);
char **yy_dir_read_full(const char *path, int *count);
void yy_dir_free(char **names);

FILE *yy_file_open(const char *path, const char *mode);
bool yy_file_read(const char *path, u8 **dat, usize *len);
bool yy_file_read_with_padding(const char *path, u8 **dat, usize *len, usize padding);
bool yy_file_write(const char *path, u8 *dat, usize len);
bool yy_file_delete(const char *path);

char *yy_str_copy(const char *str);
int yy_str_cmp(const char *str1, const char *str2, bool ignore_case);
bool yy_str_contains(const char *str, const char *search);
bool yy_str_has_prefix(const char *str, const char *prefix);
bool yy_str_has_suffix(const char *str, const char *suffix);
bool yy_str_is_utf8(const char *str, size_t len);

typedef struct yy_buf {
    u8 *cur;
    u8 *hdr;
    u8 *end;
    bool need_free;
} yy_buf;

bool yy_buf_init(yy_buf *buf, usize len);
void yy_buf_release(yy_buf *buf);
usize yy_buf_len(yy_buf *buf);
bool yy_buf_grow(yy_buf *buf, usize len);
bool yy_buf_append(yy_buf *buf, u8 *dat, usize len);

typedef struct yy_buf yy_dat;

bool yy_dat_init_with_file(yy_dat *dat, const char *path);
bool yy_dat_init_with_mem(yy_dat *dat, u8 *mem, usize len);
void yy_dat_release(yy_dat *dat);
void yy_dat_reset(yy_dat *dat);
char *yy_dat_read_line(yy_dat *dat, usize *len);
char *yy_dat_copy_line(yy_dat *dat, usize *len);

double yy_get_time(void);
double yy_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif
