
/* ========================================================================= */
/* File: entityinfodialog.cpp                                               */
/* Purpose: Implementation of entity information dialog                      */
/* ========================================================================= */

#include "entityinfodialog.h"
#include "entityinfodialog-styles.h"  // Include separate CSS file
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/Struct/formationposition.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/Utils/entityutils.h"

// ========================================================================= //
// Constructor & Basic Setup
// ========================================================================= //

EntityInfoDialog::EntityInfoDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void EntityInfoDialog::setupUI()
{
    setWindowTitle("Entity Information");
    setFixedSize(500, 600);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

    // Apply dark theme to dialog
    setStyleSheet(EntityInfoDialogStyles::Dialog);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    titleLabel = new QLabel("Entity Information");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(EntityInfoDialogStyles::TitleLabel);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(EntityInfoDialogStyles::ScrollArea);

    scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("background-color: #0F2636;");
    scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(3);
    scrollLayout->setContentsMargins(2, 2, 2, 2);

    createAttributeSection();
    createPositionSection();
    createSpeedAltTableSection();
    createEquipmentSection();
    createOptionsSection();

    scrollArea->setWidget(scrollWidget);

    closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(EntityInfoDialogStyles::CloseButton);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &EntityInfoDialog::onCloseClicked);
}

// ========================================================================= //
// UI Section Creation Methods
// ========================================================================= //

void EntityInfoDialog::createAttributeSection()
{
    attributeTable = new QTableWidget(5, 2);
    attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
    attributeTable->setStyleSheet(EntityInfoDialogStyles::TableWidget);
    attributeTable->verticalHeader()->setVisible(false);
    attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attributeTable->setFixedHeight(200);

    QStringList attributes = {"Type", "Name", "DIS name", "Damages", "Carrier"};
    for(int i = 0; i < 5; ++i) {
        attributeTable->setItem(i, 0, new QTableWidgetItem(attributes[i]));
        // Style for first column (attributes)
        attributeTable->item(i, 0)->setForeground(Qt::white);
        attributeTable->setItem(i, 1, new QTableWidgetItem("-"));
        // Style for second column (values)
        attributeTable->item(i, 1)->setForeground(Qt::white);
    }

    attributeTable->horizontalHeader()->setStretchLastSection(true);
    attributeTable->setColumnWidth(0, 150);

    scrollLayout->addWidget(attributeTable);
}

void EntityInfoDialog::createPositionSection()
{
    positionLabel = new QLabel("Position: -");
    positionLabel->setStyleSheet(EntityInfoDialogStyles::PositionLabel);
    positionLabel->setMinimumHeight(35);
    scrollLayout->addWidget(positionLabel);
}

void EntityInfoDialog::createSpeedAltTableSection()
{
    speedAltTable = new QTableWidget(2, 3);
    speedAltTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Current" << "Requested");
    speedAltTable->setStyleSheet(EntityInfoDialogStyles::TableWidget);
    speedAltTable->verticalHeader()->setVisible(false);

    speedAltTable->setEditTriggers(QAbstractItemView::CurrentChanged |
                                   QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::EditKeyPressed |
                                   QAbstractItemView::SelectedClicked);

    // Set column properties
    speedAltTable->setItem(0, 0, new QTableWidgetItem("Speed"));
    speedAltTable->item(0, 0)->setForeground(Qt::white);

    speedAltTable->setItem(0, 1, new QTableWidgetItem("-"));
    // Current column ko non-editable banayein
    speedAltTable->item(0, 1)->setFlags(speedAltTable->item(0, 1)->flags() & ~Qt::ItemIsEditable);
    speedAltTable->item(0, 1)->setForeground(Qt::white);

    speedAltTable->setItem(0, 2, new QTableWidgetItem("-"));
    // Requested column ko editable banayein
    speedAltTable->item(0, 2)->setFlags(speedAltTable->item(0, 2)->flags() | Qt::ItemIsEditable);
    speedAltTable->item(0, 2)->setForeground(Qt::white);

    speedAltTable->setItem(1, 0, new QTableWidgetItem("Altitude"));
    speedAltTable->item(1, 0)->setForeground(Qt::white);

    speedAltTable->setItem(1, 1, new QTableWidgetItem("-"));
    // Current column ko non-editable banayein
    speedAltTable->item(1, 1)->setFlags(speedAltTable->item(1, 1)->flags() & ~Qt::ItemIsEditable);
    speedAltTable->item(1, 1)->setForeground(Qt::white);

    speedAltTable->setItem(1, 2, new QTableWidgetItem("-"));
    // Requested column ko editable banayein
    speedAltTable->item(1, 2)->setFlags(speedAltTable->item(1, 2)->flags() | Qt::ItemIsEditable);
    speedAltTable->item(1, 2)->setForeground(Qt::white);

    int rowHeight = 40;
    int headerHeight = speedAltTable->horizontalHeader()->height();
    speedAltTable->setFixedHeight((2 * rowHeight) + headerHeight + 5);

    speedAltTable->horizontalHeader()->setStretchLastSection(true);
    speedAltTable->setColumnWidth(0, 100);

    connect(speedAltTable, &QTableWidget::cellChanged, this, &EntityInfoDialog::onSpeedAltCellChanged);

    connect(speedAltTable, &QTableWidget::cellPressed, this, [=](int row, int column) {
        if (column == 2) {
            QTimer::singleShot(0, this, [=]() {
                if (speedAltTable->item(row, column)) {
                    speedAltTable->editItem(speedAltTable->item(row, column));
                }
            });
        }
    });

    scrollLayout->addWidget(speedAltTable);
}

void EntityInfoDialog::createEquipmentSection()
{
    QGridLayout *equipmentLayout = new QGridLayout();
    equipmentLayout->setSpacing(3);

    sensorsButton = new QPushButton("Sensors");
    radiosButton = new QPushButton("Radios");
    iffButton = new QPushButton("IFF");
    weaponsButton = new QPushButton("Weapons");
    formationButton = new QPushButton("Formation");

    QList<QPushButton*> buttons = {sensorsButton, radiosButton, iffButton, weaponsButton, formationButton};
    for(auto button : buttons) {
        button->setStyleSheet(EntityInfoDialogStyles::PushButton);
        button->setMinimumHeight(35);
    }

    equipmentLayout->addWidget(sensorsButton, 0, 0);
    equipmentLayout->addWidget(radiosButton, 0, 1);
    equipmentLayout->addWidget(iffButton, 1, 0);
    equipmentLayout->addWidget(formationButton, 1, 1);
    equipmentLayout->addWidget(weaponsButton, 2, 0);

    connect(weaponsButton, &QPushButton::clicked, this, &EntityInfoDialog::onWeaponsClicked);
    connect(sensorsButton, &QPushButton::clicked, this, &EntityInfoDialog::onSensorsClicked);
    connect(formationButton, &QPushButton::clicked, this, &EntityInfoDialog::onFormationClicked);
    connect(radiosButton, &QPushButton::clicked, this, &EntityInfoDialog::onRadiosClicked);
    connect(iffButton, &QPushButton::clicked, this, &EntityInfoDialog::onIFFClicked);

    QWidget *equipmentWidget = new QWidget();
    equipmentWidget->setStyleSheet(EntityInfoDialogStyles::EquipmentWidget);
    equipmentWidget->setLayout(equipmentLayout);
    scrollLayout->addWidget(equipmentWidget);
}

void EntityInfoDialog::createOptionsSection()
{
    QVBoxLayout *optionsLayout = new QVBoxLayout();
    optionsLayout->setSpacing(2);

    QHBoxLayout *firstLineLayout = new QHBoxLayout();
    firstLineLayout->setSpacing(10);

    followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
    showDetectionCheckBox = new QCheckBox("Show Detection");

    followTrajectoryCheckBox->setStyleSheet(EntityInfoDialogStyles::CheckBox);
    showDetectionCheckBox->setStyleSheet(EntityInfoDialogStyles::CheckBox);

    firstLineLayout->addWidget(followTrajectoryCheckBox);
    firstLineLayout->addWidget(showDetectionCheckBox);
    firstLineLayout->addStretch();

    QHBoxLayout *secondLineLayout = new QHBoxLayout();
    secondLineLayout->setSpacing(10);

    showConnectionCheckBox = new QCheckBox("Show Connection");
    showConnectionCheckBox->setStyleSheet(EntityInfoDialogStyles::CheckBox);

    secondLineLayout->addWidget(showConnectionCheckBox);
    secondLineLayout->addStretch();

    optionsLayout->addLayout(firstLineLayout);
    optionsLayout->addLayout(secondLineLayout);

    showDetectionCheckBox->setChecked(true);

    connect(followTrajectoryCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
        if(!currentEntityId.isEmpty() && entryInfo) {
            if (entryInfo->dynamicModel) {
                entryInfo->dynamicModel->follow = checked;
                if (!checked) {
                    entryInfo->dynamicModel->currentSpeed = 0;
                }
            }

            if (entryInfo->trajectory) {
                entryInfo->trajectory->Active = checked;
                entryInfo->trajectory->FollowPath = checked;
            }

            emit update();
        }
    });

    connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
        if(!currentEntityId.isEmpty() && entryInfo && entryInfo->entity) {
            entryInfo->detection = checked;
            entryInfo->radioVisible = checked;
            emit update();
        }
    });

    QWidget *optionsWidget = new QWidget();
    optionsWidget->setStyleSheet(EntityInfoDialogStyles::OptionsWidget);
    optionsWidget->setLayout(optionsLayout);
    scrollLayout->addWidget(optionsWidget);
}

// ========================================================================= //
// Entity Data Management Methods
// ========================================================================= //

void EntityInfoDialog::setEntityInfo(const QString& entityId, const QString& entityName, MeshEntry* info)
{
    if (!info) {
        clearInfo();
        hide();
        return;
    }

    if (!info->entity) {
        clearInfo();
        hide();
        return;
    }

    if (!info->entity->Active) {
        clearInfo();
        hide();
        return;
    }

    currentEntityId = entityId;
    currentEntityName = entityName;
    entryInfo = info;

    // Set title
    if (!currentEntityName.isEmpty()) {
        titleLabel->setText("Name: " + currentEntityName);
    } else {
        titleLabel->setText("Name: " + entityId);
    }

    // Update trajectory checkbox
    if (entryInfo->trajectory) {
        followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
    } else {
        followTrajectoryCheckBox->setChecked(false);
    }

    // Set detection checkbox state
    if (entryInfo->entity) {
        entryInfo->detection = showDetectionCheckBox->isChecked();
    }
}

void EntityInfoDialog::updateEntityInfo()
{
    if (currentEntityId.isEmpty() || !entryInfo) {
        if (isVisible()) {
            close();
        }
        return;
    }

    if (!entryInfo->entity || !entryInfo->entity->Active) {
        if (isVisible()) {
            close();
        }
        return;
    }

    if (!entryInfo->coreTransform) {
        if (isVisible()) {
            close();
        }
        return;
    }

    if (!currentEntityName.isEmpty()) {
        titleLabel->setText("Name: " + currentEntityName);
    }

    QString typeStr = "Unknown";
    if(entryInfo->entity) {
        switch(entryInfo->entity->type) {
        case Constants::EntityType::Platform: typeStr = "Platform"; break;
        case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
        case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
        default: typeStr = "Unknown"; break;
        }
    }

    if (attributeTable) {
        attributeTable->item(0, 1)->setText(typeStr);

        if (!currentEntityName.isEmpty()) {
            attributeTable->item(1, 1)->setText(currentEntityName);
        } else {
            attributeTable->item(1, 1)->setText("-");
        }

        attributeTable->item(2, 1)->setText("-");
        attributeTable->item(3, 1)->setText("0 %");
        attributeTable->item(4, 1)->setText("Not Embarked");
    }

    // Get position data
    float lat = 0.0f, lon = 0.0f, x = 0.0f, z = 0.0f;

    if(entryInfo->coreTransform) {
        lat = entryInfo->coreTransform->getLatitude();
        lon = entryInfo->coreTransform->getLongitude();
    }

    if(entryInfo->transform) {
        x = entryInfo->transform->translation().x();
        z = entryInfo->transform->translation().z();
    }

    // Get altitude
    float y = 0.0f;
    if (entryInfo->trajectory && !entryInfo->trajectory->Trajectories.empty()) {
        int currentIdx = (entryInfo->trajectory->current >= 0 &&
                          entryInfo->trajectory->current < entryInfo->trajectory->Trajectories.size()) ?
                             entryInfo->trajectory->current : 0;

        if (Waypoints* wp = entryInfo->trajectory->Trajectories[currentIdx]) {
            if (wp->position) y = wp->position->y;
        }
    }

    // Update position display
    QString currentPos = QString("Lat: %1, Long: %2, X: %3, Z: %4")
                             .arg(lat, 0, 'f', 6)
                             .arg(lon, 0, 'f', 6)
                             .arg(x, 0, 'f', 2)
                             .arg(z, 0, 'f', 2);

    positionLabel->setText("Position: " + currentPos);

    DynamicModel* dynModel = entryInfo->dynamicModel ? entryInfo->dynamicModel :
                                 (entryInfo->platform && entryInfo->platform->dynamicModel) ?
                                 entryInfo->platform->dynamicModel : nullptr;

    // Update speed/altitude table
    if (speedAltTable && dynModel) {
        // For CURRENT values: check if we should use mothership's values
        float currentSpeed = dynModel->currentSpeed;
        float currentAltitude = dynModel->currentAltitude * KMtoFT;

        // If this is an ally following a mothership, use mothership's current values
        if (dynModel->follow && dynModel->followEntity) {
            Platform* mothership = dynamic_cast<Platform*>(dynModel->followEntity);
            if (mothership && mothership->dynamicModel) {
                currentSpeed = mothership->dynamicModel->currentSpeed;
                currentAltitude = mothership->dynamicModel->currentAltitude * KMtoFT;
            }
        }

        // Set the values
        speedAltTable->item(0, 1)->setText(QString("%1 km/h").arg(currentSpeed, 0, 'f', 2));
        speedAltTable->item(0, 2)->setText(QString("%1 km/h").arg(dynModel->moveSpeed, 0, 'f', 2));
        speedAltTable->item(1, 1)->setText(QString("%1 ft").arg(currentAltitude, 0, 'f', 2));
        speedAltTable->item(1, 2)->setText(QString("%1 ft").arg(dynModel->Altitude, 0, 'f', 2));
    } else if (speedAltTable) {
        // No dynamic model
        speedAltTable->item(0, 1)->setText("0.0 km/h");
        speedAltTable->item(0, 2)->setText("0.0 km/h");
        speedAltTable->item(1, 1)->setText("0.0 ft");
        speedAltTable->item(1, 2)->setText("0.0 ft");
    }

    // Update checkboxes
    if (followTrajectoryCheckBox) {
        followTrajectoryCheckBox->setChecked(entryInfo->trajectory ?
                                                 entryInfo->trajectory->FollowPath : false);
    }
}

void EntityInfoDialog::clearInfo()
{
    titleLabel->setText("Entity Information");
    currentEntityId.clear();

    if (attributeTable) {
        for(int i = 0; i < 5; ++i) {
            attributeTable->item(i, 1)->setText("-");
        }
    }

    positionLabel->setText("Position: -");

    if (speedAltTable) {
        speedAltTable->item(0, 1)->setText("-");
        speedAltTable->item(0, 2)->setText("-");
        speedAltTable->item(1, 1)->setText("-");
        speedAltTable->item(1, 2)->setText("-");
    }

    QList<QCheckBox*> checkboxes = {followTrajectoryCheckBox, showConnectionCheckBox};

    for(auto checkbox : checkboxes) {
        if(checkbox) checkbox->setChecked(false);
    }

    if(showDetectionCheckBox) showDetectionCheckBox->setChecked(true);
}

// ========================================================================= //
// Button Click Handlers
// ========================================================================= //

void EntityInfoDialog::onCloseClicked()
{
    hide();
}

void EntityInfoDialog::onWeaponsClicked()
{
    QMessageBox::information(this, "Weapons",
                             QString("Weapons for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg("No weapons data available"));
}

void EntityInfoDialog::onSensorsClicked()
{
    if(!currentEntityId.isEmpty() && entryInfo && entryInfo->platform) {
        QDialog *sensorsDialog = new QDialog(this);
        sensorsDialog->setWindowTitle("Sensors - " + currentEntityId);
        sensorsDialog->setMinimumSize(450, 300);
        sensorsDialog->setMaximumSize(500, 400);
        sensorsDialog->setStyleSheet(EntityInfoDialogStyles::SubDialog);

        QVBoxLayout *layout = new QVBoxLayout(sensorsDialog);
        layout->setSpacing(8);
        layout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("Sensor Systems");
        titleLabel->setProperty("class", "title");
        titleLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.title { font-size: 14px; color: white; font-weight: bold; }");
        layout->addWidget(titleLabel);

        QTableWidget *sensorsTable = new QTableWidget();
        sensorsTable->setColumnCount(4);
        sensorsTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Range" << "FOV");
        sensorsTable->verticalHeader()->setVisible(false);
        sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        sensorsTable->setStyleSheet(EntityInfoDialogStyles::SubDialogTable);

        QTimer *sensorsUpdateTimer = new QTimer(sensorsDialog);
        connect(sensorsUpdateTimer, &QTimer::timeout, [=]() {
            updateSensorsTable(sensorsTable, entryInfo->platform);
        });
        sensorsUpdateTimer->start(100);

        updateSensorsTable(sensorsTable, entryInfo->platform);

        sensorsTable->setColumnWidth(0, 120);
        sensorsTable->setColumnWidth(1, 80);
        sensorsTable->setColumnWidth(2, 80);
        sensorsTable->setColumnWidth(3, 60);
        layout->addWidget(sensorsTable);

        QLabel *summaryLabel = new QLabel();
        summaryLabel->setProperty("class", "summary");
        summaryLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.summary { color: #B0B0B0; font-size: 11px; }");
        layout->addWidget(summaryLabel);

        connect(sensorsUpdateTimer, &QTimer::timeout, [=]() {
            int total = 0;
            if(Platform* platform = dynamic_cast<Platform*>(entryInfo->platform)) {
                if(platform->sensors && platform->sensors->sensors) {
                    total = platform->sensors->sensors->size();
                }
            }
            summaryLabel->setText(QString("Total: %1 sensors").arg(total));
        });

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton *closeButton = new QPushButton("Close");
        closeButton->setStyleSheet(EntityInfoDialogStyles::SubDialogButton);
        connect(closeButton, &QPushButton::clicked, [=]() {
            sensorsUpdateTimer->stop();
            sensorsDialog->close();
        });
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        sensorsDialog->show();
        sensorsDialog->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        QMessageBox::information(this, "Sensors", "No platform data available.");
    }
}

void EntityInfoDialog::onRadiosClicked()
{
    if(!currentEntityId.isEmpty() && entryInfo && entryInfo->entity) {
        QDialog *radiosDialog = new QDialog(this);
        radiosDialog->setWindowTitle("Radios - " + currentEntityId);
        radiosDialog->setMinimumSize(300, 300);
        radiosDialog->setMaximumSize(400, 400);
        radiosDialog->setStyleSheet(EntityInfoDialogStyles::SubDialog);

        QVBoxLayout *layout = new QVBoxLayout(radiosDialog);
        layout->setSpacing(8);
        layout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("Radio Communication Systems");
        titleLabel->setProperty("class", "title");
        titleLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.title { font-size: 14px; color: white; font-weight: bold; }");
        layout->addWidget(titleLabel);

        QTableWidget *radiosTable = new QTableWidget();
        radiosTable->setColumnCount(2);
        radiosTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Range");
        radiosTable->verticalHeader()->setVisible(false);
        radiosTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        radiosTable->setStyleSheet(EntityInfoDialogStyles::SubDialogTable);

        QTimer *radiosUpdateTimer = new QTimer(radiosDialog);
        connect(radiosUpdateTimer, &QTimer::timeout, [=]() {
            if(entryInfo->platform && entryInfo->platform->radios && entryInfo->platform->radios->radios) {
                radiosTable->setRowCount(0);
                int row = 0;
                for (auto pair : *entryInfo->platform->radios->radios) {
                    if(Radio* radio = pair.second) {
                        radiosTable->insertRow(row);
                        radiosTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(radio->Name)));
                        radiosTable->setItem(row, 1, new QTableWidgetItem(
                                                         radio->Range > 0 ? QString("%1 km").arg(radio->Range, 0, 'f', 1) : "N/A"));
                        row++;
                    }
                }
                if(row == 0) {
                    radiosTable->setRowCount(1);
                    radiosTable->setItem(0, 0, new QTableWidgetItem("No radios"));
                    radiosTable->setItem(0, 1, new QTableWidgetItem(""));
                }
            }
        });
        radiosUpdateTimer->start(100);

        radiosTable->setColumnWidth(0, 120);
        radiosTable->setColumnWidth(1, 80);
        layout->addWidget(radiosTable);

        QLabel *summaryLabel = new QLabel();
        summaryLabel->setProperty("class", "summary");
        summaryLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.summary { color: #B0B0B0; font-size: 11px; }");
        layout->addWidget(summaryLabel);

        connect(radiosUpdateTimer, &QTimer::timeout, [=]() {
            int total = entryInfo->platform && entryInfo->platform->radios ?
                            entryInfo->platform->radios->radios->size() : 0;
            summaryLabel->setText(QString("Total: %1 radios").arg(total));
        });

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton *closeButton = new QPushButton("Close");
        closeButton->setStyleSheet(EntityInfoDialogStyles::SubDialogButton);
        connect(closeButton, &QPushButton::clicked, [=]() {
            radiosUpdateTimer->stop();
            radiosDialog->close();
        });
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        radiosDialog->show();
        radiosDialog->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        QMessageBox::information(this, "Radios", "No entity selected.");
    }
}

void EntityInfoDialog::onIFFClicked()
{
    if(!currentEntityId.isEmpty() && entryInfo && entryInfo->entity) {
        QDialog *iffDialog = new QDialog(this);
        iffDialog->setWindowTitle("IFF Systems - " + currentEntityId);
        iffDialog->setMinimumSize(450, 300);
        iffDialog->setMaximumSize(500, 400);
        iffDialog->setStyleSheet(EntityInfoDialogStyles::SubDialog);

        QVBoxLayout *layout = new QVBoxLayout(iffDialog);
        layout->setSpacing(8);
        layout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("IFF (Identification Friend or Foe) Systems");
        titleLabel->setProperty("class", "title");
        titleLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.title { font-size: 14px; color: white; font-weight: bold; }");
        layout->addWidget(titleLabel);

        QTableWidget *iffTable = new QTableWidget();
        iffTable->setColumnCount(3);
        iffTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Mode" << "Range");
        iffTable->verticalHeader()->setVisible(false);
        iffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        iffTable->setStyleSheet(EntityInfoDialogStyles::SubDialogTable);

        QTimer *iffUpdateTimer = new QTimer(iffDialog);
        connect(iffUpdateTimer, &QTimer::timeout, [=]() {
            if(entryInfo->platform && entryInfo->platform->iffs && entryInfo->platform->iffs->iffs) {
                iffTable->setRowCount(0);
                int row = 0;
                for (auto pair : *entryInfo->platform->iffs->iffs) {
                    if(IFF* iff = pair.second) {
                        iffTable->insertRow(row);
                        iffTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(iff->Name)));

                        QString modeStr = "Unknown";
                        switch(iff->operationalMode) {
                        case IFF::OperationalMode::Active: modeStr = "Active"; break;
                        case IFF::OperationalMode::Passive: modeStr = "Passive"; break;
                        case IFF::OperationalMode::Off: modeStr = "Off"; break;
                        case IFF::OperationalMode::Simulation: modeStr = "Simulation"; break;
                        default: modeStr = "Unknown"; break;
                        }
                        iffTable->setItem(row, 1, new QTableWidgetItem(modeStr));

                        iffTable->setItem(row, 2, new QTableWidgetItem(
                                                      QString("%1 km").arg(iff->emittingRange, 0, 'f', 1)));
                        row++;
                    }
                }
                if(row == 0) {
                    iffTable->setRowCount(1);
                    iffTable->setItem(0, 0, new QTableWidgetItem("No IFF systems"));
                    iffTable->setItem(0, 1, new QTableWidgetItem(""));
                    iffTable->setItem(0, 2, new QTableWidgetItem(""));
                }
            }
        });
        iffUpdateTimer->start(100);

        iffTable->setColumnWidth(0, 120);
        iffTable->setColumnWidth(1, 80);
        iffTable->setColumnWidth(2, 80);
        layout->addWidget(iffTable);

        QLabel *summaryLabel = new QLabel();
        summaryLabel->setProperty("class", "summary");
        summaryLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.summary { color: #B0B0B0; font-size: 11px; }");
        layout->addWidget(summaryLabel);

        connect(iffUpdateTimer, &QTimer::timeout, [=]() {
            int total = 0, active = 0;
            if(entryInfo && entryInfo->entity) {
                total = entryInfo->entity->iffList.size();
                for (IFF* iff : entryInfo->entity->iffList) {
                    if(iff->operationalMode != IFF::OperationalMode::Off) active++;
                }
            }
            summaryLabel->setText(QString("Total: %1 IFF systems | Active: %2").arg(total).arg(active));
        });

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton *closeButton = new QPushButton("Close");
        closeButton->setStyleSheet(EntityInfoDialogStyles::SubDialogButton);
        connect(closeButton, &QPushButton::clicked, [=]() {
            iffUpdateTimer->stop();
            iffDialog->close();
        });
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        iffDialog->show();
        iffDialog->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        QMessageBox::information(this, "IFF", "No entity selected.");
    }
}

// by WARIS
void EntityInfoDialog::onFormationClicked()
{
    if(!currentEntityId.isEmpty() && entryInfo && entryInfo->platform) {
        QDialog *formationDialog = new QDialog(this);
        formationDialog->setWindowTitle("Formation - " + currentEntityId);
        formationDialog->setMinimumSize(500, 400);
        formationDialog->setMaximumSize(600, 500);
        formationDialog->setStyleSheet(EntityInfoDialogStyles::SubDialog);

        QVBoxLayout *layout = new QVBoxLayout(formationDialog);
        layout->setSpacing(8);
        layout->setContentsMargins(10, 10, 10, 10);

        QLabel *titleLabel = new QLabel("Formation Information");
        titleLabel->setProperty("class", "title");
        titleLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.title { font-size: 16px; color: white; font-weight: bold; padding-bottom: 10px; }");
        layout->addWidget(titleLabel);

        // Try to find formation for this entity
        Formation* formation = findFormationForEntity(entryInfo->platform);

        if (formation) {
            // Display formation information
            displayFormationInfo(formation, entryInfo->platform, layout);
        } else {
            // Check if this entity IS a Formation entity itself
            Formation* thisFormation = dynamic_cast<Formation*>(entryInfo->platform);
            if (thisFormation) {
                // This is a Formation entity
                displayFormationInfo(thisFormation, entryInfo->platform, layout);

                // Add note that this is the Formation entity itself
                QLabel *noteLabel = new QLabel("Note: This is the Formation entity that manages the formation.");
                noteLabel->setProperty("class", "note");
                noteLabel->setStyleSheet(EntityInfoDialogStyles::SubDialog + " QLabel.note { color: #F1C40F; background-color: #1A3652; padding: 8px; border: 1px solid #F1C40F; border-radius: 3px; font-size: 11px; }");
                layout->addWidget(noteLabel);
            } else {
                // Entity is not part of any formation
                QLabel *noFormationLabel = new QLabel(
                    "This entity is not part of any formation.\n\n");
                noFormationLabel->setAlignment(Qt::AlignCenter);
                noFormationLabel->setStyleSheet(EntityInfoDialogStyles::NoDataLabel);
                noFormationLabel->setWordWrap(true);
                layout->addWidget(noFormationLabel);
            }
        }

        // Close button
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton *closeButton = new QPushButton("Close");
        closeButton->setStyleSheet(EntityInfoDialogStyles::SubDialogButton);
        connect(closeButton, &QPushButton::clicked, [=]() {
            formationDialog->close();
        });
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        formationDialog->show();
        formationDialog->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        QMessageBox::information(this, "Formation", "No entity selected or entity data missing.");
    }
}

// by WARIS
// Add this helper function implementation
Formation* EntityInfoDialog::findFormationForEntity(Entity* entity)
{
    if (!entity) return nullptr;

    // Get the parent hierarchy
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(entity);
    if (!parent || !parent->Entities) return nullptr;

    // Search through all entities to find Formation entities
    for (auto& pair : *parent->Entities) {
        Formation* formation = dynamic_cast<Formation*>(pair.second);
        if (formation) {
            // Check if this entity is the mothership
            if (formation->mothership && formation->mothership->entity == entity) {
                return formation;
            }

            // Check if this entity is an ally
            if (formation->formationPositions) {
                for (auto& fp : *formation->formationPositions) {
                    if (fp.second && fp.second->entity == entity) {
                        return formation;
                    }
                }
            }
        }
    }

    return nullptr;
}

// by WARIS
// Helper function to display formation information
void EntityInfoDialog::displayFormationInfo(Formation* formation, Entity* currentEntity, QVBoxLayout* layout)
{
    if (!formation || !currentEntity) return;

    QString formationType = formation->formationTypeToString(formation->formationType);
    int allyCount = formation->count;

    // Get mothership info
    QString mothershipName = "Unknown";
    QString mothershipId = "Unknown";
    if (formation->mothership && formation->mothership->entity) {
        mothershipName = QString::fromStdString(formation->mothership->entity->Name);
        mothershipId = QString::fromStdString(formation->mothership->entity->ID);
    }

    // Determine if current entity is mothership
    bool isMothership = (formation->mothership && formation->mothership->entity == currentEntity);
    QString currentRole = isMothership ? "Mothership" : "Ally";

    // Create main info section
    QGridLayout *infoLayout = new QGridLayout();
    infoLayout->setSpacing(5);

    QLabel *typeLabel = new QLabel("Formation Type:");
    typeLabel->setStyleSheet("color: white;");
    infoLayout->addWidget(typeLabel, 0, 0);

    QLabel *typeValue = new QLabel(formationType);
    typeValue->setStyleSheet("color: #E0E0E0;");
    infoLayout->addWidget(typeValue, 0, 1);

    QLabel *mothershipLabel = new QLabel("Mothership:");
    mothershipLabel->setStyleSheet("color: white;");
    infoLayout->addWidget(mothershipLabel, 1, 0);

    QLabel *mothershipValue = new QLabel(mothershipName + " (" + mothershipId + ")");
    mothershipValue->setStyleSheet("color: #E0E0E0;");
    infoLayout->addWidget(mothershipValue, 1, 1);

    QLabel *allyCountLabel = new QLabel("Number of Allies:");
    allyCountLabel->setStyleSheet("color: white;");
    infoLayout->addWidget(allyCountLabel, 2, 0);

    QLabel *allyCountValue = new QLabel(QString::number(allyCount));
    allyCountValue->setStyleSheet("color: #E0E0E0;");
    infoLayout->addWidget(allyCountValue, 2, 1);

    QLabel *roleLabel = new QLabel("Your Role:");
    roleLabel->setStyleSheet("color: white;");
    infoLayout->addWidget(roleLabel, 3, 0);

    QLabel *roleValue = new QLabel(currentRole);
    roleValue->setStyleSheet(isMothership ? "color: #F39C12; font-weight: bold;" : "color: #3498DB; font-weight: bold;");
    infoLayout->addWidget(roleValue, 3, 1);

    QWidget *infoWidget = new QWidget();
    infoWidget->setStyleSheet("background-color: transparent;");
    infoWidget->setLayout(infoLayout);
    layout->addWidget(infoWidget);

    // Create table for all formation positions
    QLabel *tableLabel = new QLabel("Formation Positions:");
    tableLabel->setStyleSheet("QLabel { font-size: 12px; font-weight: bold; color: #E0E0E0; padding-top: 10px; }");
    layout->addWidget(tableLabel);

    QTableWidget *formationTable = new QTableWidget();
    formationTable->setColumnCount(3);
    formationTable->setHorizontalHeaderLabels(QStringList() << "Position" << "Entity" << "Offset (X, Y, Z)");
    formationTable->verticalHeader()->setVisible(false);
    formationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    formationTable->setStyleSheet(EntityInfoDialogStyles::SubDialogTable);

    // Track row count
    int row = 0;

    // Add mothership first
    if (formation->mothership && formation->mothership->entity) {
        formationTable->insertRow(row);
        formationTable->setItem(row, 0, new QTableWidgetItem("Mothership"));
        formationTable->setItem(row, 1, new QTableWidgetItem(
                                            QString::fromStdString(formation->mothership->entity->Name)));

        QString offsetStr = "0, 0, 0";
        if (formation->mothership->Offset) {
            offsetStr = QString("%1, %2, %3")
                            .arg(formation->mothership->Offset->x, 0, 'f', 2)
                            .arg(formation->mothership->Offset->y, 0, 'f', 2)
                            .arg(formation->mothership->Offset->z, 0, 'f', 2);
        }
        formationTable->setItem(row, 2, new QTableWidgetItem(offsetStr));

        // Highlight if this is the current entity
        if (formation->mothership->entity == currentEntity) {
            for (int col = 0; col < 3; col++) {
                QTableWidgetItem *item = formationTable->item(row, col);
                if (item) {
                    item->setBackground(QColor(45, 75, 105)); // Dark orange for mothership
                    item->setToolTip("Current Entity (Mothership)");
                }
            }
        }
        row++;
    }

    // Add allies
    if (formation->formationPositions && !formation->formationPositions->empty()) {
        // Sort by position name for consistent display
        std::vector<std::pair<std::string, FormationPosition*>> sortedPositions;
        for (const auto& pair : *formation->formationPositions) {
            sortedPositions.push_back(pair);
        }

        // Simple alphabetical sort
        std::sort(sortedPositions.begin(), sortedPositions.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto& pair : sortedPositions) {
            const std::string& positionName = pair.first;
            FormationPosition* position = pair.second;

            if (position && position->entity) {
                formationTable->insertRow(row);
                formationTable->setItem(row, 0,
                                        new QTableWidgetItem(QString::fromStdString(positionName)));
                formationTable->setItem(row, 1,
                                        new QTableWidgetItem(QString::fromStdString(position->entity->Name)));

                QString offsetStr = "0, 0, 0";
                if (position->Offset) {
                    offsetStr = QString("%1, %2, %3")
                                    .arg(position->Offset->x, 0, 'f', 2)
                                    .arg(position->Offset->y, 0, 'f', 2)
                                    .arg(position->Offset->z, 0, 'f', 2);
                }
                formationTable->setItem(row, 2, new QTableWidgetItem(offsetStr));

                // Highlight if this is the current entity
                if (position->entity == currentEntity) {
                    for (int col = 0; col < 3; col++) {
                        QTableWidgetItem *item = formationTable->item(row, col);
                        if (item) {
                            item->setBackground(QColor(35, 65, 95)); // Dark blue for ally
                            item->setToolTip("Current Entity (Ally)");
                        }
                    }
                }
                row++;
            }
        }
    }

    // Adjust table size
    formationTable->setColumnWidth(0, 100);
    formationTable->setColumnWidth(1, 150);
    formationTable->setColumnWidth(2, 150);
    formationTable->setMinimumHeight(qMin(row * 30 + 30, 300));
    layout->addWidget(formationTable);
}

// ========================================================================= //
// Helper Methods
// ========================================================================= //

void EntityInfoDialog::updateSensorsTable(QTableWidget* sensorsTable, Entity* platform)
{
    if(!sensorsTable || !platform) return;

    sensorsTable->setRowCount(0);
    Platform* platformPtr = dynamic_cast<Platform*>(platform);

    if(platformPtr && platformPtr->sensors && platformPtr->sensors->sensors) {
        int row = 0;
        for (const auto& pair : *platformPtr->sensors->sensors) {
            if(Sensor* sensor = pair.second) {
                sensorsTable->insertRow(row);
                sensorsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(sensor->Name)));

                QString typeStr = "Unknown";
                switch(sensor->subType) {
                case Sensor::SubType::Generic: typeStr = "Radar"; break;
                case Sensor::SubType::CSM: typeStr = "CSM"; break;
                case Sensor::SubType::ESM: typeStr = "ESM"; break;
                default: typeStr = "Other"; break;
                }
                sensorsTable->setItem(row, 1, new QTableWidgetItem(typeStr));

                QString rangeStr = "-";
                if(sensor->range > 0) rangeStr = QString("%1 km").arg(sensor->range, 0, 'f', 1);
                sensorsTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

                QString fovStr = "-";
                if(sensor->subType == Sensor::SubType::Generic && sensor->maxDetectionAngle > 0) {
                    fovStr = QString("%1°").arg(sensor->maxDetectionAngle, 0, 'f', 0);
                }
                sensorsTable->setItem(row, 3, new QTableWidgetItem(fovStr));
                row++;
            }
        }

        if(row == 0) {
            sensorsTable->setRowCount(1);
            sensorsTable->setItem(0, 0, new QTableWidgetItem("No sensors available"));
            sensorsTable->setItem(0, 1, new QTableWidgetItem(""));
            sensorsTable->setItem(0, 2, new QTableWidgetItem(""));
            sensorsTable->setItem(0, 3, new QTableWidgetItem(""));
        }
    }
}

// ========================================================================= //
// Speed/Altitude Table Handlers
// ========================================================================= //

void EntityInfoDialog::onSpeedAltCellChanged(int row, int column)
{
    if (column != 2 || row > 1) return;

    static bool updating = false;
    if (updating) return;

    if (!currentEntityId.isEmpty() && entryInfo) {
        QTableWidgetItem* item = speedAltTable->item(row, column);
        if (!item) return;
        QString text = item->text();
        QString cleanText = text;
        QString numberText;
        for (int i = 0; i < cleanText.length(); ++i) {
            QChar ch = cleanText[i];
            if (ch.isDigit() || ch == '.' || ch == '-') {
                numberText.append(ch);
            }
        }
        if (numberText.isEmpty()) {
            // If no number found, revert
            updateEntityInfo();
            return;
        }
        bool ok;
        float value = numberText.toFloat(&ok);
        if (!ok) {
            updateEntityInfo();
            return;
        }
        DynamicModel* dynModel = entryInfo->dynamicModel ? entryInfo->dynamicModel :
                                     (entryInfo->platform && entryInfo->platform->dynamicModel) ?
                                     entryInfo->platform->dynamicModel : nullptr;

        if (!dynModel) {
            updateEntityInfo();
            return;
        }
        updating = true;
        if (row == 0) {
            dynModel->moveSpeed = value;

            if (dynModel->follow && dynModel->followEntity) {
                Platform* mothership = dynamic_cast<Platform*>(dynModel->followEntity);
                if (mothership && mothership->dynamicModel) {
                    mothership->dynamicModel->moveSpeed = value;
                }
            }
            item->setText(QString::number(value, 'f', 2) + " km/h");
        }
        else if (row == 1) {
            dynModel->Altitude = value;
            item->setText(QString::number(value, 'f', 2) + " ft");
        }
        updating = false;

        emit update();
        emit speedAltitudeUpdated(currentEntityId,
                                  row == 0 ? value : -1,
                                  row == 1 ? value : -1);
    }
}
