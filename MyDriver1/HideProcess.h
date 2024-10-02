#pragma once
#include<ntddk.h>

#include"HideProcess.h"
#include"OBlevel.h"
#include"stringTools.h"
// 进程创建/退出通知回调

//NTSTATUS HideProcess();
const CHAR* imageNames[];
SIZE_T imageCount;
//
//NTSTATUS HidePID(PEPROCESS pEprocess);


// 函数声明
NTSTATUS HideProcess();
NTSTATUS HidePID(PEPROCESS pEprocess);

//必须是ntddk的头文件 wdm不行
VOID ProcessCreateNotifyRoutineEx(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
);
