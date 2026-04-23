/* =============================================================================
 * FILE:         profileinfodialog.h
 * MODULE:       Profile Information Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ProfileInfoDialog class which provides a modal
 *               dialog for displaying real‑time application performance metrics
 *               and profile information. It shows frames per second (FPS), CPU
 *               usage, memory usage, and other runtime statistics. The dialog
 *               updates periodically via a timer and allows copying the
 *               information to the clipboard.
 *
 * REQUIREMENTS: REQ-PROFILE-010  Profile information dialog
 *               REQ-PROFILE-011  Display real‑time performance metrics
 *               REQ-PROFILE-012  Update metrics periodically via timer
 *               REQ-PROFILE-013  Copy profile data to clipboard
 *               REQ-PROFILE-014  Static method to show dialog conveniently
 *               REQ-PROFILE-015  Proper cleanup on dialog close
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-PROFILE-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef PROFILEINFODIALOG_H
#define PROFILEINFODIALOG_H

#include "qelapsedtimer.h"      // For high-resolution timing measurements
#include <QDialog>              // For dialog base class
#include <QTimer>               // For periodic updates
#include <QTextEdit>            // For multi-line text display
#include <QLabel>               // For label display
#include <QPushButton>          // For button controls

// %%% Class Definition %%%
/* Dialog window showing real-time application performance metrics and profile data */
class ProfileInfoDialog : public QDialog
{
    Q_OBJECT

public:
    // %%% Constructor %%%
    /* Initialize profile information dialog */
    explicit ProfileInfoDialog(QWidget *parent = nullptr);

    // %%% Destructor %%%
    /* Clean up dialog resources */
    ~ProfileInfoDialog();

    // %%% Static Display Method %%%
    /* Show profile information dialog (convenience method) */
    static void showProfileInfo(QWidget *parent = nullptr);
    // static void runUnitTestsOnce();

protected:
    // %%% Event Handler %%%
    /* Handle dialog close events */
    void closeEvent(QCloseEvent *event) override;

private slots:
    // %%% Update Slots %%%
    /* Update real-time performance information */
    void updateRealTimeInfo();

    // %%% Action Slots %%%
    /* Copy profile information to clipboard */
    void copyToClipboard();

private:
    // %%% UI Setup Methods %%%
    /* Set up user interface components */
    void setupUI();

    // %%% Data Collection Methods %%%
    /* Collect and format performance metrics */
    QString getPerformanceMetrics();

    // %%% UI Component Members %%%
    QTextEdit* textEdit;        // Main text display for profile info
    QLabel* titleLabel;         // Dialog title label
    QPushButton* copyButton;    // Button to copy to clipboard
    QPushButton* closeButton;   // Button to close dialog

    // %%% Performance Measurement Members %%%
    QTimer* refreshTimer;       // Timer for periodic updates
    int frameCount;             // Frame counter for FPS calculation
    int fps;                    // Current frames per second
    QElapsedTimer fpsTimer;     // Timer for FPS measurement
};

#endif // PROFILEINFODIALOG_H
