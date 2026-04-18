#!/bin/bash

BINARY="$HOME/Downloads/TDF_4.0.16_15April/tests/build/Desktop-Debug/tdf_tests"
TESTS_DIR="$HOME/Downloads/TDF_4.0.16_15April/tests"
BUILD_DIR="$TESTS_DIR/build/Desktop-Debug"

# Test class -> header file mapping (exact paths)
declare -A TEST_HEADERS=(
    ["TestDatabaseEditor"]="databaseeditortest/test_databaseeditor.h"
    ["TestMenuBar"]="menubartest/test_menubar.h"
    ["TestApplicationDialog"]="applicationdialogtest/applicationdialog_test.h"
    ["TestConsoleView"]="consoleviewtest/consoleview_test.h"
    ["TestCustomResizableOverlayDock"]="customdocktest/customdock_test.h"
    ["TestDesignToolBar"]="designtoolbartest/gui_designtoolbar_test.h"
    ["TestDoctrineParameters"]="doctrineparameterstest/doctrineparameters_test.h"
    ["TestEntityInfoDialog"]="entityinfodialogtest/entityinfodialog_test.h"
    ["TestFeedbackDialog"]="feedbacktest/feedback_test.h"
    ["TestGraphWidget"]="graphwidgettest/graphwidget_test.h"
    ["TestAddFormationDialog"]="hierarchytreetest/addformationdialog_test.h"
    ["TestAddItemDialog"]="hierarchytreetest/additemdialog_test.h"
    ["TestContextMenu"]="hierarchytreetest/contextmenu_test.h"
    ["TestHierarchyTree"]="hierarchytreetest/gui_hierarchytree_test.h"
    ["TestInspector"]="inspectortest/inspectortest.h"
    ["TestLayerPanel"]="layerpaneltest/layerpanel_test.h"
    ["TestMainWindow"]="mainwindowtest/mainwindow_test.h"
    ["TestMeasureDistanceDialog"]="measuredistancedialogtest/measuredistancedialog_test.h"
    ["TestMissionEditor"]="missioneditortest/gui_mission_test.h"
    ["TestNavigationPage"]="navigationpagetest/navigationpage_test.h"
    ["TestProfileInfoDialog"]="profileinfodialogtest/profileinfodialog_test.h"
    ["TestRecentProjectsManager"]="recentprojectsmanagertest/recentprojectsmanager_test.h"
    ["TestRuntimeToolBar"]="runtimetoolbartest/gui_runtimetoolbar_test.h"
    ["TestScenarioEditor"]="scenarioeditortest/scenarioeditor_test.h"
    ["TestSidebarWidget"]="sidebarwidgettest/sidebarwidget_test.h"
    ["TestStatusBar"]="statusbartest/statusbar_test.h"
    ["TestTacticalDisplay"]="tacticaldisplaytest/tacticaldisplay_test.h"
    ["TestTacticalRules"]="tacticalrulestest/tacticalrules_test.h"
    ["TestTestScriptDialog"]="testscriptdialogtest/testscriptdialog_test.h"
    ["TestTextScriptWidget"]="textscriptwidgettest/textscriptwidget_test.h"
    ["TestTooltipHelper"]="tooltiphelpertest/tooltiphelper_test.h"
    ["TestCanvasWidget"]="canvaswidgettest/canvaswidget_test.h"
    ["TestRuntimeEditor"]="runtimeeditortest/runtimeeditor_test.h"
)

show_help() {
    echo "======================================"
    echo "       TDF Test Runner"
    echo "======================================"
    echo ""
    echo "Usage:"
    echo "  ./run_tests.sh                          -> Sab tests run karo"
    echo "  ./run_tests.sh list                     -> Available tests dekho"
    echo "  ./run_tests.sh TestInspector            -> Sirf ek class run karo"
    echo "  ./run_tests.sh TestInspector::testLockFunctionality  -> Ek function"
    echo ""
}

list_tests() {
    echo "======================================"
    echo "  Available Test Classes:"
    echo "======================================"
    for key in $(echo "${!TEST_HEADERS[@]}" | tr ' ' '\n' | sort); do
        echo "  - $key"
    done
    echo ""
    echo "Example:"
    echo "  ./run_tests.sh TestInspector"
    echo "  ./run_tests.sh TestInspector::testLockFunctionality"
}

run_all_tests() {
    echo "======================================"
    echo "  Running ALL Tests"
    echo "======================================"
    $BINARY
}

run_specific_test() {
    INPUT=$1
    CLASS_NAME=$(echo $INPUT | cut -d: -f1)
    FUNC_NAME=$(echo $INPUT | grep '::' | cut -d: -f3)

    # Check valid class
    if [ -z "${TEST_HEADERS[$CLASS_NAME]}" ]; then
        echo "ERROR: '$CLASS_NAME' nahi mila!"
        echo "Sahi naam ke liye: ./run_tests.sh list"
        exit 1
    fi

    HEADER="${TEST_HEADERS[$CLASS_NAME]}"

    echo "======================================"
    echo "  Running: $INPUT"
    echo "======================================"

    # Backup
    cp $TESTS_DIR/main_test.cpp $TESTS_DIR/main_test_backup.cpp

    # New main likhein
    cat > $TESTS_DIR/main_test.cpp << MAINEOF
#include <QApplication>
#include <QTest>
#include "${HEADER}"

class RuntimeToolBar;
class Console;
void runRuntimeToolBarTests(RuntimeToolBar*, Console*) {}
void runDesignToolBarTests(void*, void*) {}
void runMissionEditorTests(void*, void*) {}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ${CLASS_NAME} testObj;
    return QTest::qExec(&testObj, argc, argv);
}
MAINEOF

    # Rebuild
    echo "Building..."
    make -j4 -C $BUILD_DIR
    echo ""

    # Run - function filter ke saath ya bina
    if [ -n "$FUNC_NAME" ]; then
        echo "Running function: $FUNC_NAME"
        $BINARY $FUNC_NAME
    else
        $BINARY
    fi

    # Restore original
    echo ""
    echo "Restoring original main_test.cpp..."
    cp $TESTS_DIR/main_test_backup.cpp $TESTS_DIR/main_test.cpp
    make -j4 -C $BUILD_DIR > /dev/null 2>&1
    echo "Done!"
}

# Main
case "$1" in
    "")         run_all_tests ;;
    "list")     list_tests ;;
    "help")     show_help ;;
    *)          run_specific_test $1 ;;
esac
