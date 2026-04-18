#ifndef MEASUREDISTANCEDIALOG_TEST_H
#define MEASUREDISTANCEDIALOG_TEST_H

#include <QObject>

class MeasureDistanceDialog;

class TestMeasureDistanceDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testWindowTitle();
    void testMinimumSize();
    void testListWidgetExists();
    void testTotalLineEdit();
    void testUnitCombo();
    void testEllipsoidalRadio();
    void testButtonsExist();
    void testHelperMethods();

private:
    MeasureDistanceDialog* dialog = nullptr;
};

#endif
