#include "networkobject.h"

NetworkObject::NetworkObject() {}

QJsonObject NetworkObject::toJson() const {
    return QJsonObject();
}

void NetworkObject::fromJson(const QJsonObject& obj) {
}
