/* ========================================================================= */
/* File: inspector.cpp                                                     */
/* Purpose: Implements inspector widget for editing component properties     */
/* ========================================================================= */

#include "inspector.h"                             // For inspector class
#include "qjsondocument.h"                         // For JSON document handling
#include "qtimer.h"
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
#include <QInputDialog>
#include <QScrollArea>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <GUI/Settings/applicationdialog.h>

static bool safeAddWidget(QLayout* layout, QWidget* widget, const QString& context = "") {
    if (!layout) {
        return false;
    }
    if (!widget) {
        return false;
    }
    layout->addWidget(widget);
    return true;
}

// %%% Utility Functions %%%
/* Format number for UI display */
QString Inspector::formatNumberForUI(double value)
{
    if (qFuzzyCompare(value, qRound(value))) {
        return QString::number(qRound(value));
    }
    QString result = QString::number(value, 'f', 6).trimmed();
    while (result.endsWith('0')) result.chop(1);
    if (result.endsWith('.')) result.chop(1);
    return result;
}

// %%% WheelableLineEdit Implementation %%%
/* Initialize wheelable line edit */
WheelableLineEdit::WheelableLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setAlignment(Qt::AlignCenter);
}

/* Handle mouse wheel events */
void WheelableLineEdit::wheelEvent(QWheelEvent *event)
{
    if (hasFocus()) {
        double step = event->angleDelta().y() > 0 ? 1.0 : -1.0;
        if (event->modifiers() & Qt::ControlModifier) step *= 0.1;
        else if (event->modifiers() & Qt::ShiftModifier) step *= 10.0;
        bool ok;
        double value = text().toDouble(&ok);
        if (ok) {
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
    setupUI();
}

// %%% Lock State Management %%%
/* Set inspector lock state */
void Inspector::setLocked(bool locked)
{
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
    titleBarWidget = new QWidget(this);
    if (!titleBarWidget) {
        return;
    }
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBarWidget);
    if (!titleLayout) {
        delete titleBarWidget;
        return;
    }
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);
    // Create title label
    titleLabel = new QLabel("Inspector", titleBarWidget);
    if (!titleLabel) {
        delete titleLayout;
        delete titleBarWidget;
        return;
    }
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: black; background-color: white; padding: 5px;"
        );
    titleLabel->setAlignment(Qt::AlignCenter);
    menuButton = new QPushButton("⋮", titleBarWidget);
    if (!menuButton) {
        delete titleLabel;
        delete titleLayout;
        delete titleBarWidget;
        return;
    }
    menuButton->setStyleSheet(
        "QPushButton { font-size: 16px; color: black; background-color: white; border: 1px solid #ccc; padding: 5px 10px; }"
        "QPushButton:hover { background-color: #f0f0f0; }"
        );
    menuButton->setFixedWidth(30);
    if (!safeAddWidget(titleLayout, titleLabel, "setupTitleBar - titleLabel") ||
        !safeAddWidget(titleLayout, menuButton, "setupTitleBar - menuButton")) {
        delete menuButton;
        delete titleLabel;
        delete titleLayout;
        delete titleBarWidget;
        return;
    }
    connect(menuButton, &QPushButton::clicked, this, [this]() {
        QMenu *menu = createContextMenu();
        if (menu && menuButton) {
            menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
        }
    });
}
/* Create context menu for title bar */
QMenu* Inspector::createContextMenu()
{
    QMenu *menu = new QMenu(this);
    if (!menu) {
        return nullptr;
    }
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
    connect(addTabAction, &QAction::triggered, this, &Inspector::handleAddTab);
    connect(closeAction, &QAction::triggered, this, [](){});
    return menu;
}
/* Copy current component data */
void Inspector::copyCurrentComponent()
{
    if (!hierarchy || Name.isEmpty() || ConnectedID.isEmpty()) {
        return;
    }
    copiedComponentData = hierarchy->getComponentData(ConnectedID, Name);
    if (copiedComponentData.isEmpty()) {
        return;
    }
    copiedComponentType = Name;
}
void Inspector::pasteToCurrentComponent()
{
    if (!hierarchy || Name.isEmpty() || ConnectedID.isEmpty() || copiedComponentData.isEmpty()) {
        return;
    }
    if (Name == copiedComponentType) {
        QString changedId = ConnectedID + "_paste_" + Name;
        QJsonObject deltaWithId = copiedComponentData;
        deltaWithId["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, deltaWithId);
        init(ConnectedID, Name, hierarchy->getComponentData(ConnectedID, Name));
    }
}
/* Handle add tab action */
void Inspector::handleAddTab()
{
    emit addTabRequested();
}
void Inspector::setupUI()
{
    QWidget *container = new QWidget(this);
    if (!container) {
        return;
    }
    QVBoxLayout *layout = new QVBoxLayout(container);
    if (!layout) {
        delete container;
        return;
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setupTitleBar();
    if (titleBarWidget) {
        setTitleBarWidget(titleBarWidget);
    }
    tableWidget = new QTableWidget(5, 2, this);
    if (!tableWidget) {
        delete layout;
        delete container;
        return;
    }
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);
    // tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Set initial column behavior
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setStyleSheet(
        "QTableWidget { background-color: white; color: black; border: 1px solid #ccc; }"
        "QTableWidget::item { border: 1px solid #ddd; color: black; }"
        "QTableWidget::item:selected { background-color: #e6f3ff; color: black; }"
        );
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setStyleSheet("alternate-background-color: #f9f9f9; background-color: white;");
    if (!safeAddWidget(layout, tableWidget, "setupUI - tableWidget")) {
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    if (!buttonLayout) {
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }
    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QPushButton *addButton = new QPushButton("Add", this);
    if (!addButton) {
        delete buttonLayout;
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }
    addButton->setFixedSize(40, 25);
    addButton->setStyleSheet(
        "QPushButton { color: black; border: 1px solid #ccc; border-radius: 3px; background-color: #e9ecef; }"
        "QPushButton:hover { background-color: #dde1e4; }"
        );
    addButton->setToolTip("Add new parameter");
    buttonLayout->addSpacerItem(spacer);
    if (!safeAddWidget(buttonLayout, addButton, "setupUI - addButton to buttonLayout")) {
        delete addButton;
        delete buttonLayout;
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(10, 5, 10, 5);
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
        QString changedId = ConnectedID + "_" + keyPath;
        delta["_changedId"] = changedId;
        delta["_entityId"] = ConnectedID;
        delta["_component"] = Name;
        delta["_parameter"] = keyPath;
        emit valueChanged(ConnectedID, Name, delta);
    });
    connect(addButton, &QPushButton::clicked, this, &Inspector::handleAddParameter);
}
/* Create remove button for parameter */
QPushButton* Inspector::createRemoveButton(const QString &parameterName)
{
    QPushButton *removeButton = new QPushButton("❌", this);
    if (!removeButton) {
        return nullptr;
    }
    removeButton->setFixedSize(20, 20);
    removeButton->setStyleSheet(
        "QPushButton { color: black; border-radius: 3px; background-color: #e9ecef; border: 1px solid #ccc; }"
        "QPushButton:hover { background-color: #dde1e4; }"
        );
    removeButton->setProperty("parameterName", parameterName);
    connect(removeButton, &QPushButton::clicked, this, [=]() {
        QPushButton *senderButton = qobject_cast<QPushButton*>(sender());
        if (!senderButton) return;
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
        if (currentRow == -1) return;
        QString key = rowToKeyPath.value(currentRow);
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
        QJsonObject delta;
        delta["_id"] = mainID;
        delta[parameterName] = QJsonValue();
        emit parameterChanged(ConnectedID, Name, key, "", false);
        emit valueChanged(ConnectedID, Name, delta);
    });
    return removeButton;
}
/* Add parameter row to table */
void Inspector::addParameterRow(const QString &parameterName, int row)
{
    tableWidget->setRowCount(row + 1);
    rowToKeyPath[row] = parameterName;
    customParameterKeys.insert(parameterName);
    QTableWidgetItem *keyItem = new QTableWidgetItem(parameterName);
    if (keyItem) {
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#f8f9fa"));
        keyItem->setForeground(Qt::black);
        tableWidget->setItem(row, 0, keyItem);
    }
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
            addParameterRow(parameterName, row);
            QJsonObject delta;
            delta["_id"] = mainID;
            if (parameterType == "string") {
                setupStringCell(row, parameterName, parameterValue);
                delta[parameterName] = parameterValue;
            }
            else if (parameterType == "number") {
                setupNumberCell(row, parameterName, parameterValue.toDouble());
                delta[parameterName] = parameterValue.toDouble();
            }
            else if (parameterType == "boolean") {
                setupBooleanCell(row, parameterName, parameterValue.toLower() == "true");
                delta[parameterName] = (parameterValue.toLower() == "true");
            }
            else if (parameterType == "unitParam") {
                QJsonObject unitParamObj;
                unitParamObj["type"] = "unitParam";
                unitParamObj["value"] = parameterValue.toDouble();
                unitParamObj["unit"] = "unit";
                setupUnitParameterCell(row, parameterName, unitParamObj);
                delta[parameterName] = unitParamObj;
            }
            else if (parameterType == "vector") {
                VectorTemplate *vector = new VectorTemplate(this);
                vector->setConnectedID(ConnectedID);
                vector->setName(Name);
                vector->setMainID(mainID);
                QJsonObject vectorData;
                QStringList components = parameterValue.split(",");
                vectorData["x"] = components.size() > 0 ? components[0].toDouble() : 0.0;
                vectorData["y"] = components.size() > 1 ? components[1].toDouble() : 0.0;
                vectorData["z"] = components.size() > 2 ? components[2].toDouble() : 0.0;
                vectorData["type"] = "vector";
                vector->setupVectorCell(row, parameterName, vectorData, tableWidget);
                connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = vectorData;
            }
            else if (parameterType == "geocord") {
                GeocordsTemplate *geocords = new GeocordsTemplate(this, this);
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
            }
            else if (parameterType == "option") {
                OptionTemplate *option = new OptionTemplate(this);
                option->setConnectedID(ConnectedID);
                option->setName(Name);
                option->setMainID(mainID);
                QJsonObject optionObj;
                optionObj["value"] = parameterValue;
                optionObj["options"] = QJsonArray{"Option1", "Option2"};
                optionObj["type"] = "option";
                option->setupOptionCell(row, parameterName, optionObj, tableWidget);
                connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = optionObj;
            }
            else if (parameterType == "color") {
                ColorTemplate *color = new ColorTemplate(this);
                color->setConnectedID(ConnectedID);
                color->setName(Name);
                color->setMainID(mainID);
                QJsonObject colorObj;
                colorObj["value"] = parameterValue;
                colorObj["type"] = "color";
                color->setupColorCell(row, parameterName, colorObj, tableWidget);
                connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = colorObj;
            }
            else if (parameterType == "image") {
                ImageTemplate *image = new ImageTemplate(this);
                image->setConnectedID(ConnectedID);
                image->setName(Name);
                image->setMainID(mainID);
                QJsonObject spriteObj;
                spriteObj["value"] = parameterValue;
                spriteObj["type"] = "image";
                image->setupImageCell(row, parameterName, spriteObj, tableWidget);
                connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
                delta[parameterName] = spriteObj;
            }
            tableWidget->setRowHeight(row, parameterType == "image" ? ImageTemplate::ROW_HEIGHT : 30);
            tableWidget->viewport()->update();
            emit parameterChanged(ConnectedID, Name, parameterName, parameterType, true);
            emit valueChanged(ConnectedID, Name, delta);
        }
    }
}
/* Setup boolean cell */
void Inspector::setupBooleanCell(int row, const QString &fullKey, bool value)
{
    QWidget *checkboxWidget = new QWidget();
    if (!checkboxWidget) {
        return;
    }
    QCheckBox *checkBox = new QCheckBox();
    if (!checkBox) {
        delete checkboxWidget;
        return;
    }
    checkBox->setChecked(value);
    checkBox->setStyleSheet(
        "QCheckBox { color: black; border: none; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #666; background-color: white; subcontrol-origin: padding; subcontrol-position: center; }"
        "QCheckBox::indicator:checked { image: url(:/icons/images/check-box.png); background-color: #007bff; }"
        "QCheckBox::indicator:unchecked { image: none; background-color: white; }"
        "QCheckBox::indicator:hover { border: 1px solid #007bff; }"
        );
    QHBoxLayout *layout = new QHBoxLayout(checkboxWidget);
    if (!layout) {
        delete checkBox;
        delete checkboxWidget;
        return;
    }
    if (!safeAddWidget(layout, checkBox, "setupBooleanCell - checkBox to layout")) {
        delete checkBox;
        delete checkboxWidget;
        return;
    }
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
        QJsonObject delta;
        if (fullKey.contains(".")) {
            QStringList parts = fullKey.split(".");
            delta[parts[0]] = QJsonObject{{parts[1], checked}};
        } else {
            delta[fullKey] = checked;
        }
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, checkboxWidget);
}
void Inspector::setupArrayCell(int row, const QString &fullKey, const QJsonArray &array)
{
    QWidget *arrayWidget = new QWidget();
    if (!arrayWidget) {
        return;
    }
    arrayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QPushButton *dropdownButton = new QPushButton("▼", this);
    if (!dropdownButton) {
        delete arrayWidget;
        return;
    }
    dropdownButton->setStyleSheet(
        "QPushButton { color: black; border-radius: 3px; padding: 2px 8px; background-color: #e9ecef; border: 1px solid #ccc; }"
        "QPushButton:hover { background: #dde1e4; }"
        "QPushButton:pressed { background: #ced4da; }"
        );
    dropdownButton->setFixedSize(30, 25);
    dropdownButton->setCheckable(true);
    QListWidget *listWidget = new QListWidget();
    if (!listWidget) {
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    listWidget->setProperty("row", row);
    listWidget->viewport()->installEventFilter(this);
    int fixedListHeight = 200;
    int itemHeight = 30;
    if (fullKey == "trajectories") {
        listWidget->setAcceptDrops(false);
        listWidget->setFixedHeight(fixedListHeight);
        listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        listWidget->setAcceptDrops(true);
        listWidget->setDropIndicatorShown(true);
        listWidget->setDragDropMode(QAbstractItemView::DropOnly);
        int maxVisibleItems = 5;
        int listHeight = qMin(array.size(), maxVisibleItems) * itemHeight + 10;
        listWidget->setMaximumHeight(listHeight);
    }
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    listWidget->setVisible(false);
    listWidget->setStyleSheet(
        "QListWidget { background: white; border: 1px solid #ccc; color: black; }"
        "QListWidget::item { color: black; border-bottom: 1px solid #eee; padding: 8px; height: 35px; }" // Fixed item height
        "QListWidget::item:selected { background-color: #e6f3ff; color: black; }"
        );
    int itemsPerView = fixedListHeight / itemHeight;
    listWidget->setProperty("itemsPerView", itemsPerView);
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        QString displayText;

        if (fullKey == "entity" || obj["type"].toString() == "reference") {
            QString name = obj["name"].toString();
            displayText = capitalizeFirstLetter(name) +
                          (obj["id"].toString().isEmpty() ? "" : " (ID: " + obj["id"].toString() + ")");
        }
        else if (fullKey == "trajectories" && obj.contains("position")) {
            QJsonObject pos = obj["position"].toObject();
            displayText = QString("Lat: %1, Alt: %2, Lon: %3")
                              .arg(formatNumberForUI(pos["x"].toDouble()))
                              .arg(formatNumberForUI(pos["y"].toDouble()))
                              .arg(formatNumberForUI(pos["z"].toDouble()));
        }
        else {
            displayText = capitalizeFirstLetter(obj["name"].toString());
            if (displayText.isEmpty()) {
                displayText = QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                if (!displayText.isEmpty() && displayText[0].isLetter()) {
                    displayText[0] = displayText[0].toUpper();
                }
            }
        }
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setSizeHint(QSize(0, itemHeight));
        listWidget->addItem(item);
    }
    QVBoxLayout *mainLayout = new QVBoxLayout(arrayWidget);
    if (!mainLayout) {
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    QWidget *topRowWidget = new QWidget(arrayWidget);
    if (!topRowWidget) {
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    QHBoxLayout *topRowLayout = new QHBoxLayout(topRowWidget);
    if (!topRowLayout) {
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(5);
    if (!safeAddWidget(topRowLayout, dropdownButton, "setupArrayCell - dropdownButton to topRowLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    topRowLayout->addStretch();
    if (!safeAddWidget(mainLayout, topRowWidget, "setupArrayCell - topRowWidget to mainLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    if (!safeAddWidget(mainLayout, listWidget, "setupArrayCell - listWidget to mainLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }
    QWidget *buttonWidget = nullptr;
    QPushButton *removeBtn = nullptr;
    QPushButton *addBtn = nullptr;
    if (fullKey != "entity" && fullKey != "trajectories") {
        buttonWidget = new QWidget();
        if (buttonWidget) {
            QHBoxLayout *btnLayout = new QHBoxLayout(buttonWidget);
            if (btnLayout) {
                removeBtn = new QPushButton("Remove");
                if (removeBtn) {
                    removeBtn->setStyleSheet(
                        "QPushButton { color: black; border-radius: 3px; background-color: #e9ecef; border: 1px solid #ccc; padding: 3px 8px; }"
                        "QPushButton:hover { background-color: #dde1e4; }"
                        );

                    if (safeAddWidget(btnLayout, removeBtn, "setupArrayCell - removeBtn to btnLayout")) {
                        btnLayout->setContentsMargins(0, 0, 0, 0);
                        btnLayout->setSpacing(5);
                        buttonWidget->setVisible(false);
                        safeAddWidget(mainLayout, buttonWidget, "setupArrayCell - buttonWidget to mainLayout");
                    }
                }
            }
        }
    }
    if (fullKey == "trajectories") {
        buttonWidget = new QWidget();
        if (buttonWidget) {
            QHBoxLayout *btnLayout = new QHBoxLayout(buttonWidget);
            if (btnLayout) {
                addBtn = new QPushButton("Add", this);
                if (addBtn) {
                    addBtn->setStyleSheet(
                        "QPushButton { color: black; border-radius: 3px; background-color: #e9ecef; border: 1px solid #ccc; padding: 3px 8px; }"
                        "QPushButton:hover { background-color: #dde1e4; }"
                        );
                    safeAddWidget(btnLayout, addBtn, "setupArrayCell - addBtn to btnLayout");
                }
                removeBtn = new QPushButton("Remove");
                if (removeBtn) {
                    removeBtn->setStyleSheet(
                        "QPushButton { color: black; border-radius: 3px; background-color: #e9ecef; border: 1px solid #ccc; padding: 3px 8px; }"
                        "QPushButton:hover { background-color: #dde1e4; }"
                        );
                    safeAddWidget(btnLayout, removeBtn, "setupArrayCell - removeBtn to btnLayout");
                }
                btnLayout->setContentsMargins(0, 0, 0, 0);
                btnLayout->setSpacing(5);
                buttonWidget->setVisible(false);
                mainLayout->addWidget(buttonWidget);
            }
        }
    }
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
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
        if (fullKey == "trajectories") {
            emit trajectoryWaypointsChanged(ConnectedID, delta[fullKey].toArray());
        }
    };
    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        bool isVisible = !listWidget->isVisible();
        listWidget->setVisible(isVisible);
        if (buttonWidget) {
            buttonWidget->setVisible(isVisible);
        }
        if (isVisible) {
            dropdownButton->setText("▲");
            int totalHeight = topRowWidget->height();
            if (fullKey == "trajectories") {
                totalHeight += fixedListHeight;
                if (buttonWidget && buttonWidget->isVisible()) {
                    totalHeight += buttonWidget->height();
                }
                totalHeight += 5;
            } else if (fullKey == "entity") {
                totalHeight = 80;
            } else {
                totalHeight = 200;
            }
            tableWidget->setRowHeight(row, totalHeight);
        } else {
            dropdownButton->setText("▼");
            tableWidget->setRowHeight(row, 30);
        }
    });
    if (fullKey != "entity" && removeBtn) {
        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                delete listWidget->takeItem(listWidget->row(item));
                emitArrayChanged();
            }
        });
    }
    if (fullKey == "trajectories" && addBtn) {
        connect(addBtn, &QPushButton::clicked, this, [=]() {
            QDialog *addDialog = new QDialog(this);
            addDialog->setWindowTitle("Add New Waypoint");
            addDialog->setModal(true);
            addDialog->setStyleSheet(
                "QDialog { background-color: #ffffff; } "
                "QLabel { color: #2c3e50; font-weight: bold; } "
                "QDoubleSpinBox { background-color: white; color: black; border: 1px solid #ccc; padding: 5px; } "
                "QPushButton { padding: 8px 15px; border-radius: 3px; font-weight: bold; }"
                );
            QVBoxLayout *dialogLayout = new QVBoxLayout(addDialog);
            dialogLayout->setSpacing(15);
            dialogLayout->setContentsMargins(20, 20, 20, 20);
            QHBoxLayout *latLayout = new QHBoxLayout();
            QLabel *latLabel = new QLabel("Latitude:");
            latLabel->setFixedWidth(150);
            QDoubleSpinBox *latSpinBox = new QDoubleSpinBox();
            latSpinBox->setRange(-9999999.0, 9999999.0);
            latSpinBox->setDecimals(6);
            latSpinBox->setValue(0.0);
            latSpinBox->setSingleStep(0.000001);
            latSpinBox->setFixedWidth(200);
            latLayout->addWidget(latLabel);
            latLayout->addWidget(latSpinBox);
            latLayout->addStretch();
            dialogLayout->addLayout(latLayout);
            QHBoxLayout *altLayout = new QHBoxLayout();
            QLabel *altLabel = new QLabel("Altitude:");
            altLabel->setFixedWidth(150);
            QDoubleSpinBox *altSpinBox = new QDoubleSpinBox();
            altSpinBox->setRange(-9999999.0, 9999999.0);
            altSpinBox->setDecimals(2);
            altSpinBox->setValue(0.0);
            altSpinBox->setSingleStep(1.0);
            altSpinBox->setFixedWidth(200);
            altLayout->addWidget(altLabel);
            altLayout->addWidget(altSpinBox);
            altLayout->addStretch();
            dialogLayout->addLayout(altLayout);
            QHBoxLayout *lonLayout = new QHBoxLayout();
            QLabel *lonLabel = new QLabel("Longitude:");
            lonLabel->setFixedWidth(150);
            QDoubleSpinBox *lonSpinBox = new QDoubleSpinBox();
            lonSpinBox->setRange(-9999999.0, 9999999.0);
            lonSpinBox->setDecimals(6);
            lonSpinBox->setValue(0.0);
            lonSpinBox->setSingleStep(0.000001);
            lonSpinBox->setFixedWidth(200);
            lonLayout->addWidget(lonLabel);
            lonLayout->addWidget(lonSpinBox);
            lonLayout->addStretch();
            dialogLayout->addLayout(lonLayout);
            dialogLayout->addSpacing(20);
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            QPushButton *saveButton = new QPushButton("OK", addDialog);
            saveButton->setStyleSheet(
                "QPushButton { "
                "    background-color: #e9ecef; color: black; border: 1px solid #ccc; "
                "    border-radius: 3px; font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #dde1e4; }"
                );

            QPushButton *cancelButton = new QPushButton("Cancel", addDialog);
            cancelButton->setStyleSheet(
                "QPushButton { "
                "    background-color: #e9ecef; color: black; border: 1px solid #ccc; "
                "    border-radius: 3px; font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #dde1e4; }"
                );

            buttonLayout->addStretch();
            buttonLayout->addWidget(saveButton);
            buttonLayout->addWidget(cancelButton);
            dialogLayout->addLayout(buttonLayout);
            connect(saveButton, &QPushButton::clicked, this, [=]() {
                double lat = latSpinBox->value();
                double alt = altSpinBox->value();
                double lon = lonSpinBox->value();
                QJsonObject newWaypoint;
                QJsonObject pos;
                pos["x"] = lat;
                pos["y"] = alt;
                pos["z"] = lon;
                newWaypoint["position"] = pos;
                QString displayText = QString("Lat: %1, Alt: %2, Lon: %3")
                                          .arg(formatNumberForUI(lat))
                                          .arg(formatNumberForUI(alt))
                                          .arg(formatNumberForUI(lon));

                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, newWaypoint.toVariantMap());
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setSizeHint(QSize(0, itemHeight));
                listWidget->addItem(item);
                emitArrayChanged();
                addDialog->accept();
            });
            connect(cancelButton, &QPushButton::clicked, addDialog, &QDialog::reject);
            addDialog->exec();
            addDialog->deleteLater();
        });
    }
    connect(listWidget, &QListWidget::itemChanged, this, [=](QListWidgetItem *item) {
        if (fullKey != "trajectories") return;
        QString newText = item->text();
        QRegExp regex("Lat:\\s*([\\d\\.\\-]+),\\s*Alt:\\s*([\\d\\.\\-]+),\\s*Lon:\\s*([\\d\\.\\-]+)");
        if (regex.indexIn(newText) != -1) {
            double lat = regex.cap(1).toDouble();
            double alt = regex.cap(2).toDouble();
            double lon = regex.cap(3).toDouble();
            QVariantMap itemData = item->data(Qt::UserRole).toMap();
            QVariantMap position = itemData["position"].toMap();
            position["x"] = lat;
            position["y"] = alt;
            position["z"] = lon;
            itemData["position"] = position;
            item->setData(Qt::UserRole, itemData);
            emitArrayChanged();
        }
    });
    connect(listWidget, &QListWidget::doubleClicked, this, [=](const QModelIndex &index) {
        QListWidgetItem *item = listWidget->item(index.row());
        if (!item) return;
        QVariantMap itemData = item->data(Qt::UserRole).toMap();
        if (fullKey == "trajectories") {
            QVariantMap position = itemData["position"].toMap();
            double currentLat = position["x"].toDouble();
            double currentAlt = position["y"].toDouble();
            double currentLon = position["z"].toDouble();
            QDialog *dialog = new QDialog(this);
            dialog->setWindowTitle("Edit Waypoint");
            dialog->setModal(true);
            dialog->setStyleSheet(
                "QDialog { background-color: #ffffff; } "
                "QLabel { color: #2c3e50; font-weight: bold; } "
                "QDoubleSpinBox { background-color: white; color: black; border: 1px solid #ccc; padding: 5px; } "
                "QPushButton { padding: 8px 15px; border-radius: 3px; font-weight: bold; }"
                );
            QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
            mainLayout->setSpacing(15);
            mainLayout->setContentsMargins(20, 20, 20, 20);
            QHBoxLayout *latLayout = new QHBoxLayout();
            QLabel *latLabel = new QLabel("Latitude:");
            latLabel->setFixedWidth(120);
            QDoubleSpinBox *latSpinBox = new QDoubleSpinBox();
            latSpinBox->setRange(-9999999.0, 9999999.0);
            latSpinBox->setDecimals(6);
            latSpinBox->setValue(currentLat);
            latSpinBox->setSingleStep(0.000001);
            latSpinBox->setFixedWidth(200);
            latLayout->addWidget(latLabel);
            latLayout->addWidget(latSpinBox);
            latLayout->addStretch();
            mainLayout->addLayout(latLayout);
            QHBoxLayout *altLayout = new QHBoxLayout();
            QLabel *altLabel = new QLabel("Altitude:");
            altLabel->setFixedWidth(120);
            QDoubleSpinBox *altSpinBox = new QDoubleSpinBox();
            altSpinBox->setRange(-9999999.0, 9999999.0);
            altSpinBox->setDecimals(2);
            altSpinBox->setValue(currentAlt);
            altSpinBox->setSingleStep(1.0);
            altSpinBox->setFixedWidth(200);
            altLayout->addWidget(altLabel);
            altLayout->addWidget(altSpinBox);
            altLayout->addStretch();
            mainLayout->addLayout(altLayout);
            QHBoxLayout *lonLayout = new QHBoxLayout();
            QLabel *lonLabel = new QLabel("Longitude:");
            lonLabel->setFixedWidth(120);
            QDoubleSpinBox *lonSpinBox = new QDoubleSpinBox();
            lonSpinBox->setRange(-9999999.0, 9999999.0);
            lonSpinBox->setDecimals(6);
            lonSpinBox->setValue(currentLon);
            lonSpinBox->setSingleStep(0.000001);
            lonSpinBox->setFixedWidth(200);
            lonLayout->addWidget(lonLabel);
            lonLayout->addWidget(lonSpinBox);
            lonLayout->addStretch();
            mainLayout->addLayout(lonLayout);
            mainLayout->addSpacing(20);
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            QPushButton *updateButton = new QPushButton("OK", dialog);
            updateButton->setStyleSheet(
                "QPushButton { "
                "    background-color: #e9ecef; color: black; border: 1px solid #ccc; "
                "    border-radius: 3px; font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #dde1e4; }"
                );
            QPushButton *deleteButton = new QPushButton("Delete", dialog);
            deleteButton->setStyleSheet(
                "QPushButton { "
                "    background-color: #e9ecef; color: black; border: 1px solid #ccc; "
                "    border-radius: 3px; font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #dde1e4; }"
                );
            QPushButton *cancelButton = new QPushButton("Cancel", dialog);
            cancelButton->setStyleSheet(
                "QPushButton { "
                "    background-color: #e9ecef; color: black; border: 1px solid #ccc; "
                "    border-radius: 3px; font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #dde1e4; }"
                );
            buttonLayout->addStretch();
            buttonLayout->addWidget(updateButton);
            buttonLayout->addWidget(deleteButton);
            buttonLayout->addWidget(cancelButton);
            mainLayout->addLayout(buttonLayout);
            connect(updateButton, &QPushButton::clicked, this, [=]() {
                double newLat = latSpinBox->value();
                double newAlt = altSpinBox->value();
                double newLon = lonSpinBox->value();
                QVariantMap newPosition = position;
                newPosition["x"] = newLat;
                newPosition["y"] = newAlt;
                newPosition["z"] = newLon;
                QVariantMap newItemData = itemData;
                newItemData["position"] = newPosition;
                item->setData(Qt::UserRole, newItemData);
                QString displayText = QString("Lat: %1, Alt: %2, Lon: %3")
                                          .arg(formatNumberForUI(newLat))
                                          .arg(formatNumberForUI(newAlt))
                                          .arg(formatNumberForUI(newLon));
                item->setText(displayText);
                emitArrayChanged();
                dialog->accept();
            });
            connect(deleteButton, &QPushButton::clicked, this, [=]() {
                int itemIndex = listWidget->row(item);
                delete listWidget->takeItem(itemIndex);

                emitArrayChanged();
                dialog->accept();
            });
            connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
            dialog->exec();
            dialog->deleteLater();
        }
        else if (itemData.contains("id") && !itemData["id"].toString().isEmpty()) {
            emit foucsEntity(itemData["id"].toString());
        }
    });
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, arrayWidget);
}
/* Setup string cell */
void Inspector::setupStringCell(int row, const QString &fullKey, const QString &value)
{
    QTableWidgetItem *keyItem = tableWidget->item(row, 0);
    if (!keyItem) {
        QString displayKey = capitalizeFirstLetter(fullKey.split(".").last());
        keyItem = new QTableWidgetItem(displayKey);
        if (keyItem) {
            keyItem->setBackground(QColor("#f8f9fa"));
            keyItem->setForeground(QColor(Qt::black));
            keyItem->setFlags(Qt::ItemIsEnabled);
            tableWidget->setItem(row, 0, keyItem);
        }
    }
    QLineEdit *lineEdit = new QLineEdit();
    if (!lineEdit) {
        return;
    }
    lineEdit->setText(value);
    lineEdit->setStyleSheet(
        "QLineEdit { color: black !important; background: white; border: 1px solid #ccc; padding: 2px; }"
        "QLineEdit:focus { border: 1px solid #007bff; background: white; }"
        );
    lineEdit->setFrame(true);
    lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (fullKey.contains("id") || fullKey.contains("name") || fullKey.contains("branch")) {
        lineEdit->setReadOnly(true);
    }
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, lineEdit);
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QString newValue = lineEdit->text();
        QJsonObject delta;
        if (fullKey.contains(".")) {
            QStringList parts = fullKey.split(".");
            delta[parts[0]] = QJsonObject{{parts[1], newValue}};
        } else {
            delta[fullKey] = newValue;
        }
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });
}

void Inspector::setupNumberCell(int row, const QString &fullKey, double value)
{
    WheelableLineEdit *lineEdit = new WheelableLineEdit();
    if (!lineEdit) {
        return;
    }
    lineEdit->setText(formatNumberForUI(value));
    lineEdit->setStyleSheet(
        "QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; }"
        );
    QDoubleValidator *validator = new QDoubleValidator(lineEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(4);
    lineEdit->setValidator(validator);
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, lineEdit);
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QJsonObject delta;
        delta[fullKey] = lineEdit->text().toDouble();
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });
}
void Inspector::handleRemoveParameter()
{
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
        QJsonObject delta;
        delta["_id"] = mainID;
        delta[selectedKey] = QJsonValue();
        emit parameterChanged(ConnectedID, Name, selectedKey, "", false);
        emit valueChanged(ConnectedID, Name, delta);
    }
}
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
                    QJsonObject delta;
                    delta["_id"] = mainID;
                    if (key == "entity") {
                        if (!listWidget) {
                            dropEvent->acceptProposedAction();
                            return true;
                        }
                        if (customData.contains("type") && customData["type"].toString() == "entity") {
                            QString actualID = "";
                            QString actualName = "";
                            if (customData.contains("ID") && customData["ID"].isValid()) {
                                actualID = customData["ID"].toString();
                            } else if (customData.contains("id") && customData["id"].isValid()) {
                                actualID = customData["id"].toString();
                            }
                            if (customData.contains("name") && customData["name"].isValid()) {
                                actualName = customData["name"].toString();
                            } else if (customData.contains("Name") && customData["Name"].isValid()) {
                                actualName = customData["Name"].toString();
                            }
                            listWidget->clear();
                            QJsonObject entityObj;
                            entityObj["type"] = "reference";
                            entityObj["name"] = actualName.isEmpty() ? "Entity" : actualName;
                            entityObj["id"] = actualID.isEmpty() ? "unknown_id" : actualID;

                            QString displayText = entityObj["name"].toString() +
                                                  (entityObj["id"].toString().isEmpty() ?
                                                       "" : " (ID: " + entityObj["id"].toString() + ")");

                            QListWidgetItem *newItem = new QListWidgetItem(displayText);
                            if (newItem) {
                                newItem->setData(Qt::UserRole, entityObj.toVariantMap());
                                listWidget->addItem(newItem);
                                QJsonObject delta;
                                delta["entity"] = entityObj;
                                if (!ConnectedID.isEmpty() && !Name.isEmpty()) {
                                    emit valueChanged(ConnectedID, Name, delta);
                                } else {
                                }
                            } else {
                            }
                            dropEvent->acceptProposedAction();
                            return true;
                        }
                    }
                    else if (key == "sensors" || key == "iffs" ||key == "radios") {
                        if (customData["type"].toString() != "entity" ||
                            !customData.contains("name") || customData["name"].toString().isEmpty() ||
                            !customData.contains("ID") || customData["ID"].toString().isEmpty()) {
                            continue;
                        }
                        listWidget->clear();
                        QJsonObject refObj;
                        refObj["type"] = "reference";
                        refObj["subtype"] = key;
                        refObj["name"] = customData["name"].toString();
                        refObj["id"] = customData["ID"].toString();
                        QString displayText = refObj["name"].toString() + " (ID: " + refObj["id"].toString() + ")";
                        QListWidgetItem *newItem = new QListWidgetItem(displayText);
                        newItem->setData(Qt::UserRole, refObj.toVariantMap());
                        listWidget->addItem(newItem);
                        delta["ref"] = refObj;
                        emit valueChanged(ConnectedID, Name, delta);
                    }
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

void Inspector::init(QString ID, QString name, QJsonObject object)
{
    // ✅ Block signals during initialization
    if (tableWidget) {
        tableWidget->blockSignals(true);
    }
    // ✅ Clear previous state FIRST
    rowToKeyPath.clear();
    customParameterKeys.clear();
    sectionInfo.clear();
    sectionRows.clear();
    // ✅ Reset currentlyExpandedButton
    currentlyExpandedButton = nullptr;
    ConnectedID = ID;
    // Normalize component name
    QString newName = name.toLower();
    if (name.compare("Trajectories", Qt::CaseInsensitive) == 0) {
        newName = QString("trajectory");
    } else if (name.compare("dynamicModel", Qt::CaseInsensitive) == 0) {
        newName = QString("dynamicModel");
    } else if (name.compare("meshRenderer2d", Qt::CaseInsensitive) == 0) {
        newName = QString("meshRenderer2d");
    } else if (name.compare("crossSection", Qt::CaseInsensitive) == 0) {
        newName = QString("crossSection");
    }
    // ✅ Set column resize mode based on component type
    if (tableWidget) {
        if (newName == "sensors" || newName == "iffs" || newName == "radios") {
            tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        } else {
            tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        }
    }
    // Determine component ID
    QString currentComponentId = "";
    if (object.contains("type") && object["type"].toString().toLower() == "subcomponent") {
        if (object.contains("id")) {
            currentComponentId = object["id"].toString();
        } else if (object.contains("ID")) {
            currentComponentId = object["ID"].toString();
        } else {
            currentComponentId = ID + "_sub_" + name;
        }
    } else {
        if (object.contains("id")) {
            currentComponentId = object["id"].toString();
        } else if (object.contains("ID")) {
            currentComponentId = object["ID"].toString();
        } else if (object.contains("Id")) {
            currentComponentId = object["Id"].toString();
        } else {
            for (const QString& key : object.keys()) {
                if (object[key].isObject()) {
                    QJsonObject subObj = object[key].toObject();
                    if (subObj.contains("id")) {
                        currentComponentId = subObj["id"].toString();
                        break;
                    }
                }
            }
            if (currentComponentId.isEmpty()) {
                currentComponentId = ID + "_" + name;
            }
        }
    }
    mainID = currentComponentId;
    Name = newName;
    // Handle multi-component containers (sensors, iffs, radios)
    if (Name == "sensors" || Name == "iffs" || Name == "radios") {
        handleMultiComponentContainer(ID, name, object);
        return;
    }
    // ✅ Update title
    if (titleLabel) {
        titleLabel->setText(name);
    }
    // ✅ Clear and reset table
    if (tableWidget) {
        tableWidget->clearContents();
        tableWidget->setRowCount(0);
    }
    // Load component data if empty
    if ((Name == QString("trajectory") || Name == QString("dynamicModel") ||
         Name == QString("meshRenderer2d") || Name == QString("collider")) &&
        object.isEmpty() && hierarchy) {
        QString dataType = Name;
        object = hierarchy->getComponentData(ID, dataType);
    }
    // Calculate row count
    int rowCount = 0;
    for (const QString &key : object.keys()) {
        QJsonValue value = object[key];
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString type = obj["type"].toString().toLower();
            if (type == "section") {
                rowCount += 1 + (obj.keys().size() - 1);
            }
            else if (key == "modeConfiguration") {
                QJsonObject subObj = object[key].toObject();
                rowCount += subObj.size();
            }
            else if (key == "entity" || obj["type"].toString() == "reference") {
                rowCount += 1;
            }
            else {
                rowCount += 1;
            }
        }
        else if (value.isArray()) {
            rowCount += 1;
        }
        else {
            rowCount += 1;
        }
    }
    if (tableWidget) {
        tableWidget->setRowCount(rowCount);
    }
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
    if (tableWidget) {
        tableWidget->blockSignals(false);
        tableWidget->resizeRowsToContents();
        tableWidget->resizeColumnsToContents();
        if (tableWidget->rowCount() > 0) {
            tableWidget->scrollToTop();
        }
        tableWidget->viewport()->update();
    }
}
int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
    bool developerMode = ApplicationDialog::getGlobalDeveloperMode();
    if (!developerMode && (key.toLower() == "id" || key.toLower() == "type")) {
        QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(key));
        if (keyItem) {
            keyItem->setFlags(Qt::ItemIsEnabled);
            keyItem->setBackground(QColor("#f8f9fa"));
            keyItem->setForeground(Qt::black);
            tableWidget->setItem(row, 0, keyItem);
        }
        setupValueCell(row, key, value);
        tableWidget->setRowHidden(row, true);
        return row + 1;
    }
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        if (type == "section") {
            QString sectionKey = key;
            int headerRow = row;
            QWidget *headerWidget = createSectionHeader(sectionKey, headerRow, obj);

            if (!headerWidget) {
                return row + 1;
            }
            tableWidget->setCellWidget(row, 0, headerWidget);
            tableWidget->setRowHeight(row, 35);
            QTableWidgetItem *emptyItem = new QTableWidgetItem("");
            emptyItem->setFlags(Qt::ItemIsEnabled);
            emptyItem->setBackground(QColor("#2c3e50"));
            tableWidget->setItem(row, 1, emptyItem);
            sectionInfo[sectionKey] = SectionInfo(
                headerRow,
                obj.keys().size() - 1,
                true
                );
            int currentRow = row + 1;
            for (const QString &paramKey : obj.keys()) {
                if (paramKey == "type") continue;
                tableWidget->insertRow(currentRow);
                QString fullKey = QString("%1.%2").arg(sectionKey).arg(paramKey);
                rowToKeyPath[currentRow] = fullKey;
                QString displayKey = paramKey;

                for (int i = 1; i < displayKey.length(); i++) {
                    if (displayKey[i].isUpper() && displayKey[i-1].isLower()) {
                        displayKey.insert(i, " ");
                        i++;
                    }
                }
                QTableWidgetItem *paramKeyItem = new QTableWidgetItem("    " + capitalizeFirstLetter(displayKey));
                if (paramKeyItem) {
                    paramKeyItem->setFlags(Qt::ItemIsEnabled);
                    paramKeyItem->setBackground(QColor("#ecf0f1"));
                    paramKeyItem->setForeground(Qt::black);
                    tableWidget->setItem(currentRow, 0, paramKeyItem);
                }
                QJsonValue paramValue = obj[paramKey];
                if (paramValue.isObject()) {
                    QJsonObject paramObj = paramValue.toObject();

                    if (paramObj.contains("type") && paramObj["type"].toString() == "unitParam") {

                        setupUnitParameterCell(currentRow, fullKey, paramObj);
                    } else {
                        setupValueCell(currentRow, fullKey, paramValue);
                    }
                } else {
                    setupValueCell(currentRow, fullKey, paramValue);
                }
                sectionRows[currentRow] = sectionKey;
                currentRow++;
            }
            return currentRow;
        }
        else if (type == "unitparam") {
            rowToKeyPath[row] = key;
            QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(key));
            if (keyItem) {
                keyItem->setFlags(Qt::ItemIsEnabled);
                keyItem->setBackground(QColor("#f8f9fa"));
                keyItem->setForeground(Qt::black);
                tableWidget->setItem(row, 0, keyItem);
            }
            setupUnitParameterCell(row, key, obj);
            return row + 1;
        }
    }
    rowToKeyPath[row] = key;
    QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(key));
    if (keyItem) {
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#f8f9fa"));
        keyItem->setForeground(Qt::black);
        tableWidget->setItem(row, 0, keyItem);
    }
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        if (type == "component" || type == "parameter") {
            return row;
        }
        if (key == "modeConfiguration") {
            QStringList subKeys = {"mode1", "mode2", "mode3A", "mode4", "modeC"};
            int currentRow = row;
            for (const QString &subKey : subKeys) {
                QString fullKey = QString("modeConfiguration.%1").arg(subKey);
                rowToKeyPath[currentRow] = fullKey;
                QTableWidgetItem *subKeyItem = new QTableWidgetItem(capitalizeFirstLetter(fullKey.split(".").last()));
                if (subKeyItem) {
                    subKeyItem->setFlags(Qt::ItemIsEnabled);
                    subKeyItem->setBackground(QColor("#f8f9fa"));
                    subKeyItem->setForeground(Qt::black);
                    tableWidget->setItem(currentRow, 0, subKeyItem);
                }
                setupStringCell(currentRow, fullKey, obj.value(subKey).toString());
                currentRow++;
            }
            return currentRow;
        }
    }
    if (key == "entity" && (value.isObject() || value.isNull())) {
        QJsonArray singleItemArray;
        if (value.isObject()) singleItemArray.append(value.toObject());
        setupArrayCell(row, key, singleItemArray);
    }
    else if (value.isArray()) {
        setupArrayCell(row, key, value.toArray());
    }
    else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        if (type == "vector") {
            VectorTemplate *vector = new VectorTemplate(this);
            vector->setConnectedID(ConnectedID);
            vector->setName(Name);
            vector->setMainID(mainID);
            vector->setupVectorCell(row, key, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "geocord" || type == "geooffset") {
            GeocordsTemplate *geocords = new GeocordsTemplate(this, this);
            geocords->setConnectedID(ConnectedID);
            geocords->setName(Name);
            geocords->setupGeocordsCell(row, key, obj, tableWidget);
            connect(geocords, &GeocordsTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "option") {
            OptionTemplate *option = new OptionTemplate(this);
            option->setConnectedID(ConnectedID);
            option->setName(Name);
            option->setMainID(mainID);
            option->setupOptionCell(row, key, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setMainID(mainID);
            color->setupColorCell(row, key, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setMainID(mainID);
            image->setupImageCell(row, key, obj, tableWidget);
            connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else {
            setupGenericObjectCell(row, key, obj);
        }
    }
    else {
        setupValueCell(row, key, value);
    }
    return row + 1;
}
void Inspector::setupValueCell(int row, const QString &fullKey, const QJsonValue &value)
{
    if (value.isBool()) {
        setupBooleanCell(row, fullKey, value.toBool());
    }
    else if (value.isArray()) {
        setupArrayCell(row, fullKey, value.toArray());
    }
    else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();
        if (type == "unitparam") {
            setupUnitParameterCell(row, fullKey, obj);
        }
        else if (type == "vector") {
            VectorTemplate *vector = new VectorTemplate(this);
            vector->setConnectedID(ConnectedID);
            vector->setName(Name);
            vector->setMainID(mainID);
            vector->setupVectorCell(row, fullKey, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "geocord" || type == "geooffset") {
            GeocordsTemplate *geocords = new GeocordsTemplate(this, this);
            geocords->setConnectedID(ConnectedID);
            geocords->setName(Name);
            geocords->setupGeocordsCell(row, fullKey, obj, tableWidget);
            connect(geocords, &GeocordsTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "option") {
            OptionTemplate *option = new OptionTemplate(this);
            option->setConnectedID(ConnectedID);
            option->setName(Name);
            option->setMainID(mainID);
            option->setupOptionCell(row, fullKey, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setMainID(mainID);
            color->setupColorCell(row, fullKey, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setMainID(mainID);
            image->setupImageCell(row, fullKey, obj, tableWidget);
            connect(image, &ImageTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else {
            setupGenericObjectCell(row, fullKey, obj);
        }
    }
    else if (value.isString()) {
        setupStringCell(row, fullKey, value.toString());
    }
    else {
        setupNumberCell(row, fullKey, value.toDouble());
    }
}

void Inspector::setupGenericObjectCell(int row, const QString &fullKey, const QJsonObject &obj)
{

    QWidget *valueWidget = new QWidget();
    if (!valueWidget) {
        return;
    }
    QVBoxLayout *layout = new QVBoxLayout(valueWidget);
    if (!layout) {

        delete valueWidget;
        return;
    }
    layout->setContentsMargins(0, 0, 0, 0);
    for (const QString &subKey : obj.keys()) {
        if (subKey == "type") continue;
        QHBoxLayout *subLayout = new QHBoxLayout();
        if (!subLayout) {
            continue;
        }
        QLabel *label = new QLabel(capitalizeFirstLetter(subKey));
        if (!label) {
            delete subLayout;
            continue;
        }
        label->setStyleSheet("color: black; min-width: 20px;");
        safeAddWidget(subLayout, label, "setupGenericObjectCell - label to subLayout");
        QJsonValue subValue = obj[subKey];
        bool isModulationField = fullKey.contains("modulation", Qt::CaseInsensitive);
        if (isModulationField && subValue.isString()) {
            QLineEdit *edit = new QLineEdit();
            if (!edit) {
                delete subLayout;
                delete label;
                continue;
            }
            edit->setText(subValue.toString());
            edit->setStyleSheet("QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; }");
            edit->setObjectName(subKey);
            if (!safeAddWidget(subLayout, edit, "setupGenericObjectCell - edit to subLayout")) {
                delete edit;
                delete subLayout;
                delete label;
                continue;
            }
            layout->addLayout(subLayout);
            connect(edit, &QLineEdit::editingFinished, this, [=]() {
                QJsonObject delta;
                delta["_id"] = mainID;
                delta[fullKey] = QJsonObject{{subKey, edit->text()}};
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
        else if (subValue.isDouble()) {
            WheelableLineEdit *edit = new WheelableLineEdit();
            if (!edit) {
                delete subLayout;
                delete label;
                continue;
            }
            edit->setText(formatNumberForUI(subValue.toDouble()));
            edit->setStyleSheet("QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; }");
            QDoubleValidator *validator = new QDoubleValidator(edit);
            validator->setNotation(QDoubleValidator::StandardNotation);
            validator->setDecimals(4);
            edit->setValidator(validator);
            edit->setObjectName(subKey);
            if (!safeAddWidget(subLayout, edit, "setupGenericObjectCell - edit to subLayout")) {
                delete edit;
                delete subLayout;
                delete label;
                continue;
            }
            layout->addLayout(subLayout);
            connect(edit, &QLineEdit::editingFinished, this, [=]() {
                QJsonObject delta;
                delta["_id"] = mainID;

                delta[fullKey] = QJsonObject{{subKey, edit->text().toDouble()}};
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
        else if (subValue.isBool()) {
            QCheckBox *checkBox = new QCheckBox();
            if (!checkBox) {
                delete subLayout;
                delete label;
                continue;
            }
            checkBox->setChecked(subValue.toBool());
            checkBox->setStyleSheet(
                "QCheckBox { color: black; border: none; }"
                "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #666; background-color: white; }"
                "QCheckBox::indicator:checked { image: url(:/icons/images/check-box.png); background-color: #007bff; }"
                "QCheckBox::indicator:unchecked { image: none; background-color: white; }"
                );
            QHBoxLayout *checkboxLayout = new QHBoxLayout();
            checkboxLayout->addWidget(checkBox);
            checkboxLayout->addStretch();
            if (!safeAddWidget(subLayout, checkBox, "setupGenericObjectCell - checkBox to subLayout")) {
                delete checkBox;
                delete subLayout;
                delete label;
                continue;
            }
            layout->addLayout(subLayout);
            connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta;
                delta["_id"] = mainID;
                delta[fullKey] = QJsonObject{{subKey, checked}};
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
        else {
            QLineEdit *edit = new QLineEdit();
            if (!edit) {
                delete subLayout;
                delete label;
                continue;
            }
            edit->setText(subValue.toString());
            edit->setStyleSheet("QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; }");
            edit->setObjectName(subKey);
            if (!safeAddWidget(subLayout, edit, "setupGenericObjectCell - edit to subLayout")) {
                delete edit;
                delete subLayout;
                delete label;
                continue;
            }
            layout->addLayout(subLayout);
            connect(edit, &QLineEdit::editingFinished, this, [=]() {
                QJsonObject delta;
                delta["_id"] = mainID;
                delta[fullKey] = QJsonObject{{subKey, edit->text()}};
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
    }
    tableWidget->setRowHeight(row, 30 * obj.size());
    tableWidget->setCellWidget(row, 1, valueWidget);
}

void Inspector::updateTrajectory(QString entityId, QJsonArray waypoints)
{
    int trajRow = -1;
    for (int r = 0; r < tableWidget->rowCount(); ++r) {
        if (rowToKeyPath[r] == "trajectories") {
            trajRow = r;
            break;
        }
    }
    if (trajRow == -1) {
        tableWidget->setRowCount(tableWidget->rowCount() + 1);
        trajRow = tableWidget->rowCount() - 1;
        rowToKeyPath[trajRow] = "trajectories";
        QTableWidgetItem *keyItem = new QTableWidgetItem("trajectories");
        if (keyItem) {
            keyItem->setFlags(Qt::ItemIsEnabled);
            keyItem->setBackground(QColor("#f8f9fa"));
            keyItem->setForeground(Qt::black);
            tableWidget->setItem(trajRow, 0, keyItem);
        }
        setupArrayCell(trajRow, "trajectories", QJsonArray());
    }
    QWidget *arrayWidget = tableWidget->cellWidget(trajRow, 1);
    if (!arrayWidget) {
        setupArrayCell(trajRow, "trajectories", QJsonArray());
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
    }
    if (!arrayWidget) return;
    QListWidget *listWidget = arrayWidget->findChild<QListWidget*>();
    if (!listWidget) return;
    if (ConnectedID != entityId || Name != "trajectory") {
        ConnectedID = entityId;
        Name = "trajectory";
        if (titleLabel) {
            titleLabel->setText("Trajectories");
        }
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
    listWidget->clear();
    for (const QJsonValue &val : waypoints) {
        QJsonObject obj = val.toObject();
        if (!obj.contains("position")) continue;
        QJsonObject pos = obj["position"].toObject();
        QString displayText = QString("Lat: %1, Alt: %2, Lon: %3")
                                  .arg(formatNumberForUI(pos["x"].toDouble()))   // Latitude
                                  .arg(formatNumberForUI(pos["y"].toDouble()))   // Altitude
                                  .arg(formatNumberForUI(pos["z"].toDouble()));  // Longitude

        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        listWidget->addItem(item);
    }
    if (waypoints.isEmpty() && hierarchy) {
        QJsonObject trajData = hierarchy->getComponentData(ConnectedID, "trajectory");
        if (!trajData.isEmpty() && trajData.contains("trajectories")) {
            QJsonArray array = trajData["trajectories"].toArray();
            for (const QJsonValue &val : array) {
                QJsonObject obj = val.toObject();
                if (!obj.contains("position")) continue;
                QJsonObject pos = obj["position"].toObject();
                QString displayText = QString("Lat: %1, Alt: %2, Lon: %3")
                                          .arg(formatNumberForUI(pos["x"].toDouble()))   // Latitude
                                          .arg(formatNumberForUI(pos["y"].toDouble()))   // Altitude
                                          .arg(formatNumberForUI(pos["z"].toDouble()));  // Longitude

                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, obj.toVariantMap());
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                listWidget->addItem(item);
            }
        }
    }
    tableWidget->viewport()->update();
}
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}
void Inspector::setupUnitParameterCell(int row, const QString &fullKey, const QJsonObject &paramObj)
{
    QWidget *unitWidget = new QWidget();
    if (!unitWidget) return;
    QHBoxLayout *layout = new QHBoxLayout(unitWidget);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(8);
    WheelableLineEdit *valueEdit = new WheelableLineEdit();
    valueEdit->setText(formatNumberForUI(paramObj["value"].toDouble()));
    valueEdit->setStyleSheet(
        "QLineEdit {"
        "    background: white;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    color: black;"
        "    min-width: 80px;"
        "}"
        );
    QDoubleValidator *validator = new QDoubleValidator(valueEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(4);
    valueEdit->setValidator(validator);
    QString unit = paramObj["unit"].toString();
    QLabel *unitLabel = new QLabel(unit);
    unitLabel->setStyleSheet(
        "QLabel {"
        "    color: #7f8c8d;"
        "    font-style: italic;"
        "    min-width: 50px;"
        "}"
        );
    layout->addWidget(valueEdit);
    layout->addWidget(unitLabel);
    layout->addStretch();
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, unitWidget);
    connect(valueEdit, &QLineEdit::editingFinished, this, [=]() {
        QJsonObject delta;
        QJsonObject updatedParam = paramObj;
        updatedParam["value"] = valueEdit->text().toDouble();
        if (fullKey.contains(".")) {
            QStringList parts = fullKey.split(".");
            delta[parts[0]] = QJsonObject{{parts[1], updatedParam}};
        } else {
            delta[fullKey] = updatedParam;
        }
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });
}
QWidget* Inspector::createSectionHeader(const QString &sectionKey, int headerRow, const QJsonObject &sectionObj)
{
    QWidget *headerWidget = new QWidget();
    if (!headerWidget) return nullptr;
    QHBoxLayout *layout = new QHBoxLayout(headerWidget);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(10);
    QPushButton *dropdownButton = new QPushButton("▼", this);
    dropdownButton->setFixedSize(20, 20);
    dropdownButton->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    color: white;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(255, 255, 255, 0.1);"
        "    border-radius: 3px;"
        "}"
        );
    dropdownButton->setProperty("sectionKey", sectionKey);
    dropdownButton->setProperty("headerRow", headerRow);
    QLabel *sectionLabel = new QLabel(capitalizeFirstLetter(sectionKey));
    sectionLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "}"
        );
    layout->addWidget(dropdownButton);
    layout->addWidget(sectionLabel);
    layout->addStretch();
    headerWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #2c3e50;"
        "    border-radius: 4px;"
        "}"
        );
    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        toggleSectionExpansion(sectionKey, headerRow, dropdownButton);
    });
    return headerWidget;
}
void Inspector::toggleSectionExpansion(const QString &sectionKey, int headerRow, QPushButton *dropdownButton)
{
    if (!sectionInfo.contains(sectionKey)) {
        return;
    }
    SectionInfo &info = sectionInfo[sectionKey];
    bool isExpanded = info.isExpanded;
    info.isExpanded = !isExpanded;
    if (info.isExpanded) {
        dropdownButton->setText("▼");
    } else {
        dropdownButton->setText("▶");
    }
    int paramCount = info.parameterCount;
    for (int i = 1; i <= paramCount; i++) {
        int paramRow = headerRow + i;
        if (paramRow < tableWidget->rowCount()) {
            tableWidget->setRowHidden(paramRow, !info.isExpanded);
        }
    }
    tableWidget->viewport()->update();
}

void Inspector::addSectionToLayout(const QString &sectionName, const QJsonObject &sectionObj, QVBoxLayout *parentLayout, const QString &subComponentId)
{
    QWidget *sectionWidget = new QWidget();
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionWidget);
    sectionLayout->setContentsMargins(10, 5, 0, 5);
    sectionLayout->setSpacing(5);
    QLabel *sectionLabel = new QLabel(sectionName + ":");
    sectionLabel->setStyleSheet("color: #2c3e50; font-weight: bold;");
    sectionLayout->addWidget(sectionLabel);
    for (const QString &key : sectionObj.keys()) {
        if (key == "type") continue;
        QWidget *propWidget = new QWidget();
        QHBoxLayout *propLayout = new QHBoxLayout(propWidget);
        propLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *propLabel = new QLabel(capitalizeFirstLetter(key) + ":");
        propLabel->setStyleSheet("color: black; min-width: 120px;");
        QJsonValue propValue = sectionObj[key];
        QWidget *valueWidget = createValueWidgetForJson(key, propValue, subComponentId, sectionName.toLower());
        propLayout->addWidget(propLabel);
        if (valueWidget) {
            propLayout->addWidget(valueWidget);
        }
        propLayout->addStretch();
        sectionLayout->addWidget(propWidget);
    }
    parentLayout->addWidget(sectionWidget);
}
QWidget* Inspector::createValueWidgetForJson(const QString &key, const QJsonValue &value, const QString &subComponentId, const QString &parentKey)
{
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    if (value.isBool()) {
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setChecked(value.toBool());
        checkBox->setStyleSheet(
            "QCheckBox { color: black; }"
            "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #666; background-color: white; }"
            "QCheckBox::indicator:checked { image: url(:/icons/images/check-box.png); background-color: #007bff; }"
            );
        layout->addWidget(checkBox);
        connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
            QJsonObject delta;
            delta["_id"] = mainID;
            QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;
            delta[subComponentId] = QJsonObject{{fullKey, checked}};
            emit valueChanged(ConnectedID, Name, delta);
        });
    }
    else if (value.isDouble()) {
        WheelableLineEdit *edit = new WheelableLineEdit();
        edit->setText(formatNumberForUI(value.toDouble()));
        edit->setStyleSheet(
            "QLineEdit {"
            "    background: white;"
            "    border: 1px solid #ccc;"
            "    border-radius: 3px;"
            "    padding: 2px 5px;"
            "    color: black;"
            "}"
            );
        edit->setFixedWidth(120);
        layout->addWidget(edit);
        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            QJsonObject delta;
            delta["_id"] = mainID;
            QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;
            delta[subComponentId] = QJsonObject{{fullKey, edit->text().toDouble()}};
            emit valueChanged(ConnectedID, Name, delta);
        });
    }
    else if (value.isString()) {
        QLineEdit *edit = new QLineEdit(value.toString());
        edit->setStyleSheet(
            "QLineEdit {"
            "    background: white;"
            "    border: 1px solid #ccc;"
            "    border-radius: 3px;"
            "    padding: 2px 5px;"
            "    color: black;"
            "}"
            );
        edit->setFixedWidth(120);
        layout->addWidget(edit);
        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            QJsonObject delta;
            delta["_id"] = mainID;
            QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;
            delta[subComponentId] = QJsonObject{{fullKey, edit->text()}};
            emit valueChanged(ConnectedID, Name, delta);
        });
    }
    else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        if (obj["type"].toString() == "unitParam") {
            WheelableLineEdit *valueEdit = new WheelableLineEdit();
            valueEdit->setText(formatNumberForUI(obj["value"].toDouble()));
            valueEdit->setStyleSheet(
                "QLineEdit {"
                "    background: white;"
                "    border: 1px solid #ccc;"
                "    border-radius: 3px;"
                "    padding: 2px 5px;"
                "    color: black;"
                "}"
                );
            valueEdit->setFixedWidth(80);
            QLabel *unitLabel = new QLabel(obj["unit"].toString());
            unitLabel->setStyleSheet("color: #7f8c8d; font-size: 11px; margin-left: 5px;");
            layout->addWidget(valueEdit);
            layout->addWidget(unitLabel);
            connect(valueEdit, &QLineEdit::editingFinished, this, [=]() {
                QJsonObject delta;
                delta["_id"] = mainID;
                QJsonObject updatedParam = obj;
                updatedParam["value"] = valueEdit->text().toDouble();
                QString fullKey = parentKey.isEmpty() ? key : parentKey + "." + key;
                delta[subComponentId] = QJsonObject{{fullKey, updatedParam}};
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
    }
    layout->addStretch();
    return widget;
}
QWidget* Inspector::createSubcomponentWidget(const QString &subKey, const QJsonObject &subObj, const QString &parentComponentName)
{
    QWidget *subWidget = new QWidget();
    subWidget->setProperty("subComponentId", subObj["id"].toString());
    QVBoxLayout *subLayout = new QVBoxLayout(subWidget);
    subLayout->setContentsMargins(0, 0, 0, 0);
    subLayout->setSpacing(0);
    QString subName = subObj["name"].toString();
    QPushButton *subDropdownButton = new QPushButton("▼ " + subName);
    subDropdownButton->setStyleSheet(
        "QPushButton {"
        "    text-align: left;"
        "    padding: 5px 10px 5px 20px;"
        "    background-color: #f8f9fa;"
        "    border: 1px solid #ddd;"
        "    border-radius: 3px;"
        "    color: #2c3e50;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e9ecef;"
        "}"
        );
    subDropdownButton->setFixedHeight(30);
    subDropdownButton->setProperty("isExpanded", false);
    // Container for subcomponent properties (initially hidden)
    QWidget *propertiesWidget = new QWidget();
    QVBoxLayout *propertiesLayout = new QVBoxLayout(propertiesWidget);
    propertiesLayout->setContentsMargins(30, 5, 5, 5);
    propertiesLayout->setSpacing(5);
    propertiesWidget->setVisible(false);
    subLayout->addWidget(subDropdownButton);
    subLayout->addWidget(propertiesWidget);
    for (const QString &propKey : subObj.keys()) {
        if (propKey == "default" || propKey == "parameters" || propKey == "name" || propKey == "id") {
            continue;
        }
        QWidget *propWidget = new QWidget();
        QHBoxLayout *propLayout = new QHBoxLayout(propWidget);
        propLayout->setContentsMargins(0, 0, 0, 0);

        QString displayKey = propKey;
        for (int i = 1; i < displayKey.length(); i++) {
            if (displayKey[i].isUpper() && displayKey[i-1].isLower()) {
                displayKey.insert(i, " ");
                i++;
            }
        }
        QLabel *propLabel = new QLabel(capitalizeFirstLetter(displayKey) + ":");
        propLabel->setStyleSheet("color: black; min-width: 100px;");
        QJsonValue propValue = subObj[propKey];
        QWidget *valueWidget = createValueWidgetForJson(propKey, propValue, subObj["id"].toString(), "");
        propLayout->addWidget(propLabel);
        if (valueWidget) {
            propLayout->addWidget(valueWidget);
        }
        propLayout->addStretch();
        propertiesLayout->addWidget(propWidget);
    }
    if (subObj.contains("default")) {
        QJsonObject defaultObj = subObj["default"].toObject();
        addSectionToLayout("Default", defaultObj, propertiesLayout, subObj["id"].toString());
    }
    if (subObj.contains("parameters")) {
        QJsonObject paramsObj = subObj["parameters"].toObject();
        addSectionToLayout("Parameters", paramsObj, propertiesLayout, subObj["id"].toString());
    }
    connect(subDropdownButton, &QPushButton::clicked, this, [=]() {
        if (currentlyExpandedButton && currentlyExpandedButton != subDropdownButton) {
            currentlyExpandedButton->setProperty("isExpanded", false);
            currentlyExpandedButton->setText("▼ " + currentlyExpandedButton->text().mid(2));
            QWidget *parentWidget = currentlyExpandedButton->parentWidget();
            if (parentWidget) {
                QList<QWidget*> children = parentWidget->findChildren<QWidget*>();
                for (QWidget *child : children) {
                    if (child != currentlyExpandedButton && child->objectName().isEmpty()) {
                        child->setVisible(false);
                        break;
                    }
                }
            }
        }
        bool isVisible = !propertiesWidget->isVisible();
        propertiesWidget->setVisible(isVisible);
        if (isVisible) {
            subDropdownButton->setText("▲ " + subName);
            currentlyExpandedButton = subDropdownButton;
        } else {
            subDropdownButton->setText("▼ " + subName);
            currentlyExpandedButton = nullptr;
        }
        subDropdownButton->setProperty("isExpanded", isVisible);
    });
    return subWidget;
}
int Inspector::addMultiComponentContainerRow(int row, const QString &key, const QJsonObject &containerObj, const QString &componentName)
{
    rowToKeyPath[row] = key;
    QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(key));
    if (keyItem) {
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#f8f9fa"));
        keyItem->setForeground(Qt::black);
        tableWidget->setItem(row, 0, keyItem);
    }
    QWidget *containerWidget = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(5);
    for (const QString &subKey : containerObj.keys()) {
        QJsonObject subObj = containerObj[subKey].toObject();
        QWidget *subWidget = createSubcomponentWidget(subKey, subObj, componentName);
        if (subWidget) {
            containerLayout->addWidget(subWidget);
        }
    }
    tableWidget->setCellWidget(row, 1, containerWidget);
    int subCount = containerObj.size();
    tableWidget->setRowHeight(row, subCount * 40);
    return row + 1;
}


void Inspector::handleMultiComponentContainer(QString ID, QString name, QJsonObject object)
{
    ConnectedID = ID;
    Name = name.toLower();
    mainID = object.contains("id") ? object["id"].toString() : ID + "_" + name;
    if (titleLabel) {
        titleLabel->setText(name);
    }
    tableWidget->clearContents();
    tableWidget->blockSignals(true);
    rowToKeyPath.clear();
    sectionInfo.clear();
    sectionRows.clear();
    QJsonObject containerObj;
    QString containerKey = name.toLower();
    if (object.contains(containerKey)) {
        containerObj = object[containerKey].toObject();
    }
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    bool developerMode = ApplicationDialog::getGlobalDeveloperMode();
    int row = 0;
    tableWidget->setRowCount(1);
    if (object.contains("active")) {
        rowToKeyPath[row] = "active";
        QTableWidgetItem *activeKeyItem = new QTableWidgetItem("Active");
        if (activeKeyItem) {
            activeKeyItem->setFlags(Qt::ItemIsEnabled);
            activeKeyItem->setBackground(QColor("#f8f9fa"));
            activeKeyItem->setForeground(Qt::black);
            tableWidget->setItem(row, 0, activeKeyItem);
        }

        setupBooleanCell(row, "active", object["active"].toBool());
        row++;
    }
    tableWidget->setRowCount(row + 1);
    QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(containerKey));
    if (keyItem) {
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#f8f9fa"));
        keyItem->setForeground(Qt::black);
        tableWidget->setItem(row, 0, keyItem);
    }
    rowToKeyPath[row] = containerKey;
    QWidget *containerWidget = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(5);
    int subCount = 0;
    for (const QString &subKey : containerObj.keys()) {
        subCount++;
    }
    QPushButton *dropdownButton = new QPushButton("▲ " + QString::number(subCount) + " " +
                                                  capitalizeFirstLetter(containerKey));
    dropdownButton->setStyleSheet(
        "QPushButton {"
        "    text-align: left;"
        "    padding: 5px 10px;"
        "    background-color: #e9ecef;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    color: #2c3e50;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #dde1e4;"
        "}"
        );
    dropdownButton->setFixedHeight(30);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVisible(true);
    QWidget *subcomponentsWidget = new QWidget();
    QVBoxLayout *subcomponentsLayout = new QVBoxLayout(subcomponentsWidget);
    subcomponentsLayout->setContentsMargins(10, 10, 10, 10);
    subcomponentsLayout->setSpacing(15);
    int totalSubcomponentsHeight = 0;
    subCount = 0;
    for (const QString &subKey : containerObj.keys()) {
        QJsonObject subObj = containerObj[subKey].toObject();
        QString subComponentId = subObj.contains("id") ? subObj["id"].toString() : subKey;

        QString subName = subObj.contains("name") ? subObj["name"].toString() :
                              (subObj.contains("Name") ? subObj["Name"].toString() :
                                   capitalizeFirstLetter(subKey));

        QGroupBox *subGroupBox = new QGroupBox(subName);
        subGroupBox->setProperty("subcomponentId", subComponentId);
        subGroupBox->setStyleSheet(
            "QGroupBox {"
            "    background-color: #f8f9fa;"
            "    border: 2px solid #dee2e6;"
            "    border-radius: 8px;"
            "    margin-top: 10px;"
            "    padding-top: 15px;"
            "    font-weight: bold;"
            "}"
            "QGroupBox::title {"
            "    subcontrol-origin: margin;"
            "    left: 15px;"
            "    padding: 0 10px 0 10px;"
            "    color: #495057;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "}"
            );

        QVBoxLayout *subLayout = new QVBoxLayout(subGroupBox);
        subLayout->setContentsMargins(15, 20, 15, 15);
        subLayout->setSpacing(10);
        int subcomponentItemCount = 0;
        if (subObj.contains("id") && developerMode) {
            subcomponentItemCount++;
            QWidget *idWidget = new QWidget();
            QHBoxLayout *idLayout = new QHBoxLayout(idWidget);
            idLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *idLabel = new QLabel("ID:");
            idLabel->setStyleSheet("color: #6c757d; min-width: 100px; font-weight: normal;");
            idLabel->setFixedWidth(100);
            QLineEdit *idEdit = new QLineEdit(subObj["id"].toString());
            idEdit->setStyleSheet(
                "QLineEdit {"
                "    background: #e9ecef;"
                "    border: 1px solid #ced4da;"
                "    border-radius: 3px;"
                "    padding: 3px 8px;"
                "    color: #495057;"
                "}"
                );
            idEdit->setReadOnly(true);
            idEdit->setFixedWidth(200);
            idLayout->addWidget(idLabel);
            idLayout->addWidget(idEdit);
            idLayout->addStretch();
            subLayout->addWidget(idWidget);
        }

        if (subObj.contains("active")) {
            subcomponentItemCount++;
            QWidget *activeWidget = new QWidget();
            QHBoxLayout *activeLayout = new QHBoxLayout(activeWidget);
            activeLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *activeLabel = new QLabel("Active:");
            activeLabel->setStyleSheet("color: #495057; min-width: 100px; font-weight: normal;");
            activeLabel->setFixedWidth(100);
            QCheckBox *activeCheckBox = new QCheckBox();
            activeCheckBox->setChecked(subObj["active"].toBool());
            activeCheckBox->setStyleSheet(
                "QCheckBox { color: #212529; }"
                "QCheckBox::indicator { width: 18px; height: 18px; }"
                );
            connect(activeCheckBox, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta;
                delta["_id"] = mainID;
                QJsonObject updatedSubObj = subObj;
                updatedSubObj["active"] = checked;
                delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                emit valueChanged(ConnectedID, Name, delta);
            });
            activeLayout->addWidget(activeLabel);
            activeLayout->addWidget(activeCheckBox);
            activeLayout->addStretch();
            subLayout->addWidget(activeWidget);
        }
        if (subObj.contains("SensorType")) {
            subcomponentItemCount++;
            QWidget *typeWidget = new QWidget();
            QHBoxLayout *typeLayout = new QHBoxLayout(typeWidget);
            typeLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *typeLabel = new QLabel("Sensor Type:");
            typeLabel->setStyleSheet("color: #495057; min-width: 100px; font-weight: normal;");
            typeLabel->setFixedWidth(100);
            QLineEdit *typeEdit = new QLineEdit(subObj["SensorType"].toString());
            typeEdit->setStyleSheet(
                "QLineEdit {"
                "    background: #e9ecef;"
                "    border: 1px solid #ced4da;"
                "    border-radius: 3px;"
                "    padding: 3px 8px;"
                "    color: #495057;"
                "}"
                );
            typeEdit->setReadOnly(true);
            typeEdit->setFixedWidth(200);
            typeLayout->addWidget(typeLabel);
            typeLayout->addWidget(typeEdit);
            typeLayout->addStretch();
            subLayout->addWidget(typeWidget);
        }

        if (subObj.contains("on")) {
            subcomponentItemCount++;
            QWidget *onWidget = new QWidget();
            QHBoxLayout *onLayout = new QHBoxLayout(onWidget);
            onLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *onLabel = new QLabel("On:");
            onLabel->setStyleSheet("color: #495057; min-width: 100px; font-weight: normal;");
            onLabel->setFixedWidth(100);
            QCheckBox *onCheckBox = new QCheckBox();
            onCheckBox->setChecked(subObj["on"].toBool());
            onCheckBox->setStyleSheet(
                "QCheckBox { color: #212529; }"
                "QCheckBox::indicator { width: 18px; height: 18px; }"
                );

            connect(onCheckBox, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta;
                delta["_id"] = mainID;
                QJsonObject updatedSubObj = subObj;
                updatedSubObj["on"] = checked;
                delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                emit valueChanged(ConnectedID, Name, delta);
            });
            onLayout->addWidget(onLabel);
            onLayout->addWidget(onCheckBox);
            onLayout->addStretch();
            subLayout->addWidget(onWidget);
        }

        if (subObj.contains("radioType")) {
            subcomponentItemCount++;
            QWidget *radioTypeWidget = new QWidget();
            QHBoxLayout *radioTypeLayout = new QHBoxLayout(radioTypeWidget);
            radioTypeLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *radioTypeLabel = new QLabel("Radio Type:");
            radioTypeLabel->setStyleSheet("color: #495057; min-width: 100px; font-weight: normal;");
            radioTypeLabel->setFixedWidth(100);
            QLineEdit *radioTypeEdit = new QLineEdit(subObj["radioType"].toString());
            radioTypeEdit->setStyleSheet(
                "QLineEdit {"
                "    background: #e9ecef;"
                "    border: 1px solid #ced4da;"
                "    border-radius: 3px;"
                "    padding: 3px 8px;"
                "    color: #495057;"
                "}"
                );
            radioTypeEdit->setReadOnly(true);
            radioTypeEdit->setFixedWidth(200);
            radioTypeLayout->addWidget(radioTypeLabel);
            radioTypeLayout->addWidget(radioTypeEdit);
            radioTypeLayout->addStretch();
            subLayout->addWidget(radioTypeWidget);
        }
        if (subObj.contains("default")) {
            QJsonObject defaultObj = subObj["default"].toObject();
            QGroupBox *defaultGroupBox = new QGroupBox("Default Parameters");
            defaultGroupBox->setStyleSheet(
                "QGroupBox {"
                "    background-color: #ffffff;"
                "    border: 1px solid #adb5bd;"
                "    border-radius: 5px;"
                "    margin-top: 5px;"
                "    padding-top: 10px;"
                "}"
                "QGroupBox::title {"
                "    subcontrol-origin: margin;"
                "    left: 10px;"
                "    padding: 0 5px 0 5px;"
                "    color: #6c757d;"
                "    font-size: 12px;"
                "    font-weight: bold;"
                "}"
                );
            QVBoxLayout *defaultLayout = new QVBoxLayout(defaultGroupBox);
            defaultLayout->setContentsMargins(10, 15, 10, 10);
            defaultLayout->setSpacing(8);
            for (const QString &defaultKey : defaultObj.keys()) {
                if (defaultKey == "type") continue;
                if (!developerMode && defaultKey.toLower() == "type") {
                    continue;
                }
                QWidget *defaultPropWidget = new QWidget();
                QHBoxLayout *defaultPropLayout = new QHBoxLayout(defaultPropWidget);
                defaultPropLayout->setContentsMargins(0, 0, 0, 0);

                QString displayDefaultKey = defaultKey;
                for (int i = 1; i < displayDefaultKey.length(); i++) {
                    if (displayDefaultKey[i].isUpper() && displayDefaultKey[i-1].isLower()) {
                        displayDefaultKey.insert(i, " ");
                        i++;
                    }
                }
                QLabel *defaultLabel = new QLabel(capitalizeFirstLetter(displayDefaultKey) + ":");
                defaultLabel->setStyleSheet("color: #495057; min-width: 120px; font-weight: normal;");
                defaultLabel->setFixedWidth(120);
                QJsonValue defaultValue = defaultObj[defaultKey];
                if (defaultValue.isObject()) {
                    QJsonObject defaultParamObj = defaultValue.toObject();
                    if (defaultParamObj.contains("type") && defaultParamObj["type"].toString() == "unitParam") {
                        subcomponentItemCount++;
                        QWidget *unitWidget = new QWidget();
                        QHBoxLayout *unitLayout = new QHBoxLayout(unitWidget);
                        unitLayout->setContentsMargins(0, 0, 0, 0);
                        WheelableLineEdit *valueEdit = new WheelableLineEdit();
                        valueEdit->setText(formatNumberForUI(defaultParamObj["value"].toDouble()));
                        valueEdit->setStyleSheet(
                            "QLineEdit {"
                            "    background: white;"
                            "    border: 1px solid #ced4da;"
                            "    border-radius: 3px;"
                            "    padding: 3px 8px;"
                            "    color: #212529;"
                            "}"
                            );
                        valueEdit->setFixedWidth(100);
                        QLabel *unitLabel = new QLabel(defaultParamObj["unit"].toString());
                        unitLabel->setStyleSheet("color: #6c757d; font-size: 12px; margin-left: 5px;");
                        connect(valueEdit, &QLineEdit::editingFinished, this, [=]() {
                            QJsonObject delta;
                            delta["_id"] = mainID;
                            QJsonObject updatedDefault = defaultObj;
                            QJsonObject updatedParam = defaultParamObj;
                            updatedParam["value"] = valueEdit->text().toDouble();
                            updatedDefault[defaultKey] = updatedParam;
                            QJsonObject updatedSubObj = subObj;
                            updatedSubObj["default"] = updatedDefault;
                            delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                            emit valueChanged(ConnectedID, Name, delta);
                        });
                        unitLayout->addWidget(valueEdit);
                        unitLayout->addWidget(unitLabel);
                        unitLayout->addStretch();
                        defaultPropLayout->addWidget(defaultLabel);
                        defaultPropLayout->addWidget(unitWidget);
                        defaultPropLayout->addStretch();
                    } else if (defaultParamObj.contains("type") && defaultParamObj["type"].toString() == "Section") {

                        for (const QString &nestedKey : defaultParamObj.keys()) {
                            if (nestedKey == "type") continue;
                            subcomponentItemCount++;
                            QJsonValue nestedValue = defaultParamObj[nestedKey];
                            if (nestedValue.isObject()) {
                                QJsonObject nestedParamObj = nestedValue.toObject();
                                if (nestedParamObj.contains("type") && nestedParamObj["type"].toString() == "unitParam") {
                                    QWidget *nestedWidget = new QWidget();
                                    QHBoxLayout *nestedLayout = new QHBoxLayout(nestedWidget);
                                    nestedLayout->setContentsMargins(0, 0, 0, 0);
                                    QString displayNestedKey = nestedKey;
                                    for (int i = 1; i < displayNestedKey.length(); i++) {
                                        if (displayNestedKey[i].isUpper() && displayNestedKey[i-1].isLower()) {
                                            displayNestedKey.insert(i, " ");
                                            i++;
                                        }
                                    }
                                    QLabel *nestedLabel = new QLabel("    " + capitalizeFirstLetter(displayNestedKey) + ":");
                                    nestedLabel->setStyleSheet("color: #495057; min-width: 140px; font-weight: normal;");
                                    nestedLabel->setFixedWidth(140);
                                    WheelableLineEdit *nestedValueEdit = new WheelableLineEdit();
                                    nestedValueEdit->setText(formatNumberForUI(nestedParamObj["value"].toDouble()));
                                    nestedValueEdit->setStyleSheet(
                                        "QLineEdit {"
                                        "    background: white;"
                                        "    border: 1px solid #ced4da;"
                                        "    border-radius: 3px;"
                                        "    padding: 3px 8px;"
                                        "    color: #212529;"
                                        "}"
                                        );
                                    nestedValueEdit->setFixedWidth(100);
                                    QLabel *nestedUnitLabel = new QLabel(nestedParamObj["unit"].toString());
                                    nestedUnitLabel->setStyleSheet("color: #6c757d; font-size: 12px; margin-left: 5px;");
                                    connect(nestedValueEdit, &QLineEdit::editingFinished, this, [=]() {
                                        QJsonObject delta;
                                        delta["_id"] = mainID;
                                        QJsonObject updatedDefault = defaultObj;
                                        QJsonObject updatedSection = defaultParamObj;
                                        QJsonObject updatedNestedParam = nestedParamObj;
                                        updatedNestedParam["value"] = nestedValueEdit->text().toDouble();
                                        updatedSection[nestedKey] = updatedNestedParam;
                                        updatedDefault[defaultKey] = updatedSection;
                                        QJsonObject updatedSubObj = subObj;
                                        updatedSubObj["default"] = updatedDefault;
                                        delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                                        emit valueChanged(ConnectedID, Name, delta);
                                    });
                                    QWidget *nestedUnitWidget = new QWidget();
                                    QHBoxLayout *nestedUnitLayout = new QHBoxLayout(nestedUnitWidget);
                                    nestedUnitLayout->setContentsMargins(0, 0, 0, 0);
                                    nestedUnitLayout->addWidget(nestedValueEdit);
                                    nestedUnitLayout->addWidget(nestedUnitLabel);
                                    nestedUnitLayout->addStretch();
                                    nestedLayout->addWidget(nestedLabel);
                                    nestedLayout->addWidget(nestedUnitWidget);
                                    nestedLayout->addStretch();
                                    defaultLayout->addWidget(nestedWidget);
                                }
                            }
                        }
                        continue;
                    }
                } else if (defaultValue.isBool()) {
                    subcomponentItemCount++;
                    QCheckBox *checkBox = new QCheckBox();
                    checkBox->setChecked(defaultValue.toBool());
                    checkBox->setStyleSheet(
                        "QCheckBox { color: #212529; }"
                        "QCheckBox::indicator { width: 18px; height: 18px; }"
                        );
                    connect(checkBox, &QCheckBox::toggled, this, [=](bool checked) {
                        QJsonObject delta;
                        delta["_id"] = mainID;
                        QJsonObject updatedDefault = defaultObj;
                        updatedDefault[defaultKey] = checked;
                        QJsonObject updatedSubObj = subObj;
                        updatedSubObj["default"] = updatedDefault;
                        delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                        emit valueChanged(ConnectedID, Name, delta);
                    });
                    defaultPropLayout->addWidget(defaultLabel);
                    defaultPropLayout->addWidget(checkBox);
                    defaultPropLayout->addStretch();
                } else if (defaultValue.isDouble()) {
                    subcomponentItemCount++;
                    QLineEdit *edit = new QLineEdit(QString::number(defaultValue.toDouble(), 'f', 2));
                    edit->setStyleSheet(
                        "QLineEdit {"
                        "    background: white;"
                        "    border: 1px solid #ced4da;"
                        "    border-radius: 3px;"
                        "    padding: 3px 8px;"
                        "    color: #212529;"
                        "}"
                        );
                    edit->setFixedWidth(150);
                    connect(edit, &QLineEdit::editingFinished, this, [=]() {
                        QJsonObject delta;
                        delta["_id"] = mainID;
                        QJsonObject updatedDefault = defaultObj;
                        updatedDefault[defaultKey] = edit->text().toDouble();
                        QJsonObject updatedSubObj = subObj;
                        updatedSubObj["default"] = updatedDefault;
                        delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                        emit valueChanged(ConnectedID, Name, delta);
                    });
                    defaultPropLayout->addWidget(defaultLabel);
                    defaultPropLayout->addWidget(edit);
                    defaultPropLayout->addStretch();
                } else if (defaultValue.isString()) {
                    subcomponentItemCount++;
                    QLineEdit *edit = new QLineEdit(defaultValue.toString());
                    edit->setStyleSheet(
                        "QLineEdit {"
                        "    background: white;"
                        "    border: 1px solid #ced4da;"
                        "    border-radius: 3px;"
                        "    padding: 3px 8px;"
                        "    color: #212529;"
                        "}"
                        );
                    edit->setFixedWidth(150);
                    connect(edit, &QLineEdit::editingFinished, this, [=]() {
                        QJsonObject delta;
                        delta["_id"] = mainID;
                        QJsonObject updatedDefault = defaultObj;
                        updatedDefault[defaultKey] = edit->text();
                        QJsonObject updatedSubObj = subObj;
                        updatedSubObj["default"] = updatedDefault;
                        delta[containerKey] = QJsonObject{{subKey, updatedSubObj}};
                        emit valueChanged(ConnectedID, Name, delta);
                    });
                    defaultPropLayout->addWidget(defaultLabel);
                    defaultPropLayout->addWidget(edit);
                    defaultPropLayout->addStretch();
                }
                defaultLayout->addWidget(defaultPropWidget);
            }
            subLayout->addWidget(defaultGroupBox);
        }
        subLayout->addStretch();
        subcomponentsLayout->addWidget(subGroupBox);
        subCount++;
        int subcomponentHeight = 40 + (subcomponentItemCount * 40) + 20;
        totalSubcomponentsHeight += subcomponentHeight;
    }
    if (subCount == 0) {
        QLabel *emptyLabel = new QLabel("No " + containerKey + " configured");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #6c757d; font-style: italic; padding: 20px;");
        subcomponentsLayout->addWidget(emptyLabel);
        totalSubcomponentsHeight += 60;
    }
    subcomponentsLayout->addStretch();
    scrollArea->setWidget(subcomponentsWidget);
    const int SCROLL_THRESHOLD = 650;
    int requiredHeight = totalSubcomponentsHeight + 50;
    if (requiredHeight > SCROLL_THRESHOLD) {
        scrollArea->setMinimumHeight(SCROLL_THRESHOLD);
        scrollArea->setMaximumHeight(SCROLL_THRESHOLD);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        scrollArea->setMinimumHeight(requiredHeight);
        scrollArea->setMaximumHeight(requiredHeight);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        bool isVisible = !scrollArea->isVisible();
        scrollArea->setVisible(isVisible);
        if (isVisible) {
            dropdownButton->setText("▲ " + QString::number(subCount) + " " +
                                    capitalizeFirstLetter(containerKey));

            int contentHeight = SCROLL_THRESHOLD + 40;
            if (requiredHeight < SCROLL_THRESHOLD) {
                contentHeight = requiredHeight + 40;
            }

            tableWidget->setRowHeight(row, contentHeight);
        } else {
            dropdownButton->setText("▼ " + QString::number(subCount) + " " +
                                    capitalizeFirstLetter(containerKey));
            tableWidget->setRowHeight(row, 35);
        }

        tableWidget->viewport()->update();
        containerWidget->adjustSize();
    });

    containerLayout->addWidget(dropdownButton);
    containerLayout->addWidget(scrollArea);
    tableWidget->setCellWidget(row, 1, containerWidget);
    tableWidget->setRowHeight(row, 35);
    tableWidget->blockSignals(false);
    tableWidget->resizeRowsToContents();
    if (tableWidget->rowCount() > 0) {
        tableWidget->scrollToTop();
    }
    tableWidget->viewport()->update();
}
void Inspector::refreshForDeveloperMode()
{
    if (!hierarchy || ConnectedID.isEmpty() || Name.isEmpty()) {
        return;
    }
    QJsonObject currentData = hierarchy->getComponentData(ConnectedID, Name);
    init(ConnectedID, Name, currentData);
}
void Inspector::resetState()
{

    rowToKeyPath.clear();
    customParameterKeys.clear();
    sectionInfo.clear();
    sectionRows.clear();
    ConnectedID.clear();
    Name.clear();
    mainID.clear();
    if (tableWidget) {
        tableWidget->blockSignals(true);
        tableWidget->clearContents();
        tableWidget->setRowCount(0);
        tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        tableWidget->blockSignals(false);
    }
    if (titleLabel) {
        titleLabel->setText("Inspector");
    }
    currentlyExpandedButton = nullptr;
    copiedComponentData = QJsonObject();
    copiedComponentType.clear();
}
