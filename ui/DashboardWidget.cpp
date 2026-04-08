#include "DashboardWidget.h"
#include "../models/AppState.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QDateTime>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include "../persistence/SettingsManager.h"
#include "../models/Profile.h"

// Helper to create a styled card frame
QFrame* createCard(QWidget* parent) {
    QFrame* frame = new QFrame(parent);
    frame->setStyleSheet("QFrame { background-color: rgba(30, 41, 59, 0.6); border-radius: 12px; border: 1px solid rgba(255, 255, 255, 0.05); }");
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(frame);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 4);
    frame->setGraphicsEffect(shadow);
    return frame;
}

// Helper to create a button with icon, title, and subtitle
QPushButton* createActionButton(const QString& title, const QString& subtitle, const QString& colorHex, QWidget* parent) {
    QPushButton* btn = new QPushButton(parent);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(70);
    
    QVBoxLayout* btnLayout = new QVBoxLayout(btn);
    btnLayout->setContentsMargins(15, 10, 15, 10);
    btnLayout->setAlignment(Qt::AlignCenter);

    QLabel* titleLabel = new QLabel(title, btn);
    titleLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 16px; background: transparent; border: none;").arg(colorHex));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    QLabel* subLabel = new QLabel(subtitle, btn);
    subLabel->setStyleSheet("color: #94a3b8; font-size: 12px; background: transparent; border: none;");
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    btnLayout->addWidget(titleLabel);
    btnLayout->addWidget(subLabel);

    btn->setStyleSheet(QString(R"(
        QPushButton { 
            background-color: rgba(30, 41, 59, 0.5); 
            border: 1px solid rgba(255,255,255,0.05); 
            border-radius: 12px; 
        }
        QPushButton:hover { 
            background-color: rgba(51, 65, 85, 0.7); 
            border: 1px solid %1; 
        }
        QPushButton:pressed {
            background-color: rgba(15, 23, 42, 0.8);
        }
    )").arg(colorHex));

    return btn;
}

DashboardWidget::DashboardWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); 
    
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; } QWidget#scrollContent { background: transparent; }");

    QWidget* scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName("scrollContent");
    QVBoxLayout* layout = new QVBoxLayout(scrollContent);
    layout->setContentsMargins(35, 35, 35, 35);
    layout->setSpacing(25);

    // --- Header Section ---
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QVBoxLayout* titleLayout = new QVBoxLayout();
    QLabel* title = new QLabel("Privacy Protection Dashboard", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; letter-spacing: 0.5px;");
    QLabel* subtitle = new QLabel("Real-time telemetry and control", this);
    subtitle->setStyleSheet("color: #94a3b8; font-size: 14px;");
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);

    m_statusLabel = new QLabel("● SYSTEM PROTECTED", this);
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold; font-size: 14px; background-color: rgba(16, 185, 129, 0.1); padding: 8px 16px; border-radius: 16px; border: 1px solid rgba(16, 185, 129, 0.2);");

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(m_statusLabel, 0, Qt::AlignVCenter);

    // --- Section 1: Status Cards (Top Row) ---
    QHBoxLayout* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(20);

    // Card 1
    QFrame* card1 = createCard(this);
    QVBoxLayout* c1Layout = new QVBoxLayout(card1);
    QLabel* c1Title = new QLabel("Engine Status", card1);
    c1Title->setStyleSheet("color: #94a3b8; font-weight: bold; font-size: 12px; background: transparent; border: none;");
    QLabel* c1Val = new QLabel("Active", card1);
    c1Val->setStyleSheet("color: #f8fafc; font-weight: 800; font-size: 26px; background: transparent; border: none;");
    QLabel* c1Sub = new QLabel("Kernel hooks deployed", card1);
    c1Sub->setStyleSheet("color: #10b981; font-size: 12px; background: transparent; border: none;");
    c1Layout->addWidget(c1Title);
    c1Layout->addWidget(c1Val);
    c1Layout->addWidget(c1Sub);

    // Card 2
    QFrame* card2 = createCard(this);
    QVBoxLayout* c2Layout = new QVBoxLayout(card2);
    QLabel* c2Title = new QLabel("Shielded Apps", card2);
    c2Title->setStyleSheet("color: #94a3b8; font-weight: bold; font-size: 12px; background: transparent; border: none;");
    m_shieldedCountLabel = new QLabel("0", card2);
    m_shieldedCountLabel->setStyleSheet("color: #f8fafc; font-weight: 800; font-size: 26px; background: transparent; border: none;");
    QLabel* c2Sub = new QLabel("Actively masked by engine", card2);
    c2Sub->setStyleSheet("color: #94a3b8; font-size: 12px; background: transparent; border: none;");
    c2Layout->addWidget(c2Title);
    c2Layout->addWidget(m_shieldedCountLabel);
    c2Layout->addWidget(c2Sub);

    cardsLayout->addWidget(card1);
    cardsLayout->addWidget(card2);

    // --- Section 2: Quick Controls ---
    QLabel* controlsTitle = new QLabel("Quick Controls", this);
    controlsTitle->setStyleSheet("color: #e2e8f0; font-size: 18px; font-weight: bold; margin-top: 10px;");
    
    QHBoxLayout* btnsLayout = new QHBoxLayout();
    btnsLayout->setSpacing(15);
    
    m_btnScan = createActionButton("Refresh Data", "Scan for active anomalies", "#38bdf8", this);
    m_btnManage = createActionButton("Preset Profiles", "Manage app rulesets", "#818cf8", this);
    m_killSwitchBtn = createActionButton("Emergency Blackout", "Disable privacy shield", "#ef4444", this);
    
    m_killSwitchBtn->setStyleSheet(R"(
        QPushButton { background-color: rgba(153, 27, 27, 0.4); border: 1px solid rgba(239, 68, 68, 0.4); border-radius: 12px; }
        QPushButton:hover { background-color: rgba(220, 38, 38, 0.6); border: 1px solid #ef4444; }
    )");

    btnsLayout->addWidget(m_btnScan);
    btnsLayout->addWidget(m_btnManage);
    btnsLayout->addWidget(m_killSwitchBtn);

    // --- Section 3: Data Views (60/40 Split) ---
    QHBoxLayout* dataLayout = new QHBoxLayout();
    dataLayout->setSpacing(25);
    dataLayout->setContentsMargins(0, 10, 0, 0);

    // Column A: Live Monitoring (Stretch 6)
    QWidget* colA = new QWidget(this);
    QVBoxLayout* colALayout = new QVBoxLayout(colA);
    colALayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout* liveHeader = new QHBoxLayout();
    QLabel* liveTitle = new QLabel("Live Monitoring", colA);
    liveTitle->setStyleSheet("color: #e2e8f0; font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    
    QPushButton* btnExpand = new QPushButton("⤢", colA);
    btnExpand->setToolTip("View Full Monitoring List");
    btnExpand->setStyleSheet("QPushButton { background: transparent; color: #94a3b8; font-size: 16px; border: none; } QPushButton:hover { color: #f8fafc; }");
    btnExpand->setCursor(Qt::PointingHandCursor);
    
    liveHeader->addWidget(liveTitle);
    liveHeader->addStretch();
    liveHeader->addWidget(btnExpand);
    
    QFrame* liveCard = createCard(colA);
    liveCard->setFixedHeight(350); // Matching a fixed height for max height limits
    
    QVBoxLayout* liveCardLayout = new QVBoxLayout(liveCard);
    liveCardLayout->setContentsMargins(0,0,0,0);
    
    QScrollArea* liveScroll = new QScrollArea(liveCard);
    liveScroll->setWidgetResizable(true);
    liveScroll->setStyleSheet("QScrollArea { border: none; background: transparent; } QWidget#liveScrollContent { background: transparent; }");
    
    QWidget* liveScrollContent = new QWidget(liveScroll);
    liveScrollContent->setObjectName("liveScrollContent");
    m_liveMonitoringLayout = new QVBoxLayout(liveScrollContent);
    m_liveMonitoringLayout->setContentsMargins(15, 15, 15, 15);
    m_liveMonitoringLayout->setSpacing(10);
    m_liveMonitoringLayout->setAlignment(Qt::AlignTop);
    
    liveScroll->setWidget(liveScrollContent);
    liveCardLayout->addWidget(liveScroll);

    colALayout->addLayout(liveHeader);
    colALayout->addWidget(liveCard);

    // Column B: Recent Activity (Stretch 4)
    QWidget* colB = new QWidget(this);
    QVBoxLayout* colBLayout = new QVBoxLayout(colB);
    colBLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* activityTitle = new QLabel("Recent Activity", colB);
    activityTitle->setStyleSheet("color: #e2e8f0; font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    
    QFrame* activityCard = createCard(colB);
    activityCard->setFixedHeight(350); // Synchronizing height constraints
    QVBoxLayout* actCardLayout = new QVBoxLayout(activityCard);
    actCardLayout->setContentsMargins(0, 0, 0, 0);
    
    QScrollArea* actScroll = new QScrollArea(activityCard);
    actScroll->setWidgetResizable(true);
    actScroll->setStyleSheet("QScrollArea { border: none; background: transparent; } QWidget#actScrollContent { background: transparent; }");
    
    QWidget* actScrollContent = new QWidget(actScroll);
    actScrollContent->setObjectName("actScrollContent");
    m_recentActivityLayout = new QVBoxLayout(actScrollContent);
    m_recentActivityLayout->setContentsMargins(15, 15, 15, 15);
    m_recentActivityLayout->setSpacing(10);
    m_recentActivityLayout->setAlignment(Qt::AlignTop);
    
    actScroll->setWidget(actScrollContent);
    actCardLayout->addWidget(actScroll);

    colBLayout->addWidget(activityTitle);
    colBLayout->addWidget(activityCard);

    dataLayout->addWidget(colA, 6);
    dataLayout->addWidget(colB, 4);

    // --- Build Main Layout ---
    layout->addLayout(headerLayout);
    layout->addLayout(cardsLayout);
    layout->addWidget(controlsTitle);
    layout->addLayout(btnsLayout);
    layout->addLayout(dataLayout);
    layout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // Connecting logic
    // Connecting logic
    addActivityLogEvent("System Initialized", "Privacy Engine started on launch", "info");

    connect(m_btnScan, &QPushButton::clicked, this, [this]() {
        emit refreshRequested();
        addActivityLogEvent("Data Force Rescan", "Manual discovery scan triggered", "warning");
    });
    
    QMenu* presetMenu = new QMenu(m_btnManage);
    presetMenu->setStyleSheet("QMenu { background-color: #1e293b; color: white; border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; } QMenu::item:selected { background-color: #38bdf8; } QMenu::item { padding: 8px 30px; }");
    m_btnManage->setMenu(presetMenu);
    
    connect(m_btnManage, &QPushButton::clicked, this, [this, presetMenu]() {
        presetMenu->clear();
        auto profiles = persistence::SettingsManager::getInstance().loadAllProfiles();
        models::Profile activeProf = persistence::SettingsManager::getInstance().getActiveProfile();
        
        for (int i = 0; i < profiles.size(); ++i) {
            QString title = profiles[i].name;
            if (profiles[i].id == activeProf.id) title += " (Active)";
            
            QAction* act = presetMenu->addAction(title);
            connect(act, &QAction::triggered, this, [this, i]() {
                emit presetSwitchRequested(i);
                addActivityLogEvent("Profile Switched", "Active ruleset updated globally", "success");
            });
        }
    });

    connect(m_killSwitchBtn, &QPushButton::clicked, this, [this, c1Val, c1Sub]() {
        m_isEmergencyMuted = !m_isEmergencyMuted;
        if (m_isEmergencyMuted) {
            m_statusLabel->setText("⚠ EMERGENCY MUTED");
            m_statusLabel->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px; background-color: rgba(239, 68, 68, 0.1); padding: 8px 16px; border-radius: 16px; border: 1px solid rgba(239, 68, 68, 0.2);");
            
            QList<QLabel*> labels = m_killSwitchBtn->findChildren<QLabel*>();
            if (labels.size() >= 2) {
                 labels[0]->setText("Disable Privacy Mode");
                 labels[0]->setStyleSheet("color: #38bdf8; font-weight: bold; font-size: 16px; background: transparent; border: none;");
                 labels[1]->setText("Restore saved protections");
            }
            m_killSwitchBtn->setStyleSheet(R"(
                QPushButton { background-color: rgba(2, 132, 199, 0.4); border: 1px solid rgba(56, 189, 248, 0.4); border-radius: 12px; }
                QPushButton:hover { background-color: rgba(2, 132, 199, 0.6); border: 1px solid #38bdf8; }
                QPushButton:pressed { background-color: rgba(7, 89, 133, 0.8); }
            )");

            c1Val->setText("Blackout");
            c1Val->setStyleSheet("color: #ef4444; font-weight: 800; font-size: 26px; background: transparent; border: none;");
            c1Sub->setText("All active apps hidden");
            c1Sub->setStyleSheet("color: #ef4444; font-size: 12px; background: transparent; border: none;");

            addActivityLogEvent("Emergency Triggered", "All active applications force masked", "warning");
            emit emergencyBlackoutTriggered(true);
        }
        else {
            m_statusLabel->setText("● SYSTEM PROTECTED");
            m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold; font-size: 14px; background-color: rgba(16, 185, 129, 0.1); padding: 8px 16px; border-radius: 16px; border: 1px solid rgba(16, 185, 129, 0.2);");
            
            QList<QLabel*> labels = m_killSwitchBtn->findChildren<QLabel*>();
            if (labels.size() >= 2) {
                 labels[0]->setText("Emergency Blackout");
                 labels[0]->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 16px; background: transparent; border: none;");
                 labels[1]->setText("Mask all applications");
            }
            m_killSwitchBtn->setStyleSheet(R"(
                QPushButton { background-color: rgba(153, 27, 27, 0.4); border: 1px solid rgba(239, 68, 68, 0.4); border-radius: 12px; }
                QPushButton:hover { background-color: rgba(220, 38, 38, 0.6); border: 1px solid #ef4444; }
                QPushButton:pressed { background-color: rgba(127, 29, 29, 0.8); }
            )");

            c1Val->setText("Active");
            c1Val->setStyleSheet("color: #f8fafc; font-weight: 800; font-size: 26px; background: transparent; border: none;");
            c1Sub->setText("Rules dynamically deployed");
            c1Sub->setStyleSheet("color: #10b981; font-size: 12px; background: transparent; border: none;");

            addActivityLogEvent("Blackout Reverted", "System restored to normal privacy profile", "success");
            emit emergencyBlackoutTriggered(false);
        }
    });

    connect(btnExpand, &QPushButton::clicked, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle("Detailed Process Monitoring");
        dialog.resize(600, 700);
        dialog.setStyleSheet("QDialog { background-color: #0f172a; } QLabel { color: white; }");
        
        QVBoxLayout* dLayout = new QVBoxLayout(&dialog);
        QLabel* dTitle = new QLabel("Actively Scanned Applications", &dialog);
        dTitle->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px;");
        dLayout->addWidget(dTitle);
        
        QScrollArea* dScroll = new QScrollArea(&dialog);
        dScroll->setWidgetResizable(true);
        dScroll->setStyleSheet("QScrollArea { border: 1px solid #1e293b; background: #1e293b; border-radius: 8px; }");
        
        QWidget* dContent = new QWidget(dScroll);
        QVBoxLayout* dListLayout = new QVBoxLayout(dContent);
        
        // This is a static snapshot modal. We loop through the child widgets in the real layout to mirror it or construct items natively
        for (int i = 0; i < m_liveMonitoringLayout->count(); ++i) {
            QWidget* existingRow = m_liveMonitoringLayout->itemAt(i)->widget();
            if (!existingRow) continue;
            
            // Re-render a proxy item natively
            QList<QLabel*> labels = existingRow->findChildren<QLabel*>();
            if (labels.size() >= 4) {
                 QWidget* row = new QWidget(dContent);
                 row->setStyleSheet("border-bottom: 1px solid rgba(255,255,255,0.05);");
                 QHBoxLayout* rowL = new QHBoxLayout(row);
                 
                 QLabel* iconL = new QLabel(row);
                 if (labels[0]->pixmap().isNull()) iconL->setText(labels[0]->text());
                 else iconL->setPixmap(labels[0]->pixmap());
                 
                 QLabel* nameL = new QLabel(labels[1]->text(), row);
                 nameL->setStyleSheet("font-weight: bold; font-size: 15px;");
                 
                 QLabel* pathL = new QLabel(labels[2]->text(), row);
                 pathL->setStyleSheet("color: #94a3b8; font-size: 11px;");
                 
                 QLabel* statL = new QLabel(labels[3]->text(), row);
                 statL->setStyleSheet(labels[3]->styleSheet());
                 
                 QVBoxLayout* textL2 = new QVBoxLayout();
                 textL2->addWidget(nameL);
                 textL2->addWidget(pathL);
                 
                 rowL->addWidget(iconL);
                 rowL->addLayout(textL2);
                 rowL->addStretch();
                 rowL->addWidget(statL);
                 
                 dListLayout->addWidget(row);
            }
        }
        dListLayout->addStretch();
        dScroll->setWidget(dContent);
        dLayout->addWidget(dScroll);
        
        QPushButton* closeBtn = new QPushButton("Close", &dialog);
        closeBtn->setStyleSheet("background-color: #38bdf8; color: white; border: none; padding: 10px; border-radius: 6px; font-weight: bold;");
        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        dLayout->addWidget(closeBtn, 0, Qt::AlignRight);
        
        dialog.exec();
    });
}

void DashboardWidget::addActivityLogEvent(const QString& title, const QString& description, const QString& statusLevel) {
    QWidget* block = new QWidget();
    block->setStyleSheet("background-color: rgba(255, 255, 255, 0.03); border-radius: 8px;");
    QVBoxLayout* blayout = new QVBoxLayout(block);
    blayout->setContentsMargins(12, 12, 12, 12);
    blayout->setSpacing(4);

    QHBoxLayout* head = new QHBoxLayout();
    QLabel* titleL = new QLabel(title, block);
    
    QString tColor = "#e2e8f0";
    if (statusLevel == "warning") tColor = "#ef4444";
    else if (statusLevel == "info") tColor = "#38bdf8";
    
    titleL->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 13px; background: transparent; border: none;").arg(tColor));
    
    QLabel* timeL = new QLabel(QDateTime::currentDateTime().toString("hh:mm AP"), block);
    timeL->setStyleSheet("color: #64748b; font-size: 11px; background: transparent; border: none;");
    
    head->addWidget(titleL);
    head->addStretch();
    head->addWidget(timeL);

    QLabel* descL = new QLabel(description, block);
    descL->setStyleSheet("color: #94a3b8; font-size: 12px; background: transparent; border: none;");
    descL->setWordWrap(true);

    blayout->addLayout(head);
    blayout->addWidget(descL);

    m_recentActivityLayout->insertWidget(0, block); 
}

void DashboardWidget::onGlobalStateChanged(const QMap<QString, models::AppState>& state) {
    // Clear existing layout items (except the stretch if possible, but taking clear approach)
    QLayoutItem* child;
    while ((child = m_liveMonitoringLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    QWidget* parentCard = m_liveMonitoringLayout->parentWidget();

    int activeCount = 0;
    int shieldedCount = 0;

    for (const auto& app : state) {
        if (app.isActive) {
            activeCount++;
            if (app.isHidden) shieldedCount++;

            QWidget* row = new QWidget(parentCard);
            row->setStyleSheet("background: transparent; border-bottom: 1px solid rgba(255,255,255,0.05);");
            QHBoxLayout* rlayout = new QHBoxLayout(row);
            rlayout->setContentsMargins(5, 10, 5, 10);
            
            QLabel* icon = new QLabel(row); 
            if (!app.cachedNativeIcon.isNull()) {
                icon->setPixmap(app.cachedNativeIcon.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                icon->setText("🔹");
                icon->setStyleSheet("font-size: 18px; border: none;");
            }
            
            QVBoxLayout* textL = new QVBoxLayout();
            QLabel* n = new QLabel(app.displayName, row);
            n->setStyleSheet("color: #f8fafc; font-weight: bold; font-size: 14px; border: none;");
            QLabel* d = new QLabel(app.exeName, row);
            d->setStyleSheet("color: #94a3b8; font-size: 12px; border: none;");
            textL->addWidget(n);
            textL->addWidget(d);
            
            QString status = app.getStatusLabel();
            QString color = app.isHidden ? "#38bdf8" : "#94a3b8"; // Blue if masked, gray if visible
            
            QLabel* s = new QLabel(status, row);
            s->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px; padding: 4px 8px; border-radius: 6px; background-color: rgba(255,255,255,0.05); border: none;").arg(color));
            
            rlayout->addWidget(icon);
            rlayout->addLayout(textL);
            rlayout->addStretch();
            rlayout->addWidget(s);
            
            m_liveMonitoringLayout->addWidget(row);
        }
    }
    m_liveMonitoringLayout->addStretch();

    if (m_shieldedCountLabel) {
        m_shieldedCountLabel->setText(QString::number(shieldedCount));
    }
}