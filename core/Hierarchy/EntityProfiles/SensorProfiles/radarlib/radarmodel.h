#pragma once
// =============================================================================
// radarmodel.h  —  Public API for the radar simulation model
//
// THIS IS THE ONLY HEADER THAT radar.cpp (THE BRIDGE) EVER INCLUDES.
// The internal split into RadarSignalProcessor / RadarAntenna / RadarTracker
// / RadarSignalLibrary is an implementation detail — nothing outside this
// file needs to know.
//
// Design contract (unchanged from original):
//   • Zero Qt / engine dependencies.  Only std C++17.
//   • Lifecycle:  init() → start() → update() [loop] → end()
//   • All configuration comes in through RadarConfig.
//   • Per-tick sensor pose comes in through RadarPose.
//   • Per-tick world state comes in through std::vector<TargetInput>.
//   • Everything the engine/display needs comes out through RadarOutput.
//   • The model owns no Qt types, no global state, no singletons.
//   • All public methods are thread-safe (internal mutex).
//
// Change log vs previous version:
//   + ModulationType, PRFType, DetectionCategory enums
//   + ScanType extended: RASTER, HELICAL, LISSAJOUS
//   + RadarConfig: hopStopFrequency, modulation, prfType, targetCategory,
//                  scanDwellTime[2], emitterID, emitterCode
//   + SignalIntercept struct  (signal-library output per emitter)
//   + SignalLibraryEntry struct (loaded emitter reference table)
//   + RadarOutput::intercepts field
//   + RadarSignalLibrary forward declaration + library_ member
//   + activePRFIndex_ in RadarModel private state
//   All existing fields, structs, and methods are UNCHANGED.
// =============================================================================

#ifndef RADARMODEL_H
#define RADARMODEL_H

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

// =============================================================================
// §1  Enumerations
// =============================================================================

/// High-level operational mode of the radar.
enum class RadarMode
{
    SURVEILLANCE,   ///< Wide-area search
    TWS,            ///< Track-While-Scan
    LOCK_ON         ///< Single-target fire-control
};

/// Surface type under/around the target — drives clutter model.
enum class SurfaceType
{
    AIR,
    SEA,
    LAND
};

/// Antenna scan mechanism.
enum class ScanType
{
    MECHANICAL,   ///< Constant-rate azimuth sector scan
    CONICAL,      ///< Conical / nodding scan (sinusoidal elevation)
    RASTER,       ///< Bar-by-bar raster: azimuth sweep + elevation step per bar
    HELICAL,      ///< Continuous helical: az rotates, el ramps slowly
    LISSAJOUS     ///< Lissajous figure: az + el at different sinusoidal rates
};

/// Transmit waveform modulation — drives processing-gain model in SINR.
enum class ModulationType
{
    NONE,   ///< Unmodulated pulse (no pulse compression)
    LFM,    ///< Linear frequency modulation (up-chirp / down-chirp)
    NLFM,   ///< Non-linear FM (Taylor-weighted, low sidelobe)
    CW,     ///< Continuous-wave (Doppler-only, no range resolution)
    FMCW    ///< Frequency-modulated CW (range + Doppler)
};

/// PRF schedule type — controls which prfLevels[] entries are active.
enum class PRFType
{
    FIXED,      ///< Only prfLevels[0] used
    STAGGERED,  ///< Cycles through all non-zero prfLevels[] each pulse
    JITTERED,   ///< Random variation ±5 % around prfLevels[0]
    SWITCHED    ///< Operator/mode-controlled switch between levels
};

/// Target category filter — restricts detections to a surface class.
enum class DetectionCategory
{
    ALL,            ///< Detect all surface types (default)
    AIR_ONLY,       ///< Suppress SurfaceType::SEA and LAND detections
    SURFACE_ONLY    ///< Suppress SurfaceType::AIR detections
};

// =============================================================================
// §2  RadarConfig  —  static (or slow-changing) radar parameters
// =============================================================================

/// Per-axis measurement noise standard deviations.
struct NoiseModel
{
    double rangeStdDev     = 0.0;
    double azimuthStdDev   = 0.0;
    double elevationStdDev = 0.0;
    double dopplerStdDev   = 0.0;
};

/// Electronic countermeasure — jammer carried by a target or stand-off.
struct JammerConfig
{
    bool   active         = false;
    double power_kW       = 0.0;
    double gain_dBi       = 0.0;
    double bandwidth_Hz   = 1e6;
    double range_m        = 0.0;
    bool   selfScreening  = false;
};

/// Master radar parameter block.
struct RadarConfig
{
    // ---- Propagation / atmosphere ----------------------------------------
    double earthRadiusFactor   = 1.33;
    double atmosphericFactor   = 1.0;
    double rainRate_mmph       = 0.0;
    double fogVisibility_m     = 0.0;

    // ---- Transmitter / antenna -------------------------------------------
    double emissionPower_kW    = 100.0;
    double frequency_Hz        = 3e9;
    float  antennaGain         = 30.0f;  ///< dBi
    double antennaBandwidth    = 1e6;    ///< Hz
    float  beamWidth           = 3.0f;   ///< degrees
    float  peakSidelobeLevel   = -25.0f; ///< dB relative to main beam
    float  avgSidelobeLevel    = -35.0f; ///< dB relative to main beam
    float  pulseWidth          = 1e-6f;  ///< seconds

    // ---- PRF / waveform --------------------------------------------------
    float prfLevels[4]         = { 5000, 0, 0, 0 }; ///< Hz; all 4 used when prfType != FIXED
    PRFType prfType            = PRFType::FIXED;
    bool  frequencyAgility     = false;
    float hopStepFrequency     = 0.0f;   ///< Hz step size per hop
    float hopStartFrequency    = 0.0f;   ///< Hz lower bound of hop range
    float hopStopFrequency     = 0.0f;   ///< Hz upper bound of hop range
    float hopRate              = 0.0f;   ///< hops per second

    // ---- Waveform modulation ---------------------------------------------
    ModulationType modulation  = ModulationType::NONE;

    // ---- Scan geometry ---------------------------------------------------
    float    minElevation      = -10.0f;
    float    maxElevation      =  30.0f;
    float    minAzimuth        = -180.0f;
    float    maxAzimuth        =  180.0f;
    float    scanningRate_RPM  =   6.0f;
    ScanType scanType          = ScanType::MECHANICAL;
    float    scanDwellTime[2]  = { 0.0f, 0.0f }; ///< ms dwell at [0]=min-az [1]=max-az endpoints

    // ---- Receiver / detection --------------------------------------------
    double systemTemperature_K = 290.0;
    double noiseFigure_dB      =   5.0;
    double targetPfa           =   1e-6;

    // ---- Clutter ---------------------------------------------------------
    double seaState            = 0.0;
    double landClutter         = 0.5;

    // ---- Platform geometry -----------------------------------------------
    double radarHeight         = 20.0;
    double minDetectableRange  = 30.0;

    // ---- Target category filter ------------------------------------------
    DetectionCategory targetCategory = DetectionCategory::ALL;

    // ---- Track lifecycle -------------------------------------------------
    int    missedScansToDrop   = 5;
    double trackCoastSeconds   = 60.0;
    int    minHitsToValidate   = 3;
    double maxTrackSpeed       = 3000.0;

    // ---- Measurement noise -----------------------------------------------
    NoiseModel  noise;



    // ---- Emitter identity -----------------------------------------------
    std::string emitterID      = "";   ///< Radar's own emitter designation
    uint32_t    emitterCode    = 0;    ///< Numeric emitter code

    // ---- Operational mode ------------------------------------------------
    RadarMode mode             = RadarMode::SURVEILLANCE;
    uint32_t  lockedTargetID   = 0;
};

// =============================================================================
// §3  RadarPose  —  platform position + attitude
// =============================================================================

struct RadarPose
{
    double x = 0.0; ///< East  (metres)
    double y = 0.0; ///< Up    (metres, altitude)
    double z = 0.0; ///< North (metres)
    float roll    = 0.0f;
    float pitch   = 0.0f;
    float heading = 0.0f;
};

// =============================================================================
// §4  TargetInput  —  one entry per potentially detectable object per tick
// =============================================================================

struct TargetInput
{
    uint32_t    id      = 0;
    double      x       = 0.0;
    double      y       = 0.0;
    double      z       = 0.0;
    double      vx      = 0.0;
    double      vy      = 0.0;
    double      vz      = 0.0;
    double      rcs     = 1.0;
    SurfaceType surface = SurfaceType::AIR;
    JammerConfig jammer;
};

// =============================================================================
// §5  Signal intercept types  —  emitter intelligence output
// =============================================================================

/// Measured signal parameters accumulated per-emitter across detections.
/// Populated by RadarSignalLibrary; exposed through RadarOutput::intercepts.
struct SignalIntercept
{
    uint32_t       targetID        = 0;

    // Accumulated measurements (running averages)
    double         frequency_Hz    = 0.0;   ///< Measured carrier frequency
    double         pri_s           = 0.0;   ///< Measured pulse repetition interval (s)
    double         pulseWidth_s    = 0.0;   ///< Measured pulse width (s)
    double         signalLevel_dBW = 0.0;   ///< Received level (dBW, averaged)

    // Hit counters
    int            priCount        = 0;     ///< Number of PRI measurements
    int            pwCount         = 0;     ///< Number of PW measurements
    int            freqCount       = 0;     ///< Number of frequency measurements
    int            signalDepth     = 0;     ///< Consecutive detections above threshold

    ModulationType modulation      = ModulationType::NONE;
    std::string    emitterID       = "";    ///< Library match (empty = unknown)
};

/// One entry in the emitter reference library.
/// Loaded into RadarSignalLibrary; matched against live SignalIntercept data.
struct SignalLibraryEntry
{
    std::string    emitterID         = "";
    uint32_t       emitterCode       = 0;

    double         frequency_Hz      = 0.0;
    double         freqTolerance_Hz  = 1e6;   ///< ± match window

    double         pri_s             = 0.0;
    double         priTolerance_s    = 1e-5;

    double         pulseWidth_s      = 0.0;
    double         pwTolerance_s     = 1e-7;

    ModulationType modulation        = ModulationType::NONE;
    std::string    description       = "";
};

// =============================================================================
// §6  RadarOutput  —  everything the engine/display needs per tick
// =============================================================================

struct DetectionOutput
{
    uint32_t targetID       = 0;
    double range            = 0.0;
    double azimuth          = 0.0;
    double elevation        = 0.0;
    double snr              = 0.0;
    double radialVelocity   = 0.0;
    double cpa_distance     = 0.0;
    double time_to_cpa      = 0.0;
    double Pk               = 0.0;
    double heading          = 0.0;
    double speedOverGround  = 0.0;
    double acceleration     = 0.0;
    double targetAspect     = 0.0;
    bool   isAmbiguous      = false;
    bool   lockBroken       = false;
};

struct TrackOutput
{
    uint32_t id             = 0;
    double x  = 0.0, y  = 0.0, z  = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double range            = 0.0;
    double azimuth          = 0.0;
    double elevation        = 0.0;
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

struct RadarOutput
{
    std::vector<DetectionOutput> detections;
    std::vector<TrackOutput>     tracks;

    // Signal intercept records — one per detected emitter
    std::vector<SignalIntercept> intercepts;

    double currentAzimuth   = 0.0;
    double currentElevation = 0.0;
    RadarMode mode          = RadarMode::SURVEILLANCE;
    bool      lockBroken    = false;
    double displayRange_km  = 100.0;
};

// =============================================================================
// §7  TrackFile  —  internal Kalman track state (unchanged)
// =============================================================================

struct TrackFile
{
    uint32_t id = 0;
    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double range          = 0.0;
    double velocity       = 0.0;
    double predictedRange = 0.0;
    double lastSeenTime   = 0.0;
    int    hitCount       = 0;
    bool isValidated      = false;
    bool isUpdated        = false;
    bool wasAmbiguous     = false;
    int  missCount        = 0;
    int  scanMissCount    = 0;
    bool updatedThisScan  = false;
    std::array<double, 6> X = {};
    double P[6][6] = {};
    double Q[6][6] = {};
    double R[3][3] = {};
};

// =============================================================================
// §8  RadarModel  —  public simulation model class
// =============================================================================

class RadarSignalProcessor;
class RadarAntenna;
class RadarTracker;
class RadarSignalLibrary;      // ← new 4th sub-object (invisible to radar.cpp)

class RadarModel
{
public:
    RadarModel();
    ~RadarModel();

    // Lifecycle
    void init(const RadarConfig& cfg);
    void start();
    void update(double dt,
                const RadarPose& pose,
                const std::vector<TargetInput>& worldInputs,
                double simTime);
    void end();
    void reset();

    // Configuration
    void        setConfig(const RadarConfig& cfg);
    RadarConfig getConfig() const;

    // Mode control
    void setMode  (RadarMode mode);
    void lockOn   (uint32_t targetID);
    void breakLock();

    // Output
    RadarOutput getOutput() const;

    // Utility
    double computeMaxDetectionRange(double rcs = 10.0) const;
    double resolveRangeAmbiguity(double measuredRange,
                                 double predictedRange,
                                 double Rmax) const;

    // Signal library — load before mission start
    void loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries);

private:
    RadarPose currentPose_;   // ← stored each tick so processTargetDetection() can use it

    mutable std::mutex mutex_;
    RadarConfig        config_;

    // Sub-objects (pimpl)
    std::unique_ptr<RadarSignalProcessor> signal_;
    std::unique_ptr<RadarAntenna>         antenna_;
    std::unique_ptr<RadarTracker>         tracker_;
    std::unique_ptr<RadarSignalLibrary>   library_;   // ← new

    // Simulation state
    bool   initialised_      = false;
    bool   running_          = false;
    int    activePRFIndex_   = 0;    // cycles through prfLevels[] for STAGGERED/SWITCHED

    mutable double cachedDisplayRange_km_ = 100.0;
    mutable bool   displayRangeDirty_     = true;

    RadarOutput latestOutput_;

    // Private helpers (all called with mutex_ held)
    bool processTargetDetection(
        const TargetInput&               target,
        double                           dt,
        double                           simTime,
        double                           maxUnambiguousRange,
        std::vector<DetectionOutput>&    scanDetections,
        std::normal_distribution<double>& rangeNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dopplerNoise);

    double computeMaxDetectionRange_locked(double rcs = 10.0) const;
    void   assembleFinalOutput();
    double selectActivePRF();   // returns Hz; advances activePRFIndex_ if needed
    /// Convert body-frame beam az/el to world frame using platform attitude.
    /// heading/pitch/roll come from currentPose_.
    void applyAttitudeToBeam(double bodyAz, double bodyEl,
                             double& worldAz, double& worldEl) const;
};

// =============================================================================
// §9  C ABI
// =============================================================================
extern "C"
{
RadarModel* radarmodel_create();
void        radarmodel_destroy(RadarModel* p);
}

#endif // RADARMODEL_H
// #pragma once
// // =============================================================================
// // radarmodel.h  —  Public API for the radar simulation model
// //
// // THIS IS THE ONLY HEADER THAT radar.cpp (THE BRIDGE) EVER INCLUDES.
// // The internal split into RadarSignalProcessor / RadarAntenna / RadarTracker
// // is an implementation detail — nothing outside this file needs to know.
// //
// // Design contract (unchanged from original):
// //   • Zero Qt / engine dependencies.  Only std C++17.
// //   • Lifecycle:  init() → start() → update() [loop] → end()
// //   • All configuration comes in through RadarConfig  (set once at init,
// //     or via setConfig() for hot-reload).
// //   • Per-tick sensor pose comes in through RadarPose.
// //   • Per-tick world state comes in through std::vector<TargetInput>.
// //   • Everything the engine/display needs comes out through RadarOutput.
// //   • The model owns no Qt types, no global state, no singletons.
// //   • All public methods are thread-safe (internal mutex).
// //   • Safe to move to a dedicated thread — bridge calls update() from
// //     whatever thread it likes; getOutput() is always guarded.
// //
// // Refactor note (internal only):
// //   radarmodel.cpp now delegates to three sub-objects:
// //     RadarSignalProcessor  — pure physics / signal chain (stateless)
// //     RadarAntenna          — scan state machine
// //     RadarTracker          — Kalman track database
// //   None of those types appear in this header.  The public API is identical.
// // =============================================================================

// #ifndef RADARMODEL_H
// #define RADARMODEL_H

// #include <array>
// #include <cmath>
// #include <cstdint>
// #include <memory>   // std::unique_ptr — required for sub-object members
// #include <mutex>
// #include <random>
// #include <string>
// #include <vector>
// //#include "jammingconfig.h"
// // =============================================================================
// // §1  Enumerations
// // =============================================================================

// /// High-level operational mode of the radar.
// enum class RadarMode
// {
//     SURVEILLANCE,   ///< Wide-area search — rotates beam, reports raw detections
//     TWS,            ///< Track-While-Scan — maintains Kalman track database
//     LOCK_ON         ///< Single-target fire-control — beam slaves to locked target
// };

// /// Surface type under/around the target — drives clutter model selection.
// enum class SurfaceType
// {
//     AIR,
//     SEA,
//     LAND
// };

// /// Antenna scan mechanism — affects beam dwell and elevation profile.
// enum class ScanType
// {
//     MECHANICAL,   ///< Constant-rate azimuth rotation
//     CONICAL       ///< Conical / nodding scan
// };

// // =============================================================================
// // §2  RadarConfig  —  static (or slow-changing) radar parameters
// //
// // Set once via init(), updated at runtime via setConfig().
// // The model copies this struct internally; callers own their copy.
// // Sub-objects receive a const ref to the master copy — never a separate copy.
// // =============================================================================

// /// Per-axis measurement noise standard deviations applied to detections.
// struct NoiseModel
// {
//     double rangeStdDev     = 0.0;   ///< metres
//     double azimuthStdDev   = 0.0;   ///< degrees
//     double elevationStdDev = 0.0;   ///< degrees
//     double dopplerStdDev   = 0.0;   ///< m/s
// };

// // Electronic countermeasure — jammer parameters.
// struct JammerConfig
// {
//     bool   active         = false;
//     double power_kW       = 0.0;
//     double gain_dBi       = 0.0;
//     double bandwidth_Hz   = 1e6;
//     double range_m        = 0.0;    ///< Stand-off range; ignored when selfScreening
//     bool   selfScreening  = false;  ///< True → jammer rides on the target itself
// };

// /// Master radar parameter block.  All sub-objects read from a const ref to
// /// the single copy stored in RadarModel::config_.
// struct RadarConfig
// {
//     // ---- Propagation / atmosphere ----------------------------------------
//     double earthRadiusFactor   = 1.33;  ///< 4/3 earth for standard refraction
//     double atmosphericFactor   = 1.0;   ///< >1 ducting, <1 sub-refraction
//     double rainRate_mmph       = 0.0;   ///< mm/hr  (0 = clear sky)
//     double fogVisibility_m     = 0.0;   ///< metres (0 = disabled)

//     // ---- Transmitter / antenna -------------------------------------------
//     double emissionPower_kW    = 100.0;
//     double frequency_Hz        = 3e9;
//     float  antennaGain         = 30.0f; ///< dBi
//     double antennaBandwidth    = 1e6;   ///< Hz
//     float  beamWidth           = 3.0f;  ///< degrees (one-way half-power)
//     float  peakSidelobeLevel   = -25.0f;
//     float  avgSidelobeLevel    = -35.0f;
//     float  pulseWidth          = 1e-6f; ///< seconds

//     // ---- PRF / waveform --------------------------------------------------
//     float prfLevels[4]         = { 5000, 0, 0, 0 }; ///< Hz; [0] is primary PRF
//     bool  frequencyAgility     = false;
//     float hopStepFrequency     = 0.0f;
//     float hopStartFrequency    = 0.0f;
//     float hopRate              = 0.0f;

//     // ---- Scan geometry ---------------------------------------------------
//     float    minElevation      = -10.0f;  ///< degrees
//     float    maxElevation      =  30.0f;
//     float    minAzimuth        = -180.0f;
//     float    maxAzimuth        =  180.0f;
//     float    scanningRate_RPM  =   6.0f;
//     ScanType scanType          = ScanType::MECHANICAL;

//     // ---- Receiver / detection --------------------------------------------
//     double systemTemperature_K = 290.0;
//     double noiseFigure_dB      =   5.0;
//     double targetPfa           =   1e-6; ///< False-alarm probability

//     // ---- Clutter ---------------------------------------------------------
//     double seaState            = 0.0;   ///< Beaufort scale
//     double landClutter         = 0.5;   ///< Normalised [0,1]

//     // ---- Platform geometry -----------------------------------------------
//     double radarHeight         = 20.0;  ///< metres above sea level
//     double minDetectableRange  = 30.0;  ///< metres (blind zone)

//     // ---- Track lifecycle -------------------------------------------------
//     int    missedScansToDrop   = 5;
//     double trackCoastSeconds   = 60.0;
//     int    minHitsToValidate   = 3;
//     double maxTrackSpeed       = 3000.0; ///< m/s — Kalman velocity clamp

//     // ---- Measurement noise -----------------------------------------------
//     NoiseModel  noise;

//     // ---- ECM -------------------------------------------------------------
//     JammerConfig jammer;

//     // ---- Operational mode (may be changed via setMode()) -----------------
//     RadarMode mode             = RadarMode::SURVEILLANCE;
//     uint32_t  lockedTargetID   = 0;     ///< Valid only in LOCK_ON mode
// };

// // =============================================================================
// // §3  RadarPose  —  platform position + attitude, updated every tick
// //
// // The bridge builds this from the engine's Transform and hands it to update().
// // The model uses it to:
// //   • Derive the correct radar-local coordinate frame
// //   • Update radarHeight for horizon calculations
// //   • (Future) apply roll/pitch corrections to beam pointing
// // =============================================================================

// struct RadarPose
// {
//     // World position in metres (bridge converts from engine coordinate system)
//     double x = 0.0; ///< East
//     double y = 0.0; ///< Up (altitude)
//     double z = 0.0; ///< North

//     // Attitude in degrees — reserved for future beam-pointing correction
//     float roll    = 0.0f;
//     float pitch   = 0.0f;
//     float heading = 0.0f;
// };

// // =============================================================================
// // §4  TargetInput  —  one entry per potentially detectable object, per tick
// //
// // Positions and velocities are in radar-local coordinates (metres, m/s).
// // The bridge is responsible for the world→local transform.
// // =============================================================================

// struct TargetInput
// {
//     uint32_t    id      = 0;
//     double      x       = 0.0;   ///< Local position relative to radar origin (m)
//     double      y       = 0.0;
//     double      z       = 0.0;
//     double      vx      = 0.0;   ///< Velocity in m/s (same local frame)
//     double      vy      = 0.0;
//     double      vz      = 0.0;
//     double      rcs     = 1.0;   ///< Radar cross-section in m²
//     SurfaceType surface = SurfaceType::AIR;
//     JammerConfig jammer;          ///< ECM carried by this specific target (selfScreening)
// };

// // =============================================================================
// // §5  RadarOutput  —  everything the engine/display needs per tick
// //
// // Written atomically by update() under the mutex.
// // Read by getOutput() which returns a full copy — callers never hold a ref.
// // =============================================================================

// /// Per-detection record — raw hit from the signal processor.
// struct DetectionOutput
// {
//     uint32_t targetID       = 0;

//     double range            = 0.0;   ///< metres
//     double azimuth          = 0.0;   ///< degrees (radar-local)
//     double elevation        = 0.0;   ///< degrees
//     double snr              = 0.0;   ///< SINR after clutter + noise + jamming

//     double radialVelocity   = 0.0;   ///< m/s (negative = closing)
//     double cpa_distance     = 0.0;   ///< Closest-point-of-approach, metres
//     double time_to_cpa      = 0.0;   ///< Seconds to CPA
//     double Pk               = 0.0;   ///< Probability-of-kill estimate

//     double heading          = 0.0;   ///< degrees true
//     double speedOverGround  = 0.0;   ///< m/s horizontal
//     double acceleration     = 0.0;
//     double targetAspect     = 0.0;   ///< degrees

//     bool   isAmbiguous      = false; ///< True if range was beyond PRF Rmax
//     bool   lockBroken       = false;
// };

// /// Per-track record — Kalman-filtered state from the tracker.
// struct TrackOutput
// {
//     uint32_t id             = 0;

//     double x  = 0.0, y  = 0.0, z  = 0.0;   ///< Estimated position (m)
//     double vx = 0.0, vy = 0.0, vz = 0.0;   ///< Estimated velocity (m/s)

//     double range            = 0.0;
//     double azimuth          = 0.0;   ///< degrees
//     double elevation        = 0.0;   ///< degrees
//     double radialVelocity   = 0.0;
//     double speedOverGround  = 0.0;
//     double heading          = 0.0;
//     double targetAspect     = 0.0;
//     double cpa_distance     = 0.0;
//     double time_to_cpa      = 0.0;
//     double Pk               = 0.0;

//     int    hitCount         = 0;
//     int    scanMissCount    = 0;
//     bool   isValidated      = false;
//     bool   wasAmbiguous     = false;
// };

// /// Top-level output bundle — bridge reads this atomically via getOutput().
// struct RadarOutput
// {
//     // Raw detections (SURVEILLANCE; also filled during TWS beam pass)
//     std::vector<DetectionOutput> detections;

//     // Validated + Kalman-filtered track list (TWS + LOCK_ON)
//     std::vector<TrackOutput>     tracks;

//     // Current antenna pointing (for display sweep-line rendering)
//     double currentAzimuth   = 0.0;   ///< degrees
//     double currentElevation = 0.0;   ///< degrees

//     // Operational state
//     RadarMode mode          = RadarMode::SURVEILLANCE;
//     bool      lockBroken    = false; ///< True if LOCK_ON target lost this tick

//     // Computed display range in km — recomputed when config changes
//     double displayRange_km  = 100.0;
// };

// // =============================================================================
// // §6  TrackFile  —  internal Kalman track state
// //
// // Declared here so diagnostics code in radar.cpp can inspect track internals
// // without reaching into radartracker.h.  Bridge code should prefer the public
// // TrackOutput type.  RadarTracker owns a vector of these.
// // =============================================================================

// struct TrackFile
// {
//     uint32_t id = 0;

//     // Current best estimate (mirrored from Kalman state vector X)
//     double x = 0.0, y = 0.0, z = 0.0;
//     double vx = 0.0, vy = 0.0, vz = 0.0;

//     double range          = 0.0;
//     double velocity       = 0.0;   ///< Radial velocity (m/s)
//     double predictedRange = 0.0;   ///< Kalman-predicted range for next association

//     double lastSeenTime   = 0.0;   ///< Simulation clock at last update (s)
//     int    hitCount       = 0;     ///< Total confirmed detections

//     bool isValidated      = false; ///< True once hitCount >= minHitsToValidate
//     bool isUpdated        = false; ///< True if updated this tick (cleared by predict)
//     bool wasAmbiguous     = false; ///< True if last detection was range-ambiguous

//     int  missCount        = 0;     ///< Cumulative miss count (diagnostics)
//     int  scanMissCount    = 0;     ///< Per-scan miss counter (used for dropout)
//     bool updatedThisScan  = false; ///< Reset at each scan boundary

//     // Kalman filter state
//     std::array<double, 6> X = {};  ///< State vector [x, y, z, vx, vy, vz]
//     double P[6][6] = {};           ///< Covariance matrix
//     double Q[6][6] = {};           ///< Process noise
//     double R[3][3] = {};           ///< Measurement noise
// };

// // =============================================================================
// // §7  RadarModel  —  the public simulation model class
// //
// // Internally delegates to:
// //   RadarSignalProcessor  (member: signal_)
// //   RadarAntenna          (member: antenna_)
// //   RadarTracker          (member: tracker_)
// //
// // The mutex_, config_, and latestOutput_ live here.
// // Sub-objects are called only from inside update() which already holds mutex_.
// // =============================================================================

// // Forward-declare sub-objects so radarmodel.h stays free of their headers.
// // Definitions are in radarmodel.cpp which includes the sub-object headers.
// class RadarSignalProcessor;
// class RadarAntenna;
// class RadarTracker;

// class RadarModel
// {
// public:
//     // -------------------------------------------------------------------------
//     // Construction / destruction
//     // -------------------------------------------------------------------------

//     RadarModel();
//     ~RadarModel();

//     // -------------------------------------------------------------------------
//     // Lifecycle
//     // -------------------------------------------------------------------------

//     /// Allocate internal state, apply config, zero all counters.
//     /// Must be called before start().
//     void init(const RadarConfig& cfg);

//     /// Arm the model: reset scan angle, clear track DB, start simClock at 0.
//     /// Safe to call again for a hot-restart without re-init.
//     void start();

//     /// Main per-tick entry point.
//     ///   dt          — seconds since last call
//     ///   pose        — current platform position + attitude
//     ///   worldInputs — all visible targets in radar-local coordinates
//     ///   simTime     — absolute simulation clock (seconds from mission start)
//     ///
//     /// Thread-safe: acquires internal mutex for the duration of the call.
//     void update(double                          dt,
//                 const RadarPose&                pose,
//                 const std::vector<TargetInput>& worldInputs,
//                 double                          simTime);

//     /// Graceful shutdown — flush tracks, mark outputs stale.
//     /// After end(), call init() + start() to reuse the object.
//     void end();

//     // -------------------------------------------------------------------------
//     // Configuration — hot-reload, thread-safe
//     // -------------------------------------------------------------------------

//     /// Replace the running configuration. Takes effect on the next update().
//     void setConfig(const RadarConfig& cfg);

//     /// Read back the current configuration (returns a copy — thread-safe).
//     RadarConfig getConfig() const;

//     // -------------------------------------------------------------------------
//     // Mode control — thread-safe convenience wrappers
//     // -------------------------------------------------------------------------

//     void setMode (RadarMode mode);
//     void lockOn  (uint32_t targetID);  ///< Sets mode = LOCK_ON + lockedTargetID
//     void breakLock();                  ///< Returns to SURVEILLANCE

//     // -------------------------------------------------------------------------
//     // Output — thread-safe snapshot
//     // -------------------------------------------------------------------------

//     /// Returns a complete copy of the latest computed output.
//     /// Safe to call from any thread; does not block the update thread for long.
//     RadarOutput getOutput() const;

//     // -------------------------------------------------------------------------
//     // Utility
//     // -------------------------------------------------------------------------

//     /// Compute theoretical maximum detection range for a given RCS (m²).
//     /// Thread-safe; uses current config.
//     double computeMaxDetectionRange(double rcs = 10.0) const;

//     /// Unfold a measured range given predicted range and PRF Rmax.
//     double resolveRangeAmbiguity(double measuredRange,
//                                  double predictedRange,
//                                  double Rmax) const;

//     /// Hard reset — equivalent to end() + init(current config) + start().
//     void reset();

// private:
//     // =========================================================================
//     // Internal mutex — guards config_, latestOutput_, and all sub-objects.
//     // Sub-objects have NO mutex of their own; they are only ever called from
//     // update() (or from private helpers called by update()), which already
//     // holds this lock.
//     // =========================================================================
//     mutable std::mutex mutex_;

//     // =========================================================================
//     // Configuration — single source of truth.
//     // Sub-objects always receive a const ref to this; they never store a copy.
//     // =========================================================================
//     RadarConfig config_;

//     // =========================================================================
//     // Sub-objects (pimpl-style forward declarations above)
//     // Owned via unique_ptr to keep their headers out of this public header.
//     // =========================================================================
//     std::unique_ptr<RadarSignalProcessor> signal_;
//     std::unique_ptr<RadarAntenna>         antenna_;
//     std::unique_ptr<RadarTracker>         tracker_;

//     // =========================================================================
//     // Simulation state
//     // =========================================================================
//     bool   initialised_ = false;
//     bool   running_     = false;

//     // Cached display range — recomputed when config changes (dirty flag)
//     mutable double cachedDisplayRange_km_ = 100.0;
//     mutable bool   displayRangeDirty_     = true;

//     // Output snapshot — written by update(), read by getOutput()
//     RadarOutput latestOutput_;

//     // =========================================================================
//     // Private helpers — all called from update() with mutex_ already held.
//     // These are the orchestration steps; physics is delegated to sub-objects.
//     // =========================================================================

//     /// Inner per-target detection pipeline — calls signal_ and tracker_.
//     /// Returns true if the target was detected this tick.
//     bool processTargetDetection(
//         const TargetInput&               target,
//         double                           dt,
//         double                           simTime,
//         double                           maxUnambiguousRange,
//         std::vector<DetectionOutput>&    scanDetections,
//         std::normal_distribution<double>& rangeNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dopplerNoise);

//     /// Internal unlocked wrapper — called from update() (mutex already held).
//     /// Avoids the deadlock that would occur if the public wrapper re-locked.
//     double computeMaxDetectionRange_locked(double rcs = 10.0) const;

//     /// Output assembly placeholder — retained for API parity with original
//     /// radarmodel.cpp.  Actual assembly is done inline at the end of update().
//     void assembleFinalOutput();
// };

// // =============================================================================
// // §8  C ABI — for dlopen / FFI usage
// // =============================================================================
// extern "C"
// {
// RadarModel* radarmodel_create();
// void        radarmodel_destroy(RadarModel* obj);
// }

// #endif // RADARMODEL_H
