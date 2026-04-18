#include "iffprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/hierarchy.h"

IFFProfile::IFFProfile(Hierarchy* h):Component(h) {
    Active = true;
    iffs =  new std::unordered_map<std::string, IFF*>();
}

void IFFProfile::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    IFF* iff = new IFF(parent);
    if(!data2.isEmpty()){
        std::string id = iff->ID;
        iff->fromJson(data3);
        iff->ID = id;
        iff->parentID = parentID;
    }
    iff->parentEntity = parentEntity;
    iff->Name = name;
    iffs->insert({iff->ID,iff});
    parent->Iffs.insert({iff->ID,iff});
    emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(iff->ID),QString::fromStdString(name));

}

void IFFProfile::removeSubComponent(std::string ID){
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = iffs->find(ID);
    if (it != iffs->end() && it->second != nullptr) {
        IFF *iff = it->second;
        emit parent->subComponentRemoved(QString::fromStdString(this->ID),QString::fromStdString(ID),QString::fromStdString(iff->Name));
        iffs->erase(iff->ID);
        parent->Iffs.erase(iff->ID);
        delete iff;
    }
}

void IFFProfile::updateSubComponent(std::string ID, const QJsonObject& obj){
    auto it = iffs->find(ID);
    if (it != iffs->end() && it->second != nullptr) {
        it->second->fromJson(obj);
    }

}

QJsonObject IFFProfile::getsubComponentData(std::string ID) const{
    auto it = iffs->find(ID);
    if (it != iffs->end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject(); // Return empty object if not found
}

QJsonObject IFFProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;

    QJsonObject iffObj;
    for (const auto& [key, iffPtr] : *iffs) {
        if (iffPtr) {
            iffObj[QString::fromStdString(key)] = iffPtr->toJson();
        }
    }
    obj["iffs"] = iffObj;
    return obj;
}

void IFFProfile::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        //ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("iffs") && obj["iffs"].isObject()) {
        QJsonObject iffsObj = obj["iffs"].toObject();
        for (const QString& key : iffsObj.keys()) {
            QJsonObject iffObj = iffsObj[key].toObject();
            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            std::string id = iffObj["id"].toString().toStdString();

            IFF *iff;
            bool is = iffs->count(id);
            if ( is > 0) {
                iff = (*iffs)[id];   // OR iffs->at(id)

            } else {
                iff = new IFF(parent);
            }
            iff->parentEntity = parentEntity;
            iff->Name = iffObj["name"].toString().toStdString();
            iff->ID = iffObj["id"].toString().toStdString();
            iff->fromJson(iffObj);
            if (!is) {
                iffs->insert({iff->ID,iff});
                parent->Iffs.insert({iff->ID,iff});
                emit parent->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(iff->ID),QString::fromStdString(iff->Name));

            }
        }
    }

    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
void IFFProfile::renameSubComponent(std::string id, QString newName) {
    auto it = iffs->find(id);
    if (it != iffs->end() && it->second != nullptr) {
        Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parent->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
