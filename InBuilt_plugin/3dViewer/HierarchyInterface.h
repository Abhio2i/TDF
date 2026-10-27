#ifndef HIERARCHYINTERFACE_H
#define HIERARCHYINTERFACE_H
#include <QtPlugin>
#include <string>

class HierarchyInterface {
public:
    struct geoData{
        float lat;
        float lon;
        float alt;
        float head;
        float pitch;
        float roll;
        float yaw;
    };
    struct transformData{
        float x;
        float y;
        float z;
        float rotX;
        float rotY;
        float rotZ;
        float rotW;
        float scaleX;
        float scaleY;
        float scaleZ;
    };

    struct dynamicData{
        float speed;
        float climbrate;
    };

    struct meshData{
        std::string bitmap;
        std::string model;
        std::string texture;
    };

    virtual ~HierarchyInterface() = default;
    // Simple function: parameters pass ho rahe hain aur int return ho raha hai
    virtual void entityAdded(std::string id,std::string type) = 0;
    virtual void entityRemoved(std::string id) = 0;
    virtual void entitySelected(std::string id) = 0;

    virtual void update(float delta) =0;

    using EntityCreateCallback = std::function<bool(std::string,std::string)>;
    EntityCreateCallback createEntity = nullptr;

    using EntityJsonCreateCallback = std::function<bool(std::string,std::string,std::string)>;
    EntityJsonCreateCallback createEntityByJson = nullptr;

    using EntityRenameCallback = std::function<bool(std::string,std::string)>;
    EntityRenameCallback renameEntity = nullptr;

    using EntityRemoveCallback = std::function<bool(std::string,std::string)>;
    EntityRemoveCallback removeEntity = nullptr;

    using EntitiesDataCallback = std::function<std::vector<std::string>(std::string)>;
    EntitiesDataCallback getAllEntitiesByType = nullptr;

    using EntityGeoDataCallback = std::function<geoData(std::string)>;
    EntityGeoDataCallback getEntityGeoDataByID = nullptr;

    using EntityTransformCallback = std::function<transformData(std::string)>;
    EntityTransformCallback getEntityTransformByID = nullptr;

    using EntityDynamicCallback = std::function<dynamicData(std::string)>;
    EntityDynamicCallback getEntityDynamicByID = nullptr;

    using EntityMeshDataCallback = std::function<meshData(std::string)>;
    EntityMeshDataCallback getEntityMeshDataByID = nullptr;

    void registerCreateEntityCallback(EntityCreateCallback cb){
        createEntity = cb;
    }

    void registerCreateEntityByJsonCallback(EntityJsonCreateCallback cb){
        createEntityByJson = cb;
    }

    void registerRenameEntityCallback(EntityRenameCallback cb){
        renameEntity = cb;
    }

    void registerRemoveEntityCallback(EntityRemoveCallback cb){
        removeEntity = cb;
    }

    void registerGetEntityByTypeCallback(EntitiesDataCallback cb){
        getAllEntitiesByType = cb;
    }

    void registerGetEntityGeoDataByIDCallback(EntityGeoDataCallback cb){
        getEntityGeoDataByID = cb;
    }

    void registerGetEntityTransformByIDCallback(EntityTransformCallback cb){
        getEntityTransformByID = cb;
    }

    void registerGetEntityDynamicByIDCallback(EntityDynamicCallback cb){
        getEntityDynamicByID = cb;
    }

    void registerGetEntityMeshDataByIDCallback(EntityMeshDataCallback cb){
        getEntityMeshDataByID = cb;
    }
};
// Qt ko batao ki yeh interface hai
Q_DECLARE_INTERFACE(HierarchyInterface, "com.linux.HierarchyInterface/1.0")

#endif // HIERARCHYINTERFACE_H
