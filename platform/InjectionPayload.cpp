#include "InjectionPayload.h"
#include <windows.h>
#include <process.h>

// Function to perform the actual masking
#include <fstream>

#include <stdio.h> // For swprintf_s

#include <vector>

void ApplyMasking(bool enable) {
    DWORD targetAffinity = enable ? 0x00000011 : 0x00000000; // WDA_EXCLUDEFROMCAPTURE
    DWORD fallbackAffinity = enable ? 0x00000001 : 0x00000000; // WDA_MONITOR
    DWORD currentPid = GetCurrentProcessId();

    struct EnumData {
        DWORD pid;
        std::vector<HWND> hwnds;
    };
    EnumData data;
    data.pid = currentPid;

    // Pass 1: Collect windows safely inside EnumWindows without sending synchronous messages
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        EnumData* pData = (EnumData*)lp;
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == pData->pid) {
            pData->hwnds.push_back(hwnd);
        }
        return TRUE;
    }, (LPARAM)&data);

    // Pass 2: Apply affinity outside of the EnumWindows lock to prevent deadlocks
    for (HWND hwnd : data.hwnds) {
        DWORD desiredAffinity = targetAffinity;

        if (!IsWindowVisible(hwnd)) {
            desiredAffinity = 0;
        } else {
            wchar_t className[256];
            if (GetClassNameW(hwnd, className, 256) > 0) {
                if (wcscmp(className, L"Ghost") == 0) {
                    desiredAffinity = 0;
                }
            }
        }

        DWORD currentAffinity = 0;
        GetWindowDisplayAffinity(hwnd, &currentAffinity);
        
        if (currentAffinity != desiredAffinity) {
            if (!SetWindowDisplayAffinity(hwnd, desiredAffinity)) {
                if (desiredAffinity != 0 && fallbackAffinity != desiredAffinity) {
                    SetWindowDisplayAffinity(hwnd, fallbackAffinity);
                }
            }
        }
    }
}

// Global flag to stop thread cleanly
bool g_runThread = true;

// Thread function to run safely outside the Loader Lock
unsigned int __stdcall MaskingThread(void* param) {
    DWORD currentPid = GetCurrentProcessId();
    wchar_t eventName[256];
    swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", currentPid);

    // Create or Open the named event. It acts as our IPC channel.
    HANDLE hEvent = CreateEventW(NULL, TRUE, TRUE, eventName); 

    while (g_runThread) {
        bool shouldMask = false;
        if (hEvent) {
            DWORD waitRes = WaitForSingleObject(hEvent, 0);
            shouldMask = (waitRes == WAIT_OBJECT_0);
        }

        ApplyMasking(shouldMask);
        Sleep(500); // Check loop every 500ms
    }
    
    if (hEvent) CloseHandle(hEvent);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        _beginthreadex(NULL, 0, MaskingThread, NULL, 0, NULL);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        g_runThread = false;
    }
    return TRUE;
}