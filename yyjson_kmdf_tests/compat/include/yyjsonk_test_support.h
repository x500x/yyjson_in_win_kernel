#ifndef YYJSONK_TEST_SUPPORT_H
#define YYJSONK_TEST_SUPPORT_H

#include "yyjsonk_runtime.h"

#ifndef YYJSONK_DEFAULT_TEST_ROOT
#define YYJSONK_DEFAULT_TEST_ROOT "C:\\json\\yyjson\\test"
#endif

#ifndef YYJSONK_FILE_BOTH_DIR_INFORMATION_DEFINED
#define YYJSONK_FILE_BOTH_DIR_INFORMATION_DEFINED
typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan
    );

#ifdef __cplusplus
extern "C" {
#endif

VOID yyjsonk_set_test_root_pcstr(_In_ const char *path);
const char *yyjsonk_get_test_root_pcstr(VOID);
const char *yyjson_test_data_path(VOID);

VOID yyjsonk_set_current_test(_In_opt_ const char *name);
const char *yyjsonk_get_current_test(VOID);
VOID yyjsonk_clear_current_test_failure(VOID);
BOOLEAN yyjsonk_current_test_failed(VOID);
const char *yyjsonk_last_failure_message(VOID);

#ifdef __cplusplus
}
#endif

#endif
