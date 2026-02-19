//============================================================================
// Written by: Waris
// FormationPosition
// Purpose:
// - Represents a single slot within a formation
// - Holds an optional entity reference and positional offsets
// - Supports JSON serialization/deserialization for persistence
//============================================================================

#include "formationposition.h"
#include "core/Hierarchy/EntityProfiles/platform.h"


// Constructor
// Initializes a formation slot with a dummy entity and default offsets
FormationPosition::FormationPosition() {
    entity = new Platform(nullptr);  // OK
    entity->Name = "dummy";
    Offset = new Vector();  // MISSING
    geoOffset = new Geocords(); // MISSING
}

// Serializes the formation position into JSON
// Stores entity reference (if valid), local offset, and geo offset
// QJsonObject FormationPosition::toJson() {
//     QJsonObject obj;

//     // Save entity reference if valid
//     if (entity && entity->ID != "dummy" && !entity->ID.empty()){
//         QJsonObject refrence;
//         refrence["type"] = "reference";
//         refrence["name"] = QString::fromStdString(entity->Name);
//         refrence["id"] = QString::fromStdString(entity->ID);
//         obj["entity"] = refrence;
//     }

//     // Save offset
//     if (Offset) {
//         obj["Offset"] = Offset->toJson();
//     }

//     // Save geo offset
//     if (geoOffset) {
//         obj["geoOffset"] = geoOffset->toJson();
//     }

//     return obj;
// }
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

    // Save offset - CRITICAL: Make sure this is included
    if (Offset) {
        qDebug() << "Serializing Offset: x=" << Offset->x << " y=" << Offset->y << " z=" << Offset->z;
        obj["Offset"] = Offset->toJson();
    } else {
        qDebug() << "Offset is NULL!";
        // Create default offset if null
        QJsonObject offsetObj;
        offsetObj["type"] = "section";
        offsetObj["x"] = 0;
        offsetObj["y"] = 0;
        offsetObj["z"] = 0;
        obj["Offset"] = offsetObj;
    }

    // Save geo offset - CRITICAL: Make sure this is included
    if (geoOffset) {
        qDebug() << "Serializing GeoOffset";
        obj["geoOffset"] = geoOffset->toJson();
    } else {
        qDebug() << "GeoOffset is NULL!";
        // Create default geoOffset if null
        QJsonObject geoOffsetObj;
        geoOffsetObj["type"] = "section";
        geoOffsetObj["lat"] = 0;
        geoOffsetObj["lon"] = 0;
        geoOffsetObj["alt"] = 0;
        geoOffsetObj["heading"] = 0;
        obj["geoOffset"] = geoOffsetObj;
    }

    return obj;
}


// Deserializes formation position data from JSON
// Restores offsets while safely allocating missing objects
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
