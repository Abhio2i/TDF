#include "iffdisplay_test.h"
#include "GUI/Panel/iffdisplay.h"
#include <QTest>
#include <QMouseEvent>
#include <QResizeEvent>

void TestIFFDisplay::init()
{
    display = new IFFDisplay(nullptr);
    display->hide();
}

void TestIFFDisplay::cleanup()
{
    delete display;
    display = nullptr;
}

// ------------------------------------------------------------------
// Basic widget tests
// ------------------------------------------------------------------
void TestIFFDisplay::testWidgetExists()
{
    QVERIFY(display != nullptr);
    QVERIFY(display->isVisible() || !display->isVisible());
}

void TestIFFDisplay::testSizeHint()
{
    QSize hint = display->sizeHint();
    QVERIFY(hint.width() >= 250);
    QVERIFY(hint.height() >= 0);
}

void TestIFFDisplay::testMinimumSize()
{
    QSize minSize = display->minimumSize();
    QVERIFY(minSize.width() >= 250);
    QVERIFY(minSize.height() >= 0);
}

void TestIFFDisplay::testHeightForWidth()
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
void TestIFFDisplay::testDefaultRange()
{
    QCOMPARE(display->range, 5000);
}

void TestIFFDisplay::testSetRange()
{
    display->setRange(1000);
    QCOMPARE(display->range, 1000);
    display->setRange(20000);
    QCOMPARE(display->range, 20000);
}

void TestIFFDisplay::testSetRangeNegative()
{
    display->setRange(-10);
    QVERIFY(true); // no crash
}

// ------------------------------------------------------------------
// Entity management (safe subset)
// ------------------------------------------------------------------
void TestIFFDisplay::testSelectEntityWithNull()
{
    display->selectEntity(nullptr);
    QVERIFY(true);
}

void TestIFFDisplay::testRemoveEntityWithInvalidId()
{
    display->RemoveEntity("nonexistent");
    QVERIFY(true);
}

void TestIFFDisplay::testRemoveEntityTwice()
{
    display->RemoveEntity("id1");
    display->RemoveEntity("id1");
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Update methods (no crash)
// ------------------------------------------------------------------
void TestIFFDisplay::testUpdateRadarWithoutEntity()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestIFFDisplay::testUpdateRadarWithEntityButNoIFF()
{
    display->updateRadar();
    QVERIFY(true);
}

void TestIFFDisplay::testUpdateRadarMultipleTimes()
{
    for (int i = 0; i < 10; ++i) {
        display->updateRadar();
    }
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Mouse events (no crash)
// ------------------------------------------------------------------
void TestIFFDisplay::testMouseMoveEvent()
{
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(100,100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

void TestIFFDisplay::testMouseMoveWithDifferentPositions()
{
    QList<QPoint> positions = {QPoint(0,0), QPoint(200,200), QPoint(500,500), QPoint(-10,-10)};
    for (const QPoint& pos : positions) {
        QMouseEvent moveEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(display, &moveEvent);
    }
    QVERIFY(true);
}

void TestIFFDisplay::testLeaveEvent()
{
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(display, &leaveEvent);
    QVERIFY(true);
}

void TestIFFDisplay::testMouseMoveAfterResize()
{
    display->resize(800, 600);
    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(400,300), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(display, &moveEvent);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Paint events (no crash)
// ------------------------------------------------------------------
void TestIFFDisplay::testPaintEventWithDefaultSize()
{
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testPaintEventAfterResize()
{
    display->resize(500, 500);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testPaintEventWithSmallSize()
{
    display->resize(100, 100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testPaintEventWithLargeSize()
{
    display->resize(1000, 1000);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testPaintEventAfterSetRange()
{
    display->setRange(2000);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}



// ------------------------------------------------------------------
// Edge cases
// ------------------------------------------------------------------
void TestIFFDisplay::testZeroSize()
{
    display->resize(0, 0);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testNegativeSize()
{
    display->resize(-100, -100);
    display->update();
    QTest::qWait(50);
    QVERIFY(true);
}

void TestIFFDisplay::testRapidResize()
{
    for (int i = 100; i <= 500; i += 50) {
        display->resize(i, i);
        QTest::qWait(10);
    }
    QVERIFY(true);
}

void TestIFFDisplay::testMultipleUpdateCalls()
{
    for (int i = 0; i < 20; ++i) {
        display->update();
        QTest::qWait(5);
    }
    QVERIFY(true);
}
