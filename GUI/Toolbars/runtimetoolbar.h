/* ========================================================================= */
/* File: runtimetooolbar.h                                                  */
/* Purpose: Defines toolbar for runtime control and monitoring               */
/* ========================================================================= */

#ifndef RUNTIMETOOLBAR_H
#define RUNTIMETOOLBAR_H

#include <QToolBar>                               // For toolbar base class
#include <QAction>                                // For action items
#include <QSlider>                                // For slider widget
#include <QLabel>                                 // For label widget
#include "GUI/Timing/timingdialog.h"              // For timing dialog
#include "GUI/Logger/loggerdialog.h"              // For logger dialog

// %%% Class Definition %%%
/* Toolbar for runtime operations */
class RuntimeToolBar : public QToolBar
{
    Q_OBJECT

public:
    // Initialize toolbar
    explicit RuntimeToolBar(QWidget *parent = nullptr);

signals:
    // Signal start action triggered
    void startTriggered();
    // Signal pause action triggered
    void pauseTriggered();
    // Signal stop action triggered
    void stopTriggered();
    // Signal next step triggered
    void nextStepTriggered();
    // Signal speed change
    void speedChanged(int speed);
    // Signal replay action triggered
    void replayTriggered();
    // Signal bookmark triggered (commented)
    // void bookmarkTriggered();
    // Signal bookmark comment submitted (commented)
    // void bookmarkCommentSubmitted(QString comment);
    // Signal logger toggle
    void loggerTriggered(bool checked);
    // Signal start recording
    void startRecording();
    // Signal stop recording
    void stopRecording();
    // Signal replay recording
    void replayRecording(const QString &filePath);
    // Signal event types selected
    void eventTypesSelected(QStringList eventTypes);
    // Signal radar display toggle
    void radarDisplayToggled();

public:
    // Update elapsed time
    void onElapsedTime(float time);

private:
    // %%% UI Components %%%
    // Start action
    QAction *startAction;
    // Pause action
    QAction *pauseAction;
    // Stop action
    QAction *stopAction;
    // Next step action
    QAction *nextStepAction;
    // Bookmark action (commented)
    // QAction *bookmarkAction;
    // Replay action
    QAction *replayAction;
    // Timing action
    QAction *timingAction;
    // Logger action
    QAction *loggerAction;
    // Radar toggle action
    QAction *radarToggleAction;
    // Speed slider
    QSlider *speedSlider;
    // Time display label
    QLabel *timeLabel;
    // Timer for updates
    QTimer *timer;
    // Elapsed time in seconds
    float elapsedSeconds;
    // Timing dialog instance
    TimingDialog *timingDialog = nullptr;

    // %%% Utility Methods %%%
    // Create pixmap with white background
    QPixmap withWhiteBg(const QString &iconPath);
    // Create toolbar actions
    void createActions();
    // Setup toolbar
    void setupToolBar();
    // Highlight action
    void highlightAction(QAction *action);
    // Update time display
    void updateTimeDisplay();
};

#endif // RUNTIMETOOLBAR_H
