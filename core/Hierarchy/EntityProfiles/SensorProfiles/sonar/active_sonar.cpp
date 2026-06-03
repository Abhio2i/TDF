// ============================================================
// active_sonar.cpp
//
// Active sonar implementation.
//
// Responsibilities:
//
//   • Ping transmission
//   • Echo reception
//   • Range estimation
//   • Beam filtering
//   • Target detection
//   • False contact simulation
//   • Multi-target scanning
//
// Used by Sonar sensor as the low-level active
// acoustic processing engine.
// ============================================================

#include "active_sonar.h"
#include <cmath>
#include <cstdlib>

ActiveSonar::ActiveSonar()
    : m_entityLat(0.0)
    , m_entityLon(0.0)
    , m_pingTime(0.0f)
    , m_echoTime(0.0f)
    , m_travelTime(0.0f)
    , m_computedDist(0.0f)
    , m_soundSpeed(1500.0f)
    , m_maxRange(10000.0f)
    , m_maxDepth(500.0f)
    , m_beamWidth(60.0f)   //  60° default
    , m_heading(0.0f)      // North default
    , m_pingInterval(3.0f)    //  3 sec default
    , m_lastPingTime(-999.0f) // first ping interval
    , m_falseDetectionRate(0.1f)  //  10% default
    , m_noiseVariance(5.0f)       //  ±5 dB default
    , m_echoReceived(false)
{}

void ActiveSonar::setSoundSpeed(float mps)  { m_soundSpeed = mps; }
void ActiveSonar::setMaxRange(float meters) { m_maxRange   = meters; }
void ActiveSonar::setMaxDepth(float meters) { m_maxDepth   = meters; }

float ActiveSonar::getComputedDistance() const { return m_computedDist; }
float ActiveSonar::getTravelTime()       const { return m_travelTime; }
bool  ActiveSonar::hasEcho()             const { return m_echoReceived; }

void ActiveSonar::setBeamWidth(float degrees) { m_beamWidth = degrees; }
void ActiveSonar::setHeading(float degrees)   { m_heading   = degrees; }

void ActiveSonar::setPingInterval(float seconds) { m_pingInterval = seconds; }
float ActiveSonar::getLastPingTime() const        { return m_lastPingTime; }

void ActiveSonar::setFalseDetectionRate(float rate)    { m_falseDetectionRate = rate; }
void ActiveSonar::setNoiseVariance(float variance)     { m_noiseVariance = variance; }

void ActiveSonar::setEntityPosition(double lat, double lon)
{
    m_entityLat = lat;
    m_entityLon = lon;
}

bool ActiveSonar::sendPing(float currentTime, float targetDepth)
{
    if (targetDepth > m_maxDepth)  // 0 > 500 = false ✅
        return false;

    m_pingTime     = currentTime;
    m_lastPingTime = currentTime;  // ← update
    return true;
}

void ActiveSonar::receiveEcho(float currentTime)
{
    m_echoTime     = currentTime;
    m_travelTime   = m_echoTime - m_pingTime;
    m_computedDist = computeDistance();
    m_echoReceived = true;
}

float ActiveSonar::computeDistance() const
{
    // Sound gaya + wapas aaya → /2
    return (m_soundSpeed * m_travelTime) / 2.0f;
}

float ActiveSonar::getSoundSpeed() const { return m_soundSpeed; }

float ActiveSonar::getSoundSpeedAtDepth(float depth) const
{
    // simple realistic behavior (no JSON, no complex math)

    if (depth < 100.0f) return 1520.0f;   // surface
    if (depth < 300.0f) return 1480.0f;   // thermocline
    return 1500.0f;                       // deep
}

DetectionResult ActiveSonar::detect(const SonarInput &input,
                                    double targetLat,
                                    double targetLon)
{
    DetectionResult result;
    result.detected     = false;
    result.distance     = 0.0f;
    result.bearing      = 0.0f;
    result.signalExcess = 0.0f;
    result.confidence   = 0.0f;

    // Echo not come
    if (!m_echoReceived)
        return result;

    // Beam width check
    float bearing = SonarModel::computeBearing(m_entityLat, m_entityLon,
                                               targetLat,   targetLon);
    if (!isInBeam(bearing))
        return result;

    // Range check
    if (m_computedDist > m_maxRange)
        return result;

    // SNR calculate
    float tl = SonarModel::computeTransmissionLoss(
        m_computedDist,
        input.absorption
        );

    // DI from beam width
    float DI = 10.0f * log10(360.0f / m_beamWidth);

    float snr = SonarModel::computeActiveSNR(
        input.sourceLevel,
        tl,
        input.targetStrength,
        input.noiseLevel,
        DI
        );

    float signalExcess = snr - input.detectionThreshold;

    // Detection decision
    if (!SonarModel::detectionDecision(snr, input.detectionThreshold))
        return result;

    // Detected — result fill
    result.detected     = true;
    result.distance     = m_computedDist;
    result.bearing      = SonarModel::computeBearing(
        m_entityLat, m_entityLon,
        targetLat,   targetLon);
    result.signalExcess = signalExcess;
    result.confidence   = SonarModel::computeConfidence(signalExcess);

    return result;
}

bool ActiveSonar::isInBeam(float targetBearing) const
{
    float half = m_beamWidth / 2.0f;

    // Angle difference (0-180)
    float diff = targetBearing - m_heading;

    // Normalize -180 to +180
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    return std::fabs(diff) <= half;
}

// ── one ping → all targets scan ──
std::vector<DetectionResult> ActiveSonar::scan(
    const std::vector<SonarTarget> &targets,
    const SonarInput               &input)
{
    std::vector<DetectionResult> results;

    // Real targets process
    for (const auto &target : targets)
        results.push_back(processSingleTarget(target, input));

    // ── False detection check ──
    float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

    if (roll < m_falseDetectionRate)
        results.push_back(generateFalseContact());

    return results;
}

// ── single target internal ──
DetectionResult ActiveSonar::processSingleTarget(
    const SonarTarget &target,
    const SonarInput  &input) const
{
    DetectionResult result;
    result.detected     = false;
    result.distance     = 0.0f;
    result.bearing      = 0.0f;
    result.signalExcess = 0.0f;
    result.confidence   = 0.0f;
    result.name         = target.name;

    // Check 1 — depth
    if (target.depth > m_maxDepth)
    {
        result.reason = "TOO DEEP";
        return result;
    }

    // Bearing
    float bearing = SonarModel::computeBearing(m_entityLat, m_entityLon,
                                               target.lat,  target.lon);

    // Check 2 — beam
    if (!isInBeam(bearing))
    {
        result.reason  = "OUTSIDE BEAM";
        result.bearing = bearing;
        return result;
    }

    // Distance
    float distance = SonarModel::geoDistance(m_entityLat, m_entityLon,
                                             target.lat,  target.lon);

    // Check 3 — range
    if (distance > m_maxRange)
    {
        result.reason   = "OUT OF RANGE";
        result.distance = distance;
        result.bearing  = bearing;
        return result;
    }

    // SNR
    float tl  = SonarModel::computeTransmissionLoss(distance, input.absorption);

    // DI from beam width
    float DI = 10.0f * log10(360.0f / m_beamWidth);

    float snr = SonarModel::computeActiveSNR(input.sourceLevel, tl,
                                             target.targetStrength,
                                             input.noiseLevel,  DI);

    float dt = SonarModel::validateDetectionThreshold(
        input.detectionThreshold);

    float signalExcess = snr - dt;

    // Check 4 — SNR
    if (!SonarModel::detectionDecision(snr, dt))
    {
        result.reason   = "WEAK SIGNAL";
        result.distance = distance;
        result.bearing  = bearing;
        return result;
    }

    // Detected
    result.detected     = true;
    result.distance     = distance;
    result.bearing      = bearing;
    result.signalExcess = signalExcess;
    result.confidence   = SonarModel::computeConfidence(signalExcess);
    result.reason       = "DETECTED";

    return result;
}

// Interval check
bool ActiveSonar::canPing(float currentTime) const
{
    return (currentTime - m_lastPingTime) >= m_pingInterval;
}

DetectionResult ActiveSonar::generateFalseContact() const
{
    // Random distance
    float randDist = 500.0f + static_cast<float>(rand() % static_cast<int>(m_maxRange - 500.0f));

    // Random bearing
    float half       = m_beamWidth / 2.0f;
    float randOffset = (static_cast<float>(rand()) / RAND_MAX) * m_beamWidth - half;
    float randBearing = std::fmod(m_heading + randOffset + 360.0f, 360.0f);

    // Noise spike — weak signal
    float randSE = m_noiseVariance * (static_cast<float>(rand()) / RAND_MAX);

    DetectionResult false_contact;
    false_contact.detected     = true;
    false_contact.distance     = randDist;
    false_contact.bearing      = randBearing;
    false_contact.signalExcess = randSE;
    false_contact.confidence   = SonarModel::computeConfidence(randSE);
    false_contact.name         = "FALSE CONTACT";
    false_contact.reason       = "NOISE SPIKE";

    return false_contact;
}
