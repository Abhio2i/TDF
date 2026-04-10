#include "gui_designtoolbar_test.h"
#include "GUI/Toolbars/designtoolbar.h"
#include "core/Debug/console.h"
#include <QAction>
#include <QMenu>
#include <QDebug>

#define TOOLBAR_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runDesignToolBarTests(DesignToolBar* toolbar, Console* console)
{
    if (!toolbar || !console) {
        if (console) console->error("DesignToolBar or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("       DESIGN TOOLBAR UNIT TESTS         "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Main actions exist -----
    TOOLBAR_TEST(toolbar->zoomInAction != nullptr, "Zoom In action exists");
    TOOLBAR_TEST(toolbar->zoomOutAction != nullptr, "Zoom Out action exists");
    TOOLBAR_TEST(toolbar->mapSelectLayerAction != nullptr, "Map Layer action exists");
    TOOLBAR_TEST(toolbar->selectCenterAction != nullptr, "Select Center action exists");
    TOOLBAR_TEST(toolbar->searchPlaceAction != nullptr, "Search Place action exists");
    TOOLBAR_TEST(toolbar->layerInfoAction != nullptr, "Layer Info action exists");
    TOOLBAR_TEST(toolbar->shapeAction != nullptr, "Shape action exists");
    TOOLBAR_TEST(toolbar->bitmapAction != nullptr, "Bitmap action exists");
    TOOLBAR_TEST(toolbar->selectBitmapAction != nullptr, "Select Bitmap action exists");
    TOOLBAR_TEST(toolbar->presetLayersAction != nullptr, "Preset Layers action exists");
    TOOLBAR_TEST(toolbar->importGeoJsonAction != nullptr, "Import GeoJSON action exists");
    TOOLBAR_TEST(toolbar->geoJsonLayersAction != nullptr, "GeoJSON Layers action exists");
    TOOLBAR_TEST(toolbar->coordinateSystemAction != nullptr, "Coordinate System action exists");

    // ----- Test 2: Additional actions (if getters available) -----
    QAction* measureAction = toolbar->getMeasureDistanceAction();
    TOOLBAR_TEST(measureAction != nullptr, "Measure Distance action exists");

    QAction* addTrajAction = toolbar->getAddTrajectoryAction();
    TOOLBAR_TEST(addTrajAction != nullptr, "Add Trajectory action exists");

    // ----- Test 3: Check default checked states -----
    // View action should be checked by default (panning mode)
    // But we cannot assume; we can check that at least one mode action is checked
    bool anyModeChecked = false;
    QList<QAction*> modeActions = {toolbar->zoomInAction, toolbar->zoomOutAction,
                                    toolbar->selectCenterAction, toolbar->shapeAction,
                                    toolbar->bitmapAction, toolbar->presetLayersAction,
                                    toolbar->importGeoJsonAction, toolbar->geoJsonLayersAction,
                                    toolbar->coordinateSystemAction};
    for (QAction* act : modeActions) {
        if (act && act->isChecked()) {
            anyModeChecked = true;
            break;
        }
    }
    TOOLBAR_TEST(anyModeChecked, "At least one action is checked (valid initial state)");

    // ----- Test 4: Verify menus are attached to appropriate actions -----
    TOOLBAR_TEST(toolbar->mapSelectLayerAction->menu() != nullptr, "Map Layer has a menu");
    TOOLBAR_TEST(toolbar->searchPlaceAction->menu() != nullptr, "Search Place has a menu");
    TOOLBAR_TEST(toolbar->shapeAction->menu() != nullptr, "Shape has a menu");
    TOOLBAR_TEST(toolbar->bitmapAction->menu() != nullptr, "Bitmap has a menu");
    TOOLBAR_TEST(toolbar->coordinateSystemAction->menu() != nullptr, "Coordinate System has a menu");
    TOOLBAR_TEST(toolbar->geoJsonLayersAction->menu() != nullptr, "GeoJSON Layers has a menu");
    TOOLBAR_TEST(toolbar->presetLayersAction->menu() != nullptr, "Preset Layers has a menu");

    // ----- Test 5: Check that zoom actions are not checkable (should be momentary) -----
    TOOLBAR_TEST(!toolbar->zoomInAction->isCheckable(), "Zoom In is not checkable (momentary)");
    TOOLBAR_TEST(!toolbar->zoomOutAction->isCheckable(), "Zoom Out is not checkable (momentary)");

    // ----- Test 6: Verify that critical signals can be emitted (just check non-null) -----
    // We can't test signal emission without mocking, but we can check that the action is enabled.
    TOOLBAR_TEST(toolbar->zoomInAction->isEnabled(), "Zoom In action is enabled");
    TOOLBAR_TEST(toolbar->zoomOutAction->isEnabled(), "Zoom Out action is enabled");

    // ----- Test 7: Check that layer options menu contains expected items (optional) -----
    QMenu* layerMenu = toolbar->layerSelectAction->menu();
    if (layerMenu) {
        bool hasTooltip = false;
        for (QAction* act : layerMenu->actions()) {
            if (act->text() == "ToolTip") hasTooltip = true;
        }
        TOOLBAR_TEST(hasTooltip, "Layer menu contains 'ToolTip' option");
    } else {
        TOOLBAR_TEST(false, "Layer select action has a menu");
    }

    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("DESIGN TOOLBAR TESTS: Some tests FAILED."));
    else
        console->log(std::string("DESIGN TOOLBAR TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
