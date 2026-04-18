#include "canvaswidget_test.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QTest>
#include <QJsonObject>

void TestCanvasWidget::testWidgetExists()
{
    canvas = new CanvasWidget(nullptr);
    QVERIFY(canvas != nullptr);
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testDefaultTransformMode()
{
    canvas = new CanvasWidget(nullptr);
    QCOMPARE(canvas->currentMode, Translate);
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testSetTransformMode()
{
    canvas = new CanvasWidget(nullptr);
    canvas->setTransformMode(Rotate);
    QCOMPARE(canvas->currentMode, Rotate);
    canvas->setTransformMode(Scale);
    QCOMPARE(canvas->currentMode, Scale);
    canvas->setTransformMode(MeasureDistance);
    QCOMPARE(canvas->currentMode, MeasureDistance);
    canvas->setTransformMode(Translate);
    QCOMPARE(canvas->currentMode, Translate);
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testSetTrajectoryDrawingMode()
{
    canvas = new CanvasWidget(nullptr);
    canvas->setTrajectoryDrawingMode(true);
    QVERIFY(canvas->isDrawingTrajectory == true);
    canvas->setTrajectoryDrawingMode(false);
    QVERIFY(canvas->isDrawingTrajectory == false);
    QCOMPARE(canvas->currentMode, Translate);
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testLayerVisibilityToggles()
{
    canvas = new CanvasWidget(nullptr);
    canvas->toggleLayerVisibility("Image", false);
    QVERIFY(!canvas->showImage);
    canvas->toggleLayerVisibility("Image", true);
    QVERIFY(canvas->showImage);

    canvas->toggleLayerVisibility("Trajectories", false);
    QVERIFY(!canvas->showTrajectories);
    canvas->toggleLayerVisibility("Trajectories", true);
    QVERIFY(canvas->showTrajectories);

    canvas->toggleLayerVisibility("FPS", false);
    QVERIFY(!canvas->showFPS);
    canvas->toggleLayerVisibility("FPS", true);
    QVERIFY(canvas->showFPS);
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testSetImageScale()
{
    canvas = new CanvasWidget(nullptr);
    int original = canvas->ImageScale;
    canvas->setImageScale(100);
    QCOMPARE(canvas->ImageScale, 100);
    canvas->setImageScale(original); // restore
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testJsonSerialization()
{
    canvas = new CanvasWidget(nullptr);
    QJsonObject empty;
    canvas->fromJson(empty);
    QVERIFY(true); // no crash

    QJsonObject saved = canvas->toJson();
    QVERIFY(saved.contains("selectedBitmapType") || saved.contains("tempMeshes"));
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testReInit()
{
    canvas = new CanvasWidget(nullptr);
    canvas->ReInit();
    QVERIFY(true); // no crash
    delete canvas;
    canvas = nullptr;
}

void TestCanvasWidget::testPointSelectionAndWaypoints()
{
    canvas = new CanvasWidget(nullptr);
    canvas->deselectWaypoint();
    QVERIFY(true);
    canvas->selectWaypoint(-1);
    QVERIFY(true);
    delete canvas;
    canvas = nullptr;
}
