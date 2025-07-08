
#include "collider.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include <QMetaEnum>
#include <core/Debug/console.h>

Collider::Collider() {
    Active = true;
    Radius = 1;
    Width = 1;
    Length = 1;
    Height = 1;
    collider = Constants::ColliderType::Box;
    vector = nullptr; // Initialize pointer
    type = Constants::EntityType::Platform; // Use Platform as default
}

QString colliderTypeToString(Constants::ColliderType type) {
    switch (type) {
    case Constants::ColliderType::Box: return "Box";
    case Constants::ColliderType::Sphere: return "Sphere";
    case Constants::ColliderType::Cyclinder: return "Cyclinder"; // Note: Typo, consider fixing to Cylinder
    default: return "Box";
    }
}

Constants::ColliderType stringToColliderType(const QString& str) {
    if (str == "Box") return Constants::ColliderType::Box;
    if (str == "Sphere") return Constants::ColliderType::Sphere;
    if (str == "Cyclinder") return Constants::ColliderType::Cyclinder;
    return Constants::ColliderType::Box; // Default
}

QStringList colliderTypeOptions() {
    QStringList list;
    int index = Constants::staticMetaObject.indexOfEnumerator("ColliderType");
    QMetaEnum metaEnum = Constants::staticMetaObject.enumerator(index);
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        list << QString::fromLatin1(metaEnum.key(i));
    }
    return list;
}

QJsonObject Collider::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["radius"] = Radius;
    obj["width"] = Width;
    obj["length"] = Length;
    obj["height"] = Height;

    QJsonObject colliderObj;
    colliderObj["type"] = "option";
    QJsonArray optionsArray;
    for (const QString& opt : colliderTypeOptions())
        optionsArray.append(opt);
    colliderObj["options"] = optionsArray;
    colliderObj["value"] = colliderTypeToString(collider);
    obj["collider"] = colliderObj;

    // Include custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    Console::log("Collider::toJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
    Console::log("Collider::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

void Collider::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("radius"))
        Radius = obj["radius"].toDouble();
    if (obj.contains("width"))
        Width = obj["width"].toDouble();
    if (obj.contains("length"))
        Length = obj["length"].toDouble();
    if (obj.contains("height"))
        Height = obj["height"].toDouble();
    if (obj.contains("collider") && obj["collider"].isObject()) {
        QJsonObject colliderObj = obj["collider"].toObject();
        if (colliderObj.contains("value"))
            collider = stringToColliderType(colliderObj["value"].toString());
    }

    // Merge custom parameters
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() != "active" && it.key() != "radius" && it.key() != "width" &&
            it.key() != "length" && it.key() != "height" && it.key() != "collider") {
            customParameters[it.key()] = it.value();
        }
    }
    Console::log("Collider::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}
