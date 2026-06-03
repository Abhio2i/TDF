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
void NetworkToolbar::setDISPlugin(DISNetworkPlugin* plugin)
{
    m_disPlugin = plugin;
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
    m_connectAction = new QAction(QIcon(":/icons/images/play.png"),
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
    dialog.setFixedSize(520, 560);
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

    QSpinBox* portSpin = new QSpinBox();
    portSpin->setRange(1024, 65535);
    portSpin->setValue(config.port);
    form->addRow("Communication Port:", portSpin);
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
    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&config, peerGroup](int idx) {
                if (idx == 1) config.connectionMode = "Unicast";
                else if (idx == 2) config.connectionMode = "Broadcast";
                else config.connectionMode = "Multicast";
                peerGroup->setVisible(idx == 1);
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
    versionCombo->addItems({"DIS Version 6 (IEEE 1278.1-1995)",
                            "DIS Version 7 (IEEE 1278.1-2012)"});
    versionCombo->setCurrentIndex(config.defaultVersion == 7 ? 1 : 0);
    form->addRow("DIS Version:", versionCombo);
    connect(versionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [&config](int idx) {
                config.defaultVersion = static_cast<uint8_t>(idx == 1 ? 7 : 6);
            });

    // Exercise ID
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
// QWidget* NetworkToolbar::createMessagesTab()
// {
//     QWidget* tab    = new QWidget();
//     QVBoxLayout* vl = new QVBoxLayout(tab);

//     QLabel* infoLabel = new QLabel(
//         "Select which PDU types to send and receive:");
//     infoLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
//     vl->addWidget(infoLabel);

//     // Table of PDU types
//     QTableWidget* table = new QTableWidget(0, 3);
//     table->setHorizontalHeaderLabels({"PDU Type", "Send", "Receive"});
//     table->horizontalHeader()->setStretchLastSection(false);
//     table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
//     table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
//     table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
//     table->setColumnWidth(1, 60);
//     table->setColumnWidth(2, 70);
//     table->verticalHeader()->setVisible(false);
//     table->setSelectionMode(QAbstractItemView::NoSelection);

//     struct PDURow { QString name; bool send; bool recv; };
//     QList<PDURow> pdus = {
//                           {"Entity State",              true,  true},
//                           {"Fire",                      true,  true},
//                           {"Detonation",                true,  true},
//                           {"Start / Resume",            true,  true},
//                           {"Stop / Freeze",             true,  true},
//                           {"Create Entity",             true,  true},
//                           {"Remove Entity",             true,  true},
//                           {"Electromagnetic Emission",  false, false},
//                           };

//     for (const PDURow& pdu : pdus) {
//         int row = table->rowCount();
//         table->insertRow(row);

//         table->setItem(row, 0, new QTableWidgetItem(pdu.name));
//         table->item(row, 0)->setFlags(Qt::ItemIsEnabled);

//         QCheckBox* sendCheck = new QCheckBox();
//         sendCheck->setChecked(pdu.send);
//         QWidget* sendWidget = new QWidget();
//         QHBoxLayout* sl = new QHBoxLayout(sendWidget);
//         sl->addWidget(sendCheck); sl->setAlignment(Qt::AlignCenter);
//         sl->setContentsMargins(0,0,0,0);
//         table->setCellWidget(row, 1, sendWidget);

//         QCheckBox* recvCheck = new QCheckBox();
//         recvCheck->setChecked(pdu.recv);
//         QWidget* recvWidget = new QWidget();
//         QHBoxLayout* rl = new QHBoxLayout(recvWidget);
//         rl->addWidget(recvCheck); rl->setAlignment(Qt::AlignCenter);
//         rl->setContentsMargins(0,0,0,0);
//         table->setCellWidget(row, 2, recvWidget);
//     }

//     vl->addWidget(table);
//     return tab;
// }

// =============================================================================
// createStatusTab
// Status tab — live TX/RX rates, peer list, PDU log
// =============================================================================
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
