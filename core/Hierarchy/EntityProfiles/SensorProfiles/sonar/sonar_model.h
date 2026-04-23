#pragma once
#include <string>

// ─────────────────────────────────────────────
// sonar_model.h
// Core math + data structs
// Future: add more models here
// ─────────────────────────────────────────────

struct SonarInput
{
    float sourceLevel;          // SL (dB)
    float targetStrength;       // TS (dB)
    float noiseLevel;           // NL (dB)
    float detectionThreshold;   // DT (dB)
    float absorption;           // absorption coefficient
};

// ── SonarTarget struct ──
struct SonarTarget
{
    std::string name;
    double      lat;
    double      lon;
    float       depth;
    float       targetStrength;
    std::string id;
};

struct DetectionResult
{
    bool  detected;             // target mila ya nahi
    float distance;             // meters (echo se computed)
    float bearing;              // degrees 0-360 (lat/lon se)
    float signalExcess;         // dB (SNR - threshold)
    float confidence;           // 0.0 - 1.0
    std::string name;
    std::string reason;
    std::string category;
};

class SonarModel
{
public:
    static float computeTransmissionLoss(float distance,
                                         float absorption);

    // static float computeActiveSNR(float sourceLevel,
    //                               float transmissionLoss,
    //                               float targetStrength,
    //                               float noiseLevel);

    static float computeActiveSNR(float sourceLevel,
                                  float transmissionLoss,
                                  float targetStrength,
                                  float noiseLevel,
                                  float DI);

    static bool  detectionDecision(float snr,
                                  float threshold);

    static float computeBearing(double fromLat, double fromLon,
                                double toLat,   double toLon);

    static float computeConfidence(float signalExcess);

    static float geoDistance(double lat1, double lon1,
                             double lat2, double lon2);

    static float computeThorpAbsorption(float frequencyKHz);

    static float computeSnellRefraction(float theta1Deg,
                                        float c1,
                                        float c2);

    // Snell's Law — ray bending
    static float computeSnellAngle(float theta1_deg,
                                   float c1,
                                   float c2);

    static float applyConvergenceZone(float rangeMeters, float tl);

    static float validateDetectionThreshold(float dt);

    static float computeFOM(float SL,
                            float NL,
                            float DI,
                            float DT);

    static float computeReverbVolume(float c,
                                     float tau,
                                     float R,
                                     float psi);

    static float computeVolumeReverbSNR(float SL,
                                        float TL,
                                        float Sv,
                                        float V);

    static float computeBottomReverbSNR(float SL,
                                        float TL,
                                        float Sb,
                                        float A);

    static float computeDopplerShift(float v,
                                     float f0,
                                     float c);

    static float computeVelocityFromDoppler(float deltaF,
                                            float f0,
                                            float c);

    static float computeDopplerLoss(float deltaF,
                                    float T);

    static float computeDopplerFrequency(float f0,
                                         float v_tx,
                                         float v_rx,
                                         float c);

};
