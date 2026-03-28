#pragma once
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QString>

class AppListItem : public QWidget {
    Q_OBJECT

public:
    // Added fullExePath to extract the icon
    explicit AppListItem(const QString& windowTitle, const QString& fullExePath, const QString& exeName, bool initialState, QWidget* parent = nullptr);

signals:
    void privacyToggled(const QString& exeName, bool isEnabled);

private:
    QLabel* m_iconLabel;
    QLabel* m_nameLabel;
    QLabel* m_exeLabel;
    QCheckBox* m_toggleSwitch;
};