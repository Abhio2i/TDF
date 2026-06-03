// =============================================================================
// FILE:        DISTransport.h
// MODULE:      DIS Network Plugin — Transport
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Pure UDP multicast socket layer.
//              Sends and receives raw bytes only.
//              No DIS knowledge. No engine knowledge.
//              Just UDP multicast on LAN.
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   readyRead() fires on network thread.
//   sendDatagram() called from network thread.
//   Main thread never touches this class directly.
//
// STANDARD:
//   UDP Multicast per IEEE 1278.1 DIS standard
//   Default group : 239.255.0.1
//   Default port  : 3000
//   No compression. No framing. Raw PDU bytes only.
// =============================================================================

#ifndef DISTransport_H
#define DISTransport_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QByteArray>
#include <QString>
#include <atomic>
#include <QTimer>

class DISTransport : public QObject {
    Q_OBJECT

public:
    explicit DISTransport(QObject* parent = nullptr);
    ~DISTransport() override;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    // Initialize with multicast group and port
    // Call before start()
    // void init(const QString& multicastGroup,
    //           quint16        port,
    //           const QString& interfaceAddr = "");
    void init(const QString& multicastGroup,
              quint16        port,
              const QString& interfaceAddr = "",
              const QString& networkMode   = "Multicast");

    // Join multicast group and start listening
    // Must be called on the network thread
    bool start();

    // Leave multicast group and close socket
    void stop();

    // ── Send ─────────────────────────────────────────────────────────────────

    // Send raw PDU bytes to multicast group
    // All DIS nodes on LAN receive this
    void sendMulticast(const QByteArray& pduBytes);

    // Send raw PDU bytes to specific peer (unicast)
    // Used for V6 peers that need separate packets
    void sendUnicast(const QByteArray& pduBytes,
                     const QHostAddress& peerAddress,
                     quint16 peerPort = 0); // 0 = use default port

    // ── Status ────────────────────────────────────────────────────────────────
    bool isRunning()    const { return m_running.load(); }
    QString localIP()   const { return m_localIP; }
    int pdusSentSec()   const { return m_pdusSentSec.load(); }
    int pdusRecvSec()   const { return m_pdusRecvSec.load(); }

signals:
    // Emitted when a datagram arrives
    // Receiver connects to this
    void datagramReceived(QByteArray data, QHostAddress sender, quint16 senderPort);

    // Status signals
    void transportStarted();
    void transportStopped();
    void errorOccurred(QString error);

private slots:
    // Called by QUdpSocket::readyRead signal
    void onReadyRead();

    // Called every second to calculate TX/RX rates
    void onStatsTimer();

private:
    // Socket
    QUdpSocket*  m_socket       = nullptr;
    QTimer*      m_statsTimer   = nullptr;
    QNetworkInterface m_joinedInterface;
    // Config
    QHostAddress m_multicastGroup;
    quint16      m_port          = 3000;
    QString      m_interfaceAddr = "";
    QString      m_localIP       = "";
    QString      m_networkMode   = "Multicast";

    // State
    std::atomic<bool> m_running{false};

    // Stats counters
    std::atomic<int> m_pdusSentSec{0};
    std::atomic<int> m_pdusRecvSec{0};
    std::atomic<int> m_pdusSentCount{0};
    std::atomic<int> m_pdusRecvCount{0};
    QTimer* m_watchdogTimer = nullptr;  // ← ADD THIS

    // Helpers
    QString getLocalIP() const;
    QNetworkInterface getBestInterface() const;
};

#endif // DISTransport_H
