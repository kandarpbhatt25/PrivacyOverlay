#pragma once
#include <QObject>
#include <QThread>
#include <QList>
#include "../models/CachedApp.h"

namespace platform {
    class AppDiscoveryWorker : public QObject {
        Q_OBJECT
    public:
        AppDiscoveryWorker() {}
    public slots:
        void doWork();
    private:
        QList<models::CachedApp> scanRegistry();
        QList<models::CachedApp> scanLocalAppData();
        QList<models::CachedApp> scanUWP();
        QList<models::CachedApp> scanStartMenu();
    signals:
        void appsDiscovered(QList<models::CachedApp> newApps);
        void finished();
    };

    class AppDiscoveryEngine : public QObject {
        Q_OBJECT
    public:
        explicit AppDiscoveryEngine(QObject* parent = nullptr);
        ~AppDiscoveryEngine();
        void startDiscovery();
    signals:
        void newAppsFound(QList<models::CachedApp> apps);
    private:
        QThread* workerThread;
        AppDiscoveryWorker* worker;
    };
}
