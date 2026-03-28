#include "InjectionPayload.h"
#include <windows.h>
#include <process.h>

// Function to perform the actual masking
#include <fstream>

#include <stdio.h> // For swprintf_s

void ApplyMasking(bool enable) {
    DWORD targetAffinity = enable ? 0x00000011 : 0x00000000; // WDA_EXCLUDEFROMCAPTURE
    DWORD fallbackAffinity = enable ? 0x00000001 : 0x00000000; // WDA_MONITOR
    DWORD currentPid = GetCurrentProcessId();

    struct EnumData {
        DWORD pid;
        DWORD affinity;
        DWORD fallbackAffinity;
    };
    EnumData data = { currentPid, targetAffinity, fallbackAffinity };

    // Use EnumWindows to find all top-level windows of this process
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        EnumData* pData = (EnumData*)lp;
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == pData->pid) {
            DWORD currentAffinity = 0;
            GetWindowDisplayAffinity(hwnd, &currentAffinity);
            
            // Only apply if it's different to prevent spamming the DWM
            if (currentAffinity != pData->affinity && currentAffinity != pData->fallbackAffinity) {
                if (!SetWindowDisplayAffinity(hwnd, pData->affinity)) {
                    if (pData->fallbackAffinity != pData->affinity) {
                        SetWindowDisplayAffinity(hwnd, pData->fallbackAffinity);
                    }
                }
            }
        }
        return TRUE;
    }, (LPARAM)&data);
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