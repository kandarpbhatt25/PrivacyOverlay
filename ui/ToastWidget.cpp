#include "ToastWidget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

ToastWidget::ToastWidget(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 15, 20, 15);
    layout->setSpacing(5);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");
    
    m_messageLabel = new QLabel(this);
    m_messageLabel->setStyleSheet("color: #cbd5e1; font-size: 13px;");
    m_messageLabel->setWordWrap(true);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_messageLabel);

    setFixedWidth(300);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 120));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    m_animation = new QPropertyAnimation(this, "windowOpacity");
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ToastWidget::fadeOut);
    
    hide();
}

void ToastWidget::showToast(const QString& title, const QString& message, bool isError) {
    m_titleLabel->setText(title);
    m_messageLabel->setText(message);
    
    if (isError) {
        m_titleLabel->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
    } else {
        m_titleLabel->setStyleSheet("color: #10b981; font-weight: bold; font-size: 14px;");
    }

    adjustSize();

    if (parentWidget()) {
        int x = parentWidget()->width() - width() - 20;
        int y = parentWidget()->height() - height() - 20;
        move(x, y);
    }
    
    setWindowOpacity(1.0);
    show();
    raise();
    
    m_timer->start(3500); // Format for 3.5 seconds
}

void ToastWidget::fadeOut() {
    m_animation->setDuration(500);
    m_animation->setStartValue(1.0);
    m_animation->setEndValue(0.0);
    connect(m_animation, &QPropertyAnimation::finished, this, &QWidget::hide, Qt::UniqueConnection);
    m_animation->start();
}

void ToastWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    
    painter.fillPath(path, QColor(30, 41, 59, 230)); // Slate-800 with transparency
    painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
    painter.drawPath(path);
}
