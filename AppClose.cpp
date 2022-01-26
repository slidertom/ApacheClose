#include "stdafx.h"

#include "NTProcessInfo.h"

#include "string"
#include "vector"

#include "psapi.h"

std::wstring GetPathByPid(int32_t pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        return L"";
    }

    wchar_t path[MAX_PATH];
    int32_t ret_val = GetModuleFileNameExW(hProcess, NULL, path, sizeof(path));
    CloseHandle(hProcess);
    if (ret_val == 0 ) {
        return L"";
    }
    return path;
}

static HWND GetHWNDFromPid(int32_t pid)
{
    HWND hwnd = NULL;

    do
    {
        hwnd = ::FindWindowEx(NULL, hwnd, NULL, NULL);
        DWORD dwProcessID = 0;
        GetWindowThreadProcessId(hwnd, &dwProcessID);
        if ( dwProcessID == pid ) {
            return hwnd;
        }
    } while (hwnd != NULL);
    return NULL;
}

static int32_t OnError()
{
    printf("ERROR\n");
#ifdef _DEBUG
    getchar();
#endif
    return -1;
}

static DWORD GetPid(const char *path_to_pid)
{
    FILE *f = ::fopen(path_to_pid, "r");
    if (!f) {
        printf ("File '%s' not found or locked...\n", path_to_pid);
        return OnError();
    }

    char str[10];
    ::fread(str, 1, 10, f);
    ::fclose(f);
    DWORD pid = ::atoi(str);
    return pid;
}

static DWORD GetParentPid(DWORD dwPID)
{
    DWORD dwSizeNeeded	 = 0;
    DWORD dwPIDCount	 = 0;

    //Get he debug Previlage 
    if(!MS_EnableTokenPrivilege(SE_DEBUG_NAME)) {
        //	return 0;
    }

    msPROCESSINFO spi = {0};
    HMODULE hNtDll = MS_LoadNTDLLFunctions();
    if(hNtDll) {
        MS_GetNtProcessInfo(dwPID, spi);
        MS_FreeNTDLLFunctions(hNtDll);
    }
    return spi.dwParentPID;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf ("Please provide parameter - process PID\n");
        printf ("  Can be number or path to the file where number is stored\n");
        printf ("  e.g. AppClose 1234\n");
        printf ("       AppClose c:\\path\\app.pid\n");
        printf ("Please provide parameter - title\n");
        return OnError();
    }

    char *param = argv[1];

    DWORD pid = ::atoi(param);

    if (pid == 0) {
        pid = ::GetPid(param);
    }

    if ( pid == -1) {
        return -1;
    }

    printf("Process PID:              %d\n", pid);
    const DWORD parent_pid = ::GetParentPid(pid);
    printf("Parent Process PID:       %d\n", parent_pid);
    if ( parent_pid == 0) {
        return OnError();
    }
    std::wstring apache_exe_path = GetPathByPid(pid);
    ::wprintf(L"Path to Apache process: %s\n", apache_exe_path.c_str());
    HWND hWnd = GetHWNDFromPid(parent_pid);
    if (!hWnd)  
    {
        printf ("HWND from parent pid not found...\n");
        const char *sTitle = argv[2];
        hWnd = FindWindowEx(NULL, NULL, NULL, sTitle);
        if ( !hWnd ) {
            printf ("HWND from title = %s not found...\n", sTitle);
            return OnError();
        }
    }

    printf("Sending WM_CLOSE to HWND: 0x%X\n", (unsigned int)hWnd);
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);

    printf("DONE\n");
    #ifdef _DEBUG
        getchar();
    #endif
    return 0;
}