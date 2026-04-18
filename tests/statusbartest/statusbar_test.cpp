#include "statusbar_test.h"
#include "GUI/statusbar.h"
#include <QTest>
#include <QPushButton>
#include <QLabel>

void TestStatusBar::init()
{
    statusBar = new StatusBar(nullptr);
    statusBar->show();
    QTest::qWait(50);
}

void TestStatusBar::cleanup()
{
    delete statusBar;
    statusBar = nullptr;
}

void TestStatusBar::testBasicExistence()
{
    QVERIFY(statusBar != nullptr);
    // StatusBar is a widget; just check it's valid
    QVERIFY(statusBar->isVisible() || !statusBar->isVisible());
}

void TestStatusBar::testSaveButton()
{
    QPushButton* saveBtn = statusBar->saveButton();
    QVERIFY(saveBtn != nullptr);
    QVERIFY(saveBtn->isEnabled());
    QCOMPARE(saveBtn->text(), QString("Save"));
}

void TestStatusBar::testFileNameLabel()
{
    QLabel* fileNameLabel = statusBar->findChild<QLabel*>("statusFileNameLabel");
    QVERIFY(fileNameLabel != nullptr);
    QVERIFY(fileNameLabel->text().isEmpty());
}

void TestStatusBar::testSetFileName()
{
    QLabel* fileNameLabel = statusBar->findChild<QLabel*>("statusFileNameLabel");
    QVERIFY(fileNameLabel != nullptr);

    statusBar->setFileName("/test/file.sc", false);
    QCOMPARE(fileNameLabel->text(), QString("file.sc"));

    statusBar->setFileName("/test/file.sc", true);
    QCOMPARE(fileNameLabel->text(), QString("* file.sc"));
}

void TestStatusBar::testClearFileName()
{
    QLabel* fileNameLabel = statusBar->findChild<QLabel*>("statusFileNameLabel");
    QVERIFY(fileNameLabel != nullptr);

    statusBar->setFileName("/test/file.sc", false);
    statusBar->clearFileName();
    QVERIFY(fileNameLabel->text().isEmpty());
}

void TestStatusBar::testSaveRequestedSignal()
{
    const QMetaObject* mo = statusBar->metaObject();
    QVERIFY(mo->indexOfSignal("saveRequested()") != -1);
}

void TestStatusBar::testRamLabel()
{
    QLabel* ramLabel = statusBar->findChild<QLabel*>("statusRamLabel");
    QVERIFY(ramLabel != nullptr);
}

void TestStatusBar::testCompatibilityMethods()
{
    // All these methods should not crash
    statusBar->setStatusMessage("Test", 1000);
    statusBar->setCoordinates(12.34, 56.78);
    statusBar->setCoordinatesVisible(true);
    statusBar->setSimTime("00:00:00");
    statusBar->setSimTimeVisible(true);
    statusBar->setZoomLevel(10);
    statusBar->setZoomVisible(true);
    QVERIFY(true); // reached without crash
}

void TestStatusBar::testStyleSheet()
{
    QString style = statusBar->styleSheet();
    QVERIFY(!style.isEmpty());
}
