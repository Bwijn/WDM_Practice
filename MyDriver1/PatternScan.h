#pragma once
//#include<wdm.h>

#include"LDR.h"

PVOID ScanPattern(PVOID startAddress, SIZE_T length, const unsigned char* pattern, SIZE_T patternLength);
DWORD64 GetNtoskrnlBaseAddress();
typedef NTSTATUS(*PS_TERMINATE_PROCESS)(
    PEPROCESS Process,
    NTSTATUS ExitStatus
    );

PVOID FindPattern(UCHAR* base, SIZE_T size, UCHAR* pattern, CHAR* mask);
PVOID GetModuleBaseAddress(PDRIVER_OBJECT pDriverObj, PCWSTR moduleName);




LIST_ENTRY PsLoadedModuleList;
