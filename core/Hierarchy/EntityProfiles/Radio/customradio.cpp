#include "customradio.h"
#include "core/Hierarchy/hierarchy.h"
CustomRadio::CustomRadio(Hierarchy* h) : Radio(h)
{

}

CustomRadio::~CustomRadio()
{
    if(parentEntity&&parentEntity->root){
        emit parentEntity->root->RemoveCustomRadio(QString::fromStdString(parentEntity->ID),parentEntity->ecsID,QString::fromStdString(ID));
    }
}


QJsonObject CustomRadio::toJson() const
{
    QJsonObject obj;

    // --- Identity (unchanged) ---
    obj["id"]         = QString::fromStdString(ID);
    obj["name"]       = QString::fromStdString(Name);
    obj["Active"]     = Active;
    obj["SensorType"] = type;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;
    if(radio){
        obj = radio->toJson();
        obj["id"] = QString::fromStdString(ID);
    }

    return obj;
}


void CustomRadio::fromJson(const QJsonObject& obj)
{


    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
    if(radio)
    radio->fromJson(obj);
}
