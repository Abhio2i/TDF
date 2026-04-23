//============================================================================
// FILE:         StatusBar.h
// MODULE:       Custom Status Bar
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the StatusBar class which provides a customised
//               QStatusBar containing only a Save button and a file name
//               display. Legacy methods for status messages, coordinates,
//               simulation time, and zoom level are kept for compatibility
//               but perform no action. Includes a RAM usage monitor with
//               a timer for periodic updates.
//
// REQUIREMENTS: REQ-STATUSBAR-010  Custom status bar with Save button
//               REQ-STATUSBAR-011  Display current file name with unsaved
//                                  change indicator
//               REQ-STATUSBAR-012  Clear file name display
//               REQ-STATUSBAR-013  Signal saveRequested on Save button click
//               REQ-STATUSBAR-014  RAM usage label with periodic update timer
//               REQ-STATUSBAR-015  Legacy compatibility methods (no-op)
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-STATUSBAR-001
//
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
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
