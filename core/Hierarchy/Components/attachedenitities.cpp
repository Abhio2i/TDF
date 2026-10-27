#include "attachedenitities.h"

AttachedEnitities::AttachedEnitities():Component() {}

void AttachedEnitities::init(Entity *Entity){
    parentEntity = Entity;
}
void AttachedEnitities::Awake(){}
void AttachedEnitities::Start(){}

QJsonObject AttachedEnitities::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject AttachedEnitities::toJson() const {
    return QJsonObject();
}

void AttachedEnitities::fromJson(const QJsonObject& obj) {
}
