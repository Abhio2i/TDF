#ifndef META_DATA_COMPONENT_H
#define META_DATA_COMPONENT_H
namespace ECS {

#pragma pack(push, 1)
struct Entity_Data{
    bool active = true;
    bool destroy = false;
};
#pragma pack(pop)

}
#endif // META_DATA_COMPONENT_H
