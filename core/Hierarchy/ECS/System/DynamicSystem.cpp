#include "core/Hierarchy/ECS/Struct/trajectory_component.h"
#include "core/Hierarchy/ECS/Struct/transform_component.h"
#include "core/Hierarchy/ECS/World/world.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include <QtConcurrent>
#include <cmath>
namespace ECS {

void DynamicSystem(float delta, ECS::World* m_world) {
    if (!m_world) return;

    auto& transforms = m_world->GetComponentsArray<ECS::TransformComponent>();
    const auto& trajectorys = m_world->GetComponentsArray<ECS::trajectoryPathComponent>();
    const auto& trajectoryDynamics = m_world->GetComponentsArray<ECS::trajectoryDynamicComponent>();

    int size = std::min({(int)transforms.size(), (int)trajectorys.size(), (int)trajectoryDynamics.size()});

    // 1. Threading ke liye Indices ki list banayein (0, 1, 2, ... size-1)
    std::vector<int> indices(size);
    std::iota(indices.begin(), indices.end(), 0);

    // 2. Un indices par Blocking Map chalayein (Perfect Multi-threading)
    QtConcurrent::blockingMap(indices, [&](int i) {
        int currentWay = trajectoryDynamics[i].currentWaypoint;

        // Bounds Check: path fixed [50] size ka array hai, isliye .size() hata diya
        if (currentWay >= 50) return;

        // 1. Get Waypoint Flat Coordinates
        FlatXYZ xyz = geoToFlatXYZ(trajectorys[i].path[currentWay].lat,
                                   trajectorys[i].path[currentWay].lon,
                                   0);

        // 2. Map Flat Coordinates to 3D World Space (X+ = Right, Y+ = Up, Z+ = Forward)
        QVector3D targetPos(xyz.x, transforms[i].position.y(), xyz.y);
        QVector3D diff = targetPos - transforms[i].position;

        // Distance check to prevent jitter near target
        if (diff.lengthSquared() < 0.01f) return;

        // 3. Heading (Yaw) Calculation (atan2(x, z) because Z+ is forward)
        float heading = std::atan2(diff.x(), diff.z()) * (180.0f / M_PI);

        // 4. Update Rotation (Yaw set on Y-axis)
        QVector3D euAngle = transforms[i].rotation.toEulerAngles();
        euAngle.setY(heading);
        QQuaternion newRotation = QQuaternion::fromEulerAngles(euAngle);

        // 5. Calculate Forward Movement (Z+ Vector rotated by transform orientation)
        QVector3D forwardVector = newRotation.rotatedVector(QVector3D(0.0f, 0.0f, 1.0f));
        QVector3D newPosition = transforms[i].position + (forwardVector * 10.0f * delta);

        // 6. Apply Updated Position & Rotation back to Component
        transforms[i].position = newPosition;
        transforms[i].rotation = newRotation;
    });
}
}
