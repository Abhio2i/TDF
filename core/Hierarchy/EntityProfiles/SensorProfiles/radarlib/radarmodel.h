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
#include <unordered_map>
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

// Per-PRF folded range measurement for multi-PRF ambiguity resolution
struct PRFMeasurement {
    int    prfIndex;      // which prfLevels[] index was active
    double foldedRange;   // measured (folded) range in metres
    double Rmax;          // max unambiguous range for this PRF
    double simTime;       // simulation time of this measurement
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
    // Multi-PRF ambiguity resolution buffer
    std::unordered_map<uint32_t, std::vector<PRFMeasurement>> prfBuffer_;

    // Attempt to resolve range ambiguity using stored multi-PRF measurements.
    // Returns resolved range > 0 on success, or -1.0 if not enough data yet.
    double resolveMultiPRF(uint32_t targetID);
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
