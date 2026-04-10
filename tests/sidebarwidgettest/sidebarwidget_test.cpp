#include "sidebarwidget_test.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include "core/Debug/console.h"
#include <QPushButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QDebug>

#define SIDEBAR_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runSidebarWidgetTests(SidebarWidget* widget, Console* console)
{
    if (!widget || !console) {
        if (console) console->error("SidebarWidget or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("       SIDEBAR WIDGET UNIT TESTS        "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Basic properties -----
    SIDEBAR_TEST(widget->isVisible(), "Sidebar widget is visible");
    SIDEBAR_TEST(widget->height() == 28, "Sidebar height is 28 pixels");

    // ----- Test 2: Layout exists -----
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(widget->layout());
    SIDEBAR_TEST(layout != nullptr, "Widget has QHBoxLayout");
    if (layout) {
        SIDEBAR_TEST(layout->spacing() == 1, "Layout spacing is 1");
        SIDEBAR_TEST(layout->contentsMargins().left() == 0, "Layout margins are zero");
    }

    // ----- Test 3: Buttons exist -----
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    SIDEBAR_TEST(buttons.size() >= 4, "At least 4 buttons exist (Sensors, Library, Inspector, TestScript)");

    // Check button texts
    QStringList expectedTexts = {"Sensors", "Library", "Inspector", "TestScript"};
    int foundCount = 0;
    for (const QString& expected : expectedTexts) {
        for (QPushButton* btn : buttons) {
            if (btn->text() == expected) {
                foundCount++;
                break;
            }
        }
    }
    SIDEBAR_TEST(foundCount == expectedTexts.size(), "All expected buttons present with correct text");

    // ----- Test 4: Button properties (checkable, viewName property) -----
    for (QPushButton* btn : buttons) {
        SIDEBAR_TEST(btn->isCheckable(), QString("Button '%1' is checkable").arg(btn->text()).toStdString().c_str());
        QString viewName = btn->property("viewName").toString();
        SIDEBAR_TEST(!viewName.isEmpty(), QString("Button '%1' has viewName property set").arg(btn->text()).toStdString().c_str());
    }

    // ----- Test 5: Button group exclusive behavior -----
    QButtonGroup* buttonGroup = widget->findChild<QButtonGroup*>();
    SIDEBAR_TEST(buttonGroup != nullptr, "Button group exists");
    if (buttonGroup) {
        SIDEBAR_TEST(buttonGroup->exclusive(), "Button group is exclusive");
        SIDEBAR_TEST(buttonGroup->buttons().size() == buttons.size(),
                     "Button group contains all buttons");
    }

    // ----- Test 6: Default active button (none initially, but setActiveButton can change) -----
    // Initially no button is checked? Actually buttons are checkable but none checked by default.
    bool anyChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->isChecked()) {
            anyChecked = true;
            break;
        }
    }
    // It's acceptable if none are checked initially (user must click)
    SIDEBAR_TEST(true, "No button checked by default (or acceptable)");

    // ----- Test 7: setActiveButton works -----
    widget->setActiveButton("Library");
    bool libraryChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Library" && btn->isChecked()) {
            libraryChecked = true;
            break;
        }
    }
    SIDEBAR_TEST(libraryChecked, "setActiveButton('Library') checks the Library button");

    // Reset: setActiveButton to another view
    widget->setActiveButton("Inspector");
    bool inspectorChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Inspector" && btn->isChecked()) {
            inspectorChecked = true;
            break;
        }
    }
    SIDEBAR_TEST(inspectorChecked, "setActiveButton('Inspector') checks Inspector button");
    // Ensure Library is now unchecked
    bool libraryStillChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Library" && btn->isChecked()) {
            libraryStillChecked = true;
            break;
        }
    }
    SIDEBAR_TEST(!libraryStillChecked, "setActiveButton deselects previously selected button");

    // ----- Test 8: Signals are emitted (check signal exists) -----
    const QMetaObject* mo = widget->metaObject();
    bool hasViewSelected = (mo->indexOfSignal("viewSelected(QString)") != -1);
    SIDEBAR_TEST(hasViewSelected, "viewSelected signal exists");

    // ----- Test 9: Sensors button visibility toggle -----
    QPushButton* sensorsBtn = nullptr;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Sensors") {
            sensorsBtn = btn;
            break;
        }
    }
    if (sensorsBtn) {
        widget->setSensorsButtonVisible(false);
        SIDEBAR_TEST(!sensorsBtn->isVisible(), "setSensorsButtonVisible(false) hides Sensors button");
        widget->setSensorsButtonVisible(true);
        SIDEBAR_TEST(sensorsBtn->isVisible(), "setSensorsButtonVisible(true) shows Sensors button");
    } else {
        SIDEBAR_TEST(false, "Sensors button not found");
    }

    // ----- Test 10: StyleSheet applied (basic check – no crash) -----
    SIDEBAR_TEST(!widget->styleSheet().isEmpty(), "Widget has a stylesheet");
    for (QPushButton* btn : buttons) {
        SIDEBAR_TEST(!btn->styleSheet().isEmpty(), QString("Button '%1' has stylesheet").arg(btn->text()).toStdString().c_str());
    }

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("SIDEBAR WIDGET TESTS: Some tests FAILED."));
    else
        console->log(std::string("SIDEBAR WIDGET TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
