#pragma once
#include <QString>

#include <QPixmap>

namespace models {

struct AppState {
    QString exeName;
    QString fullExePath;
    QString displayName;
    QString displayIcon;
    QPixmap cachedNativeIcon;
    bool isActive = false;
    bool isHidden = false; // Is the privacy toggle ON for this app?

    // Derived state for the UI
    QString getStatusLabel() const {
        if (!isActive) return "Idle";
        return isHidden ? "MASKED" : "VISIBLE";
    }
};

}
