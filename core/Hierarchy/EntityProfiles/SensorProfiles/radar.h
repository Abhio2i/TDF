#pragma once
// =============================================================================
// radar.h  —  Qt/engine bridge for RadarModel
//
// Radar owns a RadarModel instance and translates between:
//   • Engine/Qt types  (Platform*, Transform*, QJsonObject, Sensor base class)
//   • Model types      (RadarConfig, RadarPose, TargetInput, RadarOutput)
//
// No radar physics live here.  All physics, tracking and signal processing
// are inside radarmodel.h / radarmodel.cpp.
// =============================================================================

#ifndef RADAR_H
#define RADAR_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/radarlib/radarmodel.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <unordered_map>
#include <unordered_set>

class Radar : public Sensor
{
    Q_OBJECT

public:
    explicit Radar(Hierarchy* h);

    // ------------------------------------------------------------------
    // Lifecycle wrappers
    // (init + start called in constructor; end() if you destroy radar mid-sim)
    // ------------------------------------------------------------------
    void stop() { radarCore_.end(); }

    // ------------------------------------------------------------------
    // Config access  (bridge reads/writes RadarConfig as a whole)
    // ------------------------------------------------------------------
    RadarConfig getRadarConfig()               const { return radarCore_.getConfig(); }
    void        setRadarConfig(const RadarConfig& c) { radarCore_.setConfig(c); displayRangeDirty_ = true; }

    // ------------------------------------------------------------------
    // Mode control
    // ------------------------------------------------------------------
    void lockOn  (uint32_t radarTargetID);
    void breakLock();

    // ------------------------------------------------------------------
    // Read-only state (sourced from model — no duplicate fields)
    // ------------------------------------------------------------------
    double    getCurrentAzimuth() const { return radarCore_.getOutput().currentAzimuth; }
    RadarMode getMode()           const { return radarCore_.getOutput().mode; }

    // ------------------------------------------------------------------
    // Engine tick
    // ------------------------------------------------------------------
    void scan() override;

    // ------------------------------------------------------------------
    // Serialisation
    // ------------------------------------------------------------------
    QJsonObject toJson()                     const override;
    void        fromJson(const QJsonObject&)       override;

    // ------------------------------------------------------------------
    // Invalidate cached display range (called when config changes externally)
    // ------------------------------------------------------------------
    void markDisplayRangeDirty() { displayRangeDirty_ = true; }
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

private:
    // ------------------------------------------------------------------
    // Model — owns ALL radar state + physics
    // ------------------------------------------------------------------
    RadarModel radarCore_;

    // ------------------------------------------------------------------
    // Timing
    // ------------------------------------------------------------------
    QElapsedTimer frameTimer_;
    bool          timerStarted_ = false;
    double        simTime_      = 0.0;

    // ------------------------------------------------------------------
    // Display range cache
    // ------------------------------------------------------------------
    float cachedDisplayRange_ = 100.0f;
    bool  displayRangeDirty_  = true;

    // ------------------------------------------------------------------
    // Bridge helpers — translation only, no physics
    // ------------------------------------------------------------------

    /// Build a RadarPose from own platform transform
    RadarPose buildPose() const;

    /// Collect all other platforms as TargetInput (radar-local coords)
    std::vector<TargetInput> collectTargets(
        Transform*                            source,
        std::unordered_map<uint32_t, Platform*>& outIdMap) const;

    /// Translate surveillance detections → engine Target list
    void processSurveillance(
        const RadarOutput&                          output,
        const std::unordered_map<uint32_t, Platform*>& idMap,
        std::unordered_set<uint32_t>&                addedIDs);

    /// Translate TWS tracks → engine Target list
    void processTWS(
        const RadarOutput&                          output,
        const std::unordered_map<uint32_t, Platform*>& idMap,
        std::unordered_set<uint32_t>&                addedIDs);

    /// Translate locked track → engine Target
    void processLockOn(
        const RadarOutput&                          output,
        const std::unordered_map<uint32_t, Platform*>& idMap);

    // ------------------------------------------------------------------
    // Pure static utilities
    // ------------------------------------------------------------------
    static uint32_t platformToRadarID(const std::string& key);
    static double   platformRCS(const Platform* platform);
    static void     velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                         double& vx, double& vy, double& vz);
};

#endif // RADAR_H // RADAR_H
// #ifndef RADAR_H
// #define RADAR_H

// #include "radarmodel.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"

// #include <QJsonObject>
// #include <QElapsedTimer>

// // =============================================================================
// // Radar — integration wrapper around RadarModel
// //
// // Design rules:
// //   1. RadarAttributes is the SINGLE source of truth for all radar config.
// //      Do NOT add duplicate float/int fields here that mirror RadarAttributes.
// //   2. All config access goes through radarCore.getConfiguration().
// //   3. dt is measured from a real elapsed timer, not hardcoded.
// // =============================================================================
// class Radar : public Sensor
// {
//     Q_OBJECT

// public:
//     explicit Radar(Hierarchy* h);

//     // ------------------------------------------------------------------
//     // Public API
//     // ------------------------------------------------------------------

//     // Returns current radar configuration (read-only).
//     // To change config: get → modify → setConfiguration().
//     RadarAttributes getRadarConfig() const { return radarCore.getConfiguration(); }
//     void            setRadarConfig(const RadarAttributes& cfg) { radarCore.setConfiguration(cfg); }

//     // Convenience accessors (read-only, sourced from radarCore — no duplicate state)
//     double  getCurrentAzimuth()  const { return radarCore.getCurrentAzimuth(); }
//     RadarMode getMode()          const { return radarCore.getMode(); }

//     // Lock onto a specific platform by its radar ID
//     void lockOn(uint32_t radarTargetID);
//     void breakLock();

//     void scan() override;

//     QJsonObject toJson()                    const override;
//     void        fromJson(const QJsonObject& obj) override;
//     // In radar.h — public section:
//     void markDisplayRangeDirty() { displayRangeDirty = true; }
// private:
//     // ------------------------------------------------------------------
//     // Core radar model — owns ALL radar state
//     // ------------------------------------------------------------------
//     RadarModel radarCore;

//     // ------------------------------------------------------------------
//     // Timing — dt derived from real elapsed time, not hardcoded
//     // ------------------------------------------------------------------
//     QElapsedTimer frameTimer;
//     bool          timerStarted = false;
//     double        simTime      = 0.0;

//     // ------------------------------------------------------------------
//     // Internal helpers
//     // ------------------------------------------------------------------

//     // Build the TargetInput list from the current scene
//     std::vector<TargetInput> collectTargets(
//         Transform* source,
//         std::unordered_map<uint32_t, Platform*>& outIdMap) const;

//     // Build Target structs from surveillance detections
//     void processSurveillance(
//         const std::vector<DetectionOutput>& detections,
//         const std::unordered_map<uint32_t, Platform*>& idMap,
//         std::unordered_set<uint32_t>& addedIDs);

//     // Build Target structs from TWS tracks
//     void processTWS(
//         const std::vector<TrackFile>& tracks,
//         const std::unordered_map<uint32_t, Platform*>& idMap,
//         std::unordered_set<uint32_t>& addedIDs);

//     // Build Target struct from LOCK_ON track
//     void processLockOn(
//         const std::vector<TrackFile>& tracks,
//         const std::unordered_map<uint32_t, Platform*>& idMap);

//     // Log mode banner (called once per scan)
//     void logModeBanner(RadarMode mode) const;

//     // Convert platform heading+speed to velocity vector
//     static void  velocityFromHeadingSpeed(double headingDeg, double speedMs,
//                                          double& vx, double& vy, double& vz);

//     // Derive radar ID from platform key (stable hash)
//     static uint32_t platformToRadarID(const std::string& key);

//     // Get RCS from platform profile, fallback to default
//     static double   platformRCS(const Platform* platform);
//     // FIX 4: cache display range — recompute only when config changes
//     float cachedDisplayRange = 100.0f;
//     bool  displayRangeDirty  = true;
// };

// #endif
// -----------------------------------------------------------------------
// DYNAMIC RANGE — uncomment to enable physics-based scope scaling
// Range auto-adjusts based on radar equation + horizon + altitude
// -----------------------------------------------------------------------
// if (displayRangeDirty) {
//     cachedDisplayRange = static_cast<float>(radarCore.computeMaxDetectionRange());
//     cachedDisplayRange = std::clamp(cachedDisplayRange, 5.0f, 1000.0f);
//     displayRangeDirty  = false;
// }
// range = cachedDisplayRange;

// -----------------------------------------------------------------------
// FIXED RANGE — uncomment to use fixed 100km scope (default fallback)
// Comment this out and uncomment DYNAMIC RANGE block above to restore
// -----------------------------------------------------------------------
//range = 100.0f;
// ===========================================================================
// NOTE — Azimuth reference frames explained
//
// There are TWO different angle references in this radar system and they must
// not be confused:
//
// 1. SCAN SECTOR (minAzimuth / maxAzimuth in RadarAttributes)
//    These are RELATIVE angles — measured from the platform's own forward
//    direction (local frame). e.g. minAzimuth=-60, maxAzimuth=+60 means
//    the antenna can physically sweep 60° left and 60° right of the nose.
//    These are mechanical constraints on antenna travel, not compass bearings.
//
// 2. REPORTED AZIMUTH (currentAzimuth, shown in HUD as AZ: xxx°)
//    This is a WORLD-SPACE angle — absolute compass bearing from North,
//    normalised to 0-360°. It is computed from atan2(t.y, t.x) on the
//    target's local Cartesian coordinates, then normalised via += 360 if
//    negative. This matches real fire control radar convention — operators
//    always work in absolute bearings, not hull-relative angles.
//
// WHY LOCK-ON AZIMUTH SHOWS 343-344° WHEN SECTOR IS -60 TO +60:
//    The target sits at roughly -16° relative to the platform's nose
//    (within the -60/+60 sector — valid). After atan2 normalisation:
//    -16° + 360° = 344°  ← what the HUD reports as absolute bearing.
//    This is correct behaviour. The number is not in the same reference
//    as minAzimuth/maxAzimuth and should not be compared directly to them.
//
// WHY AZIMUTH DRIFTS DURING LOCK-ON:
//    Lock-on does not freeze the azimuth number — it freezes the antenna
//    onto the target. If the target moves, the antenna follows it, so the
//    bearing number changes. Smooth gradual drift = correct tracking.
//    Large jumps = track drop/re-initiation (prevented by getActiveTracks
//    always returning the locked track regardless of validation state).
// ===========================================================================
