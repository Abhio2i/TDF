/* =============================================================================
 * FILE:         NetworkToolbar.cpp
 * MODULE:       DIS Network Toolbar
 * PROJECT:      Tactical Display Framework (TDF)
 * ORGANISATION: Oxygen 2 Innovation (O2I)
 * =============================================================================
 */

#include "networktoolbar.h"
#include "networktoolbar-styles.h"
#include "core/DISPlugin/config/disconfigloader.h"
#include <QHeaderView>
#include <QDebug>
#include <QFile>
#include <QScrollArea>
#include "core/DISPlugin/utils/dislogger.h"
#include <QListWidget>
#include <QTextEdit>
// =============================================================================
// Constructor
// =============================================================================
NetworkToolbar::NetworkToolbar(QWidget* parent)
    : QToolBar(parent)
{
    setObjectName("NetworkToolbar");
    setWindowTitle("DIS Network");
    setMovable(false);
    setStyleSheet(NetworkToolbarStyles::Toolbar);
    setIconSize(QSize(20, 20));
    createActions();
    // Status label shown in toolbar
    m_statusLabel = new QLabel("DIS: Offline", this);
    m_statusLabel->setStyleSheet("color: #C42B1C; font-weight: bold; "
                                 "font-size: 11px; padding: 0 8px;");
    addWidget(m_statusLabel);
    // Status update timer
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(1000);
    connect(m_statusTimer, &QTimer::timeout,
            this, &NetworkToolbar::updateStatusIndicator);
    m_statusTimer->start();
}

NetworkToolbar::~NetworkToolbar() {}

// =============================================================================
// setDISPlugin
// =============================================================================
// void NetworkToolbar::setDISPlugin(DISNetworkPlugin* plugin)
// {
//     m_disPlugin = plugin;
// }
void NetworkToolbar::setDISPlugin(DISNetworkPlugin* plugin)
{
    m_disPlugin = plugin;
    if (m_disPlugin) {
        connect(m_disPlugin, &DISNetworkPlugin::transferOwnershipRequested,
                this, &NetworkToolbar::onTransferOwnershipRequested);
    }
}
// =============================================================================
// onTransferOwnershipRequested
// Called when another machine sends a Transfer Ownership PDU for an entity
// we should decide on. Shows accept/reject prompt, then reports the
// decision back to the plugin, which sends the Acknowledge PDU.
// =============================================================================
void NetworkToolbar::onTransferOwnershipRequested(uint16_t originSite,
                                                  uint16_t originApp,
                                                  QString entityKey,
                                                  QString entityName)
{
    if (!m_disPlugin) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Incoming Transfer Request",
        QString("Machine Site=%1 App=%2 wants to transfer entity '%3' to you.\n\n"
                "Accept ownership?")
            .arg(originSite)
            .arg(originApp)
            .arg(entityName),
        QMessageBox::Yes | QMessageBox::No);

    m_disPlugin->respondToTransferOwnership(entityKey, reply == QMessageBox::Yes);
}
// =============================================================================
// createActions
// =============================================================================
void NetworkToolbar::onStatusClicked()
{
    onSettingsClicked();
}
void NetworkToolbar::createActions()
{
    // Connect
    m_connectAction = new QAction(QIcon(":/icons/images/networkicon.png"),
                                  tr("DIS Connect"), this);
    m_connectAction->setToolTip("Connect to DIS network");
    connect(m_connectAction, &QAction::triggered, this, &NetworkToolbar::onConnectClicked);
    addAction(m_connectAction);

    // Disconnect
    m_disconnectAction = new QAction(QIcon(":/icons/images/stop.png"),
                                     tr("DIS Disconnect"), this);
    m_disconnectAction->setToolTip("Disconnect from DIS network");
    m_disconnectAction->setEnabled(false);
    connect(m_disconnectAction, &QAction::triggered, this, &NetworkToolbar::onDisconnectClicked);
    addAction(m_disconnectAction);
    addSeparator();
    // Settings
    m_settingsAction = new QAction(QIcon(":/icons/images/status-update.png"),
                                   tr("DIS Settings"), this);
    m_settingsAction->setToolTip("Configure DIS network settings");
    connect(m_settingsAction, &QAction::triggered, this, &NetworkToolbar::onSettingsClicked);
    addAction(m_settingsAction);
    addSeparator();
}

// =============================================================================
// onConnectClicked
// Opens settings dialog if not configured, then connects
// =============================================================================
void NetworkToolbar::onConnectClicked()
{
    if (!m_disPlugin) return;
    if (m_disPlugin->isRunning()) {
        QMessageBox::information(this, "DIS", "Already connected.");
        return;
    }
    m_disPlugin->start("dis_config.json");
    m_connectAction->setEnabled(false);
    m_disconnectAction->setEnabled(true);
    qDebug() << "[NetworkToolbar] Connected";
}

// =============================================================================
// onDisconnectClicked
// =============================================================================
void NetworkToolbar::onDisconnectClicked()
{
    if (!m_disPlugin) return;
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("DIS Disconnect");
    msgBox.setText("Disconnect from DIS network?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setStyleSheet(NetworkToolbarStyles::Dialog);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_disPlugin->stop();
        m_connectAction->setEnabled(true);
        m_disconnectAction->setEnabled(false);
        qDebug() << "[NetworkToolbar] Disconnected";
    }
}

// =============================================================================
// onSettingsClicked
// Opens DIS Settings dialog with tabs matching STAGE 22.0
// =============================================================================
void NetworkToolbar::onSettingsClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("DIS / HLA Configuration");
   // dialog.setFixedSize(520, 560);
    dialog.setFixedSize(650, 640);
    dialog.setStyleSheet(NetworkToolbarStyles::Dialog);

    // Load current config
    DISConfig config = m_disPlugin
                           ? m_disPlugin->currentConfig()
                           : DISDefaultConfig();

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // ── Tab widget ────────────────────────────────────────────────────────────
    QTabWidget* tabs = new QTabWidget();
    tabs->addTab(createConnectionTab(config), "DIS");
    tabs->addTab(createOptionsTab(config),    "Options");
    tabs->addTab(createMessagesTab(config),          "Messages");
    tabs->addTab(createStatusTab(),            "Status");
    tabs->addTab(createTransferTab(), "Transfer");
    tabs->addTab(createDataExchangeTab(), "Data Exchange");
    mainLayout->addWidget(tabs);

    // ── Buttons ───────────────────────────────────────────────────────────────
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* applyBtn  = new QPushButton("Apply");
    QPushButton* okBtn     = new QPushButton("OK");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    applyBtn->setStyleSheet(NetworkToolbarStyles::ConnectButton);
    okBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    cancelBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    btnLayout->addStretch();
    btnLayout->addWidget(applyBtn);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    // ── Apply — save config ───────────────────────────────────────────────────
    connect(applyBtn, &QPushButton::clicked, [&]() {
        DISConfigLoader::save(config, "dis_config.json");
        if (m_disPlugin && m_disPlugin->isRunning())
            m_disPlugin->reloadConfig(config);
        QMessageBox::information(&dialog, "DIS", "Settings saved.");
    });
    connect(okBtn, &QPushButton::clicked, [&]() {
        DISConfigLoader::save(config, "dis_config.json");
        if (m_disPlugin && m_disPlugin->isRunning())
            m_disPlugin->reloadConfig(config);
        dialog.accept();
    });
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    dialog.exec();
}

// =============================================================================
// createConnectionTab
// DIS tab — matches STAGE Connection tab
// =============================================================================
QWidget* NetworkToolbar::createConnectionTab(DISConfig& config)
{
    QWidget* tab    = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(tab);

    // ── Enable Interface ──────────────────────────────────────────────────────
    QCheckBox* enableCheck = new QCheckBox("Enable Interface");
    enableCheck->setChecked(m_disPlugin && m_disPlugin->isRunning());
    vl->addWidget(enableCheck);

    connect(enableCheck, &QCheckBox::toggled, [this](bool checked) {
        if (!m_disPlugin) return;
        if (checked && !m_disPlugin->isRunning()) {
            m_disPlugin->start("dis_config.json");
            m_connectAction->setEnabled(false);
            m_disconnectAction->setEnabled(true);
        } else if (!checked && m_disPlugin->isRunning()) {
            m_disPlugin->stop();
            m_connectAction->setEnabled(true);
            m_disconnectAction->setEnabled(false);
        }
    });

    // ── Standard Connection Properties ────────────────────────────────────────
    QGroupBox* connGroup = new QGroupBox("Standard Connection Properties");
    QFormLayout* form    = new QFormLayout(connGroup);

    // QComboBox* modeCombo = new QComboBox();
    // modeCombo->addItems({"Multicast", "Unicast"});
    // modeCombo->setCurrentIndex(
    //     config.connectionMode == "Unicast" ? 1 : 0);
    QComboBox* modeCombo = new QComboBox();
    modeCombo->addItems({"Multicast", "Unicast", "Broadcast"});
    int modeIndex = 0;
    if (config.connectionMode == "Unicast")   modeIndex = 1;
    if (config.connectionMode == "Broadcast") modeIndex = 2;
    modeCombo->setCurrentIndex(modeIndex);
    form->addRow("Connection Mode:", modeCombo);

    QLineEdit* multicastEdit = new QLineEdit(
        QString::fromStdString(config.multicastGroup));
    form->addRow("Multicast Address:", multicastEdit);
    connect(multicastEdit, &QLineEdit::textChanged, [&config](const QString& t) {
        config.multicastGroup = t.toStdString();
    });

    // QSpinBox* portSpin = new QSpinBox();
    // portSpin->setRange(1024, 65535);
    // portSpin->setValue(config.port);
    // form->addRow("Communication Port:", portSpin);
    // connect(portSpin, QOverload<int>::of(&QSpinBox::valueChanged),
    //         [&config](int v) { config.port = static_cast<uint16_t>(v); });
    QSpinBox* portSpin = new QSpinBox();
    portSpin->setRange(1024, 65535);
    portSpin->setValue(config.port);
    QLabel* portLabel = new QLabel(
        config.connectionMode == "Unicast" ? "Local Listen Port:" : "Communication Port:");
    form->addRow(portLabel, portSpin);
    connect(portSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.port = static_cast<uint16_t>(v); });

    vl->addWidget(connGroup);

    // ── Unicast Peers — only visible when Unicast mode selected ───────────────
    QGroupBox* peerGroup = new QGroupBox("Unicast Peers");
    QVBoxLayout* peerVl  = new QVBoxLayout(peerGroup);

    // Scrollable area for peer rows
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setMaximumHeight(150);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* peerListWidget = new QWidget();
    QVBoxLayout* peerListLayout = new QVBoxLayout(peerListWidget);
    peerListLayout->setSpacing(4);
    peerListLayout->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidget(peerListWidget);

    // Helper to add one peer row
    auto addPeerRow = [&](const QString& ip, uint16_t port) {
        QWidget* row     = new QWidget();
        QHBoxLayout* hl  = new QHBoxLayout(row);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(6);

        QLineEdit* ipEdit   = new QLineEdit(ip);
        QLineEdit* portEdit = new QLineEdit(QString::number(port));
        ipEdit->setPlaceholderText("IP Address e.g. 192.168.1.101");
        portEdit->setFixedWidth(60);
        portEdit->setPlaceholderText("Port");

        QPushButton* removeBtn = new QPushButton("✕");
        removeBtn->setFixedWidth(28);
        removeBtn->setFixedHeight(28);
        removeBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);

        hl->addWidget(ipEdit);
        hl->addWidget(portEdit);
        hl->addWidget(removeBtn);

        peerListLayout->addWidget(row);

        // Sync all rows back to config on any change
        auto syncPeers = [peerListLayout, peerListWidget, &config]() {
            config.unicastPeers.clear();
            for (int i = 0; i < peerListLayout->count(); ++i) {
                QWidget* r = peerListLayout->itemAt(i)->widget();
                if (!r) continue;
                QHBoxLayout* hl2 = qobject_cast<QHBoxLayout*>(r->layout());
                if (!hl2) continue;
                QLineEdit* ip2   = qobject_cast<QLineEdit*>(hl2->itemAt(0)->widget());
                QLineEdit* port2 = qobject_cast<QLineEdit*>(hl2->itemAt(1)->widget());
                if (!ip2 || !port2) continue;
                QString ipStr = ip2->text().trimmed();
                if (ipStr.isEmpty() || ipStr == "IP Address e.g. 192.168.1.101")
                    continue;
                DISConfig::UnicastPeer p;
                p.ip   = ipStr.toStdString();
                p.port = static_cast<uint16_t>(port2->text().toUShort());
                if (p.port == 0) p.port = 3000;
                config.unicastPeers.push_back(p);
            }
        };

        connect(ipEdit,   &QLineEdit::textChanged, syncPeers);
        connect(portEdit, &QLineEdit::textChanged, syncPeers);

        connect(removeBtn, &QPushButton::clicked, [row, peerListLayout, syncPeers]() {
            peerListLayout->removeWidget(row);
            row->deleteLater();
            syncPeers();
        });
    };

    // Populate from config
    for (const DISConfig::UnicastPeer& p : config.unicastPeers)
        addPeerRow(QString::fromStdString(p.ip), p.port);

    peerVl->addWidget(scrollArea);

    QPushButton* addPeerBtn = new QPushButton("+ Add Peer");
    addPeerBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    connect(addPeerBtn, &QPushButton::clicked,
            [peerListLayout, &config]() {
                QWidget* row    = new QWidget();
                QHBoxLayout* hl = new QHBoxLayout(row);
                hl->setContentsMargins(0, 0, 0, 0);
                hl->setSpacing(6);

                QLineEdit* ipEdit   = new QLineEdit();
                QLineEdit* portEdit = new QLineEdit("3000");
                ipEdit->setPlaceholderText("IP Address e.g. 192.168.1.101");
                portEdit->setFixedWidth(60);

                QPushButton* removeBtn = new QPushButton("✕");
                removeBtn->setFixedWidth(28);
                removeBtn->setFixedHeight(28);

                hl->addWidget(ipEdit);
                hl->addWidget(portEdit);
                hl->addWidget(removeBtn);

                peerListLayout->addWidget(row);

                auto syncPeers = [peerListLayout, &config]() {
                    config.unicastPeers.clear();
                    for (int i = 0; i < peerListLayout->count(); ++i) {
                        QWidget* r = peerListLayout->itemAt(i)->widget();
                        if (!r) continue;
                        QHBoxLayout* hl2 = qobject_cast<QHBoxLayout*>(r->layout());
                        if (!hl2) continue;
                        QLineEdit* ip2   = qobject_cast<QLineEdit*>(hl2->itemAt(0)->widget());
                        QLineEdit* port2 = qobject_cast<QLineEdit*>(hl2->itemAt(1)->widget());
                        if (!ip2 || !port2) continue;
                        QString ipStr = ip2->text().trimmed();
                        if (ipStr.isEmpty()) continue;
                        DISConfig::UnicastPeer p;
                        p.ip   = ipStr.toStdString();
                        p.port = static_cast<uint16_t>(port2->text().toUShort());
                        if (p.port == 0) p.port = 3000;
                        config.unicastPeers.push_back(p);
                    }
                };

                connect(ipEdit,   &QLineEdit::textChanged, syncPeers);
                connect(portEdit, &QLineEdit::textChanged, syncPeers);
                connect(removeBtn, &QPushButton::clicked,
                        [row, peerListLayout, syncPeers]() {
                            peerListLayout->removeWidget(row);
                            row->deleteLater();
                            syncPeers();
                        });
            });

    peerVl->addWidget(addPeerBtn);
    vl->addWidget(peerGroup);

    // Show peer group only when Unicast is selected
    peerGroup->setVisible(config.connectionMode == "Unicast");
    // connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //         [&config, peerGroup](int idx) {
    //             config.connectionMode = (idx == 1) ? "Unicast" : "Multicast";
    //             peerGroup->setVisible(idx == 1);
    //         });
    // connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //         [&config, peerGroup](int idx) {
    //             if (idx == 1) config.connectionMode = "Unicast";
    //             else if (idx == 2) config.connectionMode = "Broadcast";
    //             else config.connectionMode = "Multicast";
    //             peerGroup->setVisible(idx == 1);
    //         });
    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&config, peerGroup, portLabel](int idx) {
                if (idx == 1) config.connectionMode = "Unicast";
                else if (idx == 2) config.connectionMode = "Broadcast";
                else config.connectionMode = "Multicast";
                peerGroup->setVisible(idx == 1);
                portLabel->setText(idx == 1 ? "Local Listen Port:" : "Communication Port:");
            });

    // ── Advanced Connection Properties ────────────────────────────────────────
    QGroupBox* advGroup  = new QGroupBox("Advanced Connection Properties");
    QFormLayout* advForm = new QFormLayout(advGroup);

    // QLineEdit* ifaceEdit = new QLineEdit("(auto)");
    // advForm->addRow("Interface Address:", ifaceEdit);
    // Show current networkInterface from config, empty = auto-detect
    QString ifaceDisplay = config.networkInterface.empty()
                               ? ""
                               : QString::fromStdString(config.networkInterface);
    QLineEdit* ifaceEdit = new QLineEdit(ifaceDisplay);
    ifaceEdit->setPlaceholderText("e.g. 192.168.1.2  (empty = auto-detect)");
    advForm->addRow("Interface Address:", ifaceEdit);
    connect(ifaceEdit, &QLineEdit::textChanged, [&config](const QString& t) {
        config.networkInterface = t.trimmed().toStdString();
    });

    QSpinBox* bufSpin = new QSpinBox();
    bufSpin->setRange(1024, 1048576);
    bufSpin->setValue(config.receiveBufferSize);
    bufSpin->setSuffix(" bytes");
    advForm->addRow("Receive Buffer Size:", bufSpin);
    connect(bufSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.receiveBufferSize = v; });

    QSpinBox* ttlSpin = new QSpinBox();
    ttlSpin->setRange(1, 255);
    ttlSpin->setValue(config.ttl);
    advForm->addRow("TTL (Time To Live):", ttlSpin);
    connect(ttlSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.ttl = static_cast<uint8_t>(v); });

    vl->addWidget(advGroup);
    vl->addStretch();

    return tab;
}
// QWidget* NetworkToolbar::createConnectionTab(DISConfig& config)
// {
//     QWidget* tab    = new QWidget();
//     QVBoxLayout* vl = new QVBoxLayout(tab);

//     // ── Enable Interface ──────────────────────────────────────────────────────
//     QCheckBox* enableCheck = new QCheckBox("Enable Interface");
//     enableCheck->setChecked(m_disPlugin && m_disPlugin->isRunning());
//     vl->addWidget(enableCheck);

//     connect(enableCheck, &QCheckBox::toggled, [this](bool checked) {
//         if (!m_disPlugin) return;
//         if (checked && !m_disPlugin->isRunning()) {
//             m_disPlugin->start("dis_config.json");
//             m_connectAction->setEnabled(false);
//             m_disconnectAction->setEnabled(true);
//         } else if (!checked && m_disPlugin->isRunning()) {
//             m_disPlugin->stop();
//             m_connectAction->setEnabled(true);
//             m_disconnectAction->setEnabled(false);
//         }
//     });

//     // ── Connection Mode ───────────────────────────────────────────────────────
//     QGroupBox* connGroup = new QGroupBox("Standard Connection Properties");
//     QFormLayout* form    = new QFormLayout(connGroup);

//     QComboBox* modeCombo = new QComboBox();
//     modeCombo->addItems({"Multicast", "Unicast"});
//     modeCombo->setCurrentIndex(
//         config.connectionMode == "Unicast" ? 1 : 0);
//     form->addRow("Connection Mode:", modeCombo);
//     connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             [&config](int idx) {
//                 config.connectionMode = (idx == 1) ? "Unicast" : "Multicast";
//             });

//     QLineEdit* multicastEdit = new QLineEdit(
//         QString::fromStdString(config.multicastGroup));
//     form->addRow("Multicast Address:", multicastEdit);
//     connect(multicastEdit, &QLineEdit::textChanged, [&config](const QString& t) {
//         config.multicastGroup = t.toStdString();
//     });

//     QSpinBox* portSpin = new QSpinBox();
//     portSpin->setRange(1024, 65535);
//     portSpin->setValue(config.port);
//     form->addRow("Communication Port:", portSpin);
//     connect(portSpin, QOverload<int>::of(&QSpinBox::valueChanged),
//             [&config](int v) { config.port = static_cast<uint16_t>(v); });
//     // ── Unicast peer list ─────────────────────────────────────────────────────
//     QGroupBox* peerGroup  = new QGroupBox("Unicast Peers (used when mode is Unicast)");
//     QVBoxLayout* peerVl   = new QVBoxLayout(peerGroup);

//     QTableWidget* peerTable = new QTableWidget(0, 2);
//     peerTable->setHorizontalHeaderLabels({"IP Address", "Port"});
//     peerTable->horizontalHeader()->setStretchLastSection(false);
//     peerTable->horizontalHeader()->setSectionResizeMode(
//         0, QHeaderView::Stretch);
//     peerTable->horizontalHeader()->setSectionResizeMode(
//         1, QHeaderView::Fixed);
//     peerTable->setColumnWidth(1, 70);
//     peerTable->verticalHeader()->setVisible(false);
//     peerTable->setMaximumHeight(130);

//     // Populate existing peers from config
//     for (const DISConfig::UnicastPeer& p : config.unicastPeers) {
//         int row = peerTable->rowCount();
//         peerTable->insertRow(row);
//         peerTable->setItem(row, 0,
//                            new QTableWidgetItem(QString::fromStdString(p.ip)));
//         peerTable->setItem(row, 1,
//                            new QTableWidgetItem(QString::number(p.port)));
//     }

//     QHBoxLayout* peerBtnLayout = new QHBoxLayout();
//     QPushButton* addPeerBtn    = new QPushButton("+ Add Peer");
//     QPushButton* removePeerBtn = new QPushButton("- Remove");
//     addPeerBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
//     removePeerBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
//     peerBtnLayout->addWidget(addPeerBtn);
//     peerBtnLayout->addWidget(removePeerBtn);
//     peerBtnLayout->addStretch();

//     connect(addPeerBtn, &QPushButton::clicked, [peerTable]() {
//         int row = peerTable->rowCount();
//         peerTable->insertRow(row);
//         peerTable->setItem(row, 0, new QTableWidgetItem("192.168.1.x"));
//         peerTable->setItem(row, 1, new QTableWidgetItem("3000"));
//     });

//     connect(removePeerBtn, &QPushButton::clicked, [peerTable]() {
//         int row = peerTable->currentRow();
//         if (row >= 0) peerTable->removeRow(row);
//     });

//     // Sync table back to config whenever a cell changes
//     connect(peerTable, &QTableWidget::cellChanged,
//             [peerTable, &config](int, int) {
//                 config.unicastPeers.clear();
//                 for (int r = 0; r < peerTable->rowCount(); ++r) {
//                     QTableWidgetItem* ipItem   = peerTable->item(r, 0);
//                     QTableWidgetItem* portItem = peerTable->item(r, 1);
//                     if (!ipItem || !portItem) continue;
//                     DISConfig::UnicastPeer p;
//                     p.ip   = ipItem->text().toStdString();
//                     p.port = static_cast<uint16_t>(
//                         portItem->text().toUShort());
//                     if (!p.ip.empty())
//                         config.unicastPeers.push_back(p);
//                 }
//             });

//     peerVl->addWidget(peerTable);
//     peerVl->addLayout(peerBtnLayout);
//     vl->addWidget(peerGroup);

//     vl->addWidget(connGroup);

//     // ── Advanced ─────────────────────────────────────────────────────────────
//     QGroupBox* advGroup = new QGroupBox("Advanced Connection Properties");
//     QFormLayout* advForm = new QFormLayout(advGroup);

//     QLineEdit* ifaceEdit = new QLineEdit("(auto)");
//     advForm->addRow("Interface Address:", ifaceEdit);

//     QSpinBox* bufSpin = new QSpinBox();
//     bufSpin->setRange(1024, 1048576);
//     bufSpin->setValue(config.receiveBufferSize);
//     bufSpin->setSuffix(" bytes");
//     advForm->addRow("Receive Buffer Size:", bufSpin);
//     connect(bufSpin, QOverload<int>::of(&QSpinBox::valueChanged),
//             [&config](int v) { config.receiveBufferSize = v; });

//     QSpinBox* ttlSpin = new QSpinBox();
//     ttlSpin->setRange(1, 255);
//     ttlSpin->setValue(config.ttl);
//     advForm->addRow("TTL (Time To Live):", ttlSpin);
//     connect(ttlSpin, QOverload<int>::of(&QSpinBox::valueChanged),
//             [&config](int v) { config.ttl = static_cast<uint8_t>(v); });

//     vl->addWidget(advGroup);
//     vl->addStretch();

//     return tab;
// }

// =============================================================================
// createOptionsTab
// Options tab — Exercise ID, Site ID, App ID, DIS Version
// =============================================================================
QWidget* NetworkToolbar::createOptionsTab(DISConfig& config)
{
    QWidget* tab    = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(tab);

    QGroupBox* disGroup = new QGroupBox("DIS Parameters");
    QFormLayout* form   = new QFormLayout(disGroup);

    // DIS Version
    QComboBox* versionCombo = new QComboBox();
    //   versionCombo->addItems({"DIS Version 6 (IEEE 1278.1-1995)",
    //                           "DIS Version 7 (IEEE 1278.1-2012)"});
    //   versionCombo->setCurrentIndex(config.defaultVersion == 7 ? 1 : 0);
    //   form->addRow("DIS Version:", versionCombo);
    //   connect(versionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //           [&config](int idx) {
    //               config.defaultVersion = static_cast<uint8_t>(idx == 1 ? 7 : 6);
    //           });
    versionCombo->addItems({"DIS Version 5 (IEEE 1278.1-1995)",
                            "DIS Version 6 (IEEE 1278.1-1998)",
                            "DIS Version 7 (IEEE 1278.1-2012)"});
    int versionIndex = 2;
    if (config.defaultVersion == 5) versionIndex = 0;
    if (config.defaultVersion == 6) versionIndex = 1;
    versionCombo->setCurrentIndex(versionIndex);
    form->addRow("DIS Version:", versionCombo);
    connect(versionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&config](int idx) {
                const uint8_t versions[] = {5, 6, 7};
                config.defaultVersion = versions[idx];
            });
    QSpinBox* exerciseSpin = new QSpinBox();
    exerciseSpin->setRange(1, 255);
    exerciseSpin->setValue(config.exerciseID);
    form->addRow("Exercise ID:", exerciseSpin);
    connect(exerciseSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.exerciseID = static_cast<uint8_t>(v); });

    // Site ID
    QSpinBox* siteSpin = new QSpinBox();
    siteSpin->setRange(1, 65535);
    siteSpin->setValue(config.siteID);
    form->addRow("Site ID:", siteSpin);
    // connect(siteSpin, QOverload<int>::of(&QSpinBox::valueChanged),
    //         [&config](int v) { config.siteID = static_cast<uint8_t>(v); });
    connect(siteSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.siteID = static_cast<uint16_t>(v); });

    // Application ID
    QSpinBox* appSpin = new QSpinBox();
    appSpin->setRange(1, 65535);
    appSpin->setValue(config.applicationID);
    form->addRow("Application ID:", appSpin);
    // connect(appSpin, QOverload<int>::of(&QSpinBox::valueChanged),
    //         [&config](int v) { config.applicationID = static_cast<uint8_t>(v); });
    connect(appSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.applicationID = static_cast<uint16_t>(v); });
    // Country Code
    QSpinBox* countrySpin = new QSpinBox();
    countrySpin->setRange(0, 65535);
    countrySpin->setValue(config.countryCode);
    countrySpin->setToolTip("164 = India (SISO-REF-010)");
    form->addRow("Country Code:", countrySpin);
    connect(countrySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [&config](int v) { config.countryCode = static_cast<uint16_t>(v); });

    vl->addWidget(disGroup);

    // ── Dead Reckoning ────────────────────────────────────────────────────────
    QGroupBox* drGroup = new QGroupBox("Dead Reckoning Thresholds");
    QFormLayout* drForm = new QFormLayout(drGroup);

    QDoubleSpinBox* posSpin = new QDoubleSpinBox();
    posSpin->setRange(0.1, 100.0);
    posSpin->setValue(static_cast<double>(config.positionThresholdMeters));
    posSpin->setSuffix(" m");
    drForm->addRow("Position Threshold:", posSpin);
    connect(posSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [&config](double v) {
                config.positionThresholdMeters = static_cast<float>(v);
            });

    QDoubleSpinBox* hbSpin = new QDoubleSpinBox();
    hbSpin->setRange(1.0, 30.0);
    hbSpin->setValue(static_cast<double>(config.heartbeatIntervalSeconds));
    hbSpin->setSuffix(" s");
    drForm->addRow("Heartbeat Interval:", hbSpin);
    connect(hbSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [&config](double v) {
                config.heartbeatIntervalSeconds = static_cast<float>(v);
            });

    vl->addWidget(drGroup);
    // ── Log Level ─────────────────────────────────────────────────────────────
    QGroupBox* logGroup  = new QGroupBox("Logging");
    QFormLayout* logForm = new QFormLayout(logGroup);

    QComboBox* logCombo = new QComboBox();
    logCombo->addItems({"None", "Error", "Warning", "Basic", "Full"});

    // Set current index from config
    QString currentLevel = QString::fromStdString(config.globalTraceLevel);
    int logIndex = 3; // default Basic
    if      (currentLevel == "None")    logIndex = 0;
    else if (currentLevel == "Error")   logIndex = 1;
    else if (currentLevel == "Warning") logIndex = 2;
    else if (currentLevel == "Basic")   logIndex = 3;
    else if (currentLevel == "Full")    logIndex = 4;
    logCombo->setCurrentIndex(logIndex);

    logForm->addRow("Log Level:", logCombo);
    vl->addWidget(logGroup);

    connect(logCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&config](int idx) {
                const char* levels[] = {"None","Error","Warning","Basic","Full"};
                config.globalTraceLevel = levels[idx];
                // Apply immediately — no restart needed
                DISLogger::instance().setLevelFromString(config.globalTraceLevel);
            });

    // ── Ignore other versions ─────────────────────────────────────────────────
    QCheckBox* ignoreCheck = new QCheckBox("Ignore other DIS versions");
    ignoreCheck->setChecked(config.ignoreOtherVersions);
    connect(ignoreCheck, &QCheckBox::toggled,
            [&config](bool v) { config.ignoreOtherVersions = v; });
    vl->addWidget(ignoreCheck);
    vl->addStretch();
    return tab;
}

// =============================================================================
// createMessagesTab
// Messages tab — which PDUs to send/receive (matches STAGE Messages tab)
// =============================================================================
QWidget* NetworkToolbar::createMessagesTab(DISConfig& config)
{
    QWidget* tab    = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(tab);
    QLabel* infoLabel = new QLabel(
        "Select which PDU types to send and receive:");
    infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
    vl->addWidget(infoLabel);
    QTableWidget* table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels({"PDU Type", "Send", "Receive"});
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    table->setColumnWidth(1, 60);
    table->setColumnWidth(2, 70);
    table->verticalHeader()->setVisible(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    // Populate rows from config.pduConfigs — not hardcoded values
    for (int i = 0; i < static_cast<int>(config.pduConfigs.size()); ++i) {
        int row = table->rowCount();
        table->insertRow(row);
        // PDU name from config
        table->setItem(row, 0, new QTableWidgetItem(
                                   QString::fromStdString(config.pduConfigs[i].pduName)));
        table->item(row, 0)->setFlags(Qt::ItemIsEnabled);
        // Send checkbox — reads from config, writes back to config on toggle
        QCheckBox* sendCheck = new QCheckBox();
        sendCheck->setChecked(config.pduConfigs[i].send);
        QWidget* sendWidget = new QWidget();
        QHBoxLayout* sl = new QHBoxLayout(sendWidget);
        sl->addWidget(sendCheck);
        sl->setAlignment(Qt::AlignCenter);
        sl->setContentsMargins(0, 0, 0, 0);
        table->setCellWidget(row, 1, sendWidget);

        connect(sendCheck, &QCheckBox::toggled,
                [&config, i](bool checked) {
                    config.pduConfigs[i].send = checked;
                });

        // Receive checkbox — reads from config, writes back to config on toggle
        QCheckBox* recvCheck = new QCheckBox();
        recvCheck->setChecked(config.pduConfigs[i].receive);
        QWidget* recvWidget = new QWidget();
        QHBoxLayout* rl = new QHBoxLayout(recvWidget);
        rl->addWidget(recvCheck);
        rl->setAlignment(Qt::AlignCenter);
        rl->setContentsMargins(0, 0, 0, 0);
        table->setCellWidget(row, 2, recvWidget);

        connect(recvCheck, &QCheckBox::toggled,
                [&config, i](bool checked) {
                    config.pduConfigs[i].receive = checked;
                });
    }

    vl->addWidget(table);
    return tab;
}
// =============================================================================
// createTransferTab
// Allows user to transfer ownership of one or more locally owned entities
// to another machine. Supports multi-select with Select All / Deselect All.
// Sends one Transfer Ownership PDU (type 35) per selected entity.
// IEEE 1278.1 — one PDU per entity, sequential send, auto-accept on receiver.
// =============================================================================
QWidget* NetworkToolbar::createTransferTab()
{
    QWidget*     tab = new QWidget();
    QVBoxLayout* vl  = new QVBoxLayout(tab);

    QLabel* infoLabel = new QLabel(
        "Select one or more locally owned entities to transfer to another machine.\n"
        "One Transfer Ownership PDU is sent per entity (IEEE 1278.1).");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
    vl->addWidget(infoLabel);

    // ── Entity selection — multi-select list with checkboxes ──────────────────
    QGroupBox*   entityGroup = new QGroupBox("Entities to Transfer");
    QVBoxLayout* entityVl    = new QVBoxLayout(entityGroup);

    QListWidget* entityList = new QListWidget();
    entityList->setSelectionMode(QAbstractItemView::NoSelection);
    entityList->setMaximumHeight(160);

    if (m_disPlugin && m_disPlugin->isRunning()) {
        QList<QPair<QString,QString>> entities = m_disPlugin->getLocalEntityList();
        if (entities.isEmpty()) {
            QListWidgetItem* item = new QListWidgetItem("No local entities");
            item->setFlags(Qt::ItemIsEnabled);  // not checkable
            entityList->addItem(item);
        } else {
            for (const auto& pair : entities) {
                QListWidgetItem* item = new QListWidgetItem(
                    pair.second + "  [" + pair.first + "]");
                item->setData(Qt::UserRole, pair.first);  // store engineID
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
                entityList->addItem(item);
            }
        }
    } else {
        QListWidgetItem* item = new QListWidgetItem("Connect DIS first");
        item->setFlags(Qt::ItemIsEnabled);
        entityList->addItem(item);
    }
    entityVl->addWidget(entityList);
    // ── Select All / Deselect All buttons ─────────────────────────────────────
    QHBoxLayout* selectBtnLayout = new QHBoxLayout();
    QPushButton* selectAllBtn   = new QPushButton("Select All");
    QPushButton* deselectAllBtn = new QPushButton("Deselect All");
    QPushButton* refreshBtn = new QPushButton("Refresh List");
    refreshBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    selectBtnLayout->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, [this, entityList]() {
        entityList->clear();
        if (m_disPlugin && m_disPlugin->isRunning()) {
            QList<QPair<QString,QString>> entities = m_disPlugin->getLocalEntityList();
            if (entities.isEmpty()) {
                QListWidgetItem* item = new QListWidgetItem("No local entities");
                item->setFlags(Qt::ItemIsEnabled);
                entityList->addItem(item);
            } else {
                for (const auto& pair : entities) {
                    QListWidgetItem* item = new QListWidgetItem(
                        pair.second + "  [" + pair.first + "]");
                    item->setData(Qt::UserRole, pair.first);
                    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                    item->setCheckState(Qt::Unchecked);
                    entityList->addItem(item);
                }
            }
        }
    });
    selectAllBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    deselectAllBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    selectBtnLayout->addWidget(selectAllBtn);
    selectBtnLayout->addWidget(deselectAllBtn);
    selectBtnLayout->addStretch();
    connect(selectAllBtn, &QPushButton::clicked, [entityList]() {
        for (int i = 0; i < entityList->count(); ++i) {
            QListWidgetItem* item = entityList->item(i);
            if (item->flags() & Qt::ItemIsUserCheckable)
                item->setCheckState(Qt::Checked);
        }
    });
    connect(deselectAllBtn, &QPushButton::clicked, [entityList]() {
        for (int i = 0; i < entityList->count(); ++i) {
            QListWidgetItem* item = entityList->item(i);
            if (item->flags() & Qt::ItemIsUserCheckable)
                item->setCheckState(Qt::Unchecked);
        }
    });
    entityVl->addLayout(selectBtnLayout);
    vl->addWidget(entityGroup);

    // ── Target machine ────────────────────────────────────────────────────────
    QGroupBox*   targetGroup = new QGroupBox("Target Machine");
    QFormLayout* targetForm  = new QFormLayout(targetGroup);

    QSpinBox* siteSpinTarget = new QSpinBox();
    siteSpinTarget->setRange(1, 65535);
    siteSpinTarget->setValue(1);
    siteSpinTarget->setToolTip("Site ID of the receiving machine");
    targetForm->addRow("Target Site ID:", siteSpinTarget);

    QSpinBox* appSpinTarget = new QSpinBox();
    appSpinTarget->setRange(1, 65535);
    appSpinTarget->setValue(2);
    appSpinTarget->setToolTip("Application ID of the receiving machine");
    targetForm->addRow("Target App ID:", appSpinTarget);

    vl->addWidget(targetGroup);

    // ── Transfer button ───────────────────────────────────────────────────────
    QPushButton* transferBtn = new QPushButton("Transfer Selected Entities");
    transferBtn->setStyleSheet(NetworkToolbarStyles::ConnectButton);

    connect(transferBtn, &QPushButton::clicked,
            [this, entityList, siteSpinTarget, appSpinTarget]() {

                if (!m_disPlugin || !m_disPlugin->isRunning()) {
                    QMessageBox::warning(nullptr, "DIS", "DIS not connected.");
                    return;
                }

                // Collect checked entities
                QList<QPair<QString,QString>> selected;
                for (int i = 0; i < entityList->count(); ++i) {
                    QListWidgetItem* item = entityList->item(i);
                    if ((item->flags() & Qt::ItemIsUserCheckable) &&
                        item->checkState() == Qt::Checked) {
                        QString engineID    = item->data(Qt::UserRole).toString();
                        QString displayName = item->text();
                        if (!engineID.isEmpty())
                            selected.append({engineID, displayName});
                    }
                }

                if (selected.isEmpty()) {
                    QMessageBox::warning(nullptr, "DIS",
                                         "Select at least one entity to transfer.");
                    return;
                }

                uint16_t targetSite = static_cast<uint16_t>(siteSpinTarget->value());
                uint16_t targetApp  = static_cast<uint16_t>(appSpinTarget->value());

                // Confirm with summary of selected entities
                QString entityNames;
                for (const auto& pair : selected)
                    entityNames += "\n  • " + pair.second;

                QMessageBox::StandardButton reply = QMessageBox::question(
                    nullptr, "Confirm Transfer",
                    QString("Transfer %1 entity/entities to machine Site=%2 App=%3?%4\n\n"
                            "The target machine will take control of all selected entities.")
                        .arg(selected.size())
                        .arg(targetSite)
                        .arg(targetApp)
                        .arg(entityNames),
                    QMessageBox::Yes | QMessageBox::No);

                if (reply != QMessageBox::Yes) return;

                // ── Send one Transfer Ownership PDU per selected entity ────────────────
                // IEEE 1278.1: each entity transfer is a separate PDU + Acknowledge pair
                int sentCount = 0;
                for (const auto& pair : selected) {
                    m_disPlugin->transferEntityOwnership(pair.first, targetSite, targetApp);
                    sentCount++;
                }

                QMessageBox::information(nullptr, "DIS",
                                         QString("%1 transfer request(s) sent.\n"
                                                 "Waiting for acknowledgements from target machine.")
                                             .arg(sentCount));
            });

    vl->addWidget(transferBtn);

    QLabel* noteLabel = new QLabel(
        "IEEE 1278.1 Transfer Ownership PDU (type 35).\n"
        "One PDU sent per entity. Target must be connected and running TDF.\n"
        "On success: entity ownership moves automatically on all machines.");
    noteLabel->setWordWrap(true);
    noteLabel->setStyleSheet("color: #666666; font-size: 10px;");
    vl->addWidget(noteLabel);

    vl->addStretch();
    return tab;
}
// =============================================================================
// createDataExchangeTab
// Manual test-trigger UI for Data Query (18) / Set Data (19) / Data (20)
// PDUs — IEEE 1278.1 §5.3.6.8-10.
// =============================================================================
QWidget* NetworkToolbar::createDataExchangeTab()
{
    QWidget*     tab = new QWidget();
    QVBoxLayout* vl  = new QVBoxLayout(tab);
    vl->setSpacing(10);

    QLabel* infoLabel = new QLabel(
        "Send a real Data Query or Set Data PDU (IEEE 1278.1 §5.3.6.8/9) "
        "and view the Data PDU response (§5.3.6.10).");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
    vl->addWidget(infoLabel);

    // ── Participants ─────────────────────────────────────────────────────
    QGroupBox*   participantsGroup = new QGroupBox("Query Participants");
    QFormLayout* participantsForm  = new QFormLayout(participantsGroup);

    QComboBox* fromCombo = new QComboBox();
    QComboBox* toCombo   = new QComboBox();
    fromCombo->setEditable(true);
    toCombo->setEditable(true);

    auto populateParticipants = [this, fromCombo, toCombo]() {
        fromCombo->clear();
        toCombo->clear();

        if (!m_disPlugin || !m_disPlugin->isRunning()) {
            fromCombo->addItem("Connect DIS first", "");
            toCombo->addItem("Connect DIS first", "");
            return;
        }

        QList<QPair<QString,QString>> localEntities  = m_disPlugin->getLocalEntityList();
        QList<QPair<QString,QString>> remoteEntities = m_disPlugin->getRemoteEntityList();

        for (const auto& pair : localEntities)
            fromCombo->addItem(pair.second + "  [" + pair.first + "]", pair.first);

        for (const auto& pair : localEntities)
            toCombo->addItem("[LOCAL] " + pair.second + "  [" + pair.first + "]", pair.first);
        for (const auto& pair : remoteEntities)
            toCombo->addItem("[REMOTE] " + pair.second + "  [" + pair.first + "]", pair.first);

        if (fromCombo->count() == 0) fromCombo->addItem("No local entities", "");
        if (toCombo->count() == 0)   toCombo->addItem("No known entities", "");
    };
    populateParticipants();

    participantsForm->addRow("From (your entity):", fromCombo);
    participantsForm->addRow("To (target entity):", toCombo);

    QPushButton* refreshBtn = new QPushButton("Refresh Lists");
    refreshBtn->setStyleSheet(NetworkToolbarStyles::ButtonBox);
    connect(refreshBtn, &QPushButton::clicked, populateParticipants);
    participantsForm->addRow("", refreshBtn);

    vl->addWidget(participantsGroup);

    auto resolveEntityID = [](QComboBox* combo) -> QString {
        QVariant data = combo->currentData();
        if (data.isValid() && !data.toString().isEmpty())
            return data.toString();
        return combo->currentText().trimmed();
    };

    // ── Data Query ───────────────────────────────────────────────────────
    QGroupBox*   queryGroup = new QGroupBox("Query Data — Data Query PDU (§5.3.6.8)");
    QVBoxLayout* queryVl    = new QVBoxLayout(queryGroup);
    queryVl->setSpacing(6);   // ← add this line
    QCheckBox* cbWeapon = new QCheckBox("Weapon System Status (Datum 100200)");
    QCheckBox* cbIdent  = new QCheckBox("Entity/Track Identity (Datum 100500)");
    QCheckBox* cbEngage = new QCheckBox("Engagement Data / ROI (Datum 100600)");
    QCheckBox* cbEnviro = new QCheckBox("Environmental Data (Datum 101100)");
    QCheckBox* cbTime   = new QCheckBox("Time Synchronization (Datum 101400)");
    for (QCheckBox* cb : {cbWeapon, cbIdent, cbEngage, cbEnviro, cbTime}) {
        cb->setChecked(true);
        queryVl->addWidget(cb);
    }

    QFormLayout* intervalForm = new QFormLayout();
    QSpinBox* intervalSpin = new QSpinBox();
    intervalSpin->setRange(0, 3600);
    intervalSpin->setSuffix(" s");
    intervalSpin->setToolTip(
        "0 = answer once. Non-zero = standing subscription — the target "
        "keeps resending the Data PDU at this interval (IEEE 1278.1 §5.3.6.8).");
    intervalForm->addRow("Time Interval:", intervalSpin);
    queryVl->addLayout(intervalForm);

    QPushButton* sendQueryBtn = new QPushButton("Send Data Query");
    sendQueryBtn->setStyleSheet(NetworkToolbarStyles::ConnectButton);
    queryVl->addWidget(sendQueryBtn);

    vl->addWidget(queryGroup);

    // ── Set Data ─────────────────────────────────────────────────────────
    QGroupBox*   setGroup = new QGroupBox("Set Engagement ROI — Set Data PDU (§5.3.6.9, Datum 100600)");
    QFormLayout* setForm  = new QFormLayout(setGroup);

    QComboBox* roiCombo = new QComboBox();
    roiCombo->addItems({
        "HOLD_FIRE", "RETURN_FIRE_ONLY", "DEFENSIVE_ONLY", "FIRE_ON_DETECTION",
        "FIRE_ON_IDENTIFICATION", "FREE_FIRE", "COMMAND_AUTHORIZATION_REQUIRED"
    });
    setForm->addRow("New ROI:", roiCombo);

    QPushButton* sendSetBtn = new QPushButton("Send Set Data");
    sendSetBtn->setStyleSheet(NetworkToolbarStyles::ConnectButton);
    setForm->addRow("", sendSetBtn);

    vl->addWidget(setGroup);

    // ── Responses ────────────────────────────────────────────────────────
    QGroupBox*   responseGroup = new QGroupBox("Responses — Data PDU (§5.3.6.10)");
    QVBoxLayout* responseVl    = new QVBoxLayout(responseGroup);

    QTextEdit* responseLog = new QTextEdit();
    responseLog->setReadOnly(true);
    responseLog->setMaximumHeight(140);
    responseVl->addWidget(responseLog);

    vl->addWidget(responseGroup);
    vl->addStretch();

    if (m_disPlugin) {
        connect(m_disPlugin, &DISNetworkPlugin::dataResponseReceived,
                responseLog, [responseLog](QString summary) {
                    responseLog->append(summary);
                });
    }

    connect(sendQueryBtn, &QPushButton::clicked,
            [this, fromCombo, toCombo, resolveEntityID,
             cbWeapon, cbIdent, cbEngage, cbEnviro, cbTime,
             intervalSpin, responseLog]() {

                if (!m_disPlugin || !m_disPlugin->isRunning()) {
                    QMessageBox::warning(nullptr, "DIS", "DIS not connected.");
                    return;
                }

                QString from = resolveEntityID(fromCombo);
                QString to   = resolveEntityID(toCombo);
                if (from.isEmpty() || to.isEmpty()) {
                    QMessageBox::warning(nullptr, "DIS",
                                         "Select or enter both From and To entities.");
                    return;
                }

                static uint32_t requestID = 1000;

                DISDatumSnapshot snap;
                snap.originatingEntityID = from.toStdString();
                snap.receivingEntityID   = to.toStdString();
                snap.requestID           = requestID++;
                snap.timeInterval        = static_cast<uint32_t>(intervalSpin->value());

                if (cbWeapon->isChecked()) snap.fixedDatums.push_back({DISDatumID::WeaponSystemStatus, 0});
                if (cbEngage->isChecked()) snap.fixedDatums.push_back({DISDatumID::EngagementData, 0});
                if (cbTime->isChecked())   snap.fixedDatums.push_back({DISDatumID::TimeSynchronization, 0});
                if (cbIdent->isChecked())  snap.variableDatums.push_back({DISDatumID::EntityTrackIdentity, QByteArray()});
                if (cbEnviro->isChecked()) snap.variableDatums.push_back({DISDatumID::EnvironmentalData, QByteArray()});

                if (snap.fixedDatums.empty() && snap.variableDatums.empty()) {
                    QMessageBox::warning(nullptr, "DIS", "Select at least one Datum ID to query.");
                    return;
                }

                m_disPlugin->publishDataQuery(snap);
                responseLog->append(QString("→ Sent DataQuery requestID=%1 to %2")
                                        .arg(snap.requestID).arg(to));
            });

    connect(sendSetBtn, &QPushButton::clicked,
            [this, fromCombo, toCombo, resolveEntityID, roiCombo, responseLog]() {

                if (!m_disPlugin || !m_disPlugin->isRunning()) {
                    QMessageBox::warning(nullptr, "DIS", "DIS not connected.");
                    return;
                }

                QString from = resolveEntityID(fromCombo);
                QString to   = resolveEntityID(toCombo);
                if (from.isEmpty() || to.isEmpty()) {
                    QMessageBox::warning(nullptr, "DIS",
                                         "Select or enter both From and To entities.");
                    return;
                }

                static uint32_t requestID = 5000;

                DISDatumSnapshot snap;
                snap.originatingEntityID = from.toStdString();
                snap.receivingEntityID   = to.toStdString();
                snap.requestID           = requestID++;
                snap.fixedDatums.push_back({
                    DISDatumID::EngagementData,
                    static_cast<uint32_t>(roiCombo->currentIndex())
                });

                m_disPlugin->publishSetData(snap);
                responseLog->append(QString("→ Sent SetData requestID=%1 to %2 — ROI=%3")
                                        .arg(snap.requestID).arg(to).arg(roiCombo->currentText()));
            });
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(tab);
    return scrollArea;
}
    // return tab;
// }
// // =============================================================================
// // createTransferTab
// // Allows user to transfer ownership of a locally owned entity to another
// // machine. Enter target site/app ID manually or select from discovered peers.
// // IEEE 1278.1 Transfer Ownership PDU (type 35).
// // =============================================================================
// QWidget* NetworkToolbar::createTransferTab()
// {
//     QWidget*     tab = new QWidget();
//     QVBoxLayout*  vl = new QVBoxLayout(tab);

//     QLabel* infoLabel = new QLabel(
//         "Transfer ownership of a locally owned entity to another machine.\n"
//         "The entity keeps its DIS ID — all machines see continuity.");
//     infoLabel->setWordWrap(true);
//     infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
//     vl->addWidget(infoLabel);

//     // ── Entity selection ──────────────────────────────────────────────────────
//     QGroupBox*   entityGroup = new QGroupBox("Entity to Transfer");
//     QFormLayout* entityForm  = new QFormLayout(entityGroup);

//     QComboBox* entityCombo = new QComboBox();
//     if (m_disPlugin && m_disPlugin->isRunning()) {
//         QList<QPair<QString,QString>> entities = m_disPlugin->getLocalEntityList();
//         if (entities.isEmpty()) {
//             entityCombo->addItem("No local entities", "");
//         } else {
//             for (const auto& pair : entities) {
//                 entityCombo->addItem(pair.second + "  [" + pair.first + "]",
//                                      pair.first);
//             }
//         }
//     } else {
//         entityCombo->addItem("Connect DIS first", "");
//     }
//     entityForm->addRow("Entity:", entityCombo);
//     vl->addWidget(entityGroup);

//     // ── Target machine ────────────────────────────────────────────────────────
//     QGroupBox*   targetGroup = new QGroupBox("Target Machine");
//     QFormLayout* targetForm  = new QFormLayout(targetGroup);

//     QSpinBox* siteSpinTarget = new QSpinBox();
//     siteSpinTarget->setRange(1, 65535);
//     siteSpinTarget->setValue(1);
//     siteSpinTarget->setToolTip("Site ID of the receiving machine");
//     targetForm->addRow("Target Site ID:", siteSpinTarget);

//     QSpinBox* appSpinTarget = new QSpinBox();
//     appSpinTarget->setRange(1, 65535);
//     appSpinTarget->setValue(2);
//     appSpinTarget->setToolTip("Application ID of the receiving machine");
//     targetForm->addRow("Target App ID:", appSpinTarget);

//     vl->addWidget(targetGroup);

//     // ── Transfer button ───────────────────────────────────────────────────────
//     QPushButton* transferBtn = new QPushButton("Transfer Ownership");
//     transferBtn->setStyleSheet(NetworkToolbarStyles::ConnectButton);

//     connect(transferBtn, &QPushButton::clicked,
//             [this, entityCombo, siteSpinTarget, appSpinTarget]() {

//                 if (!m_disPlugin || !m_disPlugin->isRunning()) {
//                     QMessageBox::warning(nullptr, "DIS", "DIS not connected.");
//                     return;
//                 }

//                 QString entityID = entityCombo->currentData().toString();
//                 if (entityID.isEmpty()) {
//                     QMessageBox::warning(nullptr, "DIS",
//                                          "Select a locally owned entity to transfer.");
//                     return;
//                 }

//                 uint16_t targetSite = static_cast<uint16_t>(siteSpinTarget->value());
//                 uint16_t targetApp  = static_cast<uint16_t>(appSpinTarget->value());

//                 QMessageBox::StandardButton reply = QMessageBox::question(
//                     nullptr, "Confirm Transfer",
//                     QString("Transfer '%1' to machine Site=%2 App=%3?\n\n"
//                             "The target machine will take control of this entity.")
//                         .arg(entityCombo->currentText())
//                         .arg(targetSite)
//                         .arg(targetApp),
//                     QMessageBox::Yes | QMessageBox::No);

//                 if (reply == QMessageBox::Yes) {
//                     m_disPlugin->transferEntityOwnership(entityID, targetSite, targetApp);
//                     QMessageBox::information(nullptr, "DIS",
//                                              "Transfer request sent.\n"
//                                              "Waiting for acknowledgement from target machine.");
//                 }
//             });

//     vl->addWidget(transferBtn);

//     QLabel* noteLabel = new QLabel(
//         "IEEE 1278.1 Transfer Ownership PDU (type 35).\n"
//         "Target machine must be connected and running TDF.\n"
//         "On success: entity ownership moves automatically.");
//     noteLabel->setWordWrap(true);
//     noteLabel->setStyleSheet("color: #666666; font-size: 10px;");
//     vl->addWidget(noteLabel);

//     vl->addStretch();
//     return tab;
// }
QWidget* NetworkToolbar::createStatusTab()
{
    QWidget* tab    = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(tab);

    // ── Connection status ─────────────────────────────────────────────────────
    QGroupBox* connGroup = new QGroupBox("Connection");
    QFormLayout* connForm = new QFormLayout(connGroup);

    bool connected = m_disPlugin && m_disPlugin->isRunning();

    QLabel* connLabel = new QLabel(connected ? "Connected" : "Disconnected");
    connLabel->setStyleSheet(connected
                                 ? "color: #00C853; font-weight: bold;"
                                 : "color: #C42B1C; font-weight: bold;");
    connForm->addRow("Status:", connLabel);

    if (m_disPlugin) {
        DISConfig cfg = m_disPlugin->currentConfig();
        connForm->addRow("Multicast:",
                         new QLabel(QString::fromStdString(cfg.multicastGroup)));
        connForm->addRow("Port:",
                         new QLabel(QString::number(cfg.port)));
        connForm->addRow("Exercise ID:",
                         new QLabel(QString::number(cfg.exerciseID)));
        connForm->addRow("Site ID:",
                         new QLabel(QString::number(cfg.siteID)));
        connForm->addRow("Application ID:",
                         new QLabel(QString::number(cfg.applicationID)));
    }
    vl->addWidget(connGroup);

    // ── Live stats ────────────────────────────────────────────────────────────
    QGroupBox* statsGroup = new QGroupBox("Statistics");
    QFormLayout* statsForm = new QFormLayout(statsGroup);

    QLabel* txLabel = new QLabel("—");
    QLabel* rxLabel = new QLabel("—");
    QLabel* peerLabel = new QLabel("—");

    statsForm->addRow("PDUs Sent/sec:", txLabel);
    statsForm->addRow("PDUs Recv/sec:", rxLabel);
    statsForm->addRow("Active Peers:", peerLabel);

    vl->addWidget(statsGroup);

    // Update stats every second
    QTimer* statsTimer = new QTimer(tab);
    statsTimer->setInterval(1000);
    connect(statsTimer, &QTimer::timeout, [=]() {
        if (!m_disPlugin) return;
        DISNetworkStatus status = m_disPlugin->currentStatus();
        txLabel->setText(QString::number(status.pdusSentPerSec) + " /s");
        rxLabel->setText(QString::number(status.pdusRecvPerSec) + " /s");
        peerLabel->setText(QString::number(status.activePeers));
    });
    statsTimer->start();

    // ── Peer list ─────────────────────────────────────────────────────────────
    QGroupBox* peerGroup = new QGroupBox("Discovered Peers");
    QVBoxLayout* peerVl = new QVBoxLayout(peerGroup);

    QTableWidget* peerTable = new QTableWidget(0, 3);
    peerTable->setHorizontalHeaderLabels({"Address", "DIS Version", "Site/App"});
    peerTable->horizontalHeader()->setStretchLastSection(true);
    peerTable->verticalHeader()->setVisible(false);
    peerTable->setMaximumHeight(120);

    peerVl->addWidget(peerTable);
    vl->addWidget(peerGroup);
    vl->addStretch();
    return tab;
}

// =============================================================================
// updateStatusIndicator
// Called every second to update the toolbar status label
// =============================================================================
void NetworkToolbar::updateStatusIndicator()
{
    if (!m_disPlugin || !m_disPlugin->isRunning()) {
        m_statusLabel->setText("DIS: Offline");
        m_statusLabel->setStyleSheet(
            "color: #C42B1C; font-weight: bold; font-size: 11px; padding: 0 8px;");
        return;
    }

    DISNetworkStatus status = m_disPlugin->currentStatus();
    m_statusLabel->setText(
        QString("DIS: Online | TX: %1 | Peers: %2")
            .arg(status.pdusSentPerSec)
            .arg(status.activePeers));
    m_statusLabel->setStyleSheet(
        "color: #00C853; font-weight: bold; font-size: 11px; padding: 0 8px;");
}

// =============================================================================
// withWhiteBg — icon helper
// =============================================================================
QPixmap NetworkToolbar::withWhiteBg(const QString& iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) return QPixmap();
    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);
    QPainter painter(&newPixmap);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();
    return newPixmap;
}
// =============================================================================
// PLUGIN SUPPORT — exactly same pattern as DesignToolBar
// =============================================================================

QAction* NetworkToolbar::registerPluginAction(const QString& pluginId,
                                              QAction* action,
                                              int position)
{
    if (!action || pluginId.isEmpty())
        return nullptr;

    if (position < 0 || position >= this->actions().size()) {
        addAction(action);
    } else {
        insertAction(this->actions().at(position), action);
    }

    QWidget* btn = widgetForAction(action);
    if (btn) {
        btn->setStyleSheet(NetworkToolbarStyles::Toolbar);   // existing style use karo
        // Agar aapke paas ToolbarButton style hai to wo use karo
        // btn->setStyleSheet(NetworkToolbarStyles::ToolbarButton);
    }

    m_pluginOwnedObjects[pluginId].append(action);
    return action;
}

QToolButton* NetworkToolbar::registerPluginToolButton(const QString& pluginId,
                                                      QAction* action,
                                                      QToolButton::ToolButtonPopupMode mode,
                                                      int position)
{
    if (!action || pluginId.isEmpty())
        return nullptr;

    QToolButton* btn = new QToolButton(this);
    btn->setDefaultAction(action);
    btn->setPopupMode(mode);
    btn->setIconSize(QSize(20, 20));
    btn->setStyleSheet(NetworkToolbarStyles::Toolbar);   // same style

    QAction* wrapperAction = nullptr;
    if (position < 0 || position >= this->actions().size()) {
        wrapperAction = addWidget(btn);
    } else {
        wrapperAction = insertWidget(this->actions().at(position), btn);
    }

    m_pluginOwnedObjects[pluginId].append(btn);
    m_pluginOwnedObjects[pluginId].append(action);
    if (wrapperAction) {
        m_pluginOwnedObjects[pluginId].append(wrapperAction);
    }

    return btn;
}

QAction* NetworkToolbar::registerPluginMenu(const QString& pluginId,
                                            const QString& title,
                                            QMenu* menu,
                                            int position)
{
    if (!menu || pluginId.isEmpty())
        return nullptr;

    QAction* act = new QAction(title, this);
    act->setMenu(menu);
    registerPluginToolButton(pluginId, act, QToolButton::InstantPopup, position);
    m_pluginOwnedObjects[pluginId].append(menu);
    return act;
}

void NetworkToolbar::unregisterPlugin(const QString& pluginId)
{
    if (!m_pluginOwnedObjects.contains(pluginId))
        return;

    const QList<QObject*> objects = m_pluginOwnedObjects.value(pluginId);
    for (QObject* obj : objects) {
        if (QAction* act = qobject_cast<QAction*>(obj)) {
            removeAction(act);
            act->deleteLater();
        }
        else if (QToolButton* btn = qobject_cast<QToolButton*>(obj)) {
            btn->deleteLater();
        }
        else if (QMenu* menu = qobject_cast<QMenu*>(obj)) {
            menu->deleteLater();
        }
    }
    m_pluginOwnedObjects.remove(pluginId);
}
