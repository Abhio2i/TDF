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
Transform::Transform():Component(nullptr) {
    ID = Uuid::generateShortUniqueID();
    Active = true;

    geocord = new Geocords();
    matrix = new Qt3DCore::QTransform();
    customParameters = QJsonObject();
    //Delhi 28.6139∘ N 77.2090∘ E
    setGeoCord(2842341234.70418974987,772344123.1025413276);
    // setTranslation(QVector3D(2842341234.70418974987,0,772344123.1025413276));
    connect(matrix,&Qt3DCore::QTransform::translationChanged,this,&Transform::VectorChanged);
    connect(matrix,&Qt3DCore::QTransform::rotationChanged,this,&Transform::rotationChanged);
}


void Transform::VectorChanged(QVector3D v){

    GeoPos geo = flatXYZToGeo(v.x(), v.y()*KMtoFT, v.z());
    geocord->latitude = geo.lat;
    geocord->longitude = geo.lon;
    geocord->altitude = geo.alt;
}

void Transform::rotationChanged(QQuaternion r){
    geocord->Heading = toEulerAngles().y();
}

void Transform::setGeoCord(float lat,float lon){

    geocord->latitude = lat;
    geocord->longitude = lon;
    FlatXYZ xyz = geoToFlatXYZ(lat,lon,geocord->altitude*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
    ////qDebug()<<geocord->latitude<<","<<geocord->longitude<<","<<geocord->altitude;
}

void Transform::setGeoCord(float lat,float lon, float alt){

    geocord->latitude = lat;
    geocord->longitude = lon;
    geocord->altitude = alt;
    FlatXYZ xyz = geoToFlatXYZ(lat,lon,alt*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

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

void Transform::setHeading(float heading){

    geocord->Heading = heading;
    QVector3D euAngle = toEulerAngles();
    euAngle.setY(heading);
    setFromEulerAngles(euAngle);
}

float Transform::getHeading(){

    return geocord->Heading;
}

void Transform::setLatitude(float lat){

    geocord->latitude = lat;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude*FTtoKM);
    ////qDebug()<<geocord->latitude<<","<<geocord->longitude<<","<<geocord->altitude;
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

float Transform::getLatitude(){

    return geocord->latitude;
}

void Transform::setLongitude(float lon){

    geocord->longitude = lon;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}
float Transform::getLongitude(){

    return geocord->longitude;
}

void Transform::setAltitude(float alt){

    geocord->altitude = alt;
    FlatXYZ xyz = geoToFlatXYZ(geocord->latitude,geocord->longitude,geocord->altitude*FTtoKM);
    setTranslation(QVector3D(xyz.x,xyz.y,xyz.z));
}

float Transform::getAltitude(){

    return geocord->altitude;
}

// ===== Unity-like Directional Methods (using QQuaternion) =====
QVector3D Transform::toEulerAngles() const {

    if(Simulation::isPlay && false){
        return rotationbuffer.toEulerAngles();
    }else{
        return matrix->rotation().toEulerAngles();
    }
}

void Transform::setFromEulerAngles(const QVector3D& eulerAngles) {

    if(Simulation::isPlay && false){
        RotUpdate = true;
        rotationbuffer.fromEulerAngles(eulerAngles);
    }else{
        matrix->setRotation(QQuaternion::fromEulerAngles(eulerAngles));
    }
}

float Transform::pitch(){
    return qRadiansToDegrees(qAsin(forward().y()));
}

float Transform::roll(){
    return qRadiansToDegrees(qAtan2(right().y(), up().y()));
}

float Transform::yaw(){
    return qRadiansToDegrees(qAtan2(forward().x(), forward().z()));
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
    //relativePosition.setZ(-relativePosition.z());
    QVector3D final = matrix->rotation().inverted().rotatedVector(relativePosition);
    // 2. Then, apply the inverse of rotation.
    //final.setZ(-final.z());
    return final;
}

void Transform::lookAt(const QVector3D& targetWorldPos) {
    QVector3D diff = targetWorldPos - this->translation();

    // Standard Heading (Yaw): Z+ forward hai isliye atan2(x, z) use hoga
    // atan2(right, forward)
    float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);

    this->setHeading(heading);
}

void Transform::lookAt3D(const QVector3D& targetWorldPos) {
    QVector3D direction = (targetWorldPos - this->translation()).normalized();

    // Agar direction zero hai toh rotate na karein (prevents crash)
    if (direction.lengthSquared() < 0.001f) return;

    // FromDirection(forward_vector, up_vector)
    // Yeh function Z+ ko forward maan kar rotation banata hai
    QQuaternion targetRot = QQuaternion::fromDirection(direction, QVector3D(0, 1, 0));

    this->setRotation(targetRot);
}

// ===== Other Methods =====

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

QVector3D Transform::translation() const {

    if(Simulation::isPlay && false){
        return positionbuffer;
    }else
    {
        return matrix->translation();
    }

}

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

QQuaternion Transform::rotation() {

    if(Simulation::isPlay && false){
        return rotationbuffer;
    }else
    {
        return matrix->rotation();
    }
}

void Transform::setScale3D(const QVector3D& vector) {

    matrix->setScale3D(vector);
}


QVector3D Transform::scale3D() {

    return matrix->scale3D();
}

void Transform::addSubComponent(std::string name, QString data1, QString data2, QString data3){

}

void Transform::removeSubComponent(std::string ID){

}

void Transform::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject Transform::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

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

    // Add custom parameters
    for (auto it = customParameters.begin(); it != customParameters.end(); ++it) {
        obj[it.key()] = it.value();
    }
    // Add other fields and custom parameters
    return obj;
}

void Transform::fromJson(const QJsonObject &obj) {
    // if (obj.contains("id")) ID = obj["id"].toString().toStdString();
    if (obj.contains("active")) Active = obj["active"].toBool();

    if (obj.contains("geocord") && obj["geocord"].isObject()){
        geocord->fromJson(obj["geocord"].toObject());
        // setGeoCord(geocord->latitude,geocord->longitude,geocord->altitude,geocord->Heading);
    }
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

    // Optional: you can serialize customParameters to PDU if needed
}

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

    // Optional: update UI/custom parameters if stored in PDU
}


void Transform::sync(){
    QMetaObject::invokeMethod(this, "invokesync", Qt::QueuedConnection);
}

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
