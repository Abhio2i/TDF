#ifndef STATUSBAR_TEST_H
#define STATUSBAR_TEST_H

#include <QObject>

class StatusBar;

class TestStatusBar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasicExistence();
    void testSaveButton();
    void testFileNameLabel();
    void testSetFileName();
    void testClearFileName();
    void testSaveRequestedSignal();
    void testRamLabel();
    void testCompatibilityMethods();
    void testStyleSheet();

private:
    StatusBar* statusBar = nullptr;
};

#endif
