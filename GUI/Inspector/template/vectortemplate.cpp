
#include "vectortemplate.h"                        // For vector template class
#include <QHBoxLayout>                             // For horizontal layout
#include <QLabel>                                  // For axis labels
#include <QMenu>                                   // For context menus
#include <QAction>                                 // For menu actions
#include <QDoubleValidator>                        // For input validation
#include <QtMath>                                  // For math functions

// %%% Constructor %%%
/* Initialize vector template widget */
VectorTemplate::VectorTemplate(QWidget *parent)
    : QWidget(parent)
    , inspectorRef(nullptr)
{

}

// %%% Utility Function %%%
/* Format number for vector display */
QString VectorTemplate::formatVectorNumber(double value)
{
    // Check for integer values
    if (qFuzzyCompare(value, qRound(value))) {
        return QString::number(qRound(value));
    }

    // Use fixed notation with 6 decimal places
    QString result = QString::number(value, 'f', 6);

    // Trim trailing zeros and decimal point if needed
    if (result.contains('.')) {
        while (result.endsWith('0')) {
            result.chop(1);
        }
        if (result.endsWith('.')) {
            result.chop(1);
        }
    }

    return result;
}
void VectorTemplate::setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{

    QVBoxLayout *vectorLayout = new QVBoxLayout(this);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setSpacing(2);

    // Enable context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        // Create context menu
        QMenu contextMenu;
        QAction *copyVectorAction = contextMenu.addAction("Copy Vector");
        QAction *pasteVectorAction = contextMenu.addAction("Paste Vector");
        pasteVectorAction->setEnabled(!copiedVectorData.isEmpty());

        // Create copy component submenu
        QMenu *copyComponentMenu = contextMenu.addMenu("Copy Component");
        QAction *copyXAction = copyComponentMenu->addAction("Copy X");
        QAction *copyYAction = copyComponentMenu->addAction("Copy Y");
        QAction *copyZAction = copyComponentMenu->addAction("Copy Z");

        // Create paste component submenu
        QMenu *pasteComponentMenu = contextMenu.addMenu("Paste Component");
        QAction *pasteXAction = pasteComponentMenu->addAction("Paste X");
        pasteXAction->setEnabled(copiedVectorData.contains("x"));
        QAction *pasteYAction = pasteComponentMenu->addAction("Paste Y");
        pasteYAction->setEnabled(copiedVectorData.contains("y"));
        QAction *pasteZAction = pasteComponentMenu->addAction("Paste Z");
        pasteZAction->setEnabled(copiedVectorData.contains("z"));

        // Execute context menu
        QAction *selectedAction = contextMenu.exec(mapToGlobal(pos));

        // Handle copy vector
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
        // Handle paste vector
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
        // Handle copy component
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
        // Handle paste component
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
    QStringList keys = obj.keys();
    // Process axes in order: x, y, z
    QStringList orderedKeys = {"x", "y", "z"};
    for (const QString &axis : orderedKeys) {
        if (!obj.contains(axis) || axis.contains("type")) continue;
        // Create horizontal layout for each axis
        QHBoxLayout *axisLayout = new QHBoxLayout();
        axisLayout->setContentsMargins(0, 0, 0, 0);
        axisLayout->setSpacing(5);
        QLabel *lbl = new QLabel(axis + ":");
        lbl->setStyleSheet("color: black;");
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        WheelableLineEdit *edit = new WheelableLineEdit();
        double axisValue = obj[axis].toDouble();
        edit->setText(formatVectorNumber(axisValue));
        QDoubleValidator *validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setDecimals(6);
        edit->setValidator(validator);
        edit->setObjectName(axis);
        edit->setStyleSheet(
            "QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; text-align: left; }"
            "QLineEdit:focus { border: 1px solid #007bff; }"
            );
        edit->setAlignment(Qt::AlignLeft);
        edit->setMinimumWidth(100);
        edit->setMaximumWidth(150);
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
