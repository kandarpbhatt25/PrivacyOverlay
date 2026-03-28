#ifndef CAPTUREDETECTIONENGINE_H
#define CAPTUREDETECTIONENGINE_H

#include <QObject>
#include <QTimer>
#include <QString>

class CaptureDetectionEngine : public QObject {
    Q_OBJECT

public:
    explicit CaptureDetectionEngine(QObject *parent = nullptr);
    ~CaptureDetectionEngine();

    void startMonitoring(int intervalMs = 1000);
    void stopMonitoring();
    bool isCurrentlyCaptured() const;

signals:
    void captureStarted();
    void captureStopped();
    void monitoringError(const QString &message);

private slots:
    void checkCaptureState();

private:
    QTimer *m_timer;
    bool m_isCaptureActive; // State tracking to prevent duplicate signals
};

#endif // CAPTUREDETECTIONENGINE_H