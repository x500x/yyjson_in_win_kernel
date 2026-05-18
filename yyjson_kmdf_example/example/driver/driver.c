#include <ntddk.h>
#include <wdf.h>

#include "example_sample.h"
#include "kprintf.h"
#include "yyjsonk_runtime.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD example_driver_unload;

VOID
example_driver_unload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
    kprintf("%s", "unloading driver\n");
    yyjsonk_runtime_uninit();
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = example_driver_unload;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = yyjsonk_runtime_init(RegistryPath);
    if (!NT_SUCCESS(status)) {
        yyjsonk_runtime_uninit();
        return status;
    }

    status = yyjsonk_run_example();
    if (!NT_SUCCESS(status)) {
        yyjsonk_runtime_uninit();
        return status;
    }

    return STATUS_SUCCESS;
}
