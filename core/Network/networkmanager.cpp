
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

// std::unique_ptr<Server> NetworkManager::ser = nullptr;
// std::unique_ptr<Client> NetworkManager::cli = nullptr;
std::atomic<bool> NetworkManager::serverRunning{false};
std::atomic<bool> NetworkManager::clientRunning{false};


NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {

    network = new NetworkTransport();
    connect(network,&NetworkTransport::onConnect,this,&NetworkManager::onConnect);
    connect(network,&NetworkTransport::onNewConnection,this,&NetworkManager::onNewConnction);
    connect(network,&NetworkTransport::onReceivedMessage,this,&NetworkManager::onMessaageRecevied);
}

QString getLocalIP() {
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && addr != QHostAddress::LocalHost)
            return addr.toString();
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}

void NetworkManager::init(const QString& ip, int port){
    network->init(ip,port);
}

bool NetworkManager::startServer(int port) {
    network->start(true);
    return true;
}

void NetworkManager::onNewConnction(){

}

void NetworkManager::onConnect(){
    network->sendMessage("give me");
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

bool NetworkManager::startClient() {
    network->start();
    return true;
}

void NetworkManager::getJsonData(const QJsonObject& obj){
    QJsonObject copy = obj;
    copy["role"] = "init";
    copy["type"] = "data";
    sendJson(copy);
}

void NetworkManager::sendJson(const QJsonObject& obj) {
    if(!network->isServer()) return;
    QJsonDocument doc(obj);
    QString msg = doc.toJson(QJsonDocument::Compact);
    std::string stdMsg = msg.toStdString();
    std::cout << "Log: " << stdMsg << std::endl;
    //qDebug() << "[NetworkManager JSON] Sending:" << &stdMsg;
    network->sendMessage(msg);
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
void NetworkManager::UpdateClient(){
    if(!network->isServer()) return;
    QJsonObject msg;
    msg["role"] = "update"; // Corrected typo: "upate" -> "update"
    msg["type"] = "frame";

    QJsonObject delta;
    for (auto& [key, entity] : *hierarchy->Entities)
    {
        Platform* platform = dynamic_cast<Platform*>(entity);
        if (platform) {
            QVector3D pos = platform->transform->translation();

            // --- FIX: Use QJsonArray for the position coordinates ---
            QJsonArray positionArray;
            positionArray.append(pos.x());
            positionArray.append(pos.y());
            positionArray.append(pos.z());
            positionArray.append(platform->transform->toEulerAngles().y());

            delta[QString::fromStdString(key)] = positionArray;
        }
    }
    msg["delta"] = delta;
    QJsonDocument doc(msg);
    QString msgs = doc.toJson(QJsonDocument::Compact);
    std::string stdMsg = msgs.toStdString();
    std::cout << "Log: " << stdMsg << std::endl;
    //qDebug() << "[NetworkManager JSON] Sending:" << &stdMsg;
    network->sendUDPMessage(msgs);

    //emit this->getCurrentJsonData();
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

