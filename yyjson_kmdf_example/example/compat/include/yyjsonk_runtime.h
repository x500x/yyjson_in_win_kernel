#ifndef YYJSONK_RUNTIME_H
#define YYJSONK_RUNTIME_H

#include <ntddk.h>
#include <wdf.h>
#include <stdarg.h>
#include <stddef.h>

#ifndef va_copy
#define va_copy(destination, source) ((destination) = (source))
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

#include "yyjsonk_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum yyjsonk_file_kind {
    YYJSONK_FILE_KIND_CONSOLE = 1,
    YYJSONK_FILE_KIND_DISK = 2
} yyjsonk_file_kind;

typedef struct yyjsonk_FILE {
    yyjsonk_file_kind kind;
    HANDLE handle;
    LARGE_INTEGER offset;
    BOOLEAN can_read;
    BOOLEAN can_write;
    BOOLEAN eof;
    BOOLEAN error;
    BOOLEAN registered;
    LIST_ENTRY link;
} FILE;

NTSTATUS yyjsonk_runtime_init(_In_opt_ PCUNICODE_STRING registry_path);
VOID yyjsonk_runtime_uninit(VOID);

BOOLEAN yyjsonk_file_api_allowed(VOID);

VOID yyjsonk_log_text(_In_ ULONG level, _In_ const char *text);
VOID yyjsonk_logf(_In_ ULONG level, _In_ const char *fmt, ...);
__declspec(noreturn) VOID yyjsonk_fail_assert(_In_ const char *expr,
                                              _In_ const char *file,
                                              _In_ int line,
                                              _In_opt_ const char *fmt,
                                              ...);

VOID yyjsonk_stdio_runtime_init(VOID);
VOID yyjsonk_stdio_runtime_uninit(VOID);
int yyjsonk_file_write_buffer(_In_ FILE *file,
                              _In_reads_bytes_(len) const char *data,
                              _In_ size_t len);

BOOLEAN yyjsonk_resolve_path_pcstr(_In_ const char *path,
                                   _Out_writes_z_(cap) char *out,
                                   _In_ size_t cap);
NTSTATUS yyjsonk_path_to_unicode_pcstr(_In_ const char *path,
                                       _Out_ PUNICODE_STRING unicode_path,
                                       _Outptr_result_maybenull_z_ PWSTR *storage);
VOID yyjsonk_free_unicode_buffer(_In_opt_ PWSTR storage);

#ifdef __cplusplus
}
#endif

#endif
