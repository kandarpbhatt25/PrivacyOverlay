#include "WindowEnumerator.h"
#include "WinApiWrapper.h"

namespace platform {

    std::vector<WindowData> WindowEnumerator::EnumerateActiveWindows() {
        std::vector<WindowData> windows;

        // Pass the memory address of our vector via lParam so the callback can access it
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

        return windows;
    }

    BOOL CALLBACK WindowEnumerator::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        // Cast lParam back to our vector pointer
        auto* windows = reinterpret_cast<std::vector<WindowData>*>(lParam);

        // 1. Validation: Skip invisible or junk windows immediately
        if (!WinApiWrapper::IsWindowVisibleAndValid(hwnd)) {
            return TRUE; // Continue enumeration
        }

        // 2. Fetch the Title
        std::wstring title = WinApiWrapper::GetWindowTitle(hwnd);
        if (title.empty()) {
            return TRUE; // Ignore windows without titles
        }

        // 3. Fetch the PID
        DWORD pid = WinApiWrapper::GetProcessIdFromWindow(hwnd);
        if (pid == 0) {
            return TRUE; // Ignore if we can't tie it to a process
        }

        // 4. Fetch Executable Path
        std::wstring execPath = WinApiWrapper::GetProcessExecutablePath(pid);

        // We add the window even if execPath is empty (due to Access Denied).
        // The title and HWND are still valuable for masking.

        // 5. Package and store
        WindowData data;
        data.hwnd = hwnd;
        data.pid = pid;
        data.title = title;
        data.executableName = execPath;

        windows->push_back(data);

        return TRUE; // Continue to the next window
    }

} // namespace platform