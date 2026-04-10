/* ========================================================================= */
/* File: profileinfodialog.h                                                */
/* Purpose: Dialog for displaying application performance and profile information */
// Written by   : Arti Rajpoot
/* ========================================================================= */

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
    static void runUnitTestsOnce();

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
