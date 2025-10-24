
#include <QtGui/QIcon>
#include "feedback.h"
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QDebug>
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/entity.h"
#include "core/Hierarchy/EntityProfiles/iff.h"


Feedback::Feedback(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    // Load dynamic data from Hierarchy instead of hardcoded JSON
    // loadDashboardData();
    // loadDashboardData("{}");
}



Feedback::~Feedback()
{
}

void Feedback::setupUi()
{
    // Create the central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create a QScrollArea to make the entire window scrollable
    QScrollArea *scrollArea = new QScrollArea(centralWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: black; border: none; }");

    // Create a container widget for the scroll area
    QWidget *scrollContent = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(scrollContent);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Apply global stylesheet to the scroll content
    scrollContent->setStyleSheet(
        "background-color: black; color: #ffffff;"
        "QLabel { font-size: 14px; font-family: Arial; }"
        "QTableWidget { background-color: #2a3555; color: #ffffff; gridline-color: #3a4565; font-size: 10px; }"
        "QTableWidget::item { padding: 2px; }"
        "QHeaderView::section { background-color: #2a3555; color: #ffffff; font-size: 10px; padding: 2px; text-align: center; border: none; }"
        "QTextEdit { background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; font-size: 12px; }"
        "QProgressBar { background-color: #3a4565; border: 1px solid #3a4565; color: #ffffff; text-align: center; font-size: 12px; height: 15px; }"
        "QProgressBar::chunk { background-color: #4a90e2; }"
        "QComboBox { background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 10px; height: 10px; }"
        "QListWidget { background-color: #1e2a44; color: #ffffff; border: 1px solid #3a4565; font-size: 14px; }"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: #4a90e2; color: #ffffff; }"
        "QPushButton { background-color: #4a90e2; color: #ffffff; border: 1px solid #3a4565; padding: 5px; font-size: 12px; }"
        "QPushButton:hover { background-color: #5aa0f2; }"
        );

    // Sidebar setup
    sidebar = new QListWidget();
    sidebar->setFixedWidth(200);
    sidebar->addItem("Overview");
    sidebar->addItem("Storage");
    sidebar->addItem("Sensors");
    sidebar->addItem("Radio");
    sidebar->addItem("Network");
    sidebar->addItem("Logs");
    sidebar->addItem("CanvasWidget");
    sidebar->addItem("Entity");
    sidebar->addItem("IFF");
    sidebar->addItem("Formation");
    sidebar->addItem("FixedPoint");
    sidebar->addItem("Weapon");
    sidebar->setCurrentRow(0);
    connect(sidebar, &QListWidget::itemClicked, this, &Feedback::onSidebarItemClicked);
    mainLayout->addWidget(sidebar);

    // Content stack setup
    contentStack = new QStackedWidget();
    mainLayout->addWidget(contentStack);

    // Setup each section's widget
    setupOverviewWidget();
    setupStorageWidget();
    setupSensorsWidget();
    setupRadioWidget();
    setupNetworkWidget();
    setupLogsWidget();
    setupCanvasWidget();
    setupEntityWidget();
    setupIffWidget();
    setupFormationWidget();
    setupFixedPointWidget();
    setupWeaponWidget();

    // Set Entity tab as the initial widget to force rendering
    contentStack->setCurrentWidget(entityFrame);

    // Use a QTimer to switch back to Overview after the window is shown
    QTimer::singleShot(0, this, [this]() {
        contentStack->setCurrentWidget(overviewWidget);
        qDebug() << "Switched back to Overview tab after rendering Entity tab";
    });

    // Set the scroll content as the scroll area's widget
    scrollArea->setWidget(scrollContent);

    // Add the scroll area to the central widget's layout
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->addWidget(scrollArea);

    setWindowTitle("Feedback Analysis");
    resize(800, 600);
}

void Feedback::setupOverviewWidget()
{
    overviewWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(overviewWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *overviewFrame = new QFrame();
    overviewFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *overviewFrameLayout = new QVBoxLayout(overviewFrame);

    QLabel *header = new QLabel("System Overview");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    overviewFrameLayout->addWidget(header);

    QGridLayout *metricsLayout = new QGridLayout();
    metricsLayout->setVerticalSpacing(15);
    metricsLayout->setHorizontalSpacing(30);

    systemStatusLabel = new QLabel("System: Unknown  Sim: Unknown  RTC: Unknown");
    systemStatusLabel->setStyleSheet(
        "font-size: 14px;"
        "border: 1px solid #3a4565;"
        "border-radius: 3px;"
        "padding: 5px;"
        "background-color: #2a3552;"
        "color: #ffffff;"
        );
    metricsLayout->addWidget(systemStatusLabel, 0, 0, 1, 3);

    uptimeLabel = new QLabel("Uptime: Unknown");
    uptimeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(uptimeLabel, 1, 0);

    entitiesLabel = new QLabel("Entities: Unknown");
    entitiesLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(entitiesLabel, 2, 0);

    feedbackEventsLabel = new QLabel("Accumulated Feedback Events: Unknown");
    feedbackEventsLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(feedbackEventsLabel, 3, 0);

    QLabel *accuracyLabel = new QLabel("Simulation Accuracy: Unknown");
    accuracyLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(accuracyLabel, 4, 0);

    cpuProgressBar = new QProgressBar();
    cpuProgressBar->setRange(0, 100);
    cpuProgressBar->setValue(0);
    cpuProgressBar->setTextVisible(true);
    cpuProgressBar->setFormat("CPU: %p%");
    cpuProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #3a4565;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "   font-size: 12px;"
        "   height: 20px;"
        "   background-color: #3a4565;"
        "   color: #ffffff;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4a90e2;"
        "   border-radius: 4px;"
        "}"
        );
    metricsLayout->addWidget(cpuProgressBar, 5, 0, 1, 3);

    overviewFrameLayout->addLayout(metricsLayout);
    mainLayout->addWidget(overviewFrame);
    mainLayout->addStretch();

    overviewWidget->setLayout(mainLayout);
    contentStack->addWidget(overviewWidget);
}
void Feedback::setupStorageWidget()
{
    storageWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(storageWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *storageFrame = new QFrame();
    storageFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *storageFrameLayout = new QVBoxLayout(storageFrame);

    QLabel *header = new QLabel("Storage Usage");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    storageFrameLayout->addWidget(header);

    QHBoxLayout *storageContentLayout = new QHBoxLayout();
    storageContentLayout->setSpacing(20);
    storageContentLayout->setAlignment(Qt::AlignTop);

    storageChart = new CustomGraphWidget(CustomGraphWidget::Pie);
    storageChart->setFixedSize(150, 100);
    storageContentLayout->addWidget(storageChart);

    QVBoxLayout *storageTextLayout = new QVBoxLayout();
    storageTextLayout->setSpacing(5);
    storageTextLayout->setAlignment(Qt::AlignTop);

    QString labelStyle =
        "color: white;"
        "background-color: #182038;"
        "border: 1px solid #3a4565;"
        "border-radius: 5px;"
        "padding: 4px 8px;"
        "margin-bottom: 5px;"
        "min-height: 28px;"
        "max-height: 28px;"
        "min-width: 200px;";

    mongoDbLabel = new QLabel("MongoDB: Unknown");
    mongoDbLabel->setStyleSheet(labelStyle);
    mongoDbLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    logsLabel = new QLabel("Logs: Unknown");
    logsLabel->setStyleSheet(labelStyle);
    logsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    scenariosLabel = new QLabel("Scenarios: Unknown");
    scenariosLabel->setStyleSheet(labelStyle);
    scenariosLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    totalLabel = new QLabel("Total: Unknown");
    totalLabel->setStyleSheet(labelStyle);
    totalLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    storageTextLayout->addWidget(mongoDbLabel);
    storageTextLayout->addWidget(logsLabel);
    storageTextLayout->addWidget(scenariosLabel);
    storageTextLayout->addWidget(totalLabel);

    storageContentLayout->addLayout(storageTextLayout);
    storageFrameLayout->addLayout(storageContentLayout);
    mainLayout->addWidget(storageFrame);
    mainLayout->addStretch();

    storageWidget->setLayout(mainLayout);
    contentStack->addWidget(storageWidget);
}
void Feedback::setupSensorsWidget()
{
    sensorsWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(sensorsWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *sensorsFrame = new QFrame();
    sensorsFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 5px;");
    QVBoxLayout *sensorsFrameLayout = new QVBoxLayout(sensorsFrame);

    QLabel *header = new QLabel("Sensors");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    sensorsFrameLayout->addWidget(header);

    QLabel *sensorListHeader = new QLabel("Sensor List");
    sensorListHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; margin-bottom: 10px;");
    sensorsFrameLayout->addWidget(sensorListHeader);

    sensorTable = new QTableWidget();
    sensorTable->setColumnCount(2);
    sensorTable->setHorizontalHeaderLabels({"Type", "Status"});
    sensorTable->setRowCount(0); // Initialize with zero rows
    sensorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sensorTable->verticalHeader()->setVisible(false);
    sensorTable->setShowGrid(false);
    sensorTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sensorTable->verticalHeader()->setDefaultSectionSize(15);
    sensorTable->horizontalHeader()->setFixedHeight(50);
    sensorTable->horizontalHeader()->setVisible(true);
    sensorsFrameLayout->addWidget(sensorTable);

    QLabel *feedbackHeader = new QLabel("Sensor Feedback");
    feedbackHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; margin: 20px 0 10px 0;");
    sensorsFrameLayout->addWidget(feedbackHeader);

    QHBoxLayout *feedbackContentLayout = new QHBoxLayout();
    feedbackContentLayout->setSpacing(20);
    feedbackContentLayout->setAlignment(Qt::AlignTop);

    sensorChart = new CustomGraphWidget(CustomGraphWidget::Bar);
    sensorChart->setFixedSize(200, 150);
    feedbackContentLayout->addWidget(sensorChart);

    QVBoxLayout *feedbackTextLayout = new QVBoxLayout();
    feedbackTextLayout->setSpacing(10);
    feedbackTextLayout->setAlignment(Qt::AlignTop);

    QString labelStyle =
        "color: white;"
        "background-color: #182038;"
        "border: 1px solid #3a4565;"
        "border-radius: 5px;"
        "padding: 4px 8px;"
        "margin-bottom: 5px;"
        "min-height: 28px;"
        "max-height: 28px;"
        "min-width: 250px;";

    radarFeedbackLabel = new QLabel("RADAR: Unknown");
    radarFeedbackLabel->setStyleSheet(labelStyle);
    radarFeedbackLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    iffFeedbackLabel = new QLabel("IFF: Unknown");
    iffFeedbackLabel->setStyleSheet(labelStyle);
    iffFeedbackLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    feedbackTextLayout->addWidget(radarFeedbackLabel);
    feedbackTextLayout->addWidget(iffFeedbackLabel);

    feedbackContentLayout->addLayout(feedbackTextLayout);
    sensorsFrameLayout->addLayout(feedbackContentLayout);
    sensorsFrameLayout->addStretch();

    mainLayout->addWidget(sensorsFrame);
    mainLayout->addStretch();

    contentStack->addWidget(sensorsWidget);
}
void Feedback::setupRadioWidget()
{
    radioWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(radioWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *radioFrame = new QFrame();
    radioFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *radioFrameLayout = new QVBoxLayout(radioFrame);

    QLabel *header = new QLabel("Radio Status");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    radioFrameLayout->addWidget(header);

    QVBoxLayout *radioTextLayout = new QVBoxLayout();
    radioTextLayout->setSpacing(5);
    radioTextLayout->setAlignment(Qt::AlignTop);

    QString labelStyle =
        "color: white;"
        "background-color: #182038;"
        "border: 1px solid #3a4565;"
        "border-radius: 5px;"
        "padding: 4px 8px;"
        "margin-bottom: 5px;"
        "min-height: 28px;"
        "max-height: 28px;"
        "min-width: 200px;";

    radioSystemLabel = new QLabel("Radio System: Unknown");
    radioSystemLabel->setStyleSheet(labelStyle);
    radioSystemLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    frequencyLabel = new QLabel("Frequency: Unknown");
    frequencyLabel->setStyleSheet(labelStyle);
    frequencyLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    signalStrengthLabel = new QLabel("Signal Strength: Unknown");
    signalStrengthLabel->setStyleSheet(labelStyle);
    signalStrengthLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    radioTextLayout->addWidget(radioSystemLabel);
    radioTextLayout->addWidget(frequencyLabel);
    radioTextLayout->addWidget(signalStrengthLabel);

    radioFrameLayout->addLayout(radioTextLayout);
    radioFrameLayout->addStretch();

    mainLayout->addWidget(radioFrame);
    mainLayout->addStretch();

    contentStack->addWidget(radioWidget);
}
void Feedback::setupNetworkWidget()
{
    networkWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(networkWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *networkFrame = new QFrame();
    networkFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *networkFrameLayout = new QVBoxLayout(networkFrame);

    QLabel *header = new QLabel("Network Status");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    networkFrameLayout->addWidget(header);

    QVBoxLayout *networkTextLayout = new QVBoxLayout();
    networkTextLayout->setSpacing(5);
    networkTextLayout->setAlignment(Qt::AlignTop);

    QString labelStyle =
        "color: white;"
        "background-color: #182038;"
        "border: 1px solid #3a4565;"
        "border-radius: 5px;"
        "padding: 4px 8px;"
        "margin-bottom: 5px;"
        "min-height: 28px;"
        "max-height: 28px;"
        "min-width: 200px;";

    connectivityLabel = new QLabel("Connectivity: Unknown");
    connectivityLabel->setStyleSheet(labelStyle);
    connectivityLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    bandwidthLabel = new QLabel("Bandwidth Usage: Unknown");
    bandwidthLabel->setStyleSheet(labelStyle);
    bandwidthLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    latencyLabel = new QLabel("Latency: Unknown");
    latencyLabel->setStyleSheet(labelStyle);
    latencyLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    networkTextLayout->addWidget(connectivityLabel);
    networkTextLayout->addWidget(bandwidthLabel);
    networkTextLayout->addWidget(latencyLabel);

    networkFrameLayout->addLayout(networkTextLayout);
    networkFrameLayout->addStretch();

    mainLayout->addWidget(networkFrame);
    mainLayout->addStretch();

    contentStack->addWidget(networkWidget);
}
void Feedback::setupLogsWidget()
{
    logsWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(logsWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QFrame *logsFrame = new QFrame();
    logsFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *logsFrameLayout = new QVBoxLayout(logsFrame);

    QLabel *header = new QLabel("RTC and Logs");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    logsFrameLayout->addWidget(header);

    logsTextEdit = new QTextEdit();
    logsTextEdit->setReadOnly(true);
    logsTextEdit->setMinimumHeight(100);
    logsTextEdit->setText("No logs available");
    logsFrameLayout->addWidget(logsTextEdit);
    logsFrameLayout->addStretch();

    mainLayout->addWidget(logsFrame);
    mainLayout->addStretch();

    contentStack->addWidget(logsWidget);
}
void Feedback::setupCanvasWidget()
{
    canvasWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(canvasWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15); // Fixed line

    QFrame *canvasFrame = new QFrame();
    canvasFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *canvasFrameLayout = new QVBoxLayout(canvasFrame);

    QLabel *header = new QLabel("CanvasWidget Interactions");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    canvasFrameLayout->addWidget(header);

    QVBoxLayout *interactionsTableLayout = new QVBoxLayout();
    interactionsTable = new QTableWidget();
    interactionsTable->setColumnCount(4);
    interactionsTable->setHorizontalHeaderLabels({"Time", "ID", "GeoCoords", "Fixed Point List"});
    interactionsTable->setRowCount(0); // Initialize with zero rows
    interactionsTable->setMinimumHeight(100);
    interactionsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    interactionsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    interactionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    interactionsTable->horizontalHeader()->setFixedHeight(50);
    interactionsTable->verticalHeader()->setVisible(false);
    interactionsTable->setShowGrid(false);
    interactionsTableLayout->addWidget(interactionsTable);

    canvasFrameLayout->addLayout(interactionsTableLayout);
    mainLayout->addWidget(canvasFrame);
    mainLayout->addStretch();

    contentStack->addWidget(canvasWidget);
}
void Feedback::setupEntityWidget()
{
    entityWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(entityWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    entityFrame = new QFrame();
    entityFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 3px; background-color: #1e2a44; padding: 8px;");
    QVBoxLayout *entityFrameLayout = new QVBoxLayout(entityFrame);
    entityFrameLayout->setContentsMargins(0, 0, 0, 0);
    entityFrameLayout->setSpacing(8);

    QLabel *entityListHeader = new QLabel("Entity List");
    QFont headerFont("Arial", 12, QFont::Bold);
    entityListHeader->setFont(headerFont);
    entityListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 5px;");
    entityFrameLayout->addWidget(entityListHeader);

    QHBoxLayout *entitySummaryLayout = new QHBoxLayout();
    entitySummaryLayout->setSpacing(8);
    entitySummaryLayout->setAlignment(Qt::AlignLeft);

    totalEntitiesLabel = new QLabel("Total: 0");
    totalEntitiesLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    entitySummaryLayout->addWidget(totalEntitiesLabel);

    activeEntitiesLabel = new QLabel("Active: 0");
    activeEntitiesLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    entitySummaryLayout->addWidget(activeEntitiesLabel);

    selectEntityCombo = new QComboBox();
    selectEntityCombo->setFixedWidth(150);
    selectEntityCombo->setStyleSheet(
        "background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px;"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 8px; height: 8px; }"
        );
    entitySummaryLayout->addWidget(selectEntityCombo);
    entitySummaryLayout->addStretch();

    entityFrameLayout->addLayout(entitySummaryLayout);

    detailedViewWidget = new QWidget();
    QVBoxLayout *detailedViewLayout = new QVBoxLayout(detailedViewWidget);
    detailedViewLayout->setContentsMargins(0, 0, 0, 0);
    detailedViewLayout->setSpacing(5);

    detailedViewTextEdit = new QTextEdit();
    detailedViewTextEdit->setReadOnly(true);
    detailedViewTextEdit->setStyleSheet(
        "background-color: #182038; color: white; border: 1px solid #3a4565; border-radius: 3px; padding: 4px; font-size: 12px;"
        );
    detailedViewTextEdit->setMinimumHeight(100);
    detailedViewTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    detailedViewTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    detailedViewTextEdit->setText("Select an Entity to view details");
    detailedViewLayout->addWidget(detailedViewTextEdit);

    detailedViewWidget->setVisible(true);
    entityFrameLayout->addWidget(detailedViewWidget);

    entityFrameLayout->addStretch();

    mainLayout->addWidget(entityFrame);
    mainLayout->addStretch();

    contentStack->addWidget(entityFrame);

    connect(selectEntityCombo, &QComboBox::currentTextChanged, this, &Feedback::onEntityComboChanged);
}
void Feedback::setupIffWidget()
{
    iffWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(iffWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    iffFrame = new QFrame();
    iffFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 3px; background-color: #1e2a44; padding: 8px;");
    QVBoxLayout *iffFrameLayout = new QVBoxLayout(iffFrame);
    iffFrameLayout->setContentsMargins(0, 0, 0, 0);
    iffFrameLayout->setSpacing(8);

    QLabel *iffListHeader = new QLabel("IFF List");
    QFont headerFont("Arial", 12, QFont::Bold);
    iffListHeader->setFont(headerFont);
    iffListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 5px;");
    iffFrameLayout->addWidget(iffListHeader);

    QHBoxLayout *iffSummaryLayout = new QHBoxLayout();
    iffSummaryLayout->setSpacing(8);
    iffSummaryLayout->setAlignment(Qt::AlignLeft);

    totalIffsLabel = new QLabel("Total: 0");
    totalIffsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    iffSummaryLayout->addWidget(totalIffsLabel);

    activeIffsLabel = new QLabel("Active: 0");
    activeIffsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    iffSummaryLayout->addWidget(activeIffsLabel);

    selectIffCombo = new QComboBox();
    selectIffCombo->setFixedWidth(150);
    selectIffCombo->setStyleSheet(
        "background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px;"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 8px; height: 8px; }"
        );
    iffSummaryLayout->addWidget(selectIffCombo);
    iffSummaryLayout->addStretch();

    iffFrameLayout->addLayout(iffSummaryLayout);

    iffDetailedViewWidget = new QWidget();
    QVBoxLayout *iffDetailedViewLayout = new QVBoxLayout(iffDetailedViewWidget);
    iffDetailedViewLayout->setContentsMargins(0, 0, 0, 0);
    iffDetailedViewLayout->setSpacing(5);

    iffDetailedViewTextEdit = new QTextEdit();
    iffDetailedViewTextEdit->setReadOnly(true);
    iffDetailedViewTextEdit->setStyleSheet(
        "background-color: #182038; color: white; border: 1px solid #3a4565; border-radius: 3px; padding: 4px; font-size: 12px;"
        );
    iffDetailedViewTextEdit->setMinimumHeight(100);
    iffDetailedViewTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    iffDetailedViewTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    iffDetailedViewTextEdit->setText("Select an IFF to view details");
    iffDetailedViewLayout->addWidget(iffDetailedViewTextEdit);

    iffDetailedViewWidget->setVisible(true);
    iffFrameLayout->addWidget(iffDetailedViewWidget);

    iffFrameLayout->addStretch();

    mainLayout->addWidget(iffFrame);
    mainLayout->addStretch();

    contentStack->addWidget(iffFrame);

    connect(selectIffCombo, &QComboBox::currentTextChanged, this, &Feedback::onIffComboChanged);
}
void Feedback::setupFormationWidget()
{
    formationWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(formationWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    formationFrame = new QFrame();
    formationFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 3px; background-color: #1e2a44; padding: 8px;");
    QVBoxLayout *formationFrameLayout = new QVBoxLayout(formationFrame);
    formationFrameLayout->setContentsMargins(0, 0, 0, 0);
    formationFrameLayout->setSpacing(8);

    QLabel *formationListHeader = new QLabel("Formation List");
    QFont headerFont("Arial", 12, QFont::Bold);
    formationListHeader->setFont(headerFont);
    formationListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 5px;");
    formationFrameLayout->addWidget(formationListHeader);

    QHBoxLayout *formationSummaryLayout = new QHBoxLayout();
    formationSummaryLayout->setSpacing(8);
    formationSummaryLayout->setAlignment(Qt::AlignLeft);

    totalFormationsLabel = new QLabel("Total: 0");
    totalFormationsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    formationSummaryLayout->addWidget(totalFormationsLabel);

    activeFormationsLabel = new QLabel("Active: 0");
    activeFormationsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    formationSummaryLayout->addWidget(activeFormationsLabel);

    selectFormationCombo = new QComboBox();
    selectFormationCombo->setFixedWidth(150);
    selectFormationCombo->setStyleSheet(
        "background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px;"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 8px; height: 8px; }"
        );
    formationSummaryLayout->addWidget(selectFormationCombo);
    formationSummaryLayout->addStretch();

    formationFrameLayout->addLayout(formationSummaryLayout);

    formationDetailedViewWidget = new QWidget();
    QVBoxLayout *formationDetailedViewLayout = new QVBoxLayout(formationDetailedViewWidget);
    formationDetailedViewLayout->setContentsMargins(0, 0, 0, 0);
    formationDetailedViewLayout->setSpacing(5);

    formationDetailedViewTextEdit = new QTextEdit();
    formationDetailedViewTextEdit->setReadOnly(true);
    formationDetailedViewTextEdit->setStyleSheet(
        "background-color: #182038; color: white; border: 1px solid #3a4565; border-radius: 3px; padding: 4px; font-size: 12px;"
        );
    formationDetailedViewTextEdit->setMinimumHeight(100);
    formationDetailedViewTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    formationDetailedViewTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    formationDetailedViewTextEdit->setText("Select a Formation to view details");
    formationDetailedViewLayout->addWidget(formationDetailedViewTextEdit);

    formationDetailedViewWidget->setVisible(true);
    formationFrameLayout->addWidget(formationDetailedViewWidget);

    formationFrameLayout->addStretch();

    mainLayout->addWidget(formationFrame);
    mainLayout->addStretch();

    contentStack->addWidget(formationFrame);

    connect(selectFormationCombo, &QComboBox::currentTextChanged, this, &Feedback::onFormationComboChanged);
}
void Feedback::setupFixedPointWidget()
{
    fixedPointWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(fixedPointWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    fixedPointFrame = new QFrame();
    fixedPointFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 3px; background-color: #1e2a44; padding: 8px;");
    QVBoxLayout *fixedPointFrameLayout = new QVBoxLayout(fixedPointFrame);
    fixedPointFrameLayout->setContentsMargins(0, 0, 0, 0);
    fixedPointFrameLayout->setSpacing(8);

    QLabel *fixedPointListHeader = new QLabel("FixedPoint List");
    QFont headerFont("Arial", 12, QFont::Bold);
    fixedPointListHeader->setFont(headerFont);
    fixedPointListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 5px;");
    fixedPointFrameLayout->addWidget(fixedPointListHeader);

    QHBoxLayout *fixedPointSummaryLayout = new QHBoxLayout();
    fixedPointSummaryLayout->setSpacing(8);
    fixedPointSummaryLayout->setAlignment(Qt::AlignLeft);

    totalFixedPointsLabel = new QLabel("Total: 0");
    totalFixedPointsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    fixedPointSummaryLayout->addWidget(totalFixedPointsLabel);

    activeFixedPointsLabel = new QLabel("Active: 0");
    activeFixedPointsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    fixedPointSummaryLayout->addWidget(activeFixedPointsLabel);

    selectFixedPointCombo = new QComboBox();
    selectFixedPointCombo->setFixedWidth(150);
    selectFixedPointCombo->setStyleSheet(
        "background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px;"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 8px; height: 8px; }"
        );
    fixedPointSummaryLayout->addWidget(selectFixedPointCombo);
    fixedPointSummaryLayout->addStretch();

    fixedPointFrameLayout->addLayout(fixedPointSummaryLayout);

    fixedPointDetailedViewWidget = new QWidget();
    QVBoxLayout *fixedPointDetailedViewLayout = new QVBoxLayout(fixedPointDetailedViewWidget);
    fixedPointDetailedViewLayout->setContentsMargins(0, 0, 0, 0);
    fixedPointDetailedViewLayout->setSpacing(5);

    fixedPointDetailedViewTextEdit = new QTextEdit();
    fixedPointDetailedViewTextEdit->setReadOnly(true);
    fixedPointDetailedViewTextEdit->setStyleSheet(
        "background-color: #182038; color: white; border: 1px solid #3a4565; border-radius: 3px; padding: 4px; font-size: 12px;"
        );
    fixedPointDetailedViewTextEdit->setMinimumHeight(100);
    fixedPointDetailedViewTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    fixedPointDetailedViewTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    fixedPointDetailedViewTextEdit->setText("Select a FixedPoint to view details");
    fixedPointDetailedViewLayout->addWidget(fixedPointDetailedViewTextEdit);

    fixedPointDetailedViewWidget->setVisible(true);
    fixedPointFrameLayout->addWidget(fixedPointDetailedViewWidget);

    fixedPointFrameLayout->addStretch();

    mainLayout->addWidget(fixedPointFrame);
    mainLayout->addStretch();

    contentStack->addWidget(fixedPointFrame);

    connect(selectFixedPointCombo, &QComboBox::currentTextChanged, this, &Feedback::onFixedPointComboChanged);
}
void Feedback::setupWeaponWidget()
{
    weaponWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(weaponWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    weaponFrame = new QFrame();
    weaponFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 3px; background-color: #1e2a44; padding: 8px;");
    QVBoxLayout *weaponFrameLayout = new QVBoxLayout(weaponFrame);
    weaponFrameLayout->setContentsMargins(0, 0, 0, 0);
    weaponFrameLayout->setSpacing(8);

    QLabel *weaponListHeader = new QLabel("Weapon List");
    QFont headerFont("Arial", 12, QFont::Bold);
    weaponListHeader->setFont(headerFont);
    weaponListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 5px;");
    weaponFrameLayout->addWidget(weaponListHeader);

    QHBoxLayout *weaponSummaryLayout = new QHBoxLayout();
    weaponSummaryLayout->setSpacing(8);
    weaponSummaryLayout->setAlignment(Qt::AlignLeft);

    totalWeaponsLabel = new QLabel("Total: 0");
    totalWeaponsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    weaponSummaryLayout->addWidget(totalWeaponsLabel);

    activeWeaponsLabel = new QLabel("Active: 0");
    activeWeaponsLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #ffffff;");
    weaponSummaryLayout->addWidget(activeWeaponsLabel);

    selectWeaponCombo = new QComboBox();
    selectWeaponCombo->setFixedWidth(150);
    selectWeaponCombo->setStyleSheet(
        "background-color: #2a3555; color: #ffffff; border: 1px solid #3a4565; padding: 2px; font-size: 12px;"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(:/images/down-arrow.png); width: 8px; height: 8px; }"
        );
    weaponSummaryLayout->addWidget(selectWeaponCombo);
    weaponSummaryLayout->addStretch();

    weaponFrameLayout->addLayout(weaponSummaryLayout);

    weaponDetailedViewWidget = new QWidget();
    QVBoxLayout *weaponDetailedViewLayout = new QVBoxLayout(weaponDetailedViewWidget);
    weaponDetailedViewLayout->setContentsMargins(0, 0, 0, 0);
    weaponDetailedViewLayout->setSpacing(5);

    weaponDetailedViewTextEdit = new QTextEdit();
    weaponDetailedViewTextEdit->setReadOnly(true);
    weaponDetailedViewTextEdit->setStyleSheet(
        "background-color: #182038; color: white; border: 1px solid #3a4565; border-radius: 3px; padding: 4px; font-size: 12px;"
        );
    weaponDetailedViewTextEdit->setMinimumHeight(100);
    weaponDetailedViewTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    weaponDetailedViewTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    weaponDetailedViewTextEdit->setText("Select a Weapon to view details");
    weaponDetailedViewLayout->addWidget(weaponDetailedViewTextEdit);

    weaponDetailedViewWidget->setVisible(true);
    weaponFrameLayout->addWidget(weaponDetailedViewWidget);

    weaponFrameLayout->addStretch();

    mainLayout->addWidget(weaponFrame);
    mainLayout->addStretch();

    contentStack->addWidget(weaponFrame);

    connect(selectWeaponCombo, &QComboBox::currentTextChanged, this, &Feedback::onWeaponComboChanged);
}

void Feedback::onSidebarItemClicked(QListWidgetItem *item)
{
    QString itemText = item->text();
    if (itemText == "Overview") {
        contentStack->setCurrentWidget(overviewWidget);
    }
    else if (itemText == "Storage") {
        contentStack->setCurrentWidget(storageWidget);
    }
    else if (itemText == "Sensors") {
        contentStack->setCurrentWidget(sensorsWidget);
    }
    else if (itemText == "Radio") {
        contentStack->setCurrentWidget(radioWidget);
    }
    else if (itemText == "Network") {
        contentStack->setCurrentWidget(networkWidget);
    }
    else if (itemText == "Logs") {
        contentStack->setCurrentWidget(logsWidget);
    }
    else if (itemText == "CanvasWidget") {
        contentStack->setCurrentWidget(canvasWidget);
    }
    else if (itemText == "Entity") {
        contentStack->setCurrentWidget(entityFrame);
    }
    else if (itemText == "IFF") {
        contentStack->setCurrentWidget(iffFrame);
    }
    else if (itemText == "Formation") {
        contentStack->setCurrentWidget(formationFrame);
    }
    else if (itemText == "FixedPoint") {
        contentStack->setCurrentWidget(fixedPointFrame);
    }
    else if (itemText == "Weapon") {
        contentStack->setCurrentWidget(weaponFrame);
    }
}

void Feedback::onDetailsButtonClicked()
{
    detailedViewWidget->setVisible(false);
    detailsButton->setEnabled(false);
}
void Feedback::onEntityComboChanged(const QString &text)
{
    Q_UNUSED(text);
    //Hierarchy* h = Hierarchy::getCurrentContext();
    if (!h) {
        detailedViewTextEdit->setText("Hierarchy not available");
        return;
    }

    QString id = selectEntityCombo->currentData().toString();
    if (id.isEmpty()) {
        detailedViewTextEdit->setText("Select an Entity to view details");
        return;
    }

    auto it = h->Entities->find(id.toStdString());
    if (it == h->Entities->end()) {
        detailedViewTextEdit->setText("Entity not found");
        return;
    }

    Entity* e = it->second;
    QJsonObject details = e->toJson();
    QJsonObject sensorDetections = details.contains("sensor_detections") ? details["sensor_detections"].toObject() : QJsonObject();
    QJsonArray logsArray = details.contains("logs") ? details["logs"].toArray() : QJsonArray();
    QString logsText;
    for (const auto& log : logsArray) {
        QJsonObject logObj = log.toObject();
        QString type = logObj.contains("type") ? logObj["type"].toString() : "Unknown";
        QString message = logObj.contains("message") ? logObj["message"].toString() : "Unknown";
        logsText += QString("- [%1] %2\n").arg(type, message);
    }
    if (logsText.isEmpty()) logsText = "None";

    QString detailsText = QString(
                              "- Type: %1\n"
                              "- GeoCoords: %2\n"
                              "- Status: %3\n"
                              "- Tracking Accuracy: %4\n"
                              "- Accumulated Interactions: %5\n"
                              "- Mission: %6\n"
                              "- Formation: %7\n"
                              "- Sensor Detections:\n"
                              "  - Radar: %8\n"
                              "  - IFF: %9\n"
                              "- Logs:\n%10"
                              )
                              .arg(details.contains("type") ? details["type"].toString() : "Unknown")
                              .arg(details.contains("geoCoords") ? details["geoCoords"].toString() : "Unknown")
                              .arg(details.contains("status") ? details["status"].toString() : "Unknown")
                              .arg(details.contains("tracking_accuracy") ? details["tracking_accuracy"].toString() : "Unknown")
                              .arg(details.contains("accumulated_interactions") ? QString::number(details["accumulated_interactions"].toInt()) : "Unknown")
                              .arg(details.contains("mission") ? details["mission"].toString() : "Unknown")
                              .arg(details.contains("formation") ? details["formation"].toString() : "Unknown")
                              .arg(sensorDetections.contains("radar") ? sensorDetections["radar"].toString() : "Unknown")
                              .arg(sensorDetections.contains("iff") ? sensorDetections["iff"].toString() : "Unknown")
                              .arg(logsText);

    detailedViewTextEdit->setText(detailsText);
}
void Feedback::onIffComboChanged(const QString &text)
{
    Q_UNUSED(text);
    //Hierarchy* h = Hierarchy::getCurrentContext();
    if (!h) {
        iffDetailedViewTextEdit->setText("Hierarchy not available");
        return;
    }

    QString id = selectIffCombo->currentData().toString();
    if (id.isEmpty()) {
        iffDetailedViewTextEdit->setText("Select an IFF to view details");
        return;
    }

    auto it = h->Entities->find(id.toStdString());
    if (it == h->Entities->end()) {
        iffDetailedViewTextEdit->setText("IFF not found");
        return;
    }

    Entity* e = it->second;
    QJsonObject details = e->toJson();
    QJsonObject modeConfig = details.contains("modeConfiguration") ? details["modeConfiguration"].toObject() : QJsonObject();

    QString detailsText = QString(
                              "- Transponder: %1\n"
                              "- Emitting Range: %2\n"
                              "- Emitting Frequency: %3\n"
                              "- DIS Type: %4\n"
                              "- DIS Name: %5\n"
                              "- Operational Mode: %6\n"
                              "- Mode Configuration:\n"
                              "  - Mode1: %7\n"
                              "  - Mode2: %8\n"
                              "  - Mode3A: %9\n"
                              "  - Mode4: %10\n"
                              "  - ModeC: %11\n"
                              "- Code System: %12\n"
                              "- Encryption Type: %13\n"
                              "- Spoofable: %14\n"
                              "- Response Delay: %15\n"
                              "- Last Interrogation Time: %16"
                              )
                              .arg(details.contains("transponder") ? (details["transponder"].isBool() && details["transponder"].toBool() ? "True" : "False") : "Unknown")
                              .arg(details.contains("emittingRange") ? QString::number(details["emittingRange"].toDouble()) : "Unknown")
                              .arg(details.contains("emittingFrequency") ? QString::number(details["emittingFrequency"].toDouble()) : "Unknown")
                              .arg(details.contains("disType") ? (details["disType"].toString().isEmpty() ? "Unknown" : details["disType"].toString()) : "Unknown")
                              .arg(details.contains("disName") ? (details["disName"].toString().isEmpty() ? "Unknown" : details["disName"].toString()) : "Unknown")
                              .arg(details.contains("operationalMode") ? (details["operationalMode"].toString().isEmpty() ? "Unknown" : details["operationalMode"].toString()) : "Unknown")
                              .arg(modeConfig.contains("mode1") ? (modeConfig["mode1"].toString().isEmpty() ? "Unknown" : modeConfig["mode1"].toString()) : "Unknown")
                              .arg(modeConfig.contains("mode2") ? (modeConfig["mode2"].toString().isEmpty() ? "Unknown" : modeConfig["mode2"].toString()) : "Unknown")
                              .arg(modeConfig.contains("mode3A") ? (modeConfig["mode3A"].toString().isEmpty() ? "Unknown" : modeConfig["mode3A"].toString()) : "Unknown")
                              .arg(modeConfig.contains("mode4") ? (modeConfig["mode4"].toString().isEmpty() ? "Unknown" : modeConfig["mode4"].toString()) : "Unknown")
                              .arg(modeConfig.contains("modeC") ? (modeConfig["modeC"].toString().isEmpty() ? "Unknown" : modeConfig["modeC"].toString()) : "Unknown")
                              .arg(details.contains("codeSystem") ? (details["codeSystem"].toString().isEmpty() ? "Unknown" : details["codeSystem"].toString()) : "Unknown")
                              .arg(details.contains("encryptionType") ? (details["encryptionType"].toString().isEmpty() ? "Unknown" : details["encryptionType"].toString()) : "Unknown")
                              .arg(details.contains("spoofable") ? (details["spoofable"].isBool() && details["spoofable"].toBool() ? "True" : "False") : "Unknown")
                              .arg(details.contains("responseDelay") ? QString::number(details["responseDelay"].toDouble()) : "Unknown")
                              .arg(details.contains("lastInterrogationTime") ? (details["lastInterrogationTime"].toString().isEmpty() ? "Unknown" : details["lastInterrogationTime"].toString()) : "Unknown");

    iffDetailedViewTextEdit->setText(detailsText);
}

void Feedback::onFormationComboChanged(const QString &text)
{
    // Similar to IFF, but since formation not in code, set default
    formationDetailedViewTextEdit->setText("Formation details not available");
}

void Feedback::onFixedPointComboChanged(const QString &text)
{
    Q_UNUSED(text);
    //Hierarchy* h = Hierarchy::getCurrentContext();
    if (!h) {
        fixedPointDetailedViewTextEdit->setText("Hierarchy not available");
        return;
    }

    QString id = selectFixedPointCombo->currentData().toString();
    if (id.isEmpty()) {
        fixedPointDetailedViewTextEdit->setText("Select a FixedPoint to view details");
        return;
    }

    auto it = h->Entities->find(id.toStdString());
    if (it == h->Entities->end()) {
        fixedPointDetailedViewTextEdit->setText("FixedPoint not found");
        return;
    }

    Entity* e = it->second;
    QJsonObject details = e->toJson();
    QJsonObject transform = details.contains("transform") ? details["transform"].toObject() : QJsonObject();
    QJsonObject position = transform.contains("position") ? transform["position"].toObject() : QJsonObject();
    QJsonObject rotation = transform.contains("rotation") ? transform["rotation"].toObject() : QJsonObject();
    QJsonObject scale = transform.contains("scale") ? transform["scale"].toObject() : QJsonObject();
    QJsonObject collider = details.contains("collider") ? details["collider"].toObject() : QJsonObject();
    QJsonObject colliderSize = collider.contains("size") ? collider["size"].toObject() : QJsonObject();
    QJsonObject colliderOffset = collider.contains("offset") ? collider["offset"].toObject() : QJsonObject();
    QJsonObject meshRenderer = details.contains("meshRenderer2d") ? details["meshRenderer2d"].toObject() : QJsonObject();

    QString detailsText = QString(
                              "- Name: %1\n"
                              "- ID: %2\n"
                              "- Parent ID: %3\n"
                              "- Active: %4\n"
                              "- Type: %5\n"
                              "- Transform:\n"
                              "  - Position: (%6, %7, %8)\n"
                              "  - Rotation: (%9, %10, %11)\n"
                              "  - Scale: (%12, %13, %14)\n"
                              "- Collider:\n"
                              "  - Size: (Width: %15, Height: %16)\n"
                              "  - Offset: (X: %17, Y: %18)\n"
                              "- MeshRenderer2D:\n"
                              "  - Sprite: %19\n"
                              "  - Visible: %20"
                              )
                              .arg(details.contains("name") ? details["name"].toString() : "Unknown")
                              .arg(details.contains("id") ? details["id"].toString() : "Unknown")
                              .arg(details.contains("parent_id") ? details["parent_id"].toString() : "Unknown")
                              .arg(details.contains("active") ? (details["active"].toBool() ? "True" : "False") : "Unknown")
                              .arg(details.contains("type") ? details["type"].toString() : "Unknown")
                              .arg(position.contains("x") ? QString::number(position["x"].toDouble()) : "Unknown")
                              .arg(position.contains("y") ? QString::number(position["y"].toDouble()) : "Unknown")
                              .arg(position.contains("z") ? QString::number(position["z"].toDouble()) : "Unknown")
                              .arg(rotation.contains("x") ? QString::number(rotation["x"].toDouble()) : "Unknown")
                              .arg(rotation.contains("y") ? QString::number(rotation["y"].toDouble()) : "Unknown")
                              .arg(rotation.contains("z") ? QString::number(rotation["z"].toDouble()) : "Unknown")
                              .arg(scale.contains("x") ? QString::number(scale["x"].toDouble()) : "Unknown")
                              .arg(scale.contains("y") ? QString::number(scale["y"].toDouble()) : "Unknown")
                              .arg(scale.contains("z") ? QString::number(scale["z"].toDouble()) : "Unknown")
                              .arg(colliderSize.contains("width") ? QString::number(colliderSize["width"].toDouble()) : "Unknown")
                              .arg(colliderSize.contains("height") ? QString::number(colliderSize["height"].toDouble()) : "Unknown")
                              .arg(colliderOffset.contains("x") ? QString::number(colliderOffset["x"].toDouble()) : "Unknown")
                              .arg(colliderOffset.contains("y") ? QString::number(colliderOffset["y"].toDouble()) : "Unknown")
                              .arg(meshRenderer.contains("sprite") ? meshRenderer["sprite"].toString() : "Unknown")
                              .arg(meshRenderer.contains("visible") ? (meshRenderer["visible"].toBool() ? "True" : "False") : "Unknown");

    fixedPointDetailedViewTextEdit->setText(detailsText);
}

void Feedback::onWeaponComboChanged(const QString &text)
{
    Q_UNUSED(text);
    //Hierarchy* h = Hierarchy::getCurrentContext();
    if (!h) {
        weaponDetailedViewTextEdit->setText("Hierarchy not available");
        return;
    }

    QString id = selectWeaponCombo->currentData().toString();
    if (id.isEmpty()) {
        weaponDetailedViewTextEdit->setText("Select a Weapon to view details");
        return;
    }

    auto it = h->Entities->find(id.toStdString());
    if (it == h->Entities->end()) {
        weaponDetailedViewTextEdit->setText("Weapon not found");
        return;
    }

    Entity* e = it->second;
    QJsonObject details = e->toJson();
    QString weaponType = details.contains("weaponType") ? details["weaponType"].toString() : "Unknown";
    QJsonObject params = details.contains("parameters") ? details["parameters"].toObject() : QJsonObject();
    QString paramsText;
    for (auto it = params.begin(); it != params.end(); ++it) {
        QJsonObject paramObj = it.value().toObject();
        QString value = paramObj.contains("value") ? QString::number(paramObj["value"].toDouble()) : "Unknown";
        QString type = paramObj.contains("type") ? paramObj["type"].toString() : "Unknown";
        paramsText += QString("%1: %2 (%3)\n").arg(it.key(), value, type);
    }
    if (paramsText.isEmpty()) paramsText = "None";

    QString detailsText = QString(
                              "- Name: %1\n"
                              "- ID: %2\n"
                              "- Parent ID: %3\n"
                              "- Active: %4\n"
                              "- Weapon Type: %5\n"
                              "- Parameters:\n%6"
                              )
                              .arg(details.contains("name") ? details["name"].toString() : "Unknown")
                              .arg(details.contains("id") ? details["id"].toString() : "Unknown")
                              .arg(details.contains("parent_id") ? details["parent_id"].toString() : "Unknown")
                              .arg(details.contains("active") ? (details["active"].toBool() ? "True" : "False") : "Unknown")
                              .arg(weaponType)
                              .arg(paramsText);

    if (weaponType == "Missile") {
        QJsonObject loc = details.contains("locationOffset") ? details["locationOffset"].toObject() : QJsonObject();
        detailsText += QString(
                           "- Location Offset: (X: %1, Y: %2, Z: %3)\n"
                           "- Thrust: %4 N\n"
                           "- Burn Time: %5 s\n"
                           "- Max Speed: %6 m/s\n"
                           "- Acceleration: %7 m/s²\n"
                           "- Payload: %8 kg\n"
                           "- Proximity Range: %9 m\n"
                           "- Range: %10 km\n"
                           "- Guiding Sensor: %11\n"
                           "- Seeker Lock-On Time: %12 s\n"
                           "- ECCM Capability: %13\n"
                           "- Launch Platform: %14\n"
                           "- Control System: %15"
                           )
                           .arg(loc.contains("x") ? QString::number(loc["x"].toDouble()) : "Unknown")
                           .arg(loc.contains("y") ? QString::number(loc["y"].toDouble()) : "Unknown")
                           .arg(loc.contains("z") ? QString::number(loc["z"].toDouble()) : "Unknown")
                           .arg(details.contains("thrust") ? QString::number(details["thrust"].toDouble()) : "Unknown")
                           .arg(details.contains("burnTime") ? QString::number(details["burnTime"].toDouble()) : "Unknown")
                           .arg(details.contains("maxSpeed") ? QString::number(details["maxSpeed"].toDouble()) : "Unknown")
                           .arg(details.contains("acceleration") ? QString::number(details["acceleration"].toDouble()) : "Unknown")
                           .arg(details.contains("payload") ? QString::number(details["payload"].toDouble()) : "Unknown")
                           .arg(details.contains("proximityRange") ? QString::number(details["proximityRange"].toDouble()) : "Unknown")
                           .arg(details.contains("range") ? QString::number(details["range"].toDouble() / 1000.0) : "Unknown")
                           .arg(details.contains("guidingSensor") ? details["guidingSensor"].toString() : "Unknown")
                           .arg(details.contains("seekerLockOnTime") ? QString::number(details["seekerLockOnTime"].toDouble()) : "Unknown")
                           .arg(details.contains("eccmCapability") ? (details["eccmCapability"].toBool() ? "True" : "False") : "Unknown")
                           .arg(details.contains("launchPlatformType") ? details["launchPlatformType"].toString() : "Unknown")
                           .arg(details.contains("controlSystem") ? details["controlSystem"].toString() : "Unknown");
    } else if (weaponType == "Gun") {
        detailsText += QString(
                           "- Muzzle Speed: %1 m/s\n"
                           "- Maximum Rate: %2 rpm\n"
                           "- Barrel Length: %3 m\n"
                           "- Caliber: %4 mm\n"
                           "- Magazine Capacity: %5\n"
                           "- Effective Range: %6 m\n"
                           "- Burst Mode: %7\n"
                           "- Reload Time: %8 s\n"
                           "- Recoil Force: %9 N"
                           )
                           .arg(details.contains("muzzleSpeed") ? QString::number(details["muzzleSpeed"].toDouble()) : "Unknown")
                           .arg(details.contains("maximumRate") ? QString::number(details["maximumRate"].toDouble()) : "Unknown")
                           .arg(details.contains("barrelLength") ? QString::number(details["barrelLength"].toDouble()) : "Unknown")
                           .arg(details.contains("caliber") ? QString::number(details["caliber"].toDouble()) : "Unknown")
                           .arg(details.contains("magazineCapacity") ? QString::number(details["magazineCapacity"].toInt()) : "Unknown")
                           .arg(details.contains("effectiveRange") ? QString::number(details["effectiveRange"].toDouble()) : "Unknown")
                           .arg(details.contains("burstMode") ? (details["burstMode"].toBool() ? "True" : "False") : "Unknown")
                           .arg(details.contains("reloadTime") ? QString::number(details["reloadTime"].toDouble()) : "Unknown")
                           .arg(details.contains("recoilForce") ? QString::number(details["recoilForce"].toDouble()) : "Unknown");
    } else if (weaponType == "Bomb") {
        QJsonObject loc = details.contains("locationOffset") ? details["locationOffset"].toObject() : QJsonObject();
        detailsText += QString(
                           "- Location Offset: (X: %1, Y: %2, Z: %3)\n"
                           "- Payload: %4 kg\n"
                           "- Proximity Range: %5 m\n"
                           "- Guiding Sensor: %6\n"
                           "- Fall Time: %7 s\n"
                           "- Arming Delay: %8 s\n"
                           "- Fuse Type: %9\n"
                           "- Glide Capability: %10"
                           )
                           .arg(loc.contains("x") ? QString::number(loc["x"].toDouble()) : "Unknown")
                           .arg(loc.contains("y") ? QString::number(loc["y"].toDouble()) : "Unknown")
                           .arg(loc.contains("z") ? QString::number(loc["z"].toDouble()) : "Unknown")
                           .arg(details.contains("payload") ? QString::number(details["payload"].toDouble()) : "Unknown")
                           .arg(details.contains("proximityRange") ? QString::number(details["proximityRange"].toDouble()) : "Unknown")
                           .arg(details.contains("guidingSensor") ? details["guidingSensor"].toString() : "Unknown")
                           .arg(details.contains("fallTime") ? QString::number(details["fallTime"].toDouble()) : "Unknown")
                           .arg(details.contains("armingDelay") ? QString::number(details["armingDelay"].toDouble()) : "Unknown")
                           .arg(details.contains("fuseType") ? details["fuseType"].toString() : "Unknown")
                           .arg(details.contains("glideCapability") ? (details["glideCapability"].toBool() ? "True" : "False") : "Unknown");
    }

    weaponDetailedViewTextEdit->setText(detailsText);
}


// void Feedback::loadDashboardData(const QString& jsonData)
// {

//     qDebug() << "Starting loadDashboardData with JSON:" << jsonData;

//     //Hierarchy* h = hier;//Hierarchy::getCurrentContext();
//     qDebug() << "Hierarchy context:" << (h ? "Valid" : "Null");

//     // Parse external JSON data if provided
//     QJsonObject externalData;
//     if (!jsonData.isEmpty()) {
//         QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
//         if (doc.isObject()) {
//             externalData = doc.object();
//             qDebug() << "Parsed external JSON:" << QJsonDocument(externalData).toJson(QJsonDocument::Compact);
//         } else {
//             qDebug() << "Invalid JSON data provided";
//         }
//     }


//     // Update Overview tab
//     QString systemStatus = externalData.contains("systemStatus") ? externalData["systemStatus"].toString() : "System: ONLINE  Sim: RUNNING  RTC: 2025-10-15";
//     systemStatusLabel->setText(systemStatus);
//     qDebug() << "System Status set to:" << systemStatus;

//     QString uptime = externalData.contains("uptime") ? externalData["uptime"].toString() : "Uptime: Unknown";
//     uptimeLabel->setText(uptime);
//     qDebug() << "Uptime set to:" << uptime;

//     int mainEntities = 0;
//     if (h) {
//         qDebug() << "Processing entities... Total entities in Hierarchy:" << h->Entities->size();
//         for (const auto& [id, e] : *h->Entities) {
//             // if (h->ProfileCategories.find(e->parentID) == h->ProfileCategories.end()) {

//                 mainEntities++;
//                 QJsonObject entityJson = e->toJson();
//                 qDebug() << "Entity ID:" << QString::fromStdString(id)
//                          << "Name:" << QString::fromStdString(e->Name)
//                          << "ParentID:" << QString::fromStdString(e->parentID)
//                          << "JSON:" << QJsonDocument(entityJson).toJson(QJsonDocument::Compact);
//             }
//         }
//     // }
//     entitiesLabel->setText("Entities: " + QString::number(mainEntities));
//     qDebug() << "Entities count set to:" << mainEntities;

//     QString feedbackEvents = externalData.contains("feedbackEvents") ? externalData["feedbackEvents"].toString() : "Accumulated Feedback Events: Unknown";
//     feedbackEventsLabel->setText(feedbackEvents);
//     qDebug() << "Feedback Events set to:" << feedbackEvents;

//     int cpuUsage = externalData.contains("cpuUsage") ? externalData["cpuUsage"].toInt() : 0;
//     cpuProgressBar->setValue(cpuUsage);
//     qDebug() << "CPU Progress set to:" << cpuUsage;

//     // Update Storage tab
//     QString mongoDb = externalData.contains("mongoDb") ? externalData["mongoDb"].toString() : "MongoDB: Unknown";
//     mongoDbLabel->setText(mongoDb);
//     QString logs = externalData.contains("logs") ? externalData["logs"].toString() : "Logs: Unknown";
//     logsLabel->setText(logs);
//     QString scenarios = externalData.contains("scenarios") ? externalData["scenarios"].toString() : "Scenarios: Unknown";
//     scenariosLabel->setText(scenarios);
//     QString totalStorage = externalData.contains("totalStorage") ? externalData["totalStorage"].toString() : "Total: Unknown";
//     totalLabel->setText(totalStorage);
//     qDebug() << "Storage tab: MongoDB =" << mongoDb << ", Logs =" << logs << ", Scenarios =" << scenarios << ", Total =" << totalStorage;

//     // Update Sensors tab
//     sensorTable->setRowCount(0);
//     QString radarFeedback = "RADAR: Unknown";
//     QString iffFeedback = "IFF: Unknown";
//     if (h) {
//         for (const auto& [id, e] : *h->Entities) {
//             QJsonObject entityJson = e->toJson();
//             if (entityJson.contains("sensors")) {
//                 QJsonArray sensors = entityJson["sensors"].toArray();
//                 sensorTable->setRowCount(sensors.size());
//                 for (int i = 0; i < sensors.size(); ++i) {
//                     QJsonObject sensor = sensors[i].toObject();
//                     QString sensorName = sensor.contains("name") ? sensor["name"].toString() : "Unknown";
//                     QString sensorStatus = sensor.contains("status") ? sensor["status"].toString() : "Unknown";
//                     sensorTable->setItem(i, 0, new QTableWidgetItem(sensorName));
//                     sensorTable->setItem(i, 1, new QTableWidgetItem(sensorStatus));
//                     qDebug() << "Sensor added to table: Name =" << sensorName << ", Status =" << sensorStatus;
//                     if (sensor.contains("type") && sensor["type"].toString() == "RADAR") {
//                         radarFeedback = "RADAR: " + sensorStatus;
//                     }
//                 }
//             }
//         }
//     }
//     radarFeedbackLabel->setText(radarFeedback);
//     iffFeedbackLabel->setText(iffFeedback);
//     qDebug() << "Sensors tab: RADAR =" << radarFeedback << ", IFF =" << iffFeedback;

//     // Update Radio tab
//     QString radioSystem = externalData.contains("radioSystem") ? externalData["radioSystem"].toString() : "Radio System: Unknown";
//     radioSystemLabel->setText(radioSystem);
//     QString frequency = externalData.contains("frequency") ? externalData["frequency"].toString() : "Frequency: Unknown";
//     frequencyLabel->setText(frequency);
//     QString signalStrength = externalData.contains("signalStrength") ? externalData["signalStrength"].toString() : "Signal Strength: Unknown";
//     signalStrengthLabel->setText(signalStrength);
//     qDebug() << "Radio tab: System =" << radioSystem << ", Frequency =" << frequency << ", Signal Strength =" << signalStrength;

//     // Update Network tab
//     QString connectivity = externalData.contains("connectivity") ? externalData["connectivity"].toString() : "Connectivity: Unknown";
//     connectivityLabel->setText(connectivity);
//     QString bandwidth = externalData.contains("bandwidth") ? externalData["bandwidth"].toString() : "Bandwidth Usage: Unknown";
//     bandwidthLabel->setText(bandwidth);
//     QString latency = externalData.contains("latency") ? externalData["latency"].toString() : "Latency: Unknown";
//     latencyLabel->setText(latency);
//     qDebug() << "Network tab: Connectivity =" << connectivity << ", Bandwidth =" << bandwidth << ", Latency =" << latency;

//     // Update Logs tab
//     QString logsText = externalData.contains("logs") ? externalData["logs"].toString() : "No logs available";
//     logsTextEdit->setText(logsText);
//     qDebug() << "Logs tab set to:" << logsText;

//     // Update CanvasWidget tab
//     interactionsTable->setRowCount(0);
//     if (externalData.contains("interactions")) {
//         QJsonArray interactions = externalData["interactions"].toArray();
//         interactionsTable->setRowCount(interactions.size());
//         for (int i = 0; i < interactions.size(); ++i) {
//             QJsonObject interaction = interactions[i].toObject();
//             QString source = interaction.contains("source") ? interaction["source"].toString() : "Unknown";
//             QString target = interaction.contains("target") ? interaction["target"].toString() : "Unknown";
//             interactionsTable->setItem(i, 0, new QTableWidgetItem(source));
//             interactionsTable->setItem(i, 1, new QTableWidgetItem(target));
//             qDebug() << "Interaction added to table: Source =" << source << ", Target =" << target;
//         }
//     }
//     qDebug() << "CanvasWidget tab: Interactions table updated";

//     // Populate Entity Combo
//     selectEntityCombo->clear();
//     int totalEntities = 0;
//     int activeEntities = 0;
//     if (h) {
//         qDebug() << "Populating Entity Combo...";
//         for (const auto& [id, e] : *h->Entities) {
//             // if (h->ProfileCategories.find(e->parentID) != h->ProfileCategories.end()) {
//             //     qDebug() << "Skipping entity ID:" << QString::fromStdString(id) << "due to profile category";
//             //     continue;
//             // }
//             QJsonObject entityJson = e->toJson();
//             totalEntities++;
//             bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
//             if (isActive) activeEntities++;
//             QString entityName = QString::fromStdString(e->Name);
//             QString entityId = QString::fromStdString(id);
//             selectEntityCombo->addItem(entityName, entityId);
//             qDebug() << "Added Entity to Combo: Name =" << entityName << ", ID =" << entityId << ", Active =" << isActive;
//         }
//     }
//     totalEntitiesLabel->setText("Total: " + QString::number(totalEntities));
//     activeEntitiesLabel->setText("Active: " + QString::number(activeEntities));
//     qDebug() << "Entity tab: Total =" << totalEntities << ", Active =" << activeEntities;

//     // Populate IFF Combo
//     selectIffCombo->clear();
//     int totalIffs = 0;
//     int activeIffs = 0;
//     if (h) {
//         qDebug() << "Populating IFF Combo...";
//         for (const auto& [mainId, mainE] : *h->Entities) {
//             if (h->ProfileCategories.find(mainE->parentID) != h->ProfileCategories.end()) {
//                 qDebug() << "Skipping entity ID:" << QString::fromStdString(mainId) << "due to profile category";
//                 continue;
//             }
//             QJsonObject entityJson = mainE->toJson();
//             if (entityJson.contains("iffList")) {
//                 QJsonArray iffArray = entityJson["iffList"].toArray();
//                 for (const auto& iffValue : iffArray) {
//                     QJsonObject iffJson = iffValue.toObject();
//                     totalIffs++;
//                     QString mode = iffJson.contains("operationalMode") ? iffJson["operationalMode"].toString() : "";
//                     if (mode == "Active") activeIffs++;
//                     QString iffName = iffJson.contains("Name") ? iffJson["Name"].toString() : "Unknown";
//                     QString iffId = iffJson.contains("ID") ? iffJson["ID"].toString() : "";
//                     QString name = iffName + " - " + QString::fromStdString(mainE->Name);
//                     selectIffCombo->addItem(name, iffId);
//                     qDebug() << "Added IFF to Combo: Name =" << name << ", ID =" << iffId << ", Mode =" << mode;
//                     if (iffJson.contains("status")) {
//                         iffFeedback = "IFF: " + iffJson["status"].toString();
//                     }
//                 }
//             } else {
//                 qDebug() << "No iffList found for entity ID:" << QString::fromStdString(mainId);
//             }
//         }
//     }
//     totalIffsLabel->setText("Total: " + QString::number(totalIffs));
//     activeIffsLabel->setText("Active: " + QString::number(activeIffs));
//     qDebug() << "IFF tab: Total =" << totalIffs << ", Active =" << activeIffs;

//     // Populate Formation Combo (placeholder)
//     selectFormationCombo->clear();
//     totalFormationsLabel->setText("Total: 0");
//     activeFormationsLabel->setText("Active: 0");
//     qDebug() << "Formation tab: Total = 0, Active = 0 (placeholder)";

//     // Populate FixedPoint Combo
//     selectFixedPointCombo->clear();
//     int totalFixedPoints = 0;
//     int activeFixedPoints = 0;
//     if (h) {
//         qDebug() << "Populating FixedPoint Combo...";
//         for (const auto& [id, e] : *h->Entities) {
//             QJsonObject entityJson = e->toJson();
//             if (entityJson.contains("type") && entityJson["type"].toString() == "FixedPoint") {
//                 totalFixedPoints++;
//                 bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
//                 if (isActive) activeFixedPoints++;
//                 QString entityName = QString::fromStdString(e->Name);
//                 QString entityId = QString::fromStdString(id);
//                 selectFixedPointCombo->addItem(entityName, entityId);
//                 qDebug() << "Added FixedPoint to Combo: Name =" << entityName << ", ID =" << entityId << ", Active =" << isActive;
//             }
//         }
//     }
//     totalFixedPointsLabel->setText("Total: " + QString::number(totalFixedPoints));
//     activeFixedPointsLabel->setText("Active: " + QString::number(activeFixedPoints));
//     qDebug() << "FixedPoint tab: Total =" << totalFixedPoints << ", Active =" << activeFixedPoints;

//     // Populate Weapon Combo
//     selectWeaponCombo->clear();
//     int totalWeapons = 0;
//     int activeWeapons = 0;
//     if (h) {
//         qDebug() << "Populating Weapon Combo...";
//         for (const auto& [id, e] : *h->Entities) {
//             QJsonObject entityJson = e->toJson();
//             if (entityJson.contains("weaponType")) {
//                 totalWeapons++;
//                 bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
//                 if (isActive) activeWeapons++;
//                 QString entityName = QString::fromStdString(e->Name);
//                 QString entityId = QString::fromStdString(id);
//                 selectWeaponCombo->addItem(entityName, entityId);
//                 qDebug() << "Added Weapon to Combo: Name =" << entityName << ", ID =" << entityId << ", Active =" << isActive;
//             }
//         }
//     }
//     totalWeaponsLabel->setText("Total: " + QString::number(totalWeapons));
//     activeWeaponsLabel->setText("Active: " + QString::number(activeWeapons));
//     qDebug() << "Weapon tab: Total =" << totalWeapons << ", Active =" << activeWeapons;

//     // Trigger updates if selected
//     if (selectEntityCombo->count() > 0) {
//         QString currentEntity = selectEntityCombo->currentText();
//         qDebug() << "Triggering Entity Combo update for:" << currentEntity;
//         onEntityComboChanged(currentEntity);
//     }
//     if (selectIffCombo->count() > 0) {
//         QString currentIff = selectIffCombo->currentText();
//         qDebug() << "Triggering IFF Combo update for:" << currentIff;
//         onIffComboChanged(currentIff);
//     }
//     if (selectFormationCombo->count() > 0) {
//         QString currentFormation = selectFormationCombo->currentText();
//         qDebug() << "Triggering Formation Combo update for:" << currentFormation;
//         onFormationComboChanged(currentFormation);
//     }
//     if (selectFixedPointCombo->count() > 0) {
//         QString currentFixedPoint = selectFixedPointCombo->currentText();
//         qDebug() << "Triggering FixedPoint Combo update for:" << currentFixedPoint;
//         onFixedPointComboChanged(currentFixedPoint);
//     }
//     if (selectWeaponCombo->count() > 0) {
//         QString currentWeapon = selectWeaponCombo->currentText();
//         qDebug() << "Triggering Weapon Combo update for:" << currentWeapon;
//         onWeaponComboChanged(currentWeapon);
//     }

//     qDebug() << "Finished loadDashboardData";
// }


void Feedback::loadDashboardData(const QString& jsonData)
{
    qDebug() << "Starting loadDashboardData with JSON:" << jsonData;

    // Hierarchy* h = hier; // Hierarchy::getCurrentContext();
    qDebug() << "Hierarchy context:" << (h ? "Valid" : "Null");

    // Parse external JSON data if provided
    QJsonObject externalData;
    if (!jsonData.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (doc.isObject()) {
            externalData = doc.object();
            qDebug() << "Parsed external JSON:" << QJsonDocument(externalData).toJson(QJsonDocument::Compact);
        } else {
            qDebug() << "Invalid JSON data provided";
        }
    }

    // Update Overview tab
    QString systemStatus = externalData.contains("systemStatus") ? externalData["systemStatus"].toString() : "System: ONLINE  Sim: RUNNING  RTC: 2025-10-15";
    systemStatusLabel->setText(systemStatus);
    qDebug() << "System Status set to:" << systemStatus;

    QString uptime = externalData.contains("uptime") ? externalData["uptime"].toString() : "Uptime: Unknown";
    uptimeLabel->setText(uptime);
    qDebug() << "Uptime set to:" << uptime;

    // Count mainEntities for entitiesLabel
    int mainEntities = 0;
    if (h) {
        qDebug() << "Processing entities... Total entities in Hierarchy:" << h->Entities->size();
        for (const auto& [id, e] : *h->Entities) {
            // Declare entityJson before checking conditions
            QJsonObject entityJson = e->toJson();
            // Include entities inside profile category or those with type == "Platform"
            if (h->ProfileCategories.find(e->parentID) != h->ProfileCategories.end() ||
                (entityJson.contains("type") && entityJson["type"].isObject() &&
                 entityJson["type"].toObject().value("value").toString() == "Platform")) {
                mainEntities++;
                qDebug() << "Entity ID:" << QString::fromStdString(id)
                         << "Name:" << QString::fromStdString(e->Name)
                         << "ParentID:" << QString::fromStdString(e->parentID)
                         << "Type:" << (entityJson.contains("type") ? entityJson["type"].toObject().value("value").toString() : "Unknown")
                         << "JSON:" << QJsonDocument(entityJson).toJson(QJsonDocument::Compact);
            } else {
                qDebug() << "Skipping entity ID:" << QString::fromStdString(id)
                         << "because it is outside profile category and type is not 'Platform' (Type:"
                         << (entityJson.contains("type") ? entityJson["type"].toObject().value("value").toString() : "Unknown") << ")";
            }
        }
    }
    entitiesLabel->setText("Entities: " + QString::number(mainEntities));
    qDebug() << "Entities count set to:" << mainEntities;


    QString feedbackEvents = externalData.contains("feedbackEvents") ? externalData["feedbackEvents"].toString() : "Accumulated Feedback Events: Unknown";
    feedbackEventsLabel->setText(feedbackEvents);
    qDebug() << "Feedback Events set to:" << feedbackEvents;

    int cpuUsage = externalData.contains("cpuUsage") ? externalData["cpuUsage"].toInt() : 0;
    cpuProgressBar->setValue(cpuUsage);
    qDebug() << "CPU Progress set to:" << cpuUsage;

    // Update Storage tab
    QString mongoDb = externalData.contains("mongoDb") ? externalData["mongoDb"].toString() : "MongoDB: Unknown";
    mongoDbLabel->setText(mongoDb);
    QString logs = externalData.contains("logs") ? externalData["logs"].toString() : "Logs: Unknown";
    logsLabel->setText(logs);
    QString scenarios = externalData.contains("scenarios") ? externalData["scenarios"].toString() : "Scenarios: Unknown";
    scenariosLabel->setText(scenarios);
    QString totalStorage = externalData.contains("totalStorage") ? externalData["totalStorage"].toString() : "Total: Unknown";
    totalLabel->setText(totalStorage);
    qDebug() << "Storage tab: MongoDB =" << mongoDb << ", Logs =" << logs << ", Scenarios =" << scenarios << ", Total =" << totalStorage;

    // Update Sensors tab
    sensorTable->setRowCount(0);
    QString radarFeedback = "RADAR: Unknown";
    QString iffFeedback = "IFF: Unknown";
    if (h) {
        for (const auto& [id, e] : *h->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("sensors")) {
                QJsonArray sensors = entityJson["sensors"].toArray();
                sensorTable->setRowCount(sensors.size());
                for (int i = 0; i < sensors.size(); ++i) {
                    QJsonObject sensor = sensors[i].toObject();
                    QString sensorName = sensor.contains("name") ? sensor["name"].toString() : "Unknown";
                    QString sensorStatus = sensor.contains("status") ? sensor["status"].toString() : "Unknown";
                    sensorTable->setItem(i, 0, new QTableWidgetItem(sensorName));
                    sensorTable->setItem(i, 1, new QTableWidgetItem(sensorStatus));
                    qDebug() << "Sensor added to table: Name =" << sensorName << ", Status =" << sensorStatus;
                    if (sensor.contains("type") && sensor["type"].toString() == "RADAR") {
                        radarFeedback = "RADAR: " + sensorStatus;
                    }
                }
            }
        }
    }
    radarFeedbackLabel->setText(radarFeedback);
    iffFeedbackLabel->setText(iffFeedback);
    qDebug() << "Sensors tab: RADAR =" << radarFeedback << ", IFF =" << iffFeedback;

    // Update Radio tab
    QString radioSystem = externalData.contains("radioSystem") ? externalData["radioSystem"].toString() : "Radio System: Unknown";
    radioSystemLabel->setText(radioSystem);
    QString frequency = externalData.contains("frequency") ? externalData["frequency"].toString() : "Frequency: Unknown";
    frequencyLabel->setText(frequency);
    QString signalStrength = externalData.contains("signalStrength") ? externalData["signalStrength"].toString() : "Signal Strength: Unknown";
    signalStrengthLabel->setText(signalStrength);
    qDebug() << "Radio tab: System =" << radioSystem << ", Frequency =" << frequency << ", Signal Strength =" << signalStrength;

    // Update Network tab
    QString connectivity = externalData.contains("connectivity") ? externalData["connectivity"].toString() : "Connectivity: Unknown";
    connectivityLabel->setText(connectivity);
    QString bandwidth = externalData.contains("bandwidth") ? externalData["bandwidth"].toString() : "Bandwidth Usage: Unknown";
    bandwidthLabel->setText(bandwidth);
    QString latency = externalData.contains("latency") ? externalData["latency"].toString() : "Latency: Unknown";
    latencyLabel->setText(latency);
    qDebug() << "Network tab: Connectivity =" << connectivity << ", Bandwidth =" << bandwidth << ", Latency =" << latency;

    // Update Logs tab
    QString logsText = externalData.contains("logs") ? externalData["logs"].toString() : "No logs available";
    logsTextEdit->setText(logsText);
    qDebug() << "Logs tab set to:" << logsText;

    // Update CanvasWidget tab
    interactionsTable->setRowCount(0);
    if (externalData.contains("interactions")) {
        QJsonArray interactions = externalData["interactions"].toArray();
        interactionsTable->setRowCount(interactions.size());
        for (int i = 0; i < interactions.size(); ++i) {
            QJsonObject interaction = interactions[i].toObject();
            QString source = interaction.contains("source") ? interaction["source"].toString() : "Unknown";
            QString target = interaction.contains("target") ? interaction["target"].toString() : "Unknown";
            interactionsTable->setItem(i, 0, new QTableWidgetItem(source));
            interactionsTable->setItem(i, 1, new QTableWidgetItem(target));
            qDebug() << "Interaction added to table: Source =" << source << ", Target =" << target;
        }
    }
    qDebug() << "CanvasWidget tab: Interactions table updated";

    // Populate Entity Combo
    selectEntityCombo->clear();
    int totalEntities = 0;
    int activeEntities = 0;
    if (h) {
        qDebug() << "Populating Entity Combo...";
        for (const auto& [id, e] : *h->Entities) {
            // Declare entityJson before checking conditions
            QJsonObject entityJson = e->toJson();
            // Include entities inside profile category or those with type == "Platform"
            if (h->ProfileCategories.find(e->parentID) != h->ProfileCategories.end() ||
                (entityJson.contains("type") && entityJson["type"].isObject() &&
                 entityJson["type"].toObject().value("value").toString() == "Platform")) {
                totalEntities++;
                bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
                if (isActive) activeEntities++;
                QString entityName = QString::fromStdString(e->Name);
                QString entityId = QString::fromStdString(id);
                selectEntityCombo->addItem(entityName, entityId);
                qDebug() << "Added Entity to Combo: Name =" << entityName
                         << ", ID =" << entityId
                         << ", Active =" << isActive
                         << ", Type =" << (entityJson.contains("type") ? entityJson["type"].toObject().value("value").toString() : "Unknown");
            } else {
                qDebug() << "Skipping entity ID:" << QString::fromStdString(id)
                         << "because it is outside profile category and type is not 'Platform' (Type:"
                         << (entityJson.contains("type") ? entityJson["type"].toObject().value("value").toString() : "Unknown") << ")";
            }
        }
    }
    totalEntitiesLabel->setText("Total: " + QString::number(totalEntities));
    activeEntitiesLabel->setText("Active: " + QString::number(activeEntities));
    qDebug() << "Entity tab: Total =" << totalEntities << ", Active =" << activeEntities;

    // Populate IFF Combo
    selectIffCombo->clear();
    int totalIffs = 0;
    int activeIffs = 0;
    if (h) {
        qDebug() << "Populating IFF Combo...";
        for (const auto& [mainId, mainE] : *h->Entities) {
            QJsonObject entityJson = mainE->toJson();
            if (entityJson.contains("iffList")) {
                QJsonArray iffArray = entityJson["iffList"].toArray();
                for (const auto& iffValue : iffArray) {
                    QJsonObject iffJson = iffValue.toObject();
                    totalIffs++;
                    QString mode = iffJson.contains("operationalMode") ? iffJson["operationalMode"].toString() : "";
                    if (mode == "Active") activeIffs++;
                    QString iffName = iffJson.contains("Name") ? iffJson["Name"].toString() : "Unknown";
                    QString iffId = iffJson.contains("ID") ? iffJson["ID"].toString() : "";
                    QString name = iffName + " - " + QString::fromStdString(mainE->Name);
                    selectIffCombo->addItem(name, iffId);
                    qDebug() << "Added IFF to Combo: Name =" << name << ", ID =" << iffId << ", Mode =" << mode;
                    if (iffJson.contains("status")) {
                        iffFeedback = "IFF: " + iffJson["status"].toString();
                    }
                }
            } else {
                qDebug() << "No iffList found for entity ID:" << QString::fromStdString(mainId);
            }
        }
    }
    totalIffsLabel->setText("Total: " + QString::number(totalIffs));
    activeIffsLabel->setText("Active: " + QString::number(activeIffs));
    qDebug() << "IFF tab: Total =" << totalIffs << ", Active =" << activeIffs;

    // Populate Formation Combo (placeholder)
    selectFormationCombo->clear();
    totalFormationsLabel->setText("Total: 0");
    activeFormationsLabel->setText("Active: 0");
    qDebug() << "Formation tab: Total = 0, Active = 0 (placeholder)";

    // Populate FixedPoint Combo
    selectFixedPointCombo->clear();
    int totalFixedPoints = 0;
    int activeFixedPoints = 0;
    if (h) {
        qDebug() << "Populating FixedPoint Combo...";
        for (const auto& [id, e] : *h->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("type") && entityJson["type"].isObject() &&
                entityJson["type"].toObject().value("value").toString() == "FixedPoint") {
                totalFixedPoints++;
                bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
                if (isActive) activeFixedPoints++;
                QString entityName = QString::fromStdString(e->Name);
                QString entityId = QString::fromStdString(id);
                selectFixedPointCombo->addItem(entityName, entityId);
                qDebug() << "Added FixedPoint to Combo: Name =" << entityName << ", ID =" << entityId << ", Active =" << isActive;
            }
        }
    }
    totalFixedPointsLabel->setText("Total: " + QString::number(totalFixedPoints));
    activeFixedPointsLabel->setText("Active: " + QString::number(activeFixedPoints));
    qDebug() << "FixedPoint tab: Total =" << totalFixedPoints << ", Active =" << activeFixedPoints;

    // Populate Weapon Combo
    selectWeaponCombo->clear();
    int totalWeapons = 0;
    int activeWeapons = 0;
    if (h) {
        qDebug() << "Populating Weapon Combo...";
        for (const auto& [id, e] : *h->Entities) {
            QJsonObject entityJson = e->toJson();
            if (entityJson.contains("weaponType")) {
                totalWeapons++;
                bool isActive = entityJson.contains("active") && entityJson["active"].toBool();
                if (isActive) activeWeapons++;
                QString entityName = QString::fromStdString(e->Name);
                QString entityId = QString::fromStdString(id);
                selectWeaponCombo->addItem(entityName, entityId);
                qDebug() << "Added Weapon to Combo: Name =" << entityName << ", ID =" << entityId << ", Active =" << isActive;
            }
        }
    }
    totalWeaponsLabel->setText("Total: " + QString::number(totalWeapons));
    activeWeaponsLabel->setText("Active: " + QString::number(activeWeapons));
    qDebug() << "Weapon tab: Total =" << totalWeapons << ", Active =" << activeWeapons;

    // Trigger updates if selected
    if (selectEntityCombo->count() > 0) {
        QString currentEntity = selectEntityCombo->currentText();
        qDebug() << "Triggering Entity Combo update for:" << currentEntity;
        onEntityComboChanged(currentEntity);
    }
    if (selectIffCombo->count() > 0) {
        QString currentIff = selectIffCombo->currentText();
        qDebug() << "Triggering IFF Combo update for:" << currentIff;
        onIffComboChanged(currentIff);
    }
    if (selectFormationCombo->count() > 0) {
        QString currentFormation = selectFormationCombo->currentText();
        qDebug() << "Triggering Formation Combo update for:" << currentFormation;
        onFormationComboChanged(currentFormation);
    }
    if (selectFixedPointCombo->count() > 0) {
        QString currentFixedPoint = selectFixedPointCombo->currentText();
        qDebug() << "Triggering FixedPoint Combo update for:" << currentFixedPoint;
        onFixedPointComboChanged(currentFixedPoint);
    }
    if (selectWeaponCombo->count() > 0) {
        QString currentWeapon = selectWeaponCombo->currentText();
        qDebug() << "Triggering Weapon Combo update for:" << currentWeapon;
        onWeaponComboChanged(currentWeapon);
    }

    qDebug() << "Finished loadDashboardData";
}


