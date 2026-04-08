#include "IconManager.h"
#include <QFileIconProvider>
#include <QFileInfo>
#include <QPainter>
#include <QDir>
#include <QCoreApplication>
#include <windows.h>
#include <shellapi.h>

namespace core {

QPixmap IconManager::getIcon(const QString& fullExePath, const QString& fallbackTitle) {
    QMutexLocker locker(&m_mutex);

    QString key = fullExePath.isEmpty() ? fallbackTitle : fullExePath.toLower();
    
    if (m_cache.contains(key)) {
        return m_cache[key];
    }

    QPixmap iconPixmap;
    bool iconLoaded = false;

    // 1. Query OS for Native Icon
    if (!fullExePath.isEmpty() && fullExePath.endsWith(".exe", Qt::CaseInsensitive)) {
        UINT iconCount = ExtractIconExW((LPCWSTR)fullExePath.utf16(), -1, nullptr, nullptr, 0);
        if (iconCount > 0) {
            QFileInfo fileInfo(fullExePath);
            QFileIconProvider iconProvider;
            QIcon icon = iconProvider.icon(fileInfo);
            
            if (!icon.isNull()) {
                iconPixmap = icon.pixmap(32, 32);
                if (!iconPixmap.isNull()) iconLoaded = true;
            }
        }
    }

    // 2. Custom icons folder fallback
    if (!iconLoaded && !fullExePath.isEmpty()) {
        QString exeName = QFileInfo(fullExePath).fileName();
        QDir appDir(QCoreApplication::applicationDirPath());
        QString customIconPath = appDir.absoluteFilePath("icons/" + QString(exeName).replace(".exe", ".png", Qt::CaseInsensitive));
        if (QFile::exists(customIconPath)) {
            iconPixmap = QPixmap(customIconPath).scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            if (!iconPixmap.isNull()) iconLoaded = true;
        }
    }

    // 3. Complete fallback: Letter badge
    if (!iconLoaded) {
        iconPixmap = generateFallbackBadge(fallbackTitle.isEmpty() ? QFileInfo(fullExePath).completeBaseName() : fallbackTitle);
    }

    m_cache[key] = iconPixmap;
    return iconPixmap;
}

QPixmap IconManager::generateFallbackBadge(const QString& title) {
    QPixmap iconPixmap(40, 40);
    iconPixmap.fill(Qt::transparent);
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#3b82f6")); 
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 40, 40);
    
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setPixelSize(18);
    f.setBold(true);
    painter.setFont(f);
    
    QString letter = title.isEmpty() ? "?" : title.left(1).toUpper();
    painter.drawText(iconPixmap.rect(), Qt::AlignCenter, letter);
    
    return iconPixmap;
}

} // namespace core
