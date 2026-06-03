#include "yyjsonk_runtime.h"
#include "locale.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "goo_double_conv.h"

static char g_yyjsonk_locale_name[16] = "C";
static char g_yyjsonk_decimal_point[2] = ".";
static struct lconv g_yyjsonk_lconv = {
    g_yyjsonk_decimal_point
};

static int
yyjsonk_ascii_tolower(int ch)
{
    if ('A' <= ch && ch <= 'Z') {
        return ch + ('a' - 'A');
    }
    return ch;
}

static int
yyjsonk_hex_value(int ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    }
    ch = yyjsonk_ascii_tolower(ch);
    if ('a' <= ch && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    return -1;
}

static int
yyjsonk_is_space(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' ||
           ch == '\r' || ch == '\f' || ch == '\v';
}

static int
yyjsonk_ascii_stricmp(const char *lhs, const char *rhs)
{
    int lch;
    int rch;

    if (lhs == rhs) {
        return 0;
    }
    if (!lhs) {
        return -1;
    }
    if (!rhs) {
        return 1;
    }

    while (*lhs || *rhs) {
        lch = yyjsonk_ascii_tolower((unsigned char)*lhs++);
        rch = yyjsonk_ascii_tolower((unsigned char)*rhs++);
        if (lch != rch) {
            return lch - rch;
        }
    }
    return 0;
}

static unsigned long long
yyjsonk_strtoull_core(const char *nptr,
                      char **endptr,
                      int base,
                      bool *negative)
{
    const char *cur = nptr;
    unsigned long long value = 0;
    bool neg = false;
    bool any = false;

    while (yyjsonk_is_space((unsigned char)*cur)) {
        cur++;
    }

    if (*cur == '+' || *cur == '-') {
        neg = (*cur == '-');
        cur++;
    }

    if (base == 0) {
        if (cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X')) {
            base = 16;
            cur += 2;
        } else if (cur[0] == '0') {
            base = 8;
            cur += 1;
            any = true;
            value = 0;
        } else {
            base = 10;
        }
    } else if (base == 16 && cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X')) {
        cur += 2;
    }

    while (*cur) {
        int digit = yyjsonk_hex_value((unsigned char)*cur);
        if (digit < 0 || digit >= base) {
            break;
        }
        any = true;
        value = value * (unsigned long long)base + (unsigned long long)digit;
        cur++;
    }

    if (negative) {
        *negative = neg;
    }
    if (endptr) {
        *endptr = (char *)(any ? cur : nptr);
    }
    return value;
}

static bool
yyjsonk_match_literal(const char *cur, const char *lit, size_t *len_out)
{
    size_t idx = 0;
    while (lit[idx]) {
        if (yyjsonk_ascii_tolower((unsigned char)cur[idx]) !=
            yyjsonk_ascii_tolower((unsigned char)lit[idx])) {
            return false;
        }
        idx++;
    }
    if (len_out) {
        *len_out = idx;
    }
    return true;
}

static double
yyjsonk_nan_value(void)
{
    unsigned long long bits = 0x7ff8000000000000ull;
    double value = 0.0;
    RtlCopyMemory(&value, &bits, sizeof(value));
    return value;
}

static double
yyjsonk_pos_inf_value(void)
{
    unsigned long long bits = 0x7ff0000000000000ull;
    double value = 0.0;
    RtlCopyMemory(&value, &bits, sizeof(value));
    return value;
}

char * __cdecl
setlocale(int category, const char *locale)
{
    (void)category;

    if (!locale) {
        return g_yyjsonk_locale_name;
    }

    if (yyjsonk_ascii_stricmp(locale, "C") == 0) {
        strcpy(g_yyjsonk_locale_name, "C");
        strcpy(g_yyjsonk_decimal_point, ".");
        return g_yyjsonk_locale_name;
    }

    if (yyjsonk_ascii_stricmp(locale, "fr_FR") == 0) {
        strcpy(g_yyjsonk_locale_name, "fr_FR");
        strcpy(g_yyjsonk_decimal_point, ",");
        return g_yyjsonk_locale_name;
    }

    return NULL;
}

struct lconv * __cdecl
localeconv(void)
{
    return &g_yyjsonk_lconv;
}

unsigned long long __cdecl
strtoull(const char *nptr, char **endptr, int base)
{
    bool negative = false;
    unsigned long long value = yyjsonk_strtoull_core(nptr, endptr, base, &negative);
    if (negative) {
        return (unsigned long long)(-(long long)value);
    }
    return value;
}

long long __cdecl
strtoll(const char *nptr, char **endptr, int base)
{
    bool negative = false;
    unsigned long long value = yyjsonk_strtoull_core(nptr, endptr, base, &negative);
    if (negative) {
        return -(long long)value;
    }
    return (long long)value;
}

double __cdecl
strtod(const char *nptr, char **endptr)
{
    const char *cur = nptr;
    size_t literal_len = 0;
    size_t len = 0;
    bool saw_digit = false;
    bool saw_exp = false;
    bool saw_dec = false;
    char *tmp;
    int proc = 0;
    double value = 0.0;

    while (yyjsonk_is_space((unsigned char)*cur)) {
        cur++;
    }

    if (*cur == '+' || *cur == '-') {
        len++;
        cur++;
    }

    if (yyjsonk_match_literal(cur, "nan", &literal_len)) {
        if (endptr) {
            *endptr = (char *)(cur + literal_len);
        }
        return yyjsonk_nan_value();
    }
    if (yyjsonk_match_literal(cur, "infinity", &literal_len)) {
        if (endptr) {
            *endptr = (char *)(cur + literal_len);
        }
        return (nptr[0] == '-') ? -yyjsonk_pos_inf_value() : yyjsonk_pos_inf_value();
    }
    if (yyjsonk_match_literal(cur, "inf", &literal_len)) {
        if (endptr) {
            *endptr = (char *)(cur + literal_len);
        }
        return (nptr[0] == '-') ? -yyjsonk_pos_inf_value() : yyjsonk_pos_inf_value();
    }

    cur = nptr;
    if (*cur == '+' || *cur == '-') {
        cur++;
    }
    while (*cur) {
        if (*cur >= '0' && *cur <= '9') {
            saw_digit = true;
            len++;
            cur++;
            continue;
        }
        if (!saw_dec && (*cur == '.' || *cur == ',')) {
            saw_dec = true;
            len++;
            cur++;
            continue;
        }
        if (!saw_exp && (*cur == 'e' || *cur == 'E')) {
            saw_exp = true;
            len++;
            cur++;
            if (*cur == '+' || *cur == '-') {
                len++;
                cur++;
            }
            continue;
        }
        break;
    }

    if (!saw_digit || len == 0) {
        if (endptr) {
            *endptr = (char *)nptr;
        }
        return 0.0;
    }

    tmp = (char *)malloc(len + 1);
    if (!tmp) {
        if (endptr) {
            *endptr = (char *)nptr;
        }
        return 0.0;
    }

    memcpy(tmp, nptr, len);
    tmp[len] = '\0';
    for (proc = 0; proc < (int)len; proc++) {
        if (tmp[proc] == ',') {
            tmp[proc] = '.';
        }
    }

    value = goo_strtod(tmp, (int)len, &proc);
    free(tmp);

    if (endptr) {
        *endptr = (char *)(nptr + ((proc > 0) ? (size_t)proc : 0));
    }
    return value;
}

float __cdecl
strtof(const char *nptr, char **endptr)
{
    return (float)strtod(nptr, endptr);
}
