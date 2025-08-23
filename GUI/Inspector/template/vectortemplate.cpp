#include "vectortemplate.h"
#include <QHBoxLayout>
#include <QLabel>

VectorTemplate::VectorTemplate(QWidget *parent) : QWidget(parent) {}

void VectorTemplate::setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QHBoxLayout *vectorLayout = new QHBoxLayout(this);
    vectorLayout->setContentsMargins(0, 0, 0, 0);

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
        } else if (selectedAction == pasteVectorAction && !copiedVectorData.isEmpty()) {
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
                            line->setText(QString::number(value));
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
        } else if (selectedAction == copyXAction || selectedAction == copyYAction || selectedAction == copyZAction) {
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
        } else if ((selectedAction == pasteXAction && copiedVectorData.contains("x")) ||
                   (selectedAction == pasteYAction && copiedVectorData.contains("y")) ||
                   (selectedAction == pasteZAction && copiedVectorData.contains("z"))) {
            QString target = selectedAction == pasteXAction ? "x" : selectedAction == pasteYAction ? "y" : "z";
            QJsonObject newVectorData = obj;
            double value = copiedVectorData[target].toDouble();
            newVectorData[target] = value;
            for (QObject *child : children()) {
                WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                if (line && line->objectName() == target) {
                    line->setText(QString::number(value));
                    break;
                }
            }
            QJsonObject delta;
            delta[fullKey] = newVectorData;
            emit valueChanged(connectedID, name, delta);
        }
    });

    QStringList keys = obj.keys();
    for (const QString &axis : keys) {
        if (axis.contains("type")) continue;
        QLabel *lbl = new QLabel(axis);
        lbl->setStyleSheet("color: #aaa; min-width: 20px;");
        WheelableLineEdit *edit = new WheelableLineEdit();
        edit->setText(QString::number(obj[axis].toDouble()));
        QDoubleValidator *validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setDecimals(4);
        edit->setValidator(validator);
        edit->setObjectName(axis);
        edit->setStyleSheet(
            "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; min-width: 60px; max-width: 60px; color: white; }"
            );
        vectorLayout->addWidget(lbl);
        vectorLayout->addWidget(edit);
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
    vectorLayout->addStretch();
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}
