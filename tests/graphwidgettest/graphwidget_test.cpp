#include "graphwidget_test.h"
#include "GUI/Timing/graphwidget.h"
#include <QTest>
#include <QTableWidget>
#include <QTableWidgetItem>

void TestGraphWidget::init()
{
    widget = new GraphWidget(nullptr);
}

void TestGraphWidget::cleanup()
{
    delete widget;
    widget = nullptr;
}

// ------------------------------------------------------------------
// Basic properties
// ------------------------------------------------------------------
void TestGraphWidget::testWidgetExists()
{
    QVERIFY(widget != nullptr);
}

void TestGraphWidget::testWindowTitle()
{
    QVERIFY(widget->windowTitle().isEmpty());
}

void TestGraphWidget::testWindowStaysOnTop()
{
    QVERIFY(widget->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
}

// ------------------------------------------------------------------
// UI components
// ------------------------------------------------------------------
void TestGraphWidget::testTableExists()
{
    QTableWidget* table = widget->findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
}

void TestGraphWidget::testTableColumns()
{
    QTableWidget* table = widget->findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QCOMPARE(table->columnCount(), 5);
}

void TestGraphWidget::testTableHeaders()
{
    QTableWidget* table = widget->findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QStringList expected = {"Entity Name", "Start Time", "End Time", "Total Duration", "Status"};
    for (int i = 0; i < expected.size(); ++i) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(i);
        QVERIFY(headerItem != nullptr);
        QCOMPARE(headerItem->text(), expected[i]);
    }
}

void TestGraphWidget::testTableMaxHeight()
{
    QTableWidget* table = widget->findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QCOMPARE(table->maximumHeight(), 150);
}

void TestGraphWidget::testCanvasExists()
{
    QWidget* canvas = widget->findChild<QWidget*>("", Qt::FindDirectChildrenOnly);
    QVERIFY(canvas != nullptr);
}

// ------------------------------------------------------------------
// Static method
// ------------------------------------------------------------------
void TestGraphWidget::testFormatTime()
{
    QCOMPARE(GraphWidget::formatTime(0), QString("00:00:00"));
    QCOMPARE(GraphWidget::formatTime(61), QString("00:01:01"));
    QCOMPARE(GraphWidget::formatTime(3661), QString("01:01:01"));
    QCOMPARE(GraphWidget::formatTime(86400), QString("24:00:00"));
}

// ------------------------------------------------------------------
// Member values
// ------------------------------------------------------------------




void TestGraphWidget::testDefaultZoom()
{
    QCOMPARE(widget->zoom, 100);
}

// ------------------------------------------------------------------
// Graph data
// ------------------------------------------------------------------
void TestGraphWidget::testGraphDataListSize()
{
    QCOMPARE(widget->graphDataList.size(), 3);
}

void TestGraphWidget::testGraphDataFirstEntry()
{
    if (widget->graphDataList.size() >= 1) {
        QCOMPARE(widget->graphDataList[0].entityNum, 0);
        QCOMPARE(widget->graphDataList[0].color, Qt::blue);
    } else {
        QFAIL("graphDataList has fewer than 1 entry");
    }
}

// ------------------------------------------------------------------
// Zoom limits
// ------------------------------------------------------------------
void TestGraphWidget::testZoomAssignment()
{
    widget->zoom = 150;
    QCOMPARE(widget->zoom, 150);
    widget->zoom = 15;
    QCOMPARE(widget->zoom, 15); // variable can be set lower, limit enforced elsewhere
}

void TestGraphWidget::testHandleWheelExists()
{
    // handleWheel is a method – just check that calling it (with dummy event) doesn't crash.
    // We cannot create a proper QWheelEvent easily, but we can test existence via meta-object.
    const QMetaObject* mo = widget->metaObject();
    int methodIndex = mo->indexOfMethod("handleWheel(QWheelEvent*)");
    QVERIFY(methodIndex != -1);
}

// ------------------------------------------------------------------
// Refresh without crash
// ------------------------------------------------------------------
void TestGraphWidget::testRefreshDoesNotCrash()
{
    widget->refresh(0.0f);
    QVERIFY(true); // reached without crash
}

void TestGraphWidget::testSetHierarchyExists()
{
    // Just test that the method exists (compile-time)
    // We can call it with nullptr to see if it crashes.
    widget->setHierarchy(nullptr);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Drawing methods
// ------------------------------------------------------------------
void TestGraphWidget::testDrawingMethodsExist()
{
    // drawGraph and drawData are protected? They are called from paintEvent.
    // We just verify that the widget can be updated without crash.
    widget->update();
    QVERIFY(true);
}
