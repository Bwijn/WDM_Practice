#pragma once
#include<ntddk.h>

#include"HideProcess.h"
#include"OBlevel.h"
#include"stringTools.h"
// ���̴���/�˳�֪ͨ�ص�

//NTSTATUS HideProcess();
const CHAR* imageNames[];
SIZE_T imageCount;
//
//NTSTATUS HidePID(PEPROCESS pEprocess);


// ��������
NTSTATUS HideProcess();
NTSTATUS HidePID(PEPROCESS pEprocess);

//������ntddk��ͷ�ļ� wdm����
VOID ProcessCreateNotifyRoutineEx(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
);
