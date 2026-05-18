#include "locale.h"
#include "yyjsonk_runtime.h"
#include "stdio.h"

#include <stdlib.h>
#include <string.h>

static BOOLEAN g_yyjsonk_runtime_ready = FALSE;

static BOOLEAN
YYJSONK_HasPrefix(const char *str, const char *prefix)
{
    size_t len;

    if (!str || !prefix) {
        return FALSE;
    }

    len = strlen(prefix);
    return strncmp(str, prefix, len) == 0;
}

static BOOLEAN
YYJSONK_IsAbsoluteDosPath(const char *path)
{
    if (!path || !path[0] || !path[1] || !path[2]) {
        return FALSE;
    }

    if (!(('A' <= path[0] && path[0] <= 'Z') ||
          ('a' <= path[0] && path[0] <= 'z'))) {
        return FALSE;
    }

    if (path[1] != ':') {
        return FALSE;
    }

    return path[2] == '\\' || path[2] == '/';
}

static BOOLEAN
YYJSONK_IsNtPath(const char *path)
{
    return YYJSONK_HasPrefix(path, "\\??\\") ||
           YYJSONK_HasPrefix(path, "\\Device\\");
}

static VOID
YYJSONK_NormalizeSeparators(char *path)
{
    if (!path) {
        return;
    }

    while (*path) {
        if (*path == '/') {
            *path = '\\';
        }
        path++;
    }
}

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
    if (g_yyjsonk_runtime_ready) {
        return STATUS_SUCCESS;
    }

    yyjsonk_stdio_runtime_init();
    setlocale(LC_ALL, "C");
    g_yyjsonk_runtime_ready = TRUE;
    return STATUS_SUCCESS;
}

VOID
yyjsonk_runtime_uninit(VOID)
{
    if (!g_yyjsonk_runtime_ready) {
        return;
    }

    yyjsonk_stdio_runtime_uninit();
    g_yyjsonk_runtime_ready = FALSE;
}

BOOLEAN
yyjsonk_file_api_allowed(VOID)
{
    return KeGetCurrentIrql() == PASSIVE_LEVEL;
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
                 "[example][assert] %s (%s:%d): %s\n",
                 expr ? expr : "<expr>",
                 file ? file : "<file>",
                 line,
                 detail);
    } else {
        snprintf(message,
                 sizeof(message),
                 "[example][assert] %s (%s:%d)\n",
                 expr ? expr : "<expr>",
                 file ? file : "<file>",
                 line);
    }

    yyjsonk_log_text(DPFLTR_ERROR_LEVEL, message);
    ExRaiseStatus(STATUS_ASSERTION_FAILURE);
}

BOOLEAN
yyjsonk_resolve_path_pcstr(const char *path, char *out, size_t cap)
{
    size_t len;

    if (!path || !out || cap == 0) {
        return FALSE;
    }

    if (!YYJSONK_IsAbsoluteDosPath(path) && !YYJSONK_IsNtPath(path)) {
        return FALSE;
    }

    len = strlen(path);
    if (len + 1 > cap) {
        return FALSE;
    }

    memcpy(out, path, len + 1);
    YYJSONK_NormalizeSeparators(out);
    return TRUE;
}

NTSTATUS
yyjsonk_path_to_unicode_pcstr(const char *path,
                              PUNICODE_STRING unicode_path,
                              PWSTR *storage)
{
    char resolved[YYJSONK_MAX_PATH];
    const char *cur;
    size_t prefix_len = 0;
    size_t resolved_len;
    size_t total_len;
    size_t idx = 0;
    PWSTR wide;

    if (!unicode_path || !storage) {
        return STATUS_INVALID_PARAMETER;
    }

    *storage = NULL;
    RtlZeroMemory(unicode_path, sizeof(*unicode_path));

    if (!yyjsonk_resolve_path_pcstr(path, resolved, sizeof(resolved))) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (YYJSONK_IsAbsoluteDosPath(resolved)) {
        prefix_len = 4;
    }

    resolved_len = strlen(resolved);
    total_len = prefix_len + resolved_len;
    if (total_len > (UNICODE_STRING_MAX_BYTES / sizeof(WCHAR)) - 1) {
        return STATUS_NAME_TOO_LONG;
    }

    wide = (PWSTR)malloc((total_len + 1) * sizeof(WCHAR));
    if (!wide) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (prefix_len != 0) {
        wide[idx++] = L'\\';
        wide[idx++] = L'?';
        wide[idx++] = L'?';
        wide[idx++] = L'\\';
    }

    cur = resolved;
    while (*cur) {
        wide[idx++] = (WCHAR)(unsigned char)(*cur++);
    }
    wide[idx] = L'\0';

    unicode_path->Buffer = wide;
    unicode_path->Length = (USHORT)(idx * sizeof(WCHAR));
    unicode_path->MaximumLength = (USHORT)((idx + 1) * sizeof(WCHAR));
    *storage = wide;
    return STATUS_SUCCESS;
}

VOID
yyjsonk_free_unicode_buffer(PWSTR storage)
{
    free(storage);
}
