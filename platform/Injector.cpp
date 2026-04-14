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
    if (pid == 0 || dllPath.empty()) return false;

    // 1. Elevate privileges to talk to other apps
    SetDebugPrivilege(true);

    // Provide an initial graceful check. If this fails, no use trying PROCESS_ALL_ACCESS
    HANDLE hTest = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hTest) {
        std::cerr << "[Injector] Graceful Refusal: Cannot access PID " << pid << " due to high privileges/protection." << std::endl;
        return false;
    }
    CloseHandle(hTest);

    // 2. Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "[Injector] Failed to open process for injection. Error: " << GetLastError() << std::endl;
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
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibraryAddr) {
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 6. Create a remote thread in the target process that calls LoadLibraryA(remoteBuf)
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteBuf, 0, NULL);
    
    if (!hThread) {
        std::cerr << "[Injector] CreateRemoteThread failed. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for the thread to finish loading the DLL
    WaitForSingleObject(hThread, 5000); // 5 sec timeout instead of INFINITE to prevent blocking UI if it hangs

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    std::cout << "[Injector] Successfully injected DLL into PID: " << pid << std::endl;
    return true;
}

} // namespace platform