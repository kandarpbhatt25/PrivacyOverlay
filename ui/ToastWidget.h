#pragma once
#include <QWidget>
#include <QString>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

class ToastWidget : public QWidget {
    Q_OBJECT
public:
    explicit ToastWidget(QWidget* parent = nullptr);
    void showToast(const QString& title, const QString& message, bool isError = true);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void fadeOut();

private:
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QTimer* m_timer;
    QPropertyAnimation* m_animation;
};
