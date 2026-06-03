// =============================================================================
// FILE:        DISTransport.cpp
// MODULE:      DIS Network Plugin — Transport
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

// Change line 1:
#include "DISTransport.h"

#include <QTimer>
#include <QDebug>
#include <QNetworkInterface>
#include <QDateTime>
#include <core/DISPlugin//utils/dislogger.h>
// =============================================================================
// Constructor
// =============================================================================
DISTransport::DISTransport(QObject* parent)
    : QObject(parent)
{
    // Default DIS standard multicast group and port
    m_multicastGroup = QHostAddress("239.255.0.1");
    m_port           = 3000;
}

// =============================================================================
// Destructor
// =============================================================================
DISTransport::~DISTransport()
{
    stop();
}

// =============================================================================
// init
// Configure multicast group, port, and optional interface
// Call before start()
// =============================================================================
// void DISTransport::init(const QString& multicastGroup,
//                             quint16        port,
//                             const QString& interfaceAddr)
// {
//     m_multicastGroup = QHostAddress(multicastGroup);
//     m_port           = port;
//     m_interfaceAddr  = interfaceAddr;
//     m_localIP        = getLocalIP();

//     qDebug() << "[DISTransport] Configured:"
//              << "group=" << multicastGroup
//              << "port="  << port
//              << "localIP=" << m_localIP;
// }
void DISTransport::init(const QString& multicastGroup,
                        quint16        port,
                        const QString& interfaceAddr,
                        const QString& networkMode)
{
    m_multicastGroup = QHostAddress(multicastGroup);
    m_port           = port;
    m_interfaceAddr  = interfaceAddr;
    m_networkMode    = networkMode;
    m_localIP        = getLocalIP();

    DIS_LOG_BASIC("[DISTransport] Configured:"
            << "group="   << multicastGroup
             << "port="    << port
             << "localIP=" << m_localIP
             << "mode="    << networkMode);
}
// =============================================================================
// start
// Create socket, bind to port, join multicast group
// Must be called on the network thread (called by DISManager)
// =============================================================================
bool DISTransport::start()
{
    if (m_running.load()) {
        DIS_LOG_WARNING("[DISTransport] Already running");
        return true;
    }

    // Create UDP socket
    m_socket = new QUdpSocket(this);

    // ── Bind to DIS port ──────────────────────────────────────────────────────
    // ShareAddress + ReuseAddressHint allows multiple apps on same machine
    // to listen on the same multicast port (important for testing on loopback)
    bool bound = m_socket->bind(
        QHostAddress::AnyIPv4,
        m_port,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        );

    if (!bound) {
        QString err = QString("[DISTransport] Failed to bind port %1: %2")
        .arg(m_port)
            .arg(m_socket->errorString());
        qCritical() << err;
        emit errorOccurred(err);
        delete m_socket;
        m_socket = nullptr;
        return false;
    }

    DIS_LOG_BASIC("[DISTransport] Bound to port" << m_port);

    // ── Join multicast group ──────────────────────────────────────────────────
    bool joined = false;
    if (!m_interfaceAddr.isEmpty()) {
        // Try by interface name first (e.g. "enp3s0", "eno2")
        m_joinedInterface = QNetworkInterface::interfaceFromName(m_interfaceAddr);

        // If not found by name, try by IP address (e.g. "192.168.1.2")
        if (!m_joinedInterface.isValid()) {
            for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
                for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                    if (entry.ip().toString() == m_interfaceAddr) {  // ← no conversion needed
                        m_joinedInterface = iface;
                        break;
                    }
                }
                if (m_joinedInterface.isValid()) break;
            }
        }

        DIS_LOG_BASIC("[DISTransport] Using configured interface:" << m_joinedInterface.name());
    } else {
        m_joinedInterface = getBestInterface();
    }

    if (m_joinedInterface.isValid()) {
        joined = m_socket->joinMulticastGroup(m_multicastGroup, m_joinedInterface);
        DIS_LOG_BASIC("[DISTransport] Join on" << m_joinedInterface.name()
                 << (joined ? ": SUCCESS" : ": FAILED") << m_socket->errorString());
    }

    if (!joined) {
        DIS_LOG_ERROR("[DISTransport] Multicast join FAILED"
                    << "— this machine will NOT receive PDUs");
        emit errorOccurred("Multicast join failed: " + m_socket->errorString());
    }

    // Watchdog — auto re-joins if network changes (WiFi change etc)
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(15000);
    connect(m_watchdogTimer, &QTimer::timeout, this, [this]() {
        if (!m_running.load() || !m_socket || !m_joinedInterface.isValid()) return;
        m_socket->leaveMulticastGroup(m_multicastGroup, m_joinedInterface);
        bool ok = m_socket->joinMulticastGroup(m_multicastGroup, m_joinedInterface);
        // qDebug() << "[DISTransport] Watchdog rejoin on"
        //          << m_joinedInterface.name()
        //          << (ok ? ": OK" : ": FAILED");
    });
    m_watchdogTimer->start();


    // Critical: outgoing multicast must use the SAME interface that joined.
    // Without this, send goes via OS routing (may pick wrong interface).
    if (m_joinedInterface.isValid()) {
        m_socket->setMulticastInterface(m_joinedInterface);
        DIS_LOG_BASIC("[DISTransport] Multicast outgoing interface pinned to:" << m_joinedInterface.name());
    }

    if (!joined) {
        // Not fatal — some environments restrict multicast
        // Still works for unicast and loopback testing
        DIS_LOG_WARNING("[DISTransport] Could not join multicast group" << m_multicastGroup.toString());
    } else {
        DIS_LOG_BASIC("[DISTransport] Joined multicast group" << m_multicastGroup.toString());
    }

    // ── Set multicast TTL ─────────────────────────────────────────────────────
    // TTL=1 means packets stay on local subnet only (standard for DIS on LAN)
    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);

    // ── Enable loopback for local testing ─────────────────────────────────────
    // When both sender and receiver are on same machine
    m_socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);

    // ── Connect readyRead signal ──────────────────────────────────────────────
    connect(m_socket, &QUdpSocket::readyRead,
            this, &DISTransport::onReadyRead);

    // ── Connect error signal ──────────────────────────────────────────────────
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                QString err = m_socket->errorString();
        DIS_LOG_WARNING("[DISTransport] Socket error:" << err);
            });

    // ── Stats timer (every 1 second) ──────────────────────────────────────────
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout,
            this, &DISTransport::onStatsTimer);
    m_statsTimer->start(1000);

    m_running.store(true);

    DIS_LOG_BASIC("[DISTransport] Started successfully");
    emit transportStarted();
    return true;
}

// =============================================================================
// stop
// Leave multicast group, close socket, stop stats timer
// =============================================================================
void DISTransport::stop()
{
    if (!m_running.load()) return;

    m_running.store(false);

    if (m_watchdogTimer) {
        m_watchdogTimer->stop();
        m_watchdogTimer = nullptr;
    }
    if (m_statsTimer) {
        m_statsTimer->stop();
        m_statsTimer = nullptr;
    }

    if (m_socket) {
        // Leave multicast group cleanly
        //m_socket->leaveMulticastGroup(m_multicastGroup);
        if (m_joinedInterface.isValid())
            m_socket->leaveMulticastGroup(m_multicastGroup, m_joinedInterface);
        else
            m_socket->leaveMulticastGroup(m_multicastGroup);
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    DIS_LOG_BASIC("[DISTransport] Stopped");
    emit transportStopped();
}

// =============================================================================
// sendMulticast
// Send raw PDU bytes to multicast group
// Every DIS node on LAN that joined this group receives it
// No compression. No framing. Raw bytes exactly as DIS standard requires.
// =============================================================================
void DISTransport::sendMulticast(const QByteArray& pduBytes)
{
    if (!m_running.load() || !m_socket) return;
    if (pduBytes.isEmpty()) return;

    // qint64 sent = m_socket->writeDatagram(
    //     pduBytes,
    //     m_multicastGroup,
    //     m_port
    //     );
    QHostAddress target = (m_networkMode == "Broadcast")
                              ? QHostAddress::Broadcast
                              : m_multicastGroup;

    qint64 sent = m_socket->writeDatagram(
        pduBytes,
        target,
        m_port
        );
    // qDebug() << "[TX]" << QDateTime::currentMSecsSinceEpoch()
    //          << "bytes=" << pduBytes.size() << "sent=" << sent
    //          << (sent == -1 ? m_socket->errorString() : "ok");
    if (sent == -1) {
        DIS_LOG_WARNING("[DISTransport] Send failed:"
                   << m_socket->errorString());
        emit errorOccurred("Send failed: " + m_socket->errorString());
    } else {
        m_pdusSentCount.fetch_add(1);
    }
}

// =============================================================================
// sendUnicast
// Send raw PDU bytes to a specific IP address
// Used for V6 peers that need separate version-specific packets
// =============================================================================
void DISTransport::sendUnicast(const QByteArray& pduBytes,
                                   const QHostAddress& peerAddress,
                                   quint16 peerPort)
{
    if (!m_running.load() || !m_socket) return;
    if (pduBytes.isEmpty()) return;

    quint16 targetPort = (peerPort == 0) ? m_port : peerPort;

    qint64 sent = m_socket->writeDatagram(
        pduBytes,
        peerAddress,
        targetPort
        );

    if (sent == -1) {
        DIS_LOG_WARNING("[DISTransport] Unicast send failed to" << peerAddress.toString());
    } else {
        m_pdusSentCount.fetch_add(1);
    }
}

// =============================================================================
// onReadyRead
// Called when UDP socket has incoming datagrams
// Reads ALL pending datagrams and emits datagramReceived for each
// This runs on the network thread
// =============================================================================
void DISTransport::onReadyRead()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray      datagram;
        QHostAddress    sender;
        quint16         senderPort;

        // Resize to exact datagram size
        datagram.resize(static_cast<int>(m_socket->pendingDatagramSize()));

        qint64 read = m_socket->readDatagram(
            datagram.data(),
            datagram.size(),
            &sender,
            &senderPort
            );

        if (read <= 0) {
            DIS_LOG_WARNING("[DISTransport] readDatagram failed");
            continue;
        }

        // Trim to actual bytes read
        datagram.resize(static_cast<int>(read));

        // Basic sanity check — DIS PDU minimum size is 12 bytes (header)
        if (datagram.size() < 12) {
            DIS_LOG_WARNING("[DISTransport] Datagram too small:" << datagram.size() << "bytes");
            continue;
        }

        m_pdusRecvCount.fetch_add(1);

        // Emit for PDUDispatcher to handle
        emit datagramReceived(datagram, sender, senderPort);
    }
}

// =============================================================================
// onStatsTimer
// Called every 1 second to update TX/RX rates
// =============================================================================
void DISTransport::onStatsTimer()
{
    m_pdusSentSec.store(m_pdusSentCount.exchange(0));
    m_pdusRecvSec.store(m_pdusRecvCount.exchange(0));
}

// =============================================================================
// getLocalIP
// Returns this machine's local IPv4 address
// Skips loopback and non-active interfaces
// =============================================================================
QString DISTransport::getLocalIP() const
{
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol
            && addr != QHostAddress::LocalHost
            && !addr.toString().startsWith("169.")) // skip APIPA
        {
            return addr.toString();
        }
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

// =============================================================================
// getBestInterface
// Returns the best network interface for multicast
// Prefers interfaces that are up, running, and not loopback
// =============================================================================
QNetworkInterface DISTransport::getBestInterface() const
{
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        QNetworkInterface::InterfaceFlags flags = iface.flags();

        // Must be up, running, and not loopback
        if (flags.testFlag(QNetworkInterface::IsUp)
            && flags.testFlag(QNetworkInterface::IsRunning)
            && !flags.testFlag(QNetworkInterface::IsLoopBack))
        {
            // Must have at least one IPv4 address
            for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    DIS_LOG_BASIC("[DISTransport] Selected interface:" << iface.name());
                    return iface;
                }
            }
        }
    }

    // Fallback to default
    return QNetworkInterface();
}
