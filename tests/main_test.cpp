#include <QApplication>
#include <QTest>
#include <QDebug>
#include <QMetaMethod>
#include <QMetaObject>

// All test headers (as in original)
#include "databaseeditortest/test_databaseeditor.h"
#include "menubartest/test_menubar.h"
// #include "runtimetoolbartest/test_runtimetoolbar.h"
#include "applicationdialogtest/applicationdialog_test.h"
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

// Helper: Count test functions (slots whose name starts with "test")
int countTestFunctions(const QObject* test) {
    const QMetaObject* mo = test->metaObject();
    int count = 0;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (method.methodType() == QMetaMethod::Slot && method.parameterCount() == 0) {
            QString name = QString::fromLatin1(method.name());
            if (name.startsWith("test")) {
                count++;
            }
        }
    }
    return count;
}

// Helper: Run a single test class, accumulate totals
int runTest(QObject* test, int argc, char* argv[], const char* className,
            int& totalTests, int& totalFails) {
    int testCount = countTestFunctions(test);
    totalTests += testCount;
    qDebug() << "\n[ RUN      ]" << className << "(" << testCount << " test functions)";
    int failCount = QTest::qExec(test, argc, argv);
    totalFails += failCount;
    qDebug() << "[ COMPLETE ]" << className << "- failures:" << failCount;
    return failCount;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    int overallStatus = 0;
    int overallTotalTests = 0;
    int overallTotalFails = 0;

    // List all test classes (order as in original)
    TestDatabaseEditor dbTest;
    overallStatus |= runTest(&dbTest, argc, argv, "TestDatabaseEditor", overallTotalTests, overallTotalFails);

    TestMenuBar menuTest;
    overallStatus |= runTest(&menuTest, argc, argv, "TestMenuBar", overallTotalTests, overallTotalFails);

    // TestRuntimeToolBar runtimeToolBarTest; // commented out in original
    // overallStatus |= runTest(&runtimeToolBarTest, ...);

    TestApplicationDialog appDialogTest;
    overallStatus |= runTest(&appDialogTest, argc, argv, "TestApplicationDialog", overallTotalTests, overallTotalFails);

    TestConsoleView consoleViewTest;
    overallStatus |= runTest(&consoleViewTest, argc, argv, "TestConsoleView", overallTotalTests, overallTotalFails);

    TestCustomResizableOverlayDock customDockTest;
    overallStatus |= runTest(&customDockTest, argc, argv, "TestCustomResizableOverlayDock", overallTotalTests, overallTotalFails);

    TestDesignToolBar designToolBarTest;
    overallStatus |= runTest(&designToolBarTest, argc, argv, "TestDesignToolBar", overallTotalTests, overallTotalFails);

    TestDoctrineParameters doctrineParamsTest;
    overallStatus |= runTest(&doctrineParamsTest, argc, argv, "TestDoctrineParameters", overallTotalTests, overallTotalFails);

    TestEntityInfoDialog entityInfoTest;
    overallStatus |= runTest(&entityInfoTest, argc, argv, "TestEntityInfoDialog", overallTotalTests, overallTotalFails);

    TestFeedbackDialog feedbackTest;
    overallStatus |= runTest(&feedbackTest, argc, argv, "TestFeedbackDialog", overallTotalTests, overallTotalFails);

    TestGraphWidget graphWidgetTest;
    overallStatus |= runTest(&graphWidgetTest, argc, argv, "TestGraphWidget", overallTotalTests, overallTotalFails);

    TestAddFormationDialog addFormationTest;
    overallStatus |= runTest(&addFormationTest, argc, argv, "TestAddFormationDialog", overallTotalTests, overallTotalFails);

    TestAddItemDialog addItemTest;
    overallStatus |= runTest(&addItemTest, argc, argv, "TestAddItemDialog", overallTotalTests, overallTotalFails);

    TestContextMenu contextMenuTest;
    overallStatus |= runTest(&contextMenuTest, argc, argv, "TestContextMenu", overallTotalTests, overallTotalFails);

    TestHierarchyTree hierarchyTreeTest;
    overallStatus |= runTest(&hierarchyTreeTest, argc, argv, "TestHierarchyTree", overallTotalTests, overallTotalFails);

    TestInspector inspectorTest;
    overallStatus |= runTest(&inspectorTest, argc, argv, "TestInspector", overallTotalTests, overallTotalFails);

    TestLayerPanel layerPanelTest;
    overallStatus |= runTest(&layerPanelTest, argc, argv, "TestLayerPanel", overallTotalTests, overallTotalFails);

    TestMainWindow mainWindowTest;
    overallStatus |= runTest(&mainWindowTest, argc, argv, "TestMainWindow", overallTotalTests, overallTotalFails);

    TestMeasureDistanceDialog measureDistanceTest;
    overallStatus |= runTest(&measureDistanceTest, argc, argv, "TestMeasureDistanceDialog", overallTotalTests, overallTotalFails);

    TestMissionEditor missionEditorTest;
    overallStatus |= runTest(&missionEditorTest, argc, argv, "TestMissionEditor", overallTotalTests, overallTotalFails);

    TestNavigationPage navigationPageTest;
    overallStatus |= runTest(&navigationPageTest, argc, argv, "TestNavigationPage", overallTotalTests, overallTotalFails);

    TestProfileInfoDialog profileInfoTest;
    overallStatus |= runTest(&profileInfoTest, argc, argv, "TestProfileInfoDialog", overallTotalTests, overallTotalFails);

    TestRecentProjectsManager recentProjectsTest;
    overallStatus |= runTest(&recentProjectsTest, argc, argv, "TestRecentProjectsManager", overallTotalTests, overallTotalFails);

    TestRuntimeToolBar runtimeToolBarTest;
    overallStatus |= runTest(&runtimeToolBarTest, argc, argv, "TestRuntimeToolBar", overallTotalTests, overallTotalFails);

    TestRuntimeEditor runtimeEditorTest;
    overallStatus |= runTest(&runtimeEditorTest, argc, argv, "TestRuntimeEditor", overallTotalTests, overallTotalFails);

    TestScenarioEditor scenarioEditorTest;
    overallStatus |= runTest(&scenarioEditorTest, argc, argv, "TestScenarioEditor", overallTotalTests, overallTotalFails);

    TestSidebarWidget sidebarWidgetTest;
    overallStatus |= runTest(&sidebarWidgetTest, argc, argv, "TestSidebarWidget", overallTotalTests, overallTotalFails);

    TestStatusBar statusBarTest;
    overallStatus |= runTest(&statusBarTest, argc, argv, "TestStatusBar", overallTotalTests, overallTotalFails);

    TestTacticalDisplay tacticalDisplayTest;
    overallStatus |= runTest(&tacticalDisplayTest, argc, argv, "TestTacticalDisplay", overallTotalTests, overallTotalFails);

    TestTacticalRules tacticalRulesTest;
    overallStatus |= runTest(&tacticalRulesTest, argc, argv, "TestTacticalRules", overallTotalTests, overallTotalFails);

    TestTestScriptDialog testScriptDialogTest;
    overallStatus |= runTest(&testScriptDialogTest, argc, argv, "TestTestScriptDialog", overallTotalTests, overallTotalFails);

    TestTextScriptWidget textScriptWidgetTest;
    overallStatus |= runTest(&textScriptWidgetTest, argc, argv, "TestTextScriptWidget", overallTotalTests, overallTotalFails);

    TestTooltipHelper tooltipHelperTest;
    overallStatus |= runTest(&tooltipHelperTest, argc, argv, "TestTooltipHelper", overallTotalTests, overallTotalFails);

    TestCanvasWidget canvasWidgetTest;
    overallStatus |= runTest(&canvasWidgetTest, argc, argv, "TestCanvasWidget", overallTotalTests, overallTotalFails);

    TestIFFDisplay iffDisplayTest;
    overallStatus |= runTest(&iffDisplayTest, argc, argv, "TestIFFDisplay", overallTotalTests, overallTotalFails);

    TestRADIODisplay radioDisplayTest;
    overallStatus |= runTest(&radioDisplayTest, argc, argv, "TestRADIODisplay", overallTotalTests, overallTotalFails);

    TestESMDisplay esmDisplayTest;
    overallStatus |= runTest(&esmDisplayTest, argc, argv, "TestESMDisplay", overallTotalTests, overallTotalFails);

    TestCSMDisplay csmDisplayTest;
    overallStatus |= runTest(&csmDisplayTest, argc, argv, "TestCSMDisplay", overallTotalTests, overallTotalFails);

    // Calculate pass percentage
    int passedTests = overallTotalTests - overallTotalFails;
    double passPercent = (overallTotalTests > 0) ? (passedTests * 100.0 / overallTotalTests) : 0.0;

    qDebug() << "\n========== OVERALL GUI TEST SUMMARY ==========";
    qDebug() << "Total test functions :" << overallTotalTests;
    qDebug() << "Passed               :" << passedTests;
    qDebug() << "Failed               :" << overallTotalFails;
    qDebug() << "percentage      :" << QString::number(passPercent, 'f', 2) << "%";
    qDebug() << "===========================================";
    return overallStatus;
}
