#include "stanag_4347.h"
#include <cmath>

STANAG_4347::STANAG_4347() {}

float STANAG_4347::deltaTemp(float targetTemp, float backgroundTemp)
{
    return (targetTemp > backgroundTemp) ?
               targetTemp - backgroundTemp :
               backgroundTemp - targetTemp ;
}

float STANAG_4347::atmTransmission(float extinctionCoeff, float range)
{
    float power = -1*extinctionCoeff*range;
    return exp(power);
}

float STANAG_4347::NETD(float rmsNoise, float deltaSignalByDeltaTemp)
{
    return rmsNoise/deltaSignalByDeltaTemp;
}

float STANAG_4347::MTF(float outputContrast, float inputContrast)
{
    return outputContrast/inputContrast;
}

float STANAG_4347::MRTD(float NETDValue, float MTFValue)
{
    return NETDValue/MTFValue;
}

float STANAG_4347::MRTDAverage(float MRTDVertical, float MRTDHorizontal)
{
    return (MRTDHorizontal+MRTDVertical)/2;
}

float STANAG_4347::calculateNominalStaticRanges(float MRTDAverage,
    float deltaTemp, TargetSize targetSize, Resolution_Criteria resolution_criteria)
{
    if(MRTDAverage <= 0.0f)
    {
        return 0.0f;
    }

    if(deltaTemp <= 0.0f)
    {
        return 0.0f;
    }

    // Spatial frequency (cycles/mrad)
    float spatialFrequency = deltaTemp / MRTDAverage;

    // Convert cycles/mrad -> cycles/rad
    spatialFrequency *= 1000.0f;

    float targetDimension =
        std::sqrt(targetSize.x * targetSize.y);

    float result =
        (targetDimension * spatialFrequency) /
        static_cast<float>(resolution_criteria);

    return result*1000;
}
