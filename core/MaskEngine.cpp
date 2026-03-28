#include "MaskEngine.h"
#include "../platform/Injector.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace core {

MaskEngine::MaskEngine(QObject *parent) : QObject(parent) {
    // Determine the path to the DLL. 
    // Usually, it's in the same folder as your .exe
    QString appDir = QCoreApplication::applicationDirPath();
    m_payloadPath = QDir::toNativeSeparators(appDir + "/InjectionPayload.dll").toStdString();
}

std::string MaskEngine::getPayloadPath() const {
    return m_payloadPath;
}

void MaskEngine::applyShieldToProcess(DWORD pid) {
    if (pid == 0) return;

    qDebug() << "[MaskEngine] Attempting to shield PID:" << pid;

    // Call our platform-specific injector
    if (platform::Injector::InjectDLL(pid, m_payloadPath)) {
        qDebug() << "[MaskEngine] Shield successfully injected into PID:" << pid;
        emit maskApplied(pid);
    } else {
        qDebug() << "[MaskEngine] Failed to inject shield into PID:" << pid;
        emit maskError(pid, "Injection failed. Check Admin privileges.");
    }
}

void MaskEngine::maskSensitiveWindows(const std::vector<HWND>& hwnds) {
    // For each window we want to hide, we find its owner PID and inject
    for (HWND hwnd : hwnds) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        
        if (pid != 0 && pid != GetCurrentProcessId()) {
            applyShieldToProcess(pid);
        }
    }
}

} // namespace core