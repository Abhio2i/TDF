#include "eo.h"
#include <cmath>
#include <iostream>

EO::EO(const eoSensorPayload &sensor, const Surrounding &surrounding)
{
    setSensor(sensor);
    setEnvironment(surrounding);
    //std::cout<<"\tElectro Optic Module is Inilisalized"<<std::endl;
}

EO::~EO()
{
    //std::cout<<"\tElectro Optic Module is Deinilisalized"<<std::endl;
}
float EO::getSetThreshold(float distance,float area)
{
    if (distance <= 0.0 || area <= 0.0){
        detectionThreshold = 0;
        return detectionThreshold;
    }
    maxRange = distance;
    detectionThreshold = area / (distance * distance);
    return detectionThreshold;
}

void EO::detectList(EO_PayLoad eo_payload){
    setSensor(eo_payload.eoParameters.sensor);
    setEnvironment(eo_payload.surrounding);
    const EntityList &el =
        eo_payload.entityList;
    PreProcessEntityList &ppel =
        eo_payload.preProcessEntityList;
    entityDetectedList = std::unordered_map<std::string,bool>();
    for(auto e = el.begin();
         e != el.end(); ++e){
        const PreProcessEntity ppe = ppel[(*e).first];
        entityDetectedList[(*e).first] =
            isDetected(ppe.frontalSurfaceArea,
                       ppe.distanceBtwUser,
                       ppe.angleBtwUser);
    }
}

float EO::getMaxRange(float maxArea)
{
    // float radius   = 10000.0,
    // float atmCoeff = 0.00001,
    // float rainRate = 0.0,
    // float fog      = 10000.0,
    // float humidity = 0.5
    if (maxArea <= 0.0) return 0.0;
    float range = sqrt(maxArea/detectionThreshold);
    return range;
}

void EO::setSensor(const eoSensorPayload &sensor) {
    detectionThreshold = sensor.detectionThreshold;
    sensorGain = sensor.sensorGain;
    maxRange   = sensor.maxRange;
    fov        = sensor.fov;
    if (sensor.detectionThreshold <= 0.0) {
        throw std::invalid_argument("Detection threshold must be > 0");
    }
}

void EO::setEnvironment(const Surrounding &surrounding) {
    k_atm      = surrounding.k_atm;
    k_rain     = surrounding.k_rain;
    k_fog      = surrounding.k_fog;
    k_humidity = surrounding.k_humidity;
}

float EO::computeSignal(float area,
                         float distance,
                         float angleDeg,
                         float illumination,
                         float glintFactor) const {

    if (distance <= 0.0) return 0.0;

    float signal = area / (distance * distance);

    float thetaRad = angleDeg * M_PI / 180.0;
    signal *= std::max(0.0, cos(thetaRad));

    // // Combined attenuation
    // float k_total = k_atm + k_rain + k_fog + k_humidity;

    // float Tatm = exp(-k_total * distance);

    // signal *= sensorGain;
    // signal *= Tatm;
    // signal *= illumination;
    // signal *= glintFactor;

    return signal;
}

// float EO::computeSignal(float area,
//                          float distance,
//                          float angleDeg,
//                          float illumination,
//                          float glintFactor ) const {

//     if (distance <= 0.0) return 0.0;

//     // Base signal
//     float signal = area / (distance * distance);

//     // Angle effect (less visible at angle)
//     float thetaRad = angleDeg * M_PI / 180.0;
//     signal *= std::max(0.0, cos(thetaRad));

//     // Atmospheric loss
//     float Tatm = exp(-atmCoeff * distance);

//     // Weather loss
//     float Tweather = exp(-rainCoeff * distance);

//     // Final signal
//     signal *= sensorGain;
//     //signal *= Tatm;
//     //signal *= Tweather;
//     signal *= illumination;
//     signal *= glintFactor;

//     return signal;
// }

bool EO::isDetected(float area,
                    float distance,
                    float angleDeg,
                    float illumination,
                    float glintFactor ) const {

    if (distance > maxRange) return false;
    if (angleDeg > fov / 2.0) return false;
    float signal = computeSignal(
        area, distance, angleDeg,
        illumination, glintFactor);

    bool result   = signal >= detectionThreshold;
    return result;
}

