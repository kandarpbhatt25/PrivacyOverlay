#include "NotificationCenterWidget.h"
#include <QDateTime>

NotificationCenterWidget::NotificationCenterWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* title = new QLabel("Notification Center", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; letter-spacing: 1px;");
    
    QPushButton* clearBtn = new QPushButton("Clear All", this);
    clearBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #ef4444;
            border: 1px solid rgba(239, 68, 68, 0.3);
            border-radius: 8px;
            padding: 6px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: rgba(239, 68, 68, 0.1);
        }
    )");
    connect(clearBtn, &QPushButton::clicked, this, &NotificationCenterWidget::clearNotifications);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(clearBtn);

    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
            outline: none;
        }
        QListWidget::item {
            background-color: rgba(30, 41, 59, 0.6);
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            margin-bottom: 8px;
            padding: 15px;
        }
        QListWidget::item:hover {
            background-color: rgba(51, 65, 85, 0.6);
        }
    )");
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    layout->addLayout(headerLayout);
    layout->addWidget(m_listWidget);
}

void NotificationCenterWidget::addNotification(const QString& title, const QString& message, bool isError) {
    QListWidgetItem* item = new QListWidgetItem(m_listWidget);
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QHBoxLayout* topLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(title);
    QString color = isError ? "#ef4444" : "#10b981";
    titleLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;").arg(color));

    QLabel* timeLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm:ss AP"));
    timeLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(timeLabel);

    QLabel* msgLabel = new QLabel(message);
    msgLabel->setStyleSheet("color: #cbd5e1; font-size: 13px;");
    msgLabel->setWordWrap(true);

    layout->addLayout(topLayout);
    layout->addWidget(msgLabel);

    item->setSizeHint(QSize(0, 70));
    m_listWidget->insertItem(0, item); // Insert at top
    m_listWidget->setItemWidget(item, widget);
}

void NotificationCenterWidget::clearNotifications() {
    m_listWidget->clear();
}
