#ifndef EO_H
#define EO_H
#include "payload.h"
#include <unordered_map>
class EO
{
public:
    EO(const CustomSensor &sensor, const Surrounding &surrounding);
    ~EO();
    bool   isSensorSet = false;
    double detectionThreshold;
    double sensorGain;

    double illumination   = 1.0; // day/night factor
    double glintFactor    = 1.0;  // reflection boost
    double maxRange;
    double fov;

    // NEW ENVIRONMENT FACTORS
    bool   isEnvironmentSet = false;

    double k_atm      = 1.0;  // atmospheric attenuation
    double k_rain     = 1.0;  // rain/fog attenuation
    double k_fog      = 1.0;  // Fog
    double k_humidity = 1.0;  // Humidity



    //Detected List
    std::unordered_map<std::string,bool> entityDetectedList;
public:
    void detectList(EO_PayLoad eo_payload);
    double getMaxRange(double maxArea);
    void detectBySNR();
    void setEnvironment(const Surrounding &surrounding);
    void setSensor(const CustomSensor &sensor);
    double computeSignal(double area,
                         double distance,
                         double angleDeg,
                         double illumination,
                         double glintFactor ) const;

    bool isDetected(double area,
                    double distance,
                    double angleDeg,
                    double illumination = 1.0,
                    double glintFactor  = 1.0
                    ) const;
    double getThreshold()
    {
        return detectionThreshold;
    }
private:
    EO_PayLoad m_eo_payload;
};

#endif // EO_H
