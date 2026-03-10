#include "SettingsWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QDebug>

SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QLabel* title = new QLabel("Advanced Settings", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 20px;");
    layout->addWidget(title);

    QString checkStyle = "QCheckBox { color: #cbd5e1; font-size: 14px; padding: 5px; }";

    QCheckBox* startupCb = new QCheckBox("Launch minimized on Windows startup", this);
    QCheckBox* stealthCb = new QCheckBox("Stealth Mode (Hide from Taskbar, System Tray only)", this);
    QCheckBox* defaultProtectCb = new QCheckBox("Auto-protect newly opened applications", this);

    startupCb->setStyleSheet(checkStyle);
    stealthCb->setStyleSheet(checkStyle);
    defaultProtectCb->setStyleSheet(checkStyle);

    layout->addWidget(startupCb);
    layout->addWidget(stealthCb);
    layout->addWidget(defaultProtectCb);
    layout->addStretch();

    connect(startupCb, &QCheckBox::toggled, [](bool checked) { qDebug() << "[Backend Placeholder] Startup registry key set to:" << checked; });
}