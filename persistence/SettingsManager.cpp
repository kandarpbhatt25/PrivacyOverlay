#include "SettingsManager.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDebug> // Temporary logger

namespace persistence {

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

QList<models::Profile> SettingsManager::loadAllProfiles() {
    QSettings settings("PrivacyOverlay", "App");
    QString jsonString = settings.value(PROFILES_KEY, "").toString();

    if (jsonString.isEmpty()) {
        return {}; // First launch or empty
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[ERROR] Corrupted JSON in settings. Backing up and resetting.";
        settings.setValue(PROFILES_KEY + "_corrupted", jsonString);
        settings.remove(PROFILES_KEY);
        return {};
    }

    QList<models::Profile> profiles;
    QJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        profiles.append(models::Profile::fromJson(arr[i].toObject()));
    }

    return profiles;
}

void SettingsManager::saveProfile(const models::Profile& profile) {
    QList<models::Profile> allProfiles = loadAllProfiles();
    
    // Update existing or append new
    bool found = false;
    for (int i = 0; i < allProfiles.size(); ++i) {
        if (allProfiles[i].id == profile.id) {
            allProfiles[i] = profile;
            found = true;
            break;
        }
    }
    if (!found) {
        allProfiles.append(profile);
    }

    // Serialize back to JSON array
    QJsonArray arr;
    for (const auto& p : allProfiles) {
        arr.append(p.toJson());
    }

    QJsonDocument doc(arr);
    QSettings settings("PrivacyOverlay", "App");
    settings.setValue(PROFILES_KEY, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

models::Profile SettingsManager::getActiveProfile() {
    QList<models::Profile> profiles = loadAllProfiles();
    for (const auto& p : profiles) {
        if (p.isActive) return p;
    }
    
    // Return a default if none active
    models::Profile defaultProfile;
    defaultProfile.name = "Default Profile";
    defaultProfile.isActive = true;
    return defaultProfile;
}

void SettingsManager::setActiveProfile(const QString& profileId) {
    QList<models::Profile> allProfiles = loadAllProfiles();
    for (auto& p : allProfiles) {
        p.isActive = (p.id == profileId);
    }
    
    // Re-save entire list
    QJsonArray arr;
    for (const auto& p : allProfiles) {
        arr.append(p.toJson());
    }
    QSettings settings("PrivacyOverlay", "App");
    settings.setValue(PROFILES_KEY, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void SettingsManager::deleteProfile(const QString& profileId) {
    QList<models::Profile> allProfiles = loadAllProfiles();
    allProfiles.erase(std::remove_if(allProfiles.begin(), allProfiles.end(), 
        [&](const models::Profile& p) { return p.id == profileId; }), allProfiles.end());
    
    QJsonArray arr;
    for (const auto& p : allProfiles) {
        arr.append(p.toJson());
    }
    QSettings settings("PrivacyOverlay", "App");
    settings.setValue(PROFILES_KEY, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

} // namespace persistence