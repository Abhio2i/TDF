#include "mission.h"

Mission::Mission():Component() {}

void Mission::init(Entity *Entity){
    parentEntity = Entity;
}
void Mission::Awake(){}
void Mission::Start(){}
void Mission::addSubComponent(std::string name, QString data1, QString data2 , QJsonObject data3){

}

void Mission::removeSubComponent(std::string ID){

}

void Mission::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject Mission::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject Mission::toJson() const {
    return QJsonObject();
}

void Mission::fromJson(const QJsonObject& obj) {
}
