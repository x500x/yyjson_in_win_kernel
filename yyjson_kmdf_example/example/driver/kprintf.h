#ifndef YYJSONK_EXAMPLE_KPRINTF_H
#define YYJSONK_EXAMPLE_KPRINTF_H

#include <ntddk.h>

#ifdef DBG
#define kprintf(Format, ...) DbgPrintEx(0, 0, "TestDriver : " Format, __VA_ARGS__)
#else
#define kprintf(Format, ...)
#endif

#endif
