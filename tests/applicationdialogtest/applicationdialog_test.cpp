#include "applicationdialog_test.h"
#include "GUI/Settings/applicationdialog.h"
#include <QTest>
#include <QSignalSpy>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>

void TestApplicationDialog::init()
{
    dialog = new ApplicationDialog();
}

void TestApplicationDialog::cleanup()
{
    delete dialog;
    dialog = nullptr;
}

// ------------------------------------------------------------------
// UI structure tests
// ------------------------------------------------------------------
void TestApplicationDialog::testDialogProperties()
{
    QCOMPARE(dialog->windowTitle(), QString("Application Settings"));
    QVERIFY(dialog->isModal());
    QCOMPARE(dialog->size().width(), 430);
    QCOMPARE(dialog->size().height(), 400);
}




void TestApplicationDialog::testDatabaseTabElements()
{
    QTabWidget* tabWidget = dialog->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QWidget* dbTab = tabWidget->widget(1);
    QVERIFY(dbTab != nullptr);

    QCheckBox* dbEnabledCheck = dbTab->findChild<QCheckBox*>();
    QVERIFY(dbEnabledCheck != nullptr);

    QLineEdit* dbPathEdit = dbTab->findChild<QLineEdit*>();
    QVERIFY(dbPathEdit != nullptr);

    QList<QPushButton*> btns = dbTab->findChildren<QPushButton*>();
    bool hasBrowse = false, hasReset = false;
    for (QPushButton* btn : btns) {
        if (btn->text() == "Browse…") hasBrowse = true;
        if (btn->text() == "Reset Path") hasReset = true;
    }
    QVERIFY(hasBrowse);
    QVERIFY(hasReset);
}

void TestApplicationDialog::testOkCancelButtons()
{
    QList<QPushButton*> btns = dialog->findChildren<QPushButton*>();
    bool hasOk = false, hasCancel = false;
    for (QPushButton* btn : btns) {
        if (btn->text() == "OK") hasOk = true;
        if (btn->text() == "Cancel") hasCancel = true;
    }
    QVERIFY(hasOk);
    QVERIFY(hasCancel);
}

// ------------------------------------------------------------------
// Static getter/setter tests
// ------------------------------------------------------------------

void TestApplicationDialog::testDeveloperMode()
{
    bool orig = ApplicationDialog::getGlobalDeveloperMode();
    ApplicationDialog::setGlobalDeveloperMode(true);
    QVERIFY(ApplicationDialog::getGlobalDeveloperMode() == true);
    ApplicationDialog::setGlobalDeveloperMode(false);
    QVERIFY(ApplicationDialog::getGlobalDeveloperMode() == false);
    ApplicationDialog::setGlobalDeveloperMode(orig);
}

void TestApplicationDialog::testFpsSettings()
{
    int fps = ApplicationDialog::getGlobalFPS();
    QVERIFY(fps >= 1 && fps <= 1000);
    // Optionally test setter if exists, but not shown in original code
}

void TestApplicationDialog::testImageSizeSettings()
{
    QString imgSize = ApplicationDialog::getGlobalImageSize();
    QVERIFY(!imgSize.isEmpty());

    QString pixels = ApplicationDialog::getImageSizeInPixels();
    bool ok = false;
    int pixelVal = pixels.toInt(&ok);
    QVERIFY(ok && pixelVal > 0);
}

// ------------------------------------------------------------------
// Signal tests (using QSignalSpy)
// ------------------------------------------------------------------
void TestApplicationDialog::testFpsStateSignal()
{
    const QMetaObject* mo = dialog->metaObject();
    int signalIndex = mo->indexOfSignal("fpsState(int)");
    QVERIFY(signalIndex != -1);

    // Emit the signal (if possible through UI interaction)
    // For simplicity, we just check existence and that connecting doesn't crash
    QSignalSpy spy(dialog, SIGNAL(fpsState(int)));
    // In a real test, you'd trigger UI action that emits it.
    // Here we just verify the signal is declared.
    QVERIFY(spy.isValid());
}

void TestApplicationDialog::testDatabaseSettingsChangedSignal()
{
    const QMetaObject* mo = dialog->metaObject();
    int signalIndex = mo->indexOfSignal("databaseSettingsChanged(bool,QString)");
    QVERIFY(signalIndex != -1);

    QSignalSpy spy(dialog, SIGNAL(databaseSettingsChanged(bool,QString)));
    QVERIFY(spy.isValid());
}
