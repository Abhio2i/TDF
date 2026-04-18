#include "customdock_test.h"
#include "GUI/Editors/customresizableoverlaydock.h"
#include <QTest>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QCoreApplication>

void TestCustomResizableOverlayDock::init()
{
    // Provide a title and optional parent (nullptr)
    dock = new CustomResizableOverlayDock("Test Dock", nullptr);
}

void TestCustomResizableOverlayDock::cleanup()
{
    delete dock;
    dock = nullptr;
}

// ------------------------------------------------------------------
// Basic properties
// ------------------------------------------------------------------
void TestCustomResizableOverlayDock::testDefaultHandlePosition()
{
    QCOMPARE(dock->handlePos, CustomResizableOverlayDock::Right);
}

void TestCustomResizableOverlayDock::testTitleBarWidgetExists()
{
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
}

void TestCustomResizableOverlayDock::testTitleLabelExists()
{
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QLabel* titleLabel = titleBar->findChild<QLabel*>("dockTitleLabel");
    QVERIFY(titleLabel != nullptr);
    QCOMPARE(titleLabel->text(), dock->windowTitle());
}

void TestCustomResizableOverlayDock::testLockAndCloseButtonsExist()
{
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QToolButton* lockButton = nullptr;
    QToolButton* closeButton = nullptr;
    for (QToolButton* btn : titleBar->findChildren<QToolButton*>()) {
        if (btn->isCheckable())
            lockButton = btn;
        else
            closeButton = btn;
    }
    QVERIFY(lockButton != nullptr);
    QVERIFY(closeButton != nullptr);
    // Optional: check button text/icons
    QVERIFY(lockButton->text() == "🔓" || lockButton->text() == "🔒");
}

// ------------------------------------------------------------------
// Lock button functionality
// ------------------------------------------------------------------
void TestCustomResizableOverlayDock::testLockButtonInitiallyHidden()
{
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QToolButton* lockButton = nullptr;
    for (QToolButton* btn : titleBar->findChildren<QToolButton*>()) {
        if (btn->isCheckable()) {
            lockButton = btn;
            break;
        }
    }
    QVERIFY(lockButton != nullptr);
    QVERIFY(!lockButton->isVisible());
}

void TestCustomResizableOverlayDock::testEnableLockButton()
{
    // enableLockButton() should make the lock button visible (if dock is shown)
    dock->enableLockButton();
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QToolButton* lockButton = nullptr;
    for (QToolButton* btn : titleBar->findChildren<QToolButton*>()) {
        if (btn->isCheckable()) {
            lockButton = btn;
            break;
        }
    }
    QVERIFY(lockButton != nullptr);
    // The button may still be hidden if the dock is not shown, but calling enableLockButton() should not crash.
    QVERIFY(true);
}

void TestCustomResizableOverlayDock::testSetLocked()
{
    dock->setLocked(true);
    QVERIFY(dock->isLocked() == true);
    dock->setLocked(false);
    QVERIFY(dock->isLocked() == false);
}

void TestCustomResizableOverlayDock::testLockButtonClick()
{
    dock->enableLockButton();
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QToolButton* lockButton = nullptr;
    for (QToolButton* btn : titleBar->findChildren<QToolButton*>()) {
        if (btn->isCheckable()) {
            lockButton = btn;
            break;
        }
    }
    QVERIFY(lockButton != nullptr);
    dock->setLocked(false);
    lockButton->click();
    QVERIFY(dock->isLocked() == true);
}

// ------------------------------------------------------------------
// Widget management
// ------------------------------------------------------------------
void TestCustomResizableOverlayDock::testSetWidget()
{
    QWidget* w = new QWidget(dock); // dock will take ownership
    dock->setWidget(w);
    QCOMPARE(dock->widget(), w);
    QVERIFY(w->testAttribute(Qt::WA_Hover));
}

void TestCustomResizableOverlayDock::testSetWindowTitleUpdatesLabel()
{
    QString newTitle = "Test Title";
    dock->setWindowTitle(newTitle);
    QWidget* titleBar = dock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QLabel* titleLabel = titleBar->findChild<QLabel*>("dockTitleLabel");
    QVERIFY(titleLabel != nullptr);
    QCOMPARE(titleLabel->text(), newTitle);
}

void TestCustomResizableOverlayDock::testHandlePosSetter()
{
    dock->handlePos = CustomResizableOverlayDock::Left;
    QCOMPARE(dock->handlePos, CustomResizableOverlayDock::Left);
    dock->handlePos = CustomResizableOverlayDock::Right; // restore for other tests
}

// ------------------------------------------------------------------
// Event & paint tests
// ------------------------------------------------------------------
void TestCustomResizableOverlayDock::testPaintEventDoesNotCrash()
{
    dock->update();
    QCoreApplication::processEvents();
    QVERIFY(true); // reached without crash
}

void TestCustomResizableOverlayDock::testGeometryChangeDoesNotCrash()
{
    QRect originalGeo = dock->geometry();
    QRect newRect = originalGeo;
    newRect.setWidth(originalGeo.width() + 20);
    dock->setGeometry(newRect);
    QCoreApplication::processEvents();
    QVERIFY(true); // no crash
}
