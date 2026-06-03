// ============================================================
// sonar_model.cpp
//
// Implementation of sonar acoustic processing algorithms.
//
// Includes:
//
//  • Transmission loss calculations
//  • Active sonar equation
//  • Bearing and distance calculations
//  • Sound propagation models
//  • Doppler processing
//  • Reverberation modelling
//  • Noise estimation
//  • Detection probability functions
//  • Sonar performance metrics
//
// ============================================================

#include "sonar_model.h"
#include <cmath>
#include <QDebug>
#include <QtMath>

static constexpr double PI = 3.14159265358979;


/**
 * @brief Calculates acoustic transmission loss.
 *
 * Models propagation loss using:
 *  - Spherical spreading
 *  - Frequency dependent absorption
 *
 * @param distance Propagation distance (m)
 * @param absorption Absorption coefficient (dB/km)
 *
 * @return Total transmission loss (dB)
 */
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

/**
 * @brief Computes active sonar Signal-to-Noise Ratio.
 *
 * Active Sonar Equation:
 *
 * SNR = SL - 2TL + TS - NL + DI
 *
 * Where:
 *   SL = Source Level
 *   TL = Transmission Loss
 *   TS = Target Strength
 *   NL = Noise Level
 *   DI = Directivity Index
 *
 * @return Signal-to-Noise Ratio (dB)
 */
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

/**
 * @brief Determines whether a target is detected.
 *
 * Detection occurs when SNR exceeds the
 * configured detection threshold.
 *
 * @return true if detected
 */
bool SonarModel::detectionDecision(float snr, float threshold)
{
    return snr >= threshold;
}

/**
 * @brief Computes geographic bearing between two coordinates.
 *
 * Uses forward azimuth calculation and returns
 * heading normalized to 0–360 degrees.
 *
 * @return Bearing in degrees
 */
float SonarModel::computeBearing(double fromLat, double fromLon,
                                 double toLat,   double toLon)
{
    if (fromLat == toLat && fromLon == toLon)
        return 0.0f;

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
    if (std::isnan(signalExcess))
        return 0.0f;

    // signalExcess <= 0  → no confidence
    // signalExcess >= 40 → full confidence
    if (signalExcess <= 0.0f)  return 0.0f;
    if (signalExcess >= 40.0f) return 1.0f;

    return signalExcess / 40.0f;
}

/**
 * @brief Computes great-circle distance between coordinates.
 *
 * Uses the Haversine formula.
 *
 * @return Distance in meters
 */
float SonarModel::geoDistance(double lat1, double lon1,
                              double lat2, double lon2)
{
    // clamp inputs
    lat1 = std::clamp(lat1, -90.0, 90.0);
    lat2 = std::clamp(lat2, -90.0, 90.0);
    lon1 = std::clamp(lon1, -180.0, 180.0);
    lon2 = std::clamp(lon2, -180.0, 180.0);

    // same point
    if (lat1 == lat2 && lon1 == lon2)
        return 0.0f;

    static constexpr double R = 6371000.0;

    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;

    double phi1 = lat1 * PI / 180.0;
    double phi2 = lat2 * PI / 180.0;

    double a = std::sin(dLat/2) * std::sin(dLat/2)
               + std::cos(phi1)
                     * std::cos(phi2)
                     * std::sin(dLon/2) * std::sin(dLon/2);

    a = std::clamp(a, 0.0, 1.0); // precision safety

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return static_cast<float>(R * c);
}

/**
 * @brief Calculates underwater acoustic absorption.
 *
 * Uses Thorp empirical absorption model.
 *
 * @param frequencyKHz Operating frequency (kHz)
 *
 * @return Absorption coefficient (dB/km)
 */
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

/**
 * @brief Calculates refracted ray angle using Snell's Law.
 *
 * Models bending of acoustic rays caused by
 * sound speed gradients between layers.
 *
 * @return Refracted angle (degrees)
 */
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

/**
 * @brief Computes refracted propagation angle.
 *
 * Handles:
 *   - Critical angle conditions
 *   - Total internal reflection
 *   - Negative angle preservation
 *   - Invalid sound speed protection
 *
 * @return Refracted angle (degrees)
 */
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

/**
 * @brief Applies convergence zone gain correction.
 *
 * Simulates long-range focusing effects where
 * acoustic energy converges and improves detection.
 *
 * @return Modified transmission loss
 */
float SonarModel::applyConvergenceZone(float rangeMeters, float tl)
{
    // NaN
    if (std::isnan(rangeMeters) ||
        std::isnan(tl))
    {
        qDebug() << "[CZ] NaN input";
        return tl;
    }

    // negative range
    if (rangeMeters < 0.0f)
    {
        qDebug() << "[CZ] Negative range";
        return tl;
    }

    // zero range
    if (rangeMeters <= 0.0f)
        return tl;

    // CZ spacing
    const float CZ_RANGE = 50000.0f; // 50 km

    // CZ width
    const float CZ_WIDTH = 10000.0f; // ±5 km

    // gain
    const float CZ_GAIN = 12.0f;

    // nearest CZ index
    int zone =
        static_cast<int>(
            std::round(
                rangeMeters / CZ_RANGE));

    // nearest CZ center
    float center =
        zone * CZ_RANGE;

    // distance from center
    float diff =
        std::abs(rangeMeters - center);

    // inside CZ ring
    if (diff <= CZ_WIDTH * 0.5f)
    {
        tl -= CZ_GAIN;

        qDebug() << "[CZ ACTIVE]"
                 << "Range:" << rangeMeters
                 << "Zone:" << zone
                 << "TL reduced:" << CZ_GAIN
                 << "New TL:" << tl;
    }

    return tl;
}

/**
 * @brief Validates detection threshold value.
 *
 * Performs:
 *   - NaN handling
 *   - Negative clamping
 *   - Upper bound protection
 *
 * @return Safe threshold value
 */
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

/**
 * @brief Computes Figure Of Merit.
 *
 * FOM = SL - NL + DI - DT
 *
 * Indicates maximum allowable transmission loss
 * for successful detection.
 *
 * @return Figure Of Merit (dB)
 */
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

/**
 * @brief Computes reverberation volume.
 *
 * Represents insonified water volume contributing
 * to volume reverberation.
 */
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

/**
 * @brief Computes volume reverberation limited SNR.
 */
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

/**
 * @brief Computes bottom reverberation limited SNR.
 */
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

/**
 * @brief Calculates insonified seabed area.
 */
float SonarModel::computeEnsonifiedArea(
    float c,
    float tau,
    float R,
    float psi)
{
    // NaN protection
    if (std::isnan(c) ||
        std::isnan(tau) ||
        std::isnan(R) ||
        std::isnan(psi))
    {
        qDebug() << "[ENSONIFIED AREA] NaN input";
        return 0.0f;
    }

    // absolute values
    tau = std::abs(tau);
    R   = std::abs(R);
    psi = std::abs(psi);

    // zero / invalid cases
    if (c <= 0.0f ||
        tau <= 0.0f ||
        R <= 0.0f ||
        psi <= 0.0f)
    {
        qDebug() << "[ENSONIFIED AREA] zero case";
        return 0.0f;
    }

    float area =
        c * tau * R * psi;

    qDebug() << "[ENSONIFIED AREA]"
             << "c:" << c
             << "tau:" << tau
             << "R:" << R
             << "psi:" << psi
             << "A:" << area;

    return area;
}

/**
 * @brief Computes Doppler frequency shift.
 *
 * Used to estimate target radial velocity.
 */
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

/**
 * @brief Estimates velocity from Doppler shift.
 */
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

/**
 * @brief Calculates Doppler mismatch loss.
 *
 * Models degradation caused by frequency mismatch
 * during pulse compression processing.
 */
float SonarModel::computeDopplerLoss(float deltaF,
                                     float T)
{
    // NaN protection
    if (std::isnan(deltaF) || std::isnan(T))
    {
        qDebug() << "[DOPPLER LOSS] NaN input";
        return 1.0f;
    }

    // absolute values
    deltaF = std::abs(deltaF);
    T      = std::abs(T);

    // zero cases
    if (deltaF <= 0.0f || T <= 0.0f)
    {
        qDebug() << "[DOPPLER LOSS]"
                 << "deltaF:" << deltaF
                 << "T:" << T
                 << "Loss=1 (zero mismatch)";
        return 1.0f;
    }

    float x = static_cast<float>(M_PI) * deltaF * T;

    // sinc(0)=1
    if (std::abs(x) < 1e-6f)
    {
        qDebug() << "[DOPPLER LOSS]"
                 << "x≈0"
                 << "Loss=1";
        return 1.0f;
    }

    float sinx = std::sin(x);

    float sinc = sinx / x;

    float loss = sinc * sinc;

    loss = std::clamp(loss, 0.0f, 1.0f);

    qDebug() << "[DOPPLER LOSS]"
             << "deltaF:" << deltaF
             << "T:" << T
             << "x(pi*df*T):" << x
             << "sin(x):" << sinx
             << "sinc:" << sinc
             << "Loss:" << loss;

    // Special visibility for SON-133 normal test
    if (std::abs(x - static_cast<float>(M_PI)) < 1e-4f)
    {
        qDebug() << "[SON-133]"
                 << "sin(pi)=0 → Doppler Loss = 0";
    }

    return loss;
}

/**
 * @brief Computes received echo frequency.
 *
 * Considers transmitter and receiver motion.
 */
float SonarModel::computeDopplerFrequency(float f0,
                                          float v_tx,
                                          float v_rx,
                                          float c)
{
    // NaN protection
    if (std::isnan(f0) ||
        std::isnan(v_tx) ||
        std::isnan(v_rx) ||
        std::isnan(c))
    {
        qDebug() << "[DOPPLER FREQ] NaN input";
        return 0.0f;
    }

    // invalid physics
    if (f0 <= 0.0f)
    {
        qDebug() << "[DOPPLER FREQ] f0<=0";
        return 0.0f;
    }

    if (c <= 0.0f)
    {
        qDebug() << "[DOPPLER FREQ] c<=0";
        return 0.0f;
    }

    float denom = c - v_tx;

    // denominator collapse
    if (std::abs(denom) < 1e-6f)
    {
        qDebug() << "[DOPPLER FREQ]"
                 << "v_tx≈c"
                 << "division by zero prevented";

        return 0.0f;
    }

    float fEcho =
        f0 * ((c + v_rx) / denom);

    qDebug() << "[DOPPLER FREQ]"
             << "f0:" << f0
             << "v_tx:" << v_tx
             << "v_rx:" << v_rx
             << "c:" << c
             << "f_echo:" << fEcho;

    return fEcho;
}

/**
 * @brief Ambient ocean noise model.
 */
float SonarModel::computeAmbientNoise(
    float frequencyKHz)
{
    if (std::isnan(frequencyKHz))
    {
        qDebug() << "[AMBIENT NOISE] NaN input";
        return 50.0f;
    }

    frequencyKHz = std::abs(frequencyKHz);

    if (frequencyKHz <= 0.0f)
    {
        qDebug() << "[AMBIENT NOISE] f=0";
        return 50.0f;
    }

    float NL =
        50.0f +
        20.0f * log10(frequencyKHz);

    qDebug() << "[AMBIENT NOISE]"
             << "f:" << frequencyKHz
             << "NL:" << NL;

    return NL;
}

/**
 * @brief Wind-generated surface noise model.
 */
float SonarModel::computeWindNoise(
    float frequencyKHz,
    float windKnots)
{
    if (std::isnan(frequencyKHz) ||
        std::isnan(windKnots))
    {
        qDebug() << "[WIND NOISE] NaN input";
        return 0.0f;
    }

    frequencyKHz = std::abs(frequencyKHz);
    windKnots    = std::abs(windKnots);

    frequencyKHz = std::max(frequencyKHz, 0.001f);
    windKnots    = std::max(windKnots, 0.001f);

    float NL =
        40.0f +
        2.0f * log10(windKnots) +
        10.0f * log10(frequencyKHz);

    qDebug() << "[WIND NOISE]"
             << "f:" << frequencyKHz
             << "wind:" << windKnots
             << "NL:" << NL;

    return NL;
}

/**
 * @brief Shipping traffic noise model.
 */
float SonarModel::computeShippingNoise(
    float frequencyKHz,
    float density)
{
    if (std::isnan(frequencyKHz) ||
        std::isnan(density))
    {
        qDebug() << "[SHIPPING NOISE] NaN input";
        return 30.0f;
    }

    frequencyKHz = std::abs(frequencyKHz);
    density      = std::abs(density);

    frequencyKHz = std::max(frequencyKHz, 0.001f);
    density      = std::max(density, 0.001f);

    float NL =
        30.0f +
        10.0f * log10(density) +
        20.0f * log10(frequencyKHz);

    qDebug() << "[SHIPPING NOISE]"
             << "f:" << frequencyKHz
             << "density:" << density
             << "NL:" << NL;

    return NL;
}

/**
 * @brief Self-noise generated by platform motion.
 */
float SonarModel::computeFlowNoise(float speedKnots)
{
    if (std::isnan(speedKnots))
    {
        qDebug() << "[FLOW NOISE] NaN input";
        return 0.0f;
    }

    speedKnots = std::abs(speedKnots);

    float NL =
        5.0f * log10(speedKnots + 1.0f);

    qDebug() << "[FLOW NOISE]"
             << "speed:" << speedKnots
             << "NL:" << NL;

    return NL;
}

/**
 * @brief Combines multiple noise sources.
 *
 * Uses logarithmic power summation.
 */
float SonarModel::computeTotalNoise(
    const std::vector<float>& levels)
{
    if (levels.empty())
        return 0.0f;

    double powerSum = 0.0;

    for (float nl : levels)
    {
        if (std::isnan(nl))
            continue;

        nl = std::max(0.0f, nl);

        powerSum +=
            std::pow(10.0,
                     nl / 10.0);
    }

    if (powerSum <= 0.0)
        return 0.0f;

    float total =
        10.0f * log10(powerSum);

    qDebug() << "[TOTAL NOISE]"
             << "NL_total:" << total;

    return total;
}

/**
 * @brief Calculates sound speed in seawater.
 *
 * Uses Mackenzie empirical equation.
 *
 * Inputs:
 *   - Temperature
 *   - Salinity
 *   - Depth
 *
 * @return Sound speed (m/s)
 */
float SonarModel::computeMackenzieSoundSpeed(
    float T,
    float S,
    float D)
{
    if (std::isnan(T) ||
        std::isnan(S) ||
        std::isnan(D))
    {
        qDebug() << "[MACKENZIE] NaN input";
        return 1500.0f;
    }

    T = std::max(0.0f, T);

    S = std::clamp(S,
                   0.0f,
                   40.0f);

    D = std::max(0.0f, D);

    float c =
        1448.96f
        + 4.591f * T
        - 5.304e-2f * T * T
        + 2.374e-4f * T * T * T
        + 1.340f * (S - 35.0f)
        + 1.630e-2f * D
        + 1.675e-7f * D * D
        - 1.025e-2f * T * (S - 35.0f)
        - 7.139e-13f * T * D * D * D;

    // qDebug() << "[MACKENZIE]"
    //          << "T:" << T
    //          << "S:" << S
    //          << "D:" << D
    //          << "c:" << c;

    return c;
}

/**
 * @brief Computes Time Difference Of Arrival.
 *
 * Used by hydrophone arrays for bearing estimation.
 */
float SonarModel::computeTDOA(
    float d,
    float thetaDeg,
    float c)
{
    if (c <= 0.0f)
        return 0.0f;

    d = std::abs(d);

    float thetaRad =
        thetaDeg * M_PI / 180.0f;

    return (d * sin(thetaRad)) / c;
}

/**
 * @brief Estimates bearing from TDOA measurement.
 */
float SonarModel::computeBearingFromTDOA(
    float tau,
    float d,
    float c)
{
    if (d <= 0.0f || c <= 0.0f)
        return 0.0f;

    float x = (tau * c) / d;

    x = std::clamp(x,
                   -1.0f,
                   1.0f);

    return asin(x) * 180.0f / M_PI;
}

/**
 * @brief Generic target strength calculation.
 */
float SonarModel::computeTargetStrength(
    float sigma_bs)
{
    // NaN
    if (std::isnan(sigma_bs))
    {
        qDebug() << "[TS] NaN input";
        return -20.0f;
    }

    // absolute
    sigma_bs = std::abs(sigma_bs);

    // zero case
    if (sigma_bs <= 0.0f)
    {
        qDebug() << "[TS] sigma=0";
        return -INFINITY;
    }

    float TS =
        10.0f * log10(sigma_bs);

    qDebug() << "[TS]"
             << "sigma:" << sigma_bs
             << "TS:" << TS;

    return TS;
}

/**
 * @brief Submarine target strength model.
 */
float SonarModel::computeSubmarineTargetStrength(
    float L,
    float lambda)
{
    // NaN
    if (std::isnan(L) ||
        std::isnan(lambda))
    {
        qDebug() << "[SUB TS] NaN input";
        return -20.0f;
    }

    // absolute
    L      = std::abs(L);
    lambda = std::abs(lambda);

    // zero cases
    if (L <= 0.0f ||
        lambda <= 0.0f)
    {
        qDebug() << "[SUB TS] invalid";
        return -INFINITY;
    }

    float TS =
        10.0f * log10(L / lambda)
        - 29.0f;

    qDebug() << "[SUB TS]"
             << "L:" << L
             << "lambda:" << lambda
             << "TS:" << TS;

    return TS;
}

/**
 * @brief Cylindrical target strength approximation.
 *
 * Suitable for ships, frigates and submarines.
 */
float SonarModel::computeCylinderTargetStrength(
    float a,
    float L,
    float lambda,
    float R)
{
    Q_UNUSED(R);

    // NaN
    if (std::isnan(a) ||
        std::isnan(L) ||
        std::isnan(lambda))
    {
        qDebug() << "[CYLINDER TS] NaN input";
        return -20.0f;
    }

    // absolute
    a      = std::abs(a);
    L      = std::abs(L);
    lambda = std::abs(lambda);

    // zero cases
    if (a <= 0.0f ||
        L <= 0.0f ||
        lambda <= 0.0f)
    {
        qDebug() << "[CYLINDER TS] invalid";
        return -INFINITY;
    }

    float TS =
        20.0f * log10(a)
        + 10.0f * log10(L / lambda);

    qDebug() << "[CYLINDER TS]"
             << "a:" << a
             << "L:" << L
             << "lambda:" << lambda
             << "TS:" << TS;

    return TS;
}

/**
 * @brief Computes array directivity index.
 *
 * Larger arrays provide greater spatial gain.
 */
float SonarModel::computeDirectivityIndex(
    float elements)
{
    if (std::isnan(elements))
        return 0.0f;

    elements = std::abs(elements);

    if (elements <= 1.0f)
        return 0.0f;

    return 10.0f * log10(elements);
}

/**
 * @brief Probability of false alarm.
 */
float SonarModel::computeFalseAlarmProbability(float DT)
{
    // NaN case
    if (std::isnan(DT))
    {
        qDebug() << "[PFA] NaN input → default 0.01";
        return 0.01f;
    }

    // Negative DT
    if (DT < 0.0f)
    {
        float val = exp(-DT);

        qDebug() << "[PFA]"
                 << "Negative DT:" << DT
                 << "Raw:" << val
                 << "Clamped: 1";

        return 1.0f;
    }

    // Normal
    float pfa = exp(-DT);

    // Clamp
    pfa = std::clamp(pfa, 0.0f, 1.0f);

    qDebug() << "[PFA]"
             << "DT:" << DT
             << "Pfa:" << pfa;

    return pfa;
}

/**
 * @brief Probability of detection.
 */
float SonarModel::computeProbabilityOfDetection(float SNR, float Pfa)
{
    // NaN handling
    if (std::isnan(SNR))
    {
        qDebug() << "[Pd] SNR NaN → return 0";
        return 0.0f;
    }

    if (std::isnan(Pfa))
    {
        qDebug() << "[Pd] Pfa NaN → default 1e-6";
        Pfa = 1e-6f;
    }

    // Clamp Pfa
    Pfa = std::clamp(Pfa, 1e-9f, 1.0f);

    // Special case
    if (Pfa >= 1.0f)
        return 1.0f;

    // Convert Pfa → threshold
    float T = -log(Pfa);

    // Logistic steepness (tuneable)
    float k = 0.6f;

    float Pd = 1.0f / (1.0f + exp(-k * (SNR - T)));

    // Clamp safety
    Pd = std::clamp(Pd, 0.0f, 1.0f);

    return Pd;
}

/**
 * @brief Calculates required SNR for desired Pd/Pfa.
 */
float SonarModel::computeRequiredSNR(float Pd, float Pfa)
{
    // NaN handling
    if (std::isnan(Pd) || std::isnan(Pfa))
    {
        qDebug() << "[SNR] NaN input → return 0";
        return 0.0f;
    }

    // Clamp Pd
    Pd = std::clamp(Pd, 0.000001f, 0.999999f);

    // Clamp Pfa
    Pfa = std::clamp(Pfa, 1e-9f, 1.0f);

    // Special case
    if (Pfa >= 1.0f)
        return 0.0f;

    // Threshold from Pfa
    float T = -log(Pfa);

    // Same k as Pd function
    float k = 0.6f;

    // Inverse logistic
    float snr = T + (1.0f / k) * log(Pd / (1.0f - Pd));

    return snr;
}

/**
 * @brief Incoherent integration gain.
 *
 * Models gain obtained by combining
 * multiple received pings.
 */
float SonarModel::computeIncoherentIntegrationGain(float n)
{
    // NaN
    if (std::isnan(n))
    {
        qDebug() << "[INTEGRATION] NaN input";
        return 0.0f;
    }

    // absolute
    n = std::abs(n);

    // zero
    if (n <= 0.0f)
    {
        qDebug() << "[INTEGRATION] n=0 → Gain=0";
        return 0.0f;
    }

    // rounded pulse count
    int pulses =
        static_cast<int>(std::round(n));

    if (pulses <= 1)
    {
        qDebug() << "[INTEGRATION]"
                 << "Pulses:" << pulses
                 << "Gain=0";

        return 0.0f;
    }

    float gain =
        5.0f * log10(pulses);

    qDebug() << "[INTEGRATION]"
             << "Pulses:" << pulses
             << "Gain(dB):" << gain;

    return gain;
}

/**
 * @brief Range resolution.
 */
float SonarModel::computeRangeResolution(float c, float tau)
{
    // NaN
    if (std::isnan(c) || std::isnan(tau))
    {
        qDebug() << "[RANGE RES] NaN input";
        return 0.0f;
    }

    // absolute pulse width
    tau = std::abs(tau);

    // invalid cases
    if (tau <= 0.0f || c <= 0.0f)
    {
        qDebug() << "[RANGE RES]"
                 << "Invalid input"
                 << "c:" << c
                 << "tau:" << tau;

        return 0.0f;
    }

    float resolution =
        (c * tau) / 2.0f;

    qDebug() << "[RANGE RES]"
             << "c:" << c
             << "tau:" << tau
             << "Resolution(m):" << resolution;

    return resolution;
}

/**
 * @brief Bearing resolution.
 */
float SonarModel::computeBearingResolution(float lambda, float L)
{
    // NaN
    if (std::isnan(lambda) || std::isnan(L))
    {
        qDebug() << "[BEARING RES] NaN input";
        return 0.0f;
    }

    // absolute array length
    L = std::abs(L);

    // invalid cases
    if (L <= 0.0f)
    {
        qDebug() << "[BEARING RES] L=0";
        return 0.0f;
    }

    if (lambda <= 0.0f)
    {
        qDebug() << "[BEARING RES] lambda=0";
        return 0.0f;
    }

    float theta = lambda / L;

    qDebug() << "[BEARING RES]"
             << "lambda:" << lambda
             << "L:" << L
             << "theta(rad):" << theta
             << "theta(deg):"
             << theta * 180.0f / M_PI;

    return theta;
}

/**
 * @brief Maximum unambiguous range.
 */
float SonarModel::computeMaxUnambiguousRange(float c, float PRI)
{
    // NaN
    if (std::isnan(c) || std::isnan(PRI))
        return 0.0f;

    // negative PRI → absolute
    PRI = std::abs(PRI);

    // zero cases
    if (PRI <= 0.0f || c <= 0.0f)
        return 0.0f;

    return (c * PRI) / 2.0f;
}

/**
 * @brief Maximum unambiguous radial velocity.
 */
float SonarModel::computeMaxUnambiguousSpeed(float lambda, float PRI)
{
    if (std::isnan(lambda) || std::isnan(PRI))
        return 0.0f;

    lambda = std::abs(lambda);
    PRI    = std::abs(PRI);

    if (lambda <= 0.0f || PRI <= 0.0f)
        return 0.0f;

    return lambda / (4.0f * PRI);
}

/**
 * @brief Generates collision/obstacle warning.
 *
 * Warning is issued when:
 *   - Target is detected
 *   - Target is closing
 *   - Distance is below safety threshold
 *
 * @return true if warning should be raised
 */
bool SonarModel::computeObstacleWarning(
    float distance,
    float safeDistance,
    float relativeVelocity,
    bool detected)
{
    if (!detected)
        return false;

    // negative → closing
    if (relativeVelocity >= 0.0f)
        return false;

    // safe boundary included
    if (distance <= safeDistance)
        return true;

    return false;
}
