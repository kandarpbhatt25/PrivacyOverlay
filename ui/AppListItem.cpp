#include "AppListItem.h"
#include <QVBoxLayout>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>

AppListItem::AppListItem(const QString& windowTitle, const QString& fullExePath, const QString& exeName, bool initialState, QWidget* parent)
    : QWidget(parent) {

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 1. Extract Real App Icon
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    QFileInfo fileInfo(fullExePath);
    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon(fileInfo);

    // Fallback to a default style if the icon can't be found (e.g., UWP apps)
    if (icon.isNull()) {
        m_iconLabel->setStyleSheet("background-color: rgba(255, 255, 255, 0.1); border-radius: 8px;");
    }
    else {
        m_iconLabel->setPixmap(icon.pixmap(32, 32)); // Scale to 32x32
    }

    // 2. Text Container
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_nameLabel = new QLabel(windowTitle, this);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    font.setPointSize(11);
    m_nameLabel->setFont(font);
    m_nameLabel->setStyleSheet("color: #f1f5f9;");
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setMinimumWidth(100);

    m_exeLabel = new QLabel(exeName, this);
    m_exeLabel->setStyleSheet("color: #64748b; font-size: 11px;");

    textLayout->addWidget(m_nameLabel);
    textLayout->addWidget(m_exeLabel);

    // 3. Toggle Switch (Bluish Dark Theme)
    m_toggleSwitch = new QCheckBox(this);
    m_toggleSwitch->setCursor(Qt::PointingHandCursor);
    m_toggleSwitch->setChecked(initialState);

    m_toggleSwitch->setStyleSheet(R"(
        QCheckBox::indicator {
            width: 44px;
            height: 24px;
            border-radius: 12px;
            border: 2px solid #334155;
            background-color: rgba(15, 23, 42, 0.8);
        }
        QCheckBox::indicator:unchecked { background-color: rgba(15, 23, 42, 0.8); }
        QCheckBox::indicator:checked { background-color: #0284c7; border: 2px solid #38bdf8; }
    )");

    mainLayout->addWidget(m_iconLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(m_toggleSwitch);

    connect(m_toggleSwitch, &QCheckBox::toggled, this, [this, exeName](bool checked) {
        emit privacyToggled(exeName, checked);
        });
}