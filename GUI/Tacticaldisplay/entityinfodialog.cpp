
#include "entityinfodialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>

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
    // Create position display like carrier (not as table)
    positionLayout = new QVBoxLayout();

    positionCurrentLabel = new QLabel("Current Position: -");
    positionRequestedLabel = new QLabel("Requested Position: -");

    // Style position labels
    QString positionStyle = "QLabel { padding: 6px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; margin-bottom: 2px; }";
    positionCurrentLabel->setStyleSheet(positionStyle);
    positionRequestedLabel->setStyleSheet(positionStyle);

    positionCurrentLabel->setMinimumHeight(30);
    positionRequestedLabel->setMinimumHeight(30);

    positionLayout->addWidget(positionCurrentLabel);
    positionLayout->addWidget(positionRequestedLabel);

    QWidget *positionWidget = new QWidget();
    positionWidget->setLayout(positionLayout);
    scrollLayout->addWidget(positionWidget);
}


// void EntityInfoDialog::createPositionSection()
// {
//     // Create position display like carrier (not as table)
//     positionLayout = new QVBoxLayout();

//     // SIRF CURRENT POSITION - requested position remove karein
//     positionCurrentLabel = new QLabel("Current Position: -");

//     // Style position label
//     QString positionStyle = "QLabel { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }";
//     positionCurrentLabel->setStyleSheet(positionStyle);

//     positionCurrentLabel->setMinimumHeight(35);

//     positionLayout->addWidget(positionCurrentLabel);

//     QWidget *positionWidget = new QWidget();
//     positionWidget->setLayout(positionLayout);
//     scrollLayout->addWidget(positionWidget);
// }

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

    QWidget *optionsWidget = new QWidget();
    optionsWidget->setLayout(optionsLayout);
    scrollLayout->addWidget(optionsWidget);
}

void EntityInfoDialog::setEntityInfo(const QString& entityId, const QVariantMap& entityData)
{
    currentEntityId = entityId;
    currentEntityData = entityData;

    titleLabel->setText("Entity: " + entityId);

    // Update attribute table
    if (attributeTable) {
        attributeTable->item(0, 1)->setText(entityData.value("type", "Unknown").toString());
        attributeTable->item(1, 1)->setText(entityData.value("name", "Unknown").toString());
        attributeTable->item(2, 1)->setText(entityData.value("displayName", "Unknown").toString());
    }

    // Update carrier section
    carrierLabel->setText("Carrier: " + entityData.value("carrier", "None").toString());

    // Update position section - show Lat/Long values
    QString currentPos = QString("Lat: %1, Long: %2")
                             .arg(entityData.value("latitude", "Unknown").toString())
                             .arg(entityData.value("longitude", "Unknown").toString());

    QString requestedPos = QString("Lat: %1, Long: %2")
                               .arg(entityData.value("requestedLatitude", "Unknown").toString())
                               .arg(entityData.value("requestedLongitude", "Unknown").toString());

    positionCurrentLabel->setText("Current Position: " + currentPos);
    positionRequestedLabel->setText("Requested Position: " + requestedPos);

    // Update Speed and Altitude table
    if (speedAltTable) {
        speedAltTable->item(0, 1)->setText(entityData.value("speed", "Unknown").toString());
        speedAltTable->item(0, 2)->setText(entityData.value("requestedSpeed", "Unknown").toString());

        speedAltTable->item(1, 1)->setText(entityData.value("altitude", "Unknown").toString());
        speedAltTable->item(1, 2)->setText(entityData.value("requestedAltitude", "Unknown").toString());
    }

    // Update checkboxes from entity data
    trackCheckBox->setChecked(entityData.value("track", false).toBool());
    centreCheckBox->setChecked(entityData.value("centre", false).toBool());
    aggregatedScriptCheckBox->setChecked(entityData.value("aggregatedScript", false).toBool());
    activeCheckBox->setChecked(entityData.value("active", false).toBool());
    followTrajectoryCheckBox->setChecked(entityData.value("followTrajectory", false).toBool());
    showConnectionCheckBox->setChecked(entityData.value("showConnection", false).toBool());
    showDetectionCheckBox->setChecked(entityData.value("showDetection", false).toBool());
    controlDecisiveCheckBox->setChecked(entityData.value("controlDecisive", false).toBool());
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
    positionCurrentLabel->setText("Current Position: -");
    positionRequestedLabel->setText("Requested Position: -");

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
    // Show sensors list popup
    QMessageBox::information(this, "Sensors",
                             QString("Sensors for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("sensors", "No sensors data").toString()));
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
    // Show radios list popup
    QMessageBox::information(this, "Radios",
                             QString("Radios for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("radios", "No radios data").toString()));
}

void EntityInfoDialog::onIFFClicked()
{
    // Show IFF details popup
    QMessageBox::information(this, "IFF",
                             QString("IFF for entity %1:\n%2")
                                 .arg(currentEntityId)
                                 .arg(currentEntityData.value("iff", "No IFF data").toString()));
}
