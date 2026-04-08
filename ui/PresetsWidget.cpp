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
#include <QDialog>
#include <QScrollArea>
#include <QCheckBox>
#include <QLineEdit>
#include "persistence/AppCacheManager.h"
#include "core/IconManager.h"

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
        
        QPushButton* editBtn = new QPushButton("Edit", card);
        editBtn->setStyleSheet("QPushButton { background-color: #64748b; color: white; border: none; border-radius: 6px; padding: 6px; font-weight: bold; max-width: 50px; } QPushButton:hover { background-color: #475569; }");
        editBtn->setCursor(Qt::PointingHandCursor);
        connect(editBtn, &QPushButton::clicked, this, [this, id = profile.id]() { onEditProfile(id); });
        btnLayout->addWidget(editBtn);

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
    models::Profile newProfile;
    newProfile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newProfile.name = "New Profile";
    newProfile.isActive = false;
    
    // Save it temporarily so the editor can grab it
    persistence::SettingsManager::getInstance().saveProfile(newProfile);
    
    onEditProfile(newProfile.id);
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

void PresetsWidget::onEditProfile(const QString& id) {
    auto profiles = persistence::SettingsManager::getInstance().loadAllProfiles();
    models::Profile* targetProfile = nullptr;
    for (auto& p : profiles) {
        if (p.id == id) {
            targetProfile = &p;
            break;
        }
    }
    if (!targetProfile) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Profile: " + targetProfile->name);
    dialog.resize(500, 600);
    dialog.setStyleSheet("QDialog { background-color: #111827; } QLabel { color: #f8fafc; }");

    QVBoxLayout* dLayout = new QVBoxLayout(&dialog);
    
    QLabel* renameLabel = new QLabel("Profile Name:", &dialog);
    renameLabel->setStyleSheet("font-weight: bold; color: #94a3b8;");
    dLayout->addWidget(renameLabel);
    
    QLineEdit* nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(targetProfile->name);
    nameEdit->setStyleSheet("QLineEdit { background-color: rgba(30, 41, 59, 1); border: 1px solid #38bdf8; border-radius: 6px; padding: 8px; color: white; font-weight: bold; font-size: 16px; margin-bottom: 10px; }");
    dLayout->addWidget(nameEdit);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* modalSearchField = new QLineEdit(&dialog);
    modalSearchField->setPlaceholderText("Search to filter apps...");
    modalSearchField->setStyleSheet("QLineEdit { background-color: rgba(30, 41, 59, 1); border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; padding: 6px; color: white; margin-bottom: 5px; }");
    searchLayout->addWidget(modalSearchField);

    QCheckBox* showSelectedOnly = new QCheckBox("Show Selected Only", &dialog);
    showSelectedOnly->setStyleSheet("color: #94a3b8; font-weight: bold;");
    showSelectedOnly->setCursor(Qt::PointingHandCursor);
    searchLayout->addWidget(showSelectedOnly);
    dLayout->addLayout(searchLayout);

    QHBoxLayout* bulkLayout = new QHBoxLayout();
    bulkLayout->setContentsMargins(0, 0, 0, 10);
    QPushButton* btnSelectAll = new QPushButton("Select All", &dialog);
    btnSelectAll->setCursor(Qt::PointingHandCursor);
    btnSelectAll->setStyleSheet("color: #38bdf8; background: transparent; border: none; font-weight: bold; text-align: left; max-width: 80px;");
    
    QPushButton* btnDeselectAll = new QPushButton("Deselect All", &dialog);
    btnDeselectAll->setCursor(Qt::PointingHandCursor);
    btnDeselectAll->setStyleSheet("color: #ef4444; background: transparent; border: none; font-weight: bold; text-align: left; max-width: 80px;");
    
    bulkLayout->addWidget(btnSelectAll);
    bulkLayout->addWidget(btnDeselectAll);
    bulkLayout->addStretch();
    dLayout->addLayout(bulkLayout);

    QLabel* title = new QLabel("Select applications to shield:", &dialog);
    title->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 5px; color: #94a3b8;");
    dLayout->addWidget(title);

    QScrollArea* scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #1e293b; background: transparent; border-radius: 8px; } QWidget#scrollContent { background: transparent; }");

    QWidget* scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName("scrollContent");
    QVBoxLayout* listLayout = new QVBoxLayout(scrollContent);

    auto cachedApps = persistence::AppCacheManager::getInstance().loadCache();
    QList<QCheckBox*> checkboxes;

    for (const auto& app : cachedApps) {
        QWidget* rowWidget = new QWidget(scrollContent);
        rowWidget->setStyleSheet("border-bottom: 1px solid rgba(255,255,255,0.05);");
        QHBoxLayout* rowL = new QHBoxLayout(rowWidget);
        rowL->setContentsMargins(5, 5, 5, 5);
        
        QLabel* iconL = new QLabel(rowWidget);
        iconL->setFixedSize(24, 24);
        iconL->setPixmap(core::IconManager::getInstance().getIcon(app.executablePath, app.displayName).scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        
        QCheckBox* cb = new QCheckBox(app.displayName, rowWidget);
        cb->setProperty("exeName", app.executableName);
        cb->setStyleSheet("QCheckBox { color: #f8fafc; font-size: 14px; font-weight: bold; border: none;} QCheckBox::indicator { width: 18px; height: 18px; }");
        
        bool isAlreadyProtected = false;
        for (const auto& rule : targetProfile->rules) {
            if (rule.matchType == models::MatchType::ProcessName && rule.matchPattern == app.executableName) {
                isAlreadyProtected = true;
                break;
            }
        }
        cb->setChecked(isAlreadyProtected);
        
        rowL->addWidget(iconL);
        rowL->addWidget(cb);
        rowL->addStretch();
        
        listLayout->addWidget(rowWidget);
        checkboxes.append(cb);
    }
    
    listLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    dLayout->addWidget(scrollArea);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* cancelBtn = new QPushButton("Cancel", &dialog);
    cancelBtn->setStyleSheet("background-color: #475569; color: white; border: none; padding: 8px 16px; border-radius: 6px;");
    QPushButton* saveBtn = new QPushButton("Save Changes", &dialog);
    saveBtn->setStyleSheet("background-color: #0ea5e9; color: white; border: none; padding: 8px 16px; border-radius: 6px; font-weight: bold;");
    
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    dLayout->addLayout(btnLayout);

    auto applyModalFilter = [=]() {
        QString query = modalSearchField->text().trimmed().toLower();
        bool filterChecked = showSelectedOnly->isChecked();
        
        for (QCheckBox* cb : checkboxes) {
            bool show = true;
            if (filterChecked && !cb->isChecked()) show = false;
            if (show && !query.isEmpty() && !cb->text().toLower().contains(query) && !cb->property("exeName").toString().toLower().contains(query)) {
                show = false;
            }
            cb->parentWidget()->setVisible(show);
        }
    };

    connect(modalSearchField, &QLineEdit::textChanged, &dialog, applyModalFilter);
    connect(showSelectedOnly, &QCheckBox::toggled, &dialog, applyModalFilter);
    connect(btnSelectAll, &QPushButton::clicked, &dialog, [=]() {
        for (QCheckBox* cb : checkboxes) {
            if (cb->parentWidget()->isVisible()) cb->setChecked(true);
        }
    });
    connect(btnDeselectAll, &QPushButton::clicked, &dialog, [=]() {
        for (QCheckBox* cb : checkboxes) {
            if (cb->parentWidget()->isVisible()) cb->setChecked(false);
        }
    });

    // Make manual checkbox toggles invoke filter evaluation immediately if "Show Selected" is active
    for (QCheckBox* cb : checkboxes) {
        connect(cb, &QCheckBox::toggled, &dialog, [=]() {
            if (showSelectedOnly->isChecked()) applyModalFilter();
        });
    }

    if (dialog.exec() == QDialog::Accepted) {
        targetProfile->name = nameEdit->text().trimmed();
        if (targetProfile->name.isEmpty()) targetProfile->name = "Unnamed Profile";
        
        targetProfile->rules.clear();
        for (QCheckBox* cb : checkboxes) {
            if (cb->isChecked()) {
                models::AppRule rule;
                rule.matchType = models::MatchType::ProcessName;
                rule.matchPattern = cb->property("exeName").toString();
                rule.maskMode = models::MaskMode::BlackMask;
                targetProfile->rules.append(rule);
            }
        }
        persistence::SettingsManager::getInstance().saveProfile(*targetProfile);
        renderProfiles();
        
        // If they edited the active profile, trigger the update map!
        if (targetProfile->isActive) {
            emit profileActivated();
        }
    } else {
        // Did they cancel a New Profile? If it has 0 rules and "New Profile", maybe delete it?
        // Let's just leave it, or they can click Delete manually. But we should render anyway to show default name.
        renderProfiles();
    }
}