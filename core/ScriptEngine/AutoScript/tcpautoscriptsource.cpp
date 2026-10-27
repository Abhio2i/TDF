#include "tcpautoscriptsource.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtEndian>

namespace {
constexpr int kMaxConnections = 8;

// constant-time comparison (token is sent in clear on LAN, do not leak via timing too)
bool safeEquals(const QString& a, const QString& b)
{
    const QByteArray x = a.toUtf8();
    const QByteArray y = b.toUtf8();
    int diff = x.size() ^ y.size();
    const int n = qMin(x.size(), y.size());
    for (int i = 0; i < n; ++i)
        diff |= (static_cast<unsigned char>(x[i]) ^ static_cast<unsigned char>(y[i]));
    return diff == 0;
}
} // namespace

TcpAutoScriptSource::TcpAutoScriptSource(QObject* parent)
    : AutoScriptSource(parent)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpAutoScriptSource::onNewConnection);
}

TcpAutoScriptSource::~TcpAutoScriptSource()
{
    stop();
}

void TcpAutoScriptSource::setListenAddress(const QHostAddress& a)     { m_address = a; }
void TcpAutoScriptSource::setListenPort(quint16 p)                    { m_port = p; }
// void TcpAutoScriptSource::setSharedToken(const QString& t)            { m_token = t; }
void TcpAutoScriptSource::setAllowedHosts(const QList<QHostAddress>& h) { m_allowed = h; }
void TcpAutoScriptSource::setMaxScriptSize(int bytes)                 { m_maxScriptSize = qMax(1024, bytes); }

bool TcpAutoScriptSource::start()
{
    if (m_server->isListening())
        return true;

    if (!m_server->listen(m_address, m_port)) {
        emit sourceError(QString("TCP listen failed on %1:%2 : %3")
                             .arg(m_address.toString()).arg(m_port)
                             .arg(m_server->errorString()));
        return false;
    }
    qDebug() << "[TcpAutoScriptSource] listening on"
             << m_address.toString() << ":" << m_port;
    return true;
}

void TcpAutoScriptSource::stop()
{
    const QList<QTcpSocket*> sockets = m_buffers.keys();
    for (QTcpSocket* s : sockets) {
        s->disconnect(this);
        s->abort();
        s->deleteLater();
    }
    m_buffers.clear();
    m_socketIds.clear();
    m_sockets.clear();
    m_rawSockets.clear();

    if (m_server && m_server->isListening())
        m_server->close();
}

bool TcpAutoScriptSource::isRunning() const
{
    return m_server && m_server->isListening();
}

bool TcpAutoScriptSource::isHostAllowed(QHostAddress peer) const
{
    if (m_allowed.isEmpty())
        return true;

    bool ok = false;
    const quint32 v4 = peer.toIPv4Address(&ok);   // also unwraps ::ffff:a.b.c.d
    if (ok)
        peer = QHostAddress(v4);

    for (const QHostAddress& a : m_allowed)
        if (a == peer)
            return true;
    return false;
}

void TcpAutoScriptSource::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket)
            continue;

        if (!isHostAllowed(socket->peerAddress())) {
            emit sourceError("Connection refused (host not allowed): "
                             + socket->peerAddress().toString());
            socket->abort();
            socket->deleteLater();
            continue;
        }

        if (m_buffers.size() >= kMaxConnections) {
            emit sourceError("Connection refused (too many connections)");
            socket->abort();
            socket->deleteLater();
            continue;
        }

        const quint64 id = m_nextConnId++;
        m_buffers.insert(socket, QByteArray());
        m_socketIds.insert(socket, id);
        m_sockets.insert(id, socket);

        connect(socket, &QTcpSocket::readyRead, this, &TcpAutoScriptSource::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &TcpAutoScriptSource::onSocketDisconnected);
    }
}

void TcpAutoScriptSource::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !m_buffers.contains(socket))
        return;

    QByteArray& buf = m_buffers[socket];
    buf.append(socket->readAll());

    // ---- raw text mode (nc < file.as): first byte is not 0x00 ----
    // A JSON frame always starts with 0x00 (4-byte big-endian length < 16 MB).
    if (!m_rawSockets.contains(socket) && !buf.isEmpty()
        && static_cast<unsigned char>(buf.at(0)) != 0x00) {
        m_rawSockets.insert(socket,
                            QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort()));
    }
    if (m_rawSockets.contains(socket)) {
        if (buf.size() > m_maxScriptSize) {
            emit sourceError("Raw script too large from " + socket->peerAddress().toString());
            socket->write("REJECTED: script exceeds size limit\n");
            m_rawSockets.remove(socket);
            disconnect(socket, &QTcpSocket::readyRead, this, &TcpAutoScriptSource::onReadyRead);
            buf.clear();
            socket->disconnectFromHost();
        }
        return;   // keep collecting until the sender closes the connection
    }
    // ---- end raw mode ----

    // JSON escaping can grow the payload, allow 2x + header room
    const quint32 maxFrame = static_cast<quint32>(m_maxScriptSize) * 2u + 4096u;

    while (buf.size() >= 4) {
        const quint32 len = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(buf.constData()));

        if (len == 0 || len > maxFrame) {
            emit sourceError("Invalid/oversized frame from " + socket->peerAddress().toString());
            reply(socket, QString(), "REJECTED", "Invalid or oversized frame");
            disconnect(socket, &QTcpSocket::readyRead, this, &TcpAutoScriptSource::onReadyRead);
            buf.clear();
            socket->disconnectFromHost();
            return;
        }

        if (buf.size() < 4 + static_cast<int>(len))
            break;   // wait for the rest

        const QByteArray payload = buf.mid(4, static_cast<int>(len));
        buf.remove(0, 4 + static_cast<int>(len));

        parseAndEmit(socket, payload);

        // manager may have reacted synchronously; make sure the socket is still tracked
        if (!m_buffers.contains(socket))
            return;
    }
}

void TcpAutoScriptSource::onSocketDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    const quint64 id = m_socketIds.take(socket);
    m_sockets.remove(id);

    const bool isRaw = m_rawSockets.contains(socket);
    const QString peer = m_rawSockets.take(socket);
    QByteArray data = m_buffers.take(socket);
    if (isRaw)
        data.append(socket->readAll());          // any bytes still unread at close
    socket->deleteLater();

    if (!isRaw)
        return;

    // raw script complete (sender closed the connection)
    if (data.startsWith("\xEF\xBB\xBF"))
        data.remove(0, 3);                       // UTF-8 BOM
    const QString content = QString::fromUtf8(data);
    if (content.trimmed().isEmpty()) {
        emit sourceError("Empty raw script from " + peer);
        return;
    }

    AutoScriptRequest req;
    req.id             = QString("raw-%1").arg(QDateTime::currentMSecsSinceEpoch());
    req.content        = content;
    req.replaceRunning = false;
    req.trusted        = true;                   // LAN sender as allowed
    req.origin         = this;
    req.connectionId   = 0;                      // no reply channel in raw mode
    req.sourceName     = "tcp-raw:" + peer;
    emit scriptReceived(req);
}

void TcpAutoScriptSource::parseAndEmit(QTcpSocket* socket, const QByteArray& payload)
{
    const QString peer = socket->peerAddress().toString();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit sourceError("Invalid JSON from " + peer);
        reply(socket, QString(), "REJECTED", "Invalid JSON");
        return;
    }
    const QJsonObject obj = doc.object();
    QString id = obj.value("id").toString();
    if (obj.value("type").toString() != QLatin1String("script")) {
        reply(socket, id, "REJECTED", "Unsupported message type");
        return;
    }

    const QString content = obj.value("content").toString();
    if (content.trimmed().isEmpty()) {
        reply(socket, id, "REJECTED", "Empty content");
        return;
    }
    if (id.isEmpty())
        id = QString("tcp-%1").arg(QDateTime::currentMSecsSinceEpoch());

    AutoScriptRequest req;
    req.id             = id;
    req.content        = content;
    req.replaceRunning = obj.value("replace").toBool(false);
    req.trusted        = true;   // LAN sender as allowed
    req.origin         = this;
    req.connectionId   = m_socketIds.value(socket, 0);
    req.sourceName     = QString("tcp:%1:%2").arg(peer).arg(socket->peerPort());
    emit scriptReceived(req);
}

void TcpAutoScriptSource::sendStatus(const AutoScriptRequest& request,
                                     const QString& status, const QString& message)
{
    QTcpSocket* s = m_sockets.value(request.connectionId).data();
    reply(s, request.id, status, message);
}

void TcpAutoScriptSource::reply(QTcpSocket* socket, const QString& id,
                                const QString& status, const QString& message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;

    QJsonObject o;
    o.insert("id", id);
    o.insert("status", status);
    o.insert("message", message);

    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    const quint32 be = qToBigEndian<quint32>(static_cast<quint32>(body.size()));

    QByteArray frame;
    frame.append(reinterpret_cast<const char*>(&be), 4);
    frame.append(body);
    socket->write(frame);
}
