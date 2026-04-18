#ifndef SIDEBARWIDGET_TEST_H
#define SIDEBARWIDGET_TEST_H

#include <QObject>

class SidebarWidget;

class TestSidebarWidget : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasicProperties();
    void testLayout();
    void testButtonsExist();
    void testButtonProperties();
    void testButtonGroup();
    void testSetActiveButton();
    void testSignalsExist();
    void testSensorsButtonVisibility();
    void testStyleSheets();

private:
    SidebarWidget* widget = nullptr;
};

#endif
