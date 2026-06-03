#pragma once

#include <vector>
#include "sonar_model.h"

/**
 * @brief Active sonar processor.
 *
 * Simulates active sonar behaviour including:
 *  - Ping transmission
 *  - Echo reception
 *  - Beam filtering
 *  - Range validation
 *  - SNR calculation
 *  - Target detection
 *  - False contact generation
 */
class ActiveSonar
{
public:

    /**
     * @brief Constructor.
     *
     * Initializes sonar parameters with default values.
     */
    ActiveSonar();

    // =====================================================
    // Sonar Configuration
    // =====================================================

    /**
     * @brief Sets sound speed used for propagation.
     * @param metersPerSecond Speed of sound in water.
     */
    void setSoundSpeed(float metersPerSecond);

    /**
     * @brief Sets maximum detection range.
     * @param meters Maximum range in meters.
     */
    void setMaxRange(float meters);

    /**
     * @brief Sets maximum operating depth.
     * @param meters Maximum depth in meters.
     */
    void setMaxDepth(float meters);

    /**
     * @brief Sets sonar beam width.
     * @param degrees Beam width in degrees.
     */
    void setBeamWidth(float degrees);

    /**
     * @brief Sets platform heading.
     * @param degrees Heading angle in degrees.
     */
    void setHeading(float degrees);

    /**
     * @brief Sets ping repetition interval.
     * @param seconds Time between pings.
     */
    void setPingInterval(float seconds);

    /**
     * @brief Sets probability of false contacts.
     *
     * Value range:
     *   0.0 = never
     *   1.0 = always
     */
    void setFalseDetectionRate(float rate);

    /**
     * @brief Sets noise variance used when generating
     * false contacts.
     */
    void setNoiseVariance(float variance);

    // =====================================================
    // Platform Position
    // =====================================================

    /**
     * @brief Updates sonar platform position.
     */
    void setEntityPosition(
        double lat,
        double lon);

    // =====================================================
    // Sonar Ping Lifecycle
    // =====================================================

    /**
     * @brief Transmit sonar ping.
     *
     * Validates target depth before transmission.
     *
     * @return true if ping accepted.
     */
    bool sendPing(
        float currentTime,
        float targetDepth);

    /**
     * @brief Receive returning echo.
     *
     * Calculates travel time and estimated range.
     */
    void receiveEcho(
        float currentTime);

    /**
     * @brief Perform detection on a single target.
     *
     * Calculates:
     *  - Bearing
     *  - Range
     *  - Transmission loss
     *  - SNR
     *  - Confidence
     */
    DetectionResult detect(
        const SonarInput& input,
        double targetLat,
        double targetLon);

    /**
     * @brief Scan multiple targets using one ping.
     *
     * Returns detection results for all supplied targets.
     */
    std::vector<DetectionResult> scan(
        const std::vector<SonarTarget>& targets,
        const SonarInput& input);

    /**
     * @brief Checks whether sonar can transmit
     * a new ping.
     */
    bool canPing(float currentTime) const;

    /**
     * @brief Determines whether target lies inside beam.
     */
    bool isInBeam(float targetBearing) const;

    // =====================================================
    // Status Getters
    // =====================================================

    /**
     * @brief Last computed target range.
     */
    float getComputedDistance() const;

    /**
     * @brief Last measured echo travel time.
     */
    float getTravelTime() const;

    /**
     * @brief Returns true if echo was received.
     */
    bool hasEcho() const;

    /**
     * @brief Current sound speed.
     */
    float getSoundSpeed() const;

    /**
     * @brief Last ping transmission time.
     */
    float getLastPingTime() const;

    /**
     * @brief Estimates sound speed at depth.
     */
    float getSoundSpeedAtDepth(float depth) const;

private:

    // =====================================================
    // Platform State
    // =====================================================

    double m_entityLat;      // Platform latitude
    double m_entityLon;      // Platform longitude

    // =====================================================
    // Ping Timing
    // =====================================================

    float m_pingTime;        // Ping transmission time
    float m_echoTime;        // Echo reception time
    float m_travelTime;      // Echo round-trip time

    // =====================================================
    // Computed Results
    // =====================================================

    float m_computedDist;    // Computed target distance

    // =====================================================
    // Sonar Configuration
    // =====================================================

    float m_soundSpeed;      // Speed of sound (m/s)
    float m_maxRange;        // Maximum detection range
    float m_maxDepth;        // Maximum operating depth

    float m_beamWidth;       // Sonar beam width
    float m_heading;         // Platform heading

    float m_pingInterval;    // Time between pings
    float m_lastPingTime;    // Previous ping timestamp

    // =====================================================
    // False Contact Modelling
    // =====================================================

    float m_falseDetectionRate; // False detection probability
    float m_noiseVariance;      // Random noise magnitude

    // =====================================================
    // Echo State
    // =====================================================

    bool m_echoReceived;     // Echo available flag

    // =====================================================
    // Internal Helpers
    // =====================================================

    /**
     * @brief Converts travel time into range.
     *
     * Uses:
     * Distance = (c × t) / 2
     */
    float computeDistance() const;

    /**
     * @brief Processes a single target.
     *
     * Performs:
     *  - Depth validation
     *  - Beam validation
     *  - Range validation
     *  - SNR calculation
     *  - Detection decision
     */
    DetectionResult processSingleTarget(
        const SonarTarget& target,
        const SonarInput& input) const;

    /**
     * @brief Generates synthetic false contact.
     *
     * Simulates random detections caused by noise.
     */
    DetectionResult generateFalseContact() const;
};
