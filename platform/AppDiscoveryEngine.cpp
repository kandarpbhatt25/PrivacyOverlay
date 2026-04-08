#include "AppDiscoveryEngine.h"
#include "../persistence/AppCacheManager.h"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QSet>
#include <algorithm>
#include <QDirIterator>

namespace platform {

// --- Engine ---
AppDiscoveryEngine::AppDiscoveryEngine(QObject* parent) : QObject(parent), workerThread(nullptr), worker(nullptr) {}

AppDiscoveryEngine::~AppDiscoveryEngine() {
    if (workerThread) {
        workerThread->requestInterruption();
        workerThread->quit();
        if (!workerThread->wait(1500)) {
            workerThread->terminate();
            workerThread->wait();
        }
    }
}

void AppDiscoveryEngine::startDiscovery() {
    if (workerThread && workerThread->isRunning()) return;

    workerThread = new QThread(this);
    worker = new AppDiscoveryWorker();
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker, &AppDiscoveryWorker::doWork);
    connect(worker, &AppDiscoveryWorker::appsDiscovered, this, &AppDiscoveryEngine::newAppsFound);
    connect(worker, &AppDiscoveryWorker::finished, workerThread, &QThread::quit);
    connect(worker, &AppDiscoveryWorker::finished, worker, &QObject::deleteLater);

    workerThread->start();
}

// --- Worker ---
void AppDiscoveryWorker::doWork() {
    QList<models::CachedApp> allApps;
    
    // 1. Proof of Life check on existing cache
    QList<models::CachedApp> currentCache = persistence::AppCacheManager::getInstance().loadCache();
    for (const auto& app : currentCache) {
        if (!app.executablePath.isEmpty() && QFile::exists(app.executablePath)) {
            allApps.append(app);
        } else if (app.executablePath.isEmpty() && !app.displayIcon.isEmpty() && QFile::exists(app.displayIcon)) {
            allApps.append(app);
        } else if (app.source == models::AppSource::UWP) {
            allApps.append(app); // Can't easily verify UWP existences with just paths sometimes, so keep them to be overridden or verified later.
        }
    }

    // Scan all four zones in sequence
    allApps.append(scanRegistry());
    allApps.append(scanStartMenu());
    allApps.append(scanLocalAppData());
    allApps.append(scanUWP());

    // Filter duplicates and prepare final list
    QList<models::CachedApp> uniqueApps;
    QSet<QString> seenExes;
    for (const auto& app : allApps) {
        QString deduplicationKey = app.executableName.toLower();
        if (!seenExes.contains(deduplicationKey)) {
            seenExes.insert(deduplicationKey);
            uniqueApps.append(app);
        }
        else {
            // Priority given to Registry or UWP over inferred apps if duplicated
            if (app.source == models::AppSource::Registry || app.source == models::AppSource::UWP) {
                for (int i = 0; i < uniqueApps.size(); ++i) {
                    if (uniqueApps[i].executableName.toLower() == deduplicationKey && uniqueApps[i].inferred) {
                        uniqueApps[i] = app;
                        break;
                    }
                }
            }
        }
    }

    // Sort alphabetically by display name
    std::sort(uniqueApps.begin(), uniqueApps.end(), [](const models::CachedApp& a, const models::CachedApp& b) {
        return a.displayName.compare(b.displayName, Qt::CaseInsensitive) < 0;
    });

    // DIFF against cache
    QList<models::CachedApp> oldCache = persistence::AppCacheManager::getInstance().loadCache();
    
    bool needsUpdate = uniqueApps.size() != oldCache.size();
    if (!needsUpdate) {
        for (int i = 0; i < uniqueApps.size(); ++i) {
            if (uniqueApps[i].executableName != oldCache[i].executableName ||
                uniqueApps[i].displayName != oldCache[i].displayName ||
                uniqueApps[i].displayIcon != oldCache[i].displayIcon) {
                needsUpdate = true;
                break;
            }
        }
    }

    if (needsUpdate) {
        persistence::AppCacheManager::getInstance().saveCache(uniqueApps);
        emit appsDiscovered(uniqueApps); // Emit full list to refresh UI
    }

    emit finished();
}

QList<models::CachedApp> AppDiscoveryWorker::scanRegistry() {
    QList<models::CachedApp> apps;
    QStringList paths = {
        "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    for (const QString& basePath : paths) {
        QSettings reg(basePath, QSettings::NativeFormat);
        for (const QString& group : reg.childGroups()) {
            reg.beginGroup(group);
            QString displayName = reg.value("DisplayName").toString();
            QString displayIcon = reg.value("DisplayIcon").toString();
            QString installLocation = reg.value("InstallLocation").toString();
            reg.endGroup();

            if (displayName.isEmpty()) continue;
            
            QString exePath;
            QString validIconPath;

            if (!displayIcon.isEmpty()) {
                QString tempIcon = displayIcon;
                tempIcon.remove("\"");
                int commaIdx = tempIcon.lastIndexOf(',');
                if (commaIdx != -1) tempIcon = tempIcon.left(commaIdx);
                
                if (QFile::exists(tempIcon)) {
                    validIconPath = tempIcon;
                }

                if (tempIcon.endsWith(".exe", Qt::CaseInsensitive)) {
                    exePath = tempIcon;
                }
            }

            if (exePath.isEmpty() && !installLocation.isEmpty()) {
                installLocation.remove("\"");
                QDir dir(installLocation);
                if (dir.exists()) {
                    QStringList exes = dir.entryList(QStringList() << "*.exe", QDir::Files);
                    if (!exes.isEmpty()) {
                        qint64 maxSize = 0;
                        for (const QString& exeName : exes) {
                            if (exeName.contains("unins", Qt::CaseInsensitive) || 
                                exeName.contains("update", Qt::CaseInsensitive) || 
                                exeName.contains("crash", Qt::CaseInsensitive)) {
                                continue;
                            }
                            QFileInfo fi(dir.filePath(exeName));
                            if (fi.size() > maxSize) {
                                maxSize = fi.size();
                                exePath = dir.filePath(exeName);
                            }
                        }
                    }
                }
            }

            if (exePath.isEmpty()) continue;

            QString exeName = QFileInfo(exePath).fileName();

            models::CachedApp app;
            app.displayName = displayName;
            app.displayIcon = !validIconPath.isEmpty() ? validIconPath : exePath;
            app.executablePath = exePath;
            app.executableName = exeName;
            app.source = models::AppSource::Registry;
            apps.append(app);
        }
    }
    return apps;
}

QList<models::CachedApp> AppDiscoveryWorker::scanStartMenu() {
    QList<models::CachedApp> apps;
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);

    for (const QString& path : paths) {
        QDirIterator it(path, QStringList() << "*.lnk", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo lnkInfo = it.fileInfo();
            QString targetPath = lnkInfo.symLinkTarget();
            
            if (targetPath.isEmpty() || !targetPath.endsWith(".exe", Qt::CaseInsensitive)) {
                continue;
            }

            QString exeName = QFileInfo(targetPath).fileName();
            if (exeName.contains("unins", Qt::CaseInsensitive) || 
                exeName.contains("update", Qt::CaseInsensitive)) {
                continue;
            }

            models::CachedApp app;
            app.displayName = lnkInfo.completeBaseName();
            app.displayIcon = lnkInfo.absoluteFilePath();
            app.executablePath = targetPath;
            app.executableName = exeName;
            app.source = models::AppSource::StartMenu;
            apps.append(app);
        }
    }
    return apps;
}

QList<models::CachedApp> AppDiscoveryWorker::scanLocalAppData() {
    QList<models::CachedApp> apps;
    QDir localDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    localDir.mkpath("AppData/Local/Programs");
    localDir.cd("AppData/Local/Programs");

    for (const QFileInfo& dirInfo : localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir appDir(dirInfo.absoluteFilePath());
        for (const QFileInfo& fileInfo : appDir.entryInfoList(QStringList() << "*.exe", QDir::Files)) {
            QString exeName = fileInfo.fileName();
            if (exeName.contains("update", Qt::CaseInsensitive) || exeName.contains("installer", Qt::CaseInsensitive) || exeName.contains("unins", Qt::CaseInsensitive)) {
                continue;
            }

            models::CachedApp app;
            app.displayName = dirInfo.fileName(); 
            app.displayIcon = fileInfo.absoluteFilePath();
            app.executablePath = fileInfo.absoluteFilePath();
            app.executableName = exeName;
            app.source = models::AppSource::LocalAppData;
            apps.append(app);
            break; 
        }
    }

    QDir waDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/AppData/Local/WhatsApp");
    if (waDir.exists()) {
        QFileInfo exe(waDir.filePath("WhatsApp.exe"));
        if (exe.exists()) {
            models::CachedApp app;
            app.displayName = "WhatsApp Classic";
            app.displayIcon = exe.absoluteFilePath();
            app.executablePath = exe.absoluteFilePath();
            app.executableName = "WhatsApp.exe";
            app.source = models::AppSource::LocalAppData;
            apps.append(app);
        }
    }

    return apps;
}

QList<models::CachedApp> AppDiscoveryWorker::scanUWP() {
    QList<models::CachedApp> apps;
    QProcess p;
    // We execute Powershell silently to snatch the Windows Apps list
    p.start("powershell", QStringList() << "-NoProfile" << "-Command" << "Get-AppxPackage | Where-Object { $_.InstallLocation -ne $null -and $_.IsFramework -eq $False } | Select-Object Name, InstallLocation | ConvertTo-Json");
    p.waitForFinished(15000); // Allow up to 15s for large systems

    QByteArray output = p.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);

    auto parseUWP = [&](const QJsonObject& obj) {
        QString name = obj["Name"].toString();
        int lastDotIndex = name.lastIndexOf('.');
        if (lastDotIndex != -1 && lastDotIndex < name.length() - 1) {
            name = name.mid(lastDotIndex + 1);
        }
        
        QString location = obj["InstallLocation"].toString();
        if (location.isEmpty()) return;

        QDir dir(location);
        QStringList exes = dir.entryList(QStringList() << "*.exe", QDir::Files);
        if (exes.isEmpty()) return;

        QString bestExe = exes.first();
        qint64 maxSize = 0;
        for (const QString& exeName : exes) {
            QFileInfo fi(dir.filePath(exeName));
            if (fi.size() > maxSize) {
                maxSize = fi.size();
                bestExe = exeName;
            }
        }

        models::CachedApp app;
        app.displayName = name; 
        app.displayIcon = dir.filePath(bestExe);
        app.executablePath = dir.filePath(bestExe);
        app.executableName = bestExe;
        app.source = models::AppSource::UWP;
        apps.append(app);
    };

    if (doc.isArray()) {
        for (const QJsonValue& val : doc.array()) {
            parseUWP(val.toObject());
        }
    } else if (doc.isObject()) {
        parseUWP(doc.object());
    }

    return apps;
}

}
