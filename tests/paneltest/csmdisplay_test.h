#ifndef CSMDISPLAY_TEST_H
#define CSMDISPLAY_TEST_H

#include <QObject>

class CSMDisplay;

class TestCSMDisplay : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic widget tests
    void testWidgetExists();
    void testSizeHint();
    void testMinimumSize();
    void testHeightForWidth();


    // Display properties
    void testDefaultRange();
    void testSetRange();
    void testSetRangeNegative();

    // Entity management (safe subset)
    void testSelectEntityWithNull();
    void testRemoveEntityWithInvalidId();
    void testRemoveEntityTwice();

    // Update methods (no crash)
    void testUpdateRadarWithoutEntity();
    void testUpdateRadarWithEntityButNoSensor();
    void testUpdateRadarMultipleTimes();

    // Mouse events (no crash)
    void testMouseMoveEvent();
    void testMouseMoveWithDifferentPositions();
    void testLeaveEvent();
    void testMouseMoveAfterResize();

    // Paint events (no crash)
    void testPaintEventWithDefaultSize();
    void testPaintEventAfterResize();
    void testPaintEventWithSmallSize();
    void testPaintEventWithLargeSize();
    void testPaintEventAfterSetRange();



    // Edge cases
    void testZeroSize();
    void testNegativeSize();
    void testRapidResize();
    void testMultipleUpdateCalls();

private:
    CSMDisplay* display = nullptr;
};

#endif
