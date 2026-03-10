#include "MainWindow.h"
#include "AppListItem.h"
#include "DashboardWidget.h"
#include "PresetsWidget.h"
#include "SettingsWidget.h"
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
    setWindowTitle("PrivacyOverlay");
    resize(950, 650);
    // Deep bluish-dark background for the whole app
    setStyleSheet("QMainWindow { background-color: #0b101e; }");

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
    m_sidebar->addItem("Settings");
    m_sidebar->setCurrentRow(1);

    // Responsive UI fix: Prevent sidebar from getting too wide
    m_sidebar->setMaximumWidth(220);

    m_sidebar->setStyleSheet(R"(
        QListWidget {
            background-color: rgba(16, 24, 43, 0.85); /* Slightly translucent dark blue */
            border-right: 1px solid #1e293b;
            outline: none;
            padding-top: 20px;
        }
        QListWidget::item {
            color: #94a3b8; /* Slate gray text */
            font-weight: bold;
            font-size: 14px;
            padding: 15px 20px;
            border-bottom: 1px solid transparent;
        }
        QListWidget::item:hover {
            background-color: rgba(30, 41, 59, 0.5);
        }
        QListWidget::item:selected {
            color: #38bdf8; /* Neon Sky Blue */
            background-color: rgba(56, 189, 248, 0.1); /* Glowing glass effect */
            border-left: 4px solid #38bdf8;
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
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 15px; letter-spacing: 1px;");
    m_appListLayout->addWidget(titleLabel);

    scrollArea->setWidget(m_appListContainer);

    m_stackedWidget->addWidget(new DashboardWidget(this)); // Index 0
    m_stackedWidget->addWidget(scrollArea);                // Index 1 (App List)
    m_stackedWidget->addWidget(new PresetsWidget(this));   // Index 2
    m_stackedWidget->addWidget(new SettingsWidget(this));  // Index 3
}

void MainWindow::loadActiveApps() {
    models::Profile activeProfile = persistence::SettingsManager::getInstance().getActiveProfile();
    auto windows = platform::WindowEnumerator::EnumerateActiveWindows();
    std::vector<QString> seenExecutables;

    for (const auto& win : windows) {
        if (win.executableName.empty()) continue;

        QString fullPath = QString::fromStdWString(win.executableName);
        QString exeName = QFileInfo(fullPath).fileName();
        QString appTitle = QString::fromStdWString(win.title);

        if (std::find(seenExecutables.begin(), seenExecutables.end(), exeName) != seenExecutables.end()) {
            continue;
        }
        seenExecutables.push_back(exeName);

        bool isProtected = false;
        for (const auto& rule : activeProfile.rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == exeName) {
                isProtected = true;
                break;
            }
        }

        // We now pass `fullPath` so AppListItem can grab the real Windows Icon
        AppListItem* item = new AppListItem(appTitle, fullPath, exeName, isProtected, this);

        item->setStyleSheet(R"(
            AppListItem { 
                background-color: rgba(30, 41, 59, 0.6); 
                border: 1px solid #1e293b; 
                border-radius: 12px; 
            } 
            AppListItem:hover { 
                background-color: rgba(30, 41, 59, 0.9);
                border: 1px solid #38bdf8; 
            }
        )");

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