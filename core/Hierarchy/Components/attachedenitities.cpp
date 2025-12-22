#include "attachedenitities.h"

AttachedEnitities::AttachedEnitities():Component(nullptr) {}

QJsonObject AttachedEnitities::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject AttachedEnitities::toJson() const {
    return QJsonObject();
}

void AttachedEnitities::fromJson(const QJsonObject& obj) {
}
