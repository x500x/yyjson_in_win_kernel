#include "yyjsonk_runtime.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct yyjsonk_alloc_header {
    size_t size;
} yyjsonk_alloc_header;

int _fltused = 0;

static size_t
yyjsonk_min_size(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

static void
yyjsonk_swap_bytes(char *lhs, char *rhs, size_t size, char *tmp)
{
    if (lhs == rhs || size == 0) {
        return;
    }
    RtlCopyMemory(tmp, lhs, size);
    RtlCopyMemory(lhs, rhs, size);
    RtlCopyMemory(rhs, tmp, size);
}

static void
yyjsonk_qsort_impl(char *base,
                   size_t count,
                   size_t size,
                   int (__cdecl *compare)(const void *, const void *),
                   char *tmp)
{
    size_t pivot_index;
    size_t last_index;
    size_t store_index;
    size_t idx;

    if (count < 2 || size == 0) {
        return;
    }

    pivot_index = count / 2;
    last_index = count - 1;
    yyjsonk_swap_bytes(base + pivot_index * size,
                       base + last_index * size,
                       size,
                       tmp);

    store_index = 0;
    for (idx = 0; idx < last_index; idx++) {
        if (compare(base + idx * size, base + last_index * size) < 0) {
            yyjsonk_swap_bytes(base + idx * size,
                               base + store_index * size,
                               size,
                               tmp);
            store_index++;
        }
    }

    yyjsonk_swap_bytes(base + store_index * size,
                       base + last_index * size,
                       size,
                       tmp);

    yyjsonk_qsort_impl(base, store_index, size, compare, tmp);
    yyjsonk_qsort_impl(base + (store_index + 1) * size,
                       count - store_index - 1,
                       size,
                       compare,
                       tmp);
}

void * __cdecl
malloc(size_t size)
{
    yyjsonk_alloc_header *header;
    size_t total;

    if (size == 0) {
        size = 1;
    }
    if (size > (SIZE_MAX - sizeof(*header))) {
        return NULL;
    }

    total = size + sizeof(*header);
    header = (yyjsonk_alloc_header *)ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                     total,
                                                     YYJSONK_POOL_TAG);
    if (!header) {
        return NULL;
    }
    header->size = size;
    return header + 1;
}

void * __cdecl
calloc(size_t count, size_t size)
{
    void *ptr;
    size_t total;

    if (count != 0 && size > (SIZE_MAX / count)) {
        return NULL;
    }
    total = count * size;
    ptr = malloc(total);
    if (ptr) {
        RtlZeroMemory(ptr, total);
    }
    return ptr;
}

void * __cdecl
realloc(void *ptr, size_t size)
{
    yyjsonk_alloc_header *header;
    void *new_ptr;

    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    header = ((yyjsonk_alloc_header *)ptr) - 1;
    new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    RtlCopyMemory(new_ptr, ptr, yyjsonk_min_size(header->size, size));
    free(ptr);
    return new_ptr;
}

void __cdecl
free(void *ptr)
{
    yyjsonk_alloc_header *header;

    if (!ptr) {
        return;
    }

    header = ((yyjsonk_alloc_header *)ptr) - 1;
    ExFreePool(header);
}

void __cdecl
qsort(void *base,
      size_t count,
      size_t size,
      int (__cdecl *compare)(const void *, const void *))
{
    char *tmp;

    if (!base || !compare || count < 2 || size == 0) {
        return;
    }

    tmp = (char *)malloc(size);
    if (!tmp) {
        return;
    }

    yyjsonk_qsort_impl((char *)base, count, size, compare, tmp);
    free(tmp);
}

void __cdecl
abort(void)
{
    yyjsonk_log_text(DPFLTR_ERROR_LEVEL, "[yyjson_kmdf] abort()\n");
    ExRaiseStatus(STATUS_ASSERTION_FAILURE);
}
