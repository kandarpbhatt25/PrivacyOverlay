#include "CaptureDetectionEngine.h"
#include "../platform/CaptureMonitor.h"
#include <QDebug>

CaptureDetectionEngine::CaptureDetectionEngine(QObject *parent)
    : QObject(parent), m_isCaptureActive(false) {
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CaptureDetectionEngine::checkCaptureState);
}

CaptureDetectionEngine::~CaptureDetectionEngine() {
    stopMonitoring();
}

void CaptureDetectionEngine::startMonitoring(int intervalMs) {
    if (m_timer->isActive()) {
        qDebug() << "Monitoring is already active.";
        return;
    }

    if (intervalMs <= 0) {
        emit monitoringError("Invalid interval specified. Interval must be > 0.");
        return;
    }

    m_timer->start(intervalMs);
    
    if (!m_timer->isActive()) {
        emit monitoringError("Failed to start the capture monitoring timer.");
    } else {
        qDebug() << "Capture monitoring started. Polling every" << intervalMs << "ms.";
        // Run an immediate check so we don't wait for the first tick
        checkCaptureState(); 
    }
}

void CaptureDetectionEngine::stopMonitoring() {
    if (m_timer->isActive()) {
        m_timer->stop();
        qDebug() << "Capture monitoring stopped.";
    }
}

bool CaptureDetectionEngine::isCurrentlyCaptured() const {
    return m_isCaptureActive;
}

void CaptureDetectionEngine::checkCaptureState() {
    
    // ADD THIS HEARTBEAT LINE
    qDebug() << "[Heartbeat] Timer ticked. Querying OS...";
    // 1. Query the OS
    bool currentSearch = CaptureMonitor::IsScreenCaptureActive();

    // 2. State Transition Logic
    if (!m_isCaptureActive && currentSearch) {
        m_isCaptureActive = true;
        qDebug() << "Capture Started!";
        emit captureStarted();
    } 
    else if (m_isCaptureActive && !currentSearch) {
        m_isCaptureActive = false;
        qDebug() << "Capture Stopped!";
        emit captureStopped();
    }
}