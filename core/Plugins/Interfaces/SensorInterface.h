#ifndef SENSORINTERFACE_H
#define SENSORINTERFACE_H

#include <QtPlugin>
#include <string>

class SensorInterface {
public:
    virtual ~SensorInterface() = default;

    using registerSensorCallback = std::function<bool(std::string)>;
    registerSensorCallback registerSensor = nullptr;

    using unRegisterSensorCallback = std::function<bool(std::string)>;
    unRegisterSensorCallback unRegisterSensor = nullptr;

    void registerNewSensorCallback(registerSensorCallback cb){
        registerSensor = cb;
    }

    void registerRemoveNewSensorCallback(registerSensorCallback cb){
        unRegisterSensor = cb;
    }

};
// Qt ko batao ki yeh interface hai
Q_DECLARE_INTERFACE(SensorInterface, "com.linux.SensorInterface/1.0")

#endif // SENSORINTERFACE_H
