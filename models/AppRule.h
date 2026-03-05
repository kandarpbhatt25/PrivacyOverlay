#pragma once
#include <QString>
#include <QJsonObject>
#include <QUuid>

namespace models {

enum class MatchType { ProcessName, WindowTitle };
enum class MaskMode { Affinity, BlackMask };

struct AppRule {
    QString id;
    MatchType matchType;
    QString matchPattern;
    MaskMode maskMode;

    AppRule(); // Default constructor

    QJsonObject toJson() const;
    static AppRule fromJson(const QJsonObject& json);
};

} // namespace modelss