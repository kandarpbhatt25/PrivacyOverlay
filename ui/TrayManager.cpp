#include "TrayManager.h"
#include "MainWindow.h"
#include <QApplication>
#include <QAction>
#include <QPainter>
#include <QPixmap>
#include <QIcon>

TrayManager::TrayManager(MainWindow* mainWindow, QObject* parent) 
    : QObject(parent), m_mainWindow(mainWindow) {
    createTrayIcon();
}

void TrayManager::createTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    
    // Attempt to load from resources
    QIcon icon = QIcon(":/icons/app_icon.png");
    
    // If we have no resource icon, Qt will fail to show the tray.
    // We must generate a guaranteed fallback visual icon natively.
    if (icon.isNull()) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(14, 165, 233)); // Sky blue base
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(2, 2, 28, 28); 
        
        // Draw a generic 'shield' or lock shape proxy inside
        painter.setBrush(Qt::white);
        painter.drawRect(12, 12, 8, 10);
        painter.drawEllipse(12, 9, 8, 8);
        
        icon = QIcon(pixmap);
    }
    
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Privacy Overlay Engine Core");

    m_trayMenu = new QMenu(m_mainWindow);
    
    QAction* showAction = new QAction("Open Dashboard", this);
    connect(showAction, &QAction::triggered, this, &TrayManager::onShowDashboard);
    m_trayMenu->addAction(showAction);

    m_trayMenu->addSeparator();

    QAction* exitAction = new QAction("Exit Applications Completely", this);
    connect(exitAction, &QAction::triggered, this, &TrayManager::onExitApplication);
    m_trayMenu->addAction(exitAction);

    m_trayIcon->setContextMenu(m_trayMenu);
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayIconActivated);
    
    m_trayIcon->show();
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onShowDashboard();
    }
}

void TrayManager::onShowDashboard() {
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}

void TrayManager::onExitApplication() {
    QApplication::quit();
}
