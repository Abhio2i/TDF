#include "formationposition.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
FormationPosition::FormationPosition() {
    entity = new Platform(nullptr);  // OK
    entity->Name = "dummy";
    Offset = new Vector();  // MISSING
    geoOffset = new Geocords(); // MISSING
}


QJsonObject FormationPosition::toJson() {
    QJsonObject obj;

    if (entity){
        QJsonObject refrence;
        refrence["type"] = "refrence";
        refrence["name"] = QString::fromStdString(entity->Name);
        refrence["id"] = QString::fromStdString(entity->ID);
        obj["entity"] = refrence;
    }

    if (Offset)
        obj["Offset"] = Offset->toJson();

    if (geoOffset)
        obj["geoOffset"] = geoOffset->toJson();

    return obj;
}

void FormationPosition::fromJson(const QJsonObject& obj) {
    if (obj.contains("Offset") && obj["Offset"].isObject()) {
        //if (!Offset) Offset = new Offset();
        Offset->fromJson(obj["Offset"].toObject());
    }

    if (obj.contains("geoOffset") && obj["geoOffset"].isObject()) {
        //if (!Offset) Offset = new Offset();
        geoOffset->fromJson(obj["geoOffset"].toObject());
    }
}
