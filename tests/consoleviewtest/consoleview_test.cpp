#include "consoleview_test.h"
#include "GUI/Console/consoleview.h"
#include "core/Debug/console.h"
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QRegularExpression>

#define CV_TEST(condition, msg) \
do { \
        if (condition) { \
            console->log(std::string("[PASS] ") + msg); \
    } else { \
            console->error(std::string("[FAIL] ") + msg); \
            testFailed = true; \
    } \
} while(0)

    void runConsoleViewTests(ConsoleView* consoleView, Console* console)
{
    if (!consoleView || !console) {
        if (console) console->error("ConsoleView or Console is null, cannot run tests");
        return;
    }

    bool testFailed = false;

    console->log(std::string("\n========================================="));
    console->log(std::string("       CONSOLE VIEW UNIT TESTS           "));
    console->log(std::string("=========================================\n"));

    // ----- Test 1: Basic widget properties -----
    CV_TEST(consoleView->isVisible(), "ConsoleView is visible");
    CV_TEST(consoleView->layout() != nullptr, "ConsoleView has a layout");

    // ----- Test 2: Tab widget exists and has correct tabs -----
    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    CV_TEST(tabWidget != nullptr, "Tab widget exists");
    if (tabWidget) {
        int tabCount = tabWidget->count();
        CV_TEST(tabCount == 5, "Tab widget has 5 tabs (Console, Error, Debug, Warning, Log)");
        QStringList expectedTabs = {"Console", "Error", "Debug", "Warning", "Log"};
        for (int i = 0; i < expectedTabs.size(); ++i) {
            CV_TEST(tabWidget->tabText(i) == expectedTabs[i],
                    QString("Tab %1 has correct label '%2'").arg(i).arg(expectedTabs[i]).toStdString().c_str());
        }
    }

    // ----- Test 3: Each console text edit exists -----
    QList<QTextEdit*> textEdits = consoleView->findChildren<QTextEdit*>();
    CV_TEST(textEdits.size() == 5, "All 5 console text edits exist");

    // ----- Test 4: Buttons exist -----
    QPushButton* clearButton = consoleView->findChild<QPushButton*>("", Qt::FindDirectChildrenOnly);
    // Find by text
    QPushButton* saveButton = nullptr;
    for (QPushButton* btn : consoleView->findChildren<QPushButton*>()) {
        if (btn->text() == "Save Log") saveButton = btn;
        if (btn->text() == "Clear") clearButton = btn;
    }
    CV_TEST(clearButton != nullptr, "Clear button exists");
    CV_TEST(saveButton != nullptr, "Save Log button exists");

    // ----- Test 5: Append text to general console -----
    QString testMsg = "Unit test message";
    consoleView->appendText(testMsg);
    QTextEdit* generalConsole = nullptr;
    for (QTextEdit* te : textEdits) {
        if (tabWidget && tabWidget->indexOf(te) == 0) { // first tab is general
            generalConsole = te;
            break;
        }
    }
    if (generalConsole) {
        QString content = generalConsole->toPlainText();
        CV_TEST(content.contains(testMsg), "appendText() adds message to general console");
    }

    // ----- Test 6: Append error to error console -----
    consoleView->appendError(testMsg);
    QTextEdit* errorConsole = nullptr;
    for (QTextEdit* te : textEdits) {
        if (tabWidget && tabWidget->indexOf(te) == 1) {
            errorConsole = te;
            break;
        }
    }
    if (errorConsole) {
        CV_TEST(errorConsole->toPlainText().contains(testMsg), "appendError() adds message to error console");
    }

    // ----- Test 7: Append debug to debug console -----
    consoleView->appendDebug(testMsg);
    QTextEdit* debugConsole = nullptr;
    for (QTextEdit* te : textEdits) {
        if (tabWidget && tabWidget->indexOf(te) == 2) {
            debugConsole = te;
            break;
        }
    }
    if (debugConsole) {
        CV_TEST(debugConsole->toPlainText().contains(testMsg), "appendDebug() adds message to debug console");
    }

    // ----- Test 8: Append warning to warning console -----
    consoleView->appendWarning(testMsg);
    QTextEdit* warningConsole = nullptr;
    for (QTextEdit* te : textEdits) {
        if (tabWidget && tabWidget->indexOf(te) == 3) {
            warningConsole = te;
            break;
        }
    }
    if (warningConsole) {
        CV_TEST(warningConsole->toPlainText().contains(testMsg), "appendWarning() adds message to warning console");
    }

    // ----- Test 9: Append log to log console -----
    consoleView->appendLog(testMsg);
    QTextEdit* logConsole = nullptr;
    for (QTextEdit* te : textEdits) {
        if (tabWidget && tabWidget->indexOf(te) == 4) {
            logConsole = te;
            break;
        }
    }
    if (logConsole) {
        CV_TEST(logConsole->toPlainText().contains(testMsg), "appendLog() adds message to log console");
    }

    // ----- Test 10: Clear button clears the current console -----
    if (generalConsole && clearButton) {
        generalConsole->setPlainText("Some text to clear");
        clearButton->click();
        CV_TEST(generalConsole->toPlainText().isEmpty(), "Clear button clears current console");
    }

    // ----- Test 11: Timestamp appears in appended messages -----
    if (generalConsole) {
        QString content = generalConsole->toPlainText();
        // Timestamp format: [hh:mm:ss]
        QRegularExpression regex(R"(\[\d{2}:\d{2}:\d{2}\])");
        CV_TEST(content.contains(regex), "Appended messages include timestamp");
    }

    // ----- Test 12: Save button exists and is enabled (we cannot test file dialog without mocking) -----
    if (saveButton) {
        CV_TEST(saveButton->isEnabled(), "Save Log button is enabled");
    }

    // ----- Test 13: Console dock setter (just check it doesn't crash) -----
    QDockWidget* dummyDock = new QDockWidget();
    consoleView->setConsoleDock(dummyDock);
    CV_TEST(true, "setConsoleDock() does not crash");
    delete dummyDock;

    // Final summary
    console->log(std::string("========================================="));
    if (testFailed)
        console->error(std::string("CONSOLE VIEW TESTS: Some tests FAILED."));
    else
        console->log(std::string("CONSOLE VIEW TESTS: ALL PASSED."));
    console->log(std::string("=========================================\n"));
}
