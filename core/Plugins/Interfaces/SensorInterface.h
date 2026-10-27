#ifndef SENSORINTERFACE_H
#define SENSORINTERFACE_H

#include <QtPlugin>
#include <string>

class SensorInterface {
public:
    virtual ~SensorInterface() = default;
    virtual void init() = 0;
    virtual void update(float delta) =0;
    virtual void entitySelected(std::string id) = 0;
    virtual void sensorAdded(std::string parentId,std::string ID) = 0;
    virtual void sensorRemove(std::string parentId,std::string ID) = 0;


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

};
// Qt ko batao ki yeh interface hai
Q_DECLARE_INTERFACE(SensorInterface, "com.linux.SensorInterface/1.0")

#endif // SENSORINTERFACE_H
