#include "ActiveProcessMonitor.h"
#include "../persistence/AppCacheManager.h"
#include <windows.h>
#include <psapi.h>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "version.lib")

namespace platform {

ActiveProcessMonitor::ActiveProcessMonitor(QObject* parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ActiveProcessMonitor::pollActiveProcesses);
}

ActiveProcessMonitor::~ActiveProcessMonitor() {
    stopMonitoring();
}

void ActiveProcessMonitor::startMonitoring(int intervalMs) {
    if (!m_timer->isActive()) {
        m_timer->start(intervalMs);
        qDebug() << "[ActiveProcessMonitor] Started polling every" << intervalMs << "ms";
        // Optionally trigger instantly once
        QTimer::singleShot(5000, this, &ActiveProcessMonitor::pollActiveProcesses);
    }
}

void ActiveProcessMonitor::stopMonitoring() {
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

bool ActiveProcessMonitor::extractMetadata(const QString& exePath, models::CachedApp& appOut) {
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW((LPCWSTR)exePath.utf16(), &dummy);
    if (size == 0) return false;

    QByteArray data(size, 0);
    if (!GetFileVersionInfoW((LPCWSTR)exePath.utf16(), dummy, size, data.data())) return false;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;

    UINT cbTranslate;

    if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) return false;

    QString productName;
    for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++) {
        QString subBlock = QString::asprintf("\\StringFileInfo\\%04x%04x\\ProductName",
            lpTranslate[i].wLanguage,
            lpTranslate[i].wCodePage);

        void* pvProductName = nullptr;
        UINT cbProductName = 0;

        if (VerQueryValueW(data.data(), (LPCWSTR)subBlock.utf16(), &pvProductName, &cbProductName) && cbProductName > 0) {
            productName = QString::fromWCharArray((LPCWSTR)pvProductName);
            if (!productName.trimmed().isEmpty()) break;
        }
    }

    if (productName.isEmpty()) return false;

    appOut.displayName = productName.trimmed();
    appOut.displayIcon = exePath; // Use the exe for the icon provider
    appOut.executableName = QFileInfo(exePath).fileName();
    appOut.isUWP = false;
    appOut.inferred = true;

    return true;
}

int ActiveProcessMonitor::calculateConfidence(const QString& exePath, const models::CachedApp& app) {
    int score = 0;
    QString lowerPath = exePath.toLower();

    // Positive signals
    if (lowerPath.contains("program files") || lowerPath.contains("progra~1") || lowerPath.contains("progra~2")) score += 3;
    if (lowerPath.contains("appdata")) score += 2;
    if (!app.displayName.isEmpty()) score += 3;

    // Negative signals (Auto rejects usually)
    if (lowerPath.contains("c:/windows") || lowerPath.contains("c:\\windows")) score -= 10;
    if (lowerPath.contains("temp") || lowerPath.contains("tmp")) score -= 5;
    
    // Non-gui background executables / services
    QString lowerExe = app.executableName.toLower();
    if (lowerExe == "svchost.exe" || lowerExe == "conhost.exe" || lowerExe == "cmd.exe" || lowerExe == "powershell.exe") score -= 10;
    if (lowerExe.contains("update") || lowerExe.contains("crash") || lowerExe.contains("helper") || lowerExe.contains("service")) score -= 5;

    return score;
}

void ActiveProcessMonitor::pollActiveProcesses() {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    
    QList<models::CachedApp> currentCache = persistence::AppCacheManager::getInstance().loadCache();
    QSet<QString> knownPaths;
    for (const auto& app : currentCache) {
        knownPaths.insert(app.executableName.toLower());
        QFileInfo fi(app.displayIcon);
        if (fi.exists()) {
            knownPaths.insert(fi.absoluteFilePath().toLower());
        }
    }

    QList<models::CachedApp> newlyInferred;

    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, aProcesses[i]);
            if (hProcess) {
                WCHAR szProcessName[MAX_PATH];
                DWORD nameLen = MAX_PATH;
                if (QueryFullProcessImageNameW(hProcess, 0, szProcessName, &nameLen)) {
                    QString exePath = QString::fromWCharArray(szProcessName);
                    QString lowerPath = exePath.toLower();
                    QString lowerExe = QFileInfo(exePath).fileName().toLower();

                    if (!knownPaths.contains(lowerExe) && !knownPaths.contains(lowerPath) && !m_notifiedExes.contains(lowerPath)) {
                        models::CachedApp inferredApp;
                        if (extractMetadata(exePath, inferredApp)) {
                            int confidence = calculateConfidence(exePath, inferredApp);
                            if (confidence >= 3) {
                                m_notifiedExes.insert(lowerPath);
                                newlyInferred.append(inferredApp);
                                qDebug() << "[ActiveProcessMonitor] High confidence inferred app:" << inferredApp.displayName << exePath;
                            }
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }

    if (!newlyInferred.isEmpty()) {
        emit newInferredAppsDiscovered(newlyInferred);
    }
}

}
