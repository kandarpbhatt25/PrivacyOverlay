#pragma once
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>
#include <QAbstractButton>
#include <QPropertyAnimation>
#include <QFrame>

// Custom Animated Toggle Switch (Android/iOS/Win11 style)
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
        Q_PROPERTY(int offset READ offset WRITE setOffset)

public:
    explicit ToggleSwitch(bool initialState = false, QWidget* parent = nullptr);
    int offset() const;
    void setOffset(int o);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_offset;
    QPropertyAnimation* m_anim;
};

// The List Item Widget
class AppListItem : public QWidget {
    Q_OBJECT

public:
    explicit AppListItem(const QString& windowTitle, const QString& fullExePath, const QString& exeName, bool initialState, QWidget* parent = nullptr);
    void setToggleState(bool checked);

signals:
    void privacyToggled(const QString& exeName, bool isEnabled);

private:
    QFrame* m_cardFrame; // The background container
    QLabel* m_iconLabel;
    QLabel* m_nameLabel;
    QLabel* m_exeLabel;
    ToggleSwitch* m_toggleSwitch;
};