#include "AppCacheManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

namespace persistence {

AppCacheManager& AppCacheManager::getInstance() {
    static AppCacheManager instance;
    return instance;
}

QString AppCacheManager::getCachePath() const {
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("app_cache.json");
}

QList<models::CachedApp> AppCacheManager::loadCache() {
    QList<models::CachedApp> apps;
    QFile file(getCachePath());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return apps; // Return empty list if no cache exists
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (val.isObject()) {
                apps.append(models::CachedApp::fromJson(val.toObject()));
            }
        }
    }
    return apps;
}

void AppCacheManager::saveCache(const QList<models::CachedApp>& apps) {
    QJsonArray arr;
    for (const auto& app : apps) {
        arr.append(app.toJson());
    }

    QJsonDocument doc(arr);
    QFile file(getCachePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
        file.close();
    } else {
        qWarning() << "[AppCacheManager] Failed to write cache file:" << getCachePath();
    }
}

}
