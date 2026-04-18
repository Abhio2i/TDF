#ifndef TEXTSCRIPTWIDGET_TEST_H
#define TEXTSCRIPTWIDGET_TEST_H

#include <QObject>

class TextScriptWidget;

class TestTextScriptWidget : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasicProperties();
    void testUIElementsExist();
    void testScriptLoading();
    void testSignalsExist();
    void testContextMenuActions();
    void testAddScriptButton();
    void testItemWidgetCreation();
    void testStatusIconUpdateMethod();
    void testDirectoryPathHandling();

private:
    TextScriptWidget* widget = nullptr;
};

#endif
