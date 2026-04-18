#include "networkobject.h"

NetworkObject::NetworkObject():Component(nullptr) {}

void NetworkObject::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

}

void NetworkObject::removeSubComponent(std::string ID){

}

void NetworkObject::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject NetworkObject::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject NetworkObject::toJson() const {
    return QJsonObject();
}

void NetworkObject::fromJson(const QJsonObject& obj) {
}
