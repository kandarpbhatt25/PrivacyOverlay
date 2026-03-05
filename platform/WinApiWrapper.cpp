#include "WinApiWrapper.h"
#include <iostream> // Temporary: Using std::wcerr until Logger.h is built
#include <vector>

namespace platform {

    std::wstring WinApiWrapper::GetWindowTitle(HWND hwnd) {
        if (!hwnd) return L"";

        int length = GetWindowTextLengthW(hwnd);
        if (length == 0) {
            return L""; // Window has no title or an error occurred
        }

        // Allocate buffer (+1 for null terminator)
        std::vector<wchar_t> buffer(length + 1);
        if (GetWindowTextW(hwnd, buffer.data(), buffer.size())) {
            return std::wstring(buffer.data());
        }

        // std::wcerr << L"[DEBUG] Failed to get window text for HWND: " << hwnd << L"\n";
        return L"";
    }

    DWORD WinApiWrapper::GetProcessIdFromWindow(HWND hwnd) {
        if (!hwnd) return 0;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == 0) {
            // std::wcerr << L"[DEBUG] Failed to get PID for HWND: " << hwnd << L"\n";
        }

        return pid;
    }

    std::wstring WinApiWrapper::GetProcessExecutablePath(DWORD pid) {
        if (pid == 0) return L"";

        // PROCESS_QUERY_LIMITED_INFORMATION is critical here. It requires fewer privileges
        // than PROCESS_QUERY_INFORMATION, reducing "Access Denied" errors for elevated apps.
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) {
            // std::wcerr << L"[WARNING] Access Denied opening process ID " << pid << L"\n";
            return L"";
        }

        std::vector<wchar_t> buffer(MAX_PATH);
        DWORD bufferSize = static_cast<DWORD>(buffer.size());

        // QueryFullProcessImageNameW is preferred over older PSAPI functions
        if (QueryFullProcessImageNameW(hProcess, 0, buffer.data(), &bufferSize)) {
            CloseHandle(hProcess);
            return std::wstring(buffer.data(), bufferSize);
        }

        // std::wcerr << L"[WARNING] Failed to query image name for process ID " << pid << L"\n";
        CloseHandle(hProcess);
        return L"";
    }

    bool WinApiWrapper::IsWindowVisibleAndValid(HWND hwnd) {
        if (!hwnd) return false;

        // 1. Must be visible
        if (!IsWindowVisible(hwnd)) {
            return false;
        }

        // 2. Filter out tool windows (like background utility overlays)
        LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (exStyle & WS_EX_TOOLWINDOW) {
            return false;
        }

        // 3. We only want top-level windows. If it has an owner, it's a dialog/popup, 
        // which usually isn't what we want to track as a main application window.
        HWND owner = GetWindow(hwnd, GW_OWNER);
        if (owner != NULL) {
            return false;
        }

        return true;
    }

} // namespace platform