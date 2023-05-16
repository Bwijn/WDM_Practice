﻿#include<wdm.h>

PVOID pRegistrationHandle;
# define PROTECT_NAME "myEx86_64.exe"
#ifdef _WIN64
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

// 操作这个进程之前的操作。
OB_PREOP_CALLBACK_STATUS  PobPreOperationCallback(
	PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION pOperationInformation
) {

	// 判断对象类型 
	if (*PsProcessType != pOperationInformation->ObjectType)
	{
		return OB_PREOP_SUCCESS;
	}

	UNREFERENCED_PARAMETER(RegistrationContext);
	HANDLE pid = PsGetProcessId((PEPROCESS)pOperationInformation->Object);
	char szProcName[16] = { 0 };
	UNREFERENCED_PARAMETER(RegistrationContext);
	strcpy(szProcName, GetProcessImageNameByProcessID((ULONG)pid));

	if (!_stricmp(szProcName, PROTECT_NAME))//等于0 也就是字符串相等时
	{

		if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_TERMINATE) == PROCESS_TERMINATE)
		{
			//Terminate the process, such as by calling the user-mode TerminateProcess routine..
			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
		}
		if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_OPERATION) == PROCESS_VM_OPERATION)
		{
			//Modify the address space of the process, such as by calling the user-mode WriteProcessMemory and VirtualProtectEx routines.
			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_OPERATION;
		}
		if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_READ) == PROCESS_VM_READ)
		{
			//Read to the address space of the process, such as by calling the user-mode ReadProcessMemory routine.
			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_READ;
		}
		if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_WRITE) == PROCESS_VM_WRITE)
		{
			//Write to the address space of the process, such as by calling the user-mode WriteProcessMemory routine.
			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
		}
	}


	//获取该进程结构对象的名称
	//pEProcess = (PEPROCESS)pOperationInformation->Object;
	//PUCHAR pProcessName = PsGetProcessImageFileName(pEProcess);

	// 判断是否为保护进程，不是则放行
	//if (NULL != pProcessName)
	//{
	//	if (0 != _stricmp(pProcessName, PROTECT_NAME))
	//	{
	//		return OB_PREOP_SUCCESS;
	//	}
	//}

	// 判断操作类型,如果该句柄是终止操作，则拒绝该操作
	//switch (pOperationInformation->Operation)
	//{
	//case OB_OPERATION_HANDLE_DUPLICATE:
	//	break;
	//	PROCESS_TERMINATE
	//case OB_OPERATION_HANDLE_CREATE:
	//	{
	//		//如果要结束进程,进程管理器结束进程发送0x1001，taskkill指令结束进程发送0x0001，taskkil加/f参数结束进程发送0x1401
	//		int code = pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess;
	//		if ((code == PROCESS_TERMINATE_0) || (code == PROCESS_TERMINATE_1) || (code == PROCESS_KILL_F))
	//		{
	//			//给进程赋予新权限
	//			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
	//			DbgPrint("拒绝执行当前操作");
	//		}
	//		if (code == PROCESS_TERMINATE_2)
	//			pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = STANDARD_RIGHTS_ALL;
	//		break;
	//	}
	//}
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
	oor.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	oor.PreOperation = PobPreOperationCallback;



	ocr.Version = OB_FLT_REGISTRATION_VERSION;
	ocr.OperationRegistrationCount = 0;
	ocr.RegistrationContext = NULL;
	ocr.OperationRegistration = &oor;
	RtlInitUnicodeString(&ocr.Altitude, L"321000"); // 设置加载顺序



	return ObRegisterCallbacks(&ocr, &pRegistrationHandle);
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
