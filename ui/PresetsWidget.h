#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include "persistence/SettingsManager.h"

class PresetsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PresetsWidget(QWidget* parent = nullptr);

signals:
    void profileActivated(); // Emitted when an active profile changes so the MainWindow can react

private slots:
    void onAddProfile();
    void onDeleteProfile(const QString& id);
    void onActivateProfile(const QString& id);
    void onEditProfile(const QString& id);

private:
    void renderProfiles();
    
    QGridLayout* m_gridLayout;
    QVBoxLayout* m_mainLayout;
};