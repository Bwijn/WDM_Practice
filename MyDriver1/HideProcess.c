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

ULONG ulProcessImageNameOffset = 0x5a8; // 偏移量，进程名称
ULONG ulProcessIDOffset = 0x440; // 偏移量，进程 ID
ULONG ulNextProcessOffset = 0x448; // 偏移量，链表位置

// 全局链表头，用于保存进程信息
LIST_ENTRY processListHead;



// 用于保存原始PID
typedef struct _PROCESS_INFO {
	char imageName[40];
	HANDLE ProcessId;
	HANDLE OriginalPid;
	LIST_ENTRY ListEntry;  // 用于将该结构体链接到链表中
} PROCESS_INFO, * PPROCESS_INFO;


// 查找并删除进程信息
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

// 记录进程原本信息
VOID AddProcessInfo(PEPROCESS Process, HANDLE OriginalPid) {
	PPROCESS_INFO pProcessInfo = (PPROCESS_INFO)ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), 'pinf');

	ULONG_PTR imageNameAddress = (ULONG_PTR)Process + 0x5a8; // ImageName 的偏移
	//PCHAR processImageName = (PCHAR)imageNameAddress;
	if (pProcessInfo) {
		// 填充 imageName（复制字符串）
	 // 使用 strncpy 复制字符串，避免越界
		strncpy(pProcessInfo->imageName, (PCHAR)imageNameAddress, sizeof(pProcessInfo->imageName) - 1);
		pProcessInfo->ProcessId = OriginalPid;
		pProcessInfo->OriginalPid = OriginalPid;
		InsertTailList(&processListHead, &pProcessInfo->ListEntry);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "&pProcessInfo->ListEntry -- [%p] \n", &pProcessInfo->ListEntry));

	}
}

// 查找并回复监控的进程
BOOLEAN RecoverProcess(PEPROCESS Process) {
	//id！=4直接pass
	DWORD64 pid = (DWORD64)Process + ulProcessIDOffset;
	if (*(DWORD64*)pid != 4)
	{
		return FALSE;
	}

	ULONG_PTR imageNameAddress = (ULONG_PTR)Process + 0x5a8; // ImageName 的偏移
	PCHAR processImageName = (PCHAR)imageNameAddress;

	// 遍历 ANSI 字符串列表 imageNames 进行比较
	for (SIZE_T i = 0; i < imageCount; i++) {
		// 比较进程名和列表中的字符串，忽略大小写
		if (_stricmp(processImageName, imageNames[i]) == 0) {
			//去链表查原始信息 看是否有记录

			PLIST_ENTRY entry;
			PPROCESS_INFO pProcessInfo;
			//entry = processListHead.Flink;
			for (entry = processListHead.Flink; entry != &processListHead; entry = entry->Flink) {
				// 先保存下一个节点的地址，防止当前节点被删除后链表指针无效
				//nextEntry = entry->Flink;
				//这个宏无需偏移取值CONTAINING_RECORD
				pProcessInfo = CONTAINING_RECORD(entry, PROCESS_INFO, ListEntry);
				if (CompareStrings(processImageName, imageNames[i], FALSE, FALSE, TRUE))
				{
					//有记录就删节点释放内存
					//1.恢复
					ULONG_PTR pidAddress = (ULONG_PTR)Process + ulProcessIDOffset;
					*(DWORD64*)pidAddress = pProcessInfo->OriginalPid;
					//2.释放list内存 删除节点
					RemoveEntryList(entry);
					ExFreePoolWithTag(pProcessInfo, 'pinf');
					KdPrint(("Restoring [%s] PID=%d.\n", processImageName, pProcessInfo->OriginalPid));

					//负责清理eprocess残留
					if (PsTerminateProcess)
					{
						PsTerminateProcess(Process, STATUS_SUCCESS);

					}

				}

			}

			return TRUE;  // 找到匹配的进程名
		}
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[%s] Can not found process,but PID = %d.\n", imageNameAddress, pid));

	return FALSE;  // 未找到匹配的进程名
}



VOID ProcessCreateNotifyRoutineEx(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);

	// 初始化链表头
	// 只有在链表未初始化时才进行初始化
	if (processListHead.Flink == NULL && processListHead.Blink == NULL) {
		InitializeListHead(&processListHead);
	}



	if (CreateInfo != NULL) {
		// 进程创建事件
			// 提取文件名部分
		PCWSTR fullPath = CreateInfo->ImageFileName->Buffer;
		PCWSTR fileName = wcsrchr(fullPath, L'\\');

		// 如果找到反斜杠，则 fileName 指向文件名的开始
		if (fileName != NULL) {
			fileName++; // 跳过反斜杠
		}
		else {
			fileName = fullPath; // 如果没有找到反斜杠，则直接是文件名
		}
		// 比较进程名
		for (SIZE_T i = 0; i < imageCount; i++) {
			if (CompareStrings(fileName, imageNamesW[i], TRUE, TRUE, TRUE)) {

				// 执行你想要的操作
				  // 获取原始 PID
				DWORD64 originalPid = *(DWORD64*)((ULONG_PTR)Process + ulProcessIDOffset);

				// 保存原始 PID
				AddProcessInfo(Process, originalPid);
				HidePID(Process);
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Process Created: PID=[%d], FileName=[%s]\n", ProcessId, fileName));

			}
		}

	}
	else {
		// 进程退出时处理
		RecoverProcess(Process);


	}
}

NTSTATUS HidePID(PEPROCESS pEprocess) {
	DWORD64 ulProcessIDOffset = 0x440; // 偏移量，进程 ID
	// 获取 UniqueProcessId 的地址并修改
	ULONG_PTR pidAddress = (ULONG_PTR)pEprocess + ulProcessIDOffset;
	*(DWORD64*)pidAddress = 4;
	//(ULONG)pEprocess +ulProcessIDOffset
	return STATUS_SUCCESS;

}
NTSTATUS HideProcess()
{
	PEPROCESS pEprocess = PsInitialSystemProcess; // 从系统进程开始
	PEPROCESS pFirstEprocess = pEprocess; // 记录第一个进程



	while (pEprocess) {
		// 获取当前遍历的进程名称imagename
		CHAR* processName = (CHAR*)((ULONG_PTR)pEprocess + ulProcessImageNameOffset);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Current process[%s].\n", processName));

		// 对比进程名称，找出要隐藏的进程
		for (SIZE_T i = 0; i < imageCount; i++) {
			// 比较进程名称
			if (_stricmp(processName, imageNames[i]) == 0) {
				NTSTATUS hideStatus = HidePID(pEprocess); // 隐藏进程
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
		// 指向下一个进程                                                  //
		//-----------------------------------------------------------------//
		 // 当前进程在双向链表中的位置 就是这个链表把系统中的所有进程都给串起来了
	// 所以根据循环这个双向链表 就可以找到系统中所有的进程
		PLIST_ENTRY List = (PLIST_ENTRY)((DWORD64)pEprocess + 0x448);

		pEprocess = (PEPROCESS)((DWORD64)List->Flink - 0x448);
		//(LONG)pEprocess + 0x448;
		// 判断标准：如果当前进程与第一个进程相等，则停止遍历
		if (pEprocess == pFirstEprocess) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "遍历结束!\r\n"));
			break;
		}
	}
	return STATUS_SUCCESS;

}