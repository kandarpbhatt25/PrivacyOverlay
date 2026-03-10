#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);
private:
    QLabel* m_statusLabel;
    QPushButton* m_killSwitchBtn;
    QTextEdit* m_activityLog;
    bool m_isEngineActive = true;
};