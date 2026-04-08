#pragma once
#include <QString>
#include <QJsonObject>

namespace models {
    enum class AppSource { Registry, StartMenu, LocalAppData, UWP, ActiveProcess };

    struct CachedApp {
        QString displayName;
        QString displayIcon;
        QString executableName;
        QString executablePath;
        AppSource source = AppSource::Registry;
        bool inferred = false;

        QJsonObject toJson() const {
            QJsonObject obj;
            obj["displayName"] = displayName;
            obj["displayIcon"] = displayIcon;
            obj["executableName"] = executableName;
            obj["executablePath"] = executablePath;
            obj["source"] = static_cast<int>(source);
            obj["inferred"] = inferred;
            return obj;
        }

        static CachedApp fromJson(const QJsonObject& obj) {
            CachedApp app;
            app.displayName = obj["displayName"].toString();
            app.displayIcon = obj["displayIcon"].toString();
            app.executableName = obj["executableName"].toString();
            app.executablePath = obj["executablePath"].toString();
            app.source = static_cast<AppSource>(obj["source"].toInt(static_cast<int>(AppSource::Registry)));
            app.inferred = obj["inferred"].toBool(false);
            return app;
        }
    };
}
