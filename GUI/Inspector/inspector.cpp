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

// Helper function to safely add widgets to layouts
static bool safeAddWidget(QLayout* layout, QWidget* widget, const QString& context = "") {
    if (!layout) {
        qDebug() << "safeAddWidget: Layout is null in context:" << context;
        return false;
    }
    if (!widget) {
        qDebug() << "safeAddWidget: Widget is null in context:" << context;
        return false;
    }
    layout->addWidget(widget);
    return true;
}

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
    if (!titleBarWidget) {
        qDebug() << "setupTitleBar: Failed to create titleBarWidget";
        return;
    }

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBarWidget);
    if (!titleLayout) {
        qDebug() << "setupTitleBar: Failed to create titleLayout";
        delete titleBarWidget;
        return;
    }

    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    // Create title label
    titleLabel = new QLabel("Inspector", titleBarWidget);
    if (!titleLabel) {
        qDebug() << "setupTitleBar: Failed to create titleLabel";
        delete titleLayout;
        delete titleBarWidget;
        return;
    }

    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: black; background-color: white; padding: 5px;"
        );
    titleLabel->setAlignment(Qt::AlignCenter);

    // Create menu button
    menuButton = new QPushButton("⋮", titleBarWidget);
    if (!menuButton) {
        qDebug() << "setupTitleBar: Failed to create menuButton";
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

    // Connect menu button to show context menu
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
    // Create menu
    QMenu *menu = new QMenu(this);
    if (!menu) {
        qDebug() << "createContextMenu: Failed to create menu";
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
        // Add changed ID to delta
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
    // Emit add tab signal
    emit addTabRequested();
}


void Inspector::setupUI()
{
    // Create container widget
    QWidget *container = new QWidget(this);
    if (!container) {
        qDebug() << "setupUI: Failed to create container";
        return;
    }

    QVBoxLayout *layout = new QVBoxLayout(container);
    if (!layout) {
        qDebug() << "setupUI: Failed to create main layout";
        delete container;
        return;
    }

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Setup title bar
    setupTitleBar();
    if (titleBarWidget) {
        setTitleBarWidget(titleBarWidget);
    }

    // Create table widget
    tableWidget = new QTableWidget(5, 2, this);
    if (!tableWidget) {
        qDebug() << "setupUI: Failed to create tableWidget";
        delete layout;
        delete container;
        return;
    }

    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setStyleSheet(
        "QTableWidget { background-color: white; color: black; border: 1px solid #ccc; }"
        "QTableWidget::item { border: 1px solid #ddd; color: black; }"
        "QTableWidget::item:selected { background-color: #e6f3ff; color: black; }"
        );
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setStyleSheet("alternate-background-color: #f9f9f9; background-color: white;");

    // Add table to layout
    if (!safeAddWidget(layout, tableWidget, "setupUI - tableWidget")) {
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }

    // Create button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    if (!buttonLayout) {
        qDebug() << "setupUI: Failed to create buttonLayout";
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }

    // Info button - LEFT SIDE
    QPushButton *infoButton = new QPushButton("Info", this);
    if (!infoButton) {
        qDebug() << "setupUI: Failed to create infoButton";
        delete buttonLayout;
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }

    infoButton->setFixedSize(40, 25);
    infoButton->setStyleSheet(
        "QPushButton { color: black; border: 1px solid #ccc; border-radius: 3px; background-color: #e9ecef; }"
        "QPushButton:hover { background-color: #dde1e4; }"
        );
    infoButton->setToolTip("Show component information");

    // Add stretch to push Add button to the right
    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    // Add button - RIGHT SIDE
    QPushButton *addButton = new QPushButton("Add", this);
    if (!addButton) {
        qDebug() << "setupUI: Failed to create addButton";
        delete infoButton;
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

    // Add widgets to button layout: Info (left) -> Spacer -> Add (right)
    if (!safeAddWidget(buttonLayout, infoButton, "setupUI - infoButton to buttonLayout")) {
        delete addButton;
        delete infoButton;
        delete buttonLayout;
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }

    buttonLayout->addSpacerItem(spacer);

    if (!safeAddWidget(buttonLayout, addButton, "setupUI - addButton to buttonLayout")) {
        delete addButton;
        delete infoButton;
        delete buttonLayout;
        delete tableWidget;
        delete layout;
        delete container;
        return;
    }

    // Set spacing for button layout
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(10, 5, 10, 5);

    // Add button layout to main layout
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

        // Add changed ID information
        QString changedId = ConnectedID + "_" + keyPath;
        delta["_changedId"] = changedId;
        delta["_entityId"] = ConnectedID;
        delta["_component"] = Name;
        delta["_parameter"] = keyPath;

        emit valueChanged(ConnectedID, Name, delta);
    });

    // Connect add button
    connect(addButton, &QPushButton::clicked, this, &Inspector::handleAddParameter);
    // Connect info button
    connect(infoButton, &QPushButton::clicked, this, &Inspector::handleInfoButton);
}
/* Create remove button for parameter */
QPushButton* Inspector::createRemoveButton(const QString &parameterName)
{
    // Create remove button
    QPushButton *removeButton = new QPushButton("❌", this);
    if (!removeButton) {
        qDebug() << "createRemoveButton: Failed to create removeButton";
        return nullptr;
    }

    removeButton->setFixedSize(20, 20);
    removeButton->setStyleSheet(
        "QPushButton { color: black; border-radius: 3px; background-color: #e9ecef; border: 1px solid #ccc; }"
        "QPushButton:hover { background-color: #dde1e4; }"
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

        // Create delta with ID information
        QJsonObject delta;

delta["_id"] = mainID;
         delta[parameterName] = QJsonValue();
        // Emit parameter changed signal
        emit parameterChanged(ConnectedID, Name, key, "", false);
        emit valueChanged(ConnectedID, Name, delta);
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

            // SIMPLIFIED delta
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
                // CHANGED: Pass this (Inspector pointer) to constructor
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
    qDebug() << "setupBooleanCell: Setting row:" << row << "for key:" << fullKey << "value:" << value;

    // Create checkbox widget
    QWidget *checkboxWidget = new QWidget();
    if (!checkboxWidget) {
        qDebug() << "setupBooleanCell: Failed to create checkboxWidget";
        return;
    }

    QCheckBox *checkBox = new QCheckBox();
    if (!checkBox) {
        qDebug() << "setupBooleanCell: Failed to create checkBox";
        delete checkboxWidget;
        return;
    }

    checkBox->setChecked(value);
    // Style checkbox
    checkBox->setStyleSheet(
        "QCheckBox { color: black; border: none; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #666; background-color: white; subcontrol-origin: padding; subcontrol-position: center; }"
        "QCheckBox::indicator:checked { image: url(:/icons/images/check-box.png); background-color: #007bff; }"
        "QCheckBox::indicator:unchecked { image: none; background-color: white; }"
        "QCheckBox::indicator:hover { border: 1px solid #007bff; }"
        );

    // Create layout for checkbox
    QHBoxLayout *layout = new QHBoxLayout(checkboxWidget);
    if (!layout) {
        qDebug() << "setupBooleanCell: Failed to create layout";
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
    if (!arrayWidget) {
        qDebug() << "setupArrayCell: Failed to create arrayWidget";
        return;
    }

    arrayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create dropdown button for right side
    QPushButton *dropdownButton = new QPushButton("▼", this);
    if (!dropdownButton) {
        qDebug() << "setupArrayCell: Failed to create dropdownButton";
        delete arrayWidget;
        return;
    }

    dropdownButton->setStyleSheet(
        "QPushButton { color: black;  border-radius: 3px; padding: 2px 8px; background-color: #e9ecef; border: 1px solid #ccc; }"
        "QPushButton:hover { background: #dde1e4; }"
        "QPushButton:pressed { background: #ced4da; }"
        );
    dropdownButton->setFixedSize(30, 25); // Fixed size for right alignment

    // Create list widget but initially hide it
    QListWidget *listWidget = new QListWidget();
    if (!listWidget) {
        qDebug() << "setupArrayCell: Failed to create listWidget";
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    listWidget->setProperty("row", row);
    listWidget->viewport()->installEventFilter(this);
    listWidget->setAcceptDrops(true);
    listWidget->setDropIndicatorShown(true);
    listWidget->setDragDropMode(QAbstractItemView::DropOnly);
    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listWidget->setMaximumHeight(fullKey == "entity" ? 50 : 200);
    listWidget->setVisible(false); // Initially hidden

    // Style list widget
    listWidget->setStyleSheet(
        "QListWidget { background: white; border: 1px solid #ccc; color: black; }"
        "QListWidget::item { color: black; border-bottom: 1px solid #eee; }"
        "QListWidget::item:selected { background-color: #e6f3ff; color: black; }"
        );

    // Populate list with array items
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        QString displayText;

        if (fullKey == "entity" || obj["type"].toString() == "reference") {
            // Capitalize first letter of name
            QString name = obj["name"].toString();
            displayText = capitalizeFirstLetter(name) +
                          (obj["id"].toString().isEmpty() ? "" : " (ID: " + obj["id"].toString() + ")");
        }
        else if (fullKey == "trajectories" && obj.contains("position")) {
            QJsonObject pos = obj["position"].toObject();
            displayText = QString("(%1, %2)")
                              .arg(formatNumberForUI(pos["x"].toDouble()))
                              .arg(formatNumberForUI(pos["z"].toDouble()));
        }
        else {
            // Capitalize name field
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
        listWidget->addItem(item);
    }

    // Create main layout for array widget
    QVBoxLayout *mainLayout = new QVBoxLayout(arrayWidget);
    if (!mainLayout) {
        qDebug() << "setupArrayCell: Failed to create mainLayout";
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }


    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // Create top row with dropdown button aligned to right
    QWidget *topRowWidget = new QWidget(arrayWidget);
    if (!topRowWidget) {
        qDebug() << "setupArrayCell: Failed to create topRowWidget";
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    QHBoxLayout *topRowLayout = new QHBoxLayout(topRowWidget);
    if (!topRowLayout) {
        qDebug() << "setupArrayCell: Failed to create topRowLayout";
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(0);

    // Add stretch to push button to right
    topRowLayout->addStretch();
    if (!safeAddWidget(topRowLayout, dropdownButton, "setupArrayCell - dropdownButton to topRowLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    // Add top row widget to main layout
    if (!safeAddWidget(mainLayout, topRowWidget, "setupArrayCell - topRowWidget to mainLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    // Add list widget to main layout
    if (!safeAddWidget(mainLayout, listWidget, "setupArrayCell - listWidget to mainLayout")) {
        delete topRowLayout;
        delete topRowWidget;
        delete mainLayout;
        delete listWidget;
        delete dropdownButton;
        delete arrayWidget;
        return;
    }

    // Add buttons for non-entity arrays (initially hidden)
    QWidget *buttonWidget = nullptr;
    QPushButton *removeBtn = nullptr;

    if (fullKey != "entity") {
        buttonWidget = new QWidget();
        if (!buttonWidget) {
            qDebug() << "setupArrayCell: Failed to create buttonWidget";
            // Continue without buttons
        } else {
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
                        buttonWidget->setVisible(false); // Initially hidden
                        safeAddWidget(mainLayout, buttonWidget, "setupArrayCell - buttonWidget to mainLayout");
                    }
                }
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

    // Toggle dropdown visibility
    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        bool isVisible = !listWidget->isVisible();
        listWidget->setVisible(isVisible);
        if (buttonWidget) {
            buttonWidget->setVisible(isVisible);
        }

        // Update button icon
        if (isVisible) {
            dropdownButton->setText("▲");
            // Adjust row height when expanded
            tableWidget->setRowHeight(row, fullKey == "entity" ? 80 : 200);
        } else {
            dropdownButton->setText("▼");
            // Reset row height when collapsed
            tableWidget->setRowHeight(row, 30);
        }
    });

    // Connect remove button
    if (fullKey != "entity" && removeBtn) {
        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                delete listWidget->takeItem(listWidget->row(item));

                // Create delta with ID for removal
                QJsonObject delta;
                delta["_id"] = mainID;

                // Update array
                QJsonArray updatedArray;
                for (int i = 0; i < listWidget->count(); ++i) {
                    QVariantMap itemData = listWidget->item(i)->data(Qt::UserRole).toMap();
                    updatedArray.append(QJsonObject::fromVariantMap(itemData));
                }
                delta[fullKey] = updatedArray;

                emitArrayChanged();
            }
        });
    }

    // Connect list item changes
    connect(listWidget, &QListWidget::itemChanged, this, [=](QListWidgetItem *item) {
        emitArrayChanged();
    });

    // Connect double-click to focus entity
    connect(listWidget, &QListWidget::doubleClicked, this, [=](const QModelIndex &index) {
        QListWidgetItem *item = listWidget->item(index.row());
        if (!item) return;
        QVariantMap itemData = item->data(Qt::UserRole).toMap();
        if (itemData.contains("id") && !itemData["id"].toString().isEmpty()) {
            emit foucsEntity(itemData["id"].toString());
        }
        emitArrayChanged();
    });

    // Set initial row height (collapsed state)
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, arrayWidget);
}

/* Setup string cell */
void Inspector::setupStringCell(int row, const QString &fullKey, const QString &value)
{
    qDebug() << "setupStringCell: Key:" << fullKey << "Value:" << value;

    // Create key item if not exists
    QTableWidgetItem *keyItem = tableWidget->item(row, 0);
    if (!keyItem) {
        // Capitalize the visible key (last part of fullKey)
        QString displayKey = capitalizeFirstLetter(fullKey.split(".").last());
        keyItem = new QTableWidgetItem(displayKey);
        if (keyItem) {
            keyItem->setBackground(QColor("#f8f9fa"));
            keyItem->setForeground(QColor(Qt::black));
            keyItem->setFlags(Qt::ItemIsEnabled);
            tableWidget->setItem(row, 0, keyItem);
        }
    }

    // Create input field
    QLineEdit *lineEdit = new QLineEdit();
    if (!lineEdit) {
        qDebug() << "setupStringCell: Failed to create lineEdit";
        return;
    }

    lineEdit->setText(value);
    lineEdit->setStyleSheet(
        "QLineEdit { color: black !important; background: white; border: 1px solid #ccc; padding: 2px; }"
        "QLineEdit:focus { border: 1px solid #007bff; background: white; }"
        );
    lineEdit->setFrame(true);
    lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Set read-only for specific fields
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

/* Setup number cell */
void Inspector::setupNumberCell(int row, const QString &fullKey, double value)
{
    qDebug() << "setupNumberCell: Setting row:" << row << "for key:" << fullKey << "value:" << value;

    // Create input field
    WheelableLineEdit *lineEdit = new WheelableLineEdit();
    if (!lineEdit) {
        qDebug() << "setupNumberCell: Failed to create lineEdit";
        return;
    }

    lineEdit->setText(formatNumberForUI(value));
    lineEdit->setStyleSheet(
        "QLineEdit { background: white; border: 1px solid #ccc; border-radius: 3px; color: black; }"
        );

    // Set validator
    QDoubleValidator *validator = new QDoubleValidator(lineEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(4);
    lineEdit->setValidator(validator);

    // Set row height and add widget
    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, lineEdit);

    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QJsonObject delta;

        delta[fullKey] = lineEdit->text().toDouble();


        delta["_id"] = mainID;

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

        // Create delta with ID information
        QJsonObject delta;
        delta["_id"] = mainID;
        delta[selectedKey] = QJsonValue();
        emit parameterChanged(ConnectedID, Name, selectedKey, "", false);
        emit valueChanged(ConnectedID, Name, delta);
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

                    // Create delta with ID information

                    QJsonObject delta;
                    delta["_id"] = mainID;

                    // Handle ENTITY drop specifically for formation
                    if (key == "entity") {
                        qDebug() << "ENTITY DROP DETECTED for formation";
                        qDebug() << "Custom data keys:" << customData.keys();

                        // CRITICAL: Check if listWidget is valid
                        if (!listWidget) {
                            qDebug() << "listWidget is null!";
                            dropEvent->acceptProposedAction();
                            return true;
                        }

                        // Check if this is an entity drop with proper validation
                        if (customData.contains("type") && customData["type"].toString() == "entity") {
                            // Get actual entity data - NOT DUMMY with safer access
                            QString actualID = "";
                            QString actualName = "";

                            // Try to get actual ID from different possible keys
                            if (customData.contains("ID") && customData["ID"].isValid()) {
                                actualID = customData["ID"].toString();
                            } else if (customData.contains("id") && customData["id"].isValid()) {
                                actualID = customData["id"].toString();
                            }

                            // Try to get actual name from different possible keys
                            if (customData.contains("name") && customData["name"].isValid()) {
                                actualName = customData["name"].toString();
                            } else if (customData.contains("Name") && customData["Name"].isValid()) {
                                actualName = customData["Name"].toString();
                            }

                            qDebug() << "Actual entity ID:" << actualID;
                            qDebug() << "Actual entity name:" << actualName;

                            // Clear the list and add the actual entity
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

                                // Create delta with the actual entity data
                                QJsonObject delta;
                                delta["entity"] = entityObj;

                                qDebug() << "Emitting valueChanged for entity drop with:" << entityObj;

                                // Check if ConnectedID and Name are valid
                                if (!ConnectedID.isEmpty() && !Name.isEmpty()) {
                                    emit valueChanged(ConnectedID, Name, delta);
                                } else {
                                    qDebug() << "Warning: ConnectedID or Name is empty!";
                                }
                            } else {
                                qDebug() << "Failed to create QListWidgetItem!";
                            }

                            dropEvent->acceptProposedAction();
                            return true;
                        }
                    }



                    // Handle entity drop
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
    qDebug() << "init: Initializing for ID:" << ID << "name:" << name << "object:" << object;
    // Set connected ID and name
    ConnectedID = ID;

    // Check if this is a subcomponent (type = "subcomponent")
    QString currentComponentId = "";

    if (object.contains("type") && object["type"].toString().toLower() == "subcomponent") {
        // This is a subcomponent, use its own ID
        if (object.contains("id")) {
            currentComponentId = object["id"].toString();
        } else if (object.contains("ID")) {
            currentComponentId = object["ID"].toString();
        } else {
            currentComponentId = ID + "_sub_" + name;
        }
        qDebug() << "This is a SUBCOMPONENT, using ID:" << currentComponentId;
    } else {
        // This is a regular component, use existing logic
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

    Name = name.toLower();
    // Normalize specific component names
    if (name.compare("Trajectories", Qt::CaseInsensitive) == 0) {
        Name = QString("trajectory");
    } else if (name.compare("dynamicModel", Qt::CaseInsensitive) == 0) {
        Name = QString("dynamicModel");
    } else if (name.compare("meshRenderer2d", Qt::CaseInsensitive) == 0) {
        Name = QString("meshRenderer2d");
    } else if (name.compare("crossSection", Qt::CaseInsensitive) == 0) {
        Name = QString("crossSection");
    }

    if (titleLabel) {
        titleLabel->setText(name);
    }

    // Clear table and data structures
    tableWidget->clearContents();
    tableWidget->blockSignals(true);
    rowToKeyPath.clear();
    customParameterKeys.clear();
    sectionInfo.clear();
    sectionRows.clear();

    // Fetch component data if empty
    if ((Name == QString("trajectory") || Name == QString("dynamicModel") ||
         Name == QString("meshRenderer2d") || Name == QString("collider")) &&
        object.isEmpty() && hierarchy) {
        QString dataType = Name;
        object = hierarchy->getComponentData(ID, dataType);
    }

    // Calculate row count including section parameters
    int rowCount = 0;
    for (const QString &key : object.keys()) {
        QJsonValue value = object[key];

        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString type = obj["type"].toString().toLower();

            if (type == "section") {
                // Section header + each parameter in section
                rowCount += 1 + (obj.keys().size() - 1); // -1 for "type" field
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

    // FIX: Scroll to FIRST row instead of last row
    if (tableWidget->rowCount() > 0) {
        tableWidget->scrollToTop();
        qDebug() << "init: Scrolled to top";
    }

    tableWidget->viewport()->update();
    qDebug() << "init: Table updated, rows:" << tableWidget->rowCount();
}



int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
    qDebug() << "addSimpleRow: Adding row for key:" << key << "at row:" << row << "type:" << value.type();

    // Check if this is a Section type object
    if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();

        if (type == "section") {
            // Store section info
            QString sectionKey = key;
            int headerRow = row;

            // Create section header with dropdown button
            QWidget *headerWidget = createSectionHeader(sectionKey, headerRow, obj);

            if (!headerWidget) {
                return row + 1;
            }

            // Set the header widget
            tableWidget->setCellWidget(row, 0, headerWidget);
            tableWidget->setRowHeight(row, 35);

            // Create an empty cell for the right side
            QTableWidgetItem *emptyItem = new QTableWidgetItem("");
            emptyItem->setFlags(Qt::ItemIsEnabled);
            emptyItem->setBackground(QColor("#2c3e50"));
            tableWidget->setItem(row, 1, emptyItem);

            // Store section info for tracking
            sectionInfo[sectionKey] = SectionInfo(
                headerRow,
                obj.keys().size() - 1, // number of parameters (-1 for type)
                true // initially expanded
                );

            // Add parameters rows (initially visible)
            int currentRow = row + 1;
            for (const QString &paramKey : obj.keys()) {
                if (paramKey == "type") continue;

                // Insert row for parameter
                tableWidget->insertRow(currentRow);
                QString fullKey = QString("%1.%2").arg(sectionKey).arg(paramKey);
                rowToKeyPath[currentRow] = fullKey;

                // Parameter name with indentation
                QString displayKey = paramKey;
                // Convert camelCase to readable format
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

                // Handle the parameter value
                QJsonValue paramValue = obj[paramKey];

                if (paramValue.isObject()) {
                    QJsonObject paramObj = paramValue.toObject();

                    if (paramObj.contains("type") && paramObj["type"].toString() == "unitParam") {
                        // Create widget for unit parameter
                        setupUnitParameterCell(currentRow, fullKey, paramObj);
                    } else {
                        // Regular object parameter
                        setupValueCell(currentRow, fullKey, paramValue);
                    }
                } else {
                    // Simple value parameter
                    setupValueCell(currentRow, fullKey, paramValue);
                }

                // Store that this row belongs to this section
                sectionRows[currentRow] = sectionKey;

                currentRow++;
            }

            return currentRow;  // Return next available row
        }
        // NEW: Check for unitParam type directly (not in section)
        else if (type == "unitparam") {
            rowToKeyPath[row] = key;

            // Capitalize key label
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

    // Rest of the existing code for non-section objects...
    rowToKeyPath[row] = key;

    // Capitalize key label
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
            vector->setMainID(mainID);  // NEW: Pass mainID
            vector->setupVectorCell(row, key, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "geocord" || type == "geooffset") {
            // CHANGED: Pass this (Inspector pointer)
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
            option->setMainID(mainID);  // NEW: Pass mainID
            option->setupOptionCell(row, key, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setMainID(mainID);  // NEW: Pass mainID
            color->setupColorCell(row, key, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setMainID(mainID);  // NEW: Pass mainID
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
    qDebug() << "setupValueCell: Processing key:" << fullKey << "type:" << value.type();

    // Handle different value types
    if (value.isBool()) {
        setupBooleanCell(row, fullKey, value.toBool());
    }
    else if (value.isArray()) {
        setupArrayCell(row, fullKey, value.toArray());
    }
    else if (value.isObject()) {
        QJsonObject obj = value.toObject();
        QString type = obj["type"].toString().toLower();

        // Check for unitParam type
        if (type == "unitparam") {
            setupUnitParameterCell(row, fullKey, obj);
        }
        // Setup other specialized templates
        else if (type == "vector") {
            VectorTemplate *vector = new VectorTemplate(this);
            vector->setConnectedID(ConnectedID);
            vector->setName(Name);
            vector->setMainID(mainID);  // NEW: Pass mainID
            vector->setupVectorCell(row, fullKey, obj, tableWidget);
            connect(vector, &VectorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "geocord" || type == "geooffset") {
            // CHANGED: Pass this (Inspector pointer)
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
            option->setMainID(mainID);  // NEW: Pass mainID
            option->setupOptionCell(row, fullKey, obj, tableWidget);
            connect(option, &OptionTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "color") {
            ColorTemplate *color = new ColorTemplate(this);
            color->setConnectedID(ConnectedID);
            color->setName(Name);
            color->setMainID(mainID);  // NEW: Pass mainID
            color->setupColorCell(row, fullKey, obj, tableWidget);
            connect(color, &ColorTemplate::valueChanged, this, &Inspector::valueChanged);
        }
        else if (type == "image") {
            ImageTemplate *image = new ImageTemplate(this);
            image->setConnectedID(ConnectedID);
            image->setName(Name);
            image->setMainID(mainID);  // NEW: Pass mainID
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
    qDebug() << "setupGenericObjectCell: Setting row:" << row << "for key:" << fullKey;

    QWidget *valueWidget = new QWidget();
    if (!valueWidget) {
        qDebug() << "setupGenericObjectCell: Failed to create valueWidget";
        return;
    }

    QVBoxLayout *layout = new QVBoxLayout(valueWidget);
    if (!layout) {
        qDebug() << "setupGenericObjectCell: Failed to create layout";
        delete valueWidget;
        return;
    }

    layout->setContentsMargins(0, 0, 0, 0);

    for (const QString &subKey : obj.keys()) {
        // Skip type field if it exists
        if (subKey == "type") continue;

        QHBoxLayout *subLayout = new QHBoxLayout();
        if (!subLayout) {
            qDebug() << "setupGenericObjectCell: Failed to create subLayout for" << subKey;
            continue;
        }

        // Capitalize subfield label
        QLabel *label = new QLabel(capitalizeFirstLetter(subKey));
        if (!label) {
            delete subLayout;
            continue;
        }

        label->setStyleSheet("color: black; min-width: 20px;");
        safeAddWidget(subLayout, label, "setupGenericObjectCell - label to subLayout");

        QJsonValue subValue = obj[subKey];

        // Check if this is a modulation field - यहाँ change किया
        bool isModulationField = fullKey.contains("modulation", Qt::CaseInsensitive);

        if (isModulationField && subValue.isString()) {
            // Modulation fields के लिए string input
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

                delta[fullKey] = QJsonObject{{subKey, edit->text()}}; // ✅ String value
                emit valueChanged(ConnectedID, Name, delta);
            });
        }
        else if (subValue.isDouble()) {
            // Number values के लिए (original code)
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
            // Boolean values के लिए - inline checkbox
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

            // Add checkbox to layout
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
            // Other types (string, etc.)
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
        if (keyItem) {
            keyItem->setFlags(Qt::ItemIsEnabled);
            keyItem->setBackground(QColor("#f8f9fa"));
            keyItem->setForeground(Qt::black);
            tableWidget->setItem(trajRow, 0, keyItem);
        }
        setupArrayCell(trajRow, "trajectories", QJsonArray());
    }
    // Get array widget
    QWidget *arrayWidget = tableWidget->cellWidget(trajRow, 1);
    if (!arrayWidget) {
        setupArrayCell(trajRow, "trajectories", QJsonArray());
        arrayWidget = tableWidget->cellWidget(trajRow, 1);
    }
    if (!arrayWidget) return;

    QListWidget *listWidget = arrayWidget->findChild<QListWidget*>();
    if (!listWidget) return;
    // Update inspector if needed
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
    // Clear and populate list
    listWidget->clear();
    for (const QJsonValue &val : waypoints) {
        QJsonObject obj = val.toObject();
        if (!obj.contains("position")) continue;
        QJsonObject pos = obj["position"].toObject();
        QString displayText = QString("(%1, %2)")
                                  .arg(formatNumberForUI(pos["x"].toDouble()))
                                  .arg(formatNumberForUI(pos["z"].toDouble()));
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


static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}

void Inspector::handleInfoButton()
{
    // Create NON-MODAL dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("📊 Unit Information");
    dialog->setMinimumSize(300, 200);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Style the dialog
    dialog->setStyleSheet(
        "QDialog {"
        "    background-color: #ffffff;"
        "    border: 2px solid #3498db;"
        "    border-radius: 10px;"
        "}"
        );

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(25, 25, 25, 25);

    // Header
    QLabel *headerLabel = new QLabel("📏 UNITS INFORMATION");
    headerLabel->setStyleSheet(
        "QLabel {"
        "    color: #2c3e50;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    padding-bottom: 10px;"
        "    border-bottom: 2px solid #3498db;"
        "}"
        );
    headerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(headerLabel);

    // Create info layout
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(15);
    infoLayout->setContentsMargins(10, 10, 10, 10);

    // 1. SPEED
    QHBoxLayout *speedLayout = new QHBoxLayout();
    QLabel *speedIcon = new QLabel("⚡");
    speedIcon->setStyleSheet("font-size: 22px;");

    QLabel *speedText = new QLabel("<b>Speed:</b> Kilometers per hour (km/h)");
    speedText->setStyleSheet("font-size: 14px; color: #2c3e50;");

    speedLayout->addWidget(speedIcon);
    speedLayout->addWidget(speedText);
    speedLayout->addStretch();
    infoLayout->addLayout(speedLayout);

    // Separator
    QFrame *separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("color: #ecf0f1;");
    infoLayout->addWidget(separator1);

    // 2. TIME
    QHBoxLayout *timeLayout = new QHBoxLayout();
    QLabel *timeIcon = new QLabel("⏱️");
    timeIcon->setStyleSheet("font-size: 22px;");

    QLabel *timeText = new QLabel("<b>Time:</b> Seconds (s)");
    timeText->setStyleSheet("font-size: 14px; color: #2c3e50;");

    timeLayout->addWidget(timeIcon);
    timeLayout->addWidget(timeText);
    timeLayout->addStretch();
    infoLayout->addLayout(timeLayout);

    // Separator
    QFrame *separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet("color: #ecf0f1;");
    infoLayout->addWidget(separator2);

    // 3. TURN RADIUS
    QHBoxLayout *radiusLayout = new QHBoxLayout();
    QLabel *radiusIcon = new QLabel("🔄");
    radiusIcon->setStyleSheet("font-size: 22px;");

    QLabel *radiusText = new QLabel("<b>Turn Radius:</b> Meters (m)");
    radiusText->setStyleSheet("font-size: 14px; color: #2c3e50;");

    radiusLayout->addWidget(radiusIcon);
    radiusLayout->addWidget(radiusText);
    radiusLayout->addStretch();
    infoLayout->addLayout(radiusLayout);

    layout->addLayout(infoLayout);

    // Note
    QLabel *noteLabel = new QLabel(
        "ℹ️ This information shows the standard units\n"
        "used for movement parameters in the system."
        );
    noteLabel->setStyleSheet(
        "QLabel {"
        "    color: #7f8c8d;"
        "    font-size: 12px;"
        "    font-style: italic;"
        "    padding: 10px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 5px;"
        "    border: 1px solid #dee2e6;"
        "}"
        );
    noteLabel->setAlignment(Qt::AlignCenter);
    noteLabel->setWordWrap(true);
    layout->addWidget(noteLabel);

    layout->addStretch();

    // Close button
    QPushButton *closeButton = new QPushButton("✅ Got it", dialog);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 30px;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #219653;"
        "}"
        );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    // Connect close button
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::close);

    // Show dialog
    dialog->show();
    // dialog->raise();
    // dialog->activateWindow();
}

void Inspector::setupUnitParameterCell(int row, const QString &fullKey, const QJsonObject &paramObj)
{
    qDebug() << "setupUnitParameterCell: Setting row:" << row << "for key:" << fullKey;

    // Create widget for unit parameter
    QWidget *unitWidget = new QWidget();
    if (!unitWidget) return;

    QHBoxLayout *layout = new QHBoxLayout(unitWidget);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(8);

    // Value editor
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

    // Unit label
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
