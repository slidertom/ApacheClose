#ifndef _ORIONSCORPION_NTPROCESSINFO_H_
#define _ORIONSCORPION_NTPROCESSINFO_H_

#pragma once
//#include <windows.h>
#include <winternl.h>
#include <psapi.h>


#define STRSAFE_LIB
#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "psapi.lib")

#ifndef NTSTATUS
#define LONG NTSTATUS
#endif

// Unicode path usually prefix with '\\?\'
#define MAX_UNICODE_PATH	32767L

// Used in PEB struct
typedef ULONG _PPS_POST_PROCESS_INIT_ROUTINE;

// Used in PEB struct
typedef struct MS_PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} msPEB_LDR_DATA, *_PPEB_LDR_DATA;

// Used in PEB struct
typedef struct MS_RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} msRTL_USER_PROCESS_PARAMETERS, *_PRTL_USER_PROCESS_PARAMETERS;

// struct PROCESS_BASIC_INFORMATION struct
typedef struct MS_PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    _PPEB_LDR_DATA Ldr;
    _PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    BYTE Reserved4[104];
    PVOID Reserved5[52];
    _PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
}  msPEB, *_PPEB;

// Struct NtQueryInformationProcess from ntdll
typedef struct MS_PROCESS_BASIC_INFORMATION {
    LONG ExitStatus;
    _PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} msPROCESS_BASIC_INFORMATION, *_PPROCESS_BASIC_INFORMATION;

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS (NTAPI *pfnNtQueryInformationProcess)(
    IN	HANDLE ProcessHandle,
    IN	PROCESSINFOCLASS ProcessInformationClass,
    OUT	PVOID ProcessInformation,
    IN	ULONG ProcessInformationLength,
    OUT	PULONG ReturnLength	OPTIONAL
   );

extern pfnNtQueryInformationProcess msNtQueryInformationProcess;

typedef struct MS_PROCESSINFO
{
    DWORD	dwPID;
    DWORD	dwParentPID;
    DWORD	dwSessionID;
    DWORD	dwPEBBaseAddress;
    DWORD	dwAffinityMask;
    LONG	dwBasePriority;
    LONG	dwExitStatus;
    BYTE	cBeingDebugged;
    TCHAR	szImgPath[MAX_UNICODE_PATH];
    TCHAR	szCmdLine[MAX_UNICODE_PATH];
} msPROCESSINFO;

HMODULE MS_LoadNTDLLFunctions(void);
void MS_FreeNTDLLFunctions(IN HMODULE hNtDll);
BOOL MS_EnableTokenPrivilege(IN LPCTSTR pszPrivilege);
BOOL MS_GetNtProcessInfo(IN const DWORD dwPID, msPROCESSINFO &spi);

#endif	// _ORIONSCORPION_NTPROCESSINFO_H_
