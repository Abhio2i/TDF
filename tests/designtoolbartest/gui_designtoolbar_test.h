#ifndef GUI_DESIGNTOOLBAR_TEST_H
#define GUI_DESIGNTOOLBAR_TEST_H

#include <QObject>

class DesignToolBar;

class TestDesignToolBar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Action existence tests
    void testZoomActionsExist();
    void testMapLayerActionsExist();
    void testSelectCenterActionExists();
    void testSearchPlaceActionExists();
    void testLayerInfoActionExists();
    void testShapeActionExists();
    void testBitmapActionsExist();
    void testPresetLayersActionExists();
    void testImportGeoJsonActionExists();
    void testGeoJsonLayersActionExists();
    void testCoordinateSystemActionExists();
    void testMeasureDistanceActionExists();
    void testAddTrajectoryActionExists();

    // Menu tests
    void testMapLayerHasMenu();
    void testSearchPlaceHasMenu();
    void testShapeHasMenu();
    void testBitmapHasMenu();
    void testCoordinateSystemHasMenu();
    void testGeoJsonLayersHasMenu();
    void testPresetLayersHasMenu();

    // State tests
    void testZoomActionsNotCheckable();
    void testAtLeastOneModeActionChecked();
    void testZoomActionsEnabled();

    // Optional: layer menu content
    void testLayerMenuContainsToolTip();

private:
    DesignToolBar* toolbar = nullptr;
};

#endif
