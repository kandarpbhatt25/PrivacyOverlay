#include "CaptureMonitor.h"
#include <iostream>
#include <vector>
#include <iostream>
#include <objbase.h>
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>

// Initialize static variables
std::atomic<bool> CaptureMonitor::m_isDxgiCapturing(false);
TRACEHANDLE CaptureMonitor::m_sessionHandle = 0;
TRACEHANDLE CaptureMonitor::m_traceHandle = 0;
std::thread CaptureMonitor::m_etwThread;
bool CaptureMonitor::m_isListening = false;

// Microsoft-Windows-DXGI Provider GUID
// {CA11C036-0102-4A2D-A6AD-F03CFED5D3C9}
static const GUID DXGI_PROVIDER_GUID = 
{ 0xca11c036, 0x0102, 0x4a2d, { 0xa6, 0xad, 0xf0, 0x3c, 0xfe, 0xd5, 0xd3, 0xc9 } };

// --- Master Check ---
bool CaptureMonitor::IsScreenCaptureActive() {
    // If ANY of our layers detect a capture, we assume the screen is compromised.
    if (CheckHeuristics()) return true;
    if (CheckRegistryConsent()) return true;
    if (CheckETWTrace()) return true;
    
    return false;
}

// --- Layer 2: Heuristics (Window Overlays Only) ---
bool CaptureMonitor::CheckHeuristics() {
    // 1. WGC Host Window (Modern Screen Snipping / Teams / GameBar WGC)
    // This window ONLY exists during an active WGC capture session.
    if (FindWindowW(L"GraphicsCaptureHost", NULL) != NULL) {
        std::wcout << L"[Debug] WGC Active: GraphicsCaptureHost window found!" << std::endl;
        return true;
    }

    // 2. Zoom's sharing overlay (Green border)
    if (FindWindowW(L"ZPContentViewWndClass", NULL) != NULL) {
        std::wcout << L"[Debug] Zoom Active: Green border window found!" << std::endl;
        return true;
    }

    // We completely removed the CreateToolhelp32Snapshot process check here 
    // because background apps (like gamebar.exe) cause false positives.
    
    return false;
}

// --- Layer 3: Windows Privacy Registry (Active State Tracking) ---
bool CaptureMonitor::CheckRegistryConsent() {
    HKEY hKey;
    LPCWSTR keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\screen";
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        // Silently return false if the key doesn't exist (e.g., older Windows versions)
        return false; 
    }

    DWORD numSubKeys = 0;
    DWORD maxSubKeyLen = 0;
    RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &numSubKeys, &maxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

    bool isCaptured = false;
    
    // Iterate through all apps that have requested screen access
    for (DWORD i = 0; i < numSubKeys; i++) {
        DWORD nameLen = maxSubKeyLen + 1;
        std::vector<wchar_t> subKeyName(nameLen);
        
        if (RegEnumKeyExW(hKey, i, subKeyName.data(), &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY hSubKey;
            if (RegOpenKeyExW(hKey, subKeyName.data(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                
                // Read the LastUsedTimeStop value.
                FILETIME timeStop = {0};
                DWORD type = REG_QWORD;
                DWORD size = sizeof(FILETIME);
                
                if (RegQueryValueExW(hSubKey, L"LastUsedTimeStop", NULL, &type, (LPBYTE)&timeStop, &size) == ERROR_SUCCESS) {
                    // If LastUsedTimeStop is 0, the capture has started but hasn't stopped yet!
                    if (timeStop.dwLowDateTime == 0 && timeStop.dwHighDateTime == 0) {
                        std::wcout << L"[Debug] Registry Alert: App actively capturing -> " << subKeyName.data() << std::endl;
                        isCaptured = true;
                        RegCloseKey(hSubKey);
                        break;
                    }
                }
                RegCloseKey(hSubKey);
            }
        }
    }
    RegCloseKey(hKey);
    return isCaptured;
}

// --- Layer 1: ETW Trace Check ---
bool CaptureMonitor::CheckETWTrace() {
    return m_isDxgiCapturing.load();
}

// --- ETW Engine ---
void CaptureMonitor::StartETWListener() {
    if (m_isListening) return;
    
    m_isListening = true;
    m_etwThread = std::thread(ETWThreadWorker);
    std::wcout << L"[ETW] Kernel listener thread started." << std::endl;
}

void CaptureMonitor::StopETWListener() {
    if (!m_isListening) return;
    m_isListening = false;
    
    if (m_sessionHandle != 0) {
        EVENT_TRACE_PROPERTIES properties = { 0 };
        properties.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(L"PrivacyOverlayTrace");
        properties.Wnode.Guid = DXGI_PROVIDER_GUID;
        properties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        
        ControlTraceW(m_sessionHandle, L"PrivacyOverlayTrace", &properties, EVENT_TRACE_CONTROL_STOP);
    }
    
    if (m_traceHandle != 0) {
        CloseTrace(m_traceHandle);
    }

    if (m_etwThread.joinable()) {
        m_etwThread.join();
    }
    std::wcout << L"[ETW] Kernel listener stopped." << std::endl;
}

// The Background Thread that talks to the Windows Kernel
void CaptureMonitor::ETWThreadWorker() {
    ULONG bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(L"PrivacyOverlayTrace");
    EVENT_TRACE_PROPERTIES* properties = (EVENT_TRACE_PROPERTIES*)malloc(bufferSize);
    ZeroMemory(properties, bufferSize);

    properties->Wnode.BufferSize = bufferSize;
    properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    properties->Wnode.ClientContext = 1; // QPC clock resolution
    properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // 1. Start the trace session
    ULONG status = StartTraceW(&m_sessionHandle, L"PrivacyOverlayTrace", properties);
    if (status != ERROR_SUCCESS && status != ERROR_ALREADY_EXISTS) {
        std::wcout << L"[ETW Error] Failed to start trace. Are you running as Administrator? Code: " << status << std::endl;
        free(properties);
        return;
    }

    // 2. Enable the DXGI Provider
    status = EnableTraceEx2(m_sessionHandle, &DXGI_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0, 0, 0, NULL);
    if (status != ERROR_SUCCESS) {
        std::wcout << L"[ETW Error] Failed to enable DXGI provider." << std::endl;
    }

    // 3. Open the Trace
    EVENT_TRACE_LOGFILEW logFile = { 0 };
    logFile.LoggerName = (LPWSTR)L"PrivacyOverlayTrace";
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.EventRecordCallback = EventRecordCallback;

    m_traceHandle = OpenTraceW(&logFile);
    if (m_traceHandle == INVALID_PROCESSTRACE_HANDLE) {
        std::wcout << L"[ETW Error] Failed to open trace." << std::endl;
        free(properties);
        return;
    }

    std::wcout << L"[ETW] Successfully hooked into kernel DXGI events." << std::endl;

    // 4. Block and process events until stopped
    ProcessTrace(&m_traceHandle, 1, 0, 0);

    free(properties);
}

// The Callback fired by the Kernel whenever a DXGI event occurs
VOID WINAPI CaptureMonitor::EventRecordCallback(PEVENT_RECORD EventRecord) {
    // Event ID 42 & 43 in DXGI often correlate to OutputDuplication (Screen Capture)
    // For MVP, we flag ANY high-frequency DXGI activity as a potential capture.
    if (EventRecord->EventHeader.EventDescriptor.Id == 42 || 
        EventRecord->EventHeader.EventDescriptor.Id == 43) {
        
        // Reset a timer or flag indicating active capture
        m_isDxgiCapturing.store(true);
        
        // In a production app, you would parse the event payload (TDH) here 
        // to strictly confirm it's a duplication start/stop event.
    }
}