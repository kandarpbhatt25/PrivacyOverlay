#include "AppListItem.h"
#include <QVBoxLayout>
#include <windows.h>
#include <shellapi.h>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QDir>
#include <QCoreApplication>
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

    QPixmap iconPixmap;
    bool iconLoaded = false;

    // 1. Query OS for Native Icon (Highest Priority)
    if (!fullExePath.isEmpty() && fullExePath.endsWith(".exe", Qt::CaseInsensitive)) {
        UINT iconCount = ExtractIconExW((LPCWSTR)fullExePath.utf16(), -1, nullptr, nullptr, 0);
        if (iconCount > 0) {
            QFileInfo fileInfo(fullExePath);
            QFileIconProvider iconProvider;
            QIcon icon = iconProvider.icon(fileInfo);
            
            if (!icon.isNull()) {
                iconPixmap = icon.pixmap(32, 32);
                if (!iconPixmap.isNull()) iconLoaded = true;
            }
        }
    }

    // 2. Check local custom icons network/theme fallback (Optional constraint compliance)
    if (!iconLoaded) {
        QDir appDir(QCoreApplication::applicationDirPath());
        QString customIconPath = appDir.absoluteFilePath("icons/" + QString(exeName).replace(".exe", ".png", Qt::CaseInsensitive));
        if (QFile::exists(customIconPath)) {
            iconPixmap = QPixmap(customIconPath).scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            if (!iconPixmap.isNull()) iconLoaded = true;
        }
    }

    // 3. Absolute Fallback: Dynamic Letter-Badge
    if (!iconLoaded) {
        iconPixmap = QPixmap(40, 40);
        iconPixmap.fill(Qt::transparent);
        QPainter painter(&iconPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#3b82f6")); 
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 40, 40);
        
        painter.setPen(Qt::white);
        QFont f = painter.font();
        f.setPixelSize(18);
        f.setBold(true);
        painter.setFont(f);
        
        QString letter = windowTitle.isEmpty() ? "?" : windowTitle.left(1).toUpper();
        painter.drawText(iconPixmap.rect(), Qt::AlignCenter, letter);
    }

    m_iconLabel->setPixmap(iconPixmap);
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_nameLabel = new QLabel(windowTitle, m_cardFrame);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    font.setPointSize(11);
    m_nameLabel->setFont(font);
    m_nameLabel->setStyleSheet("color: #f8fafc; background: transparent;");
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setMinimumWidth(0);

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

void AppListItem::setToggleState(bool checked) {
    if (m_toggleSwitch->isChecked() != checked) {
        m_toggleSwitch->setChecked(checked);
    }
}