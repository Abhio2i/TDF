// =============================================================================
// FILE:         aesaradar.cpp
// MODULE:       AESA Radar — Qt / Engine Bridge
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements AESARadar, the Qt/engine bridge that owns a
//               RadarModel_AESA and translates its output into the engine's
//               Sensor::targets list and Qt signals each simulation frame.
//
//               This file contains ONLY translation and coordination logic:
//               - RadarConfig construction calibrated to match Generic radar
//               - Pose and target data assembly from engine scene graph
//               - Mode-specific output translation (SURVEILLANCE / TWS / LOCK_ON)
//               - Serialisation (toJson / fromJson) for scenario save/load
//
//               All radar physics, JPDA tracking, Swerling RCS, Albersheim Pd,
//               Doppler notch, IFF, DRFM, chaff, beam spoiling and duty-cycle
//               enforcement are inside RadarModel_AESA (radarmodel_aesa.cpp).
//
// REQUIREMENTS: REQ-AESA-001  Lifecycle
//               REQ-AESA-002  Configuration serialisation
//               REQ-AESA-003  Mode control
//               REQ-AESA-004  Output assembly
//               REQ-AESA-020  Duty cycle reporting
//               REQ-AESA-027  External track injection
//               REQ-AESA-050  IFF result forwarding
//               REQ-AESA-060  DRFM ghost filtering
//               REQ-AESA-061  Chaff management
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
//   Rev 3  01 Apr 2026  Staggered PRF config wired in (9:10 ratio). IMM /
//                       JPDA disabled for legacy parity. Config calibrated
//                       to match Generic radar parameters.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Named constants replace all magic literals.
//                       Commented-out code removed per NS-05.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "aesaradar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <memory>
#include "core/Plugins/aesalogicregistry.h"
#ifdef AESA_REGRESSION_TEST
extern "C" void aesa_test_seed_model(unsigned seed);
extern "C" void aesa_test_seed_signal(unsigned seed);
#endif

// =============================================================================
// NAMED CONSTANTS
// All numeric literals used in bridge computations are declared here.
// Satisfies VI-08 (no magic numbers) and ensures single-point change.
// =============================================================================

namespace
{
// Degrees-to-radians conversion factor. Used in velocityFromHeadingSpeed.
// REQ-AESA-004.
static constexpr double DEG2RAD = M_PI / 180.0;

// Engine-to-metres coordinate scale factor. Engine units are km; model
// uses metres. Applied to all position components. REQ-AESA-004.
static constexpr double COORD_SCALE = 1000.0;

// Minimum dt clamp (seconds). Prevents near-zero dt on the first frame
// or after a frame stall. REQ-AESA-001.
static constexpr double MIN_DT = 1e-4;

// Maximum dt clamp (seconds). Prevents excessively large Kalman prediction
// steps after a simulation pause or scheduler hitch. REQ-AESA-001.
static constexpr double MAX_DT = 1.0;

// Default dt used on the very first scan() call before frameTimer_ has
// accumulated a real measurement (seconds). REQ-AESA-001.
static constexpr double FIRST_FRAME_DT = 0.05;

// Nominal default RCS for AESA air targets (m²). REQ-AESA-040.
static constexpr double DEFAULT_RCS = 5.0;

// Minimum platform displacement (engine units) required to update the
// computed heading. Below this threshold the heading is held constant
// to avoid numerical noise from floating-point position quantisation.
// REQ-AESA-004.
static constexpr float MIN_MOVE_THRESHOLD = 0.001f;

// Altitude threshold below which radarHeight is not updated from pose.y.
// Prevents zero or ground-level pose readings from corrupting the horizon
// computation. REQ-AESA-071.
static constexpr double MIN_RADAR_HEIGHT = 50.0;

// Minimum radarHeight change (metres) that triggers a setConfig() update.
// Prevents redundant config writes on every tick when altitude is stable.
// REQ-AESA-071.
static constexpr double HEIGHT_UPDATE_THRESHOLD = 1.0;

// Doppler notch warning threshold (m/s). Tracks with |radialVelocity| below
// this value are at risk of being lost in the clutter notch. REQ-AESA-040.
static constexpr double DOPPLER_NOTCH_THRESHOLD = 30.0;

// Display range clamp lower bound (km). REQ-AESA-004.
static constexpr float DISPLAY_RANGE_MIN_KM = 5.0f;

// Display range clamp upper bound (km). REQ-AESA-004.
static constexpr float DISPLAY_RANGE_MAX_KM = 1000.0f;

// km-to-metres conversion for range output (target radius field is in km).
static constexpr double RANGE_M_TO_KM = 1.0 / 1000.0;

// FNV-1a 32-bit offset basis. REQ-AESA-004.
static constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;

// FNV-1a 32-bit prime. REQ-AESA-004.
static constexpr uint32_t FNV_PRIME = 16777619u;

// Minimum valid radar ID — 0 is reserved for "no target". REQ-AESA-004.
static constexpr uint32_t MIN_RADAR_ID = 1u;

// Velocity estimate smoothing time constant (s). First-order filter on the
// finite-difference velocity; suppresses float-position quantisation noise.
// MODEL PARAMETER — default agreed for Phase 2c. REQ-AESA-004.
static constexpr double VEL_SMOOTH_TAU_S = 0.5;

} // anonymous namespace
static float computeERP_dBm(const aesa::RadarConfig& cfg)
{
    int numActive = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt_watts = static_cast<double>(numActive)
                      * cfg.peakPowerPerElement_W
                      * cfg.moduleEfficiency;
    double antennaGainLinear = std::pow(10.0, cfg.antennaGain / 10.0);
    double erp_watts = Pt_watts * antennaGainLinear;
    return static_cast<float>(10.0 * std::log10(std::max(erp_watts, 1e-12) * 1000.0));
}

// True (stabilised) az/el -> body azimuth for the display. REQ-AESA-004.
static double stabToBodyAz(const aesa::RadarPose& pose, double sAz, double sEl)
{
    double s[3], b[3], bAz, bEl;
    aesa::azElToVec(sAz, sEl, s);
    aesa::stabToBody(pose.bodyFromStab, s, b);
    aesa::vecToAzEl(b, bAz, bEl);
    return bAz;
}

// =============================================================================
// ANTENNA MOUNT ORIENTATION. REQ-AESA-010.
// Platform body: x = forward, y = right, z = up. The array is rotated by
// mount azimuth (clockwise from the nose) then tilted by mount elevation.
// Rows of M = array axes (forward, right, up) expressed in body axes, so
// arrayFromStab = M * bodyFromStab and body = M^T * array.
// =============================================================================
static void mountFromBody(double azDeg, double elDeg, double M[3][3])
{
    constexpr double D2R = 3.14159265358979323846 / 180.0;
    const double a  = azDeg * D2R, e = elDeg * D2R;
    const double ca = std::cos(a), sa = std::sin(a);
    const double ce = std::cos(e), se = std::sin(e);
    M[0][0] =  ce * ca;  M[0][1] =  ce * sa;  M[0][2] = se;    // array forward
    M[1][0] = -sa;       M[1][1] =  ca;       M[1][2] = 0.0;   // array right
    M[2][0] = -se * ca;  M[2][1] = -se * sa;  M[2][2] = ce;    // array up
}

// Array-frame az/el -> platform-body az/el (degrees). Exact pass-through for
// mount 0/0, so nose-mounted radars report bit-identical values.
static void arrayToPlatformAzEl(float mountAz, float mountEl,
                                double az, double el,
                                double& pAz, double& pEl)
{
    if (mountAz == 0.0f && mountEl == 0.0f) { pAz = az; pEl = el; return; }
    double M[3][3], a[3], b[3];
    mountFromBody(mountAz, mountEl, M);
    aesa::azElToVec(az, el, a);
    aesa::bodyToStab(M, a, b);   // b = M^T a  (array -> platform body)
    aesa::vecToAzEl(b, pAz, pEl);
}

// =============================================================================
// FUNCTION:    AESARadar::getCurrentAzimuth / getCurrentElevation
// DESCRIPTION: Beam direction relative to the platform body (model output is
//              in the array frame; converted with the mount orientation).
//              REQ-AESA-010.
// =============================================================================
double AESARadar::getCurrentAzimuth() const
{
    const aesa::RadarOutput o = radarCore_.getOutput();
    double az = 0.0, el = 0.0;
    arrayToPlatformAzEl(mountAzimuth_deg_, mountElevation_deg_,
                        o.currentAzimuth, o.currentElevation, az, el);
    return az;
}

double AESARadar::getCurrentElevation() const
{
    const aesa::RadarOutput o = radarCore_.getOutput();
    double az = 0.0, el = 0.0;
    arrayToPlatformAzEl(mountAzimuth_deg_, mountElevation_deg_,
                        o.currentAzimuth, o.currentElevation, az, el);
    return el;
}

// =============================================================================
// PLUGIN LOGIC STAGES (AESA plugin SDK). REQ-AESA-090.
// Translate between model types and SDK types and call the plugin hooks.
// Every call into plugin code is wrapped in a StageGuard (host callbacks are
// refused while it is active) and in try/catch: a plugin exception falls back
// to the built-in logic for that call and never reaches the engine.
// =============================================================================
namespace
{
struct AesaStageContext
{
    const AesaLogicSnapshot*                       hooks = nullptr;
    const std::unordered_map<uint32_t, Platform*>* idMap = nullptr;
    aesaplugin::RadarInfo                          radar;
};

int aesaModeToInt(aesa::RadarMode m)
{
    switch (m)
    {
    case aesa::RadarMode::SURVEILLANCE: return 0;
    case aesa::RadarMode::TWS:          return 1;
    case aesa::RadarMode::LOCK_ON:      return 2;
    }
    return 0;
}

aesaplugin::TargetGeometry stageGeometry(const AesaStageContext& c,
                                         const aesa::TargetInput& t,
                                         double range_m)
{
    aesaplugin::TargetGeometry g;
    g.targetId = t.id;
    if (c.idMap != nullptr)
    {
        auto it = c.idMap->find(t.id);
        if (it != c.idMap->end() && it->second != nullptr)
            g.targetEntityId = it->second->ID;
    }
    g.x = t.x;   g.y = t.y;   g.z = t.z;
    g.vx = t.vx; g.vy = t.vy; g.vz = t.vz;
    g.range_m = range_m;
    return g;
}

bool stageMeanRcs(void* ctx, const aesa::TargetInput& t, double range_m,
                  double frequency_Hz, double& meanRcs_m2)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->rcs) return false;

    aesaplugin::RcsQuery q;
    q.radar           = c->radar;
    q.target          = stageGeometry(*c, t, range_m);
    q.platformType    = t.platformType;
    q.length_m        = t.dimensions.length;
    q.width_m         = t.dimensions.width;
    q.height_m        = t.dimensions.height;
    q.dimensionsValid = t.dimensions.valid;
    q.frequency_Hz    = frequency_Hz;

    bool ok = false;
    {
        AesaLogicRegistry::StageGuard guard;
        try { ok = c->hooks->rcs(q, meanRcs_m2); } catch (...) { ok = false; }
    }
    if (ok && !(std::isfinite(meanRcs_m2) && meanRcs_m2 >= 0.0))
    {
        static bool warned = false;
        if (!warned) { warned = true; qWarning() << "[AESA plugin] invalid RCS value; built-in RCS used"; }
        return false;
    }
    return ok;
}

bool stageDetect(void* ctx, const aesa::TargetInput& t, double range_m,
                 const aesa::DetectionStageInput& in, bool& detected)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->detection) return false;

    aesaplugin::DetectionQuery q;
    q.radar              = c->radar;
    q.target             = stageGeometry(*c, t, range_m);
    q.sinr               = in.sinr;
    q.cfarThreshold      = in.cfarThreshold;
    q.radialVelocity_m_s = in.radialVelocity_m_s;
    q.mode               = aesaModeToInt(in.mode);
    q.builtInDetected    = in.builtInDetected;

    AesaLogicRegistry::StageGuard guard;
    try { return c->hooks->detection(q, detected); } catch (...) { return false; }
}

bool stagePropagation(void* ctx, double range_m, const aesa::RadarConfig& cfg,
                      double& lossFactor)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->propagation) return false;

    aesaplugin::PropagationQuery q;
    q.radar           = c->radar;
    q.range_m         = range_m;
    q.frequency_Hz    = cfg.frequency_Hz;
    q.temperature_C   = cfg.atmosphere.temperature_C;
    q.humidity_pct    = cfg.atmosphere.humidity_pct;
    q.pressure_hPa    = cfg.atmosphere.pressure_hPa;
    q.rainRate_mmph   = cfg.atmosphere.rainRate_mmph;
    q.fogVisibility_m = cfg.atmosphere.fogVisibility_m;

    bool ok = false;
    {
        AesaLogicRegistry::StageGuard guard;
        try { ok = c->hooks->propagation(q, lossFactor); } catch (...) { ok = false; }
    }
    if (ok && !(std::isfinite(lossFactor) && lossFactor > 0.0 && lossFactor <= 1.0))
    {
        static bool warned = false;
        if (!warned) { warned = true; qWarning() << "[AESA plugin] invalid propagation loss; built-in model used"; }
        return false;
    }
    return ok;
}
} // namespace

static const aesa::BeamWaveform& currentWaveformFor(const aesa::RadarConfig& cfg,
                                                    aesa::RadarMode mode)
{
    switch (mode) {
    case aesa::RadarMode::LOCK_ON: return cfg.fireControlWaveform;
    case aesa::RadarMode::TWS:     return cfg.trackWaveform;
    default:                       return cfg.searchWaveform;
    }
}
// =============================================================================
// CONSTRUCTOR
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::AESARadar
// (Full description in header)
// =============================================================================
AESARadar::AESARadar(Hierarchy* h) : Sensor(h)
{
    // Set sensor classification fields for the engine's sensor registry.
    // REQ-AESA-001.
    subType    = SubType::AESA;
    sensortype = Type::Active;

    aesa::RadarConfig cfg;

    // -------------------------------------------------------------------------
    // Array parameters — calibrated to match Generic radar total output power.
    // 1000 elements × 100 W × 0.70 efficiency = 70 kW peak transmitted power.
    // REQ-AESA-012.
    // -------------------------------------------------------------------------
    cfg.numElements           = 1000;
    cfg.peakPowerPerElement_W = 100.0f;   // 100 W/element — matches Generic 100 kW total
    cfg.moduleEfficiency      = 0.70f;
    cfg.failedModules         = 0;
    cfg.maxDutyCycle          = 0.50f;    // 50% — increased headroom vs default 25%

    // -------------------------------------------------------------------------
    // Antenna parameters — calibrated to match Generic radar exactly.
    // REQ-AESA-010, REQ-AESA-012.
    // -------------------------------------------------------------------------
    cfg.frequency_Hz         = 8.0e9;    // 8 GHz — matches Generic (default was 10 GHz)
    cfg.antennaGain          = 35.0f;    // dBi — matches Generic (default was 34 dBi)
    cfg.antennaBandwidth     = 1e6;      // Hz — matches Generic 1 MHz (default was 500 MHz)
    cfg.beamWidth            = 3.0f;     // degrees — matches Generic (default was 2.0)
    cfg.maxSteeringAngle_deg = 60.0f;

    // Sidelobe control — LOW_SLL with values calibrated to Generic sidelobe levels.
    // REQ-AESA-010.
    cfg.sidelobeMode        = aesa::SidelobeMode::LOW_SLL;
    cfg.peakSidelobeLevel   = -25.0f;   // dB — matches Generic (default was -45 dB)
    cfg.avgSidelobeLevel    = -35.0f;   // dB — matches Generic (default was -55 dB)
    cfg.sidelobeBlanking_dB = -15.0f;

    // -------------------------------------------------------------------------
    // Field of view — matches Generic exactly. REQ-AESA-011.
    // -------------------------------------------------------------------------
    cfg.minAzimuth   = -60.0f;  cfg.maxAzimuth   =  60.0f;
    cfg.minElevation =  -2.0f;  cfg.maxElevation =  15.0f;

    // -------------------------------------------------------------------------
    // Dwell times per beam task. REQ-AESA-020.
    // -------------------------------------------------------------------------
    cfg.searchDwellTime_ms      = 2.0f;
    cfg.trackDwellTime_ms       = 5.0f;
    cfg.fireControlDwellTime_ms = 5.0f;

    // -------------------------------------------------------------------------
    // Waveforms — simplified to match Generic pulse width and PRF.
    // pulsesPerDwell notes:
    //   search:       PRF 300 Hz  × dwell 50 ms  ≈ 15 pulses → using 10 (conservative)
    //   track/fire:   PRF 1000 Hz × dwell 10 ms  ≈ 10 pulses → using 25 (more dwells)
    // REQ-AESA-020.
    // -------------------------------------------------------------------------
    cfg.searchWaveform      = { aesa::ModulationType::LFM,  50e-6f, 300.0f,  5e6f, 10,
                          aesa::WaveformMode::LPRF };
    cfg.trackWaveform       = { aesa::ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25,
                         aesa::WaveformMode::MPRF };
    cfg.fireControlWaveform = { aesa::ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 25,
                               aesa::WaveformMode::HPRF };

    // Range-keyed waveform selection table — sorted ascending by maxRange_m.
    // Sentinel entries (maxRange_m = 0) mark end of active table. REQ-AESA-020.
    cfg.waveformTable[0] = {  30000.0f, { aesa::ModulationType::NLFM,  5e-6f, 2000.0f,
                                       50e6f, 25, aesa::WaveformMode::HPRF } };
    cfg.waveformTable[1] = { 100000.0f, { aesa::ModulationType::LFM,  10e-6f, 1000.0f,
                                        20e6f, 25, aesa::WaveformMode::MPRF } };
    cfg.waveformTable[2] = { 400000.0f, { aesa::ModulationType::LFM,  50e-6f,  300.0f,
                                        5e6f, 10, aesa::WaveformMode::LPRF } };
    cfg.waveformTable[3] = {};
    cfg.waveformTable[4] = {};
    cfg.waveformTable[5] = {};

    // -------------------------------------------------------------------------
    // Staggered PRF — 9:10 ratio gives LCM Rmax >> any real air target range.
    //   MPRF:  PRF1 = 1000 Hz (Rmax = 150 km), PRF2 = 1111 Hz (Rmax = 135 km)
    //          LCM unambiguous range ≈ 1350 km.
    //   LPRF:  PRF1 =  300 Hz (Rmax = 500 km), PRF2 =  333 Hz (Rmax = 450 km)
    //          LCM unambiguous range ≈ 4500 km.
    //   HPRF has no range ambiguity worth resolving (Rmax ≈ 75 km, velocity
    //   is unambiguous — prf2_Hz left at 0 for HPRF entries).
    // REQ-AESA-021.
    // -------------------------------------------------------------------------
    cfg.waveformTable[1].waveform.prf2_Hz = 1111.0f;
    cfg.waveformTable[2].waveform.prf2_Hz =  333.0f;
    cfg.trackWaveform.prf2_Hz             = 1111.0f;
    cfg.searchWaveform.prf2_Hz            =  333.0f;
    cfg.prfType = aesa::PRFType::STAGGERED;

    // -------------------------------------------------------------------------
    // Frequency agility — disabled to match Generic radar behaviour.
    // REQ-AESA-020.
    // -------------------------------------------------------------------------
    cfg.frequencyAgility  = false;
    cfg.hopStartFrequency = 0.0f;
    cfg.hopStopFrequency  = 0.0f;

    // -------------------------------------------------------------------------
    // Receiver parameters — calibrated to match Generic radar exactly.
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    cfg.systemTemperature_K = 290.0;
    cfg.noiseFigure_dB      = 5.0;       // dB — matches Generic (default was 4.0)
    cfg.targetPfa           = 1e-6;

    // -------------------------------------------------------------------------
    // Platform parameters — calibrated to match Generic radar exactly.
    // platformSpeed_m_s = 0 here; updated from dynamicModel each scan() tick.
    // REQ-AESA-071.
    // -------------------------------------------------------------------------
    cfg.radarHeight        = 20.0;       // metres — matches Generic (default was 8000)
    cfg.minDetectableRange = 30.0;       // metres — matches Generic (default was 100)
    cfg.platformSpeed_m_s  = 0.0f;       // m/s — zero until first scan() update
    cfg.earthRadiusFactor  = 1.33;
    cfg.atmosphericFactor  = 1.0;

    // -------------------------------------------------------------------------
    // Clutter — matches Generic radar (no sea or land clutter initially).
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    cfg.seaState    = 0.0f;
    cfg.landClutter = 0.0f;

    // -------------------------------------------------------------------------
    // Track lifecycle — calibrated to match Generic radar parameters.
    // REQ-AESA-030.
    // -------------------------------------------------------------------------
    cfg.targetCategory       = aesa::DetectionCategory::ALL;
    cfg.missedScansToDrop    = 2;
    cfg.trackCoastSeconds    = 8.0;
    cfg.minHitsToValidate    = 2;
    cfg.maxTrackSpeed        = 2000.0;   // m/s — matches Generic (default was 3000)
    cfg.manoeuvreThreshold_m = 500.0;

    // -------------------------------------------------------------------------
    // JPDA disabled for legacy parity with Generic radar. NN association used.
    // Can be re-enabled via setRadarConfig() or fromJson(). REQ-AESA-030.
    // -------------------------------------------------------------------------
    cfg.useJPDA               = false;
    cfg.jpdaFalseAlarmDensity = 1e-6f;

    // -------------------------------------------------------------------------
    // IFF — Mode 3/A civil aviation squawk. REQ-AESA-050.
    // -------------------------------------------------------------------------
    cfg.interrogationMode = aesa::IFFMode::MODE_3A;

    // -------------------------------------------------------------------------
    // Measurement noise — zero standard deviations match Generic ideal sensor.
    // Enables like-for-like comparison with the legacy radar model. REQ-AESA-040.
    // -------------------------------------------------------------------------
    cfg.noise.rangeStdDev     = 0.0;
    cfg.noise.azimuthStdDev   = 0.0;
    cfg.noise.elevationStdDev = 0.0;
    cfg.noise.dopplerStdDev   = 0.0;

    // -------------------------------------------------------------------------
    // Initial mode and atmospheric conditions. REQ-AESA-001, REQ-AESA-071.
    // -------------------------------------------------------------------------
    cfg.mode                       = aesa::RadarMode::TWS;
    cfg.platformSpeed_m_s          = 0.0f;
    cfg.atmosphere.temperature_C   = 30.0f;    // degrees C — warm climate default
    cfg.atmosphere.humidity_pct    = 30.0f;    // percent
    cfg.atmosphere.pressure_hPa    = 1013.25f; // hPa — sea level standard
    cfg.atmosphere.rainRate_mmph   = 0.0f;     // clear weather
    cfg.atmosphere.fogVisibility_m = 0.0f;     // clear weather

    // -------------------------------------------------------------------------
    // Initialise and start the model. reset() clears any pre-fix ghost tracks
    // that may have been created during internal subsystem initialisation.
    // REQ-AESA-001.
    // -------------------------------------------------------------------------
    radarCore_.init(cfg);
    radarCore_.start();
    radarCore_.reset();

    // Initialise the Sensor base class FoV limit from the config. REQ-AESA-011.
    maxDetectionAngle = cfg.maxAzimuth;
}
AESARadar::~AESARadar()
{
    if (parentEntity)
    {
        auto& data = getSensorData();
        data.sensor[dataid].range = 1;
    }
}
ECS::sensorComponent& AESARadar::getSensorData() const{
    return *parentEntity->root->world.GetComponent<ECS::sensorComponent>(parentEntity->ecsID);
}

// =============================================================================
// LOCK / BREAK LOCK
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::lockOn
// (Full description in header)
// =============================================================================
void AESARadar::lockOn(uint32_t radarTargetID)
{
    // Pure delegation to the model layer. All fire-control state management
    // is inside RadarModel_AESA. REQ-AESA-003.
    radarCore_.lockOn(radarTargetID);
}

// =============================================================================
// FUNCTION:    AESARadar::breakLock
// (Full description in header)
// =============================================================================
void AESARadar::breakLock()
{
    // Pure delegation to the model layer. REQ-AESA-003.
    radarCore_.breakLock();
}

// =============================================================================
// EXTERNAL TRACK INJECTION  (FIX-12)
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::injectExternalTrack
// (Full description in header)
// =============================================================================
void AESARadar::injectExternalTrack(const aesa::TrackOutput& ext)
{
    // Delegate injection to the model. The model's tracker validates the ID
    // and marks the track isExternalTrack = isValidated = true. REQ-AESA-027.
    radarCore_.injectExternalTrack(ext);

    // Notify UI / fire-control consumers that a new datalink track is available.
    emit externalTrackInjected(ext.id);
}

// =============================================================================
// STATIC UTILITIES
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::platformToRadarID
// (Full description in header)
// =============================================================================
uint32_t AESARadar::platformToRadarID(const std::string& key)
{
    // FNV-1a 32-bit hash — deterministic across all platforms and runs.
    // This is the de facto standard for non-cryptographic string hashing in
    // real-time simulation. Chosen for minimal collision probability over
    // typical scene-graph key lengths (4–32 characters). REQ-AESA-004.
    uint32_t hash = FNV_OFFSET_BASIS;
    for (unsigned char c : key)
    {
        hash ^= c;
        hash *= FNV_PRIME;
    }
    // 0 is reserved as "no target" — map zero-hash to 1. REQ-AESA-004.
    return hash == 0u ? MIN_RADAR_ID : hash;
}

// =============================================================================
// FUNCTION:    AESARadar::velocityFromHeadingSpeed
// (Full description in header)
// =============================================================================
void AESARadar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                         double& vx, double& vy, double& vz)
{
    // Decompose scalar speed into Cartesian x/y components using the platform
    // heading. vz = 0: no vertical velocity is estimable from heading/speed.
    // REQ-AESA-004.
    double rad = headingDeg * DEG2RAD;
    vx = speedMs * std::cos(rad);
    vy = speedMs * std::sin(rad);
    vz = 0.0;
}

// =============================================================================
// POSE BUILDER
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::buildPose
// (Full description in header)
// =============================================================================
aesa::RadarPose AESARadar::buildPose() const
{
    aesa::RadarPose pose;

    // Guard against uninitialised state — return zero pose if hierarchy or
    // parentEntity are not yet set. REQ-AESA-001.
    if (!root || !parentEntity) return pose;

    auto it = root->Platforms.find(parentEntity->ID);
    if (it == root->Platforms.end() /*|| !it->second->transform*/)
        return pose;

    // Extract world-space position from the platform's transform matrix.
    // Scale from engine units to metres via COORD_SCALE. REQ-AESA-010.
    QVector3D wpos = it->second->transform.matrix.translation();
    pose.x = static_cast<double>(wpos.x()) * COORD_SCALE;
    pose.y = static_cast<double>(wpos.y()) * COORD_SCALE;
    pose.z = static_cast<double>(wpos.z()) * COORD_SCALE;

    // Extract attitude from dynamicModel if available. REQ-AESA-010.
    // if (it->second->dynamicModel)
    // {
    pose.heading = it->second->dynamicModel.TrueHeading;
    pose.pitch   = it->second->dynamicModel.pitch;
    pose.roll    = it->second->dynamicModel.roll;
    // }
    return pose;
}

// =============================================================================
// TARGET COLLECTOR
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::collectTargets
// (Full description in header)
// =============================================================================
std::vector<aesa::TargetInput> AESARadar::collectTargets(
    const QVector3D&                         ownPosKm,
    double                                   velDt,
    std::unordered_map<uint32_t, Platform*>& outIdMap) const
{
    std::vector<aesa::TargetInput> inputs;
    outIdMap.clear();

    if (!root) return inputs;
    inputs.reserve(root->Platforms.size());

    for (auto& [key, entity] : root->Platforms)
    {
        // Skip own platform and null entries. REQ-AESA-004.
        if (!entity || key == parentEntity->ID) continue;

        Platform* platform = entity;
        const QVector3D wpos = platform->transform.matrix.translation();

        aesa::TargetInput t;

        // ---- Position: relative to radar, stabilised frame ------------------
        // Engine world: x = East, y = Up, z = North (km).
        // Model: x = North, y = East, z = Up (m). No rotation by own attitude;
        // the model converts to body angles where the antenna needs them.
        // REQ-AESA-004.
        t.x = (static_cast<double>(wpos.z()) - static_cast<double>(ownPosKm.z())) * COORD_SCALE;
        t.y = (static_cast<double>(wpos.x()) - static_cast<double>(ownPosKm.x())) * COORD_SCALE;
        t.z = (static_cast<double>(wpos.y()) - static_cast<double>(ownPosKm.y())) * COORD_SCALE;

        t.id = platformToRadarID(key);
        outIdMap[t.id] = platform;

        // ---- Velocity: absolute, stabilised frame (m/s) ---------------------
        // Measured from position change over simulation time; valid for
        // every entity type. REQ-AESA-004.
        const std::array<double, 3> vW = updateVelocityEstimate(t.id, wpos, velDt);
        t.vx = vW[2];   // North = world z
        t.vy = vW[0];   // East  = world x
        t.vz = vW[1];   // Up    = world y

        // ---- Physical dimensions (unchanged) --------------------------------
        t.dimensions.length   = static_cast<double>(platform->collider.getDimension().Length);
        t.dimensions.height   = static_cast<double>(platform->collider.getDimension().Height);
        t.dimensions.width    = static_cast<double>(platform->collider.getDimension().Width);
        t.dimensions.valid    = true;
        t.dimensions.material = aesa::TargetMaterialType::METAL;
        t.dimensions.shape    = aesa::TargetShapeType::GENERIC;

        // ---- Target descriptor defaults (unchanged) -------------------------
        t.rcs          = 1.0;
        t.rcsTable     = {};
        t.platformType = "GENERIC";
        t.swerlingCase = aesa::SwerlingCase::CASE_I;
        t.surface      = aesa::SurfaceType::AIR;
        t.jammer.active = false;

        inputs.push_back(t);
    }
    return inputs;
}

// =============================================================================
// FUNCTION:    AESARadar::updateVelocityEstimate
// DESCRIPTION: Finite-difference velocity from world position change over
//              actual elapsed simulation time, smoothed by a first-order
//              filter (VEL_SMOOTH_TAU_S). Only updates when simulation time
//              advanced (velDt > 0): replay can call scan() before time
//              advances, and dividing a full frame of movement by a
//              near-zero step would give a huge false velocity.
//              Returns world-axis velocity (x = E, y = U, z = N) in m/s.
//              REQ-AESA-004.
// =============================================================================
std::array<double, 3> AESARadar::updateVelocityEstimate(
    uint32_t id, const QVector3D& posKm, double velDt) const
{
    VelocityState& s = velState_[id];
    s.seen = true;

    if (s.samples == 0)
    {
        // First sighting: record position, no velocity yet.
        s.prevPosKm = posKm;
        s.samples   = 1;
        return { 0.0, 0.0, 0.0 };
    }

    if (velDt > 0.0)
    {
        // Finite difference in double; engine km -> m.
        const double raw[3] = {
            (static_cast<double>(posKm.x()) - static_cast<double>(s.prevPosKm.x())) * COORD_SCALE / velDt,
            (static_cast<double>(posKm.y()) - static_cast<double>(s.prevPosKm.y())) * COORD_SCALE / velDt,
            (static_cast<double>(posKm.z()) - static_cast<double>(s.prevPosKm.z())) * COORD_SCALE / velDt };

        if (s.samples == 1)
        {
            for (int i = 0; i < 3; ++i) s.v[i] = raw[i];
        }
        else
        {
            const double alpha = velDt / (VEL_SMOOTH_TAU_S + velDt);
            for (int i = 0; i < 3; ++i) s.v[i] += alpha * (raw[i] - s.v[i]);
        }
        s.prevPosKm = posKm;
        s.samples   = 2;
    }
    return { s.v[0], s.v[1], s.v[2] };
}

// =============================================================================
// OUTPUT TRANSLATION
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::processSurveillance
// (Full description in header)
// =============================================================================
void AESARadar::processSurveillance(
    const aesa::RadarOutput&                        output,
    const aesa::RadarConfig&                        cfg,
    const std::unordered_map<uint32_t, Platform*>&  idMap,
    std::unordered_set<uint32_t>&                   addedIDs)
{
    for (const auto& d : output.detections)
    {
        // FIX-03: DRFM ghosts are forwarded as warnings and never added to
        // the target list. Prevents fire-control engaging on a false target.
        // REQ-AESA-060.
        if (d.isDRFMGhost)
        {
            emit drfmGhostDetected(d.targetID,
                                   static_cast<float>(d.range),
                                   static_cast<float>(d.azimuth),
                                   static_cast<float>(d.elevation));
            continue;
        }

        // Deduplicate — only the first detection per targetID per scan tick
        // is added. REQ-AESA-004.
        if (addedIDs.count(d.targetID)) continue;
        addedIDs.insert(d.targetID);

        // Resolve Platform* from the id map built by collectTargets(). REQ-AESA-004.
        Platform* platform = nullptr;
        auto it = idMap.find(d.targetID);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity = platform;

        // Range: model output is in metres; Sensor::Target::radius is in km.
        t.radius = static_cast<float>(d.range * RANGE_M_TO_KM);

        // Model angles are true (stabilised); the display needs body azimuth.
        // REQ-AESA-004.
        t.angle       = static_cast<float>(stabToBodyAz(lastPose_, d.azimuth, d.elevation));
        t.trueBearing = static_cast<float>(d.azimuth);

        t.speed          = static_cast<float>(d.speedOverGround);
        t.direction      = static_cast<float>(d.heading);

        // Altitude: add radarHeight so the display shows absolute altitude
        // rather than altitude relative to the radar. REQ-AESA-004.
        t.altitude = static_cast<float>(
            d.range * std::sin(d.elevation * M_PI / 180.0)
            + cfg.radarHeight);

        t.radialVelocity = static_cast<float>(d.radialVelocity);
        targets.append(t);
    }
}

// =============================================================================
// FUNCTION:    AESARadar::processTWS
// (Full description in header)
// =============================================================================
void AESARadar::processTWS(
    const aesa::RadarOutput&                        output,
    const aesa::RadarConfig&                        cfg,
    const std::unordered_map<uint32_t, Platform*>&  idMap,
    std::unordered_set<uint32_t>&                   addedIDs)
{
    // Emit DRFM ghost warnings first — before iterating tracks — so that
    // all ghost detections are forwarded even if the corresponding track id
    // is later found in the track list. FIX-03. REQ-AESA-060.
    for (const auto& d : output.detections)
    {
        if (d.isDRFMGhost)
            emit drfmGhostDetected(d.targetID,
                                   static_cast<float>(d.range),
                                   static_cast<float>(d.azimuth),
                                   static_cast<float>(d.elevation));
    }

    for (const auto& tr : output.tracks)
    {
        // Deduplicate. REQ-AESA-004.
        if (addedIDs.count(tr.id)) continue;
        addedIDs.insert(tr.id);

        // FIX-03: DRFM-suspect tracks never appear in the target list.
        // The fire-control system must not be given a DRFM-contaminated track.
        // REQ-AESA-060.
        if (tr.isDRFMSuspect) continue;

        Platform* platform = nullptr;
        auto it = idMap.find(tr.id);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity = platform;
        t.radius = static_cast<float>(tr.range * RANGE_M_TO_KM);

        // Model angles are true (stabilised); the display needs body azimuth.
        // REQ-AESA-004.
        t.angle       = static_cast<float>(stabToBodyAz(lastPose_, tr.azimuth, tr.elevation));
        t.trueBearing = static_cast<float>(tr.azimuth);

        t.speed          = static_cast<float>(tr.speedOverGround);
        t.direction      = static_cast<float>(tr.heading);

        // Altitude: Kalman z state plus radar platform height. REQ-AESA-004.
        t.altitude = static_cast<float>(tr.z + cfg.radarHeight);

        t.radialVelocity = static_cast<float>(tr.radialVelocity);
        targets.append(t);

        // FIX-04: Emit IFF result for every validated track. REQ-AESA-050.
        if (tr.isValidated)
            emit iffResult(tr.id,
                           static_cast<int>(tr.iff.response),
                           tr.iff.squawk,
                           static_cast<float>(tr.iff.confidence));

        // FIX-01: Warn if a validated track is approaching the Doppler notch.
        // Threshold = DOPPLER_NOTCH_THRESHOLD m/s. REQ-AESA-040.
        if (tr.isValidated && std::abs(tr.radialVelocity) < DOPPLER_NOTCH_THRESHOLD)
            emit trackBelowDopplerNotch(tr.id);
    }
}

// =============================================================================
// FUNCTION:    AESARadar::processLockOn
// (Full description in header)
// =============================================================================
void AESARadar::processLockOn(
    const aesa::RadarOutput&                        output,
    const aesa::RadarConfig&                        cfg,
    const std::unordered_map<uint32_t, Platform*>&  idMap)
{
    // Break-lock condition — model has lost the target. REQ-AESA-003.
    if (output.lockBroken)
    {
        qDebug() << "[AESARadar] Lock broken";
        return;
    }

    if (output.tracks.empty()) return;

    uint32_t lockedID = cfg.lockedTargetID;

    // Find the locked track in the output. Linear search over the (typically
    // small) fire-control track list. REQ-AESA-003.
    const aesa::TrackOutput* locked = nullptr;
    for (const auto& tr : output.tracks)
        if (tr.id == lockedID) { locked = &tr; break; }

    if (!locked) return;

    // FIX-03: DRFM-contaminated locked track — emit ghost warning and abort.
    // Do NOT hand a DRFM ghost to the fire-control system. REQ-AESA-060.
    if (locked->isDRFMSuspect)
    {
        emit drfmGhostDetected(locked->id, 0.0f, 0.0f, 0.0f);
        return;
    }

    Platform* platform = nullptr;
    auto it = idMap.find(locked->id);
    if (it != idMap.end()) platform = it->second;

    Target t{};
    t.entity = platform;
    t.radius = static_cast<float>(locked->range * RANGE_M_TO_KM);

    // Model angles are true (stabilised); the display needs body azimuth.
    // REQ-AESA-004.
    t.angle       = static_cast<float>(stabToBodyAz(lastPose_, locked->azimuth, locked->elevation));
    t.trueBearing = static_cast<float>(locked->azimuth);

    t.speed          = static_cast<float>(locked->speedOverGround);
    t.direction      = static_cast<float>(locked->heading);
    t.altitude       = static_cast<float>(locked->z + cfg.radarHeight);
    t.radialVelocity = static_cast<float>(locked->radialVelocity);
    targets.append(t);

    // FIX-04: IFF result for the locked track. REQ-AESA-050.
    emit iffResult(locked->id,
                   static_cast<int>(locked->iff.response),
                   locked->iff.squawk,
                   static_cast<float>(locked->iff.confidence));
}

// =============================================================================
// MAIN ENGINE TICK
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::scan
// (Full description in header)
// =============================================================================
void AESARadar::scan()
{
    // Guard against uninitialised state. REQ-AESA-001.
    if (!parentEntity) return;
#ifdef AESA_REGRESSION_TEST
    static bool s_seeded = false;
    if (!s_seeded) { aesa_test_seed_model(12345u); aesa_test_seed_signal(12345u); s_seeded = true; }
#endif

    // Own platform lookup with find(): operator[] would insert a null entry
    // and then dereference it if the platform is missing. REQ-AESA-001.
    auto ownIt = root->Platforms.find(parentEntity->ID);
    if (ownIt == root->Platforms.end() || !ownIt->second) return;
    Transform& source = ownIt->second->transform;

    // -------------------------------------------------------------------------
    // dt computation — identical pattern to radar.cpp.
    // First call uses FIRST_FRAME_DT to avoid a zero initial step.
    // Subsequent calls measure real elapsed time and clamp to [MIN_DT, MAX_DT].
    // REQ-AESA-001.
    // -------------------------------------------------------------------------
    // Simulation time from the owning platform. Platform::update() copies the
    // engine time into Platform::time immediately before calling scan() on its
    // sensors, in the same call chain: no dependency on Simulation and no
    // cross-thread read. REQ-AESA-001.
    const double now = static_cast<double>(ownIt->second->time);
    double dt;
    double velDt = 0.0;   // actual elapsed sim time for velocity estimation (0 = no update)
    if (!timerStarted_)
    {
        timerStarted_ = true;
        dt = FIRST_FRAME_DT;
        velState_.clear();
    }
    else if (now < lastSimTime_)
    {
        // Simulation time moved backwards (Reset / timeJump to earlier time).
        radarCore_.reset();
        velState_.clear();
        dt = FIRST_FRAME_DT;
    }
    else
    {
        const double elapsed = now - lastSimTime_;
        if (elapsed > MAX_DT) velState_.clear();   // forward jump: positions not continuous
        else                  velDt = elapsed;     // 0 when time did not advance
        dt = std::clamp(elapsed, MIN_DT, MAX_DT);
    }
    lastSimTime_ = now;
    simTime_     = now;

    // -------------------------------------------------------------------------
    // Pose — build from current platform transform. REQ-AESA-010.
    // -------------------------------------------------------------------------
    aesa::RadarPose pose = buildPose();

    // ---- Attitude and own velocity (stabilised frame). REQ-AESA-010. -------
    for (auto& kv : velState_) kv.second.seen = false;
    const QVector3D ownPosKm = source.matrix.translation();
    {
        // Rows = body axes (forward, right, up) in (North, East, Up)
        // = (world z, world x, world y) of each transform direction vector.
        const QVector3D f = source.forward();
        const QVector3D r = source.right();
        const QVector3D u = source.up();
        const double R[3][3] = { { f.z(), f.x(), f.y() },
                                 { r.z(), r.x(), r.y() },
                                 { u.z(), u.x(), u.y() } };
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) pose.bodyFromStab[i][j] = R[i][j];

        const std::array<double, 3> ov =
            updateVelocityEstimate(platformToRadarID(parentEntity->ID), ownPosKm, velDt);
        pose.vx = ov[2];   // North
        pose.vy = ov[0];   // East
        pose.vz = ov[1];   // Up
    }
    lastPose_ = pose;   // platform body frame: used for display angles (t.angle)

    // ---- Antenna mount (REQ-AESA-010) ---------------------------------------
    // The model works in the ARRAY frame: arrayFromStab = M * bodyFromStab.
    // Scan limits, steering limit, beam gate, gain and track / fire-control
    // pointing then all apply around the array's facing direction.
    // Skipped for mount 0/0: bit-identical to the nose-mounted radar.
    if (mountAzimuth_deg_ != 0.0f || mountElevation_deg_ != 0.0f)
    {
        double M[3][3];
        mountFromBody(mountAzimuth_deg_, mountElevation_deg_, M);
        double A[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                A[i][j] = M[i][0] * pose.bodyFromStab[0][j]
                        + M[i][1] * pose.bodyFromStab[1][j]
                        + M[i][2] * pose.bodyFromStab[2][j];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) pose.bodyFromStab[i][j] = A[i][j];
    }

    // radarHeight: updated inside RadarModel_AESA::update() step 2 from
    // pose.y with the same thresholds (50 m, 1 m). REQ-AESA-071.

    // platformSpeed_m_s: feeds clutter notch / STAP. FIX-01. REQ-AESA-040.
    {
        auto it = root->Platforms.find(parentEntity->ID);
        if (it != root->Platforms.end())
        {
            radarCore_.setPlatformSpeed(
                static_cast<float>(it->second->dynamicModel.currentSpeed / 3.6));
        }
    }

    // -------------------------------------------------------------------------
    // Collect targets and drive the model update. REQ-AESA-004.
    // -------------------------------------------------------------------------
    std::unordered_map<uint32_t, Platform*> idMap;
    std::vector<aesa::TargetInput> radarInputs = collectTargets(ownPosKm, velDt, idMap);

    // Drop velocity-estimator entries for entities no longer present.
    for (auto it = velState_.begin(); it != velState_.end(); )
        it = it->second.seen ? std::next(it) : velState_.erase(it);

    // ---- Plugin logic (AESA plugin SDK). REQ-AESA-090. ----------------------
    // One snapshot per scan. Empty registry = nothing below runs and the
    // model uses its built-in logic only.
    const std::shared_ptr<const AesaLogicSnapshot> pluginHooks =
        AesaLogicRegistry::instance().snapshot();
    AesaStageContext stageCtx;
    if (pluginHooks && !pluginHooks->empty())
    {
        stageCtx.hooks          = pluginHooks.get();
        stageCtx.idMap          = &idMap;
        stageCtx.radar.radarId  = ID;
        stageCtx.radar.entityId = parentEntity->ID;
        stageCtx.radar.name     = Name;
    }

    // Pre-filter: plugins may remove targets before the radar sees them.
    if (stageCtx.hooks != nullptr && !stageCtx.hooks->preFilters.empty())
    {
        radarInputs.erase(
            std::remove_if(radarInputs.begin(), radarInputs.end(),
                [&stageCtx](const aesa::TargetInput& t)
                {
                    const double r = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
                    const aesaplugin::TargetGeometry g = stageGeometry(stageCtx, t, r);
                    for (const auto& keepFn : stageCtx.hooks->preFilters)
                    {
                        bool keep = true;
                        {
                            AesaLogicRegistry::StageGuard guard;
                            try { keep = keepFn(stageCtx.radar, g); } catch (...) { keep = true; }
                        }
                        if (!keep) return true;   // remove
                    }
                    return false;
                }),
            radarInputs.end());
    }

    // Replaceable stages: installed for this update() only.
    const bool pluginStages = (stageCtx.hooks != nullptr && stageCtx.hooks->hasStages());
    if (pluginStages)
    {
        aesa::LogicOverrides ov;
        ov.context = &stageCtx;
        if (stageCtx.hooks->rcs)         ov.meanRcs         = &stageMeanRcs;
        if (stageCtx.hooks->detection)   ov.detect          = &stageDetect;
        if (stageCtx.hooks->propagation) ov.propagationLoss = &stagePropagation;
        radarCore_.setLogicOverrides(ov);
    }

    radarCore_.update(dt, pose, radarInputs, simTime_);

    // stageCtx lives on this stack frame: clear before it goes out of scope.
    if (pluginStages)
        radarCore_.setLogicOverrides(aesa::LogicOverrides{});

    aesa::RadarOutput output = radarCore_.getOutput();
    aesa::RadarConfig cfg    = radarCore_.getConfig();
#ifdef AESA_REGRESSION_TEST
    qInfo().noquote() << QString("TICK %1 mode=%2 dets=%3 trks=%4 dr=%5")
        .arg(simTime_, 0, 'f', 6).arg(int(output.mode))
        .arg(output.detections.size()).arg(output.tracks.size())
        .arg(output.displayRange_km, 0, 'g', 17);
    for (const auto& d : output.detections)
        qInfo().noquote() << QString("  D %1 %2 %3 %4 %5").arg(d.targetID)
            .arg(d.range, 0, 'g', 17).arg(d.azimuth, 0, 'g', 17)
            .arg(d.elevation, 0, 'g', 17).arg(d.snr, 0, 'g', 17);
    for (const auto& t : output.tracks)
        qInfo().noquote() << QString("  T %1 %2 %3 %4").arg(t.id)
            .arg(t.x, 0, 'g', 17).arg(t.y, 0, 'g', 17).arg(t.z, 0, 'g', 17);
#endif

    // -------------------------------------------------------------------------
    // Sync Sensor base fields from model output. REQ-AESA-004.
    // -------------------------------------------------------------------------
    // Beam azimuth relative to the nose (model output is array frame).
    // REQ-AESA-010.
    {
        double pAz = 0.0, pEl = 0.0;
        arrayToPlatformAzEl(mountAzimuth_deg_, mountElevation_deg_,
                            output.currentAzimuth, output.currentElevation,
                            pAz, pEl);
        azimuth = static_cast<float>(pAz);
    }
    beamWidth         = cfg.beamWidth;
    maxDetectionAngle = cfg.maxAzimuth;

    cachedDisplayRange_ = std::clamp(
        static_cast<float>(output.displayRange_km),
        DISPLAY_RANGE_MIN_KM,
        DISPLAY_RANGE_MAX_KM);
    range = cachedDisplayRange_;
    if(parentEntity){
        auto& data = getSensorData();
        data.sensor[dataid].range = range;
    }
    // Sync Sensor mode from model output mode. REQ-AESA-003.
    switch (output.mode)
    {
    case aesa::RadarMode::SURVEILLANCE: mode = Sensor::Mode::Search;         break;
    case aesa::RadarMode::TWS:          mode = Sensor::Mode::TrackWhileScan; break;
    case aesa::RadarMode::LOCK_ON:      mode = Sensor::Mode::FireControl;    break;
    }

    // -------------------------------------------------------------------------
    // Populate Sensor::targets from mode-specific output translator.
    // REQ-AESA-004.
    // -------------------------------------------------------------------------
    targets.clear();
    std::unordered_set<uint32_t> addedIDs;

    switch (output.mode)
    {
    case aesa::RadarMode::SURVEILLANCE:
        processSurveillance(output, cfg, idMap, addedIDs);
        break;
    case aesa::RadarMode::TWS:
        processTWS(output, cfg, idMap, addedIDs);
        break;
    case aesa::RadarMode::LOCK_ON:
        processLockOn(output, cfg, idMap);
        break;
    }

    // Post-filter (AESA plugin SDK): plugins may remove entries from the
    // engine-visible target list. Radar tracks and DIS emission are not
    // affected. REQ-AESA-090.
    if (stageCtx.hooks != nullptr && !stageCtx.hooks->postFilters.empty())
    {
        decltype(targets) kept;
        kept.reserve(targets.size());
        for (const auto& t : targets)
        {
            aesaplugin::OutputTarget o;
            o.targetEntityId     = (t.entity != nullptr) ? t.entity->ID : std::string();
            o.range_km           = t.radius;
            o.bearingBody_deg    = t.angle;
            o.trueBearing_deg    = t.trueBearing;
            o.speed_m_s          = t.speed;
            o.heading_deg        = t.direction;
            o.altitude_m         = t.altitude;
            o.radialVelocity_m_s = t.radialVelocity;

            bool keep = true;
            for (const auto& keepFn : stageCtx.hooks->postFilters)
            {
                {
                    AesaLogicRegistry::StageGuard guard;
                    try { keep = keepFn(stageCtx.radar, o); } catch (...) { keep = true; }
                }
                if (!keep) break;
            }
            if (keep) kept.append(t);
        }
        targets = kept;
    }

    // -------------------------------------------------------------------------
    // FIX-08: Emit duty cycle every tick. REQ-AESA-020.
    // -------------------------------------------------------------------------
    emit schedulerDutyCycle(static_cast<float>(output.currentDutyCycle));

    // -------------------------------------------------------------------------
    // Base-class detection signals. REQ-AESA-004.
    // -------------------------------------------------------------------------
    if (!targets.isEmpty())
        emit enemyDetected();
    else
        emit enemyNotFound();

    {
        DISEmissionSnapshot snap;
        snap.emittingEntityID = parentEntity->ID;
        // snap.systemID          = cfg.emitterCode;
        snap.systemID          = static_cast<uint8_t>(emitterIndex > 0 ? emitterIndex : 1);
        snap.active            = true;
        snap.emitterFunction   = 0;  // TODO: real SISO-REF-010 UID 76 code

        DISEmissionBeam beam;
        beam.beamID = 1;

        const aesa::BeamWaveform& wf = currentWaveformFor(cfg, output.mode);
        beam.frequency  = static_cast<float>(cfg.frequency_Hz);
        beam.freqRange  = static_cast<float>(cfg.antennaBandwidth);
        beam.pulseWidth = wf.pulseWidth_s * 1.0e6f;
        beam.prf        = wf.prf_Hz;
        beam.erpPower   = computeERP_dBm(cfg);

        // DIS beam centre is relative to the emitting entity's body: convert
        // from the array frame with the mount orientation. REQ-AESA-010.
        {
            double pAz = 0.0, pEl = 0.0;
            arrayToPlatformAzEl(mountAzimuth_deg_, mountElevation_deg_,
                                output.currentAzimuth, output.currentElevation,
                                pAz, pEl);
            beam.beamAzCenter = static_cast<float>(pAz);
            beam.beamElCenter = static_cast<float>(pEl);
        }
        beam.beamAzSweep  = cfg.beamWidth;
        beam.beamElSweep  = cfg.beamWidth;
        beam.beamFunction = 0;  // TODO: real SISO-REF-010 UID 78 code

        if (output.mode == aesa::RadarMode::SURVEILLANCE) {
            for (const auto& d : output.detections) {
                auto it = idMap.find(d.targetID);
                if (it != idMap.end() && it->second)
                    beam.trackedEntityIDs.push_back(it->second->ID);
            }
        } else {
            for (const auto& tr : output.tracks) {
                auto it = idMap.find(tr.id);
                if (it != idMap.end() && it->second)
                    beam.trackedEntityIDs.push_back(it->second->ID);
            }
        }

        snap.beams.push_back(beam);
        emit emissionSnapshotReady(snap);

    }
}

// =============================================================================
// SERIALISATION
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::toJson
// (Full description in header)
// =============================================================================
QJsonObject AESARadar::toJson() const
{
    QJsonObject obj;
    aesa::RadarConfig cfg = radarCore_.getConfig();

    // ---- Identity fields ----------------------------------------------------
    obj["id"]         = QString::fromStdString(ID);
    obj["name"]       = QString::fromStdString(Name);
    obj["Active"]     = Active;
    obj["SensorType"] = "AESA";

    // ---- Array section (AESA-specific T/R module parameters) ----------------
    QJsonObject array;
    array["type"]                  = "Section";
    array["numElements"]           = toParm(cfg.numElements,            "",   100.0f,  10000.0f, "Number of T/R elements");
    array["peakPowerPerElement_W"] = toParm(cfg.peakPowerPerElement_W,  "W",    0.1f,    100.0f, "Peak power per element");
    array["moduleEfficiency"]      = toParm(cfg.moduleEfficiency,       "",     0.0f,      1.0f, "T/R module efficiency");
    array["failedModules"]         = toParm(cfg.failedModules,          "",     0.0f,   1000.0f, "Failed modules count");
    array["maxDutyCycle"]          = toParm(cfg.maxDutyCycle,           "",     0.0f,      1.0f, "Max duty cycle");
    obj["array"] = array;

    // ---- Transmitter section ------------------------------------------------
    QJsonObject transmitter;
    transmitter["type"]              = "Section";
    transmitter["frequency_Hz"]      = toParm(cfg.frequency_Hz,       "Hz",   1e6f,   1e11f,  "Operating frequency");
    transmitter["antennaGain"]       = toParm(cfg.antennaGain,        "dBi",  0.0f,   60.0f,  "Antenna gain");
    transmitter["antennaBandwidth"]  = toParm(cfg.antennaBandwidth,   "Hz",   0.0f,   1e9f,   "Receiver bandwidth");
    transmitter["beamWidth"]         = toParm(cfg.beamWidth,          "deg",  0.1f,   30.0f,  "Beam width");
    transmitter["sidelobeMode"]      = static_cast<int>(cfg.sidelobeMode);
    transmitter["peakSidelobeLevel"] = toParm(cfg.peakSidelobeLevel,  "dB", -80.0f,   0.0f,  "Peak sidelobe level");
    transmitter["avgSidelobeLevel"]  = toParm(cfg.avgSidelobeLevel,   "dB", -80.0f,   0.0f,  "Average sidelobe level");
    transmitter["sidelobeBlanking"]  = toParm(cfg.sidelobeBlanking_dB,"dB", -80.0f,   0.0f,  "Guard-horn blanking threshold");
    obj["transmitter"] = transmitter;

    // ---- Scan section -------------------------------------------------------
    QJsonObject scan;
    scan["type"]             = "Section";
    scan["minAzimuth"]       = toParm(cfg.minAzimuth,            "deg", -180.0f,   0.0f, "Min azimuth");
    scan["maxAzimuth"]       = toParm(cfg.maxAzimuth,            "deg",    0.0f, 180.0f, "Max azimuth");
    scan["minElevation"]     = toParm(cfg.minElevation,          "deg",  -90.0f,   0.0f, "Min elevation");
    scan["maxElevation"]     = toParm(cfg.maxElevation,          "deg",    0.0f,  90.0f, "Max elevation");
    scan["maxSteeringAngle"] = toParm(cfg.maxSteeringAngle_deg,  "deg",    0.0f,  90.0f, "Max electronic steering angle");
    scan["mountAzimuth"]     = toParm(mountAzimuth_deg_,         "deg", -180.0f, 180.0f, "Antenna mount azimuth (0 = forward, 180 = rearward, +90 = right)");
    scan["mountElevation"]   = toParm(mountElevation_deg_,       "deg",  -90.0f,  90.0f, "Antenna mount elevation (tilt, + = up)");
    scan["searchDwellTime"]  = toParm(cfg.searchDwellTime_ms,    "ms",     0.1f, 100.0f, "Search dwell time");
    scan["trackDwellTime"]   = toParm(cfg.trackDwellTime_ms,     "ms",     0.1f, 100.0f, "Track dwell time");
    scan["fcDwellTime"]      = toParm(cfg.fireControlDwellTime_ms,"ms",    0.1f, 100.0f, "Fire-control dwell time");
    obj["scan"] = scan;

    // ---- Waveform section ---------------------------------------------------
    // Local lambda: serialise one BeamWaveform struct to a QJsonObject.
    auto serWF = [](const aesa::BeamWaveform& w) -> QJsonObject
    {
        QJsonObject o;
        o["modulation"]    = static_cast<int>(w.modulation);
        o["pulseWidth_s"]  = static_cast<double>(w.pulseWidth_s);
        o["prf_Hz"]        = static_cast<double>(w.prf_Hz);
        o["prf2_Hz"]       = static_cast<double>(w.prf2_Hz);   // staggered PRF
        o["bandwidth_Hz"]  = static_cast<double>(w.bandwidth_Hz);
        o["pulsesPerDwell"]= w.pulsesPerDwell;
        o["mode"]          = static_cast<int>(w.mode);
        return o;
    };

    QJsonObject waveform;
    waveform["type"]              = "Section";
    waveform["frequencyAgility"]  = cfg.frequencyAgility;
    waveform["hopStartFrequency"] = toParm(cfg.hopStartFrequency, "Hz", 0.0f, 1e11f, "Hop lower bound");
    waveform["hopStopFrequency"]  = toParm(cfg.hopStopFrequency,  "Hz", 0.0f, 1e11f, "Hop upper bound");
    waveform["searchWaveform"]     = serWF(cfg.searchWaveform);
    waveform["trackWaveform"]      = serWF(cfg.trackWaveform);
    waveform["fireControlWaveform"]= serWF(cfg.fireControlWaveform);

    // Waveform selection table — omit sentinel entries (maxRange_m <= 0).
    QJsonArray wfTable;
    for (const auto& e : cfg.waveformTable)
    {
        if (e.maxRange_m <= 0.0f) break;
        QJsonObject we;
        we["maxRange_m"] = static_cast<double>(e.maxRange_m);
        we["waveform"]   = serWF(e.waveform);
        wfTable.append(we);
    }
    waveform["table"] = wfTable;
    obj["waveform"] = waveform;

    // ---- Detection section --------------------------------------------------
    QJsonObject detection;
    detection["type"]                = "Section";
    detection["systemTemperature_K"] = toParm(cfg.systemTemperature_K, "K",    0.0f, 1000.0f, "System noise temperature");
    detection["noiseFigure_dB"]      = toParm(cfg.noiseFigure_dB,      "dB",   0.0f,   30.0f, "Receiver noise figure");
    detection["targetPfa"]           = toParm(cfg.targetPfa,           "",     0.0f,    1.0f, "False alarm probability");
    detection["minDetectableRange"]  = toParm(cfg.minDetectableRange,  "m",    0.0f, 1000.0f, "Min detectable range-not editabale");
    detection["seaState"]            = toParm(cfg.seaState,            "",     0.0f,    9.0f, "Sea state");
    detection["landClutter"]         = toParm(cfg.landClutter,         "",     0.0f,    1.0f, "Land clutter factor");
    detection["targetCategory"]      = static_cast<int>(cfg.targetCategory);
    obj["detection"] = detection;

    // ---- Platform section ---------------------------------------------------
    QJsonObject platform;
    platform["type"]               = "Section";
    platform["radarHeight"]        = toParm(cfg.radarHeight,       "m",   0.0f, 30000.0f, "Radar platform height");
    platform["platformSpeed_m_s"]  = toParm(cfg.platformSpeed_m_s, "m/s", 0.0f,  3000.0f, "Platform speed");
    platform["minDetectableRange"] = toParm(cfg.minDetectableRange,"m",   0.0f, 10000.0f, "Min detectable range");
    obj["platform"] = platform;

    // ---- Tracking section ---------------------------------------------------
    QJsonObject tracking;
    tracking["type"]                  = "Section";
    tracking["missedScansToDrop"]     = toParm(cfg.missedScansToDrop,     "",    0.0f,   20.0f, "Missed scans before drop");
    tracking["trackCoastSeconds"]     = toParm(cfg.trackCoastSeconds,     "s",   0.0f,  300.0f, "Coast duration");
    tracking["minHitsToValidate"]     = toParm(cfg.minHitsToValidate,     "",    1.0f,   10.0f, "Hits to validate");
    tracking["maxTrackSpeed"]         = toParm(cfg.maxTrackSpeed,         "m/s", 0.0f, 5000.0f, "Max track speed");
    tracking["manoeuvreThreshold"]    = toParm(cfg.manoeuvreThreshold_m,  "m",   0.0f, 5000.0f, "Manoeuvre detection threshold");
    tracking["useJPDA"]               = cfg.useJPDA;
    tracking["jpdaFalseAlarmDensity"] = toParm(cfg.jpdaFalseAlarmDensity, "",   0.0f,    1.0f, "JPDA false alarm density");
    obj["tracking"] = tracking;

    // ---- Propagation section ------------------------------------------------
    QJsonObject propagation;
    propagation["type"]              = "Section";
    propagation["earthRadiusFactor"] = toParm(cfg.earthRadiusFactor,          "",      1.0f,    2.0f, "Earth radius factor");
    propagation["atmosphericFactor"] = toParm(cfg.atmosphericFactor,          "",      0.0f,    2.0f, "Atmospheric refraction");
    //propagation["temperature_C"]     = toParm(cfg.atmosphere.temperature_C,   "°C",  -60.0f,   60.0f, "Ambient temperature");
    //propagation["humidity_pct"]      = toParm(cfg.atmosphere.humidity_pct,    "%",     0.0f,  100.0f, "Relative humidity");
    //propagation["pressure_hPa"]      = toParm(cfg.atmosphere.pressure_hPa,    "hPa", 800.0f, 1100.0f, "Atmospheric pressure");
    //propagation["rainRate_mmph"]     = toParm(cfg.atmosphere.rainRate_mmph,   "mm/h",  0.0f,  200.0f, "Rain rate");
    // propagation["fogVisibility_m"]   = toParm(cfg.atmosphere.fogVisibility_m, "m",     0.0f, 10000.0f, "Fog visibility");
    obj["propagation"] = propagation;

    // ---- Noise section ------------------------------------------------------
    QJsonObject noise;
    noise["type"]            = "Section";
    noise["rangeStdDev"]     = toParm(cfg.noise.rangeStdDev,     "m",   0.0f, 1000.0f, "Range noise σ");
    noise["azimuthStdDev"]   = toParm(cfg.noise.azimuthStdDev,   "deg", 0.0f,   10.0f, "Azimuth noise σ");
    noise["elevationStdDev"] = toParm(cfg.noise.elevationStdDev, "deg", 0.0f,   10.0f, "Elevation noise σ");
    noise["dopplerStdDev"]   = toParm(cfg.noise.dopplerStdDev,   "m/s", 0.0f,  100.0f, "Doppler noise σ");
    obj["noise"] = noise;

    // ---- IFF section (FIX-04) -----------------------------------------------
    // QJsonObject iff;
    // iff["type"]              = "Section";
    // iff["interrogationMode"] = static_cast<int>(cfg.interrogationMode);
    // QJsonArray squawks;
    // for (uint32_t sq : cfg.friendlySquawks) squawks.append(static_cast<int>(sq));
    // iff["friendlySquawks"]   = squawks;
    // obj["iff"] = iff;

    // ---- Null steering section ----------------------------------------------
    QJsonObject nullSteering;
    nullSteering["type"]          = "Section";
    nullSteering["active"]        = cfg.nullSteering.active;
    nullSteering["azimuth_deg"]   = toParm(cfg.nullSteering.azimuth_deg,   "deg", -180.0f,  180.0f, "Null azimuth");
    nullSteering["elevation_deg"] = toParm(cfg.nullSteering.elevation_deg, "deg",  -90.0f,   90.0f, "Null elevation");
    nullSteering["nullDepth_dB"]  = toParm(cfg.nullSteering.nullDepth_dB,  "dB",  -100.0f,    0.0f, "Null depth");
    obj["nullSteering"] = nullSteering;

    // ---- Mode ---------------------------------------------------------------
    obj["mode"] = static_cast<int>(cfg.mode);

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    obj["emitterIndex"] = emitterIndex;
    return obj;
}

// =============================================================================
// FUNCTION:    AESARadar::fromJson
// (Full description in header)
// =============================================================================
void AESARadar::fromJson(const QJsonObject& obj)
{
    if (obj.contains("Active"))
        Active = obj["Active"].toBool();
    // Start from the current live config so that missing JSON keys leave the
    // corresponding fields unchanged. REQ-AESA-002.
    aesa::RadarConfig cfg = radarCore_.getConfig();

    // ---- Array section ------------------------------------------------------
    if (obj.contains("array") && obj["array"].isObject())
    {
        QJsonObject a = obj["array"].toObject();
        if (a.contains("numElements"))           cfg.numElements           = static_cast<int>(valueFromParm(a["numElements"].toObject()));
        if (a.contains("peakPowerPerElement_W")) cfg.peakPowerPerElement_W = valueFromParm(a["peakPowerPerElement_W"].toObject());
        if (a.contains("moduleEfficiency"))      cfg.moduleEfficiency      = valueFromParm(a["moduleEfficiency"].toObject());
        if (a.contains("failedModules"))         cfg.failedModules         = static_cast<int>(valueFromParm(a["failedModules"].toObject()));
        if (a.contains("maxDutyCycle"))          cfg.maxDutyCycle          = valueFromParm(a["maxDutyCycle"].toObject());
    }

    // ---- Transmitter section ------------------------------------------------
    if (obj.contains("transmitter") && obj["transmitter"].isObject())
    {
        QJsonObject t = obj["transmitter"].toObject();
        if (t.contains("frequency_Hz"))       cfg.frequency_Hz       = valueFromParm(t["frequency_Hz"].toObject());
        if (t.contains("antennaGain"))        cfg.antennaGain        = valueFromParm(t["antennaGain"].toObject());
        if (t.contains("antennaBandwidth"))   cfg.antennaBandwidth   = valueFromParm(t["antennaBandwidth"].toObject());
        if (t.contains("beamWidth"))          cfg.beamWidth          = valueFromParm(t["beamWidth"].toObject());
        if (t.contains("sidelobeMode"))       cfg.sidelobeMode       = static_cast<aesa::SidelobeMode>(t["sidelobeMode"].toInt());
        if (t.contains("peakSidelobeLevel"))  cfg.peakSidelobeLevel  = valueFromParm(t["peakSidelobeLevel"].toObject());
        if (t.contains("avgSidelobeLevel"))   cfg.avgSidelobeLevel   = valueFromParm(t["avgSidelobeLevel"].toObject());
        if (t.contains("sidelobeBlanking"))   cfg.sidelobeBlanking_dB= valueFromParm(t["sidelobeBlanking"].toObject());
    }

    // ---- Scan section -------------------------------------------------------
    if (obj.contains("scan") && obj["scan"].isObject())
    {
        QJsonObject s = obj["scan"].toObject();
        if (s.contains("minAzimuth"))        cfg.minAzimuth              = valueFromParm(s["minAzimuth"].toObject());
        if (s.contains("maxAzimuth"))        cfg.maxAzimuth              = valueFromParm(s["maxAzimuth"].toObject());
        if (s.contains("minElevation"))      cfg.minElevation            = valueFromParm(s["minElevation"].toObject());
        if (s.contains("maxElevation"))      cfg.maxElevation            = valueFromParm(s["maxElevation"].toObject());
        if (s.contains("maxSteeringAngle"))  cfg.maxSteeringAngle_deg    = valueFromParm(s["maxSteeringAngle"].toObject());
        if (s.contains("searchDwellTime"))   cfg.searchDwellTime_ms      = valueFromParm(s["searchDwellTime"].toObject());
        if (s.contains("trackDwellTime"))    cfg.trackDwellTime_ms       = valueFromParm(s["trackDwellTime"].toObject());
        if (s.contains("fcDwellTime"))       cfg.fireControlDwellTime_ms = valueFromParm(s["fcDwellTime"].toObject());
        if (s.contains("mountAzimuth"))      mountAzimuth_deg_           = valueFromParm(s["mountAzimuth"].toObject());
        if (s.contains("mountElevation"))    mountElevation_deg_         = valueFromParm(s["mountElevation"].toObject());
        // Map consumers draw the wedge as heading + min/max: give them the
        // sector around the array's facing direction. REQ-AESA-011.
        minAzimuth = cfg.minAzimuth + mountAzimuth_deg_;
        maxAzimuth = cfg.maxAzimuth + mountAzimuth_deg_;
        if(parentEntity){
            auto& data = getSensorData();
            data.sensor[dataid].minAzimuth = cfg.minAzimuth + mountAzimuth_deg_;
            data.sensor[dataid].maxAzimuth = cfg.maxAzimuth + mountAzimuth_deg_;
            data.sensor[dataid].range = range;
        }
    }

    // ---- Waveform section ---------------------------------------------------
    // Local lambda: deserialise one BeamWaveform from a QJsonObject.
    auto readWF = [](const QJsonObject& o, aesa::BeamWaveform& wf)
    {
        if (o.isEmpty()) return;
        if (o.contains("modulation"))    wf.modulation    = static_cast<aesa::ModulationType>(o["modulation"].toInt());
        if (o.contains("pulseWidth_s"))  wf.pulseWidth_s  = static_cast<float>(o["pulseWidth_s"].toDouble());
        if (o.contains("prf_Hz"))        wf.prf_Hz        = static_cast<float>(o["prf_Hz"].toDouble());
        if (o.contains("prf2_Hz"))       wf.prf2_Hz       = static_cast<float>(o["prf2_Hz"].toDouble());   // staggered PRF
        if (o.contains("bandwidth_Hz"))  wf.bandwidth_Hz  = static_cast<float>(o["bandwidth_Hz"].toDouble());
        if (o.contains("pulsesPerDwell"))wf.pulsesPerDwell= o["pulsesPerDwell"].toInt();
        if (o.contains("mode"))          wf.mode          = static_cast<aesa::WaveformMode>(o["mode"].toInt());
    };

    if (obj.contains("waveform") && obj["waveform"].isObject())
    {
        QJsonObject w = obj["waveform"].toObject();
        if (w.contains("frequencyAgility"))    cfg.frequencyAgility  = w["frequencyAgility"].toBool();
        if (w.contains("hopStartFrequency"))   cfg.hopStartFrequency = valueFromParm(w["hopStartFrequency"].toObject());
        if (w.contains("hopStopFrequency"))    cfg.hopStopFrequency  = valueFromParm(w["hopStopFrequency"].toObject());
        if (w.contains("searchWaveform"))      readWF(w["searchWaveform"].toObject(),       cfg.searchWaveform);
        if (w.contains("trackWaveform"))       readWF(w["trackWaveform"].toObject(),        cfg.trackWaveform);
        if (w.contains("fireControlWaveform")) readWF(w["fireControlWaveform"].toObject(),  cfg.fireControlWaveform);

        // Waveform selection table — read active entries up to array capacity.
        if (w.contains("table"))
        {
            QJsonArray arr = w["table"].toArray();
            int idx = 0;
            for (const QJsonValue& v : arr)
            {
                if (idx >= 6) break;
                QJsonObject we = v.toObject();
                cfg.waveformTable[idx].maxRange_m =
                    static_cast<float>(we.value("maxRange_m").toDouble(0.0));
                if (we.contains("waveform"))
                    readWF(we["waveform"].toObject(), cfg.waveformTable[idx].waveform);
                ++idx;
            }
        }
    }

    // ---- Detection section --------------------------------------------------
    if (obj.contains("detection") && obj["detection"].isObject())
    {
        QJsonObject d = obj["detection"].toObject();
        if (d.contains("systemTemperature_K")) cfg.systemTemperature_K = valueFromParm(d["systemTemperature_K"].toObject());
        if (d.contains("noiseFigure_dB"))      cfg.noiseFigure_dB      = valueFromParm(d["noiseFigure_dB"].toObject());
        if (d.contains("targetPfa"))           cfg.targetPfa           = valueFromParm(d["targetPfa"].toObject());
        if (d.contains("minDetectableRange"))  cfg.minDetectableRange  = valueFromParm(d["minDetectableRange"].toObject());
        if (d.contains("seaState"))            cfg.seaState            = valueFromParm(d["seaState"].toObject());
        if (d.contains("landClutter"))         cfg.landClutter         = valueFromParm(d["landClutter"].toObject());
        if (d.contains("targetCategory"))      cfg.targetCategory      = static_cast<aesa::DetectionCategory>(d["targetCategory"].toInt());
    }

    // ---- Platform section ---------------------------------------------------
    if (obj.contains("platform") && obj["platform"].isObject())
    {
        QJsonObject p = obj["platform"].toObject();
        if (p.contains("radarHeight"))        cfg.radarHeight        = valueFromParm(p["radarHeight"].toObject());
        if (p.contains("platformSpeed_m_s"))  cfg.platformSpeed_m_s  = valueFromParm(p["platformSpeed_m_s"].toObject());
        // if (p.contains("minDetectableRange")) cfg.minDetectableRange = valueFromParm(p["minDetectableRange"].toObject());
    }

    // ---- Tracking section ---------------------------------------------------
    if (obj.contains("tracking") && obj["tracking"].isObject())
    {
        QJsonObject t = obj["tracking"].toObject();
        if (t.contains("missedScansToDrop"))     cfg.missedScansToDrop     = static_cast<int>(valueFromParm(t["missedScansToDrop"].toObject()));
        if (t.contains("trackCoastSeconds"))     cfg.trackCoastSeconds     = valueFromParm(t["trackCoastSeconds"].toObject());
        if (t.contains("minHitsToValidate"))     cfg.minHitsToValidate     = static_cast<int>(valueFromParm(t["minHitsToValidate"].toObject()));
        if (t.contains("maxTrackSpeed"))         cfg.maxTrackSpeed         = valueFromParm(t["maxTrackSpeed"].toObject());
        if (t.contains("manoeuvreThreshold"))    cfg.manoeuvreThreshold_m  = valueFromParm(t["manoeuvreThreshold"].toObject());
        if (t.contains("useJPDA"))               cfg.useJPDA               = t["useJPDA"].toBool();
        if (t.contains("jpdaFalseAlarmDensity")) cfg.jpdaFalseAlarmDensity = valueFromParm(t["jpdaFalseAlarmDensity"].toObject());
    }

    // ---- Propagation section ------------------------------------------------
    if (obj.contains("propagation") && obj["propagation"].isObject())
    {
        QJsonObject p = obj["propagation"].toObject();
        if (p.contains("earthRadiusFactor")) cfg.earthRadiusFactor            = valueFromParm(p["earthRadiusFactor"].toObject());
        if (p.contains("atmosphericFactor")) cfg.atmosphericFactor            = valueFromParm(p["atmosphericFactor"].toObject());
        // if (p.contains("temperature_C"))     cfg.atmosphere.temperature_C     = valueFromParm(p["temperature_C"].toObject());
        // if (p.contains("humidity_pct"))      cfg.atmosphere.humidity_pct      = valueFromParm(p["humidity_pct"].toObject());
        // if (p.contains("pressure_hPa"))      cfg.atmosphere.pressure_hPa      = valueFromParm(p["pressure_hPa"].toObject());
        // if (p.contains("rainRate_mmph"))     cfg.atmosphere.rainRate_mmph     = valueFromParm(p["rainRate_mmph"].toObject());
        //if (p.contains("fogVisibility_m"))   cfg.atmosphere.fogVisibility_m   = valueFromParm(p["fogVisibility_m"].toObject());
    }

    // ---- Noise section ------------------------------------------------------
    if (obj.contains("noise") && obj["noise"].isObject())
    {
        QJsonObject n = obj["noise"].toObject();
        if (n.contains("rangeStdDev"))     cfg.noise.rangeStdDev     = valueFromParm(n["rangeStdDev"].toObject());
        if (n.contains("azimuthStdDev"))   cfg.noise.azimuthStdDev   = valueFromParm(n["azimuthStdDev"].toObject());
        if (n.contains("elevationStdDev")) cfg.noise.elevationStdDev = valueFromParm(n["elevationStdDev"].toObject());
        if (n.contains("dopplerStdDev"))   cfg.noise.dopplerStdDev   = valueFromParm(n["dopplerStdDev"].toObject());
    }

    // ---- IFF section --------------------------------------------------------
    if (obj.contains("iff") && obj["iff"].isObject())
    {
        QJsonObject f = obj["iff"].toObject();
        if (f.contains("interrogationMode"))
            cfg.interrogationMode = static_cast<aesa::IFFMode>(f["interrogationMode"].toInt());
        if (f.contains("friendlySquawks"))
        {
            cfg.friendlySquawks.clear();
            for (const QJsonValue& v : f["friendlySquawks"].toArray())
                cfg.friendlySquawks.push_back(static_cast<uint32_t>(v.toInt()));
        }
    }

    // ---- Null steering section ----------------------------------------------
    if (obj.contains("nullSteering") && obj["nullSteering"].isObject())
    {
        QJsonObject n = obj["nullSteering"].toObject();
        if (n.contains("active"))        cfg.nullSteering.active        = n["active"].toBool();
        if (n.contains("azimuth_deg"))   cfg.nullSteering.azimuth_deg   = valueFromParm(n["azimuth_deg"].toObject());
        if (n.contains("elevation_deg")) cfg.nullSteering.elevation_deg = valueFromParm(n["elevation_deg"].toObject());
        if (n.contains("nullDepth_dB"))  cfg.nullSteering.nullDepth_dB  = valueFromParm(n["nullDepth_dB"].toObject());
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    // ---- Mode ---------------------------------------------------------------
    if (obj.contains("mode"))
        cfg.mode = static_cast<aesa::RadarMode>(obj["mode"].toInt());
    if (obj.contains("emitterIndex"))
        emitterIndex = obj["emitterIndex"].toInt();
    // Apply the fully assembled config to the model. REQ-AESA-002.
    radarCore_.setConfig(cfg);
    displayRangeDirty_ = true;
}


