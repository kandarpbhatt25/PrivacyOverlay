#include "AppListItem.h"
#include <QVBoxLayout>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

// --- ToggleSwitch Implementation ---
ToggleSwitch::ToggleSwitch(bool initialState, QWidget* parent)
    : QAbstractButton(parent), m_offset(initialState ? 20 : 0) {
    setCheckable(true);
    setChecked(initialState);
    setFixedSize(44, 24); // Pill size
    setCursor(Qt::PointingHandCursor);

    m_anim = new QPropertyAnimation(this, "offset", this);
    m_anim->setDuration(150);

    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        m_anim->setStartValue(m_offset);
        m_anim->setEndValue(checked ? 20 : 0);
        m_anim->start();
        });
}

int ToggleSwitch::offset() const { return m_offset; }

void ToggleSwitch::setOffset(int o) {
    m_offset = o;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath trackPath;
    trackPath.addRoundedRect(0, 0, width(), height(), height() / 2.0, height() / 2.0);

    QColor trackColor = isChecked() ? QColor("#0284c7") : QColor(30, 41, 59, 200);
    p.fillPath(trackPath, trackColor);

    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.drawPath(trackPath);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(2 + m_offset, 2, 20, 20);
}

// --- AppListItem Implementation ---
AppListItem::AppListItem(const QString& windowTitle, const QString& fullExePath, const QString& exeName, bool initialState, QWidget* parent)
    : QWidget(parent) {

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("CardFrame");

    // PERFORMANCE FIX: Removed QGraphicsDropShadowEffect. 
    // Added a thick, dark bottom border to create a zero-lag 3D depth illusion.
    m_cardFrame->setStyleSheet(R"(
        QFrame#CardFrame {
            background-color: #121826; 
            border-radius: 10px;       
            border: 1px solid #1e293b; 
            border-bottom: 4px solid #090e17; /* Creates the 3D lift effect instantly */
        }
        QFrame#CardFrame:hover {
            background-color: #1a2333; 
            border: 1px solid #38bdf8; 
            border-bottom: 4px solid #0284c7; /* Glowing bottom lip on hover */
        }
    )");

    QHBoxLayout* cardLayout = new QHBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(15, 12, 15, 12);
    cardLayout->setSpacing(15);

    m_iconLabel = new QLabel(m_cardFrame);
    m_iconLabel->setFixedSize(36, 36);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    QFileInfo fileInfo(fullExePath);
    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon(fileInfo);

    if (icon.isNull()) {
        m_iconLabel->setStyleSheet("background-color: rgba(255, 255, 255, 0.05); border-radius: 8px;");
    }
    else {
        m_iconLabel->setPixmap(icon.pixmap(32, 32));
    }

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_nameLabel = new QLabel(windowTitle, m_cardFrame);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    font.setPointSize(11);
    m_nameLabel->setFont(font);
    m_nameLabel->setStyleSheet("color: #f8fafc; background: transparent;");
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setMinimumWidth(100);

    m_exeLabel = new QLabel(exeName, m_cardFrame);
    m_exeLabel->setStyleSheet("color: #94a3b8; font-size: 11px; background: transparent;");

    textLayout->addWidget(m_nameLabel);
    textLayout->addWidget(m_exeLabel);

    m_toggleSwitch = new ToggleSwitch(initialState, m_cardFrame);

    cardLayout->addWidget(m_iconLabel);
    cardLayout->addLayout(textLayout);
    cardLayout->addStretch();
    cardLayout->addWidget(m_toggleSwitch);

    outerLayout->addWidget(m_cardFrame);

    connect(m_toggleSwitch, &ToggleSwitch::toggled, this, [this, exeName](bool checked) {
        emit privacyToggled(exeName, checked);
        });
}