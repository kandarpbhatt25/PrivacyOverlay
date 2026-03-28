#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace platform {

class Injector {
public:
    /**
     * Injects the specified DLL into a target process.
     * @param pid The Process ID of the target application.
     * @param dllPath The full absolute path to the InjectionPayload.dll.
     * @return True if injection succeeded, false otherwise.
     */
    static bool InjectDLL(DWORD pid, const std::string& dllPath);

private:
    // Helper to escalate privileges (required for many apps)
    static bool SetDebugPrivilege(bool enable);
};

} // namespace platform