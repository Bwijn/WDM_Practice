#pragma once
#include<wdm.h>
#define PAGE_SIZE 0x000000003C4C00
#define PAGE_RVA 0x5C7000

DWORD64 ntosBase;
UCHAR* PAGE_base;

extern UCHAR pattern[];
extern CHAR mask[];

//��ֹ������
PS_TERMINATE_PROCESS PsTerminateProcess;


//�����汾
char ntImageName[];
UNICODE_STRING ntImageName_U;
WCHAR ntImageName_W[];
