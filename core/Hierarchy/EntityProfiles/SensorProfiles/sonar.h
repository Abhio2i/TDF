#ifndef SONAR_H
#define SONAR_H

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h"
#include <QElapsedTimer>
#include <list>
#include <algorithm>


/**
 * @brief Pending sonar echo information.
 *
 * Stores target-related data after a ping has been transmitted.
 * The echo remains in the queue until its calculated arrival time
 * is reached and processed by the sonar system.
 */
struct PendingEcho
{
    std::string targetName;  // Target name
    float       arrivalTime;   // Simulation time when echo should arrive
    float       distance;      // Current target distance (meters)
    float       bearing;       // Target bearing (degrees)
    float       targetStrength;   // Target strength (dB)
    float       dirFactor      = 1.0f;  // Directional sensitivity factor

    std::string category;   // Entity category
    std::string subCategory;  // Entity sub-category

    double targetLat;   // Target latitude
    double targetLon;   // Target longitude
    float targetDepth;  // Target depth (meters)

    float moveSpeed; // Target speed
    std::string targetId;  // Unique target identifier
};

/**
 * @brief Represents a sound velocity layer in water.
 *
 * Used for modelling underwater acoustic propagation where
 * sound speed varies with depth.
 */
struct SoundLayer
{
    float minDepth;  // Minimum depth of layer
    float maxDepth;  // Maximum depth of layer
    float soundSpeed;   // Sound speed within layer (m/s)
};

/**
 * @brief Active Sonar sensor implementation.
 *
 * Performs active sonar operations including:
 * - Ping transmission
 * - Echo scheduling
 * - Echo processing
 * - Target detection
 * - Reverberation modelling
 * - Doppler estimation
 * - Occlusion filtering
 */
class Sonar: public Sensor
{
    Q_OBJECT

public:

    /**
     * @brief Constructor.
     * @param h Pointer to hierarchy object.
     */
    explicit Sonar(Hierarchy* h);

    /**
     * @brief Executes one sonar scan cycle.
     *
     * Handles ping generation, echo scheduling,
     * target collection and detection processing.
     */
    void scan() override;

    /**
     * @brief Serialize sonar configuration to JSON.
     */
    QJsonObject toJson() const override;

    /**
     * @brief Serialize sonar configuration to JSON.
     */
    void fromJson(const QJsonObject& obj) override;

    /**
     * @brief Load sonar configuration from JSON.
     */
    const std::vector<DetectionResult>& getLastResults() const
    {
        return m_lastResults;
    }

    /**
     * @brief Current ping interval in seconds.
     */
    float getPingInterval() const { return m_pingInterval; }


private:

    // =====================================================
    // Core sonar engine
    // =====================================================
    ActiveSonar    m_activeSonar;  // Active sonar model
    QElapsedTimer  m_timer;  // Internal simulation timer
    bool           m_timerStarted = false;

    // ── Last scan results ──
    std::vector<DetectionResult> m_lastResults;  // Detection results

    // Sonar configuration parameters
    // These values are exposed through Inspector
    float m_soundSpeed   = 1531.0f;  // Water sound speed (m/s)
    float m_maxDepth     = 500.0f;   // Maximum operating depth (m)
    float m_pingInterval = 60.0f;     // Ping repetition interval (sec)

    float m_sourceLevel  = 220.0f;   // Source level (dB)
    float m_noiseLevel   = 25.0f;    // Background noise level (dB)
    float m_threshold    = 3.0f;     // Detection threshold (dB)

    float frequency = 2.0f;   // Operating frequency (kHz)
    float m_arrayLength = 8.0f;   // Hydrophone array length (m)

    // Platform position tracking
    double m_lastLat = 0.0;
    double m_lastLon = 0.0;

    // Environmental parameters
    float m_windSpeed = 10.0f;  // Wind speed (knots)
    float m_shippingDensity = 1.0f;  // Shipping traffic density
    float m_waterTemperature = 4.0f;   // Water temperature (°C)
    float m_salinity = 35.0f;  // Salinity (ppt)

    // Signal processing parameters
    float m_pulseDuration = 0.05f; // Pulse duration (sec)
    float m_integrationPulses = 20.0f;  // Number of integrated pulses

    // =====================================================
    // Safety / reverberation parameters
    // =====================================================
    float m_safeObstacleDistance = 500.0f;  // Warning distance (m)
    float m_bottomBackscatter = -20.0f;  // Bottom backscatter coefficient (dB)

    /**
     * @brief Returns sound speed at specified depth.
     */
    float getSoundSpeed(float depth);

    // =====================================================
    // Internal helper functions
    // =====================================================

    /**
     * @brief Collect valid sonar targets from hierarchy.
     */
    std::vector<SonarTarget> collectTargets(double lat, double lon) const;

    /**
     * @brief Queue of pending echoes waiting for arrival.
     */
    std::list<PendingEcho> m_echoQueue;

    /**
     * @brief Process arrived echoes.
     */
    void processEchoQueue(float simTime);

    std::vector<SonarTarget> applyOcclusionFilter(
        const std::vector<SonarTarget>& input,
        double selfLat, double selfLon);

    /**
     * @brief Calculate directional sensitivity factor.
     *
     * Models forward lobe and rear baffle zone behaviour.
     */
    float computeDirectionalFactor(float bearing, float heading) const;
};

#endif // SONAR_H
