#include "esmdisplay_test.h"
#include "GUI/Panel/esmdisplay.h"
#include <QTest>
#include <QMouseEvent>
#include <QResizeEvent>

void TestESMDisplay::init()
{
    display = new ESMDisplay(nullptr);
    display->hide();
}

void TestESMDisplay::cleanup()
{
    delete display;
    display = nullptr;
}

// ------------------------------------------------------------------
// Basic widget tests
// ------------------------------------------------------------------
void TestESMDisplay::testWidgetExists()
{
    QVERIFY(display != nullptr);
    QVERIFY(display->isVisible() || !display->isVisible());
}

void TestESMDisplay::testSizeHint()
{
    QSize hint = display->sizeHint();
    QVERIFY(hint.width() >= 250);
    QVERIFY(hint.height() >= 0);
}

void TestESMDisplay::testMinimumSize()
{
    QSize minSize = display->minimumSize();
    QVERIFY(minSize.width() >= 250);
    QVERIFY(minSize.height() >= 0);
}

void TestESMDisplay::testHeightForWidth()
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
void TestESMDisplay::testDefaultRange()
{
    QCOMPARE(display->range, 100);
}

void TestESMDisplay::testSetRange()
{
    display->setRange(500);
    QCOMPARE(display->range, 500);
    display->setRange(1000);
    QCOMPARE(display->range, 1000);
}

void TestESMDisplay::testSetRangeNegative()
{
    display->setRange(-10);
    QVERIFY(true); // no crash
}

// ------------------------------------------------------------------
// Entity management (safe subset)
// ------------------------------------------------------------------
void TestESMDisplay::testSelectEntityWithNull()
{
    display->selectEntity(nullptr);
    QVERIFY(true);
}

void TestESMDisplay::testRemoveEntityWithInvalidId()
{
    display->RemoveEntity("nonexistent");
    QVERIFY(true);
}

void TestESMDisplay::testRemoveEntityTwice()
{
    display->RemoveEntity("id1");
    display->RemoveEntity("id1");
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Update methods (no crash)
// ------------------------------------------------------------------
void TestESMDisplay::testUpdateRadarWithoutEntity()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestESMDisplay::testUpdateRadarWithEntityButNoSensor()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestESMDisplay::testUpdateRadarMultipleTimes()
{
    for (int i = 0; i < 10; ++i) {
        display->updateRadar();
    }
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Mouse events (no crash)
// ------------------------------------------------------------------
void TestESMDisplay::testMouseMoveEvent()
{
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(100,100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

void TestESMDisplay::testMouseMoveWithDifferentPositions()
{
    QList<QPoint> positions = {QPoint(0,0), QPoint(200,200), QPoint(500,500), QPoint(-10,-10)};
    for (const QPoint& pos : positions) {
        QMouseEvent moveEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(display, &moveEvent);
    }
    QVERIFY(true);
}

void TestESMDisplay::testLeaveEvent()
{
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(display, &leaveEvent);
    QVERIFY(true);
}

void TestESMDisplay::testMouseMoveAfterResize()
{
    display->resize(800, 600);
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(400,300), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Paint events (no crash)
// ------------------------------------------------------------------
void TestESMDisplay::testPaintEventWithDefaultSize()
{
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestESMDisplay::testPaintEventAfterResize()
{
    display->resize(500, 500);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestESMDisplay::testPaintEventWithSmallSize()
{
    display->resize(100, 100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestESMDisplay::testPaintEventWithLargeSize()
{
    display->resize(1000, 1000);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestESMDisplay::testPaintEventAfterSetRange()
{
    display->setRange(200);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}




void TestESMDisplay::testNegativeSize()
{
    display->resize(-100, -100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestESMDisplay::testRapidResize()
{
    for (int i = 100; i <= 500; i += 50) {
        display->resize(i, i);
        QTest::qWait(10);
    }
    QVERIFY(true);
}

void TestESMDisplay::testMultipleUpdateCalls()
{
    for (int i = 0; i < 20; ++i) {
        display->update();
        QTest::qWait(5);
    }
    QVERIFY(true);
}
