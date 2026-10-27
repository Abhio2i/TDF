#ifndef ENVIRONMENT_COMPONENT_H
#define ENVIRONMENT_COMPONENT_H
#include "qvector3d.h"
namespace ECS {

struct WindSpeedComponent{
    QVector3D direction;
};

struct enviorment_Data_Component{
    float Temprature = 30;       // Ambient temperature (deg C)
    float humidity = 30;         // Relative humidity (%)
    float rain = 0;              // Precipitation rate (mm/h)
    float fog = 0;               // Visibility reduction (%)
    float AtmoshPhericPressure= 0;
    float AirDensity = 0;
    float gasAttenuation = 0;
    float shadowZone = 0;
};

}
#endif // ENVIRONMENT_COMPONENT_H
