#include "formation.h"
#include "qjsonarray.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/GlobalRegistry.h"
#include <core/Hierarchy/hierarchy.h>
#include <core/Debug/console.h>

Formation::Formation(Hierarchy* h)
    : Entity(h), formationType(Constants::FormationType::Line), count(0),
    mothership(nullptr), formationPositions(new std::unordered_map<std::string,FormationPosition*>())
{
}
void Formation::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
    addComponent("mothership");

}

std::vector<std::string> Formation::getSupportedComponents() {
    std::vector<std::string> supported;
    return supported;
}


QJsonObject Formation::toJson() const {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;
    obj["count"] = QString::number(count);



    if (mothership) obj["mothership"] = mothership->toJson();
    // QJsonArray formationObject;
    // for (FormationPosition* fo : *formationPositions) {
    //     if (fo) formationObject.append(fo->toJson());
    // }
    // obj["formationObject"] = formationObject;



    QJsonObject entityObj;
    entityObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : formationTypeOptions())
        optionsArray.append(opt);

    entityObj["options"] = optionsArray;
    entityObj["value"] = formationTypeToString(formationType);
    obj["type"] = entityObj;
    return obj;
}


void Formation::fromJson(const QJsonObject& obj) {
    if(obj.contains("name")){
        Name = obj["name"].toString().toStdString();
    }
    if(obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if(obj.contains("parent_id")){
        parentID = obj["parent_id"].toString().toStdString();
    }
    if(obj.contains("active")){
        Active = obj["active"].toBool();
    }
    if(obj.contains("count")){
        count = obj["count"].toString().toInt();  // Since count was stored as QString
    }






    // Deserialize mothership
    if (obj.contains("mothership") && obj["mothership"].isObject()) {
        if (!mothership) mothership = new FormationPosition();
        mothership->fromJson(obj["mothership"].toObject());
    }


    // Deserialize formation type
    if (obj.contains("type") && obj["type"].isObject()) {
        QJsonObject typeObj = obj["type"].toObject();
        QString typeVal = typeObj["value"].toString();
        formationType = stringToFormationType(typeVal);
        formationCreate();
    }
}

void Formation::formationCreate(){
    std::vector<std::string> keys;
    for (const auto& pair : *formationPositions) {
        keys.push_back(pair.first);
    }
    for (const auto& name : keys) {
        removeComponent(name);
    }

    if(formationType == Constants::FormationType::V){
        std::string name = "ally_" + std::to_string(0);
         addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(10,10,0);

        name = "ally_" + std::to_string(1);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(-10,10,0);
        count = 2;

    }else
    if(formationType == Constants::FormationType::Line){
        std::string name = "ally_" + std::to_string(0);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(0,10,0);

        name = "ally_" + std::to_string(1);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(0,20,0);

        count = 2;

    }else
    if(formationType == Constants::FormationType::Diamond){
        std::string name = "ally_" + std::to_string(0);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(10,10,0);

        name = "ally_" + std::to_string(1);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(-10,20,0);

        name = "ally_" + std::to_string(2);
        addComponent(name);
        (*formationPositions)[name]->Offset = new Vector(0,30,0);

        count = 3;
    }


}

void Formation::addComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    FormationPosition* fp = new FormationPosition();
    if(name == "mothership"){
        mothership = fp;
    }else{
        (*formationPositions)[name] = fp;
    }


    emit parent->componentAdded(QString::fromStdString(ID), QString::fromStdString(name));
}

void Formation::removeComponent(std::string name) {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    (*formationPositions).erase(name);
    emit parent->componentRemoved(QString::fromStdString(ID), QString::fromStdString(name));
}

QJsonObject Formation::getComponent(std::string name) {
    if(name == "mothership"){
        return mothership->toJson();
    }else{
        if (formationPositions->find(name) != formationPositions->end()) {
            // exists
            return (*formationPositions)[name]->toJson();
        }
    }

    return QJsonObject();
}

// void Formation::updateComponent(QString name, const QJsonObject& obj) {
//     if (formationPositions->find(name.toStdString()) != formationPositions->end()) {
//         // exists
//         if(obj.contains("entity")){
//             QJsonObject entityObj = obj["entity"].toObject();
//             Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
//             (*formationPositions)[name.toStdString()]->entity = parent->Entities[entityObj["id"].toString().toStdString()];
//         }
//         (*formationPositions)[name.toStdString()]->fromJson(obj);
//     }
// }

void Formation::updateComponent(QString name, const QJsonObject& obj) {

    if(name == "mothership"){
        if (obj.contains("entity") && obj["entity"].isObject()) {
            QJsonObject entityObj = obj["entity"].toObject();
            if (entityObj.contains("id")) {
                std::string id = entityObj["id"].toString().toStdString();

                Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
                if (parent && parent->Entities->find(id) != parent->Entities->end()) {
                    mothership->entity = (*parent->Entities)[id];

                }
            }
        }

        mothership->fromJson(obj);
        return;
    }
    if (!formationPositions) return;

    std::string key = name.toStdString();

    auto it = formationPositions->find(key);
    if (it == formationPositions->end()) return;

    FormationPosition* pos = it->second;
    if (obj.contains("entity") && obj["entity"].isObject()) {
        QJsonObject entityObj = obj["entity"].toObject();
        if (entityObj.contains("id")) {
            std::string id = entityObj["id"].toString().toStdString();

            Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
            if (parent && parent->Entities->find(id) != parent->Entities->end()) {
                pos->entity = (*parent->Entities)[id];
                Platform* platform = dynamic_cast<Platform*>(pos->entity);
                Platform* mother = dynamic_cast<Platform*>(mothership->entity);
                if (platform) {
                    platform->dynamicModel->followEntity = mother;
                    platform->dynamicModel->formationPosition = pos;
                    platform->dynamicModel->follow = true;

                }
            }
        }
    }

    pos->fromJson(obj);
}
