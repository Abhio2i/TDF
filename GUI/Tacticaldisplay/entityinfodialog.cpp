
// #include "entityinfodialog.h"
// #include <QJsonDocument>
// #include <QJsonObject>
// #include <QDebug>
// #include <QMessageBox>
// #include <QTableWidget>
// #include <QHeaderView>
// #include "core/Hierarchy/EntityProfiles/sensor.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Hierarchy/EntityProfiles/iff.h"
// #include "qtimer.h"


// EntityInfoDialog::EntityInfoDialog(QWidget *parent)
//     : QDialog(parent)
// {
//     setupUI();


//     connect(followTrajectoryCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
//         if(!currentEntityId.isEmpty() && entryInfo) {
//             if (entryInfo->dynamicModel) {
//                 entryInfo->dynamicModel->followPath = checked;

//                 if (!checked) {
//                     // STOP - only set movement variables to zero
//                     entryInfo->dynamicModel->speeed = 0;
//                     entryInfo->dynamicModel->currentSpeed = 0;
//                     // 🆕 DON'T change moveSpeed
//                 } else {
//                     // RESUME - 🆕 DON'T set moveSpeed to 100, let it use existing value
//                     // moveSpeed already has the correct value
//                 }
//             }

//             if (entryInfo->trajectory) {
//                 entryInfo->trajectory->Active = checked;
//                 entryInfo->trajectory->FollowPath = checked;
//             }

//             emit update();
//         }
//     });
// }
// // void EntityInfoDialog::setupUI()
// // {
// //     setWindowTitle("Entity Information");
// //     setFixedSize(500, 600);
// //     setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

// //     // Main layout
// //     mainLayout = new QVBoxLayout(this);

// //     // Title
// //     titleLabel = new QLabel("Entity Information");
// //     titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 16px; color: #2c3e50; padding: 10px; }");
// //     titleLabel->setAlignment(Qt::AlignCenter);

// //     // Scroll area for content
// //     scrollArea = new QScrollArea();
// //     scrollArea->setWidgetResizable(true);
// //     scrollArea->setFrameShape(QFrame::NoFrame);
// //     scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
// //     scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

// //     scrollWidget = new QWidget();
// //     scrollLayout = new QVBoxLayout(scrollWidget);

// //     // Create sections without group boxes
// //     createAttributeSection();
// //     createCarrierSection();
// //     createPositionSection();
// //     createSpeedAltTableSection();
// //     createTrackSection();
// //     createActiveSection();
// //     createEquipmentSection();
// //     createOptionsSection();

// //     scrollArea->setWidget(scrollWidget);

// //     // Close button
// //     closeButton = new QPushButton("Close");
// //     closeButton->setStyleSheet(
// //         "QPushButton { "
// //         "background-color: #3498db; "
// //         "color: white; "
// //         "padding: 8px 16px; "
// //         "border: none; "
// //         "border-radius: 4px; "
// //         "font-weight: bold; "
// //         "}"
// //         "QPushButton:hover { "
// //         "background-color: #2980b9; "
// //         "}"
// //         );

// //     // Add widgets to main layout
// //     mainLayout->addWidget(titleLabel);
// //     mainLayout->addWidget(scrollArea);
// //     mainLayout->addWidget(closeButton);

// //     connect(closeButton, &QPushButton::clicked, this, &EntityInfoDialog::onCloseClicked);
// // }

// // void EntityInfoDialog::createAttributeSection()
// // {
// //     // Create table for attributes without group box
// //     attributeTable = new QTableWidget();
// //     attributeTable->setColumnCount(2);
// //     attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
// //     attributeTable->setRowCount(3);

// //     // Set header style
// //     attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 5px; }");
// //     attributeTable->verticalHeader()->setVisible(false);
// //     attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
// //     attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
// //     attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

// //     // Set table properties - no scrollbars inside table and no extra space
// //     attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setFixedHeight(120);
// //     attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

// //     // Set column widths to fill the space without extra right space
// //     attributeTable->horizontalHeader()->setStretchLastSection(true);
// //     attributeTable->setColumnWidth(0, 150); // Fixed width for Attribute column

// //     // Add rows
// //     attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
// //     attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(1, 0, new QTableWidgetItem("Name"));
// //     attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(2, 0, new QTableWidgetItem("Display Name"));
// //     attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

// //     // Style the table - remove extra borders and padding
// //     attributeTable->setStyleSheet(
// //         "QTableWidget { "
// //         "gridline-color: #bdc3c7; "
// //         "background-color: white; "
// //         "border: 1px solid #bdc3c7; "
// //         "border-radius: 3px; "
// //         "}"
// //         "QTableWidget::item { "
// //         "padding: 5px; "
// //         "border-bottom: 1px solid #ecf0f1; "
// //         "}"
// //         "QTableWidget::item:selected { "
// //         "background-color: #3498db; "
// //         "color: white; "
// //         "}"
// //         );

// //     scrollLayout->addWidget(attributeTable);
// // }

// // void EntityInfoDialog::createCarrierSection()
// // {
// //     carrierLabel = new QLabel("Carrier: -");
// //     carrierLabel->setStyleSheet("QLabel { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }");
// //     carrierLabel->setMinimumHeight(35);

// //     scrollLayout->addWidget(carrierLabel);
// // }

// // void EntityInfoDialog::createPositionSection()
// // {
// //     // Create only current position display (no requested position)
// //     positionLabel = new QLabel("Position: -");

// //     // Style position label
// //     QString positionStyle = "QLabel { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }";
// //     positionLabel->setStyleSheet(positionStyle);
// //     positionLabel->setMinimumHeight(35);

// //     scrollLayout->addWidget(positionLabel);
// // }

// // void EntityInfoDialog::createSpeedAltTableSection()
// // {
// //     // Create table for Speed and Altitude with three columns
// //     speedAltTable = new QTableWidget();
// //     speedAltTable->setColumnCount(3);
// //     speedAltTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Current" << "Requested");
// //     speedAltTable->setRowCount(2);

// //     // Set header style
// //     speedAltTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 5px; }");
// //     speedAltTable->verticalHeader()->setVisible(false);
// //     speedAltTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
// //     speedAltTable->setSelectionBehavior(QAbstractItemView::SelectRows);
// //     speedAltTable->setSelectionMode(QAbstractItemView::SingleSelection);

// //     // NO SCROLLBARS
// //     speedAltTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     speedAltTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

// //     // Dynamic height calculation
// //     int rowHeight = 40;
// //     int headerHeight = speedAltTable->horizontalHeader()->height();
// //     int totalHeight = (speedAltTable->rowCount() * rowHeight) + headerHeight + 5;

// //     speedAltTable->setFixedHeight(totalHeight);
// //     speedAltTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

// //     // Set column widths
// //     speedAltTable->horizontalHeader()->setStretchLastSection(true);
// //     speedAltTable->setColumnWidth(0, 100);

// //     // Add rows for Speed and Altitude only
// //     speedAltTable->setItem(0, 0, new QTableWidgetItem("Speed"));
// //     speedAltTable->setItem(0, 1, new QTableWidgetItem("-"));
// //     speedAltTable->setItem(0, 2, new QTableWidgetItem("-"));

// //     speedAltTable->setItem(1, 0, new QTableWidgetItem("Altitude"));
// //     speedAltTable->setItem(1, 1, new QTableWidgetItem("-"));
// //     speedAltTable->setItem(1, 2, new QTableWidgetItem("-"));

// //     // Style the table
// //     speedAltTable->setStyleSheet(
// //         "QTableWidget { "
// //         "gridline-color: #bdc3c7; "
// //         "background-color: white; "
// //         "border: 1px solid #bdc3c7; "
// //         "border-radius: 3px; "
// //         "}"
// //         "QTableWidget::item { "
// //         "padding: 5px; "
// //         "border-bottom: 1px solid #ecf0f1; "
// //         "}"
// //         "QTableWidget::item:selected { "
// //         "background-color: #3498db; "
// //         "color: white; "
// //         "}"
// //         );

// //     scrollLayout->addWidget(speedAltTable);
// // }

// // void EntityInfoDialog::createTrackSection()
// // {
// //     trackLayout = new QVBoxLayout();

// //     trackCheckBox = new QCheckBox("Track");
// //     centreCheckBox = new QCheckBox("Centre");
// //     aggregatedScriptCheckBox = new QCheckBox("Aggregated Script");

// //     // Style checkboxes
// //     QString checkboxStyle = "QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; margin-bottom: 2px; }";
// //     trackCheckBox->setStyleSheet(checkboxStyle);
// //     centreCheckBox->setStyleSheet(checkboxStyle);
// //     aggregatedScriptCheckBox->setStyleSheet(checkboxStyle);

// //     trackCheckBox->setMinimumHeight(35);
// //     centreCheckBox->setMinimumHeight(35);
// //     aggregatedScriptCheckBox->setMinimumHeight(35);

// //     trackLayout->addWidget(trackCheckBox);
// //     trackLayout->addWidget(centreCheckBox);
// //     trackLayout->addWidget(aggregatedScriptCheckBox);

// //     QWidget *trackWidget = new QWidget();
// //     trackWidget->setLayout(trackLayout);
// //     scrollLayout->addWidget(trackWidget);
// // }

// // void EntityInfoDialog::createActiveSection()
// // {
// //     activeCheckBox = new QCheckBox("Active");
// //     activeCheckBox->setStyleSheet("QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }");
// //     activeCheckBox->setMinimumHeight(35);

// //     scrollLayout->addWidget(activeCheckBox);
// // }

// // void EntityInfoDialog::createEquipmentSection()
// // {
// //     equipmentLayout = new QGridLayout();


// //     sensorsButton = new QPushButton("Sensors");
// //     radiosButton = new QPushButton("Radios");
// //     iffButton = new QPushButton("IFF");
// //     weaponsButton = new QPushButton("Weapons");
// //     formationButton = new QPushButton("Formation");

// //     // Style for equipment buttons
// //     QString buttonStyle =
// //         "QPushButton { "
// //         "background-color: #95a5a6; "
// //         "color: white; "
// //         "padding: 8px 12px; "
// //         "border: none; "
// //         "border-radius: 3px; "
// //         "margin: 2px; "
// //         "}"
// //         "QPushButton:hover { "
// //         "background-color: #7f8c8d; "
// //         "}";

// //     // weaponsButton->setStyleSheet(buttonStyle);
// //     sensorsButton->setStyleSheet(buttonStyle);
// //     formationButton->setStyleSheet(buttonStyle);
// //     radiosButton->setStyleSheet(buttonStyle);
// //     iffButton->setStyleSheet(buttonStyle);
// //       weaponsButton->setStyleSheet(buttonStyle);
// //         weaponsButton->setStyleSheet(buttonStyle);

// //     weaponsButton->setMinimumHeight(35);
// //     sensorsButton->setMinimumHeight(35);
// //     formationButton->setMinimumHeight(35);
// //     radiosButton->setMinimumHeight(35);
// //     iffButton->setMinimumHeight(35);

// //     // CORRECT SEQUENCE: Sensors, Radio, IFF, Formation, Weapons
// //     equipmentLayout->addWidget(sensorsButton, 0, 0);    // Row 0, Col 0 - Sensors (First)
// //     equipmentLayout->addWidget(radiosButton, 0, 1);     // Row 0, Col 1 - Radios (Second)
// //     equipmentLayout->addWidget(iffButton, 1, 0);        // Row 1, Col 0 - IFF (Third)
// //     equipmentLayout->addWidget(formationButton, 1, 1);  // Row 1, Col 1 - Formation (Fourth)
// //     equipmentLayout->addWidget(weaponsButton, 2, 0);    // Row 2, Col 0 - Weapons (Fifth)


// //     // Connect signals
// //     connect(weaponsButton, &QPushButton::clicked, this, &EntityInfoDialog::onWeaponsClicked);
// //     connect(sensorsButton, &QPushButton::clicked, this, &EntityInfoDialog::onSensorsClicked);
// //     connect(formationButton, &QPushButton::clicked, this, &EntityInfoDialog::onFormationClicked);
// //     connect(radiosButton, &QPushButton::clicked, this, &EntityInfoDialog::onRadiosClicked);
// //     connect(iffButton, &QPushButton::clicked, this, &EntityInfoDialog::onIFFClicked);

// //     QWidget *equipmentWidget = new QWidget();
// //     equipmentWidget->setLayout(equipmentLayout);
// //     scrollLayout->addWidget(equipmentWidget);
// // }

// // void EntityInfoDialog::createOptionsSection()
// // {
// //     optionsLayout = new QVBoxLayout();

// //     followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
// //     showConnectionCheckBox = new QCheckBox("Show Connection");
// //     showDetectionCheckBox = new QCheckBox("Show Detection");
// //     controlDecisiveCheckBox = new QCheckBox("Control Decisive");

// //     // Style checkboxes
// //     QString checkboxStyle = "QCheckBox { padding: 8px; background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; margin-bottom: 2px; }";
// //     followTrajectoryCheckBox->setStyleSheet(checkboxStyle);
// //     showConnectionCheckBox->setStyleSheet(checkboxStyle);
// //     showDetectionCheckBox->setStyleSheet(checkboxStyle);
// //     controlDecisiveCheckBox->setStyleSheet(checkboxStyle);

// //     followTrajectoryCheckBox->setMinimumHeight(35);
// //     showConnectionCheckBox->setMinimumHeight(35);
// //     showDetectionCheckBox->setMinimumHeight(35);
// //     controlDecisiveCheckBox->setMinimumHeight(35);
// //       showDetectionCheckBox->setChecked(true);

// //     optionsLayout->addWidget(followTrajectoryCheckBox);
// //     optionsLayout->addWidget(showConnectionCheckBox);
// //     optionsLayout->addWidget(showDetectionCheckBox);
// //     optionsLayout->addWidget(controlDecisiveCheckBox);


// //     connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
// //         if(!currentEntityId.isEmpty()){
// //             if(entryInfo){
// //                 // 🆕 DONO PROPERTIES KO EK SAATH SET KAREN
// //                 entryInfo->detection = checked;      // Sensors ke liye
// //                 entryInfo->radioVisible = checked;   // Radios ke liye
// //                 emit update();
// //             }
// //         }
// //     });

// //     QWidget *optionsWidget = new QWidget();
// //     optionsWidget->setLayout(optionsLayout);
// //     scrollLayout->addWidget(optionsWidget);
// // }
// void EntityInfoDialog::setupUI()
// {
//     setWindowTitle("Entity Information");
//     setFixedSize(500, 600);
//     setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

//     // Simple white background
//     setStyleSheet("QDialog { background-color: white; }");

//     // Main layout
//     mainLayout = new QVBoxLayout(this);
//     mainLayout->setSpacing(5);
//     mainLayout->setContentsMargins(8, 8, 8, 8);

//     // Title - waise hi
//     titleLabel = new QLabel("Entity Information");
//     titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 16px; color: #000000; padding: 10px; }");
//     titleLabel->setAlignment(Qt::AlignCenter);

//     // Scroll area for content
//     scrollArea = new QScrollArea();
//     scrollArea->setWidgetResizable(true);
//     scrollArea->setFrameShape(QFrame::NoFrame);
//     scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

//     scrollWidget = new QWidget();
//     scrollLayout = new QVBoxLayout(scrollWidget);
//     scrollLayout->setSpacing(3);
//     scrollLayout->setContentsMargins(2, 2, 2, 2);

//     // Create sections
//     createAttributeSection();
//     // createCarrierSection();
//     createPositionSection();
//     createSpeedAltTableSection();
//     createTrackSection();
//     // createActiveSection();
//     createEquipmentSection();
//     createOptionsSection();

//     scrollArea->setWidget(scrollWidget);

//     // Close button - aapka existing waise hi
//     closeButton = new QPushButton("Close");
//     closeButton->setStyleSheet(
//         "QPushButton { "
//         "background-color: #3498db; "
//         "color: white; "
//         "padding: 8px 16px; "
//         "border: none; "
//         "border-radius: 4px; "
//         "font-weight: bold; "
//         "}"
//         "QPushButton:hover { "
//         "background-color: #2980b9; "
//         "}"
//         );

//     // Add widgets to main layout
//     mainLayout->addWidget(titleLabel);
//     mainLayout->addWidget(scrollArea);
//     mainLayout->addWidget(closeButton);

//     connect(closeButton, &QPushButton::clicked, this, &EntityInfoDialog::onCloseClicked);
// }

// // void EntityInfoDialog::createAttributeSection()
// // {
// //     // Create table for attributes
// //     attributeTable = new QTableWidget();
// //     attributeTable->setColumnCount(2);
// //     attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
// //     attributeTable->setRowCount(3);

// //     // Simple header style - no dark colors
// //     attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
// //     attributeTable->verticalHeader()->setVisible(false);
// //     attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
// //     attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
// //     attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

// //     // No scrollbars
// //     attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setFixedHeight(120);
// //     attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

// //     // Column widths
// //     attributeTable->horizontalHeader()->setStretchLastSection(true);
// //     attributeTable->setColumnWidth(0, 150);

// //     // Add rows
// //     attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
// //     attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(1, 0, new QTableWidgetItem("Name"));
// //     attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(2, 0, new QTableWidgetItem("Display Name"));
// //     attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

// //     // Simple table style
// //     attributeTable->setStyleSheet(
// //         "QTableWidget { "
// //         "gridline-color: #cccccc; "
// //         "background-color: white; "
// //         "border: 1px solid #cccccc; "
// //         "}"
// //         "QTableWidget::item { "
// //         "padding: 5px; "
// //         "border-bottom: 1px solid #f0f0f0; "
// //         "}"
// //         );

// //     scrollLayout->addWidget(attributeTable);
// // }

// // void EntityInfoDialog::createAttributeSection()
// // {
// //     // Create table for attributes
// //     attributeTable = new QTableWidget();
// //     attributeTable->setColumnCount(2);
// //     attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
// //     attributeTable->setRowCount(4); // 4 rows: Type, DIS NAME, Damages, Carrier

// //     // Simple header style - no dark colors
// //     attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
// //     attributeTable->verticalHeader()->setVisible(false);
// //     attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
// //     attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
// //     attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

// //     // No scrollbars
// //     attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
// //     attributeTable->setFixedHeight(160); // Height increased for 4 rows
// //     attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

// //     // Column widths
// //     attributeTable->horizontalHeader()->setStretchLastSection(true);
// //     attributeTable->setColumnWidth(0, 150);

// //     // Add rows - Type, DIS NAME, Damages, Carrier
// //     attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
// //     attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(1, 0, new QTableWidgetItem("DIS NAME"));
// //     attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(2, 0, new QTableWidgetItem("Damages"));
// //     attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

// //     attributeTable->setItem(3, 0, new QTableWidgetItem("Carrier"));
// //     attributeTable->setItem(3, 1, new QTableWidgetItem("-"));

// //     // Simple table style
// //     attributeTable->setStyleSheet(
// //         "QTableWidget { "
// //         "gridline-color: #cccccc; "
// //         "background-color: white; "
// //         "border: 1px solid #cccccc; "
// //         "}"
// //         "QTableWidget::item { "
// //         "padding: 5px; "
// //         "border-bottom: 1px solid #f0f0f0; "
// //         "}"
// //         );

// //     scrollLayout->addWidget(attributeTable);
// // }
// void EntityInfoDialog::createAttributeSection()
// {
//     // Create table for attributes
//     attributeTable = new QTableWidget();
//     attributeTable->setColumnCount(2);
//     attributeTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
//     attributeTable->setRowCount(5); // 5 rows: Type, Name, DIS name, Damages, Carrier

//     // Simple header style - no dark colors
//     attributeTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
//     attributeTable->verticalHeader()->setVisible(false);
//     attributeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//     attributeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//     attributeTable->setSelectionMode(QAbstractItemView::SingleSelection);

//     // No scrollbars
//     attributeTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     attributeTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     attributeTable->setFixedHeight(200); // Height increased for 5 rows
//     attributeTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

//     // Column widths
//     attributeTable->horizontalHeader()->setStretchLastSection(true);
//     attributeTable->setColumnWidth(0, 150);

//     // Add rows - Type, Name, DIS name, Damages, Carrier
//     attributeTable->setItem(0, 0, new QTableWidgetItem("Type"));
//     attributeTable->setItem(0, 1, new QTableWidgetItem("-"));

//     attributeTable->setItem(1, 0, new QTableWidgetItem("Name"));
//     attributeTable->setItem(1, 1, new QTableWidgetItem("-"));

//     attributeTable->setItem(2, 0, new QTableWidgetItem("DIS name"));
//     attributeTable->setItem(2, 1, new QTableWidgetItem("-"));

//     attributeTable->setItem(3, 0, new QTableWidgetItem("Damages"));
//     attributeTable->setItem(3, 1, new QTableWidgetItem("-"));

//     attributeTable->setItem(4, 0, new QTableWidgetItem("Carrier"));
//     attributeTable->setItem(4, 1, new QTableWidgetItem("-"));

//     // Simple table style
//     attributeTable->setStyleSheet(
//         "QTableWidget { "
//         "gridline-color: #cccccc; "
//         "background-color: white; "
//         "border: 1px solid #cccccc; "
//         "}"
//         "QTableWidget::item { "
//         "padding: 5px; "
//         "border-bottom: 1px solid #f0f0f0; "
//         "}"
//         );

//     scrollLayout->addWidget(attributeTable);
// }
// // void EntityInfoDialog::createCarrierSection()
// // {
// //     carrierLabel = new QLabel("Carrier: -");
// //     carrierLabel->setStyleSheet("QLabel { padding: 8px; color: #000000; }");
// //     carrierLabel->setMinimumHeight(35);
// //     scrollLayout->addWidget(carrierLabel);
// // }

// void EntityInfoDialog::createPositionSection()
// {
//     positionLabel = new QLabel("Position: -");
//     positionLabel->setStyleSheet("QLabel { padding: 8px; color: #000000; }");
//     positionLabel->setMinimumHeight(35);
//     scrollLayout->addWidget(positionLabel);
// }

// void EntityInfoDialog::createSpeedAltTableSection()
// {
//     // Create table
//     speedAltTable = new QTableWidget();
//     speedAltTable->setColumnCount(3);
//     speedAltTable->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Current" << "Requested");
//     speedAltTable->setRowCount(2);

//     // Simple header style
//     speedAltTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #f0f0f0; color: #000000; padding: 5px; }");
//     speedAltTable->verticalHeader()->setVisible(false);
//     speedAltTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//     speedAltTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//     speedAltTable->setSelectionMode(QAbstractItemView::SingleSelection);

//     // No scrollbars
//     speedAltTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     speedAltTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

//     // Height calculation
//     int rowHeight = 40;
//     int headerHeight = speedAltTable->horizontalHeader()->height();
//     int totalHeight = (speedAltTable->rowCount() * rowHeight) + headerHeight + 5;

//     speedAltTable->setFixedHeight(totalHeight);
//     speedAltTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

//     // Column widths
//     speedAltTable->horizontalHeader()->setStretchLastSection(true);
//     speedAltTable->setColumnWidth(0, 100);

//     // Add rows
//     speedAltTable->setItem(0, 0, new QTableWidgetItem("Speed"));
//     speedAltTable->setItem(0, 1, new QTableWidgetItem("-"));
//     speedAltTable->setItem(0, 2, new QTableWidgetItem("-"));

//     speedAltTable->setItem(1, 0, new QTableWidgetItem("Altitude"));
//     speedAltTable->setItem(1, 1, new QTableWidgetItem("-"));
//     speedAltTable->setItem(1, 2, new QTableWidgetItem("-"));

//     // Simple table style
//     speedAltTable->setStyleSheet(
//         "QTableWidget { "
//         "gridline-color: #cccccc; "
//         "background-color: white; "
//         "border: 1px solid #cccccc; "
//         "}"
//         "QTableWidget::item { "
//         "padding: 5px; "
//         "border-bottom: 1px solid #f0f0f0; "
//         "}"
//         );

//     scrollLayout->addWidget(speedAltTable);
// }

// // void EntityInfoDialog::createTrackSection()
// // {
// //     trackLayout = new QVBoxLayout();
// //     trackLayout->setSpacing(2);

// //     trackCheckBox = new QCheckBox("Track");
// //     centreCheckBox = new QCheckBox("Centre");
// //     aggregatedScriptCheckBox = new QCheckBox("Aggregated");

// //     // Simple checkbox style
// //     QString checkboxStyle = "QCheckBox { color: #000000; padding: 8px; }";
// //     trackCheckBox->setStyleSheet(checkboxStyle);
// //     centreCheckBox->setStyleSheet(checkboxStyle);
// //     aggregatedScriptCheckBox->setStyleSheet(checkboxStyle);

// //     trackCheckBox->setMinimumHeight(35);
// //     centreCheckBox->setMinimumHeight(35);
// //     aggregatedScriptCheckBox->setMinimumHeight(35);

// //     trackLayout->addWidget(trackCheckBox);
// //     trackLayout->addWidget(centreCheckBox);
// //     trackLayout->addWidget(aggregatedScriptCheckBox);

// //     QWidget *trackWidget = new QWidget();
// //     trackWidget->setLayout(trackLayout);
// //     scrollLayout->addWidget(trackWidget);
// // }
// void EntityInfoDialog::createTrackSection()
// {
//     // Horizontal layout for one line
//     QHBoxLayout *trackLayout = new QHBoxLayout();
//     trackLayout->setSpacing(0); // No space between checkboxes

//     trackCheckBox = new QCheckBox("Track");
//     centreCheckBox = new QCheckBox("Center");
//     aggregatedScriptCheckBox = new QCheckBox("Aggregated");

//     // Simple checkbox style
//     QString checkboxStyle = "QCheckBox { color: #000000; padding: 8px; }";
//     trackCheckBox->setStyleSheet(checkboxStyle);
//     centreCheckBox->setStyleSheet(checkboxStyle);
//     aggregatedScriptCheckBox->setStyleSheet(checkboxStyle);

//     // Add to horizontal layout with equal stretch
//     trackLayout->addWidget(trackCheckBox);
//     trackLayout->addWidget(centreCheckBox);
//     trackLayout->addWidget(aggregatedScriptCheckBox);

//     // Set equal width for all checkboxes
//     trackLayout->setStretch(0, 1);
//     trackLayout->setStretch(1, 1);
//     trackLayout->setStretch(2, 1);

//     QWidget *trackWidget = new QWidget();
//     trackWidget->setLayout(trackLayout);
//     scrollLayout->addWidget(trackWidget);
// }
// // void EntityInfoDialog::createActiveSection()
// // {
// //     activeCheckBox = new QCheckBox("Active");
// //     activeCheckBox->setStyleSheet("QCheckBox { color: #000000; padding: 8px; }");
// //     activeCheckBox->setMinimumHeight(35);
// //     scrollLayout->addWidget(activeCheckBox);
// // }

// void EntityInfoDialog::createEquipmentSection()
// {
//     equipmentLayout = new QGridLayout();
//     equipmentLayout->setSpacing(3);

//     // Aapke existing buttons
//     sensorsButton = new QPushButton("Sensors");
//     radiosButton = new QPushButton("Radios");
//     iffButton = new QPushButton("IFF");
//     weaponsButton = new QPushButton("Weapons");
//     formationButton = new QPushButton("Formation");

//     // Simple button style - light gray
//     QString buttonStyle =
//         "QPushButton { "
//         "background-color: #f0f0f0; "
//         "color: #000000; "
//         "padding: 8px 12px; "
//         "border: 1px solid #cccccc; "
//         "border-radius: 3px; "
//         "margin: 2px; "
//         "}"
//         "QPushButton:hover { "
//         "background-color: #e0e0e0; "
//         "}";

//     sensorsButton->setStyleSheet(buttonStyle);
//     radiosButton->setStyleSheet(buttonStyle);
//     iffButton->setStyleSheet(buttonStyle);
//     weaponsButton->setStyleSheet(buttonStyle);
//     formationButton->setStyleSheet(buttonStyle);

//     weaponsButton->setMinimumHeight(35);
//     sensorsButton->setMinimumHeight(35);
//     formationButton->setMinimumHeight(35);
//     radiosButton->setMinimumHeight(35);
//     iffButton->setMinimumHeight(35);

//     // Aapka existing layout
//     equipmentLayout->addWidget(sensorsButton, 0, 0);
//     equipmentLayout->addWidget(radiosButton, 0, 1);
//     equipmentLayout->addWidget(iffButton, 1, 0);
//     equipmentLayout->addWidget(formationButton, 1, 1);
//     equipmentLayout->addWidget(weaponsButton, 2, 0);

//     // Connect signals - waise hi
//     connect(weaponsButton, &QPushButton::clicked, this, &EntityInfoDialog::onWeaponsClicked);
//     connect(sensorsButton, &QPushButton::clicked, this, &EntityInfoDialog::onSensorsClicked);
//     connect(formationButton, &QPushButton::clicked, this, &EntityInfoDialog::onFormationClicked);
//     connect(radiosButton, &QPushButton::clicked, this, &EntityInfoDialog::onRadiosClicked);
//     connect(iffButton, &QPushButton::clicked, this, &EntityInfoDialog::onIFFClicked);

//     QWidget *equipmentWidget = new QWidget();
//     equipmentWidget->setLayout(equipmentLayout);
//     scrollLayout->addWidget(equipmentWidget);
// }

// // void EntityInfoDialog::createOptionsSection()
// // {
// //     optionsLayout = new QVBoxLayout();
// //     optionsLayout->setSpacing(2);

// //     // Aapke existing checkboxes
// //     followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
// //     showConnectionCheckBox = new QCheckBox("Show Connection");
// //     showDetectionCheckBox = new QCheckBox("Show Detection");
// //     controlDecisiveCheckBox = new QCheckBox("Control Decisive");

// //     // Simple checkbox style
// //     QString checkboxStyle = "QCheckBox { color: #000000; padding: 8px; }";
// //     followTrajectoryCheckBox->setStyleSheet(checkboxStyle);
// //     showConnectionCheckBox->setStyleSheet(checkboxStyle);
// //     showDetectionCheckBox->setStyleSheet(checkboxStyle);
// //     controlDecisiveCheckBox->setStyleSheet(checkboxStyle);

// //     followTrajectoryCheckBox->setMinimumHeight(35);
// //     showConnectionCheckBox->setMinimumHeight(35);
// //     showDetectionCheckBox->setMinimumHeight(35);
// //     controlDecisiveCheckBox->setMinimumHeight(35);
// //     showDetectionCheckBox->setChecked(true);

// //     optionsLayout->addWidget(followTrajectoryCheckBox);
// //     optionsLayout->addWidget(showConnectionCheckBox);
// //     optionsLayout->addWidget(showDetectionCheckBox);
// //     optionsLayout->addWidget(controlDecisiveCheckBox);

// //     // Aapka existing connection
// //     connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
// //         if(!currentEntityId.isEmpty()){
// //             if(entryInfo){
// //                 entryInfo->detection = checked;
// //                 entryInfo->radioVisible = checked;
// //                 emit update();
// //             }
// //         }
// //     });

// //     QWidget *optionsWidget = new QWidget();
// //     optionsWidget->setLayout(optionsLayout);
// //     scrollLayout->addWidget(optionsWidget);
// // }
// void EntityInfoDialog::createOptionsSection()
// {
//     // Main vertical layout
//     optionsLayout = new QVBoxLayout();
//     optionsLayout->setSpacing(2);

//     // FIRST LINE: Active, Follow Trajectory, Show Detection
//     QHBoxLayout *firstLineLayout = new QHBoxLayout();
//     firstLineLayout->setSpacing(10);

//     // Active checkbox - NEW
//     QCheckBox *activeCheckBox = new QCheckBox("Active");
//     followTrajectoryCheckBox = new QCheckBox("Follow Trajectory");
//     showDetectionCheckBox = new QCheckBox("Show Detection");

//     // Simple checkbox style
//     QString checkboxStyle = "QCheckBox { color: #000000; }";
//     activeCheckBox->setStyleSheet(checkboxStyle);
//     followTrajectoryCheckBox->setStyleSheet(checkboxStyle);
//     showDetectionCheckBox->setStyleSheet(checkboxStyle);

//     // Add to first line
//     firstLineLayout->addWidget(activeCheckBox);
//     firstLineLayout->addWidget(followTrajectoryCheckBox);
//     firstLineLayout->addWidget(showDetectionCheckBox);
//     firstLineLayout->addStretch(); // Push to left

//     // SECOND LINE: Show Connection, Freeze Motion
//     QHBoxLayout *secondLineLayout = new QHBoxLayout();
//     secondLineLayout->setSpacing(10);

//     showConnectionCheckBox = new QCheckBox("Show Connection");
//     QCheckBox *freezeMotionCheckBox = new QCheckBox("Freeze Motion"); // NEW

//     showConnectionCheckBox->setStyleSheet(checkboxStyle);
//     freezeMotionCheckBox->setStyleSheet(checkboxStyle);

//     // Add to second line
//     secondLineLayout->addWidget(showConnectionCheckBox);
//     secondLineLayout->addWidget(freezeMotionCheckBox);
//     secondLineLayout->addStretch(); // Push to left

//     // Add both lines to main layout
//     optionsLayout->addLayout(firstLineLayout);
//     optionsLayout->addLayout(secondLineLayout);

//     showDetectionCheckBox->setChecked(true);

//     // Aapka existing connection
//     connect(showDetectionCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
//         if(!currentEntityId.isEmpty()){
//             if(entryInfo){
//                 entryInfo->detection = checked;
//                 entryInfo->radioVisible = checked;
//                 emit update();
//             }
//         }
//     });

//     QWidget *optionsWidget = new QWidget();
//     optionsWidget->setLayout(optionsLayout);
//     scrollLayout->addWidget(optionsWidget);
// }
// void EntityInfoDialog::setEntityInfo(const QString& entityId,  MeshEntry* info)
// {
//     currentEntityId = entityId;
//     entryInfo = info;

//     if (!entryInfo) {
//         titleLabel->setText("Entity: No Data");
//         clearInfo();
//         return;
//     }

//     titleLabel->setText("Entity: " + entityId);

//     // 🆕 UPDATE: FollowPath property se checkbox set karen
//     if (entryInfo->trajectory) {
//         followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
//     } else {
//         followTrajectoryCheckBox->setChecked(false);
//     }

//     if (entryInfo->entity) {
//         entryInfo->detection = showDetectionCheckBox->isChecked();
//     }
// }
// // void EntityInfoDialog::updateEntityInfo(){
// //     if(currentEntityId.isEmpty())return;
// //     if(entryInfo){
// //         if(entryInfo->entity){
// //             // Convert entity type to string
// //             QString typeStr = "Unknown";
// //             switch(entryInfo->entity->type) {
// //             case Constants::EntityType::Platform: typeStr = "Platform"; break;
// //             case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
// //             case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
// //             // Add other entity types as needed
// //             default: typeStr = "Unknown"; break;
// //             }

// //             // Update attribute table
// //             if (attributeTable) {
// //                 attributeTable->item(0, 1)->setText(typeStr);
// //                 attributeTable->item(1, 1)->setText(entryInfo->name);
// //                 attributeTable->item(2, 1)->setText(entryInfo->name);
// //             }

// //             // Update position section - show only current position
// //             QString currentPos = QString("Lat: %1, Long: %2")
// //                                      .arg(entryInfo->transform->translation().x())
// //                                      .arg(entryInfo->transform->translation().z());

// //             positionLabel->setText("Position: " + currentPos);

// //             // 🆕 UPDATE SPEED INFORMATION
// //             if (speedAltTable) {
// //                 if (entryInfo->dynamicModel) {
// //                     // Current speed
// //                     QString currentSpeed = QString("%1 km/h").arg(entryInfo->dynamicModel->moveSpeed);
// //                     speedAltTable->item(0, 1)->setText(currentSpeed);

// //                     // Requested speed (same as current for now, or you can add requested speed logic)
// //                     speedAltTable->item(0, 2)->setText(currentSpeed);
// //                 } else {
// //                     speedAltTable->item(0, 1)->setText("-");
// //                     speedAltTable->item(0, 2)->setText("-");
// //                 }

// //                 // 🆕 UPDATE ALTITUDE INFORMATION - USE LONGITUDE VALUE HERE
// //                 QString currentLongitude = QString("%1").arg(entryInfo->transform->translation().z());
// //                 speedAltTable->item(1, 1)->setText(currentLongitude);
// //                 speedAltTable->item(1, 2)->setText(currentLongitude); // Same as current for now
// //             }

// //         }
// //     }


// // }


// // void EntityInfoDialog::updateEntityInfo(){
// //     if(currentEntityId.isEmpty())return;
// //     if(entryInfo){
// //         if(entryInfo->entity){
// //             // Convert entity type to string
// //             QString typeStr = "Unknown";
// //             switch(entryInfo->entity->type) {
// //             case Constants::EntityType::Platform: typeStr = "Platform"; break;
// //             case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
// //             case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
// //             // Add other entity types as needed
// //             default: typeStr = "Unknown"; break;
// //             }

// //             // Update attribute table - NEW FIELDS
// //             if (attributeTable) {
// //                 attributeTable->item(0, 1)->setText(typeStr); // Type
// //                 attributeTable->item(1, 1)->setText(entryInfo->name); // DIS NAME
// //                 attributeTable->item(2, 1)->setText("0 %"); // Damages - aap yahan actual damages data set kar sakte hain
// //                 attributeTable->item(3, 1)->setText("Not Embarked"); // Carrier - aap yahan actual carrier data set kar sakte hain
// //             }

// //             // Update position section - show only current position
// //             QString currentPos = QString("Lat: %1, Long: %2")
// //                                      .arg(entryInfo->transform->translation().x())
// //                                      .arg(entryInfo->transform->translation().z());

// //             positionLabel->setText("Position: " + currentPos);

// //             // 🆕 UPDATE SPEED INFORMATION
// //             if (speedAltTable) {
// //                 if (entryInfo->dynamicModel) {
// //                     // Current speed
// //                     QString currentSpeed = QString("%1 km/h").arg(entryInfo->dynamicModel->moveSpeed);
// //                     speedAltTable->item(0, 1)->setText(currentSpeed);

// //                     // Requested speed (same as current for now, or you can add requested speed logic)
// //                     speedAltTable->item(0, 2)->setText(currentSpeed);
// //                 } else {
// //                     speedAltTable->item(0, 1)->setText("-");
// //                     speedAltTable->item(0, 2)->setText("-");
// //                 }

// //                 // 🆕 UPDATE ALTITUDE INFORMATION - USE LONGITUDE VALUE HERE
// //                 QString currentLongitude = QString("%1").arg(entryInfo->transform->translation().z());
// //                 speedAltTable->item(1, 1)->setText(currentLongitude);
// //                 speedAltTable->item(1, 2)->setText(currentLongitude); // Same as current for now
// //             }

// //         }
// //     }
// // }

// void EntityInfoDialog::updateEntityInfo(){
//     if(currentEntityId.isEmpty())return;
//     if(entryInfo){
//         if(entryInfo->entity){
//             // Convert entity type to string
//             QString typeStr = "Unknown";
//             switch(entryInfo->entity->type) {
//             case Constants::EntityType::Platform: typeStr = "Platform"; break;
//             case Constants::EntityType::Sensor: typeStr = "Sensor"; break;
//             case Constants::EntityType::Weapon: typeStr = "Weapon"; break;
//             // Add other entity types as needed
//             default: typeStr = "Unknown"; break;
//             }

//             // Update attribute table - NEW FIELDS
//             if (attributeTable) {
//                 attributeTable->item(0, 1)->setText(typeStr); // Type
//                 attributeTable->item(1, 1)->setText(entryInfo->name); // Name
//                 attributeTable->item(2, 1)->setText("-"); // DIS name - empty for now
//                 attributeTable->item(3, 1)->setText("0 %"); // Damages
//                 attributeTable->item(4, 1)->setText("Not Embarked"); // Carrier
//             }

//             // Baaki sab code waise hi...
//             // Update position section - show only current position
//             QString currentPos = QString("Lat: %1, Long: %2")
//                                      .arg(entryInfo->transform->translation().x())
//                                      .arg(entryInfo->transform->translation().z());

//             positionLabel->setText("Position: " + currentPos);

//             // 🆕 UPDATE SPEED INFORMATION
//             if (speedAltTable) {
//                 if (entryInfo->dynamicModel) {
//                     // Current speed
//                     QString currentSpeed = QString("%1 km/h").arg(entryInfo->dynamicModel->moveSpeed);
//                     speedAltTable->item(0, 1)->setText(currentSpeed);

//                     // Requested speed (same as current for now, or you can add requested speed logic)
//                     speedAltTable->item(0, 2)->setText(currentSpeed);
//                 } else {
//                     speedAltTable->item(0, 1)->setText("-");
//                     speedAltTable->item(0, 2)->setText("-");
//                 }

//                 // 🆕 UPDATE ALTITUDE INFORMATION - USE LONGITUDE VALUE HERE
//                 QString currentLongitude = QString("%1").arg(entryInfo->transform->translation().z());
//                 speedAltTable->item(1, 1)->setText(currentLongitude);
//                 speedAltTable->item(1, 2)->setText(currentLongitude); // Same as current for now
//             }

//         }
//     }
// }
// // void EntityInfoDialog::clearInfo()
// // {
// //     titleLabel->setText("Entity Information");
// //     currentEntityId.clear();
// //     currentEntityData.clear();

// //     // Reset attribute table
// //     if (attributeTable) {
// //         attributeTable->item(0, 1)->setText("-");
// //         attributeTable->item(1, 1)->setText("-");
// //         attributeTable->item(2, 1)->setText("-");
// //     }

// //     // Reset carrier section
// //     carrierLabel->setText("Carrier: -");

// //     // Reset position section
// //     positionLabel->setText("Position: -");

// //     // Reset Speed and Altitude table
// //     if (speedAltTable) {
// //         speedAltTable->item(0, 1)->setText("-");
// //         speedAltTable->item(0, 2)->setText("-");
// //         speedAltTable->item(1, 1)->setText("-");
// //         speedAltTable->item(1, 2)->setText("-");
// //     }

// //     // Uncheck all checkboxes EXCEPT Show Detection
// //     trackCheckBox->setChecked(false);
// //     centreCheckBox->setChecked(false);
// //     aggregatedScriptCheckBox->setChecked(false);
// //     activeCheckBox->setChecked(false);
// //     followTrajectoryCheckBox->setChecked(false);
// //     showConnectionCheckBox->setChecked(false);
// //     // 🆕 SHOW DETECTION KO CHECKED RAKHEN
// //     showDetectionCheckBox->setChecked(true);  // YAHAN CHANGE KAREN
// //     controlDecisiveCheckBox->setChecked(false);
// // }
// // void EntityInfoDialog::clearInfo()
// // {
// //     titleLabel->setText("Entity Information");
// //     currentEntityId.clear();
// //     currentEntityData.clear();

// //     // Reset attribute table - NEW FIELDS
// //     if (attributeTable) {
// //         attributeTable->item(0, 1)->setText("-"); // Type
// //         attributeTable->item(1, 1)->setText("-"); // DIS NAME
// //         attributeTable->item(2, 1)->setText("-"); // Damages
// //         attributeTable->item(3, 1)->setText("-"); // Carrier
// //     }

// //     // Carrier section ab table mein hai, isliye hide karen
// //     carrierLabel->setText("");

// //     // Reset position section
// //     positionLabel->setText("Position: -");

// //     // Reset Speed and Altitude table
// //     if (speedAltTable) {
// //         speedAltTable->item(0, 1)->setText("-");
// //         speedAltTable->item(0, 2)->setText("-");
// //         speedAltTable->item(1, 1)->setText("-");
// //         speedAltTable->item(1, 2)->setText("-");
// //     }

// //     // Uncheck all checkboxes EXCEPT Show Detection
// //     trackCheckBox->setChecked(false);
// //     centreCheckBox->setChecked(false);
// //     aggregatedScriptCheckBox->setChecked(false);
// //     activeCheckBox->setChecked(false);
// //     followTrajectoryCheckBox->setChecked(false);
// //     showConnectionCheckBox->setChecked(false);
// //     // 🆕 SHOW DETECTION KO CHECKED RAKHEN
// //     showDetectionCheckBox->setChecked(true);  // YAHAN CHANGE KAREN
// //     controlDecisiveCheckBox->setChecked(false);
// // }


// void EntityInfoDialog::clearInfo()
// {
//     titleLabel->setText("Entity Information");
//     currentEntityId.clear();
//     currentEntityData.clear();

//     // Reset attribute table - NEW FIELDS
//     if (attributeTable) {
//         attributeTable->item(0, 1)->setText("-"); // Type
//         attributeTable->item(1, 1)->setText("-"); // Name
//         attributeTable->item(2, 1)->setText("-"); // DIS name
//         attributeTable->item(3, 1)->setText("-"); // Damages
//         attributeTable->item(4, 1)->setText("-"); // Carrier
//     }

//     // Carrier section ab table mein hai, isliye hide karen
//     carrierLabel->setText("");

//     // Baaki sab code waise hi...
//     // Reset position section
//     positionLabel->setText("Position: -");

//     // Reset Speed and Altitude table
//     if (speedAltTable) {
//         speedAltTable->item(0, 1)->setText("-");
//         speedAltTable->item(0, 2)->setText("-");
//         speedAltTable->item(1, 1)->setText("-");
//         speedAltTable->item(1, 2)->setText("-");
//     }

//     // Uncheck all checkboxes EXCEPT Show Detection
//     trackCheckBox->setChecked(false);
//     centreCheckBox->setChecked(false);
//     aggregatedScriptCheckBox->setChecked(false);
//     activeCheckBox->setChecked(false);
//     followTrajectoryCheckBox->setChecked(false);
//     showConnectionCheckBox->setChecked(false);
//     showDetectionCheckBox->setChecked(true);
//     controlDecisiveCheckBox->setChecked(false);
// }
// void EntityInfoDialog::onCloseClicked()
// {
//     hide();
// }

// void EntityInfoDialog::onWeaponsClicked()
// {
//     // Show weapons list popup
//     QMessageBox::information(this, "Weapons",
//                              QString("Weapons for entity %1:\n%2")
//                                  .arg(currentEntityId)
//                                  .arg(currentEntityData.value("weapons", "No weapons data").toString()));
// }


// void EntityInfoDialog::onSensorsClicked()
// {
//     if(!currentEntityId.isEmpty()){
//         if(entryInfo){
//             if(entryInfo->entity){
//                 // Create a simple clean dialog for sensors
//                 QDialog *sensorsDialog = new QDialog(this);
//                 sensorsDialog->setWindowTitle("Sensors - " + currentEntityId);
//                 sensorsDialog->setMinimumSize(450, 300);
//                 sensorsDialog->setMaximumSize(500, 400);

//                 // Simple window flags
//                 sensorsDialog->setWindowFlags(Qt::Dialog);

//                 // Minimal styling - clean and professional
//                 sensorsDialog->setStyleSheet(
//                     "QDialog { background-color: white; border: 1px solid #ccc; }"
//                     "QLabel { color: #333; font-weight: normal; }"
//                     "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
//                     "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
//                     "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
//                     "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
//                     "QPushButton:hover { background-color: #005a9e; }"
//                     );

//                 QVBoxLayout *layout = new QVBoxLayout(sensorsDialog);
//                 layout->setSpacing(8);
//                 layout->setContentsMargins(10, 10, 10, 10);

//                 // Simple title
//                 QLabel *titleLabel = new QLabel("Sensor Systems");
//                 titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
//                 layout->addWidget(titleLabel);

//                 // Create clean table
//                 QTableWidget *sensorsTable = new QTableWidget();
//                 sensorsTable->setColumnCount(4);
//                 sensorsTable->setHorizontalHeaderLabels(QStringList()
//                                                         << "Name" << "Type" << "Range" << "FOV");

//                 sensorsTable->verticalHeader()->setVisible(false);
//                 sensorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//                 sensorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//                 sensorsTable->setSelectionMode(QAbstractItemView::SingleSelection);

//                 // Clean table properties
//                 sensorsTable->setShowGrid(true);
//                 sensorsTable->setAlternatingRowColors(false);

//                 // Real-time update timer
//                 QTimer *sensorsUpdateTimer = new QTimer(sensorsDialog);
//                 QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
//                     updateSensorsTable(sensorsTable, entryInfo->entity);
//                 });
//                 sensorsUpdateTimer->start(100);

//                 // Initial population
//                 updateSensorsTable(sensorsTable, entryInfo->entity);

//                 // Simple column sizing
//                 sensorsTable->horizontalHeader()->setStretchLastSection(false);
//                 sensorsTable->setColumnWidth(0, 120); // Name
//                 sensorsTable->setColumnWidth(1, 80);  // Type
//                 sensorsTable->setColumnWidth(2, 80);  // Range
//                 sensorsTable->setColumnWidth(3, 60);  // FOV

//                 layout->addWidget(sensorsTable);

//                 // Simple summary
//                 QLabel *summaryLabel = new QLabel();
//                 summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
//                 layout->addWidget(summaryLabel);

//                 // Update summary
//                 QObject::connect(sensorsUpdateTimer, &QTimer::timeout, sensorsDialog, [=]() {
//                     int total = 0;
//                     if(entryInfo && entryInfo->entity) {
//                         for (Sensor* s : entryInfo->entity->sensorList) {
//                             if(s->subType == Sensor::SubType::Generic ||
//                                 s->subType == Sensor::SubType::CSM ||
//                                 s->subType == Sensor::SubType::ESM) {
//                                 total++;
//                             }
//                         }
//                     }
//                     summaryLabel->setText(QString("Total: %1 sensors").arg(total));
//                 });

//                 // Simple close button
//                 QHBoxLayout *buttonLayout = new QHBoxLayout();
//                 buttonLayout->addStretch();
//                 QPushButton *closeButton = new QPushButton("Close");
//                 QObject::connect(closeButton, &QPushButton::clicked, sensorsDialog, [=]() {
//                     sensorsUpdateTimer->stop();
//                     sensorsDialog->close();
//                 });
//                 buttonLayout->addWidget(closeButton);
//                 layout->addLayout(buttonLayout);

//                 // Show dialog
//                 sensorsDialog->show();
//                 sensorsDialog->setAttribute(Qt::WA_DeleteOnClose);

//             }
//         }
//     } else {
//         QMessageBox::information(this, "Sensors", "No entity selected.");
//     }
// }

// void EntityInfoDialog::updateSensorsTable(QTableWidget* sensorsTable, Entity* entity)
// {
//     if(!sensorsTable || !entity) return;

//     sensorsTable->setRowCount(0);
//     int row = 0;

//     for (Sensor* s : entity->sensorList) {
//         if(s->subType == Sensor::SubType::Generic ||
//             s->subType == Sensor::SubType::CSM ||
//             s->subType == Sensor::SubType::ESM) {

//             sensorsTable->insertRow(row);

//             // Name
//             QString sensorName = QString::fromStdString(s->Name);
//             sensorsTable->setItem(row, 0, new QTableWidgetItem(sensorName));

//             // Type
//             QString typeStr;
//             if(s->subType == Sensor::SubType::Generic) typeStr = "Radar";
//             else if(s->subType == Sensor::SubType::CSM) typeStr = "CSM";
//             else if(s->subType == Sensor::SubType::ESM) typeStr = "ESM";
//             sensorsTable->setItem(row, 1, new QTableWidgetItem(typeStr));

//             // Range
//             QString rangeStr;
//             if(s->subType == Sensor::SubType::Generic) rangeStr = QString("%1").arg(s->range, 0, 'f', 1);
//             else if(s->subType == Sensor::SubType::CSM) rangeStr = QString("%1").arg(s->csmrange, 0, 'f', 1);
//             else if(s->subType == Sensor::SubType::ESM) rangeStr = QString("%1").arg(s->esrange, 0, 'f', 1);
//             sensorsTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

//             // FOV
//             QString fovStr;
//             if(s->subType == Sensor::SubType::Generic) fovStr = QString("%1°").arg(s->maxDetectionAngle, 0, 'f', 0);
//             else fovStr = "-";
//             sensorsTable->setItem(row, 3, new QTableWidgetItem(fovStr));

//             row++;
//         }
//     }

//     if (row == 0) {
//         sensorsTable->setRowCount(1);
//         sensorsTable->setItem(0, 0, new QTableWidgetItem("No sensors"));
//         sensorsTable->setItem(0, 1, new QTableWidgetItem(""));
//         sensorsTable->setItem(0, 2, new QTableWidgetItem(""));
//         sensorsTable->setItem(0, 3, new QTableWidgetItem(""));
//     }
// }

// void EntityInfoDialog::onFormationClicked()
// {
//     // Show formation details popup
//     QMessageBox::information(this, "Formation",
//                              QString("Formation for entity %1:\n%2")
//                                  .arg(currentEntityId)
//                                  .arg(currentEntityData.value("formation", "No formation data").toString()));
// }



// void EntityInfoDialog::onRadiosClicked()
// {
//     if(!currentEntityId.isEmpty()){
//         if(entryInfo){
//             if(entryInfo->entity){
//                 // Create a simple clean dialog for radios - EXACTLY LIKE SENSORS
//                 QDialog *radiosDialog = new QDialog(this);
//                 radiosDialog->setWindowTitle("Radios - " + currentEntityId);
//                 radiosDialog->setMinimumSize(300, 300);
//                 radiosDialog->setMaximumSize(400, 400);

//                 radiosDialog->setWindowFlags(Qt::Dialog);
//                 radiosDialog->setStyleSheet(
//                     "QDialog { background-color: white; border: 1px solid #ccc; }"
//                     "QLabel { color: #333; font-weight: normal; }"
//                     "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
//                     "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
//                     "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
//                     "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
//                     "QPushButton:hover { background-color: #005a9e; }"
//                     );

//                 QVBoxLayout *layout = new QVBoxLayout(radiosDialog);
//                 layout->setSpacing(8);
//                 layout->setContentsMargins(10, 10, 10, 10);

//                 QLabel *titleLabel = new QLabel("Radio Communication Systems");
//                 titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
//                 layout->addWidget(titleLabel);

//                 QTableWidget *radiosTable = new QTableWidget();
//                 radiosTable->setColumnCount(2);
//                 radiosTable->setHorizontalHeaderLabels(QStringList() << "Name" << "Range");

//                 radiosTable->verticalHeader()->setVisible(false);
//                 radiosTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//                 radiosTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//                 radiosTable->setSelectionMode(QAbstractItemView::SingleSelection);
//                 radiosTable->setShowGrid(true);
//                 radiosTable->setAlternatingRowColors(false);

//                 // Real-time update timer - ALL CODE INSIDE LAMBDA
//                 QTimer *radiosUpdateTimer = new QTimer(radiosDialog);
//                 QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
//                     if(!radiosTable || !entryInfo->entity) return;

//                     radiosTable->setRowCount(0);
//                     int row = 0;

//                     for (Radio* radio : entryInfo->entity->radioList) {
//                         radiosTable->insertRow(row);

//                         // NAME - First field
//                         QString radioName = QString::fromStdString(radio->Name);
//                         radiosTable->setItem(row, 0, new QTableWidgetItem(radioName));

//                         // RANGE - Second field
//                         QString rangeStr = "N/A";
//                         if (radio->Range > 0) {
//                             rangeStr = QString("%1 km").arg(radio->Range, 0, 'f', 1);
//                         }
//                         radiosTable->setItem(row, 1, new QTableWidgetItem(rangeStr));

//                         row++;
//                     }

//                     if (row == 0) {
//                         radiosTable->setRowCount(1);
//                         radiosTable->setItem(0, 0, new QTableWidgetItem("No radios"));
//                         radiosTable->setItem(0, 1, new QTableWidgetItem(""));
//                     }
//                 });
//                 radiosUpdateTimer->start(100);

//                 radiosTable->horizontalHeader()->setStretchLastSection(false);
//                 radiosTable->setColumnWidth(0, 120);
//                 radiosTable->setColumnWidth(1, 80);

//                 layout->addWidget(radiosTable);

//                 QLabel *summaryLabel = new QLabel();
//                 summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
//                 layout->addWidget(summaryLabel);

//                 QObject::connect(radiosUpdateTimer, &QTimer::timeout, radiosDialog, [=]() {
//                     int total = 0;
//                     if(entryInfo && entryInfo->entity) {
//                         total = entryInfo->entity->radioList.size();
//                     }
//                     summaryLabel->setText(QString("Total: %1 radios").arg(total));
//                 });

//                 QHBoxLayout *buttonLayout = new QHBoxLayout();
//                 buttonLayout->addStretch();
//                 QPushButton *closeButton = new QPushButton("Close");
//                 QObject::connect(closeButton, &QPushButton::clicked, radiosDialog, [=]() {
//                     radiosUpdateTimer->stop();
//                     radiosDialog->close();
//                 });
//                 buttonLayout->addWidget(closeButton);
//                 layout->addLayout(buttonLayout);

//                 radiosDialog->show();
//                 radiosDialog->setAttribute(Qt::WA_DeleteOnClose);
//             }
//         }
//     } else {
//         QMessageBox::information(this, "Radios", "No entity selected.");
//     }
// }



// void EntityInfoDialog::onIFFClicked()
// {
//     if(!currentEntityId.isEmpty()){
//         if(entryInfo){
//             if(entryInfo->entity){
//                 // Create a simple clean dialog for IFF - EXACTLY LIKE SENSORS AND RADIOS
//                 QDialog *iffDialog = new QDialog(this);
//                 iffDialog->setWindowTitle("IFF Systems - " + currentEntityId);
//                 iffDialog->setMinimumSize(450, 300);
//                 iffDialog->setMaximumSize(500, 400);

//                 iffDialog->setWindowFlags(Qt::Dialog);
//                 iffDialog->setStyleSheet(
//                     "QDialog { background-color: white; border: 1px solid #ccc; }"
//                     "QLabel { color: #333; font-weight: normal; }"
//                     "QTableWidget { border: 1px solid #ddd; gridline-color: #eee; }"
//                     "QHeaderView::section { background-color: #f5f5f5; color: #333; padding: 6px; border: none; }"
//                     "QTableWidget::item { padding: 4px; border-bottom: 1px solid #f0f0f0; }"
//                     "QPushButton { background-color: #007acc; color: white; padding: 6px 12px; border: none; border-radius: 3px; }"
//                     "QPushButton:hover { background-color: #005a9e; }"
//                     );

//                 QVBoxLayout *layout = new QVBoxLayout(iffDialog);
//                 layout->setSpacing(8);
//                 layout->setContentsMargins(10, 10, 10, 10);

//                 QLabel *titleLabel = new QLabel("IFF (Identification Friend or Foe) Systems");
//                 titleLabel->setStyleSheet("QLabel { font-size: 14px; color: #222; font-weight: bold; }");
//                 layout->addWidget(titleLabel);

//                 QTableWidget *iffTable = new QTableWidget();
//                 iffTable->setColumnCount(3);
//                 iffTable->setHorizontalHeaderLabels(QStringList()
//                                                     << "Name" << "Mode" << "Range");

//                 iffTable->verticalHeader()->setVisible(false);
//                 iffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
//                 iffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//                 iffTable->setSelectionMode(QAbstractItemView::SingleSelection);
//                 iffTable->setShowGrid(true);
//                 iffTable->setAlternatingRowColors(false);

//                 // Real-time update timer
//                 QTimer *iffUpdateTimer = new QTimer(iffDialog);
//                 QObject::connect(iffUpdateTimer, &QTimer::timeout, iffDialog, [=]() {
//                     if(!iffTable || !entryInfo->entity) return;

//                     iffTable->setRowCount(0);
//                     int row = 0;

//                     for (IFF* iff : entryInfo->entity->iffList) {
//                         iffTable->insertRow(row);

//                         // NAME - First field
//                         QString iffName = QString::fromStdString(iff->Name);
//                         iffTable->setItem(row, 0, new QTableWidgetItem(iffName));

//                         // MODE - Second field
//                         QString modeStr = "Active";
//                         switch(iff->operationalMode) {
//                         case IFF::OperationalMode::Active: modeStr = "Active"; break;
//                         case IFF::OperationalMode::Passive: modeStr = "Passive"; break;
//                         case IFF::OperationalMode::Off: modeStr = "Off"; break;
//                         case IFF::OperationalMode::Simulation: modeStr = "Simulation"; break;
//                         default: modeStr = "Unknown"; break;
//                         }
//                         iffTable->setItem(row, 1, new QTableWidgetItem(modeStr));

//                         // RANGE - Third field
//                         QString rangeStr = QString("%1 km").arg(iff->emittingRange, 0, 'f', 1);
//                         iffTable->setItem(row, 2, new QTableWidgetItem(rangeStr));

//                         row++;
//                     }

//                     if (row == 0) {
//                         iffTable->setRowCount(1);
//                         iffTable->setItem(0, 0, new QTableWidgetItem("No IFF systems"));
//                         iffTable->setItem(0, 1, new QTableWidgetItem(""));
//                         iffTable->setItem(0, 2, new QTableWidgetItem(""));
//                     }
//                 });
//                 iffUpdateTimer->start(100);

//                 // Column sizing - same as sensors
//                 iffTable->horizontalHeader()->setStretchLastSection(false);
//                 iffTable->setColumnWidth(0, 120); // Name
//                 iffTable->setColumnWidth(1, 80);  // Mode
//                 iffTable->setColumnWidth(2, 80);  // Range

//                 layout->addWidget(iffTable);

//                 // Simple summary
//                 QLabel *summaryLabel = new QLabel();
//                 summaryLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");
//                 layout->addWidget(summaryLabel);

//                 // Update summary
//                 QObject::connect(iffUpdateTimer, &QTimer::timeout, iffDialog, [=]() {
//                     int total = 0;
//                     int active = 0;
//                     if(entryInfo && entryInfo->entity) {
//                         total = entryInfo->entity->iffList.size();
//                         for (IFF* iff : entryInfo->entity->iffList) {
//                             if(iff->operationalMode != IFF::OperationalMode::Off) {
//                                 active++;
//                             }
//                         }
//                     }
//                     summaryLabel->setText(QString("Total: %1 IFF systems | Active: %2").arg(total).arg(active));
//                 });

//                 // Simple close button
//                 QHBoxLayout *buttonLayout = new QHBoxLayout();
//                 buttonLayout->addStretch();
//                 QPushButton *closeButton = new QPushButton("Close");
//                 QObject::connect(closeButton, &QPushButton::clicked, iffDialog, [=]() {
//                     iffUpdateTimer->stop();
//                     iffDialog->close();
//                 });
//                 buttonLayout->addWidget(closeButton);
//                 layout->addLayout(buttonLayout);

//                 iffDialog->show();
//                 iffDialog->setAttribute(Qt::WA_DeleteOnClose);
//             }
//         }
//     } else {
//         QMessageBox::information(this, "IFF", "No entity selected.");
//     }
// }







//====================================['kp['pi[i[-i[iu0-pu0=gujf===============================================================



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
#include "qtimer.h"

EntityInfoDialog::EntityInfoDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();


    connect(followTrajectoryCheckBox, &QCheckBox::clicked, this, [=](bool checked) {
        if(!currentEntityId.isEmpty() && entryInfo) {
            if (entryInfo->dynamicModel) {
                entryInfo->dynamicModel->followPath = checked;

                if (!checked) {
                    // STOP - only set movement variables to zero
                    entryInfo->dynamicModel->speeed = 0;
                    entryInfo->dynamicModel->currentSpeed = 0;
                    // 🆕 DON'T change moveSpeed
                } else {

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

    // Close button - aapka existing waise hi
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
    trackLayout->setSpacing(0); // No space between checkboxes

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

    // 🆕 UPDATE: FollowPath property se checkbox set karen
    if (entryInfo->trajectory) {
        followTrajectoryCheckBox->setChecked(entryInfo->trajectory->FollowPath);
    } else {
        followTrajectoryCheckBox->setChecked(false);
    }

    if (entryInfo->entity) {
        entryInfo->detection = showDetectionCheckBox->isChecked();
    }
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

            // Update attribute table - NEW FIELDS
            if (attributeTable) {
                attributeTable->item(0, 1)->setText(typeStr); // Type
                attributeTable->item(1, 1)->setText(entryInfo->name); // Name
                attributeTable->item(2, 1)->setText("-"); // DIS name - empty for now
                attributeTable->item(3, 1)->setText("0 %"); // Damages
                attributeTable->item(4, 1)->setText("Not Embarked"); // Carrier
            }

            // Baaki sab code waise hi...
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

    // Reset attribute table - NEW FIELDS
    if (attributeTable) {
        attributeTable->item(0, 1)->setText("-"); // Type
        attributeTable->item(1, 1)->setText("-"); // Name
        attributeTable->item(2, 1)->setText("-"); // DIS name
        attributeTable->item(3, 1)->setText("-"); // Damages
        attributeTable->item(4, 1)->setText("-"); // Carrier
    }

    // Carrier section ab table mein hai, isliye hide karen
    carrierLabel->setText("");

    // Baaki sab code waise hi...
    // Reset position section
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
                    if(!radiosTable || !entryInfo->entity) return;

                    radiosTable->setRowCount(0);
                    int row = 0;

                    for (Radio* radio : entryInfo->entity->radioList) {
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
                    if(entryInfo && entryInfo->entity) {
                        total = entryInfo->entity->radioList.size();
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
                    if(!iffTable || !entryInfo->entity) return;

                    iffTable->setRowCount(0);
                    int row = 0;

                    for (IFF* iff : entryInfo->entity->iffList) {
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
