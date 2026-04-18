#include "sidebarwidget_test.h"
#include "GUI/Sidebar/sidebarwidget.h"
#include <QTest>
#include <QPushButton>
#include <QButtonGroup>
#include <QHBoxLayout>

void TestSidebarWidget::init()
{
    widget = new SidebarWidget(nullptr);
    widget->show(); // ensure visibility for layout checks
    QTest::qWait(50);
}

void TestSidebarWidget::cleanup()
{
    delete widget;
    widget = nullptr;
}

void TestSidebarWidget::testBasicProperties()
{
    QVERIFY(widget->isVisible());
    QCOMPARE(widget->height(), 28);
}

void TestSidebarWidget::testLayout()
{
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(widget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->spacing(), 1);
    QCOMPARE(layout->contentsMargins().left(), 0);
}

void TestSidebarWidget::testButtonsExist()
{
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    QVERIFY(buttons.size() >= 4);

    QStringList expected = {"Sensors", "Library", "Inspector", "TestScript"};
    int found = 0;
    for (const QString& exp : expected) {
        for (QPushButton* btn : buttons) {
            if (btn->text() == exp) {
                found++;
                break;
            }
        }
    }
    QCOMPARE(found, expected.size());
}

void TestSidebarWidget::testButtonProperties()
{
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        QVERIFY(btn->isCheckable());
        QString viewName = btn->property("viewName").toString();
        QVERIFY(!viewName.isEmpty());
    }
}

void TestSidebarWidget::testButtonGroup()
{
    QButtonGroup* group = widget->findChild<QButtonGroup*>();
    QVERIFY(group != nullptr);
    QVERIFY(group->exclusive());
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    QCOMPARE(group->buttons().size(), buttons.size());
}

void TestSidebarWidget::testSetActiveButton()
{
    widget->setActiveButton("Library");
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    bool libraryChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Library" && btn->isChecked()) {
            libraryChecked = true;
            break;
        }
    }
    QVERIFY(libraryChecked);
    widget->setActiveButton("Inspector");
    bool inspectorChecked = false;
    bool libraryStillChecked = false;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Inspector" && btn->isChecked()) inspectorChecked = true;
        if (btn->text() == "Library" && btn->isChecked()) libraryStillChecked = true;
    }
    QVERIFY(inspectorChecked);
    QVERIFY(!libraryStillChecked);
}

void TestSidebarWidget::testSignalsExist()
{
    const QMetaObject* mo = widget->metaObject();
    QVERIFY(mo->indexOfSignal("viewSelected(QString)") != -1);
}

void TestSidebarWidget::testSensorsButtonVisibility()
{
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    QPushButton* sensorsBtn = nullptr;
    for (QPushButton* btn : buttons) {
        if (btn->text() == "Sensors") {
            sensorsBtn = btn;
            break;
        }
    }
    QVERIFY(sensorsBtn != nullptr);

    widget->setSensorsButtonVisible(false);
    QVERIFY(!sensorsBtn->isVisible());
    widget->setSensorsButtonVisible(true);
    QVERIFY(sensorsBtn->isVisible());
}

void TestSidebarWidget::testStyleSheets()
{
    QVERIFY(!widget->styleSheet().isEmpty());
    QList<QPushButton*> buttons = widget->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        QVERIFY(!btn->styleSheet().isEmpty());
    }
}
