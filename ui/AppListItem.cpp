#include "AppListItem.h"
#include <QVBoxLayout>

AppListItem::AppListItem(const QString& windowTitle, const QString& exeName, bool initialState, QWidget* parent)
    : QWidget(parent) {

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 1. Icon Placeholder
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setStyleSheet("background-color: #e8f5e9; border-radius: 8px; border: 1px solid #c8e6c9;");

    // 2. Text Container (Vertical layout for Title and Subtitle)
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    // Window Title
    m_nameLabel = new QLabel(windowTitle, this);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    font.setPointSize(11);
    m_nameLabel->setFont(font);
    m_nameLabel->setStyleSheet("color: #1b5e20;");

    // **Responsive Design Fix:** If the title is too long, truncate it with "..."
    // Note: To make this work perfectly, we set a size policy
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setMinimumWidth(100);

    // Executable Name (Subtitle)
    m_exeLabel = new QLabel(exeName, this);
    m_exeLabel->setStyleSheet("color: #666666; font-size: 11px;");

    textLayout->addWidget(m_nameLabel);
    textLayout->addWidget(m_exeLabel);

    // 3. Toggle Switch
    m_toggleSwitch = new QCheckBox(this);
    m_toggleSwitch->setCursor(Qt::PointingHandCursor);
    m_toggleSwitch->setChecked(initialState); // Set from backend data!

    m_toggleSwitch->setStyleSheet(R"(
        QCheckBox::indicator {
            width: 44px;
            height: 24px;
            border-radius: 12px;
            border: 2px solid #4CAF50;
            background-color: #fafafa;
        }
        QCheckBox::indicator:unchecked {
            background-color: #fafafa;
        }
        QCheckBox::indicator:checked {
            background-color: #81c784;
            border: 2px solid #388E3C;
        }
    )");

    // Assemble layout
    mainLayout->addWidget(m_iconLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(textLayout);
    mainLayout->addStretch(); // Pushes the toggle to the far right
    mainLayout->addWidget(m_toggleSwitch);

    // Wire up the signal
    connect(m_toggleSwitch, &QCheckBox::toggled, this, [this, exeName](bool checked) {
        emit privacyToggled(exeName, checked);
        });
}