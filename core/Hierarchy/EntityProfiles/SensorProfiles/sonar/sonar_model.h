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

    static float computeActiveSNR(float sourceLevel,
                                  float transmissionLoss,
                                  float targetStrength,
                                  float noiseLevel);

    static bool  detectionDecision(float snr,
                                  float threshold);

    static float computeBearing(double fromLat, double fromLon,
                                double toLat,   double toLon);

    static float computeConfidence(float signalExcess);

    static float geoDistance(double lat1, double lon1,
                             double lat2, double lon2);
};
