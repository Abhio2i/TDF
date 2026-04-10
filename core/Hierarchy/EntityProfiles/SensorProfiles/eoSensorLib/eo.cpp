#include "eo.h"
#include <cmath>
#include <iostream>

EO::EO(const CustomSensor &sensor, const Surrounding &surrounding)
{
    setSensor(sensor);
    setEnvironment(surrounding);
    //std::cout<<"\tElectro Optic Module is Inilisalized"<<std::endl;
}

EO::~EO()
{
    //std::cout<<"\tElectro Optic Module is Deinilisalized"<<std::endl;
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

double EO::getMaxRange(double maxArea)
{
    if (maxArea <= 0.0) return 0.0;
    double range = sqrt(maxArea/detectionThreshold);
    return range;
}

void EO::setSensor(const CustomSensor &sensor) {
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

double EO::computeSignal(double area,
                         double distance,
                         double angleDeg,
                         double illumination,
                         double glintFactor) const {

    if (distance <= 0.0) return 0.0;

    double signal = area / (distance * distance);

    double thetaRad = angleDeg * M_PI / 180.0;
    signal *= std::max(0.0, cos(thetaRad));

    // Combined attenuation
    double k_total = k_atm + k_rain + k_fog + k_humidity;

    double Tatm = exp(-k_total * distance);

    signal *= sensorGain;
    signal *= Tatm;
    signal *= illumination;
    signal *= glintFactor;

    return signal;
}

// double EO::computeSignal(double area,
//                          double distance,
//                          double angleDeg,
//                          double illumination,
//                          double glintFactor ) const {

//     if (distance <= 0.0) return 0.0;

//     // Base signal
//     double signal = area / (distance * distance);

//     // Angle effect (less visible at angle)
//     double thetaRad = angleDeg * M_PI / 180.0;
//     signal *= std::max(0.0, cos(thetaRad));

//     // Atmospheric loss
//     double Tatm = exp(-atmCoeff * distance);

//     // Weather loss
//     double Tweather = exp(-rainCoeff * distance);

//     // Final signal
//     signal *= sensorGain;
//     //signal *= Tatm;
//     //signal *= Tweather;
//     signal *= illumination;
//     signal *= glintFactor;

//     return signal;
// }

bool EO::isDetected(double area,
                    double distance,
                    double angleDeg,
                    double illumination,
                    double glintFactor ) const {

    // if (distance > maxRange) return false;
    if (angleDeg > fov / 2.0) return false;
    double signal = computeSignal(
        area, distance, angleDeg,
        illumination, glintFactor);

    bool result   = signal >= detectionThreshold;
    return result;
}
