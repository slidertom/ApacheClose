#include "stdafx.h"
#include "NTProcessInfo.h"

#include <iostream>

pfnNtQueryInformationProcess msNtQueryInformationProcess;
//extern ostream cout;

//Make sure we have infromaion to debug prvilage
BOOL MS_EnableTokenPrivilege(LPCTSTR pszPrivilege)
{
	HANDLE hToken		 = 0;
	TOKEN_PRIVILEGES tkp = {0}; 

	// Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(),
						  TOKEN_ADJUST_PRIVILEGES |
						  TOKEN_QUERY, &hToken))
	{
        return FALSE;
	}

	 
	if(LookupPrivilegeValue(NULL, pszPrivilege,
						    &tkp.Privileges[0].Luid)) 
	{
        tkp.PrivilegeCount = 1;  // one privilege to set    
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Set the privilege for this process. 
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
							  (PTOKEN_PRIVILEGES)NULL, 0); 

		if (GetLastError() != ERROR_SUCCESS)
			return FALSE;
		
		return TRUE;
	}

	return FALSE;
}

// Load NTDLL Library and get entry address
// for NtQueryInformationProcess
HMODULE MS_LoadNTDLLFunctions()
{
	HMODULE hNtDll = LoadLibrary(_T("ntdll.dll"));
	if(hNtDll == NULL) return NULL;

	msNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll,
														"NtQueryInformationProcess");
	if(msNtQueryInformationProcess == NULL) {
		FreeLibrary(hNtDll);
		return NULL;
	}
	return hNtDll;
}


void MS_FreeNTDLLFunctions(HMODULE hNtDll)
{
	if(hNtDll)
		FreeLibrary(hNtDll);
	msNtQueryInformationProcess = NULL;
}

// Gets information on process with NtQueryInformationProcess
BOOL MS_GetNtProcessInfo(const DWORD dwPID, msPROCESSINFO &spi)
{
    spi = {0};

	BOOL  bReturnStatus						= TRUE;
	DWORD dwSize							= 0;
	DWORD dwSizeNeeded						= 0;
	SIZE_T dwBytesRead						= 0;
	DWORD dwBufferSize						= 0;
	HANDLE hHeap							= 0;
	WCHAR *pwszBuffer						= NULL;

	_PPROCESS_BASIC_INFORMATION pbi		= NULL;

	msPEB peb								= {0};
	msPEB_LDR_DATA peb_ldr					= {0};
	msRTL_USER_PROCESS_PARAMETERS peb_upp	= {0};

	ZeroMemory(&spi, sizeof(spi));
	ZeroMemory(&peb, sizeof(peb));
	ZeroMemory(&peb_ldr, sizeof(peb_ldr));
	ZeroMemory(&peb_upp, sizeof(peb_upp));

	spi.dwPID = dwPID;

	// Attempt to access process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | 
								  PROCESS_VM_READ, FALSE, dwPID);
	if(hProcess == INVALID_HANDLE_VALUE) {

		std::wcout<< "Invalid Process handle" ;
		return FALSE;
	}
	 
	// Try to allocate buffer 
	hHeap = GetProcessHeap();

	dwSize = sizeof(msPROCESS_BASIC_INFORMATION);

	pbi = (_PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
												  HEAP_ZERO_MEMORY,
												  dwSize);

	
	 
	if(!pbi) {
		CloseHandle(hProcess);
		return FALSE;
	} 

	// Attempt to get basic info on process

	NTSTATUS dwStatus = msNtQueryInformationProcess(hProcess,
												  ProcessBasicInformation,
												  pbi,
												  dwSize,
												  &dwSizeNeeded);

	// If we had error and buffer was too small, try again
	// with larger buffer size (dwSizeNeeded)
	if(dwStatus >= 0 && dwSize < dwSizeNeeded)
	{
		if(pbi)
			HeapFree(hHeap, 0, pbi);
		pbi = (_PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
													  HEAP_ZERO_MEMORY,
                                                      dwSizeNeeded);
		
		if(!pbi) {

			std::wcout<< "Closing Handle"; std::wcout<<std::endl;
			CloseHandle(hProcess);
			return FALSE;
		}

		dwStatus = msNtQueryInformationProcess(hProcess,
											 ProcessBasicInformation,
											 pbi, dwSizeNeeded, &dwSizeNeeded);
	}

	// Did we successfully get basic info on process
	if(dwStatus >= 0)
	{
		// Basic Info
//        spi.dwPID			 = (DWORD)pbi->UniqueProcessId;
		spi.dwParentPID		 = (DWORD)pbi->InheritedFromUniqueProcessId;
		spi.dwBasePriority	 = (LONG)pbi->BasePriority;
		spi.dwExitStatus	 = (NTSTATUS)pbi->ExitStatus;
		spi.dwPEBBaseAddress = (DWORD)pbi->PebBaseAddress;
		spi.dwAffinityMask	 = (DWORD)pbi->AffinityMask;

		 
		// Read Process Environment Block (PEB)
		if(pbi->PebBaseAddress)
		{
			if(ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead))
			{
				 
				spi.dwSessionID	   = (DWORD)peb.SessionId;
				spi.cBeingDebugged = (BYTE)peb.BeingDebugged;

	 
				dwBytesRead = 0;
				if(ReadProcessMemory(hProcess,
									 peb.ProcessParameters,
									 &peb_upp,
									 sizeof(msRTL_USER_PROCESS_PARAMETERS),
									 &dwBytesRead))
				{
					// We got Process Parameters, is CommandLine filled in
					if(peb_upp.CommandLine.Length > 0) {
						// Yes, try to read CommandLine
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap,
														HEAP_ZERO_MEMORY,
														peb_upp.CommandLine.Length);
						// If memory was allocated, continue
						if(pwszBuffer)
						{
							if(ReadProcessMemory(hProcess,
												 peb_upp.CommandLine.Buffer,
												 pwszBuffer,
												 peb_upp.CommandLine.Length,
												 &dwBytesRead))
							{
								// if commandline is larger than our variable, truncate
								if(peb_upp.CommandLine.Length >= sizeof(spi.szCmdLine)) 
									dwBufferSize = sizeof(spi.szCmdLine) - sizeof(TCHAR);
								else
									dwBufferSize = peb_upp.CommandLine.Length;
							
								// Copy CommandLine to our structure variable
#if defined(UNICODE) || (_UNICODE)
								// Since core NT functions operate in Unicode
								// there is no conversion if application is
								// compiled for Unicode
								StringCbCopyN(spi.szCmdLine, sizeof(spi.szCmdLine),
											  pwszBuffer, dwBufferSize);
								wcout<< "Command Line passed when the process is launched: " << pwszBuffer;
#else
								// Since core NT functions operate in Unicode
								// we must convert to Ansi since our application
								// is not compiled for Unicode
								WideCharToMultiByte(CP_ACP, 0, pwszBuffer,
													(int)(dwBufferSize / sizeof(WCHAR)),
													spi.szCmdLine, sizeof(spi.szCmdLine),
													NULL, NULL);
								std::wcout<< "Complete command line used to run the application. "; 
                                std::wcout<<std::endl;
                                std::wcout<<std::endl << pwszBuffer; 
                                std::wcout<<std::endl;
                                std::wcout<<std::endl;
#endif
							}
							if(!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read CommandLine in Process Parameters

					// We got Process Parameters, is ImagePath filled in
					if(peb_upp.ImagePathName.Length > 0) {
						// Yes, try to read ImagePath
						dwBytesRead = 0;
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap,
														HEAP_ZERO_MEMORY,
														peb_upp.ImagePathName.Length);
						if(pwszBuffer)
						{
                            if(ReadProcessMemory(hProcess,
												 peb_upp.ImagePathName.Buffer,
												 pwszBuffer,
												 peb_upp.ImagePathName.Length,
												 &dwBytesRead))
							{
								// if ImagePath is larger than our variable, truncate
								if(peb_upp.ImagePathName.Length >= sizeof(spi.szImgPath)) 
									dwBufferSize = sizeof(spi.szImgPath) - sizeof(TCHAR);
								else
									dwBufferSize = peb_upp.ImagePathName.Length;

								// Copy ImagePath to our structure
#if defined(UNICODE) || (_UNICODE)
								StringCbCopyN(spi.szImgPath, sizeof(spi.szImgPath),
											  pwszBuffer, dwBufferSize);
#else
								WideCharToMultiByte(CP_ACP, 0, pwszBuffer,
													(int)(dwBufferSize / sizeof(WCHAR)),
													spi.szImgPath, sizeof(spi.szImgPath),
													NULL, NULL);
#endif
							}
							if(!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// IMAGEPATH
				}	// PROCESS PARAMETERS
			}	// READ PEB
		}	// PEB CHECK

		
	}	// Read Basic Info

gnpiFreeMemFailed:

	// Free memory if allocated
	if(pbi != NULL)
		if(!HeapFree(hHeap, 0, pbi)) {
			// failed to free memory
		}

	CloseHandle(hProcess);
	return bReturnStatus;
}