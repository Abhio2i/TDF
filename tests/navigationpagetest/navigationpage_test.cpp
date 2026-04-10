#include "navigationpage_test.h"
#include "GUI/Navigation/navigationpage.h"
#include "core/Debug/console.h"
#include <QToolButton>
#include <QLayout>
#include <QDebug>

#define NAV_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runNavigationPageTests(NavigationPage* navPage, Console* console)
{
    if (!navPage || !console) {
        if (console) console->error("NavigationPage or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("      NAVIGATION PAGE UNIT TESTS         "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Basic properties -----
    NAV_TEST(navPage->isVisible(), "Navigation page is visible");
    NAV_TEST(navPage->height() == 50, "Navigation page height is 50 pixels");

    // ----- Test 2: All buttons exist -----
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    NAV_TEST(buttons.size() >= 5, "At least 5 navigation buttons exist");

    // Check button texts
    QStringList expectedTexts = {"Database", "Scenario", "Mission", "Runtime", "Analysis/Reports"};
    int foundCount = 0;
    for (const QString& expected : expectedTexts) {
        for (QToolButton* btn : buttons) {
            if (btn->text() == expected) {
                foundCount++;
                break;
            }
        }
    }
    NAV_TEST(foundCount == expectedTexts.size(), "All expected buttons present with correct labels");

    // ----- Test 3: Default active button is Database (without clicking) -----
    QToolButton* databaseBtn = nullptr;
    for (QToolButton* btn : buttons) {
        if (btn->text() == "Database") {
            databaseBtn = btn;
            break;
        }
    }
    if (databaseBtn) {
        QString style = databaseBtn->styleSheet();
        bool isActive = style.contains("#0d6efd") || style.contains("background-color: #0d6efd");
        NAV_TEST(isActive, "Database button is active by default");
    } else {
        NAV_TEST(false, "Database button not found");
    }

    // Check that a non-active button (e.g., Scenario) has different style
    QToolButton* scenarioBtn = nullptr;
    for (QToolButton* btn : buttons) {
        if (btn->text() == "Scenario") {
            scenarioBtn = btn;
            break;
        }
    }
    if (scenarioBtn) {
        QString style = scenarioBtn->styleSheet();
        bool isNotActive = !style.contains("#0d6efd") && !style.contains("background-color: #0d6efd");
        NAV_TEST(isNotActive, "Scenario button is not active initially");
    }

    // ----- Test 4: Button properties (icon, size, cursor) -----
    if (databaseBtn) {
        NAV_TEST(!databaseBtn->icon().isNull(), "Database button has icon");
        NAV_TEST(databaseBtn->minimumWidth() >= 100, "Button minimum width >= 100");
        NAV_TEST(databaseBtn->cursor().shape() == Qt::PointingHandCursor, "Button has pointing hand cursor");
    }

    // ----- Test 5: All buttons are enabled -----
    bool allEnabled = true;
    for (QToolButton* btn : buttons) {
        if (!btn->isEnabled()) {
            allEnabled = false;
            break;
        }
    }
    NAV_TEST(allEnabled, "All buttons are enabled");

    // ----- Test 6: Layout margins and spacing -----
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(navPage->layout());
    if (layout) {
        NAV_TEST(layout->contentsMargins().left() == 15, "Layout left margin is 15");
        NAV_TEST(layout->spacing() == 5, "Layout spacing is 5");
    } else {
        NAV_TEST(false, "Navigation page has QHBoxLayout");
    }

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("NAVIGATION PAGE TESTS: Some tests FAILED."));
    else
        console->log(std::string("NAVIGATION PAGE TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
