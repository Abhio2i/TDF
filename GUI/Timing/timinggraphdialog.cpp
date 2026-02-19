// TimingGraphDialog.cpp
#include "GUI/Timing/timinggraphdialog.h"
#include <QVBoxLayout>
#include <QHeaderView>

TimingGraphDialog::TimingGraphDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Timing Graph + Table");
    resize(800, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5,5,5,5);
    mainLayout->setSpacing(0);

    // 1. Table (ऊपर)
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"Name", "Start", "End", "Status"});
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->verticalHeader()->setVisible(false);

    mainLayout->addWidget(tableWidget, 1);     // stretch = 1

    // 2. Graph (नीचे)
    graphWidget = new GraphWidget(this);
    mainLayout->addWidget(graphWidget, 3);     // stretch = 3 → ग्राफ ज्यादा जगह ले

    setLayout(mainLayout);
}

void TimingGraphDialog::setHierarchy(Hierarchy* hier)
{
    if (graphWidget)
        graphWidget->setHierarchy(hier);
}

void TimingGraphDialog::updateTable()
{
    if (!graphWidget || !graphWidget->h) return;

    tableWidget->setRowCount(0);

    int row = 0;
    for (auto& [key, platform] : *graphWidget->h->Platforms)
    {
        if (!platform || !platform->dynamicModel) continue;

        double start = platform->dynamicModel->startTime;
        double end   = platform->dynamicModel->endTime < 0 ? graphWidget->t : platform->dynamicModel->endTime;

        QString status = (end >= graphWidget->t) ? "Running" : "Completed";

        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(key))); // या platform->name अगर है
        tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(start, 'f', 2)));
        tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(end,   'f', 2)));
        tableWidget->setItem(row, 3, new QTableWidgetItem(status));

        row++;
    }
}
