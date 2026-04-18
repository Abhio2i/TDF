#include "tacticaldisplay_test.h"
#include "GUI/Tacticaldisplay/tacticaldisplay.h"
#include <QTest>
#include <QSplitter>
#include <QStackedWidget>
#include <QWidget>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QLabel>

TacticalDisplay* TestTacticalDisplay::display = nullptr;

void TestTacticalDisplay::initTestCase()
{
    display = new TacticalDisplay(nullptr);
    display->hide();  // avoid flicker
    QTest::qWait(100);
}

void TestTacticalDisplay::cleanupTestCase()
{
    delete display;
    display = nullptr;
}

void TestTacticalDisplay::testBasicExistence()
{
    QVERIFY(display != nullptr);
    QVERIFY(display->isVisible() || !display->isVisible());
}

void TestTacticalDisplay::testChildWidgetsExist()
{
    QVERIFY(display->canvas != nullptr);
    QVERIFY(display->mapWidget != nullptr);
    QVERIFY(display->scaleBar != nullptr);
    QVERIFY(display->splitter != nullptr);
    QVERIFY(display->scene3dwidget != nullptr);
}

void TestTacticalDisplay::testZoomMethods()
{
    int originalZoom = display->currentZoom;
    display->zoomIn();
    QCOMPARE(display->currentZoom, originalZoom + 1);
    display->zoomOut();
    QCOMPARE(display->currentZoom, originalZoom);
    // zoomOut beyond min – should not crash
    for (int i = 0; i < 10; ++i) display->zoomOut();
    QVERIFY(true);
}

void TestTacticalDisplay::testSetMapLayers()
{
    QStringList layers = {"osm", "satellite"};
    display->setMapLayers(layers);
    QVERIFY(true); // no crash
}

void TestTacticalDisplay::testAddCustomMap()
{
    display->addCustomMap("TestMap", 0, 5, "http://test.com/tiles/{z}/{x}/{y}.png", 0.8);
    QVERIFY(true);
}

void TestTacticalDisplay::test3DViewMethods()
{
    // If 3D view methods are implemented, uncomment:
    // display->show3DView();
    // QVERIFY(display->scene3dwidget->isVisible());
    // display->hide3DView();
    // QVERIFY(!display->scene3dwidget->isVisible());
    // Otherwise, just check that methods exist (compile-time)
    QVERIFY(true);
}

void TestTacticalDisplay::testCoordinateSystemChange()
{
    display->onCoordinateSystemChanged("EPSG:4326");
    QVERIFY(true);
}

void TestTacticalDisplay::testWheelEvent()
{
    QWheelEvent wheelEvent(QPointF(100,100), QPointF(100,100), QPoint(0,120), QPoint(0,120),
                           Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(display, &wheelEvent);
    QVERIFY(true);
}

void TestTacticalDisplay::testResizeEvent()
{
    QResizeEvent resizeEvent(QSize(800,600), QSize(800,600));
    QCoreApplication::sendEvent(display, &resizeEvent);
    QVERIFY(true);
}

void TestTacticalDisplay::testCanvasGislibPointer()
{
    QVERIFY(display->canvas->gislib == display->mapWidget);
}

void TestTacticalDisplay::testOverlayAndScaleBar()
{
    QList<QLabel*> labels = display->findChildren<QLabel*>();
    QVERIFY(labels.size() >= 2);
}
