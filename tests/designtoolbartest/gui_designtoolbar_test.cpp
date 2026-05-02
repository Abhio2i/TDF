#include "gui_designtoolbar_test.h"
#include "GUI/Toolbars/designtoolbar.h"
#include <QTest>
#include <QAction>
#include <QMenu>

void TestDesignToolBar::init()
{
    toolbar = new DesignToolBar();
}

void TestDesignToolBar::cleanup()
{
    delete toolbar;
    toolbar = nullptr;
}

// ------------------------------------------------------------------
// Action existence tests
// ------------------------------------------------------------------
void TestDesignToolBar::testZoomActionsExist()
{
    QVERIFY(toolbar->zoomInAction != nullptr);
    QVERIFY(toolbar->zoomOutAction != nullptr);
}

void TestDesignToolBar::testMapLayerActionsExist()
{
    QVERIFY(toolbar->mapSelectLayerAction != nullptr);
}

void TestDesignToolBar::testSelectCenterActionExists()
{
    QVERIFY(toolbar->selectCenterAction != nullptr);
}

void TestDesignToolBar::testSearchPlaceActionExists()
{
    QVERIFY(toolbar->searchPlaceAction != nullptr);
}

void TestDesignToolBar::testLayerInfoActionExists()
{
    QVERIFY(toolbar->layerInfoAction != nullptr);
}

void TestDesignToolBar::testShapeActionExists()
{
    QVERIFY(toolbar->shapeAction != nullptr);
}

void TestDesignToolBar::testBitmapActionsExist()
{
    QVERIFY(toolbar->bitmapAction != nullptr);
    QVERIFY(toolbar->selectBitmapAction != nullptr);
}

void TestDesignToolBar::testPresetLayersActionExists()
{
    QVERIFY(toolbar->presetLayersAction != nullptr);
}

// void TestDesignToolBar::testImportGeoJsonActionExists()
// {
//     QVERIFY(toolbar->importGeoJsonAction != nullptr);
// }

void TestDesignToolBar::testGeoJsonLayersActionExists()
{
    QVERIFY(toolbar->geoJsonLayersAction != nullptr);
}

void TestDesignToolBar::testCoordinateSystemActionExists()
{
    QVERIFY(toolbar->coordinateSystemAction != nullptr);
}

void TestDesignToolBar::testMeasureDistanceActionExists()
{
    QAction* measureAction = toolbar->getMeasureDistanceAction();
    QVERIFY(measureAction != nullptr);
}

void TestDesignToolBar::testAddTrajectoryActionExists()
{
    QAction* addTrajAction = toolbar->getAddTrajectoryAction();
    QVERIFY(addTrajAction != nullptr);
}

// ------------------------------------------------------------------
// Menu tests
// ------------------------------------------------------------------
void TestDesignToolBar::testMapLayerHasMenu()
{
    QVERIFY(toolbar->mapSelectLayerAction->menu() != nullptr);
}

void TestDesignToolBar::testSearchPlaceHasMenu()
{
    QVERIFY(toolbar->searchPlaceAction->menu() != nullptr);
}

void TestDesignToolBar::testShapeHasMenu()
{
    QVERIFY(toolbar->shapeAction->menu() != nullptr);
}

void TestDesignToolBar::testBitmapHasMenu()
{
    QVERIFY(toolbar->bitmapAction->menu() != nullptr);
}

void TestDesignToolBar::testCoordinateSystemHasMenu()
{
    QVERIFY(toolbar->coordinateSystemAction->menu() != nullptr);
}

void TestDesignToolBar::testGeoJsonLayersHasMenu()
{
    QVERIFY(toolbar->geoJsonLayersAction->menu() != nullptr);
}

void TestDesignToolBar::testPresetLayersHasMenu()
{
    QVERIFY(toolbar->presetLayersAction->menu() != nullptr);
}

// ------------------------------------------------------------------
// State tests
// ------------------------------------------------------------------
void TestDesignToolBar::testZoomActionsNotCheckable()
{
    QVERIFY(!toolbar->zoomInAction->isCheckable());
    QVERIFY(!toolbar->zoomOutAction->isCheckable());
}

void TestDesignToolBar::testAtLeastOneModeActionChecked()
{
    QList<QAction*> modeActions = {
        toolbar->zoomInAction,
        toolbar->zoomOutAction,
        toolbar->selectCenterAction,
        toolbar->shapeAction,
        toolbar->bitmapAction,
        toolbar->presetLayersAction,
        // toolbar->importGeoJsonAction,
        toolbar->geoJsonLayersAction,
        toolbar->coordinateSystemAction
    };
    bool anyChecked = false;
    for (QAction* act : modeActions) {
        if (act && act->isChecked()) {
            anyChecked = true;
            break;
        }
    }
    QVERIFY(anyChecked);
}

void TestDesignToolBar::testZoomActionsEnabled()
{
    QVERIFY(toolbar->zoomInAction->isEnabled());
    QVERIFY(toolbar->zoomOutAction->isEnabled());
}

// ------------------------------------------------------------------
// Optional: Layer menu content
// ------------------------------------------------------------------
void TestDesignToolBar::testLayerMenuContainsToolTip()
{
    QMenu* layerMenu = toolbar->layerSelectAction->menu();
    QVERIFY(layerMenu != nullptr);
    bool hasToolTip = false;
    for (QAction* act : layerMenu->actions()) {
        if (act->text() == "ToolTip") {
            hasToolTip = true;
            break;
        }
    }
    QVERIFY(hasToolTip);
}
