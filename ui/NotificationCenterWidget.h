#pragma once
#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class NotificationCenterWidget : public QWidget {
    Q_OBJECT
public:
    explicit NotificationCenterWidget(QWidget* parent = nullptr);

public slots:
    void addNotification(const QString& title, const QString& message, bool isError = true);
    void clearNotifications();

private:
    QListWidget* m_listWidget;
};
