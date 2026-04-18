#ifndef GUI_RUNTIMETOOLBAR_TEST_H
#define GUI_RUNTIMETOOLBAR_TEST_H

#include <QObject>

class RuntimeToolBar;

class TestRuntimeToolBar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Existing 8 tests
    void testMainActionsExist();
    void testStartActionIsCheckable();
    void testInitialState();
    void testSpeedSlider();
    void testTimeLabel();
    void testSimulationStatusLabel();
    void testActionsEnabled();
    void testResetActionExists();

    // New tests (to reach 30+)
    void testPauseActionExists();
    void testStopActionExists();
    void testNextStepActionExists();
    void testTimingActionExists();
    void testLoggerActionExists();
    void testRadarToggleActionExists();

    void testTimeLabelTextFormat();
    void testStartActionShortcut();
    void testStopActionShortcut();
    void testStartActionTriggeredSignal();

    void testStopActionTriggeredSignal();
    void testNextStepActionTriggeredSignal();
    void testResetActionTriggeredSignal();
    void testSpeedChangedSignal();
    void testLoggerTriggeredSignal();
    void testRadarDisplayToggledSignal();
    void testTimeChangedSignal();

    void testSetSimulationStateRunning();
    void testSetSimulationStatePaused();
    void testSetSimulationStateStopped();
    void testOnElapsedTimeUpdatesLabel();
    void testStoreAndGetSnapshot();
    void testHighlightAction();
    void testEventFilterOnTimeLabel();

private:
    RuntimeToolBar* toolbar = nullptr;
};

#endif
