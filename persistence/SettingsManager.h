#pragma once
#include <QList>
#include <QString>
#include "models/Profile.h"

namespace persistence {

class SettingsManager {
public:
    static SettingsManager& getInstance();

    void saveProfile(const models::Profile& profile);
    QList<models::Profile> loadAllProfiles();
    models::Profile getActiveProfile();
    void setActiveProfile(const QString& profileId);
    void deleteProfile(const QString& profileId);

private:
    SettingsManager() = default;
    ~SettingsManager() = default;
    
    // Delete copy/move constructors for Singleton
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    const QString PROFILES_KEY = "profiles_json";
};

} // namespace persistence