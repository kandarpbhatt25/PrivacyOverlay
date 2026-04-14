#pragma once

#include <QObject>
#include <vector>
#include <string>
#include <windows.h>

namespace core {

class MaskEngine : public QObject {
    Q_OBJECT

public:
    explicit MaskEngine(QObject *parent = nullptr);

    /**
     * Finds the absolute path of the InjectionPayload.dll relative to the app.
     */
    std::string getPayloadPath() const;

    /**
     * Orchestrates the injection into a specific process.
     * @param pid The Process ID to shield.
     * @return True if injection succeeded, false otherwise.
     */
    bool applyShieldToProcess(DWORD pid);

    /**
     * Iterates through a list of HWNDs and applies the shield via the injector.
     */
    void maskSensitiveWindows(const std::vector<HWND>& hwnds);

signals:
    void maskApplied(DWORD pid);
    void maskError(DWORD pid, const QString& message);

private:
    std::string m_payloadPath;
};

} // namespace core