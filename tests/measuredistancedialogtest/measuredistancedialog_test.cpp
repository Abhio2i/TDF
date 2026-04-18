#include "measuredistancedialog_test.h"
#include "GUI/measuredistance/measuredistancedialog.h"
#include <QTest>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>

void TestMeasureDistanceDialog::init()
{
    dialog = new MeasureDistanceDialog(nullptr);
}

void TestMeasureDistanceDialog::cleanup()
{
    delete dialog;
    dialog = nullptr;
}

void TestMeasureDistanceDialog::testWindowTitle()
{
    QCOMPARE(dialog->windowTitle(), QString("Measure Distance"));
}

void TestMeasureDistanceDialog::testMinimumSize()
{
    QVERIFY(dialog->minimumWidth() >= 400);
    QVERIFY(dialog->minimumHeight() >= 300);
}

void TestMeasureDistanceDialog::testListWidgetExists()
{
    QListWidget* listWidget = dialog->findChild<QListWidget*>();
    QVERIFY(listWidget != nullptr);
}

void TestMeasureDistanceDialog::testTotalLineEdit()
{
    QLineEdit* totalEdit = dialog->findChild<QLineEdit*>();
    QVERIFY(totalEdit != nullptr);
    QVERIFY(totalEdit->isReadOnly());
    QVERIFY(totalEdit->placeholderText().contains("0.000"));
}

void TestMeasureDistanceDialog::testUnitCombo()
{
    QComboBox* unitCombo = dialog->findChild<QComboBox*>();
    QVERIFY(unitCombo != nullptr);
    QCOMPARE(unitCombo->count(), 5);
    QStringList expected = {"meters", "kilometers", "feet", "miles", "degrees"};
    for (int i = 0; i < expected.size(); ++i) {
        QCOMPARE(unitCombo->itemText(i), expected[i]);
    }
}

void TestMeasureDistanceDialog::testEllipsoidalRadio()
{
    QRadioButton* ellipsoidal = nullptr;
    for (QRadioButton* rb : dialog->findChildren<QRadioButton*>()) {
        if (rb->text() == "Ellipsoidal") {
            ellipsoidal = rb;
            break;
        }
    }
    QVERIFY(ellipsoidal != nullptr);
    QVERIFY(ellipsoidal->isChecked());
}

void TestMeasureDistanceDialog::testButtonsExist()
{
    QPushButton* newButton = nullptr;
    QPushButton* closeButton = nullptr;
    for (QPushButton* btn : dialog->findChildren<QPushButton*>()) {
        if (btn->text() == "New") newButton = btn;
        else if (btn->text() == "Close") closeButton = btn;
    }
    QVERIFY(newButton != nullptr);
    QVERIFY(closeButton != nullptr);
    QVERIFY(closeButton->isEnabled());
}

void TestMeasureDistanceDialog::testHelperMethods()
{
    // Test default conversion factor and unit string
    QVERIFY(qFuzzyCompare(dialog->getCurrentConversionFactor(), 1.0));
    QCOMPARE(dialog->getCurrentUnitString(), QString("m"));
}
