#ifndef EO_H
#define EO_H
#include "payload.h"
#include <unordered_map>
class EO
{
public:
    EO(const eoSensorPayload &sensor, const Surrounding &surrounding);
    ~EO();
    bool   isSensorSet = false;
    float detectionThreshold;
    float sensorGain;

    float illumination   = 1.0; // day/night factor
    float glintFactor    = 1.0;  // reflection boost
    float maxRange;
    float fov;

    // NEW ENVIRONMENT FACTORS
    bool   isEnvironmentSet = false;

    float k_atm      = 1.0;  // atmospheric attenuation
    float k_rain     = 1.0;  // rain/fog attenuation
    float k_fog      = 1.0;  // Fog
    float k_humidity = 1.0;  // Humidity



    //Detected List
    std::unordered_map<std::string,bool> entityDetectedList;
public:
    void detectList(EO_PayLoad eo_payload);
    float getMaxRange(float maxArea);
    void detectBySNR();
    void setEnvironment(const Surrounding &surrounding);
    void setSensor(const eoSensorPayload &sensor);
    float computeSignal(float area,
                         float distance,
                         float angleDeg,
                         float illumination,
                         float glintFactor ) const;

    bool isDetected(float area,
                    float distance,
                    float angleDeg,
                    float illumination = 1.0,
                    float glintFactor  = 1.0
                    ) const;
    float getSetThreshold(float distance,float area);
    float getThreshold()
    {
        return detectionThreshold;
    }
private:
    EO_PayLoad m_eo_payload;
};

#endif // EO_H
