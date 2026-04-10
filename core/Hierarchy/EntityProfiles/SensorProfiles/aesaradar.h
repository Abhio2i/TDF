#pragma once
// =============================================================================
// aesaradar.h  —  Qt / engine bridge for RadarModel_AESA
//
// Mirrors radar.h exactly in structure.
// AESARadar owns a RadarModel_AESA instance and translates between:
//   • Engine / Qt types  (Platform*, Transform*, QJsonObject, Sensor base class)
//   • Model types        (aesa::RadarConfig, aesa::RadarPose,
//                         aesa::TargetInput, aesa::RadarOutput)
//
// No AESA physics live here.  All physics, JPDA tracking, Swerling RCS,
// Albersheim Pd, Doppler notch, IFF, DRFM, chaff, beam spoiling and
// duty-cycle enforcement are inside the model layer (radarmodel_aesa.h/.cpp).
//
// Additional AESA signals (vs radar.h):
//   drfmGhostDetected      FIX-03  DRFM false target warning
//   iffResult              FIX-04  per-track IFF interrogation result
//   trackBelowDopplerNotch FIX-01  informational near-notch warning
//   schedulerDutyCycle     FIX-08  current T/R duty cycle each tick
//   chaffContactDetected   FIX-10  chaff cloud region warning
//   externalTrackInjected  FIX-12  Link-16 / CEC track confirmed
// =============================================================================

#ifndef AESARADAR_H
#define AESARADAR_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"          // model — only header needed
#include "core/Hierarchy/EntityProfiles/sensor.h"  // Sensor base class

#include <QElapsedTimer>
#include <QJsonObject>
#include <unordered_map>
#include <unordered_set>

class AESARadar : public Sensor
{
    Q_OBJECT

public:
    explicit AESARadar(Hierarchy* h);

    // ------------------------------------------------------------------
    // Lifecycle wrappers
    // ------------------------------------------------------------------
    void stop() { radarCore_.end(); }

    // ------------------------------------------------------------------
    // Config access
    // ------------------------------------------------------------------
    aesa::RadarConfig getRadarConfig()                     const { return radarCore_.getConfig(); }
    void              setRadarConfig(const aesa::RadarConfig& c) { radarCore_.setConfig(c); displayRangeDirty_ = true; }

    // ------------------------------------------------------------------
    // Mode control
    // ------------------------------------------------------------------
    void lockOn   (uint32_t radarTargetID);
    void breakLock();

    // ------------------------------------------------------------------
    // FIX-10  Chaff management
    // ------------------------------------------------------------------
    void deployChaffCloud(const aesa::ChaffCloud& cloud) { radarCore_.addChaffCloud(cloud); }
    void clearAllChaff()                                  { radarCore_.clearChaffClouds();   }

    // ------------------------------------------------------------------
    // FIX-12  External track injection (Link-16 / CEC)
    // ------------------------------------------------------------------
    void injectExternalTrack(const aesa::TrackOutput& ext);

    // ------------------------------------------------------------------
    // Read-only state (sourced from model — no duplicate fields)
    // ------------------------------------------------------------------
    double          getCurrentAzimuth()  const { return radarCore_.getOutput().currentAzimuth;   }
    double          getCurrentElevation()const { return radarCore_.getOutput().currentElevation;  }
    aesa::RadarMode getMode()            const { return radarCore_.getOutput().mode;              }
    double          getCurrentDutyCycle()const { return radarCore_.getOutput().currentDutyCycle;  }

    // ------------------------------------------------------------------
    // Engine tick  (called every simulation frame by the engine)
    // ------------------------------------------------------------------
    void scan() override;

    // ------------------------------------------------------------------
    // Serialisation
    // ------------------------------------------------------------------
    QJsonObject toJson()                      const override;
    void        fromJson(const QJsonObject&)        override;

    // ------------------------------------------------------------------
    // Invalidate cached display range when config changes externally
    // ------------------------------------------------------------------
    void markDisplayRangeDirty() { displayRangeDirty_ = true; }

signals:
    // ---- AESA-specific signals ----------------------------------------

    /// FIX-03  A DRFM-jammer ghost detection was produced.
    /// UI / fire-control must NOT engage on this track.
    void drfmGhostDetected(uint32_t targetID, float ghostRange_m,
                           float ghostAz_deg, float ghostEl_deg);

    /// FIX-04  IFF interrogation result for a validated track.
    /// responseCode maps to aesa::IFFResponseCode:
    ///   0 = NO_REPLY  1 = FRIENDLY  2 = UNKNOWN  3 = HOSTILE  4 = CORRUPTED
    void iffResult(uint32_t trackID, int responseCode,
                   uint32_t squawk, float confidence);

    /// FIX-01  Track radial velocity is near the Doppler blind zone.
    /// Informational — the track may degrade on the next scan.
    void trackBelowDopplerNotch(uint32_t trackID);

    /// FIX-08  Current T/R module duty cycle (0.0–1.0) emitted every tick.
    void schedulerDutyCycle(float dutyCycle);

    /// FIX-10  A chaff cloud is present in the current beam region.
    void chaffContactDetected(uint32_t region);

    /// FIX-12  External (Link-16 / CEC) track successfully injected.
    void externalTrackInjected(uint32_t trackID);

private:
    // ------------------------------------------------------------------
    // Model — owns ALL AESA radar state + physics
    // ------------------------------------------------------------------
    aesa::RadarModel_AESA radarCore_;

    // ------------------------------------------------------------------
    // Timing
    // ------------------------------------------------------------------
    QElapsedTimer frameTimer_;
    bool          timerStarted_ = false;
    double        simTime_      = 0.0;

    // ------------------------------------------------------------------
    // Display range cache
    // ------------------------------------------------------------------
    float cachedDisplayRange_ = 200.0f;
    bool  displayRangeDirty_  = true;

    // ------------------------------------------------------------------
    // Bridge helpers — translation only, no physics
    // ------------------------------------------------------------------

    /// Build an aesa::RadarPose from own platform transform.
    aesa::RadarPose buildPose() const;

    /// Collect AIR platforms as aesa::TargetInput in radar-local coordinates.
    std::vector<aesa::TargetInput> collectTargets(
        Transform*                              source,
        std::unordered_map<uint32_t, Platform*>& outIdMap) const;

    /// Translate surveillance detections → Sensor::targets.
    void processSurveillance(
        const aesa::RadarOutput&                        output,
        const std::unordered_map<uint32_t, Platform*>&  idMap,
        std::unordered_set<uint32_t>&                   addedIDs);

    /// Translate TWS tracks → Sensor::targets + emit AESA signals.
    void processTWS(
        const aesa::RadarOutput&                        output,
        const std::unordered_map<uint32_t, Platform*>&  idMap,
        std::unordered_set<uint32_t>&                   addedIDs);

    /// Translate fire-control (lock-on) detections → Sensor::targets.
    void processLockOn(
        const aesa::RadarOutput&                        output,
        const std::unordered_map<uint32_t, Platform*>&  idMap);

    // ------------------------------------------------------------------
    // Pure static utilities (identical pattern to radar.cpp)
    // ------------------------------------------------------------------
    static uint32_t platformToRadarID(const std::string& key);
    static double   platformRCS      (const Platform* platform);
    static void     velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                         double& vx, double& vy, double& vz);
    // Instance-specific velocity tracking — NOT static
   mutable std::unordered_map<uint32_t, QVector3D> prevPositions_;
   mutable std::unordered_map<uint32_t, float>     computedHeadings_;
};

#endif // AESARADAR_H
