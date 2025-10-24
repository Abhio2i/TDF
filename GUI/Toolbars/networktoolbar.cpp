
#include "networktoolbar.h"
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

// // Stub NetworkManager with packet signal
// class NetworkManager {
// public:
//     bool startSession(const QString &, int) { return true; }
//     bool stopSession() { return true; }
//     bool connectClient(const QString &, const QString &, int, const QString &) { return true; }
//     bool disconnectClient(const QString &) { return true; }
//     QStringList getNetworkStatus() { return {"Client_01,192.168.1.10,Connected,90 ms", "Client_02,192.168.1.11,Disconnected,N/A"}; }
//     bool runTest(const QString &, const QString &) { return true; }
//     bool sendPDU(const QString &, const QString &, const QString &) { return true; }
//     QStringList getPackets() {
//         QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
//         return {time + ",Entity,192.168.1.10,Pos:100,200", time + ",Fire,192.168.1.11,Target:12345"};
//     }
// };




NetworkToolbar::NetworkToolbar(QWidget *parent) : QToolBar(parent)
{
    setObjectName("NetworkToolbar");
    setWindowTitle("Network Tools");
    setMovable(false);

    // Define JSON configuration as a string
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

    // Add actions to the toolbar with separators for grouping
    addAction(startAction);
    addAction(stopAction);
    addSeparator();
    addAction(connectAction);
    addAction(disconnectAction);
    addSeparator();
    addAction(statusAction);
    addAction(packetAction);
    addSeparator();
    addAction(testAction);
    addAction(syncAction);
    addAction(logAction);
    addSeparator();
    addAction(pduAction);
    addAction(refreshAction);

    // Initialize network manager and timer
    //networkManager = new NetworkManager();
    packetTimer = new QTimer(this);
    connect(packetTimer, &QTimer::timeout, this, &NetworkToolbar::updatePacketTable);

    // Set icon size to match StandardToolBar
    setIconSize(QSize(24, 24));
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
    //startAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(startAction, &QAction::triggered, this, &NetworkToolbar::startSession);

    stopAction = new QAction(QIcon(withWhiteBg(":/icons/images/stop.png")), tr("Stop"), this);
    stopAction->setToolTip("Stop the active network session (Ctrl+Shift+N)");
    //stopAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    stopAction->setEnabled(false);
    connect(stopAction, &QAction::triggered, this, &NetworkToolbar::stopSession);

    connectAction = new QAction(QIcon(withWhiteBg(":/icons/images/network.png")), tr("Connect"), this);
    connectAction->setToolTip("Connect to a Slave node (Ctrl+C)");
    //connectAction->setShortcut(QKeySequence("Ctrl+C"));
    connect(connectAction, &QAction::triggered, this, &NetworkToolbar::connectClient);

    disconnectAction = new QAction(QIcon(withWhiteBg(":/icons/images/disconnect.png")), tr("Disconnect"), this);
    disconnectAction->setToolTip("Disconnect a selected Slave node (Ctrl+D)");
    //disconnectAction->setShortcut(QKeySequence("Ctrl+D"));
    disconnectAction->setEnabled(false);
    connect(disconnectAction, &QAction::triggered, this, &NetworkToolbar::disconnectClient);

    statusAction = new QAction(QIcon(withWhiteBg(":/icons/images/status-update.png")), tr("Status"), this);
    statusAction->setToolTip("Display real-time network metrics (Ctrl+S)");
    //statusAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(statusAction, &QAction::triggered, this, &NetworkToolbar::viewNetworkStatus);

    packetAction = new QAction(QIcon(withWhiteBg(":/icons/images/packet.png")), tr("Packet Analyzer"), this);
    packetAction->setToolTip("Launch packet capture and analysis (Ctrl+P)");
    //packetAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(packetAction, &QAction::triggered, this, &NetworkToolbar::openPacketAnalyzer);

    testAction = new QAction(QIcon(withWhiteBg(":/icons/images/test.png")), tr("Test"), this);
    testAction->setToolTip("Run TTCN-3 test to validate protocol (Ctrl+T)");
    //testAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(testAction, &QAction::triggered, this, &NetworkToolbar::runProtocolTest);

    syncAction = new QAction(QIcon(withWhiteBg(":/icons/images/sync.png")), tr("Sync"), this);
    syncAction->setToolTip("Configure network synchronization settings (Ctrl+Y)");
    //syncAction->setShortcut(QKeySequence("Ctrl+Y"));
    connect(syncAction, &QAction::triggered, this, &NetworkToolbar::configureSyncSettings);

    logAction = new QAction(QIcon(withWhiteBg(":/icons/images/log-file.png")), tr("Logs"), this);
    logAction->setToolTip("View network-related logs (Ctrl+L)");
    //logAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(logAction, &QAction::triggered, this, &NetworkToolbar::viewLogs);

    pduAction = new QAction(QIcon(withWhiteBg(":/icons/images/pdu.png")), tr("PDU"), this);
    pduAction->setToolTip("Create or simulate a sample DIS PDU (Ctrl+M)");
    //pduAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(pduAction, &QAction::triggered, this, &NetworkToolbar::simulatePDU);


    refreshAction = new QAction(QIcon(withWhiteBg(":/icons/images/loading-arrow")), tr("Refresh"), this);
    connect(refreshAction, &QAction::triggered, this, &NetworkToolbar::updateNetwork);

}
void NetworkToolbar::updateNetwork(){
    qDebug() << " iam working";
    //networkManager->UpdateClient();
}

void NetworkToolbar::setupDialog(QDialog *dialog, const QString &title)
{
    dialog->setWindowTitle(title);
    dialog->setFixedSize(400, 300);
    dialog->setStyleSheet("QDialog { background-color: black; }");
}

void NetworkToolbar::showMessage(const QString &title, const QString &message, bool isError)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setStyleSheet(isError ? "QMessageBox { color: red; }" : "");
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

    layout->addWidget(new QLabel("Session Type:"));
    layout->addWidget(typeCombo);
    layout->addWidget(new QLabel("Port:"));
    layout->addWidget(portEdit);
    layout->addWidget(new QLabel("Server IP (for Slave):"));
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

        qDebug() << "[UI] Selected session type:" << typeCombo->currentText();
        qDebug() << "[UI] Trying to start on port:" << selectedPort;

        if (typeCombo->currentText() == "Master") {
            networkManager->init(selectedIP,selectedPort);
            // enableMessageSendingUI();
            success = networkManager->startServer(selectedPort);
        } else {
           // networkManager->initClient(selectedIP, selectedPort);
            networkManager->init(selectedIP,selectedPort);
            success = networkManager->startClient();
        }

        qDebug() << "[UI] Session success:" << success;
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
            disconnectAction->setEnabled(true);
        }
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}
void NetworkToolbar::enableMessageSendingUI() {
    QWidget *msgWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(msgWidget);

    QLineEdit *inputField = new QLineEdit();
    QPushButton *sendButton = new QPushButton("Send");

    layout->addWidget(inputField);
    layout->addWidget(sendButton);
    msgWidget->setLayout(layout);

    // Add to your main window (assumes you have a layout to add to)
    mainWindowLayout->addWidget(msgWidget);  // Replace with your layout

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
    if (msgBox.exec() == QMessageBox::Yes) {
        // bool success = networkManager->stopSession();
        // showMessage("Session Stopped", success ? "Network session stopped successfully" : "Failed to stop session", !success);
        // if (success) {
        //     stopAction->setEnabled(false);
        //     disconnectAction->setEnabled(false);
        // }
    }
}

void NetworkToolbar::connectClient()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Connect to Client");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QComboBox *idCombo = new QComboBox();
    QLineEdit *ipEdit = new QLineEdit();
    QLineEdit *portEdit = new QLineEdit();
    QComboBox *protocolCombo = new QComboBox();
    QJsonArray clients = config["clients"].toArray();
    for (const QJsonValue &client : clients) {
        idCombo->addItem(client.toObject()["id"].toString());
    }
    QStringList protocols;
    QJsonArray protocolArray = config["protocols"].toArray();
    for (const QJsonValue &value : protocolArray) {
        protocols.append(value.toString());
    }
    protocolCombo->addItems(protocols);

    // Update IP and port when client ID changes
    auto updateFields = [=]() {
        for (const QJsonValue &client : clients) {
            if (client.toObject()["id"].toString() == idCombo->currentText()) {
                ipEdit->setText(client.toObject()["ip"].toString());
                portEdit->setText(QString::number(client.toObject()["port"].toInt()));
            }
        }
    };
    updateFields();
    connect(idCombo, &QComboBox::currentTextChanged, updateFields);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("Client ID:"));
    layout->addWidget(idCombo);
    layout->addWidget(new QLabel("IP Address:"));
    layout->addWidget(ipEdit);
    layout->addWidget(new QLabel("Port:"));
    layout->addWidget(portEdit);
    layout->addWidget(new QLabel("Protocol:"));
    layout->addWidget(protocolCombo);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        // bool success = networkManager->connectClient(idCombo->currentText(), ipEdit->text(), portEdit->text().toInt(), protocolCombo->currentText());
        // dialog.accept();
        // showMessage("Connection", success ? QString("Connected to %1 at %2:%3").arg(idCombo->currentText(), ipEdit->text(), portEdit->text())
        //                                   : "Failed to connect: Invalid IP", !success);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void NetworkToolbar::disconnectClient()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Disconnect Client");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QComboBox *clientCombo = new QComboBox();
    QJsonArray clients = config["clients"].toArray();
    for (const QJsonValue &client : clients) {
        clientCombo->addItem(client.toObject()["id"].toString());
    }
    QLabel *idLabel = new QLabel("Client ID: " + clientCombo->currentText());
    QLabel *ipLabel = new QLabel("IP Address: ");
    QLabel *portLabel = new QLabel("Port: ");
    QLabel *activityLabel = new QLabel("Last Activity: ");

    // Update labels when client ID changes
    auto updateLabels = [=]() {
        for (const QJsonValue &client : clients) {
            if (client.toObject()["id"].toString() == clientCombo->currentText()) {
                idLabel->setText("Client ID: " + client.toObject()["id"].toString());
                ipLabel->setText("IP Address: " + client.toObject()["ip"].toString());
                portLabel->setText("Port: " + QString::number(client.toObject()["port"].toInt()));
                activityLabel->setText("Last Activity: " + client.toObject()["last_activity"].toString());
            }
        }
    };
    updateLabels();
    connect(clientCombo, &QComboBox::currentTextChanged, updateLabels);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("Select Client:"));
    layout->addWidget(clientCombo);
    layout->addWidget(idLabel);
    layout->addWidget(ipLabel);
    layout->addWidget(portLabel);
    layout->addWidget(activityLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        // bool success = networkManager->disconnectClient(clientCombo->currentText());
        // dialog.accept();
        // showMessage("Disconnection", success ? QString("%1 disconnected").arg(clientCombo->currentText())
        //                                      : "Failed to disconnect: Network timeout", !success);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void NetworkToolbar::viewNetworkStatus()
{
    QDialog *dialog = new QDialog(this);
    setupDialog(dialog, "Network Status");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);

    QLabel *countLabel = new QLabel("Connected Clients: 0");
    QTableWidget *table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"Client ID", "IP", "Status", "Latency"});
    table->horizontalHeader()->setStretchLastSection(true);

    // Update status function
    auto updateStatus = [=]() {
        // QStringList status = networkManager->getNetworkStatus();
        // table->setRowCount(0);
        // int connectedCount = 0;
        // for (const QString &entry : status) {
        //     QStringList fields = entry.split(",");
        //     int row = table->rowCount();
        //     table->insertRow(row);
        //     table->setItem(row, 0, new QTableWidgetItem(fields[0]));
        //     table->setItem(row, 1, new QTableWidgetItem(fields[1]));
        //     table->setItem(row, 2, new QTableWidgetItem(fields[2]));
        //     table->setItem(row, 3, new QTableWidgetItem(fields[3]));
        //     if (fields[2] == "Connected") connectedCount++;
        // }
        // countLabel->setText(QString("Connected Clients: %1").arg(connectedCount));
    };

    // Initial update
    updateStatus();

    // Real-time updates
    QTimer *statusTimer = new QTimer(dialog);
    connect(statusTimer, &QTimer::timeout, updateStatus);
    statusTimer->start(1000);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Save);
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

    QPushButton *startButton = new QPushButton("Start Capture");
    QPushButton *stopButton = new QPushButton("Stop Capture");
    QPushButton *closeButton = new QPushButton("Close");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(closeButton);

    layout->addWidget(new QLabel("Filter:"));
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
    // QStringList packets = networkManager->getPackets();
    // int rowCount = packetTable->rowCount();

    // for (const QString &packet : packets) {
    //     QStringList fields = packet.split(",");
    //     packetTable->insertRow(rowCount);
    //     packetTable->setItem(rowCount, 0, new QTableWidgetItem(fields[0]));
    //     packetTable->setItem(rowCount, 1, new QTableWidgetItem(fields[1]));
    //     packetTable->setItem(rowCount, 2, new QTableWidgetItem(fields[2]));
    //     packetTable->setItem(rowCount, 3, new QTableWidgetItem(fields[3]));
    //     rowCount++;
    // }

    // packetTable->scrollToBottom();
}

void NetworkToolbar::runProtocolTest()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Run Protocol Test");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QComboBox *protocolCombo = new QComboBox();
    QStringList protocols;
    QJsonArray protocolArray = config["protocols"].toArray();
    for (const QJsonValue &value : protocolArray) {
        protocols.append(value.toString());
    }
    protocolCombo->addItems(protocols);

    QComboBox *testCombo = new QComboBox();
    QStringList testTypes;
    QJsonArray testArray = config["test_types"].toArray();
    for (const QJsonValue &value : testArray) {
        testTypes.append(value.toString());
    }
    testCombo->addItems(testTypes);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("Protocol:"));
    layout->addWidget(protocolCombo);
    layout->addWidget(new QLabel("Test Type:"));
    layout->addWidget(testCombo);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        // bool success = networkManager->runTest(protocolCombo->currentText(), testCombo->currentText());
        // dialog.accept();
        // showMessage("Test Results", success ? QString("%1 %2: Passed").arg(protocolCombo->currentText(), testCombo->currentText())
        //                                     : "Test Failed: Protocol mismatch", !success);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void NetworkToolbar::configureSyncSettings()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Synchronization Settings");

    // QVBoxLayout *layout = new QVBoxLayout(&dialog);
    // QComboBox *freqCombo = new QComboBox();
    // QStringList frequencies;
    // QJsonArray freqArray = config["heartbeat_frequencies"].toArray();
    // for (const QJsonValue &value : freqArray) {
    //     frequencies.append(value.toString());
    // }
    // freqCombo->addItems(frequencies);
    // QLineEdit *latencyEdit = new QLineEdit(QString::number(config["default_latency_threshold"].toInt()));
    // QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // layout->addWidget(new QLabel("Heartbeat Frequency:"));
    // layout->addWidget(freqCombo);
    // layout->addWidget(new QLabel("Latency Threshold (ms):"));
    // layout->addWidget(latencyEdit);
    // layout->addWidget(buttons);

    // connect(buttons, &QDialogButtonBox::accepted, [&]() {
    //     dialog.accept();
    //     showMessage("Settings Applied", "Sync settings updated successfully");
    // });
    // connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void NetworkToolbar::viewLogs()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Network Logs");

    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);

    QTextEdit *logText = new QTextEdit();
    logText->setReadOnly(true);
    logText->setText("2025-06-06 11:18: Client_01 connected\n2025-06-06 11:19: Client_02 disconnected");

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Save);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::clicked, [=](QAbstractButton *button) {
        if (buttons->buttonRole(button) == QDialogButtonBox::ActionRole) {
            // Export logic here
        }
    });

    layout->addWidget(logText);
    layout->addWidget(buttons);
    dialog.setLayout(layout);
    dialog.exec();
}

void NetworkToolbar::simulatePDU()
{
    QDialog dialog(this);
    setupDialog(&dialog, "Simulate PDU");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QComboBox *pduCombo = new QComboBox();
    QStringList pduTypes;
    QJsonArray pduArray = config["pdu_types"].toArray();
    for (const QJsonValue &value : pduArray) {
        pduTypes.append(value.toString());
    }
    pduCombo->addItems(pduTypes);
    QLineEdit *idEdit = new QLineEdit(config["default_pdu"].toObject()["id"].toString());
    QLineEdit *posEdit = new QLineEdit(config["default_pdu"].toObject()["position"].toString());
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    layout->addWidget(new QLabel("PDU Type:"));
    layout->addWidget(pduCombo);
    layout->addWidget(new QLabel("Entity ID:"));
    layout->addWidget(idEdit);
    layout->addWidget(new QLabel("Position:"));
    layout->addWidget(posEdit);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, [&]() {
        // bool success = networkManager->sendPDU(pduCombo->currentText(), idEdit->text(), posEdit->text());
        // dialog.accept();
        // showMessage("PDU Sent", success ? QString("%1 PDU sent successfully").arg(pduCombo->currentText())
        //                                 : "Failed to send PDU: Network error", !success);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}
