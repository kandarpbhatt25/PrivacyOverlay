#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QSet>
#include <windows.h>
#include <QMap>
#include "core/MaskEngine.h"
#include "platform/AppDiscoveryEngine.h"
#include "platform/ActiveProcessMonitor.h"
#include "models/CachedApp.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
    void handleNewWindow(HWND hwnd);

private slots:
    void onMonitorTick();
    void onAppToggled(const QString& toggledExe, bool isEnabled);
    void onAppsDiscovered(QList<models::CachedApp> newApps);
    void onInferredAppsDiscovered(QList<models::CachedApp> inferredApps);

private:
    void setupSidebar();
    void setupAppListScreen();
    void setupInstalledAppsScreen();
    void refreshActiveAppsUI();
    void loadInstalledApps();

    QListWidget* m_sidebar;
    QStackedWidget* m_stackedWidget;
    QWidget* m_appListContainer;
    QVBoxLayout* m_appListLayout;
    QWidget* m_installedAppListContainer;
    QVBoxLayout* m_installedAppListLayout;
    core::MaskEngine* m_maskEngine;
    
    QTimer* m_monitorTimer;
    QSet<DWORD> m_shieldedPIDs;
    HWINEVENTHOOK m_hWinEventHook;
    QMap<QString, class AppListItem*> m_activeAppWidgets;
    platform::AppDiscoveryEngine* m_appDiscoveryEngine;
    platform::ActiveProcessMonitor* m_processMonitor;
};