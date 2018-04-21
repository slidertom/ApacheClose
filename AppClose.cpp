#include "stdafx.h"

#include "windows.h"

#include "string"
#include "vector"

//#include "str_util.h"
// This function compares text strings, one of which can have wildcards ('*').
inline bool str_wild_match(const wchar_t *pWildText,				  // A (potentially) corresponding string with wildcards        
						   const wchar_t *pTameText,				  // A string without wildcards
						   bool bCaseSensitive = false,       // By default, match on 'X' vs 'x'
						   wchar_t cAltTerminator = L'\0')    // For function names, for example, you can stop at the first '('
{
    bool bMatch = true;
    const wchar_t * pAfterLastWild = nullptr; // The location after the last '*', if we’ve encountered one
    const wchar_t * pAfterLastTame = nullptr; // The location in the tame string, from which we started after last wildcard
    wchar_t  t, w;
 
    // Walk the text strings one character at a time.
    while (1)
    {
        t = *pTameText;
        w = *pWildText;
 
        // How do you match a unique text string?
        if (!t || t == cAltTerminator) {
            // Easy: unique up on it!
            if (!w || w == cAltTerminator) {
				break; // "x" matches "x"
            }
            else if (w == L'*') {
                pWildText++;
                continue; // "x*" matches "x" or "xy"
            }
            else if (pAfterLastTame) {
                if (!(*pAfterLastTame) || *pAfterLastTame == cAltTerminator) {
                    bMatch = false;
                    break;
                }
                pTameText = pAfterLastTame++;
                pWildText = pAfterLastWild;
                continue;
            }
 
            bMatch = false;
            break; // "x" doesn't match "xy"
        }
        else
        {
            if (!bCaseSensitive)
            {   // Lowercase the characters to be compared.
                if (t >= L'A' && t <= L'Z') {
					t += (L'a' - L'A');
                }
 
                if (w >= L'A' && w <= L'Z') {
					w += (L'a' - L'A');
                }
            }
 
            // How do you match a tame text string?
            if (t != w) {
                if (w == L'*') // The tame way: unique up on it!
                {
                    pAfterLastWild = ++pWildText;
                    pAfterLastTame = pTameText;
                    w = *pWildText;
 
                    if (!w || w == cAltTerminator) {
						break; // "*" matches "x"
                    }
                    continue; // "*y" matches "xy"
                }
                else if (pAfterLastWild)
                {
                    if (pAfterLastWild != pWildText)
                    {
                        pWildText = pAfterLastWild;
                        w = *pWildText;
                                                
                        if (!bCaseSensitive && w >= L'A' && w <= L'Z') {
							w += (L'a' - L'A');
                        }
 
                        if (t == w) {
							pWildText++;
                        }
                    }
                    pTameText++;
                    continue;  // "*sip*" matches "mississippi"
                }
                else {
                    bMatch = false;
                    break; // "x" doesn't match "y"
                }
            }
        }
 
        pTameText++;
        pWildText++;
    }
 
    return bMatch;
}
// http://www.codeproject.com/KB/string/stringsplit.aspx
//-----------------------------------------------------------
// StrT:    Type of string to be constructed
//          Must have char* ctor.
// str:     String to be parsed.
// delim:   Pointer to delimiter.
// results: Vector of StrT for strings between delimiter.
// empties: Include empty strings in the results. 
//-----------------------------------------------------------
template <typename StrT>
inline size_t str_split_string(const wchar_t *str, const wchar_t *delim, std::vector<StrT> &results, bool empties = true)
{
    wchar_t *pstr = const_cast<wchar_t *>(str);
    wchar_t *r = ::wcsstr(pstr, delim); // Returns a pointer to the first occurrence of str2 in str1, or a null pointer if str2 is not part of str1.
                                         // A pointer to the first occurrence in str1 of the entire sequence of characters specified in str2, or a null pointer if the sequence is not present in str1.
    size_t dlen = ::wcslen(delim);

    while (r != NULL)
    {
        wchar_t *cp = new wchar_t[(r-pstr)+1];
        memcpy(cp, pstr, sizeof(wchar_t)*(r-pstr));

        cp[(r-pstr)] = '\0';

        if ( ::wcslen(cp) > 0 || empties )
        {
            StrT s(cp);
            results.push_back(s);
        }

        delete[] cp;

        pstr = r + dlen;
        r = ::wcsstr(pstr, delim);
    }

    if ( ::wcslen(pstr) > 0 || empties ) {
        results.push_back(StrT(pstr));
    }

    return results.size();
}

static HWND GetHWNDFromPath(WCHAR* path_to_apache)
{
	HWND hwnd = NULL;

	//std::vector<DWORD> tester_array;
	//std::vector<std::wstring> tester_window_names;

	do
	{
		hwnd = ::FindWindowEx(NULL, hwnd, NULL, NULL);
		wchar_t ProccessName[128];
		::GetWindowTextW(hwnd, ProccessName, 128);
		std::wstring t = std::wstring(ProccessName);
		//tester_window_names.push_back(t);
		//DWORD dwPID = 0;
		//GetWindowThreadProcessId(hwnd, &dwPID);
		//tester_array.push_back(dwPID);

		if (::str_wild_match(path_to_apache, ProccessName))
		{
			return hwnd;
		}
	} while (hwnd != NULL);
	return NULL;
}

int main(int argc, char* argv[])
{
	DWORD pid;
	HWND hWnd;
	char* param;

	/* AppClose.exe expected location: /bin/Apache2.2
	C:\\Users\\user_name\\Desktop\\Web\\WebDemo1925\\\\bin\\Apache2.2\\bin\\AppClose.exe

	it's enought to  take po -f esancia dali:
	-f C:\\Users\\user_name\\Desktop\\Web\\WebDemo1925\\\\bin\\Apache2.2\\conf\\httpdlocal.c */

	// Get Current Path
	HMODULE hModule = ::GetModuleHandleW(NULL);
	wchar_t path[MAX_PATH];
	::GetModuleFileNameW(hModule, path, MAX_PATH);

	wchar_t drive[_MAX_DRIVE];
	wchar_t dir[_MAX_DIR];
	wchar_t fname[_MAX_FNAME];
	wchar_t ext[_MAX_EXT];
	_wsplitpath(path, drive, dir, fname, ext);

	wchar_t path_to_exe[MAX_PATH]; // array to hold the result.
	wcscpy(path_to_exe, drive);  // copy string one into the result.
	wcscat(path_to_exe, dir);    // append string two to the result.

	 // Construct wildcard path to any http processes in this dir
	std::vector<std::wstring> path_to_exe_dirs;

	::str_split_string(path_to_exe, L"\\", path_to_exe_dirs, false);

	wchar_t path_to_apache[MAX_PATH];   // array to hold the result.

	// Anything can go before the path, since path is only a param -f <path>
	::wcscpy(path_to_apache, L"*");

	// join dirs except the last one
    auto nDirSize = path_to_exe_dirs.size();
	for (int i = 0; i <  int(nDirSize - 1); ++i) {
		::wcscat(path_to_apache, path_to_exe_dirs[i].c_str());
		::wcscat(path_to_apache, L"\\");
	}

	// Apache process is in conf folder. This way appclose.exe process gets excluded, since it is not in conf folder
	::wcscat(path_to_apache, L"conf");

	// anything can go after (but it's most like going to be 'httpdlocal.c')
	::wcscat(path_to_apache, L"*");

	if (argc < 2)
	{
		printf ("Please provide parameter - process PID\n");
		printf ("  Can be number or path to the file where number is stored\n");
		printf ("  e.g. AppClose 1234\n");
		printf ("       AppClose c:\\path\\app.pid\n");
		goto ON_ERROR;
	}

	param = argv[1];
	pid = ::atoi(param);

	//pid = 0;
	//param = "C:\\Users\\Algirdas\\Desktop\\Web\\WebDemo1925\\bin\\Apache2.2\\httpd.pid";

	if (pid == 0)
	{
		FILE *f = fopen(param, "r");

		if (!f)
		{
			printf ("File '%s' not found or locked...\n", param);
			goto ON_ERROR;
		}

		char str[10];
		fread(str, 1, 10, f);
		fclose(f);
		pid = atoi(str);
	}

	//top_pid = GetTopProcessPID(pid);

	printf("Path to Apache process: %ls", path_to_apache);
	hWnd = GetHWNDFromPath(path_to_apache);

	// apache console doesn't react to ctrl+c even if you do it in console itself by hand
	// (while it does react to ctrl+break -> server restart)
	//ctrl_event_success = GenerateConsoleCtrlEvent(0, top_pid);

	printf("Process PID:              %d\n", pid);
	//printf("Top process PID:          %d\n", top_pid);

	if (!hWnd) 
    {
		printf ("HWND not found...\n");
		goto ON_ERROR;
	}

	printf("Sending WM_CLOSE to HWND: 0x%X\n", (unsigned int)hWnd);

	::SendMessage(hWnd, WM_CLOSE, 0, 0);

//ON_OK:
	printf("DONE\n");
	#ifdef _DEBUG
		getchar();
	#endif
	return 0;

ON_ERROR:
	printf("ERROR\n");
	#ifdef _DEBUG
		getchar();
	#endif
	return -1;
}