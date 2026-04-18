#include "consoleview_test.h"
#include "GUI/Console/consoleview.h"
#include <QTest>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QDockWidget>
#include <QRegularExpression>

void TestConsoleView::init()
{
    consoleView = new ConsoleView();
}

void TestConsoleView::cleanup()
{
    delete consoleView;
    consoleView = nullptr;
}

// ------------------------------------------------------------------
// UI structure tests
// ------------------------------------------------------------------
void TestConsoleView::testHasLayout()
{
    QVERIFY(consoleView->layout() != nullptr);
}

void TestConsoleView::testTabWidgetExists()
{
    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QCOMPARE(tabWidget->count(), 5);
}

void TestConsoleView::testTabLabels()
{
    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QStringList expected = {"Console", "Error", "Debug", "Warning", "Log"};
    for (int i = 0; i < expected.size(); ++i) {
        QCOMPARE(tabWidget->tabText(i), expected[i]);
    }
}

void TestConsoleView::testTextEditsExist()
{
    QList<QTextEdit*> textEdits = consoleView->findChildren<QTextEdit*>();
    QCOMPARE(textEdits.size(), 5);
}

void TestConsoleView::testButtonsExist()
{
    QPushButton* clearButton = nullptr;
    QPushButton* saveButton = nullptr;
    for (QPushButton* btn : consoleView->findChildren<QPushButton*>()) {
        if (btn->text() == "Clear") clearButton = btn;
        if (btn->text() == "Save Log") saveButton = btn;
    }
    QVERIFY(clearButton != nullptr);
    QVERIFY(saveButton != nullptr);
}

// ------------------------------------------------------------------
// Functional tests
// ------------------------------------------------------------------
void TestConsoleView::testAppendText()
{
    QString testMsg = "Unit test message";
    consoleView->appendText(testMsg);

    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* generalConsole = qobject_cast<QTextEdit*>(tabWidget->widget(0));
    QVERIFY(generalConsole != nullptr);
    QVERIFY(generalConsole->toPlainText().contains(testMsg));
}

void TestConsoleView::testAppendError()
{
    QString testMsg = "Error test";
    consoleView->appendError(testMsg);

    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* errorConsole = qobject_cast<QTextEdit*>(tabWidget->widget(1));
    QVERIFY(errorConsole != nullptr);
    QVERIFY(errorConsole->toPlainText().contains(testMsg));
}

void TestConsoleView::testAppendDebug()
{
    QString testMsg = "Debug test";
    consoleView->appendDebug(testMsg);

    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* debugConsole = qobject_cast<QTextEdit*>(tabWidget->widget(2));
    QVERIFY(debugConsole != nullptr);
    QVERIFY(debugConsole->toPlainText().contains(testMsg));
}

void TestConsoleView::testAppendWarning()
{
    QString testMsg = "Warning test";
    consoleView->appendWarning(testMsg);

    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* warningConsole = qobject_cast<QTextEdit*>(tabWidget->widget(3));
    QVERIFY(warningConsole != nullptr);
    QVERIFY(warningConsole->toPlainText().contains(testMsg));
}

void TestConsoleView::testAppendLog()
{
    QString testMsg = "Log test";
    consoleView->appendLog(testMsg);

    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* logConsole = qobject_cast<QTextEdit*>(tabWidget->widget(4));
    QVERIFY(logConsole != nullptr);
    QVERIFY(logConsole->toPlainText().contains(testMsg));
}

void TestConsoleView::testTimestampInMessages()
{
    // Clear first
    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* generalConsole = qobject_cast<QTextEdit*>(tabWidget->widget(0));
    QVERIFY(generalConsole != nullptr);
    generalConsole->clear();

    QString testMsg = "Timestamp test";
    consoleView->appendText(testMsg);
    QString content = generalConsole->toPlainText();
    QRegularExpression regex(R"(\[\d{2}:\d{2}:\d{2}\])");
    QVERIFY(content.contains(regex));
}

void TestConsoleView::testClearButton()
{
    QTabWidget* tabWidget = consoleView->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QTextEdit* generalConsole = qobject_cast<QTextEdit*>(tabWidget->widget(0));
    QVERIFY(generalConsole != nullptr);

    generalConsole->setPlainText("Some text to clear");
    QPushButton* clearButton = nullptr;
    for (QPushButton* btn : consoleView->findChildren<QPushButton*>()) {
        if (btn->text() == "Clear") {
            clearButton = btn;
            break;
        }
    }
    QVERIFY(clearButton != nullptr);
    clearButton->click();
    QVERIFY(generalConsole->toPlainText().isEmpty());
}

void TestConsoleView::testSaveButtonEnabled()
{
    QPushButton* saveButton = nullptr;
    for (QPushButton* btn : consoleView->findChildren<QPushButton*>()) {
        if (btn->text() == "Save Log") {
            saveButton = btn;
            break;
        }
    }
    QVERIFY(saveButton != nullptr);
    QVERIFY(saveButton->isEnabled());
}

void TestConsoleView::testSetConsoleDock()
{
    QDockWidget* dummyDock = new QDockWidget();
    // Should not crash
    consoleView->setConsoleDock(dummyDock);
    delete dummyDock;
    QVERIFY(true);
}
