

#include "entityinfodialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qtimer.h"

EntityInfoDialog::EntityInfoDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();



    connect(followTrajectoryCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
        if(!currentEntityId.isEmpty() && entryInfo) {
            if (entryInfo->dynamicModel) {
                entryInfo->dynamicModel->follow = checked;

                if (!checked) {

                    // entryInfo->dynamicModel->speeed = 0;
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
}

void EntityInfoDialog::setupUI()
{
    setWindowTitle("Entity Information");
    setFixedSize(500, 600);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

    // Simple white background
    setStyleSheet("QDialog { background-color: white; }");

    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Title - waise hi
    titleLabel = new QLabel("Entity Information");
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 16px; color: #000000; padding: 10px; }");
    titleLabel->setAlignment(Qt::AlignCenter);

    // Scroll area for content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollWidget = new QWidget();
    scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(3);
    scrollLayout->setContentsMargins(2, 2, 2, 2);

    // Create sections
    createAttributeSection();
    // createCarrierSection();
    createPositionSection();
    createSpeedAltTableSection();
    createTrackSection();
    // createActiveSection();
    createEquipmentSection();
    createOptionsSection();

    scrollArea->setWidget(scrollWidget);


    closeButton = new QPushButton("Close");
    closeButton->setStyleSheet(
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "padding: 8px 16px; "
        "border: none; "
        "border-radius: 4px; "
        "font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "background-color: #2980b9; "
        "}"
        );

    // Add widgets to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &EntityInfoDialog::onCloseClicked);
}

void EntityInfoDialog::createAttributeSection()
{
    // Create table for attributes
    attributeTable = new QTableWidget();
    attributeTable->setColumnCount(2);
    attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
    attributeTable->setRowCount(5); // 5 rows: Type, Name, DIS name, Damages, Carrier

    // Simple header style - no dark colors
    attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
    attributeTable->verticalHeader()->setVisible(false);
    attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // No scrollbars
    attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attributeTable->setFixedHeight(200); // Height increased for 5 rows
    attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Column widths
    attributeTable->horizontalHeader()->setStretchLastSection(true);
    attributeTable->setColumnWidth(0, 150);

    // Add rows - Type, Name, DIS name, Damages, Carrier
    attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
    attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(1, 0, new QTableWidgetItem("Name"));
    attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(2, 0, new QTableWidgetItem("DIS name"));
    attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(3, 0, new QTableWidgetItem("Damages"));
    attributeTable->setItem(3, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(4, 0, new QTableWidgetItem("Carrier"));
    attributeTable->setItem(4, 1, new QTableWidgetItem("-"));

    // Simple table style
    attributeTable->setStyleSheet(
        "QTableWidget { "
        "gridline-color: #cccccc; "
        "background-color: white; "
        "border: 1px solid #cccccc; "
        "}"
        "QTableWidget::item { "
        "padding: 5px; "
        "border-bottom: 1px solid #f0f0f0; "
        "}"
        );

    scrollLayout->addWidget(attributeTable);
}

void EntityInfoDialog::createPositionSection()
{
    positionLabel = new QLabel("Position: -");
    positionLabel->setStyleSheet("QLabel { padding: 8px; color: #000000; }");
    positionLabel->setMinimumHeight(35);
    scrollLayout->addWidget(positionLabel);
}

void EntityInfoDialog::createSpeedAltTableSection()
{
    // Create table
    speedAltTable = new QTableWidget();
    speedAltTable->setColumnCount(3);
    speedAltTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Current" << "Requested");
    speedAltTable->setRowCount(2);

    // Simple header style
    speedAltTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
    speedAltTable->verticalHeader()->setVisible(false);
    speedAltTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    speedAltTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    speedAltTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // No scrollbars
    speedAltTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    speedAltTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Height calculation
    int rowHeight = 40;
    int headerHeight = speedAltTable->horizontalHeader()->height();
    int totalHeight = (speedAltTable->rowCount() * rowHeight) + headerHeight + 5;

    speedAltTable->setFixedHeight(totalHeight);
    speedAltTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Column widths
    speedAltTable->horizontalHeader()->setStretchLastSection(true);
    speedAltTable->setColumnWidth(0, 100);

    // Add rows
    speedAltTable->setItem(0, 0, new QTableWidgetItem("Speed"));
    speedAltTable->setItem(0, 1, new QTableWidgetItem("-"));
    speedAltTable->setItem(0, 2, new QTableWidgetItem("-"));

    speedAltTable->setItem(1, 0, new QTableWidgetItem("Altitude"));
    speedAltTable->setItem(1, 1, new QTableWidgetItem("-"));
    speedAltTable->setItem(1, 2, new QTableWidgetItem("-"));

    // Simple table style
    speedAltTable->setStyleSheet(
        "QTableWidget { "
        "gridline-color: #cccccc; "
        "background-color: white; "
        "border: 1px solid #cccccc; "
        "}"
        "QTableWidget::item { "
        "padding: 5px; "
        "border-bottom: 1px solid #f0f0f0; "
        "}"
        );

    scrollLayout->addWidget(speedAltTable);
}

void EntityInfoDialog::createTrackSection()
{
    // Horizontal layout for one line
    QHBoxLayout *trackLayout = new QHBoxLayout();
    trackLayout->setSpacing(0);

    trackCheckBox = new QCheckBox("Track");
    centreCheckBox = new QCheckBox("Center");
    aggregatedScriptCheckBox = new QCheckBox("Aggregated");

    // Simple checkbox style
    QString checkboxStyle = "QCheckBox { color: #000000; padding: 8px; }";
    trackCheckBox->setStyleSheet(checkboxStyle);
    centreCheckBox->setStyleSheet(checkboxStyle);
    aggregatedScriptCheckBox->setStyleSheet(checkboxStyle);

    // Add to horizontal layout with equal stretch
    trackLayout->addWidget(trackCheckBox);
    trackLayout->addWidget(centreCheckBox);
    trackLayout->addWidget(aggregatedScriptCheckBox);

    // Set equal width for all checkboxes
    trackLayout->setStretch(0, 1);
    trackLayout->setStretch(1, 1);
    trackLayout->setStretch(2, 1);

    QWidget *trackWidget = new QWidget();
    trackWidget->setLayout(trackLayout);
    scrollLayout->addWidget(trackWidget);
}


void EntityInfoDialog::createEquipmentSection()
{
    equipmentLayout = new QGridLayout();
    equipmentLayout->setSpacing(3);

    // Aapke existing buttons
    sensorsButton = new QPushButton("Sensors");
    radiosButton = new QPushButton("Radios");
    iffButton = new QPushButton("IFF");
    weaponsButton = new QPushButton("Weapons");
    formationButton = new QPushButton("Formation");

    // Simple button style - light gray
    QString buttonStyle =
        "QPushButton { "
        "background-color: #f0f0f0; "
        "color: #000000; "
        "padding: 8px 12px; "
        "border: 1px solid #cccccc; "
        "border-radius: 3px; "
        "margin: 2px; "
        "}"
        "QPushButton:hover { "
        "background-color: #e0e0e0; "
        "}";

    sensorsButton->setStyleSheet(buttonStyle);
    radiosButton->setStyleSheet(buttonStyle);
    iffButton->setStyleSheet(buttonStyle);
    weaponsButton->setStyleSheet(buttonStyle);
    formationButton->setStyleSheet(buttonStyle);

    weaponsButton->setMinimumHeight(35);
    sensorsButton->setMinimumHeight(35);
    formationButton->setMinimumHeight(35);
    radiosButton->setMinimumHeight(35);
    iffButton->setMinimumHeight(35);

    // Aapka existing layout
    equipmentLayout->addWidget(sensorsButton, 0, 0);
    equipmentLayout->addWidget(radiosButton, 0, 1);
    equipmentLayout->addWidget(iffButton, 1, 0);
    equipmentLayout->addWidget(formationButton, 1, 1);
    equipmentLayout->addWidget(weaponsButton, 2, 0);

    // Connect signals - waise hi
    connect(weaponsButton, &QPushButton::clicked, this, &EntityInfoDialog::onWeaponsClicked);
    connect(sensorsButton, &QPushButton::clicked, this, &EntityInfoDialog::onSensorsClicked);
    connect(formationButton, &QPushButton::clicked, this, &EntityInfoDialog::onFormationClicked);
    connect(radiosButton, &QPushButton::clicked, this, &EntityInfoDialog::onRadiosClicked);
    connect(iffButton, &QPushButton::clicked, this, &EntityInfoDialog::onIFFClicked);

    QWidget *equipmentWidget = new QWidget();
    equipmentWidget->setLayout(equipmentLayout);
    scrollLayout->addWidget(equipmentWidget);
}

void EntityInfoDialog::createOptionsSection()
{
    // Main vertical layout
    optionsLayout = new QVBoxLayout();
    optionsLayout->setSpacing(2);

    // FIRST LINE: Active, Follow Trajectory, Show Detection
    QHBoxLayout *firstLineLayout = new QHBoxLayout();
    firstLineLayout->setSpacing(10);

    // Active checkbox - NEW
    QCheckBox *activeCheckBox = new QCheckBox("Active");
    followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
    showDetectionCheckBox = new QCheckBox("Show Detection");

    // Simple checkbox style
    QString checkboxStyle = "QCheckBox { color: #000000; }";
    activeCheckBox->setStyleSheet(checkboxStyle);
    followTrajectoryCheckBox->setStyleSheet(checkboxStyle);
    showDetectionCheckBox->setStyleSheet(checkboxStyle);

    // Add to first line
    firstLineLayout->addWidget(activeCheckBox);
    firstLineLayout->addWidget(followTrajectoryCheckBox);
    firstLineLayout->addWidget(showDetectionCheckBox);
    firstLineLayout->addStretch(); // Push to left

    // SECOND LINE: Show Connection, Freeze Motion
    QHBoxLayout *secondLineLayout = new QHBoxLayout();
    secondLineLayout->setSpacing(10);

    showConnectionCheckBox = new QCheckBox("Show Connection");
    QCheckBox *freezeMotionCheckBox = new QCheckBox("Freeze Motion");

    showConnectionCheckBox->setStyleSheet(checkboxStyle);
    freezeMotionCheckBox->setStyleSheet(checkboxStyle);

    // Add to second line
    secondLineLayout->addWidget(showConnectionCheckBox);
    secondLineLayout->addWidget(freezeMotionCheckBox);
    secondLineLayout->addStretch(); // Push to left

    // Add both lines to main layout
    optionsLayout->addLayout(firstLineLayout);
    optionsLayout->addLayout(secondLineLayout);

    showDetectionCheckBox->setChecked(true);

    // Aapka existing connection
    connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
        if(!currentEntityId.isEmpty()){
            if(entryInfo){
                entryInfo->detection = checked;
                entryInfo->radioVisible = checked;
                emit update();
            }
        }
    });

    QWidget *optionsWidget = new QWidget();
    optionsWidget->setLayout(optionsLayout);
    scrollLayout->addWidget(optionsWidget);
}
void EntityInfoDialog::setEntityInfo(const QString& entityId,  MeshEntry* info)
{
    currentEntityId = entityId;
    entryInfo = info;

    if (!entryInfo) {
        titleLabel->setText("Entity: No Data");
        clearInfo();
        return;
    }

    titleLabel->setText("Entity: " + entityId);


    if (entryInfo->trajectory) {
        followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
    } else {
        followTrajectoryCheckBox->setChecked(false);
    }

    if (entryInfo->entity) {
        entryInfo->detection = showDetectionCheckBox->isChecked();
    }
}

// void EntityInfoDialog::updateEntityInfo(){
//     if(currentEntityId.isEmpty()) return;
//     if(entryInfo){
//         if(entryInfo->entity){
//             // Convert entity type to string
//             QString typeStr = "Unknown";
//             switch(entryInfo->entity->type) {
//             case Constants::EntityType::Platform: typeStr = "Platform"; break;
//             case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
//             case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
//             default: typeStr = "Unknown"; break;
//             }

//             // Update attribute table
//             if (attributeTable) {
//                 attributeTable->item(0, 1)->setText(typeStr);
//                 attributeTable->item(1, 1)->setText(entryInfo->name);
//                 attributeTable->item(2, 1)->setText("-");
//                 attributeTable->item(3, 1)->setText("0 %");
//                 attributeTable->item(4, 1)->setText("Not Embarked");
//             }

//             // Get current values
//             float lat = entryInfo->coreTransform->getLatitude();
//             float lon = entryInfo->coreTransform->getLongitude();
//             float x = entryInfo->transform->translation().x();
//             float z = entryInfo->transform->translation().z();

//             // IMPORTANT: Get altitude from TRAJECTORY, not transform
//             float y = 0.0f; // Default altitude

//             // Debug: Check what's in trajectory
//             if (entryInfo->trajectory) {


//                 // Check if we have waypoints
//                 if (!entryInfo->trajectory->Trajectories.empty()) {
//                     // If current index is valid, use that waypoint's altitude
//                     if (entryInfo->trajectory->current >= 0 &&
//                         entryInfo->trajectory->current < entryInfo->trajectory->Trajectories.size()) {

//                         Waypoints* wp = entryInfo->trajectory->Trajectories[entryInfo->trajectory->current];
//                         if (wp && wp->position) {
//                             y = wp->position->y; // This is the altitude from trajectory

//                         }
//                     }
//                     // Otherwise use first waypoint's altitude
//                     else {
//                         Waypoints* wp = entryInfo->trajectory->Trajectories[0];
//                         if (wp && wp->position) {
//                             y = wp->position->y;

//                         }
//                     }

//                     // Debug all waypoints
//                     for (int i = 0; i < entryInfo->trajectory->Trajectories.size(); i++) {
//                         Waypoints* wp = entryInfo->trajectory->Trajectories[i];
//                         if (wp && wp->position) {

//                         }
//                     }
//                 }

//             }
//             else {

//             }

//             // Update position display
//             QString currentPos = QString("Lat: %1, Long: %2, X: %3, Z: %4")
//                                      .arg(lat, 0, 'f', 6)
//                                      .arg(lon, 0, 'f', 6)
//                                      .arg(x, 0, 'f', 2)
//                                      .arg(z, 0, 'f', 2);

//             positionLabel->setText("Position: " + currentPos);

//             if (speedAltTable) {
//                 // Speed update
//                 if (entryInfo->dynamicModel) {
//                     QString RequstSpeed = QString("%1 km/h").arg((entryInfo->dynamicModel->moveSpeed), 0, 'f', 2);
//                     QString currentSpeed = QString("%1 km/h").arg((entryInfo->dynamicModel->currentSpeed), 0, 'f', 2);
//                     speedAltTable->item(0, 1)->setText(currentSpeed);
//                     speedAltTable->item(0, 2)->setText(RequstSpeed);
//                 } else {
//                     speedAltTable->item(0, 1)->setText("-");
//                     speedAltTable->item(0, 2)->setText("-");
//                 }

//                 // Altitude - FROM TRAJECTORY
//                 QString Altitude = QString("%1 ft").arg(entryInfo->dynamicModel->Altitude, 0, 'f', 2);
//                 QString currentAltitude = QString("%1 ft").arg((entryInfo->dynamicModel->currentAltitude * 3280.84f), 0, 'f', 2);
//                 speedAltTable->item(1, 1)->setText(currentAltitude);
//                 speedAltTable->item(1, 2)->setText(Altitude);


//             }
//         }
//     }
// }

// void EntityInfoDialog::updateEntityInfo(){
// void EntityInfoDialog::updateEntityInfo() {
//     qDebug() << "=== DEBUG UPDATE ENTITY INFO ===";
//     qDebug() << "Entity ID:" << currentEntityId;
//     qDebug() << "entryInfo:" << (entryInfo ? "NOT NULL" : "NULL");

//     if(entryInfo) {
//         qDebug() << "dynamicModel:" << (entryInfo->dynamicModel ? "NOT NULL" : "NULL");
//         if(entryInfo->dynamicModel) {
//             qDebug() << "  moveSpeed:" << entryInfo->dynamicModel->moveSpeed;
//             qDebug() << "  Altitude:" << entryInfo->dynamicModel->Altitude;
//         }
//         qDebug() << "platform:" << (entryInfo->platform ? "NOT NULL" : "NULL");
//         if(entryInfo->platform && entryInfo->platform->dynamicModel) {
//             qDebug() << "  Platform moveSpeed:" << entryInfo->platform->dynamicModel->moveSpeed;
//             qDebug() << "  Platform Altitude:" << entryInfo->platform->dynamicModel->Altitude;
//         }
//     }
//     if(currentEntityId.isEmpty()) return;
//     if(entryInfo){
//         if(entryInfo->entity){
//             // Convert entity type to string
//             QString typeStr = "Unknown";
//             switch(entryInfo->entity->type) {
//             case Constants::EntityType::Platform: typeStr = "Platform"; break;
//             case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
//             case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
//             default: typeStr = "Unknown"; break;
//             }

//             // Update attribute table
//             if (attributeTable) {
//                 attributeTable->item(0, 1)->setText(typeStr);
//                 attributeTable->item(1, 1)->setText(entryInfo->name);
//                 attributeTable->item(2, 1)->setText("-");
//                 attributeTable->item(3, 1)->setText("0 %");
//                 attributeTable->item(4, 1)->setText("Not Embarked");
//             }

//             // Get current values
//             float lat = entryInfo->coreTransform->getLatitude();
//             float lon = entryInfo->coreTransform->getLongitude();
//             float x = entryInfo->transform->translation().x();
//             float z = entryInfo->transform->translation().z();

//             // Get altitude from trajectory
//             float y = 0.0f; // Default altitude

//             if (entryInfo->trajectory) {
//                 // Check if we have waypoints
//                 if (!entryInfo->trajectory->Trajectories.empty()) {
//                     // If current index is valid, use that waypoint's altitude
//                     if (entryInfo->trajectory->current >= 0 &&
//                         entryInfo->trajectory->current < entryInfo->trajectory->Trajectories.size()) {

//                         Waypoints* wp = entryInfo->trajectory->Trajectories[entryInfo->trajectory->current];
//                         if (wp && wp->position) {
//                             y = wp->position->y; // This is the altitude from trajectory
//                         }
//                     }
//                     // Otherwise use first waypoint's altitude
//                     else {
//                         Waypoints* wp = entryInfo->trajectory->Trajectories[0];
//                         if (wp && wp->position) {
//                             y = wp->position->y;
//                         }
//                     }
//                 }
//             }

//             // Update position display
//             QString currentPos = QString("Lat: %1, Long: %2, X: %3, Z: %4")
//                                      .arg(lat, 0, 'f', 6)
//                                      .arg(lon, 0, 'f', 6)
//                                      .arg(x, 0, 'f', 2)
//                                      .arg(z, 0, 'f', 2);

//             positionLabel->setText("Position: " + currentPos);

//             if (speedAltTable) {
//                 // Speed update - WITH NULL CHECK
//                 if (entryInfo->dynamicModel) {
//                     QString RequstSpeed = QString("%1 km/h").arg((entryInfo->dynamicModel->moveSpeed), 0, 'f', 2);
//                     QString currentSpeed = QString("%1 km/h").arg((entryInfo->dynamicModel->currentSpeed), 0, 'f', 2);
//                     speedAltTable->item(0, 1)->setText(currentSpeed);
//                     speedAltTable->item(0, 2)->setText(RequstSpeed);
//                 } else {
//                     speedAltTable->item(0, 1)->setText("-");
//                     speedAltTable->item(0, 2)->setText("-");
//                 }

//                 // Altitude - WITH NULL CHECK
//                 if (entryInfo->dynamicModel) {
//                     QString Altitude = QString("%1 ft").arg(entryInfo->dynamicModel->Altitude, 0, 'f', 2);
//                     QString currentAltitude = QString("%1 ft").arg((entryInfo->dynamicModel->currentAltitude * 3280.84f), 0, 'f', 2);
//                     speedAltTable->item(1, 1)->setText(currentAltitude);
//                     speedAltTable->item(1, 2)->setText(Altitude);
//                 } else {
//                     speedAltTable->item(1, 1)->setText("-");
//                     speedAltTable->item(1, 2)->setText("-");
//                 }
//             }

//             // Update follow trajectory checkbox - WITH NULL CHECK
//             if (followTrajectoryCheckBox) {
//                 if (entryInfo->trajectory) {
//                     followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
//                 } else {
//                     followTrajectoryCheckBox->setChecked(false);
//                 }
//             }

//             // Update show detection checkbox - WITH NULL CHECK
//             if (showDetectionCheckBox) {
//                 if (entryInfo->entity) {
//                     entryInfo->detection = showDetectionCheckBox->isChecked();
//                 }
//             }
//         }
//     }
// }

void EntityInfoDialog::updateEntityInfo() {
    if(currentEntityId.isEmpty()) {
        // qDebug() << "updateEntityInfo: No entity selected";
        return;
    }

    if(!entryInfo) {
        qDebug() << "updateEntityInfo: entryInfo is null for entity" << currentEntityId;
        clearInfo();
        return;
    }

    qDebug() << "=== DEBUG UPDATE ENTITY INFO ===";
    qDebug() << "Entity ID:" << currentEntityId;
    qDebug() << "entryInfo:" << (entryInfo ? "NOT NULL" : "NULL");

    // Get dynamicModel pointer - pehle entry se, phir platform se
    DynamicModel* dynModel = nullptr;

    if(entryInfo->dynamicModel) {
        dynModel = entryInfo->dynamicModel;
        qDebug() << "Using dynamicModel from entryInfo";
    } else if(entryInfo->platform && entryInfo->platform->dynamicModel) {
        dynModel = entryInfo->platform->dynamicModel;
        qDebug() << "Using dynamicModel from platform";
    } else {
        qDebug() << "No dynamicModel found anywhere";
    }

    // Debug platform data
    if(entryInfo->platform) {
        qDebug() << "platform:" << "NOT NULL";
        qDebug() << "Platform dynamicModel:" << (entryInfo->platform->dynamicModel ? "NOT NULL" : "NULL");
        if(entryInfo->platform->dynamicModel) {
            qDebug() << "  Platform moveSpeed:" << entryInfo->platform->dynamicModel->moveSpeed;
            qDebug() << "  Platform Altitude:" << entryInfo->platform->dynamicModel->Altitude;
        }
    }

    // Convert entity type to string
    QString typeStr = "Unknown";
    if(entryInfo->entity) {
        switch(entryInfo->entity->type) {
        case Constants::EntityType::Platform: typeStr = "Platform"; break;
        case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
        case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
        default: typeStr = "Unknown"; break;
        }
    }

    // Update attribute table
    if (attributeTable) {
        attributeTable->item(0, 1)->setText(typeStr);
        attributeTable->item(1, 1)->setText(entryInfo->name);
        attributeTable->item(2, 1)->setText("-");
        attributeTable->item(3, 1)->setText("0 %");
        attributeTable->item(4, 1)->setText("Not Embarked");
    }

    // Get current values with null checks
    float lat = 0.0f, lon = 0.0f, x = 0.0f, z = 0.0f;

    if(entryInfo->coreTransform) {
        lat = entryInfo->coreTransform->getLatitude();
        lon = entryInfo->coreTransform->getLongitude();
    }

    if(entryInfo->transform) {
        x = entryInfo->transform->translation().x();
        z = entryInfo->transform->translation().z();
    }

    // Get altitude from trajectory
    float y = 0.0f; // Default altitude

    if (entryInfo->trajectory && !entryInfo->trajectory->Trajectories.empty()) {
        // If current index is valid, use that waypoint's altitude
        if (entryInfo->trajectory->current >= 0 &&
            entryInfo->trajectory->current < entryInfo->trajectory->Trajectories.size()) {

            Waypoints* wp = entryInfo->trajectory->Trajectories[entryInfo->trajectory->current];
            if (wp && wp->position) {
                y = wp->position->y;
            }
        }
        // Otherwise use first waypoint's altitude
        else {
            Waypoints* wp = entryInfo->trajectory->Trajectories[0];
            if (wp && wp->position) {
                y = wp->position->y;
            }
        }
    }

    // Update position display
    QString currentPos = QString("Lat: %1, Long: %2, X: %3, Z: %4")
                             .arg(lat, 0, 'f', 6)
                             .arg(lon, 0, 'f', 6)
                             .arg(x, 0, 'f', 2)
                             .arg(z, 0, 'f', 2);

    positionLabel->setText("Position: " + currentPos);

    if (speedAltTable) {
        // Speed update - Check dynModel (which could be from entryInfo OR platform)
        if (dynModel) {
            QString requestSpeed = QString("%1 km/h").arg(dynModel->moveSpeed, 0, 'f', 2);
            QString currentSpeed = QString("%1 km/h").arg(dynModel->currentSpeed, 0, 'f', 2);
            speedAltTable->item(0, 1)->setText(currentSpeed);
            speedAltTable->item(0, 2)->setText(requestSpeed);
        } else {
            // Display default values if no dynamicModel found
            speedAltTable->item(0, 1)->setText("0.0 km/h");
            speedAltTable->item(0, 2)->setText("0.0 km/h");
        }

        // Altitude - Same logic
        if (dynModel) {
            QString requestAltitude = QString("%1 ft").arg(dynModel->Altitude, 0, 'f', 2);
            QString currentAltitude = QString("%1 ft").arg((dynModel->currentAltitude * KMtoFT), 0, 'f', 2);
            speedAltTable->item(1, 1)->setText(currentAltitude);
            speedAltTable->item(1, 2)->setText(requestAltitude);
        } else {
            // Display default values
            speedAltTable->item(1, 1)->setText("0.0 ft");
            speedAltTable->item(1, 2)->setText("0.0 ft");
        }
    }

    // Update follow trajectory checkbox
    if (followTrajectoryCheckBox) {
        if (entryInfo->trajectory) {
            followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
        } else {
            followTrajectoryCheckBox->setChecked(false);
        }
    }

    // Update show detection checkbox
    if (showDetectionCheckBox) {
        if (entryInfo->entity) {
            entryInfo->detection = showDetectionCheckBox->isChecked();
        }
    }
}
void EntityInfoDialog::clearInfo()
{
    titleLabel->setText("Entity Information");
    currentEntityId.clear();
    currentEntityData.clear();


    if (attributeTable) {
        attributeTable->item(0, 1)->setText("-"); // Type
        attributeTable->item(1, 1)->setText("-"); // Name
        attributeTable->item(2, 1)->setText("-"); // DIS name
        attributeTable->item(3, 1)->setText("-"); // Damages
        attributeTable->item(4, 1)->setText("-"); // Carrier
    }

    // Carrier section ab table mein hai, isliye hide karen
    carrierLabel->setText("");



    positionLabel->setText("Position: -");

    // Reset Speed and Altitude table
    if (speedAltTable) {
        speedAltTable->item(0, 1)->setText("-");
        speedAltTable->item(0, 2)->setText("-");
        speedAltTable->item(1, 1)->setText("-");
        speedAltTable->item(1, 2)->setText("-");
    }

    // Uncheck all checkboxes EXCEPT Show Detection
    trackCheckBox->setChecked(false);
    centreCheckBox->setChecked(false);
    aggregatedScriptCheckBox->setChecked(false);
    activeCheckBox->setChecked(false);
    followTrajectoryCheckBox->setChecked(false);
    showConnectionCheckBox->setChecked(false);
    showDetectionCheckBox->setChecked(true);
    controlDecisiveCheckBox->setChecked(false);
}
void EntityInfoDialog::onCloseClicked()
{
    hide();
}

void EntityInfoDialog::onWeaponsClicked()
{
    // Show weapons list popup
    QMessageBox::information(this, "Weapons",
                             QString("Weapons for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("weapons", "No weapons data").toString()));
}

void EntityInfoDialog::onSensorsClicked()
{
    if(!currentEntityId.isEmpty()){
        if(entryInfo){
            // Platform check करें (Radios और IFF की तरह)
            if(entryInfo->platform){
                qDebug() << "Opening sensors dialog for platform:" << currentEntityId;

                // Create a simple clean dialog for sensors
                QDialog *sensorsDialog = new QDialog(this);
                sensorsDialog->setWindowTitle("Sensors - " + currentEntityId);
                sensorsDialog->setMinimumSize(450, 300);
                sensorsDialog->setMaximumSize(500, 400);

                // Simple window flags
                sensorsDialog->setWindowFlags(Qt::Dialog);

                // Minimal styling
                sensorsDialog->setStyleSheet(
                    "QDialog { background-color: white; border: 1px solid #ccc; }"
                    "QLabel { color: #333; font-weight: normal; }"
                    "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
                    "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
                    "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
                    "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
                    "QPushButton:hover { background-color: #005a9e; }"
                    );

                QVBoxLayout *layout = new QVBoxLayout(sensorsDialog);
                layout->setSpacing(8);
                layout->setContentsMargins(10, 10, 10, 10);

                // Simple title
                QLabel *titleLabel = new QLabel("Sensor Systems");
                titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
                layout->addWidget(titleLabel);

                // Create clean table
                QTableWidget *sensorsTable = new QTableWidget();
                sensorsTable->setColumnCount(4);
                sensorsTable->setHorizontalHeaderLabels(QStringList()
                                                        << "Name" << "Type" << "Range" << "FOV");

                sensorsTable->verticalHeader()->setVisible(false);
                sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
                sensorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
                sensorsTable->setSelectionMode(QAbstractItemView::SingleSelection);

                // Clean table properties
                sensorsTable->setShowGrid(true);
                sensorsTable->setAlternatingRowColors(false);

                // Real-time update timer - Platform pass करें
                QTimer *sensorsUpdateTimer = new QTimer(sensorsDialog);
                QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
                    updateSensorsTable(sensorsTable, entryInfo->platform);
                });
                sensorsUpdateTimer->start(100);

                // Initial population
                updateSensorsTable(sensorsTable, entryInfo->platform);

                // Simple column sizing
                sensorsTable->horizontalHeader()->setStretchLastSection(false);
                sensorsTable->setColumnWidth(0, 120);
                sensorsTable->setColumnWidth(1, 80);
                sensorsTable->setColumnWidth(2, 80);
                sensorsTable->setColumnWidth(3, 60);

                layout->addWidget(sensorsTable);

                // Simple summary
                QLabel *summaryLabel = new QLabel();
                summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
                layout->addWidget(summaryLabel);


                QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
                    int total = 0;
                    if(entryInfo && entryInfo->platform) {
                        Platform* platform = dynamic_cast<Platform*>(entryInfo->platform);
                        if(platform && platform->sensors && platform->sensors->sensors) {
                            total = platform->sensors->sensors->size();
                        }
                    }
                    summaryLabel->setText(QString("Total: %1 sensors").arg(total));
                });

                // Simple close button
                QHBoxLayout *buttonLayout = new QHBoxLayout();
                buttonLayout->addStretch();
                QPushButton *closeButton = new QPushButton("Close");
                QObject::connect(closeButton, &QPushButton::clicked, sensorsDialog, [=]() {
                    sensorsUpdateTimer->stop();
                    sensorsDialog->close();
                });
                buttonLayout->addWidget(closeButton);
                layout->addLayout(buttonLayout);

                // Show dialog
                sensorsDialog->show();
                sensorsDialog->setAttribute(Qt::WA_DeleteOnClose);

            } else {
                QMessageBox::information(this, "Sensors", "No platform data available.");
                qDebug() << "No platform found for entity:" << currentEntityId;
            }
        }
    } else {
        QMessageBox::information(this, "Sensors", "No entity selected.");
    }
}

void EntityInfoDialog::updateSensorsTable(QTableWidget* sensorsTable, Entity* platform)
{
    if(!sensorsTable || !platform) {
        qDebug() << "updateSensorsTable: Invalid input parameters!";
        return;
    }

    sensorsTable->setRowCount(0);
    int row = 0;


    Platform* platformPtr = dynamic_cast<Platform*>(platform);
    if(!platformPtr) {
        qDebug() << "updateSensorsTable: Failed to cast to Platform!";
        return;
    }


    if(platformPtr->sensors && platformPtr->sensors->sensors) {
        // Debug log
        qDebug() << "Sensors map size:" << platformPtr->sensors->sensors->size();


        for (const auto& pair : *platformPtr->sensors->sensors) {
            Sensor* sensor = pair.second;

            if(!sensor) {
                qDebug() << "Found null sensor in map!";
                continue;
            }

            qDebug() << "Processing sensor:" << QString::fromStdString(sensor->Name)
                     << "Type:" << sensor->subTypeToString(sensor->subType);

            // Insert new row
            sensorsTable->insertRow(row);

            // 1. Name
            QString sensorName = QString::fromStdString(sensor->Name);
            sensorsTable->setItem(row, 0, new QTableWidgetItem(sensorName));

            // 2. Type
            QString typeStr = "Unknown";
            switch(sensor->subType) {
            case Sensor::SubType::Generic:
                typeStr = "Radar";
                break;
            case Sensor::SubType::CSM:
                typeStr = "CSM";
                break;
            case Sensor::SubType::ESM:
                typeStr = "ESM";
                break;
            default:
                typeStr = "Other";
                break;
            }
            sensorsTable->setItem(row, 1, new QTableWidgetItem(typeStr));

            // 3. Range
            QString rangeStr = "-";
            switch(sensor->subType) {
            case Sensor::SubType::Generic:
                if(sensor->range > 0) {
                    rangeStr = QString("%1 km").arg(sensor->range, 0, 'f', 1);
                }
                break;
            case Sensor::SubType::CSM:
                if(sensor->range > 0) {
                    rangeStr = QString("%1 km").arg(sensor->range, 0, 'f', 1);
                }
                break;
            case Sensor::SubType::ESM:
                if(sensor->range > 0) {
                    rangeStr = QString("%1 km").arg(sensor->range, 0, 'f', 1);
                }
                break;
            default:
                rangeStr = "-";
                break;
            }
            sensorsTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

            // 4. FOV (Field of View)
            QString fovStr = "-";
            if(sensor->subType == Sensor::SubType::Generic) {
                if(sensor->maxDetectionAngle > 0) {
                    fovStr = QString("%1°").arg(sensor->maxDetectionAngle, 0, 'f', 0);
                }
            }
            sensorsTable->setItem(row, 3, new QTableWidgetItem(fovStr));

            row++;
        }
    } else {
        qDebug() << "No sensors profile found or sensors map is null!";
        qDebug() << "Platform sensors pointer:" << platformPtr->sensors;
        if(platformPtr->sensors) {
            qDebug() << "Sensors map pointer:" << platformPtr->sensors->sensors;
        }
    }


    if (row == 0) {
        sensorsTable->setRowCount(1);
        sensorsTable->setItem(0, 0, new QTableWidgetItem("No sensors available"));
        sensorsTable->setItem(0, 1, new QTableWidgetItem(""));
        sensorsTable->setItem(0, 2, new QTableWidgetItem(""));
        sensorsTable->setItem(0, 3, new QTableWidgetItem(""));

        qDebug() << "No sensors found in platform!";
    } else {
        qDebug() << "Successfully displayed" << row << "sensors";
    }
}
void EntityInfoDialog::onFormationClicked()
{
    // Show formation details popup
    QMessageBox::information(this, "Formation",
                             QString("Formation for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("formation", "No formation data").toString()));
}



void EntityInfoDialog::onRadiosClicked()
{
    if(!currentEntityId.isEmpty()){
        if(entryInfo){
            if(entryInfo->entity){
                // Create a simple clean dialog for radios - EXACTLY LIKE SENSORS
                QDialog *radiosDialog = new QDialog(this);
                radiosDialog->setWindowTitle("Radios - " + currentEntityId);
                radiosDialog->setMinimumSize(300, 300);
                radiosDialog->setMaximumSize(400, 400);

                radiosDialog->setWindowFlags(Qt::Dialog);
                radiosDialog->setStyleSheet(
                    "QDialog { background-color: white; border: 1px solid #ccc; }"
                    "QLabel { color: #333; font-weight: normal; }"
                    "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
                    "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
                    "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
                    "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
                    "QPushButton:hover { background-color: #005a9e; }"
                    );

                QVBoxLayout *layout = new QVBoxLayout(radiosDialog);
                layout->setSpacing(8);
                layout->setContentsMargins(10, 10, 10, 10);

                QLabel *titleLabel = new QLabel("Radio Communication Systems");
                titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
                layout->addWidget(titleLabel);

                QTableWidget *radiosTable = new QTableWidget();
                radiosTable->setColumnCount(2);
                radiosTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Range");

                radiosTable->verticalHeader()->setVisible(false);
                radiosTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
                radiosTable->setSelectionBehavior(QAbstractItemView::SelectRows);
                radiosTable->setSelectionMode(QAbstractItemView::SingleSelection);
                radiosTable->setShowGrid(true);
                radiosTable->setAlternatingRowColors(false);

                // Real-time update timer - ALL CODE INSIDE LAMBDA
                QTimer *radiosUpdateTimer = new QTimer(radiosDialog);
                QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
                    if(!radiosTable || !entryInfo->platform) return;

                    radiosTable->setRowCount(0);
                    int row = 0;

                    for (auto pair: *entryInfo->platform->radios->radios) {
                        Radio* radio = pair.second;
                        radiosTable->insertRow(row);

                        // NAME - First field
                        QString radioName = QString::fromStdString(radio->Name);
                        radiosTable->setItem(row, 0, new QTableWidgetItem(radioName));

                        // RANGE - Second field
                        QString rangeStr = "N/A";
                        if (radio->Range > 0) {
                            rangeStr = QString("%1 km").arg(radio->Range, 0, 'f', 1);
                        }
                        radiosTable->setItem(row, 1, new QTableWidgetItem(rangeStr));

                        row++;
                    }

                    if (row == 0) {
                        radiosTable->setRowCount(1);
                        radiosTable->setItem(0, 0, new QTableWidgetItem("No radios"));
                        radiosTable->setItem(0, 1, new QTableWidgetItem(""));
                    }
                });
                radiosUpdateTimer->start(100);

                radiosTable->horizontalHeader()->setStretchLastSection(false);
                radiosTable->setColumnWidth(0, 120);
                radiosTable->setColumnWidth(1, 80);

                layout->addWidget(radiosTable);

                QLabel *summaryLabel = new QLabel();
                summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
                layout->addWidget(summaryLabel);

                QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
                    int total = 0;
                    if(entryInfo && entryInfo->platform) {
                        total = entryInfo->platform->radios->radios->size();
                    }
                    summaryLabel->setText(QString("Total: %1 radios").arg(total));
                });

                QHBoxLayout *buttonLayout = new QHBoxLayout();
                buttonLayout->addStretch();
                QPushButton *closeButton = new QPushButton("Close");
                QObject::connect(closeButton, &QPushButton::clicked, radiosDialog, [=]() {
                    radiosUpdateTimer->stop();
                    radiosDialog->close();
                });
                buttonLayout->addWidget(closeButton);
                layout->addLayout(buttonLayout);

                radiosDialog->show();
                radiosDialog->setAttribute(Qt::WA_DeleteOnClose);
            }
        }
    } else {
        QMessageBox::information(this, "Radios", "No entity selected.");
    }
}



void EntityInfoDialog::onIFFClicked()
{
    if(!currentEntityId.isEmpty()){
        if(entryInfo){
            if(entryInfo->entity){
                // Create a simple clean dialog for IFF - EXACTLY LIKE SENSORS AND RADIOS
                QDialog *iffDialog = new QDialog(this);
                iffDialog->setWindowTitle("IFF Systems - " + currentEntityId);
                iffDialog->setMinimumSize(450, 300);
                iffDialog->setMaximumSize(500, 400);

                iffDialog->setWindowFlags(Qt::Dialog);
                iffDialog->setStyleSheet(
                    "QDialog { background-color: white; border: 1px solid #ccc; }"
                    "QLabel { color: #333; font-weight: normal; }"
                    "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
                    "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
                    "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
                    "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
                    "QPushButton:hover { background-color: #005a9e; }"
                    );

                QVBoxLayout *layout = new QVBoxLayout(iffDialog);
                layout->setSpacing(8);
                layout->setContentsMargins(10, 10, 10, 10);

                QLabel *titleLabel = new QLabel("IFF (Identification Friend or Foe) Systems");
                titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
                layout->addWidget(titleLabel);

                QTableWidget *iffTable = new QTableWidget();
                iffTable->setColumnCount(3);
                iffTable->setHorizontalHeaderLabels(QStringList()
                                                    << "Name" << "Mode" << "Range");

                iffTable->verticalHeader()->setVisible(false);
                iffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
                iffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
                iffTable->setSelectionMode(QAbstractItemView::SingleSelection);
                iffTable->setShowGrid(true);
                iffTable->setAlternatingRowColors(false);

                // Real-time update timer
                QTimer *iffUpdateTimer = new QTimer(iffDialog);
                QObject::connect(iffUpdateTimer, &QTimer::timeout, iffDialog, [=]() {
                    if(!iffTable || !entryInfo->platform) return;

                    iffTable->setRowCount(0);
                    int row = 0;

                    for (auto pair : *entryInfo->platform->iffs->iffs) {
                        IFF* iff = pair.second;
                        iffTable->insertRow(row);

                        // NAME - First field
                        QString iffName = QString::fromStdString(iff->Name);
                        iffTable->setItem(row, 0, new QTableWidgetItem(iffName));

                        // MODE - Second field
                        QString modeStr = "Active";
                        switch(iff->operationalMode) {
                        case IFF::OperationalMode::Active: modeStr = "Active"; break;
                        case IFF::OperationalMode::Passive: modeStr = "Passive"; break;
                        case IFF::OperationalMode::Off: modeStr = "Off"; break;
                        case IFF::OperationalMode::Simulation: modeStr = "Simulation"; break;
                        default: modeStr = "Unknown"; break;
                        }
                        iffTable->setItem(row, 1, new QTableWidgetItem(modeStr));

                        // RANGE - Third field
                        QString rangeStr = QString("%1 km").arg(iff->emittingRange, 0, 'f', 1);
                        iffTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

                        row++;
                    }

                    if (row == 0) {
                        iffTable->setRowCount(1);
                        iffTable->setItem(0, 0, new QTableWidgetItem("No IFF systems"));
                        iffTable->setItem(0, 1, new QTableWidgetItem(""));
                        iffTable->setItem(0, 2, new QTableWidgetItem(""));
                    }
                });
                iffUpdateTimer->start(100);

                // Column sizing - same as sensors
                iffTable->horizontalHeader()->setStretchLastSection(false);
                iffTable->setColumnWidth(0, 120); // Name
                iffTable->setColumnWidth(1, 80);  // Mode
                iffTable->setColumnWidth(2, 80);  // Range
                layout->addWidget(iffTable);

                // Simple summary
                QLabel *summaryLabel = new QLabel();
                summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
                layout->addWidget(summaryLabel);

                // Update summary
                QObject::connect(iffUpdateTimer, &QTimer::timeout, iffDialog, [=]() {
                    int total = 0;
                    int active = 0;
                    if(entryInfo && entryInfo->entity) {
                        total = entryInfo->entity->iffList.size();
                        for (IFF* iff : entryInfo->entity->iffList) {
                            if(iff->operationalMode != IFF::OperationalMode::Off) {
                                active++;
                            }
                        }
                    }
                    summaryLabel->setText(QString("Total: %1 IFF systems | Active: %2").arg(total).arg(active));
                });

                // Simple close button
                QHBoxLayout *buttonLayout = new QHBoxLayout();
                buttonLayout->addStretch();
                QPushButton *closeButton = new QPushButton("Close");
                QObject::connect(closeButton, &QPushButton::clicked, iffDialog, [=]() {
                    iffUpdateTimer->stop();
                    iffDialog->close();
                });
                buttonLayout->addWidget(closeButton);
                layout->addLayout(buttonLayout);

                iffDialog->show();
                iffDialog->setAttribute(Qt::WA_DeleteOnClose);
            }
        }
    } else {
        QMessageBox::information(this, "IFF", "No entity selected.");
    }
}
