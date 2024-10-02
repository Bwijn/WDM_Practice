#include<	ntifs.h >
#include"OBlevel.h"
#include<ntddk.h>
#include<wdm.h>
#include"HideProcess.h"

//#include<winnt.h> // ����ԭ����ֱ�Ӷ��� process_xxxxx ��û�� ������ͷ�ļ�,���ͷ�ļ�����error
// Ҫ���Ľ�������
//#define TARGET_PROCESS_1 "notepad.exe"
//#define TARGET_PROCESS_2 "cheatengine-x86_64-SSE4-AVX2.exe"


//��������   ������

NTKERNELAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);



// �����������֮ǰ�Ĳ�����
OB_PREOP_CALLBACK_STATUS  PobPreOperationCallback(
	PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION OperationInformation
) {
	UNREFERENCED_PARAMETER(RegistrationContext);
	UNREFERENCED_PARAMETER(OperationInformation);

	// �ж϶������� ��һ�������ƥ��ֱ�� �����ˣ�ֱ�ӷ��� SUCCESS;
	if (*PsProcessType != OperationInformation->ObjectType)
	{
		return OB_PREOP_SUCCESS;
	}

	// ��ȡ�̶߳�Ӧ�Ľ��� PEPROCESS
	//PEPROCESS	pEprocess = IoThreadToProcess((PEPROCESS)OperationInformation->Object);
	//��ǰϵͳ������object
	char certainProcess[40] = { 0 };
	strcpy(certainProcess, PsGetProcessImageFileName((PEPROCESS)OperationInformation->Object));
	// �����������Ƿ�ƥ��
	for (SIZE_T i = 0; i < imageCount; i++) {
		if (strcmp(imageNames[i], certainProcess) == 0) {

			// ������else Ҳ���Ǽ�⵽�������̵Ĳ���
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

			// ��ȡ��ǰ�̵߳Ľ���ID
			HANDLE processId = PsGetThreadProcessId(PsGetCurrentThread());

			// ���ݽ���ID����PEPROCESS�ṹ
			PEPROCESS currentProcess;
			NTSTATUS status = PsLookupProcessByProcessId(processId, &currentProcess);

			if (NT_SUCCESS(status)) {
				// ��ȡ�����ߵĽ�������
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

	//PLDR_DATA pld;//ָ��_LDR_DATA_TABLE_ENTRY�ṹ���ָ��

	//��ʼ��
	pRegistrationHandle = 0;
	RtlZeroMemory(&oor, sizeof(OB_OPERATION_REGISTRATION));
	RtlZeroMemory(&ocr, sizeof(OB_CALLBACK_REGISTRATION));


	//�߳�����
	oor.ObjectType = PsProcessType;
	oor.Operations = OB_OPERATION_HANDLE_CREATE;
	oor.PreOperation = PobPreOperationCallback;



	ocr.Version = OB_FLT_REGISTRATION_VERSION;
	ocr.OperationRegistrationCount = 1;
	ocr.RegistrationContext = NULL;
	ocr.OperationRegistration = &oor;
	RtlInitUnicodeString(&ocr.Altitude, L"321000"); // ���ü���˳�� Altitude n. ���ε���˼���������Ǵ�����������˳�򣬾Ͱ���MSDN���������С�



	return ObRegisterCallbacks(&ocr, &pRegistrationHandle);
}