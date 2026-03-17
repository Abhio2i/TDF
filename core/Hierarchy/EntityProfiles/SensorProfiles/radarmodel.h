#pragma once
// =============================================================================
// radarmodel.h  —  Pure C++ radar simulation model
//
// Design contract:
//   • Zero Qt / engine dependencies.  Only std C++17.
//   • Lifecycle:  init() → start() → update() [loop] → end()
//   • All configuration comes in through RadarConfig  (set once at init,
//     or via setConfig() to hot-reload).
//   • Per-tick sensor pose comes in through RadarPose  (position + attitude).
//   • Per-tick world state comes in through RadarWorldInput (target list).
//   • Everything the engine/display needs comes out through RadarOutput.
//   • The model owns no Qt types, no global state, no singletons.
//   • All public methods are thread-safe (internal mutex).
//   • The model is safe to move to a dedicated thread — the bridge (radar.cpp)
//     calls update() from whatever thread it likes; getOutput() is guarded.
// =============================================================================

#ifndef RADARMODEL_H
#define RADARMODEL_H

#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <vector>

// =============================================================================
// §1  Enumerations
// =============================================================================

enum class RadarMode
{
    SURVEILLANCE,   ///< wide-area search
    TWS,            ///< track-while-scan — Kalman-filtered track database
    LOCK_ON         ///< single-target fire-control
};

enum class SurfaceType
{
    AIR,
    SEA,
    LAND
};

enum class ScanType
{
    MECHANICAL,   ///< constant-rate azimuth rotation
    CONICAL       ///< conical / nodding scan
};

// =============================================================================
// §2  RadarConfig  —  static (or slow-changing) parameters
//
// Set once via init(), or updated at runtime via setConfig().
// The model copies the struct internally; callers own their copy.
// =============================================================================

struct NoiseModel
{
    double rangeStdDev     = 0.0;
    double azimuthStdDev   = 0.0;
    double elevationStdDev = 0.0;
    double dopplerStdDev   = 0.0;
};

struct JammerConfig
{
    bool   active         = false;
    double power_kW       = 0.0;
    double gain_dBi       = 0.0;
    double bandwidth_Hz   = 1e6;
    double range_m        = 0.0;  ///< stand-off jammer range (ignored when selfScreening)
    bool   selfScreening  = false;///< true → jammer rides on the target itself
};

struct RadarConfig
{
    // ---- Propagation / atmosphere ----------------------------------------
    double earthRadiusFactor   = 1.33;  ///< 4/3 earth (standard)
    double atmosphericFactor   = 1.0;   ///< >1 = ducting, <1 = sub-refraction
    double rainRate_mmph       = 0.0;   ///< mm/hr  (0 = clear)
    double fogVisibility_m     = 0.0;   ///< metres (0 = disabled)

    // ---- Transmitter / antenna -------------------------------------------
    double emissionPower_kW    = 100.0;
    double frequency_Hz        = 3e9;
    float  antennaGain         = 30.0f; ///< dBi
    double antennaBandwidth    = 1e6;   ///< Hz
    float  beamWidth           = 3.0f;  ///< degrees
    float  peakSidelobeLevel   = -25.0f;
    float  avgSidelobeLevel    = -35.0f;
    float  pulseWidth          = 1e-6f; ///< seconds

    // ---- PRF / waveform --------------------------------------------------
    float prfLevels[4]         = { 5000, 0, 0, 0 };///< Hz; [0] is primary
    bool  frequencyAgility     = false;
    float hopStepFrequency     = 0.0f;
    float hopStartFrequency    = 0.0f;
    float hopRate              = 0.0f;

    // ---- Scan geometry ---------------------------------------------------
    float    minElevation      = -10.0f;
    float    maxElevation      =  30.0f;
    float    minAzimuth        = -180.0f;
    float    maxAzimuth        =  180.0f;
    float    scanningRate_RPM  =   6.0f;
    ScanType scanType          = ScanType::MECHANICAL;

    // ---- Receiver / detection --------------------------------------------
    double systemTemperature_K = 290.0;
    double noiseFigure_dB      =   5.0;
    double targetPfa           =   1e-6;

    // ---- Clutter ---------------------------------------------------------
    double seaState            = 0.0;
    double landClutter         = 0.5;

    // ---- Geometry / platform --------------------------------------------
    double radarHeight         = 20.0;  ///< metres above sea level
    double minDetectableRange  = 30.0;  ///< metres

    // ---- Track lifecycle -------------------------------------------------
    int    missedScansToDrop   = 5;
    double trackCoastSeconds   = 60.0;
    int    minHitsToValidate   = 3;
    double maxTrackSpeed       = 3000.0;

    // ---- Noise model -----------------------------------------------------
    NoiseModel  noise;

    // ---- ECM -------------------------------------------------------------
    JammerConfig jammer;

    // ---- Operational mode (may be changed via setMode()) ----------------
    RadarMode mode             = RadarMode::SURVEILLANCE;
    uint32_t  lockedTargetID   = 0;     ///< valid only in LOCK_ON mode
};

// =============================================================================
// §3  RadarPose  —  updated every tick by the bridge
//
// Position and attitude of the radar platform in world coordinates.
// The model uses this to:
//   • derive the correct local coordinate frame for target geometry
//   • update radarHeight for horizon calculations
//   • (future) apply platform roll/pitch corrections to beam pointing
// =============================================================================

struct RadarPose
{
    // Position in world metres (engine converts from its own coords)
    double x = 0.0; ///< East  (or +X in NED)
    double y = 0.0; ///< Up    (altitude)
    double z = 0.0; ///< North (or +Z in NED)

    // Attitude in degrees — supplied by the engine, reserved for future use
    float roll    = 0.0f;
    float pitch   = 0.0f;
    float heading = 0.0f;
};

// =============================================================================
// §4  RadarWorldInput  —  one entry per potentially detectable object
// =============================================================================

struct TargetInput
{
    uint32_t    id      = 0;
    double      x       = 0.0;   ///< local position relative to radar origin (m)
    double      y       = 0.0;
    double      z       = 0.0;
    double      vx      = 0.0;   ///< velocity in m/s (same frame)
    double      vy      = 0.0;
    double      vz      = 0.0;
    double      rcs     = 1.0;   ///< radar cross-section m²
    SurfaceType surface = SurfaceType::AIR;
};

// =============================================================================
// §5  RadarOutput  —  everything the engine/display needs per tick
// =============================================================================

/// Per-detection record (surveillance and raw TWS hits)
struct DetectionOutput
{
    uint32_t targetID       = 0;

    double range            = 0.0;   ///< metres
    double azimuth          = 0.0;   ///< degrees (radar-local)
    double elevation        = 0.0;   ///< degrees
    double snr              = 0.0;   ///< SINR after clutter + noise + jamming

    double radialVelocity   = 0.0;   ///< m/s (negative = closing)
    double cpa_distance     = 0.0;   ///< metres
    double time_to_cpa      = 0.0;   ///< seconds
    double Pk               = 0.0;   ///< probability of kill estimate

    double heading          = 0.0;   ///< degrees
    double speedOverGround  = 0.0;   ///< m/s horizontal
    double acceleration     = 0.0;
    double targetAspect     = 0.0;   ///< degrees

    bool   isAmbiguous      = false;
    bool   lockBroken       = false;
};

/// Per-track record (TWS / lock-on)
struct TrackOutput
{
    uint32_t id             = 0;

    double x  = 0.0, y  = 0.0, z  = 0.0;   ///< estimated position (m)
    double vx = 0.0, vy = 0.0, vz = 0.0;   ///< estimated velocity (m/s)

    double range            = 0.0;
    double azimuth          = 0.0;   ///< degrees
    double elevation        = 0.0;   ///< degrees
    double radialVelocity   = 0.0;
    double speedOverGround  = 0.0;
    double heading          = 0.0;
    double targetAspect     = 0.0;
    double cpa_distance     = 0.0;
    double time_to_cpa      = 0.0;
    double Pk               = 0.0;

    int    hitCount         = 0;
    int    scanMissCount    = 0;
    bool   isValidated      = false;
    bool   wasAmbiguous     = false;
};

/// Top-level output bundle — the bridge reads this atomically
struct RadarOutput
{
    // Raw detections (SURVEILLANCE mode; also filled during TWS beam pass)
    std::vector<DetectionOutput> detections;

    // Validated track list (TWS + LOCK_ON)
    std::vector<TrackOutput>     tracks;

    // Current antenna state (for display sweep line)
    double currentAzimuth   = 0.0;   ///< degrees
    double currentElevation = 0.0;   ///< degrees

    // Operational state
    RadarMode mode          = RadarMode::SURVEILLANCE;
    bool      lockBroken    = false; ///< true if LOCK_ON target was lost this tick

    // Computed display range (km) — recomputed when config changes
    double displayRange_km  = 100.0;
};

// =============================================================================
// §6  Internal structures  —  not part of the public API but declared here
//     so that radar.cpp can inspect track internals if needed for diagnostics.
//     Bridge code should prefer RadarOutput / TrackOutput.
// =============================================================================

struct TrackFile
{
    uint32_t id = 0;

    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;

    double range          = 0.0;
    double velocity       = 0.0;   ///< radial
    double predictedRange = 0.0;

    double lastSeenTime   = 0.0;
    int    hitCount       = 0;

    bool isValidated      = false;
    bool isUpdated        = false;
    bool wasAmbiguous     = false;

    int  missCount        = 0;   ///< cycle-based (diagnostics only)
    int  scanMissCount    = 0;   ///< per-scan miss counter
    bool updatedThisScan  = false;

    std::array<double, 6> X = {};
    double P[6][6] = {};
    double Q[6][6] = {};
    double R[3][3] = {};
};

// =============================================================================
// §7  RadarModel  —  the simulation model class
// =============================================================================

class RadarModel
{
public:
    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// Allocate internal state, apply config, zero all counters.
    /// Must be called before start().
    void init(const RadarConfig& cfg);

    /// Arm the model: reset scan angle, clear track DB, start simClock at 0.
    /// Safe to call again to perform a hot-restart without re-init.
    void start();

    /// Main per-tick entry point.
    ///   dt          — seconds since last call
    ///   pose        — current platform position + attitude
    ///   worldInputs — all visible targets in radar-local coordinates
    ///   simTime     — absolute simulation clock (seconds from mission start)
    ///
    /// Thread-safe: acquires internal mutex for the duration of the call.
    void update(double                        dt,
                const RadarPose&              pose,
                const std::vector<TargetInput>& worldInputs,
                double                        simTime);

    /// Graceful shutdown — flush tracks, mark all outputs stale.
    /// After end(), call init() + start() to reuse the object.
    void end();

    // -------------------------------------------------------------------------
    // Configuration (hot-reload, thread-safe)
    // -------------------------------------------------------------------------

    /// Replace the running configuration.  Takes effect on the next update().
    /// Marks displayRange dirty so it is recomputed.
    void setConfig(const RadarConfig& cfg);

    /// Read back the current config (a copy — thread-safe).
    RadarConfig getConfig() const;

    // -------------------------------------------------------------------------
    // Mode control (thread-safe convenience wrappers)
    // -------------------------------------------------------------------------

    void setMode(RadarMode mode);
    void lockOn(uint32_t targetID);   ///< sets mode = LOCK_ON + lockedTargetID
    void breakLock();                 ///< returns to SURVEILLANCE

    // -------------------------------------------------------------------------
    // Output (thread-safe snapshot)
    // -------------------------------------------------------------------------

    /// Returns a complete snapshot of the latest computed output.
    /// Safe to call from any thread; does not block the update thread for long.
    RadarOutput getOutput() const;

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    /// Compute theoretical maximum detection range for a given RCS (m²).
    /// Thread-safe; uses current config.
    double computeMaxDetectionRange(double rcs = 10.0) const;

    /// Unfold a measured range given a predicted range and PRF Rmax.
    double resolveRangeAmbiguity(double measuredRange,
                                 double predictedRange,
                                 double Rmax) const;

    /// Hard reset — equivalent to end() + init(current config) + start().
    void reset();

private:
    // =========================================================================
    // Internal mutex — guards config_, output_, trackDatabase_
    // =========================================================================
    mutable std::mutex mutex_;

    // =========================================================================
    // State
    // =========================================================================
    RadarConfig config_;

    double currentAzimuth_   = 0.0;
    double currentElevation_ = 0.0;
    double scanDirection_    = 1.0;
    double previousAzimuth_  = 0.0;
    bool   scanBoundaryOccurred_ = false;

    bool   initialised_ = false;
    bool   running_     = false;

    // Cached display range — recomputed when config changes
    mutable double cachedDisplayRange_km_ = 100.0;
    mutable bool   displayRangeDirty_     = true;

    // Track database
    std::vector<TrackFile> trackDatabase_;

    // Output snapshot — written by update(), read by getOutput()
    RadarOutput latestOutput_;

    // =========================================================================
    // Antenna
    // =========================================================================
    void updateAntennaScan  (double dt);
    bool updateAntennaLockOn(const std::vector<TargetInput>& targets);

    bool detectScanBoundary (double prevAz, double newAz) const;
    void applyScanMissLogic (double simTime);

    // =========================================================================
    // Beam / horizon geometry
    // =========================================================================
    bool checkHorizon(double range, double targetZ) const;

    bool isTargetInBeam(double targetAz, double targetEl, double dt,
                        double& outAzDiff, double& outElDiff,
                        double& outScanMargin) const;

    double computeEffectiveRCS(const TargetInput& target, double range) const;

    // =========================================================================
    // Signal chain
    // =========================================================================
    double calculateSignalStrength (double range, double rcs)           const;
    double computeNoisePower       ()                                   const;
    double computeClutterPower     (double range, SurfaceType surface)  const;
    double computeJammerPower      (double targetRange_m)               const;
    double computePropagationLoss  (double range_m)                     const;

    /// Returns SINR = Pr / (Pn + Pc + Pj)
    double computeSINR(double receivedPower, double range,
                       SurfaceType surface) const;

    std::vector<double> generateReferenceCells(SurfaceType surface) const;

    double computeCFARThreshold        (const std::vector<double>& cells) const;
    double computeCFARThresholdRelaxed (const std::vector<double>& cells) const;

    // =========================================================================
    // Motion / geometry helpers
    // =========================================================================
    void   computeTargetMotionParams(DetectionOutput& det,
                                   const TargetInput& target,
                                   double range) const;

    double computeRadialVelocity(const TargetInput& target, double range,
                                 std::normal_distribution<double>& dopplerNoise) const;

    void   computeCPA(DetectionOutput& det,
                    const TargetInput& target,
                    double range) const;

    double computePk(double range, double radialVelocity) const;

    // =========================================================================
    // Range ambiguity
    // =========================================================================
    void applyRangeAmbiguity(DetectionOutput& det, double range,
                             double maxUnambiguousRange,
                             std::normal_distribution<double>& rangeNoise) const;

    void resolveRangeForLockOn(DetectionOutput& det, double range,
                               double maxUnambiguousRange, uint32_t targetId) const;

    // =========================================================================
    // Detection merge / association
    // =========================================================================
    bool shouldMergeDetection(const DetectionOutput& det,
                              const std::vector<DetectionOutput>& existing) const;

    // =========================================================================
    // Tracking
    // =========================================================================
    double computeAssociationProbability(double measurementRange,
                                         double predictedRange,
                                         double gateSize) const;

    TrackFile* findBestTrackMatch(const DetectionOutput& det,
                                  double maxUnambiguousRange,
                                  double& outBestProb);

    void performKalmanUpdate(TrackFile& track, const DetectionOutput& det,
                             double simTime, double dt,
                             double maxUnambiguousRange);

    void createNewTrack(const DetectionOutput& det,
                        const TargetInput& target,
                        double maxUnambiguousRange,
                        double simTime);

    void updateTWSPrediction(double dt);

    // =========================================================================
    // Per-target detection pipeline
    // =========================================================================
    bool processTargetDetection(
        const TargetInput&              target,
        double                          dt,
        double                          simTime,
        double                          maxUnambiguousRange,
        std::vector<DetectionOutput>&   scanDetections,
        std::normal_distribution<double>& rangeNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dopplerNoise);

    // =========================================================================
    // TWS / output assembly
    // =========================================================================
    TrackOutput     buildTrackOutput   (const TrackFile& track)            const;
    DetectionOutput buildTWSDetection  (const TrackFile& track)            const;
    void            generateTWSReport  (std::vector<TrackOutput>& out)     const;
    void            assembleFinalOutput();

    // Internal unlocked worker for computeMaxDetectionRange —
    // called from update() (mutex already held) to avoid deadlock.
    double computeMaxDetectionRange_locked(double rcs = 10.0)              const;
};

// =============================================================================
// §8  C ABI — for dlopen / FFI usage
// =============================================================================
extern "C"
{
RadarModel* radarmodel_create();
void        radarmodel_destroy(RadarModel* obj);
}

#endif // RADARMODEL_H// RADARMODEL_H// RADARMODEL_H
// #ifndef RADARMODEL_H
// #define RADARMODEL_H

// #include <vector>
// #include <string>
// #include <cmath>
// #include <cstdint>
// #include <array>
// #include <mutex>
// #include <random>

// enum class RadarMode {
//     SURVEILLANCE,
//     LOCK_ON,
//     TWS
// };

// enum class SurfaceType {
//     AIR,
//     SEA,
//     LAND
// };

// enum class ScanType {
//     MECHANICAL,
//     CONICAL
// };

// struct NoiseModel {
//     double rangeStdDev     = 0.0;
//     double azimuthStdDev   = 0.0;
//     double elevationStdDev = 0.0;
//     double dopplerStdDev   = 0.0;
// };
// // -----------------------------------------------------------------------
// // Jamming / ECM
// // -----------------------------------------------------------------------
// struct JammerConfig {
//     bool   active          = false;
//     double power_kW        = 0.0;   // jammer transmit power
//     double gain_dBi        = 0.0;   // jammer antenna gain toward radar
//     double bandwidth_Hz    = 1e6;   // jammer bandwidth
//     double range_m         = 0.0;   // distance from radar to jammer (m)
//     bool   selfScreening   = false; // true = jammer is on the target itself
// };
// struct RadarAttributes {

//     double earthRadiusFactor  = 1.33;   // standard 4/3 earth model
//     double atmosphericFactor  = 1.0;    // multiplier on top of earthRadiusFactor
//     // >1.0 = super-refraction / ducting
//     // <1.0 = sub-refraction (dry cold air)
//     std::string categories;
//     std::string detectionCapabilities;

//     double emissionPower_kW  = 100.0;
//     double frequency_Hz      = 3e9;

//     float minElevation       = -10.0f;
//     float maxElevation       =  30.0f;
//     float minAzimuth         = -180.0f;
//     float maxAzimuth         =  180.0f;

//     float scanningRate_RPM   = 6.0f;
//     int   scanningNumHits    = 1;
//     ScanType scanType        = ScanType::MECHANICAL;
//     float scanTime0          = 0.0f;
//     float scanTime1          = 0.0f;

//     float  antennaGain       = 30.0f;        // dBi
//     double antennaBandwidth  = 1e6;          // Hz — double, not float
//     float  beamWidth         = 3.0f;         // degrees
//     float  peakSidelobeLevel = -25.0f;
//     float  avgSidelobeLevel  = -35.0f;

//     float hopStepFrequency   = 0.0f;
//     float hopStartFrequency  = 0.0f;
//     float hopRate            = 0.0f;
//     bool  frequencyAgility   = false;
//     int   emitterIdentity    = 0;

//     std::string signalType;
//     float signalPRI          = 0.0f;
//     int   priCount           = 0;
//     int   pwCount            = 0;
//     int   frequencyCount     = 0;

//     std::string currentPattern;
//     bool pattern1 = false, pattern2 = false, pattern3 = false;

//     float       pulseWidth   = 1e-6f;        // seconds
//     std::string prfType;
//     float       prfLevels[4] = {5000, 0, 0, 0};
//     int         modulationType = 0;

//     RadarMode mode           = RadarMode::SURVEILLANCE;
//     uint32_t  lockedTargetID = 0;

//     NoiseModel noise;
//     JammerConfig jammer;

//     double radarHeight        = 20.0;
//     double minDetectableRange = 30.0;

//     double seaState    = 0.0;
//     double landClutter = 0.5;
//     // Environmental propagation losses
//     double rainRate_mmph      = 0.0;   // mm/hr — 0=clear, 4=light, 16=moderate, 100=heavy
//     double fogVisibility_m = 0.0;   // metres — 0=disabled, 200=dense fog, 2000=light fog

//     double maxTrackSpeed = 3000.0;

//     // -----------------------------------------------------------------------
//     // Receiver noise (Fix 3)
//     // -----------------------------------------------------------------------
//     double systemTemperature_K = 290.0;
//     double noiseFigure_dB      = 5.0;

//     // -----------------------------------------------------------------------
//     // CFAR (Fix 2)
//     // -----------------------------------------------------------------------
//     double targetPfa = 1e-6;

//     // -----------------------------------------------------------------------
//     // Track lifecycle — PRODUCTION FIX
//     //
//     // missedScansToDrop : drop a track after this many consecutive missed
//     //                     antenna scans (not update() cycles).  3-5 is typical
//     //                     for a real tracker.  Stationary targets in clutter
//     //                     may need 5-8.
//     //
//     // trackCoastSeconds : hard upper bound — drop any track not seen for this
//     //                     many seconds regardless of scan count.  Safety net.
//     //
//     // minHitsToValidate : M-of-N initiation gate.  Track is shown on display
//     //                     only after this many detections.  Default 3.
//     // -----------------------------------------------------------------------
//     int    missedScansToDrop  = 5;
//     double trackCoastSeconds  = 60.0;
//     int    minHitsToValidate  = 3;
// };

// // ---------------------------------------------------------------------------
// // TrackFile — added scanMissCount (per-scan miss counter, replaces missCount
// // for dropout logic) and lastScanAzimuth (to detect scan boundary crossing).
// // ---------------------------------------------------------------------------
// struct TrackFile {
//     uint32_t id = 0;

//     double x = 0.0, y = 0.0, z = 0.0;
//     double vx = 0.0, vy = 0.0, vz = 0.0;

//     double range         = 0.0;
//     double velocity      = 0.0;   // radial
//     double predictedRange = 0.0;

//     double lastSeenTime  = 0.0;
//     int    hitCount      = 0;

//     bool isValidated  = false;
//     bool isUpdated    = false;
//     bool wasAmbiguous = false;

//     // --- old cycle-based counter kept for diagnostics only ---
//     int missCount     = 0;

//     // --- PRODUCTION: per-scan miss counter ---
//     int  scanMissCount    = 0;   // increments once per full antenna scan with no hit
//     bool updatedThisScan  = false; // was this track hit during the current scan pass?

//     std::array<double, 6> X = {};

//     double P[6][6] = {};
//     double Q[6][6] = {};
//     double R[3][3] = {};
// };

// struct TargetInput {
//     uint32_t   id = 0;
//     double     x = 0.0, y = 0.0, z = 0.0;
//     double     rcs = 1.0;
//     double     vx = 0.0, vy = 0.0, vz = 0.0;
//     SurfaceType surface = SurfaceType::AIR;
// };

// struct DetectionOutput {
//     uint32_t targetID  = 0;

//     double range       = 0.0;
//     double azimuth     = 0.0;
//     double elevation   = 0.0;
//     double snr         = 0.0;     // true SINR after Fix 1+3

//     bool   isAmbiguous = false;

//     double radialVelocity = 0.0;
//     double cpa_distance   = 0.0;
//     double time_to_cpa    = 0.0;
//     double Pk             = 0.0;

//     bool   lockBroken     = false;

//     double heading         = 0.0;
//     double speedOverGround = 0.0;
//     double acceleration    = 0.0;
//     double targetAspect    = 0.0;
// };

// // ===========================================================================
// class RadarModel {
// public:
//     RadarModel();

//     void setConfiguration(const RadarAttributes& attrs);

//     void update(double dt,
//                 const std::vector<TargetInput>& worldTargets,
//                 double currentTime);

//     std::vector<DetectionOutput> getActiveDetections();
//     std::vector<TrackFile>       getActiveTracks();

//     RadarAttributes getConfiguration() const { return config; }
//     double          getCurrentAzimuth() const { return currentAzimuth; }
//     RadarMode       getMode()           const { return config.mode; }

//     void reset();

//     double resolveRangeAmbiguity(double measuredRange,
//                                  double predictedRange,
//                                  double Rmax);
//     // In radarmodel.h — add this declaration
//     double computeMaxDetectionRange(double rcs = 10.0) const;
// private:
//     std::mutex radarMutex;

//     RadarAttributes config;

//     double currentAzimuth   = 0.0;
//     double currentElevation = 0.0;
//     double scanDirection    = 1.0;

//     // ------------------------------------------------------------------
//     // Scan-boundary tracking — used to detect when the antenna has
//     // completed one full pass so scanMissCount can be updated correctly.
//     // ------------------------------------------------------------------
//     double previousAzimuth      = 0.0;
//     bool   scanBoundaryOccurred = false;

//     std::vector<DetectionOutput> lastDetections;
//     std::vector<TrackFile>       trackDatabase;

//     // ==================================================================
//     // TWS + Scan
//     // ==================================================================
//     void updateTWS(double dt, double currentTime);
//     void updateAntennaScan(double dt);
//     bool updateAntennaLockOn(const std::vector<TargetInput>& worldTargets);

//     // PRODUCTION: detect scan boundary crossing this cycle
//     bool detectScanBoundary(double prevAz, double newAz) const;

//     // PRODUCTION: after each scan boundary, update per-scan miss counts
//     //             and prune tracks that have missed too many scans.
//     void applyScanMissLogic(double currentTime);

//     // ==================================================================
//     // Beam & Horizon
//     // ==================================================================
//     bool   checkHorizon(double range, double targetZ);
//     bool   checkBeamIntersection(double targetAz, double targetEl);
//     double computeEffectiveRCS(const TargetInput& target, double range);

//     bool isTargetInBeam(double targetAz, double targetEl, double dt,
//                         double& outAzDiff, double& outElDiff,
//                         double& outScanMargin);

//     // ==================================================================
//     // Radar Physics
//     // ==================================================================
//     double calculateSignalStrength(double range, double rcs);
//     double computeNoisePower()                                        const;
//     double computeClutterPower(double range, SurfaceType surface)     const;
//     double applyClutterEffects(double receivedPower, double range, SurfaceType surface);

//     std::vector<double> generateReferenceCells(SurfaceType surface);
//     double computeCFARThreshold(const std::vector<double>& referenceCells);

//     // PRODUCTION: relaxed CFAR threshold for near-stationary targets
//     double computeCFARThresholdRelaxed(const std::vector<double>& referenceCells);

//     // ==================================================================
//     // Motion / Geometry
//     // ==================================================================
//     void   computeTargetMotionParams(DetectionOutput& det,
//                                    const TargetInput& target,
//                                    double range);

//     double computeRadialVelocity(const TargetInput& target, double range,
//                                  std::normal_distribution<double>& dopplerNoise);

//     void   computeCPA(DetectionOutput& det, const TargetInput& target,
//                     double range);

//     double computePk(double range, double radialVelocity);

//     // ==================================================================
//     // Range Ambiguity
//     // ==================================================================
//     void applyRangeAmbiguity(DetectionOutput& det, double range,
//                              double maxUnambiguousRange,
//                              std::normal_distribution<double>& rangeNoise);

//     void resolveRangeForLockOn(DetectionOutput& det, double range,
//                                double maxUnambiguousRange, int targetId);

//     // ==================================================================
//     // Detection Filtering
//     // ==================================================================
//     bool shouldMergeDetection(const DetectionOutput& det,
//                               const std::vector<DetectionOutput>& scanDetections);

//     // ==================================================================
//     // Tracking
//     // ==================================================================
//     double computeAssociationProbability(double measurementRange,
//                                          double predictedRange,
//                                          double gateSize);

//     TrackFile* findBestTrackMatch(const DetectionOutput& det,
//                                   double maxUnambiguousRange,
//                                   double& outBestProb);

//     void performKalmanUpdate(TrackFile& track, const DetectionOutput& det,
//                              double currentTime, double dt,
//                              double maxUnambiguousRange);

//     void createNewTrack(const DetectionOutput& det, const TargetInput& target,
//                         double maxUnambiguousRange, double currentTime);

//     bool processTargetDetection(
//         const TargetInput& target, double dt, double currentTime,
//         double maxUnambiguousRange,
//         std::vector<DetectionOutput>& scanDetections,
//         std::normal_distribution<double>& rangeNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dopplerNoise);
//     double computePropagationLoss(double range_m) const;
//     double computeJammerPower(double targetRange_m) const;

//     // ==================================================================
//     // TWS Output
//     // ==================================================================
//     DetectionOutput buildTWSOutput(const TrackFile& track);
//     void            generateTWSReport();

// };

// // ===========================================================================
// extern "C" {
// RadarModel* createRadar();
// void        destroyRadar(RadarModel* obj);
// }

// #endif
// #ifndef RADARMODEL_H
// #define RADARMODEL_H

// #include <vector>
// #include <string>
// #include <cmath>
// #include <cstdint>
// #include <array>
// #include <mutex>
// #include <random>
// enum class RadarMode {
//     SURVEILLANCE,
//     LOCK_ON,
//     TWS
// };

// enum class SurfaceType
// {
//     AIR,
//     SEA,
//     LAND
// };

// enum class ScanType
// {
//     MECHANICAL,
//     CONICAL
// };

// struct NoiseModel
// {
//     double rangeStdDev = 0.0;
//     double azimuthStdDev = 0.0;
//     double elevationStdDev = 0.0;
//     double dopplerStdDev = 0.0;
// };

// struct RadarAttributes {

//     double earthRadiusFactor = 1.33;

//     std::string categories;
//     std::string detectionCapabilities;

//     double emissionPower_kW;
//     double frequency_Hz;

//     float minElevation;
//     float maxElevation;
//     float minAzimuth = -180;
//     float maxAzimuth = 180;

//     float scanningRate_RPM;
//     int scanningNumHits;
//     ScanType scanType = ScanType::MECHANICAL;
//     float scanTime0;
//     float scanTime1;

//     float antennaGain;
//     //float antennaBandwidth;
//     double antennaBandwidth = 1e6;   // Hz, default 1 MHz

//     float beamWidth;
//     float peakSidelobeLevel;
//     float avgSidelobeLevel;

//     float hopStepFrequency;
//     float hopStartFrequency;
//     float hopRate;
//     bool frequencyAgility;
//     int emitterIdentity;

//     std::string signalType;
//     float signalPRI;
//     int priCount;
//     int pwCount;
//     int frequencyCount;

//     std::string currentPattern;
//     bool pattern1, pattern2, pattern3;

//     float pulseWidth;
//     std::string prfType;
//     float prfLevels[4];
//     int modulationType;

//     RadarMode mode = RadarMode::SURVEILLANCE;
//     uint32_t lockedTargetID = 0;

//     NoiseModel noise;

//     double radarHeight = 20.0;
//     double minDetectableRange = 30.0;

//     double seaState = 0.0;
//     double landClutter = 0.5;

//     double maxTrackSpeed = 3000.0;
//     // Noise floor parameters (Fix 3)
//     double systemTemperature_K = 290.0;
//     double noiseFigure_dB      = 5.0;

//     // CFAR false-alarm probability (Fix 2)
//     double targetPfa = 1e-6;

// };

// struct TrackFile
// {
//     uint32_t id;

//     double x = 0.0;
//     double y = 0.0;
//     double z = 0.0;

//     double vx = 0.0;
//     double vy = 0.0;
//     double vz = 0.0;

//     double range = 0.0;
//     double velocity = 0.0;//radial

//     double predictedRange = 0.0;

//     double lastSeenTime = 0.0;
//     int hitCount = 0;

//     bool isValidated = false;
//     bool isUpdated = false;
//     bool wasAmbiguous = false;
//     int missCount = 0;

//     std::array<double,6> X;

//     double P[6][6] = {0};
//     double Q[6][6] = {0};
//     double R[3][3] = {0};
// };

// struct TargetInput {
//     uint32_t id;
//     double x,y,z;
//     double rcs;
//     double vx,vy,vz;
//     SurfaceType surface = SurfaceType::AIR;
// };

// struct DetectionOutput {

//     uint32_t targetID = 0;

//     double range = 0;
//     double azimuth = 0;
//     double elevation = 0;

//     double snr = 0;

//     bool isAmbiguous = false;

//     double radialVelocity = 0;

//     double cpa_distance = 0;
//     double time_to_cpa = 0;

//     double Pk = 0;

//     bool lockBroken = false;

//     double heading = 0;
//     double speedOverGround = 0;
//     double acceleration = 0;
//     double targetAspect = 0;
// };

// class RadarModel {
// public:

//     RadarModel();

//     void setConfiguration(const RadarAttributes& attrs);

//     void update(double dt,
//                 const std::vector<TargetInput>& worldTargets,
//                 double currentTime);

//     std::vector<DetectionOutput> getActiveDetections();
//     std::vector<TrackFile> getActiveTracks();

//     RadarAttributes getConfiguration() const { return config; }

//     double getCurrentAzimuth() const { return currentAzimuth; }

//     void reset();

//     double resolveRangeAmbiguity(double measuredRange,
//                                  double predictedRange,
//                                  double Rmax);

//     RadarMode getMode() const { return config.mode; }

// private:

//     std::mutex radarMutex;

//     RadarAttributes config;

//     double currentAzimuth;
//     double currentElevation;

//     std::vector<DetectionOutput> lastDetections;
//     std::vector<TrackFile> trackDatabase;

//     double scanDirection = 1.0;

//     /* ===================================================== */
//     /* TWS + Scan Control                                    */
//     /* ===================================================== */

//     void updateTWS(double dt, double currentTime);
//     void updateAntennaScan(double dt);
//     bool updateAntennaLockOn(const std::vector<TargetInput>& worldTargets);

//     /* ===================================================== */
//     /* Beam & Horizon                                       */
//     /* ===================================================== */

//     bool checkHorizon(double range, double targetZ);
//     bool checkBeamIntersection(double targetAz, double targetEl);
//     double computeEffectiveRCS(const TargetInput& target, double range);

//     bool isTargetInBeam(
//         double targetAz,
//         double targetEl,
//         double dt,
//         double& outAzDiff,
//         double& outElDiff,
//         double& outScanMargin);

//     /* ===================================================== */
//     /* Radar Physics                                         */
//     /* ===================================================== */

//     // double calculateSignalStrength(double range, double rcs);

//     // double applyClutterEffects(double snr,
//     //                            double range,
//     //                            SurfaceType surface);
//     double calculateSignalStrength(double range, double rcs);

//     double computeNoisePower() const;                                    // Fix 3
//     double computeClutterPower(double range, SurfaceType surface) const; // Fix 1
//     double applyClutterEffects(double receivedPower, double range, SurfaceType surface);

//     std::vector<double> generateReferenceCells(SurfaceType surface);

//     double computeCFARThreshold(
//         const std::vector<double>& referenceCells);

//     /* ===================================================== */
//     /* Motion / Geometry                                     */
//     /* ===================================================== */

//     void computeTargetMotionParams(
//         DetectionOutput& det,
//         const TargetInput& target,
//         double range);

//     double computeRadialVelocity(
//         const TargetInput& target,
//         double range,
//         std::normal_distribution<double>& dopplerNoise);

//     void computeCPA(
//         DetectionOutput& det,
//         const TargetInput& target,
//         double range);

//     double computePk(double range,
//                      double radialVelocity);

//     /* ===================================================== */
//     /* Range Ambiguity                                       */
//     /* ===================================================== */

//     void applyRangeAmbiguity(
//         DetectionOutput& det,
//         double range,
//         double maxUnambiguousRange,
//         std::normal_distribution<double>& rangeNoise);

//     void resolveRangeForLockOn(
//         DetectionOutput& det,
//         double range,
//         double maxUnambiguousRange,
//         int targetId);

//     /* ===================================================== */
//     /* Detection Filtering                                   */
//     /* ===================================================== */

//     bool shouldMergeDetection(
//         const DetectionOutput& det,
//         const std::vector<DetectionOutput>& scanDetections);

//     /* ===================================================== */
//     /* Tracking                                              */
//     /* ===================================================== */

//     TrackFile* findBestTrackMatch(
//         const DetectionOutput& det,
//         double maxUnambiguousRange,
//         double& outBestProb);

//     void performKalmanUpdate(
//         TrackFile& track,
//         const DetectionOutput& det,
//         double currentTime,
//         double dt,
//         double maxUnambiguousRange);

//     void createNewTrack(
//         const DetectionOutput& det,
//         const TargetInput& target,
//         double maxUnambiguousRange,
//         double currentTime);

//     bool processTargetDetection(
//         const TargetInput& target,
//         double dt,
//         double currentTime,
//         double maxUnambiguousRange,
//         std::vector<DetectionOutput>& scanDetections,
//         std::normal_distribution<double>& rangeNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dopplerNoise);
//     double computeAssociationProbability(
//         double measurementRange,
//         double predictedRange,
//         double gateSize);
//     /* ===================================================== */
//     /* TWS Output                                            */
//     /* ===================================================== */

//     DetectionOutput buildTWSOutput(const TrackFile& track);

//     void generateTWSReport();
// };

// /* ===================================================== */
// /* Shared Library Interface                              */
// /* ===================================================== */

// extern "C" {
// RadarModel* createRadar();
// void destroyRadar(RadarModel* obj);
// }

// #endif
// #ifndef RADARMODEL_H
// #define RADARMODEL_H

// #include <vector>
// #include <string>
// #include <cmath>
// #include <cstdint>
// #include <array>
// #include <mutex>
//     enum class RadarMode {
//         SURVEILLANCE,  // 360-degree spinning (Default)
//         LOCK_ON,      // Focused "Stare" at a single Azimuth/Elevation
//         TWS
//     };
// enum class SurfaceType
// {
//     AIR,
//     SEA,
//     LAND
// };
// enum class ScanType
// {
//     MECHANICAL,
//     CONICAL
// };
// struct NoiseModel
// {
//     double rangeStdDev = 0.0;//30.0;       // meters
//     double azimuthStdDev = 0.0;//0.2;      // degrees
//     double elevationStdDev = 0.0;//0.2;    // degrees
//     double dopplerStdDev = 0.0;//1.0;      // m/s
// };
// struct RadarAttributes {
//     // Horizon [cite: 35-37]
//     double earthRadiusFactor=1.33; //

//     // Detection Capabilities [cite: 40-43]
//     std::string categories; // [cite: 39]
//     std::string detectionCapabilities; // [cite: 42]

//     // Emission [cite: 44-48]
//     double emissionPower_kW; // [cite: 47]
//     double frequency_Hz; // [cite: 48]

//     // Detection Envelope [cite: 49-54]
//     float minElevation; // [cite: 50]
//     float maxElevation; // [cite: 51]
//     float minAzimuth=-180; // [cite: 52]
//     float maxAzimuth=180; // [cite: 53]

//     // Scanning [cite: 55-56, 61-63, 69]
//     float scanningRate_RPM; //
//     int scanningNumHits; // [cite: 56]
//     ScanType scanType = ScanType::MECHANICAL;
//     float scanTime0; // [cite: 62]
//     float scanTime1; // [cite: 63]

//     // Antenna [cite: 57-60, 64-65]
//     float antennaGain; // [cite: 58]
//     float antennaBandwidth; // [cite: 59]
//     float beamWidth; //
//     float peakSidelobeLevel; //
//     float avgSidelobeLevel; // [cite: 65]

//     // Frequency Agility [cite: 66-68]
//     float hopStepFrequency; // [cite: 66]
//     float hopStartFrequency; // [cite: 66]
//     float hopRate; // [cite: 67]
//     bool frequencyAgility; // [cite: 68]
//     int emitterIdentity; // [cite: 68]

//     // Signal Info [cite: 70-74]
//     std::string signalType; // [cite: 70]
//     float signalPRI; // [cite: 70]
//     int priCount; // [cite: 71]
//     int pwCount; // [cite: 72]
//     int frequencyCount; // [cite: 73]

//     // Scan Patterns [cite: 74-77]
//     std::string currentPattern; // [cite: 74]
//     bool pattern1, pattern2, pattern3; // [cite: 75-77]

//     // Pulse & Modulation [cite: 78-83]
//     float pulseWidth; // [cite: 79]
//     std::string prfType; // [cite: 80]
//     float prfLevels[4]; //
//     int modulationType; // [cite: 83]
//     RadarMode mode = RadarMode::SURVEILLANCE;
//     uint32_t lockedTargetID = 0; // ID of the target we are staring at
//     NoiseModel noise;   // ⭐ NEW
//     double radarHeight = 20.0;   // meters above surface
//     double minDetectableRange = 30.0;
//     double seaState = 0.0;   // sea clutter strength.. // 0 calm, 1 light, 2 moderate, 3 rough
//     double landClutter = 0.5; // ground clutter strength
//     double maxTrackSpeed = 3000.0;
// };

// struct TrackFile
// {
//     uint32_t id;

//     // Full 3D state
//     double x = 0.0;
//     double y = 0.0;
//     double z = 0.0;

//     double vx = 0.0;
//     double vy = 0.0;
//     double vz = 0.0;

//     double range = 0.0;
//     double velocity = 0.0;  // radial velocity

//     double predictedRange = 0.0;

//     double lastSeenTime = 0.0;
//     int hitCount = 0;

//     bool isValidated = false;
//     bool isUpdated = false;
//     bool wasAmbiguous = false;
//     int missCount = 0;

//    // bool isTentative = true;
//     // ---------- NEW: Kalman filter ----------

//     std::array<double,6> X;               // state vector
//     double P[6][6] = {0};                 // covariance matrix
//     double Q[6][6] = {0};   // process noise
//     double R[3][3] = {0};   // measurement noise
// };

// struct TargetInput {
//     uint32_t id;
//     double x, y, z;
//     double rcs;
//     double vx, vy, vz; // Velocity in m/s
//     SurfaceType surface = SurfaceType::AIR;   // default

// };

// struct DetectionOutput {
//     uint32_t targetID = 0;
//     double range = 0;
//     double azimuth = 0;
//     double elevation = 0;
//     double snr = 0;
//     bool isAmbiguous = false;
//     double radialVelocity = 0;
//     double cpa_distance = 0;
//     double time_to_cpa = 0;
//     double Pk = 0;
//     bool lockBroken = false;
//     // NEW FIELDS
//     double heading = 0;          // movement direction (deg)
//     double speedOverGround = 0;  // |velocity|
//     double acceleration = 0;     // dv/dt
//     double targetAspect = 0;     // angle relative to radar
// };

// class RadarModel {
// public:
//     RadarModel();
//     void setConfiguration(const RadarAttributes& attrs);
//     void update(double dt, const std::vector<TargetInput>& worldTargets, double currentTime);
//     // const std::vector<DetectionOutput>& getActiveDetections() const;
//     std::vector<DetectionOutput> getActiveDetections();
//     std::vector<TrackFile> getActiveTracks();
//     RadarAttributes getConfiguration() const { return config; }
//     double getCurrentAzimuth() const { return currentAzimuth; }
//     void reset();
//     double resolveRangeAmbiguity(double measuredRange,
//                                  double predictedRange,
//                                  double Rmax);
// RadarMode getMode() const { return config.mode; }
// private:
//     std::mutex radarMutex;

//     RadarAttributes config;
//     double currentAzimuth;
//     double currentElevation;
//     std::vector<DetectionOutput> lastDetections;
//     std::vector<TrackFile> trackDatabase;
//     void updateTWS(double dt, double currentTime); // The new private helper function
//     // Core Logic Functions
//     bool checkHorizon(double range, double targetZ);
//     bool checkBeamIntersection(double targetAz, double targetEl);
//     double calculateSignalStrength(double range, double rcs);
//     double computeCFARThreshold(
//         const std::vector<double>& referenceCells);
//     double computeAssociationProbability(
//         double measurementRange,
//         double predictedRange,
//         double gateSize);
//     double scanDirection = 1.0;
// };

// // Interface for .so linkage
// extern "C" {
// RadarModel* createRadar();
// void destroyRadar(RadarModel* obj);
// }

// #endif
