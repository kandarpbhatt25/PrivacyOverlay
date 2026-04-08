#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>
#include <QString>
#include "../models/AppState.h"

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);

signals:
signals:
    void refreshRequested();
    void presetSwitchRequested(int index);
    void emergencyBlackoutTriggered(bool isBlackout);

public:
    // Provide a helper to add styled logs to the new "Recent Activity" view
    void addActivityLogEvent(const QString& title, const QString& description, const QString& statusLevel);

public slots:
    void onGlobalStateChanged(const QMap<QString, models::AppState>& state);

private:
    QLabel* m_statusLabel;
    QLabel* m_shieldedCountLabel;
    
    // Quick Controls
    QPushButton* m_btnScan;
    QPushButton* m_btnManage;
    QPushButton* m_killSwitchBtn; // The Emergency Blackout
    
    // Bottom Section Containers
    QVBoxLayout* m_liveMonitoringLayout;
    QVBoxLayout* m_recentActivityLayout;

    bool m_isEmergencyMuted = false;
};