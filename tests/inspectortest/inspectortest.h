#ifndef INSPECTORTEST_H
#define INSPECTORTEST_H

#include <QObject>

class Inspector;
class Hierarchy;

class TestInspector : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasicProperties();
    void testTableWidgetExists();
    void testSimpleJsonObject();
    void testArrayProperty();
    void testMultiComponentContainer();
    void testResetState();
    void testLockFunctionality();
    void testFormatNumberForUI();
    void testCopyPasteMethodsExist();
    void testRefreshForDeveloperMode();
    void testBooleanCell();
    void testNumberCell();
    void testStringCell();
    void testArrayCell();
    void testUnitParameterCell();
    void testVectorCell();
    void testGeocordCell();
    void testOptionCell();
    void testColorCell();
    void testImageCell();
    void testGenericObjectCell();
    void testSensorsContainer();
    void testRadiosContainer();
    void testIffsContainer();
    void testContainerActiveToggle();
    void testContainerSectionExpansion();

    void testSectionExpansion();
    // void testMultipleSections();
    void testCopyComponent();
    void testPasteComponent();
    void testLockState();
    void testResetStateAfterLoad();
    void testDeveloperModeRefresh();
    void testInitialDataStore();
    void testValueChangedSignal();
    void testParameterChangedSignal();
    void testTrajectoryWaypointsChanged();

    void testFormatNumberForUIEdgeCases();


    void testWheelableLineEditModifiers();
    void testDropEntityToArrayCell();
    void testDropSensorToSensorsContainer();
    void testAddCustomParameter();
    void testRemoveCustomParameter();
    void testCustomParameterTypes();

private:
    Inspector* inspector = nullptr;
    Hierarchy* dummyHierarchy = nullptr;
};

#endif
