/* =============================================================================
 * FILE:         colortemplate.cpp
 * MODULE:       Color Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the ColorTemplate class which provides a widget for
 *               managing color templates. It interfaces with the Inspector
 *               panel to set up color cells in a table, maintain connected
 *               entity IDs, template names, and emit value changes when a
 *               color is modified.
 *
 * REQUIREMENTS: Implements REQ-COLOR-010 through REQ-COLOR-013
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-COLOR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "vectortemplate.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QDoubleValidator>
#include <QtMath>
#include "GUI/Inspector/inspector-styles.h"

VectorTemplate::VectorTemplate(QWidget *parent) : QWidget(parent), inspectorRef(nullptr) {}

QString VectorTemplate::formatVectorNumber(double value)
{
    if (qFuzzyCompare(value, qRound(value))) {
        return QString::number(qRound(value));
    }
    QString result = QString::number(value, 'f', 6);
    if (result.contains('.')) {
        while (result.endsWith('0')) result.chop(1);
        if (result.endsWith('.')) result.chop(1);
    }
    return result;
}

void VectorTemplate::setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QVBoxLayout *vectorLayout = new QVBoxLayout(this);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setSpacing(2);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        QMenu contextMenu;
        QAction *copyVectorAction = contextMenu.addAction("Copy Vector");
        QAction *pasteVectorAction = contextMenu.addAction("Paste Vector");
        pasteVectorAction->setEnabled(!copiedVectorData.isEmpty());

        QMenu *copyComponentMenu = contextMenu.addMenu("Copy Component");
        QAction *copyXAction = copyComponentMenu->addAction("Copy X");
        QAction *copyYAction = copyComponentMenu->addAction("Copy Y");
        QAction *copyZAction = copyComponentMenu->addAction("Copy Z");

        QMenu *pasteComponentMenu = contextMenu.addMenu("Paste Component");
        QAction *pasteXAction = pasteComponentMenu->addAction("Paste X");
        pasteXAction->setEnabled(copiedVectorData.contains("x"));
        QAction *pasteYAction = pasteComponentMenu->addAction("Paste Y");
        pasteYAction->setEnabled(copiedVectorData.contains("y"));
        QAction *pasteZAction = pasteComponentMenu->addAction("Paste Z");
        pasteZAction->setEnabled(copiedVectorData.contains("z"));
        QAction *selectedAction = contextMenu.exec(mapToGlobal(pos));

        if (selectedAction == copyVectorAction) {
            copiedVectorData = QJsonObject();
            QStringList axes = {"x", "y", "z"};
            for (QObject *child : children()) {
                WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                if (line && axes.contains(line->objectName())) {
                    bool ok;
                    double value = line->text().toDouble(&ok);
                    if (ok) copiedVectorData[line->objectName()] = value;
                }
            }
        }
        else if (selectedAction == pasteVectorAction && !copiedVectorData.isEmpty()) {
            QJsonObject newVectorData = obj;
            bool updated = false;
            for (const QString& key : copiedVectorData.keys()) {
                if (newVectorData.contains(key)) {
                    double value = copiedVectorData[key].toDouble();
                    newVectorData[key] = value;
                    updated = true;
                    for (QObject *child : children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == key) {
                            line->setText(formatVectorNumber(value));
                            break;
                        }
                    }
                }
            }
            if (updated) {
                QJsonObject delta;
                delta[fullKey] = newVectorData;
                emit valueChanged(connectedID, name, delta);
            }
        }
        else if (selectedAction == copyXAction || selectedAction == copyYAction || selectedAction == copyZAction) {
            copiedVectorData = QJsonObject();
            QString target = selectedAction == copyXAction ? "x" : selectedAction == copyYAction ? "y" : "z";
            for (QObject *child : children()) {
                WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                if (line && line->objectName() == target) {
                    bool ok;
                    double value = line->text().toDouble(&ok);
                    if (ok) copiedVectorData[target] = value;
                    break;
                }
            }
        }
        else if ((selectedAction == pasteXAction && copiedVectorData.contains("x")) ||
                 (selectedAction == pasteYAction && copiedVectorData.contains("y")) ||
                 (selectedAction == pasteZAction && copiedVectorData.contains("z"))) {
            QString target = selectedAction == pasteXAction ? "x" : selectedAction == pasteYAction ? "y" : "z";
            QJsonObject newVectorData = obj;
            double value = copiedVectorData[target].toDouble();
            newVectorData[target] = value;
            for (QObject *child : children()) {
                WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                if (line && line->objectName() == target) {
                    line->setText(formatVectorNumber(value));
                    break;
                }
            }
            QJsonObject delta;
            delta[fullKey] = newVectorData;
            emit valueChanged(connectedID, name, delta);
        }
    });

    QStringList orderedKeys = {"x", "y", "z"};
    for (const QString &axis : orderedKeys) {
        if (!obj.contains(axis) || axis.contains("type")) continue;

        QHBoxLayout *axisLayout = new QHBoxLayout();
        axisLayout->setContentsMargins(0, 0, 0, 0);
        axisLayout->setSpacing(5);

        QLabel *lbl = new QLabel(axis + ":");
        lbl->setStyleSheet(InspectorStyles::VectorLabel);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        WheelableLineEdit *edit = new WheelableLineEdit();
        double axisValue = obj[axis].toDouble();
        edit->setText(formatVectorNumber(axisValue));
        edit->setStyleSheet(InspectorStyles::VectorInput);

        QDoubleValidator *validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setDecimals(6);
        edit->setValidator(validator);
        edit->setObjectName(axis);
        edit->setAlignment(Qt::AlignLeft);

        axisLayout->addWidget(lbl);
        axisLayout->addWidget(edit);
        axisLayout->addStretch();
        vectorLayout->addLayout(axisLayout);

        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            QJsonObject vectorDelta;
            for (QObject *child : children()) {
                WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                if (line) {
                    vectorDelta[line->objectName()] = line->text().toDouble();
                }
            }
            QJsonObject delta;
            delta[fullKey] = vectorDelta;
            emit valueChanged(connectedID, name, delta);
        });
    }

    vectorLayout->setAlignment(Qt::AlignLeft);
    int axesCount = orderedKeys.size();
    int rowHeight = 30 + (axesCount * 25);
    tableWidget->setRowHeight(row, rowHeight);
    tableWidget->setCellWidget(row, 1, this);
}
