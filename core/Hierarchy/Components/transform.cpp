

#include "transform.h"
#include <core/Utility/uuid.h>
#include <math.h>
// #include "console.h"

Transform::Transform() {
    ID = Uuid::generateShortUniqueID();
    Active = true; // Initialize Active
    geocord = new Geocords();
    position = new Vector(0, 0, 0);
    rotation = new Vector();
    size = new Vector(1, 1, 1);
    localPosition = new Vector();
    localRotation = new Vector();
    localSize = new Vector(1, 1, 1);
    customParameters = QJsonObject(); // Initialize customParameters
}

// ===== Unity-like Directional Methods =====

// Forward is X-axis based
Vector Transform::forward() {
    float yaw = rotation->z * M_PI / 180.0f;   // Yaw around z-axis
    float pitch = rotation->y * M_PI / 180.0f; // Pitch around y-axis
    float roll = rotation->x * M_PI / 180.0f;  // Roll around x-axis

    // Compute forward vector (x-axis) with yaw, pitch, and roll
    float x = cos(yaw) * cos(pitch);
    float y = sin(yaw) * cos(pitch);
    float z = -sin(pitch);

    // Apply roll (rotation around x-axis)
    Vector forward(x, y, z);
    if (roll != 0) {
        // Rotate around x-axis (forward vector)
        float cosRoll = cos(roll);
        float sinRoll = sin(roll);
        float newY = y * cosRoll - z * sinRoll;
        float newZ = y * sinRoll + z * cosRoll;
        forward.y = newY;
        forward.z = newZ;
    }

    return forward.normalized();
}

// Left is y-axis based (90° yaw from forward)
Vector Transform::left() {
    float yaw = (rotation->z - 90.0f) * M_PI / 180.0f; // Yaw - 90° for left
    float pitch = rotation->y * M_PI / 180.0f;         // Pitch around y-axis
    float roll = rotation->x * M_PI / 180.0f;          // Roll around x-axis

    // Compute left vector (y-axis) with adjusted yaw
    float x = cos(yaw) * cos(pitch);
    float y = sin(yaw) * cos(pitch);
    float z = -sin(pitch);

    // Apply roll
    Vector left(x, y, z);
    if (roll != 0) {
        float cosRoll = cos(roll);
        float sinRoll = sin(roll);
        float newY = y * cosRoll - z * sinRoll;
        float newZ = y * sinRoll + z * cosRoll;
        left.y = newY;
        left.z = newZ;
    }

    return left.normalized();
}

// Up is z-axis based
Vector Transform::up() {
    float yaw = rotation->z * M_PI / 180.0f;   // Yaw around z-axis
    float pitch = rotation->y * M_PI / 180.0f; // Pitch around y-axis
    float roll = rotation->x * M_PI / 180.0f;  // Roll around x-axis

    // Compute up vector (z-axis) with yaw, pitch, and roll
    float x = sin(pitch) * cos(yaw);
    float y = sin(pitch) * sin(yaw);
    float z = cos(pitch);

    // Apply roll
    Vector up(x, y, z);
    if (roll != 0) {
        float cosRoll = cos(roll);
        float sinRoll = sin(roll);
        float newY = y * cosRoll - z * sinRoll;
        float newZ = y * sinRoll + z * cosRoll;
        up.y = newY;
        up.z = newZ;
    }

    return up.normalized();
}

Vector Transform::back() {
    return forward() * -1.0f;
}

Vector Transform::right() {
    return left() * -1.0f;
}

Vector Transform::down() {
    return up() * -1.0f;
}

Vector Transform::inverseTransformDirection(const Vector& worldDir) {
    // Get local axis directions
    Vector forwardVec = forward();
    Vector rightVec = right();
    Vector upVec = up();

    // Project world direction onto local axis
    float x = Vector::Dot(worldDir, forwardVec);
    float y = Vector::Dot(worldDir, rightVec);
    float z = Vector::Dot(worldDir, upVec);

    return Vector(x, y, z);
}

QJsonObject Transform::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["geocord"] = geocord->toJson();
    obj["position"] = position->toJson();
    obj["rotation"] = rotation->toJson();
    obj["size"] = size->toJson();
    obj["localPosition"] = localPosition->toJson();
    obj["localRotation"] = localRotation->toJson();
    obj["localSize"] = localSize->toJson();

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }

    // Console::log("Transform::toJson output: " + QString(QJsonDocument(obj).toJson()).toStdString());
    return obj;
}

void Transform::fromJson(const QJsonObject &obj) {
    // Console::log("Transform::fromJson input: " + QString(QJsonDocument(obj).toJson()).toStdString());

    // Standard fields
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("geocord") && obj["geocord"].isObject())
        geocord->fromJson(obj["geocord"].toObject());
    if (obj.contains("position") && obj["position"].isObject())
        position->fromJson(obj["position"].toObject());
    if (obj.contains("rotation") && obj["rotation"].isObject())
        rotation->fromJson(obj["rotation"].toObject());
    if (obj.contains("size") && obj["size"].isObject())
        size->fromJson(obj["size"].toObject());
    if (obj.contains("localPosition") && obj["localPosition"].isObject())
        localPosition->fromJson(obj["localPosition"].toObject());
    if (obj.contains("localRotation") && obj["localRotation"].isObject())
        localRotation->fromJson(obj["localRotation"].toObject());
    if (obj.contains("localSize") && obj["localSize"].isObject())
        localSize->fromJson(obj["localSize"].toObject());

    // Custom parameters
    QStringList standardKeys = {"id", "active", "geocord", "position", "rotation", "size",
                                "localPosition", "localRotation", "localSize"};
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }

    // Console::log("Transform::fromJson customParameters: " + QString(QJsonDocument(customParameters).toJson()).toStdString());
}

void Transform::translate(Vector *vector) {
    // Implementation for translate (not provided in original code)
}

void Transform::rotate(Vector *vector) {
    // Implementation for rotate (not provided in original code)
}

void Transform::scale(Vector *vector) {
    // Implementation for scale (not provided in original code)
}
