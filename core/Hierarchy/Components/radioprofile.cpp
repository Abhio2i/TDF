#include "radioprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/hierarchy.h"
RadioProfile::RadioProfile(Hierarchy* h):Component(h) {
    Active = true;
    radios =  new std::unordered_map<std::string, Radio*>();
}

void RadioProfile::addSubComponent(std::string name, QString data1, QString data2, QString data3){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    Radio* radio = new Radio(parent);
    if(!data2.isEmpty()){
        std::string id = radio->ID;
        QJsonObject obj = (*parent->Radios)[data2.toStdString()]->toJson();
        radio->fromJson(obj);
        radio->ID = id;
        radio->parentID = parentID;
    }
    radio->parentEntity = parentEntity;
    radio->Name = name;
    radios->insert({radio->ID,radio});
    parent->Radios->insert({radio->ID,radio});
    emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radio->ID),QString::fromStdString(name));

}

void RadioProfile::removeSubComponent(std::string ID){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = radios->find(ID);
    if (it != radios->end() && it->second != nullptr) {
        Radio *radio = it->second;
        emit parent->subComponentRemoved(QString::fromStdString(this->ID),QString::fromStdString(ID),QString::fromStdString(radio->Name));
        radios->erase(radio->ID);
        parent->Radios->erase(radio->ID);
        delete radio;
    }
}

void RadioProfile::updateSubComponent(std::string ID, const QJsonObject& obj){
    auto it = radios->find(ID);
    if (it != radios->end() && it->second != nullptr) {
        it->second->fromJson(obj);
    }

}

QJsonObject RadioProfile::getsubComponentData(std::string ID) const{
    auto it = radios->find(ID);
    if (it != radios->end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject(); // Return empty object if not found
}

QJsonObject RadioProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    QJsonObject radioObj;
    for (const auto& [key, radioPtr] : *radios) {
        if (radioPtr) {
            radioObj[QString::fromStdString(key)] = radioPtr->toJson();
        }
    }
    obj["radios"] = radioObj;
    return obj;
}

void RadioProfile::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        // ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("radios") && obj["radios"].isObject()) {
        QJsonObject radiosObj = obj["radios"].toObject();
        for (const QString& key : radiosObj.keys()) {
            QJsonObject radioObj = radiosObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            std::string id = radioObj["id"].toString().toStdString();

            Radio *radio;
            bool is = radios->count(id);
            if ( is > 0) {
                radio = (*radios)[id];   // OR iffs->at(id)

            } else {
                radio = new Radio(parent);
                radio->parentEntity = parentEntity;
            }
            radio->Name = radioObj["name"].toString().toStdString();
            radio->ID = radioObj["id"].toString().toStdString();
            radio->fromJson(radioObj);
            if (!is) {
                radios->insert({radio->ID,radio});
                parent->Radios->insert({radio->ID,radio});
                emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radio->ID),QString::fromStdString(radio->Name));

            }
        }
    }
    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
void RadioProfile::renameSubComponent(std::string id, QString newName) {
    auto it = radios->find(id);
    if (it != radios->end() && it->second != nullptr) {
        Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parent->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
