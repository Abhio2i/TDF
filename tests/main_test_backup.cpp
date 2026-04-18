#include <QApplication>
#include <QTest>
#include "databaseeditortest/test_databaseeditor.h"
#include "menubartest/test_menubar.h"
// #include "runtimetoolbartest/test_runtimetoolbar.h"
#include "applicationdialogtest/applicationdialog_test.h"   // ADD THIS
#include "consoleviewtest/consoleview_test.h"
#include "customdocktest/customdock_test.h"
#include "designtoolbartest/gui_designtoolbar_test.h"
#include "doctrineparameterstest/doctrineparameters_test.h"
#include "entityinfodialogtest/entityinfodialog_test.h"
#include "feedbacktest/feedback_test.h"
#include "graphwidgettest/graphwidget_test.h"
#include "hierarchytreetest/addformationdialog_test.h"
#include "hierarchytreetest/additemdialog_test.h"
#include "hierarchytreetest/contextmenu_test.h"
#include "hierarchytreetest/gui_hierarchytree_test.h"
#include "inspectortest/inspectortest.h"
#include "layerpaneltest/layerpanel_test.h"
#include "mainwindowtest/mainwindow_test.h"
#include "measuredistancedialogtest/measuredistancedialog_test.h"
#include "missioneditortest/gui_mission_test.h"
#include "navigationpagetest/navigationpage_test.h"
#include "profileinfodialogtest/profileinfodialog_test.h"
#include "recentprojectsmanagertest/recentprojectsmanager_test.h"
#include "runtimetoolbartest/gui_runtimetoolbar_test.h"
#include "runtimeeditortest/runtimeeditor_test.h"
#include "scenarioeditortest/scenarioeditor_test.h"
#include "sidebarwidgettest/sidebarwidget_test.h"
#include "statusbartest/statusbar_test.h"
#include "tacticaldisplaytest/tacticaldisplay_test.h"
#include "tacticalrulestest/tacticalrules_test.h"
#include "testscriptdialogtest/testscriptdialog_test.h"
#include "textscriptwidgettest/textscriptwidget_test.h"
#include "tooltiphelpertest/tooltiphelper_test.h"
#include "canvaswidgettest/canvaswidget_test.h"
#include "paneltest/iffdisplay_test.h"
#include "paneltest/radiodisplay_test.h"
#include "paneltest/esmdisplay_test.h"
#include "paneltest/csmdisplay_test.h"



// Stubs for any remaining test functions called from production code
class RuntimeToolBar;
class Console;
void runRuntimeToolBarTests(RuntimeToolBar*, Console*) {}
void runDesignToolBarTests(void*, void*) {}
void runMissionEditorTests(void*, void*) {}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    int status = 0;

    TestDatabaseEditor dbTest;
    status |= QTest::qExec(&dbTest, argc, argv);

    TestMenuBar menuTest;
    status |= QTest::qExec(&menuTest, argc, argv);

    // TestRuntimeToolBar runtimeToolBarTest;
    // status |= QTest::qExec(&runtimeToolBarTest, argc, argv);

    TestApplicationDialog appDialogTest;        // ADD THIS
    status |= QTest::qExec(&appDialogTest, argc, argv);

    TestConsoleView consoleViewTest;
    status |= QTest::qExec(&consoleViewTest, argc, argv);

    TestCustomResizableOverlayDock customDockTest;
    status |= QTest::qExec(&customDockTest, argc, argv);

    TestDesignToolBar designToolBarTest;
    status |= QTest::qExec(&designToolBarTest, argc, argv);
    TestDoctrineParameters doctrineParamsTest;
    status |= QTest::qExec(&doctrineParamsTest, argc, argv);
    TestEntityInfoDialog entityInfoTest;
    status |= QTest::qExec(&entityInfoTest, argc, argv);
    TestFeedbackDialog feedbackTest;
    status |= QTest::qExec(&feedbackTest, argc, argv);
    TestGraphWidget graphWidgetTest;
    status |= QTest::qExec(&graphWidgetTest, argc, argv);
    TestAddFormationDialog addFormationTest;
    status |= QTest::qExec(&addFormationTest, argc, argv);
    TestAddItemDialog addItemTest;
    status |= QTest::qExec(&addItemTest, argc, argv);
    TestContextMenu contextMenuTest;
    status |= QTest::qExec(&contextMenuTest, argc, argv);
    TestHierarchyTree hierarchyTreeTest;
    status |= QTest::qExec(&hierarchyTreeTest, argc, argv);
    TestInspector inspectorTest;
    status |= QTest::qExec(&inspectorTest, argc, argv);
    TestLayerPanel layerPanelTest;
    status |= QTest::qExec(&layerPanelTest, argc, argv);
    TestMainWindow mainWindowTest;
    status |= QTest::qExec(&mainWindowTest, argc, argv);
    TestMeasureDistanceDialog measureDistanceTest;
    status |= QTest::qExec(&measureDistanceTest, argc, argv);
    TestMissionEditor missionEditorTest;
    status |= QTest::qExec(&missionEditorTest, argc, argv);
    TestNavigationPage navigationPageTest;
    status |= QTest::qExec(&navigationPageTest, argc, argv);
    TestProfileInfoDialog profileInfoTest;
    status |= QTest::qExec(&profileInfoTest, argc, argv);
    TestRecentProjectsManager recentProjectsTest;
    status |= QTest::qExec(&recentProjectsTest, argc, argv);
    TestRuntimeToolBar runtimeToolBarTest;
    status |= QTest::qExec(&runtimeToolBarTest, argc, argv);
    TestScenarioEditor scenarioEditorTest;
    status |= QTest::qExec(&scenarioEditorTest, argc, argv);
    TestSidebarWidget sidebarWidgetTest;
    status |= QTest::qExec(&sidebarWidgetTest, argc, argv);
    TestStatusBar statusBarTest;
    status |= QTest::qExec(&statusBarTest, argc, argv);
    TestTacticalDisplay tacticalDisplayTest;
    status |= QTest::qExec(&tacticalDisplayTest, argc, argv);
    TestTacticalRules tacticalRulesTest;
    status |= QTest::qExec(&tacticalRulesTest, argc, argv);
    TestTestScriptDialog testScriptDialogTest;
    status |= QTest::qExec(&testScriptDialogTest, argc, argv);
    TestTextScriptWidget textScriptWidgetTest;
    status |= QTest::qExec(&textScriptWidgetTest, argc, argv);
    TestTooltipHelper tooltipHelperTest;
    status |= QTest::qExec(&tooltipHelperTest, argc, argv);
    TestCanvasWidget canvasWidgetTest;
    status |= QTest::qExec(&canvasWidgetTest, argc, argv);
    TestRuntimeEditor runtimeEditorTest;
    status |= QTest::qExec(&runtimeEditorTest, argc, argv);
    TestIFFDisplay iffDisplayTest;
    status |= QTest::qExec(&iffDisplayTest, argc, argv);
    TestRADIODisplay radioDisplayTest;
    status |= QTest::qExec(&radioDisplayTest, argc, argv);
    TestESMDisplay esmDisplayTest;
    status |= QTest::qExec(&esmDisplayTest, argc, argv);
    TestCSMDisplay csmDisplayTest;
    status |= QTest::qExec(&csmDisplayTest, argc, argv);
    return status;
}
