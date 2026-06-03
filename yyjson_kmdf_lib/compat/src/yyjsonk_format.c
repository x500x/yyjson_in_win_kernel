#include "yyjsonk_runtime.h"
#include "stdio.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "goo_double_conv.h"

typedef enum yyjsonk_length_mod {
    YYJSONK_LEN_NONE = 0,
    YYJSONK_LEN_LONG = 1,
    YYJSONK_LEN_LLONG = 2,
    YYJSONK_LEN_SIZE = 3
} yyjsonk_length_mod;

typedef struct yyjsonk_format_output {
    char *buf;
    size_t cap;
    size_t len;
} yyjsonk_format_output;

static void
YYJSONK_OutChar(yyjsonk_format_output *out, char ch)
{
    if (out->buf && out->len + 1 < out->cap) {
        out->buf[out->len] = ch;
    }
    out->len++;
}

static void
YYJSONK_OutBytes(yyjsonk_format_output *out, const char *src, size_t len)
{
    size_t idx;
    for (idx = 0; idx < len; idx++) {
        YYJSONK_OutChar(out, src[idx]);
    }
}

static size_t
YYJSONK_FormatUnsigned(char *buf, unsigned long long value, unsigned base, int upper_case)
{
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char *digits = upper_case ? digits_upper : digits_lower;
    char tmp[32];
    size_t idx = 0;
    size_t out_len = 0;

    if (base < 2 || base > 16) {
        buf[0] = '\0';
        return 0;
    }

    do {
        tmp[idx++] = digits[value % base];
        value /= base;
    } while (value != 0);

    while (idx > 0) {
        buf[out_len++] = tmp[--idx];
    }
    buf[out_len] = '\0';
    return out_len;
}

static int
YYJSONK_IsDigit(char ch)
{
    return ('0' <= ch && ch <= '9');
}

static int
YYJSONK_F64IsNan(double value)
{
    unsigned long long bits = 0;
    RtlCopyMemory(&bits, &value, sizeof(bits));
    return (((bits >> 52) & 0x7ffu) == 0x7ffu) &&
           ((bits & ((((unsigned long long)1) << 52) - 1)) != 0);
}

static int
YYJSONK_F64IsInf(double value)
{
    unsigned long long bits = 0;
    RtlCopyMemory(&bits, &value, sizeof(bits));
    return (((bits >> 52) & 0x7ffu) == 0x7ffu) &&
           ((bits & ((((unsigned long long)1) << 52) - 1)) == 0);
}

static size_t
YYJSONK_FormatFloat(char *buf, size_t cap, double value, char spec, int precision, int precision_set)
{
    int out_len = 0;
    int goo_prec;

    if (cap == 0) {
        return 0;
    }

    if (YYJSONK_F64IsNan(value)) {
        strcpy(buf, "nan");
        return 3;
    }
    if (YYJSONK_F64IsInf(value)) {
        if (value < 0) {
            strcpy(buf, "-inf");
            return 4;
        }
        strcpy(buf, "inf");
        return 3;
    }

    if (spec == 'f') {
        goo_prec = precision_set ? precision : 6;
        if (goo_prec < 0) {
            goo_prec = 6;
        }
        out_len = goo_dtoa(value, GOO_FMT_FIXED, goo_prec, buf, (int)cap);
    } else {
        goo_prec = precision_set ? precision : 6;
        if (goo_prec == 0) {
            goo_prec = 1;
        }
        if (goo_prec < 0) {
            goo_prec = 6;
        }
        out_len = goo_dtoa(value, GOO_FMT_PRECISION, goo_prec, buf, (int)cap);
    }

    if (out_len < 0) {
        out_len = 0;
    }
    if ((size_t)out_len >= cap) {
        out_len = (int)(cap - 1);
    }
    buf[out_len] = '\0';
    return (size_t)out_len;
}

int __cdecl
vsnprintf(char *buf, size_t len, const char *fmt, va_list args)
{
    yyjsonk_format_output out;
    const char *cur;
    va_list ap;

    out.buf = buf;
    out.cap = len;
    out.len = 0;

    if (!fmt) {
        if (buf && len) {
            buf[0] = '\0';
        }
        return -1;
    }

    va_copy(ap, args);

    for (cur = fmt; *cur; cur++) {
        int precision = 0;
        int precision_set = 0;
        yyjsonk_length_mod length_mod = YYJSONK_LEN_NONE;

        if (*cur != '%') {
            YYJSONK_OutChar(&out, *cur);
            continue;
        }

        cur++;
        if (*cur == '%') {
            YYJSONK_OutChar(&out, '%');
            continue;
        }

        while (YYJSONK_IsDigit(*cur)) {
            cur++;
        }

        if (*cur == '.') {
            cur++;
            precision_set = 1;
            if (*cur == '*') {
                precision = va_arg(ap, int);
                if (precision < 0) {
                    precision_set = 0;
                    precision = 0;
                }
                cur++;
            } else {
                while (YYJSONK_IsDigit(*cur)) {
                    precision = precision * 10 + (*cur - '0');
                    cur++;
                }
            }
        }

        if (*cur == 'l') {
            length_mod = YYJSONK_LEN_LONG;
            cur++;
            if (*cur == 'l') {
                length_mod = YYJSONK_LEN_LLONG;
                cur++;
            }
        } else if (*cur == 'z') {
            length_mod = YYJSONK_LEN_SIZE;
            cur++;
        }

        switch (*cur) {
        case 's': {
            const char *str = va_arg(ap, const char *);
            size_t str_len;
            if (!str) {
                str = "(null)";
            }
            str_len = strlen(str);
            if (precision_set && precision >= 0 && (size_t)precision < str_len) {
                str_len = (size_t)precision;
            }
            YYJSONK_OutBytes(&out, str, str_len);
            break;
        }
        case 'c': {
            int ch = va_arg(ap, int);
            YYJSONK_OutChar(&out, (char)ch);
            break;
        }
        case 'd':
        case 'i': {
            long long value;
            unsigned long long magnitude;
            char tmp[64];
            size_t tmp_len;

            if (length_mod == YYJSONK_LEN_LLONG) {
                value = va_arg(ap, long long);
            } else if (length_mod == YYJSONK_LEN_LONG) {
                value = va_arg(ap, long);
            } else if (length_mod == YYJSONK_LEN_SIZE) {
                value = (long long)va_arg(ap, ptrdiff_t);
            } else {
                value = va_arg(ap, int);
            }

            if (value < 0) {
                YYJSONK_OutChar(&out, '-');
                magnitude = (unsigned long long)(-(value + 1)) + 1;
            } else {
                magnitude = (unsigned long long)value;
            }

            tmp_len = YYJSONK_FormatUnsigned(tmp, magnitude, 10, 0);
            YYJSONK_OutBytes(&out, tmp, tmp_len);
            break;
        }
        case 'u':
        case 'x':
        case 'X': {
            unsigned long long value;
            unsigned base = (*cur == 'u') ? 10u : 16u;
            char tmp[64];
            size_t tmp_len;

            if (length_mod == YYJSONK_LEN_LLONG) {
                value = va_arg(ap, unsigned long long);
            } else if (length_mod == YYJSONK_LEN_LONG) {
                value = va_arg(ap, unsigned long);
            } else if (length_mod == YYJSONK_LEN_SIZE) {
                value = (unsigned long long)va_arg(ap, size_t);
            } else {
                value = va_arg(ap, unsigned int);
            }

            tmp_len = YYJSONK_FormatUnsigned(tmp, value, base, *cur == 'X');
            YYJSONK_OutBytes(&out, tmp, tmp_len);
            break;
        }
        case 'f':
        case 'g': {
            double value = va_arg(ap, double);
            char tmp[128];
            size_t tmp_len = YYJSONK_FormatFloat(tmp, sizeof(tmp), value, *cur, precision, precision_set);
            YYJSONK_OutBytes(&out, tmp, tmp_len);
            break;
        }
        default:
            YYJSONK_OutChar(&out, '%');
            if (*cur) {
                YYJSONK_OutChar(&out, *cur);
            }
            break;
        }
    }

    if (out.buf && out.cap) {
        size_t term = (out.len < out.cap) ? out.len : (out.cap - 1);
        out.buf[term] = '\0';
    }

    va_end(ap);
    return (int)out.len;
}

int __cdecl
snprintf(char *buf, size_t len, const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vsnprintf(buf, len, fmt, args);
    va_end(args);
    return ret;
}

int __cdecl
sprintf_s(char *buf, size_t len, const char *fmt, ...)
{
    va_list args;
    int ret;

    if (!buf || len == 0) {
        return -1;
    }

    va_start(args, fmt);
    ret = vsnprintf(buf, len, fmt, args);
    va_end(args);
    return ret;
}

int __cdecl
sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vsnprintf(buf, (size_t)-1, fmt, args);
    va_end(args);
    return ret;
}

static int
YYJSONK_VfprintfCommon(FILE *file, const char *fmt, va_list args)
{
    char stack_buf[512];
    va_list measure_args;
    int needed;

    va_copy(measure_args, args);
    needed = vsnprintf(stack_buf, sizeof(stack_buf), fmt, measure_args);
    va_end(measure_args);
    if (needed < 0) {
        return needed;
    }

    if ((size_t)needed < sizeof(stack_buf)) {
        if (yyjsonk_file_write_buffer(file, stack_buf, (size_t)needed) < 0) {
            return -1;
        }
        return needed;
    }

    {
        char *dyn_buf = (char *)malloc((size_t)needed + 1);
        int ret;
        if (!dyn_buf) {
            return -1;
        }
        ret = vsnprintf(dyn_buf, (size_t)needed + 1, fmt, args);
        if (ret >= 0) {
            ret = yyjsonk_file_write_buffer(file, dyn_buf, (size_t)ret);
        }
        free(dyn_buf);
        return ret;
    }
}

int __cdecl
fprintf(FILE *file, const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = YYJSONK_VfprintfCommon(file, fmt, args);
    va_end(args);
    return ret;
}

int __cdecl
printf(const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = YYJSONK_VfprintfCommon(stdout, fmt, args);
    va_end(args);
    return ret;
}
