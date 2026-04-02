#include "MainWindow.h"
#include "AppListItem.h"
#include "DashboardWidget.h"
#include "PresetsWidget.h"
#include "SettingsWidget.h"
#include "../platform/WindowEnumerator.h"
#include "../platform/WinApiWrapper.h"
#include "../persistence/SettingsManager.h"
#include "../persistence/AppCacheManager.h"
#include "../models/Profile.h"
#include "../models/AppRule.h"
#include "../core/MaskEngine.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QDebug>
#include <QFileInfo> 
#include <QSettings> 
#include <algorithm>

static MainWindow* g_mainWindowInstance = nullptr;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (g_mainWindowInstance && hwnd) {
        g_mainWindowInstance->handleNewWindow(hwnd);
    }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("PrivacyOverlay");
    resize(950, 650);

    // --- ADDED: Premium Subtle Gradient Background ---
    setStyleSheet(R"(
        QMainWindow { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                        stop:0 #111827, stop:1 #060b14); 
        }
    )");
    // --------------------------------------------------

    m_maskEngine = new core::MaskEngine(this);

    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &MainWindow::onMonitorTick);
    m_monitorTimer->start(2000);

    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_sidebar = new QListWidget(this);
    m_stackedWidget = new QStackedWidget(this);

    setupSidebar();

    // Setup pages strictly in Sidebar index order
    m_stackedWidget->addWidget(new DashboardWidget(this)); // Index 0
    setupApplicationsScreen();                             // Index 1 (Applications)
    
    PresetsWidget* presetsWidget = new PresetsWidget(this);
    connect(presetsWidget, &PresetsWidget::profileActivated, this, &MainWindow::onProfileActivated);
    m_stackedWidget->addWidget(presetsWidget);             // Index 2
    
    m_stackedWidget->addWidget(new SettingsWidget(this));  // Index 3

    mainLayout->addWidget(m_sidebar, 0);
    mainLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(centralWidget);
    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    refreshActiveAppsUI();
    loadInstalledApps();

    g_mainWindowInstance = this;
    m_hWinEventHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    m_appDiscoveryEngine = new platform::AppDiscoveryEngine(this);
    connect(m_appDiscoveryEngine, &platform::AppDiscoveryEngine::newAppsFound, this, &MainWindow::onAppsDiscovered);
    m_appDiscoveryEngine->startDiscovery();

    m_processMonitor = new platform::ActiveProcessMonitor(this);
    connect(m_processMonitor, &platform::ActiveProcessMonitor::newInferredAppsDiscovered, this, &MainWindow::onInferredAppsDiscovered);
    m_processMonitor->startMonitoring(30000);
}

MainWindow::~MainWindow() {
    if (m_hWinEventHook) {
        UnhookWinEvent(m_hWinEventHook);
    }
    g_mainWindowInstance = nullptr;
}

void MainWindow::setupSidebar() {
    m_sidebar->addItem("Dashboard");
    m_sidebar->addItem("Applications");
    m_sidebar->addItem("Presets");
    m_sidebar->addItem("Settings");

    m_sidebar->setFixedWidth(220);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // --- UPDATED: Premium Pill-style Sidebar ---
    m_sidebar->setStyleSheet(R"(
        QListWidget {
            background-color: transparent; /* Lets the gradient peek through */
            border-right: 1px solid rgba(255, 255, 255, 0.05);
            outline: none;
            padding: 20px 10px; /* Pushes items slightly inward from the edges */
        }
        QListWidget::item {
            color: #94a3b8; 
            font-weight: 600;
            font-size: 14px;
            padding: 12px 15px;
            margin-bottom: 5px; /* Spacing creates the individual pill look */
            border-radius: 8px; /* Rounded pill corners */
        }
        QListWidget::item:hover {
            background-color: rgba(255, 255, 255, 0.05);
            color: #f8fafc;
        }
        QListWidget::item:selected {
            color: #ffffff; 
            background-color: #0284c7; /* Solid accent blue */
        }
    )");
    // -------------------------------------------
}

void MainWindow::setupApplicationsScreen() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_appsContainer = new QWidget(this);
    m_appsContainer->setStyleSheet("background-color: transparent;");

    QVBoxLayout* mainLayout = new QVBoxLayout(m_appsContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(35); // Space between sections

    // --- Active Apps Section ---
    QVBoxLayout* activeSectionLayout = new QVBoxLayout();
    activeSectionLayout->setAlignment(Qt::AlignTop);
    activeSectionLayout->setSpacing(10);
    
    QLabel* activeTitle = new QLabel("Active Applications", this);
    activeTitle->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 5px; letter-spacing: 1px;");
    activeSectionLayout->addWidget(activeTitle);
    
    m_activeAppsLayout = new QVBoxLayout(); 
    m_activeAppsLayout->setSpacing(10);
    activeSectionLayout->addLayout(m_activeAppsLayout);

    mainLayout->addLayout(activeSectionLayout);

    // --- Divider ---
    QFrame* divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    divider->setStyleSheet("background-color: #1e293b; height: 1px; margin-top: 5px; margin-bottom: 5px;");
    mainLayout->addWidget(divider);

    // --- Installed Apps Section ---
    QVBoxLayout* installedSectionLayout = new QVBoxLayout();
    installedSectionLayout->setAlignment(Qt::AlignTop);
    installedSectionLayout->setSpacing(10);
    
    QLabel* installedTitle = new QLabel("Installed Applications", this);
    installedTitle->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 5px; letter-spacing: 1px;");
    installedSectionLayout->addWidget(installedTitle);
    
    m_installedAppsLayout = new QVBoxLayout(); 
    m_installedAppsLayout->setSpacing(10);
    installedSectionLayout->addLayout(m_installedAppsLayout);

    mainLayout->addLayout(installedSectionLayout);

    scrollArea->setWidget(m_appsContainer);
    m_stackedWidget->addWidget(scrollArea);
}

void MainWindow::refreshActiveAppsUI() {
    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();
    auto windows = platform::WindowEnumerator::EnumerateActiveWindows();

    QSet<QString> currentActiveExes;
    QMap<QString, std::pair<QString, QString>> exeDetails;

    for (const auto& win : windows) {
        if (win.executableName.empty()) continue;
        QString fullPath = QString::fromStdWString(win.executableName);
        QString exeName = QFileInfo(fullPath).fileName();
        if (!exeName.endsWith(".exe", Qt::CaseInsensitive)) continue;

        currentActiveExes.insert(exeName);
        if (!exeDetails.contains(exeName)) {
            exeDetails[exeName] = { QString::fromStdWString(win.title), fullPath };
        }
    }

    // 1. DYNAMIC UI DIFFERENCE ALGORITHM: Remove apps that exited
    QList<QString> toRemove;
    for (auto it = m_activeAppWidgets.begin(); it != m_activeAppWidgets.end(); ++it) {
        if (!currentActiveExes.contains(it.key())) {
            toRemove.append(it.key());
            m_activeAppsLayout->removeWidget(it.value());
            it.value()->deleteLater(); // Securely delete Qt widget memory
        }
    }
    for (const QString& key : toRemove) {
        m_activeAppWidgets.remove(key);
    }

    for (const QString& exeName : currentActiveExes) {
        if (!m_activeAppWidgets.contains(exeName)) {
            bool isProtected = false;
            for (const auto& rule : activeProfile.rules) {
                if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == exeName) {
                    isProtected = true;
                    break;
                }
            }

            QString appTitle = exeDetails[exeName].first;
            QString fullPath = exeDetails[exeName].second;

            AppListItem* item = new AppListItem(appTitle, fullPath, exeName, isProtected, this);
            connect(item, &AppListItem::privacyToggled, this, &MainWindow::onAppToggled);

            m_activeAppsLayout->addWidget(item);
            m_activeAppWidgets[exeName] = item;
        }
    }
}

void MainWindow::loadInstalledApps() {
    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();
    QList<models::CachedApp> cachedApps = persistence::AppCacheManager::getInstance().loadCache();

    for (const auto& app : cachedApps) {
        bool isProtected = false;
        for (const auto& rule : activeProfile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == app.executableName) {
                isProtected = true;
                break;
            }
        }

        AppListItem* item = new AppListItem(app.displayName, app.displayIcon, app.executableName, isProtected, this);
        connect(item, &AppListItem::privacyToggled, this, &MainWindow::onAppToggled);
        m_installedAppsLayout->addWidget(item);
    }
}

void MainWindow::onAppsDiscovered(QList<models::CachedApp> newApps) {
    if (newApps.isEmpty()) return;
    qDebug() << "[AppDiscovery] Found" << newApps.size() << "new applications in background. Populating UI...";

    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();

    for (const auto& app : newApps) {
        bool isProtected = false;
        for (const auto& rule : activeProfile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == app.executableName) {
                isProtected = true;
                break;
            }
        }

        AppListItem* item = new AppListItem(app.displayName, app.displayIcon, app.executableName, isProtected, this);
        connect(item, &AppListItem::privacyToggled, this, &MainWindow::onAppToggled);
        m_installedAppsLayout->addWidget(item);
    }
}

void MainWindow::onInferredAppsDiscovered(QList<models::CachedApp> inferredApps) {
    if (inferredApps.isEmpty()) return;
    qDebug() << "[ProcessMonitor] Inferred" << inferredApps.size() << "new applications at runtime!";

    QList<models::CachedApp> currentCache = persistence::AppCacheManager::getInstance().loadCache();
    currentCache.append(inferredApps);

    std::sort(currentCache.begin(), currentCache.end(), [](const models::CachedApp& a, const models::CachedApp& b) {
        return a.displayName.compare(b.displayName, Qt::CaseInsensitive) < 0;
        });

    persistence::AppCacheManager::getInstance().saveCache(currentCache);
    onAppsDiscovered(inferredApps);
}

void MainWindow::onAppToggled(const QString& toggledExe, bool isEnabled) {
    models::Profile profile = persistence::SettingsManager::getInstance().getActiveProfile();

    if (isEnabled) {
        models::AppRule newRule;
        newRule.matchType = models::MatchType::ProcessName;
        newRule.matchPattern = toggledExe;
        newRule.maskMode = models::MaskMode::BlackMask;
        profile.rules.append(newRule);
        qDebug() << "[Backend] Added rule for:" << toggledExe;

        auto activeWindows = platform::WindowEnumerator::EnumerateActiveWindows();
        std::vector<DWORD> pidsToMask;
        for (const auto& w : activeWindows) {
            QString windowExe = QFileInfo(QString::fromStdWString(w.executableName)).fileName();
            if (windowExe == toggledExe && w.pid != 0) {
                if (std::find(pidsToMask.begin(), pidsToMask.end(), w.pid) == pidsToMask.end()) {
                    pidsToMask.push_back(w.pid);
                }
            }
        }
        for (DWORD pid : pidsToMask) {
            if (this->m_shieldedPIDs.contains(pid)) {
                wchar_t eventName[256];
                swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
                HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
                if (hEvent) {
                    SetEvent(hEvent);
                    CloseHandle(hEvent);
                    qDebug() << "[Backend] Signaled DLL to un-hide PID:" << pid;
                }
            }
            else {
                qDebug() << "[Backend] Dynamic Shielding PID:" << pid;
                this->m_maskEngine->applyShieldToProcess(pid);
                this->m_shieldedPIDs.insert(pid);
            }
        }
    }
    else {
        profile.rules.erase(std::remove_if(profile.rules.begin(), profile.rules.end(),
            [&](const models::AppRule& r) { return r.matchPattern == toggledExe; }), profile.rules.end());
        qDebug() << "[Backend] Removed rule for:" << toggledExe;

        auto activeWindows = platform::WindowEnumerator::EnumerateActiveWindows();
        for (const auto& w : activeWindows) {
            QString windowExe = QFileInfo(QString::fromStdWString(w.executableName)).fileName();
            if (windowExe == toggledExe && w.pid != 0 && this->m_shieldedPIDs.contains(w.pid)) {
                wchar_t eventName[256];
                swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", w.pid);
                HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
                if (hEvent) {
                    ResetEvent(hEvent);
                    CloseHandle(hEvent);
                    qDebug() << "[Backend] Signaled DLL to UNMASK PID:" << w.pid;
                }
            }
        }
    }

    persistence::SettingsManager::getInstance().saveProfile(profile);
    qDebug() << "[Backend] Saved profile to disk.";
}

void MainWindow::onMonitorTick() {
    QList<DWORD> deadPids;
    for (DWORD pid : m_shieldedPIDs) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                deadPids.append(pid);
            }
            CloseHandle(hProcess);
        }
        else {
            deadPids.append(pid);
        }
    }
    for (DWORD pid : deadPids) m_shieldedPIDs.remove(pid);

    refreshActiveAppsUI();
}

void MainWindow::handleNewWindow(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0 || m_shieldedPIDs.contains(pid)) return;

    QString fullPath = QString::fromStdWString(platform::WinApiWrapper::GetProcessExecutablePath(pid));
    if (fullPath.isEmpty()) return;

    QString winExe = QFileInfo(fullPath).fileName();

    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();
    bool requiresShield = false;
    for (const auto& rule : activeProfile.rules) {
        if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == winExe) {
            requiresShield = true;
            break;
        }
    }

    if (requiresShield) {
        qDebug() << "[Instant-Protect] Caught new window for:" << winExe << "PID:" << pid;
        m_maskEngine->applyShieldToProcess(pid);
        m_shieldedPIDs.insert(pid);
    }
}

void MainWindow::onProfileActivated() {
    models::Profile profile = persistence::SettingsManager::getInstance().getActiveProfile();
    
    auto activeWindows = platform::WindowEnumerator::EnumerateActiveWindows();
    QSet<DWORD> newPidsToMask;

    for (const auto& w : activeWindows) {
        QString windowExe = QFileInfo(QString::fromStdWString(w.executableName)).fileName();
        bool requiresShield = false;
        for (const auto& rule : profile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == windowExe) {
                requiresShield = true;
                break;
            }
        }
        
        if (requiresShield && w.pid != 0) {
            newPidsToMask.insert(w.pid);
        }
    }

    // Unmask old PIDs that no longer need shielding
    QList<DWORD> toUnmask;
    for (DWORD pid : m_shieldedPIDs) {
        if (!newPidsToMask.contains(pid)) {
            toUnmask.append(pid);
        }
    }
    
    for (DWORD pid : toUnmask) {
        wchar_t eventName[256];
        swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
        if (hEvent) {
             ResetEvent(hEvent);
             CloseHandle(hEvent);
        }
        m_shieldedPIDs.remove(pid);
    }

    // Mask new PIDs
    for (DWORD pid : newPidsToMask) {
        if (!m_shieldedPIDs.contains(pid)) {
             m_maskEngine->applyShieldToProcess(pid);
             m_shieldedPIDs.insert(pid);
        } else {
             wchar_t eventName[256];
             swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
             HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
             if (hEvent) {
                  SetEvent(hEvent);
                  CloseHandle(hEvent);
             }
        }
    }

    // Completely rebuild UI to match new profile
    QList<QString> toRemove;
    for (auto it = m_activeAppWidgets.begin(); it != m_activeAppWidgets.end(); ++it) {
        toRemove.append(it.key());
        m_activeAppsLayout->removeWidget(it.value());
        it.value()->deleteLater();
    }
    for (const QString& key : toRemove) m_activeAppWidgets.remove(key);

    refreshActiveAppsUI();

    QLayoutItem* child;
    while ((child = m_installedAppsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    loadInstalledApps();
}