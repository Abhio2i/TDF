#ifndef APPLICATIONDIALOG_TEST_H
#define APPLICATIONDIALOG_TEST_H

#include <QObject>

class ApplicationDialog;

class TestApplicationDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // UI structure tests
    void testDialogProperties();

    void testDatabaseTabElements();
    void testOkCancelButtons();

    // Static getter/setter tests

    void testDeveloperMode();
    void testFpsSettings();
    void testImageSizeSettings();

    // Signal tests
    void testFpsStateSignal();
    void testDatabaseSettingsChangedSignal();

private:
    ApplicationDialog* dialog = nullptr;
};

#endif
