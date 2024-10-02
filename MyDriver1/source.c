//#include<	ntifs.h >
//#include<ntddk.h>

#include"HideProcess.h"
#include"OBlevel.h"
#include"PatternScan.h"
#include<wdm.h>
#include"constants.h"
VOID UnloadDriver(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotifyRoutineEx, TRUE);
	// unregister callbacks
	if (pRegistrationHandle != NULL) {
		ObUnRegisterCallbacks(pRegistrationHandle);
		pRegistrationHandle = NULL;
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProcessProtect: unload driver\n"));
}

void prepare(PDRIVER_OBJECT DriverObject) {

	// 绕过MmVerifyCallbackFunction。
	PLDR_DATA pld;
	pld = (PLDR_DATA)DriverObject->DriverSection;
	pld->Flags |= 0x20;


	ntosBase = GetModuleBaseAddress(DriverObject, ntImageName_W);
	if (ntosBase == NULL) {
		DbgPrint("Failed to find ntkrnlmp.exe base address.\n");
		return STATUS_UNSUCCESSFUL;
	}
	PAGE_base = (UCHAR*)((DWORD64)ntosBase + PAGE_RVA);  // 强制转换为 UCHAR*
	PVOID foundAddress = FindPattern(PAGE_base, PAGE_SIZE, pattern, mask);


	if (foundAddress != NULL) {
		DbgPrint("Pattern found at address: %p\n", foundAddress);
		PsTerminateProcess = foundAddress;

	}
	else {
		DbgPrint("Pattern not found.\n");
		return STATUS_UNSUCCESSFUL;
	}
}


NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT   DriverObject,
	_In_ PUNICODE_STRING  RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = UnloadDriver;


	//前置准备 1.找到ntoskrl地址找到函数地址 2 .设置ldr 绕过签名检测 3. 
	 //PsLoadedModuleList = *((PLDR_DATA_TABLE_ENTRY*)(DriverObject->DriverSection));
	prepare(DriverObject);



	//1. 隐藏 Pid
	//NTSTATUS HideStatus = HideProcess();
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotifyRoutineEx, FALSE);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to register process notify routine!\n"));
		return status;
	}




	// 2.注册函数实施降权
	NTSTATUS Status = InitObRegistration();
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Error status: 0x%08X\n", Status)); // 打印十六进制格式的状态码
	//通过句柄获取EProcess
	if (!NT_SUCCESS(Status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProcessProtect: ObRegisterCallbcks failed status 0x%x\n", Status));
		return Status;
	}


	return STATUS_SUCCESS;
}
