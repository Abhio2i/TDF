#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H
#include "qvector4d.h"
#include <cstdint>
namespace ECS {

struct SpriteComponent {
    uint32_t texture_id{0};
    float scale{30.0f};
    QVector4D uvRect{0.0f, 0.0f, 1.0f, 1.0f};
    QVector4D colorData{1.0f, 1.0f, 1.0f, 1.0f};
};

struct SpritePath{
    char path[128];
    float scale = 1.0f;
};


}
#endif // MESH_COMPONENT_H
