#include "SettingsWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QCheckBox>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QSettings>

SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QLabel* title = new QLabel("Advanced Settings", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 20px;");
    layout->addWidget(title);

    QString checkStyle = "QCheckBox { color: #cbd5e1; font-size: 14px; padding: 5px; }";

    QCheckBox* startupCb = new QCheckBox("Launch minimized on Windows startup (Standard User)", this);
    QCheckBox* elevatedStartupCb = new QCheckBox("Launch minimized on Windows startup as ADMIN (Bypass UAC)", this);
    QCheckBox* stealthCb = new QCheckBox("Stealth Mode (Hide from Taskbar, System Tray only)", this);
    QCheckBox* defaultProtectCb = new QCheckBox("Auto-protect newly opened applications", this);

    startupCb->setStyleSheet(checkStyle);
    elevatedStartupCb->setStyleSheet(checkStyle);
    stealthCb->setStyleSheet(checkStyle);
    defaultProtectCb->setStyleSheet(checkStyle);

    layout->addWidget(startupCb);
    layout->addWidget(elevatedStartupCb);
    layout->addWidget(stealthCb);
    layout->addWidget(defaultProtectCb);
    QSettings settings("PrivacyOverlay", "App");
    elevatedStartupCb->setChecked(settings.value("ElevatedStartup", false).toBool());

    connect(startupCb, &QCheckBox::toggled, [](bool checked) { 
        qDebug() << "[Settings] Standard Startup set to:" << checked; 
    });

    // Schedule Task Hookup
    connect(elevatedStartupCb, &QCheckBox::toggled, [this, elevatedStartupCb](bool checked) { 
        QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        QSettings settings("PrivacyOverlay", "App");
        
        if (checked) {
            QString command = QString("schtasks /create /f /tn \"PrivacyOverlay_Elevated\" /tr \"\\\"%1\\\"\" /sc onlogon /rl highest").arg(exePath);
            int res = system(command.toStdString().c_str());
            if (res != 0) {
                QMessageBox::warning(this, "Permission Denied", "Failed to create scheduled task. Please run PrivacyOverlay as Administrator once to enable this feature.");
                elevatedStartupCb->blockSignals(true);
                elevatedStartupCb->setChecked(false);
                elevatedStartupCb->blockSignals(false);
                settings.setValue("ElevatedStartup", false);
            } else {
                QMessageBox::information(this, "Task Created", "Success! PrivacyOverlay will now start automatically with Administrator privileges on logon, bypassing the UAC prompt. If you launch the app directly, it will also remember to elevate itself!");
                settings.setValue("ElevatedStartup", true);
            }
        } else {
            QString command = "schtasks /delete /f /tn \"PrivacyOverlay_Elevated\"";
            system(command.toStdString().c_str());
            settings.setValue("ElevatedStartup", false);
        }
    });
}