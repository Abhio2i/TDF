#ifndef NAVIGATIONPAGE_TEST_H
#define NAVIGATIONPAGE_TEST_H

#include <QObject>

class NavigationPage;

class TestNavigationPage : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();


    void testButtonsExist();
    void testButtonLabels();
    void testDefaultActiveButton();
    void testButtonProperties();
    void testButtonsEnabled();
    void testLayout();

private:
    NavigationPage* navPage = nullptr;
};

#endif
