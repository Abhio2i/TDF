#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H
namespace ECS {

struct ColliderSizeComponent{
    float Width;
    float Length;
    float Height;
};

struct ColliderWarningComponent{
    float CollideRadius = 0;
    float WarningRadius = 0 ;
    float blinkRadius = 0;
};

}
#endif // COLLIDER_COMPONENT_H
