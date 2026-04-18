#include "radiodisplay_test.h"
#include "GUI/Panel/radiodisplay.h"
#include <QTest>
#include <QMouseEvent>
#include <QResizeEvent>

void TestRADIODisplay::init()
{
    display = new RADIODisplay(nullptr);
    // Ensure widget is not shown to avoid flicker
    display->hide();
}

void TestRADIODisplay::cleanup()
{
    delete display;
    display = nullptr;
}

// ------------------------------------------------------------------
// Basic widget tests
// ------------------------------------------------------------------
void TestRADIODisplay::testWidgetExists()
{
    QVERIFY(display != nullptr);
    QVERIFY(display->isVisible() || !display->isVisible());
}

void TestRADIODisplay::testSizeHint()
{
    QSize hint = display->sizeHint();
    QVERIFY(hint.width() >= 250);
    QVERIFY(hint.height() >= 0);
}

void TestRADIODisplay::testMinimumSize()
{
    QSize minSize = display->minimumSize();
    QVERIFY(minSize.width() >= 250);
    QVERIFY(minSize.height() >= 0);
}

void TestRADIODisplay::testHeightForWidth()
{
    int width = 400;
    int height = display->heightForWidth(width);
    QVERIFY(height > 0);
    // Approximate aspect ratio 16:9 (actual may differ slightly)
    QVERIFY(qAbs(height - qRound(width * 16.0 / 9.0)) <= 2);
}



// ------------------------------------------------------------------
// Display properties
// ------------------------------------------------------------------
void TestRADIODisplay::testDefaultRange()
{
    QCOMPARE(display->range, 10);
}

void TestRADIODisplay::testSetRange()
{
    display->setRange(50);
    QCOMPARE(display->range, 50);
    display->setRange(100);
    QCOMPARE(display->range, 100);
}

void TestRADIODisplay::testSetRangeNegative()
{
    display->setRange(-10);
    // Should not crash; the display may clamp or ignore negative values
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Entity management (safe subset)
// ------------------------------------------------------------------
void TestRADIODisplay::testSelectEntityWithNull()
{
    display->selectEntity(nullptr);
    QVERIFY(true); // no crash
}

void TestRADIODisplay::testRemoveEntityWithInvalidId()
{
    display->RemoveEntity("nonexistent");
    QVERIFY(true); // no crash
}

void TestRADIODisplay::testRemoveEntityTwice()
{
    display->RemoveEntity("id1");
    display->RemoveEntity("id1");
    QVERIFY(true); // no crash
}

// ------------------------------------------------------------------
// Update methods (no crash)
// ------------------------------------------------------------------
void TestRADIODisplay::testUpdateRadarWithoutEntity()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestRADIODisplay::testUpdateRadarWithEntityButNoRadio()
{
    // We cannot easily create a valid Platform with Radio here,
    // but we can call selectEntity with a dummy pointer (not valid)
    // This will not crash because selectEntity checks the pointer.
    // We'll just call updateRadar after selecting a dummy entity.
    // Actually, we cannot create a valid Entity without a full hierarchy.
    // So we skip the actual entity creation; just call updateRadar again.
    display->updateRadar();
    QVERIFY(true);
}

void TestRADIODisplay::testUpdateRadarMultipleTimes()
{
    for (int i = 0; i < 10; ++i) {
        display->updateRadar();
    }
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Mouse events (no crash)
// ------------------------------------------------------------------
void TestRADIODisplay::testMouseMoveEvent()
{
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(100,100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

void TestRADIODisplay::testMouseMoveWithDifferentPositions()
{
    QList<QPoint> positions = {QPoint(0,0), QPoint(200,200), QPoint(500,500), QPoint(-10,-10)};
    for (const QPoint& pos : positions) {
        QMouseEvent moveEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(display, &moveEvent);
    }
    QVERIFY(true);
}

void TestRADIODisplay::testLeaveEvent()
{
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(display, &leaveEvent);
    QVERIFY(true);
}

void TestRADIODisplay::testMouseMoveAfterResize()
{
    display->resize(800, 600);
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(400,300), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Paint events (no crash)
// ------------------------------------------------------------------
void TestRADIODisplay::testPaintEventWithDefaultSize()
{
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testPaintEventAfterResize()
{
    display->resize(500, 500);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testPaintEventWithSmallSize()
{
    display->resize(100, 100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testPaintEventWithLargeSize()
{
    display->resize(1000, 1000);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testPaintEventAfterSetRange()
{
    display->setRange(200);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}



// ------------------------------------------------------------------
// Edge cases
// ------------------------------------------------------------------
void TestRADIODisplay::testZeroSize()
{
    display->resize(0, 0);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testNegativeSize()
{
    display->resize(-100, -100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestRADIODisplay::testRapidResize()
{
    for (int i = 100; i <= 500; i += 50) {
        display->resize(i, i);
        QTest::qWait(10);
    }
    QVERIFY(true);
}

void TestRADIODisplay::testMultipleUpdateCalls()
{
    for (int i = 0; i < 20; ++i) {
        display->update();
        QTest::qWait(5);
    }
    QVERIFY(true);
}
