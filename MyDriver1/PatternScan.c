
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


// 函数声明
PVOID ScanPattern(PVOID startAddress, SIZE_T length, const unsigned char* pattern, SIZE_T patternLength);

// 扫描内存以找到特征码
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
	//这里初始化一个unicode 的  ntoskrl以便统一compare
	RtlUnicodeStringInit(&ntImageName_U, moduleName);


	PLDR_DATA_TABLE_ENTRY pLdr = NULL;
	PLIST_ENTRY pListEntry = NULL;
	PLIST_ENTRY pCurrentListEntry = NULL;

	PLDR_DATA_TABLE_ENTRY pCurrentModule = NULL;
	pLdr = (PLDR_DATA_TABLE_ENTRY)pDriverObj->DriverSection;
	pListEntry = pLdr->InLoadOrderLinks.Flink;
	pCurrentListEntry = pListEntry->Flink;

	while (pCurrentListEntry != pListEntry) //前后不相等
	{
		//获取LDR_DATA_TABLE_ENTRY结构
		pCurrentModule = CONTAINING_RECORD(pCurrentListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (pCurrentModule->SizeOfImage && pCurrentModule != NULL && pCurrentModule->BaseDllName.Buffer != 0)
		{
			if (RtlCompareUnicodeString(&pCurrentModule->BaseDllName, &ntImageName_U, TRUE) == 0) {
				DbgPrint("ModuleName = %wZ ModuleBase = %p \r\n",
					pCurrentModule->BaseDllName,
					pCurrentModule->DllBase);
				return   pCurrentModule->DllBase;  // 返回模块的基地址
			}
		}

		pCurrentListEntry = pCurrentListEntry->Flink;
	}
	return STATUS_SUCCESS;


	//PLIST_ENTRY currentEntry = PsLoadedModuleList.Flink;
	//PLDR_DATA_TABLE_ENTRY currentModule = NULL;

	//// 遍历整个链表，直到回到链表头部
	//while (currentEntry != &PsLoadedModuleList) {
	//	currentModule = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
	//	//有的模块这个值是0 容易读到非法内存
	//	if (currentModule->SizeOfImage && currentModule != NULL && currentModule->BaseDllName.Buffer != NULL) {
	//		// 比较模块名称，找到匹配的模块
	//		if (RtlCompareUnicodeString(&currentModule->BaseDllName, &ntImageName_U, TRUE) == 0) {
	//			return currentModule->DllBase;  // 返回模块的基地址
	//		}
	//	}


	//	currentEntry = currentEntry->Flink;  // 继续下一个模块
	//}

	//return NULL;  // 没有找到
}

//DWORD64 GetNtoskrnlBaseAddress()
//{
//	NTSTATUS status;
//	ULONG len = 0;
//	PSYSTEM_MODULE_INFORMATION pSystemModuleInfo;
//
//	// 获取模块信息大小
//	status = NtQuerySystemInformation(11, NULL, 0, &len);
//	if (status != STATUS_INFO_LENGTH_MISMATCH) {
//		return status;
//	}
//
//	// 分配内存存储模块信息
//	pSystemModuleInfo = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, len, 'modi');
//	if (!pSystemModuleInfo) {
//		return STATUS_INSUFFICIENT_RESOURCES;
//	}
//
//	// 查询系统模块信息
//	status = NtQuerySystemInformation(11, pSystemModuleInfo, len, &len);
//	if (NT_SUCCESS(status)) {
//		for (ULONG i = 0; i < pSystemModuleInfo->ModuleCount; i++) {
//			PSYSTEM_MODULE_INFORMATION_ENTRY pEntry = &pSystemModuleInfo->Modules[i];
//			if (strstr((char*)pEntry->FullPathName + pEntry->OffsetToFileName, ntImageName)) {
//				KdPrint(("Found %s base address: %p\n", ntImageName, pEntry->ImageBase));
//				ExFreePoolWithTag(pSystemModuleInfo, 'modi');
//				return pEntry->ImageBase;  // 返回 基地址
//			}
//		}
//	}
//
//	ExFreePoolWithTag(pSystemModuleInfo, 'modi');
//	return STATUS_NOT_FOUND;
//}

// 特征码扫描函数

PVOID FindPattern(UCHAR* base, SIZE_T size, UCHAR* pattern, CHAR* mask) {
	SIZE_T patternLength = strlen(mask);  // 获取 mask 的长度

	for (SIZE_T i = 0; i <= size - patternLength; i++) {
		BOOLEAN found = TRUE;

		// 遍历 pattern 和 mask
		for (SIZE_T j = 0; j < patternLength; j++) {
			// 比较每个字节，mask 是 '?' 时忽略比较
			if (mask[j] != '?' && base[i + j] != pattern[j]) {
				found = FALSE;
				break;
			}
		}

		// 如果找到匹配的 pattern，返回地址
		if (found) {
			return (PVOID)(base + i);
		}
	}

	return NULL;  // 未找到
}
