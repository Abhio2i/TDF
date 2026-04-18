//============================================================================
// File        : StatusBar.cpp
// Description : Custom status bar with only Save button
// Written by: Arti Rajpoot
//============================================================================
#include "statusbar.h"
#include <QSizeGrip>
#include <QFileInfo>
#include <QTimer>
#ifdef Q_OS_LINUX
#include <sys/resource.h>
#include <unistd.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

// ────────────────────────────────────────────────────────────────────────────
//  Constructor
// ────────────────────────────────────────────────────────────────────────────
StatusBar::StatusBar(QWidget *parent)
    : QStatusBar(parent)
{
    setSizeGripEnabled(false);
    buildUI();
    applyStyles();
    // runUnitTestsOnce();

}

void StatusBar::buildUI()
{
    // ── File name label ───────────────────────────────────────
    m_fileNameLabel = new QLabel(this);
    m_fileNameLabel->setObjectName("statusFileNameLabel");
    m_fileNameLabel->setStyleSheet(
        "QLabel#statusFileNameLabel {"
        "  color: #8BAFC7;"
        "  font-size: 12px;"
        "  padding-left: 5px;"
        "}"
        );
    addWidget(m_fileNameLabel);

    // ── RAM usage label──────────────────
    m_ramLabel = new QLabel("RAM: -- MB", this);
    m_ramLabel->setObjectName("statusRamLabel");
    m_ramLabel->setFixedWidth(110);
    m_ramLabel->setAlignment(Qt::AlignCenter);
    m_ramLabel->setStyleSheet(
        "QLabel#statusRamLabel {"
        "  color: #00BFFF;"
        "  font-size: 12px;"
        "  padding: 0 6px;"
        "  border-left: 1px solid #1A3A4F;"
        "}"
        );
    addWidget(m_ramLabel);

    // ── Save button (far right) ───────────────────────────────────────────
    m_saveBtn = new QPushButton("Save", this);
    m_saveBtn->setObjectName("statusSaveBtn");
    m_saveBtn->setFixedSize(55, 20);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setToolTip("Save (Ctrl+S)");
    addPermanentWidget(m_saveBtn);

    connect(m_saveBtn, &QPushButton::clicked, this, &StatusBar::saveRequested);

    // ── RAM update timer (500ms) ──────────────────────────────────────────
    m_ramTimer = new QTimer(this);
    m_ramTimer->setInterval(500);
    connect(m_ramTimer, &QTimer::timeout, this, &StatusBar::updateRamUsage);
    m_ramTimer->start();
    updateRamUsage();
}

void StatusBar::setFileName(const QString& fileName, bool hasUnsavedChanges)
{
    if (!m_fileNameLabel) return;

    if (fileName.isEmpty()) {
        m_fileNameLabel->setText("");
    } else {
        QFileInfo fileInfo(fileName);
        QString displayText = fileInfo.fileName();
        if (hasUnsavedChanges) {
            displayText = "* " + displayText;
        }
        m_fileNameLabel->setText(displayText);
    }
}

void StatusBar::clearFileName()
{
    if (m_fileNameLabel) {
        m_fileNameLabel->clear();
    }
}
// ────────────────────────────────────────────────────────────────────────────
//  applyStyles
// ────────────────────────────────────────────────────────────────────────────
void StatusBar::applyStyles()
{
    // ── Status bar background ──────────────────────────────────────────────
    setStyleSheet(
        "QStatusBar {"
        "  background-color: #0A1E2E;"
        "  border-top: 1px solid #1A3A4F;"
        "  color: #8BAFC7;"
        "  min-height: 28px;"
        "  max-height: 28px;"
        "}"
        "QStatusBar::item { border: none; }"
        );

    // ── Save button ────────────────────────────────────────────────────────
    m_saveBtn->setStyleSheet(
        "QPushButton#statusSaveBtn {"
        "  background-color: #1A3652;"
        "  color: white;"
        "  border: 1px solid #27446d;"
        "  border-radius: 2px;"
        "  font-size: 12px;"
        "  padding: 1px 1px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#statusSaveBtn:hover  { background-color: #27446d; }"
        "QPushButton#statusSaveBtn:pressed { background-color: #0F2636; }"
        );
}

// ────────────────────────────────────────────────────────────────────────────
//  Public API - Keep empty implementations for compatibility
// ────────────────────────────────────────────────────────────────────────────

void StatusBar::setStatusMessage(const QString& message, int msec)
{
    Q_UNUSED(message);
    Q_UNUSED(msec);
    // Do nothing - functionality removed
}

void StatusBar::setCoordinates(double lat, double lon)
{
    Q_UNUSED(lat);
    Q_UNUSED(lon);
    // Do nothing - functionality removed
}

void StatusBar::setCoordinatesVisible(bool visible)
{
    Q_UNUSED(visible);
    // Do nothing - functionality removed
}

void StatusBar::setSimTime(const QString& timeStr)
{
    Q_UNUSED(timeStr);
    // Do nothing - functionality removed
}

void StatusBar::setSimTimeVisible(bool visible)
{
    Q_UNUSED(visible);
    // Do nothing - functionality removed
}

void StatusBar::setZoomLevel(int zoom)
{
    Q_UNUSED(zoom);
    // Do nothing - functionality removed
}

void StatusBar::setZoomVisible(bool visible)
{
    Q_UNUSED(visible);
    // Do nothing - functionality removed
}
void StatusBar::updateRamUsage()
{
    long ramKB = 0;

#ifdef Q_OS_LINUX
    QFile statm("/proc/self/statm");
    if (statm.open(QIODevice::ReadOnly)) {
        QByteArray data = statm.readAll();
        statm.close();


        QList<QByteArray> parts = data.simplified().split(' ');
        if (parts.size() >= 2) {
            long pages    = parts[1].toLong();
            long pageSize = sysconf(_SC_PAGESIZE);
            ramKB = (pages * pageSize) / 1024L;
        }
    }

#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        ramKB = static_cast<long>(pmc.WorkingSetSize / 1024);
#endif

    if (ramKB > 0) {
        double ramMB = ramKB / 1024.0;
        if (ramMB >= 1024.0) {
            m_ramLabel->setText(QString("RAM: %1 GB")
                                    .arg(ramMB / 1024.0, 0, 'f', 2));
        } else {
            m_ramLabel->setText(QString("RAM: %1 MB")
                                    .arg(ramMB, 0, 'f', 1));
        }
    } else {
        m_ramLabel->setText("RAM: N/A");
    }
}

