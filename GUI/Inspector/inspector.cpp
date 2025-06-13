

#include "inspector.h"
#include "qcombobox.h"
#include "qcoreevent.h"
#include "qevent.h"
#include "qjsonarray.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QCheckBox>
#include <core/Debug/console.h>
#include <QPushButton>
#include <QColorDialog>
#include <QListWidget>
#include <qlineedit.h>
#include <qfiledialog.h>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMenu>
#include <GUI/Inspector/customparameterdialog.h>

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
        "   font-size: 16px;"
        "   color: white;"
        "   background-color: #222;"
        "   border: none;"
        "   padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #444;"
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
        qDebug() << "Cannot paste - component type mismatch. Source:"
                 << copiedComponentType << "Destination:" << Name;
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

    tableWidget = new QTableWidget(5, 2, this); // 2 columns: key, value
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

    qDebug() << "Created add button with right alignment";

    this->setWidget(container);

    connect(tableWidget, &QTableWidget::cellChanged, this, [=](int r, int col){
        if (col != 1 || !rowToKeyPath.contains(r)) return;
        QString keyPath = rowToKeyPath[r];
        QString newValue = tableWidget->item(r, 1)->text();
        QStringList parts = keyPath.split(".");

        QJsonObject delta;
        if (parts.size() == 1) { delta[parts[0]] = newValue; }
        else { delta[parts[0]] = QJsonObject{{parts[1], newValue}}; }

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
            customParameterKeys.insert(parameterName); // Mark as custom parameter

            // Set key in first column
            QTableWidgetItem *keyItem = new QTableWidgetItem(parameterName);
            keyItem->setFlags(Qt::ItemIsEnabled);
            keyItem->setBackground(QColor("#2A3F54")); // Blue for name
            keyItem->setForeground(Qt::white);
            tableWidget->setItem(row, 0, keyItem);
            qDebug() << "Set name for row" << row << "to" << parameterName << "with bg color #2A3F54";

            // Create remove button
            QPushButton *removeButton = new QPushButton("❌", this);
            removeButton->setFixedSize(20, 20);
            removeButton->setStyleSheet(
                "QPushButton { color: white;  border-radius: 3px; background-color: #2A3F54; }" // White icon, blue background
                "QPushButton:hover { background-color: #444; }"
                );
            removeButton->setProperty("parameterName", parameterName); // Store parameterName

            // Connect remove button slot
            connect(removeButton, &QPushButton::clicked, this, [=]() {
                QPushButton *senderButton = qobject_cast<QPushButton*>(sender());
                if (!senderButton) {
                    qDebug() << "Error: Sender is not a QPushButton";
                    return;
                }

                // Find the row by checking which cell contains the sender button in column 1
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
                customParameterKeys.remove(key); // Remove from custom parameters

                // Rebuild rowToKeyPath to match table rows
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
                // Create widget for value column with value and remove button
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QTableWidgetItem *valueItem = new QTableWidgetItem(parameterValue);
                valueItem->setForeground(Qt::white);
                QLabel *valueLabel = new QLabel(parameterValue); // Use QLabel to display text
                valueLabel->setStyleSheet("color: white;"); // White foreground
                valueLayout->addWidget(valueLabel);
                valueLayout->addStretch(); // Push remove button to the right
                valueLayout->addWidget(removeButton);

                tableWidget->setCellWidget(row, 1, valueWidget);
                delta[parameterName] = parameterValue;
                qDebug() << "Set string value for row" << row << "to" << parameterValue << "with bg color #2A3F54";
            } else if (parameterType == "number") {
                QWidget *valueWidget = new QWidget();
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                WheelableLineEdit *lineEdit = new WheelableLineEdit();
                lineEdit->setText(parameterValue);
                lineEdit->setStyleSheet(
                    "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }" // Blue
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
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
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
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
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
                        "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; min-width: 60px; max-width: 60px; color: white; }" // Blue
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
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
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
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
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
                valueWidget->setStyleSheet("background-color: #2A3F54;"); // Blue background
                QHBoxLayout *valueLayout = new QHBoxLayout(valueWidget);
                valueLayout->setContentsMargins(2, 2, 2, 2);
                valueLayout->setSpacing(5);

                QWidget *spriteWidget = new QWidget();
                QVBoxLayout *spriteLayout = new QVBoxLayout(spriteWidget);
                spriteLayout->setContentsMargins(0, 0, 0, 0);
                QLineEdit *lineEdit = new QLineEdit(parameterValue);
                lineEdit->setStyleSheet(
                    "QLineEdit { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }" // Blue
                    );
                QPushButton *browseBtn = new QPushButton("Browse");
                browseBtn->setStyleSheet(
                    "QPushButton { background-color: #2A3F54; border: 1px solid #555; border-radius: 3px; color: white; }" // Blue
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

            // Verify cell content
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

        // Rebuild rowToKeyPath to match table rows
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

void Inspector::init(QString ID, QString name, QJsonObject obj)
{
    ConnectedID = ID;
    Name = name;
    titleLabel->setText(Name);
    tableWidget->clearContents();
    tableWidget->blockSignals(true);
    rowToKeyPath.clear();
    customParameterKeys.clear(); // Clear custom parameters

    tableWidget->setRowCount(obj.size());

    int row = 0;
    for (const QString &key : obj.keys()) {
        row = addSimpleRow(row, key, obj[key]);
    }

    tableWidget->blockSignals(false);
    tableWidget->viewport()->update();
    qDebug() << "Initialized Inspector with ID:" << ID << "name:" << name << "data:" << obj;
}

int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
    rowToKeyPath[row] = key;
    QTableWidgetItem *keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setBackground(QColor("#111")); // Darker background for non-custom
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
            QListWidgetItem *item = new QListWidgetItem(obj["name"].toString());
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
            QListWidgetItem *newItem = new QListWidgetItem("New Item");
            listWidget->addItem(newItem);
            listWidget->setCurrentItem(newItem);
            emitArrayChanged();
        });

        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                delete listWidget->takeItem(listWidget->row(item));
                emitArrayChanged();
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
                    for (const QString& key : obj.keys()) {
                        if (key != "type") {
                            copiedVectorData[key] = obj[key];
                        }
                    }
                }
                else if (selectedAction == pasteVectorAction) {
                    if (!copiedVectorData.isEmpty()) {
                        QJsonObject newVectorData = obj;
                        for (const QString& key : copiedVectorData.keys()) {
                            if (newVectorData.contains(key)) {
                                newVectorData[key] = copiedVectorData[key];
                                for (QObject *child : vectorWidget->children()) {
                                    QLineEdit *line = qobject_cast<QLineEdit *>(child);
                                    if (line && line->objectName() == key) {
                                        line->setText(QString::number(copiedVectorData[key].toDouble()));
                                        break;
                                    }
                                }
                            }
                        }
                        QJsonObject delta;
                        delta[fullKey] = newVectorData;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
                }
                else if (selectedAction == copyXAction && obj.contains("x")) {
                    copiedVectorData = QJsonObject{{"x", obj["x"]}};
                }
                else if (selectedAction == copyYAction && obj.contains("y")) {
                    copiedVectorData = QJsonObject{{"y", obj["y"]}};
                }
                else if (selectedAction == copyZAction && obj.contains("z")) {
                    copiedVectorData = QJsonObject{{"z", obj["z"]}};
                }
                else if (selectedAction == pasteXAction && copiedVectorData.contains("x")) {
                    QJsonObject newVectorData = obj;
                    newVectorData["x"] = copiedVectorData["x"];
                    for (QObject *child : vectorWidget->children()) {
                        QLineEdit *line = qobject_cast<QLineEdit *>(child);
                        if (line && line->objectName() == "x") {
                            line->setText(QString::number(copiedVectorData["x"].toDouble()));
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
                }
                else if (selectedAction == pasteYAction && copiedVectorData.contains("y")) {
                    QJsonObject newVectorData = obj;
                    newVectorData["y"] = copiedVectorData["y"];
                    for (QObject *child : vectorWidget->children()) {
                        QLineEdit *line = qobject_cast<QLineEdit *>(child);
                        if (line && line->objectName() == "y") {
                            line->setText(QString::number(copiedVectorData["y"].toDouble()));
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
                }
                else if (selectedAction == pasteZAction && copiedVectorData.contains("z")) {
                    QJsonObject newVectorData = obj;
                    newVectorData["z"] = copiedVectorData["z"];
                    for (QObject *child : vectorWidget->children()) {
                        QLineEdit *line = qobject_cast<QLineEdit *>(child);
                        if (line && line->objectName() == "z") {
                            line->setText(QString::number(copiedVectorData["z"].toDouble()));
                            break;
                        }
                    }
                    QJsonObject delta;
                    delta[fullKey] = newVectorData;
                    emit valueChanged(ConnectedID, Name, delta);
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
                    "   background: #333;"
                    "   border: 1px solid #555;"
                    "   border-radius: 3px;"
                    "   min-width: 60px;"
                    "   max-width: 60px;"
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
        }
    } else if (value.isString()) {
        QTableWidgetItem *valueItem = new QTableWidgetItem(value.toString());
        valueItem->setBackground(QColor("#111")); // Original color for pre-existing
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
            "   background: #333;"
            "   border: 1px solid #555;"
            "   border-radius: 3px;"
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
