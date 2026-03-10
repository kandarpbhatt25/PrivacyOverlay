#include "MainWindow.h"
#include "AppListItem.h"
#include "../platform/WindowEnumerator.h"
#include "../persistence/SettingsManager.h"
#include "../models/Profile.h"
#include "../models/AppRule.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QDebug>
#include <QFileInfo> // Used to extract "chrome.exe" from full file paths
#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("PrivacyOverlay Dashboard");
    resize(900, 600); // Slightly larger default window
    setStyleSheet("background-color: #f5f5f5;"); // Soft background

    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_sidebar = new QListWidget(this);
    m_stackedWidget = new QStackedWidget(this);

    setupSidebar();
    setupAppListScreen();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_stackedWidget);

    setCentralWidget(centralWidget);
    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // Initialize backend data into UI
    loadActiveApps();
}

void MainWindow::setupSidebar() {
    m_sidebar->addItem("Dashboard");
    m_sidebar->addItem("APP List");
    m_sidebar->addItem("Presets");
    m_sidebar->setCurrentRow(1);

    // Responsive UI fix: Prevent sidebar from getting too wide
    m_sidebar->setMaximumWidth(220);

    m_sidebar->setStyleSheet(R"(
        QListWidget {
            background-color: white;
            border-right: 1px solid #e0e0e0;
            outline: none;
            padding-top: 20px;
        }
        QListWidget::item {
            color: #555555;
            font-weight: bold;
            font-size: 14px;
            padding: 15px 20px;
            border-bottom: 1px solid #f0f0f0;
        }
        QListWidget::item:selected {
            color: #2E7D32;
            background-color: #e8f5e9;
            border-left: 4px solid #4CAF50;
        }
    )");
}

void MainWindow::setupAppListScreen() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_appListContainer = new QWidget(this);
    m_appListContainer->setStyleSheet("background-color: transparent;");

    m_appListLayout = new QVBoxLayout(m_appListContainer);
    m_appListLayout->setAlignment(Qt::AlignTop);
    m_appListLayout->setContentsMargins(30, 30, 30, 30); // Breathing room
    m_appListLayout->setSpacing(10); // Space between items

    QLabel* titleLabel = new QLabel("Active Applications", this);
    titleLabel->setStyleSheet("font-size: 26px; font-weight: 800; color: #1b5e20; margin-bottom: 10px;");
    m_appListLayout->addWidget(titleLabel);

    scrollArea->setWidget(m_appListContainer);

    m_stackedWidget->addWidget(new QLabel("Dashboard Content"));
    m_stackedWidget->addWidget(scrollArea);
    m_stackedWidget->addWidget(new QLabel("Presets Content"));
}

void MainWindow::loadActiveApps() {
    // 1. Fetch the user's active profile from SettingsManager
    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();

    // 2. Fetch live windows from OS
    auto windows = platform::WindowEnumerator::EnumerateActiveWindows();
    std::vector<QString> seenExecutables;

    for (const auto& win : windows) {
        // Skip windows that don't belong to a clear executable
        if (win.executableName.empty()) continue;

        // Convert raw paths (C:\Program Files\Google\Chrome\Application\chrome.exe) 
        // into just the filename ("chrome.exe")
        QString fullPath = QString::fromStdWString(win.executableName);
        QString exeName = QFileInfo(fullPath).fileName();
        QString appTitle = QString::fromStdWString(win.title);

        // Deduplicate: Don't show 5 list items for 5 Chrome tabs
        if (std::find(seenExecutables.begin(), seenExecutables.end(), exeName) != seenExecutables.end()) {
            continue;
        }
        seenExecutables.push_back(exeName);

        // 3. Check if this app is already saved in our rules
        bool isProtected = false;
        for (const auto& rule : activeProfile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == exeName) {
                isProtected = true;
                break;
            }
        }

        // 4. Create the UI item with the correct initial state
        AppListItem* item = new AppListItem(appTitle, exeName, isProtected, this);
        item->setStyleSheet("AppListItem { background-color: white; border: 1px solid #e0e0e0; border-radius: 10px; } "
            "AppListItem:hover { border: 1px solid #81c784; }"); // Hover effect!

        // 5. Wire the toggle switch to save data instantly
        connect(item, &AppListItem::privacyToggled, this, [](const QString& toggledExe, bool isEnabled) {

            // Get the absolute latest profile
            models::Profile profile = persistence::SettingsManager::getInstance().getActiveProfile();

            if (isEnabled) {
                // Add new rule
                models::AppRule newRule;
                newRule.matchType = models::MatchType::ProcessName;
                newRule.matchPattern = toggledExe;
                newRule.maskMode = models::MaskMode::BlackMask; // From your blueprint
                profile.rules.append(newRule);
                qDebug() << "[Backend] Added rule for:" << toggledExe;
            }
            else {
                // Remove existing rule
                profile.rules.erase(std::remove_if(profile.rules.begin(), profile.rules.end(),
                    [&](const models::AppRule& r) { return r.matchPattern == toggledExe; }), profile.rules.end());
                qDebug() << "[Backend] Removed rule for:" << toggledExe;
            }

            // Save back to JSON file
            persistence::SettingsManager::getInstance().saveProfile(profile);
            qDebug() << "[Backend] Saved profile to disk.";
            });

        m_appListLayout->addWidget(item);
    }
}