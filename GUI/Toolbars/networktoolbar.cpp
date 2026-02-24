

//============================================================================
// File        : networktoolbar.cpp
// Description : Implementation of NetworkToolbar class for network session
//               management including start/stop sessions, status monitoring,
//               packet analysis, and real-time network metrics visualization.
//============================================================================

#include "networktoolbar.h"
#include "networktoolbar-styles.h"  // Include separate CSS file
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QDateTime>
#include <QDockWidget>
#include <QPainter>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

NetworkToolbar::NetworkToolbar(QWidget *parent) : QToolBar(parent)
{
    setObjectName("NetworkToolbar");
    setWindowTitle("Network Toolbar");
    setMovable(false);

    // Apply toolbar styles
    setStyleSheet(NetworkToolbarStyles::Toolbar);

    // Set smaller icon size (16x16 instead of 24x24)
    setIconSize(QSize(20, 20));

    QString configJson = R"(
    {
        "session": {
            "default_port": 3000,
            "session_types": ["Master", "Slave"]
        },
        "clients": [
            {
                "id": "Client_01",
                "ip": "192.168.1.10",
                "port": 3000,
                "last_activity": "2025-06-06 11:17:30"
            },
            {
                "id": "Client_02",
                "ip": "192.168.1.11",
                "port": 3000,
                "last_activity": "2025-06-06 11:17:30"
            },
            {
                "id": "Client_03",
                "ip": "192.168.1.12",
                "port": 3000,
                "last_activity": "2025-06-06 11:17:30"
            }
        ],
        "protocols": ["OpenDIS", "HLA"],
        "pdu_types": ["Entity State", "Fire", "Detonation"],
        "test_types": ["Compliance", "Stress"],
        "heartbeat_frequencies": ["0.5 Hz", "1 Hz", "2 Hz"],
        "default_latency_threshold": 120,
        "default_pdu": {
            "id": "12345",
            "position": "X: 100, Y: 200"
        },
        "packet_filters": ["All", "Entity State", "Fire", "Detonation"]
    }
    )";

    // Parse JSON configuration
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON configuration";
    } else {
        config = doc.object();
    }

    createActions();
    addAction(startAction);
    addAction(stopAction);
    addAction(statusAction);

    packetTimer = new QTimer(this);
    connect(packetTimer, &QTimer::timeout, this, &NetworkToolbar::updatePacketTable);
}

NetworkToolbar::~NetworkToolbar()
{
    delete networkManager;
}

void NetworkToolbar::setNetworkManager(NetworkManager *netmger){
    networkManager = netmger;
}

QPixmap NetworkToolbar::withWhiteBg(const QString &iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) {
        qWarning() << "Failed to load icon:" << iconPath;
        return QPixmap();
    }

    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);

    QPainter painter(&newPixmap);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();

    return newPixmap;
}

void NetworkToolbar::createActions()
{
    startAction = new QAction(QIcon(withWhiteBg(":/icons/images/play.png")), tr("Start"), this);
    startAction->setToolTip("Start a Master or Slave network session (Ctrl+N)");
    // Apply button style
    startAction->setProperty("class", "toolbar-button");
    connect(startAction, &QAction::triggered, this, &NetworkToolbar::startSession);

    stopAction = new QAction(QIcon(withWhiteBg(":/icons/images/stop.png")), tr("Stop"), this);
    stopAction->setToolTip("Stop the active network session (Ctrl+Shift+N)");
    stopAction->setEnabled(false);
    connect(stopAction, &QAction::triggered, this, &NetworkToolbar::stopSession);

    statusAction = new QAction(QIcon(withWhiteBg(":/icons/images/status-update.png")), tr("Status"), this);
    statusAction->setToolTip("Display real-time network metrics (Ctrl+S)");
    connect(statusAction, &QAction::triggered, this, &NetworkToolbar::viewNetworkStatus);
}

void NetworkToolbar::updateNetwork(){
    // qDebug() << " iam working";
    //networkManager->UpdateClient();
}

void NetworkToolbar::setupDialog(QDialog *dialog, const QString &title)
{
    dialog->setWindowTitle(title);
    dialog->setFixedSize(400, 300);
    dialog->setStyleSheet(NetworkToolbarStyles::Dialog);
}

void NetworkToolbar::showMessage(const QString &title, const QString &message, bool isError)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setStyleSheet(NetworkToolbarStyles::MessageBox);
    msgBox.exec();
}

void NetworkToolbar::startSession()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Start Network Session");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QComboBox *typeCombo = new QComboBox();
    QStringList sessionTypes;
    QJsonArray sessionArray = config["session"].toObject()["session_types"].toArray();
    for (const QJsonValue &value : sessionArray) {
        sessionTypes.append(value.toString());
    }
    typeCombo->addItems(sessionTypes);

    QLineEdit *portEdit = new QLineEdit(QString::number(config["session"].toObject()["default_port"].toInt()));
    QLineEdit *ipEdit = new QLineEdit("127.0.0.1");  // Default to loopback IP for local testing

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->setStyleSheet(NetworkToolbarStyles::ButtonBox);

    QLabel *typeLabel = new QLabel("Session Type:");
    QLabel *portLabel = new QLabel("Port:");
    QLabel *ipLabel = new QLabel("Server IP (for Slave):");

    layout->addWidget(typeLabel);
    layout->addWidget(typeCombo);
    layout->addWidget(portLabel);
    layout->addWidget(portEdit);
    layout->addWidget(ipLabel);
    layout->addWidget(ipEdit);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        bool success = false;

        bool ok;
        int selectedPort = portEdit->text().toInt(&ok);
        QString selectedIP = ipEdit->text().trimmed();

        if (!ok || selectedPort < 1025 || selectedPort > 65535) {
            showMessage("Invalid Port", "Please enter a valid port between 1025 and 65535.", false);
            return;
        }

        if (typeCombo->currentText() == "Master") {
            networkManager->init(selectedIP,selectedPort);
            success = networkManager->startServer(selectedPort);
        } else {
            networkManager->init(selectedIP,selectedPort);
            success = networkManager->startClient();
        }

        dialog.accept();

        showMessage("Session Started",
                    success
                        ? QString("Session started as %1 on port %2")
                              .arg(typeCombo->currentText(), QString::number(selectedPort))
                        : "Failed to start session: Port in use or internal error",
                    !success
                    );

        if (success) {
            stopAction->setEnabled(true);
        }
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}

void NetworkToolbar::enableMessageSendingUI() {
    QWidget *msgWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(msgWidget);

    QLineEdit *inputField = new QLineEdit();
    inputField->setStyleSheet(NetworkToolbarStyles::Dialog); // Reuse line edit style

    QPushButton *sendButton = new QPushButton("Send");
    sendButton->setStyleSheet(NetworkToolbarStyles::PushButton);

    layout->addWidget(inputField);
    layout->addWidget(sendButton);
    msgWidget->setLayout(layout);

    // Add to your main window (assumes you have a layout to add to)
    if (mainWindowLayout) {
        mainWindowLayout->addWidget(msgWidget);
    }

    connect(sendButton, &QPushButton::clicked, this, [this, inputField]() {
        QString msg = inputField->text().trimmed();
        if (!msg.isEmpty()) {
            // networkManager->sendServerMessage(msg);
            inputField->clear();
        }
    });
}

void NetworkToolbar::stopSession()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Confirm Stop Session");
    msgBox.setText("Stop current network session?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setStyleSheet(NetworkToolbarStyles::MessageBox);

    if (msgBox.exec() == QMessageBox::Yes) {
        bool success = networkManager->stopSession();
        showMessage("Session Stopped", success ? "Network session stopped successfully" : "Failed to stop session", !success);
        if (success) {
            stopAction->setEnabled(false);
        }
    }
}

void NetworkToolbar::viewNetworkStatus()
{
    QDialog *dialog = new QDialog(this);
    setupDialog(dialog, "Network Status");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);

    QLabel *countLabel = new QLabel("Connected Clients: 0");
    countLabel->setStyleSheet("color: white; font-weight: bold;");

    QTableWidget *table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"Client ID", "IP", "Status", "Latency"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setStyleSheet(NetworkToolbarStyles::TableWidget);

    // Update status function
    auto updateStatus = [=]() {
        QStringList status = networkManager->getNetworkStatus();
        table->setRowCount(0);
        int connectedCount = 0;
        for (const QString &entry : status) {
            QStringList fields = entry.split(",");
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(fields[0]));
            table->setItem(row, 1, new QTableWidgetItem(fields[1]));
            table->setItem(row, 2, new QTableWidgetItem(fields[2]));
            table->setItem(row, 3, new QTableWidgetItem(fields[3]));
            if (fields[2] == "Connected") connectedCount++;
        }
        countLabel->setText(QString("Connected Clients: %1").arg(connectedCount));
    };

    // Initial update
    updateStatus();

    // Real-time updates
    QTimer *statusTimer = new QTimer(dialog);
    connect(statusTimer, &QTimer::timeout, updateStatus);
    statusTimer->start(1000);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Save);
    buttons->setStyleSheet(NetworkToolbarStyles::ButtonBox);

    connect(buttons, &QDialogButtonBox::accepted, [=]() {
        statusTimer->stop();
        dialog->accept();
    });
    connect(buttons, &QDialogButtonBox::clicked, [=](QAbstractButton *button) {
        if (button->text() == "Save") {
            // Export logic here
        }
    });

    layout->addWidget(countLabel);
    layout->addWidget(table);
    layout->addWidget(buttons);
    dialog->setLayout(layout);
    dialog->exec();
}

void NetworkToolbar::openPacketAnalyzer()
{
    QDialog *dialog = new QDialog(this);
    setupDialog(dialog, "Packet Analyzer");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);

    QComboBox *filterCombo = new QComboBox();
    QStringList filters;
    QJsonArray filterArray = config["packet_filters"].toArray();
    for (const QJsonValue &value : filterArray) {
        filters.append(value.toString());
    }
    filterCombo->addItems(filters);

    packetTable = new QTableWidget(0, 4);
    packetTable->setHorizontalHeaderLabels({"Time", "Type", "Source", "Data"});
    packetTable->horizontalHeader()->setStretchLastSection(true);
    packetTable->setStyleSheet(NetworkToolbarStyles::TableWidget);

    QPushButton *startButton = new QPushButton("Start Capture");
    QPushButton *stopButton = new QPushButton("Stop Capture");
    QPushButton *closeButton = new QPushButton("Close");

    startButton->setStyleSheet(NetworkToolbarStyles::PushButton);
    stopButton->setStyleSheet(NetworkToolbarStyles::PushButton);
    closeButton->setStyleSheet(NetworkToolbarStyles::PushButton);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(closeButton);
    QLabel *filterLabel = new QLabel("Filter:");
    filterLabel->setStyleSheet("color: white;");
    layout->addWidget(filterLabel);
    layout->addWidget(filterCombo);
    layout->addWidget(packetTable);
    layout->addLayout(buttonLayout);
    dialog->setLayout(layout);

    connect(startButton, &QPushButton::clicked, [=]() {
        packetTimer->start(1000);
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
    });
    connect(stopButton, &QPushButton::clicked, [=]() {
        packetTimer->stop();
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    });
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    updatePacketTable();
    dialog->exec();

    packetTimer->stop();
}

void NetworkToolbar::updatePacketTable()
{

}
