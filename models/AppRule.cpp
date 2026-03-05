#include "AppRule.h"

namespace models {

AppRule::AppRule() 
    : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), 
      matchType(MatchType::ProcessName), 
      maskMode(MaskMode::Affinity) {}

QJsonObject AppRule::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["matchType"] = (matchType == MatchType::ProcessName) ? "ProcessName" : "WindowTitle";
    json["matchPattern"] = matchPattern;
    json["maskMode"] = (maskMode == MaskMode::Affinity) ? "Affinity" : "BlackMask";
    return json;
}

AppRule AppRule::fromJson(const QJsonObject& json) {
    AppRule rule;
    rule.id = json["id"].toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    QString typeStr = json["matchType"].toString("ProcessName");
    rule.matchType = (typeStr == "WindowTitle") ? MatchType::WindowTitle : MatchType::ProcessName;
    
    rule.matchPattern = json["matchPattern"].toString("");
    
    QString maskStr = json["maskMode"].toString("Affinity"); // Safe default from blueprint
    rule.maskMode = (maskStr == "BlackMask") ? MaskMode::BlackMask : MaskMode::Affinity;
    
    return rule;
}

} // namespace models