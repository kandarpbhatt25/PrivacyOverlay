#pragma once

// WIN32_LEAN_AND_MEAN excludes rarely-used services from Windows headers, 
// significantly speeding up build times and preventing macro collisions.
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <string>

namespace platform {

    class WinApiWrapper {
    public:
        // Extracts the window title text
        static std::wstring GetWindowTitle(HWND hwnd);

        // Retrieves the Process ID (PID) associated with the window
        static DWORD GetProcessIdFromWindow(HWND hwnd);

        // Retrieves the executable path of the given PID
        static std::wstring GetProcessExecutablePath(DWORD pid);

        // Filters out invisible, background, and system utility windows
        static bool IsWindowVisibleAndValid(HWND hwnd);
    };

} // namespace platform