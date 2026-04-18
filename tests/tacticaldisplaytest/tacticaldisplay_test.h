#ifndef TACTICALDISPLAY_TEST_H
#define TACTICALDISPLAY_TEST_H

#include <QObject>

class TacticalDisplay;

class TestTacticalDisplay : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();      // called once before all tests
    void cleanupTestCase();   // called once after all tests

    void testBasicExistence();
    void testChildWidgetsExist();
    void testZoomMethods();
    void testSetMapLayers();
    void testAddCustomMap();
    void test3DViewMethods();
    void testCoordinateSystemChange();
    void testWheelEvent();
    void testResizeEvent();
    void testCanvasGislibPointer();
    void testOverlayAndScaleBar();

private:
    static TacticalDisplay* display;
};

#endif
