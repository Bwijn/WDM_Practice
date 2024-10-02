#include<ntifs.h>
#include"HideProcess.h"
#include"OBlevel.h"
#include"stringTools.h"
#include"PatternScan.h"	
#include<ntddk.h>
#include<wdm.h>
#include"constants.h"
//PCWSTR imageNames[] = { L"uxeatengine-x86_64.vmp.exe" ,L"notepad.exe" };
const CHAR* imageNames[] = { "uxeatengine-x86_64.vmp.exe", "notepad.exe" };
const WCHAR* imageNamesW[] = { L"uxeatengine-x86_64.vmp.exe", L"notepad.exe" };
SIZE_T imageCount = sizeof(imageNames) / sizeof(imageNames[0]);

ULONG ulProcessImageNameOffset = 0x5a8; // ƫ��������������
ULONG ulProcessIDOffset = 0x440; // ƫ���������� ID
ULONG ulNextProcessOffset = 0x448; // ƫ����������λ��

// ȫ������ͷ�����ڱ��������Ϣ
LIST_ENTRY processListHead;



// ���ڱ���ԭʼPID
typedef struct _PROCESS_INFO {
	char imageName[40];
	HANDLE ProcessId;
	HANDLE OriginalPid;
	LIST_ENTRY ListEntry;  // ���ڽ��ýṹ�����ӵ�������
} PROCESS_INFO, * PPROCESS_INFO;


// ���Ҳ�ɾ��������Ϣ
//PPROCESS_INFO FindOriginalProcessInfo(PEPROCESS Process) {
//	PLIST_ENTRY entry;
//	PPROCESS_INFO pProcessInfo;
//	for (entry = processListHead.Flink; entry != &processListHead; entry = entry->Flink) {
//		pProcessInfo = CONTAINING_RECORD(entry, PROCESS_INFO, ListEntry);
//		if (pProcessInfo->ProcessId == ProcessId) {
//			RemoveEntryList(entry);
//			return pProcessInfo;
//		}
//	}
//	return NULL;
//}

// ��¼����ԭ����Ϣ
VOID AddProcessInfo(PEPROCESS Process, HANDLE OriginalPid) {
	PPROCESS_INFO pProcessInfo = (PPROCESS_INFO)ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), 'pinf');

	ULONG_PTR imageNameAddress = (ULONG_PTR)Process + 0x5a8; // ImageName ��ƫ��
	//PCHAR processImageName = (PCHAR)imageNameAddress;
	if (pProcessInfo) {
		// ��� imageName�������ַ�����
	 // ʹ�� strncpy �����ַ���������Խ��
		strncpy(pProcessInfo->imageName, (PCHAR)imageNameAddress, sizeof(pProcessInfo->imageName) - 1);
		pProcessInfo->ProcessId = OriginalPid;
		pProcessInfo->OriginalPid = OriginalPid;
		InsertTailList(&processListHead, &pProcessInfo->ListEntry);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "&pProcessInfo->ListEntry -- [%p] \n", &pProcessInfo->ListEntry));

	}
}

// ���Ҳ��ظ���صĽ���
BOOLEAN RecoverProcess(PEPROCESS Process) {
	//id��=4ֱ��pass
	DWORD64 pid = (DWORD64)Process + ulProcessIDOffset;
	if (*(DWORD64*)pid != 4)
	{
		return FALSE;
	}

	ULONG_PTR imageNameAddress = (ULONG_PTR)Process + 0x5a8; // ImageName ��ƫ��
	PCHAR processImageName = (PCHAR)imageNameAddress;

	// ���� ANSI �ַ����б� imageNames ���бȽ�
	for (SIZE_T i = 0; i < imageCount; i++) {
		// �ȽϽ��������б��е��ַ��������Դ�Сд
		if (_stricmp(processImageName, imageNames[i]) == 0) {
			//ȥ�����ԭʼ��Ϣ ���Ƿ��м�¼

			PLIST_ENTRY entry;
			PPROCESS_INFO pProcessInfo;
			//entry = processListHead.Flink;
			for (entry = processListHead.Flink; entry != &processListHead; entry = entry->Flink) {
				// �ȱ�����һ���ڵ�ĵ�ַ����ֹ��ǰ�ڵ㱻ɾ��������ָ����Ч
				//nextEntry = entry->Flink;
				//���������ƫ��ȡֵCONTAINING_RECORD
				pProcessInfo = CONTAINING_RECORD(entry, PROCESS_INFO, ListEntry);
				if (CompareStrings(processImageName, imageNames[i], FALSE, FALSE, TRUE))
				{
					//�м�¼��ɾ�ڵ��ͷ��ڴ�
					//1.�ָ�
					ULONG_PTR pidAddress = (ULONG_PTR)Process + ulProcessIDOffset;
					*(DWORD64*)pidAddress = pProcessInfo->OriginalPid;
					//2.�ͷ�list�ڴ� ɾ���ڵ�
					RemoveEntryList(entry);
					ExFreePoolWithTag(pProcessInfo, 'pinf');
					KdPrint(("Restoring [%s] PID=%d.\n", processImageName, pProcessInfo->OriginalPid));

					//��������eprocess����
					if (PsTerminateProcess)
					{
						PsTerminateProcess(Process, STATUS_SUCCESS);

					}

				}

			}

			return TRUE;  // �ҵ�ƥ��Ľ�����
		}
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[%s] Can not found process,but PID = %d.\n", imageNameAddress, pid));

	return FALSE;  // δ�ҵ�ƥ��Ľ�����
}



VOID ProcessCreateNotifyRoutineEx(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);

	// ��ʼ������ͷ
	// ֻ��������δ��ʼ��ʱ�Ž��г�ʼ��
	if (processListHead.Flink == NULL && processListHead.Blink == NULL) {
		InitializeListHead(&processListHead);
	}



	if (CreateInfo != NULL) {
		// ���̴����¼�
			// ��ȡ�ļ�������
		PCWSTR fullPath = CreateInfo->ImageFileName->Buffer;
		PCWSTR fileName = wcsrchr(fullPath, L'\\');

		// ����ҵ���б�ܣ��� fileName ָ���ļ����Ŀ�ʼ
		if (fileName != NULL) {
			fileName++; // ������б��
		}
		else {
			fileName = fullPath; // ���û���ҵ���б�ܣ���ֱ�����ļ���
		}
		// �ȽϽ�����
		for (SIZE_T i = 0; i < imageCount; i++) {
			if (CompareStrings(fileName, imageNamesW[i], TRUE, TRUE, TRUE)) {

				// ִ������Ҫ�Ĳ���
				  // ��ȡԭʼ PID
				DWORD64 originalPid = *(DWORD64*)((ULONG_PTR)Process + ulProcessIDOffset);

				// ����ԭʼ PID
				AddProcessInfo(Process, originalPid);
				HidePID(Process);
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Process Created: PID=[%d], FileName=[%s]\n", ProcessId, fileName));

			}
		}

	}
	else {
		// �����˳�ʱ����
		RecoverProcess(Process);


	}
}

NTSTATUS HidePID(PEPROCESS pEprocess) {
	DWORD64 ulProcessIDOffset = 0x440; // ƫ���������� ID
	// ��ȡ UniqueProcessId �ĵ�ַ���޸�
	ULONG_PTR pidAddress = (ULONG_PTR)pEprocess + ulProcessIDOffset;
	*(DWORD64*)pidAddress = 4;
	//(ULONG)pEprocess +ulProcessIDOffset
	return STATUS_SUCCESS;

}
NTSTATUS HideProcess()
{
	PEPROCESS pEprocess = PsInitialSystemProcess; // ��ϵͳ���̿�ʼ
	PEPROCESS pFirstEprocess = pEprocess; // ��¼��һ������



	while (pEprocess) {
		// ��ȡ��ǰ�����Ľ�������imagename
		CHAR* processName = (CHAR*)((ULONG_PTR)pEprocess + ulProcessImageNameOffset);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Current process[%s].\n", processName));

		// �ԱȽ������ƣ��ҳ�Ҫ���صĽ���
		for (SIZE_T i = 0; i < imageCount; i++) {
			// �ȽϽ�������
			if (_stricmp(processName, imageNames[i]) == 0) {
				NTSTATUS hideStatus = HidePID(pEprocess); // ���ؽ���
				if (hideStatus)
				{
					ULONG_PTR pid = (ULONG_PTR)pEprocess + ulProcessIDOffset;
					pid = *(ULONG*)pid;
					KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Process with PID=%d [%s]hide Successful.\n", pid, imageNames[i]));

				}
				break;
			}
		}


		//-----------------------------------------------------------------//
		// ָ����һ������                                                  //
		//-----------------------------------------------------------------//
		 // ��ǰ������˫�������е�λ�� ������������ϵͳ�е����н��̶�����������
	// ���Ը���ѭ�����˫������ �Ϳ����ҵ�ϵͳ�����еĽ���
		PLIST_ENTRY List = (PLIST_ENTRY)((DWORD64)pEprocess + 0x448);

		pEprocess = (PEPROCESS)((DWORD64)List->Flink - 0x448);
		//(LONG)pEprocess + 0x448;
		// �жϱ�׼�������ǰ�������һ��������ȣ���ֹͣ����
		if (pEprocess == pFirstEprocess) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "��������!\r\n"));
			break;
		}
	}
	return STATUS_SUCCESS;

}