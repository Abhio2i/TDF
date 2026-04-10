#ifndef SONAR_H
#define SONAR_H

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h"
#include <QElapsedTimer>
#include <list>
#include <algorithm>

// Pending echo — future me aayega
struct PendingEcho
{
    std::string targetName;
    float       arrivalTime;   // simTime jab echo aayega
    float       distance;      // target distance
    float       bearing;       // target bearing
    float       targetStrength;
    std::string category;
};

class Sonar: public Sensor
{
    Q_OBJECT
public:
    explicit Sonar(Hierarchy* h);

    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

    const std::vector<DetectionResult>& getLastResults() const
    {
        return m_lastResults;
    }

    float getPingInterval() const { return m_pingInterval; }


private:
    ActiveSonar    m_activeSonar;
    QElapsedTimer  m_timer;
    bool           m_timerStarted = false;

    // ── Last scan results ──
    std::vector<DetectionResult> m_lastResults;

    // ── Sonar parameters — Inspector se set honge ──
    float m_soundSpeed   = 1531.0f;  // m/s
    float m_maxDepth     = 500.0f;   // meters
    float m_pingInterval = 5.0f;     // seconds
    float m_sourceLevel  = 220.0f;   // dB
    float m_noiseLevel   = 30.0f;    // dB
    float m_threshold    = 10.0f;    // dB;

    // Position tracking
    double m_lastLat = 0.0;
    double m_lastLon = 0.0;

    // ── Internal helpers ──
    std::vector<SonarTarget> collectTargets(double lat, double lon) const;

    std::list<PendingEcho> m_echoQueue;

    void processEchoQueue(float simTime);

};

#endif // SONAR_H
