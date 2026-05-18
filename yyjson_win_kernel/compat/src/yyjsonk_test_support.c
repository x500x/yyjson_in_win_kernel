#include "yy_test_utils.h"
#include "locale.h"
#include "yyjsonk_runtime.h"

#include <stdlib.h>
#include <string.h>

static u64 g_yy_rand_seed = 0;
static char g_yyjsonk_test_root[YYJSONK_MAX_PATH] = YYJSONK_DEFAULT_TEST_ROOT;
static const char *g_yyjsonk_current_test = NULL;
static BOOLEAN g_yyjsonk_test_failed = FALSE;
static char g_yyjsonk_last_failure[YYJSONK_MAX_LOG];

VOID
yyjsonk_log_text(ULONG level, const char *text)
{
    if (!text) {
        return;
    }
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, level, "%s", text);
}

VOID
yyjsonk_logf(ULONG level, const char *fmt, ...)
{
    char buf[YYJSONK_MAX_LOG];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    yyjsonk_log_text(level, buf);
}

NTSTATUS
yyjsonk_runtime_init(PCUNICODE_STRING registry_path)
{
    UNREFERENCED_PARAMETER(registry_path);
    yyjsonk_stdio_runtime_init();
    yyjsonk_set_test_root_pcstr(YYJSONK_DEFAULT_TEST_ROOT);
    yyjsonk_clear_current_test_failure();
    setlocale(LC_ALL, "C");
    return STATUS_SUCCESS;
}

VOID
yyjsonk_runtime_uninit(VOID)
{
    yyjsonk_stdio_runtime_uninit();
}

VOID
yyjsonk_set_test_root_pcstr(const char *path)
{
    size_t len;
    if (!path) {
        path = YYJSONK_DEFAULT_TEST_ROOT;
    }
    len = strlen(path);
    if (len >= sizeof(g_yyjsonk_test_root)) {
        len = sizeof(g_yyjsonk_test_root) - 1;
    }
    memcpy(g_yyjsonk_test_root, path, len);
    g_yyjsonk_test_root[len] = '\0';
}

const char *
yyjsonk_get_test_root_pcstr(VOID)
{
    return g_yyjsonk_test_root;
}

const char *
yyjson_test_data_path(void)
{
    return yyjsonk_get_test_root_pcstr();
}

BOOLEAN
yyjsonk_file_api_allowed(VOID)
{
    return KeGetCurrentIrql() == PASSIVE_LEVEL;
}

VOID
yyjsonk_set_current_test(const char *name)
{
    g_yyjsonk_current_test = name;
}

const char *
yyjsonk_get_current_test(VOID)
{
    return g_yyjsonk_current_test;
}

VOID
yyjsonk_clear_current_test_failure(VOID)
{
    g_yyjsonk_test_failed = FALSE;
    g_yyjsonk_last_failure[0] = '\0';
}

BOOLEAN
yyjsonk_current_test_failed(VOID)
{
    return g_yyjsonk_test_failed;
}

const char *
yyjsonk_last_failure_message(VOID)
{
    return g_yyjsonk_last_failure;
}

__declspec(noreturn) VOID
yyjsonk_fail_assert(const char *expr, const char *file, int line, const char *fmt, ...)
{
    char detail[YYJSONK_MAX_LOG];
    char message[YYJSONK_MAX_LOG];
    va_list args;

    detail[0] = '\0';
    if (fmt) {
        va_start(args, fmt);
        vsnprintf(detail, sizeof(detail), fmt, args);
        va_end(args);
    }

    if (fmt && detail[0]) {
        snprintf(message,
                 sizeof(message),
                 "[yyjson_kmdf][FAIL][%s] %s (%s:%d): %s\n",
                 g_yyjsonk_current_test ? g_yyjsonk_current_test : "<no-test>",
                 expr ? expr : "<expr>",
                 file ? file : "<file>",
                 line,
                 detail);
    } else {
        snprintf(message,
                 sizeof(message),
                 "[yyjson_kmdf][FAIL][%s] %s (%s:%d)\n",
                 g_yyjsonk_current_test ? g_yyjsonk_current_test : "<no-test>",
                 expr ? expr : "<expr>",
                 file ? file : "<file>",
                 line);
    }

    g_yyjsonk_test_failed = TRUE;
    strncpy(g_yyjsonk_last_failure, message, sizeof(g_yyjsonk_last_failure) - 1);
    g_yyjsonk_last_failure[sizeof(g_yyjsonk_last_failure) - 1] = '\0';
    yyjsonk_log_text(DPFLTR_ERROR_LEVEL, message);
    ExRaiseStatus(STATUS_ASSERTION_FAILURE);
}

void
yy_rand_reset(u64 seed)
{
    g_yy_rand_seed = seed;
}

u32
yy_rand_u32(void)
{
    u64 z = (g_yy_rand_seed += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 33)) * 0x62a9d9ed799705f5ull;
    z = (z ^ (z >> 28)) * 0xcb24d0a5c88c35b3ull;
    return (u32)(z >> 32);
}

u32
yy_rand_u32_uniform(u32 bound)
{
    if (yy_unlikely(bound < 2)) {
        return 0;
    }
    for (;;) {
        u32 r = yy_rand_u32();
        u32 x = r % bound;
        if (r - x <= (u32)(-(i32)bound)) {
            return x;
        }
    }
}

u32
yy_rand_u32_range(u32 min, u32 max)
{
    return yy_rand_u32_uniform(max - min + 1) + min;
}

u64
yy_rand_u64(void)
{
    u64 z = (g_yy_rand_seed += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

u64
yy_rand_u64_uniform(u64 bound)
{
    if (yy_unlikely(bound < 2)) {
        return 0;
    }
    for (;;) {
        u64 r = yy_rand_u64();
        u64 x = r % bound;
        if (r - x <= (u64)(-(i64)bound)) {
            return x;
        }
    }
}

u64
yy_rand_u64_range(u64 min, u64 max)
{
    return yy_rand_u64_uniform(max - min + 1) + min;
}

f32
yy_rand_f32(void)
{
    for (;;) {
        u32 r = yy_rand_u32();
        u32 x = ~(u32)0;
        if (r != x) {
            return (f32)r / (f32)x;
        }
    }
}

f32
yy_rand_f32_range(f32 min, f32 max)
{
    return min + (max - min) * yy_rand_f32();
}

f64
yy_rand_f64(void)
{
    for (;;) {
        u64 r = yy_rand_u64();
        u64 x = ~(u64)0;
        if (r != x) {
            return (f64)r / (f64)x;
        }
    }
}

f64
yy_rand_f64_range(f64 min, f64 max)
{
    return min + (max - min) * yy_rand_f64();
}

FILE *
yy_file_open(const char *path, const char *mode)
{
    FILE *file = NULL;
    if (!path || !mode) {
        return NULL;
    }
    if (fopen_s(&file, path, mode) != 0) {
        return NULL;
    }
    return file;
}

bool
yy_file_read(const char *path, u8 **dat, usize *len)
{
    return yy_file_read_with_padding(path, dat, len, 1);
}

bool
yy_file_read_with_padding(const char *path, u8 **dat, usize *len, usize padding)
{
    FILE *file;
    long file_size;
    void *buf;

    if (!path || !*path || !dat || !len) {
        return false;
    }

    file = yy_file_open(path, "rb");
    if (!file) {
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    buf = malloc((usize)file_size + padding);
    if (!buf) {
        fclose(file);
        return false;
    }

    if (file_size > 0) {
        if (fread_s(buf, (size_t)file_size, 1, (size_t)file_size, file) != (size_t)file_size) {
            free(buf);
            fclose(file);
            return false;
        }
    }
    fclose(file);

    memset((unsigned char *)buf + file_size, 0, padding);
    *dat = (u8 *)buf;
    *len = (usize)file_size;
    return true;
}

bool
yy_file_write(const char *path, u8 *dat, usize len)
{
    FILE *file;

    if (!path || !*path) {
        return false;
    }
    if (len != 0 && !dat) {
        return false;
    }

    file = yy_file_open(path, "wb");
    if (!file) {
        return false;
    }
    if (len != 0 && fwrite(dat, len, 1, file) != 1) {
        fclose(file);
        return false;
    }
    if (fclose(file) != 0) {
        return false;
    }
    return true;
}

static char
YYJSONK_ToLower(char c)
{
    return ('A' <= c && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

char *
yy_str_copy(const char *str)
{
    usize len;
    char *dup;

    if (!str) {
        return NULL;
    }
    len = strlen(str) + 1;
    dup = (char *)malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

int
yy_str_cmp(const char *str1, const char *str2, bool ignore_case)
{
    if (str1 == str2) {
        return 0;
    }
    if (!str1) {
        return -1;
    }
    if (!str2) {
        return 1;
    }
    if (!ignore_case) {
        return strcmp(str1, str2);
    }
    while (*str1 || *str2) {
        int diff = YYJSONK_ToLower(*str1++) - YYJSONK_ToLower(*str2++);
        if (diff != 0) {
            return diff;
        }
    }
    return 0;
}

bool
yy_str_contains(const char *str, const char *search)
{
    if (!str || !search) {
        return false;
    }
    return strstr(str, search) != NULL;
}

bool
yy_str_has_prefix(const char *str, const char *prefix)
{
    usize len1;
    usize len2;
    if (!str || !prefix) {
        return false;
    }
    len1 = strlen(str);
    len2 = strlen(prefix);
    if (len2 > len1) {
        return false;
    }
    return memcmp(str, prefix, len2) == 0;
}

bool
yy_str_has_suffix(const char *str, const char *suffix)
{
    usize len1;
    usize len2;
    if (!str || !suffix) {
        return false;
    }
    len1 = strlen(str);
    len2 = strlen(suffix);
    if (len2 > len1) {
        return false;
    }
    return memcmp(str + (len1 - len2), suffix, len2) == 0;
}

bool
yy_str_is_utf8(const char *str, size_t len)
{
    const uint8_t *cur = (const uint8_t *)str;
    const uint8_t *end = cur + len;
    uint32_t u;

    if (!str) {
        return false;
    }

    while (cur < end) {
        if ((cur[0] & 0x80) == 0) {
            cur++;
            continue;
        }
        if ((cur[0] & 0xE0) == 0xC0) {
            if (end - cur < 2 || (cur[1] & 0xC0) != 0x80) {
                return false;
            }
            u = ((uint32_t)(cur[1] & 0x3F) << 0) |
                ((uint32_t)(cur[0] & 0x1F) << 6);
            if (u < 0x80 || u > 0x7FF) {
                return false;
            }
            cur += 2;
            continue;
        }
        if ((cur[0] & 0xF0) == 0xE0) {
            if (end - cur < 3 ||
                (cur[1] & 0xC0) != 0x80 ||
                (cur[2] & 0xC0) != 0x80) {
                return false;
            }
            u = ((uint32_t)(cur[2] & 0x3F) << 0) |
                ((uint32_t)(cur[1] & 0x3F) << 6) |
                ((uint32_t)(cur[0] & 0x0F) << 12);
            if (u < 0x800 || u > 0xFFFF) {
                return false;
            }
            if (0xD800 <= u && u <= 0xDFFF) {
                return false;
            }
            cur += 3;
            continue;
        }
        if ((cur[0] & 0xF8) == 0xF0) {
            if (end - cur < 4 ||
                (cur[1] & 0xC0) != 0x80 ||
                (cur[2] & 0xC0) != 0x80 ||
                (cur[3] & 0xC0) != 0x80) {
                return false;
            }
            u = ((uint32_t)(cur[3] & 0x3F) << 0) |
                ((uint32_t)(cur[2] & 0x3F) << 6) |
                ((uint32_t)(cur[1] & 0x3F) << 12) |
                ((uint32_t)(cur[0] & 0x07) << 18);
            if (u < 0x10000 || u > 0x10FFFF) {
                return false;
            }
            cur += 4;
            continue;
        }
        return false;
    }
    return true;
}

bool
yy_buf_init(yy_buf *buf, usize len)
{
    if (!buf) {
        return false;
    }
    if (len < 16) {
        len = 16;
    }
    memset(buf, 0, sizeof(*buf));
    buf->hdr = (u8 *)malloc(len);
    if (!buf->hdr) {
        return false;
    }
    buf->cur = buf->hdr;
    buf->end = buf->hdr + len;
    buf->need_free = true;
    return true;
}

void
yy_buf_release(yy_buf *buf)
{
    if (!buf || !buf->hdr) {
        return;
    }
    if (buf->need_free) {
        free(buf->hdr);
    }
    memset(buf, 0, sizeof(*buf));
}

usize
yy_buf_len(yy_buf *buf)
{
    if (!buf) {
        return 0;
    }
    return (usize)(buf->cur - buf->hdr);
}

bool
yy_buf_grow(yy_buf *buf, usize len)
{
    usize used;
    usize cap;
    u8 *tmp;

    if (!buf) {
        return false;
    }
    if ((usize)(buf->end - buf->cur) >= len) {
        return true;
    }
    if (!buf->hdr) {
        return yy_buf_init(buf, len);
    }

    used = (usize)(buf->cur - buf->hdr);
    cap = (usize)(buf->end - buf->hdr);
    do {
        if (cap > (SIZE_MAX / 2)) {
            return false;
        }
        cap *= 2;
    } while (cap - used < len);

    tmp = (u8 *)realloc(buf->hdr, cap);
    if (!tmp) {
        return false;
    }
    buf->hdr = tmp;
    buf->cur = tmp + used;
    buf->end = tmp + cap;
    return true;
}

bool
yy_buf_append(yy_buf *buf, u8 *dat, usize len)
{
    if (!buf) {
        return false;
    }
    if (len == 0) {
        return true;
    }
    if (!dat || !yy_buf_grow(buf, len)) {
        return false;
    }
    memcpy(buf->cur, dat, len);
    buf->cur += len;
    return true;
}

bool
yy_dat_init_with_file(yy_dat *dat, const char *path)
{
    u8 *mem;
    usize len;
    if (!dat) {
        return false;
    }
    memset(dat, 0, sizeof(*dat));
    if (!yy_file_read(path, &mem, &len)) {
        return false;
    }
    dat->hdr = mem;
    dat->cur = mem;
    dat->end = mem + len;
    dat->need_free = true;
    return true;
}

bool
yy_dat_init_with_mem(yy_dat *dat, u8 *mem, usize len)
{
    if (!dat) {
        return false;
    }
    if (len && !mem) {
        return false;
    }
    dat->hdr = mem;
    dat->cur = mem;
    dat->end = mem + len;
    dat->need_free = false;
    return true;
}

void
yy_dat_release(yy_dat *dat)
{
    yy_buf_release(dat);
}

void
yy_dat_reset(yy_dat *dat)
{
    if (dat) {
        dat->cur = dat->hdr;
    }
}

char *
yy_dat_read_line(yy_dat *dat, usize *len)
{
    u8 *str;
    u8 *cur;

    if (len) {
        *len = 0;
    }
    if (!dat || dat->cur >= dat->end) {
        return NULL;
    }

    str = dat->cur;
    cur = dat->cur;
    while (cur < dat->end && *cur != '\r' && *cur != '\n' && *cur != '\0') {
        cur++;
    }
    if (len) {
        *len = (usize)(cur - str);
    }
    if (cur < dat->end) {
        if (cur + 1 < dat->end && *cur == '\r' && cur[1] == '\n') {
            cur += 2;
        } else {
            cur++;
        }
    }
    dat->cur = cur;
    return (char *)str;
}

char *
yy_dat_copy_line(yy_dat *dat, usize *len)
{
    char *src;
    char *dst;
    usize read_len;

    if (len) {
        *len = 0;
    }
    src = yy_dat_read_line(dat, &read_len);
    if (!src) {
        return NULL;
    }
    dst = (char *)malloc(read_len + 1);
    if (!dst) {
        return NULL;
    }
    memcpy(dst, src, read_len);
    dst[read_len] = '\0';
    if (len) {
        *len = read_len;
    }
    return dst;
}

double
yy_get_time(void)
{
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER counter = KeQueryPerformanceCounter(&freq);
    if (freq.QuadPart == 0) {
        return 0.0;
    }
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

double
yy_get_timestamp(void)
{
    LARGE_INTEGER now;
    KeQuerySystemTimePrecise(&now);
    return (double)now.QuadPart * 1e-7 - 11644473600.0;
}
