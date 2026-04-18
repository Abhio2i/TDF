#ifndef CUSTOMDOCK_TEST_H
#define CUSTOMDOCK_TEST_H

#include <QObject>

class CustomResizableOverlayDock;

class TestCustomResizableOverlayDock : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic properties
    void testDefaultHandlePosition();
    void testTitleBarWidgetExists();
    void testTitleLabelExists();
    void testLockAndCloseButtonsExist();

    // Lock button functionality
    void testLockButtonInitiallyHidden();
    void testEnableLockButton();
    void testSetLocked();
    void testLockButtonClick();

    // Widget management
    void testSetWidget();
    void testSetWindowTitleUpdatesLabel();
    void testHandlePosSetter();

    // Event & paint tests
    void testPaintEventDoesNotCrash();
    void testGeometryChangeDoesNotCrash();

private:
    CustomResizableOverlayDock* dock = nullptr;
};

#endif
