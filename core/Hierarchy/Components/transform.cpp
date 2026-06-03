/**
 * @file transform.cpp
 * @brief Implementation of the Transform component for 3D positioning, rotation, and geospatial conversion.
 */

#include "transform.h"
#include "core/Hierarchy/Struct/geocords.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qjsonarray.h"
#include "qmath.h"
#include <core/Utility/uuid.h>
#include <cmath>
#include <QVector3D>
#include <QQuaternion>
#include <core/Simulation/simulation.h>

/**
 * @brief Constructs a Transform component with default identity transform.
 */
Transform::Transform():Component(nullptr) {
    ID = Uuid::generateShortUniqueID();
    Active = true;

    geocord = new Geocords();
    matrix = new Qt3DCore::QTransform();
    // Delhi 28.6139° N 77.2090° E
    setGeoCord(2842341234.70418974987,772344123.1025413276);
    // setTranslation(QVector3D(2842341234.70418974987,0,772344123.1025413276));
    connect(matrix,&Qt3DCore::QTransform::translationChanged,this,&Transform::VectorChanged);
    connect(matrix,&Qt3DCore::QTransform::rotationChanged,this,&Transform::rotationChanged);
}

/**
 * @brief Slot called when the translation changes; updates geocord latitude/longitude.
 * @param v New translation vector.
 */
void Transform::VectorChanged(QVector3D v){
    GeoPos geo = flatXYZToGeo(v.x(), v.y()*KMtoFT, v.z());
    geocord->latitude = geo.lat;
    geocord->longitude = geo.lon;
    geocord->altitude = geo.alt;
}

/**
 * @brief Slot called when rotation changes; updates geocord heading.
 * @param r New rotation quaternion.
 */
void Transform::rotationChanged(QQuaternion r){
    geocord->Heading = toEulerAngles().y();
}

/**
 * @brief Sets geographic coordinates (latitude, longitude) with altitude unchanged.
 * @param lat Latitude in degrees * 10^7 (or internal units).
 * @param lon Longitude in degrees * 10^7.
 */
void Transform::setGeoCord(float lat,float lon){
    geocord->latitude = lat;
    geocord->longitude = lon;
    FlatXYZ xyz = geoToFlatXYZ(lat,lon,geocord->altitude*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
    ////qDebug()<<geocord->latitude<<","<<geocord->longitude<<","<<geocord->altitude;
}

/**
 * @brief Sets geographic coordinates (latitude, longitude, altitude).
 * @param lat Latitude.
 * @param lon Longitude.
 * @param alt Altitude in feet.
 */
void Transform::setGeoCord(float lat,float lon, float alt){
    geocord->latitude = lat;
    geocord->longitude = lon;
    geocord->altitude = alt;
    FlatXYZ xyz = geoToFlatXYZ(lat,lon,alt*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

/**
 * @brief Sets geographic coordinates and heading.
 * @param lat Latitude.
 * @param lon Longitude.
 * @param alt Altitude in feet.
 * @param heading Yaw angle in degrees.
 */
void Transform::setGeoCord(float lat,float lon, float alt, float heading){
    geocord->latitude = lat;
    geocord->longitude = lon;
    geocord->altitude = alt;
    geocord->Heading = heading;
    FlatXYZ xyz = geoToFlatXYZ(lat,lon,alt*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
    QVector3D euAngle = toEulerAngles();
    euAngle.setY(heading);
    setFromEulerAngles(euAngle);
}

/**
 * @brief Sets the heading (yaw) without changing position.
 * @param heading Yaw angle in degrees.
 */
void Transform::setHeading(float heading){
    geocord->Heading = heading;
    QVector3D euAngle = toEulerAngles();
    euAngle.setY(heading);
    setFromEulerAngles(euAngle);
}

/**
 * @brief Returns the current heading.
 * @return Heading in degrees.
 */
float Transform::getHeading(){
     if(!geocord) return 0.0f;
    return geocord->Heading;
}

/**
 * @brief Sets latitude and updates position.
 * @param lat Latitude.
 */
void Transform::setLatitude(float lat){
    geocord->latitude = lat;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude*FTtoKM);
    ////qDebug()<<geocord->latitude<<","<<geocord->longitude<<","<<geocord->altitude;
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

/**
 * @brief Returns the current latitude.
 */
float Transform::getLatitude(){
     if(!geocord) return 0.0f;
    return geocord->latitude;
}

/**
 * @brief Sets longitude and updates position.
 * @param lon Longitude.
 */
void Transform::setLongitude(float lon){
    geocord->longitude = lon;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

/**
 * @brief Returns the current longitude.
 */
float Transform::getLongitude(){
     if(!geocord) return 0.0f;
    return geocord->longitude;
}

/**
 * @brief Sets altitude (feet) and updates position.
 * @param alt Altitude in feet.
 */
void Transform::setAltitude(float alt){
    geocord->altitude = alt;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

/**
 * @brief Returns altitude in feet.
 */
float Transform::getAltitude(){
    if(!geocord) return 0.0f;
    return geocord->altitude;
}

// ===== Unity-like Directional Methods (using QQuaternion) =====

/**
 * @brief Returns the current rotation as Euler angles (pitch, yaw, roll).
 * @return QVector3D with x = pitch, y = yaw, z = roll.
 */
QVector3D Transform::toEulerAngles() const {
    if(Simulation::isPlay && false){
        return rotationbuffer.toEulerAngles();
    }else{
        return matrix->rotation().toEulerAngles();
    }
}

/**
 * @brief Sets rotation from Euler angles.
 * @param eulerAngles QVector3D (pitch, yaw, roll) in degrees.
 */
void Transform::setFromEulerAngles(const QVector3D& eulerAngles) {
    if(Simulation::isPlay && false){
        RotUpdate = true;
        rotationbuffer.fromEulerAngles(eulerAngles);
    }else{
        matrix->setRotation(QQuaternion::fromEulerAngles(eulerAngles));
    }
}

/**
 * @brief Returns pitch angle in degrees (rotation about X axis).
 */
float Transform::pitch(){
    return qRadiansToDegrees(qAsin(forward().y()));
}

/**
 * @brief Returns roll angle in degrees (rotation about Z axis).
 */
float Transform::roll(){
    return qRadiansToDegrees(qAtan2(right().y(), up().y()));
}

/**
 * @brief Returns yaw angle in degrees (rotation about Y axis).
 */
float Transform::yaw(){
    return qRadiansToDegrees(qAtan2(forward().x(), forward().z()));
}

/**
 * @brief Returns the forward direction vector (local Z+) transformed to world space.
 */
QVector3D Transform::forward() {
    return matrix->rotation().rotatedVector(QVector3D(0.0f, 0.0f, 1.0f));
}

/**
 * @brief Returns the up direction vector (local Y+) transformed to world space.
 */
QVector3D Transform::up() {
    return matrix->rotation().rotatedVector(QVector3D(0.0f, 1.0f, 0.0f));
}

/**
 * @brief Returns the right direction vector (local X+) transformed to world space.
 */
QVector3D Transform::right() {
    return matrix->rotation().rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
}

/**
 * @brief Returns the opposite of forward.
 */
QVector3D Transform::back() {
    return -forward();
}

/**
 * @brief Returns the opposite of right.
 */
QVector3D Transform::left() {
    return -right();
}

/**
 * @brief Returns the opposite of up.
 */
QVector3D Transform::down() {
    return -up();
}

/**
 * @brief Transforms a direction vector from world space to local space (rotation only).
 * @param worldDir Direction in world coordinates.
 * @return Direction in local coordinates.
 */
QVector3D Transform::inverseTransformDirection(const QVector3D& worldDir) {
    return matrix->rotation().inverted().rotatedVector(worldDir);
}

/**
 * @brief Transforms a direction vector from local space to world space (rotation only).
 * @param localDir Direction in local coordinates.
 * @return Direction in world coordinates.
 */
QVector3D Transform::TransformDirection(const QVector3D& localDir) {
    return matrix->rotation().rotatedVector(localDir);
}

/**
 * @brief Transforms a vector (e.g., velocity) from world space to local space.
 * @param worldVec Vector in world coordinates.
 * @return Vector in local coordinates.
 */
QVector3D Transform::inverseTransformVector(const QVector3D& worldVec) {
    return matrix->rotation().inverted().rotatedVector(worldVec);
}

/**
 * @brief Transforms a point from local space to world space.
 * @param localPos Position in local coordinates.
 * @return Position in world coordinates.
 */
QVector3D Transform::transformPoint(const QVector3D& localPos) {
    // 1. Pehle rotation apply karein
    QVector3D rotated = matrix->rotation().rotatedVector(localPos);

    // 2. Phir world translation add karein
    QVector3D worldPos = rotated + matrix->translation();

    return worldPos;
}

/**
 * @brief Transforms a point (position) from world space to local space (translation + rotation).
 * @param worldPos Position in world coordinates.
 * @return Position in local coordinates.
 */
QVector3D Transform::inverseTransformPoint(const QVector3D& worldPos) {
    QVector3D relativePosition = worldPos - matrix->translation();
    QVector3D final = matrix->rotation().inverted().rotatedVector(relativePosition);
    return final;
}


/**
 * @brief Rotates the transform so that its forward vector points toward the target (2D look-at, ignoring vertical).
 * @param targetWorldPos Target position in world coordinates.
 */
void Transform::lookAt(const QVector3D& targetWorldPos) {
    QVector3D diff = targetWorldPos - this->translation();

    // Standard Heading (Yaw): using atan2(x, z) because forward is Z+
    float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);
    this->setHeading(heading);
}

/**
 * @brief Rotates the transform to face the target in 3D (full orientation).
 * @param targetWorldPos Target position.
 */
void Transform::lookAt3D(const QVector3D& targetWorldPos) {
    QVector3D direction = (targetWorldPos - this->translation()).normalized();

    // Avoid rotation if direction is zero
    if (direction.lengthSquared() < 0.001f) return;

    // fromDirection assumes forward = Z+, up = Y+
    QQuaternion targetRot = QQuaternion::fromDirection(direction, QVector3D(0, 1, 0));
    this->setRotation(targetRot);
}

// ===== Other Methods =====

/**
 * @brief Sets the translation (position) in world coordinates.
 * @param vector New position.
 */
void Transform::setTranslation(const QVector3D& vector) {
    if(Simulation::isPlay && false){
        PosUpdate = true;
        positionbuffer.setX(vector.x());
        positionbuffer.setY(vector.y());
        positionbuffer.setZ(vector.z());
    }else
    {
        matrix->setTranslation(vector);
    }
    // //qDebug()<< vector;
}

/**
 * @brief Adds a translation offset to the current position.
 * @param vector Offset to add.
 */
void Transform::addTranslation(const QVector3D& vector) {
    if(Simulation::isPlay && false){
        PosUpdate = true;
        positionbuffer.setX(positionbuffer.x() + vector.x());
        positionbuffer.setY(positionbuffer.y() + vector.y());
        positionbuffer.setZ(positionbuffer.z() + vector.z());
    }else
    {
        matrix->setTranslation(translation()+vector);
    }
}

/**
 * @brief Returns the current translation (position).
 */
QVector3D Transform::translation() const {
    if(Simulation::isPlay && false){
        return positionbuffer;
    }else
    {
        return matrix->translation();
    }
}

/**
 * @brief Sets the rotation quaternion.
 * @param quat New rotation.
 */
void Transform::setRotation(const QQuaternion& quat) {
    if(Simulation::isPlay && false){
        RotUpdate = true;
        rotationbuffer.setX(quat.x());
        rotationbuffer.setY(quat.y());
        rotationbuffer.setZ(quat.z());
        rotationbuffer.setScalar(quat.scalar());
    }else
    {
        matrix->setRotation(quat);
    }
}

/**
 * @brief Returns the current rotation quaternion.
 */
QQuaternion Transform::rotation() {
    if(Simulation::isPlay && false){
        return rotationbuffer;
    }else
    {
        return matrix->rotation();
    }
}

/**
 * @brief Sets the scale factor per axis.
 * @param vector Scale (x, y, z).
 */
void Transform::setScale3D(const QVector3D& vector) {
    matrix->setScale3D(vector);
}

/**
 * @brief Returns the current scale factor.
 */
QVector3D Transform::scale3D() {
    return matrix->scale3D();
}

/**
 * @brief Placeholder – not used for Transform.
 */
void Transform::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){
}

/**
 * @brief Placeholder – not used.
 */
void Transform::removeSubComponent(std::string ID){
}

/**
 * @brief Placeholder – not used.
 */
void Transform::updateSubComponent(std::string ID, const QJsonObject& obj){
}

/**
 * @brief Placeholder – not used.
 */
QJsonObject Transform::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

/**
 * @brief Serializes the Transform component to JSON.
 * @return QJsonObject containing position, rotation, scale, and geocord data.
 */
QJsonObject Transform::toJson() const {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(ID);
    obj["active"] = Active;
    obj["geocord"] = geocord->toJson();
    obj["position"] = (new Vector(translation().x(),translation().y(),translation().z()))->toJson();
    QVector3D rot = toEulerAngles();
    obj["rotation"] = (new Vector(rot.x(),rot.y(),rot.z()))->toJson();
    obj["size"] = (new Vector(matrix->scale3D().x(),matrix->scale3D().y(),matrix->scale3D().z()))->toJson();
    // obj["localPosition"] = (new Vector(localPosition->x(),localPosition->y(),localPosition->z()))->toJson();
    // QVector3D localrot = toEulerAngles();
    // obj["localRotation"] = (new Vector(localrot.x(),localrot.y(),localrot.z()))->toJson();
    // obj["localSize"] = (new Vector(localSize->x(),localSize->y(),localSize->z()))->toJson();
    obj["type"] = "component";

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;
    return obj;
}

/**
 * @brief Deserializes the Transform component from JSON.
 * @param obj JSON object containing transform data.
 */
void Transform::fromJson(const QJsonObject &obj) {
    // if (obj.contains("id")) ID = obj["id"].toString().toStdString();
    if (obj.contains("active")) Active = obj["active"].toBool();

    bool cordchange = false;
    bool rotchange = false;
    if (obj.contains("geocord") && obj["geocord"].isObject()){
        Geocords geo;
        geo.fromJson(geocord->toJson());
        geocord->fromJson(obj["geocord"].toObject());
        if(geo.latitude != geocord->latitude ||
           geo.longitude != geocord->longitude ||
           geo.altitude != geocord->altitude ||
           geo.Heading != geocord->Heading
            ){
            cordchange = true;
        }
        if(geo.Heading != geocord->Heading){
            rotchange = true;
        }
        setGeoCord(geocord->latitude,geocord->longitude,geocord->altitude,geocord->Heading);
    }
    if (obj.contains("position") && obj["position"].isObject())
    {   Vector* v = new Vector();
        v->fromJson(obj["position"].toObject());
        if(!cordchange)setTranslation(QVector3D(v->x,v->y,v->z));
    }
    if (obj.contains("rotation") && obj["rotation"].isObject())
    {   Vector* v = new Vector();
        v->fromJson(obj["rotation"].toObject());
        if(!rotchange)setFromEulerAngles(QVector3D(v->x,v->y,v->z));
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
    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
}

/**
 * @brief Populates a TransformPDU structure for network transmission.
 * @param pdu Output PDU struct.
 * @param entityID Entity ID.
 * @param parentID Parent ID.
 */
void Transform::toPDU(TransformPDU& pdu, const std::string& entityID, const std::string& parentID) const {
    pdu.entityID = entityID;
    pdu.parentID = parentID;
    //pdu.active = Active;

    // // Geo info
    // pdu.latitude  = geocord->latitude;
    // pdu.longitude = geocord->longitude;
    // pdu.altitude  = geocord->altitude;
    // pdu.heading   = geocord->Heading;

    // Local transform
    QVector3D pos = matrix->translation();
    QVector3D rot = toEulerAngles();
    QVector3D scale = matrix->scale3D();

    pdu.posX = pos.x();
    pdu.posY = pos.y();
    pdu.posZ = pos.z();

    pdu.rotX = rot.x();
    pdu.rotY = rot.y();
    pdu.rotZ = rot.z();

    pdu.sizeX = scale.x();
    pdu.sizeY = scale.y();
    pdu.sizeZ = scale.z();
}

/**
 * @brief Restores transform state from a TransformPDU.
 * @param pdu Input PDU struct.
 */
void Transform::fromPDU(const TransformPDU& pdu) {
    //Active = pdu.active;

    // // Geo info
    // geocord->latitude  = pdu.latitude;
    // geocord->longitude = pdu.longitude;
    // geocord->altitude  = pdu.altitude;
    // geocord->Heading   = pdu.heading;

    // Local transform
    QVector3D pos(pdu.posX, pdu.posY, pdu.posZ);
    QVector3D rot(pdu.rotX, pdu.rotY, pdu.rotZ);
    QVector3D scale(pdu.sizeX, pdu.sizeY, pdu.sizeZ);

    matrix->setTranslation(pos);
    setFromEulerAngles(rot);

    // Apply scale to UI
    matrix->scale3D().setX(scale.x());
    matrix->scale3D().setY(scale.y());
    matrix->scale3D().setZ(scale.z());
}

/**
 * @brief Queues a synchronization of position/rotation buffers to the actual matrix.
 */
void Transform::sync(){
    QMetaObject::invokeMethod(this, "invokesync", Qt::QueuedConnection);
}

/**
 * @brief Performs the actual synchronization (called via queued connection).
 */
void Transform::invokesync(){
    if(PosUpdate){
        PosUpdate = false;
        matrix->setTranslation(positionbuffer);
    }else{
        positionbuffer = matrix->translation();
    }

    if(RotUpdate){
        RotUpdate = false;
        matrix->setRotation(rotationbuffer);
    }else{
        rotationbuffer = matrix->rotation();
    }
}
