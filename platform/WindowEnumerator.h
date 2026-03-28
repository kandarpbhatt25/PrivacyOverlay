#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>

namespace platform {

    // The clean C++ structure we will pass around our application
    struct WindowData {
        HWND hwnd;
        DWORD pid;
        std::wstring title;
        std::wstring executableName;
    };

    class WindowEnumerator {
    public:
        // The main entry point for this feature
        static std::vector<WindowData> EnumerateActiveWindows();

    private:
        // The required C-style callback for the Win32 EnumWindows function
        static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    };

} // namespace platform