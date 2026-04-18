#ifndef CONSOLEVIEW_TEST_H
#define CONSOLEVIEW_TEST_H

#include <QObject>

class ConsoleView;

class TestConsoleView : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // UI structure tests
    void testHasLayout();
    void testTabWidgetExists();
    void testTabLabels();
    void testTextEditsExist();
    void testButtonsExist();

    // Functional tests
    void testAppendText();
    void testAppendError();
    void testAppendDebug();
    void testAppendWarning();
    void testAppendLog();
    void testTimestampInMessages();
    void testClearButton();
    void testSaveButtonEnabled();
    void testSetConsoleDock();

private:
    ConsoleView* consoleView = nullptr;
};

#endif
