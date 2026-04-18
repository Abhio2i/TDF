#ifndef GRAPHWIDGET_TEST_H
#define GRAPHWIDGET_TEST_H

#include <QObject>

class GraphWidget;

class TestGraphWidget : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic properties
    void testWidgetExists();
    void testWindowTitle();
    void testWindowStaysOnTop();

    // UI components
    void testTableExists();
    void testTableColumns();
    void testTableHeaders();
    void testTableMaxHeight();
    void testCanvasExists();

    // Static method
    void testFormatTime();

    // Member values


    void testDefaultZoom();

    // Graph data
    void testGraphDataListSize();
    void testGraphDataFirstEntry();

    // Zoom limits
    void testZoomAssignment();
    void testHandleWheelExists();

    // Refresh without crash
    void testRefreshDoesNotCrash();
    void testSetHierarchyExists();

    // Drawing methods
    void testDrawingMethodsExist();

private:
    GraphWidget* widget = nullptr;
};

#endif
