//============================================================================
// File        : StatusBar.h
// Description : Custom status bar with only Save button
/* Written by   : Arti Rajpoot                                               */
//============================================================================
#pragma once

#include <QStatusBar>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>

class StatusBar : public QStatusBar
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar() = default;

    /** Kept for compatibility - does nothing */
    void setStatusMessage(const QString& message, int msec = 4000);

    /** Kept for compatibility - does nothing */
    void setCoordinates(double lat, double lon);

    /** Kept for compatibility - does nothing */
    void setCoordinatesVisible(bool visible);

    /** Kept for compatibility - does nothing */
    void setSimTime(const QString& timeStr);

    /** Kept for compatibility - does nothing */
    void setSimTimeVisible(bool visible);

    /** Kept for compatibility - does nothing */
    void setZoomLevel(int zoom);

    /** Kept for compatibility - does nothing */
    void setZoomVisible(bool visible);

    /** Get Save button */
    QPushButton* saveButton() const { return m_saveBtn; }
    void setFileName(const QString& fileName, bool hasUnsavedChanges = false);
    void clearFileName();
    // static void runUnitTestsOnce();


signals:
    void saveRequested();
private slots:
    void updateRamUsage();
private:
    void buildUI();
    void applyStyles();
    QPushButton *m_saveBtn = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    QLabel      *m_ramLabel     = nullptr;
    QTimer      *m_ramTimer     = nullptr;
};
