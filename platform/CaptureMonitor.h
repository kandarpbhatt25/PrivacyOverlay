#ifndef CAPTUREMONITOR_H
#define CAPTUREMONITOR_H

#include <windows.h>
#include <evntrace.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>

class CaptureMonitor {
public:
    static bool IsScreenCaptureActive();
    
    // New ETW Lifecycle functions
    static void StartETWListener();
    static void StopETWListener();

private:
    static bool CheckHeuristics();
    static bool CheckRegistryConsent();
    static bool CheckETWTrace(); 

    // ETW Variables
    static std::atomic<bool> m_isDxgiCapturing;
    static TRACEHANDLE m_sessionHandle;
    static TRACEHANDLE m_traceHandle;
    static std::thread m_etwThread;
    static bool m_isListening;

    // ETW Callbacks and Worker
    static void ETWThreadWorker();
    static VOID WINAPI EventRecordCallback(PEVENT_RECORD EventRecord);
};

#endif // CAPTUREMONITOR_H