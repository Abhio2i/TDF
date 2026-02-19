
// नया हेडर फाइल: TimingGraphDialog.h
#ifndef TIMINGGRAPHDIALOG_H
#define TIMINGGRAPHDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "graphwidget.h"

class TimingGraphDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TimingGraphDialog(QWidget *parent = nullptr);
    void setHierarchy(Hierarchy* hier);
    GraphWidget* getGraphWidget() { return graphWidget; }

private:
    GraphWidget* graphWidget = nullptr;
    QTableWidget* tableWidget = nullptr;

    void updateTable();
};

#endif
