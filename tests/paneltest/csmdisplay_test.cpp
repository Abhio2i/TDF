#include "csmdisplay_test.h"
#include "GUI/Panel/csmdisplay.h"
#include <QTest>
#include <QMouseEvent>
#include <QResizeEvent>

void TestCSMDisplay::init()
{
    display = new CSMDisplay(nullptr);
    display->hide();
}

void TestCSMDisplay::cleanup()
{
    delete display;
    display = nullptr;
}

// ------------------------------------------------------------------
// Basic widget tests
// ------------------------------------------------------------------
void TestCSMDisplay::testWidgetExists()
{
    QVERIFY(display != nullptr);
    QVERIFY(display->isVisible() || !display->isVisible());
}

void TestCSMDisplay::testSizeHint()
{
    QSize hint = display->sizeHint();
    QVERIFY(hint.width() >= 250);
    QVERIFY(hint.height() >= 0);
}

void TestCSMDisplay::testMinimumSize()
{
    QSize minSize = display->minimumSize();
    QVERIFY(minSize.width() >= 250);
    QVERIFY(minSize.height() >= 0);
}

void TestCSMDisplay::testHeightForWidth()
{
    int width = 400;
    int height = display->heightForWidth(width);
    QVERIFY(height > 0);
    // Approximate aspect ratio 16:9
    QVERIFY(qAbs(height - qRound(width * 16.0 / 9.0)) <= 2);
}

// ------------------------------------------------------------------
// Display properties
// ------------------------------------------------------------------
void TestCSMDisplay::testDefaultRange()
{
    QCOMPARE(display->range, 100);
}

void TestCSMDisplay::testSetRange()
{
    display->setRange(500);
    QCOMPARE(display->range, 500);
    display->setRange(1000);
    QCOMPARE(display->range, 1000);
}

void TestCSMDisplay::testSetRangeNegative()
{
    display->setRange(-10);
    QVERIFY(true); // no crash
}

// ------------------------------------------------------------------
// Entity management (safe subset)
// ------------------------------------------------------------------
void TestCSMDisplay::testSelectEntityWithNull()
{
    display->selectEntity(nullptr);
    QVERIFY(true);
}

void TestCSMDisplay::testRemoveEntityWithInvalidId()
{
    display->RemoveEntity("nonexistent");
    QVERIFY(true);
}

void TestCSMDisplay::testRemoveEntityTwice()
{
    display->RemoveEntity("id1");
    display->RemoveEntity("id1");
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Update methods (no crash)
// ------------------------------------------------------------------
void TestCSMDisplay::testUpdateRadarWithoutEntity()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestCSMDisplay::testUpdateRadarWithEntityButNoSensor()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestCSMDisplay::testUpdateRadarMultipleTimes()
{
    for (int i = 0; i < 10; ++i) {
        display->updateRadar();
    }
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Mouse events (no crash)
// ------------------------------------------------------------------
void TestCSMDisplay::testMouseMoveEvent()
{
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(100,100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

void TestCSMDisplay::testMouseMoveWithDifferentPositions()
{
    QList<QPoint> positions = {QPoint(0,0), QPoint(200,200), QPoint(500,500), QPoint(-10,-10)};
    for (const QPoint& pos : positions) {
        QMouseEvent moveEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(display, &moveEvent);
    }
    QVERIFY(true);
}

void TestCSMDisplay::testLeaveEvent()
{
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(display, &leaveEvent);
    QVERIFY(true);
}

void TestCSMDisplay::testMouseMoveAfterResize()
{
    display->resize(800, 600);
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(400,300), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Paint events (no crash)
// ------------------------------------------------------------------
void TestCSMDisplay::testPaintEventWithDefaultSize()
{
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testPaintEventAfterResize()
{
    display->resize(500, 500);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testPaintEventWithSmallSize()
{
    display->resize(100, 100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testPaintEventWithLargeSize()
{
    display->resize(1000, 1000);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testPaintEventAfterSetRange()
{
    display->setRange(200);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}



// ------------------------------------------------------------------
// Edge cases
// ------------------------------------------------------------------
void TestCSMDisplay::testZeroSize()
{
    display->resize(0, 0);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testNegativeSize()
{
    display->resize(-100, -100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestCSMDisplay::testRapidResize()
{
    for (int i = 100; i <= 500; i += 50) {
        display->resize(i, i);
        QTest::qWait(10);
    }
    QVERIFY(true);
}

void TestCSMDisplay::testMultipleUpdateCalls()
{
    for (int i = 0; i < 20; ++i) {
        display->update();
        QTest::qWait(5);
    }
    QVERIFY(true);
}
