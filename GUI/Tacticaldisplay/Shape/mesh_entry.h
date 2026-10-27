#ifndef MESH_ENTRY_H
#define MESH_ENTRY_H

#include <QString>
#include <QVector3D>
#include <QQuaternion>
#include <QColor>
#include <QFont>
#include <QPolygonF>
#include <QPointer>
#include "core/Hierarchy/entity.h"
// Forward declarations
class Vector;
class Mesh;
class Entity;
class Platform;
class Collider;
class Trajectory;
class DynamicModel;
class Transform;

/**
 * @brief MeshEntry structure
 */
struct MeshEntry {
    QString name;
    QString shapeType;  // Internal object type: Circle, Rectangle, Polygon, Line, Point, Text, Bitmap
    Transform* coreTransform = nullptr;
    Transform* transform = nullptr;
    QVector3D* position = nullptr;
    QQuaternion* rotation = nullptr;
    QVector3D* velocity = nullptr;
    QVector3D* size = nullptr;
    Mesh* mesh = nullptr;
    Entity* entity = nullptr;
    Platform* platform = nullptr;
    Collider* collider = nullptr;
    Trajectory* trajectory = nullptr;
    DynamicModel* dynamicModel = nullptr;
    QString bitmapPath;
    QString text;
    QPolygonF polyline;
    QVector<QPointF> pointsToDraw;
    bool detection = true;
    bool radioVisible = false;
    QColor textColor = Qt::black;
    QFont textFont = QFont("Arial", 12);
    int textSize = 12;
    bool isTextSelected = false;
    int individualImageSize = -1;
    QColor trajectoryColor = Qt::blue;
    float circleRadius = 0.0f;

    // Constructor
    MeshEntry()
        : coreTransform(nullptr)
        , transform(nullptr)
        , position(new QVector3D(0, 0, 0))
        , rotation(new QQuaternion())
        , velocity(new QVector3D(0, 0, 0))
        , size(new QVector3D(1, 1, 1))
        , mesh(nullptr)
        , entity(nullptr)
        , platform(nullptr)
        , collider(nullptr)
        , trajectory(nullptr)
        , dynamicModel(nullptr)
    {}

    // Copy constructor
    MeshEntry(const MeshEntry& other)
        : name(other.name)
        , shapeType(other.shapeType)
        , coreTransform(other.coreTransform)
        , transform(other.transform)
        , position(other.position ? new QVector3D(*other.position) : new QVector3D(0, 0, 0))
        , rotation(other.rotation ? new QQuaternion(*other.rotation) : new QQuaternion())
        , velocity(other.velocity ? new QVector3D(*other.velocity) : new QVector3D(0, 0, 0))
        , size((other.size && other.size != nullptr) ? new QVector3D(*other.size) : new QVector3D(1, 1, 1))
        , mesh(other.mesh)
        , entity(other.entity)
        , platform(other.platform)
        , collider(other.collider)
        , trajectory(other.trajectory)
        , dynamicModel(other.dynamicModel)
        , bitmapPath(other.bitmapPath)
        , text(other.text)
        , polyline(other.polyline)
        , pointsToDraw(other.pointsToDraw)
        , detection(other.detection)
        , radioVisible(other.radioVisible)
        , textColor(other.textColor)
        , textFont(other.textFont)
        , textSize(other.textSize)
        , isTextSelected(other.isTextSelected)
        , individualImageSize(other.individualImageSize)
        , trajectoryColor(other.trajectoryColor)
        , circleRadius(other.circleRadius)
    {}

    // Assignment operator
    MeshEntry& operator=(const MeshEntry& other) {
        if (this != &other) {
            name = other.name;
            shapeType = other.shapeType;
            coreTransform = other.coreTransform;
            transform = other.transform;
            delete position;
            delete rotation;
            delete velocity;
            delete size;

            position = other.position ? new QVector3D(*other.position) : new QVector3D(0, 0, 0);
            rotation = other.rotation ? new QQuaternion(*other.rotation) : new QQuaternion();
            velocity = other.velocity ? new QVector3D(*other.velocity) : new QVector3D(0, 0, 0);
            size = other.size ? new QVector3D(*other.size) : new QVector3D(1, 1, 1);

            mesh = other.mesh;
            entity = other.entity;
            platform = other.platform;
            collider = other.collider;
            trajectory = other.trajectory;
            dynamicModel = other.dynamicModel;
            bitmapPath = other.bitmapPath;
            text = other.text;
            polyline = other.polyline;
            pointsToDraw = other.pointsToDraw;
            detection = other.detection;
            radioVisible = other.radioVisible;
            textColor = other.textColor;
            textFont = other.textFont;
            textSize = other.textSize;
            isTextSelected = other.isTextSelected;
            individualImageSize = other.individualImageSize;
            trajectoryColor = other.trajectoryColor;
            circleRadius = other.circleRadius;
        }
        return *this;
    }
    // Destructor
    ~MeshEntry() {
        delete position;
        delete rotation;
        delete velocity;
        delete size;
    }
};

#endif // MESH_ENTRY_H
