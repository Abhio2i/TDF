#ifndef TCPAUTOSCRIPTSOURCE_H
#define TCPAUTOSCRIPTSOURCE_H

#include "autoscriptsource.h"
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QPointer>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

// LAN TCP receiver. Two input formats on the same port:
//
// 1) Framed JSON (autoscript_client, ISSST sender):
//    [4-byte big-endian length][UTF-8 JSON]
//    Request : {"type":"script","id":"...","replace":false,"content":"..."}
//    Response: {"id":"...","status":"QUEUED|RUNNING|DONE|...","message":"..."}
//
// 2) Raw text (nc -N host 9090 < file.as):
//    The first byte of a JSON frame is always 0x00 (length < 16 MB).
//    If the first byte is anything else, the connection is treated as a raw
//    script: bytes are collected until the sender closes the connection, then
//    the whole text is queued as one script (id "raw-<time>", no replace,
//    no status reply).
class TcpAutoScriptSource : public AutoScriptSource
{
    Q_OBJECT
public:
    explicit TcpAutoScriptSource(QObject* parent = nullptr);
    ~TcpAutoScriptSource() override;

    QString name() const override { return QStringLiteral("TCP AutoScript Source"); }

    void setListenAddress(const QHostAddress& address);
    void setListenPort(quint16 port);
    void setAllowedHosts(const QList<QHostAddress>& hosts); // empty = any host
    void setMaxScriptSize(int bytes);

    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void sendStatus(const AutoScriptRequest& request,
                    const QString& status, const QString& message) override;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();

private:
    bool isHostAllowed(QHostAddress peer) const;
    void parseAndEmit(QTcpSocket* socket, const QByteArray& payload);
    void reply(QTcpSocket* socket, const QString& id,
               const QString& status, const QString& message);

    QTcpServer*         m_server = nullptr;
    QHostAddress        m_address = QHostAddress::AnyIPv4;
    quint16             m_port = 9090;
    QList<QHostAddress> m_allowed;
    int                 m_maxScriptSize = 512 * 1024;

    QHash<QTcpSocket*, QByteArray>       m_buffers;
    QHash<QTcpSocket*, quint64>          m_socketIds;
    QHash<quint64, QPointer<QTcpSocket>> m_sockets;
    quint64                              m_nextConnId = 1;

    // raw (nc) connections: socket -> "ip:port"; script is queued when the sender closes
    QHash<QTcpSocket*, QString>          m_rawSockets;
};

#endif
