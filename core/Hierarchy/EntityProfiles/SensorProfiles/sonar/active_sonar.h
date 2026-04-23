#pragma once
#include <vector>
#include "sonar_model.h"

class ActiveSonar
{
public:
    ActiveSonar();

    // Hardware config
    void setSoundSpeed(float metersPerSecond);
    void setMaxRange(float meters);
    void setMaxDepth(float meters);
    void setBeamWidth(float degrees);
    void setHeading(float degrees);
    void setPingInterval(float seconds);
    void setFalseDetectionRate(float rate);   //  0.0-1.0
    void setNoiseVariance(float variance);

    // Entity position (ship jis pe sonar lga hai)
    void setEntityPosition(double lat, double lon);

    // Step 1 — ping send
    // targetDepth: simulation me target kitna neeche (meters)
    // Returns false if target outside beam depth
    bool sendPing(float currentTime, float targetDepth);

    // Step 2 — echo received, distance auto-computed
    void receiveEcho(float currentTime);

    // Step 3 — full detection result
    // targetLat/Lon: target ki geo position (bearing compute ke liye)
    DetectionResult detect(const SonarInput &input,
                           double targetLat,
                           double targetLon);

    // ──  ek ping → sab targets ──
    std::vector<DetectionResult> scan(const std::vector<SonarTarget> &targets,
                                      const SonarInput &input);

    // ── interval check ──
    bool canPing(float currentTime) const;

    bool  isInBeam(float targetBearing) const;

    // Getters
    float getComputedDistance() const;
    float getTravelTime()       const;
    bool  hasEcho()             const;
    float getSoundSpeed() const;
    float getLastPingTime()     const;

    float getSoundSpeedAtDepth(float depth) const;

private:
    double m_entityLat;
    double m_entityLon;

    float m_pingTime;
    float m_echoTime;
    float m_travelTime;
    float m_computedDist;
    float m_soundSpeed;
    float m_maxRange;
    float m_maxDepth;
    float m_beamWidth;
    float m_heading;
    float m_pingInterval;
    float m_lastPingTime;
    float  m_falseDetectionRate;
    float  m_noiseVariance;
    bool  m_echoReceived;

    float computeDistance() const;

    DetectionResult processSingleTarget(const SonarTarget &target,
                                        const SonarInput  &input) const;

    DetectionResult generateFalseContact() const;
};
