/* =============================================================================
 * FILE:         runtimetoolbar.h
 * MODULE:       Runtime Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Defines the RuntimeToolBar class which provides a toolbar for
 *               runtime control and monitoring. Includes actions for start,
 *               pause, stop, next step, reset, timing graph, logger, radar
 *               toggle, and a speed slider with time label. Supports simulation
 *               state (STOPPED, RUNNING, PAUSED), snapshot storage, event
 *               filtering, and emits signals for simulation control and
 *               timing/logging.
 *
 * REQUIREMENTS: REQ-RUNTIMETOOLBAR-010 through REQ-RUNTIMETOOLBAR-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RUNTIMETOOLBAR-001
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef RUNTIMETOOLBAR_H
#define RUNTIMETOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QSlider>
#include <QLabel>
#include <QJsonObject>
#include <QToolButton>
#include <QMap>
#include <QList>
#include <QMenu>

class GraphWidget;
class LoggerDialog;

class RuntimeToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit RuntimeToolBar(QWidget *parent = nullptr);
    void Init();

    enum SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };

    // ── Public accessors ─────────────────────────────────────────────────
    QAction* getResetAction() const { return resetAction; }
    QAction* getNextStepAction() const { return nextStepAction; }
    QAction* getTimingAction() const { return timingAction; }
    QAction* getLoggerAction() const { return loggerAction; }
    QAction* getRadarToggleAction() const { return radarToggleAction; }
    QSlider* getSpeedSlider() const { return speedSlider; }
    QLabel* getTimeLabel() const { return timeLabel; }
    SimulationState getSimulationState() const { return currentState; }
    QJsonObject getSnapshot() const { return m_initialSnapshot; }

    // ── Public methods ────────────────────────────────────────────────────
    void onElapsedTime(float time);
    void highlightAction(QAction *action);
    void setSimulationState(SimulationState state);
    void storeSnapshot(const QJsonObject& hierarchySnapshot);
    bool eventFilter(QObject *obj, QEvent *event) override;

    // ── Publicly accessible action pointers ───────────────────────────────
    QAction *startAction;
    QAction *pauseAction;
    QAction *stopAction;

    // ── Snapshot storage ──────────────────────────────────────────────────
    QJsonObject m_initialSnapshot;
    QAction* registerPluginAction(const QString& pluginId,
                                  QAction* action,
                                  int position = -1);
    QToolButton* registerPluginToolButton(const QString& pluginId,
                                          QAction* action,
                                          QToolButton::ToolButtonPopupMode mode = QToolButton::InstantPopup,
                                          int position = -1);
    QAction* registerPluginMenu(const QString& pluginId,
                                const QString& title,
                                QMenu* menu,
                                int position = -1);
    void unregisterPlugin(const QString& pluginId);

signals:
    void startTriggered();
    void pauseTriggered();
    void stopTriggered();
    void nextStepTriggered();
    void speedChanged(int speed);
    void loggerTriggered(bool checked);
    void radarDisplayToggled();
    void timeChanged(float newTimeInSeconds);
    void simulationStateChanged(SimulationState state);
    void resetTriggered();

private slots:
    void onTimeLabelClicked();
    void updateSimulationStatus();

private:
    void createActions();
    void setupToolBar();
    void updateTimeDisplay();
    void updateStatusDisplay();
    QPixmap withWhiteBg(const QString &iconPath);

    // ── Toolbar actions ───────────────────────────────────────────────────
    QAction *nextStepAction;
    QAction *timingAction;
    QAction *loggerAction;
    QAction *radarToggleAction;
    QAction *resetAction;

    // ── Widgets ───────────────────────────────────────────────────────────
    QSlider *speedSlider;
    QLabel *timeLabel;
    QLabel *simulationStatusLabel;
    QLabel *speedBadgeLabel;
    QWidget *speedControlWidget;

    // ── Timers ────────────────────────────────────────────────────────────
    QTimer *timer;
    QTimer *blinkTimer;

    // ── State ─────────────────────────────────────────────────────────────
    float elapsedSeconds;
    SimulationState currentState;
    bool blinkState;

    // Plugin tracking
    QMap<QString, QList<QObject*>> m_pluginOwnedObjects;
};

#endif // RUNTIMETOOLBAR_H
