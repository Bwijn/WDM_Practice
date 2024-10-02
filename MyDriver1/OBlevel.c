#include<	ntifs.h >
#include"OBlevel.h"
#include<ntddk.h>
#include<wdm.h>
#include"HideProcess.h"

//#include<winnt.h> // 博客原文是直接定义 process_xxxxx 并没有 添加这个头文件,这个头文件各种error
// 要检测的进程名称
//#define TARGET_PROCESS_1 "notepad.exe"
//#define TARGET_PROCESS_2 "cheatengine-x86_64-SSE4-AVX2.exe"


//函数声明   才能用

NTKERNELAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);



// 操作这个进程之前的操作。
OB_PREOP_CALLBACK_STATUS  PobPreOperationCallback(
	PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION OperationInformation
) {
	UNREFERENCED_PARAMETER(RegistrationContext);
	UNREFERENCED_PARAMETER(OperationInformation);

	// 判断对象类型 这一步如果不匹配直接 不看了，直接返回 SUCCESS;
	if (*PsProcessType != OperationInformation->ObjectType)
	{
		return OB_PREOP_SUCCESS;
	}

	// 获取线程对应的进程 PEPROCESS
	//PEPROCESS	pEprocess = IoThreadToProcess((PEPROCESS)OperationInformation->Object);
	//当前系统操作的object
	char certainProcess[40] = { 0 };
	strcpy(certainProcess, PsGetProcessImageFileName((PEPROCESS)OperationInformation->Object));
	// 检查进程名称是否匹配
	for (SIZE_T i = 0; i < imageCount; i++) {
		if (strcmp(imageNames[i], certainProcess) == 0) {

			// 这里是else 也就是检测到保护进程的操作
			switch (OperationInformation->Operation)
			{
			case OB_OPERATION_HANDLE_DUPLICATE:
				OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;

			case OB_OPERATION_HANDLE_CREATE:
			{
				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;

			}
			}


			HANDLE threadId = PsGetThreadId(PsGetCurrentThread());

			// 获取当前线程的进程ID
			HANDLE processId = PsGetThreadProcessId(PsGetCurrentThread());

			// 根据进程ID查找PEPROCESS结构
			PEPROCESS currentProcess;
			NTSTATUS status = PsLookupProcessByProcessId(processId, &currentProcess);

			if (NT_SUCCESS(status)) {
				// 获取调用者的进程名称
				char callerProcessName[40] = { 0 };
				strcpy(callerProcessName, PsGetProcessImageFileName(currentProcess));
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
					"[%s] is being processed by [%s]\n", imageNames[i], callerProcessName);
			}
		}



	}
	return OB_PREOP_SUCCESS;

}

NTSTATUS InitObRegistration()
{
	OB_OPERATION_REGISTRATION oor;
	OB_CALLBACK_REGISTRATION ocr;

	//PLDR_DATA pld;//指向_LDR_DATA_TABLE_ENTRY结构体的指针

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