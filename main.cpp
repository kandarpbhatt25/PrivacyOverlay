#include <QApplication> // Changed from QCoreApplication to QApplication for UI
#include <QDebug>
#include "ui/MainWindow.h"
#include "core/CaptureDetectionEngine.h"
#include "platform/CaptureMonitor.h"

int main(int argc, char* argv[]) {
    // QApplication is required for Qt Widgets
    QApplication app(argc, argv);

    qDebug() << "Starting Privacy Shield UI...";

    // 1. Start your background Kernel listener
    CaptureMonitor::StartETWListener();

    // 2. Initialize and show the Main UI Window
    MainWindow window;
    window.show();

    // 3. Keep your engine running in the background
    CaptureDetectionEngine engine;
    QObject::connect(&engine, &CaptureDetectionEngine::captureStarted, []() {
        qDebug() << "🔴 [ALERT] Capture STARTED! Masking should engage now.";
        });
    QObject::connect(&engine, &CaptureDetectionEngine::captureStopped, []() {
        qDebug() << "🟢 [ALERT] Capture STOPPED! Masking can be removed.";
        });
    engine.startMonitoring(1000);

    // 4. Start the application event loop
    int result = app.exec();

    // Clean up on exit
    CaptureMonitor::StopETWListener();

    return result;
}