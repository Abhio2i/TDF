#ifndef TESTSCRIPTDIALOG_TEST_H
#define TESTSCRIPTDIALOG_TEST_H

#include <QObject>

class TestScriptDialog;

class TestTestScriptDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDialogProperties();
    void testUIElementsExist();
    void testSignalsExist();
    void testInitialContent();
    void testNewScriptButton();
    void testCancelButtonEnabled();
    void testSyntaxHighlighterExists();
    void testAutocompleterExists();

private:
    TestScriptDialog* dialog = nullptr;
};

#endif
