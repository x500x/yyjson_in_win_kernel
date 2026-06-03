#include <ntddk.h>
#include <wdf.h>

#include "test_registry.h"
#include "yyjsonk_test_support.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP yyjson_kmdf_tests_driver_cleanup;
EVT_WDF_DRIVER_UNLOAD yyjson_kmdf_tests_driver_unload;

VOID
yyjson_kmdf_tests_driver_cleanup(
    _In_ WDFOBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    yyjsonk_runtime_uninit();
}

VOID
yyjson_kmdf_tests_driver_unload(
    _In_ WDFDRIVER Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
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
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = yyjson_kmdf_tests_driver_unload;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = yyjson_kmdf_tests_driver_cleanup;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
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

    yyjsonk_set_test_root_pcstr(YYJSONK_DEFAULT_TEST_ROOT);
    if (yyjsonk_run_all_tests() != 0) {
        yyjsonk_runtime_uninit();
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
