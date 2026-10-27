#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H
#include <QVector3D>
#include <QQuaternion>

namespace ECS {

struct TransformComponent {
    QVector3D position{0.0f, 0.0f, 0.0f};
    QQuaternion rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct GeocordComponent {
    double lat;
    double lon;
    float altitude;
    float heading;
};

} // namespace ECS

#endif // TRANSFORM_COMPONENT_H
