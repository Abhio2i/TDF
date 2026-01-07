
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


// std::unique_ptr<Server> NetworkManager::ser = nullptr;
// std::unique_ptr<Client> NetworkManager::cli = nullptr;
std::atomic<bool> NetworkManager::serverRunning{false};
std::atomic<bool> NetworkManager::clientRunning{false};


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

NetworkManager :: ~NetworkManager() {
    if (networkThread && networkThread->isRunning()) {
        networkThread->quit();
        networkThread->wait();
    }
}

QString getLocalIP() {
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost)
            return addr.toString();
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

void NetworkManager::init(const QString& ip, int port){
    emit requestInit(ip, port);   // instead of network->init(...)
}


bool NetworkManager::startServer(int port) {
    networkActive = true;
    Q_UNUSED(port);               // you already pass it via init()
    emit requestStart(true);      // instead of network->start(true);
    return true;
}

void NetworkManager::onNewConnction(){

}

void NetworkManager::onConnect(){
    connected = true;
    emit requestSendText("give me");   // instead of network->sendMessage("give me");

}

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
                                                    if (entityMap->count(entityID.toStdString())) {
                                                        Entity* entity = entityMap->at(entityID.toStdString());
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

void NetworkManager::onBinaryMessage(QByteArray packet)
{

    QElapsedTimer timer;
    timer.start();
    static int recvCounter = 0;
    static qint64 lastRecvTime = 0;

    // qint64 now = QDateTime::currentMSecsSinceEpoch();
    // recvCounter++;

    // if (now - lastRecvTime >= 1000) {
    //     std::cerr << "[CLIENT NET FPS] =" << recvCounter;
    //     recvCounter = 0;
    //     lastRecvTime = now;
    // }

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

        // QString entityID = QString::fromStdString(pdu.entityID);
        // auto entityMap = hierarchy->Entities;
        // if (!entityMap->count(pdu.entityID)) continue;

        // Entity* entity = entityMap->at(pdu.entityID);
        // Platform* platform = dynamic_cast<Platform*>(entity);
        // if (!platform || !platform->transform) continue;

        // QVector3D newPos(pdu.posX, pdu.posY, pdu.posZ);
        // QVector3D newRot(pdu.rotX, pdu.rotY, pdu.rotZ);

        // platform->transform->setTranslation(newPos);
        // platform->transform->setFromEulerAngles(newRot);
        //platform->update();

        // qDebug() << "Updated entity" << entityID
        //          << "Pos:" << newPos
        //          << "Rot:" << newRot;
    }

    if (!network->isServer())
        emit updateScene(0.01f);
    qint64 ns = timer.nsecsElapsed();
    double ms = ns / 1'000'000.0;
    qDebug() << "[onBinaryMessage] Time taken =" << ms << "ms";
}


bool NetworkManager::startClient() {
    networkActive = true;
    emit requestStart(false);     // instead of network->start();
    return true;
}


void NetworkManager::getJsonData(const QJsonObject& obj){
    QJsonObject copy = obj;
    copy["role"] = "init";
    copy["type"] = "data";
    sendJson(copy);
}

void NetworkManager::sendJson(const QJsonObject& obj) {
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


void NetworkManager::UpdateClient() {
    QElapsedTimer timer;
    timer.start();
    static int frameCounter = 0;
    static qint64 lastTimeMs = 0;

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    frameCounter++;

    if (now - lastTimeMs >= 1000) {  // every 1 second
        qDebug() << "[SERVER NET FPS] Packets per second =" << frameCounter;
        frameCounter = 0;
        lastTimeMs = now;
    }
    if (!network || !hierarchy) return;
    // This method broadcasts from server to clients, require server mode.
    if (!network->isServer()) return;

    DIS::DataStream batchStream(DIS::BIG);

    for (auto& [key, entity] : *hierarchy->Entities) {
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






void NetworkManager::fromJson() {
    QFile file("network_state.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject obj = doc.object();
    qDebug() << "Loaded state:" << QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

// Pointer-based (stubs)
void NetworkManager::profileAddedPointer(ProfileCategaory*) {}
void NetworkManager::folderAddedPointer(QString parentID, Folder*) {}
void NetworkManager::entityAddedPointer(QString parentID, Entity* entity) {
    QJsonObject msg = entity->toJson();
    msg["role"] = "add";
    msg["type"] = "entityJson";
    msg["parentID"] = parentID;
    sendJson(msg);
}

void NetworkManager::profileAdded(QString ID, QString profileName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["name"] = profileName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

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

void NetworkManager::componentAdded(QString Id, QString componentName) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "component";
    msg["id"] = Id;
    msg["name"] = componentName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

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

void NetworkManager::profileRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "profile";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::folderRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "folder";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

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

void NetworkManager::componentRemoved(QString parentID, QString componentName) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "component";
    msg["parentID"] = parentID;
    msg["name"] = componentName;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::profileRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "profile";
    msg["id"] = ID;
    msg["newName"] = name;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::folderRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "folder";
    msg["id"] = ID;
    msg["newName"] = name;
    //sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::entityRenamed(QString ID, QString name) {
    QJsonObject msg;
    msg["role"] = "rename";
    msg["type"] = "entity";
    msg["id"] = ID;
    msg["newName"] = name;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

void NetworkManager::entityMeshAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    sendJson(msg);
}

void NetworkManager::entityMeshRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "mesh";
    msg["entityID"] = ID;
    sendJson(msg);
}

void NetworkManager::entityPhysicsAdded(QString ID, Entity*) {
    QJsonObject msg;
    msg["role"] = "add";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    sendJson(msg);
}

void NetworkManager::entityPhysicsRemoved(QString ID) {
    QJsonObject msg;
    msg["role"] = "remove";
    msg["type"] = "physics";
    msg["entityID"] = ID;
    sendJson(msg);
}

void NetworkManager::entityUpdate(QString ID) {
    QJsonObject msg;
    msg["role"] = "update";
    msg["type"] = "entity";
    msg["id"] = ID;
    sendJson(msg);
    //emit this->getCurrentJsonData();
}

