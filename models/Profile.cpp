#include "Profile.h"

namespace models {

Profile::Profile() 
    : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), 
      name("New Profile"), 
      isActive(false) {}

QJsonObject Profile::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["isActive"] = isActive;

    QJsonArray rulesArray;
    for (const auto& rule : rules) {
        if (!rule.matchPattern.trimmed().isEmpty()) { // Validation Rule
            rulesArray.append(rule.toJson());
        }
    }
    json["rules"] = rulesArray;

    return json;
}

Profile Profile::fromJson(const QJsonObject& json) {
    Profile profile;
    profile.id = json["id"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    profile.name = json["name"].toString("Unnamed Profile");
    profile.isActive = json["isActive"].toBool(false);

    QJsonArray rulesArray = json["rules"].toArray();
    for (int i = 0; i < rulesArray.size(); ++i) {
        profile.rules.append(AppRule::fromJson(rulesArray[i].toObject()));
    }

    return profile;
}

} // namespace models