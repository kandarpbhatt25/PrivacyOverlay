#pragma once
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QString>

class AppListItem : public QWidget {
    Q_OBJECT

public:
    // Added exeName and initialState
    explicit AppListItem(const QString& windowTitle, const QString& exeName, bool initialState, QWidget* parent = nullptr);

signals:
    // Now emits the exeName (e.g., "chrome.exe") instead of the window title
    void privacyToggled(const QString& exeName, bool isEnabled);

private:
    QLabel* m_iconLabel;
    QLabel* m_nameLabel;
    QLabel* m_exeLabel; // Subtitle for the process name
    QCheckBox* m_toggleSwitch;
};