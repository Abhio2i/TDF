#include "sonar_model.h"
#include <cmath>
#include <QDebug>
#include <QtMath>

static constexpr double PI = 3.14159265358979;

float SonarModel::computeTransmissionLoss(float distance,
                                          float absorption)
{
    if (distance <= 0.0f)
        return 0.0f;

    // Spherical spreading + absorption
    // return 20.0f * std::log10(distance) + absorption * distance;
    return 20.0f * std::log10(distance)
           + absorption * (distance / 1000.0f);
}

float SonarModel::computeActiveSNR(float sourceLevel,
                                   float transmissionLoss,
                                   float targetStrength,
                                   float noiseLevel,
                                   float DI)
{
    // Active sonar equation: SL - 2TL + TS - NL + DI
    float snr = sourceLevel
                - (2.0f * transmissionLoss)
                + targetStrength
                - noiseLevel
                + DI;

    return snr;
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

float SonarModel::computeThorpAbsorption(float frequencyKHz)
{
    frequencyKHz = std::abs(frequencyKHz);

    if (frequencyKHz > 1000.0f)
        frequencyKHz = 1000.0f;

    float f2 = frequencyKHz * frequencyKHz;

    return
        0.003f +
        (0.11f * f2) / (1.0f + f2) +
        (44.0f * f2) / (4100.0f + f2) * 0.0001f;
}

float SonarModel::computeSnellRefraction(float theta1Deg,
                                         float c1,
                                         float c2)
{
    theta1Deg = std::abs(theta1Deg);

    if (c1 <= 0.0f || c2 <= 0.0f)
        return theta1Deg;   // fallback safe default

    float theta1Rad = theta1Deg * PI / 180.0f;

    float value = (c2 / c1) * std::sin(theta1Rad);

    // clamp to avoid asin crash
    if (value > 1.0f)
        return 90.0f;

    if (value < -1.0f)
        value = -1.0f;

    float theta2Rad = std::asin(value);

    return theta2Rad * 180.0f / PI;
}

float SonarModel::computeSnellAngle(float theta1_deg,
                                    float c1,
                                    float c2)
{
    // zero case
    if (c1 <= 0.0f || c2 <= 0.0f)
    {
        qDebug() << "[Snell] c2=0 — using default θ2=θ1";
        return theta1_deg;
    }

    // negative angle handle
    float absTheta = std::abs(theta1_deg);
    bool  negative = (theta1_deg < 0.0f);

    //  normal incidence
    if (absTheta == 0.0f)
        return 0.0f;  // θ2=0°

    // Same medium
    if (std::abs(c1 - c2) < 0.001f)
        return theta1_deg;  // θ2=θ1

    // sin(θ1) * c2/c1
    float theta1_rad = qDegreesToRadians(absTheta);
    float sinTheta1  = std::sin(theta1_rad);

    float sinTheta2 = (c2 / c1) * sinTheta1;

    //  Critical angle check
    // sinTheta2 >= 1.0 → Total Internal Reflection → duct
    // clamp / critical angle
    if (sinTheta2 >= 1.0f)
        return negative ? -90.0f : 90.0f;

    if (sinTheta2 <= -1.0f)
        return negative ? -90.0f : 90.0f;

    float theta2 = qRadiansToDegrees(std::asin(sinTheta2));

    return negative ? -theta2 : theta2;
}

float SonarModel::applyConvergenceZone(float rangeMeters, float tl)
{
    // SON-106 Zero / Negative
    if (rangeMeters <= 0.0f)
        return tl;

    float rangeKm = rangeMeters / 1000.0f;

    // Typical CZ spacing ~50 km
    float czSpacing = 50.0f;

    // Find which CZ number
    int zoneIndex = static_cast<int>(rangeKm / czSpacing);

    if (zoneIndex <= 0)
        return tl;   // BoundaryLow → no CZ

    // Apply gain per zone
    float gainPerCZ = 12.0f;  // dB (example realistic)

    float totalGain = zoneIndex * gainPerCZ;

    // Reduce TL (signal stronger)
    float newTL = tl - totalGain;

    return newTL;
}

float SonarModel::validateDetectionThreshold(float dt)
{
    // NaN check
    if (std::isnan(dt))
    {
        qDebug() << "[DT] Invalid NaN — using default 10 dB";
        return 10.0f;
    }

    // Negative clamp
    if (dt < 0.0f)
    {
        qDebug() << "[DT] Negative — clamped to 0";
        return 0.0f;
    }

    // Upper bound (optional but safe)
    if (dt > 30.0f)
    {
        qDebug() << "[DT] Too high — clamped to 30";
        return 30.0f;
    }

    return dt;
}

float SonarModel::computeFOM(float SL,
                             float NL,
                             float DI,
                             float DT)
{
    // 🔹 NaN handling (Extreme case safety)
    if (std::isnan(SL) || std::isnan(NL) ||
        std::isnan(DI) || std::isnan(DT))
    {
        qDebug() << "[FOM] NaN detected — returning 0";
        return 0.0f;
    }

    // 🔹 Negative NL (SON-110 Negative)
    if (NL < 0.0f)
    {
        qDebug() << "[FOM] Negative NL — clamped to 0";
        NL = 0.0f;
    }

    // 🔹 Normal formula
    float fom = SL - NL + DI - DT;

    return fom;
}

float SonarModel::computeReverbVolume(float c,
                                      float tau,
                                      float R,
                                      float psi)
{
    tau = std::abs(tau);

    if (R <= 0.0f || psi <= 0.0f || c <= 0.0f)
        return 0.0f;

    return (c * tau * R * R * psi) / 2.0f;
}

float SonarModel::computeVolumeReverbSNR(float SL,
                                         float TL,
                                         float Sv,
                                         float V)
{
    if (std::isnan(SL) || std::isnan(TL) ||
        std::isnan(Sv) || std::isnan(V))
    {
        qDebug() << "[VOL REVERB] NaN input";
        return 0.0f;
    }

    // V = 0 → avoid log(0)
    if (V <= 0.0f)
    {
        qDebug() << "[VOL REVERB] V=0 → -inf";
        return -INFINITY;
    }

    float term = Sv + 10.0f * log10(V);

    return SL - (2.0f * TL) - term;
}

float SonarModel::computeBottomReverbSNR(float SL,
                                         float TL,
                                         float Sb,
                                         float A)
{
    if (std::isnan(SL) || std::isnan(TL) ||
        std::isnan(Sb) || std::isnan(A))
    {
        qDebug() << "[BOTTOM REVERB] NaN input";
        return 0.0f;
    }

    // A = 0 → log(0)
    if (A <= 0.0f)
    {
        qDebug() << "[BOTTOM REVERB] A=0 → -inf";
        return -INFINITY;
    }

    float term = Sb + 10.0f * log10(A);

    return SL - (2.0f * TL) - term;
}

float SonarModel::computeDopplerShift(float v,
                                      float f0,
                                      float c)
{
    if (std::isnan(v) || std::isnan(f0) || std::isnan(c))
        return 0.0f;

    if (c <= 0.0f || f0 <= 0.0f)
        return 0.0f;

    return (2.0f * v / c) * f0;
}

float SonarModel::computeVelocityFromDoppler(float deltaF,
                                             float f0,
                                             float c)
{
    if (std::isnan(deltaF) || std::isnan(f0) || std::isnan(c))
        return 0.0f;

    if (f0 <= 0.0f || c <= 0.0f)
        return 0.0f;

    return (deltaF * c) / (2.0f * f0);
}

float SonarModel::computeDopplerLoss(float deltaF,
                                     float T)
{
    if (std::isnan(deltaF) || std::isnan(T))
        return 1.0f;

    if (T <= 0.0f)
        return 1.0f;

    float x = M_PI * deltaF * T;

    if (std::abs(x) < 1e-6f)
        return 1.0f;

    float val = sin(x) / x;
    return val * val;
}

float SonarModel::computeDopplerFrequency(float f0,
                                          float v_tx,
                                          float v_rx,
                                          float c)
{
    if (std::isnan(f0) || std::isnan(v_tx) ||
        std::isnan(v_rx) || std::isnan(c))
        return 0.0f;

    if (f0 <= 0.0f || c <= 0.0f)
        return 0.0f;

    float denom = c - v_tx;

    // division by zero protection
    if (std::abs(denom) < 1e-6f)
        return f0;

    return f0 * ((c + v_rx) / denom);
}
