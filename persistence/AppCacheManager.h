#pragma once
#include "../models/CachedApp.h"
#include <QList>
#include <QString>

namespace persistence {
    class AppCacheManager {
    public:
        static AppCacheManager& getInstance();
        QList<models::CachedApp> loadCache();
        void saveCache(const QList<models::CachedApp>& apps);

    private:
        AppCacheManager() = default;
        ~AppCacheManager() = default;
        AppCacheManager(const AppCacheManager&) = delete;
        AppCacheManager& operator=(const AppCacheManager&) = delete;

        QString getCachePath() const;
    };
}
