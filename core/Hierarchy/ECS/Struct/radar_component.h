#ifndef RADAR_COMPONENT_H
#define RADAR_COMPONENT_H
#include <cstdint>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"
namespace ECS {

struct RadarArrayConstants{
    int numElements;
    float peakPowerPerElement_W;
    float moduleEfficiency;
    float maxDutyCycle;
};

struct RadarBeamConstants{
    double frequency_Hz;
    float antennaGain;
    double antennaBandwidth;
    float beamWidth;
    float maxSteeringAngle_deg;
    float minElevation;
    float maxElevation;
    float minAzimuth;
    float maxAzimuth;
};

struct RadarSidelobeConstants{
    aesa::SidelobeMode sidelobeMode;
    float peakSidelobeLevel;
    float avgSidelobeLevel;
    float sidelobeBlanking_dB;
};

struct RadarDwellConstants{
    float searchDwellTime_ms;
    float trackDwellTime_ms;
    float fireControlDwellTime_ms;
};

struct RadarPRFConstants{
    aesa::PRFType prfType;
    float prfLevels[4];
    bool frequencyAgility;
    float hopStartFrequency;
    float hopStopFrequency;
};

struct RadarReceiverConstants{
    double systemTemperature_K;
    double noiseFigure_dB;
    double targetPfa;
};

struct RadarTrackingConstants{
    int missedScansToDrop;
    double trackCoastSeconds;
    int minHitsToValidate;
    double maxTrackSpeed;
    double manoeuvreThreshold_m;
    bool useJPDA;
    float jpdaFalseAlarmDensity;
};

struct RadarModeControls{
    aesa::RadarMode mode;
    aesa::DetectionCategory targetCategory;
    uint32_t lockedTargetID;
};

struct RadarArrayVariables{
    int failedModules;
};

struct RadarPlatformVariables{
    double radarHeight;
    float platformSpeed_m_s;
};

struct RadarBeamVariables{
    double currentAzimuth;
    double currentElevation;
    float currentSpoilFactor;
    double effectiveBeamWidth;
    bool scanBoundaryOccurred;
};

}
#endif // RADAR_COMPONENT_H
