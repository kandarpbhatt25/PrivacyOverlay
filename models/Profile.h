#pragma once
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include "AppRule.h"

namespace models {

struct Profile {
    QString id;
    QString name;
    QList<AppRule> rules;
    bool isActive;

    Profile();

    QJsonObject toJson() const;
    static Profile fromJson(const QJsonObject& json);
};

} // namespace models