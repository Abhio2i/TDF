#include "waypoints.h"

Waypoints::Waypoints() {}

QJsonObject Waypoints::toJson()const {
    QJsonObject obj;

    obj["speed"] = speed;

    if (geocord)
        obj["geocord"] = geocord->toJson();

    if (position)
        obj["position"] = position->toJson();

    return obj;
}

void Waypoints::fromJson(const QJsonObject& obj) {

    if (obj.contains("speed"))
        speed = obj["speed"].toVariant().toDouble();

    if (obj.contains("geocord") && obj["geocord"].isObject()) {
        if (!geocord) geocord = new Geocords();
        geocord->fromJson(obj["geocord"].toObject());
    }

    if (obj.contains("position") && obj["position"].isObject()) {
        if (!position) position = new Vector();
        position->fromJson(obj["position"].toObject());
    }
}
