#include "yy_test_utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int
YYJSONK_IsAbsPath(const char *path)
{
    if (!path || !path[0]) {
        return 0;
    }
    if ((('A' <= path[0] && path[0] <= 'Z') ||
         ('a' <= path[0] && path[0] <= 'z')) &&
        path[1] == ':') {
        return 1;
    }
    if ((path[0] == '\\' || path[0] == '/') &&
        (path[1] == '\\' || path[1] == '/')) {
        return 1;
    }
    if ((path[0] == '\\' || path[0] == '/') && yy_str_has_prefix(path, "\\??\\")) {
        return 1;
    }
    if ((path[0] == '\\' || path[0] == '/') && yy_str_has_prefix(path, "\\Device\\")) {
        return 1;
    }
    return 0;
}

static void
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

BOOLEAN
yyjsonk_resolve_path_pcstr(const char *path, char *out, size_t cap)
{
    const char *root;
    size_t root_len;
    size_t path_len;

    if (!path || !out || cap == 0) {
        return FALSE;
    }

    if (YYJSONK_IsAbsPath(path)) {
        path_len = strlen(path);
        if (path_len + 1 > cap) {
            return FALSE;
        }
        memcpy(out, path, path_len + 1);
        YYJSONK_NormalizeSeparators(out);
        return TRUE;
    }

    root = yyjsonk_get_test_root_pcstr();
    root_len = root ? strlen(root) : 0;
    path_len = strlen(path);
    if (root_len + 1 + path_len + 1 > cap) {
        return FALSE;
    }

    memcpy(out, root, root_len);
    if (root_len > 0 && out[root_len - 1] != '\\') {
        out[root_len++] = '\\';
    }
    memcpy(out + root_len, path, path_len + 1);
    YYJSONK_NormalizeSeparators(out);
    return TRUE;
}

NTSTATUS
yyjsonk_path_to_unicode_pcstr(const char *path, PUNICODE_STRING unicode_path, PWSTR *storage)
{
    char *resolved = NULL;
    size_t prefix_len = 0;
    size_t len;
    size_t idx;
    PWSTR wide;

    if (!unicode_path || !storage) {
        return STATUS_INVALID_PARAMETER;
    }
    *storage = NULL;
    RtlZeroMemory(unicode_path, sizeof(*unicode_path));

    resolved = (char *)malloc(YYJSONK_MAX_PATH);
    if (!resolved) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!yyjsonk_resolve_path_pcstr(path, resolved, YYJSONK_MAX_PATH)) {
        free(resolved);
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (!yy_str_has_prefix(resolved, "\\??\\") &&
        !yy_str_has_prefix(resolved, "\\Device\\")) {
        prefix_len = 4;
    }

    len = prefix_len + strlen(resolved);
    if (len > (UNICODE_STRING_MAX_BYTES / sizeof(WCHAR)) - 1) {
        free(resolved);
        return STATUS_NAME_TOO_LONG;
    }

    wide = (PWSTR)malloc((len + 1) * sizeof(WCHAR));
    if (!wide) {
        free(resolved);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    idx = 0;
    if (prefix_len) {
        wide[idx++] = L'\\';
        wide[idx++] = L'?';
        wide[idx++] = L'?';
        wide[idx++] = L'\\';
    }
    while (resolved[idx - prefix_len] != '\0') {
        wide[idx] = (WCHAR)(unsigned char)resolved[idx - prefix_len];
        idx++;
    }
    wide[len] = L'\0';
    free(resolved);

    unicode_path->Buffer = wide;
    unicode_path->Length = (USHORT)(len * sizeof(WCHAR));
    unicode_path->MaximumLength = (USHORT)((len + 1) * sizeof(WCHAR));
    *storage = wide;
    return STATUS_SUCCESS;
}

VOID
yyjsonk_free_unicode_buffer(PWSTR storage)
{
    free(storage);
}

bool
yy_path_combine(char *buf, const char *path, ...)
{
    va_list args;
    const char *item;
    size_t len;
    char *dst = buf;
    const char *hdr;

    if (!buf || !path) {
        if (buf) {
            *buf = '\0';
        }
        return false;
    }

    len = strlen(path);
    memcpy(dst, path, len);
    dst += len;
    hdr = buf;

    va_start(args, path);
    while ((item = va_arg(args, const char *)) != NULL) {
        if (dst > hdr && dst[-1] != YY_DIR_SEPARATOR) {
            *dst++ = YY_DIR_SEPARATOR;
        }
        len = strlen(item);
        while (len && (*item == '\\' || *item == '/')) {
            item++;
            len--;
        }
        memcpy(dst, item, len);
        dst += len;
    }
    va_end(args);

    *dst = '\0';
    YYJSONK_NormalizeSeparators(buf);
    return true;
}

bool
yy_path_remove_last(char *buf, const char *path)
{
    size_t len = path ? strlen(path) : 0;
    const char *cur;

    if (!buf) {
        return false;
    }
    *buf = '\0';
    if (!path || len == 0) {
        return false;
    }

    cur = path + len - 1;
    while (cur > path && (*cur == '\\' || *cur == '/')) {
        cur--;
    }
    while (cur > path && *cur != '\\' && *cur != '/') {
        cur--;
    }

    len = (size_t)(cur - path + 1);
    memcpy(buf, path, len);
    buf[len] = '\0';
    return len > 0;
}

bool
yy_path_get_last(char *buf, const char *path)
{
    size_t len = path ? strlen(path) : 0;
    const char *end;
    const char *cur;

    if (!buf) {
        return false;
    }
    *buf = '\0';
    if (!path || len == 0) {
        return false;
    }

    end = path + len;
    while (end > path && (end[-1] == '\\' || end[-1] == '/')) {
        end--;
    }
    cur = end;
    while (cur > path && cur[-1] != '\\' && cur[-1] != '/') {
        cur--;
    }

    len = (size_t)(end - cur);
    memcpy(buf, cur, len);
    buf[len] = '\0';
    return len > 0;
}

bool
yy_path_append_ext(char *buf, const char *path, const char *ext)
{
    size_t path_len;
    size_t ext_len;

    if (!buf || !path) {
        return false;
    }

    path_len = strlen(path);
    ext_len = ext ? strlen(ext) : 0;
    memcpy(buf, path, path_len);
    buf[path_len++] = '.';
    if (ext_len) {
        memcpy(buf + path_len, ext, ext_len);
        path_len += ext_len;
    }
    buf[path_len] = '\0';
    return true;
}

bool
yy_path_remove_ext(char *buf, const char *path)
{
    size_t len;
    char *cur;

    if (!buf || !path) {
        return false;
    }

    len = strlen(path);
    memcpy(buf, path, len + 1);
    for (cur = buf + len; cur >= buf; cur--) {
        if (*cur == '\\' || *cur == '/') {
            break;
        }
        if (*cur == '.') {
            *cur = '\0';
            return true;
        }
        if (cur == buf) {
            break;
        }
    }
    return false;
}

bool
yy_path_get_ext(char *buf, const char *path)
{
    size_t len;
    const char *cur;

    if (!buf || !path) {
        return false;
    }

    len = strlen(path);
    for (cur = path + len; cur >= path; cur--) {
        if (*cur == '\\' || *cur == '/') {
            break;
        }
        if (*cur == '.') {
            strcpy(buf, cur + 1);
            return true;
        }
        if (cur == path) {
            break;
        }
    }
    *buf = '\0';
    return false;
}

static bool
YYJSONK_QueryBasicInfo(const char *path, PFILE_BASIC_INFORMATION basic_info)
{
    UNICODE_STRING unicode_path;
    PWSTR storage = NULL;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK iosb;
    HANDLE handle = NULL;
    NTSTATUS status;

    if (!yyjsonk_file_api_allowed()) {
        return false;
    }
    status = yyjsonk_path_to_unicode_pcstr(path, &unicode_path, &storage);
    if (!NT_SUCCESS(status)) {
        return false;
    }

    InitializeObjectAttributes(&attributes,
                               &unicode_path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateFile(&handle,
                          FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                          &attributes,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                          NULL,
                          0);
    yyjsonk_free_unicode_buffer(storage);
    if (!NT_SUCCESS(status)) {
        return false;
    }

    status = ZwQueryInformationFile(handle,
                                    &iosb,
                                    basic_info,
                                    sizeof(*basic_info),
                                    FileBasicInformation);
    ZwClose(handle);
    return NT_SUCCESS(status);
}

bool
yy_path_exist(const char *path)
{
    FILE_BASIC_INFORMATION basic_info;
    return YYJSONK_QueryBasicInfo(path, &basic_info);
}

bool
yy_path_is_dir(const char *path)
{
    FILE_BASIC_INFORMATION basic_info;
    if (!YYJSONK_QueryBasicInfo(path, &basic_info)) {
        return false;
    }
    return (basic_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static int __cdecl
YYJSONK_DirCompare(const void *lhs, const void *rhs)
{
    const char *const *left = (const char *const *)lhs;
    const char *const *right = (const char *const *)rhs;
    return strcmp(*left, *right);
}

static char **
YYJSONK_DirReadCommon(const char *path, int *count, bool full)
{
    UNICODE_STRING unicode_path;
    PWSTR storage = NULL;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK iosb;
    HANDLE handle = NULL;
    NTSTATUS status;
    void *buffer = NULL;
    BOOLEAN restart = TRUE;
    int idx = 0;
    int cap = 0;
    char **names = NULL;
    char *resolved = NULL;
    char *tmp_name = NULL;

    if (count) {
        *count = 0;
    }
    if (!path || !yyjsonk_file_api_allowed()) {
        return NULL;
    }

    resolved = (char *)malloc(YYJSONK_MAX_PATH);
    tmp_name = (char *)malloc(YYJSONK_MAX_PATH);
    if (!resolved || !tmp_name) {
        free(resolved);
        free(tmp_name);
        return NULL;
    }

    if (!yyjsonk_resolve_path_pcstr(path, resolved, YYJSONK_MAX_PATH)) {
        free(resolved);
        free(tmp_name);
        return NULL;
    }

    status = yyjsonk_path_to_unicode_pcstr(path, &unicode_path, &storage);
    if (!NT_SUCCESS(status)) {
        free(resolved);
        free(tmp_name);
        return NULL;
    }

    InitializeObjectAttributes(&attributes,
                               &unicode_path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateFile(&handle,
                          FILE_LIST_DIRECTORY | SYNCHRONIZE,
                          &attributes,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE |
                          FILE_SYNCHRONOUS_IO_NONALERT |
                          FILE_OPEN_FOR_BACKUP_INTENT,
                          NULL,
                          0);
    yyjsonk_free_unicode_buffer(storage);
    if (!NT_SUCCESS(status)) {
        free(resolved);
        free(tmp_name);
        return NULL;
    }

    buffer = malloc(65536);
    if (!buffer) {
        ZwClose(handle);
        free(resolved);
        free(tmp_name);
        return NULL;
    }

    while (TRUE) {
        FILE_BOTH_DIR_INFORMATION *entry;
        status = ZwQueryDirectoryFile(handle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &iosb,
                                      buffer,
                                      65536,
                                      FileBothDirectoryInformation,
                                      FALSE,
                                      NULL,
                                      restart);
        restart = FALSE;

        if (status == STATUS_NO_MORE_FILES) {
            break;
        }
        if (!NT_SUCCESS(status)) {
            yy_dir_free(names);
            free(buffer);
            ZwClose(handle);
            free(resolved);
            free(tmp_name);
            return NULL;
        }

        entry = (FILE_BOTH_DIR_INFORMATION *)buffer;
        for (;;) {
            ULONG char_count = entry->FileNameLength / sizeof(WCHAR);
            ULONG i;
            char *store_name;

            if (char_count >= YYJSONK_MAX_PATH) {
                char_count = YYJSONK_MAX_PATH - 1;
            }
            for (i = 0; i < char_count; i++) {
                WCHAR ch = entry->FileName[i];
                tmp_name[i] = (ch <= 0x7f) ? (char)ch : '?';
            }
            tmp_name[char_count] = '\0';

            if (strcmp(tmp_name, ".") != 0 && strcmp(tmp_name, "..") != 0) {
                if (idx + 2 > cap) {
                    int new_cap = (cap == 0) ? 16 : (cap * 2);
                    char **tmp_names = (char **)realloc(names, (size_t)new_cap * sizeof(char *));
                    if (!tmp_names) {
                        yy_dir_free(names);
                        free(buffer);
                        ZwClose(handle);
                        free(resolved);
                        free(tmp_name);
                        return NULL;
                    }
                    names = tmp_names;
                    cap = new_cap;
                }

                if (full) {
                    size_t base_len = strlen(resolved);
                    size_t name_len = strlen(tmp_name);
                    store_name = (char *)malloc(base_len + 1 + name_len + 1);
                    if (!store_name) {
                        yy_dir_free(names);
                        free(buffer);
                        ZwClose(handle);
                        free(resolved);
                        free(tmp_name);
                        return NULL;
                    }
                    yy_path_combine(store_name, resolved, tmp_name, NULL);
                } else {
                    store_name = yy_str_copy(tmp_name);
                    if (!store_name) {
                        yy_dir_free(names);
                        free(buffer);
                        ZwClose(handle);
                        free(resolved);
                        free(tmp_name);
                        return NULL;
                    }
                }

                names[idx++] = store_name;
            }

            if (entry->NextEntryOffset == 0) {
                break;
            }
            entry = (FILE_BOTH_DIR_INFORMATION *)((unsigned char *)entry + entry->NextEntryOffset);
        }
    }

    free(buffer);
    ZwClose(handle);
    free(resolved);
    free(tmp_name);

    if (names) {
        qsort(names, (size_t)idx, sizeof(char *), YYJSONK_DirCompare);
        names[idx] = NULL;
    }
    if (count) {
        *count = idx;
    }
    return names;
}

char **
yy_dir_read(const char *path, int *count)
{
    return YYJSONK_DirReadCommon(path, count, false);
}

char **
yy_dir_read_full(const char *path, int *count)
{
    return YYJSONK_DirReadCommon(path, count, true);
}

void
yy_dir_free(char **names)
{
    int idx;
    if (!names) {
        return;
    }
    for (idx = 0; names[idx] != NULL; idx++) {
        free(names[idx]);
    }
    free(names);
}

bool
yy_file_delete(const char *path)
{
    UNICODE_STRING unicode_path;
    PWSTR storage = NULL;
    OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;

    if (!path || !*path || !yyjsonk_file_api_allowed()) {
        return false;
    }

    status = yyjsonk_path_to_unicode_pcstr(path, &unicode_path, &storage);
    if (!NT_SUCCESS(status)) {
        return false;
    }

    InitializeObjectAttributes(&attributes,
                               &unicode_path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    status = ZwDeleteFile(&attributes);
    yyjsonk_free_unicode_buffer(storage);
    return NT_SUCCESS(status);
}
