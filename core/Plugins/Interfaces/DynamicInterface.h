#ifndef DYNAMICINTERFACE_H
#define DYNAMICINTERFACE_H

#include "core/Hierarchy/ECS/World/world.h"
#pragma once
#include <QtPlugin>


// Ye factory hai jo naye instances banayegi
class DynamicFactoryInterface {
public:
    virtual ~DynamicFactoryInterface() = default;
    virtual void update() = 0;
    virtual void selectedEntity(QString ID) = 0;
    virtual void init(ECS::World* world) = 0;
    // Naya instance create karne ka function
    // virtual CustomIFFInterface* createIFF(QString entityID,uint32_t ecsID,QString IFFID) = 0;
    // virtual void RemoveIFF(QString entityID,uint32_t ecsID,QString IFFID) = 0;

    // using registerIFFCallback = std::function<bool(std::string)>;
    // registerIFFCallback registerIFF = nullptr;

    // using unRegisterIFFCallback = std::function<bool(std::string)>;
    // unRegisterIFFCallback unRegisterIFF = nullptr;

    // using registerGetParametersCallback = std::function<bool(std::string)>;
    // registerGetParametersCallback getParameters = nullptr;

    // using registerGetParameterDataCallback = std::function<std::string(std::string)>;
    // registerGetParameterDataCallback getParameterData = nullptr;

    // using registerSetParameterDataCallback = std::function<bool(std::string)>;
    // registerSetParameterDataCallback setParameterData = nullptr;

    // using registerIFFPanelCallback = std::function<bool(QWidget*)>;
    // registerIFFPanelCallback registerIFFPanel = nullptr;

    // void registerNewIFFCallback(registerIFFCallback cb){
    //     registerIFF = cb;
    // }

    // void registerRemoveNewIFFCallback(registerIFFCallback cb){
    //     unRegisterIFF = cb;
    // }

    // void registeGetIFFParametersCallback(registerGetParametersCallback cb){
    //     getParameters = cb;
    // }

    // void registerGetIFFParameterDataCallback(registerGetParameterDataCallback cb){
    //     getParameterData = cb;
    // }

    // void registerSetIFFParameterDataCallback(registerSetParameterDataCallback cb){
    //     setParameterData = cb;
    // }

    // void registerSetIFFPanelCallback(registerIFFPanelCallback cb){
    //     registerIFFPanel = cb;
    // }

    // std::unordered_map<std::string,CustomIFFInterface*> IFFs;
protected:
    ECS::World* m_world = nullptr; // ✅ Primary ECS World Pointer
};


// Qt Meta-Object system ke liye Interface declare karna zaroori hai
#define DynamicFactoryInterface_iid "com.n2i.DynamicFactoryInterface"
Q_DECLARE_INTERFACE(DynamicFactoryInterface, DynamicFactoryInterface_iid)

#endif // DYNAMICINTERFACE_H
