#ifndef CUSTOMIFFINTERFACE_H
#define CUSTOMIFFINTERFACE_H
#include "core/Hierarchy/ECS/World/world.h"
#pragma once
#include <QtPlugin>

// Ye wo class hai jiske aap multiple instances chahte hain
class CustomIFFInterface {
public:
    virtual ~CustomIFFInterface() = default;
    bool isdestroy = false;
    ECS::Entity m_ecsID = 0;
    void setWorld(ECS::World* world) { m_world = world; };
    virtual void update() = 0;
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& obj) = 0;
    using registerDestructorCallback = std::function<void()>;
    registerDestructorCallback destroy = nullptr;

    void registerDestroyCallback(registerDestructorCallback cb){
        destroy = cb;
    }

    // QJsonObject AdditionalParameters;
protected:
    ECS::World* m_world = nullptr; // ✅ Primary ECS World Pointer
};

// Ye factory hai jo naye instances banayegi
class IFFFactoryInterface {
public:
    virtual ~IFFFactoryInterface() = default;
    virtual void update() = 0;
    virtual void selectedEntity(QString ID) = 0;
    virtual void init(ECS::World* world) = 0;
    // Naya instance create karne ka function
    virtual CustomIFFInterface* createIFF(QString entityID,uint32_t ecsID,QString IFFID) = 0;
    virtual void RemoveIFF(QString entityID,uint32_t ecsID,QString IFFID) = 0;

    using registerIFFCallback = std::function<bool(std::string)>;
    registerIFFCallback registerIFF = nullptr;

    using unRegisterIFFCallback = std::function<bool(std::string)>;
    unRegisterIFFCallback unRegisterIFF = nullptr;

    using registerGetParametersCallback = std::function<bool(std::string)>;
    registerGetParametersCallback getParameters = nullptr;

    using registerGetParameterDataCallback = std::function<std::string(std::string)>;
    registerGetParameterDataCallback getParameterData = nullptr;

    using registerSetParameterDataCallback = std::function<bool(std::string)>;
    registerSetParameterDataCallback setParameterData = nullptr;

    using registerIFFPanelCallback = std::function<bool(QWidget*)>;
    registerIFFPanelCallback registerIFFPanel = nullptr;

    void registerNewIFFCallback(registerIFFCallback cb){
        registerIFF = cb;
    }

    void registerRemoveNewIFFCallback(registerIFFCallback cb){
        unRegisterIFF = cb;
    }

    void registeGetIFFParametersCallback(registerGetParametersCallback cb){
        getParameters = cb;
    }

    void registerGetIFFParameterDataCallback(registerGetParameterDataCallback cb){
        getParameterData = cb;
    }

    void registerSetIFFParameterDataCallback(registerSetParameterDataCallback cb){
        setParameterData = cb;
    }

    void registerSetIFFPanelCallback(registerIFFPanelCallback cb){
        registerIFFPanel = cb;
    }

    std::unordered_map<std::string,CustomIFFInterface*> IFFs;
protected:
    ECS::World* m_world = nullptr; // ✅ Primary ECS World Pointer
};


// Qt Meta-Object system ke liye Interface declare karna zaroori hai
#define IFFFactoryInterface_iid "com.n2i.IFFFactoryInterface"
Q_DECLARE_INTERFACE(IFFFactoryInterface, IFFFactoryInterface_iid)

#endif // CUSTOMIFFINTERFACE_H
