// =============================================================================
// FILE:         aesaradar.h
// MODULE:       AESA Radar — Qt / Engine Bridge
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares AESARadar, the Qt/engine bridge class that owns a
//               RadarModel_AESA instance and translates between:
//                 - Engine / Qt types  (Platform*, Transform*, QJsonObject,
//                                       Sensor base class, QElapsedTimer)
//                 - Model types        (aesa::RadarConfig, aesa::RadarPose,
//                                       aesa::TargetInput, aesa::RadarOutput)
//
//               No AESA physics reside in this class. All physics, JPDA
//               tracking, Swerling RCS, Albersheim Pd, Doppler notch, IFF,
//               DRFM, chaff, beam spoiling, and duty-cycle enforcement are
//               inside the model layer (radarmodel_aesa.h / .cpp).
//
//               AESARadar mirrors radar.h exactly in class structure and
//               adds the following AESA-specific Qt signals:
//                 drfmGhostDetected      REQ-AESA-060  DRFM false target warning
//                 iffResult              REQ-AESA-050  Per-track IFF result
//                 trackBelowDopplerNotch REQ-AESA-040  Near-notch track warning
//                 schedulerDutyCycle     REQ-AESA-020  T/R duty cycle each tick
//                 chaffContactDetected   REQ-AESA-061  Chaff region warning
//                 externalTrackInjected  REQ-AESA-027  Link-16 / CEC confirmation
//
// REQUIREMENTS: REQ-AESA-001  Lifecycle (init / start / update / end / reset)
//               REQ-AESA-002  Configuration serialisation (toJson / fromJson)
//               REQ-AESA-003  Mode control (scan / lockOn / breakLock)
//               REQ-AESA-004  Output assembly (scan -> Sensor::targets)
//               REQ-AESA-020  Duty cycle reporting (schedulerDutyCycle signal)
//               REQ-AESA-027  External track injection (injectExternalTrack)
//               REQ-AESA-050  IFF result forwarding (iffResult signal)
//               REQ-AESA-060  DRFM ghost filtering (drfmGhostDetected signal)
//               REQ-AESA-061  Chaff management (deployChaffCloud / clearAllChaff)
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-BRIDGE-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic surveillance bridge.
//   Rev 2  15 Feb 2026  FIX-01: Platform speed / Doppler notch warning signal.
//                       FIX-03: DRFM ghost detection signal and filter.
//                       FIX-04: IFF result signal added.
//                       FIX-08: Scheduler duty cycle signal added.
//                       FIX-10: Chaff cloud management API added.
//                       FIX-12: External track injection (Link-16 / CEC) added.
//   Rev 3  01 Apr 2026  Staggered PRF, RGPO/VGPO, IMM tracker wired in via
//                       model layer. Bridge unchanged — all physics in model.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       All static helper declarations documented.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef AESARADAR_H
#define AESARADAR_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/aisaRadarLib/radarmodel_aesa.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <unordered_map>
#include <unordered_set>

// =============================================================================
// CLASS: AESARadar
//
// DESCRIPTION:  Qt/engine bridge that owns one RadarModel_AESA and translates
//               its output into the engine's Sensor::targets list and Qt
//               signals each simulation frame. Inherits Sensor (Q_OBJECT).
//
//               Responsibilities:
//                 1. Construct radarCore_ with mission-calibrated RadarConfig.
//                 2. Each scan() call: build RadarPose + TargetInput list from
//                    engine scene, drive radarCore_.update(), translate output.
//                 3. Forward AESA-specific events as Qt signals to UI and
//                    fire-control consumers.
//                 4. Serialise / deserialise full RadarConfig via toJson() /
//                    fromJson() for scenario save/load.
//
//               This class contains NO radar physics. All numerical computation
//               is delegated to radarCore_ (RadarModel_AESA).
//
// THREAD SAFETY: Not thread-safe. scan() is called from the engine's main
//                simulation thread. Do not call from multiple threads.
//                radarCore_ is thread-safe internally (mutex-protected).
//
// REQUIREMENTS: REQ-AESA-001 through REQ-AESA-004, REQ-AESA-020,
//               REQ-AESA-027, REQ-AESA-050, REQ-AESA-060, REQ-AESA-061.
//
// TRACEABILITY:
//   Test suite:  aesaRadarBridge_test (aesaradar_bridge_test.cpp)
//   Test cases:  TC-AESA-BRG-001 through TC-AESA-BRG-NNN
// =============================================================================
class AESARadar : public Sensor
{
    Q_OBJECT

public:

    // =========================================================================
    // CONSTRUCTOR: AESARadar
    //
    // DESCRIPTION: Constructs the bridge and initialises radarCore_ with a
    //              fully specified RadarConfig calibrated to match the legacy
    //              Generic radar parameters. Calls radarCore_.init(),
    //              radarCore_.start(), and radarCore_.reset() to ensure a
    //              clean initial state with no ghost tracks. Sets subType to
    //              SubType::AESA and sensortype to Type::Active.
    //
    // REQUIREMENT: REQ-AESA-001
    //
    // PARAMETERS:
    //   h  [in]  Owning Hierarchy instance. Passed to Sensor base class.
    //            Must not be null — Sensor base requires a valid hierarchy.
    //
    // SIDE EFFECTS: Constructs radarCore_ on the stack (no heap allocation
    //               in this constructor body). Calls radarCore_.init() which
    //               heap-allocates the five subsystem unique_ptr members inside
    //               RadarModel_AESA — documented MM-01 deviation.
    //               Sets maxDetectionAngle from cfg.maxAzimuth.
    // =========================================================================
    explicit AESARadar(Hierarchy* h);

    // =========================================================================
    // LIFECYCLE WRAPPERS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    stop
    //
    // DESCRIPTION: Stops the radar model. Delegates to radarCore_.end() which
    //              clears all tracks, signals, and output caches, and sets
    //              running_ = false so subsequent update() calls are no-ops.
    //              Safe to call multiple times. REQ-AESA-001.
    // =========================================================================
    void stop() { radarCore_.end(); }

    // =========================================================================
    // CONFIGURATION ACCESS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    getRadarConfig
    //
    // DESCRIPTION: Returns a copy of the current radar configuration from the
    //              model layer. Thread-safe snapshot via radarCore_.getConfig().
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // RETURNS:    Copy of aesa::RadarConfig at time of call.
    // SIDE EFFECTS: None. Acquires radarCore_ mutex briefly.
    // =========================================================================
    aesa::RadarConfig getRadarConfig() const { return radarCore_.getConfig(); }

    // =========================================================================
    // FUNCTION:    setRadarConfig
    //
    // DESCRIPTION: Applies a new radar configuration to the model layer.
    //              Sets displayRangeDirty_ = true to force display range
    //              recalculation on the next scan(). Thread-safe.
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // PARAMETERS:
    //   c  [in]  New configuration. Applied immediately to radarCore_.
    //
    // SIDE EFFECTS: Calls radarCore_.setConfig(). Sets displayRangeDirty_.
    // =========================================================================
    void setRadarConfig(const aesa::RadarConfig& c)
    {
        radarCore_.setConfig(c);
        displayRangeDirty_ = true;
    }

    // =========================================================================
    // MODE CONTROL
    // =========================================================================

    // =========================================================================
    // FUNCTION:    lockOn
    //
    // DESCRIPTION: Transitions radarCore_ to LOCK_ON mode targeting the
    //              specified platform. Delegates to radarCore_.lockOn().
    //              The radar commits all beam time to the locked target,
    //              publishing fire-control quality updates every tick.
    //
    // REQUIREMENT: REQ-AESA-003
    //
    // PARAMETERS:
    //   radarTargetID  [in]  Radar-internal target ID (from platformToRadarID).
    //                        Must be non-zero and match a currently tracked
    //                        target. 0 has no effect.
    //
    // SIDE EFFECTS: Delegates to radarCore_.lockOn(). Clears scan cache and
    //               output inside the model.
    //
    // TRACEABILITY: TC-AESA-BRG-010  lockOn() transitions to LOCK_ON mode
    // =========================================================================
    void lockOn(uint32_t radarTargetID);

    // =========================================================================
    // FUNCTION:    breakLock
    //
    // DESCRIPTION: Returns radarCore_ from LOCK_ON to SURVEILLANCE mode.
    //              Delegates to radarCore_.breakLock(). Clears the locked
    //              target ID and rebuilds the search schedule.
    //
    // REQUIREMENT: REQ-AESA-003
    //
    // SIDE EFFECTS: Delegates to radarCore_.breakLock(). Rebuilds schedule.
    //
    // TRACEABILITY: TC-AESA-BRG-011  breakLock() returns to SURVEILLANCE
    // =========================================================================
    void breakLock();

    // =========================================================================
    // CHAFF MANAGEMENT  (REQ-AESA-061)
    // =========================================================================

    // =========================================================================
    // FUNCTION:    deployChaffCloud
    //
    // DESCRIPTION: Adds one active chaff cloud to the model. The cloud is
    //              included in SINR denominator computation from the next tick.
    //              Delegates to radarCore_.addChaffCloud(). REQ-AESA-061.
    //
    // PARAMETERS:
    //   cloud  [in]  ChaffCloud descriptor. Must have birthTime_s set to
    //                current simTime_ by caller before passing.
    //
    // SIDE EFFECTS: Appends to radarCore_'s internal chaff list.
    // =========================================================================
    void deployChaffCloud(const aesa::ChaffCloud& cloud)
    {
        radarCore_.addChaffCloud(cloud);
    }

    // =========================================================================
    // FUNCTION:    clearAllChaff
    //
    // DESCRIPTION: Removes all active chaff clouds from the model immediately.
    //              Delegates to radarCore_.clearChaffClouds(). REQ-AESA-061.
    //
    // SIDE EFFECTS: Clears radarCore_'s internal chaff list.
    // =========================================================================
    void clearAllChaff() { radarCore_.clearChaffClouds(); }

    // =========================================================================
    // EXTERNAL TRACK INJECTION  (REQ-AESA-027)
    // =========================================================================

    // =========================================================================
    // FUNCTION:    injectExternalTrack
    //
    // DESCRIPTION: Injects a track received via external data link (Link-16 /
    //              CEC) into the model tracker. The track is immediately
    //              validated and marked isExternalTrack = true. Emits the
    //              externalTrackInjected signal with the track's id.
    //
    // REQUIREMENT: REQ-AESA-027
    //
    // PARAMETERS:
    //   ext  [in]  External track descriptor. Must have ext.id != 0 and
    //              finite position / velocity. Duplicate ids are silently
    //              ignored by the tracker.
    //
    // SIDE EFFECTS: Calls radarCore_.injectExternalTrack(). Emits
    //               externalTrackInjected(ext.id).
    //
    // TRACEABILITY: TC-AESA-BRG-020  injectExternalTrack emits signal
    // =========================================================================
    void injectExternalTrack(const aesa::TrackOutput& ext);

    // =========================================================================
    // READ-ONLY STATE (sourced from model — no duplicate fields in bridge)
    // =========================================================================

    // Returns current beam azimuth from latest model output (degrees, body frame).
    // REQ-AESA-010.
    double getCurrentAzimuth()   const { return radarCore_.getOutput().currentAzimuth;  }

    // Returns current beam elevation from latest model output (degrees, body frame).
    // REQ-AESA-010.
    double getCurrentElevation() const { return radarCore_.getOutput().currentElevation; }

    // Returns current radar operating mode from latest model output. REQ-AESA-003.
    aesa::RadarMode getMode()    const { return radarCore_.getOutput().mode;             }

    // Returns current T/R module duty cycle [0.0, 1.0] from latest model output.
    // REQ-AESA-020.
    double getCurrentDutyCycle() const { return radarCore_.getOutput().currentDutyCycle; }

    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

    // =========================================================================
    // ENGINE TICK
    // =========================================================================

    // =========================================================================
    // FUNCTION:    scan  (override)
    //
    // DESCRIPTION: Main simulation tick called every frame by the engine.
    //              Executes the following pipeline each call:
    //
    //              1. Guard against null parentEntity — return immediately.
    //              2. Compute dt from QElapsedTimer (first call: dt = 0.05 s).
    //                 Clamp dt to [MIN_DT, MAX_DT].
    //              3. Build aesa::RadarPose from own platform transform.
    //              4. Update radarHeight in config from live pose.y.
    //              5. Update platformSpeed_m_s from own dynamicModel.
    //              6. Collect all peer platforms as aesa::TargetInput in
    //                 radar-local coordinates (collectTargets).
    //              7. Drive radarCore_.update(dt, pose, inputs, simTime_).
    //              8. Retrieve aesa::RadarOutput and aesa::RadarConfig.
    //              9. Sync Sensor base fields: azimuth, beamWidth,
    //                 maxDetectionAngle, range, mode.
    //             10. Clear Sensor::targets. Populate from mode-specific
    //                 output translator (processSurveillance / processTWS /
    //                 processLockOn).
    //             11. Emit schedulerDutyCycle each tick (FIX-08).
    //             12. Emit enemyDetected / enemyNotFound.
    //
    // REQUIREMENT: REQ-AESA-001, REQ-AESA-003, REQ-AESA-004
    //
    // SIDE EFFECTS: Modifies Sensor::targets, Sensor::azimuth,
    //               Sensor::beamWidth, Sensor::range, Sensor::mode.
    //               Increments simTime_. May setConfig on radarCore_.
    //               Emits schedulerDutyCycle, enemyDetected / enemyNotFound,
    //               and any AESA-specific signals raised by output translators.
    //
    // TRACEABILITY: TC-AESA-BRG-030  scan() with no targets does not crash
    //               TC-AESA-BRG-031  scan() populates targets from output
    // =========================================================================
    void scan() override;

    // =========================================================================
    // SERIALISATION
    // =========================================================================

    // =========================================================================
    // FUNCTION:    toJson  (override)
    //
    // DESCRIPTION: Serialises the complete current RadarConfig plus identity
    //              fields to a QJsonObject. Organised into named sections:
    //              array, transmitter, scan, waveform, detection, platform,
    //              tracking, propagation, noise, iff, nullSteering, mode.
    //              All numeric fields are wrapped in toParm() descriptors
    //              (matching the style used throughout the project).
    //              BeamWaveform structs serialised via a local lambda serWF().
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // RETURNS:    QJsonObject containing the full radar configuration.
    // SIDE EFFECTS: Calls radarCore_.getConfig() (acquires mutex briefly).
    //
    // TRACEABILITY: TC-AESA-BRG-040  toJson() contains expected keys
    //               TC-AESA-BRG-041  fromJson(toJson()) round-trips correctly
    // =========================================================================
    QJsonObject toJson()                     const override;

    // =========================================================================
    // FUNCTION:    fromJson  (override)
    //
    // DESCRIPTION: Deserialises a QJsonObject (produced by toJson()) back into
    //              RadarConfig and applies it via radarCore_.setConfig().
    //              Each section is optional — missing keys leave the existing
    //              config field unchanged. BeamWaveform fields are read via
    //              a local lambda readWF(). Sets displayRangeDirty_ = true.
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // PARAMETERS:
    //   obj  [in]  QJsonObject as produced by toJson(). Partial objects are
    //              accepted — only present keys are applied.
    //
    // SIDE EFFECTS: Calls radarCore_.setConfig(). Sets displayRangeDirty_.
    //
    // TRACEABILITY: TC-AESA-BRG-041  fromJson(toJson()) round-trips correctly
    // =========================================================================
    void fromJson(const QJsonObject&) override;

    // =========================================================================
    // FUNCTION:    markDisplayRangeDirty
    //
    // DESCRIPTION: Invalidates the cached display range so it is recomputed
    //              on the next scan(). Call this whenever the radar config
    //              is modified externally (e.g. from a UI property panel)
    //              without going through setRadarConfig(). REQ-AESA-004.
    // =========================================================================
    void markDisplayRangeDirty() { displayRangeDirty_ = true; }

signals:

    // =========================================================================
    // AESA-SPECIFIC Qt SIGNALS
    // All signals are emitted from scan() or injectExternalTrack() on the
    // engine's main thread. Thread safety is provided by Qt's signal/slot
    // queued connection mechanism when connecting to slots in other threads.
    // =========================================================================

    // =========================================================================
    // SIGNAL:      drfmGhostDetected
    //
    // DESCRIPTION: Emitted when a DRFM-jammer ghost detection is produced
    //              by the model. UI and fire-control must NOT engage on a
    //              DRFM ghost — it is a false target created by the target's
    //              own jammer. FIX-03. REQ-AESA-060.
    //
    // PARAMETERS:
    //   targetID    — ID of the jamming target that produced the ghost.
    //   ghostRange_m — Range of the ghost detection (metres).
    //   ghostAz_deg  — Azimuth of the ghost detection (degrees, body frame).
    //   ghostEl_deg  — Elevation of the ghost detection (degrees, body frame).
    // =========================================================================
    void drfmGhostDetected(uint32_t targetID, float ghostRange_m,
                           float ghostAz_deg, float ghostEl_deg);

    // =========================================================================
    // SIGNAL:      iffResult
    //
    // DESCRIPTION: Emitted for each validated track after IFF interrogation.
    //              responseCode maps to aesa::IFFResponseCode:
    //                0 = NO_REPLY  1 = FRIENDLY  2 = UNKNOWN
    //                3 = HOSTILE   4 = CORRUPTED
    //              FIX-04. REQ-AESA-050.
    //
    // PARAMETERS:
    //   trackID      — Validated track ID.
    //   responseCode — aesa::IFFResponseCode cast to int.
    //   squawk       — Squawk code received (0 = no reply or encrypted mode).
    //   confidence   — Classification confidence [0.0, 1.0].
    // =========================================================================
    void iffResult(uint32_t trackID, int responseCode,
                   uint32_t squawk, float confidence);

    // =========================================================================
    // SIGNAL:      trackBelowDopplerNotch
    //
    // DESCRIPTION: Emitted when a validated track's radial velocity magnitude
    //              is below 30 m/s — within the Doppler blind zone. The track
    //              may degrade on the next scan if clutter notching is active.
    //              Informational only — does not remove the track. FIX-01.
    //              REQ-AESA-040.
    //
    // PARAMETERS:
    //   trackID  — ID of the track approaching the Doppler notch.
    // =========================================================================
    void trackBelowDopplerNotch(uint32_t trackID);

    // =========================================================================
    // SIGNAL:      schedulerDutyCycle
    //
    // DESCRIPTION: Emitted every scan() tick with the current T/R module duty
    //              cycle. Allows UI to monitor duty cycle budget utilisation
    //              and warn the operator when approaching the maxDutyCycle
    //              limit. FIX-08. REQ-AESA-020.
    //
    // PARAMETERS:
    //   dutyCycle  — Current duty cycle [0.0, 1.0].
    // =========================================================================
    void schedulerDutyCycle(float dutyCycle);

    // =========================================================================
    // SIGNAL:      chaffContactDetected
    //
    // DESCRIPTION: Emitted when a chaff cloud is present in the current beam
    //              region. Informational — the cloud is included in the SINR
    //              denominator automatically. UI may display a clutter warning.
    //              FIX-10. REQ-AESA-061.
    //
    // PARAMETERS:
    //   region  — Spatial region index containing the chaff cloud.
    //             Currently reserved — value is always 0.
    // =========================================================================
    void chaffContactDetected(uint32_t region);

    // =========================================================================
    // SIGNAL:      externalTrackInjected
    //
    // DESCRIPTION: Emitted after a Link-16 / CEC track has been successfully
    //              injected into the tracker database. Allows UI to display a
    //              datalink track icon immediately. FIX-12. REQ-AESA-027.
    //
    // PARAMETERS:
    //   trackID  — ID of the injected external track (from TrackOutput::id).
    // =========================================================================
    void externalTrackInjected(uint32_t trackID);

private:

    // =========================================================================
    // PRIVATE MEMBER VARIABLES
    // =========================================================================

    // ---- Model ---------------------------------------------------------------

    // Owns all AESA radar state and physics. Stack-allocated in AESARadar.
    // Five subsystems inside radarCore_ are heap-allocated via unique_ptr
    // (documented MM-01 deviation in ICD-AESA-DEVIATION-002). REQ-AESA-001.
    aesa::RadarModel_AESA radarCore_;

    // ---- Timing --------------------------------------------------------------

    // High-resolution frame timer. Started on first scan() call.
    // Used to compute dt between consecutive scan() calls. REQ-AESA-001.
    QElapsedTimer frameTimer_;

    // true = frameTimer_ has been started. false = first scan() call pending.
    // On first call, dt is set to 0.05 s to avoid a zero-division. REQ-AESA-001.
    bool timerStarted_ = false;

    // Accumulated simulation time (seconds). Monotonically increasing.
    // Passed to radarCore_.update() each tick. REQ-AESA-001.
    double simTime_ = 0.0;

    // ---- Display range cache -------------------------------------------------

    // Last computed display range (km). Clamped to [5, 1000] km.
    // Updated from output.displayRange_km each scan(). REQ-AESA-004.
    float cachedDisplayRange_ = 200.0f;

    // true = cachedDisplayRange_ must be recomputed. Set on config change.
    // REQ-AESA-004.
    bool displayRangeDirty_ = true;

    // =========================================================================
    // PRIVATE BRIDGE HELPER METHODS
    // Translation only — no radar physics.
    // =========================================================================

    // =========================================================================
    // FUNCTION:    buildPose
    //
    // DESCRIPTION: Constructs an aesa::RadarPose from the own platform's
    //              Transform and DynamicModel. Position is read from the
    //              platform's world-space transform matrix and scaled by
    //              COORD_SCALE (1000.0) to convert from engine units to metres.
    //              Attitude (heading, pitch, roll) is read from dynamicModel.
    //              Returns a zero-initialised pose if root or parentEntity
    //              are null, or if the platform has no transform.
    //
    // REQUIREMENT: REQ-AESA-010
    //
    // RETURNS:    aesa::RadarPose with position in metres, attitude in degrees.
    // SIDE EFFECTS: None. Read-only access to root->Platforms. REQ-AESA-010.
    // =========================================================================
    aesa::RadarPose buildPose() const;

    // =========================================================================
    // FUNCTION:    collectTargets
    //
    // DESCRIPTION: Iterates root->Platforms, skipping the own platform. For
    //              each peer platform, computes its position in radar-local
    //              body frame via source->inverseTransformPoint(), applies
    //              the axis mapping (engine z→x, x→y, y→z) and COORD_SCALE,
    //              then populates an aesa::TargetInput. Velocity is estimated
    //              by finite-differencing world-space positions (stored in
    //              prevPositions_) combined with the platform's currentSpeed
    //              and a computed heading from computedHeadings_. Physical
    //              dimensions are read from platform->collider if available.
    //
    // REQUIREMENT: REQ-AESA-004  Target data collection
    //
    // PARAMETERS:
    //   source    [in]   Transform of the own platform. Used to compute
    //                    radar-local coordinates for all peer platforms.
    //   outIdMap  [out]  Maps radar target ID (from platformToRadarID) to
    //                    the original Platform* pointer, for use by output
    //                    translators to resolve the Sensor::Target::entity field.
    //                    Cleared on entry.
    //
    // RETURNS:    Vector of aesa::TargetInput, one per peer platform with a
    //             valid transform. May be empty if no peers exist.
    //
    // SIDE EFFECTS: Writes prevPositions_[tid] and computedHeadings_[tid]
    //               for each platform seen. Writes outIdMap.
    // =========================================================================
    std::vector<aesa::TargetInput> collectTargets(
        Transform*                               source,
        std::unordered_map<uint32_t, Platform*>& outIdMap) const;

    // =========================================================================
    // FUNCTION:    processSurveillance
    //
    // DESCRIPTION: Translates aesa::DetectionOutput entries from a
    //              SURVEILLANCE-mode RadarOutput into Sensor::targets entries.
    //              Filters DRFM ghosts before appending — ghosts are forwarded
    //              via drfmGhostDetected() signal and not added to targets.
    //              Deduplicates by targetID using addedIDs. REQ-AESA-004.
    //
    // PARAMETERS:
    //   output    [in]      RadarOutput from radarCore_.getOutput().
    //   idMap     [in]      Platform pointer lookup from collectTargets().
    //   addedIDs  [in/out]  Set of targetIDs already added this tick.
    //                       Prevents duplicate target entries.
    //
    // SIDE EFFECTS: Appends to Sensor::targets. Emits drfmGhostDetected
    //               for any detection flagged isDRFMGhost.
    // =========================================================================
    void processSurveillance(
        const aesa::RadarOutput&                       output,
        const std::unordered_map<uint32_t, Platform*>& idMap,
        std::unordered_set<uint32_t>&                  addedIDs);

    // =========================================================================
    // FUNCTION:    processTWS
    //
    // DESCRIPTION: Translates aesa::TrackOutput entries from a TWS-mode
    //              RadarOutput into Sensor::targets entries. Emits DRFM
    //              ghost warnings for any detection flagged isDRFMGhost.
    //              Skips DRFM-suspect tracks (isDRFMSuspect == true) in
    //              the target list — they are never shown to fire-control.
    //              Emits iffResult for validated tracks. Emits
    //              trackBelowDopplerNotch for slow validated tracks.
    //              Deduplicates by track id using addedIDs. REQ-AESA-004.
    //
    // PARAMETERS:
    //   output    [in]      RadarOutput from radarCore_.getOutput().
    //   idMap     [in]      Platform pointer lookup from collectTargets().
    //   addedIDs  [in/out]  Set of ids already added this tick.
    //
    // SIDE EFFECTS: Appends to Sensor::targets. Emits drfmGhostDetected,
    //               iffResult, trackBelowDopplerNotch as appropriate.
    // =========================================================================
    void processTWS(
        const aesa::RadarOutput&                       output,
        const std::unordered_map<uint32_t, Platform*>& idMap,
        std::unordered_set<uint32_t>&                  addedIDs);

    // =========================================================================
    // FUNCTION:    processLockOn
    //
    // DESCRIPTION: Translates the fire-control (LOCK_ON) output for the locked
    //              target into a single Sensor::targets entry. Returns
    //              immediately if lockBroken is set in output. Skips DRFM-
    //              suspect locked targets (emits drfmGhostDetected instead).
    //              Emits iffResult for the locked track. REQ-AESA-003.
    //
    // PARAMETERS:
    //   output  [in]  RadarOutput from radarCore_.getOutput().
    //   idMap   [in]  Platform pointer lookup from collectTargets().
    //
    // SIDE EFFECTS: Appends at most one entry to Sensor::targets. Emits
    //               drfmGhostDetected or iffResult as appropriate.
    // =========================================================================
    void processLockOn(
        const aesa::RadarOutput&                       output,
        const std::unordered_map<uint32_t, Platform*>& idMap);

    // =========================================================================
    // STATIC UTILITY FUNCTIONS
    // Pure computations with no side effects on object state.
    // =========================================================================

    // =========================================================================
    // FUNCTION:    platformToRadarID
    //
    // DESCRIPTION: Converts a platform scene-graph key string to a stable
    //              32-bit radar target ID using the FNV-1a hash algorithm.
    //              Deterministic across all platforms and simulation runs —
    //              the same key always produces the same ID. Never returns 0
    //              (reserved for "no target") — maps zero-hash to 1.
    //
    // PARAMETERS:
    //   key  [in]  Platform scene-graph identifier string. Must be non-empty
    //              and stable across frames for a given platform.
    //
    // RETURNS:    Non-zero uint32_t radar target ID.
    // SIDE EFFECTS: None. Pure computation.
    // =========================================================================
    static uint32_t platformToRadarID(const std::string& key);

    // =========================================================================
    // FUNCTION:    platformRCS
    //
    // DESCRIPTION: Returns a nominal RCS estimate (m²) for a platform.
    //              Currently returns DEFAULT_RCS (5.0 m²) for all platforms.
    //              Reserved for future per-platform RCS lookup. REQ-AESA-040.
    //
    // PARAMETERS:
    //   platform  [in]  Platform pointer. May be null — returns DEFAULT_RCS.
    //
    // RETURNS:    Nominal RCS (m²). Always positive.
    // SIDE EFFECTS: None. Pure computation.
    // =========================================================================
    static double platformRCS(const Platform* platform);

    // =========================================================================
    // FUNCTION:    velocityFromHeadingSpeed
    //
    // DESCRIPTION: Decomposes a scalar ground speed and magnetic heading into
    //              Cartesian velocity components in the radar-local body frame.
    //              vx = speed * cos(heading_rad), vy = speed * sin(heading_rad).
    //              vz is set to 0.0 (no vertical velocity from heading/speed).
    //
    // PARAMETERS:
    //   headingDeg  [in]   Platform heading (degrees, 0 = forward / north).
    //   speedMs     [in]   Ground speed (m/s). Must be >= 0.
    //   vx          [out]  Forward velocity component (m/s).
    //   vy          [out]  Lateral velocity component (m/s).
    //   vz          [out]  Vertical velocity component (m/s). Always 0.0.
    //
    // SIDE EFFECTS: None. Pure computation.
    // =========================================================================
    static void velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                         double& vx, double& vy, double& vz);

    // ---- Persistent velocity tracking (instance state, not static) ----------

    // Previous world-space positions per target ID. Used to compute heading
    // by finite-differencing positions between frames. Keyed by platformToRadarID.
    // Declared mutable so collectTargets() (const) can update it. REQ-AESA-004.
    mutable std::unordered_map<uint32_t, QVector3D> prevPositions_;

    // Most recently computed heading per target ID (degrees).
    // Initialised to 0.0 on first sighting. Updated when movement > 0.001 units.
    // Declared mutable so collectTargets() (const) can update it. REQ-AESA-004.
    mutable std::unordered_map<uint32_t, float> computedHeadings_;
};

#endif // AESARADAR_H



