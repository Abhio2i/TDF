/* ========================================================================= */
/* File: inspector.cpp                                                     */
/* Purpose: Implements inspector widget for editing component properties     */
/* ========================================================================= */

#include "inspector.h"                             // For inspector class
#include "qjsondocument.h"                         // For JSON document handling
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QLabel>                                  // For labels
#include <QHeaderView>                             // For table header
#include <QTableWidget>                            // For table widget
#include <QCheckBox>                               // For checkboxes
#include <QPushButton>                             // For buttons
#include <QListWidget>                             // For list widget
#include <QLineEdit>                               // For input fields
#include <QFileDialog>                             // For file dialog
#include <QMimeData>                               // For MIME data handling
#include <QMenu>                                   // For context menus
#include <QComboBox>                               // For dropdown menus
#include <QEvent>                                  // For event handling
#include <QJsonArray>                              // For JSON arrays
#include <core/Debug/console.h>                    // For debug console
#include <GUI/Inspector/customparameterdialog.h>   // For custom parameter dialog
#include <GUI/Inspector/template/colortemplate.h>  // For color template
#include <GUI/Inspector/template/imagetemplate.h>  // For image template
#include <GUI/Inspector/template/geocordstemplate.h> // For geocoordinates template
#include <GUI/Inspector/template/optiontemplate.h> // For option template
#include <GUI/Inspector/template/vectortemplate.h> // For vector template

// %%% Utility Functions %%%
/* Format number for UI display */
QString Inspector::formatNumberForUI(double value)
{
    qDebug() << "formatNumberForUI: Formatting value:" << value;
    // Return integer if value is close to a whole number
    if (qFuzzyCompare(value, qRound(value))) {
        return QString::number(qRound(value));
    }
    // Format with up to 4 decimals, trim trailing zeros
    QString result = QString::number(value, 'f', 4).trimmed();
    while (result.endsWith('0')) result.chop(1);
    if (result.endsWith('.')) result.chop(1);
    return result;
}

// %%% WheelableLineEdit Implementation %%%
/* Initialize wheelable line edit */
WheelableLineEdit::WheelableLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // Set text alignment to center
    setAlignment(Qt::AlignCenter);
}

/* Handle mouse wheel events */
void WheelableLineEdit::wheelEvent(QWheelEvent *event)
{
    // Adjust value if focused
    if (hasFocus()) {
        double step = event->angleDelta().y() > 0 ? 1.0 : -1.0;
        // Modify step size with modifiers
        if (event->modifiers() & Qt::ControlModifier) step *= 0.1;
        else if (event->modifiers() & Qt::ShiftModifier) step *= 10.0;
        bool ok;
        double value = text().toDouble(&ok);
        if (ok) {
            // Update text and emit signal
            setText(QString::number(value + step));
            emit editingFinished();
        }
    }
    QLineEdit::wheelEvent(event);
}

// %%% Inspector Constructor %%%
/* Initialize inspector dock widget */
Inspector::Inspector(QWidget *parent)
    : QDockWidget(parent)
{
    // Setup UI components
    setupUI();
}

// %%% Lock State Management %%%
/* Set inspector lock state */
void Inspector::setLocked(bool locked)
{
    // Update lock state and title
    m_locked = locked;
    if (m_locked) {
        titleLabel->setText(titleLabel->text() + " (Locked)");
    } else {
        titleLabel->setText(titleLabel->text().remove(" (Locked)"));
    }
}

// %%% UI Setup %%%
/* Setup title bar */
void Inspector::setupTitleBar()
{
    // Create title bar widget
    titleBarWidget = new QWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBarWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);
    // Create title label
    titleLabel = new QLabel("Inspector", titleBarWidget);
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: white; background-color: #222; padding: 5px;"
        );
    titleLabel->setAlignment(Qt::AlignCenter);
    // Create menu button
    menuButton = new QPushButton("⋮", titleBarWidget);
    menuButton->setStyleSheet(
        "QPushButton { font-size: 16px; color: white; background-color: #222; border: none; padding: 5px 10px; }"
        "QPushButton:hover { background-color: #444; }"
        );
    menuButton->setFixedWidth(30);
    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addWidget(menuButton);
    // Connect menu button to show context menu
    connect(menuButton, &QPushButton::clicked, this, [this]() {
        QMenu *menu = createContextMenu();
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
}

/* Create context menu for title bar */
QMenu* Inspector::createContextMenu()
{
    // Create menu
    QMenu *menu = new QMenu(this);
    QAction *copyAction = menu->addAction("Copy Component");
    QAction *pasteAction = menu->addAction("Paste Component");
    pasteAction->setEnabled(!copiedComponentData.isEmpty());
    QAction *lockAction = menu->addAction("Lock");
    QAction *unlockAction = menu->addAction("Unlock");
    QAction *addTabAction = menu->addAction("Add Tab");
    menu->addSeparator();
    QAction *closeAction = menu->addAction("Close");
    // Connect menu actions
    connect(copyAction, &QAction::triggered, this, &Inspector::copyCurrentComponent);
    connect(pasteAction, &QAction::triggered, this, &Inspector::pasteToCurrentComponent);
    connect(addTabAction, &QAction::triggered, this, &Inspector::handleAddTab);
    connect(closeAction, &QAction::triggered, this, [](){});
    return menu;
}

/* Copy current component data */
void Inspector::copyCurrentComponent()
{
    // Validate prerequisites
    if (!hierarchy || Name.isEmpty() || ConnectedID.isEmpty()) {
        return;
    }
    // Copy component data
    copiedComponentData = hierarchy->getComponentData(ConnectedID, Name);
    if (copiedComponentData.isEmpty()) {
        return;
    }
    copiedComponentType = Name;
}

/* Paste copied component data */
void Inspector::pasteToCurrentComponent()
{
    // Validate prerequisites
    if (!hierarchy || Name.isEmpty() || ConnectedID.isEmpty() || copiedComponentData.isEmpty()) {
        return;
    }
    // Paste if component types match
    if (Name == copiedComponentType) {
        emit valueChanged(ConnectedID, Name, copiedComponentData);
        init(ConnectedID, Name, hierarchy->getComponentData(ConnectedID, Name));
    }
}

/* Handle add tab action */
void Inspector::handleAddTab()
{
    // Emit add tab signal
    emit addTabRequested();
}

/* Setup main UI */
void Inspector::setupUI()
{
    // Create container widget
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    // Setup title bar
    setupTitleBar();
    setTitleBarWidget(titleBarWidget);
    // Create table widget
    tableWidget = new QTableWidget(5, 2, this);
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setStyleSheet(
        "QTableWidget { background-color: black; color: white; border: 1px solid gray; }"
        "QTableWidget::item { border: 1px solid #444; color: white; }"
        "QTableWidget::item:selected { background-color: lightgray; }"
        );
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setStyleSheet("alternate-background-color: #101010; background-color: #111111;");

    // Add table to layout
    layout->addWidget(tableWidget);
    // Create button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("Add", this);
    addButton->setFixedSize(30, 20);
    addButton->setStyleSheet(
        "QPushButton { color: white; border: 1px solid gray; border-radius: 3px; background-color: #2A3F54; }"
        "QPushButton:hover { background-color: #444; }"
        );
    buttonLayout->setSpacing(5);
    buttonLayout->addStretch();
    buttonLayout->addWidget(addButton);
    layout->addLayout(buttonLayout);
    // Set container as dock widget
    setWidget(container);
    // Connect table cell changes
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
    // Connect add button
    connect(addButton, &QPushButton::clicked, this, &Inspector::handleAddParameter);
}

/* Create remove button for parameter */
QPushButton* Inspector::createRemoveButton(const QString &parameterName)
{
    // Create remove button
    QPushButton *removeButton = new QPushButton("❌", this);
    removeButton->setFixedSize(20, 20);
    removeButton->setStyleSheet(
        "QPushButton { color: white; border-radius: 3px; background-color: #2A3F54; }"
        "QPushButton:hover { background-color: #444; }"
        );
    removeButton->setProperty("parameterName", parameterName);
    // Connect remove button signal
    connect(removeButton, &QPushButton::clicked, this, [=]() {
        QPushButton *senderButton = qobject_cast<QPushButton*>(sender());
        if (!senderButton) return;
        int currentRow = -1;
        // Find row containing button
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
        if (currentRow == -1) return;
        QString key = rowToKeyPath.value(currentRow);
        // Remove row and update mappings
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
        // Emit parameter changed signal
        emit parameterChanged(ConnectedID, Name, key, "", false);
    });
    return removeButton;
}

/* Add parameter row to table */
void Inspector::addParameterRow(const QString &parameterName, int row)
{
    // Set row count
    tableWidget->setRowCount(row + 1);
    rowToKeyPath[row] = parameterName;
    customParameterKeys.insert(parameterName);
    // Create key item
    QTableWidgetItem *keyItem = new QTableWidgetItem(parameterName);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setBackground(QColor("#2A3F54"));
    keyItem->setForeground(Qt::white);
    tableWidget->setItem(row, 0, keyItem);
}

/* Handle add parameter action */
void Inspector::handleAddParameter()
{
    // Show custom parameter dialog
    CustomParameterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString parameterName = dialog.getParameterName();
        QString parameterType = dialog.getParameterType();
        QString parameterValue = dialog.getParameterValue();
        if (!parameterName.isEmpty()) {
            int row = rowToKeyPath.size();
            addParameterRow(parameterName, row);
            QJsonObject delta;
            // Setup cell based on parameter type
            if (parameterType == "string") {
                setupStringCell(row, parameterName, parameterValue);
                delta[parameterName] = parameterValue;
            } else if (parameterType == "number") {
                setupNumberCell(row, parameterName, parameterValue.toDouble());
                delta[parameterName] = parameterValue.toDouble();
            } else if (parameterType == "boolean") {
                setupBooleanCell(row, parameterName, parameterValue.toLower() == "true");
                delta[parameterName] = (parameterValue.toLower() == "true");
            } else if (parameterType == "vector") {
                VectorTemplate *vector = new VectorTemplate(this);
                vector->setConnectedID(ConnectedID);
                vector->setName(Name);
                QJsonObject vectorData;
                QStringList components = parameterValue.split(",");
                vectorData["x"] = components.size() > 0 ? components[0].toDouble() : 0.0;
                vectorData["y"] = components.size() > 1 ? components[1].toDouble() : 0.0;
                vectorData["z"] = components.size() > 2 ? components[2].toDouble() : 0.0;
                vectorData["type"] = "vector";
                vector->setupVectorCell(row, parameterName, vectorData, tableWidget);
                connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = vectorData;
            } else if (parameterType == "geocord") {
                GeocordsTemplate *geocords = new GeocordsTemplate(this);
                geocords->setConnectedID(ConnectedID);
                geocords->setName(Name);
                QJsonObject geocordsData;
                QStringList components = parameterValue.split(",");
                geocordsData["latitude"] = components.size() > 0 ? components[0].toDouble() : 0.0;
                geocordsData["longitude"] = components.size() > 1 ? components[1].toDouble() : 0.0;
                geocordsData["altitude"] = components.size() > 2 ? components[2].toDouble() : 0.0;
                geocordsData["heading"] = components.size() > 3 ? components[3].toDouble() : 0.0;
                geocordsData["type"] = "geocord";
                geocords->setupGeocordsCell(row, parameterName, geocordsData, tableWidget);
                connect(geocords, &GeocordsTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = geocordsData;
            } else if (parameterType == "option") {
                OptionTemplate *option = new OptionTemplate(this);
                option->setConnectedID(ConnectedID);
                option->setName(Name);
                QJsonObject optionObj;
                optionObj["value"] = parameterValue;
                optionObj["options"] = QJsonArray{"Option1", "Option2"};
                optionObj["type"] = "option";
                option->setupOptionCell(row, parameterName, optionObj, tableWidget);
                connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = optionObj;
            } else if (parameterType == "color") {
                ColorTemplate *color = new ColorTemplate(this);
                color->setConnectedID(ConnectedID);
                color->setName(Name);
                QJsonObject colorObj;
                colorObj["value"] = parameterValue;
                colorObj["type"] = "color";
                color->setupColorCell(row, parameterName, colorObj, tableWidget);
                connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = colorObj;
            } else if (parameterType == "image") {
                ImageTemplate *image = new ImageTemplate(this);
                image->setConnectedID(ConnectedID);
                image->setName(Name);
                QJsonObject spriteObj;
                spriteObj["value"] = parameterValue;
                spriteObj["type"] = "image";
                image->setupImageCell(row, parameterName, spriteObj, tableWidget);
                connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = spriteObj;
            }
            // Adjust row height for images
            tableWidget->setRowHeight(row, parameterType == "image" ? ImageTemplate::ROW_HEIGHT : 30);
            tableWidget->viewport()->update();
            // Emit signals for parameter and value changes
            emit parameterChanged(ConnectedID, Name, parameterName, parameterType, true);
            emit valueChanged(ConnectedID, Name, delta);
        }
    }
}

/* Setup boolean cell */
void Inspector::setupBooleanCell(int row, const QString &fullKey, bool value)
{
    qDebug() << "setupBooleanCell: Setting row:" << row << "for key:" << fullKey << "value:" << value;
    // Create checkbox widget
    QWidget *checkboxWidget = new QWidget();
    QCheckBox *checkBox = new QCheckBox();
    checkBox->setChecked(value);
    // Style checkbox
    checkBox->setStyleSheet(
        "QCheckBox { color: white; border: none; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid white; background-color: #333; subcontrol-origin: padding; subcontrol-position: center; }"
        "QCheckBox::indicator:checked { image: url(:/icons/images/check-box.png); }"
        "QCheckBox::indicator:unchecked { image: none; }"
        );
    // Create layout for checkbox
    QHBoxLayout *layout = new QHBoxLayout(checkboxWidget);
    layout->addWidget(checkBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    // Connect checkbox toggle
    connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
        QStringList parts = fullKey.split(".");
        QJsonObject delta;
        if (parts.size() == 2) {
            delta[parts[0]] = QJsonObject{{parts[1], checked}};
        } else {
            delta[fullKey] = checked;
        }
        emit valueChanged(ConnectedID, Name, delta);
    });
    // Set row height and add widget
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, checkboxWidget);
    qDebug() << "setupBooleanCell: Assigned checkbox to row:" << row << "for key:" << fullKey << "visible:" << checkBox->isVisible();
}

/* Setup array cell */
void Inspector::setupArrayCell(int row, const QString &fullKey, const QJsonArray &array)
{
    qDebug() << "setupArrayCell: Setting row:" << row << "for key:" << fullKey;
    // Create array widget
    QWidget *arrayWidget = new QWidget();
    arrayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QListWidget *listWidget = new QListWidget();
    listWidget->setProperty("row", row);
    listWidget->viewport()->installEventFilter(this);
    listWidget->setAcceptDrops(true);
    listWidget->setDropIndicatorShown(true);
    listWidget->setDragDropMode(QAbstractItemView::DropOnly);
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listWidget->setMaximumHeight(fullKey == "entity" ? 50 : 200);
    // Style list widget
    listWidget->setStyleSheet(
        "QListWidget { background: #333; border: 1px solid #555; color: white; }"
        "QListWidget::item { color: white; }"
        "QListWidget::item:selected { background-color: #555; }"
        );
    // Populate list with array items
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        QString displayText;
        if (fullKey == "entity" || obj["type"].toString() == "reference") {
            displayText = obj["name"].toString() + (obj["id"].toString().isEmpty() ? "" : " (ID: " + obj["id"].toString() + ")");
        } else if (fullKey == "trajectories" && obj.contains("position")) {
            QJsonObject pos = obj["position"].toObject();
            displayText = QString("(%1, %2)")
                              .arg(formatNumberForUI(pos["x"].toDouble()))
                              .arg(formatNumberForUI(pos["y"].toDouble()));
        } else {
            displayText = obj["name"].toString();
            if (displayText.isEmpty()) {
                displayText = QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            }
        }
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        listWidget->addItem(item);
    }
    // Create layout for array widget
    QVBoxLayout *layout = new QVBoxLayout(arrayWidget);
    layout->addWidget(listWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    // Add buttons for non-entity arrays
    QPushButton *addBtn = nullptr;
    QPushButton *removeBtn = nullptr;
    if (fullKey != "entity") {
        addBtn = new QPushButton("➕");
        removeBtn = new QPushButton("❌");
        addBtn->setStyleSheet(
            "QPushButton { color: white; border-radius: 3px; background-color: #2A3F54; }"
            "QPushButton:hover { background-color: #444; }"
            );
        removeBtn->setStyleSheet(
            "QPushButton { color: white; border-radius: 3px; background-color: #2A3F54; }"
            "QPushButton:hover { background-color: #444; }"
            );
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(removeBtn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(5);
        layout->addLayout(btnLayout);
    }
    // Lambda to emit array changes
    auto emitArrayChanged = [=]() {
        QJsonObject delta;
        if (fullKey == "entity") {
            if (listWidget->count() > 0) {
                QVariantMap itemData = listWidget->item(0)->data(Qt::UserRole).toMap();
                delta[fullKey] = QJsonObject::fromVariantMap(itemData);
            } else {
                delta[fullKey] = QJsonObject();
            }
        } else {
            QJsonArray updatedArray;
            for (int i = 0; i < listWidget->count(); ++i) {
                QVariantMap itemData = listWidget->item(i)->data(Qt::UserRole).toMap();
                updatedArray.append(QJsonObject::fromVariantMap(itemData));
            }
            delta[fullKey] = updatedArray;
        }
        emit valueChanged(ConnectedID, Name, delta);
        if (fullKey == "trajectories") {
            emit trajectoryWaypointsChanged(ConnectedID, delta[fullKey].toArray());
        }
    };
    // Connect add and remove buttons
    if (fullKey != "entity") {
        connect(addBtn, &QPushButton::clicked, this, [=]() {
            if (fullKey == "trajectories") {
                QJsonObject newObj;
                QJsonObject posObj;
                posObj["type"] = "vector";
                posObj["x"] = 0.0;
                posObj["y"] = 0.0;
                posObj["z"] = 0.0;
                newObj["position"] = posObj;
                QString displayText = QString("(%1, %2)")
                                          .arg(formatNumberForUI(0.0))
                                          .arg(formatNumberForUI(0.0));
                QListWidgetItem *newItem = new QListWidgetItem(displayText);
                newItem->setData(Qt::UserRole, newObj.toVariantMap());
                listWidget->addItem(newItem);
                listWidget->setCurrentItem(newItem);
                emitArrayChanged();
            } else {
                QJsonObject newObj;
                newObj["name"] = "Unnamed";
                QString displayText = "Unnamed";
                QListWidgetItem *newItem = new QListWidgetItem(displayText);
                newItem->setData(Qt::UserRole, newObj.toVariantMap());
                listWidget->addItem(newItem);
                listWidget->setCurrentItem(newItem);
                emitArrayChanged();
            }
        });
        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                delete listWidget->takeItem(listWidget->row(item));
                emitArrayChanged();
            }
        });
    }
    // Connect list item changes
    connect(listWidget, &QListWidget::itemChanged, this, [=](QListWidgetItem *item) {
        QString itemText = item->text();
        QVariantMap itemData = item->data(Qt::UserRole).toMap();
        QJsonObject json = QJsonObject::fromVariantMap(itemData);
        emitArrayChanged();
    });
    // Connect double-click to focus entity
    connect(listWidget, &QListWidget::doubleClicked, this, [=](const QModelIndex &index) {
        QListWidgetItem *item = listWidget->item(index.row());
        if (!item) return;
        QString itemText = item->text();
        QVariantMap itemData = item->data(Qt::UserRole).toMap();
        QJsonObject json = QJsonObject::fromVariantMap(itemData);
        if (itemData.contains("id") && !itemData["id"].toString().isEmpty()) {
            emit foucsEntity(itemData["id"].toString());
        }
        emitArrayChanged();
    });
    // Set row height and add widget
    tableWidget->setRowHeight(row, fullKey == "entity" ? 80 : 200);
    tableWidget->setCellWidget(row, 1, arrayWidget);
}

/* Setup string cell */
void Inspector::setupStringCell(int row, const QString &fullKey, const QString &value)
{
    qDebug() << "setupStringCell: Key:" << fullKey << "Value:" << value;
    // Create key item if not exists
    QTableWidgetItem *keyItem = tableWidget->item(row, 0);
    if (!keyItem) {
        keyItem = new QTableWidgetItem(fullKey.split(".").last());
        keyItem->setBackground(QColor("#111"));
        keyItem->setForeground(QColor(Qt::white));
        keyItem->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(row, 0, keyItem);
    }
    // Create input field
    QLineEdit *lineEdit = new QLineEdit();
    lineEdit->setText(value);
    lineEdit->setStyleSheet(
        "QLineEdit { color: white !important; padding: 2px; }"
        "QLineEdit:focus { border: 1px solid #777; background: #444; }"
        );
    lineEdit->setFrame(true);
    lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Set read-only for specific fields
    if (fullKey.contains("id") || fullKey.contains("name") || fullKey.contains("branch")) {
        lineEdit->setReadOnly(true);
    }
    // Set row height and add widget
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, lineEdit);
    // Connect input changes
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QString newValue = lineEdit->text();
        QStringList parts = fullKey.split(".");
        QJsonObject delta;
        if (parts.size() == 1) {
            delta[parts[0]] = newValue;
        } else {
            delta[parts[0]] = QJsonObject{{parts[1], newValue}};
        }
        emit valueChanged(ConnectedID, Name, delta);
        qDebug() << "String cell updated: Key:" << fullKey << "Value:" << newValue << "Foreground Color:" << lineEdit->palette().text().color().name();
    });
    qDebug() << "setupStringCell: Key:" << fullKey << "Value:" << value << "Foreground Color:" << lineEdit->palette().text().color().name();
}

/* Setup number cell */
void Inspector::setupNumberCell(int row, const QString &fullKey, double value)
{
    qDebug() << "setupNumberCell: Setting row:" << row << "for key:" << fullKey << "value:" << value;
    // Create input field
    WheelableLineEdit *lineEdit = new WheelableLineEdit();
    lineEdit->setText(formatNumberForUI(value));
    lineEdit->setStyleSheet(
        "QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }"
        );
    // Set validator
    QDoubleValidator *validator = new QDoubleValidator(lineEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(4);
    lineEdit->setValidator(validator);
    // Set row height and add widget
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, lineEdit);
    // Connect input changes
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QJsonObject delta;
        delta[fullKey] = lineEdit->text().toDouble();
        emit valueChanged(ConnectedID, Name, delta);
    });
}

/* Handle remove parameter action */
void Inspector::handleRemoveParameter()
{
    // Remove selected row
    int selectedRow = tableWidget->currentRow();
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
        // Emit parameter changed signal
        emit parameterChanged(ConnectedID, Name, selectedKey, "", false);
    }
}

/* Handle drop events for lists */
bool Inspector::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        QObject *listViewport = watched;
        QListWidget *listWidget = qobject_cast<QListWidget *>(listViewport->parent());
        if (listWidget && listWidget->property("row").isValid()) {
            int row = listWidget->property("row").toInt();
            QString key = rowToKeyPath[row];
            const QMimeData *mime = dropEvent->mimeData();
            if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
                QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
                QDataStream stream(&encoded, QIODevice::ReadOnly);
                while (!stream.atEnd()) {
                    int row, col;
                    QMap<int, QVariant> roleDataMap;
                    stream >> row >> col >> roleDataMap;
                    QString text = roleDataMap.value(Qt::DisplayRole).toString();
                    QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();
                    QJsonObject json = QJsonObject::fromVariantMap(customData);
                    // Handle entity drop
                    if (key == "entity") {
                        if (customData["type"].toString() != "entity" ||
                            !customData.contains("name") || customData["name"].toString().isEmpty() ||
                            !customData.contains("ID") || customData["ID"].toString().isEmpty()) {
                            continue;
                        }
                        listWidget->clear();
                        QJsonObject refObj;
                        refObj["type"] = "reference";
                        refObj["name"] = customData["name"].toString();
                        refObj["id"] = customData["ID"].toString();
                        QString displayText = refObj["name"].toString() + " (ID: " + refObj["id"].toString() + ")";
                        QListWidgetItem *newItem = new QListWidgetItem(displayText);
                        newItem->setData(Qt::UserRole, refObj.toVariantMap());
                        listWidget->addItem(newItem);
                        QJsonObject delta;
                        delta[key] = refObj;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
                    // Handle trajectory drop
                    else if (key == "trajectories") {
                        QJsonObject newObj;
                        QJsonObject posObj;
                        posObj["type"] = "vector";
                        if (customData["type"].toString() == "entity" && hierarchy) {
                            QJsonObject transformData = hierarchy->getComponentData(customData["ID"].toString(), "transform");
                            posObj = transformData.contains("position") ? transformData["position"].toObject() : QJsonObject{{"x", 0.0}, {"y", 0.0}, {"z", 0.0}};
                        } else if (json.contains("position")) {
                            posObj = json["position"].toObject();
                        } else {
                            posObj["x"] = 0.0;
                            posObj["y"] = 0.0;
                            posObj["z"] = 0.0;
                        }
                        newObj["position"] = posObj;
                        QString displayText = QString("(%1, %2)")
                                                  .arg(formatNumberForUI(posObj["x"].toDouble()))
                                                  .arg(formatNumberForUI(posObj["y"].toDouble()));
                        QListWidgetItem *newItem = new QListWidgetItem(displayText);
                        newItem->setData(Qt::UserRole, newObj.toVariantMap());
                        listWidget->addItem(newItem);
                        QJsonArray updatedArray;
                        for (int i = 0; i < listWidget->count(); ++i) {
                            QVariantMap itemData = listWidget->item(i)->data(Qt::UserRole).toMap();
                            updatedArray.append(QJsonObject::fromVariantMap(itemData));
                        }
                        QJsonObject delta;
                        delta[key] = updatedArray;
                        emit valueChanged(ConnectedID, Name, delta);
                        emit trajectoryWaypointsChanged(ConnectedID, updatedArray);
                    } else {
                        QListWidgetItem *newItem = new QListWidgetItem(text);
                        newItem->setData(Qt::UserRole, customData);
                        listWidget->addItem(newItem);
                        QJsonArray updatedArray;
                        for (int i = 0; i < listWidget->count(); ++i) {
                            QVariantMap itemData = listWidget->item(i)->data(Qt::UserRole).toMap();
                            updatedArray.append(QJsonObject::fromVariantMap(itemData));
                        }
                        QJsonObject delta;
                        delta[key] = updatedArray;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
                }
                dropEvent->acceptProposedAction();
            }
            return true;
        }
    }
    return QDockWidget::eventFilter(watched, event);
}

/* Initialize inspector with data */
void Inspector::init(QString ID, QString name, QJsonObject object)
{
    qDebug() << "init: Initializing for ID:" << ID << "name:" << name << "object:" << object;
    // Set connected ID and name
    ConnectedID = ID;
    Name = name.toLower();
    // Normalize specific component names
    if (name.compare("Trajectories", Qt::CaseInsensitive) == 0) {
        Name = QString("trajectory");
    } else if (name.compare("dynamicModel", Qt::CaseInsensitive) == 0) {
        Name = QString("dynamicModel");
    } else if (name.compare("meshRenderer2d", Qt::CaseInsensitive) == 0) {
        Name = QString("meshRenderer2d");
    }
    titleLabel->setText(name);
    // Clear table
    tableWidget->clearContents();
    tableWidget->blockSignals(true);
    rowToKeyPath.clear();
    customParameterKeys.clear();
    // Fetch component data if empty
    if ((Name == QString("trajectory") || Name == QString("dynamicModel") ||
         Name == QString("meshRenderer2d") || Name == QString("collider")) &&
        object.isEmpty() && hierarchy) {
        QString dataType = Name;
        object = hierarchy->getComponentData(ID, dataType);
    }
    // Calculate row count
    int rowCount = 0;
    for (const QString &key : object.keys()) {
        if (key == "modeConfiguration") {
            QJsonObject subObj = object[key].toObject();
            rowCount += subObj.size();
        } else if (object[key].isArray()) {
            rowCount += 1;
        } else if (object[key].isObject()) {
            QJsonObject subObj = object[key].toObject();
            if (key == "entity" || subObj["type"].toString() == "reference") {
                rowCount += 1;
            } else {
                rowCount += subObj.size();
            }
        } else {
            rowCount += 1;
        }
    }
    tableWidget->setRowCount(rowCount);
    qDebug() << "init: Setting row count to:" << rowCount;
    // Add rows
    int row = 0;
    for (const QString &key : object.keys()) {
        row = addSimpleRow(row, key, object[key]);
        if (Name == "collider" && key != "active" && key != "radius" &&
            key != "width" && key != "length" && key != "height" &&
            key != "collider") {
            customParameterKeys.insert(key);
        }
    }
    tableWidget->blockSignals(false);
    tableWidget->resizeRowsToContents();
    tableWidget->resizeColumnsToContents();
    // Scroll to last row if exists
    if (rowToKeyPath.contains(rowCount - 1)) {
        tableWidget->scrollToItem(tableWidget->item(rowCount - 1, 0), QAbstractItemView::PositionAtTop);
        qDebug() << "init: Scrolled to last row";
    }
    tableWidget->viewport()->update();
    qDebug() << "init: Table updated, rows:" << tableWidget->rowCount() << "visible rows:" << tableWidget->visibleRegion().rects().size();
}

/* Add simple row to table */
int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
    qDebug() << "addSimpleRow: Adding row for key:" << key << "at row:" << row << "type:" << value.type();
    rowToKeyPath[row] = key;
    // Create key item
    QTableWidgetItem *keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setBackground(QColor("#111"));
    keyItem->setForeground(Qt::white);
    tableWidget->setItem(row, 0, keyItem);
    // Handle object values
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        if (type == "component" || type == "parameter") {
            qDebug() << "addSimpleRow: Skipping component or parameter type for key:" << key;
            return row;
        }
        // Handle modeConfiguration
        if (key == "modeConfiguration") {
            qDebug() << "addSimpleRow: Handling modeConfiguration for key:" << key;
            QStringList subKeys = {"mode1", "mode2", "mode3A", "mode4", "modeC"};
            int currentRow = row;
            for (const QString &subKey : subKeys) {
                QString fullKey = QString("modeConfiguration.%1").arg(subKey);
                rowToKeyPath[currentRow] = fullKey;
                QTableWidgetItem *subKeyItem = new QTableWidgetItem(fullKey);
                subKeyItem->setFlags(Qt::ItemIsEnabled);
                subKeyItem->setBackground(QColor("#111"));
                subKeyItem->setForeground(Qt::white);
                tableWidget->setItem(currentRow, 0, subKeyItem);
                setupStringCell(currentRow, fullKey, obj.value(subKey).toString());
                qDebug() << "addSimpleRow: Added modeConfiguration subkey:" << fullKey << "at row:" << currentRow;
                currentRow++;
            }
            return currentRow;
        }
    }
    // Handle entity and arrays
    if (key == "entity" && (value.isObject() || value.isNull())) {
        QJsonArray singleItemArray;
        if (value.isObject()) {
            singleItemArray.append(value.toObject());
        }
        setupArrayCell(row, key, singleItemArray);
    } else if (value.isArray()) {
        setupArrayCell(row, key, value.toArray());
    } else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        // Setup specialized templates
        if (type == "vector") {
            VectorTemplate *vector = new VectorTemplate(this);
            vector->setConnectedID(ConnectedID);
            vector->setName(Name);
            vector->setupVectorCell(row, key, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "geocord" || type == "geooffset") {
            GeocordsTemplate *geocords = new GeocordsTemplate(this);
            geocords->setConnectedID(ConnectedID);
            geocords->setName(Name);
            geocords->setupGeocordsCell(row, key, obj, tableWidget);
            connect(geocords, &GeocordsTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "option") {
            OptionTemplate *option = new OptionTemplate(this);
            option->setConnectedID(ConnectedID);
            option->setName(Name);
            option->setupOptionCell(row, key, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setupColorCell(row, key, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setupImageCell(row, key, obj, tableWidget);
            connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
        } else {
            setupGenericObjectCell(row, key, obj);
        }
    } else {
        setupValueCell(row, key, value);
    }
    return row + 1;
}

/* Setup value cell based on type */
void Inspector::setupValueCell(int row, const QString &fullKey, const QJsonValue &value)
{
    qDebug() << "setupValueCell: Processing key:" << fullKey << "type:" << value.type();
    // Handle different value types
    if (value.isBool()) {
        setupBooleanCell(row, fullKey, value.toBool());
    } else if (value.isArray()) {
        setupArrayCell(row, fullKey, value.toArray());
    } else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        // Setup specialized templates
        if (type == "vector") {
            VectorTemplate *vector = new VectorTemplate(this);
            vector->setConnectedID(ConnectedID);
            vector->setName(Name);
            vector->setupVectorCell(row, fullKey, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "geocord" || type == "geooffset") {
            GeocordsTemplate *geocords = new GeocordsTemplate(this);
            geocords->setConnectedID(ConnectedID);
            geocords->setName(Name);
            geocords->setupGeocordsCell(row, fullKey, obj, tableWidget);
            connect(geocords, &GeocordsTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "option") {
            OptionTemplate *option = new OptionTemplate(this);
            option->setConnectedID(ConnectedID);
            option->setName(Name);
            option->setupOptionCell(row, fullKey, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setupColorCell(row, fullKey, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        } else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setupImageCell(row, fullKey, obj, tableWidget);
            connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
        } else {
            setupGenericObjectCell(row, fullKey, obj);
        }
    } else if (value.isString()) {
        setupStringCell(row, fullKey, value.toString());
    } else {
        setupNumberCell(row, fullKey, value.toDouble());
    }
}

/* Setup generic object cell */
void Inspector::setupGenericObjectCell(int row, const QString &fullKey, const QJsonObject &obj)
{
    qDebug() << "setupGenericObjectCell: Setting row:" << row << "for key:" << fullKey;
    // Create widget for object
    QWidget *valueWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(valueWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    // Add subfields
    for (const QString &subKey : obj.keys()) {
        QHBoxLayout *subLayout = new QHBoxLayout();
        QLabel *label = new QLabel(subKey);
        label->setStyleSheet("color: white; min-width: 20px;");
        subLayout->addWidget(label);
        WheelableLineEdit *edit = new WheelableLineEdit();
        edit->setText(formatNumberForUI(obj[subKey].toDouble()));
        edit->setStyleSheet("QLineEdit { background: #333; border: 1px solid #555; border-radius: 3px; color: white; }");
        QDoubleValidator *validator = new QDoubleValidator(edit);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setDecimals(4);
        edit->setValidator(validator);
        edit->setObjectName(subKey);
        subLayout->addWidget(edit);
        layout->addLayout(subLayout);
        // Connect input changes
        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            QJsonObject delta;
            delta[fullKey] = QJsonObject{{subKey, edit->text().toDouble()}};
            emit valueChanged(ConnectedID, Name, delta);
        });
    }
    // Set row height and add widget
    tableWidget->setRowHeight(row, 30 * obj.size());
    tableWidget->setCellWidget(row, 1, valueWidget);
}

/* Update trajectory waypoints */
void Inspector::updateTrajectory(QString entityId, QJsonArray waypoints)
{
    qDebug() << "updateTrajectory: Updating for entityId:" << entityId;
    // Find trajectory row
    int trajRow = -1;
    for (int r = 0; r < tableWidget->rowCount(); ++r) {
        if (rowToKeyPath[r] == "trajectories") {
            trajRow = r;
            break;
        }
    }
    // Create new row if needed
    if (trajRow == -1) {
        tableWidget->setRowCount(tableWidget->rowCount() + 1);
        trajRow = tableWidget->rowCount() - 1;
        rowToKeyPath[trajRow] = "trajectories";
        QTableWidgetItem *keyItem = new QTableWidgetItem("trajectories");
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#111"));
        keyItem->setForeground(Qt::white);
        tableWidget->setItem(trajRow, 0, keyItem);
        setupArrayCell(trajRow, "trajectories", QJsonArray());
    }
    // Get array widget
    QWidget *arrayWidget = tableWidget->cellWidget(trajRow, 1);
    if (!arrayWidget) {
        setupArrayCell(trajRow, "trajectories", QJsonArray());
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
    }
    QListWidget *listWidget = arrayWidget->findChild<QListWidget*>();
    if (!listWidget) return;
    // Update inspector if needed
    if (ConnectedID != entityId || Name != "trajectory") {
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
        if (trajRow == -1) return;
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
        listWidget = arrayWidget->findChild<QListWidget*>();
        if (!listWidget) return;
    }
    // Clear and populate list
    listWidget->clear();
    for (const QJsonValue &val : waypoints) {
        QJsonObject obj = val.toObject();
        if (!obj.contains("position")) continue;
        QJsonObject pos = obj["position"].toObject();
        QString displayText = QString("(%1, %2)")
                                  .arg(formatNumberForUI(pos["x"].toDouble()))
                                  .arg(formatNumberForUI(pos["y"].toDouble()));
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        listWidget->addItem(item);
    }
    // Populate from hierarchy if empty
    if (waypoints.isEmpty() && hierarchy) {
        QJsonObject trajData = hierarchy->getComponentData(ConnectedID, "trajectory");
        if (!trajData.isEmpty()) {
            QJsonArray array = trajData["trajectories"].toArray();
            for (const QJsonValue &val : array) {
                QJsonObject obj = val.toObject();
                if (!obj.contains("position")) continue;
                QJsonObject pos = obj["position"].toObject();
                QString displayText = QString("(%1, %2)")
                                          .arg(formatNumberForUI(pos["x"].toDouble()))
                                          .arg(formatNumberForUI(pos["y"].toDouble()));
                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, obj.toVariantMap());
                listWidget->addItem(item);
            }
        }
    }
    // Update table viewport
    tableWidget->viewport()->update();
}
