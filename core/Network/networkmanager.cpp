//Author::Aman Negi
#include "networkmanager.h"
#include "qjsonarray.h"
#include <iostream>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QJsonObject>//>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QtEndian> // for qToLittleEndian / qFromLittleEndian

extern "C" {
#include "lz4.h"
}



std::atomic<bool> NetworkManager::serverRunning{false};
std::atomic<bool> NetworkManager::clientRunning{false};

/// @brief Initializes the NetworkManager and its networking thread
/// @details Creates the NetworkTransport, connects all signals/slots, and runs it in a dedicated worker thread.

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {

    // 1) Create thread and transport
    networkThread = new QThread(this);
    network = new NetworkTransport();          // constructed in main thread
    network->moveToThread(networkThread);      // but lives in networkThread

    // 2) Clean up transport when thread finishes
    connect(networkThread, &QThread::finished,
            network, &QObject::deleteLater);

    // 3) Incoming signals FROM transport → manager
    connect(network, &NetworkTransport::onConnect,
            this, &NetworkManager::onConnect);
    connect(network, &NetworkTransport::onNewConnection,
            this, &NetworkManager::onNewConnction);
    connect(network, &NetworkTransport::onReceivedMessage,
            this, &NetworkManager::onMessaageRecevied);
    connect(network, &NetworkTransport::onBinaryMessage,
            this, &NetworkManager::onBinaryMessage);

    // 4) Outgoing commands FROM manager → transport (Queued!)
    connect(this, &NetworkManager::requestInit,
            network, &NetworkTransport::init,
            Qt::QueuedConnection);

    connect(this, &NetworkManager::requestStart,
            network, &NetworkTransport::start,
            Qt::QueuedConnection);

    connect(this, &NetworkManager::requestSendText,
            network, &NetworkTransport::sendMessage,
            Qt::QueuedConnection);

    connect(this, &NetworkManager::requestSendBinary,
            network, &NetworkTransport::sendBinaryUDPMessage,
            Qt::QueuedConnection);

    // 5) Start the worker thread
    networkThread->start();
}

/// @brief Safely shuts down the networking thread
/// @details Requests the worker thread to quit and waits for it to finish before destruction.
NetworkManager :: ~NetworkManager() {
    if (networkThread && networkThread->isRunning()) {
        networkThread->quit();
        networkThread->wait();
    }
}

/// @brief Returns the machine’s local IPv4 address
/// @details Selects the first non-localhost IPv4 address, or falls back to 127.0.0.1 if none is found.
QString getLocalIP() {
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost)
            return addr.toString();
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

/// @brief Returns the current status of all connected clients
/// @details Builds a list of strings containing each client’s node ID, IP address, and connection state.
QStringList NetworkManager::getNetworkStatus() const
{
    QStringList list;

    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        const ClientInfo& c = it.value();

        list << QString("%1,%2,Connected,—")
                    .arg(c.nodeId)
                    .arg(c.address.toString());
    }

    return list;
}

/// @brief Initializes the network transport with the given IP and port
/// @details Forwards the request to the NetworkTransport via a queued signal for thread-safe execution.
void NetworkManager::init(const QString& ip, int port){
    emit requestInit(ip, port);   // instead of network->init(...)
}

/// @brief Starts the network in server mode
/// @details Marks the session as active and requests the transport layer to start a WebSocket server.
bool NetworkManager::startServer(int port) {
    networkActive = true;
    Q_UNUSED(port);               // you already pass it via init()
    sessionActive = true;   // 🔥 session running by Aman
    emit requestStart(true);      // instead of network->start(true);
    return true;
}

/// @brief Starts a new network session
/// @details Enables session state if the network layer is already active.
bool NetworkManager::startSession()
{
    if (!networkActive) return false;
    sessionActive = true;
    //emit networkStatusChanged();
    return true;
}

/// @brief Stops (pauses) the current network session
/// @details Disables session activity while keeping the network layer running.
bool NetworkManager::stopSession()
{
    if (!networkActive)
        return false;

    qDebug() << "[SESSION] Paused";

    sessionActive = false;      // 🔥 THIS is the magic
    //emit networkStatusChanged();

    return true;
}

/// @brief Handles a newly connected WebSocket client
/// @details Assigns a unique client ID, stores connection details, and logs the connection.
void NetworkManager::onNewConnction(QWebSocket* socket){
    QString id = QString("C%1").arg(nextClientId++, 3, 10, QChar('0'));

    ClientInfo info;
    info.nodeId = id;
    info.address = socket->peerAddress();
    info.port = socket->peerPort();
    info.connectedAt = QDateTime::currentDateTime();

    clients[socket] = info;

    qDebug() << "[SERVER] Client"
             << id
             << info.address.toString()
             << ":" << info.port;
}

/// @brief Called when the WebSocket connection is successfully established
/// @details Marks the client as connected and sends an initial handshake message.
void NetworkManager::onConnect(){
    connected = true;
    emit requestSendText("give me");   // instead of network->sendMessage("give me");

}

/// @brief Processes incoming text messages from the network
/// @details Parses JSON commands and dispatches scene, entity, and sync updates to the application.
void NetworkManager::onMessaageRecevied(QString message) {
    if(message.contains("give me")){
        emit getCurrentJsonData();
        return;
    }
    qDebug() << "[Client] Message received:" << message;

    QByteArray byteArray = QByteArray::fromStdString(message.toStdString());
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "[Client] Failed to parse JSON:" << parseError.errorString();

    }else{
        qDebug() << "done";
        QJsonObject obj = doc.object();
        QString role = obj.value("role").toString();
        QString type = obj.value("type").toString();
        qDebug() <<"work2";
        if (role == "init" && type == "data") {
            emit initData(obj); // Only pass expected 3 arguments
            emit initSyncComplete();


        }else
            if(role == "add" && type == "folder") {
                QString parentID = obj.value("parentID").toString();
                QString ID = obj.value("id").toString();
                QString name = obj.value("name").toString();
                qDebug() <<"work";
                emit addFolder(parentID,ID, name, true);  // Only pass expected 3 arguments
            }else
                if(role == "remove" && type == "folder") {
                    QString Id = obj.value("id").toString();
                    qDebug() <<"work remove entity";
                    emit removeFolder(Id);  // Only pass expected 3 arguments
                }else
                    if(role == "add" && type == "entity") {
                        QString parentID = obj.value("parentID").toString();
                        QString ID = obj.value("id").toString();
                        QString name = obj.value("name").toString();
                        qDebug() <<"work";
                        emit addEntity(parentID,ID, name, true);  // Only pass expected 3 arguments

                    }else
                        if(role == "add" && type == "entityJson") {
                            QString parentID = obj.value("parentID").toString();
                            QString name = obj.value("name").toString();
                            qDebug() <<"work entityjson";
                            emit addEntityFromJson(parentID, obj, false);  // Only pass expected 3 arguments
                        }else
                            if(role == "add" && type == "component") {
                                QString Id = obj.value("id").toString();
                                QString name = obj.value("name").toString();
                                qDebug() <<"work component";
                                emit addComponent(Id, name);  // Only pass expected 3 arguments
                            }else
                                if(role == "update" && type == "component") {
                                    QString Id = obj.value("id").toString();
                                    QString name = obj.value("name").toString();
                                    QJsonObject delta = obj.value("delta").toObject();
                                    qDebug() <<"work component";
                                    emit entityComponentUpdate(Id, name, delta);  // Only pass expected 3 arguments
                                }else
                                    if(role == "remove" && type == "entity") {
                                        QString Id = obj.value("id").toString();
                                        QString parentId = obj.value("parentId").toString();
                                        bool profile = obj.value("Profile").toBool();
                                        qDebug() <<"work remove entity";
                                        emit removeEntity(parentId,Id, profile);  // Only pass expected 3 arguments
                                    }else
                                        if (role == "rename" && type == "entity") {
                                            QString id = obj.value("id").toString();
                                            QString newName = obj.value("newName").toString();

                                            qDebug() << "Renaming entity" << id << "to" << newName;

                                            emit renameEntity(id, newName);
                                        }else
                                            if(role == "update" && type == "frame") {
                                                QJsonObject delta = obj.value("delta").toObject();
                                                for (auto it = delta.begin(); it != delta.end(); ++it)
                                                {
                                                    QString entityID = it.key();
                                                    QJsonValue positionValue = it.value();

                                                    if (!positionValue.isArray()) {
                                                        qWarning() << "Position data for entity" << entityID << "is not an array.";
                                                        continue;
                                                    }

                                                    QJsonArray positionArray = positionValue.toArray();

                                                    if (positionArray.size() < 3) {
                                                        qWarning() << "Position array for entity" << entityID << "does not have 3 components (x, y, z).";
                                                        continue;
                                                    }
                                                    QVector3D newPos(
                                                        (float)positionArray.at(0).toDouble(), // X
                                                        (float)positionArray.at(1).toDouble(), // Y
                                                        (float)positionArray.at(2).toDouble()  // Z
                                                        );
                                                    QVector3D rot(0,(float)positionArray.at(3).toDouble(),0);

                                                    auto entityMap = hierarchy->Entities;
                                                    if (entityMap.count(entityID.toStdString())) {
                                                        Entity* entity = entityMap.at(entityID.toStdString());
                                                        Platform* platform = dynamic_cast<Platform*>(entity);
                                                        if (platform) {
                                                            if (platform->transform) {
                                                                platform->transform->setTranslation(newPos);
                                                                platform->transform->setFromEulerAngles(rot);
                                                            } else {
                                                                qWarning() << "Platform" << entityID << "is missing a Transform component.";
                                                            }
                                                        }
                                                    } else {
                                                        //qWarning() << "Entity with ID" << entityID << "not found in hierarchy.";
                                                    }
                                                }
                                            }
        if(!network->isServer())emit updateScene(0.01f);

    }

}


/// @brief Handles incoming binary packets containing compressed transform updates
/// @details Decompresses LZ4 data, decodes PDUs, and emits per-entity transform updates for scene synchronization.
void NetworkManager::onBinaryMessage(QByteArray packet)
{
    if (!sessionActive) return;   // 🔥 freeze remote updates

    QElapsedTimer timer;
    timer.start();
    static int recvCounter = 0;
    static qint64 lastRecvTime = 0;

    constexpr int HEADER_SIZE = sizeof(quint32);
    constexpr int MAX_RAW_SIZE = 50 * 1024 * 1024; // 50 MB safety cap (tune as needed)

    // Packet must contain header + payload
    if (packet.size() < HEADER_SIZE) {
        qWarning() << "Received packet too small:" << packet.size();
        return;
    }

    // Read 4-byte little-endian raw (uncompressed) size
    quint32 rawSizeLE = 0;
    memcpy(&rawSizeLE, packet.constData(), HEADER_SIZE);
    quint32 rawSize = qFromLittleEndian(rawSizeLE);

    if (rawSize == 0 || rawSize > static_cast<quint32>(MAX_RAW_SIZE)) {
        qWarning() << "Invalid or too large rawSize in packet:" << rawSize;
        return;
    }

    // Compressed payload follows header
    const char* compData = packet.constData() + HEADER_SIZE;
    int compSize = packet.size() - HEADER_SIZE;
    if (compSize <= 0) {
        qWarning() << "No compressed payload in packet";
        return;
    }

    // Allocate exact buffer for decompression
    QByteArray decompressed;
    decompressed.resize(static_cast<int>(rawSize));

    // Decompress
    int decResult = LZ4_decompress_safe(compData, decompressed.data(), compSize, static_cast<int>(rawSize));
    if (decResult < 0) {
        qWarning() << "LZ4 decompression failed (code =" << decResult << "), compSize =" << compSize << ", expectedRaw =" << rawSize;
        return;
    }

    // Resize to actual decompressed size (should normally equal rawSize)
    decompressed.resize(decResult);

    // Now decode PDUs from decompressed data
    DIS::DataStream ds(decompressed.constData(), decompressed.size(), DIS::BIG);

    while (ds.size() > ds.GetReadPos()) {
        TransformPDU pdu;
        pdu.unmarshalCompact(ds);

        TransformUpdate msg;
        msg.id  = pdu.entityID;
        msg.pos = QVector3D(pdu.posX, pdu.posY, pdu.posZ);
        msg.rot = QVector3D(pdu.rotX, pdu.rotY, pdu.rotZ);

        emit transformReceived(msg);
    }

    if (!network->isServer())
        emit updateScene(0.01f);
    qint64 ns = timer.nsecsElapsed();
    double ms = ns / 1'000'000.0;
    qDebug() << "[onBinaryMessage] Time taken =" << ms << "ms";
}

/// @brief Starts the network in client mode
/// @details Activates the session and requests the transport layer to connect to the remote server.
bool NetworkManager::startClient() {
    networkActive = true;
    sessionActive = true;     // 🔥 client is now participating in the session
    emit requestStart(false);     // instead of network->start();
    return true;
}

/// @brief Sends the initial scene data to a connected peer
/// @details Tags the JSON object as initialization data and forwards it to the network layer.
void NetworkManager::getJsonData(const QJsonObject& obj){
    QJsonObject copy = obj;
    copy["role"] = "init";
    copy["type"] = "data";
    sendJson(copy);
}

/// @brief Sends a JSON message to all connected clients
/// @details Serializes the object and forwards it through the server only when the session is active.
void NetworkManager::sendJson(const QJsonObject& obj) {
    if (!sessionActive) return;     // 🔥 block all replication

    if(!network->isServer()) return;  // isServer() just reads a bool → ok

    QJsonDocument doc(obj);
    QString msg = doc.toJson(QJsonDocument::Compact);
    std::string stdMsg = msg.toStdString();
    std::cout << "Log: " << stdMsg << std::endl;

    emit requestSendText(msg);        // instead of network->sendMessage(msg);
}





// Stub implementations for signals/serialization
// void NetworkManager::toJson() {}
// void NetworkManager::fromJson() {}
// void NetworkManager::profileAddedPointer(ProfileCategaory*) {}
// void NetworkManager::folderAddedPointer(QString, Folder*) {}
// void NetworkManager::entityAddedPointer(QString, Entity*) {}
// void NetworkManager::profileAdded(QString, QString) {}
// void NetworkManager::folderAdded(QString, QString, QString) {}
// void NetworkManager::entityAdded(QString, QString, QString) {}
// void NetworkManager::componentAdded(QString, QString) {}
// void NetworkManager::profileRemoved(QString) {}
// void NetworkManager::folderRemoved(QString) {}
// void NetworkManager::entityRemoved(QString) {}
// void NetworkManager::componentRemoved(QString, QString) {}
// void NetworkManager::profileRenamed(QString, QString) {}
// void NetworkManager::folderRenamed(QString, QString) {}
// void NetworkManager::entityRenamed(QString, QString) {}
// void NetworkManager::entityMeshAdded(QString, Entity*) {}
// void NetworkManager::entityMeshRemoved(QString) {}
// void NetworkManager::entityPhysicsAdded(QString, Entity*) {}
// void NetworkManager::entityPhysicsRemoved(QString) {}
// void NetworkManager::entityUpdate(QString) {}


// Serialization
/// @brief Serializes the current network state to a JSON file
/// @details Writes a placeholder status object to "network_state.json" for persistence or debugging.
void NetworkManager::toJson() {
    QJsonObject obj;
    obj["status"] = "placeholder";

    QJsonDocument doc(obj);
    QFile file("network_state.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

/// @brief Collects and broadcasts changed entity transforms to all clients
/// @details Serializes, compresses, and sends only modified transform PDUs from the server for efficient synchronization.
void NetworkManager::UpdateClient() {
    if (!sessionActive) return;   // 🔥 freeze remote updates

    QElapsedTimer timer;
    timer.start();
    static int frameCounter = 0;
    static qint64 lastTimeMs = 0;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    frameCounter++;

    if (now - lastTimeMs >= 1000) {  // every 1 second
        // qDebug() << "[SERVER NET FPS] Packets per second =" << frameCounter;
        frameCounter = 0;
        lastTimeMs = now;
    }
    if (!network || !hierarchy) return;
    // This method broadcasts from server to clients, require server mode.
    if (!network->isServer()) return;

    DIS::DataStream batchStream(DIS::BIG);

    for (auto& [key, entity] : hierarchy->Entities) {
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform || !platform->transform) continue;

        TransformPDU pdu;
        platform->transform->toPDU(pdu, key, platform->parentID);

        TransformState current{
            pdu.posX, pdu.posY, pdu.posZ,
            pdu.rotX, pdu.rotY, pdu.rotZ,
            pdu.sizeX, pdu.sizeY, pdu.sizeZ
        };

        auto it = previousTransforms.find(key);
        bool changed = true;

        if (it != previousTransforms.end()) {
            const TransformState& prev = it->second;
            constexpr float epsilon = 0.0001f;
            changed = (
                std::abs(prev.posX - current.posX) > epsilon ||
                std::abs(prev.posY - current.posY) > epsilon ||
                std::abs(prev.posZ - current.posZ) > epsilon ||
                std::abs(prev.rotX - current.rotX) > epsilon ||
                std::abs(prev.rotY - current.rotY) > epsilon ||
                std::abs(prev.rotZ - current.rotZ) > epsilon ||
                std::abs(prev.sizeX - current.sizeX) > epsilon ||
                std::abs(prev.sizeY - current.sizeY) > epsilon ||
                std::abs(prev.sizeZ - current.sizeZ) > epsilon
                );
        }

        if (changed) {
            pdu.marshalCompact(batchStream);
            previousTransforms[key] = current;
        }
    }

    // Send only if we serialized something
    if (batchStream.size() > 0) {
        // Convert batchStream to a contiguous raw buffer.
        // NOTE: original code used &batchStream[0]; adjust if DIS::DataStream API differs.
        const char* rawData = reinterpret_cast<const char*>(&batchStream[0]);
        int rawSize = static_cast<int>(batchStream.size());

        // Safety guard
        constexpr int MAX_RAW_SIZE = 50 * 1024 * 1024;
        if (rawSize <= 0 || rawSize > MAX_RAW_SIZE) {
            qWarning() << "Raw batchStream size invalid or too large:" << rawSize;
            return;
        }

        // Compress (reserve space for header + compressed payload)
        int maxCompressedSize = LZ4_compressBound(rawSize);
        QByteArray packet;
        packet.resize(sizeof(quint32) + maxCompressedSize); // header + max compressed

        // Compress into packet after header
        int compressedSize = LZ4_compress_default(rawData, packet.data() + sizeof(quint32), rawSize, maxCompressedSize);
        if (compressedSize <= 0) {
            qWarning() << "LZ4 compression failed for raw size" << rawSize;
            return;
        }

        // Write raw size header in little-endian
        quint32 rawSizeLE = qToLittleEndian(static_cast<quint32>(rawSize));
        memcpy(packet.data(), &rawSizeLE, sizeof(quint32));

        // Trim packet to actual size (header + compressed payload)
        packet.resize(sizeof(quint32) + compressedSize);

        // IMPORTANT: avoid UDP fragmentation. If packet.size() > MTU, consider splitting.
        const int MTU_LIMIT = 1200; // conservative
        if (packet.size() > MTU_LIMIT) {
            // Either implement fragmentation or log warning. For now log.
            qWarning() << "Packet size" << packet.size() << "exceeds MTU limit" << MTU_LIMIT << "- consider fragmenting";
        }

        // Send packet (NetworkTransport::sendBinaryUDPMessage should forward it unchanged)
        // Send packet (NetworkTransport::sendBinaryUDPMessage should forward it unchanged)
        // network->sendBinaryUDPMessage(packet);
        emit requestSendBinary(packet);

        qint64 ns = timer.nsecsElapsed();
        double ms = ns / 1'000'000.0;
        qDebug() << "[updateClient] Time taken =" << ms << "ms";

    }
}

/// @brief Loads the network state from a JSON file
/// @details Reads and parses "network_state.json" to restore previously saved state.
void NetworkManager::fromJson() {
    QFile file("network_state.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject obj = doc.object();
    // qDebug() << "Loaded state:" << QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

// Pointer-based (stubs)
void NetworkManager::profileAddedPointer(ProfileCategaory*) {}
void NetworkManager::folderAddedPointer(QString parentID, Folder*) {}
/// @brief Sends a newly added entity to connected clients
/// @details Converts the entity to JSON, tags it as an add operation, and transmits it for replication.
void NetworkManager::entityAddedPointer(QString parentID, Entity* entity) {
    QJsonObject msg = entity->toJson();
    msg["role"] = "add";
    msg["type"] = "entityJson";
    msg["parentID"] = parentID;
    sendJson(msg);
}

/// @brief Broadcasts a newly added profile to connected clients
/// @details Packages the profile information into JSON and sends it for network replication.
void NetworkManager::profileAdded(QString ID, QString profileName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["name"] = profileName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Notifies clients about a newly created folder
/// @details Builds and sends a JSON message describing the folder and its parent for replication.
void NetworkManager::folderAdded(QString parentID, QString ID, QString folderName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "folder";
    msg["id"] = ID;
    msg["parentID"] = parentID;
    msg["name"] = folderName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Broadcasts a newly created entity to connected clients
/// @details Packages the entity ID, name, and parent into a JSON message for synchronization.
void NetworkManager::entityAdded(QString parentID, QString ID, QString entityName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["parentID"] = parentID;
    msg["name"] = entityName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Notifies clients that a component was added to an entity
/// @details Sends a JSON message describing the new component for network replication.
void NetworkManager::componentAdded(QString parentId,QString Id, QString componentName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "component";
    msg["id"] = parentId;
    msg["name"] = componentName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Sends an update for a modified entity component
/// @details Packages the component delta into JSON and broadcasts it to all connected clients.
void NetworkManager::entityComponentsUpdate(QString ID, QString componentName, QJsonObject delta)
{
    QJsonObject msg;
    msg["role"] = "update";
    msg["type"] = "component";
    msg["id"] = ID;
    msg["delta"] = delta;
    msg["name"] = componentName;
    sendJson(msg);
}

/// @brief Broadcasts the removal of a profile
/// @details Sends a JSON message identifying the removed profile for client-side synchronization.
void NetworkManager::profileRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "profile";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Notifies clients that a folder was removed
/// @details Sends a JSON message identifying the removed folder for replication.
void NetworkManager::folderRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "folder";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Broadcasts the removal of an entity
/// @details Sends the entity ID, parent, and profile flag so clients can remove it correctly.
void NetworkManager::entityRemoved(QString parentId,QString ID,bool Profile) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["parentId"] = parentId;
    msg["Profile"] = Profile;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Notifies clients that a component was removed from an entity
/// @details Sends a JSON message identifying the parent entity and removed component.
void NetworkManager::componentRemoved(QString parentID, QString componentName) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "component";
    msg["parentID"] = parentID;
    msg["name"] = componentName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Broadcasts a profile rename operation
/// @details Sends the profile ID and new name so clients can update their local state.
void NetworkManager::profileRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["newName"] = name;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Prepares a folder rename message for network replication
/// @details Builds the JSON payload with the folder ID and new name (sending currently disabled).
void NetworkManager::folderRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "folder";
    msg["id"] = ID;
    msg["newName"] = name;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Broadcasts an entity rename operation
/// @details Sends the entity ID and new name so clients can update their local state.
void NetworkManager::entityRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["newName"] = name;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

/// @brief Notifies clients that a mesh was added to an entity
/// @details Sends a JSON message identifying the affected entity for replication.
void NetworkManager::entityMeshAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    sendJson(msg);
}

/// @brief Broadcasts the removal of an entity’s mesh
/// @details Sends a JSON message so clients can remove the mesh from the specified entity.
void NetworkManager::entityMeshRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    sendJson(msg);
}

/// @brief Notifies clients that a physics component was added to an entity
/// @details Sends a JSON message identifying the affected entity for synchronization.
void NetworkManager::entityPhysicsAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    sendJson(msg);
}

/// @brief Broadcasts the removal of an entity’s physics component
/// @details Sends a JSON message so clients can remove physics from the specified entity.
void NetworkManager::entityPhysicsRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    sendJson(msg);
}

/// @brief Sends an update notification for an entity
/// @details Informs clients that the specified entity has changed and should be refreshed.
void NetworkManager::entityUpdate(QString ID) {
    QJsonObject msg;
    msg["role"] = "update";
    msg["type"] = "entity";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}
