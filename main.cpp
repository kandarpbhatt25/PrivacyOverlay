#include <QCoreApplication>
#include <QDebug>
#include "core/CaptureDetectionEngine.h"
#include "platform/CaptureMonitor.h" // Added to access the ETW lifecycle functions

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qDebug() << "Starting Privacy Shield Test...";

    // START THE KERNEL LISTENER
    // Note: This will print an error and fail safely if not run as Administrator
    CaptureMonitor::StartETWListener();

    CaptureDetectionEngine engine;

    // Connect the signals to simple lambda functions for testing
    QObject::connect(&engine, &CaptureDetectionEngine::captureStarted, []() {
        qDebug() << "🔴 [ALERT] Capture STARTED! Masking should engage now.";
    });

    QObject::connect(&engine, &CaptureDetectionEngine::captureStopped, []() {
        qDebug() << "🟢 [ALERT] Capture STOPPED! Masking can be removed.";
    });

    QObject::connect(&engine, &CaptureDetectionEngine::monitoringError, [](const QString &msg) {
        qDebug() << "🟡 [ERROR]" << msg;
    });

    // Start polling every 1 second (1000 ms)
    engine.startMonitoring(1000);

    // Capture the event loop execution result
    int result = app.exec();

    // STOP THE KERNEL LISTENER ON EXIT
    // This is crucial to prevent orphaned ETW trace sessions in Windows
    CaptureMonitor::StopETWListener();

    return result;
}