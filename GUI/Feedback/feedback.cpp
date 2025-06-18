
#include <QtGui/QIcon>
#include "feedback.h"
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QDebug>

Feedback::Feedback(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    // Define JSON data using concatenated strings as requested
    QString jsonData =
        "{"
        "\"system_status\": \"ONLINE\","
        "\"sim_status\": \"RUNNING\","
        "\"rtc\": \"2025-06-04 14:18\","
        "\"uptime\": \"12 h 34 m\","
        "\"entities\": 150,"
        "\"active_entities\": 120,"
        "\"feedback_events\": 320,"
        "\"cpu_usage\": 60,"
        "\"sensor_feedback\": ["
        "  {\"type\": \"RADAR\", \"status\": \"Active\", \"snr\": 45, \"detection_accuracy\": 92, \"accumulated_detections\": 200},"
        "  {\"type\": \"IFF\", \"status\": \"9 Hz\", \"detection_accuracy\": 98, \"accumulated_detections\": 150}"
        "],"
        "\"detections\": {"
        "  \"radar\": [10, 20, 15, 30, 25],"
        "  \"iff\": [5, 10, 8, 15, 12]"
        "},"
        "\"canvas_interactions\": ["
        "  {\"time\": \"14:18\", \"id\": \"E1\", \"geoCoords\": \"12.34, 56.78\", \"fixedPointList\": \"[A1, B2]\"},"
        "  {\"time\": \"14:17\", \"id\": \"E2\", \"geoCoords\": \"23.45, 67.89\", \"fixedPointList\": \"[C3, D4]\"}"
        "],"
        "\"entity_list\": ["
        "  {\"id\": \"E1\", \"name\": \"E1 - Aircraft\", \"type\": \"Aircraft\", \"geoCoords\": \"12.34, 56.78\", \"status\": \"Active\", \"mission\": \"M1 (Patrol)\", \"tracking_accuracy\": \"+/-5 meters\", \"accumulated_interactions\": 50},"
        "  {\"id\": \"E2\", \"name\": \"E2 - Aircraft\", \"type\": \"Aircraft\", \"geoCoords\": \"23.45, 67.89\", \"status\": \"Active\", \"mission\": \"M2 (Recon)\", \"tracking_accuracy\": \"+/-3 meters\", \"accumulated_interactions\": 30}"
        "],"
        "\"entity_details\": {"
        "  \"E1\": {"
        "    \"type\": \"Aircraft\","
        "    \"geoCoords\": \"12.34, 56.78\","
        "    \"status\": \"Active\","
        "    \"tracking_accuracy\": \"+/-5 meters\","
        "    \"accumulated_interactions\": 50,"
        "    \"mission\": \"M1 (Patrol, Start: 17:00, Actions: [Navigate, Engage])\","
        "    \"formation\": \"F1 (Leader, 3 members)\","
        "    \"sensor_detections\": {"
        "      \"radar\": \"Detected at 17:38, SNR 45 dB, Accuracy 92%\","
        "      \"iff\": \"Confirmed at 17:37, 9 Hz, Accuracy 98%\""
        "    },"
        "    \"logs\": ["
        "      {\"type\": \"INFO\", \"message\": \"E1 selected at 17:38\"},"
        "      {\"type\": \"INFO\", \"message\": \"E1 changed state to Engage at 17:37\"}"
        "    ]"
        "  }"
        "},"
        "\"interaction_frequency\": ["
        "  {\"time\": \"17:34\", \"E1\": 5, \"E2\": 3},"
        "  {\"time\": \"17:35\", \"E1\": 7, \"E2\": 4},"
        "  {\"time\": \"17:30\", \"E1\": 10, \"E2\": 6},"
        "  {\"time\": \"17:37\", \"E1\": 8, \"E2\": 5},"
        "  {\"time\": \"17:38\", \"E1\": 6, \"E2\": 4}"
        "],"
        "\"storage_usage\": {"
        "  \"mongo_db\": 500,"
        "  \"logs\": 100,"
        "  \"scenarios\": 50,"
        "  \"total\": 10000"
        "},"
        "\"rtc_logs\": {"
        "  \"timestamps\": [\"14:15\", \"14:16\", \"14:17\", \"14:18\"],"
        "  \"values\": [50, 55, 60, 58]"
        "},"
        "\"logs\": ["
        "  {\"type\": \"INFO\", \"message\": \"Entity E1 selected at 17:31\"},"
        "  {\"type\": \"ERROR\", \"message\": \"RADAR SNR low at 17:30\"}"
        "]"
        "}";

    // Validate JSON at runtime
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON Parse Error:" << parseError.errorString() << "at offset" << parseError.offset;
        return;
    }

    loadDashboardData(jsonData);
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

    // Create a frame for the overview section
    QFrame *overviewFrame = new QFrame();
    overviewFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *overviewFrameLayout = new QVBoxLayout(overviewFrame);

    // Add header
    QLabel *header = new QLabel("System Overview");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    overviewFrameLayout->addWidget(header);

    // Create a grid layout for the metrics
    QGridLayout *metricsLayout = new QGridLayout();
    metricsLayout->setVerticalSpacing(15);
    metricsLayout->setHorizontalSpacing(30);

    // System status row (combined into one label)
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

    // Uptime row
    uptimeLabel = new QLabel("Uptime: 12 h 34 m");
    uptimeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(uptimeLabel, 1, 0);

    // Entities row
    entitiesLabel = new QLabel("Entities: 150");
    entitiesLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(entitiesLabel, 2, 0);

    // Feedback events row
    feedbackEventsLabel = new QLabel("Accumulated Feedback Events: 320");
    feedbackEventsLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(feedbackEventsLabel, 3, 0);

    // Simulation accuracy row
    QLabel *accuracyLabel = new QLabel("Simulation Accuracy: 95%");
    accuracyLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    metricsLayout->addWidget(accuracyLabel, 4, 0);

    // CPU usage row (progress bar only, with integrated text)
    cpuProgressBar = new QProgressBar();
    cpuProgressBar->setRange(0, 100);
    cpuProgressBar->setValue(60);
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

    // Outer frame
    QFrame *storageFrame = new QFrame();
    storageFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *storageFrameLayout = new QVBoxLayout(storageFrame);

    // Header
    QLabel *header = new QLabel("Storage Usage");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    storageFrameLayout->addWidget(header);

    // Content area
    QHBoxLayout *storageContentLayout = new QHBoxLayout();
    storageContentLayout->setSpacing(20);
    storageContentLayout->setAlignment(Qt::AlignTop);

    // Pie chart
    storageChart = new CustomGraphWidget(CustomGraphWidget::Pie);
    storageChart->setFixedSize(150, 100);
    storageContentLayout->addWidget(storageChart);

    // Labels
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

    mongoDbLabel = new QLabel("MongoDB: 500 MB");
    mongoDbLabel->setStyleSheet(labelStyle);
    mongoDbLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    logsLabel = new QLabel("Logs: 100 MB");
    logsLabel->setStyleSheet(labelStyle);
    logsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    scenariosLabel = new QLabel("Scenarios: 50 MB");
    scenariosLabel->setStyleSheet(labelStyle);
    scenariosLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    totalLabel = new QLabel("Total: 10 GB");
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

    // Header
    QLabel *header = new QLabel("Sensors");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    sensorsFrameLayout->addWidget(header);

    // Sensor List Table
    QLabel *sensorListHeader = new QLabel("Sensor List");
    sensorListHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; margin-bottom: 10px;");
    sensorsFrameLayout->addWidget(sensorListHeader);

    sensorTable = new QTableWidget();
    sensorTable->setColumnCount(2);
    sensorTable->setHorizontalHeaderLabels({"Type", "Status"});
    sensorTable->setRowCount(2);
    sensorTable->setItem(0, 0, new QTableWidgetItem("RADAR"));
    sensorTable->setItem(0, 1, new QTableWidgetItem("Active"));
    sensorTable->setItem(1, 0, new QTableWidgetItem("IFF"));
    sensorTable->setItem(1, 1, new QTableWidgetItem("9 Hz"));
    sensorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sensorTable->verticalHeader()->setVisible(false);
    sensorTable->setShowGrid(false);
    sensorTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sensorTable->verticalHeader()->setDefaultSectionSize(15);
    sensorTable->horizontalHeader()->setFixedHeight(50);
    sensorTable->horizontalHeader()->setVisible(true);
    sensorsFrameLayout->addWidget(sensorTable);

    // Sensor Feedback Section
    QLabel *feedbackHeader = new QLabel("Sensor Feedback");
    feedbackHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; margin: 20px 0 10px 0;");
    sensorsFrameLayout->addWidget(feedbackHeader);

    // Content area: Graph + Feedback Labels side by side
    QHBoxLayout *feedbackContentLayout = new QHBoxLayout();
    feedbackContentLayout->setSpacing(20);
    feedbackContentLayout->setAlignment(Qt::AlignTop);

    // Bar chart for Accumulated Detections
    sensorChart = new CustomGraphWidget(CustomGraphWidget::Bar);
    sensorChart->setFixedSize(200, 150);
    feedbackContentLayout->addWidget(sensorChart);

    // Feedback Labels
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

    // RADAR Feedback
    radarFeedbackLabel = new QLabel("RADAR: Active, SNR: 45 dB, Accuracy: 92%, Detections: 200");
    radarFeedbackLabel->setStyleSheet(labelStyle);
    radarFeedbackLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // IFF Feedback
    iffFeedbackLabel = new QLabel("IFF: 9 Hz, Accuracy: 98%, Detections: 150");
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

    // Header
    QLabel *header = new QLabel("Radio Status");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    radioFrameLayout->addWidget(header);

    // Radio Status Labels
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

    // Radio System
    radioSystemLabel = new QLabel("Radio System: Active");
    radioSystemLabel->setStyleSheet(labelStyle);
    radioSystemLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Frequency
    frequencyLabel = new QLabel("Frequency: 120 MHz");
    frequencyLabel->setStyleSheet(labelStyle);
    frequencyLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Signal Strength
    signalStrengthLabel = new QLabel("Signal Strength: 80%");
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

    // Header
    QLabel *header = new QLabel("Network Status");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    networkFrameLayout->addWidget(header);

    // Network Status Labels
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

    // Connectivity
    connectivityLabel = new QLabel("Connectivity: Online");
    connectivityLabel->setStyleSheet(labelStyle);
    connectivityLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Bandwidth Usage
    bandwidthLabel = new QLabel("Bandwidth Usage: 50 Mbps");
    bandwidthLabel->setStyleSheet(labelStyle);
    bandwidthLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Latency
    latencyLabel = new QLabel("Latency: 20 ms");
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

    // Header
    QLabel *header = new QLabel("RTC and Logs");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    logsFrameLayout->addWidget(header);

    logsTextEdit = new QTextEdit();
    logsTextEdit->setReadOnly(true);
    logsTextEdit->setMinimumHeight(100);
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
    mainLayout->setSpacing(15);

    QFrame *canvasFrame = new QFrame();
    canvasFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *canvasFrameLayout = new QVBoxLayout(canvasFrame);

    // Header
    QLabel *header = new QLabel("CanvasWidget Interactions");
    QFont headerFont("Arial", 16, QFont::Bold);
    header->setFont(headerFont);
    header->setStyleSheet("color: #4a90e2; margin-bottom: 20px;");
    canvasFrameLayout->addWidget(header);

    // Use QVBoxLayout directly to avoid unnecessary horizontal layout
    QVBoxLayout *interactionsTableLayout = new QVBoxLayout();
    interactionsTable = new QTableWidget();
    interactionsTable->setColumnCount(4);
    interactionsTable->setHorizontalHeaderLabels({"Time", "ID", "GeoCoords", "Fixed Point List"});
    // Removed setFixedWidth to allow the table to stretch
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
    // Create the main widget
    entityWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(entityWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Create a QFrame for the content
    entityFrame = new QFrame();
    entityFrame->setStyleSheet("border: 1px solid #3a4565; border-radius: 5px; background-color: #1e2a44; padding: 15px;");
    QVBoxLayout *entityFrameLayout = new QVBoxLayout(entityFrame);

    // Header: Entity List
    QLabel *entityListHeader = new QLabel("Entity List");
    QFont headerFont("Arial", 14, QFont::Bold);
    entityListHeader->setFont(headerFont);
    entityListHeader->setStyleSheet("color: #4a90e2; margin-bottom: 10px;");
    entityFrameLayout->addWidget(entityListHeader);

    // Entity Summary Section
    QVBoxLayout *entitySummaryLayout = new QVBoxLayout();
    entitySummaryLayout->setSpacing(5);
    entitySummaryLayout->setAlignment(Qt::AlignTop);

    // Total Entities
    totalEntitiesLabel = new QLabel("Total Entities: 150");
    totalEntitiesLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");
    entitySummaryLayout->addWidget(totalEntitiesLabel);

    // Active Entities
    activeEntitiesLabel = new QLabel("Active Entities: 120");
    activeEntitiesLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");
    entitySummaryLayout->addWidget(activeEntitiesLabel);

    // Select Entity Dropdown (in a single line)
    QHBoxLayout *selectEntityLayout = new QHBoxLayout();
    selectEntityLayout->setSpacing(10);

    QLabel *selectEntityLabel = new QLabel("Select Entity:");
    selectEntityLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");
    selectEntityLayout->addWidget(selectEntityLabel);

    selectEntityCombo = new QComboBox();
    selectEntityCombo->addItem("E1 - Aircraft");
    selectEntityCombo->addItem("E2 - Aircraft");
    selectEntityCombo->setFixedWidth(200);
    selectEntityLayout->addWidget(selectEntityCombo);

    selectEntityLayout->addStretch();
    entitySummaryLayout->addLayout(selectEntityLayout);

    entityFrameLayout->addLayout(entitySummaryLayout);

    // Detailed View (visible by default)
    detailedViewWidget = new QWidget();
    QVBoxLayout *detailedViewLayout = new QVBoxLayout(detailedViewWidget);
    detailedViewLayout->setSpacing(5);

    detailedViewHeader = new QLabel("Entity E1 - Detailed View");
    detailedViewHeader->setFont(headerFont);
    detailedViewHeader->setStyleSheet("color: #4a90e2; margin: 10px 0 5px 0;");
    detailedViewLayout->addWidget(detailedViewHeader);

    QString labelStyle =
        "color: white;"
        "background-color: #182038;"
        "border: 1px solid #3a4565;"
        "border-radius: 5px;"
        "padding: 4px 8px;"
        "margin-bottom: 5px;"
        "min-height: 28px;"
        "max-height: 28px;"
        "min-width: 300px;";

    detailedViewDetailsLabel = new QLabel(
        "- Type: Aircraft\n"
        "- GeoCoords: 12.34, 56.78\n"
        "- Status: Active\n"
        "- Tracking Accuracy: ±5 meters\n"
        "- Accumulated Interactions: 50\n"
        "- Mission: M1 (Patrol, Start: 17:00, Actions: [Navigate, Engage])\n"
        "- Formation: F1 (Leader, 3 members)\n"
        "\nSensor Detections:\n"
        "- Radar: Detected at 17:38, SNR 45 dB, Accuracy 92%\n"
        "- IFF: Confirmed at 17:37, 9 Hz, Accuracy 98%"
        );
    detailedViewDetailsLabel->setStyleSheet(labelStyle);
    detailedViewDetailsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    detailedViewLayout->addWidget(detailedViewDetailsLabel);

    QLabel *detailedLogsHeader = new QLabel("Logs");
    detailedLogsHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; margin: 10px 0 5px 0;");
    detailedViewLayout->addWidget(detailedLogsHeader);

    detailedViewLogsTextEdit = new QTextEdit();
    detailedViewLogsTextEdit->setReadOnly(true);
    detailedViewLogsTextEdit->setMinimumHeight(50);
    detailedViewLogsTextEdit->setText(
        "[INFO] E1 selected at 17:38\n"
        "[INFO] E1 changed state to Engage at 17:37"
        );
    detailedViewLayout->addWidget(detailedViewLogsTextEdit);

    detailedViewWidget->setVisible(true); // Visible by default
    entityFrameLayout->addWidget(detailedViewWidget);

    // Interaction Frequency Section
    QLabel *interactionFreqHeader = new QLabel("Interaction Frequency");
    interactionFreqHeader->setFont(headerFont);
    interactionFreqHeader->setStyleSheet("color: #4a90e2; margin: 10px 0 5px 0;");
    entityFrameLayout->addWidget(interactionFreqHeader);

    interactionFreqTable = new QTableWidget();
    interactionFreqTable->setColumnCount(3);
    interactionFreqTable->setHorizontalHeaderLabels({"Time", "E1 Interactions", "E2 Interactions"});
    interactionFreqTable->setRowCount(5);
    interactionFreqTable->setItem(0, 0, new QTableWidgetItem("17:34"));
    interactionFreqTable->setItem(0, 1, new QTableWidgetItem("5"));
    interactionFreqTable->setItem(0, 2, new QTableWidgetItem("3"));
    interactionFreqTable->setItem(1, 0, new QTableWidgetItem("17:35"));
    interactionFreqTable->setItem(1, 1, new QTableWidgetItem("7"));
    interactionFreqTable->setItem(1, 2, new QTableWidgetItem("4"));
    interactionFreqTable->setItem(2, 0, new QTableWidgetItem("17:36"));
    interactionFreqTable->setItem(2, 1, new QTableWidgetItem("10"));
    interactionFreqTable->setItem(2, 2, new QTableWidgetItem("6"));
    interactionFreqTable->setItem(3, 0, new QTableWidgetItem("17:37"));
    interactionFreqTable->setItem(3, 1, new QTableWidgetItem("8"));
    interactionFreqTable->setItem(3, 2, new QTableWidgetItem("5"));
    interactionFreqTable->setItem(4, 0, new QTableWidgetItem("17:38"));
    interactionFreqTable->setItem(4, 1, new QTableWidgetItem("6"));
    interactionFreqTable->setItem(4, 2, new QTableWidgetItem("4"));
    interactionFreqTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    interactionFreqTable->verticalHeader()->setVisible(false);
    interactionFreqTable->setShowGrid(false);
    interactionFreqTable->verticalHeader()->setDefaultSectionSize(15);
    interactionFreqTable->horizontalHeader()->setFixedHeight(50);
    interactionFreqTable->horizontalHeader()->setVisible(true);

    // Explicitly ensure the header is visible
    interactionFreqTable->horizontalHeader()->show();

    // Remove custom stylesheet to use global styling
    interactionFreqTable->setStyleSheet("");

    // Set a minimum width to ensure the table has enough space
    interactionFreqTable->setMinimumWidth(400);

    // Calculate height to ensure the header and rows are fully visible
    int rowHeight = interactionFreqTable->verticalHeader()->defaultSectionSize();
    int headerHeight = interactionFreqTable->horizontalHeader()->height();
    int totalRows = interactionFreqTable->rowCount();
    int totalHeight = (totalRows * rowHeight) + headerHeight + 5;
    interactionFreqTable->setMinimumHeight(totalHeight);
    interactionFreqTable->setMaximumHeight(totalHeight);

    entityFrameLayout->addWidget(interactionFreqTable);

    mainLayout->addWidget(entityFrame);

    // Add entityFrame directly to contentStack
    contentStack->addWidget(entityFrame);

    // Force rendering of the table and its header
    interactionFreqTable->show();
    interactionFreqTable->horizontalHeader()->show();
    interactionFreqTable->repaint();
    interactionFreqTable->horizontalHeader()->repaint();

    // Log header visibility and height for debugging
    qDebug() << "interactionFreqTable header visible after setup:" << interactionFreqTable->horizontalHeader()->isVisible();
    qDebug() << "interactionFreqTable header height after setup:" << interactionFreqTable->horizontalHeader()->height();

    // Delayed visibility check after the widget is rendered
    QTimer::singleShot(1000, this, [this]() {
        qDebug() << "interactionFreqTable header visible after 1s:" << interactionFreqTable->horizontalHeader()->isVisible();
        // Force visibility again if needed
        if (!interactionFreqTable->horizontalHeader()->isVisible()) {
            interactionFreqTable->horizontalHeader()->setVisible(true);
            interactionFreqTable->show();
            interactionFreqTable->repaint();
            interactionFreqTable->horizontalHeader()->repaint();
            qDebug() << "Forced header visibility after 1s";
        }
    });
}

void Feedback::onDetailsButtonClicked()
{
    detailedViewWidget->setVisible(false); // Hide the detailed view
    detailsButton->setEnabled(false); // Disable the button to prevent further toggling
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
        contentStack->setCurrentWidget(entityFrame); // Use entityFrame instead of entityScrollArea
        // Ensure header visibility when the tab is selected
        interactionFreqTable->horizontalHeader()->setVisible(true);
        interactionFreqTable->update();
        interactionFreqTable->horizontalHeader()->update();
        qDebug() << "Entity tab selected - header visible:" << interactionFreqTable->horizontalHeader()->isVisible();
    }
}

void Feedback::loadDashboardData(const QString& jsonData)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    QJsonObject obj = doc.object();

    // Update systemStatusLabel
    systemStatusLabel->setText(
        QString("System: %1  Sim: %2  RTC: %3")
            .arg(obj["system_status"].toString(),
                 obj["sim_status"].toString(),
                 obj["rtc"].toString())
        );

    uptimeLabel->setText("Uptime: " + obj["uptime"].toString());
    entitiesLabel->setText("Entities: " + QString::number(obj["entities"].toInt()));
    feedbackEventsLabel->setText("Accumulated Feedback Events: " + QString::number(obj["feedback_events"].toInt()));
    cpuProgressBar->setValue(obj["cpu_usage"].toInt());

    // Update sensor data
    QJsonArray sensorArray = obj["sensor_feedback"].toArray();
    sensorTable->setRowCount(sensorArray.size());
    for (int i = 0; i < sensorArray.size(); ++i) {
        QJsonObject sensor = sensorArray[i].toObject();
        sensorTable->setItem(i, 0, new QTableWidgetItem(sensor["type"].toString()));
        sensorTable->setItem(i, 1, new QTableWidgetItem(sensor["status"].toString()));
    }

    // Update sensor feedback labels and chart
    if (sensorArray.size() >= 2) {
        // RADAR
        QJsonObject radar = sensorArray[0].toObject();
        radarFeedbackLabel->setText(
            QString("RADAR: %1, SNR: %2 dB, Accuracy: %3%, Detections: %4")
                .arg(radar["status"].toString(),
                     QString::number(radar["snr"].toInt()),
                     QString::number(radar["detection_accuracy"].toInt()),
                     QString::number(radar["accumulated_detections"].toInt()))
            );

        // IFF
        QJsonObject iff = sensorArray[1].toObject();
        iffFeedbackLabel->setText(
            QString("IFF: %1, Accuracy: %2%, Detections: %3")
                .arg(iff["status"].toString(),
                     QString::number(iff["detection_accuracy"].toInt()),
                     QString::number(iff["accumulated_detections"].toInt()))
            );

        // Set bar chart data
        QList<qreal> detections = {
            radar["accumulated_detections"].toDouble(),
            iff["accumulated_detections"].toDouble()
        };
        sensorChart->setData(detections);
    }

    // Update radio data (placeholder for future JSON data)
    if (obj.contains("radio_status")) {
        QJsonObject radio = obj["radio_status"].toObject();
        radioSystemLabel->setText("Radio System: " + radio["system"].toString());
        frequencyLabel->setText("Frequency: " + QString::number(radio["frequency"].toDouble()) + " MHz");
        signalStrengthLabel->setText("Signal Strength: " + QString::number(radio["signal_strength"].toInt()) + "%");
    }

    // Update network data (placeholder for future JSON data)
    if (obj.contains("network_status")) {
        QJsonObject network = obj["network_status"].toObject();
        connectivityLabel->setText("Connectivity: " + network["connectivity"].toString());
        bandwidthLabel->setText("Bandwidth Usage: " + QString::number(network["bandwidth"].toDouble()) + " Mbps");
        latencyLabel->setText("Latency: " + QString::number(network["latency"].toInt()) + " ms");
    }

    // Update entity data
    totalEntitiesLabel->setText("Total Entities: " + QString::number(obj["entities"].toInt()));
    activeEntitiesLabel->setText("Active Entities: " + QString::number(obj["active_entities"].toInt()));

    // Populate the entity dropdown
    selectEntityCombo->clear();
    QJsonArray entityArray = obj["entity_list"].toArray();
    for (const auto& entity : entityArray) {
        QJsonObject entityObj = entity.toObject();
        selectEntityCombo->addItem(entityObj["name"].toString());
    }

    // Update detailed view for E1
    if (obj.contains("entity_details")) {
        QJsonObject entityDetails = obj["entity_details"].toObject();
        if (entityDetails.contains("E1")) {
            QJsonObject e1Details = entityDetails["E1"].toObject();
            detailedViewHeader->setText("Entity E1 - Detailed View");
            detailedViewDetailsLabel->setText(
                QString("- Type: %1\n"
                        "- GeoCoords: %2\n"
                        "- Status: %3\n"
                        "- Tracking Accuracy: %4\n"
                        "- Accumulated Interactions: %5\n"
                        "- Mission: %6\n"
                        "- Formation: %7\n"
                        "\nSensor Detections:\n"
                        "- Radar: %8\n"
                        "- IFF: %9")
                    .arg(e1Details["type"].toString(),
                         e1Details["geoCoords"].toString(),
                         e1Details["status"].toString(),
                         e1Details["tracking_accuracy"].toString(),
                         QString::number(e1Details["accumulated_interactions"].toInt()),
                         e1Details["mission"].toString(),
                         e1Details["formation"].toString(),
                         e1Details["sensor_detections"].toObject()["radar"].toString(),
                         e1Details["sensor_detections"].toObject()["iff"].toString())
                );

            QString logsText;
            QJsonArray e1Logs = e1Details["logs"].toArray();
            for (const auto& log : e1Logs) {
                QJsonObject logObj = log.toObject();
                logsText += "[" + logObj["type"].toString() + "] " + logObj["message"].toString() + "\n";
            }
            detailedViewLogsTextEdit->setText(logsText);
        }
    }

    // Update interaction frequency table
    QJsonArray interactionFreqArray = obj["interaction_frequency"].toArray();
    interactionFreqTable->setRowCount(interactionFreqArray.size());
    for (int i = 0; i < interactionFreqArray.size(); ++i) {
        QJsonObject freq = interactionFreqArray[i].toObject();
        interactionFreqTable->setItem(i, 0, new QTableWidgetItem(freq["time"].toString()));
        interactionFreqTable->setItem(i, 1, new QTableWidgetItem(QString::number(freq["E1"].toInt())));
        interactionFreqTable->setItem(i, 2, new QTableWidgetItem(QString::number(freq["E2"].toInt())));
    }

    // Update canvas interactions
    QJsonArray interactionsArray = obj["canvas_interactions"].toArray();
    interactionsTable->setRowCount(interactionsArray.size());
    for (int i = 0; i < interactionsArray.size(); ++i) {
        QJsonObject interaction = interactionsArray[i].toObject();
        interactionsTable->setItem(i, 0, new QTableWidgetItem(interaction["time"].toString()));
        interactionsTable->setItem(i, 1, new QTableWidgetItem(interaction["id"].toString()));
        interactionsTable->setItem(i, 2, new QTableWidgetItem(interaction["geoCoords"].toString()));
        interactionsTable->setItem(i, 3, new QTableWidgetItem(interaction["fixedPointList"].toString()));
    }

    // Update storage data and set pie chart data
    QJsonObject storage = obj["storage_usage"].toObject();
    qreal mongoDb = storage["mongo_db"].toInt();
    qreal logs = storage["logs"].toInt();
    qreal scenarios = storage["scenarios"].toInt();

    // Set pie chart data
    QList<qreal> storageData = {mongoDb, logs, scenarios};
    storageChart->setData(storageData);

    // Update labels
    mongoDbLabel->setText("MongoDB: " + QString::number(mongoDb) + " MB");
    logsLabel->setText("Logs: " + QString::number(logs) + " MB");
    scenariosLabel->setText("Scenarios: " + QString::number(scenarios) + " MB");
    totalLabel->setText("Total: " + QString::number(storage["total"].toInt() / 1000.0) + " GB");

    QJsonArray logsArray = obj["logs"].toArray();
    QString logsText;
    for (const auto& log : logsArray) {
        QJsonObject logObj = log.toObject();
        logsText += "[" + logObj["type"].toString() + "] " + logObj["message"].toString() + "\n";
    }
    logsTextEdit->setText(logsText);
}

