//#include<Windows.h>
#include<wdm.h>

// 0‡50‹50…90…40…60ˆ00ˆ4¨°0ˆ40…90ˆ80‰10†20…40‡80‹5
 VOID UnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    KdPrint(("DriverUnload called\n"));

    //UnregisterCallbacks();

    KdPrint(("Driver unloaded successfully\n"));
}

// 0…60‹10‡80†40†30…40‡7¨¨¡À0†00…90ˆ80ˆ3¨®
NTSTATUS InitDeviceObject(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject;

    // 0…60‹10‡80†40†30…40‡7¨¨¡À0†00‡10‹40…60‡4
    RtlInitUnicodeString(&deviceName, L"\\Device\\MyDriver");

    // 0…70…70†5¡§0‡7¨¨¡À0†00…90ˆ80ˆ3¨®
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

    // 0‡7¨¨0‰00‡10‡7¨¨¡À0†00…90ˆ80ˆ3¨®0…80‡20…70„70†8¨ª0†20…40‡80‹5
    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    //deviceObject->DriverUnload = UnloadDriver;

    KdPrint(("Device object created successfully\n"));

    return STATUS_SUCCESS;
}

// ¡Á0„40…5¨¢0†30‰10…8¡Â0†20…40‡80‹5
VOID RegisterCallbacks()
{
    // 0ˆ80‰30…70‡90…70„7¡Á0„40…5¨¢0ˆ4¨¨0ˆ60„90…80‡20†30‰10…8¡Â0†20…40‡80‹5
    // 0†80‹50‡60Š40„50†2IoRegisterShutdownNotification, PsSetLoadImageNotifyRoutine, ExRegisterCallback 0…80‡6
}



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("DriverEntry called\n"));

    // 0‡7¨¨0‰00‡10‡50‹50…90…40…60ˆ00ˆ4¨°0…80‡20…70„70†8¨ª0†20…40‡80‹5
    DriverObject->DriverUnload = UnloadDriver;

    // 0…60‹10‡80†40†30…40‡7¨¨¡À0†00…90ˆ80ˆ3¨®
    NTSTATUS status = InitDeviceObject(DriverObject);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Failed to initialize device object (Status: 0x%X)\n", status));
        return status;
    }

    // ¡Á0„40…5¨¢0†30‰10…8¡Â0†20…40‡80‹5
    RegisterCallbacks();

    KdPrint(("Driver initialized successfully\n"));

    return STATUS_SUCCESS;
}
