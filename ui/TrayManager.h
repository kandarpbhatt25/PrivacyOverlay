#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow; // Forward declaration

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(MainWindow* mainWindow, QObject* parent = nullptr);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowDashboard();
    void onExitApplication();

private:
    void createTrayIcon();
    
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    MainWindow* m_mainWindow;
};
