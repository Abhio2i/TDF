#include "radioprofile.h"
#include "core/GlobalRegistry.h"
#include "core/Hierarchy/EntityProfiles/Radio/customradio.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/hierarchy.h"
RadioProfile::RadioProfile():Component() {
    Active = true;
    // radios =  new std::unordered_map<std::string, Radio*>();
}

RadioProfile::~RadioProfile(){
    while (!radios.empty()) {
        removeSubComponent(radios.begin()->second->ID);
    }
    // delete radios;
    // radios = nullptr;
}
ECS::radioComponent& RadioProfile::getRadioData() const{
    return *parentEntity->root->world.GetComponent<ECS::radioComponent>(parentEntity->ecsID);
}
void RadioProfile::init(Entity *Entity){
    parentEntity = Entity;
    auto& rd = parentEntity->root->world.AddComponent<ECS::radioComponent>(parentEntity->ecsID);
    if(parentEntity){
        for(int i=0;i<90;i++){
            rd.radios[i]=-1;
        }
    }

}
void RadioProfile::Awake(){}
void RadioProfile::Start(){}
void RadioProfile::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    if(!parentEntity->root)return;
    if(data1 == "Radio"||data1.isLower()){
        Radio* radio = new Radio(parentEntity->root);
        if(!data2.isEmpty()){
            std::string id = radio->ID;
            radio->fromJson(data3);
            radio->ID = id;
            radio->parentID = parentID;
        }
        // radio->parentEntity = parentEntity;
        // radio->Name = name;
        // radios.insert({radio->ID,radio});
        radio->parentEntity = parentEntity;
        radio->Name = name;
        if (radio->radioIndex <= 0) {
            radio->radioIndex = m_nextRadioIndex++;
        } else {
            m_nextRadioIndex = std::max(m_nextRadioIndex, radio->radioIndex + 1);
        }
        radios.insert({radio->ID,radio});
        qDebug() << "[INDEX-CHECK] radio" << QString::fromStdString(radio->ID)
                 << "assigned radioIndex=" << radio->radioIndex;
        parentEntity->root->Radios.insert({radio->ID,radio});
        emit parentEntity->root->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radio->ID),QString::fromStdString(name));
        emit parentEntity->root->radioAdded(QString::fromStdString(parentID), QString::fromStdString(radio->ID));

    }else{
        CustomRadioInterface* cradio = nullptr;
        if(parentEntity&&parentEntity->root){
            std::string iD = Uuid::generateShortUniqueID();
            emit parentEntity->root->getCustomRadio(QString::fromStdString(parentEntity->ID),parentEntity->ecsID,QString::fromStdString(iD),cradio);
            if(cradio){
                CustomRadio* radio = new CustomRadio(parentEntity->root);
                radio->ID = iD;
                radio->parentEntity = parentEntity;
                radio->radio = cradio;
                radio->type = data1;
                radio->Name = name;
                radios.insert({radio->ID, radio});
                parentEntity->root->Radios.insert({radio->ID,radio});
                emit parentEntity->root->subComponentAdded(
                    QString::fromStdString(ID),
                    QString::fromStdString(radio->ID),
                    QString::fromStdString(name));
                emit parentEntity->root->sensorAdded(QString::fromStdString(parentID), QString::fromStdString(radio->ID));
                cradio->registerDestroyCallback([this,iD,radio](){
                    if(radio){
                        radio->radio = nullptr;
                    }
                    this->removeSubComponent(iD);
                });

            }
        }
    }
}

void RadioProfile::removeSubComponent(std::string ID){
    // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    auto it = radios.find(ID);
    if (it != radios.end() && it->second != nullptr) {
        Radio *radio = it->second;
        emit parentEntity->root->subComponentRemoved(QString::fromStdString(this->ID),QString::fromStdString(ID),QString::fromStdString(radio->Name));
        radios.erase(radio->ID);
        parentEntity->root->Radios.erase(radio->ID);
        delete radio;
    }
}

void RadioProfile::updateSubComponent(std::string ID, const QJsonObject& obj){
    auto it = radios.find(ID);
    if (it != radios.end() && it->second != nullptr) {
        it->second->fromJson(obj);
    }

}

QJsonObject RadioProfile::getsubComponentData(std::string ID) const{
    auto it = radios.find(ID);
    if (it != radios.end() && it->second != nullptr) {
        return it->second->toJson();
    }
    return QJsonObject();
}

QJsonObject RadioProfile::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    QJsonObject radioObj;
    for (const auto& [key, radioPtr] : radios) {
        if (radioPtr) {
            radioObj[QString::fromStdString(key)] = radioPtr->toJson();
        }
    }
    obj["radios"] = radioObj;
    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;
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
        // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        for (const QString& key : radiosObj.keys()) {
            if(!parentEntity->root)break;
            QJsonObject radioObj = radiosObj[key].toObject();

            std::string id = radioObj["id"].toString().toStdString();

            Radio *radio;
            bool is = radios.count(id);
            if ( is > 0) {
                radio = ( radios)[id];   // OR iffs->at(id)

            } else {
                radio = new Radio(parentEntity->root);
                radio->parentEntity = parentEntity;
            }
            radio->Name = radioObj["name"].toString().toStdString();
            std::string newId = radio->ID;
            //radio->ID = radioObj["id"].toString().toStdString();
            radio->fromJson(radioObj);
            radio->ID = newId;

            // if (!is) {
            //     radios.insert({radio->ID,radio});
            //     parentEntity->root->Radios.insert({radio->ID,radio});
            if (!is) {
                if (radio->radioIndex <= 0) {
                    radio->radioIndex = m_nextRadioIndex++;
                } else {
                    m_nextRadioIndex = std::max(m_nextRadioIndex, radio->radioIndex + 1);
                }
                radios.insert({radio->ID,radio});
                parentEntity->root->Radios.insert({radio->ID,radio});
                emit parentEntity->root->subComponentAdded(QString::fromStdString(ID),QString::fromStdString(radio->ID),QString::fromStdString(radio->Name));
                emit parentEntity->root->radioAdded(QString::fromStdString(parentID), QString::fromStdString(radio->ID));
            }
        }
    }
    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    //Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
void RadioProfile::renameSubComponent(std::string id, QString newName) {
    auto it = radios.find(id);
    if (it != radios.end() && it->second != nullptr) {
        // Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
        it->second->Name = newName.toStdString();
        emit parentEntity->root->subComponentRenamed(
            QString::fromStdString(ID),
            QString::fromStdString(id),
            newName);
    }
}
