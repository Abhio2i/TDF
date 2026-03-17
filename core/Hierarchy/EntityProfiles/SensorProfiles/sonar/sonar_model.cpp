#include "sonar_model.h"
#include <cmath>

static constexpr double PI = 3.14159265358979;

float SonarModel::computeTransmissionLoss(float distance,
                                          float absorption)
{
    if (distance <= 0.0f)
        return 0.0f;

    // Spherical spreading + absorption
    return 20.0f * std::log10(distance) + absorption * distance;
}

float SonarModel::computeActiveSNR(float sourceLevel,
                                   float transmissionLoss,
                                   float targetStrength,
                                   float noiseLevel)
{
    // Active sonar equation: SL - 2TL + TS - NL
    return sourceLevel
           - (2.0f * transmissionLoss)
           + targetStrength
           - noiseLevel;
}

bool SonarModel::detectionDecision(float snr, float threshold)
{
    return snr >= threshold;
}

float SonarModel::computeBearing(double fromLat, double fromLon,
                                 double toLat,   double toLon)
{
    // Forward azimuth formula
    double lat1 = fromLat * PI / 180.0;
    double lat2 = toLat   * PI / 180.0;
    double dLon = (toLon - fromLon) * PI / 180.0;

    double x = std::sin(dLon) * std::cos(lat2);
    double y = std::cos(lat1) * std::sin(lat2)
               - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);

    double bearing = std::atan2(x, y) * 180.0 / PI;

    // Normalize to 0-360
    return static_cast<float>(std::fmod(bearing + 360.0, 360.0));
}

float SonarModel::computeConfidence(float signalExcess)
{
    // signalExcess <= 0  → no confidence
    // signalExcess >= 40 → full confidence
    if (signalExcess <= 0.0f)  return 0.0f;
    if (signalExcess >= 40.0f) return 1.0f;

    return signalExcess / 40.0f;
}

float SonarModel::geoDistance(double lat1, double lon1,
                              double lat2, double lon2)
{
    static constexpr double R = 6371000.0;
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;

    double a = std::sin(dLat/2) * std::sin(dLat/2)
               + std::cos(lat1 * PI/180.0)
                     * std::cos(lat2 * PI/180.0)
                     * std::sin(dLon/2) * std::sin(dLon/2);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return static_cast<float>(R * c);
}
