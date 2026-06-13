#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")

// ============================================================================
// R2 BYPASS - Advanced Emulator Detection Bypass System
// Version: 2.0
// Protection Target: PUBG Mobile (All Regions)
// ============================================================================

// الأوفستات الحقيقية المستخرجة من ملفات اللعبة
#define OFFSET_IS_EMULATOR_WHEN_INIT    0x02137EB4
#define OFFSET_GET_IS_ANDROID_SIMULATOR 0x021B5633
#define OFFSET_GET_EMULATOR_NAME        0x021B7DCA
#define OFFSET_ANDROID_BUILD_VERSION    0x021BB917
#define OFFSET_ANSI_FLAG_PATH           0x021BB92F
#define OFFSET_FILES_DIR                0x021B7E8E
#define OFFSET_EMULATOR_NAME_PATTERN    0x0006B8ED

// قائمة أسماء المحاكيات المشهورة
const char* EMULATOR_STRINGS[] = {
    "emulator",
    "nox",
    "memu",
    "bluestacks",
    "smartgaga",
    "gameloop",
    "andy",
    "leapdroid",
    "droid4x",
    "xamarin",
    "genymotion",
    "itools",
    "koplayer",
    "tencent",
    "vmos",
    "parallel"
};

#define EMULATOR_COUNT (sizeof(EMULATOR_STRINGS) / sizeof(EMULATOR_STRINGS[0]))

// ============================================================================
// البيانات الثابتة
// ============================================================================

typedef HANDLE (*pCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef LONG (*pRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LONG (*pRegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef HMODULE (*pGetModuleHandleW)(LPCWSTR);
typedef HMODULE (*pLoadLibraryW)(LPCWSTR);
typedef BOOL (*pGetFileAttributesExW)(LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
typedef int (*pGetSystemMetrics)(int);
typedef HWND (*pFindWindowW)(LPCWSTR, LPCWSTR);

// الدوال الأصلية
pCreateFileW OriginalCreateFileW = NULL;
pRegOpenKeyExW OriginalRegOpenKeyExW = NULL;
pRegQueryValueExW OriginalRegQueryValueExW = NULL;
pGetModuleHandleW OriginalGetModuleHandleW = NULL;
pLoadLibraryW OriginalLoadLibraryW = NULL;
pGetFileAttributesExW OriginalGetFileAttributesExW = NULL;
pGetSystemMetrics OriginalGetSystemMetrics = NULL;
pFindWindowW OriginalFindWindowW = NULL;

// ============================================================================
// الدوال المساعدة
// ============================================================================

// فحص هل السلسلة تحتوي على أسماء محاكيات
BOOL ContainsEmulatorString(const char* str) {
    if (!str) return FALSE;
    
    char lowerStr[512];
    strcpy_s(lowerStr, sizeof(lowerStr), str);
    _strlwr_s(lowerStr, sizeof(lowerStr));

    for (int i = 0; i < EMULATOR_COUNT; i++) {
        if (strstr(lowerStr, EMULATOR_STRINGS[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

// استبدال سلسلة محاكي
void SanitizeEmulatorString(char* str) {
    if (!str || strlen(str) == 0) return;
    
    if (ContainsEmulatorString(str)) {
        strcpy_s(str, strlen(str) + 1, "Generic Device");
    }
}

// استبدال سلسلة Unicode
void SanitizeEmulatorStringW(wchar_t* str) {
    if (!str || wcslen(str) == 0) return;
    
    char tempStr[512];
    wcstombs(tempStr, str, sizeof(tempStr) - 1);
    
    if (ContainsEmulatorString(tempStr)) {
        wcscpy_s(str, wcslen(str) + 1, L"Generic Device");
    }
}

// البحث عن قاعدة البيانات في الذاكرة
LPVOID FindModuleBase(DWORD dwPID, const char* moduleName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hSnapshot == INVALID_HANDLE_VALUE) return NULL;

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &me32)) {
        do {
            if (_stricmp(me32.szModule, moduleName) == 0) {
                CloseHandle(hSnapshot);
                return me32.modBaseAddr;
            }
        } while (Module32Next(hSnapshot, &me32));
    }

    CloseHandle(hSnapshot);
    return NULL;
}

// ============================================================================
// الدوال المحجوبة (Hooked Functions)
// ============================================================================

// Hook 1: CreateFileW
HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
) {
    // فحص أسماء الملفات المريبة
    if (lpFileName) {
        wchar_t tempFileName[MAX_PATH];
        wcscpy_s(tempFileName, sizeof(tempFileName) / sizeof(wchar_t), lpFileName);
        SanitizeEmulatorStringW(tempFileName);
        
        // تحقق من مسارات المحاكي
        if (wcsstr(lpFileName, L"nox") || wcsstr(lpFileName, L"bluestacks") ||
            wcsstr(lpFileName, L"memu") || wcsstr(lpFileName, L"gameloop")) {
            return CreateFileW(L"NUL", dwDesiredAccess, dwShareMode,
                lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }

    return OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Hook 2: RegOpenKeyExW
LONG WINAPI HookedRegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
) {
    // فحص المفاتيح المريبة
    if (lpSubKey) {
        if (wcsstr(lpSubKey, L"BlueStacks") || wcsstr(lpSubKey, L"Nox") ||
            wcsstr(lpSubKey, L"MEmu") || wcsstr(lpSubKey, L"GameLoop")) {
            return ERROR_FILE_NOT_FOUND;
        }
    }

    return OriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hook 3: RegQueryValueExW
LONG WINAPI HookedRegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
) {
    LONG result = OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    // معالجة النتائج
    if (result == ERROR_SUCCESS && lpData && lpcbData) {
        if (*lpType == REG_SZ) {
            char dataStr[512];
            WideCharToMultiByte(CP_ACP, 0, (wchar_t*)lpData, -1, dataStr, sizeof(dataStr), NULL, NULL);
            
            if (ContainsEmulatorString(dataStr)) {
                strcpy_s(dataStr, sizeof(dataStr), "Generic Device");
                MultiByteToWideChar(CP_ACP, 0, dataStr, -1, (wchar_t*)lpData, *lpcbData);
            }
        }
    }

    return result;
}

// Hook 4: GetModuleHandleW
HMODULE WINAPI HookedGetModuleHandleW(LPCWSTR lpModuleName) {
    // حظر تحميل DLLs المشبوهة
    if (lpModuleName) {
        if (wcsstr(lpModuleName, L"vbox") || wcsstr(lpModuleName, L"virtualbox") ||
            wcsstr(lpModuleName, L"vmware") || wcsstr(lpModuleName, L"xen")) {
            return NULL;
        }
    }

    return OriginalGetModuleHandleW(lpModuleName);
}

// Hook 5: LoadLibraryW
HMODULE WINAPI HookedLoadLibraryW(LPCWSTR lpLibFileName) {
    // حظر تحميل مكتبات المحاكاة
    if (lpLibFileName) {
        if (wcsstr(lpLibFileName, L"vbox") || wcsstr(lpLibFileName, L"virtualbox") ||
            wcsstr(lpLibFileName, L"vmware") || wcsstr(lpLibFileName, L"xen") ||
            wcsstr(lpLibFileName, L"nox") || wcsstr(lpLibFileName, L"memu")) {
            return NULL;
        }
    }

    return OriginalLoadLibraryW(lpLibFileName);
}

// Hook 6: GetFileAttributesExW
BOOL WINAPI HookedGetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
) {
    // حظر الوصول لملفات محددة من المحاكي
    if (lpFileName) {
        if (wcsstr(lpFileName, L"nox") || wcsstr(lpFileName, L"bluestacks") ||
            wcsstr(lpFileName, L"gameloop") || wcsstr(lpFileName, L"memu")) {
            return FALSE;
        }
    }

    return OriginalGetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}

// Hook 7: GetSystemMetrics
int WINAPI HookedGetSystemMetrics(int nIndex) {
    // إرجاع قيم حقيقية
    return OriginalGetSystemMetrics(nIndex);
}

// Hook 8: FindWindowW
HWND WINAPI HookedFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) {
    // حظر البحث عن نوافذ محاكي معروفة
    if (lpClassName || lpWindowName) {
        if ((lpClassName && wcsstr(lpClassName, L"nox")) ||
            (lpWindowName && wcsstr(lpWindowName, L"nox")) ||
            (lpClassName && wcsstr(lpClassName, L"MEmu")) ||
            (lpWindowName && wcsstr(lpWindowName, L"MEmu"))) {
            return NULL;
        }
    }

    return OriginalFindWindowW(lpClassName, lpWindowName);
}

// ============================================================================
// تطبيق الـ Hooks (IAT Hooking)
// ============================================================================

BOOL PatchImportTable(LPVOID baseAddr, const char* moduleName, const char* funcName, LPVOID newFunc) {
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)baseAddr;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((BYTE*)baseAddr + pDosHeader->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)
        ((BYTE*)baseAddr + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (; pImportDesc->Name; pImportDesc++) {
        const char* currentModuleName = (const char*)((BYTE*)baseAddr + pImportDesc->Name);
        
        if (_stricmp(currentModuleName, moduleName) == 0) {
            PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)
                ((BYTE*)baseAddr + pImportDesc->FirstThunk);

            for (; pThunkData->u1.Function; pThunkData++) {
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)
                    ((BYTE*)baseAddr + pThunkData->u1.AddressOfData);

                if (_strcmpi((const char*)pImportByName->Name, funcName) == 0) {
                    DWORD oldProtect;
                    VirtualProtect(&pThunkData->u1.Function, sizeof(pThunkData->u1.Function), PAGE_READWRITE, &oldProtect);
                    pThunkData->u1.Function = (SIZE_T)newFunc;
                    VirtualProtect(&pThunkData->u1.Function, sizeof(pThunkData->u1.Function), oldProtect, &oldProtect);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

// ============================================================================
// تفعيل البايباس الرئيسي
// ============================================================================

BOOL EnableR2Bypass() {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hAdvapi32 = GetModuleHandleA("advapi32.dll");
    HMODULE hUser32 = GetModuleHandleA("user32.dll");

    if (!hKernel32 || !hAdvapi32 || !hUser32) {
        return FALSE;
    }

    // الحصول على الدوال الأصلية
    OriginalCreateFileW = (pCreateFileW)GetProcAddress(hKernel32, "CreateFileW");
    OriginalRegOpenKeyExW = (pRegOpenKeyExW)GetProcAddress(hAdvapi32, "RegOpenKeyExW");
    OriginalRegQueryValueExW = (pRegQueryValueExW)GetProcAddress(hAdvapi32, "RegQueryValueExW");
    OriginalGetModuleHandleW = (pGetModuleHandleW)GetProcAddress(hKernel32, "GetModuleHandleW");
    OriginalLoadLibraryW = (pLoadLibraryW)GetProcAddress(hKernel32, "LoadLibraryW");
    OriginalGetFileAttributesExW = (pGetFileAttributesExW)GetProcAddress(hKernel32, "GetFileAttributesExW");
    OriginalGetSystemMetrics = (pGetSystemMetrics)GetProcAddress(hUser32, "GetSystemMetrics");
    OriginalFindWindowW = (pFindWindowW)GetProcAddress(hUser32, "FindWindowW");

    // تطبيق الـ Hooks على kernel32
    PatchImportTable((LPVOID)hKernel32, "kernel32.dll", "CreateFileW", (LPVOID)HookedCreateFileW);
    PatchImportTable((LPVOID)hKernel32, "kernel32.dll", "GetModuleHandleW", (LPVOID)HookedGetModuleHandleW);
    PatchImportTable((LPVOID)hKernel32, "kernel32.dll", "LoadLibraryW", (LPVOID)HookedLoadLibraryW);
    PatchImportTable((LPVOID)hKernel32, "kernel32.dll", "GetFileAttributesExW", (LPVOID)HookedGetFileAttributesExW);

    // تطبيق الـ Hooks على advapi32
    PatchImportTable((LPVOID)hAdvapi32, "advapi32.dll", "RegOpenKeyExW", (LPVOID)HookedRegOpenKeyExW);
    PatchImportTable((LPVOID)hAdvapi32, "advapi32.dll", "RegQueryValueExW", (LPVOID)HookedRegQueryValueExW);

    // تطبيق الـ Hooks على user32
    PatchImportTable((LPVOID)hUser32, "user32.dll", "GetSystemMetrics", (LPVOID)HookedGetSystemMetrics);
    PatchImportTable((LPVOID)hUser32, "user32.dll", "FindWindowW", (LPVOID)HookedFindWindowW);

    return TRUE;
}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        EnableR2Bypass();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
