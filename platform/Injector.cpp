#include "Injector.h"
#include <iostream>

namespace platform {

bool Injector::SetDebugPrivilege(bool enable) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);
    return result && (GetLastError() == ERROR_SUCCESS);
}

bool Injector::InjectDLL(DWORD pid, const std::string& dllPath) {
    // 1. Elevate privileges to talk to other apps
    SetDebugPrivilege(true);

    // 2. Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[Injector] Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    // 3. Allocate memory inside the target process for the DLL path string
    LPVOID remoteBuf = VirtualAllocEx(hProcess, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuf) {
        CloseHandle(hProcess);
        return false;
    }

    // 4. Write the DLL path into the target process's memory
    if (!WriteProcessMemory(hProcess, remoteBuf, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 5. Get the address of LoadLibraryA from kernel32.dll
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    // 6. Create a remote thread in the target process that calls LoadLibraryA(remoteBuf)
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteBuf, 0, NULL);
    
    if (!hThread) {
        std::cerr << "[Injector] CreateRemoteThread failed. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for the thread to finish loading the DLL
    WaitForSingleObject(hThread, INFINITE);

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    std::cout << "[Injector] Successfully injected DLL into PID: " << pid << std::endl;
    return true;
}

} // namespace platform