#include "yyjsonk_runtime.h"
#include "stdio.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static FILE g_yyjsonk_stdout_obj;
static FILE g_yyjsonk_stderr_obj;
FILE *stdout = &g_yyjsonk_stdout_obj;
FILE *stderr = &g_yyjsonk_stderr_obj;

static FAST_MUTEX g_yyjsonk_stdio_lock;
static LIST_ENTRY g_yyjsonk_open_files;
static BOOLEAN g_yyjsonk_stdio_ready = FALSE;

static void
YYJSONK_RegisterFile(FILE *file)
{
    ExAcquireFastMutex(&g_yyjsonk_stdio_lock);
    if (!file->registered) {
        InsertTailList(&g_yyjsonk_open_files, &file->link);
        file->registered = TRUE;
    }
    ExReleaseFastMutex(&g_yyjsonk_stdio_lock);
}

static void
YYJSONK_UnregisterFile(FILE *file)
{
    ExAcquireFastMutex(&g_yyjsonk_stdio_lock);
    if (file->registered) {
        RemoveEntryList(&file->link);
        InitializeListHead(&file->link);
        file->registered = FALSE;
    }
    ExReleaseFastMutex(&g_yyjsonk_stdio_lock);
}

static int
YYJSONK_WriteConsole(FILE *file, const char *data, size_t len)
{
    char chunk[256];
    size_t ofs = 0;
    ULONG level = (file == stderr) ? DPFLTR_ERROR_LEVEL : DPFLTR_INFO_LEVEL;

    while (ofs < len) {
        size_t copy_len = len - ofs;
        if (copy_len >= sizeof(chunk)) {
            copy_len = sizeof(chunk) - 1;
        }
        memcpy(chunk, data + ofs, copy_len);
        chunk[copy_len] = '\0';
        yyjsonk_log_text(level, chunk);
        ofs += copy_len;
    }
    return (int)len;
}

static int
YYJSONK_WriteDiskBytes(FILE *file, const void *data, size_t len)
{
    const unsigned char *src = (const unsigned char *)data;
    size_t total = 0;

    if (!file || file->kind != YYJSONK_FILE_KIND_DISK || !file->can_write) {
        if (file) {
            file->error = TRUE;
        }
        return -1;
    }

    while (total < len) {
        IO_STATUS_BLOCK iosb;
        ULONG chunk = (ULONG)((len - total > MAXULONG) ? MAXULONG : (len - total));
        LARGE_INTEGER offset = file->offset;
        NTSTATUS status = ZwWriteFile(file->handle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &iosb,
                                      (void *)(src + total),
                                      chunk,
                                      &offset,
                                      NULL);
        if (!NT_SUCCESS(status)) {
            file->error = TRUE;
            return -1;
        }
        file->offset.QuadPart += (LONGLONG)iosb.Information;
        total += (size_t)iosb.Information;
        if (iosb.Information < chunk) {
            break;
        }
    }

    return (int)total;
}

int
yyjsonk_file_write_buffer(FILE *file, const char *data, size_t len)
{
    if (!file || !data) {
        return -1;
    }
    if (file->kind == YYJSONK_FILE_KIND_CONSOLE) {
        return YYJSONK_WriteConsole(file, data, len);
    }
    return YYJSONK_WriteDiskBytes(file, data, len);
}

static int
YYJSONK_FlushOne(FILE *file)
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    if (!file || file->kind != YYJSONK_FILE_KIND_DISK || !file->can_write) {
        return 0;
    }

    status = ZwFlushBuffersFile(file->handle, &iosb);
    if (!NT_SUCCESS(status)) {
        file->error = TRUE;
        return EOF;
    }
    return 0;
}

static int
YYJSONK_ParseMode(const char *mode, BOOLEAN *can_read, BOOLEAN *can_write)
{
    BOOLEAN read_mode = FALSE;
    BOOLEAN write_mode = FALSE;

    if (!mode || !can_read || !can_write) {
        return 0;
    }

    while (*mode) {
        if (*mode == 'r') {
            read_mode = TRUE;
        } else if (*mode == 'w') {
            write_mode = TRUE;
        }
        mode++;
    }

    *can_read = read_mode;
    *can_write = write_mode;
    return read_mode || write_mode;
}

VOID
yyjsonk_stdio_runtime_init(VOID)
{
    if (g_yyjsonk_stdio_ready) {
        return;
    }

    ExInitializeFastMutex(&g_yyjsonk_stdio_lock);
    InitializeListHead(&g_yyjsonk_open_files);

    RtlZeroMemory(&g_yyjsonk_stdout_obj, sizeof(g_yyjsonk_stdout_obj));
    g_yyjsonk_stdout_obj.kind = YYJSONK_FILE_KIND_CONSOLE;
    g_yyjsonk_stdout_obj.can_write = TRUE;
    InitializeListHead(&g_yyjsonk_stdout_obj.link);

    RtlZeroMemory(&g_yyjsonk_stderr_obj, sizeof(g_yyjsonk_stderr_obj));
    g_yyjsonk_stderr_obj.kind = YYJSONK_FILE_KIND_CONSOLE;
    g_yyjsonk_stderr_obj.can_write = TRUE;
    InitializeListHead(&g_yyjsonk_stderr_obj.link);

    g_yyjsonk_stdio_ready = TRUE;
}

VOID
yyjsonk_stdio_runtime_uninit(VOID)
{
    if (!g_yyjsonk_stdio_ready) {
        return;
    }

    ExAcquireFastMutex(&g_yyjsonk_stdio_lock);
    while (!IsListEmpty(&g_yyjsonk_open_files)) {
        PLIST_ENTRY entry = RemoveHeadList(&g_yyjsonk_open_files);
        FILE *file = CONTAINING_RECORD(entry, FILE, link);
        file->registered = FALSE;
        if (file->handle) {
            ZwClose(file->handle);
            file->handle = NULL;
        }
        free(file);
    }
    ExReleaseFastMutex(&g_yyjsonk_stdio_lock);
    g_yyjsonk_stdio_ready = FALSE;
}

FILE * __cdecl
fopen(const char *path, const char *mode)
{
    BOOLEAN can_read = FALSE;
    BOOLEAN can_write = FALSE;
    FILE *file;
    UNICODE_STRING unicode_path;
    PWSTR unicode_storage = NULL;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK iosb;
    ACCESS_MASK desired_access;
    ULONG disposition;
    ULONG options;
    NTSTATUS status;

    yyjsonk_stdio_runtime_init();

    if (!yyjsonk_file_api_allowed()) {
        return NULL;
    }
    if (!YYJSONK_ParseMode(mode, &can_read, &can_write)) {
        return NULL;
    }
    if (!path) {
        return NULL;
    }

    file = (FILE *)malloc(sizeof(*file));
    if (!file) {
        return NULL;
    }
    RtlZeroMemory(file, sizeof(*file));
    InitializeListHead(&file->link);
    file->kind = YYJSONK_FILE_KIND_DISK;
    file->can_read = can_read;
    file->can_write = can_write;

    status = yyjsonk_path_to_unicode_pcstr(path, &unicode_path, &unicode_storage);
    if (!NT_SUCCESS(status)) {
        yyjsonk_logf(DPFLTR_INFO_LEVEL,
                     "[yyjson_kmdf][FILE] path convert failed path=%s status=0x%08X\n",
                     path,
                     status);
        free(file);
        return NULL;
    }

    InitializeObjectAttributes(&attributes,
                               &unicode_path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    desired_access = SYNCHRONIZE;
    if (can_read) {
        desired_access |= FILE_READ_DATA | FILE_READ_ATTRIBUTES;
    }
    if (can_write) {
        desired_access |= FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES;
    }

    disposition = can_write ? FILE_OVERWRITE_IF : FILE_OPEN;
    options = FILE_NON_DIRECTORY_FILE |
              FILE_SYNCHRONOUS_IO_NONALERT |
              FILE_OPEN_FOR_BACKUP_INTENT;

    status = ZwCreateFile(&file->handle,
                          desired_access,
                          &attributes,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          disposition,
                          options,
                          NULL,
                          0);
    yyjsonk_free_unicode_buffer(unicode_storage);
    if (!NT_SUCCESS(status)) {
        yyjsonk_logf(DPFLTR_INFO_LEVEL,
                     "[yyjson_kmdf][FILE] open failed path=%s status=0x%08X\n",
                     path,
                     status);
        free(file);
        return NULL;
    }

    YYJSONK_RegisterFile(file);
    return file;
}

errno_t __cdecl
fopen_s(FILE **file, const char *path, const char *mode)
{
    if (!file) {
        return 1;
    }
    *file = fopen(path, mode);
    return (*file != NULL) ? 0 : 1;
}

size_t __cdecl
fread(void *buf, size_t size, size_t count, FILE *file)
{
    size_t requested;
    size_t total = 0;

    if (!buf || !file) {
        return 0;
    }
    if (size == 0 || count == 0) {
        return 0;
    }
    if (!yyjsonk_file_api_allowed()) {
        file->error = TRUE;
        return 0;
    }
    if (file->kind != YYJSONK_FILE_KIND_DISK || !file->can_read) {
        file->error = TRUE;
        return 0;
    }
    if (count > (SIZE_MAX / size)) {
        file->error = TRUE;
        return 0;
    }

    requested = size * count;
    while (total < requested) {
        IO_STATUS_BLOCK iosb;
        ULONG chunk = (ULONG)((requested - total > MAXULONG) ? MAXULONG : (requested - total));
        LARGE_INTEGER offset = file->offset;
        NTSTATUS status = ZwReadFile(file->handle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &iosb,
                                     (unsigned char *)buf + total,
                                     chunk,
                                     &offset,
                                     NULL);

        if (status == STATUS_END_OF_FILE) {
            file->eof = TRUE;
            break;
        }
        if (!NT_SUCCESS(status)) {
            file->error = TRUE;
            yyjsonk_logf(DPFLTR_ERROR_LEVEL,
                         "[yyjson_kmdf][FILE] read failed status=0x%08X offset=%llu size=%lu\n",
                         status,
                         (unsigned long long)offset.QuadPart,
                         chunk);
            break;
        }
        if (iosb.Information == 0) {
            file->eof = TRUE;
            break;
        }

        file->offset.QuadPart += (LONGLONG)iosb.Information;
        total += (size_t)iosb.Information;
        if (iosb.Information < chunk) {
            file->eof = TRUE;
            break;
        }
    }

    return total / size;
}

size_t __cdecl
fread_s(void *buf, size_t buf_size, size_t elem_size, size_t count, FILE *file)
{
    size_t requested;

    if (!buf) {
        return 0;
    }
    if (elem_size == 0 || count == 0) {
        return 0;
    }
    if (count > (SIZE_MAX / elem_size)) {
        if (file) {
            file->error = TRUE;
        }
        return 0;
    }

    requested = elem_size * count;
    if (buf_size < requested) {
        if (file) {
            file->error = TRUE;
        }
        return 0;
    }
    return fread(buf, elem_size, count, file);
}

size_t __cdecl
fwrite(const void *buf, size_t size, size_t count, FILE *file)
{
    size_t requested;
    int written;

    if (!buf || !file) {
        return 0;
    }
    if (size == 0 || count == 0) {
        return 0;
    }
    if (!yyjsonk_file_api_allowed()) {
        file->error = TRUE;
        return 0;
    }
    if (count > (SIZE_MAX / size)) {
        file->error = TRUE;
        return 0;
    }

    requested = size * count;
    written = yyjsonk_file_write_buffer(file, (const char *)buf, requested);
    if (written < 0) {
        return 0;
    }
    return (size_t)written / size;
}

int __cdecl
fclose(FILE *file)
{
    if (!file) {
        return EOF;
    }
    if (file == stdout || file == stderr) {
        return 0;
    }

    YYJSONK_UnregisterFile(file);
    if (file->handle) {
        ZwClose(file->handle);
        file->handle = NULL;
    }
    free(file);
    return 0;
}

int __cdecl
fseek(FILE *file, long offset, int origin)
{
    LARGE_INTEGER base;

    if (!file) {
        return EOF;
    }
    if (file->kind != YYJSONK_FILE_KIND_DISK) {
        return EOF;
    }
    if (!yyjsonk_file_api_allowed()) {
        file->error = TRUE;
        return EOF;
    }

    if (origin == SEEK_SET) {
        base.QuadPart = 0;
    } else if (origin == SEEK_CUR) {
        base = file->offset;
    } else if (origin == SEEK_END) {
        FILE_STANDARD_INFORMATION info;
        IO_STATUS_BLOCK iosb;
        NTSTATUS status = ZwQueryInformationFile(file->handle,
                                                 &iosb,
                                                 &info,
                                                 sizeof(info),
                                                 FileStandardInformation);
        if (!NT_SUCCESS(status)) {
            file->error = TRUE;
            return EOF;
        }
        base = info.EndOfFile;
    } else {
        file->error = TRUE;
        return EOF;
    }

    file->offset.QuadPart = base.QuadPart + offset;
    if (file->offset.QuadPart < 0) {
        file->offset.QuadPart = 0;
    }
    file->eof = FALSE;
    return 0;
}

long __cdecl
ftell(FILE *file)
{
    if (!file || file->kind != YYJSONK_FILE_KIND_DISK) {
        return -1L;
    }
    if (file->offset.QuadPart > LONG_MAX) {
        return -1L;
    }
    return (long)file->offset.QuadPart;
}

int __cdecl
fflush(FILE *file)
{
    if (!g_yyjsonk_stdio_ready) {
        return 0;
    }

    if (!file) {
        PLIST_ENTRY entry;
        int rc = 0;
        ExAcquireFastMutex(&g_yyjsonk_stdio_lock);
        for (entry = g_yyjsonk_open_files.Flink; entry != &g_yyjsonk_open_files; entry = entry->Flink) {
            FILE *open_file = CONTAINING_RECORD(entry, FILE, link);
            if (YYJSONK_FlushOne(open_file) != 0) {
                rc = EOF;
            }
        }
        ExReleaseFastMutex(&g_yyjsonk_stdio_lock);
        return rc;
    }

    if (file == stdout || file == stderr) {
        return 0;
    }
    return YYJSONK_FlushOne(file);
}
