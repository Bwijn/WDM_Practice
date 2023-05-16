//#include<Windows.h>
#include<wdm.h>


 VOID UnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    KdPrint(("DriverUnload called\n"));

    //UnregisterCallbacks();

    KdPrint(("Driver unloaded successfully\n"));
}


NTSTATUS InitDeviceObject(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject;


    RtlInitUnicodeString(&deviceName, L"\\Device\\MyDriver");

 
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    //deviceObject->DriverUnload = UnloadDriver;

    KdPrint(("Device object created successfully\n"));

    return STATUS_SUCCESS;
}

VOID RegisterCallbacks()
{
    
}



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
)
{


    //初始化这个结构体

    OB_OPERATION_REGISTRATION obOperationRegistrations;
    obOperationRegistrations.ObjectType = PsProcessType;
    obOperationRegistrations.Operations |= OB_OPERATION_HANDLE_CREATE;
    obOperationRegistrations.Operations |= OB_OPERATION_HANDLE_DUPLICATE;
    obOperationRegistrations.PreOperation = PreOperationCallback;
    obOperationRegistrations.PostOperation = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("DriverEntry called\n"));

    DriverObject->DriverUnload = UnloadDriver;

    NTSTATUS status = InitDeviceObject(DriverObject);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to initialize device object (Status: 0x%X)\n", status));
        KdPrint(("Failed to initialize device object (Status: 0x%X)\n", status));
        return status;
    }

    RegisterCallbacks();

    KdPrint(("Driver initialized successfully\n"));

    return STATUS_SUCCESS;
}
