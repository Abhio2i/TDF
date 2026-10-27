#ifndef CUSTOMRADIOINTERFACE_H
#define CUSTOMRADIOINTERFACE_H

#include "core/Hierarchy/ECS/World/world.h"
#pragma once
#include <QtPlugin>

// Ye wo class hai jiske aap multiple instances chahte hain
class CustomRadioInterface {
public:
    virtual ~CustomRadioInterface() = default;
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
class RadioFactoryInterface {
public:
    virtual ~RadioFactoryInterface() = default;
    virtual void update() = 0;
    virtual void selectedEntity(QString ID) = 0;
    virtual void init(ECS::World* world) = 0;
    // Naya instance create karne ka function
    virtual CustomRadioInterface* createRadio(QString entityID,uint32_t ecsID,QString RadioID) = 0;
    virtual void RemoveRadio(QString entityID,uint32_t ecsID,QString RadioID) = 0;

    using registerRadioCallback = std::function<bool(std::string)>;
    registerRadioCallback registerRadio = nullptr;

    using unRegisterRadioCallback = std::function<bool(std::string)>;
    unRegisterRadioCallback unRegisterRadio = nullptr;

    using registerGetParametersCallback = std::function<bool(std::string)>;
    registerGetParametersCallback getParameters = nullptr;

    using registerGetParameterDataCallback = std::function<std::string(std::string)>;
    registerGetParameterDataCallback getParameterData = nullptr;

    using registerSetParameterDataCallback = std::function<bool(std::string)>;
    registerSetParameterDataCallback setParameterData = nullptr;

    using registerRadioPanelCallback = std::function<bool(QWidget*)>;
    registerRadioPanelCallback registerRadioPanel = nullptr;

    void registerNewRadioCallback(registerRadioCallback cb){
        registerRadio = cb;
    }

    void registerRemoveNewRadioCallback(registerRadioCallback cb){
        unRegisterRadio = cb;
    }

    void registeGetRadioParametersCallback(registerGetParametersCallback cb){
        getParameters = cb;
    }

    void registerGetRadioParameterDataCallback(registerGetParameterDataCallback cb){
        getParameterData = cb;
    }

    void registerSetRadioParameterDataCallback(registerSetParameterDataCallback cb){
        setParameterData = cb;
    }

    void registerSetRadioPanelCallback(registerRadioPanelCallback cb){
        registerRadioPanel = cb;
    }

    std::unordered_map<std::string,CustomRadioInterface*> Radios;
protected:
    ECS::World* m_world = nullptr; // ✅ Primary ECS World Pointer
};


// Qt Meta-Object system ke liye Interface declare karna zaroori hai
#define RadioFactoryInterface_iid "com.n2i.RadioFactoryInterface"
Q_DECLARE_INTERFACE(RadioFactoryInterface, RadioFactoryInterface_iid)

#endif // CUSTOMRADIOINTERFACE_H
