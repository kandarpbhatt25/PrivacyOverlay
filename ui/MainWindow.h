#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QTimer>
#include <QSet>
#include <windows.h>
#include <QMap>
#include "core/MaskEngine.h"
#include "platform/AppDiscoveryEngine.h"
#include "platform/ActiveProcessMonitor.h"
#include "models/CachedApp.h"
#include "models/AppState.h"

class DashboardWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
    void handleNewWindow(HWND hwnd);

signals:
    void globalStateChanged(const QMap<QString, models::AppState>& state);

private slots:
    void onMonitorTick();
    void onAppToggled(const QString& toggledExe, bool isEnabled);
    void onAppsDiscovered(QList<models::CachedApp> newApps);
    void onInferredAppsDiscovered(QList<models::CachedApp> inferredApps);
    void onProfileActivated();
    
    void onFilterChanged();
    void updateGlobalState();

private:
    void setupSidebar();
    void setupApplicationsScreen();
    void refreshUnifiedAppsUI(); 

    QListWidget* m_sidebar;
    QStackedWidget* m_stackedWidget;
    DashboardWidget* m_dashboardWidget;
    
    // Quick Dashboard Control Slots
    void on_refresh_clicked();
    void on_preset_change(int index);
    void onEmergencyBlackout(bool isBlackout);
    
    // Unified Apps Screen
    QWidget* m_appsContainer;
    QWidget* m_activeSectionWidget;
    QWidget* m_installedSectionWidget;
    QVBoxLayout* m_activeAppsLayout; 
    QVBoxLayout* m_installedAppsLayout; 
    
    // Filter toggles
    bool m_filterShowInstalled = true;
    bool m_filterShowActive = false;
    bool m_filterShowHidden = false;
    bool m_isBlackoutActive = false;

    // Filter Buttons
    QPushButton* m_btnFilterInstalled;
    QPushButton* m_btnFilterActive;
    QPushButton* m_btnFilterHidden;
    
    class QLineEdit* m_searchBox;
    QWidget* m_blackoutBanner;
    
    core::MaskEngine* m_maskEngine;
    
    QTimer* m_monitorTimer;
    QSet<DWORD> m_shieldedPIDs;
    QSet<QString> m_emergencySnapshot;
    HWINEVENTHOOK m_hWinEventHook;
    
    QMap<QString, class AppListItem*> m_unifiedAppWidgets; 
    QMap<QString, models::AppState> m_globalAppState;      // The Master State
    
    platform::AppDiscoveryEngine* m_appDiscoveryEngine;
    platform::ActiveProcessMonitor* m_processMonitor;
};