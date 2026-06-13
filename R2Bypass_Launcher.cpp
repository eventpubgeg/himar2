#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <time.h>
#include <shellapi.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

// الألوان للواجهة
#define COLOR_GOLD "\033[38;5;220m"
#define COLOR_SILVER "\033[38;5;249m"
#define COLOR_BLACK "\033[30m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"

// البيانات الإحصائية
struct BypassStats {
    int totalBypassesApplied;
    int successfulBypass;
    int failedBypass;
    char lastEmulatorName[256];
    char lastGameVersion[50];
    time_t lastBypassTime;
};

BypassStats gStats = {0, 0, 0, "", "", 0};

// دالة طباعة الرأس
void PrintHeader() {
    system("cls");
    printf("\n");
    printf("  %s╔════════════════════════════════════════════════════════╗%s\n", COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s                                                        %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s██████╗ %s██████╗ %s ██████╗ ██╗   ██╗██████╗  %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s██╔══██╗██╔═══██╗██╔════╝ ╚██╗ ██╔╝██╔══██╗ %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s██████╔╝██║   ██║██║  ███╗ ╚████╔╝ ██████╔╝ %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s██╔══██╗██║   ██║██║   ██║  ╚██╔╝  ██╔═══╝  %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s██║  ██║╚██████╔╝╚██████╔╝   ██║   ██║      %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s      %s╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ╚═╝   ╚═╝      %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_SILVER, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s                                                        %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s╚════════════════════════════════════════════════════════╝%s\n", COLOR_GOLD, COLOR_RESET);
    printf("\n");
    printf("  %s════════════════════════════════════════════════════════%s\n", COLOR_SILVER, COLOR_RESET);
    printf("  %s|  🎮 Advanced Emulator Bypass for PUBG Mobile  v2.0  |%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s════════════════════════════════════════════════════════%s\n\n", COLOR_SILVER, COLOR_RESET);
}

// دالة البحث عن العمليات
DWORD FindProcessByName(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

// دالة الحقن البعيد
BOOL InjectDLL(DWORD targetPID, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
    if (!hProcess) {
        printf("%s[✗] فشل: لا يمكن فتح العملية%s\n", COLOR_RED, COLOR_RESET);
        return FALSE;
    }

    // تخصيص ذاكرة
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        printf("%s[✗] فشل: لا يمكن تخصيص الذاكرة%s\n", COLOR_RED, COLOR_RESET);
        CloseHandle(hProcess);
        return FALSE;
    }

    // كتابة مسار DLL
    if (!WriteProcessMemory(hProcess, pDllPath, (void*)dllPath, strlen(dllPath) + 1, NULL)) {
        printf("%s[✗] فشل: لا يمكن كتابة البيانات%s\n", COLOR_RED, COLOR_RESET);
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    // إنشاء thread بعيد
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA"),
        pDllPath, 0, NULL);

    if (!hThread) {
        printf("%s[✗] فشل: لا يمكن إنشاء thread بعيد%s\n", COLOR_RED, COLOR_RESET);
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    // انتظار الانتهاء
    WaitForSingleObject(hThread, INFINITE);

    // تنظيف
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return TRUE;
}

// عرض القائمة
void DisplayMenu() {
    printf("  %s┌──── اختر المحاكي ────┐%s\n", COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  1. GameLoop          %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  2. SmartGaGa         %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  3. MEmu              %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  4. BlueStacks        %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  5. Nox Player        %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s└────────────────────────┘%s\n\n", COLOR_GOLD, COLOR_RESET);

    printf("  %s┌──── اختر إصدار اللعبة ────┐%s\n", COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  1. Global (GL)         %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  2. Korea (KR)          %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  3. Taiwan (TW)         %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s│%s  4. Vietnam (VN)        %s│%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s└─────────────────────────────┘%s\n\n", COLOR_GOLD, COLOR_RESET);
}

// عرض الإحصائيات
void DisplayStats() {
    printf("\n  %s╔════════════════════════════════════════╗%s\n", COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s        📊 إحصائيات البايباس              %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s╠════════════════════════════════════════╣%s\n", COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s  إجمالي التطبيقات: %d                %s║%s\n", COLOR_GOLD, COLOR_RESET, gStats.totalBypassesApplied, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s  النجاحات: %s%d%s                    %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_GREEN, gStats.successfulBypass, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s  الفشل: %s%d%s                      %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_RED, gStats.failedBypass, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s║%s  آخر محاكي: %s%s%s                  %s║%s\n", COLOR_GOLD, COLOR_RESET, COLOR_YELLOW, gStats.lastEmulatorName, COLOR_RESET, COLOR_GOLD, COLOR_RESET);
    printf("  %s╚════════════════════════════════════════╝%s\n", COLOR_GOLD, COLOR_RESET);
}

// الدالة الرئيسية
int main(int argc, char* argv[]) {
    // إعداد الـ console
    system("mode con: cols=70 lines=30");
    system("title R2 BYPASS - PUBG Mobile Advanced Emulator Bypass");

    char emulators[][256] = {
        "GameLoop",
        "SmartGaGa.exe",
        "MemuPlayer.exe",
        "BlueStacks",
        "nox.exe"
    };

    char gameVersions[][50] = {"GL", "KR", "TW", "VN"};

    int choice1, choice2;
    DWORD targetPID;
    char dllPath[512];

    // تحويل لـ Unicode
    wchar_t emulatorsW[5][256];
    for (int i = 0; i < 5; i++) {
        MultiByteToWideChar(CP_ACP, 0, emulators[i], -1, emulatorsW[i], 256);
    }

    while (true) {
        PrintHeader();
        DisplayMenu();

        printf("  %s➤ اختر المحاكي [1-5]: %s", COLOR_YELLOW, COLOR_RESET);
        scanf("%d", &choice1);

        if (choice1 < 1 || choice1 > 5) {
            printf("  %s[✗] اختيار غير صحيح!%s\n", COLOR_RED, COLOR_RESET);
            system("pause");
            continue;
        }

        printf("  %s➤ اختر إصدار اللعبة [1-4]: %s", COLOR_YELLOW, COLOR_RESET);
        scanf("%d", &choice2);

        if (choice2 < 1 || choice2 > 4) {
            printf("  %s[✗] اختيار غير صحيح!%s\n", COLOR_RED, COLOR_RESET);
            system("pause");
            continue;
        }

        // البحث عن العملية
        printf("\n  %s⏳ جاري البحث عن العملية...%s\n", COLOR_YELLOW, COLOR_RESET);
        targetPID = FindProcessByName(emulatorsW[choice1 - 1]);

        if (targetPID == 0) {
            printf("  %s[✗] لم يتم العثور على %s. تأكد من تشغيل المحاكي!%s\n", COLOR_RED, emulators[choice1 - 1], COLOR_RESET);
            system("pause");
            continue;
        }

        printf("  %s[✓] تم العثور على العملية (PID: %lu)%s\n", COLOR_GREEN, targetPID, COLOR_RESET);

        // البحث عن DLL
        GetModuleFileNameA(NULL, dllPath, sizeof(dllPath));
        strrchr(dllPath, '\\')[1] = '\0';
        strcat_s(dllPath, sizeof(dllPath), "R2Bypass.dll");

        if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
            printf("  %s[✗] لم يتم العثور على R2Bypass.dll%s\n", COLOR_RED, COLOR_RESET);
            system("pause");
            continue;
        }

        // الحقن
        printf("  %s⏳ جاري حقن البايباس...%s\n", COLOR_YELLOW, COLOR_RESET);

        if (InjectDLL(targetPID, dllPath)) {
            printf("  %s[✓] تم تفعيل البايباس بنجاح!%s\n", COLOR_GREEN, COLOR_RESET);
            gStats.successfulBypass++;
            strcpy_s(gStats.lastEmulatorName, sizeof(gStats.lastEmulatorName), emulators[choice1 - 1]);
            strcpy_s(gStats.lastGameVersion, sizeof(gStats.lastGameVersion), gameVersions[choice2 - 1]);
            gStats.lastBypassTime = time(NULL);
        } else {
            printf("  %s[✗] فشل تفعيل البايباس!%s\n", COLOR_RED, COLOR_RESET);
            gStats.failedBypass++;
        }

        gStats.totalBypassesApplied++;

        DisplayStats();

        printf("\n  %s[1] تطبيق مرة أخرى  [2] خروج : %s", COLOR_YELLOW, COLOR_RESET);
        int exitChoice;
        scanf("%d", &exitChoice);

        if (exitChoice == 2) {
            printf("\n  %s✓ شكراً لاستخدام R2 BYPASS!%s\n\n", COLOR_GREEN, COLOR_RESET);
            break;
        }
    }

    return 0;
}
