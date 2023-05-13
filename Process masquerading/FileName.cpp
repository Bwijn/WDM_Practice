#include <Windows.h> 
#include <winternl.h>
#pragma comment(lib,"ntdll.lib")
#include <iostream>
#include<TlHelp32.h>

BOOL GetProcessList()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;


	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolhelp32Snapshot (of processes)");
		return(FALSE);
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		printf("Process32First"); // show cause of failure
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return(FALSE);
	}

	// Now walk the snapshot of processes, and
 // display information about each process in turn
	do
	{
		printf("\n=====================================================");
		wprintf(L"\nPROCESS NAME:  %s", pe32.szExeFile);
		printf("\n-------------------------------------------------------");

		// Retrieve the priority class.
		dwPriorityClass = 0;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if (hProcess == NULL)
			printf("\nOpenProcess");
		else
		{
			dwPriorityClass = GetPriorityClass(hProcess);
			if (!dwPriorityClass)
				printf("GetPriorityClass");
			CloseHandle(hProcess);
		}

		printf("\n  Process ID        = 0x%08X", pe32.th32ProcessID);
		printf("\n  Thread count      = %d", pe32.cntThreads);
		printf("\n  Parent process ID = 0x%08X", pe32.th32ParentProcessID);
		printf("\n  Priority base     = %d", pe32.pcPriClassBase);
		//wprintf(L"\n  exefile     = %s", pe32.szExeFile);
		if (dwPriorityClass)
			printf("\n  Priority class    = %d", dwPriorityClass);


		if (wcscmp(pe32.szExeFile, L"Notepad.exe")==0)
		{
			 break;
		}
		

		
		// List the modules and threads associated with this process
		//ListProcessModules(pe32.th32ProcessID);
		//ListProcessThreads(pe32.th32ProcessID);

	} while (Process32Next(hProcessSnap, &pe32));


}

int main()
{
	DWORD pid;

	printf("输入PID：");
	scanf("%lu", &pid);
	HANDLE hwd = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	if (hwd == NULL) {
		std::cout << "openproc failed  " << hwd << std::endl;
		exit;
	}

	PROCESS_BASIC_INFORMATION pi;
	NTSTATUS status = NtQueryInformationProcess(hwd, ProcessBasicInformation, &pi, sizeof(pi), nullptr);
	//NtWow64QueryInformationProcess64

	if (NT_SUCCESS(status)) {
		PEB peb;
		RTL_USER_PROCESS_PARAMETERS upp = { 0 };
		UNICODE_STRING uni;
		WCHAR cmd[50];
		WCHAR imagePath[] = L"改名字.EXE";
		SIZE_T read;
		SIZE_T writeBytes;
		auto  usPathLen = 2 + 2 * ::wcslen(imagePath);

		if (hwd) {

			if (!ReadProcessMemory(hwd, pi.PebBaseAddress, &peb, sizeof(PEB), NULL)) {
				std::cout << "readprocmemory failed pi.base" << std::endl;
			}
			if (!ReadProcessMemory(hwd, peb.ProcessParameters, &upp, sizeof(RTL_USER_PROCESS_PARAMETERS), NULL)) {
				std::cout << "readprocmemory failed peb.proc" << std::endl;
			}
			printf("pathBuffer: %p\nupp address: %p\nsizeof paramerter: %d\n", upp.ImagePathName.Buffer, &upp,
				sizeof(RTL_USER_PROCESS_PARAMETERS));

			if (!::WriteProcessMemory(hwd, upp.ImagePathName.Buffer, imagePath, usPathLen, &writeBytes)) {
				std::cout << "writeProcMemory failed upp.cmd" << std::endl;
			}
			if (!::WriteProcessMemory(hwd, upp.CommandLine.Buffer, imagePath, usPathLen, &writeBytes)) {
				std::cout << "writeProcMemory failed upp.cmd" << std::endl;
			}

		}

	}


	GetProcessList();

	//PROCESSENTRY32 entry;
	//entry.dwSize= sizeof(PROCESSENTRY32);

	//auto Sanp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	//auto ver_b = Process32First(Sanp, &entry);
	//if (!ver_b)
	//{
	//	printf("Error: Process32Firest Call\n");
	//}
	//printf("Pid：[%d]的进程[%s]\n", entry.th32ProcessID, entry.szExeFile);
	//std::wcout << entry.szExeFile;
	////exit
	getchar();
	//if (hwd) {
	//	CloseHandle(hwd);
	//}
}