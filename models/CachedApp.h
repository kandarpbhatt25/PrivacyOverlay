#pragma once
#include <QString>
#include <QJsonObject>

namespace models {
    struct CachedApp {
        QString displayName;
        QString displayIcon;
        QString executableName;
        bool isUWP = false;
        bool inferred = false;

        QJsonObject toJson() const {
            QJsonObject obj;
            obj["displayName"] = displayName;
            obj["displayIcon"] = displayIcon;
            obj["executableName"] = executableName;
            obj["isUWP"] = isUWP;
            obj["inferred"] = inferred;
            return obj;
        }

        static CachedApp fromJson(const QJsonObject& obj) {
            CachedApp app;
            app.displayName = obj["displayName"].toString();
            app.displayIcon = obj["displayIcon"].toString();
            app.executableName = obj["executableName"].toString();
            app.isUWP = obj["isUWP"].toBool(false);
            app.inferred = obj["inferred"].toBool(false);
            return app;
        }
    };
}
