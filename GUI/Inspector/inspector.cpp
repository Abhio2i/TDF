
/* ========================================================================= */
/* File: inspector.cpp                                                     */
/* Purpose: Implements inspector widget for editing component properties     */
//               Written by Arti Rajpoot
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
#include "inspector-styles.h"
#include <QToolTip>
#include <QCursor>
#include "tests/inspectortest/inspectortest.h"
#include "GUI/mainwindow.h"

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
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setStyleSheet(InspectorStyles::WheelableLineEdit);
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
    setStyleSheet(InspectorStyles::InspectorWidget);
    setupUI();
     runUnitTestsOnce();
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
    titleBarWidget->setStyleSheet(InspectorStyles::TitleBar);
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
    titleLabel->setStyleSheet(InspectorStyles::TitleLabel);
    titleLabel->setAlignment(Qt::AlignCenter);

    menuButton = new QPushButton("⋮", titleBarWidget);
    if (!menuButton) {
        delete titleLabel;
        delete titleLayout;
        delete titleBarWidget;
        return;
    }
    menuButton->setStyleSheet(InspectorStyles::MenuButton);
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

QMenu* Inspector::createContextMenu()
{
    QMenu *menu = new QMenu(this);
    if (!menu) {
        return nullptr;
    }

    QAction *copyAction = menu->addAction("Copy Component");
    QAction *pasteAction = menu->addAction("Paste Component");
    pasteAction->setEnabled(!copiedComponentData.isEmpty());

    // ── Reset action ──────────────────────────────────────────────────────

    QAction *resetAction = menu->addAction("Reset Component");
    resetAction->setEnabled(!m_initialComponentData.isEmpty());
    resetAction->setToolTip("Restore component to its initial loaded values");
    // ─────────────────────────────────────────────────────────────────────

    // QAction *lockAction = menu->addAction("Lock");
    // QAction *unlockAction = menu->addAction("Unlock");
    // QAction *addTabAction = menu->addAction("Add Tab");

    QAction *closeAction = menu->addAction("Close");

    connect(copyAction, &QAction::triggered, this, &Inspector::copyCurrentComponent);
    connect(pasteAction, &QAction::triggered, this, &Inspector::pasteToCurrentComponent);
    // connect(addTabAction, &QAction::triggered, this, &Inspector::handleAddTab);
    connect(closeAction, &QAction::triggered, this, [](){});

    // ── Reset connection ──────────────────────────────────────────────────
    connect(resetAction, &QAction::triggered, this, [this]() {
        if (m_initialComponentData.isEmpty() || ConnectedID.isEmpty() || Name.isEmpty()) {
            return;
        }
        // Restore UI to initial state
        init(ConnectedID, Name, m_initialComponentData);

        // Emit so hierarchy also gets updated
        QJsonObject delta = m_initialComponentData;
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });
    // ─────────────────────────────────────────────────────────────────────

    return menu;
}
/* Copy current component data */
void Inspector::copyCurrentComponent()
{
    if (!hierarchy || Name.isEmpty() || ConnectedID.isEmpty()) {
        return;
    }
#ifndef GUI_UNIT_TEST
    copiedComponentData = hierarchy->getComponentData(ConnectedID, Name);
#endif
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
#ifndef GUI_UNIT_TEST
        init(ConnectedID, Name, hierarchy->getComponentData(ConnectedID, Name));
#endif
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

    // Apply border to container with proper margins to show title bar inside
    container->setStyleSheet(R"(
        QWidget {
            background-color: #0F2636;
            border: 2px solid #27446d;
            border-radius: 3px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(container);
    if (!layout) {
        delete container;
        return;
    }
    layout->setContentsMargins(2, 2, 2, 2);

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
    // FIX: Column 0 = Interactive (30%), Column 1 = Stretch (70%)
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    // Set column 0 to ~30% width (fixed pixel, will look correct at normal inspector widths)
    tableWidget->setStyleSheet(InspectorStyles::TableWidget);
    // Remove the vertical column separator line drawn by Qt between columns
    tableWidget->horizontalHeader()->setDefaultSectionSize(0);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setShowGrid(false);
    tableWidget->setFrameShape(QFrame::NoFrame);
    tableWidget->verticalHeader()->setDefaultSectionSize(30);
    tableWidget->viewport()->setStyleSheet("background-color: #0F2636; border: none;");

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
    addButton->setStyleSheet(InspectorStyles::AddButton);
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
    removeButton->setStyleSheet(InspectorStyles::RemoveButton);
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

// Add HandleAddParameter
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
    checkBox->setStyleSheet(InspectorStyles::CheckBox);

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

    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->setContentsMargins(5, 0, 0, 0);

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

/* Setup array cell */
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

    dropdownButton->setStyleSheet(InspectorStyles::DropdownButton);
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
    m_itemHeight = 30;

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
        int listHeight = qMin(array.size(), maxVisibleItems) * m_itemHeight + 10;
        listWidget->setMaximumHeight(listHeight);
    }

    listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    listWidget->setVisible(false);
    listWidget->setStyleSheet(InspectorStyles::ListWidget);

    int itemsPerView = fixedListHeight / m_itemHeight;
    listWidget->setProperty("itemsPerView", itemsPerView);

    // Add items with special background for first and last waypoints
    for (int i = 0; i < array.size(); i++) {
        const QJsonValue &val = array[i];
        QJsonObject obj = val.toObject();
        QString displayText;

        if (fullKey == "entity" || obj["type"].toString() == "reference") {
            QString name = obj["name"].toString();
            displayText = capitalizeFirstLetter(name) +
                          (obj["id"].toString().isEmpty() ? "" : " (ID: " + obj["id"].toString() + ")");
        }
        else if (fullKey == "trajectories" && obj.contains("position")) {
            QJsonObject pos = obj["position"].toObject();
            double speed = obj.value("speed").toDouble();
            displayText = QString("Lat: %1, Alt: %2, Lon: %3, Speed: %4 Km/h")
                              .arg(formatNumberForUI(pos["x"].toDouble()))
                              .arg(formatNumberForUI(pos["y"].toDouble()))
                              .arg(formatNumberForUI(pos["z"].toDouble()))
                              .arg(formatNumberForUI(speed));
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
        item->setSizeHint(QSize(0, m_itemHeight));
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
    if (fullKey == "trajectories" && removeBtn) {
        connect(removeBtn, &QPushButton::clicked, this, [=]() {
            QListWidgetItem *item = listWidget->currentItem();
            if (item) {
                int row = listWidget->row(item);
                delete listWidget->takeItem(row);
                emitArrayChanged();
            }
        });
    }

    connect(listWidget, &QListWidget::itemChanged, this, [=](QListWidgetItem *item) {
        if (fullKey != "trajectories") return;
        QString newText = item->text();
        QRegExp regex("Lat:\\s*([\\d\\.\\-]+),\\s*Alt:\\s*([\\d\\.\\-]+),\\s*Lon:\\s*([\\d\\.\\-]+),\\s*Speed:\\s*([\\d\\.\\-]+)\\s*Km/h");
        if (regex.indexIn(newText) != -1) {
            double lat = regex.cap(1).toDouble();
            double alt = regex.cap(2).toDouble();
            double lon = regex.cap(3).toDouble();
            double speed = regex.cap(4).toDouble();
            QVariantMap itemData = item->data(Qt::UserRole).toMap();
            QVariantMap position = itemData["position"].toMap();
            position["x"] = lat;
            position["y"] = alt;
            position["z"] = lon;
            itemData["position"] = position;
            itemData["speed"] = speed;
            item->setData(Qt::UserRole, itemData);
            emitArrayChanged();
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
    lineEdit->setStyleSheet(InspectorStyles::LineEdit);
    lineEdit->setFrame(true);
    lineEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (fullKey.contains("id") || fullKey.contains("name") || fullKey.contains("branch")) {
        lineEdit->setReadOnly(true);
        lineEdit->setStyleSheet(InspectorStyles::ReadOnlyLineEdit);
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

// SetupNumberCell
void Inspector::setupNumberCell(int row, const QString &fullKey, double value)
{
    WheelableLineEdit *lineEdit = new WheelableLineEdit();
    if (!lineEdit) {
        return;
    }

    lineEdit->setText(formatNumberForUI(value));
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

// HandleRemoveParameter
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

// EventFilter
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
                                }
                            }
                            dropEvent->acceptProposedAction();
                            return true;
                        }
                    }
                    else if (key == "sensors" || key == "iffs" || key == "radios") {
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
#ifndef GUI_UNIT_TEST
                            QJsonObject transformData = hierarchy->getComponentData(customData["ID"].toString(), "transform");
#else
                            QJsonObject transformData;
#endif
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

// init
void Inspector::init(QString ID, QString name, QJsonObject object)
{
    // Block signals during initialization
    if (tableWidget) {
        tableWidget->blockSignals(true);
    }

    // Clear previous state FIRST
    rowToKeyPath.clear();
    customParameterKeys.clear();
    sectionInfo.clear();
    sectionRows.clear();
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

    // FIX: Set column resize mode - key=30% (Interactive), value=70% (Stretch)
    if (tableWidget) {
        if (newName == "sensors" || newName == "iffs" || newName == "radios") {
            tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        } else {
            tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
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
    // Update title
    if (titleLabel) {
        titleLabel->setText(name);
    }
    // Clear and reset table
    if (tableWidget) {
        tableWidget->clearContents();
        tableWidget->setRowCount(0);
    }
    // Load component data if empty
    if ((Name == QString("trajectory") || Name == QString("dynamicModel") ||
         Name == QString("meshRenderer2d") || Name == QString("collider")) &&
        object.isEmpty() && hierarchy) {
        QString dataType = Name;
#ifndef GUI_UNIT_TEST
        object = hierarchy->getComponentData(ID, dataType);
#else
        object = QJsonObject{};
#endif
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
        if (m_initialComponentData.isEmpty()) {
            m_initialComponentData = object;
        }
        tableWidget->blockSignals(false);
        tableWidget->resizeRowsToContents();
        // Re-apply column width after resizeRowsToContents (it can reset columns)
        tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        if (tableWidget->rowCount() > 0) {
            tableWidget->scrollToTop();
        }
        tableWidget->viewport()->update();
    }
}

// Add addSimpleRow
int Inspector::addSimpleRow(int row, const QString &key, const QJsonValue &value)
{
#ifndef GUI_UNIT_TEST
    bool developerMode = ApplicationDialog::getGlobalDeveloperMode();
#else
    bool developerMode = false;
#endif
    if (!developerMode && (key.toLower() == "id" || key.toLower() == "type" ||
                           key.toLower() == "parent_id")) {
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
    if (key.toLower() == "entities" || key.toLower() == "folders") {
        return row; // skip entirely, don't increment row
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

// setupValueCell
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

// setupGenericObjectCell
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
        label->setStyleSheet(InspectorStyles::GenericObjectLabel);
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
            edit->setStyleSheet(InspectorStyles::LineEdit);
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
            checkBox->setStyleSheet(InspectorStyles::CheckBox);
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
            edit->setStyleSheet(InspectorStyles::LineEdit);
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

#ifndef GUI_UNIT_TEST
        QJsonObject trajData = hierarchy ? hierarchy->getComponentData(entityId, "trajectory") : QJsonObject();
#else
        QJsonObject trajData;
#endif
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

    int itemHeight = m_itemHeight > 0 ? m_itemHeight : 30;

    for (int i = 0; i < waypoints.size(); i++) {
        const QJsonValue &val = waypoints[i];
        QJsonObject obj = val.toObject();
        if (!obj.contains("position")) continue;

        QJsonObject pos = obj["position"].toObject();
        double speed = obj.value("speed").toDouble(800.0);

        QString displayText = QString("Lat: %1, Alt: %2, Lon: %3, Speed: %4 Km/h")
                                  .arg(formatNumberForUI(pos["x"].toDouble()))
                                  .arg(formatNumberForUI(pos["y"].toDouble()))
                                  .arg(formatNumberForUI(pos["z"].toDouble()))
                                  .arg(formatNumberForUI(speed));

        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, obj.toVariantMap());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setSizeHint(QSize(0, itemHeight));

        listWidget->addItem(item);
    }

    if (waypoints.isEmpty() && hierarchy) {
#ifndef GUI_UNIT_TEST
        QJsonObject trajData = hierarchy->getComponentData(ConnectedID, "trajectory");
#else
        QJsonObject trajData;
#endif
        if (!trajData.isEmpty() && trajData.contains("trajectories")) {
            QJsonArray array = trajData["trajectories"].toArray();
            for (int i = 0; i < array.size(); i++) {
                const QJsonValue &val = array[i];
                QJsonObject obj = val.toObject();
                if (!obj.contains("position")) continue;

                QJsonObject pos = obj["position"].toObject();
                double speed = obj.value("speed").toDouble(800.0);

                QString displayText = QString("Lat: %1, Alt: %2, Lon: %3, Speed: %4 Km/h")
                                          .arg(formatNumberForUI(pos["x"].toDouble()))
                                          .arg(formatNumberForUI(pos["y"].toDouble()))
                                          .arg(formatNumberForUI(pos["z"].toDouble()))
                                          .arg(formatNumberForUI(speed));

                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, obj.toVariantMap());
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setSizeHint(QSize(0, itemHeight));

                listWidget->addItem(item);
            }
        }
    }

    tableWidget->viewport()->update();
}

// capitalizeFirstLetter
static QString capitalizeFirstLetter(const QString &str)
{
    if (str.isEmpty()) return str;
    return str[0].toUpper() + str.mid(1);
}

// =========================================================================
// FIX: setupUnitParameterCell
// Value edit expands to fill column 1; unit label appears RIGHT after value,
// NOT pushed to the far end. addStretch() is at the end only.
// =========================================================================
// void Inspector::setupUnitParameterCell(int row, const QString &fullKey, const QJsonObject &paramObj)
// {
//     QWidget *unitWidget = new QWidget();
//     if (!unitWidget) return;
//     unitWidget->setFixedHeight(30);

//     QHBoxLayout *layout = new QHBoxLayout(unitWidget);
//     layout->setContentsMargins(0, 0, 0, 0);
//     layout->setSpacing(0);
//     layout->setAlignment(Qt::AlignVCenter);

//     WheelableLineEdit *valueEdit = new WheelableLineEdit();
//     valueEdit->setText(formatNumberForUI(paramObj["value"].toDouble()));
//     QDoubleValidator *validator = new QDoubleValidator(valueEdit);
//     validator->setNotation(QDoubleValidator::StandardNotation);
//     validator->setDecimals(4);
//     valueEdit->setValidator(validator);
//     valueEdit->setFixedHeight(30);
//     valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

//     QString unit = paramObj["unit"].toString();
//     QLabel *unitLabel = new QLabel(unit);
//     unitLabel->setStyleSheet(InspectorStyles::UnitParamLabel);
//     unitLabel->setFixedHeight(30);
//     unitLabel->setFixedWidth(qMax(35, unitLabel->fontMetrics().horizontalAdvance(unit) + 12));
//     unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

//     layout->addWidget(valueEdit);
//     layout->addWidget(unitLabel);

//     tableWidget->setRowHeight(row, 30);
//     tableWidget->setCellWidget(row, 1, unitWidget);

//     connect(valueEdit, &QLineEdit::editingFinished, this, [=]() {
//         QJsonObject delta;
//         QJsonObject updatedParam = paramObj;
//         updatedParam["value"] = valueEdit->text().toDouble();

//         if (fullKey.contains(".")) {
//             QStringList parts = fullKey.split(".");
//             delta[parts[0]] = QJsonObject{{parts[1], updatedParam}};
//         } else {
//             delta[fullKey] = updatedParam;
//         }
//         delta["_id"] = mainID;
//         emit valueChanged(ConnectedID, Name, delta);
//     });
// }


void Inspector::setupUnitParameterCell(int row, const QString &fullKey, const QJsonObject &paramObj)
{
    // ── Key label tooltip (column 0 ONLY) ────────────────────────────────
    if (QTableWidgetItem *keyItem = tableWidget->item(row, 0)) {
        QString desc     = paramObj["description"].toString();
        double  minVal   = paramObj["min"].toDouble(0.0);
        double  maxVal   = paramObj["max"].toDouble(0.0);
        bool    hasRange = !(qFuzzyIsNull(minVal) && qFuzzyIsNull(maxVal));

        QString tooltip;
        if (!desc.isEmpty())
            tooltip += desc;
        if (hasRange) {
            if (!tooltip.isEmpty()) tooltip += "\n";
            tooltip += QString("Range: %1 – %2 %3")
                           .arg(formatNumberForUI(minVal))
                           .arg(formatNumberForUI(maxVal))
                           .arg(paramObj["unit"].toString());
        }
        if (!tooltip.isEmpty())
            keyItem->setToolTip(tooltip);
    }
    // ── Value widget ──────────────────────────────────────────────────────
    QWidget *unitWidget = new QWidget();
    if (!unitWidget) return;
    unitWidget->setFixedHeight(30);

    QHBoxLayout *layout = new QHBoxLayout(unitWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignVCenter);

    WheelableLineEdit *valueEdit = new WheelableLineEdit();
    QString initialValue = formatNumberForUI(paramObj["value"].toDouble());
    valueEdit->setText(initialValue);
    // NO setToolTip on valueEdit
    QDoubleValidator *validator = new QDoubleValidator(valueEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(6);
    valueEdit->setValidator(validator);
    valueEdit->setFixedHeight(30);
    valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Save the original stylesheet BEFORE any error styling
    const QString originalStyle = valueEdit->styleSheet();

    // QString unit = paramObj["unit"].toString();
    // QLabel *unitLabel = new QLabel(unit);
    // unitLabel->setStyleSheet(InspectorStyles::UnitParamLabel);
    // unitLabel->setFixedHeight(30);
    // unitLabel->setFixedWidth(qMax(35, unitLabel->fontMetrics().horizontalAdvance(unit) + 12));
    // unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QString unit = paramObj["unit"].toString();
    QLabel *unitLabel = new QLabel(unit);
    unitLabel->setStyleSheet(InspectorStyles::UnitParamLabel);
    unitLabel->setFixedHeight(30);
    unitLabel->setFixedWidth(60);  // consistent fixed width for all units
    unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layout->addWidget(valueEdit);
    layout->addWidget(unitLabel);

    tableWidget->setRowHeight(row, 30);
    tableWidget->setCellWidget(row, 1, unitWidget);

    // ── Shared state ──────────────────────────────────────────────────────
    auto lastGoodValue = std::make_shared<QString>(initialValue);
    auto inErrorState  = std::make_shared<bool>(false);

    double  captMin  = paramObj["min"].toDouble(0.0);
    double  captMax  = paramObj["max"].toDouble(0.0);
    bool    hasRange = !(qFuzzyIsNull(captMin) && qFuzzyIsNull(captMax));
    QString captUnit = unit;

    // ── editingFinished ───────────────────────────────────────────────────
    connect(valueEdit, &QLineEdit::editingFinished, this, [=]() {
        double enteredVal = valueEdit->text().toDouble();

        if (hasRange && (enteredVal < captMin || enteredVal > captMax)) {
            // Enter error state — override style with red
            *inErrorState = true;
            valueEdit->setStyleSheet(
                "QLineEdit { border: 1px solid #e74c3c; background-color: #2d0a0a; "
                "color: #e74c3c; padding-left: 4px; }");
            QString errMsg = QString("⚠ Value out of range!\nAllowed: %1 – %2 %3\nEntered: %4")
                                 .arg(formatNumberForUI(captMin))
                                 .arg(formatNumberForUI(captMax))
                                 .arg(captUnit)
                                 .arg(formatNumberForUI(enteredVal));
            QToolTip::showText(QCursor::pos(), errMsg, valueEdit);
            return;   // do NOT save
        }

        // Valid value
        bool wasInError = *inErrorState;
        *inErrorState  = false;
        *lastGoodValue = formatNumberForUI(enteredVal);

        // If previously in error → restore original style (NOT empty string)
        // Normal save → never touch style at all
        if (wasInError) {
            valueEdit->setStyleSheet(originalStyle);
        }

        QJsonObject delta;
        QJsonObject updatedParam = paramObj;
        updatedParam["value"] = enteredVal;
        if (fullKey.contains(".")) {
            QStringList parts = fullKey.split(".");
            delta[parts[0]] = QJsonObject{{parts[1], updatedParam}};
        } else {
            delta[fullKey] = updatedParam;
        }
        delta["_id"] = mainID;
        emit valueChanged(ConnectedID, Name, delta);
    });

    // ── FocusOut: revert to last good value if still in error ────────────
    class FocusWatcher : public QObject {
    public:
        std::shared_ptr<QString> lastGood;
        std::shared_ptr<bool>    errorState;
        WheelableLineEdit*       edit;
        QString                  origStyle;
        FocusWatcher(WheelableLineEdit* e,
                     std::shared_ptr<QString> lg,
                     std::shared_ptr<bool> es,
                     const QString &os,
                     QObject* parent)
            : QObject(parent), lastGood(lg), errorState(es), edit(e), origStyle(os) {}
    protected:
        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (obj == edit && ev->type() == QEvent::FocusOut) {
                if (*errorState) {
                    edit->blockSignals(true);
                    edit->setText(*lastGood);
                    edit->blockSignals(false);
                    edit->setStyleSheet(origStyle);  // restore original, not ""
                    *errorState = false;
                }
            }
            return QObject::eventFilter(obj, ev);
        }
    };

    FocusWatcher* watcher = new FocusWatcher(valueEdit, lastGoodValue, inErrorState, originalStyle, valueEdit);
    valueEdit->installEventFilter(watcher);
}

QWidget* Inspector::createSectionHeader(const QString &sectionKey, int headerRow, const QJsonObject &sectionObj)
{
    QWidget *headerWidget = new QWidget();
    if (!headerWidget) return nullptr;

    headerWidget->setStyleSheet(InspectorStyles::SectionHeader);

    QHBoxLayout *layout = new QHBoxLayout(headerWidget);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(10);

    QPushButton *dropdownButton = new QPushButton("▼", this);
    dropdownButton->setFixedSize(20, 20);
    dropdownButton->setStyleSheet(InspectorStyles::SectionHeaderButton);
    dropdownButton->setProperty("sectionKey", sectionKey);
    dropdownButton->setProperty("headerRow", headerRow);

    QLabel *sectionLabel = new QLabel(capitalizeFirstLetter(sectionKey));
    sectionLabel->setStyleSheet(InspectorStyles::SectionHeaderLabel);

    layout->addWidget(dropdownButton);
    layout->addWidget(sectionLabel);
    layout->addStretch();

    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        toggleSectionExpansion(sectionKey, headerRow, dropdownButton);
    });

    return headerWidget;
}

// toggleSectionExpansion
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

// addSectionToLayout
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
        checkBox->setStyleSheet(InspectorStyles::CheckBox);
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
        edit->setStyleSheet(InspectorStyles::LineEdit);
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
            valueEdit->setFixedWidth(80);

            QLabel *unitLabel = new QLabel(obj["unit"].toString());
            unitLabel->setStyleSheet(InspectorStyles::UnitParamLabel);

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

// createSubcomponentWidget
QWidget* Inspector::createSubcomponentWidget(const QString &subKey, const QJsonObject &subObj, const QString &parentComponentName)
{
    QWidget *subWidget = new QWidget();
    subWidget->setProperty("subComponentId", subObj["id"].toString());

    QVBoxLayout *subLayout = new QVBoxLayout(subWidget);
    subLayout->setContentsMargins(0, 0, 0, 0);
    subLayout->setSpacing(0);

    QString subName = subObj["name"].toString();

    QPushButton *subDropdownButton = new QPushButton("▼ " + subName);
    subDropdownButton->setStyleSheet(InspectorStyles::ContainerDropdownButton);
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

// addMultiComponentContainerRow
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



// =========================================================================
// REPLACE handleMultiComponentContainer in inspector.cpp with this version
// =========================================================================

// void Inspector::handleMultiComponentContainer(QString ID, QString name, QJsonObject object)
// {
//     ConnectedID = ID;
//     Name = name.toLower();
//     mainID = object.contains("id") ? object["id"].toString() : ID + "_" + name;

//     if (titleLabel) {
//         titleLabel->setText(name);
//     }
//     tableWidget->clearContents();
//     tableWidget->blockSignals(true);
//     rowToKeyPath.clear();
//     sectionInfo.clear();
//     sectionRows.clear();

//     QJsonObject containerObj;
//     QString containerKey = name.toLower();
//     if (object.contains(containerKey)) {
//         containerObj = object[containerKey].toObject();
//     }

//     tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
//     tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

//     bool developerMode = ApplicationDialog::getGlobalDeveloperMode();
//     int row = 0;
//     tableWidget->setRowCount(1);

//     if (object.contains("active")) {
//         rowToKeyPath[row] = "active";
//         QTableWidgetItem *activeKeyItem = new QTableWidgetItem("Active");
//         if (activeKeyItem) {
//             activeKeyItem->setFlags(Qt::ItemIsEnabled);
//             activeKeyItem->setBackground(QColor("#1A3652"));
//             activeKeyItem->setForeground(Qt::white);
//             tableWidget->setItem(row, 0, activeKeyItem);
//         }
//         setupBooleanCell(row, "active", object["active"].toBool());
//         row++;
//     }

//     tableWidget->setRowCount(row + 1);
//     QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(containerKey));
//     if (keyItem) {
//         keyItem->setFlags(Qt::ItemIsEnabled);
//         keyItem->setBackground(QColor("#1A3652"));
//         keyItem->setForeground(Qt::white);
//         tableWidget->setItem(row, 0, keyItem);
//     }
//     rowToKeyPath[row] = containerKey;

//     // ── Outer container widget ────────────────────────────────────────────
//     QWidget *containerWidget = new QWidget();
//     containerWidget->setStyleSheet(InspectorStyles::ContainerWidget);
//     QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
//     containerLayout->setContentsMargins(0, 0, 0, 0);
//     containerLayout->setSpacing(5);

//     int subCount = containerObj.keys().size();

//     QPushButton *dropdownButton = new QPushButton(
//         "▲ " + QString::number(subCount) + " " + capitalizeFirstLetter(containerKey));
//     dropdownButton->setStyleSheet(InspectorStyles::ContainerDropdownButton);
//     dropdownButton->setFixedHeight(30);

//     QScrollArea *scrollArea = new QScrollArea();
//     scrollArea->setStyleSheet(InspectorStyles::ScrollArea);
//     scrollArea->setWidgetResizable(true);
//     scrollArea->setFrameShape(QFrame::NoFrame);
//     scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
//     scrollArea->setVisible(true);

//     QWidget *subcomponentsWidget = new QWidget();
//     subcomponentsWidget->setStyleSheet("background-color: #0F2636;");
//     QVBoxLayout *subcomponentsLayout = new QVBoxLayout(subcomponentsWidget);
//     subcomponentsLayout->setContentsMargins(10, 10, 10, 10);
//     subcomponentsLayout->setSpacing(15);

//     int totalSubcomponentsHeight = 0;
//     int renderedSubCount = 0;


//     static const QSet<QString> SKIP_KEYS = {
//         "id", "name", "branch", "parent_id", "type", "parameters",
//         "active", "on", "Active", "SensorType", "radioType"
//     };
//     auto buildSectionWidget = [&](const QString &sectionName,
//                                   const QJsonObject &sectionObj,
//                                   const QString &subKey,
//                                   const QJsonObject &subObjRef)
//         -> QWidget*
//     {
//         // Collapsible wrapper
//         QWidget *wrapper = new QWidget();
//         wrapper->setStyleSheet("background: transparent;");
//         QVBoxLayout *wl = new QVBoxLayout(wrapper);
//         wl->setContentsMargins(0, 4, 0, 0);
//         wl->setSpacing(0);

//         // Section header button
//         QString headerStyle = R"(
//             QPushButton {
//                 text-align: left;
//                 padding: 4px 8px;
//                 background-color: #162F47;
//                 border: none;
//                 border-left: 3px solid #0078D4;
//                 color: #E0E0E0;
//                 font-weight: 600;
//                 font-size: 11px;
//                 border-radius: 0px;
//             }
//             QPushButton:hover { background-color: #1A3652; }
//         )";

//         QPushButton *hdrBtn = new QPushButton("▼  " + capitalizeFirstLetter(sectionName));
//         hdrBtn->setStyleSheet(headerStyle);
//         hdrBtn->setFixedHeight(26);

//         // Content area (expanded by default)
//         QWidget *content = new QWidget();
//         content->setStyleSheet("background: transparent;");
//         QVBoxLayout *cl = new QVBoxLayout(content);
//         cl->setContentsMargins(10, 4, 4, 4);
//         cl->setSpacing(4);
//         content->setVisible(true);

//         // ── Populate section rows ─────────────────────────────────────────
//         for (const QString &paramKey : sectionObj.keys()) {
//             if (paramKey == "type") continue;

//             QJsonValue pval = sectionObj[paramKey];

//             // Build a nice display label (insert spaces before capitals)
//             QString displayKey = paramKey;
//             for (int i = 1; i < displayKey.length(); i++) {
//                 if (displayKey[i].isUpper() && !displayKey[i-1].isSpace()) {
//                     displayKey.insert(i, " ");
//                     i++;
//                 }
//             }

//             QWidget *rowW = new QWidget();
//             rowW->setStyleSheet("background: transparent;");
//             QHBoxLayout *rl = new QHBoxLayout(rowW);
//             rl->setContentsMargins(0, 0, 0, 0);
//             rl->setSpacing(8);

//             QLabel *lbl = new QLabel(capitalizeFirstLetter(displayKey) + ":");
//             lbl->setStyleSheet("color:#B0B0B0; font-size:11px; min-width:130px; background:transparent;");
//             lbl->setFixedWidth(130);
//             rl->addWidget(lbl);

//             if (pval.isObject()) {
//                 QJsonObject po = pval.toObject();
//                 QString ptype = po["type"].toString();

//                 if (ptype == "unitParam") {
//                     // ── unitParam ─────────────────────────────────────────
//                     QWidget *uw = new QWidget();
//                     uw->setStyleSheet("background:transparent;");
//                     QHBoxLayout *ul = new QHBoxLayout(uw);
//                     ul->setContentsMargins(0,0,0,0); ul->setSpacing(4);

//                     WheelableLineEdit *ve = new WheelableLineEdit();
//                     ve->setText(formatNumberForUI(po["value"].toDouble()));
//                     ve->setFixedWidth(90);
//                     QDoubleValidator *dv = new QDoubleValidator(ve);
//                     dv->setNotation(QDoubleValidator::StandardNotation);
//                     dv->setDecimals(4);
//                     ve->setValidator(dv);

//                     QLabel *unitLbl = new QLabel(po["unit"].toString());
//                     unitLbl->setStyleSheet("color:#B0B0B0; font-size:11px; background:transparent;");

//                     ul->addWidget(ve); ul->addWidget(unitLbl); ul->addStretch();
//                     rl->addWidget(uw);

//                     // Capture by value (sectionObj / subKey at this point in loop)
//                     QString captSectionName = sectionName;
//                     QString captParamKey    = paramKey;
//                     QString captSubKey      = subKey;
//                     QJsonObject captSubObj  = subObjRef;
//                     QJsonObject captSectionObj = sectionObj;
//                     QJsonObject captPo = po;

//                     connect(ve, &QLineEdit::editingFinished, this, [=]() {
//                         QJsonObject delta;
//                         delta["_id"] = mainID;
//                         QJsonObject updSection = captSectionObj;
//                         QJsonObject updParam   = captPo;
//                         updParam["value"]      = ve->text().toDouble();
//                         updSection[captParamKey] = updParam;
//                         QJsonObject updSub    = captSubObj;
//                         updSub[captSectionName] = updSection;
//                         delta[containerKey]   = QJsonObject{{captSubKey, updSub}};
//                         emit valueChanged(ConnectedID, Name, delta);
//                     });

//                 } else if (ptype == "option") {
//                     // ── option ────────────────────────────────────────────
//                     QComboBox *cb = new QComboBox();
//                     cb->setStyleSheet(InspectorStyles::OptionComboBox);
//                     QJsonArray opts = po["options"].toArray();
//                     for (const QJsonValue &o : opts) cb->addItem(o.toString());
//                     cb->setCurrentText(po["value"].toString());
//                     rl->addWidget(cb);

//                     QString captSectionName   = sectionName;
//                     QString captParamKey      = paramKey;
//                     QString captSubKey        = subKey;
//                     QJsonObject captSubObj    = subObjRef;
//                     QJsonObject captSectionObj = sectionObj;
//                     QJsonObject captPo        = po;

//                     connect(cb, &QComboBox::currentTextChanged, this, [=](const QString &text) {
//                         QJsonObject delta;
//                         delta["_id"] = mainID;
//                         QJsonObject updSection = captSectionObj;
//                         QJsonObject updOpt     = captPo;
//                         updOpt["value"]        = text;
//                         updSection[captParamKey] = updOpt;
//                         QJsonObject updSub    = captSubObj;
//                         updSub[captSectionName] = updSection;
//                         delta[containerKey]   = QJsonObject{{captSubKey, updSub}};
//                         emit valueChanged(ConnectedID, Name, delta);
//                     });

//                 } else {
//                     QLabel *vl = new QLabel(QString(QJsonDocument(po).toJson(QJsonDocument::Compact)));
//                     vl->setStyleSheet("color:#E0E0E0; font-size:11px; background:transparent;");
//                     rl->addWidget(vl);
//                 }

//             } else if (pval.isBool()) {
//                 // ── bool ──────────────────────────────────────────────────
//                 QCheckBox *cb = new QCheckBox();
//                 cb->setChecked(pval.toBool());
//                 cb->setStyleSheet(InspectorStyles::SensorCheckBox);
//                 rl->addWidget(cb);

//                 QString captSectionName   = sectionName;
//                 QString captParamKey      = paramKey;
//                 QString captSubKey        = subKey;
//                 QJsonObject captSubObj    = subObjRef;
//                 QJsonObject captSectionObj = sectionObj;

//                 connect(cb, &QCheckBox::toggled, this, [=](bool checked) {
//                     QJsonObject delta;
//                     delta["_id"] = mainID;
//                     QJsonObject updSection = captSectionObj;
//                     updSection[captParamKey] = checked;
//                     QJsonObject updSub = captSubObj;
//                     updSub[captSectionName] = updSection;
//                     delta[containerKey] = QJsonObject{{captSubKey, updSub}};
//                     emit valueChanged(ConnectedID, Name, delta);
//                 });

//             } else if (pval.isDouble()) {
//                 // ── plain number ──────────────────────────────────────────
//                 WheelableLineEdit *ne = new WheelableLineEdit();
//                 ne->setText(formatNumberForUI(pval.toDouble()));
//                 ne->setFixedWidth(90);
//                 rl->addWidget(ne);

//                 QString captSectionName   = sectionName;
//                 QString captParamKey      = paramKey;
//                 QString captSubKey        = subKey;
//                 QJsonObject captSubObj    = subObjRef;
//                 QJsonObject captSectionObj = sectionObj;

//                 connect(ne, &QLineEdit::editingFinished, this, [=]() {
//                     QJsonObject delta;
//                     delta["_id"] = mainID;
//                     QJsonObject updSection = captSectionObj;
//                     updSection[captParamKey] = ne->text().toDouble();
//                     QJsonObject updSub = captSubObj;
//                     updSub[captSectionName] = updSection;
//                     delta[containerKey] = QJsonObject{{captSubKey, updSub}};
//                     emit valueChanged(ConnectedID, Name, delta);
//                 });

//             } else if (pval.isString()) {
//                 // ── string ────────────────────────────────────────────────
//                 QLineEdit *se = new QLineEdit(pval.toString());
//                 se->setStyleSheet(InspectorStyles::LineEdit);
//                 se->setFixedWidth(150);
//                 rl->addWidget(se);

//                 QString captSectionName   = sectionName;
//                 QString captParamKey      = paramKey;
//                 QString captSubKey        = subKey;
//                 QJsonObject captSubObj    = subObjRef;
//                 QJsonObject captSectionObj = sectionObj;

//                 connect(se, &QLineEdit::editingFinished, this, [=]() {
//                     QJsonObject delta;
//                     delta["_id"] = mainID;
//                     QJsonObject updSection = captSectionObj;
//                     updSection[captParamKey] = se->text();
//                     QJsonObject updSub = captSubObj;
//                     updSub[captSectionName] = updSection;
//                     delta[containerKey] = QJsonObject{{captSubKey, updSub}};
//                     emit valueChanged(ConnectedID, Name, delta);
//                 });
//             }

//             rl->addStretch();
//             cl->addWidget(rowW);
//         }

//         wl->addWidget(hdrBtn);
//         wl->addWidget(content);

//         // Toggle collapse / expand
//         connect(hdrBtn, &QPushButton::clicked, this, [hdrBtn, content]() {
//             bool vis = !content->isVisible();
//             content->setVisible(vis);
//             QString txt = hdrBtn->text().mid(4); // strip "▼  " or "▶  "
//             hdrBtn->setText(vis ? "▼  " + txt : "▶  " + txt);
//         });

//         return wrapper;
//     };

//     // ── Iterate over sub-components ───────────────────────────────────────
//     for (const QString &subKey : containerObj.keys()) {
//         QJsonObject subObj = containerObj[subKey].toObject();

//         QString subComponentId = subObj.contains("id") ? subObj["id"].toString() : subKey;
//         QString subName = subObj.contains("name") ? subObj["name"].toString()
//                         : (subObj.contains("Name") ? subObj["Name"].toString()
//                                                    : capitalizeFirstLetter(subKey));

//         // ── GroupBox per sub-component ────────────────────────────────────
//         QGroupBox *subGroupBox = new QGroupBox(subName);
//         subGroupBox->setProperty("subcomponentId", subComponentId);
//         subGroupBox->setStyleSheet(InspectorStyles::SubcomponentGroupBox);
//         QVBoxLayout *subLayout = new QVBoxLayout(subGroupBox);
//         subLayout->setContentsMargins(15, 20, 15, 15);
//         subLayout->setSpacing(8);

//         int itemCount = 0;

//         // ── Developer-mode: show ID ───────────────────────────────────────
//         if (subObj.contains("id") && developerMode) {
//             QWidget *idW = new QWidget(); idW->setStyleSheet("background:transparent;");
//             QHBoxLayout *il = new QHBoxLayout(idW); il->setContentsMargins(0,0,0,0);
//             QLabel *il2 = new QLabel("ID:"); il2->setStyleSheet(InspectorStyles::SensorIdLabel); il2->setFixedWidth(130);
//             QLineEdit *ie = new QLineEdit(subObj["id"].toString());
//             ie->setStyleSheet(InspectorStyles::ReadOnlyLineEdit); ie->setReadOnly(true); ie->setFixedWidth(200);
//             il->addWidget(il2); il->addWidget(ie); il->addStretch();
//             subLayout->addWidget(idW);
//             itemCount++;
//         }

//         // ── active ───────────────────────────────────────────────────────
//         if (subObj.contains("active")) {
//             QWidget *aw = new QWidget(); aw->setStyleSheet("background:transparent;");
//             QHBoxLayout *al = new QHBoxLayout(aw); al->setContentsMargins(0,0,0,0);
//             QLabel *al2 = new QLabel("Active:"); al2->setStyleSheet(InspectorStyles::SensorIdLabel); al2->setFixedWidth(130);
//             QCheckBox *ac = new QCheckBox(); ac->setChecked(subObj["active"].toBool());
//             ac->setStyleSheet(InspectorStyles::SensorCheckBox);
//             connect(ac, &QCheckBox::toggled, this, [=](bool checked) {
//                 QJsonObject delta; delta["_id"] = mainID;
//                 QJsonObject updSub = subObj; updSub["active"] = checked;
//                 delta[containerKey] = QJsonObject{{subKey, updSub}};
//                 emit valueChanged(ConnectedID, Name, delta);
//             });
//             al->addWidget(al2); al->addWidget(ac); al->addStretch();
//             subLayout->addWidget(aw);
//             itemCount++;
//         }

//         // ── on (ESM / CSM) ───────────────────────────────────────────────
//         if (subObj.contains("on")) {
//             QWidget *ow = new QWidget(); ow->setStyleSheet("background:transparent;");
//             QHBoxLayout *ol = new QHBoxLayout(ow); ol->setContentsMargins(0,0,0,0);
//             QLabel *ol2 = new QLabel("On:"); ol2->setStyleSheet(InspectorStyles::SensorIdLabel); ol2->setFixedWidth(130);
//             QCheckBox *oc = new QCheckBox(); oc->setChecked(subObj["on"].toBool());
//             oc->setStyleSheet(InspectorStyles::SensorCheckBox);
//             connect(oc, &QCheckBox::toggled, this, [=](bool checked) {
//                 QJsonObject delta; delta["_id"] = mainID;
//                 QJsonObject updSub = subObj; updSub["on"] = checked;
//                 delta[containerKey] = QJsonObject{{subKey, updSub}};
//                 emit valueChanged(ConnectedID, Name, delta);
//             });
//             ol->addWidget(ol2); ol->addWidget(oc); ol->addStretch();
//             subLayout->addWidget(ow);
//             itemCount++;
//         }

//         // ── Active (capital A, sensors) ───────────────────────────────────
//         if (subObj.contains("Active") && !subObj.contains("active")) {
//             QWidget *aw = new QWidget(); aw->setStyleSheet("background:transparent;");
//             QHBoxLayout *al = new QHBoxLayout(aw); al->setContentsMargins(0,0,0,0);
//             QLabel *al2 = new QLabel("Active:"); al2->setStyleSheet(InspectorStyles::SensorIdLabel); al2->setFixedWidth(130);
//             QCheckBox *ac = new QCheckBox(); ac->setChecked(subObj["Active"].toBool());
//             ac->setStyleSheet(InspectorStyles::SensorCheckBox);
//             connect(ac, &QCheckBox::toggled, this, [=](bool checked) {
//                 QJsonObject delta; delta["_id"] = mainID;
//                 QJsonObject updSub = subObj; updSub["Active"] = checked;
//                 delta[containerKey] = QJsonObject{{subKey, updSub}};
//                 emit valueChanged(ConnectedID, Name, delta);
//             });
//             al->addWidget(al2); al->addWidget(ac); al->addStretch();
//             subLayout->addWidget(aw);
//             itemCount++;
//         }

//         // ── SensorType / radioType labels ─────────────────────────────────
//         auto addTypeLabel = [&](const QString &jsonKey, const QString &displayName) {
//             if (!subObj.contains(jsonKey)) return;
//             QWidget *tw = new QWidget(); tw->setStyleSheet("background:transparent;");
//             QHBoxLayout *tl = new QHBoxLayout(tw); tl->setContentsMargins(0,0,0,0);
//             QLabel *tl2 = new QLabel(displayName + ":"); tl2->setStyleSheet(InspectorStyles::SensorIdLabel); tl2->setFixedWidth(130);
//             QLabel *tv = new QLabel(subObj[jsonKey].toString()); tv->setStyleSheet(InspectorStyles::SensorValueLabel);
//             tl->addWidget(tl2); tl->addWidget(tv); tl->addStretch();
//             subLayout->addWidget(tw);
//             itemCount++;
//         };
//         addTypeLabel("SensorType", "Sensor Type");
//         addTypeLabel("radioType",  "Radio Type");

//         for (const QString &propKey : subObj.keys()) {
//             if (SKIP_KEYS.contains(propKey)) continue;

//             QJsonValue pval = subObj[propKey];
//             if (!pval.isObject()) continue;

//             QJsonObject pobj = pval.toObject();
//             QString ptype = pobj["type"].toString();

//             // Accept both "Section" and "section"
//             if (ptype.compare("Section", Qt::CaseInsensitive) != 0) continue;

//             QWidget *secW = buildSectionWidget(propKey, pobj, subKey, subObj);
//             if (secW) {
//                 subLayout->addWidget(secW);
//                 itemCount++;
//             }
//         }

//         subLayout->addStretch();
//         subcomponentsLayout->addWidget(subGroupBox);
//         renderedSubCount++;

//         // Rough height estimate for scroll-area sizing
//         int subH = 60 + (itemCount * 30) + 20;
//         totalSubcomponentsHeight += subH;
//     }

//     if (renderedSubCount == 0) {
//         QLabel *emptyLabel = new QLabel("No " + containerKey + " configured");
//         emptyLabel->setAlignment(Qt::AlignCenter);
//         emptyLabel->setStyleSheet(InspectorStyles::EmptyLabel);
//         subcomponentsLayout->addWidget(emptyLabel);
//         totalSubcomponentsHeight += 60;
//     }

//     subcomponentsLayout->addStretch();
//     scrollArea->setWidget(subcomponentsWidget);

//     const int SCROLL_THRESHOLD = 650;
//     int requiredHeight = totalSubcomponentsHeight + 50;

//     if (requiredHeight > SCROLL_THRESHOLD) {
//         scrollArea->setMinimumHeight(SCROLL_THRESHOLD);
//         scrollArea->setMaximumHeight(SCROLL_THRESHOLD);
//     } else {
//         scrollArea->setMinimumHeight(qMax(requiredHeight, 80));
//         scrollArea->setMaximumHeight(qMax(requiredHeight, 80));
//     }
//     scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

//     connect(dropdownButton, &QPushButton::clicked, this, [=]() {
//         bool isVisible = !scrollArea->isVisible();
//         scrollArea->setVisible(isVisible);
//         if (isVisible) {
//             dropdownButton->setText("▲ " + QString::number(renderedSubCount)
//                                     + " " + capitalizeFirstLetter(containerKey));
//             int contentHeight = (requiredHeight < SCROLL_THRESHOLD)
//                                     ? requiredHeight + 40
//                                     : SCROLL_THRESHOLD + 40;
//             tableWidget->setRowHeight(row, contentHeight);
//         } else {
//             dropdownButton->setText("▼ " + QString::number(renderedSubCount)
//                                     + " " + capitalizeFirstLetter(containerKey));
//             tableWidget->setRowHeight(row, 35);
//         }
//         tableWidget->viewport()->update();
//         containerWidget->adjustSize();
//     });

//     containerLayout->addWidget(dropdownButton);
//     containerLayout->addWidget(scrollArea);
//     tableWidget->setCellWidget(row, 1, containerWidget);
//     tableWidget->setRowHeight(row, 35);
//     tableWidget->blockSignals(false);
//     tableWidget->resizeRowsToContents();

//     if (tableWidget->rowCount() > 0) {
//         tableWidget->scrollToTop();
//     }
//     tableWidget->viewport()->update();
// }

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

#ifndef GUI_UNIT_TEST
    bool developerMode = ApplicationDialog::getGlobalDeveloperMode();
#else
    bool developerMode = false;
#endif
    int row = 0;
    tableWidget->setRowCount(1);

    if (object.contains("active")) {
        rowToKeyPath[row] = "active";
        QTableWidgetItem *activeKeyItem = new QTableWidgetItem("Active");
        if (activeKeyItem) {
            activeKeyItem->setFlags(Qt::ItemIsEnabled);
            activeKeyItem->setBackground(QColor("#1A3652"));
            activeKeyItem->setForeground(Qt::white);
            tableWidget->setItem(row, 0, activeKeyItem);
        }
        setupBooleanCell(row, "active", object["active"].toBool());
        row++;
    }

    tableWidget->setRowCount(row + 1);
    QTableWidgetItem *keyItem = new QTableWidgetItem(capitalizeFirstLetter(containerKey));
    if (keyItem) {
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setBackground(QColor("#1A3652"));
        keyItem->setForeground(Qt::white);
        tableWidget->setItem(row, 0, keyItem);
    }
    rowToKeyPath[row] = containerKey;

    QWidget *containerWidget = new QWidget();
    containerWidget->setStyleSheet(InspectorStyles::ContainerWidget);
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(5);

    int subCount = containerObj.keys().size();

    QPushButton *dropdownButton = new QPushButton(
        "▲ " + QString::number(subCount) + " " + capitalizeFirstLetter(containerKey));
    dropdownButton->setStyleSheet(InspectorStyles::ContainerDropdownButton);
    dropdownButton->setFixedHeight(30);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setStyleSheet(InspectorStyles::ScrollArea);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVisible(true);

    QWidget *subcomponentsWidget = new QWidget();
    subcomponentsWidget->setStyleSheet("background-color: #0F2636;");
    QVBoxLayout *subcomponentsLayout = new QVBoxLayout(subcomponentsWidget);
    subcomponentsLayout->setContentsMargins(10, 10, 10, 10);
    subcomponentsLayout->setSpacing(15);

    int totalSubcomponentsHeight = 0;
    int renderedSubCount = 0;

    static const QSet<QString> SKIP_KEYS = {
        "id", "name", "branch", "parent_id", "type", "parameters",
        "active", "on", "Active", "SensorType", "radioType"
    };

    auto buildSectionWidget = [&](const QString &sectionName,
                                  const QJsonObject &sectionObj,
                                  const QString &subKey,
                                  const QJsonObject &subObjRef)
        -> QWidget*
    {
        QWidget *wrapper = new QWidget();
        wrapper->setStyleSheet("background: transparent;");
        QVBoxLayout *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(0, 4, 0, 0);
        wl->setSpacing(0);

        QString headerStyle = R"(
            QPushButton {
                text-align: left;
                padding: 4px 8px;
                background-color: #162F47;
                border: none;
                border-left: 3px solid #0078D4;
                color: #E0E0E0;
                font-weight: 600;
                font-size: 11px;
                border-radius: 0px;
            }
            QPushButton:hover { background-color: #1A3652; }
        )";

        QPushButton *hdrBtn = new QPushButton("▼  " + capitalizeFirstLetter(sectionName));
        hdrBtn->setStyleSheet(headerStyle);
        hdrBtn->setFixedHeight(26);

        QWidget *content = new QWidget();
        content->setStyleSheet("background: transparent;");
        QVBoxLayout *cl = new QVBoxLayout(content);
        cl->setContentsMargins(10, 4, 4, 4);
        cl->setSpacing(4);
        content->setVisible(true);

        for (const QString &paramKey : sectionObj.keys()) {
            if (paramKey == "type") continue;

            QJsonValue pval = sectionObj[paramKey];

            QString displayKey = paramKey;
            for (int i = 1; i < displayKey.length(); i++) {
                if (displayKey[i].isUpper() && !displayKey[i-1].isSpace()) {
                    displayKey.insert(i, " ");
                    i++;
                }
            }

            QWidget *rowW = new QWidget();
            rowW->setStyleSheet("background: transparent;");
            QHBoxLayout *rl = new QHBoxLayout(rowW);
            rl->setContentsMargins(0, 0, 0, 0);
            rl->setSpacing(8);

            QLabel *lbl = new QLabel(capitalizeFirstLetter(displayKey) + ":");
            lbl->setStyleSheet("color:#B0B0B0; font-size:11px; min-width:130px; background:transparent;");
            lbl->setFixedWidth(130);
            rl->addWidget(lbl);

            if (pval.isObject()) {
                QJsonObject po = pval.toObject();
                QString ptype = po["type"].toString();

                if (ptype == "unitParam") {
                    QWidget *uw = new QWidget();
                    uw->setStyleSheet("background:transparent;");
                    QHBoxLayout *ul = new QHBoxLayout(uw);
                    ul->setContentsMargins(0,0,0,0); ul->setSpacing(4);

                    WheelableLineEdit *ve = new WheelableLineEdit();
                    QString initVal = formatNumberForUI(po["value"].toDouble());
                    ve->setText(initVal);
                    // NO setToolTip on ve
                    QDoubleValidator *dv = new QDoubleValidator(ve);
                    dv->setNotation(QDoubleValidator::StandardNotation);
                    dv->setDecimals(6);
                    ve->setValidator(dv);

                    // Save original style before any error styling
                    const QString origStyle = ve->styleSheet();

                    QLabel *unitLbl = new QLabel(po["unit"].toString());
                    unitLbl->setStyleSheet("color:#B0B0B0; font-size:11px; background:transparent;");
                    unitLbl->setFixedWidth(60);

                    ul->addWidget(ve); ul->addWidget(unitLbl); ul->addStretch();
                    rl->addWidget(uw);

                    // Tooltip on key label ONLY
                    {
                        QString desc = po["description"].toString();
                        double  mn   = po["min"].toDouble(0.0);
                        double  mx   = po["max"].toDouble(0.0);
                        bool    hasR = !(qFuzzyIsNull(mn) && qFuzzyIsNull(mx));
                        QString tip;
                        if (!desc.isEmpty()) tip += desc;
                        if (hasR) {
                            if (!tip.isEmpty()) tip += "\n";
                            tip += QString("Range: %1 – %2 %3")
                                       .arg(formatNumberForUI(mn))
                                       .arg(formatNumberForUI(mx))
                                       .arg(po["unit"].toString());
                        }
                        if (!tip.isEmpty())
                            lbl->setToolTip(tip);
                    }

                    // Shared state
                    auto lastGoodVal    = std::make_shared<QString>(initVal);
                    auto inErrState     = std::make_shared<bool>(false);

                    double captMin      = po["min"].toDouble(0.0);
                    double captMax      = po["max"].toDouble(0.0);
                    bool   captHasRange = !(qFuzzyIsNull(captMin) && qFuzzyIsNull(captMax));
                    QString captUnit    = po["unit"].toString();

                    QString captSectionName    = sectionName;
                    QString captParamKey       = paramKey;
                    QString captSubKey         = subKey;
                    QJsonObject captSubObj     = subObjRef;
                    QJsonObject captSectionObj = sectionObj;
                    QJsonObject captPo         = po;

                    connect(ve, &QLineEdit::editingFinished, this, [=]() {
                        double enteredVal = ve->text().toDouble();

                        if (captHasRange && (enteredVal < captMin || enteredVal > captMax)) {
                            *inErrState = true;
                            ve->setStyleSheet(
                                "QLineEdit { border: 1px solid #e74c3c; "
                                "background-color: #2d0a0a; color: #e74c3c; padding-left: 4px; }");
                            QString errMsg = QString("⚠ Value out of range!\nAllowed: %1 – %2 %3\nEntered: %4")
                                                 .arg(formatNumberForUI(captMin))
                                                 .arg(formatNumberForUI(captMax))
                                                 .arg(captUnit)
                                                 .arg(formatNumberForUI(enteredVal));
                            QToolTip::showText(QCursor::pos(), errMsg, ve);
                            return;
                        }

                        // Valid value
                        bool wasInError = *inErrState;
                        *inErrState   = false;
                        *lastGoodVal  = formatNumberForUI(enteredVal);

                        // Restore original style ONLY if we were in error
                        // Normal save → style never touched
                        if (wasInError) {
                            ve->setStyleSheet(origStyle);
                        }

                        QJsonObject delta;
                        delta["_id"] = mainID;
                        QJsonObject updSection   = captSectionObj;
                        QJsonObject updParam     = captPo;
                        updParam["value"]        = enteredVal;
                        updSection[captParamKey] = updParam;
                        QJsonObject updSub       = captSubObj;
                        updSub[captSectionName]  = updSection;
                        delta[containerKey]      = QJsonObject{{captSubKey, updSub}};
                        emit valueChanged(ConnectedID, Name, delta);
                    });

                    // FocusOut: revert if still in error
                    class FocusWatcher : public QObject {
                    public:
                        std::shared_ptr<QString> lastGood;
                        std::shared_ptr<bool>    errorState;
                        WheelableLineEdit*       edit;
                        QString                  origSt;
                        FocusWatcher(WheelableLineEdit* e,
                                     std::shared_ptr<QString> lg,
                                     std::shared_ptr<bool> es,
                                     const QString &os,
                                     QObject* parent)
                            : QObject(parent), lastGood(lg), errorState(es), edit(e), origSt(os) {}
                    protected:
                        bool eventFilter(QObject* obj, QEvent* ev) override {
                            if (obj == edit && ev->type() == QEvent::FocusOut) {
                                if (*errorState) {
                                    edit->blockSignals(true);
                                    edit->setText(*lastGood);
                                    edit->blockSignals(false);
                                    edit->setStyleSheet(origSt); // restore original, not ""
                                    *errorState = false;
                                }
                            }
                            return QObject::eventFilter(obj, ev);
                        }
                    };

                    FocusWatcher* watcher = new FocusWatcher(ve, lastGoodVal, inErrState, origStyle, ve);
                    ve->installEventFilter(watcher);

                } else if (ptype == "option") {
                    QComboBox *cb = new QComboBox();
                    cb->setStyleSheet(InspectorStyles::OptionComboBox);
                    QJsonArray opts = po["options"].toArray();
                    for (const QJsonValue &o : opts) cb->addItem(o.toString());
                    cb->setCurrentText(po["value"].toString());
                    cb->view()->setAlternatingRowColors(false);
                    cb->view()->setStyleSheet("background-color: #1A3652; color: white;");
                    rl->addWidget(cb);

                    QString captSectionName    = sectionName;
                    QString captParamKey       = paramKey;
                    QString captSubKey         = subKey;
                    QJsonObject captSubObj     = subObjRef;
                    QJsonObject captSectionObj = sectionObj;
                    QJsonObject captPo         = po;

                    connect(cb, &QComboBox::currentTextChanged, this, [=](const QString &text) {
                        QJsonObject delta;
                        delta["_id"] = mainID;
                        QJsonObject updSection   = captSectionObj;
                        QJsonObject updOpt       = captPo;
                        updOpt["value"]          = text;
                        updSection[captParamKey] = updOpt;
                        QJsonObject updSub       = captSubObj;
                        updSub[captSectionName]  = updSection;
                        delta[containerKey]      = QJsonObject{{captSubKey, updSub}};
                        emit valueChanged(ConnectedID, Name, delta);
                    });

                } else {
                    QLabel *vl = new QLabel(QString(QJsonDocument(po).toJson(QJsonDocument::Compact)));
                    vl->setStyleSheet("color:#E0E0E0; font-size:11px; background:transparent;");
                    rl->addWidget(vl);
                }

            } else if (pval.isBool()) {
                QCheckBox *cb = new QCheckBox();
                cb->setChecked(pval.toBool());
                cb->setStyleSheet(InspectorStyles::SensorCheckBox);
                rl->addWidget(cb);

                QString captSectionName    = sectionName;
                QString captParamKey       = paramKey;
                QString captSubKey         = subKey;
                QJsonObject captSubObj     = subObjRef;
                QJsonObject captSectionObj = sectionObj;

                connect(cb, &QCheckBox::toggled, this, [=](bool checked) {
                    QJsonObject delta;
                    delta["_id"] = mainID;
                    QJsonObject updSection   = captSectionObj;
                    updSection[captParamKey] = checked;
                    QJsonObject updSub       = captSubObj;
                    updSub[captSectionName]  = updSection;
                    delta[containerKey]      = QJsonObject{{captSubKey, updSub}};
                    emit valueChanged(ConnectedID, Name, delta);
                });

            } else if (pval.isDouble()) {
                WheelableLineEdit *ne = new WheelableLineEdit();
                ne->setText(formatNumberForUI(pval.toDouble()));
                ne->setFixedWidth(90);
                rl->addWidget(ne);

                QString captSectionName    = sectionName;
                QString captParamKey       = paramKey;
                QString captSubKey         = subKey;
                QJsonObject captSubObj     = subObjRef;
                QJsonObject captSectionObj = sectionObj;

                connect(ne, &QLineEdit::editingFinished, this, [=]() {
                    QJsonObject delta;
                    delta["_id"] = mainID;
                    QJsonObject updSection   = captSectionObj;
                    updSection[captParamKey] = ne->text().toDouble();
                    QJsonObject updSub       = captSubObj;
                    updSub[captSectionName]  = updSection;
                    delta[containerKey]      = QJsonObject{{captSubKey, updSub}};
                    emit valueChanged(ConnectedID, Name, delta);
                });

            } else if (pval.isString()) {
                QLineEdit *se = new QLineEdit(pval.toString());
                se->setStyleSheet(InspectorStyles::LineEdit);
                se->setFixedWidth(150);
                rl->addWidget(se);

                QString captSectionName    = sectionName;
                QString captParamKey       = paramKey;
                QString captSubKey         = subKey;
                QJsonObject captSubObj     = subObjRef;
                QJsonObject captSectionObj = sectionObj;

                connect(se, &QLineEdit::editingFinished, this, [=]() {
                    QJsonObject delta;
                    delta["_id"] = mainID;
                    QJsonObject updSection   = captSectionObj;
                    updSection[captParamKey] = se->text();
                    QJsonObject updSub       = captSubObj;
                    updSub[captSectionName]  = updSection;
                    delta[containerKey]      = QJsonObject{{captSubKey, updSub}};
                    emit valueChanged(ConnectedID, Name, delta);
                });
            }

            rl->addStretch();
            cl->addWidget(rowW);
        }

        wl->addWidget(hdrBtn);
        wl->addWidget(content);

        // connect(hdrBtn, &QPushButton::clicked, this, [hdrBtn, content]() {
        //     bool vis = !content->isVisible();
        //     content->setVisible(vis);
        //     QString txt = hdrBtn->text().mid(4);
        //     hdrBtn->setText(vis ? "▼  " + txt : "▶  " + txt);
        // });
        connect(hdrBtn, &QPushButton::clicked, this, [hdrBtn, content, sectionName]() {
            bool vis = !content->isVisible();
            content->setVisible(vis);
            QString label = capitalizeFirstLetter(sectionName);
            hdrBtn->setText(vis ? "▼  " + label : "▶  " + label);
        });

        return wrapper;
    };

    // ── Iterate sub-components ────────────────────────────────────────────
    for (const QString &subKey : containerObj.keys()) {
        QJsonObject subObj = containerObj[subKey].toObject();

        QString subComponentId = subObj.contains("id") ? subObj["id"].toString() : subKey;
        QString subName = subObj.contains("name") ? subObj["name"].toString()
                                                  : (subObj.contains("Name") ? subObj["Name"].toString()
                                                                             : capitalizeFirstLetter(subKey));
        QGroupBox *subGroupBox = new QGroupBox(subName);
        subGroupBox->setProperty("subcomponentId", subComponentId);
        subGroupBox->setStyleSheet(InspectorStyles::SubcomponentGroupBox);
        QVBoxLayout *subLayout = new QVBoxLayout(subGroupBox);
        subLayout->setContentsMargins(15, 20, 15, 15);
        subLayout->setSpacing(8);

        int itemCount = 0;
        if (subObj.contains("id") && developerMode) {
            QWidget *idW = new QWidget(); idW->setStyleSheet("background:transparent;");
            QHBoxLayout *il = new QHBoxLayout(idW); il->setContentsMargins(0,0,0,0);
            QLabel *il2 = new QLabel("ID:"); il2->setStyleSheet(InspectorStyles::SensorIdLabel); il2->setFixedWidth(130);
            QLineEdit *ie = new QLineEdit(subObj["id"].toString());
            ie->setStyleSheet(InspectorStyles::ReadOnlyLineEdit); ie->setReadOnly(true); ie->setFixedWidth(200);
            il->addWidget(il2); il->addWidget(ie); il->addStretch();
            subLayout->addWidget(idW);
            itemCount++;
        }

        if (subObj.contains("active")) {
            QWidget *aw = new QWidget(); aw->setStyleSheet("background:transparent;");
            QHBoxLayout *al = new QHBoxLayout(aw); al->setContentsMargins(0,0,0,0);
            QLabel *al2 = new QLabel("Active:"); al2->setStyleSheet(InspectorStyles::SensorIdLabel); al2->setFixedWidth(130);
            QCheckBox *ac = new QCheckBox(); ac->setChecked(subObj["active"].toBool());
            ac->setStyleSheet(InspectorStyles::SensorCheckBox);
            connect(ac, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta; delta["_id"] = mainID;
                QJsonObject updSub = subObj; updSub["active"] = checked;
                delta[containerKey] = QJsonObject{{subKey, updSub}};
                emit valueChanged(ConnectedID, Name, delta);
            });
            al->addWidget(al2); al->addWidget(ac); al->addStretch();
            subLayout->addWidget(aw);
            itemCount++;
        }

        if (subObj.contains("on")) {
            QWidget *ow = new QWidget(); ow->setStyleSheet("background:transparent;");
            QHBoxLayout *ol = new QHBoxLayout(ow); ol->setContentsMargins(0,0,0,0);
            QLabel *ol2 = new QLabel("On:"); ol2->setStyleSheet(InspectorStyles::SensorIdLabel); ol2->setFixedWidth(130);
            QCheckBox *oc = new QCheckBox(); oc->setChecked(subObj["on"].toBool());
            oc->setStyleSheet(InspectorStyles::SensorCheckBox);
            connect(oc, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta; delta["_id"] = mainID;
                QJsonObject updSub = subObj; updSub["on"] = checked;
                delta[containerKey] = QJsonObject{{subKey, updSub}};
                emit valueChanged(ConnectedID, Name, delta);
            });
            ol->addWidget(ol2); ol->addWidget(oc); ol->addStretch();
            subLayout->addWidget(ow);
            itemCount++;
        }

        if (subObj.contains("Active") && !subObj.contains("active")) {
            QWidget *aw = new QWidget(); aw->setStyleSheet("background:transparent;");
            QHBoxLayout *al = new QHBoxLayout(aw); al->setContentsMargins(0,0,0,0);
            QLabel *al2 = new QLabel("Active:"); al2->setStyleSheet(InspectorStyles::SensorIdLabel); al2->setFixedWidth(130);
            QCheckBox *ac = new QCheckBox(); ac->setChecked(subObj["Active"].toBool());
            ac->setStyleSheet(InspectorStyles::SensorCheckBox);
            connect(ac, &QCheckBox::toggled, this, [=](bool checked) {
                QJsonObject delta; delta["_id"] = mainID;
                QJsonObject updSub = subObj; updSub["Active"] = checked;
                delta[containerKey] = QJsonObject{{subKey, updSub}};
                emit valueChanged(ConnectedID, Name, delta);
            });
            al->addWidget(al2); al->addWidget(ac); al->addStretch();
            subLayout->addWidget(aw);
            itemCount++;
        }

        auto addTypeLabel = [&](const QString &jsonKey, const QString &displayName) {
            if (!subObj.contains(jsonKey)) return;
            QWidget *tw = new QWidget(); tw->setStyleSheet("background:transparent;");
            QHBoxLayout *tl = new QHBoxLayout(tw); tl->setContentsMargins(0,0,0,0);
            QLabel *tl2 = new QLabel(displayName + ":"); tl2->setStyleSheet(InspectorStyles::SensorIdLabel); tl2->setFixedWidth(130);
            QLabel *tv = new QLabel(subObj[jsonKey].toString()); tv->setStyleSheet(InspectorStyles::SensorValueLabel);
            tl->addWidget(tl2); tl->addWidget(tv); tl->addStretch();
            subLayout->addWidget(tw);
            itemCount++;
        };
        addTypeLabel("SensorType", "Sensor Type");
        addTypeLabel("radioType",  "Radio Type");

        for (const QString &propKey : subObj.keys()) {
            if (SKIP_KEYS.contains(propKey)) continue;
            QJsonValue pval = subObj[propKey];
            if (!pval.isObject()) continue;
            QJsonObject pobj = pval.toObject();
            QString ptype = pobj["type"].toString();
            if (ptype.compare("Section", Qt::CaseInsensitive) != 0) continue;
            QWidget *secW = buildSectionWidget(propKey, pobj, subKey, subObj);
            if (secW) {
                subLayout->addWidget(secW);
                itemCount++;
            }
        }

        subLayout->addStretch();
        subcomponentsLayout->addWidget(subGroupBox);
        renderedSubCount++;
        // totalSubcomponentsHeight += 60 + (itemCount * 30) + 20;
        // totalSubcomponentsHeight += 80 + (itemCount * 32) + 60;
        totalSubcomponentsHeight += 120 + (itemCount * 36) + 100;

    }

    if (renderedSubCount == 0) {
        QLabel *emptyLabel = new QLabel("No " + containerKey + " configured");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(InspectorStyles::EmptyLabel);
        subcomponentsLayout->addWidget(emptyLabel);
        totalSubcomponentsHeight += 60;
    }

    subcomponentsLayout->addStretch();
    scrollArea->setWidget(subcomponentsWidget);

    // const int SCROLL_THRESHOLD = 650;
    // int requiredHeight = totalSubcomponentsHeight + 50;

    // if (requiredHeight > SCROLL_THRESHOLD) {
    //     scrollArea->setMinimumHeight(SCROLL_THRESHOLD);
    //     scrollArea->setMaximumHeight(SCROLL_THRESHOLD);
    // } else {
    //     scrollArea->setMinimumHeight(qMax(requiredHeight, 80));
    //     scrollArea->setMaximumHeight(qMax(requiredHeight, 80));
    // }
    // scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    const int SCROLL_THRESHOLD = 1800;
    // int requiredHeight = totalSubcomponentsHeight + 80;
    int requiredHeight = totalSubcomponentsHeight + 150;


    if (requiredHeight > SCROLL_THRESHOLD) {
        scrollArea->setMinimumHeight(SCROLL_THRESHOLD);
        scrollArea->setMaximumHeight(SCROLL_THRESHOLD);
    } else {
        scrollArea->setMinimumHeight(qMax(requiredHeight, 100));
        scrollArea->setMaximumHeight(qMax(requiredHeight, 100));
    }
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // connect(dropdownButton, &QPushButton::clicked, this, [=]() {
    //     bool isVisible = !scrollArea->isVisible();
    //     scrollArea->setVisible(isVisible);
    //     if (isVisible) {
    //         dropdownButton->setText("▲ " + QString::number(renderedSubCount)
    //                                 + " " + capitalizeFirstLetter(containerKey));
    //         int contentHeight = (requiredHeight < SCROLL_THRESHOLD)
    //                                 ? requiredHeight + 40
    //                                 : SCROLL_THRESHOLD + 40;
    //         tableWidget->setRowHeight(row, contentHeight);
    //     } else {
    //         dropdownButton->setText("▼ " + QString::number(renderedSubCount)
    //                                 + " " + capitalizeFirstLetter(containerKey));
    //         tableWidget->setRowHeight(row, 35);
    //     }
    //     tableWidget->viewport()->update();
    //     containerWidget->adjustSize();
    // });
    connect(dropdownButton, &QPushButton::clicked, this, [=]() {
        bool isVisible = !scrollArea->isVisible();
        scrollArea->setVisible(isVisible);
        if (isVisible) {
            dropdownButton->setText("▲ " + QString::number(renderedSubCount)
                                    + " " + capitalizeFirstLetter(containerKey));
            int contentHeight = (requiredHeight < SCROLL_THRESHOLD)
                                    ? requiredHeight + 70   // 40 → 70, dropdown button + padding
                                    : SCROLL_THRESHOLD + 70;
            tableWidget->setRowHeight(row, contentHeight);
        } else {
            dropdownButton->setText("▼ " + QString::number(renderedSubCount)
                                    + " " + capitalizeFirstLetter(containerKey));
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
// refreshForDeveloperMode
void Inspector::refreshForDeveloperMode()
{
    if (!hierarchy || ConnectedID.isEmpty() || Name.isEmpty()) {
        return;
    }
#ifndef GUI_UNIT_TEST
    QJsonObject currentData = hierarchy->getComponentData(ConnectedID, Name);
#else
    QJsonObject currentData;
#endif
    init(ConnectedID, Name, currentData);
}

// resetState
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
        tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        tableWidget->blockSignals(false);
    }

    if (titleLabel) {
        titleLabel->setText("Inspector");
    }

    currentlyExpandedButton = nullptr;
    copiedComponentData = QJsonObject();
    copiedComponentType.clear();
     m_initialComponentData = QJsonObject();
}
void Inspector::storeInitialData(const QJsonObject& data)
{
    m_initialComponentData = data;
}
bool Inspector::isLocked() const
{
    return m_locked;
}
void Inspector::runUnitTestsOnce()
{
    static bool testsRun = false;
    if (testsRun) return;

    // Delay test execution until event loop starts (ensures console is ready)
    QTimer::singleShot(0, []() {
        if (testsRun) return;
        testsRun = true;

        Console* console = nullptr;
        MainWindow* mw = MainWindow::instance();
        if (mw) {
            // Try to get console from any editor that is already initialized
            if (mw->databaseEditor && mw->databaseEditor->console) {
                console = mw->databaseEditor->console;
            } else if (mw->scenarioEditor && mw->scenarioEditor->console) {
                console = mw->scenarioEditor->console;
            } else if (mw->runtimeEditor && mw->runtimeEditor->console) {
                console = mw->runtimeEditor->console;
            }
        }

        if (!console) {
            qDebug() << "Inspector: console not available, cannot run tests";
            return;
        }



        // Create a temporary inspector for testing
        Inspector* testInspector = new Inspector(nullptr);
        runInspectorTests(testInspector, console);
        testInspector->deleteLater();
    });
}
