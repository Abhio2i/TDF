#include "inspector.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QMimeData>
#include <QMenu>
#include <QComboBox>
#include <QEvent>
#include <QJsonArray>
#include <core/Debug/console.h>
#include <GUI/Inspector/customparameterdialog.h>

        // Helper function to format numbers for UI display
        QString formatNumberForUI(double value) {
    if (qFuzzyCompare(value, qRound(value))) {
        return QString::number(qRound(value));
    } else {
        QString result = QString::number(value, 'f', 4).trimmed();
        while (result.endsWith('0')) {
            result.chop(1);
        }
        if (result.endsWith('.')) {
            result.chop(1);
        }
        return result;
    }
}

// WheelableLineEdit implementation
WheelableLineEdit::WheelableLineEdit(QWidget *parent) : QLineEdit(parent)
{
    setAlignment(Qt::AlignCenter);
}

void WheelableLineEdit::wheelEvent(QWheelEvent *event)
{
    if (hasFocus()) {
        double step = event->angleDelta().y() > 0 ? 1.0 : -1.0;
        if (event->modifiers() & Qt::ControlModifier) {
            step *= 0.1;
        } else if (event->modifiers() & Qt::ShiftModifier) {
            step *= 10.0;
        }

        bool ok;
        double value = text().toDouble(&ok);
        if (ok) {
            setText(QString::number(value + step));
            emit editingFinished();
        }
    }
    QLineEdit::wheelEvent(event);
}

// Inspector implementation
Inspector::Inspector(QWidget *parent) : QDockWidget(parent)
{
    setupUI();
}

void Inspector::setupTitleBar()
{
    titleBarWidget = new QWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBarWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    titleLabel = new QLabel("Inspector", titleBarWidget);
    titleLabel->setStyleSheet(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: white;"
        "background-color: #222;"
        "padding: 5px;"
        );
    titleLabel->setAlignment(Qt::AlignCenter);

    menuButton = new QPushButton("⋮", titleBarWidget);
    menuButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 16px;"
        "    color: white;"
        "    background-color: #222;"
        "    border: none;"
        "    padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #444;"
        "}"
        );
    menuButton->setFixedWidth(30);

    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addWidget(menuButton);

    connect(menuButton, &QPushButton::clicked, this, [this]() {
        QMenu *menu = createContextMenu();
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
}

QMenu* Inspector::createContextMenu()
{
    QMenu *menu = new QMenu(this);

    QAction *copyAction = menu->addAction("Copy Component");
    QAction *pasteAction = menu->addAction("Paste Component");
    pasteAction->setEnabled(!copiedComponentData.isEmpty());

    QAction *lockAction = menu->addAction("Lock");
    QAction *unlockAction = menu->addAction("Unlock");
    QAction *addTabAction = menu->addAction("Add Tab");
    menu->addSeparator();
    QAction *closeAction = menu->addAction("Close");

    connect(copyAction, &QAction::triggered, this, &Inspector::copyCurrentComponent);
    connect(pasteAction, &QAction::triggered, this, &Inspector::pasteToCurrentComponent);
    connect(lockAction, &QAction::triggered, this, [](){ qDebug() << "Lock action triggered"; });
    connect(unlockAction, &QAction::triggered, this, [](){ qDebug() << "Unlock action triggered"; });
    connect(addTabAction, &QAction::triggered, this, &Inspector::handleAddTab);
    connect(closeAction, &QAction::triggered, this, [](){ qDebug() << "Close action triggered"; });

    return menu;
}

void Inspector::copyCurrentComponent()
{
    if (!hierarchy) {
        qDebug() << "Cannot copy - hierarchy not set";
        return;
    }

    if (Name.isEmpty() || ConnectedID.isEmpty()) {
        qDebug() << "Cannot copy - no component selected";
        return;
    }

    copiedComponentData = hierarchy->getComponentData(ConnectedID, Name);
    if (copiedComponentData.isEmpty()) {
        qDebug() << "Failed to copy component data - empty result";
        return;
    }

    copiedComponentType = Name;
    qDebug() << "Copied component:" << Name << "from entity" << ConnectedID;

    if (menuButton && menuButton->menu()) {
        QList<QAction*> actions = menuButton->menu()->actions();
        for (QAction* action : actions) {
            if (action->text() == "Paste Component") {
                action->setEnabled(true);
                break;
            }
        }
    }
}

void Inspector::pasteToCurrentComponent()
{
    if (!hierarchy) {
        qDebug() << "Cannot paste - hierarchy not set";
        return;
    }

    if (Name.isEmpty() || ConnectedID.isEmpty()) {
        qDebug() << "Cannot paste - no component selected";
        return;
    }

    if (copiedComponentData.isEmpty()) {
        qDebug() << "Nothing to paste - copy a component first";
        return;
    }

    if (Name == copiedComponentType) {
        emit valueChanged(ConnectedID, Name, copiedComponentData);
        qDebug() << "Pasted component data to:" << Name << "on entity" << ConnectedID;
        init(ConnectedID, Name, hierarchy->getComponentData(ConnectedID, Name));
    } else {
        qDebug() << "Cannot paste - component type mismatch. Source:" << copiedComponentType << "Destination:" << Name;
    }
}

void Inspector::handleAddTab()
{
    emit addTabRequested();
}

void Inspector::setupUI()
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupTitleBar();
    setTitleBarWidget(titleBarWidget);

    tableWidget = new QTableWidget(5, 2, this);
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setStyleSheet(
        "QTableWidget { background-color: black; color: white; border: 1px solid gray; }"
        "QTableWidget::item { border: 1px solid #444; }"
        "QTableWidget::item:selected { background-color: #555; }"
        );
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setStyleSheet("alternate-background-color: #101010; background-color: #111111;");

    layout->addWidget(tableWidget);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("Add", this);
    addButton->setFixedSize(30, 20);
    addButton->setStyleSheet(
        "QPushButton { color: white; border: 1px solid gray; border-radius: 3px; }"
        "QPushButton:hover { background-color: #444; }"
        );
    buttonLayout->setSpacing(5);
    buttonLayout->addStretch();
    buttonLayout->addWidget(addButton);
    layout->addLayout(buttonLayout);

    setWidget(container);

    connect(tableWidget, &QTableWidget::cellChanged, this, [=](int r, int col) {
        if (col != 1 || !rowToKeyPath.contains(r)) return;
        QString keyPath = rowToKeyPath[r];
        QString newValue = tableWidget->item(r, 1)->text();
        QStringList parts = keyPath.split(".");

        QJsonObject delta;
        if (parts.size() == 1) delta[parts[0]] = newValue;
        else delta[parts[0]] = QJsonObject{{parts[1], newValue}};

        emit valueChanged(ConnectedID, Name, delta);
    });

    connect(addButton, &QPushButton::clicked, this, &Inspector::handleAddParameter);
}

void Inspector::handleAddParameter()
{
    CustomParameterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString parameterName = dialog.getParameterName();
        QString parameterType = dialog.getParameterType();
        QString parameterValue = dialog.getParameterValue();
        if (!parameterName.isEmpty()) {
            int row = rowToKeyPath.size();
            tableWidget->setRowCount(row + 1);
            rowToKeyPath[row] = parameterName;
            customParameterKeys.insert(parameterName);

            QTableWidgetItem *keyItem = new QTableWidgetItem(parameterName);
            keyItem->setFlags(Qt::ItemIsEnabled);
            keyItem->setBackground(QColor("#2A3F54"));
            keyItem->setForeground(Qt::white);
            tableWidget->setItem(row, 0, keyItem);
            qDebug() << "Set name for row" << row << "to" << parameterName << "with bg color #2A3F54";

            QPushButton *removeButton = new QPushButton("❌", this);
            removeButton->setFixedSize(20, 20);
            removeButton->setStyleSheet(
                "QPushButton { color: white; border-radius: 3px; background-color: #2A3F54; }"
                "QPushButton:hover { background-color: #444; }"
                );
            removeButton->setProperty("parameterName", parameterName);

            connect(removeButton, &QPushButton::clicked, this, [=]() {
                QPushButton *senderButton = qobject_cast<QPushButton*>(sender());
                if (!senderButton) {
                    qDebug() << "Error: Sender is not a QPushButton";
                    return;
                }

                int currentRow = -1;
                for (int r = 0; r < tableWidget->rowCount(); ++r) {
                    QWidget *widget = tableWidget->cellWidget(r, 1);
                    if (widget) {
                        QPushButton *button = widget->findChild<QPushButton*>();
                        if (button == senderButton) {
                            currentRow = r;
                            break;
                        }
                    }
                }

                if (currentRow == -1) {
                    qDebug() << "Error: Could not find row for remove button";
                    return;
                }

                QString key = rowToKeyPath.value(currentRow);
                if (key.isEmpty()) {
                    qDebug() << "Error: No key found for row" << currentRow;
                    return;
                }

                qDebug() << "Removing row:" << currentRow << "with key:" << key;
                tableWidget->removeRow(currentRow);
                rowToKeyPath.remove(currentRow);
                customParameterKeys.remove(key);

                QMap<int, QString> newRowToKeyPath;
                for (int r = 0; r < tableWidget->rowCount(); ++r) {
                    if (QTableWidgetItem *item = tableWidget->item(r, 0)) {
                        newRowToKeyPath[r] = item->text();
                    }
                }
                rowToKeyPath = newRowToKeyPath;

                emit parameterChanged(ConnectedID, Name, key, "", false);
                qDebug() << "Emitted parameterChanged for removal of key:" << key << "rowToKeyPath after:" << rowToKeyPath;
            });

            QJsonObject delta;
            if (parameterType == "string") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QLabel *valueLabel = new QLabel(parameterValue);
                valueLabel->setStyleSheet("color: white;");
                valueLayout->addWidget(valueLabel);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                delta[parameterName] = parameterValue;
                qDebug() << "Set string value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "number") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                WheelableLineEdit *lineEdit = new WheelableLineEdit();
                lineEdit->setText(parameterValue);
                lineEdit->setStyleSheet(
                    "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }"
                    );
                QDoubleValidator *validator = new QDoubleValidator(lineEdit);
                validator->setNotation(QDoubleValidator::StandardNotation);
                validator->setDecimals(4);
                lineEdit->setValidator(validator);
                valueLayout->addWidget(lineEdit);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
                    QJsonObject delta;
                    delta[parameterName] = lineEdit->text().toDouble();
                    emit valueChanged(ConnectedID, Name, delta);
                });
                delta[parameterName] = parameterValue.toDouble();
                qDebug() << "Set number value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "boolean") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QCheckBox *checkBox = new QCheckBox();
                bool value = parameterValue.toLower() == "true";
                checkBox->setChecked(value);
                valueLayout->addWidget(checkBox);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
                    QJsonObject delta;
                    delta[parameterName] = checked;
                    emit valueChanged(ConnectedID, Name, delta);
                });
                delta[parameterName] = value;
                qDebug() << "Set boolean value for row" << row << "to" << value << "with bg color #2A3F54";
            } else if (parameterType == "vector") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QWidget *vectorWidget = new QWidget();
                QHBoxLayout *vectorInnerLayout = new QHBoxLayout(vectorWidget);
                vectorInnerLayout->setContentsMargins(0, 0, 0, 0);
                QStringList axes = {"x", "y", "z"};
                QStringList components = parameterValue.split(",");
                QJsonObject vectorData;
                int compIndex = 0;
                for (const QString &axis : axes) {
                    QLabel *lbl = new QLabel(axis);
                    lbl->setStyleSheet("color: #aaa; min-width: 20px;");
                    WheelableLineEdit *edit = new WheelableLineEdit();
                    QString compValue = (compIndex < components.size()) ? components[compIndex] : "0.0";
                    edit->setText(compValue);
                    QDoubleValidator *validator = new QDoubleValidator(edit);
                    validator->setNotation(QDoubleValidator::StandardNotation);
                    validator->setDecimals(4);
                    edit->setValidator(validator);
                    edit->setObjectName(axis);
                    edit->setStyleSheet(
                        "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; min-width: 60px; max-width: 60px; color: white; }"
                        );
                    vectorInnerLayout->addWidget(lbl);
                    vectorInnerLayout->addWidget(edit);
                    vectorData[axis] = compValue.toDouble();
                    connect(edit, &QLineEdit::editingFinished, this, [=]() {
                        QJsonObject vectorDelta;
                        for (QObject *child : vectorWidget->children()) {
                            WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                            if (line) {
                                vectorDelta[line->objectName()] = line->text().toDouble();
                            }
                        }
                        QJsonObject delta;
                        delta[parameterName] = vectorDelta;
                        emit valueChanged(ConnectedID, Name, delta);
                    });
                    compIndex++;
                }
                valueLayout->addWidget(vectorWidget);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                delta[parameterName] = vectorData;
                qDebug() << "Set vector value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "option") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QComboBox *combo = new QComboBox();
                combo->addItems({"Option1", "Option2"});
                combo->setCurrentText(parameterValue);
                valueLayout->addWidget(combo);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
                    QJsonObject delta;
                    QJsonObject optionObj;
                    optionObj["value"] = text;
                    delta[parameterName] = optionObj;
                    emit valueChanged(ConnectedID, Name, delta);
                });
                QJsonObject optionObj;
                optionObj["value"] = parameterValue;
                delta[parameterName] = optionObj;
                qDebug() << "Set option value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "color") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QPushButton *colorBtn = new QPushButton(parameterValue);
                colorBtn->setStyleSheet(QString("background-color: %1").arg(parameterValue));
                valueLayout->addWidget(colorBtn);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                connect(colorBtn, &QPushButton::clicked, this, [=]() {
                    QColor selectedColor = QColorDialog::getColor(QColor(parameterValue), this, "Select Color");
                    if (selectedColor.isValid()) {
                        QString hex = selectedColor.name();
                        colorBtn->setText(hex);
                        colorBtn->setStyleSheet(QString("background-color: %1").arg(hex));
                        QJsonObject delta;
                        QJsonObject colorObj;
                        colorObj["value"] = hex;
                        delta[parameterName] = colorObj;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
                });
                QJsonObject colorObj;
                colorObj["value"] = parameterValue;
                delta[parameterName] = colorObj;
                qDebug() << "Set color value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "image") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;");
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QWidget *spriteWidget = new QWidget();
                QVBoxLayout *spriteLayout = new QVBoxLayout(spriteWidget);
                spriteLayout->setContentsMargins(0, 0, 0, 0);
                QLineEdit *lineEdit = new QLineEdit(parameterValue);
                lineEdit->setStyleSheet(
                    "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }"
                    );
                QPushButton *browseBtn = new QPushButton("Browse");
                browseBtn->setStyleSheet(
                    "QPushButton { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }"
                    );
                QLabel *imageLabel = new QLabel();
                imageLabel->setFixedSize(60, 60);
                imageLabel->setScaledContents(true);
                if (!parameterValue.isEmpty()) {
                    imageLabel->setPixmap(QPixmap(parameterValue).scaled(imageLabel->size(), Qt::KeepAspectRatio));
                }
                spriteLayout->addWidget(imageLabel);
                spriteLayout->addWidget(lineEdit);
                spriteLayout->addWidget(browseBtn);
                valueLayout->addWidget(spriteWidget);
                valueLayout->addStretch();
                valueLayout->addWidget(removeButton);

                tableWidget->setRowHeight(row, 100);
                tableWidget->setCellWidget(row, 1, valueWidget);
                connect(browseBtn, &QPushButton::clicked, this, [=]() {
                    QString filePath = QFileDialog::getOpenFileName(this, "Select Sprite", "", "Images (*.png *.jpg *.bmp)");
                    if (!filePath.isEmpty()) {
                        lineEdit->setText(filePath);
                        imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
                        QJsonObject delta;
                        QJsonObject spriteObj;
                        spriteObj["value"] = filePath;
                        delta[parameterName] = spriteObj;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
                });
                connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
                    QString filePath = lineEdit->text();
                    imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
                    QJsonObject delta;
                    QJsonObject spriteObj;
                    spriteObj["value"] = filePath;
                    delta[parameterName] = spriteObj;
                    emit valueChanged(ConnectedID, Name, delta);
                });
                QJsonObject spriteObj;
                spriteObj["value"] = parameterValue;
                delta[parameterName] = spriteObj;
                qDebug() << "Set image value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            }

            tableWidget->setRowHeight(row, parameterType == "image" ? 100 : 30);
            tableWidget->viewport()->update();
            qDebug() << "Added parameter to table:" << parameterName << " type:" << parameterType << " value:" << parameterValue;

            emit parameterChanged(ConnectedID, Name, parameterName, parameterType, true);
            emit valueChanged(ConnectedID, Name, delta);
            qDebug() << "Emitted parameterChanged for:" << parameterName << " type:" << parameterType;
            qDebug() << "Emitted valueChanged with delta:" << delta;

            if (tableWidget->item(row, 1)) {
                qDebug() << "Cell item text at row" << row << ":" << tableWidget->item(row, 1)->text();
            } else if (tableWidget->cellWidget(row, 1)) {
                qDebug() << "Cell widget exists at row" << row;
            } else {
                qDebug() << "No item or widget set at row" << row;
            }
        }
    }
}

void Inspector::handleRemoveParameter()
{
    int selectedRow = tableWidget->currentRow();
    qDebug() << "Attempting to remove row:" << selectedRow << "rowToKeyPath before:" << rowToKeyPath;
    if (selectedRow >= 0 && rowToKeyPath.contains(selectedRow)) {
        QString selectedKey = rowToKeyPath.value(selectedRow);

        tableWidget->removeRow(selectedRow);
        rowToKeyPath.remove(selectedRow);

        QMap<int, QString> newRowToKeyPath;
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            if (QTableWidgetItem *item = tableWidget->item(row, 0)) {
                newRowToKeyPath[row] = item->text();
            }
        }
        rowToKeyPath = newRowToKeyPath;

        emit parameterChanged(ConnectedID, Name, selectedKey, "", false);
        qDebug() << "Removed selected parameter from row:" << selectedRow << "key:" << selectedKey << "rowToKeyPath after:" << rowToKeyPath;
    } else {
        qDebug() << "No row selected or invalid row:" << selectedRow;
    }
}

bool Inspector::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        QObject* listViewport = watched;
        QListWidget* listWidget = qobject_cast<QListWidget*>(listViewport->parent());

        if (listWidget && listWidget->property("row").isValid()) {
            int row = listWidget->property("row").toInt();
            qDebug() << "📥 Drop received on row:" << row;

            const QMimeData* mime = dropEvent->mimeData();
            qDebug() << "Available formats:" << mime->formats();

            if (dropEvent->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
                QByteArray encoded = dropEvent->mimeData()->data("application/x-qabstractitemmodeldatalist");
                QDataStream stream(&encoded, QIODevice::ReadOnly);

                while (!stream.atEnd()) {
                    int row, col;
                    QMap<int, QVariant> roleDataMap;
                    stream >> row >> col >> roleDataMap;

                    QString text = roleDataMap.value(Qt::DisplayRole).toString();
                    qDebug() << "Dropped item text:" << text;

                    QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();
                    QJsonObject json = QJsonObject::fromVariantMap(customData);
                    qDebug() << "Dropped item text:" << text;
                    qDebug() << "Custom data (UserRole):" << json;

                    QListWidgetItem *newItem = new QListWidgetItem(text);
                    newItem->setData(Qt::UserRole, customData);
                    listWidget->addItem(newItem);
                }

                dropEvent->acceptProposedAction();
            }

            return true;
        }
    }

    return QDockWidget::eventFilter(watched, event);
}


// void Inspector::init(QString ID, QString name, QJsonObject obj)
// {
//     ConnectedID = ID;
//     if (name.compare("Trajectories", Qt::CaseInsensitive) == 0) {
//         Name = "trajectory";

//     } else {
//         Name = name.toLower();
//     }
//     titleLabel->setText(name);
//     tableWidget->clearContents();
//     tableWidget->blockSignals(true);
//     rowToKeyPath.clear();
//     customParameterKeys.clear();

//     if (Name == "trajectory" && obj.isEmpty() && hierarchy) {
//         obj = hierarchy->getComponentData(ID, "trajectory");
//         qDebug() << "Fetched trajectory data for ID:" << ID << "data:" << obj;
//     }

//     tableWidget->setRowCount(obj.size());

//     int row = 0;
//     for (const QString &key : obj.keys()) {
//         row = addSimpleRow(row, key, obj[key]);
//     }

//     tableWidget->blockSignals(false);
//     tableWidget->viewport()->update();
//     qDebug() << "Initialized Inspector with ID:" << ID << "name:" << Name << "data:" << obj;
// }
#include <QDebug>
#include <QString>
#include <QJsonObject>
#include "inspector.h"

void Inspector::init(QString ID, QString name, QJsonObject object)
{
    ConnectedID = ID;
    // Handle name normalization for specific cases
    if (name.compare("Trajectories", Qt::CaseInsensitive) == 0) {
        Name = QString("trajectory");
    } else if (name.compare("dynamicModel", Qt::CaseInsensitive) == 0) {
        Name = QString("dynamicModel"); // Keep as dynamicModel
    } else if (name.compare("meshRenderer2d", Qt::CaseInsensitive) == 0) {
        Name = QString("meshRenderer2d"); // Keep as meshRenderer2d
    } else {
        Name = name.toLower();
    }
    titleLabel->setText(name); // Set the display name
    tableWidget->clearContents(); // Clear existing table content
    tableWidget->blockSignals(true); // Prevent signals during initialization
    rowToKeyPath.clear(); // Clear row-to-key mappings
    customParameterKeys.clear(); // Clear custom parameter keys

    // Fetch data for trajectory, dynamicModel, or meshRenderer2d if object is empty and hierarchy exists
    if ((Name == QString("trajectory") || Name == QString("dynamicModel") || Name == QString("meshRenderer2d")) && object.isEmpty() && hierarchy) {
        QString dataType = Name; // Use Name directly as dataType
        object = hierarchy->getComponentData(ID, dataType);
        qDebug() << QString("Fetched %1 data for ID:").arg(dataType) << ID << QString("data:") << object;
    }

    tableWidget->setRowCount(object.size()); // Set row count based on object size

    int row = 0;
    for (const QString &key : object.keys()) {
        row = addSimpleRow(row, key, object[key]); // Populate table rows
    }

    tableWidget->blockSignals(false); // Re-enable signals
    tableWidget->viewport()->update(); // Refresh table display
    qDebug() << QString("Initialized Inspector with ID:") << ID << QString("name:") << Name << QString("data:") << object;
}
int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
    rowToKeyPath[row] = key;
    QTableWidgetItem *keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setBackground(QColor("#111"));
    keyItem->setForeground(Qt::white);
    tableWidget->setItem(row, 0, keyItem);
    setupValueCell(row, key, value);
    return row + 1;
}

void Inspector::setupValueCell(int row, const QString &fullKey, const QJsonValue &value)
{
    tableWidget->setRowHeight(row, 30);
    if (value.isBool()) {
        QWidget *checkboxWidget = new QWidget();
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(value.toBool());

        QHBoxLayout *layout = new QHBoxLayout(checkboxWidget);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);

        connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
            QStringList parts = fullKey.split(".");
            QJsonObject delta;
            if (parts.size() == 2) delta[parts[0]] = QJsonObject{{parts[1], checked}};
            else delta[fullKey] = checked;
            emit valueChanged(ConnectedID, Name, delta);
        });

        tableWidget->setCellWidget(row, 1, checkboxWidget);
    } else if (value.isArray()) {
        QWidget *arrayWidget = new QWidget();
        arrayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QListWidget *listWidget = new QListWidget();
        listWidget->setProperty("row", row);
        listWidget->viewport()->installEventFilter(this);
        listWidget->setAcceptDrops(true);
        listWidget->setDropIndicatorShown(true);
        listWidget->setDragDropMode(QAbstractItemView::DropOnly);
        listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QJsonArray array = value.toArray();
        for (const QJsonValue &val : array) {
            QJsonObject obj = val.toObject();
            QString displayText = obj["name"].toString();
            if (fullKey == "trajectories" && obj.contains("position")) {
                QJsonObject pos = obj["position"].toObject();
                displayText = QString("(%1, %2)")
                                  .arg(pos["x"].toDouble(), 0, 'f', 6)
                                  .arg(pos["y"].toDouble(), 0, 'f', 6);
            }
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, obj.toVariantMap());
            listWidget->addItem(item);
        }

        QPushButton *addBtn = new QPushButton("➕");
        QPushButton *removeBtn = new QPushButton("❌");

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(removeBtn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(5);

        QVBoxLayout *layout = new QVBoxLayout(arrayWidget);
        layout->addWidget(listWidget);
        layout->addLayout(btnLayout);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto emitArrayChanged = [=]() {
            QJsonArray updatedArray;
            for (int i = 0; i < listWidget->count(); ++i) {
                QVariantMap itemData = listWidget->item(i)->data(Qt::UserRole).toMap();
                QJsonObject json = QJsonObject::fromVariantMap(itemData);
                updatedArray.append(json);
            }
            QJsonObject delta;
            delta[fullKey] = updatedArray;
            emit valueChanged(ConnectedID, Name, delta);
            if (fullKey == "trajectories") {
                emit trajectoryWaypointsChanged(ConnectedID, updatedArray);
            }
        };

        connect(listWidget, &QListWidget::itemChanged, this, [=](QListWidgetItem *item) {
            QString itemText = item->text();
            QVariantMap itemData = item->data(Qt::UserRole).toMap();
            QJsonObject json = QJsonObject::fromVariantMap(itemData);
            qDebug() << "📦 Edited item text:" << itemText;
            qDebug() << "🔍 Edited item data (UserRole):" << json;
            emitArrayChanged();
        });

        connect(listWidget, &QListWidget::doubleClicked, this, [=](const QModelIndex &index) {
            QListWidgetItem *item = listWidget->item(index.row());
            if (!item) return;
            QString itemText = item->text();
            QVariantMap itemData = item->data(Qt::UserRole).toMap();
            QJsonObject json = QJsonObject::fromVariantMap(itemData);
            emit foucsEntity(itemData["ID"].toString());
            qDebug() << "🖱️ Double-clicked item text:" << itemText;
            qDebug() << "🧠 Double-clicked item data (UserRole):" << json;
            emitArrayChanged();
        });

        connect(addBtn, &QPushButton::clicked, this, [=]() {
            QJsonObject newObj;
            QJsonObject posObj;
            posObj["type"] = "vector";
            posObj["x"] = 0.0;
            posObj["y"] = 0.0;
            posObj["z"] = 0.0;
            newObj["position"] = posObj;
            QString displayText = "(0.000000, 0.000000)";
            QListWidgetItem *newItem = new QListWidgetItem(displayText);
            newItem->setData(Qt::UserRole, newObj.toVariantMap());
            listWidget->addItem(newItem);
            listWidget->setCurrentItem(newItem);
            emitArrayChanged();
            qDebug() << "Added new waypoint to trajectories list";
        });

        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                delete listWidget->takeItem(listWidget->row(item));
                emitArrayChanged();
                qDebug() << "Removed waypoint from trajectories list";
            }
        });

        tableWidget->setRowHeight(row, 200);
        tableWidget->setCellWidget(row, 1, arrayWidget);
    } else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        if (obj["type"].toString().contains("vector") || obj["type"].toString().contains("geocord")) {
            currentVectorData = obj;
            currentVectorKey = fullKey;

            QWidget *vectorWidget = new QWidget();
            QHBoxLayout *vectorLayout = new QHBoxLayout(vectorWidget);
            vectorLayout->setContentsMargins(0, 0, 0, 0);

            vectorWidget->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(vectorWidget, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
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

                QAction *selectedAction = contextMenu.exec(vectorWidget->mapToGlobal(pos));

                if (selectedAction == copyVectorAction) {
                    copiedVectorData = QJsonObject();
                    QStringList axes = {"x", "y", "z"};
                    bool copiedFromUI = false;
                    // Try copying from UI first
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && axes.contains(line->objectName())) {
                            bool ok;
                            double value = line->text().toDouble(&ok);
                            if (ok) {
                                copiedVectorData[line->objectName()] = value;
                                copiedFromUI = true;
                            }
                        }
                    }
                    // Fallback to hierarchy if UI copy fails
                    if (!copiedFromUI && hierarchy) {
                        QJsonObject compData = hierarchy->getComponentData(ConnectedID, Name);
                        if (compData.contains(fullKey)) {
                            QJsonObject vecData = compData[fullKey].toObject();
                            for (const QString& key : axes) {
                                if (vecData.contains(key)) {
                                    copiedVectorData[key] = vecData[key].toDouble();
                                }
                            }
                        }
                    }
                    qDebug() << "Copied vector data" << (copiedFromUI ? "from UI" : "from hierarchy") << ":" << copiedVectorData;
                } else if (selectedAction == pasteVectorAction) {
                    if (!copiedVectorData.isEmpty()) {
                        QJsonObject newVectorData = obj;
                        bool updated = false;
                        for (const QString& key : copiedVectorData.keys()) {
                            if (newVectorData.contains(key)) {
                                double value = copiedVectorData[key].toDouble();
                                newVectorData[key] = value;
                                updated = true;
                                for (QObject *child : vectorWidget->children()) {
                                    WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                                    if (line && line->objectName() == key) {
                                        line->setText(formatNumberForUI(value));
                                        qDebug() << "Updated" << key << "to" << value << "in UI as" << line->text();
                                        break;
                                    }
                                }
                            }
                        }
                        if (updated) {
                            QJsonObject delta;
                            delta[fullKey] = newVectorData;
                            emit valueChanged(ConnectedID, Name, delta);
                            qDebug() << "Pasted vector data:" << newVectorData;
                        } else {
                            qDebug() << "No valid keys to paste in vector data";
                        }
                    } else {
                        qDebug() << "Paste vector failed: copiedVectorData is empty";
                    }
                } else if (selectedAction == copyXAction) {
                    copiedVectorData = QJsonObject();
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "x") {
                            bool ok;
                            double value = line->text().toDouble(&ok);
                            if (ok) {
                                copiedVectorData["x"] = value;
                                qDebug() << "Copied X component from UI:" << copiedVectorData;
                                break;
                            }
                        }
                    }
                    if (copiedVectorData.isEmpty() && hierarchy && obj.contains("x")) {
                        QJsonObject compData = hierarchy->getComponentData(ConnectedID, Name);
                        if (compData.contains(fullKey)) {
                            copiedVectorData["x"] = compData[fullKey].toObject()["x"].toDouble();
                            qDebug() << "Copied X component from hierarchy:" << copiedVectorData;
                        }
                    }
                } else if (selectedAction == copyYAction) {
                    copiedVectorData = QJsonObject();
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "y") {
                            bool ok;
                            double value = line->text().toDouble(&ok);
                            if (ok) {
                                copiedVectorData["y"] = value;
                                qDebug() << "Copied Y component from UI:" << copiedVectorData;
                                break;
                            }
                        }
                    }
                    if (copiedVectorData.isEmpty() && hierarchy && obj.contains("y")) {
                        QJsonObject compData = hierarchy->getComponentData(ConnectedID, Name);
                        if (compData.contains(fullKey)) {
                            copiedVectorData["y"] = compData[fullKey].toObject()["y"].toDouble();
                            qDebug() << "Copied Y component from hierarchy:" << copiedVectorData;
                        }
                    }
                } else if (selectedAction == copyZAction) {
                    copiedVectorData = QJsonObject();
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "z") {
                            bool ok;
                            double value = line->text().toDouble(&ok);
                            if (ok) {
                                copiedVectorData["z"] = value;
                                qDebug() << "Copied Z component from UI:" << copiedVectorData;
                                break;
                            }
                        }
                    }
                    if (copiedVectorData.isEmpty() && hierarchy && obj.contains("z")) {
                        QJsonObject compData = hierarchy->getComponentData(ConnectedID, Name);
                        if (compData.contains(fullKey)) {
                            copiedVectorData["z"] = compData[fullKey].toObject()["z"].toDouble();
                            qDebug() << "Copied Z component from hierarchy:" << copiedVectorData;
                        }
                    }
                } else if (selectedAction == pasteXAction && copiedVectorData.contains("x")) {
                    QJsonObject newVectorData = obj;
                    double value = copiedVectorData["x"].toDouble();
                    newVectorData["x"] = value;
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "x") {
                            line->setText(formatNumberForUI(value));
                            qDebug() << "Updated X to" << value << "in UI as" << line->text();
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
                    qDebug() << "Pasted X component:" << newVectorData;
                } else if (selectedAction == pasteYAction && copiedVectorData.contains("y")) {
                    QJsonObject newVectorData = obj;
                    double value = copiedVectorData["y"].toDouble();
                    newVectorData["y"] = value;
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "y") {
                            line->setText(formatNumberForUI(value));
                            qDebug() << "Updated Y to" << value << "in UI as" << line->text();
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
                    qDebug() << "Pasted Y component:" << newVectorData;
                } else if (selectedAction == pasteZAction && copiedVectorData.contains("z")) {
                    QJsonObject newVectorData = obj;
                    double value = copiedVectorData["z"].toDouble();
                    newVectorData["z"] = value;
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line && line->objectName() == "z") {
                            line->setText(formatNumberForUI(value));
                            qDebug() << "Updated Z to" << value << "in UI as" << line->text();
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
                    qDebug() << "Pasted Z component:" << newVectorData;
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
                    "QLineEdit {"
                    "    background: #333;"
                    "    border: 1px solid #555;"
                    "    border-radius: 3px;"
                    "    min-width: 60px;"
                    "    max-width: 60px;"
                    "}"
                    );
                vectorLayout->addWidget(lbl);
                vectorLayout->addWidget(edit);
                connect(edit, &QLineEdit::editingFinished, this, [=]() {
                    QJsonObject vectorDelta;
                    for (QObject *child : vectorWidget->children()) {
                        WheelableLineEdit *line = qobject_cast<WheelableLineEdit *>(child);
                        if (line) {
                            vectorDelta[line->objectName()] = line->text().toDouble();
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = vectorDelta;
                    emit valueChanged(ConnectedID, Name, delta);
                });
            }
            vectorLayout->addStretch();
            tableWidget->setCellWidget(row, 1, vectorWidget);
        } else if (obj["type"].toString().contains("option")) {
            QComboBox *combo = new QComboBox();
            QJsonArray options = obj["options"].toArray();
            QString selected = obj["value"].toString();

            for (const QJsonValue &val : options) {
                combo->addItem(val.toString());
            }

            int index = combo->findText(selected);
            if (index != -1)
                combo->setCurrentIndex(index);

            connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text) {
                QJsonObject delta;
                QJsonObject optionObj;
                optionObj["value"] = text;
                delta[fullKey] = optionObj;
                emit valueChanged(ConnectedID, Name, delta);
            });

            tableWidget->setCellWidget(row, 1, combo);
        } else if (obj["type"].toString().contains("color")) {
            QString currentColor = obj["value"].toString();
            QPushButton *colorBtn = new QPushButton(currentColor);
            colorBtn->setStyleSheet(QString("background-color: %1").arg(currentColor));

            connect(colorBtn, &QPushButton::clicked, this, [=]() {
                QColor color = QColorDialog::getColor(QColor(currentColor), this, "Select Color");
                if (color.isValid()) {
                    QString hex = color.name();
                    colorBtn->setText(hex);
                    colorBtn->setStyleSheet(QString("background-color: %1").arg(hex));
                    QJsonObject delta;
                    QJsonObject colorObj;
                    colorObj["value"] = hex;
                    delta[fullKey] = colorObj;
                    emit valueChanged(ConnectedID, Name, delta);
                }
            });

            tableWidget->setCellWidget(row, 1, colorBtn);
        } else if (obj["type"].toString().contains("image")) {
            QString currentPath = obj["value"].toString();
            QWidget *spriteWidget = new QWidget();
            QLineEdit *lineEdit = new QLineEdit(currentPath);
            QPushButton *browseBtn = new QPushButton("Browse");
            QLabel *imageLabel = new QLabel();
            imageLabel->setFixedSize(60, 60);
            imageLabel->setScaledContents(true);

            if (!currentPath.isEmpty())
                imageLabel->setPixmap(QPixmap(currentPath).scaled(imageLabel->size(), Qt::KeepAspectRatio));

            QVBoxLayout *layout = new QVBoxLayout(spriteWidget);
            layout->addWidget(imageLabel);
            layout->addWidget(lineEdit);
            layout->addWidget(browseBtn);
            layout->setContentsMargins(0, 0, 0, 0);

            connect(browseBtn, &QPushButton::clicked, this, [=]() {
                QString filePath = QFileDialog::getOpenFileName(this, "Select Sprite", "", "Images (*.png *.jpg *.bmp)");
                if (!filePath.isEmpty()) {
                    lineEdit->setText(filePath);
                    imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
                    QJsonObject delta;
                    QJsonObject spriteObj;
                    spriteObj["value"] = filePath;
                    delta[fullKey] = spriteObj;
                    emit valueChanged(ConnectedID, Name, delta);
                }
            });

            connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
                QString filePath = lineEdit->text();
                imageLabel->setPixmap(QPixmap(filePath).scaled(imageLabel->size(), Qt::KeepAspectRatio));
                QJsonObject delta;
                QJsonObject spriteObj;
                spriteObj["value"] = filePath;
                delta[fullKey] = spriteObj;
                emit valueChanged(ConnectedID, Name, delta);
            });
            tableWidget->setRowHeight(row, 100);
            tableWidget->setCellWidget(row, 1, spriteWidget);
        } else {
            QWidget *valueWidget = new QWidget();
            QVBoxLayout *layout = new QVBoxLayout(valueWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            for (const QString &subKey : obj.keys()) {
                QHBoxLayout *subLayout = new QHBoxLayout();
                QLabel *label = new QLabel(subKey);
                label->setStyleSheet("color: #aaa; min-width: 20px;");
                subLayout->addWidget(label);
                WheelableLineEdit *edit = new WheelableLineEdit();
                edit->setText(QString::number(obj[subKey].toDouble()));
                edit->setStyleSheet("QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; }");
                QDoubleValidator *validator = new QDoubleValidator(edit);
                validator->setNotation(QDoubleValidator::StandardNotation);
                validator->setDecimals(4);
                edit->setValidator(validator);
                edit->setObjectName(subKey);
                subLayout->addWidget(edit);
                layout->addLayout(subLayout);
                connect(edit, &QLineEdit::editingFinished, this, [=]() {
                    QJsonObject delta;
                    delta[fullKey] = QJsonObject{{subKey, edit->text().toDouble()}};
                    emit valueChanged(ConnectedID, Name, delta);
                });
            }
            tableWidget->setCellWidget(row, 1, valueWidget);
        }
    } else if (value.isString()) {
        QTableWidgetItem *valueItem = new QTableWidgetItem(value.toString());
        valueItem->setBackground(QColor("#111"));
        valueItem->setForeground(Qt::white);
        if (fullKey.contains("id") || fullKey.contains("name") || fullKey.contains("branch"))
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        tableWidget->setItem(row, 1, valueItem);
        qDebug() << "Set string value in setupValueCell for row" << row << "to" << value.toString() << "with bg color #111";
    } else {
        WheelableLineEdit *lineEdit = new WheelableLineEdit();
        lineEdit->setText(QString::number(value.toDouble()));
        lineEdit->setStyleSheet(
            "QLineEdit {"
            "    background: #333;"
            "    border: 1px solid #555;"
            "    border-radius: 3px;"
            "}"
            );
        QDoubleValidator *validator = new QDoubleValidator(lineEdit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setDecimals(4);
        lineEdit->setValidator(validator);
        tableWidget->setCellWidget(row, 1, lineEdit);

        connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
            QJsonObject delta;
            delta[fullKey] = lineEdit->text().toDouble();
            emit valueChanged(ConnectedID, Name, delta);
        });
    }
}

void Inspector::updateTrajectory(QString entityId, QJsonArray waypoints)
{
    qDebug() << "updateTrajectory called for entity:" << entityId << "with waypoints:" << waypoints;
    qDebug() << "Current ConnectedID:" << ConnectedID << "Name:" << Name;

    int trajRow = -1;
    for (int r = 0; r < tableWidget->rowCount(); ++r) {
        if (rowToKeyPath[r] == "trajectories") {
            trajRow = r;
            break;
        }
    }

    if (trajRow == -1) {
        qDebug() << "trajectories key not found, initializing row";
        tableWidget->setRowCount(tableWidget->rowCount() + 1);
        trajRow = tableWidget->rowCount() - 1;
        rowToKeyPath[trajRow] = "trajectories";
        QTableWidgetItem *keyItem = new QTableWidgetItem("trajectories");
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#111"));
        keyItem->setForeground(Qt::white);
        tableWidget->setItem(trajRow, 0, keyItem);
        setupValueCell(trajRow, "trajectories", QJsonArray());
    }

    QWidget *arrayWidget = tableWidget->cellWidget(trajRow, 1);
    if (!arrayWidget) {
        qDebug() << "No array widget found at row" << trajRow << ", reinitializing";
        setupValueCell(trajRow, "trajectories", QJsonArray());
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
    }

    QListWidget *listWidget = arrayWidget->findChild<QListWidget*>();
    if (!listWidget) {
        qDebug() << "No list widget found in array widget at row" << trajRow;
        return;
    }

    if (ConnectedID != entityId || Name != "trajectory") {
        qDebug() << "Updating ConnectedID to" << entityId << "and Name to trajectory";
        ConnectedID = entityId;
        Name = "trajectory";
        titleLabel->setText("Trajectories");
        QJsonObject trajData = hierarchy ? hierarchy->getComponentData(entityId, "trajectory") : QJsonObject();
        init(entityId, "Trajectories", trajData);
        trajRow = -1;
        for (int r = 0; r < tableWidget->rowCount(); ++r) {
            if (rowToKeyPath[r] == "trajectories") {
                trajRow = r;
                break;
            }
        }
        if (trajRow == -1) {
            qDebug() << "Failed to find trajectories row after reinitialization";
            return;
        }
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
        listWidget = arrayWidget->findChild<QListWidget*>();
        if (!listWidget) {
            qDebug() << "Failed to find list widget after reinitialization";
            return;
        }
    }

    listWidget->clear();

    for (const QJsonValue &val : waypoints) {
        QJsonObject obj = val.toObject();
        if (!obj.contains("position")) {
            qDebug() << "Skipping invalid waypoint:" << obj;
            continue;
        }
        QJsonObject pos = obj["position"].toObject();
        QString displayText = QString("(%1, %2)")
                                  .arg(pos["x"].toDouble(), 0, 'f', 6)
                                  .arg(pos["y"].toDouble(), 0, 'f', 6);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        listWidget->addItem(item);
        qDebug() << "Added waypoint to UI:" << displayText;
    }

    if (waypoints.isEmpty() && hierarchy) {
        QJsonObject trajData = hierarchy->getComponentData(ConnectedID, "trajectory");
        if (!trajData.isEmpty()) {
            QJsonArray array = trajData["trajectories"].toArray();
            for (const QJsonValue &val : array) {
                QJsonObject obj = val.toObject();
                if (!obj.contains("position")) continue;
                QJsonObject pos = obj["position"].toObject();
                QString displayText = QString("(%1, %2)")
                                          .arg(pos["x"].toDouble(), 0, 'f', 6)
                                          .arg(pos["y"].toDouble(), 0, 'f', 6);
                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, obj.toVariantMap());
                listWidget->addItem(item);
                qDebug() << "Added saved waypoint to UI:" << displayText;
            }
        }
    }

    tableWidget->viewport()->update();
    qDebug() << "Updated trajectory UI for entity:" << entityId << "with" << waypoints.size() << "waypoints";
}
