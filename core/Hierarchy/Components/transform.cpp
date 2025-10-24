#include "transform.h"
#include "core/Hierarchy/Struct/geocords.h"
#include "qjsonarray.h"
#include <core/Utility/uuid.h>
#include <cmath>
#include <QVector3D>
#include <QQuaternion>

Transform::Transform() {
    ID = Uuid::generateShortUniqueID();
    Active = true;

    geocord = new Geocords();
    // Initialize with standard values
    matrix = new Qt3DCore::QTransform();

    customParameters = QJsonObject();
    setTranslation(QVector3D(28.7041,0,77.1025));
}

// ===== Unity-like Directional Methods (using QQuaternion) =====
QVector3D Transform::toEulerAngles() const {
    return matrix->rotation().toEulerAngles();
}

void Transform::setFromEulerAngles(const QVector3D& eulerAngles) {
    matrix->setRotation(QQuaternion::fromEulerAngles(eulerAngles));
}

// Unity में forward direction Z-axis होता है।
QVector3D Transform::forward() {
    return matrix->rotation().rotatedVector(QVector3D(0.0f, 0.0f, 1.0f));
}

QVector3D Transform::up() {
    // Rotation को up vector (Y-axis) पर लागू करें।
    return matrix->rotation().rotatedVector(QVector3D(0.0f, 1.0f, 0.0f));
}

QVector3D Transform::right() {
    // Rotation को right vector (X-axis) पर लागू करें।
    return matrix->rotation().rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
}

QVector3D Transform::back() {
    return -forward();
}

QVector3D Transform::left() {
    return -right();
}

QVector3D Transform::down() {
    return -up();
}

QVector3D Transform::inverseTransformDirection(const QVector3D& worldDir) {
    return matrix->rotation().inverted().rotatedVector(worldDir);
}

QVector3D Transform::TransformDirection(const QVector3D& localDir) {
    return matrix->rotation().rotatedVector(localDir);
}

/**
 * @brief Transforms a vector from World Space to Local Space.
 * Vector transformation includes only rotation (and scaling, if implemented), not translation.
 * This is functionally the same as inverseTransformDirection.
 */
QVector3D Transform::inverseTransformVector(const QVector3D& worldVec) {
    // Vectors (like velocity or force) are only affected by rotation, not position/translation.
    return matrix->rotation().inverted().rotatedVector(worldVec);
}

/**
 * @brief Transforms a position (Point) from World Space to Local Space.
 * Position transformation includes both rotation and translation.
 */
QVector3D Transform::inverseTransformPoint(const QVector3D& worldPos) {
    // 1. First, apply the inverse of translation (subtract the world position).
    QVector3D relativePosition = worldPos - matrix->translation();

    // 2. Then, apply the inverse of rotation.
    return matrix->rotation().inverted().rotatedVector(relativePosition);
}

// ===== Other Methods =====

void Transform::setTranslation(const QVector3D& vector) {
    matrix->setTranslation(vector);
}

void Transform::addTranslation(const QVector3D& vector) {
    matrix->setTranslation(translation()+vector);
}

QVector3D Transform::translation() {
    return matrix->translation();
}

void Transform::setRotation(const QQuaternion& quat) {
    matrix->setRotation(quat);
}

QQuaternion Transform::rotation() {
    return matrix->rotation();
}

void Transform::setScale3D(const QVector3D& vector) {
    matrix->setScale3D(vector);
}


QVector3D Transform::scale3D() {
    return matrix->scale3D();
}

QJsonObject Transform::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["geocord"] = geocord->toJson();
    obj["position"] = (new Vector(matrix->translation().x(),matrix->translation().y(),matrix->translation().z()))->toJson();
    QVector3D rot = toEulerAngles();
    obj["rotation"] = (new Vector(rot.x(),rot.y(),rot.z()))->toJson();
    obj["size"] = (new Vector(matrix->scale3D().x(),matrix->scale3D().y(),matrix->scale3D().z()))->toJson();
    // obj["localPosition"] = (new Vector(localPosition->x(),localPosition->y(),localPosition->z()))->toJson();
    // QVector3D localrot = toEulerAngles();
    // obj["localRotation"] = (new Vector(localrot.x(),localrot.y(),localrot.z()))->toJson();
    // obj["localSize"] = (new Vector(localSize->x(),localSize->y(),localSize->z()))->toJson();
    obj["type"] = "component";

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }
    // Add other fields and custom parameters
    return obj;
}

void Transform::fromJson(const QJsonObject &obj) {
    if (obj.contains("id")) ID = obj["id"].toString().toStdString();
    if (obj.contains("active")) Active = obj["active"].toBool();

    if (obj.contains("geocord") && obj["geocord"].isObject())
        geocord->fromJson(obj["geocord"].toObject());
    if (obj.contains("position") && obj["position"].isObject())
    {   Vector* v = new Vector();
        v->fromJson(obj["position"].toObject());
        setTranslation(QVector3D(v->x,v->y,v->z));
    }
    if (obj.contains("rotation") && obj["rotation"].isObject())
    {   Vector* v = new Vector();
        v->fromJson(obj["rotation"].toObject());
        setFromEulerAngles(QVector3D(v->x,v->y,v->z));
    }
    if (obj.contains("size") && obj["size"].isObject())
    {   Vector* v = new Vector();
        v->fromJson(obj["size"].toObject());
        matrix->scale3D().setX(v->x);
        matrix->scale3D().setY(v->y);
        matrix->scale3D().setZ(v->z);
    }
    // if (obj.contains("localPosition") && obj["localPosition"].isObject())
    // {   Vector* v = new Vector();
    //     v->fromJson(obj["localPosition"].toObject());
    //     localPosition->setX(v->x);
    //     localPosition->setY(v->y);
    //     localPosition->setZ(v->z);
    // }
    // if (obj.contains("localRotation") && obj["localRotation"].isObject())
    // {   Vector* v = new Vector();
    //     v->fromJson(obj["localRotation"].toObject());
    //     //setFromEulerAngles(QVector3D(v->x,v->y,v->z));
    // }
    // if (obj.contains("localSize") && obj["localSize"].isObject())
    // {   Vector* v = new Vector();
    //     v->fromJson(obj["localSize"].toObject());
    //     localSize->setX(v->x);
    //     localSize->setY(v->y);
    //     localSize->setZ(v->z);
    // }
    // Custom parameters
    QStringList standardKeys = {"id", "active", "geocord", "position", "rotation", "size",
                                "localPosition", "localRotation", "localSize"};
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!standardKeys.contains(it.key())) {
            customParameters[it.key()] = it.value();
        }
    }
}
