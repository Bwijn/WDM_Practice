#include<ntddk.h>

#include<wdm.h>
//#include<winnt.h> // 博客原文是直接定义 process_xxxxx 并没有 添加这个头文件,这个头文件各种error
PVOID pRegistrationHandle;
// 要检测的进程名称
#define TARGET_PROCESS_1 "notepad.exe"
#define TARGET_PROCESS_2 "cheatengine-x86_64-SSE4-AVX2.exe"


#ifdef _WIN64
#define PROCESS_TERMINATE         0x0001  
#define PROCESS_VM_OPERATION      0x0008  
#define PROCESS_VM_READ           0x0010  
#define PROCESS_VM_WRITE          0x0020
typedef struct _LDR_DATA
{
	LIST_ENTRY listEntry;
	ULONG64 __Undefined1;
	ULONG64 __Undefined2;
	ULONG64 __Undefined3;
	ULONG64 NonPagedDebugInfo;
	ULONG64 DllBase;
	ULONG64 EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING path;
	UNICODE_STRING name;
	ULONG   Flags;
}LDR_DATA, * PLDR_DATA;
#else
typedef struct _LDR_DATA
{
	LIST_ENTRY listEntry;
	ULONG unknown1;
	ULONG unknown2;
	ULONG unknown3;
	ULONG unknown4;
	ULONG unknown5;
	ULONG unknown6;
	ULONG unknown7;
	UNICODE_STRING path;
	UNICODE_STRING name;
	ULONG   Flags;
}LDR_DATA, * PLDR_DATA;
#endif

//函数声明   才能用

NTKERNELAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);

VOID UnloadDriver(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);

	// unregister callbacks
	if (pRegistrationHandle != NULL) {
		ObUnRegisterCallbacks(pRegistrationHandle);
		pRegistrationHandle = NULL;
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProcessProtect: unload driver\n"));
}



// 操作这个进程之前的操作。
OB_PREOP_CALLBACK_STATUS  PobPreOperationCallback(
	PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION OperationInformation
) {

	// 判断对象类型 这一步如果不匹配直接 不看了，直接返回 SUCCESS;
	if (*PsProcessType != OperationInformation->ObjectType)
	{
		return OB_PREOP_SUCCESS;
	}
	//PLDR_DATA pld;//指向_LDR_DATA_TABLE_ENTRY结构体的指针

	UNREFERENCED_PARAMETER(RegistrationContext);
	UNREFERENCED_PARAMETER(OperationInformation);

	char certainProcess[40] = { 0 };
	strcpy(certainProcess, PsGetProcessImageFileName((PEPROCESS)OperationInformation->Object));

	// 不是被保护程序就直接走 return；OB_PREOP_SUCCESS value = 1 代表不相等 代表不是目标进程 直接放 此函数就结束了
	// 检查进程名称是否匹配
	if (strcmp(TARGET_PROCESS_1, certainProcess) != 0 &&
		strcmp(TARGET_PROCESS_2, certainProcess) != 0)
	{
		return;
	}
	if (strcmp(TARGET_PROCESS_1, certainProcess) == 0 )
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "notepad.exe processing\n");
	}
	//if (_stricmp(certainProcess, PROTECT_NAME))return OB_PREOP_SUCCESS;

	// 这里是else 也就是检测到保护进程的操作
	switch (OperationInformation->Operation)
	{
	case OB_OPERATION_HANDLE_DUPLICATE:
		OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
		break;

	case OB_OPERATION_HANDLE_CREATE:
	{
		// filter operation "OB_OPERATION_HANDLE_CREATE", and remove "PROCESS_TERMINAL"
		//if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TargetProcess protected: %s", certainProcess));
		OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;

		OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
		//}

	/*	if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & 1)
		{
		}*/
		break;
	}
	}

	return OB_PREOP_SUCCESS;


}

NTSTATUS InitObRegistration()
{
	OB_OPERATION_REGISTRATION oor;
	OB_CALLBACK_REGISTRATION ocr;

	PLDR_DATA pld;//指向_LDR_DATA_TABLE_ENTRY结构体的指针

	//初始化
	pRegistrationHandle = 0;
	RtlZeroMemory(&oor, sizeof(OB_OPERATION_REGISTRATION));
	RtlZeroMemory(&ocr, sizeof(OB_CALLBACK_REGISTRATION));


	//线程类型
	oor.ObjectType = PsProcessType;
	oor.Operations = OB_OPERATION_HANDLE_CREATE;
	oor.PreOperation = PobPreOperationCallback;



	ocr.Version = OB_FLT_REGISTRATION_VERSION;
	ocr.OperationRegistrationCount = 1;
	ocr.RegistrationContext = NULL;
	ocr.OperationRegistration = &oor;
	RtlInitUnicodeString(&ocr.Altitude, L"321000"); // 设置加载顺序 Altitude n. 海拔的意思，在这里是代表驱动加载顺序，就按照MSDN来操作就行。



	return ObRegisterCallbacks(&ocr, &pRegistrationHandle);
}
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT   DriverObject,
	_In_ PUNICODE_STRING  RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = UnloadDriver;

	//// 绕过MmVerifyCallbackFunction。
	PLDR_DATA pld;
	pld = (PLDR_DATA)DriverObject->DriverSection;
	pld->Flags |= 0x20;

	// 注册函数实施降权
	NTSTATUS Status = InitObRegistration();

	//通过句柄获取EProcess
	if (!NT_SUCCESS(Status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProcessProtect: ObRegisterCallbcks failed status 0x%x\n", Status));
		return Status;
	}


	return STATUS_SUCCESS;
}
