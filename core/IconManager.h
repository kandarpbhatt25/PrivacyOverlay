#pragma once
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QMutex>

namespace core {

class IconManager {
public:
    static IconManager& getInstance() {
        static IconManager instance;
        return instance;
    }

    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    QPixmap getIcon(const QString& fullExePath, const QString& fallbackTitle = "");

private:
    IconManager() = default;
    ~IconManager() = default;

    QMap<QString, QPixmap> m_cache;
    QMutex m_mutex;

    QPixmap generateFallbackBadge(const QString& title);
};

} // namespace core
