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
#include "../core/IconManager.h"
#include "TrayManager.h"
#include "ToastWidget.h"
#include "NotificationCenterWidget.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QDebug>
#include <QFileInfo> 
#include <QSettings> 
#include <QComboBox>
#include <QButtonGroup>
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

    setStyleSheet(R"(
        QMainWindow { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                                        stop:0 #111827, stop:1 #060b14); 
        }
    )");

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
    
    // Initialize standalone utilities
    m_trayManager = new TrayManager(this);
    m_toastWidget = new ToastWidget(this);

    setupSidebar();

    m_dashboardWidget = new DashboardWidget(this);
    connect(this, &MainWindow::globalStateChanged, m_dashboardWidget, &DashboardWidget::onGlobalStateChanged);
    connect(m_dashboardWidget, &DashboardWidget::refreshRequested, this, &MainWindow::on_refresh_clicked);
    connect(m_dashboardWidget, &DashboardWidget::presetSwitchRequested, this, &MainWindow::on_preset_change);
    connect(m_dashboardWidget, &DashboardWidget::emergencyBlackoutTriggered, this, &MainWindow::onEmergencyBlackout);
    
    m_stackedWidget->addWidget(m_dashboardWidget); // Index 0
    
    setupApplicationsScreen(); // Index 1
    
    PresetsWidget* presetsWidget = new PresetsWidget(this);
    connect(presetsWidget, &PresetsWidget::profileActivated, this, &MainWindow::onProfileActivated);
    m_stackedWidget->addWidget(presetsWidget); // Index 2
    
    m_stackedWidget->addWidget(new SettingsWidget(this)); // Index 3

    m_notificationWidget = new NotificationCenterWidget(this);
    m_stackedWidget->addWidget(m_notificationWidget);     // Index 4

    mainLayout->addWidget(m_sidebar, 0);
    mainLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(centralWidget);
    
    // Wire MaskEngine signals
    connect(m_maskEngine, &core::MaskEngine::maskError, this, &MainWindow::onMaskError);
    connect(m_maskEngine, &core::MaskEngine::maskApplied, this, &MainWindow::onMaskApplied);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    g_mainWindowInstance = this;
    m_hWinEventHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    m_appDiscoveryEngine = new platform::AppDiscoveryEngine(this);
    connect(m_appDiscoveryEngine, &platform::AppDiscoveryEngine::newAppsFound, this, &MainWindow::onAppsDiscovered);
    m_appDiscoveryEngine->startDiscovery();

    m_processMonitor = new platform::ActiveProcessMonitor(this);
    connect(m_processMonitor, &platform::ActiveProcessMonitor::newInferredAppsDiscovered, this, &MainWindow::onInferredAppsDiscovered);
    m_processMonitor->startMonitoring(30000);
    
    // Initial data load
    updateGlobalState();
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
    m_sidebar->addItem("Notifications");

    m_sidebar->setFixedWidth(220);
    m_sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_sidebar->setStyleSheet(R"(
        QListWidget {
            background-color: transparent; 
            border-right: 1px solid rgba(255, 255, 255, 0.05);
            outline: none;
            padding: 20px 10px; 
        }
        QListWidget::item {
            color: #94a3b8; 
            font-weight: 600;
            font-size: 14px;
            padding: 12px 15px;
            margin-bottom: 5px; 
            border-radius: 8px; 
        }
        QListWidget::item:hover {
            background-color: rgba(255, 255, 255, 0.05);
            color: #f8fafc;
        }
        QListWidget::item:selected {
            color: #ffffff; 
            background-color: #0284c7; 
        }
    )");
}

void MainWindow::setupApplicationsScreen() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_appsContainer = new QWidget(this);
    m_appsContainer->setStyleSheet("background-color: transparent;");

    QVBoxLayout* mainLayout = new QVBoxLayout(m_appsContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    QLabel* activeTitle = new QLabel("App Master List", this);
    activeTitle->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; letter-spacing: 1px;");
    mainLayout->addWidget(activeTitle);

    m_blackoutBanner = new QLabel("🛡️ Strict Privacy Active: New applications will be hidden automatically.", this);
    m_blackoutBanner->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background-color: rgba(239, 68, 68, 0.2); border: 1px solid #ef4444; border-radius: 8px; padding: 12px; margin-bottom: 5px;");
    m_blackoutBanner->setVisible(false);
    mainLayout->addWidget(m_blackoutBanner);

    // --- Filter Bar ---
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->setAlignment(Qt::AlignLeft);
    filterLayout->setSpacing(10);

    auto createCapsule = [this](const QString& text, bool isChecked) {
        QPushButton* btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setChecked(isChecked);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(
            QPushButton { 
                background-color: rgba(30, 41, 59, 0.6); 
                color: #94a3b8; 
                border-radius: 16px; 
                padding: 6px 16px; 
                border: 1px solid rgba(255,255,255,0.05);
                font-weight: bold;
            }
            QPushButton:hover { background-color: rgba(51, 65, 85, 0.8); }
            QPushButton:checked { 
                background-color: #0ea5e9; 
                color: white; 
                border: 1px solid #38bdf8;
            }
        )");
        connect(btn, &QPushButton::toggled, this, &MainWindow::onFilterChanged);
        return btn;
    };

    m_btnFilterAll = createCapsule("All Apps", true);
    m_btnFilterInstalled = createCapsule("Installed Apps", false);
    m_btnFilterActive = createCapsule("Active Processes", false);
    m_btnFilterHidden = createCapsule("Hidden Apps", false);

    QButtonGroup* filterGroup = new QButtonGroup(this);
    filterGroup->setExclusive(true);
    filterGroup->addButton(m_btnFilterAll);
    filterGroup->addButton(m_btnFilterInstalled);
    filterGroup->addButton(m_btnFilterActive);
    filterGroup->addButton(m_btnFilterHidden);

    m_sortComboBox = new QComboBox(this);
    m_sortComboBox->setStyleSheet(R"(
        QComboBox { background-color: rgba(30, 41, 59, 0.6); color: white; border: 1px solid rgba(255,255,255,0.1); border-radius: 16px; padding: 6px 16px; font-weight: bold; }
        QComboBox:focus { border: 1px solid #38bdf8; }
        QComboBox::drop-down { border: none; }
    )");
    m_sortComboBox->addItem("Name (A-Z)", 0);
    m_sortComboBox->addItem("Name (Z-A)", 1);
    m_sortComboBox->addItem("Privacy Status (Masked First)", 2);
    m_sortComboBox->addItem("Privacy Status (Unmasked First)", 3);
    connect(m_sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { onSortChanged(index); });

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search apps or executables...");
    m_searchBox->setStyleSheet("QLineEdit { background-color: rgba(30, 41, 59, 0.6); color: white; border: 1px solid rgba(255,255,255,0.1); border-radius: 16px; padding: 6px 16px; } QLineEdit:focus { border: 1px solid #38bdf8; }");
    m_searchBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_searchBox->setMinimumWidth(120);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);

    filterLayout->addWidget(m_searchBox);
    filterLayout->addWidget(m_sortComboBox);
    filterLayout->addSpacing(10);
    filterLayout->addWidget(m_btnFilterAll);
    filterLayout->addWidget(m_btnFilterInstalled);
    filterLayout->addWidget(m_btnFilterActive);
    filterLayout->addWidget(m_btnFilterHidden);
    filterLayout->addStretch();
    
    mainLayout->addLayout(filterLayout);

    // --- Bulk Action Controls ---
    QHBoxLayout* bulkActionsLayout = new QHBoxLayout();
    bulkActionsLayout->setAlignment(Qt::AlignLeft);
    bulkActionsLayout->setSpacing(10);
    
    QPushButton* btnBulkSelect = new QPushButton("Select All Visible", this);
    btnBulkSelect->setStyleSheet("color: #38bdf8; background: transparent; border: none; font-weight: bold; text-align: left; max-width: 120px;");
    btnBulkSelect->setCursor(Qt::PointingHandCursor);
    
    QPushButton* btnBulkDeselect = new QPushButton("Deselect All Visible", this);
    btnBulkDeselect->setStyleSheet("color: #ef4444; background: transparent; border: none; font-weight: bold; text-align: left; max-width: 140px;");
    btnBulkDeselect->setCursor(Qt::PointingHandCursor);
    
    bulkActionsLayout->addWidget(btnBulkSelect);
    bulkActionsLayout->addWidget(btnBulkDeselect);
    mainLayout->addLayout(bulkActionsLayout);

    connect(btnBulkSelect, &QPushButton::clicked, this, [this]() {
        for (AppListItem* widget : m_unifiedAppWidgets.values()) {
            if (widget->isVisible()) widget->setToggleState(true);
        }
    });

    connect(btnBulkDeselect, &QPushButton::clicked, this, [this]() {
        for (AppListItem* widget : m_unifiedAppWidgets.values()) {
            if (widget->isVisible()) widget->setToggleState(false);
        }
    });

    // --- Active Apps Section ---
    m_activeSectionWidget = new QWidget(this);
    QVBoxLayout* activeSectionLayout = new QVBoxLayout(m_activeSectionWidget);
    activeSectionLayout->setAlignment(Qt::AlignTop);
    activeSectionLayout->setContentsMargins(0, 10, 0, 10);
    activeSectionLayout->setSpacing(10);
    
    m_activeSubTitle = new QLabel("Active Applications", m_activeSectionWidget);
    m_activeSubTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #cbd5e1; margin-bottom: 5px;");
    activeSectionLayout->addWidget(m_activeSubTitle);
    
    m_activeAppsLayout = new QVBoxLayout(); 
    m_activeAppsLayout->setSpacing(10);
    activeSectionLayout->addLayout(m_activeAppsLayout);
    mainLayout->addWidget(m_activeSectionWidget);

    // --- Divider ---
    m_divider = new QFrame(this);
    m_divider->setFrameShape(QFrame::HLine);
    m_divider->setFrameShadow(QFrame::Sunken);
    m_divider->setStyleSheet("background-color: #1e293b; height: 1px; margin-top: 5px; margin-bottom: 5px;");
    mainLayout->addWidget(m_divider);

    // --- Installed Apps Section ---
    m_installedSectionWidget = new QWidget(this);
    QVBoxLayout* installedSectionLayout = new QVBoxLayout(m_installedSectionWidget);
    installedSectionLayout->setAlignment(Qt::AlignTop);
    installedSectionLayout->setContentsMargins(0, 10, 0, 10);
    installedSectionLayout->setSpacing(10);
    
    m_installedSubTitle = new QLabel("Installed Applications", m_installedSectionWidget);
    m_installedSubTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #cbd5e1; margin-bottom: 5px;");
    installedSectionLayout->addWidget(m_installedSubTitle);
    
    m_installedAppsLayout = new QVBoxLayout(); 
    m_installedAppsLayout->setSpacing(10);
    installedSectionLayout->addLayout(m_installedAppsLayout);
    mainLayout->addWidget(m_installedSectionWidget);

    mainLayout->addStretch();

    scrollArea->setWidget(m_appsContainer);
    m_stackedWidget->addWidget(scrollArea);
}

void MainWindow::updateGlobalState() {
    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();
    QList<models::CachedApp> cachedApps = persistence::AppCacheManager::getInstance().loadCache();
    auto activeWindows = platform::WindowEnumerator::EnumerateActiveWindows();

    QMap<QString, models::AppState> newState;

    for (const auto& app : cachedApps) {
        models::AppState s;
        s.exeName = app.executableName;
        s.displayName = app.displayName;
        s.displayIcon = app.displayIcon;
        s.isActive = false;
        
        bool isProtected = false;
        for (const auto& rule : activeProfile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == app.executableName) {
                isProtected = true;
                break;
            }
        }
        s.isHidden = isProtected;
        newState[s.exeName] = s;
    }

    for (const auto& win : activeWindows) {
        if (win.executableName.empty()) continue;
        QString fullPath = QString::fromStdWString(win.executableName);
        QString exeName = QFileInfo(fullPath).fileName();
        if (!exeName.endsWith(".exe", Qt::CaseInsensitive)) continue;

        if (!newState.contains(exeName)) {
            models::AppState s;
            s.exeName = exeName;
            s.fullExePath = fullPath;
            s.displayName = QString::fromStdWString(win.title);
            if (s.displayName.isEmpty()) s.displayName = exeName;
            
            bool isProtected = false;
            for (const auto& rule : activeProfile.rules) {
                if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == exeName) {
                    isProtected = true;
                    break;
                }
            }
            
            if (m_isBlackoutActive) {
                if (m_emergencySnapshot.contains(exeName)) {
                    // Manually hidden during blackout, or hidden by the initial blackout sweep
                    isProtected = true;
                } else if (!isProtected) {
                    // Check if it's a completely newly active process
                    bool wasActive = m_globalAppState.contains(exeName) && m_globalAppState[exeName].isActive;
                    if (!wasActive) {
                        isProtected = true;
                        m_emergencySnapshot.insert(exeName);
                        if (win.pid != 0 && !m_shieldedPIDs.contains(win.pid)) {
                            m_maskEngine->applyShieldToProcess(win.pid);
                            m_shieldedPIDs.insert(win.pid);
                        }
                    }
                }
            }

            s.isHidden = isProtected;
            s.cachedNativeIcon = core::IconManager::getInstance().getIcon(fullPath, s.displayName);
            
            newState[exeName] = s;
        } else {
            // Update an existing cached entry that wasn't active
            newState[exeName].fullExePath = fullPath;
            newState[exeName].cachedNativeIcon = core::IconManager::getInstance().getIcon(fullPath, newState[exeName].displayName);
        }
        newState[exeName].isActive = true;
    }

    m_globalAppState = newState;
    emit globalStateChanged(m_globalAppState);
    refreshUnifiedAppsUI();
}

void MainWindow::refreshUnifiedAppsUI() {
    // Generate new widgets
    for (auto it = m_globalAppState.begin(); it != m_globalAppState.end(); ++it) {
        const auto& state = it.value();
        if (!m_unifiedAppWidgets.contains(state.exeName)) {
            AppListItem* item = new AppListItem(state.displayName, "", state.exeName, state.isHidden, this);
            connect(item, &AppListItem::privacyToggled, this, &MainWindow::onAppToggled);
            
            if (state.isActive) {
                m_activeAppsLayout->addWidget(item);
            } else {
                m_installedAppsLayout->addWidget(item);
            }
            m_unifiedAppWidgets[state.exeName] = item;
        } else {
            AppListItem* item = m_unifiedAppWidgets[state.exeName];
            bool isInActiveLayout = m_activeAppsLayout->indexOf(item) != -1;
            
            if (state.isActive && !isInActiveLayout) {
                m_installedAppsLayout->removeWidget(item);
                m_activeAppsLayout->addWidget(item);
            } else if (!state.isActive && isInActiveLayout) {
                m_activeAppsLayout->removeWidget(item);
                m_installedAppsLayout->addWidget(item);
            }
        }
    }
    applySorting();
    onFilterChanged();
}

void MainWindow::onSortChanged(int /*index*/) {
    applySorting();
}

void MainWindow::applySorting() {
    int sortMode = m_sortComboBox ? m_sortComboBox->currentData().toInt() : 0;
    
    auto sortLayout = [this, sortMode](QVBoxLayout* layout) {
        if (!layout) return;
        QList<AppListItem*> items;
        for (int i = 0; i < layout->count(); ++i) {
            QWidget* widget = layout->itemAt(i)->widget();
            if (AppListItem* appItem = qobject_cast<AppListItem*>(widget)) {
                items.append(appItem);
            }
        }
        
        std::sort(items.begin(), items.end(), [this, sortMode](AppListItem* a, AppListItem* b) {
            QString exeA = a->getExeName();
            QString exeB = b->getExeName();
            models::AppState stateA = m_globalAppState.value(exeA);
            models::AppState stateB = m_globalAppState.value(exeB);
            
            if (sortMode == 0) { // A-Z
                return stateA.displayName.compare(stateB.displayName, Qt::CaseInsensitive) < 0;
            } else if (sortMode == 1) { // Z-A
                return stateA.displayName.compare(stateB.displayName, Qt::CaseInsensitive) > 0;
            } else if (sortMode == 2) { // Masked First
                if (stateA.isHidden != stateB.isHidden) return stateA.isHidden;
                return stateA.displayName.compare(stateB.displayName, Qt::CaseInsensitive) < 0;
            } else if (sortMode == 3) { // Unmasked First
                if (stateA.isHidden != stateB.isHidden) return !stateA.isHidden;
                return stateA.displayName.compare(stateB.displayName, Qt::CaseInsensitive) < 0;
            }
            return false;
        });
        
        for (AppListItem* item : items) {
            layout->removeWidget(item);
        }
        for (AppListItem* item : items) {
            layout->addWidget(item);
        }
    };
    if (m_activeAppsLayout) sortLayout(m_activeAppsLayout);
    if (m_installedAppsLayout) sortLayout(m_installedAppsLayout);
}

void MainWindow::onFilterChanged() {
    bool filterShowAll = m_btnFilterAll ? m_btnFilterAll->isChecked() : true;
    m_filterShowInstalled = m_btnFilterInstalled ? m_btnFilterInstalled->isChecked() : false;
    m_filterShowActive = m_btnFilterActive ? m_btnFilterActive->isChecked() : false;
    m_filterShowHidden = m_btnFilterHidden ? m_btnFilterHidden->isChecked() : false;

    int visibleActive = 0;
    int visibleInstalled = 0;

    for (auto it = m_globalAppState.begin(); it != m_globalAppState.end(); ++it) {
        const auto& state = it.value();
        AppListItem* widget = m_unifiedAppWidgets.value(state.exeName);
        if (!widget) continue;

        bool show = false;
        
        if (filterShowAll) {
            show = true;
        } else if (m_filterShowInstalled && !state.isActive) {
            show = true;
        } else if (m_filterShowActive && state.isActive) {
            show = true;
        } else if (m_filterShowHidden && state.isHidden) {
            show = true;
        }
        
        // Strict Search Filter (AND operation over visibility status)
        QString query = m_searchBox ? m_searchBox->text().trimmed().toLower() : "";
        if (show && !query.isEmpty()) {
            if (!state.displayName.toLower().contains(query) && !state.exeName.toLower().contains(query)) {
                show = false;
            }
        }
        
        widget->setVisible(show);
        
        if (show) {
            if (state.isActive) visibleActive++;
            else visibleInstalled++;
        }
    }

    bool hasActive = visibleActive > 0;
    bool hasInstalled = visibleInstalled > 0;

    if (m_activeSectionWidget) m_activeSectionWidget->setVisible(hasActive);
    if (m_installedSectionWidget) m_installedSectionWidget->setVisible(hasInstalled);

    if (filterShowAll) {
        if (m_activeSubTitle) m_activeSubTitle->setVisible(true);
        if (m_installedSubTitle) m_installedSubTitle->setVisible(true);
        if (m_divider) m_divider->setVisible(hasActive && hasInstalled);
    } else {
        if (m_activeSubTitle) m_activeSubTitle->setVisible(false);
        if (m_installedSubTitle) m_installedSubTitle->setVisible(false);
        if (m_divider) m_divider->setVisible(false);
    }
}

void MainWindow::onAppsDiscovered(QList<models::CachedApp> newApps) {
    updateGlobalState();
}

void MainWindow::onInferredAppsDiscovered(QList<models::CachedApp> inferredApps) {
    if (inferredApps.isEmpty()) return;
    QList<models::CachedApp> currentCache = persistence::AppCacheManager::getInstance().loadCache();
    currentCache.append(inferredApps);
    persistence::AppCacheManager::getInstance().saveCache(currentCache);
    updateGlobalState();
}

void MainWindow::onAppToggled(const QString& toggledExe, bool isEnabled) {
    if (m_isBlackoutActive) {
        if (isEnabled) {
            m_emergencySnapshot.insert(toggledExe);
        } else {
            m_emergencySnapshot.remove(toggledExe);
        }
    }

    models::Profile profile = persistence::SettingsManager::getInstance().getActiveProfile();

    if (isEnabled) {
        if (!m_isBlackoutActive) {
            models::AppRule newRule;
            newRule.matchType = models::MatchType::ProcessName;
            newRule.matchPattern = toggledExe;
            newRule.maskMode = models::MaskMode::BlackMask;
            profile.rules.append(newRule);
        }

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
        bool allSucceeded = true;
        for (DWORD pid : pidsToMask) {
            if (this->m_shieldedPIDs.contains(pid)) {
                wchar_t eventName[256];
                swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
                HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
                if (hEvent) { SetEvent(hEvent); CloseHandle(hEvent); }
            } else {
                if (this->m_maskEngine->applyShieldToProcess(pid)) {
                    this->m_shieldedPIDs.insert(pid);
                } else {
                    allSucceeded = false;
                }
            }
        }
        
        if (!allSucceeded) {
            // Revert state if we failed to shield successfully
            if (!m_isBlackoutActive) {
                profile.rules.erase(std::remove_if(profile.rules.begin(), profile.rules.end(),
                    [&](const models::AppRule& r) { return r.matchPattern == toggledExe; }), profile.rules.end());
            } else {
                m_emergencySnapshot.remove(toggledExe);
            }
            
            AppListItem* widget = m_unifiedAppWidgets.value(toggledExe);
            if (widget) {
                widget->blockSignals(true);
                widget->setToggleState(false);
                widget->blockSignals(false);
            }
            isEnabled = false; // Prevents saving the invalid rule
        }
    } else {
        if (!m_isBlackoutActive) {
            profile.rules.erase(std::remove_if(profile.rules.begin(), profile.rules.end(),
                [&](const models::AppRule& r) { return r.matchPattern == toggledExe; }), profile.rules.end());
        }

        auto activeWindows = platform::WindowEnumerator::EnumerateActiveWindows();
        for (const auto& w : activeWindows) {
            QString windowExe = QFileInfo(QString::fromStdWString(w.executableName)).fileName();
            if (windowExe == toggledExe && w.pid != 0 && this->m_shieldedPIDs.contains(w.pid)) {
                wchar_t eventName[256];
                swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", w.pid);
                HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
                if (hEvent) { ResetEvent(hEvent); CloseHandle(hEvent); }
            }
        }
    }

    if (!m_isBlackoutActive) {
        persistence::SettingsManager::getInstance().saveProfile(profile);
    }
    updateGlobalState(); // Immediately broadcast changes
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
        } else {
            deadPids.append(pid);
        }
    }
    for (DWORD pid : deadPids) m_shieldedPIDs.remove(pid);
    updateGlobalState(); // Update dashboard live
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
        if (m_maskEngine->applyShieldToProcess(pid)) {
            m_shieldedPIDs.insert(pid);
        }
    }
    
    updateGlobalState(); // Immediate view update for Dashboard
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
                requiresShield = true; break;
            }
        }
        if (requiresShield && w.pid != 0) newPidsToMask.insert(w.pid);
    }

    QList<DWORD> toUnmask;
    for (DWORD pid : m_shieldedPIDs) {
        if (!newPidsToMask.contains(pid)) toUnmask.append(pid);
    }
    
    for (DWORD pid : toUnmask) {
        wchar_t eventName[256];
        swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
        if (hEvent) { ResetEvent(hEvent); CloseHandle(hEvent); }
        m_shieldedPIDs.remove(pid);
    }

    for (DWORD pid : newPidsToMask) {
        if (!m_shieldedPIDs.contains(pid)) {
             if (m_maskEngine->applyShieldToProcess(pid)) {
                 m_shieldedPIDs.insert(pid);
             }
        } else {
             wchar_t eventName[256];
             swprintf_s(eventName, L"PrivacyShield_Enable_PID_%lu", pid);
             HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
             if (hEvent) { SetEvent(hEvent); CloseHandle(hEvent); }
        }
    }

    updateGlobalState();
}

void MainWindow::on_refresh_clicked() {
    if (m_processMonitor) {
        // Trigger a synchronous internal poll to pull active exes quickly.
        m_processMonitor->startMonitoring(30000); // the QTimer triggers one via singleShot
        updateGlobalState();
    }
}

void MainWindow::on_preset_change(int index) {
    auto profiles = persistence::SettingsManager::getInstance().loadAllProfiles();
    if (index >= 0 && index < profiles.size()) {
        persistence::SettingsManager::getInstance().setActiveProfile(profiles[index].id);
        onProfileActivated(); // Trigger shield realignments
    }
}

void MainWindow::onEmergencyBlackout(bool isBlackout) {
    m_isBlackoutActive = isBlackout;
    if (m_blackoutBanner) m_blackoutBanner->setVisible(isBlackout);

    if (isBlackout) {
        QList<QString> exesToHide;
        for (auto it = m_globalAppState.begin(); it != m_globalAppState.end(); ++it) {
            const auto& state = it.value();
            if (state.isActive && !state.isHidden) {
                exesToHide.append(state.exeName);
            }
        }
        for (const QString& exeName : exesToHide) {
            m_emergencySnapshot.insert(exeName);
            AppListItem* widget = m_unifiedAppWidgets.value(exeName);
            if (widget) widget->setToggleState(true);
        }
    } else {
        QList<QString> exesToRestore(m_emergencySnapshot.begin(), m_emergencySnapshot.end());
        for (const QString& exeName : exesToRestore) {
            AppListItem* widget = m_unifiedAppWidgets.value(exeName);
            if (widget) widget->setToggleState(false);
        }
        m_emergencySnapshot.clear();
    }
}

void MainWindow::onMaskError(DWORD pid, const QString& message) {
    QString errTitle = "Privacy Engine Error";
    QString errMsg = message;
    
    // Find matching app name if possible
    for (auto it = m_globalAppState.begin(); it != m_globalAppState.end(); ++it) {
        if (it.value().isActive) {
            // Because we don't store PID in globalAppState directly, we'd guess but for now just show error.
            // Ideally we'd look it up.
        }
    }
    
    m_toastWidget->showToast(errTitle, errMsg, true);
    m_notificationWidget->addNotification(errTitle, errMsg + QString(" (PID: %1)").arg(pid), true);
}

void MainWindow::onMaskApplied(DWORD pid) {
    // Optionally log successful applies if we want deep logging, but not necessary for standard notifications.
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_isMinimizeToTrayEnabled) {
        event->ignore();
        hide();
        m_toastWidget->showToast("Privacy Overlay Minimized", "Application is still protecting your privacy in the background.", false);
    } else {
        event->accept();
    }
}