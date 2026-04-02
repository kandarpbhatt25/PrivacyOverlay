#include "PresetsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include <QInputDialog>
#include <QUuid>
#include <QMessageBox>

PresetsWidget::PresetsWidget(QWidget* parent) : QWidget(parent) {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setAlignment(Qt::AlignTop);
    m_mainLayout->setContentsMargins(30, 30, 30, 30);

    QLabel* title = new QLabel("Quick Presets", this);
    title->setStyleSheet("font-size: 28px; font-weight: 800; color: #f8fafc; margin-bottom: 20px;");
    m_mainLayout->addWidget(title);

    m_gridLayout = new QGridLayout();
    m_gridLayout->setSpacing(20);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_mainLayout->addLayout(m_gridLayout);

    renderProfiles();
}

void PresetsWidget::renderProfiles() {
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    auto profiles = persistence::SettingsManager::getInstance().loadAllProfiles();

    int row = 0;
    int col = 0;
    const int cols = 3;

    for (const auto& profile : profiles) {
        QWidget* card = new QWidget(this);
        card->setFixedSize(280, 160);
        
        QString borderStyle = profile.isActive ? "border: 2px solid #38bdf8;" : "border: 1px solid #1e293b;";
        QString bgStyle = profile.isActive ? "background-color: rgba(56, 189, 248, 0.1);" : "background-color: rgba(30, 41, 59, 0.6);";
        card->setStyleSheet(QString("QWidget { %1 %2 border-radius: 12px; }").arg(borderStyle, bgStyle));

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);

        QLabel* nameLabel = new QLabel(profile.name, card);
        nameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: white; border: none; background: transparent;");
        cardLayout->addWidget(nameLabel);

        int ruleCount = profile.rules.size();
        QLabel* ruleLabel = new QLabel(QString::number(ruleCount) + " App(s) Shielded", card);
        ruleLabel->setStyleSheet("font-size: 12px; color: #94a3b8; border: none; background: transparent;");
        cardLayout->addWidget(ruleLabel);

        cardLayout->addStretch();

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setContentsMargins(0,0,0,0);
        
        if (!profile.isActive) {
            QPushButton* activateBtn = new QPushButton("Activate", card);
            activateBtn->setStyleSheet("QPushButton { background-color: #0284c7; color: white; border: none; border-radius: 6px; padding: 6px; font-weight: bold; } QPushButton:hover { background-color: #0369a1; }");
            activateBtn->setCursor(Qt::PointingHandCursor);
            connect(activateBtn, &QPushButton::clicked, this, [this, id = profile.id]() { onActivateProfile(id); });
            btnLayout->addWidget(activateBtn);
        } else {
            QLabel* activeLabel = new QLabel("Active", card);
            activeLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #38bdf8; border: none; background: transparent;");
            activeLabel->setAlignment(Qt::AlignCenter);
            btnLayout->addWidget(activeLabel);
        }

        if (profile.id != "default" && !profile.isActive) {
            QPushButton* deleteBtn = new QPushButton("Del", card);
            deleteBtn->setStyleSheet("QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px; font-weight: bold; max-width: 50px; } QPushButton:hover { background-color: #dc2626; }");
            deleteBtn->setCursor(Qt::PointingHandCursor);
            connect(deleteBtn, &QPushButton::clicked, this, [this, id = profile.id]() { onDeleteProfile(id); });
            btnLayout->addWidget(deleteBtn);
        }

        cardLayout->addLayout(btnLayout);
        m_gridLayout->addWidget(card, row, col);

        col++;
        if (col >= cols) { col = 0; row++; }
    }

    QPushButton* addBtn = new QPushButton("Create New Preset\n+", this);
    addBtn->setFixedSize(280, 160);
    addBtn->setStyleSheet("QPushButton { background-color: rgba(30, 41, 59, 0.2); border: 2px dashed #475569; border-radius: 12px; color: #94a3b8; font-size: 16px; font-weight: bold; } QPushButton:hover { border-color: #38bdf8; color: #38bdf8; }");
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &PresetsWidget::onAddProfile);
    
    m_gridLayout->addWidget(addBtn, row, col);
}

void PresetsWidget::onAddProfile() {
    bool ok;
    QString text = QInputDialog::getText(this, "New Preset", "Enter Preset Name:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        models::Profile newProfile;
        newProfile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        newProfile.name = text;
        newProfile.isActive = false;
        
        persistence::SettingsManager::getInstance().saveProfile(newProfile);
        renderProfiles();
    }
}

void PresetsWidget::onDeleteProfile(const QString& id) {
    if (id == "default") return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Preset", "Are you sure you want to delete this preset forever?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        persistence::SettingsManager::getInstance().deleteProfile(id);
        renderProfiles();
    }
}

void PresetsWidget::onActivateProfile(const QString& id) {
    persistence::SettingsManager::getInstance().setActiveProfile(id);
    renderProfiles();
    emit profileActivated();
}