#include "DashboardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDebug>

DashboardWidget::DashboardWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Command Center", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc;");

    // Status Indicator
    m_statusLabel = new QLabel("Status: MONITORING ACTIVE", this);
    m_statusLabel->setStyleSheet("color: #38bdf8; font-weight: bold; font-size: 16px;");

    // Big Kill Switch
    m_killSwitchBtn = new QPushButton("DISABLE PRIVACY SHIELD", this);
    m_killSwitchBtn->setFixedSize(300, 60);
    m_killSwitchBtn->setCursor(Qt::PointingHandCursor);
    m_killSwitchBtn->setStyleSheet(R"(
        QPushButton { background-color: rgba(153, 27, 27, 0.8); color: white; border-radius: 8px; font-weight: bold; font-size: 14px; border: 1px solid #ef4444; }
        QPushButton:hover { background-color: rgba(220, 38, 38, 1); }
    )");

    // Activity Log
    QLabel* logTitle = new QLabel("Recent Activity", this);
    logTitle->setStyleSheet("color: #94a3b8; font-weight: bold; margin-top: 20px;");

    m_activityLog = new QTextEdit(this);
    m_activityLog->setReadOnly(true);
    m_activityLog->setStyleSheet("background-color: rgba(15, 23, 42, 0.6); color: #cbd5e1; border: 1px solid #1e293b; border-radius: 8px; padding: 10px; font-family: Consolas;");
    m_activityLog->append(QDateTime::currentDateTime().toString("hh:mm:ss AP") + " - Privacy Engine Started");
    m_activityLog->append(QDateTime::currentDateTime().toString("hh:mm:ss AP") + " - ETW Hook Connected");

    layout->addWidget(title);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_killSwitchBtn, 0, Qt::AlignLeft);
    layout->addWidget(logTitle);
    layout->addWidget(m_activityLog);

    // Placeholder Backend Logic
    connect(m_killSwitchBtn, &QPushButton::clicked, this, [this]() {
        m_isEngineActive = !m_isEngineActive;
        if (m_isEngineActive) {
            m_statusLabel->setText("Status: MONITORING ACTIVE");
            m_statusLabel->setStyleSheet("color: #38bdf8; font-weight: bold; font-size: 16px;");
            m_killSwitchBtn->setText("DISABLE PRIVACY SHIELD");
            m_killSwitchBtn->setStyleSheet("QPushButton { background-color: rgba(153, 27, 27, 0.8); color: white; border-radius: 8px; font-weight: bold; border: 1px solid #ef4444; }");
            m_activityLog->append(QDateTime::currentDateTime().toString("hh:mm:ss AP") + " - Shield Re-Engaged");
        }
        else {
            m_statusLabel->setText("Status: ENGINE PAUSED (VULNERABLE)");
            m_statusLabel->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 16px;");
            m_killSwitchBtn->setText("ENABLE PRIVACY SHIELD");
            m_killSwitchBtn->setStyleSheet("QPushButton { background-color: rgba(2, 132, 199, 0.8); color: white; border-radius: 8px; font-weight: bold; border: 1px solid #38bdf8; }");
            m_activityLog->append(QDateTime::currentDateTime().toString("hh:mm:ss AP") + " - Shield Disabled by User");
        }
        qDebug() << "[Backend Placeholder] CaptureDetectionEngine state changed to:" << m_isEngineActive;
        });
}