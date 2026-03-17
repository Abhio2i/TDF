/* ========================================================================= */
/* File: runtimetooolbar.h                                                  */
/* Purpose: Defines toolbar for runtime control and monitoring               */
//               Written by Arti Rajpoot
/* ========================================================================= */

#ifndef RUNTIMETOOLBAR_H
#define RUNTIMETOOLBAR_H

#include <QToolBar>                               // For toolbar base class
#include <QAction>                                // For action items
#include <QSlider>                                // For slider widget
#include <QLabel>                                 // For label widget
#include "GUI/Logger/loggerdialog.h"              // For logger dialog
#include "GUI/Timing/graphwidget.h"

// %%% Class Definition %%%
/* Toolbar for runtime operations */
class RuntimeToolBar : public QToolBar
{
    Q_OBJECT

public:
    // Initialize toolbar
    explicit RuntimeToolBar(QWidget *parent = nullptr);
    void Init();
    enum SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };
signals:
    // Signal start action triggered
    void timingGraphClicked();
    void timingGraphRequested();
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
    void timeChanged(float newTimeInSeconds);
    void simulationStateChanged(SimulationState state);
    void resetTriggered();

public:
    // Update elapsed time
    void onElapsedTime(float time);
    void highlightAction(QAction *action);
    QAction *startAction;
    // Pause action
    QAction *pauseAction;
    // Stop action
    QAction *stopAction;
      void setSimulationState(SimulationState state);
    void storeSnapshot(const QJsonObject& hierarchySnapshot);
       QJsonObject m_initialSnapshot;
      QJsonObject getSnapshot() const { return m_initialSnapshot; }
    // Next step action
private slots:
    void timingActionTriggered() {
        emit timingGraphClicked();
    }
    void onTimeLabelClicked();
     void updateSimulationStatus();
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    // %%% UI Components %%%
    // Start action
    GraphWidget *graphWidget = nullptr;
    QAction *nextStepAction;
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
        QTimer *blinkTimer;
    // Elapsed time in seconds
    float elapsedSeconds;
    // Timing dialog instance
    QAction *resetAction;


    // %%% Utility Methods %%%
    // Create pixmap with white background
    QPixmap withWhiteBg(const QString &iconPath);
    // Create toolbar actions
    void createActions();
    // Setup toolbar
    void setupToolBar();
    // Update time display
    void updateTimeDisplay();
        QLabel *simulationStatusLabel;
    SimulationState currentState;
    bool blinkState;
    void updateStatusDisplay();
};

#endif // RUNTIMETOOLBAR_H
