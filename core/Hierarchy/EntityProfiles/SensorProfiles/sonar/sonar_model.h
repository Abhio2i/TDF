#pragma once
#include <string>
#include <vector>

// ============================================================
// sonar_model.h
//
// Core sonar mathematics and acoustic processing utilities.
//
// Contains:
//   • Sonar input/output data structures
//   • Target and detection representations
//   • Acoustic propagation models
//   • Noise models
//   • Doppler calculations
//   • Reverberation models
//   • Detection theory utilities
// ============================================================

/**
 * @brief Input parameters required for sonar equation calculations.
 */
struct SonarInput
{
    float sourceLevel;          // Source Level (SL) in dB
    float targetStrength;        // Target Strength (TS) in dB
    float noiseLevel;           // Noise Level (NL) in dB
    float detectionThreshold;   // Detection Threshold (DT) in dB
    float absorption;           // Acoustic absorption coefficient
};

/**
 * @brief Represents a detectable sonar target.
 */
struct SonarTarget
{
    std::string name; // Target name

    double      lat;     // Latitude
    double      lon;     // Longitude

    float       depth;                // Depth below sea surface (m)

    float       targetStrength;       // Base target strength (dB)

    std::string id;             // Unique entity ID

    std::string category;       // Main category
    std::string subCategory;    // Detailed classification
};

/**
 * @brief Sonar detection output.
 */
struct DetectionResult
{
    bool        detected      = false;   // Detection status

    float       distance      = 0.0f;    // Target range (m)
    float       bearing       = 0.0f;    // Target bearing (deg)

    float       signalExcess  = 0.0f;    // Signal excess (SNR-DT)
    float       confidence    = 0.0f;    // Detection confidence
    float       intensity     = 0.0f;    // Display intensity

    std::string name;                    // Target name
    std::string reason;                  // Detection reason

    std::string category;
    std::string subCategory;
};

/**
 * @brief Collection of static sonar modelling utilities.
 *
 * Provides mathematical models for:
 * - Acoustic propagation
 * - Transmission loss
 * - Doppler effects
 * - Reverberation
 * - Noise estimation
 * - Detection probability
 * - Sonar performance prediction
 */
class SonarModel
{
public:

    // =====================================================
    // SONAR EQUATION
    // =====================================================

    /**
     * @brief Calculates transmission loss.
     *
     * Includes spreading loss and absorption loss.
     */
    static float computeTransmissionLoss(float distance,
                                         float absorption);

    // static float computeActiveSNR(float sourceLevel,
    //                               float transmissionLoss,
    //                               float targetStrength,
    //                               float noiseLevel);

    /**
     * @brief Active sonar equation.
     *
     * Returns Signal-to-Noise Ratio (SNR).
     */
    static float computeActiveSNR(float sourceLevel,
                                  float transmissionLoss,
                                  float targetStrength,
                                  float noiseLevel,
                                  float DI);

    /**
     * @brief Determines if detection threshold is exceeded.
     */
    static bool  detectionDecision(float snr,
                                  float threshold);

    // =====================================================
    // GEOMETRY UTILITIES
    // =====================================================

    /**
     * @brief Bearing between two geographic positions.
     */
    static float computeBearing(double fromLat, double fromLon,
                                double toLat,   double toLon);

    /**
     * @brief Converts signal excess into confidence value.
     */
    static float computeConfidence(float signalExcess);

    /**
     * @brief Great-circle distance between coordinates.
     */
    static float geoDistance(double lat1, double lon1,
                             double lat2, double lon2);


    // =====================================================
    // PROPAGATION MODELS
    // =====================================================

    /**
     * @brief Thorp absorption model.
     *
     * Frequency-dependent underwater absorption.
     */
    static float computeThorpAbsorption(float frequencyKHz);

    /**
     * @brief Computes refraction using Snell's law.
     */
    static float computeSnellRefraction(float theta1Deg,
                                        float c1,
                                        float c2);

    /**
     * @brief Calculates refracted angle.
     */
    static float computeSnellAngle(float theta1_deg,
                                   float c1,
                                   float c2);

    /**
     * @brief Applies convergence zone gain correction.
     */
    static float applyConvergenceZone(float rangeMeters, float tl);

    /**
     * @brief Ensures threshold remains within valid limits.
     */
    static float validateDetectionThreshold(float dt);

    /**
     * @brief Figure Of Merit (FOM).
     */
    static float computeFOM(float SL,
                            float NL,
                            float DI,
                            float DT);

    // =====================================================
    // REVERBERATION MODELS
    // =====================================================

    /**
     * @brief Computes reverberation volume.
     */
    static float computeReverbVolume(float c,
                                     float tau,
                                     float R,
                                     float psi);

    /**
     * @brief Volume reverberation limited SNR.
     */
    static float computeVolumeReverbSNR(float SL,
                                        float TL,
                                        float Sv,
                                        float V);

    /**
     * @brief Bottom reverberation limited SNR.
     */
    static float computeBottomReverbSNR(float SL,
                                        float TL,
                                        float Sb,
                                        float A);

    /**
     * @brief Ensonified bottom area.
     */
    static float computeEnsonifiedArea(
        float c,
        float tau,
        float R,
        float psi);

    // =====================================================
    // DOPPLER PROCESSING
    // =====================================================

    /**
     * @brief Doppler frequency shift.
     */
    static float computeDopplerShift(float v,
                                     float f0,
                                     float c);

    /**
     * @brief Estimate velocity from Doppler shift.
     */
    static float computeVelocityFromDoppler(float deltaF,
                                            float f0,
                                            float c);

    /**
     * @brief Doppler mismatch loss.
     */
    static float computeDopplerLoss(float deltaF,
                                    float T);

    /**
     * @brief Calculates received echo frequency.
     */
    static float computeDopplerFrequency(float f0,
                                         float v_tx,
                                         float v_rx,
                                         float c);

    // =====================================================
    // NOISE MODELS
    // =====================================================

    /**
     * @brief Ambient ocean noise.
     */
    static float computeAmbientNoise(float frequencyKHz);

    /**
     * @brief Wind generated surface noise.
     */
    static float computeWindNoise(float frequencyKHz,
                                  float windKnots);

    /**
     * @brief Shipping traffic noise.
     */
    static float computeShippingNoise(float frequencyKHz,float density);

    /**
     * @brief Combines multiple noise sources.
     */
    static float computeTotalNoise(const std::vector<float>& levels);

    /**
     * @brief Self-noise caused by platform movement.
     */
    static float computeFlowNoise(float speedKnots);

    // =====================================================
    // ENVIRONMENTAL MODELS
    // =====================================================

    /**
     * @brief Mackenzie sound speed equation.
     */
    static float computeMackenzieSoundSpeed(
        float temperature,
        float salinity,
        float depth);

    // =====================================================
    // ARRAY PROCESSING
    // =====================================================

    /**
     * @brief Time Difference Of Arrival.
     */
    static float computeTDOA(
        float d,
        float thetaDeg,
        float c);

    /**
     * @brief Bearing estimation from TDOA.
     */
    static float computeBearingFromTDOA(
        float tau,
        float d,
        float c);

    /**
     * @brief Array directivity index.
     */
    static float computeDirectivityIndex(
        float elements);

    // =====================================================
    // TARGET STRENGTH MODELS
    // =====================================================

    /**
     * @brief Generic target strength.
     */
    static float computeTargetStrength(
        float sigma_bs);

    /**
     * @brief Submarine target strength model.
     */
    static float computeSubmarineTargetStrength(
        float L,
        float lambda);

    /**
     * @brief Cylindrical target strength model.
     */
    static float computeCylinderTargetStrength(
        float a,
        float L,
        float lambda,
        float R);


    // =====================================================
    // DETECTION THEORY
    // =====================================================

    /**
     * @brief Probability of false alarm.
     */
    static float computeFalseAlarmProbability(float DT);

    /**
     * @brief Probability of detection.
     */
    static float computeProbabilityOfDetection(float SNR, float Pfa);

    /**
     * @brief Required SNR for desired Pd/Pfa.
     */
    static float computeRequiredSNR(float Pd, float Pfa);

    /**
     * @brief Incoherent integration gain.
     */
    static float computeIncoherentIntegrationGain(float n);

    // =====================================================
    // SONAR RESOLUTION & LIMITS
    // =====================================================

    /**
     * @brief Range resolution.
     */
    static float computeRangeResolution(float c, float tau);

    /**
     * @brief Bearing resolution.
     */
    static float computeBearingResolution(float lambda, float L);

    /**
     * @brief Maximum unambiguous range.
     */
    static float computeMaxUnambiguousRange(float c, float PRI);

    /**
     * @brief Maximum unambiguous radial speed.
     */
    static float computeMaxUnambiguousSpeed(float lambda, float PRI);

    /**
     * @brief Generates obstacle warning when a detected
     * target enters safety range.
     */
    static bool computeObstacleWarning(
        float distance,
        float safeDistance,
        float relativeVelocity,
        bool detected
        );

};
