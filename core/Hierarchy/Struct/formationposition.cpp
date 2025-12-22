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

    // Save entity reference if valid
    if (entity && entity->ID != "dummy" && !entity->ID.empty()){
        QJsonObject refrence;
        refrence["type"] = "reference";
        refrence["name"] = QString::fromStdString(entity->Name);
        refrence["id"] = QString::fromStdString(entity->ID);
        obj["entity"] = refrence;
    }

    // Save offset
    if (Offset) {
        obj["Offset"] = Offset->toJson();
    }

    // Save geo offset
    if (geoOffset) {
        obj["geoOffset"] = geoOffset->toJson();
    }

    return obj;
}

void FormationPosition::fromJson(const QJsonObject& obj) {
    if (obj.contains("Offset") && obj["Offset"].isObject()) {
        if (!Offset) Offset = new Vector();  // UNCOMMENT THIS
        Offset->fromJson(obj["Offset"].toObject());
    }

    if (obj.contains("geoOffset") && obj["geoOffset"].isObject()) {
        if (!geoOffset) geoOffset = new Geocords();  // UNCOMMENT THIS
        geoOffset->fromJson(obj["geoOffset"].toObject());
    }
}
