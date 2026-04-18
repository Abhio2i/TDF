#ifndef CANVASWIDGET_TEST_H
#define CANVASWIDGET_TEST_H

#include <QObject>

class CanvasWidget;

class TestCanvasWidget : public QObject
{
    Q_OBJECT

private slots:
    void testWidgetExists();
    void testDefaultTransformMode();
    void testSetTransformMode();
    void testSetTrajectoryDrawingMode();
    void testLayerVisibilityToggles();
    void testSetImageScale();
    void testJsonSerialization();
    void testReInit();
    void testPointSelectionAndWaypoints();

private:
    CanvasWidget* canvas = nullptr;
};

#endif
