
#include<wdm.h>
//#include <winternl.h>

#include<Ntstrsafe.h>
#include"PatternScan.h"
#include"OBlevel.h"
#include"stringTools.h"
#include"constants.h"


NTSTATUS NTAPI NtQuerySystemInformation(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, * PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG ModuleCount;
	SYSTEM_MODULE_INFORMATION_ENTRY Modules[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;


// ��������
PVOID ScanPattern(PVOID startAddress, SIZE_T length, const unsigned char* pattern, SIZE_T patternLength);

// ɨ���ڴ����ҵ�������
PVOID ScanPattern(PVOID startAddress, SIZE_T length, const unsigned char* pattern, SIZE_T patternLength) {
	unsigned char* start = (unsigned char*)startAddress;
	unsigned char* end = start + length - patternLength;

	for (unsigned char* current = start; current <= end; current++) {
		if (RtlCompareMemory(current, pattern, patternLength) == patternLength) {
			return current;
		}
	}

	return NULL;
}

LIST_ENTRY PsLoadedModuleList;

PVOID GetModuleBaseAddress(PDRIVER_OBJECT pDriverObj, PCWSTR moduleName)
{
	//�����ʼ��һ��unicode ��  ntoskrl�Ա�ͳһcompare
	RtlUnicodeStringInit(&ntImageName_U, moduleName);


	PLDR_DATA_TABLE_ENTRY pLdr = NULL;
	PLIST_ENTRY pListEntry = NULL;
	PLIST_ENTRY pCurrentListEntry = NULL;

	PLDR_DATA_TABLE_ENTRY pCurrentModule = NULL;
	pLdr = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	pListEntry = pLdr->InLoadOrderLinks.Flink;
	pCurrentListEntry = pListEntry->Flink;

	while (pCurrentListEntry != pListEntry) //ǰ�����
	{
		//��ȡLDR_DATA_TABLE_ENTRY�ṹ
		pCurrentModule = CONTAINING_RECORD(pCurrentListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (pCurrentModule->SizeOfImage && pCurrentModule != NULL && pCurrentModule->BaseDllName.Buffer != 0)
		{
			if (RtlCompareUnicodeString(&pCurrentModule->BaseDllName, &ntImageName_U, TRUE) == 0) {
				DbgPrint("ModuleName = %wZ ModuleBase = %p \r\n",
					pCurrentModule->BaseDllName,
					pCurrentModule->DllBase);
				return   pCurrentModule->DllBase;  // ����ģ��Ļ���ַ
			}
		}

		pCurrentListEntry = pCurrentListEntry->Flink;
	}
	return STATUS_SUCCESS;


	//PLIST_ENTRY currentEntry = PsLoadedModuleList.Flink;
	//PLDR_DATA_TABLE_ENTRY currentModule = NULL;

	//// ������������ֱ���ص�����ͷ��
	//while (currentEntry != &PsLoadedModuleList) {
	//	currentModule = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
	//	//�е�ģ�����ֵ��0 ���׶����Ƿ��ڴ�
	//	if (currentModule->SizeOfImage && currentModule != NULL && currentModule->BaseDllName.Buffer != NULL) {
	//		// �Ƚ�ģ�����ƣ��ҵ�ƥ���ģ��
	//		if (RtlCompareUnicodeString(&currentModule->BaseDllName, &ntImageName_U, TRUE) == 0) {
	//			return currentModule->DllBase;  // ����ģ��Ļ���ַ
	//		}
	//	}


	//	currentEntry = currentEntry->Flink;  // ������һ��ģ��
	//}

	//return NULL;  // û���ҵ�
}

//DWORD64 GetNtoskrnlBaseAddress()
//{
//	NTSTATUS status;
//	ULONG len = 0;
//	PSYSTEM_MODULE_INFORMATION pSystemModuleInfo;
//
//	// ��ȡģ����Ϣ��С
//	status = NtQuerySystemInformation(11, NULL, 0, &len);
//	if (status != STATUS_INFO_LENGTH_MISMATCH) {
//		return status;
//	}
//
//	// �����ڴ�洢ģ����Ϣ
//	pSystemModuleInfo = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, len, 'modi');
//	if (!pSystemModuleInfo) {
//		return STATUS_INSUFFICIENT_RESOURCES;
//	}
//
//	// ��ѯϵͳģ����Ϣ
//	status = NtQuerySystemInformation(11, pSystemModuleInfo, len, &len);
//	if (NT_SUCCESS(status)) {
//		for (ULONG i = 0; i < pSystemModuleInfo->ModuleCount; i++) {
//			PSYSTEM_MODULE_INFORMATION_ENTRY pEntry = &pSystemModuleInfo->Modules[i];
//			if (strstr((char*)pEntry->FullPathName + pEntry->OffsetToFileName, ntImageName)) {
//				KdPrint(("Found %s base address: %p\n", ntImageName, pEntry->ImageBase));
//				ExFreePoolWithTag(pSystemModuleInfo, 'modi');
//				return pEntry->ImageBase;  // ���� ����ַ
//			}
//		}
//	}
//
//	ExFreePoolWithTag(pSystemModuleInfo, 'modi');
//	return STATUS_NOT_FOUND;
//}

// ������ɨ�躯��

PVOID FindPattern(UCHAR* base, SIZE_T size, UCHAR* pattern, CHAR* mask) {
	SIZE_T patternLength = strlen(mask);  // ��ȡ mask �ĳ���

	for (SIZE_T i = 0; i <= size - patternLength; i++) {
		BOOLEAN found = TRUE;

		// ���� pattern �� mask
		for (SIZE_T j = 0; j < patternLength; j++) {
			// �Ƚ�ÿ���ֽڣ�mask �� '?' ʱ���ԱȽ�
			if (mask[j] != '?' && base[i + j] != pattern[j]) {
				found = FALSE;
				break;
			}
		}

		// ����ҵ�ƥ��� pattern�����ص�ַ
		if (found) {
			return (PVOID)(base + i);
		}
	}

	return NULL;  // δ�ҵ�
}
