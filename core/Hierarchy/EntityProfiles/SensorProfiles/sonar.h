#ifndef SONAR_H
#define SONAR_H

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h"
#include <QElapsedTimer>

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

    // ── Internal helpers ──
    std::vector<SonarTarget> collectTargets(double lat, double lon) const;

};

#endif // SONAR_H
