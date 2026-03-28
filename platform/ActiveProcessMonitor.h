#pragma once
#include <QObject>
#include <QTimer>
#include <QSet>
#include "../models/CachedApp.h"

namespace platform {
    class ActiveProcessMonitor : public QObject {
        Q_OBJECT
    public:
        explicit ActiveProcessMonitor(QObject* parent = nullptr);
        ~ActiveProcessMonitor();

        void startMonitoring(int intervalMs = 60000); // Default 1 minute
        void stopMonitoring();

    signals:
        void newInferredAppsDiscovered(QList<models::CachedApp> inferredApps);

    private slots:
        void pollActiveProcesses();

    private:
        QTimer* m_timer;
        QSet<QString> m_notifiedExes; // Track which ones we already promoted

        bool extractMetadata(const QString& exePath, models::CachedApp& appOut);
        int calculateConfidence(const QString& exePath, const models::CachedApp& app);
    };
}
