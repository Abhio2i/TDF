#pragma once
#ifndef RADARMODEL_AESA_H
#define RADARMODEL_AESA_H
// =============================================================================
// radarmodel_aesa.h  —  Public API for the AESA radar simulation model


// =============================================================================

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>           // std::normal_distribution — must be before class defs
#include <string>
#include <unordered_map>    // std::unordered_map — must be before class defs
#include <vector>

namespace aesa {

// =============================================================================
// §1  Enumerations
// =============================================================================

enum class RadarMode       { SURVEILLANCE, TWS, LOCK_ON };
enum class SurfaceType     { AIR, SEA, LAND };
enum class ModulationType  { NONE, LFM, NLFM, FMCW };
enum class PRFType         { FIXED, STAGGERED, JITTERED, SWITCHED };
enum class DetectionCategory { ALL, AIR_ONLY, SURFACE_ONLY };
enum class SidelobeMode    { NORMAL, LOW_SLL, ULTRA_LOW };

/// FIX-03  Jammer type
enum class JammerType      { NOISE, DRFM, STAND_OFF_NOISE };

/// FIX-07  Swerling target fluctuation model
enum class SwerlingCase    { CASE_0, CASE_I, CASE_II, CASE_III, CASE_IV };

/// FIX-06  PRF regime
enum class WaveformMode    { HPRF, MPRF, LPRF, AUTO };

/// FIX-04  IFF interrogation mode
enum class IFFMode         { OFF, MODE_3A, MODE_4, MODE_5 };

/// FIX-04  IFF response classification
enum class IFFResponseCode { NO_REPLY, FRIENDLY, UNKNOWN, HOSTILE, CORRUPTED };

// =============================================================================
// §2  Sub-structs
// =============================================================================
// ============================================================================
// Occlusion result — three-state shadow model
// Ref: ITU-R P.526-15 knife-edge diffraction
//      Ruck et al Radar Cross Section Handbook Ch 5 — shadow boundaries
// ============================================================================
struct OcclusionResult {
    enum class Zone { LIT, PENUMBRA, SHADOW };
    Zone   zone              = Zone::LIT;
    double powerReduction    = 1.0;   // linear, 1.0 = full power, 0.0 = fully blocked
    double diffractionLoss_dB = 0.0;  // additional path loss due to diffraction
};
struct NullSteering {
    bool   active        = false;
    double azimuth_deg   = 0.0;
    double elevation_deg = 0.0;
    float  nullDepth_dB  = -30.0f;
};

struct BeamWaveform {
    ModulationType modulation     = ModulationType::LFM;
    float          pulseWidth_s   = 50e-6f;
    float          prf_Hz         = 300.0f;
    float          bandwidth_Hz   = 5e6f;
    int            pulsesPerDwell = 10;
    WaveformMode   mode           = WaveformMode::AUTO;
    float          prf2_Hz        = 0.0f;   // second PRF for staggered mode; 0 = disabled

};

/// FIX-06  One entry in the range-keyed waveform selection table
struct WaveformEntry {
    float        maxRange_m = 0.0f;
    BeamWaveform waveform;
};

struct BeamRequest {
    enum class Task { SEARCH, TRACK, FIRE_CONTROL, HORIZON_SEARCH };

    Task         task            = Task::SEARCH;
    double       azimuth_deg    = 0.0;
    double       elevation_deg  = 0.0;
    double       dwellTime_ms   = 2.0;
    uint32_t     targetID       = 0;
    int          priority       = 0;
    BeamWaveform waveform;
    float        spoilFactor    = 1.0f; // FIX-13
};

// =============================================================================
// §3  Noise / jammer structs
// =============================================================================

struct NoiseModel {
    double rangeStdDev     = 30.0;
    double azimuthStdDev   = 0.1;
    double elevationStdDev = 0.1;
    double dopplerStdDev   = 1.0;
};

struct JammerConfig {
    bool       active        = false;
    JammerType type          = JammerType::NOISE;  // FIX-03
    double     power_kW      = 0.0;
    double     gain_dBi      = 0.0;
    double     bandwidth_Hz  = 1e6;
    double     range_m       = 0.0;
    bool       selfScreening = false;

    // FIX-03  DRFM-specific
    float  drfmPullOffRate_m_s    = 150.0f;
    float  drfmVelocityOffset_m_s = 50.0f;
    bool   gateStealingActive     = false;
    double pullOffDistance_m      = 0.0;
    // ADD THESE — RGPO/VGPO
    bool   rgpoActive          = false;   // Range Gate Pull-Off
    float  rgpoRate_m_s        = 200.0f;  // pull-off rate
    float  rgpoMaxOffset_m     = 5000.0f; // max pull-off distance
    bool   vgpoActive          = false;   // Velocity Gate Pull-Off
    float  vgpoRate_m_s2       = 50.0f;   // acceleration of false Doppler
    float  vgpoMaxOffset_m_s   = 300.0f;  // max velocity offset
    bool   noiseModulation     = false;   // amplitude noise on DRFM return
    float  jamStrobe_dB        = 0.0f;    // additional strobe power above noise
};

// =============================================================================
// §4  Chaff cloud  (FIX-10)
// =============================================================================

struct ChaffCloud {
    double   x = 0.0, y = 0.0, z = 0.0;
    double   radius_m    = 200.0;
    double   rcsTotal    = 1000.0;
    double   decayTime_s = 60.0;
    double   birthTime_s = 0.0;
    uint32_t sourceID    = 0;
};

// =============================================================================
// §5  IFF  (FIX-04)
// =============================================================================

struct IFFResult {
    IFFResponseCode response   = IFFResponseCode::NO_REPLY;
    uint32_t        squawk     = 0;
    double          confidence = 0.0;
    IFFMode         modeUsed   = IFFMode::OFF;
};
struct AtmosphericConditions {
    float temperature_C  = 15.0f;    // °C  — ISA standard = 15°C
    float humidity_pct   = 60.0f;    // %   — midlatitude standard = 60%
    float pressure_hPa   = 1013.25f; // hPa — sea-level standard
    float rainRate_mmph  = 0.0f;     // mm/h — 0=clear, 4=light, 16=moderate, 100=heavy
    float fogVisibility_m= 0.0f;     // m   — 0=clear, <1000=fog, <200=dense fog
};
// =============================================================================
// §6  RadarConfig
// =============================================================================

struct RadarConfig {
    // ---- Propagation --------------------------------------------------------
    double earthRadiusFactor  = 1.33;
    double atmosphericFactor  = 1.0;
   // double rainRate_mmph      = 0.0;
    //double fogVisibility_m    = 0.0;
    AtmosphericConditions atmosphere;   // all weather in here now

    // ---- Environment (clutter)
    float  seaState    = 2.0f;   // Douglas sea state 0-6; used by signal processor
    float  landClutter = 0.0f;   // 0=none, 1=heavy; used by signal processor

    // ---- AESA array ---------------------------------------------------------
    int   numElements           = 1000;
    float peakPowerPerElement_W = 10.0f;
    float moduleEfficiency      = 0.7f;
    int   failedModules         = 0;
    float maxDutyCycle          = 0.25f;  // FIX-08

    // ---- Antenna / beam -----------------------------------------------------
    double frequency_Hz          = 10.0e9;
    float  antennaGain           = 34.0f;
    double antennaBandwidth      = 100e6;
    float  beamWidth             = 2.0f;
    float  maxSteeringAngle_deg  = 60.0f;

    // ---- Sidelobe control ---------------------------------------------------
    SidelobeMode sidelobeMode   = SidelobeMode::NORMAL;
    float peakSidelobeLevel     = -40.0f;
    float avgSidelobeLevel      = -50.0f;
    float sidelobeBlanking_dB   = -15.0f; // FIX-11

    // ---- Null steering
    NullSteering nullSteering;

    // ---- FoV ----------------------------------------------------------------
    float minElevation = -10.0f;
    float maxElevation =  60.0f;
    float minAzimuth   = -60.0f;
    float maxAzimuth   =  60.0f;

    // ---- Per-task dwell times -----------------------------------------------
    float searchDwellTime_ms      = 2.0f;
    float trackDwellTime_ms       = 1.0f;
    float fireControlDwellTime_ms = 5.0f;

    // ---- Per-task default waveforms -----------------------------------------
    BeamWaveform searchWaveform      = { ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF };
    BeamWaveform trackWaveform       = { ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 10, WaveformMode::MPRF };
    BeamWaveform fireControlWaveform = { ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 20, WaveformMode::HPRF };

    // ---- FIX-06  Waveform table (sorted maxRange ascending, 0 = sentinel)
    WaveformEntry waveformTable[6] = {
        { 30000.0f,  { ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 20, WaveformMode::HPRF } },
        { 100000.0f, { ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 10, WaveformMode::MPRF } },
        { 400000.0f, { ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF } },
        { 0.0f, {} }, { 0.0f, {} }, { 0.0f, {} }
    };

    // ---- PRF ----------------------------------------------------------------
    float   prfLevels[4] = { 300.0f, 1000.0f, 3000.0f, 0.0f };
    PRFType prfType      = PRFType::FIXED;

    // ---- Frequency agility --------------------------------------------------
    bool  frequencyAgility  = true;
    float hopStartFrequency = 9.0e9f;
    float hopStopFrequency  = 11.0e9f;

    // ---- Receiver -----------------------------------------------------------
    double systemTemperature_K = 290.0;
    double noiseFigure_dB      = 4.0;
    double targetPfa           = 1e-6;

    // ---- Platform -----------------------------------------------------------
    double radarHeight        = 10000.0;
    double minDetectableRange = 100.0;
    float  platformSpeed_m_s  = 250.0f;   // FIX-01 clutter notch

    // ---- Target category ---------------------------------------------------
    DetectionCategory targetCategory = DetectionCategory::AIR_ONLY;

    // ---- Track lifecycle ---------------------------------------------------
    int    missedScansToDrop  = 3;
    double trackCoastSeconds  = 30.0;
    int    minHitsToValidate  = 2;
    double maxTrackSpeed      = 3000.0;
    double manoeuvreThreshold_m = 500.0;

    // ---- FIX-04  IFF -------------------------------------------------------
    IFFMode              interrogationMode = IFFMode::MODE_3A;
    std::vector<uint32_t> friendlySquawks;

    // ---- FIX-05  JPDA ------------------------------------------------------
    bool  useJPDA             = true;
    float jpdaFalseAlarmDensity = 1e-6f;

    // ---- Measurement noise -------------------------------------------------
    NoiseModel noise;

    // ---- Emitter identity --------------------------------------------------
    std::string emitterID   = "";
    uint32_t    emitterCode = 0;

    // ---- Mode --------------------------------------------------------------
    RadarMode mode           = RadarMode::TWS;
    uint32_t  lockedTargetID = 0;
};

// =============================================================================
// §7  RadarPose
// =============================================================================

struct RadarPose {
    double x = 0.0, y = 0.0, z = 0.0;
    float  roll = 0.0f, pitch = 0.0f, heading = 0.0f;
};
// ============================================================================
// Material type — controls surface attenuation factor
// Values calibrated against measured open-literature RCS data
// Ref: Knott, Shaeffer, Tuley "Radar Cross Section" 2nd Ed, Table 5.1
// ============================================================================
enum class TargetMaterialType {
    METAL,       //  0 dB reduction — bare aluminium/steel
    COMPOSITE,   // -3 dB reduction — carbon fibre airframe
    RAM,         // -15 dB reduction — radar absorbing material coating
    STEALTHY     // -25 dB reduction — full VLO treatment
};

// ============================================================================
// Shape type — controls surface coherence efficiency per face
// Accounts for curvature reducing specular return vs flat-plate ideal
// Ref: Ruck et al "Radar Cross Section Handbook", Plenum 1970, Ch 4
// ============================================================================
enum class TargetShapeType {
    BOX,        // flat sides + corner reflectors — ground vehicle, container
    AIRCRAFT,   // curved fuselage, blended wing edges
    SHIP,       // large flat superstructure
    MISSILE,    // cylindrical body + end caps
    GENERIC     // fallback
};

struct TargetDimensions {
    double length = 0.0;   // metres — forward axis
    double height = 0.0;   // metres — vertical axis
    double width  = 0.0;   // metres — lateral axis
    bool   valid  = false;
    TargetMaterialType material = TargetMaterialType::METAL;
    TargetShapeType    shape    = TargetShapeType::GENERIC;
};
// =============================================================================
// §8  TargetInput
// =============================================================================

struct TargetInput {
    uint32_t    id      = 0;
    double      x = 0.0, y = 0.0, z = 0.0;
    double      vx = 0.0, vy = 0.0, vz = 0.0;
    double      rcs     = 1.0;
    SurfaceType surface = SurfaceType::AIR;
    JammerConfig jammer;

    SwerlingCase swerlingCase = SwerlingCase::CASE_I; // FIX-07

    std::vector<std::pair<float,float>> rcsTable;  // aspect→RCS pairs
    std::string platformType = "GENERIC";          // "FIGHTER","BOMBER","UAV","MISSILE","SHIP"
    TargetDimensions dimensions;
    // FIX-04  IFF transponder
    bool     hasIFF    = false;
    uint32_t iffSquawk = 0;
    IFFMode  iffMode   = IFFMode::MODE_3A;
};

// =============================================================================
// §9  Signal intercept
// =============================================================================

struct SignalIntercept {
    uint32_t       targetID        = 0;
    double         frequency_Hz    = 0.0;
    double         pri_s           = 0.0;
    double         pulseWidth_s    = 0.0;
    double         signalLevel_dBW = 0.0;
    int            priCount = 0, pwCount = 0, freqCount = 0, signalDepth = 0;
    ModulationType modulation      = ModulationType::NONE;
    std::string    emitterID       = "";
};

struct SignalLibraryEntry {
    std::string    emitterID         = "";
    uint32_t       emitterCode       = 0;
    double         frequency_Hz      = 0.0;
    double         freqTolerance_Hz  = 1e6;
    double         pri_s             = 0.0;
    double         priTolerance_s    = 1e-5;
    double         pulseWidth_s      = 0.0;
    double         pwTolerance_s     = 1e-7;
    ModulationType modulation        = ModulationType::NONE;
    std::string    description       = "";
};

// =============================================================================
// §10  Output structs
// =============================================================================

struct DetectionOutput {
    uint32_t targetID      = 0;
    double range           = 0.0;
    double azimuth         = 0.0;
    double elevation       = 0.0;
    double snr             = 0.0;
    double radialVelocity  = 0.0;
    double cpa_distance    = 0.0;
    double time_to_cpa     = 0.0;
    double Pk              = 0.0;
    double heading         = 0.0;
    double speedOverGround = 0.0;
    double acceleration    = 0.0;
    double targetAspect    = 0.0;
    double azError_deg     = 0.0;  // FIX-02 monopulse
    double elError_deg     = 0.0;  // FIX-02
    bool   isAmbiguous     = false;
    bool   lockBroken      = false;
    bool   isDRFMGhost     = false; // FIX-03
    bool   inDopplerBlind  = false; // FIX-01
};

struct TrackOutput {
    uint32_t id = 0;
    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double range           = 0.0;
    double azimuth         = 0.0;
    double elevation       = 0.0;
    double radialVelocity  = 0.0;
    double speedOverGround = 0.0;
    double heading         = 0.0;
    double targetAspect    = 0.0;
    double cpa_distance    = 0.0;
    double time_to_cpa     = 0.0;
    double Pk              = 0.0;
    int    hitCount        = 0;
    int    scanMissCount   = 0;
    bool   isValidated     = false;
    bool   wasAmbiguous    = false;
    bool   isManoeuvring   = false;
    double trackQuality    = 0.0;
    bool   isDRFMSuspect   = false; // FIX-03
    bool   isExternalTrack = false; // FIX-12
    IFFResult iff;                  // FIX-04
};

struct RadarOutput {
    std::vector<DetectionOutput> detections;
    std::vector<TrackOutput>     tracks;
    std::vector<SignalIntercept> intercepts;

    double    currentAzimuth   = 0.0;
    double    currentElevation = 0.0;
    RadarMode mode             = RadarMode::SURVEILLANCE;
    bool      lockBroken       = false;
    double    displayRange_km  = 200.0;
    BeamRequest::Task currentTask = BeamRequest::Task::SEARCH;
    double    currentDutyCycle = 0.0; // FIX-08
};

// =============================================================================
// §11  TrackFile  — internal Kalman state
// =============================================================================

struct TrackFile {
    uint32_t id = 0;
    double x = 0.0, y = 0.0, z = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
    double range = 0.0, velocity = 0.0, predictedRange = 0.0;
    double lastSeenTime = 0.0, lastTrackBeamTime = 0.0;
    int    hitCount = 0;
    bool   isValidated = false, isUpdated = false, wasAmbiguous = false;
    int    missCount = 0, scanMissCount = 0;
    bool   updatedThisScan = false;
    bool   isManoeuvring   = false;
    bool   isDRFMSuspect   = false; // FIX-03
    bool   isExternalTrack = false; // FIX-12
    double innovationMagnitude = 0.0;
    double trackQuality    = 0.0;
    IFFResult iff;                  // FIX-04

    std::array<double, 6> X = {};
    double P[6][6] = {};
    double Q[6][6] = {};
    double R[3][3] = {};
    // ── IMM (2-model: CV-low-Q / CV-high-Q) ──────────────────────────────
    double imm_mu[2]        = { 0.7, 0.3 };   // model probabilities
    double imm_X[2][6]      = {};              // per-model state
    double imm_P[2][6][6]   = {};              // per-model covariance
    bool   immActive        = false;           // armed after minHitsToValidate
};

// =============================================================================
// §12  Forward declarations
// =============================================================================

class RadarSignalProcessor_AESA;
class RadarAntenna_AESA;
class RadarScheduler;
class RadarTracker_AESA;
class RadarSignalLibrary_AESA;

// =============================================================================
// §13  RadarModel_AESA
// =============================================================================

class RadarModel_AESA
{
public:
    RadarModel_AESA();
    ~RadarModel_AESA();

    // Lifecycle
    void init (const RadarConfig& cfg);
    void start();
    void update(double dt, const RadarPose& pose,
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
    double computeMaxDetectionRange(double rcs = 3.0) const;
    double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;

    // Signal library
    void loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries);

    // FIX-10  Chaff
    void addChaffCloud(const ChaffCloud& cloud);
    void clearChaffClouds();

    // FIX-12  External track injection (Link-16 / CEC)
    void injectExternalTrack(const TrackOutput& ext);

private:
    RadarPose  currentPose_;
    double     currentSimTime_ = 0.0;
    bool firstScanComplete_ = false;
    mutable std::mutex mutex_;
    RadarConfig config_;

    std::unique_ptr<RadarSignalProcessor_AESA> signal_;
    std::unique_ptr<RadarAntenna_AESA>         antenna_;
    std::unique_ptr<RadarScheduler>            scheduler_;
    std::unique_ptr<RadarTracker_AESA>         tracker_;
    std::unique_ptr<RadarSignalLibrary_AESA>   library_;

    bool initialised_ = false, running_ = false;

    mutable double cachedDisplayRange_km_ = 200.0;
    mutable bool   displayRangeDirty_     = true;

    RadarOutput latestOutput_;

    std::vector<ChaffCloud>              chaffClouds_;    // FIX-10
    std::unordered_map<uint32_t, double> drfmPullOff_;    // FIX-03 accumulated pull-off per target

    // Internal pipeline helpers — signatures match implementations exactly
    bool processTargetDetection(
        const TargetInput& target,
        const BeamRequest& beam,
        double dt, double simTime,
        double maxUnambiguousRange,
        double maxUnambiguousRange2,
        std::vector<DetectionOutput>& scanDetections,
        std::normal_distribution<double>& rangeNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dopplerNoise);

    void injectDRFMGhost(
        const TargetInput& real,
        const BeamRequest& beam,
        double simTime,
        double maxUnambiguousRange,
        std::vector<DetectionOutput>& scanDetections,
        std::normal_distribution<double>& rangeNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dopplerNoise);

    // FIX-04  IFF per-track query
    IFFResult queryIFF(const TrackFile& track,
                       const std::vector<TargetInput>& worldInputs) const;

    double computeMaxDetectionRange_locked(double rcs = 3.0) const;
    void   rebuildSchedule();
    void   applyAttitudeToBeam(double bodyAz, double bodyEl,
                               double& worldAz, double& worldEl) const;
    std::vector<DetectionOutput> scanDetectionCache_;
    //std::vector<TrackOutput>     trackOutputCache_;   // ← ADD THIS
    int lockMissCount_ = 0;
    // ADD to private members:
    std::unordered_map<uint32_t, double> rgpoPullOff_;   // range pull-off per target
    std::unordered_map<uint32_t, double> vgpoPullOff_;   // velocity pull-off per target
    void injectRGPOVGPO(
        const TargetInput& real, const BeamRequest& beam,
        double simTime, double dt, double maxUnambiguousRange,
        std::vector<DetectionOutput>& scanDets,
        std::normal_distribution<double>& rNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dvNoise);
    std::vector<TargetInput> currentWorldInputs_;

    OcclusionResult computeOcclusion(
        const TargetInput& candidate,
        const std::vector<TargetInput>& allTargets,
        const RadarConfig& cfg) const;

    double computeKnifeEdgeDiffraction(double nu) const;
};

} // namespace aesa

// =============================================================================
// §14  C ABI
// =============================================================================

extern "C"
{
aesa::RadarModel_AESA* aesaradar_create();
void                   aesaradar_destroy(aesa::RadarModel_AESA* p);
}

#endif // RADARMODEL_AESA_H
