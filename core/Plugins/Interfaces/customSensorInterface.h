#ifndef CUSTOMSENSORINTERFACE_H
#define CUSTOMSENSORINTERFACE_H
#include "core/Hierarchy/ECS/World/world.h"
#pragma once
#include <QtPlugin>

// Ye wo class hai jiske aap multiple instances chahte hain
class CustomSensorInterface {
public:
    virtual ~CustomSensorInterface() = default;
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
class SensorFactoryInterface {
public:
    virtual ~SensorFactoryInterface() = default;
    virtual void update() = 0;
    virtual void selectedEntity(QString ID) = 0;
    virtual void init(ECS::World* world) = 0;
    // Naya instance create karne ka function
    virtual CustomSensorInterface* createSensor(QString entityID,uint32_t ecsID,QString sensorID) = 0;
    virtual void RemoveSensor(QString entityID,uint32_t ecsID,QString sensorID) = 0;

    using registerSensorCallback = std::function<bool(std::string)>;
    registerSensorCallback registerSensor = nullptr;

    using unRegisterSensorCallback = std::function<bool(std::string)>;
    unRegisterSensorCallback unRegisterSensor = nullptr;

    using registerGetParametersCallback = std::function<bool(std::string)>;
    registerGetParametersCallback getParameters = nullptr;

    using registerGetParameterDataCallback = std::function<std::string(std::string)>;
    registerGetParameterDataCallback getParameterData = nullptr;

    using registerSetParameterDataCallback = std::function<bool(std::string)>;
    registerSetParameterDataCallback setParameterData = nullptr;

    using registerSensorPanelCallback = std::function<bool(QWidget*)>;
    registerSensorPanelCallback registerSensorPanel = nullptr;

    void registerNewSensorCallback(registerSensorCallback cb){
        registerSensor = cb;
    }

    void registerRemoveNewSensorCallback(registerSensorCallback cb){
        unRegisterSensor = cb;
    }

    void registeGetSensorParametersCallback(registerGetParametersCallback cb){
        getParameters = cb;
    }

    void registerGetSensorParameterDataCallback(registerGetParameterDataCallback cb){
        getParameterData = cb;
    }

    void registerSetSensorParameterDataCallback(registerSetParameterDataCallback cb){
        setParameterData = cb;
    }

    void registerSetSensorPanelCallback(registerSensorPanelCallback cb){
        registerSensorPanel = cb;
    }

    std::unordered_map<std::string,CustomSensorInterface*> sensors;
protected:
    ECS::World* m_world = nullptr; // ✅ Primary ECS World Pointer
};


// Qt Meta-Object system ke liye Interface declare karna zaroori hai
#define SensorFactoryInterface_iid "com.n2i.SensorFactoryInterface"
Q_DECLARE_INTERFACE(SensorFactoryInterface, SensorFactoryInterface_iid)
#endif // CUSTOMSENSORINTERFACE_H

