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
#include <cmath>

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

} // anonymous namespace

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
    if (it == root->Platforms.end() || !it->second->transform)
        return pose;

    // Extract world-space position from the platform's transform matrix.
    // Scale from engine units to metres via COORD_SCALE. REQ-AESA-010.
    QVector3D wpos = it->second->transform->matrix->translation();
    pose.x = static_cast<double>(wpos.x()) * COORD_SCALE;
    pose.y = static_cast<double>(wpos.y()) * COORD_SCALE;
    pose.z = static_cast<double>(wpos.z()) * COORD_SCALE;

    // Extract attitude from dynamicModel if available. REQ-AESA-010.
    if (it->second->dynamicModel)
    {
        pose.heading = it->second->dynamicModel->TrueHeading;
        pose.pitch   = it->second->dynamicModel->pitch;
        pose.roll    = it->second->dynamicModel->roll;
    }
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
    Transform*                               source,
    std::unordered_map<uint32_t, Platform*>& outIdMap) const
{
    std::vector<aesa::TargetInput> inputs;
    outIdMap.clear();

    if (!root) return inputs;
    inputs.reserve(root->Platforms.size());

    for (auto& [key, entity] : root->Platforms)
    {
        // Skip own platform and platforms with no transform. REQ-AESA-004.
        if (!entity || key == parentEntity->ID) continue;

        Platform* platform = entity;
        if (!platform->transform) continue;

        // ---- Coordinate transform -------------------------------------------
        // Convert world-space position to radar-local body frame via the
        // own platform's inverse transform, then apply axis remapping and
        // COORD_SCALE. Axis mapping: engine (x,y,z) → model (y,z,x).
        // This matches the identical mapping in radar.cpp. REQ-AESA-004.
        QVector3D localPos =
            source->inverseTransformPoint(platform->transform->matrix->translation());

        aesa::TargetInput t;

        // Engine z → model x (forward), engine x → model y (lateral),
        // engine y → model z (vertical). REQ-AESA-004.
        t.x = static_cast<double>(localPos.z()) * COORD_SCALE;
        t.y = static_cast<double>(localPos.x()) * COORD_SCALE;
        t.z = static_cast<double>(localPos.y()) * COORD_SCALE;

        t.id = platformToRadarID(key);
        outIdMap[t.id] = platform;

        // ---- Velocity estimation --------------------------------------------
        // Velocity is estimated by combining:
        //   1. Finite-differenced heading from consecutive world positions
        //      (provides direction). Position delta threshold of MIN_MOVE_THRESHOLD
        //      prevents heading flicker from floating-point position noise.
        //   2. currentSpeed from dynamicModel (provides magnitude, converted
        //      from km/h to m/s by dividing by 3.6).
        // This is the same pattern as radar.cpp. REQ-AESA-004.
        if (platform->dynamicModel)
        {
            QVector3D worldPos = platform->transform->matrix->translation();
            uint32_t  tid      = platformToRadarID(key);

            if (prevPositions_.count(tid))
            {
                QVector3D delta = worldPos - prevPositions_[tid];
                float dist = std::sqrt(delta.x()*delta.x() + delta.z()*delta.z());
                if (dist > MIN_MOVE_THRESHOLD)
                {
                    // Compute heading from the horizontal displacement vector.
                    // atan2(x, z) gives heading in engine's horizontal plane.
                    float hdg = std::atan2(delta.x(), delta.z()) * (180.0f / M_PI);
                    if (hdg < 0.0f) hdg += 360.0f;
                    computedHeadings_[tid] = hdg;
                }
            }
            prevPositions_[tid] = worldPos;

            float hdg = computedHeadings_.count(tid) ? computedHeadings_[tid] : 0.0f;
            velocityFromHeadingSpeed(
                static_cast<double>(hdg),
                static_cast<double>(platform->dynamicModel->currentSpeed) / 3.6,
                t.vx, t.vy, t.vz);
        }
        else
        {
            // No dynamic model — treat as stationary. REQ-AESA-004.
            t.vx = t.vy = t.vz = 0.0;
        }

        // ---- Physical dimensions --------------------------------------------
        // Read collider dimensions for the 6-facet Physical Optics RCS model.
        // Material and shape default to METAL / GENERIC — future versions can
        // derive these from platform->type or profile. REQ-AESA-040.
        if (platform->collider)
        {
            t.dimensions.length = static_cast<double>(platform->collider->Length);
            t.dimensions.height = static_cast<double>(platform->collider->Height);
            t.dimensions.width  = static_cast<double>(platform->collider->Width);
            t.dimensions.valid  = true;
            t.dimensions.material = aesa::TargetMaterialType::METAL;
            t.dimensions.shape    = aesa::TargetShapeType::GENERIC;
        }

        // ---- Target descriptor defaults -------------------------------------
        // All AESA bridge targets are treated as AIR domain generic targets
        // with Swerling Case I fluctuation. Jammer inactive by default.
        // REQ-AESA-040.
        t.rcs          = 1.0;       // m² — fallback if dimensions.valid is false
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
// OUTPUT TRANSLATION
// =============================================================================

// =============================================================================
// FUNCTION:    AESARadar::processSurveillance
// (Full description in header)
// =============================================================================
void AESARadar::processSurveillance(
    const aesa::RadarOutput&                        output,
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

        // Azimuth: model body frame is [-180, +180]. Engine convention same.
        // Clamp values that arrived >180 (rare float edge case). REQ-AESA-004.
        float az = static_cast<float>(d.azimuth);
        if (az > 180.0f) az -= 360.0f;
        t.angle = az;

        t.speed          = static_cast<float>(d.speedOverGround);
        t.direction      = static_cast<float>(d.heading);

        // Altitude: add radarHeight so the display shows absolute altitude
        // rather than altitude relative to the radar. REQ-AESA-004.
        t.altitude = static_cast<float>(
            d.range * std::sin(d.elevation * M_PI / 180.0)
            + radarCore_.getConfig().radarHeight);

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

        // Azimuth conversion — same as processSurveillance. REQ-AESA-004.
        float bodyAz = static_cast<float>(tr.azimuth);
        if (bodyAz > 180.0f) bodyAz -= 360.0f;
        t.angle = bodyAz;

        t.speed          = static_cast<float>(tr.speedOverGround);
        t.direction      = static_cast<float>(tr.heading);

        // Altitude: Kalman z state plus radar platform height. REQ-AESA-004.
        t.altitude = static_cast<float>(tr.z + radarCore_.getConfig().radarHeight);

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
    const std::unordered_map<uint32_t, Platform*>&  idMap)
{
    // Break-lock condition — model has lost the target. REQ-AESA-003.
    if (output.lockBroken)
    {
        qDebug() << "[AESARadar] Lock broken";
        return;
    }

    if (output.tracks.empty()) return;

    uint32_t lockedID = radarCore_.getConfig().lockedTargetID;

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

    // Azimuth conversion. REQ-AESA-004.
    float lockAz = static_cast<float>(locked->azimuth);
    if (lockAz > 180.0f) lockAz -= 360.0f;
    t.angle = lockAz;

    t.speed          = static_cast<float>(locked->speedOverGround);
    t.direction      = static_cast<float>(locked->heading);
    t.altitude       = static_cast<float>(locked->z + radarCore_.getConfig().radarHeight);
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

    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    // -------------------------------------------------------------------------
    // dt computation — identical pattern to radar.cpp.
    // First call uses FIRST_FRAME_DT to avoid a zero initial step.
    // Subsequent calls measure real elapsed time and clamp to [MIN_DT, MAX_DT].
    // REQ-AESA-001.
    // -------------------------------------------------------------------------
    double dt;
    if (!timerStarted_)
    {
        frameTimer_.start();
        timerStarted_ = true;
        dt = FIRST_FRAME_DT;
    }
    else
    {
        dt = static_cast<double>(frameTimer_.elapsed()) / 1000.0;
        frameTimer_.restart();
        dt = std::clamp(dt, MIN_DT, MAX_DT);
    }
    simTime_ += dt;

    // -------------------------------------------------------------------------
    // Pose — build from current platform transform. REQ-AESA-010.
    // -------------------------------------------------------------------------
    aesa::RadarPose pose = buildPose();

    // -------------------------------------------------------------------------
    // Update radarHeight from live pose altitude.
    // Only applied when pose.y exceeds MIN_RADAR_HEIGHT and the change
    // exceeds HEIGHT_UPDATE_THRESHOLD to avoid redundant setConfig() calls.
    // REQ-AESA-071.
    // -------------------------------------------------------------------------
    if (pose.y > MIN_RADAR_HEIGHT)
    {
        aesa::RadarConfig altCfg = radarCore_.getConfig();
        if (std::abs(pose.y - altCfg.radarHeight) > HEIGHT_UPDATE_THRESHOLD)
        {
            altCfg.radarHeight = pose.y;
            radarCore_.setConfig(altCfg);
            displayRangeDirty_ = true;
        }
    }

    // {
    //     //     aesa::RadarConfig altCfg = radarCore_.getConfig();
    //     //     double liveHeight = std::max(2.0, pose.y);
    //     //     if (std::abs(liveHeight - altCfg.radarHeight) > 1.0)
    //     //     {
    //     //         altCfg.radarHeight = liveHeight;
    //     //         radarCore_.setConfig(altCfg);
    //     //         displayRangeDirty_ = true;
    //     //     }
    //     // }

    // -------------------------------------------------------------------------
    // Update platformSpeed_m_s from own dynamicModel each tick.
    // This feeds the clutter notch and Doppler blind zone computation inside
    // the signal processor. FIX-01. REQ-AESA-040.
    // -------------------------------------------------------------------------
    {
        auto it = root->Platforms.find(parentEntity->ID);
        if (it != root->Platforms.end() && it->second->dynamicModel)
        {
            aesa::RadarConfig speedCfg = radarCore_.getConfig();
            speedCfg.platformSpeed_m_s =
                static_cast<float>(it->second->dynamicModel->currentSpeed / 3.6);
            radarCore_.setConfig(speedCfg);
        }
    }

    // -------------------------------------------------------------------------
    // Collect targets and drive the model update. REQ-AESA-004.
    // -------------------------------------------------------------------------
    std::unordered_map<uint32_t, Platform*> idMap;
    std::vector<aesa::TargetInput> radarInputs = collectTargets(source, idMap);

    radarCore_.update(dt, pose, radarInputs, simTime_);

    aesa::RadarOutput output = radarCore_.getOutput();
    aesa::RadarConfig cfg    = radarCore_.getConfig();

    // -------------------------------------------------------------------------
    // Sync Sensor base fields from model output. REQ-AESA-004.
    // -------------------------------------------------------------------------
    azimuth           = static_cast<float>(output.currentAzimuth);
    beamWidth         = cfg.beamWidth;
    maxDetectionAngle = cfg.maxAzimuth;

    cachedDisplayRange_ = std::clamp(
        static_cast<float>(output.displayRange_km),
        DISPLAY_RANGE_MIN_KM,
        DISPLAY_RANGE_MAX_KM);
    range = cachedDisplayRange_;

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
        processSurveillance(output, idMap, addedIDs);
        break;
    case aesa::RadarMode::TWS:
        processTWS(output, idMap, addedIDs);
        break;
    case aesa::RadarMode::LOCK_ON:
        processLockOn(output, idMap);
        break;
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
    obj["SensorType"] = "AESARadar";

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
    detection["minDetectableRange"]  = toParm(cfg.minDetectableRange,  "m",    0.0f, 1000.0f, "Min detectable range");
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
    propagation["temperature_C"]     = toParm(cfg.atmosphere.temperature_C,   "°C",  -60.0f,   60.0f, "Ambient temperature");
    propagation["humidity_pct"]      = toParm(cfg.atmosphere.humidity_pct,    "%",     0.0f,  100.0f, "Relative humidity");
    propagation["pressure_hPa"]      = toParm(cfg.atmosphere.pressure_hPa,    "hPa", 800.0f, 1100.0f, "Atmospheric pressure");
    propagation["rainRate_mmph"]     = toParm(cfg.atmosphere.rainRate_mmph,   "mm/h",  0.0f,  200.0f, "Rain rate");
    propagation["fogVisibility_m"]   = toParm(cfg.atmosphere.fogVisibility_m, "m",     0.0f, 10000.0f, "Fog visibility");
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
    QJsonObject iff;
    iff["type"]              = "Section";
    iff["interrogationMode"] = static_cast<int>(cfg.interrogationMode);
    QJsonArray squawks;
    for (uint32_t sq : cfg.friendlySquawks) squawks.append(static_cast<int>(sq));
    iff["friendlySquawks"]   = squawks;
    obj["iff"] = iff;

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

    return obj;
}

// =============================================================================
// FUNCTION:    AESARadar::fromJson
// (Full description in header)
// =============================================================================
void AESARadar::fromJson(const QJsonObject& obj)
{
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
        if (p.contains("minDetectableRange")) cfg.minDetectableRange = valueFromParm(p["minDetectableRange"].toObject());
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
        if (p.contains("temperature_C"))     cfg.atmosphere.temperature_C     = valueFromParm(p["temperature_C"].toObject());
        if (p.contains("humidity_pct"))      cfg.atmosphere.humidity_pct      = valueFromParm(p["humidity_pct"].toObject());
        if (p.contains("pressure_hPa"))      cfg.atmosphere.pressure_hPa      = valueFromParm(p["pressure_hPa"].toObject());
        if (p.contains("rainRate_mmph"))     cfg.atmosphere.rainRate_mmph     = valueFromParm(p["rainRate_mmph"].toObject());
        if (p.contains("fogVisibility_m"))   cfg.atmosphere.fogVisibility_m   = valueFromParm(p["fogVisibility_m"].toObject());
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

    // ---- Mode ---------------------------------------------------------------
    if (obj.contains("mode"))
        cfg.mode = static_cast<aesa::RadarMode>(obj["mode"].toInt());

    // Apply the fully assembled config to the model. REQ-AESA-002.
    radarCore_.setConfig(cfg);
    displayRangeDirty_ = true;
}



