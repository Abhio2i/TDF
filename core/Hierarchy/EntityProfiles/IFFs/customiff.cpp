#include "customiff.h"
#include "core/Hierarchy/hierarchy.h"
CustomIFF::CustomIFF(Hierarchy* h) : IFF(h)
{

}

CustomIFF::~CustomIFF()
{
    if(parentEntity&&parentEntity->root){
        emit parentEntity->root->RemoveCustomIFF(QString::fromStdString(parentEntity->ID),parentEntity->ecsID,QString::fromStdString(ID));
    }
}


QJsonObject CustomIFF::toJson() const
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
    if(iff){
        obj = iff->toJson();
        obj["id"] = QString::fromStdString(ID);
    }

    return obj;
}


void CustomIFF::fromJson(const QJsonObject& obj)
{


    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
    if(iff)
        iff->fromJson(obj);
}
