
#include "entityinfodialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "qtimer.h"

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

    // Main layout
    mainLayout = new QVBoxLayout(this);

    // Title
    titleLabel = new QLabel("Entity Information");
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 16px; color: #2c3e50; padding: 10px; }");
    titleLabel->setAlignment(Qt::AlignCenter);

    // Scroll area for content
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollWidget = new QWidget();
    scrollLayout = new QVBoxLayout(scrollWidget);

    // Create sections without group boxes
    createAttributeSection();
    createCarrierSection();
    createPositionSection();
    createSpeedAltTableSection();
    createTrackSection();
    createActiveSection();
    createEquipmentSection();
    createOptionsSection();

    scrollArea->setWidget(scrollWidget);

    // Close button
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
    // Create table for attributes without group box
    attributeTable = new QTableWidget();
    attributeTable->setColumnCount(2);
    attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
    attributeTable->setRowCount(3);

    // Set header style
    attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 5px; }");
    attributeTable->verticalHeader()->setVisible(false);
    attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set table properties - no scrollbars inside table and no extra space
    attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attributeTable->setFixedHeight(120);
    attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Set column widths to fill the space without extra right space
    attributeTable->horizontalHeader()->setStretchLastSection(true);
    attributeTable->setColumnWidth(0, 150); // Fixed width for Attribute column

    // Add rows
    attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
    attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(1, 0, new QTableWidgetItem("Name"));
    attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

    attributeTable->setItem(2, 0, new QTableWidgetItem("Display Name"));
    attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

    // Style the table - remove extra borders and padding
    attributeTable->setStyleSheet(
        "QTableWidget { "
        "gridline-color: #bdc3c7; "
        "background-color: white; "
        "border: 1px solid #bdc3c7; "
        "border-radius: 3px; "
        "}"
        "QTableWidget::item { "
        "padding: 5px; "
        "border-bottom: 1px solid #ecf0f1; "
        "}"
        "QTableWidget::item:selected { "
        "background-color: #3498db; "
        "color: white; "
        "}"
        );

    scrollLayout->addWidget(attributeTable);
}

void EntityInfoDialog::createCarrierSection()
{
    carrierLabel = new QLabel("Carrier: -");
    carrierLabel->setStyleSheet("QLabel { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }");
    carrierLabel->setMinimumHeight(35);

    scrollLayout->addWidget(carrierLabel);
}

void EntityInfoDialog::createPositionSection()
{
    // Create only current position display (no requested position)
    positionLabel = new QLabel("Position: -");

    // Style position label
    QString positionStyle = "QLabel { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }";
    positionLabel->setStyleSheet(positionStyle);
    positionLabel->setMinimumHeight(35);

    scrollLayout->addWidget(positionLabel);
}

void EntityInfoDialog::createSpeedAltTableSection()
{
    // Create table for Speed and Altitude with three columns
    speedAltTable = new QTableWidget();
    speedAltTable->setColumnCount(3);
    speedAltTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Current" << "Requested");
    speedAltTable->setRowCount(2);

    // Set header style
    speedAltTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 5px; }");
    speedAltTable->verticalHeader()->setVisible(false);
    speedAltTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    speedAltTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    speedAltTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // NO SCROLLBARS
    speedAltTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    speedAltTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Dynamic height calculation
    int rowHeight = 40;
    int headerHeight = speedAltTable->horizontalHeader()->height();
    int totalHeight = (speedAltTable->rowCount() * rowHeight) + headerHeight + 5;

    speedAltTable->setFixedHeight(totalHeight);
    speedAltTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Set column widths
    speedAltTable->horizontalHeader()->setStretchLastSection(true);
    speedAltTable->setColumnWidth(0, 100);

    // Add rows for Speed and Altitude only
    speedAltTable->setItem(0, 0, new QTableWidgetItem("Speed"));
    speedAltTable->setItem(0, 1, new QTableWidgetItem("-"));
    speedAltTable->setItem(0, 2, new QTableWidgetItem("-"));

    speedAltTable->setItem(1, 0, new QTableWidgetItem("Altitude"));
    speedAltTable->setItem(1, 1, new QTableWidgetItem("-"));
    speedAltTable->setItem(1, 2, new QTableWidgetItem("-"));

    // Style the table
    speedAltTable->setStyleSheet(
        "QTableWidget { "
        "gridline-color: #bdc3c7; "
        "background-color: white; "
        "border: 1px solid #bdc3c7; "
        "border-radius: 3px; "
        "}"
        "QTableWidget::item { "
        "padding: 5px; "
        "border-bottom: 1px solid #ecf0f1; "
        "}"
        "QTableWidget::item:selected { "
        "background-color: #3498db; "
        "color: white; "
        "}"
        );

    scrollLayout->addWidget(speedAltTable);
}

void EntityInfoDialog::createTrackSection()
{
    trackLayout = new QVBoxLayout();

    trackCheckBox = new QCheckBox("Track");
    centreCheckBox = new QCheckBox("Centre");
    aggregatedScriptCheckBox = new QCheckBox("Aggregated Script");

    // Style checkboxes
    QString checkboxStyle = "QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; margin-bottom: 2px; }";
    trackCheckBox->setStyleSheet(checkboxStyle);
    centreCheckBox->setStyleSheet(checkboxStyle);
    aggregatedScriptCheckBox->setStyleSheet(checkboxStyle);

    trackCheckBox->setMinimumHeight(35);
    centreCheckBox->setMinimumHeight(35);
    aggregatedScriptCheckBox->setMinimumHeight(35);

    trackLayout->addWidget(trackCheckBox);
    trackLayout->addWidget(centreCheckBox);
    trackLayout->addWidget(aggregatedScriptCheckBox);

    QWidget *trackWidget = new QWidget();
    trackWidget->setLayout(trackLayout);
    scrollLayout->addWidget(trackWidget);
}

void EntityInfoDialog::createActiveSection()
{
    activeCheckBox = new QCheckBox("Active");
    activeCheckBox->setStyleSheet("QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }");
    activeCheckBox->setMinimumHeight(35);

    scrollLayout->addWidget(activeCheckBox);
}

void EntityInfoDialog::createEquipmentSection()
{
    equipmentLayout = new QGridLayout();

    weaponsButton = new QPushButton("Weapons");
    sensorsButton = new QPushButton("Sensors");
    formationButton = new QPushButton("Formation");
    radiosButton = new QPushButton("Radios");
    iffButton = new QPushButton("IFF");

    // Style for equipment buttons
    QString buttonStyle =
        "QPushButton { "
        "background-color: #95a5a6; "
        "color: white; "
        "padding: 8px 12px; "
        "border: none; "
        "border-radius: 3px; "
        "margin: 2px; "
        "}"
        "QPushButton:hover { "
        "background-color: #7f8c8d; "
        "}";

    weaponsButton->setStyleSheet(buttonStyle);
    sensorsButton->setStyleSheet(buttonStyle);
    formationButton->setStyleSheet(buttonStyle);
    radiosButton->setStyleSheet(buttonStyle);
    iffButton->setStyleSheet(buttonStyle);

    weaponsButton->setMinimumHeight(35);
    sensorsButton->setMinimumHeight(35);
    formationButton->setMinimumHeight(35);
    radiosButton->setMinimumHeight(35);
    iffButton->setMinimumHeight(35);

    equipmentLayout->addWidget(weaponsButton, 0, 0);
    equipmentLayout->addWidget(sensorsButton, 0, 1);
    equipmentLayout->addWidget(formationButton, 1, 0);
    equipmentLayout->addWidget(radiosButton, 1, 1);
    equipmentLayout->addWidget(iffButton, 2, 0);

    // Connect signals
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
    optionsLayout = new QVBoxLayout();

    followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
    showConnectionCheckBox = new QCheckBox("Show Connection");
    showDetectionCheckBox = new QCheckBox("Show Detection");
    controlDecisiveCheckBox = new QCheckBox("Control Decisive");

    // Style checkboxes
    QString checkboxStyle = "QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; margin-bottom: 2px; }";
    followTrajectoryCheckBox->setStyleSheet(checkboxStyle);
    showConnectionCheckBox->setStyleSheet(checkboxStyle);
    showDetectionCheckBox->setStyleSheet(checkboxStyle);
    controlDecisiveCheckBox->setStyleSheet(checkboxStyle);

    followTrajectoryCheckBox->setMinimumHeight(35);
    showConnectionCheckBox->setMinimumHeight(35);
    showDetectionCheckBox->setMinimumHeight(35);
    controlDecisiveCheckBox->setMinimumHeight(35);

    optionsLayout->addWidget(followTrajectoryCheckBox);
    optionsLayout->addWidget(showConnectionCheckBox);
    optionsLayout->addWidget(showDetectionCheckBox);
    optionsLayout->addWidget(controlDecisiveCheckBox);
    connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
    if(!currentEntityId.isEmpty()){
        if(entryInfo){
            entryInfo->detection = checked;
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
    //currentEntityData = entityData;
    entryInfo = info;
    titleLabel->setText("Entity: " + entityId);
    entryInfo->detection = showDetectionCheckBox->isChecked();
}


void EntityInfoDialog::updateEntityInfo(){
    if(currentEntityId.isEmpty())return;
    if(entryInfo){
        if(entryInfo->entity){
            // Convert entity type to string
            QString typeStr = "Unknown";
            switch(entryInfo->entity->type) {
            case Constants::EntityType::Platform: typeStr = "Platform"; break;
            case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
            case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
            // Add other entity types as needed
            default: typeStr = "Unknown"; break;
            }

            // Update attribute table
            if (attributeTable) {
                attributeTable->item(0, 1)->setText(typeStr);
                attributeTable->item(1, 1)->setText(entryInfo->name);
                attributeTable->item(2, 1)->setText(entryInfo->name);
            }

            // Update position section - show only current position
            QString currentPos = QString("Lat: %1, Long: %2")
                                     .arg(entryInfo->transform->translation().x())
                                     .arg(entryInfo->transform->translation().z());

            positionLabel->setText("Position: " + currentPos);

            // 🆕 UPDATE SPEED INFORMATION
            if (speedAltTable) {
                if (entryInfo->dynamicModel) {
                    // Current speed
                    QString currentSpeed = QString("%1 km/h").arg(entryInfo->dynamicModel->moveSpeed);
                    speedAltTable->item(0, 1)->setText(currentSpeed);

                    // Requested speed (same as current for now, or you can add requested speed logic)
                    speedAltTable->item(0, 2)->setText(currentSpeed);
                } else {
                    speedAltTable->item(0, 1)->setText("-");
                    speedAltTable->item(0, 2)->setText("-");
                }

                // 🆕 UPDATE ALTITUDE INFORMATION - USE LONGITUDE VALUE HERE
                QString currentLongitude = QString("%1").arg(entryInfo->transform->translation().z());
                speedAltTable->item(1, 1)->setText(currentLongitude);
                speedAltTable->item(1, 2)->setText(currentLongitude); // Same as current for now
            }

        }
    }


}
void EntityInfoDialog::clearInfo()
{
    titleLabel->setText("Entity Information");
    currentEntityId.clear();
    currentEntityData.clear();

    // Reset attribute table
    if (attributeTable) {
        attributeTable->item(0, 1)->setText("-");
        attributeTable->item(1, 1)->setText("-");
        attributeTable->item(2, 1)->setText("-");
    }

    // Reset carrier section
    carrierLabel->setText("Carrier: -");

    // Reset position section
    positionLabel->setText("Position: -");

    // Reset Speed and Altitude table
    if (speedAltTable) {
        speedAltTable->item(0, 1)->setText("-");
        speedAltTable->item(0, 2)->setText("-");
        speedAltTable->item(1, 1)->setText("-");
        speedAltTable->item(1, 2)->setText("-");
    }

    // Uncheck all checkboxes
    trackCheckBox->setChecked(false);
    centreCheckBox->setChecked(false);
    aggregatedScriptCheckBox->setChecked(false);
    activeCheckBox->setChecked(false);
    followTrajectoryCheckBox->setChecked(false);
    showConnectionCheckBox->setChecked(false);
    showDetectionCheckBox->setChecked(false);
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
            if(entryInfo->entity){
                // Create a simple clean dialog for sensors
                QDialog *sensorsDialog = new QDialog(this);
                sensorsDialog->setWindowTitle("Sensors - " + currentEntityId);
                sensorsDialog->setMinimumSize(450, 300);
                sensorsDialog->setMaximumSize(500, 400);

                // Simple window flags
                sensorsDialog->setWindowFlags(Qt::Dialog);

                // Minimal styling - clean and professional
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

                // Real-time update timer
                QTimer *sensorsUpdateTimer = new QTimer(sensorsDialog);
                QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
                    updateSensorsTable(sensorsTable, entryInfo->entity);
                });
                sensorsUpdateTimer->start(100);

                // Initial population
                updateSensorsTable(sensorsTable, entryInfo->entity);

                // Simple column sizing
                sensorsTable->horizontalHeader()->setStretchLastSection(false);
                sensorsTable->setColumnWidth(0, 120); // Name
                sensorsTable->setColumnWidth(1, 80);  // Type
                sensorsTable->setColumnWidth(2, 80);  // Range
                sensorsTable->setColumnWidth(3, 60);  // FOV

                layout->addWidget(sensorsTable);

                // Simple summary
                QLabel *summaryLabel = new QLabel();
                summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
                layout->addWidget(summaryLabel);

                // Update summary
                QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
                    int total = 0;
                    if(entryInfo && entryInfo->entity) {
                        for (Sensor* s : entryInfo->entity->sensorList) {
                            if(s->subType == Sensor::SubType::Generic ||
                                s->subType == Sensor::SubType::CSM ||
                                s->subType == Sensor::SubType::ESM) {
                                total++;
                            }
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

            }
        }
    } else {
        QMessageBox::information(this, "Sensors", "No entity selected.");
    }
}

void EntityInfoDialog::updateSensorsTable(QTableWidget* sensorsTable, Entity* entity)
{
    if(!sensorsTable || !entity) return;

    sensorsTable->setRowCount(0);
    int row = 0;

    for (Sensor* s : entity->sensorList) {
        if(s->subType == Sensor::SubType::Generic ||
            s->subType == Sensor::SubType::CSM ||
            s->subType == Sensor::SubType::ESM) {

            sensorsTable->insertRow(row);

            // Name
            QString sensorName = QString::fromStdString(s->Name);
            sensorsTable->setItem(row, 0, new QTableWidgetItem(sensorName));

            // Type
            QString typeStr;
            if(s->subType == Sensor::SubType::Generic) typeStr = "Radar";
            else if(s->subType == Sensor::SubType::CSM) typeStr = "CSM";
            else if(s->subType == Sensor::SubType::ESM) typeStr = "ESM";
            sensorsTable->setItem(row, 1, new QTableWidgetItem(typeStr));

            // Range
            QString rangeStr;
            if(s->subType == Sensor::SubType::Generic) rangeStr = QString("%1").arg(s->range, 0, 'f', 1);
            else if(s->subType == Sensor::SubType::CSM) rangeStr = QString("%1").arg(s->csmrange, 0, 'f', 1);
            else if(s->subType == Sensor::SubType::ESM) rangeStr = QString("%1").arg(s->esrange, 0, 'f', 1);
            sensorsTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

            // FOV
            QString fovStr;
            if(s->subType == Sensor::SubType::Generic) fovStr = QString("%1°").arg(s->maxDetectionAngle, 0, 'f', 0);
            else fovStr = "-";
            sensorsTable->setItem(row, 3, new QTableWidgetItem(fovStr));

            row++;
        }
    }

    if (row == 0) {
        sensorsTable->setRowCount(1);
        sensorsTable->setItem(0, 0, new QTableWidgetItem("No sensors"));
        sensorsTable->setItem(0, 1, new QTableWidgetItem(""));
        sensorsTable->setItem(0, 2, new QTableWidgetItem(""));
        sensorsTable->setItem(0, 3, new QTableWidgetItem(""));
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

// void EntityInfoDialog::onRadiosClicked()
// {
//     // Show radios list popup
//     QMessageBox::information(this, "Radios",
//                              QString("Radios for entity %1:\n%2")
//                                  .arg(currentEntityId)
//                                  .arg(currentEntityData.value("radios", "No radios data").toString()));
// }


void EntityInfoDialog::onRadiosClicked()
{
    if(!currentEntityId.isEmpty()){
        if(entryInfo){
            if(entryInfo->entity){
                // Create a simple clean dialog for radios (SENSORS की तरह)
                QDialog *radiosDialog = new QDialog(this);
                radiosDialog->setWindowTitle("Radios - " + currentEntityId);
                radiosDialog->setMinimumSize(500, 350);
                radiosDialog->setMaximumSize(600, 450);

                // Simple window flags
                radiosDialog->setWindowFlags(Qt::Dialog);

                // Minimal styling - clean and professional
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

                // Simple title
                QLabel *titleLabel = new QLabel("Radio Systems");
                titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
                layout->addWidget(titleLabel);

                // Create clean table for radios - ONLY NAME AND RANGE FIELDS
                QTableWidget *radiosTable = new QTableWidget();
                radiosTable->setColumnCount(2);  // ONLY 2 COLUMNS: Name and Range
                radiosTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Range");

                radiosTable->verticalHeader()->setVisible(false);
                radiosTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
                radiosTable->setSelectionBehavior(QAbstractItemView::SelectRows);
                radiosTable->setSelectionMode(QAbstractItemView::SingleSelection);

                // Clean table properties
                radiosTable->setShowGrid(true);
                radiosTable->setAlternatingRowColors(false);

                // Real-time update timer
                QTimer *radiosUpdateTimer = new QTimer(radiosDialog);
                QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
                    // updateRadiosTable(radiosTable, entryInfo->entity);
                });
                radiosUpdateTimer->start(100);

                // Initial population
                // updateRadiosTable(radiosTable, entryInfo->entity);

                // Simple column sizing
                radiosTable->horizontalHeader()->setStretchLastSection(false);
                radiosTable->setColumnWidth(0, 150); // Name
                radiosTable->setColumnWidth(1, 100); // Range

                layout->addWidget(radiosTable);

                // Simple summary
                QLabel *summaryLabel = new QLabel();
                summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
                layout->addWidget(summaryLabel);

                // Update summary
                QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
                    int total = 0;
                    if(entryInfo && entryInfo->entity) {
                        total = entryInfo->entity->radioList.size();
                    }
                    summaryLabel->setText(QString("Total: %1 radios").arg(total));
                });

                // Simple close button
                QHBoxLayout *buttonLayout = new QHBoxLayout();
                buttonLayout->addStretch();
                QPushButton *closeButton = new QPushButton("Close");
                QObject::connect(closeButton, &QPushButton::clicked, radiosDialog, [=]() {
                    radiosUpdateTimer->stop();
                    radiosDialog->close();
                });
                buttonLayout->addWidget(closeButton);
                layout->addLayout(buttonLayout);

                // Show dialog
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
    // Show IFF details popup
    QMessageBox::information(this, "IFF",
                             QString("IFF for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("iff", "No IFF data").toString()));
}


