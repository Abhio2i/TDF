// =============================================================================
// FILE:         radarmodel_aesa.h
// MODULE:       AESA Radar Simulation Model — Public API and Data Types
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Defines all public data types, structures, enumerations, and
//               the RadarModel_AESA class interface for the AESA radar
//               simulation model. This header is the single point of truth
//               for the data contract between the radar model and all
//               consuming systems (AESARadar bridge, test suite, UI layer).
//
//               The model simulates a full Active Electronically Scanned
//               Array radar including:
//                 - Electronic beam steering with spoiling (REQ-AESA-010/013)
//                 - PRF / waveform management (REQ-AESA-020)
//                 - Kalman / IMM tracking with JPDA (REQ-AESA-030)
//                 - SINR / CFAR detection (REQ-AESA-040)
//                 - IFF interrogation (REQ-AESA-050)
//                 - DRFM / RGPO / VGPO electronic warfare (REQ-AESA-060)
//                 - Chaff clutter modelling (REQ-AESA-061)
//                 - ITU-R P.526-15 knife-edge occlusion (REQ-AESA-070)
//                 - ITU-R P.676-12 gaseous attenuation (REQ-AESA-071)
//                 - Two-ray multipath (REQ-AESA-072)
//                 - Staggered PRF range/velocity ambiguity resolution
//                   (REQ-AESA-021)
//
// THREAD SAFETY: RadarModel_AESA is thread-safe. All public methods acquire
//                mutex_ before accessing shared state. Do not call public
//                methods from within a callback that already holds mutex_ —
//                that would deadlock.
//
// REQUIREMENTS: REQ-AESA-001  System initialisation and lifecycle
//               REQ-AESA-002  Configuration management
//               REQ-AESA-003  Mode control (SURVEILLANCE / TWS / LOCK_ON)
//               REQ-AESA-004  Output assembly and publication
//               REQ-AESA-010  Antenna beam steering
//               REQ-AESA-020  PRF and waveform management
//               REQ-AESA-021  Staggered PRF ambiguity resolution
//               REQ-AESA-030  Multi-target tracking (Kalman / IMM / JPDA)
//               REQ-AESA-040  SINR / CFAR detection pipeline
//               REQ-AESA-050  IFF interrogation and response
//               REQ-AESA-060  Electronic warfare — DRFM / RGPO / VGPO
//               REQ-AESA-061  Chaff cloud clutter modelling
//               REQ-AESA-070  ITU-R P.526-15 occlusion model
//               REQ-AESA-071  ITU-R P.676-12 propagation loss
//               REQ-AESA-072  Two-ray multipath factor
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-MODEL-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic surveillance model.
//   Rev 2  15 Feb 2026  Added TWS, JPDA, IFF, DRFM support.
//   Rev 3  01 Apr 2026  13 audit fixes applied. Staggered PRF, RGPO/VGPO,
//                       occlusion model, ITU-R propagation, IMM tracker added.
//                       Commented-out code removed per NS-05.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Magic numbers replaced with named constants where
//                       possible within the header scope.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARMODEL_AESA_H
#define RADARMODEL_AESA_H

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace aesa {

// =============================================================================
// SECTION 1: ENUMERATIONS
//
// All enumerations use enum class to provide strong typing and prevent
// implicit integer conversion (TS-08 compliance). Underlying type is not
// specified here — a known DO-178C deviation documented in
// ICD-AESA-DEVIATION-001. Rationale: Qt's meta-object system requires
// enums compatible with int for signal/slot parameter passing.
// REQ-AESA-002.
// =============================================================================

// -----------------------------------------------------------------------------
// RadarMode: operational mode of the radar.
// SURVEILLANCE — wide-area search, no tracking. Detections published per scan.
// TWS          — track-while-scan. Kalman tracker runs. Validated tracks
//                published at scan boundary. REQ-AESA-003.
// LOCK_ON      — fire-control mode. All beam time on one target. Kalman state
//                published every tick for minimum fire-control latency.
// -----------------------------------------------------------------------------
enum class RadarMode { SURVEILLANCE, TWS, LOCK_ON };

// -----------------------------------------------------------------------------
// SurfaceType: target medium. Affects clutter model selection in SINR
// computation and CFAR threshold generation. REQ-AESA-040.
// -----------------------------------------------------------------------------
enum class SurfaceType { AIR, SEA, LAND };

// -----------------------------------------------------------------------------
// ModulationType: pulse compression waveform type.
// NONE — unmodulated pulse. No processing gain.
// LFM  — Linear Frequency Modulation. Processing gain = BW * tau.
// NLFM — Non-Linear FM. Better sidelobe control than LFM.
// FMCW — Frequency Modulated Continuous Wave. Used for short range.
// REQ-AESA-020.
// -----------------------------------------------------------------------------
enum class ModulationType { NONE, LFM, NLFM, FMCW };

// -----------------------------------------------------------------------------
// PRFType: pulse repetition frequency management strategy. REQ-AESA-020.
// FIXED     — single PRF, Rmax fixed.
// STAGGERED — two interleaved PRFs for range ambiguity resolution.
// JITTERED  — random PRF variation for ECCM.
// SWITCHED  — mode-driven PRF switching.
// -----------------------------------------------------------------------------
enum class PRFType { FIXED, STAGGERED, JITTERED, SWITCHED };

// -----------------------------------------------------------------------------
// DetectionCategory: target surface filter applied before detection pipeline.
// ALL          — detect all targets regardless of surface type.
// AIR_ONLY     — reject SEA and LAND targets before processing.
// SURFACE_ONLY — reject AIR targets before processing.
// REQ-AESA-040.
// -----------------------------------------------------------------------------
enum class DetectionCategory { ALL, AIR_ONLY, SURFACE_ONLY };

// -----------------------------------------------------------------------------
// SidelobeMode: antenna sidelobe level control mode. REQ-AESA-010.
// NORMAL    — default aperture weighting, uses peakSidelobeLevel / avgSidelobeLevel.
// LOW_SLL   — Taylor weighting applied, -45 dB peak / -55 dB average.
// ULTRA_LOW — ultra-low sidelobe aperture, -55 dB peak / -65 dB average.
// -----------------------------------------------------------------------------
enum class SidelobeMode { NORMAL, LOW_SLL, ULTRA_LOW };

// -----------------------------------------------------------------------------
// JammerType: electronic attack technique. REQ-AESA-060.
// NOISE           — broadband noise jamming. Raises noise floor in SINR.
// DRFM            — Digital RF Memory. Coherent gate stealing + pull-off.
// STAND_OFF_NOISE — noise jamming from a separate platform (escort/stand-off).
// -----------------------------------------------------------------------------
enum class JammerType { NOISE, DRFM, STAND_OFF_NOISE };

// -----------------------------------------------------------------------------
// SwerlingCase: target RCS fluctuation model. REQ-AESA-040.
// CASE_0   — non-fluctuating (Marcum). Deterministic RCS.
// CASE_I   — scan-to-scan Rayleigh fluctuation (exponential PDF).
// CASE_II  — pulse-to-pulse Rayleigh fluctuation.
// CASE_III — scan-to-scan chi-squared (2 degrees of freedom).
// CASE_IV  — pulse-to-pulse chi-squared.
// -----------------------------------------------------------------------------
enum class SwerlingCase { CASE_0, CASE_I, CASE_II, CASE_III, CASE_IV };

// -----------------------------------------------------------------------------
// WaveformMode: PRF regime classification. REQ-AESA-020.
// HPRF — High PRF (>= 100 kHz). Velocity unambiguous, range ambiguous.
// MPRF — Medium PRF. Both range and velocity may be ambiguous.
// LPRF — Low PRF (<= 1 kHz). Range unambiguous, velocity ambiguous.
// AUTO — Signal processor selects regime based on target range.
// -----------------------------------------------------------------------------
enum class WaveformMode { HPRF, MPRF, LPRF, AUTO };

// -----------------------------------------------------------------------------
// IFFMode: Identification Friend or Foe interrogation mode. REQ-AESA-050.
// OFF     — IFF disabled. No interrogations transmitted.
// MODE_3A — ICAO Mode 3/A civil aviation squawk code.
// MODE_4  — NATO Mode 4 encrypted military IFF.
// MODE_5  — NATO Mode 5 enhanced encrypted military IFF.
// -----------------------------------------------------------------------------
enum class IFFMode { OFF, MODE_3A, MODE_4, MODE_5 };

// -----------------------------------------------------------------------------
// IFFResponseCode: result of an IFF interrogation. REQ-AESA-050.
// NO_REPLY  — target did not respond within the reply window.
// FRIENDLY  — squawk matched a code in config_.friendlySquawks.
// UNKNOWN   — squawk received but not in friendly list.
// HOSTILE   — reserved for future use (active hostile classification).
// CORRUPTED — response received but failed integrity check.
// -----------------------------------------------------------------------------
enum class IFFResponseCode { NO_REPLY, FRIENDLY, UNKNOWN, HOSTILE, CORRUPTED };

// =============================================================================
// SECTION 2: CORE DATA STRUCTURES
// =============================================================================

// -----------------------------------------------------------------------------
// OcclusionResult: output of ITU-R P.526-15 knife-edge diffraction computation.
// Represents how much a candidate target is shadowed by an intervening platform.
// REQ-AESA-070.
//
// zone              — three-state shadow classification:
//                     LIT      — no significant diffraction loss (< 6 dB)
//                     PENUMBRA — partial shadow (6–40 dB loss). Target is
//                                attenuated but may still be detectable.
//                     SHADOW   — fully shadowed (>= 40 dB loss). Target
//                                removed from detection pipeline entirely.
// powerReduction    — linear power scaling factor [0.0, 1.0].
//                     1.0 = full power (LIT), 0.0 = fully blocked (SHADOW).
//                     Applied to effective RCS before SINR computation.
// diffractionLoss_dB — two-way total diffraction loss in dB. Summed over all
//                      occluding platforms between radar and target.
// -----------------------------------------------------------------------------
struct OcclusionResult
{
    enum class Zone { LIT, PENUMBRA, SHADOW };

    // Default: LIT — no occlusion. Safe to use as initial value before
    // computeOcclusion() is called. REQ-AESA-070.
    Zone   zone               = Zone::LIT;

    // Linear power reduction factor. 1.0 = no reduction (fully lit).
    // Applied as: effRCS *= powerReduction. REQ-AESA-070.
    double powerReduction     = 1.0;

    // Two-way diffraction loss in dB. 0.0 = no loss (fully lit).
    // Informational — used for debug output and occlusion zone classification.
    double diffractionLoss_dB = 0.0;
};

// -----------------------------------------------------------------------------
// NullSteering: adaptive null placement parameters for jammer suppression.
// When active, a deep null is steered toward the jammer direction, reducing
// jammer power at the receiver by nullDepth_dB. REQ-AESA-040.
// -----------------------------------------------------------------------------
struct NullSteering
{
    // true = null steering is active this tick. false = normal aperture.
    bool   active        = false;

    // Azimuth of the null direction (degrees, body frame).
    double azimuth_deg   = 0.0;

    // Elevation of the null direction (degrees, body frame).
    double elevation_deg = 0.0;

    // Null depth in dB (negative value, e.g. -30 dB).
    // Jammer power is multiplied by pow(10, nullDepth_dB/10) when in null.
    float  nullDepth_dB  = -30.0f;
};

// -----------------------------------------------------------------------------
// BeamWaveform: complete waveform descriptor for one beam dwell.
// Defines the pulse parameters used during a specific beam position.
// Used by signal processor for processing gain, Rmax, and SINR computation.
// REQ-AESA-020.
// -----------------------------------------------------------------------------
struct BeamWaveform
{
    // Pulse compression modulation type. Determines processing gain formula.
    ModulationType modulation     = ModulationType::LFM;

    // Pulse width in seconds. Range: 1e-7 to 1e-3 s (100 ns to 1 ms).
    // Processing gain (LFM/NLFM) = bandwidth_Hz * pulseWidth_s.
    float          pulseWidth_s   = 50e-6f;

    // Primary pulse repetition frequency in Hz.
    // Rmax = SPEED_OF_LIGHT / (2 * prf_Hz). Range: 100 to 100000 Hz.
    float          prf_Hz         = 300.0f;

    // Instantaneous bandwidth in Hz. Determines range resolution and
    // pulse compression gain. Range: 1e4 to 1e8 Hz.
    float          bandwidth_Hz   = 5e6f;

    // Number of pulses integrated per beam dwell position.
    // Integration gain = pulsesPerDwell (non-coherent) applied in SINR.
    int            pulsesPerDwell = 10;

    // PRF regime classification. Used by Doppler blind zone calculation.
    WaveformMode   mode           = WaveformMode::AUTO;

    // Secondary PRF for staggered PRF mode (Hz). 0.0 = staggered mode disabled.
    // When set, Rmax2 = SPEED_OF_LIGHT / (2 * prf2_Hz) is computed and the
    // coincidence detector resolves range ambiguity. REQ-AESA-021.
    float          prf2_Hz        = 0.0f;
};

// -----------------------------------------------------------------------------
// WaveformEntry: one entry in the range-keyed waveform selection table.
// The signal processor selects the waveform whose maxRange_m first exceeds
// the target range. Entries must be sorted ascending by maxRange_m.
// Sentinel: maxRange_m = 0.0 marks end of table. REQ-AESA-020.
// -----------------------------------------------------------------------------
struct WaveformEntry
{
    // Upper range bound for this waveform (metres). 0.0 = sentinel (end of table).
    float        maxRange_m = 0.0f;

    // Waveform to use when target range < maxRange_m.
    BeamWaveform waveform;
};

// -----------------------------------------------------------------------------
// BeamRequest: complete descriptor for one beam dwell position.
// Generated by RadarScheduler and consumed by RadarModel_AESA::update().
// REQ-AESA-010, REQ-AESA-020.
// -----------------------------------------------------------------------------
struct BeamRequest
{
    // Task classification. Determines dwell time allocation and output cadence.
    enum class Task
    {
        SEARCH,         // Wide-area surveillance beam. Low priority.
        TRACK,          // Track maintenance beam. Medium priority.
        FIRE_CONTROL,   // Fire control quality illumination. Highest priority.
        HORIZON_SEARCH  // Low elevation search for cruise missile threats.
    };

    // Task type for this dwell. REQ-AESA-010.
    Task         task           = Task::SEARCH;

    // Commanded beam azimuth (degrees, body frame). Range: [-180, +180].
    double       azimuth_deg    = 0.0;

    // Commanded beam elevation (degrees, body frame).
    double       elevation_deg  = 0.0;

    // Time the beam dwells at this position (milliseconds).
    // Must be > 0. Scheduler enforces duty cycle budget. REQ-AESA-020.
    double       dwellTime_ms   = 2.0;

    // Target ID this beam is assigned to track (0 = no specific target).
    // Set for TRACK and FIRE_CONTROL tasks. REQ-AESA-030.
    uint32_t     targetID       = 0;

    // Beam priority. Higher value = higher scheduling priority.
    // FIRE_CONTROL = 100, TRACK manoeuvring = 20, TRACK normal = 10,
    // SEARCH = 0. REQ-AESA-010.
    int          priority       = 0;

    // Waveform parameters for this dwell. REQ-AESA-020.
    BeamWaveform waveform;

    // Beam spoiling factor for this dwell (dimensionless, >= 1.0).
    // 1.0 = no spoiling (pencil beam). > 1.0 = widened beam for volume search.
    // REQ-AESA-013.
    float        spoilFactor    = 1.0f;
};

// =============================================================================
// SECTION 3: NOISE AND JAMMER MODELS
// =============================================================================

// -----------------------------------------------------------------------------
// NoiseModel: measurement noise standard deviations applied to detections.
// All values in SI units. Zero values = noise-free (ideal sensor).
// Used to generate normally-distributed measurement errors on each detection.
// REQ-AESA-040.
// -----------------------------------------------------------------------------
struct NoiseModel
{
    // Range measurement noise standard deviation (metres).
    double rangeStdDev     = 30.0;

    // Azimuth measurement noise standard deviation (degrees).
    double azimuthStdDev   = 0.1;

    // Elevation measurement noise standard deviation (degrees).
    double elevationStdDev = 0.1;

    // Doppler / radial velocity measurement noise standard deviation (m/s).
    double dopplerStdDev   = 1.0;
};

// -----------------------------------------------------------------------------
// JammerConfig: electronic attack configuration for one target platform.
// Defines the type, power, and technique-specific parameters of any
// self-protection or escort jammer fitted to a TargetInput.
// REQ-AESA-060.
// -----------------------------------------------------------------------------
struct JammerConfig
{
    // true = jammer is active this tick. false = no jamming.
    bool       active        = false;

    // Jammer technique. Determines which EW injection path is taken.
    // REQ-AESA-060: DRFM triggers injectDRFMGhost().
    JammerType type          = JammerType::NOISE;

    // Jammer transmit power (kilowatts). Must be >= 0.
    double     power_kW      = 0.0;

    // Jammer antenna gain (dBi).
    double     gain_dBi      = 0.0;

    // Jammer instantaneous bandwidth (Hz). Used to compute jamming efficiency
    // ratio B_receiver / B_jammer — wider jammer bandwidth dilutes power.
    double     bandwidth_Hz  = 1e6;

    // Jammer platform range from radar (metres). Used when selfScreening=false.
    // 0.0 = use target range as default. REQ-AESA-060.
    double     range_m       = 0.0;

    // true = self-screening jammer (co-located with target).
    // false = stand-off or escort jammer at range_m from radar.
    bool       selfScreening = false;

    // ---- DRFM-specific parameters (only used when type == DRFM) ------------

    // Rate at which DRFM pulls the false gate away from real target (m/s).
    // Applied every tick: drfmPullOff_ += drfmPullOffRate_m_s * dt.
    float  drfmPullOffRate_m_s    = 150.0f;

    // Fixed velocity offset added to the DRFM ghost detection (m/s).
    float  drfmVelocityOffset_m_s = 50.0f;

    // true = gate stealing is active. DRFM ghost injection occurs only
    // when gateStealingActive is true. REQ-AESA-060.
    bool   gateStealingActive     = false;

    // Accumulated pull-off distance (metres). Maintained externally in
    // drfmPullOff_ map — this field is informational only.
    double pullOffDistance_m      = 0.0;

    // ---- RGPO parameters (Range Gate Pull-Off) ------------------------------

    // true = RGPO technique active. Creates false range target. REQ-AESA-060.
    bool   rgpoActive          = false;

    // Rate at which RGPO false target walks away in range (m/s).
    float  rgpoRate_m_s        = 200.0f;

    // Maximum range offset before RGPO pull-off saturates (metres).
    float  rgpoMaxOffset_m     = 5000.0f;

    // ---- VGPO parameters (Velocity Gate Pull-Off) ---------------------------

    // true = VGPO technique active. Creates false Doppler target. REQ-AESA-060.
    bool   vgpoActive          = false;

    // Rate at which VGPO false Doppler walks away from true velocity (m/s^2).
    float  vgpoRate_m_s2       = 50.0f;

    // Maximum velocity offset before VGPO pull-off saturates (m/s).
    float  vgpoMaxOffset_m_s   = 300.0f;

    // ---- Additional modulation flags ----------------------------------------

    // true = DRFM return has amplitude noise modulation applied.
    bool   noiseModulation     = false;

    // Additional strobe power above noise floor (dB). 0 = no strobe effect.
    float  jamStrobe_dB        = 0.0f;
};

// =============================================================================
// SECTION 4: CHAFF CLOUD MODEL
// REQ-AESA-061
// =============================================================================

// -----------------------------------------------------------------------------
// ChaffCloud: one deployed chaff cloud instance.
// Chaff is modelled as a spherical volume of radar reflectors with an
// exponentially decaying RCS. Multiple simultaneous clouds are supported.
// REQ-AESA-061.
// -----------------------------------------------------------------------------
struct ChaffCloud
{
    // Position of cloud centre in radar-local coordinates (metres).
    double   x = 0.0, y = 0.0, z = 0.0;

    // Radius of the chaff cloud (metres). Used for beam intersection check.
    double   radius_m    = 200.0;

    // Initial total RCS of the cloud at birth time (m²).
    // Decays exponentially: rcsNow = rcsTotal * exp(-age / decayTime_s).
    double   rcsTotal    = 1000.0;

    // Time constant for RCS decay (seconds). Larger = slower decay.
    double   decayTime_s = 60.0;

    // Simulation time at which this cloud was deployed (seconds).
    // Used to compute cloud age: age = simTime - birthTime_s.
    double   birthTime_s = 0.0;

    // ID of the platform that deployed this cloud. Informational only.
    uint32_t sourceID    = 0;
};

// =============================================================================
// SECTION 5: IFF RESULT
// REQ-AESA-050
// =============================================================================

// -----------------------------------------------------------------------------
// IFFResult: result of one IFF interrogation cycle for a specific track.
// Populated by queryIFF() and stored in TrackFile::iff and TrackOutput::iff.
// REQ-AESA-050.
// -----------------------------------------------------------------------------
struct IFFResult
{
    // Classification result of the interrogation. Default: NO_REPLY.
    IFFResponseCode response   = IFFResponseCode::NO_REPLY;

    // Squawk code received from the target. 0 = no reply or mode 4/5 encrypted.
    uint32_t        squawk     = 0;

    // Confidence in the classification [0.0, 1.0].
    // FRIENDLY = 0.95, UNKNOWN = 0.70, NO_REPLY = 0.0.
    double          confidence = 0.0;

    // IFF mode used for this interrogation cycle.
    IFFMode         modeUsed   = IFFMode::OFF;
};

// =============================================================================
// SECTION 5b: ATMOSPHERIC CONDITIONS
// REQ-AESA-071
// =============================================================================

// -----------------------------------------------------------------------------
// AtmosphericConditions: weather parameters for propagation loss computation.
// Used by computeGaseousAttenuation() (ITU-R P.676-12), computePropagationLoss()
// rain model (ITU-R P.838-3), and fog model (Kunkel 1984).
// REQ-AESA-071.
// -----------------------------------------------------------------------------
struct AtmosphericConditions
{
    // Ambient air temperature (degrees Celsius). ISA standard = 15.0.
    // Valid range: -60 to +60 deg C. REQ-AESA-071.
    float temperature_C   = 15.0f;

    // Relative humidity (percent). Midlatitude standard = 60%.
    // Valid range: 0 to 100%. REQ-AESA-071.
    float humidity_pct    = 60.0f;

    // Atmospheric pressure at radar altitude (hPa). Sea level = 1013.25 hPa.
    // Valid range: 800 to 1100 hPa. REQ-AESA-071.
    float pressure_hPa    = 1013.25f;

    // Rain rate (mm/h). 0 = clear, 4 = light, 16 = moderate, 100 = heavy.
    // Used in ITU-R P.838-3 rain attenuation model. REQ-AESA-071.
    float rainRate_mmph   = 0.0f;

    // Fog visibility range (metres). 0 = clear, < 1000 = fog, < 200 = dense fog.
    // Used in Kunkel (1984) fog attenuation model. REQ-AESA-071.
    // Values outside [1, 2000] are treated as clear (no fog attenuation).
    float fogVisibility_m = 0.0f;
};

// =============================================================================
// SECTION 6: RADAR CONFIGURATION
// REQ-AESA-002
// =============================================================================

// -----------------------------------------------------------------------------
// RadarConfig: complete configuration for one AESA radar instance.
// Passed to RadarModel_AESA::init() and updated via setConfig().
// All fields have safe default values — the model is functional with a
// default-constructed RadarConfig, though performance will not match any
// specific real-world system without calibration.
//
// CAUTION: RadarConfig contains std::vector (friendlySquawks) which involves
// heap allocation. This is a known MM-01 deviation documented in
// ICD-AESA-DEVIATION-002. Mitigation: friendlySquawks is populated only at
// init/config time, never during the operational update() loop.
// REQ-AESA-002.
// -----------------------------------------------------------------------------
struct RadarConfig
{
    // ---- Propagation model parameters ---------------------------------------

    // Earth radius scaling factor for horizon calculation.
    // 1.33 = 4/3 earth radius model (standard atmospheric refraction).
    // REQ-AESA-071.
    double earthRadiusFactor  = 1.33;

    // Atmospheric refraction multiplier (dimensionless).
    // 1.0 = standard atmosphere. REQ-AESA-071.
    double atmosphericFactor  = 1.0;

    // All weather conditions consolidated in AtmosphericConditions struct.
    // Replaces legacy scalar fields rainRate_mmph, fogVisibility_m.
    // REQ-AESA-071.
    AtmosphericConditions atmosphere;

    // ---- Clutter environment ------------------------------------------------

    // Douglas sea state (0–6). Used in GIT sea clutter model (Horst et al. 1978).
    // 0 = calm, 6 = very rough. REQ-AESA-040.
    float  seaState    = 2.0f;

    // Land clutter factor (0.0–1.0). Used in Billingsley terrain model.
    // 0.0 = smooth/low reflectivity terrain. 1.0 = urban/high reflectivity.
    // REQ-AESA-040.
    float  landClutter = 0.0f;

    // ---- AESA array parameters ----------------------------------------------

    // Total number of transmit/receive modules in the array.
    // Gain and power scale with this value. Must be > 0. REQ-AESA-012.
    int   numElements           = 1000;

    // Peak RF output power per T/R module (Watts).
    // Total peak power = numElements * peakPowerPerElement_W * moduleEfficiency.
    float peakPowerPerElement_W = 10.0f;

    // T/R module power conversion efficiency (dimensionless, 0.0–1.0).
    // Accounts for ohmic losses and DC-to-RF conversion efficiency.
    float moduleEfficiency      = 0.7f;

    // Number of T/R modules that have failed and produce no output.
    // Must be <= numElements. Gain degrades as (active/total)^2.
    // REQ-AESA-012.
    int   failedModules         = 0;

    // Maximum duty cycle (fraction, 0.0–1.0). Scheduler degrades waveform
    // PRF if any beam's duty cycle would exceed this limit. REQ-AESA-020.
    float maxDutyCycle          = 0.25f;

    // ---- Antenna and beam parameters ----------------------------------------

    // Operating frequency (Hz). Used in wavelength, Doppler, and propagation
    // calculations. REQ-AESA-020.
    double frequency_Hz          = 10.0e9;

    // Peak boresight antenna gain (dBi). Linear: G = 10^(antennaGain/10).
    // Represents the fully phased array at boresight with no spoiling.
    // REQ-AESA-012.
    float  antennaGain           = 34.0f;

    // Receiver instantaneous bandwidth (Hz). Used in noise power computation:
    // Pn = k * T * B * F. REQ-AESA-040.
    double antennaBandwidth      = 100e6;

    // Natural (unspoiled) half-power beamwidth (degrees).
    // Effective beamwidth after spoiling = beamWidth * spoilFactor.
    // REQ-AESA-010.
    float  beamWidth             = 2.0f;

    // Maximum electronic steering angle from boresight (degrees).
    // Defines the usable FoV half-angle. Typically 60 degrees for AESA.
    // REQ-AESA-011.
    float  maxSteeringAngle_deg  = 60.0f;

    // ---- Sidelobe control ---------------------------------------------------

    // Sidelobe control mode. Determines which SLL values are applied.
    // REQ-AESA-010.
    SidelobeMode sidelobeMode   = SidelobeMode::NORMAL;

    // Peak sidelobe level (dBi, negative). Used when sidelobeMode = NORMAL.
    float peakSidelobeLevel     = -40.0f;

    // Average sidelobe level (dBi, negative). Used when sidelobeMode = NORMAL.
    float avgSidelobeLevel      = -50.0f;

    // Sidelobe blanking threshold (dB). If a jammer in the sidelobe region
    // exceeds this level above noise, the detection is blanked. REQ-AESA-040.
    float sidelobeBlanking_dB   = -15.0f;

    // Adaptive null steering parameters. Steers a deep null toward a jammer.
    NullSteering nullSteering;

    // ---- Field of view limits -----------------------------------------------

    // Elevation FoV lower bound (degrees). Typically negative for depression.
    float minElevation = -10.0f;

    // Elevation FoV upper bound (degrees).
    float maxElevation =  60.0f;

    // Azimuth FoV lower bound (degrees, body frame). REQ-AESA-011.
    float minAzimuth   = -60.0f;

    // Azimuth FoV upper bound (degrees, body frame). REQ-AESA-011.
    float maxAzimuth   =  60.0f;

    // ---- Per-task dwell times -----------------------------------------------

    // Time the beam dwells at each search position (milliseconds). REQ-AESA-020.
    float searchDwellTime_ms      = 2.0f;

    // Time the beam dwells at each track position (milliseconds). REQ-AESA-030.
    float trackDwellTime_ms       = 1.0f;

    // Time the beam dwells on the locked target in fire-control mode (ms).
    // Longer dwell = better SNR and Pk. REQ-AESA-003.
    float fireControlDwellTime_ms = 5.0f;

    // ---- Per-task default waveforms -----------------------------------------

    // Default waveform for SEARCH beams. LPRF for long-range unambiguous range.
    BeamWaveform searchWaveform      = { ModulationType::LFM,
                                   50e-6f, 300.0f, 5e6f, 10,
                                   WaveformMode::LPRF };

    // Default waveform for TRACK beams. MPRF balances range and velocity.
    BeamWaveform trackWaveform       = { ModulationType::LFM,
                                  10e-6f, 1000.0f, 20e6f, 10,
                                  WaveformMode::MPRF };

    // Default waveform for FIRE_CONTROL beams. HPRF for velocity accuracy.
    BeamWaveform fireControlWaveform = { ModulationType::NLFM,
                                        5e-6f, 2000.0f, 50e6f, 20,
                                        WaveformMode::HPRF };

    // Range-keyed waveform selection table. Sorted ascending by maxRange_m.
    // Signal processor selects the first entry whose maxRange_m > target range.
    // Final entry with maxRange_m = 0.0 acts as sentinel — use searchWaveform.
    // REQ-AESA-020.
    WaveformEntry waveformTable[6] = {
        {  30000.0f, { ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 20,
                    WaveformMode::HPRF } },
        { 100000.0f, { ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 10,
                     WaveformMode::MPRF } },
        { 400000.0f, { ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10,
                     WaveformMode::LPRF } },
        { 0.0f, {} }, { 0.0f, {} }, { 0.0f, {} }
    };

    // ---- PRF management -----------------------------------------------------

    // Available PRF levels (Hz). Used when prfType = SWITCHED or JITTERED.
    // Sentinel: 0.0 marks end of active entries. REQ-AESA-020.
    float   prfLevels[4] = { 300.0f, 1000.0f, 3000.0f, 0.0f };

    // PRF management strategy. REQ-AESA-020.
    PRFType prfType      = PRFType::FIXED;

    // ---- Frequency agility --------------------------------------------------

    // true = pulse-to-pulse frequency hopping enabled (LPI mode). REQ-AESA-020.
    bool  frequencyAgility  = true;

    // Lower bound of frequency hop range (Hz). REQ-AESA-020.
    float hopStartFrequency = 9.0e9f;

    // Upper bound of frequency hop range (Hz). REQ-AESA-020.
    float hopStopFrequency  = 11.0e9f;

    // ---- Receiver parameters ------------------------------------------------

    // Receiver / LNA noise temperature (Kelvin). 290 K = room temperature.
    // Noise power Pn = k * systemTemperature_K * bandwidth * noiseFigure.
    // REQ-AESA-040.
    double systemTemperature_K = 290.0;

    // Receiver noise figure (dB). Linear: F = 10^(noiseFigure_dB/10).
    // Typical AESA: 4–6 dB. REQ-AESA-040.
    double noiseFigure_dB      = 4.0;

    // CFAR false alarm probability. Determines detection threshold multiplier.
    // Typical: 1e-6 (one false alarm per million range-Doppler cells).
    // REQ-AESA-040.
    double targetPfa           = 1e-6;

    // ---- Platform parameters ------------------------------------------------

    // Radar platform altitude above sea level (metres). Used in horizon and
    // display range computation. Updated each tick from platform pose.
    // REQ-AESA-071.
    double radarHeight        = 10000.0;

    // Minimum detectable range (metres). Targets closer than this are
    // rejected before detection pipeline entry. REQ-AESA-040.
    double minDetectableRange = 100.0;

    // Platform speed (m/s). Used in clutter notch / Doppler blind zone
    // computation. Updated each tick from platform dynamic model.
    // REQ-AESA-040.
    float  platformSpeed_m_s  = 250.0f;

    // ---- Target category filter ---------------------------------------------

    // Determines which target surface types enter the detection pipeline.
    // REQ-AESA-040.
    DetectionCategory targetCategory = DetectionCategory::AIR_ONLY;

    // ---- Track lifecycle parameters -----------------------------------------

    // Number of consecutive scans a track can be missed before it is dropped.
    // REQ-AESA-030.
    int    missedScansToDrop  = 3;

    // Maximum time a track can coast without a detection before deletion (s).
    // REQ-AESA-030.
    double trackCoastSeconds  = 30.0;

    // Number of detections required before a track is marked isValidated.
    // REQ-AESA-030.
    int    minHitsToValidate  = 2;

    // Maximum credible track speed (m/s). Kalman velocity states are clamped
    // to this value to prevent divergence on bad detections. REQ-AESA-030.
    double maxTrackSpeed      = 3000.0;

    // Innovation magnitude threshold for manoeuvre detection (metres).
    // If Kalman innovation > this value, track is marked isManoeuvring and
    // process noise is increased. REQ-AESA-030.
    double manoeuvreThreshold_m = 500.0;

    // ---- IFF parameters -----------------------------------------------------

    // IFF interrogation mode transmitted to targets. REQ-AESA-050.
    IFFMode interrogationMode = IFFMode::MODE_3A;

    // List of squawk codes classified as friendly.
    // CAUTION: std::vector — heap allocated at config time only. REQ-AESA-050.
    std::vector<uint32_t> friendlySquawks;

    // ---- JPDA parameters ----------------------------------------------------

    // true = use Joint Probabilistic Data Association for track updates.
    // false = use nearest-neighbour (NN) association. REQ-AESA-030.
    bool  useJPDA               = true;

    // JPDA clutter / false alarm spatial density (detections per m^3).
    // Lower values increase track update weights. REQ-AESA-030.
    float jpdaFalseAlarmDensity = 1e-6f;

    // ---- Measurement noise parameters ---------------------------------------

    // Standard deviations of measurement noise applied to raw detections.
    // Zero values produce noise-free ideal measurements. REQ-AESA-040.
    NoiseModel noise;

    // ---- Emitter identity (for signal library / ESM) ------------------------

    // Human-readable emitter designation string. Informational. REQ-AESA-040.
    std::string emitterID   = "";

    // Numeric emitter code for library matching. REQ-AESA-040.
    uint32_t    emitterCode = 0;

    // ---- Operational mode ---------------------------------------------------

    // Current radar mode. Changed via RadarModel_AESA::setMode() / lockOn() /
    // breakLock(). REQ-AESA-003.
    RadarMode mode           = RadarMode::TWS;

    // ID of the target currently locked. 0 = no lock. Set by lockOn().
    // REQ-AESA-003.
    uint32_t  lockedTargetID = 0;
};

// =============================================================================
// SECTION 7: PLATFORM POSE
// =============================================================================

// -----------------------------------------------------------------------------
// RadarPose: position and attitude of the radar platform in world coordinates.
// Supplied by the engine bridge each tick via RadarModel_AESA::update().
// Position (x, y, z) in metres. Attitude in degrees.
// REQ-AESA-010.
// -----------------------------------------------------------------------------
struct RadarPose
{
    // Platform position in world coordinates (metres).
    double x = 0.0, y = 0.0, z = 0.0;

    // Platform roll angle (degrees). Positive = right wing down.
    float  roll    = 0.0f;

    // Platform pitch angle (degrees). Positive = nose up.
    float  pitch   = 0.0f;

    // Platform heading (degrees, 0 = north). REQ-AESA-010.
    float  heading = 0.0f;
};

// =============================================================================
// SECTION 7b: TARGET MATERIAL AND SHAPE TYPES
// REQ-AESA-040
// =============================================================================

// -----------------------------------------------------------------------------
// TargetMaterialType: surface material of the target platform.
// Controls the RCS reduction factor applied in computeEffectiveRCS().
// Values calibrated against open-literature measured data.
// Ref: Knott, Shaeffer, Tuley "Radar Cross Section" 2nd Ed, Table 5.1.
// REQ-AESA-040.
// -----------------------------------------------------------------------------
enum class TargetMaterialType
{
    METAL,      //  0 dB reduction — bare aluminium or steel structure
    COMPOSITE,  // -3 dB reduction — carbon fibre reinforced polymer airframe
    RAM,        // -15 dB reduction — radar absorbing material surface coating
    STEALTHY    // -25 dB reduction — full very low observable (VLO) treatment
};

// -----------------------------------------------------------------------------
// TargetShapeType: geometric shape of the target platform.
// Controls the surface coherence efficiency per face in the 6-facet box
// RCS decomposition model. REQ-AESA-040.
// Ref: Ruck et al "Radar Cross Section Handbook", Plenum 1970, Ch 4.
// -----------------------------------------------------------------------------
enum class TargetShapeType
{
    BOX,        // Flat faces and dihedral corners — ground vehicle, container
    AIRCRAFT,   // Curved fuselage with blended wing leading edges
    SHIP,       // Large flat superstructure and hard corner reflectors
    MISSILE,    // Cylindrical body with ogive nose cone and tail fins
    GENERIC     // Fallback when no better shape classification is available
};

// -----------------------------------------------------------------------------
// TargetDimensions: physical dimensions and material properties of a target.
// When valid = true, used in the 6-facet Physical Optics RCS model.
// When valid = false, RCS falls back to rcsTable or getPlatformBaseRCS().
// REQ-AESA-040.
// -----------------------------------------------------------------------------
struct TargetDimensions
{
    // Target length along forward (velocity) axis (metres). >= 0.
    double length = 0.0;

    // Target height along vertical axis (metres). >= 0.
    double height = 0.0;

    // Target width along lateral axis (metres). >= 0.
    double width  = 0.0;

    // true = length/height/width are valid and should be used for RCS.
    // false = use rcsTable or platformType fallback instead.
    bool   valid  = false;

    // Surface material for RCS reduction factor. REQ-AESA-040.
    TargetMaterialType material = TargetMaterialType::METAL;

    // Platform shape for facet efficiency factors. REQ-AESA-040.
    TargetShapeType    shape    = TargetShapeType::GENERIC;
};

// =============================================================================
// SECTION 8: TARGET INPUT
// =============================================================================

// -----------------------------------------------------------------------------
// TargetInput: complete descriptor for one target platform in one tick.
// Supplied by the engine bridge in the worldInputs vector each call to update().
// All position and velocity values are in radar-local body-frame coordinates.
// REQ-AESA-040.
// -----------------------------------------------------------------------------
struct TargetInput
{
    // Unique target identifier. Must be non-zero and stable across ticks.
    // 0 is reserved as "no target". REQ-AESA-030.
    uint32_t    id      = 0;

    // Position in radar-local body frame (metres).
    // x = forward, y = lateral, z = vertical (up positive).
    double      x = 0.0, y = 0.0, z = 0.0;

    // Velocity in radar-local body frame (m/s).
    double      vx = 0.0, vy = 0.0, vz = 0.0;

    // Nominal (mean) radar cross section (m²). Used as fallback when
    // dimensions.valid = false and rcsTable is empty. REQ-AESA-040.
    double      rcs     = 1.0;

    // Surface type determines clutter model and category filter. REQ-AESA-040.
    SurfaceType surface = SurfaceType::AIR;

    // Electronic warfare configuration for this target. REQ-AESA-060.
    JammerConfig jammer;

    // Swerling RCS fluctuation model for this target. REQ-AESA-040.
    SwerlingCase swerlingCase = SwerlingCase::CASE_I;

    // Aspect-angle-dependent RCS table. Vector of (aspect_deg, rcs_m2) pairs.
    // Sorted ascending by aspect_deg. Interpolated in computeEffectiveRCS().
    // Empty = use dimensions model or platformType fallback. REQ-AESA-040.
    std::vector<std::pair<float,float>> rcsTable;

    // Platform type string for RCS fallback lookup.
    // Valid values: "FIGHTER", "BOMBER", "UAV", "MISSILE", "HELO", "SHIP",
    //               "STEALTH", "GENERIC". REQ-AESA-040.
    std::string platformType = "GENERIC";

    // Physical dimensions for 6-facet Physical Optics RCS model. REQ-AESA-040.
    TargetDimensions dimensions;

    // ---- IFF transponder configuration --------------------------------------

    // true = target is equipped with a functioning IFF transponder. REQ-AESA-050.
    bool     hasIFF    = false;

    // Squawk code broadcast by the target's IFF transponder. REQ-AESA-050.
    uint32_t iffSquawk = 0;

    // IFF mode the target's transponder is set to. REQ-AESA-050.
    IFFMode  iffMode   = IFFMode::MODE_3A;
};

// =============================================================================
// SECTION 9: SIGNAL INTERCEPT (ESM)
// REQ-AESA-040
// =============================================================================

// -----------------------------------------------------------------------------
// SignalIntercept: accumulated electronic surveillance measurement for one
// emitter. Built up over multiple dwells by RadarSignalLibrary_AESA.
// Averaged frequency, PRI, pulse width, and signal level are computed from
// multiple measurements to improve classification confidence. REQ-AESA-040.
// -----------------------------------------------------------------------------
struct SignalIntercept
{
    // ID of the target platform whose emissions are being intercepted.
    uint32_t       targetID        = 0;

    // Running average of measured carrier frequency (Hz). REQ-AESA-040.
    double         frequency_Hz    = 0.0;

    // Running average of measured pulse repetition interval (seconds).
    double         pri_s           = 0.0;

    // Running average of measured pulse width (seconds).
    double         pulseWidth_s    = 0.0;

    // Running average of received signal level (dBW).
    double         signalLevel_dBW = 0.0;

    // Measurement counts — number of observations averaged into each field.
    int            priCount = 0, pwCount = 0, freqCount = 0, signalDepth = 0;

    // Modulation type identified from waveform analysis. REQ-AESA-040.
    ModulationType modulation      = ModulationType::NONE;

    // Emitter identifier string from library match. Empty = unknown emitter.
    std::string    emitterID       = "";
};

// -----------------------------------------------------------------------------
// SignalLibraryEntry: one entry in the emitter identification library.
// Used by RadarSignalLibrary_AESA::matchLibrary() to classify intercepts.
// REQ-AESA-040.
// -----------------------------------------------------------------------------
struct SignalLibraryEntry
{
    // Human-readable emitter name (e.g. "SA-10 FLAP LID").
    std::string    emitterID         = "";

    // Numeric emitter code for fast lookup.
    uint32_t       emitterCode       = 0;

    // Reference carrier frequency (Hz). REQ-AESA-040.
    double         frequency_Hz      = 0.0;

    // Acceptable frequency tolerance for match (Hz).
    double         freqTolerance_Hz  = 1e6;

    // Reference PRI (seconds). 0.0 = PRI not used in matching.
    double         pri_s             = 0.0;

    // PRI match tolerance (seconds).
    double         priTolerance_s    = 1e-5;

    // Reference pulse width (seconds). 0.0 = PW not used in matching.
    double         pulseWidth_s      = 0.0;

    // Pulse width match tolerance (seconds).
    double         pwTolerance_s     = 1e-7;

    // Required modulation type. NONE = modulation not used in matching.
    ModulationType modulation        = ModulationType::NONE;

    // Human-readable description of the emitter system.
    std::string    description       = "";
};

// =============================================================================
// SECTION 10: OUTPUT STRUCTURES
// REQ-AESA-004
// =============================================================================

// -----------------------------------------------------------------------------
// DetectionOutput: one raw detection from the CFAR pipeline for one target
// in one beam dwell. Multiple detections per target per scan are possible
// (one per beam position that illuminates the target). The scan detection
// cache in RadarModel_AESA deduplicates by targetID. REQ-AESA-004.
// -----------------------------------------------------------------------------
struct DetectionOutput
{
    // ID of the detected target. Matches TargetInput::id. REQ-AESA-004.
    uint32_t targetID      = 0;

    // Slant range to target (metres). May be ambiguous (isAmbiguous = true).
    // Resolved via staggered PRF coincidence detector if prf2_Hz is set.
    double range           = 0.0;

    // Target azimuth (degrees, body frame, range [-180, +180]).
    double azimuth         = 0.0;

    // Target elevation (degrees, body frame).
    double elevation       = 0.0;

    // Signal-to-interference-plus-noise ratio (linear, dimensionless).
    // SINR > CFAR threshold = detection declared. REQ-AESA-040.
    double snr             = 0.0;

    // Radial velocity (m/s, positive = closing). From Doppler measurement.
    double radialVelocity  = 0.0;

    // Closest point of approach distance (metres). REQ-AESA-004.
    double cpa_distance    = 0.0;

    // Time to closest point of approach (seconds, future only). REQ-AESA-004.
    double time_to_cpa     = 0.0;

    // Probability of kill estimate (Albersheim model). [0.0, 0.99]. REQ-AESA-040.
    double Pk              = 0.0;

    // Target heading (degrees, 0 = north). Derived from velocity. REQ-AESA-004.
    double heading         = 0.0;

    // Target speed over ground (m/s). Magnitude of vx/vy. REQ-AESA-004.
    double speedOverGround = 0.0;

    // Target acceleration (m/s^2). Reserved — populated as 0.0 currently.
    double acceleration    = 0.0;

    // Target aspect angle (degrees). Angle between target heading and LOS.
    double targetAspect    = 0.0;

    // Monopulse azimuth angle error correction (degrees). REQ-AESA-040.
    double azError_deg     = 0.0;

    // Monopulse elevation angle error correction (degrees). REQ-AESA-040.
    double elError_deg     = 0.0;

    // true = range is folded (ambiguous). Unresolved range is modulo Rmax.
    // false = range is unambiguous or has been resolved. REQ-AESA-021.
    bool   isAmbiguous     = false;

    // true = lock was broken this tick. Valid only on LOCK_ON output.
    bool   lockBroken      = false;

    // true = this detection is a DRFM ghost — do NOT engage. REQ-AESA-060.
    bool   isDRFMGhost     = false;

    // true = target was in the Doppler blind zone this dwell. Detection
    // was achieved via HPRF or STAP — quality may be degraded. REQ-AESA-040.
    bool   inDopplerBlind  = false;
};

// -----------------------------------------------------------------------------
// TrackOutput: validated Kalman track state for one target.
// Published by getValidatedTracks() and latestOutput_.tracks.
// Differs from DetectionOutput in that position/velocity are Kalman-smoothed
// and extrapolated, not raw measurements. REQ-AESA-030.
// -----------------------------------------------------------------------------
struct TrackOutput
{
    // Track ID. Matches TargetInput::id. REQ-AESA-030.
    uint32_t id = 0;

    // Kalman-smoothed position in radar-local body frame (metres). REQ-AESA-030.
    double x = 0.0, y = 0.0, z = 0.0;

    // Kalman-smoothed velocity in radar-local body frame (m/s). REQ-AESA-030.
    double vx = 0.0, vy = 0.0, vz = 0.0;

    // Slant range to track (metres). Uses predictedRange between updates.
    double range           = 0.0;

    // Track azimuth (degrees, body frame, range [-180, +180]). REQ-AESA-030.
    double azimuth         = 0.0;

    // Track elevation (degrees, body frame). REQ-AESA-030.
    double elevation       = 0.0;

    // Kalman-estimated radial velocity (m/s, positive = closing).
    double radialVelocity  = 0.0;

    // Speed over ground (m/s). sqrt(vx^2 + vy^2). REQ-AESA-030.
    double speedOverGround = 0.0;

    // Track heading (degrees, 0 = north). atan2(vy, vx). REQ-AESA-030.
    double heading         = 0.0;

    // Target aspect angle (degrees). Angle between track heading and LOS.
    double targetAspect    = 0.0;

    // Closest point of approach distance (metres). REQ-AESA-004.
    double cpa_distance    = 0.0;

    // Time to CPA (seconds, future only). REQ-AESA-004.
    double time_to_cpa     = 0.0;

    // Probability of kill estimate [0.0, 0.99]. REQ-AESA-040.
    double Pk              = 0.0;

    // Number of confirmed detections associated to this track. REQ-AESA-030.
    int    hitCount        = 0;

    // Number of consecutive scans this track has been missed. REQ-AESA-030.
    int    scanMissCount   = 0;

    // true = track has been confirmed (hitCount >= minHitsToValidate). REQ-AESA-030.
    bool   isValidated     = false;

    // true = track experienced range ambiguity in a previous detection.
    bool   wasAmbiguous    = false;

    // true = Kalman innovation exceeds manoeuvreThreshold_m. IMM has switched
    // to high process noise model. REQ-AESA-030.
    bool   isManoeuvring   = false;

    // Track quality score [0.0, 1.0]. Function of hitCount and scanMissCount.
    double trackQuality    = 0.0;

    // true = this track originated from a DRFM ghost detection. Do not engage.
    bool   isDRFMSuspect   = false;

    // true = this track was injected via Link-16 / CEC, not by radar. REQ-AESA-030.
    bool   isExternalTrack = false;

    // Latest IFF interrogation result for this track. REQ-AESA-050.
    IFFResult iff;
};

// -----------------------------------------------------------------------------
// RadarOutput: complete output published by RadarModel_AESA each tick.
// Consumed by AESARadar bridge and forwarded to the UI and fire-control system.
// REQ-AESA-004.
// -----------------------------------------------------------------------------
struct RadarOutput
{
    // Current scan detections. Content and cadence depend on mode:
    //   SURVEILLANCE — raw detections published each tick.
    //   TWS          — accumulated detections published at scan boundary.
    //   LOCK_ON      — detections on locked target published each tick.
    // REQ-AESA-004.
    std::vector<DetectionOutput> detections;

    // Validated Kalman tracks. Empty in SURVEILLANCE mode. REQ-AESA-030.
    std::vector<TrackOutput>     tracks;

    // ESM signal intercepts accumulated since last scan boundary. REQ-AESA-040.
    std::vector<SignalIntercept> intercepts;

    // Current beam azimuth (degrees, body frame). REQ-AESA-010.
    double    currentAzimuth   = 0.0;

    // Current beam elevation (degrees, body frame). REQ-AESA-010.
    double    currentElevation = 0.0;

    // Current radar operating mode. REQ-AESA-003.
    RadarMode mode             = RadarMode::SURVEILLANCE;

    // true = lock was broken this tick (locked target not seen for > 10 dwells).
    // Consumer must stop fire-control updates when this is true. REQ-AESA-003.
    bool      lockBroken       = false;

    // Display range recommended for UI (km). Min(RF horizon, radar horizon).
    // Range: [5.0, 1000.0] km. REQ-AESA-004.
    double    displayRange_km  = 200.0;

    // Task type of the beam dwell that produced this output. REQ-AESA-010.
    BeamRequest::Task currentTask = BeamRequest::Task::SEARCH;

    // Current T/R module duty cycle [0.0, 1.0]. Emitted each tick via
    // AESARadar::schedulerDutyCycle signal. REQ-AESA-020.
    double    currentDutyCycle = 0.0;
};

// =============================================================================
// SECTION 11: INTERNAL KALMAN TRACK FILE
// REQ-AESA-030
// =============================================================================

// -----------------------------------------------------------------------------
// TrackFile: internal Kalman filter state for one tracked target.
// Maintained inside RadarTracker_AESA::db_. Not exposed via public API.
// Contains 6-state IMM filter with two CV models (low-Q / high-Q).
// REQ-AESA-030.
//
// State vector X = [x, y, z, vx, vy, vz] in radar-local frame (metres, m/s).
// Covariance matrices P, Q, R are 6x6 or 3x3 doubles.
// -----------------------------------------------------------------------------
struct TrackFile
{
    // ---- Identity -----------------------------------------------------------

    // Target ID. Matches TargetInput::id. Immutable after track creation.
    uint32_t id = 0;

    // ---- Smoothed state (convenience copies of X[0..5]) --------------------

    // Kalman-smoothed position (metres, body frame). REQ-AESA-030.
    double x = 0.0, y = 0.0, z = 0.0;

    // Kalman-smoothed velocity (m/s, body frame). REQ-AESA-030.
    double vx = 0.0, vy = 0.0, vz = 0.0;

    // ---- Range estimates ----------------------------------------------------

    // Current slant range (metres). Updated after each Kalman update.
    double range = 0.0;

    // Radial velocity (m/s). Positive = closing. REQ-AESA-030.
    double velocity = 0.0;

    // Predicted slant range for next beam dwell (metres).
    // Used for range ambiguity resolution and beam pointing. REQ-AESA-021.
    double predictedRange = 0.0;

    // ---- Timing -------------------------------------------------------------

    // Simulation time of most recent confirmed detection (seconds).
    double lastSeenTime      = 0.0;

    // Simulation time of most recent track beam dwell (seconds). Used by
    // scheduler to determine when next track beam is needed. REQ-AESA-010.
    double lastTrackBeamTime = 0.0;

    // ---- Track quality counters ---------------------------------------------

    // Total confirmed detections associated to this track. REQ-AESA-030.
    int    hitCount = 0;

    // true = hitCount >= minHitsToValidate. Track is published. REQ-AESA-030.
    bool   isValidated = false;

    // true = Kalman update was applied this tick. REQ-AESA-030.
    bool   isUpdated   = false;

    // true = most recent detection was range-ambiguous. REQ-AESA-021.
    bool   wasAmbiguous = false;

    // Number of consecutive missed detections (resets on detection). REQ-AESA-030.
    int    missCount = 0;

    // Number of consecutive scans without detection. Used for drop logic.
    int    scanMissCount = 0;

    // true = detection was associated this scan (cleared at scan boundary).
    bool   updatedThisScan = false;

    // true = Kalman innovation > manoeuvreThreshold_m. IMM in high-Q model.
    bool   isManoeuvring   = false;

    // true = track created from a DRFM ghost detection. REQ-AESA-060.
    bool   isDRFMSuspect   = false;

    // true = track injected via Link-16 / CEC. Not from radar. REQ-AESA-030.
    bool   isExternalTrack = false;

    // Magnitude of most recent Kalman innovation vector (metres). REQ-AESA-030.
    double innovationMagnitude = 0.0;

    // Track quality score [0.0, 1.0]. REQ-AESA-030.
    double trackQuality    = 0.0;

    // Latest IFF result. Updated by queryIFF() each tick. REQ-AESA-050.
    IFFResult iff;

    // ---- Kalman filter matrices ---------------------------------------------

    // State vector [x, y, z, vx, vy, vz]. Units: metres, m/s. REQ-AESA-030.
    std::array<double, 6> X = {};

    // State covariance matrix 6x6 (metres^2, (m/s)^2). REQ-AESA-030.
    double P[6][6] = {};

    // Process noise covariance matrix 6x6. Tuned by manoeuvre detection.
    // Velocity diagonal elements are set to 1.0 (steady) or 100.0 (manoeuvre).
    double Q[6][6] = {};

    // Measurement noise covariance matrix 3x3 (position only).
    // R[0][0]=R[1][1] = rangeStdDev^2, R[2][2] scaled with elevation noise.
    double R[3][3] = {};

    // ---- IMM filter state (2-model: CV-low-Q / CV-high-Q) ------------------

    // Model probability weights. imm_mu[0] = P(CV model), imm_mu[1] = P(manoeuvre).
    // Initialised to [0.7, 0.3]. Updated by measurement likelihood. REQ-AESA-030.
    double imm_mu[2]      = { 0.7, 0.3 };

    // Per-model state vector [x,y,z,vx,vy,vz]. Initialised from X on first call.
    double imm_X[2][6]    = {};

    // Per-model state covariance. Initialised from P on first call. REQ-AESA-030.
    double imm_P[2][6][6] = {};

    // true = IMM has been initialised. Set on first call to performIMMPredict()
    // after hitCount >= 2. REQ-AESA-030.
    bool   immActive      = false;
};

// =============================================================================
// SECTION 12: FORWARD DECLARATIONS
// =============================================================================

// Forward declarations of subsystem classes. Full definitions in their
// respective headers. Allows RadarModel_AESA to hold unique_ptr members
// without exposing subsystem implementations to consumers of this header.
class RadarSignalProcessor_AESA;
class RadarAntenna_AESA;
class RadarScheduler;
class RadarTracker_AESA;
class RadarSignalLibrary_AESA;

// =============================================================================
// SECTION 13: RADARMODEL_AESA — MAIN CLASS
// REQ-AESA-001 through REQ-AESA-004
// =============================================================================

// -----------------------------------------------------------------------------
// CLASS: RadarModel_AESA
//
// DESCRIPTION:  Top-level AESA radar simulation model. Owns and coordinates
//               five subsystems: antenna steering, signal processing, beam
//               scheduling, multi-target tracking, and signal library (ESM).
//               Exposes a thread-safe public API for lifecycle management,
//               configuration, mode control, and output retrieval.
//
//               All mutable state is protected by mutex_. All public methods
//               acquire mutex_ before accessing or modifying state. Private
//               helper methods (processTargetDetection, injectDRFMGhost, etc.)
//               are called only from update() which already holds mutex_ —
//               they must NOT re-acquire it.
//
// OWNERSHIP:    Subsystems (signal_, antenna_, scheduler_, tracker_, library_)
//               are owned via std::unique_ptr. Lifetime is tied to this object.
//
// THREAD SAFETY: Thread-safe. All public methods acquire mutex_ internally.
//
// REQUIREMENTS: REQ-AESA-001  Lifecycle (init/start/update/end/reset)
//               REQ-AESA-002  Configuration (setConfig/getConfig)
//               REQ-AESA-003  Mode control (setMode/lockOn/breakLock)
//               REQ-AESA-004  Output (getOutput)
//               All physics requirements delegated to subsystems.
//
// TRACEABILITY:
//   Test suite:  aesaRadar_test (aesaradar_test.cpp)
//   Test cases:  TC-AESA-MODEL-001 through TC-AESA-MODEL-020
// -----------------------------------------------------------------------------
class RadarModel_AESA
{
public:

    // =========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // =========================================================================

    // =========================================================================
    // FUNCTION:    RadarModel_AESA (constructor)
    //
    // DESCRIPTION: Constructs all five subsystems via make_unique. No
    //              operational state is initialised here — call init()
    //              before any other method. REQ-AESA-001.
    //
    // SIDE EFFECTS: Allocates subsystem objects on heap (MM-01 deviation —
    //               constructor only, documented in ICD-AESA-DEVIATION-002).
    //
    // TRACEABILITY: TC-AESA-MODEL-001  Constructor does not crash
    // =========================================================================
    RadarModel_AESA();

    // =========================================================================
    // FUNCTION:    ~RadarModel_AESA (destructor)
    //
    // DESCRIPTION: Default destructor. All subsystems released via unique_ptr
    //              destructors. No explicit cleanup required. REQ-AESA-001.
    // =========================================================================
    ~RadarModel_AESA();

    // =========================================================================
    // LIFECYCLE METHODS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    init
    //
    // DESCRIPTION: Initialises the radar model with the supplied configuration.
    //              Resets all subsystems to a clean state. Sets running_ = false.
    //              Must be called before start() and update(). Safe to call
    //              multiple times — reinitialises from new config each time.
    //
    // REQUIREMENT: REQ-AESA-001
    //
    // PARAMETERS:
    //   cfg  [in]  Complete radar configuration. Copied internally.
    //              All fields must be valid. See RadarConfig for field ranges.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Acquires mutex_. Clears all subsystem state. Resets
    //               all output caches. Sets initialised_ = true, running_ = false.
    //
    // TRACEABILITY: TC-AESA-MODEL-002  init() does not crash
    //               TC-AESA-MODEL-003  After init(), running state is false
    // =========================================================================
    void init(const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    start
    //
    // DESCRIPTION: Transitions the model from initialised to running state.
    //              Builds the initial beam schedule. Sets running_ = true.
    //              Must be called after init() and before update().
    //
    // REQUIREMENT: REQ-AESA-001
    //
    // PARAMETERS:  None.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Acquires mutex_. Clears tracker, antenna, library, chaff.
    //               Calls scheduler_->buildSchedule(). Sets running_ = true.
    //
    // TRACEABILITY: TC-AESA-MODEL-004  start() does not crash
    // =========================================================================
    void start();

    // =========================================================================
    // FUNCTION:    update
    //
    // DESCRIPTION: Main simulation tick. Advances the radar model by dt seconds.
    //              Executes the full detection, tracking, and output pipeline:
    //
    //                1. Chaff cloud expiry pruning
    //                2. Kalman prediction (TWS / LOCK_ON only)
    //                3. Scheduler advance and beam pointing
    //                4. Per-target detection pipeline
    //                5. Track association (JPDA or NN)
    //                6. Scan miss logic (at scan boundary)
    //                7. Break-lock miss counting
    //                8. IFF interrogation
    //                9. Display range update
    //               10. Scan detection cache update
    //               11. Output assembly (mode-dependent cadence)
    //               12. Schedule rebuild at scan boundary
    //
    //              Returns immediately if running_ = false.
    //
    // REQUIREMENT: REQ-AESA-001 through REQ-AESA-004, REQ-AESA-010 through
    //              REQ-AESA-072.
    //
    // PARAMETERS:
    //   dt          [in]  Elapsed time since last call (seconds).
    //                     Valid range: [1e-4, 1.0] s. Values outside this range
    //                     produce valid but potentially inaccurate Kalman
    //                     predictions. Caller must clamp before passing.
    //
    //   pose        [in]  Current platform position and attitude. Used to
    //                     update radarHeight and beam attitude compensation.
    //
    //   worldInputs [in]  All target platforms visible to the simulation
    //                     this tick. Each entry describes one target.
    //                     May be empty — model handles zero-target case safely.
    //
    //   simTime     [in]  Absolute simulation time (seconds). Used for chaff
    //                     decay, track coast, and signal library timestamping.
    //                     Must be monotonically increasing.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Acquires mutex_. Modifies all internal state. Updates
    //               latestOutput_. May modify config_.mode and config_.radarHeight.
    //
    // TRACEABILITY: TC-AESA-MODEL-005  update() with no targets does not crash
    //               TC-AESA-MODEL-006  update() before start() returns immediately
    // =========================================================================
    void update(double dt, const RadarPose& pose,
                const std::vector<TargetInput>& worldInputs,
                double simTime);

    // =========================================================================
    // FUNCTION:    end
    //
    // DESCRIPTION: Stops the radar model and clears all runtime state.
    //              Sets running_ = false. Safe to call multiple times.
    //              After end(), update() returns immediately.
    //
    // REQUIREMENT: REQ-AESA-001
    //
    // PARAMETERS:  None.
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Clears tracker, library, scheduler, chaff.
    //               Resets latestOutput_. Sets running_ = false.
    //
    // TRACEABILITY: TC-AESA-MODEL-007  end() does not crash
    // =========================================================================
    void end();

    // =========================================================================
    // FUNCTION:    reset
    //
    // DESCRIPTION: Saves current config, then calls init() and start() with
    //              the saved config. Equivalent to a full cold restart while
    //              preserving the current configuration. Clears all tracks,
    //              detections, caches, and EW pull-off state.
    //
    // REQUIREMENT: REQ-AESA-001
    //
    // PARAMETERS:  None.
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_ briefly to copy config. Then calls init()
    //               and start() which each acquire mutex_. Resets lockMissCount_,
    //               rgpoPullOff_, vgpoPullOff_.
    //
    // TRACEABILITY: TC-AESA-MODEL-008  reset() does not crash
    // =========================================================================
    void reset();

    // =========================================================================
    // CONFIGURATION METHODS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    setConfig
    //
    // DESCRIPTION: Updates the radar configuration. If mode changes from
    //              non-SURVEILLANCE to SURVEILLANCE, tracker and output are
    //              cleared to prevent stale track data appearing on the display.
    //              Sets displayRangeDirty_ = true to force recalculation.
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // PARAMETERS:
    //   cfg  [in]  New radar configuration. Applied immediately.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. May clear tracker and output caches.
    //               Sets displayRangeDirty_ = true.
    //
    // TRACEABILITY: TC-AESA-MODEL-009  setConfig() applies beamWidth change
    // =========================================================================
    void        setConfig(const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    getConfig
    //
    // DESCRIPTION: Returns a copy of the current radar configuration.
    //              Thread-safe — acquires mutex_ for the copy operation.
    //
    // REQUIREMENT: REQ-AESA-002
    //
    // PARAMETERS:  None.
    // RETURNS:     Copy of config_. Caller receives a snapshot at call time.
    // SIDE EFFECTS: Acquires mutex_ (briefly for copy).
    //
    // TRACEABILITY: TC-AESA-MODEL-009  getConfig() returns applied config
    // =========================================================================
    RadarConfig getConfig() const;

    // =========================================================================
    // MODE CONTROL METHODS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    setMode
    //
    // DESCRIPTION: Sets the radar operating mode. Rebuilds the beam schedule
    //              immediately for the new mode. Does not clear track data —
    //              use setConfig() with a SURVEILLANCE mode if track clearing
    //              is needed.
    //
    // REQUIREMENT: REQ-AESA-003
    //
    // PARAMETERS:
    //   mode  [in]  Target operating mode. REQ-AESA-003.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Updates config_.mode. Calls
    //               scheduler_->buildSchedule(). Sets displayRangeDirty_ = true.
    //
    // TRACEABILITY: TC-AESA-MODEL-010  setMode() changes mode without crash
    // =========================================================================
    void setMode(RadarMode mode);

    // =========================================================================
    // FUNCTION:    lockOn
    //
    // DESCRIPTION: Transitions to LOCK_ON mode targeting the specified platform.
    //              Clears scan detection cache and output to prevent stale TWS
    //              detections appearing during fire-control. Resets lock miss
    //              counter. Rebuilds schedule with fire-control beam on target.
    //
    // REQUIREMENT: REQ-AESA-003
    //
    // PARAMETERS:
    //   targetID  [in]  ID of the target to lock. Must match a TargetInput::id
    //                   in the worldInputs passed to update(). 0 = no effect.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Sets config_.mode = LOCK_ON. Clears caches.
    //               Resets lockMissCount_ = 0.
    //
    // TRACEABILITY: TC-AESA-MODEL-011  lockOn() does not crash
    //               TC-AESA-MODEL-012  lockOn(0) does not crash
    // =========================================================================
    void lockOn(uint32_t targetID);

    // =========================================================================
    // FUNCTION:    breakLock
    //
    // DESCRIPTION: Returns the radar from LOCK_ON to SURVEILLANCE mode.
    //              Clears the scan detection cache to prevent the last-known
    //              lock azimuth appearing as a ghost detection after mode switch.
    //              Resets firstScanComplete_ so the first full scan after
    //              break-lock is required before tracks are published.
    //
    // REQUIREMENT: REQ-AESA-003
    //
    // PARAMETERS:  None.
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Sets config_.mode = SURVEILLANCE.
    //               Sets config_.lockedTargetID = 0. Clears caches. Rebuilds schedule.
    //
    // TRACEABILITY: TC-AESA-MODEL-013  breakLock() from no-lock state does not crash
    //               TC-AESA-MODEL-014  breakLock() after lockOn() returns to SURVEILLANCE
    // =========================================================================
    void breakLock();

    // =========================================================================
    // OUTPUT METHOD
    // =========================================================================

    // =========================================================================
    // FUNCTION:    getOutput
    //
    // DESCRIPTION: Returns a copy of the latest radar output assembled by the
    //              most recent update() call. Thread-safe snapshot.
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // PARAMETERS:  None.
    // RETURNS:     Copy of latestOutput_. Caller receives a snapshot.
    // SIDE EFFECTS: Acquires mutex_ (briefly for copy).
    //
    // TRACEABILITY: TC-AESA-MODEL-015  getOutput() does not crash after init
    // =========================================================================
    RadarOutput getOutput() const;

    // =========================================================================
    // UTILITY METHODS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    computeMaxDetectionRange
    //
    // DESCRIPTION: Computes the theoretical maximum detection range for a
    //              target with the specified RCS using the current config.
    //              Iteratively solves the radar range equation with propagation
    //              loss. Used for display range computation and UI scaling.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   rcs  [in]  Target RCS (m²). Default: 3.0 m² (fighter-class target).
    //              Must be > 0.
    //
    // RETURNS:    Maximum detection range (km).
    //
    // SIDE EFFECTS: Acquires mutex_.
    //
    // TRACEABILITY: TC-AESA-MODEL-016  computeMaxDetectionRange() returns > 0
    // =========================================================================
    double computeMaxDetectionRange(double rcs = 3.0) const;

    // =========================================================================
    // FUNCTION:    resolveRangeAmbiguity
    //
    // DESCRIPTION: Resolves a folded (ambiguous) range measurement to the
    //              nearest unambiguous range using the predicted track range
    //              as a prior. Searches integer multiples of Rmax in [-5, +5].
    //
    // REQUIREMENT: REQ-AESA-021
    //
    // PARAMETERS:
    //   measured   [in]  Folded range measurement (metres).
    //   predicted  [in]  Kalman-predicted range from track file (metres).
    //   Rmax       [in]  Unambiguous range for current PRF (metres).
    //
    // RETURNS:    Resolved range (metres).
    // SIDE EFFECTS: Acquires mutex_.
    //
    // TRACEABILITY: TC-AESA-MODEL-017  resolveRangeAmbiguity() correct for k=1
    // =========================================================================
    double resolveRangeAmbiguity(double measured, double predicted,
                                 double Rmax) const;

    // =========================================================================
    // FUNCTION:    loadSignalLibrary
    //
    // DESCRIPTION: Loads the emitter identification library used for ESM
    //              classification of signal intercepts. Replaces any previously
    //              loaded library entries.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   entries  [in]  Vector of library entries. May be empty.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Calls library_->loadLibrary().
    // =========================================================================
    void loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries);

    // =========================================================================
    // CHAFF MANAGEMENT METHODS
    // REQ-AESA-061
    // =========================================================================

    // =========================================================================
    // FUNCTION:    addChaffCloud
    // DESCRIPTION: Adds one chaff cloud to the active chaff cloud list.
    //              Cloud is included in SINR denominator from the next tick.
    // REQUIREMENT: REQ-AESA-061
    // SIDE EFFECTS: Acquires mutex_. Appends to chaffClouds_.
    //               MM-01 note: push_back may allocate. This is a known
    //               deviation — chaff deployment is an infrequent event.
    // =========================================================================
    void addChaffCloud(const ChaffCloud& cloud);

    // =========================================================================
    // FUNCTION:    clearChaffClouds
    // DESCRIPTION: Removes all active chaff clouds immediately.
    // REQUIREMENT: REQ-AESA-061
    // SIDE EFFECTS: Acquires mutex_. Clears chaffClouds_.
    // =========================================================================
    void clearChaffClouds();

    // =========================================================================
    // FUNCTION:    injectExternalTrack
    //
    // DESCRIPTION: Injects a track received via external data link (Link-16 /
    //              CEC) into the tracker database. The track is marked
    //              isExternalTrack = true and isValidated = true immediately.
    //              Allows the radar to maintain a recognised air picture beyond
    //              its own sensor range.
    //
    // REQUIREMENT: REQ-AESA-030
    //
    // PARAMETERS:
    //   ext  [in]  External track descriptor. Must have ext.id != 0.
    //              A track with this ID must not already exist in the database.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Acquires mutex_. Calls tracker_->injectExternalTrack().
    // =========================================================================
    void injectExternalTrack(const TrackOutput& ext);

private:

    // =========================================================================
    // PRIVATE STATE VARIABLES
    // =========================================================================

    // Current platform pose. Updated each tick by update(). Not mutex-protected
    // internally — only accessed from update() which holds mutex_. REQ-AESA-010.
    RadarPose  currentPose_;

    // Absolute simulation time of the last update() call (seconds). REQ-AESA-001.
    double     currentSimTime_ = 0.0;

    // true = first full scan has completed in TWS mode. Track output is
    // suppressed until this is set. Prevents partial-scan ghost tracks.
    // REQ-AESA-003.
    bool firstScanComplete_ = false;

    // Mutex protecting all shared state. Acquired by all public methods.
    // Private methods called from update() must NOT acquire — deadlock risk.
    mutable std::mutex mutex_;

    // Current radar configuration. All operational parameters. REQ-AESA-002.
    RadarConfig config_;

    // =========================================================================
    // SUBSYSTEM OWNERSHIP
    // All subsystems owned exclusively by this object. REQ-AESA-001.
    // =========================================================================

    // Signal processing: SINR, CFAR, RCS, propagation, beam gain. REQ-AESA-040.
    std::unique_ptr<RadarSignalProcessor_AESA> signal_;

    // Antenna beam steering and gain computation. REQ-AESA-010.
    std::unique_ptr<RadarAntenna_AESA>         antenna_;

    // Beam schedule generation and execution. REQ-AESA-010.
    std::unique_ptr<RadarScheduler>            scheduler_;

    // Multi-target Kalman / IMM / JPDA tracker. REQ-AESA-030.
    std::unique_ptr<RadarTracker_AESA>         tracker_;

    // ESM signal intercept accumulation and library matching. REQ-AESA-040.
    std::unique_ptr<RadarSignalLibrary_AESA>   library_;

    // =========================================================================
    // OPERATIONAL STATE
    // =========================================================================

    // true = init() has been called. false = model not yet initialised.
    bool initialised_ = false;

    // true = start() has been called and end() has not. update() runs only
    // when running_ = true. REQ-AESA-001.
    bool running_     = false;

    // Cached maximum detection range (km). Recomputed when displayRangeDirty_.
    // Capped at min(RF horizon, radar horizon, 1000 km). REQ-AESA-004.
    mutable double cachedDisplayRange_km_ = 200.0;

    // true = cachedDisplayRange_km_ must be recomputed next update() tick.
    // Set when config changes, mode changes, or radarHeight changes. REQ-AESA-004.
    mutable bool   displayRangeDirty_     = true;

    // Most recently assembled radar output. Returned by getOutput(). REQ-AESA-004.
    RadarOutput latestOutput_;

    // Active chaff clouds. Pruned each tick based on birthTime_s + decayTime_s.
    // REQ-AESA-061.
    std::vector<ChaffCloud> chaffClouds_;

    // Per-target accumulated DRFM range pull-off (metres).
    // Key = TargetInput::id. Incremented each tick by drfmPullOffRate_m_s * dt.
    // REQ-AESA-060.
    std::unordered_map<uint32_t, double> drfmPullOff_;

    // Per-target accumulated RGPO range pull-off (metres). REQ-AESA-060.
    std::unordered_map<uint32_t, double> rgpoPullOff_;

    // Per-target accumulated VGPO velocity pull-off (m/s). REQ-AESA-060.
    std::unordered_map<uint32_t, double> vgpoPullOff_;

    // Scan detection cache. Accumulates detections from all beam positions
    // within one scan. Published at scan boundary (TWS) or each tick (LOCK_ON).
    // Deduplicates by targetID — latest detection per target is kept.
    // REQ-AESA-004.
    std::vector<DetectionOutput> scanDetectionCache_;

    // Consecutive missed dwell count for the locked target.
    // Lock is broken when this exceeds 10. Prevents single-dwell fluctuation
    // from breaking lock. REQ-AESA-003.
    int lockMissCount_ = 0;

    // Copy of worldInputs from current tick. Required by computeOcclusion()
    // which runs inside processTargetDetection() and needs all other targets.
    // REQ-AESA-070.
    std::vector<TargetInput> currentWorldInputs_;

    // =========================================================================
    // PRIVATE HELPER METHODS
    // All called from update() which holds mutex_. Do NOT acquire mutex_ here.
    // =========================================================================

    // =========================================================================
    // FUNCTION:    processTargetDetection
    //
    // DESCRIPTION: Executes the complete single-target detection pipeline for
    //              one TargetInput in one beam dwell. Returns true if a valid
    //              detection was produced and appended to scanDets.
    //              Returns false if the target was rejected at any gate
    //              (category, range, horizon, shadow, beam gate, sidelobe
    //              blanking, Doppler notch, or CFAR threshold).
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   target               [in]     Target to process.
    //   beam                 [in]     Current beam dwell descriptor.
    //   dt                   [in]     Tick duration (seconds, unused in body
    //                                 but kept for API consistency).
    //   simTime              [in]     Current simulation time (seconds).
    //   maxUnambiguousRange  [in]     Rmax for primary PRF (metres).
    //   maxUnambiguousRange2 [in]     Rmax for secondary PRF (0 = disabled).
    //   scanDets             [out]    Vector to append detections to.
    //   rNoise/azNoise/elNoise/dvNoise [in/out] RNG distributions for
    //                                 measurement noise. Stateful — must be
    //                                 the same objects across all targets in
    //                                 one tick. REQ-AESA-040.
    //
    // RETURNS:    true = detection produced. false = target rejected.
    // SIDE EFFECTS: May append to scanDets, drfmPullOff_, rgpoPullOff_,
    //               vgpoPullOff_. Calls library_->accumulate().
    //
    // TRACEABILITY: TC-AESA-MODEL-018  Target below minDetectableRange rejected
    //               TC-AESA-MODEL-019  Target in shadow zone rejected
    // =========================================================================
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

    // =========================================================================
    // FUNCTION:    injectDRFMGhost
    //
    // DESCRIPTION: Creates and appends a DRFM ghost detection when a target's
    //              jammer has gateStealingActive = true. The ghost appears at
    //              a range offset by drfmPullOff_[target.id] from the real
    //              target. The pull-off distance grows each call by
    //              drfmPullOffRate_m_s * 0.05 metres.
    //
    // REQUIREMENT: REQ-AESA-060
    //
    // PARAMETERS:  Same noise distributions as processTargetDetection().
    //              maxUnambiguousRange used to flag ghost as ambiguous.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Increments drfmPullOff_[real.id]. May append to scanDets.
    //               Calls library_->accumulate() with isDRFMGhost = true.
    //
    // TRACEABILITY: TC-AESA-MODEL-020  DRFM ghost has isDRFMGhost = true
    // =========================================================================
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

    // =========================================================================
    // FUNCTION:    injectRGPOVGPO
    //
    // DESCRIPTION: Creates false range (RGPO) and/or false Doppler (VGPO)
    //              ghost detections when a target's jammer has rgpoActive or
    //              vgpoActive set. RGPO ghost walks away in range at rgpoRate_m_s.
    //              VGPO ghost walks away in velocity at vgpoRate_m_s2.
    //
    // REQUIREMENT: REQ-AESA-060
    //
    // PARAMETERS:
    //   dt  [in]  Tick duration (seconds). Used to increment pull-off state.
    //
    // RETURNS:     void
    // SIDE EFFECTS: Increments rgpoPullOff_[real.id] and/or vgpoPullOff_[real.id].
    //               May append to scanDets.
    // =========================================================================
    void injectRGPOVGPO(
        const TargetInput& real, const BeamRequest& beam,
        double simTime, double dt, double maxUnambiguousRange,
        std::vector<DetectionOutput>& scanDets,
        std::normal_distribution<double>& rNoise,
        std::normal_distribution<double>& azNoise,
        std::normal_distribution<double>& elNoise,
        std::normal_distribution<double>& dvNoise);

    // =========================================================================
    // FUNCTION:    queryIFF
    //
    // DESCRIPTION: Performs one IFF interrogation cycle for the given track.
    //              Searches worldInputs for a TargetInput with matching id,
    //              then evaluates the squawk code against friendlySquawks list.
    //              Returns NO_REPLY if IFF is disabled or target not found.
    //
    // REQUIREMENT: REQ-AESA-050
    //
    // PARAMETERS:
    //   track  [in]  Track file to interrogate. Uses track.id for lookup.
    //   world  [in]  Current worldInputs. Must be the same vector passed to
    //                update() this tick.
    //
    // RETURNS:    IFFResult with response code, squawk, and confidence.
    // SIDE EFFECTS: None. Pure query.
    // =========================================================================
    IFFResult queryIFF(const TrackFile& track,
                       const std::vector<TargetInput>& worldInputs) const;

    // =========================================================================
    // FUNCTION:    computeMaxDetectionRange_locked
    //
    // DESCRIPTION: Non-locking version of computeMaxDetectionRange().
    //              Called from update() (which holds mutex_) and from
    //              computeMaxDetectionRange() (which acquires mutex_ first).
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   rcs  [in]  Target RCS (m²). Default: 3.0 m².
    //
    // RETURNS:    Maximum detection range (km). Delegates to signal processor.
    // SIDE EFFECTS: None. Read-only access to config_.
    // =========================================================================
    double computeMaxDetectionRange_locked(double rcs = 3.0) const;

    // =========================================================================
    // FUNCTION:    rebuildSchedule
    //
    // DESCRIPTION: Rebuilds the beam schedule at scan boundary using current
    //              config and validated tracks. Called from update() after
    //              scanBnd = true. REQ-AESA-010.
    //
    // SIDE EFFECTS: Calls scheduler_->buildSchedule(). Modifies schedule_.
    // =========================================================================
    void rebuildSchedule();

    // =========================================================================
    // FUNCTION:    applyAttitudeToBeam
    //
    // DESCRIPTION: Converts a beam direction from body frame to world frame
    //              by applying the current platform roll, pitch, and heading.
    //              Fast path for small roll/pitch angles (< 0.1 deg): applies
    //              heading rotation only. Full 3D rotation matrix applied for
    //              significant attitude angles. REQ-AESA-010.
    //
    // PARAMETERS:
    //   bodyAz  [in]   Body-frame azimuth (degrees).
    //   bodyEl  [in]   Body-frame elevation (degrees).
    //   worldAz [out]  World-frame azimuth (degrees, [0, 360]).
    //   worldEl [out]  World-frame elevation (degrees).
    //
    // SIDE EFFECTS: None. Pure computation on currentPose_. REQ-AESA-010.
    // =========================================================================
    void applyAttitudeToBeam(double bodyAz, double bodyEl,
                             double& worldAz, double& worldEl) const;

    // =========================================================================
    // FUNCTION:    computeOcclusion
    //
    // DESCRIPTION: Computes the ITU-R P.526-15 knife-edge diffraction loss
    //              for a candidate target, considering all other targets in
    //              allTargets as potential occluders. Returns a three-state
    //              occlusion result (LIT, PENUMBRA, SHADOW). REQ-AESA-070.
    //
    // PARAMETERS:
    //   candidate   [in]  Target whose occlusion is being assessed.
    //   allTargets  [in]  All platforms (including candidate) this tick.
    //   cfg         [in]  Radar config. Uses frequency_Hz for wavelength.
    //
    // RETURNS:    OcclusionResult with zone, powerReduction, diffractionLoss_dB.
    // SIDE EFFECTS: None. Pure computation. REQ-AESA-070.
    // =========================================================================
    OcclusionResult computeOcclusion(
        const TargetInput& candidate,
        const std::vector<TargetInput>& allTargets,
        const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    computeKnifeEdgeDiffraction
    //
    // DESCRIPTION: Computes knife-edge diffraction loss J(nu) in dB using
    //              the ITU-R P.526-15 piecewise polynomial approximation.
    //              Called by computeOcclusion() for each occluder-target pair.
    //              REQ-AESA-070. Full comment in radarmodel_aesa.cpp.
    //
    // PARAMETERS:
    //   nu  [in]  Fresnel-Kirchhoff diffraction parameter (dimensionless).
    //
    // RETURNS:    Diffraction loss J (dB). Range: [0.0, ~40 dB].
    // SIDE EFFECTS: None. Pure computation.
    // =========================================================================
    double computeKnifeEdgeDiffraction(double nu) const;
};

} // namespace aesa

// =============================================================================
// SECTION 14: C ABI
//
// DESCRIPTION:  Plain C interface for use by simulation engines or scripting
//               layers that cannot link against C++ directly. Wraps
//               RadarModel_AESA construction and destruction.
//               REQ-AESA-001.
// =============================================================================
extern "C"
{
// Create a new RadarModel_AESA on the heap. Caller owns the pointer.
// Must be paired with a call to aesaradar_destroy(). REQ-AESA-001.
aesa::RadarModel_AESA* aesaradar_create();

// Destroy a RadarModel_AESA created by aesaradar_create(). REQ-AESA-001.
// p must be a valid non-null pointer from aesaradar_create().
void aesaradar_destroy(aesa::RadarModel_AESA* p);
}

#endif // RADARMODEL_AESA_H
// #pragma once
// #ifndef RADARMODEL_AESA_H
// #define RADARMODEL_AESA_H
// // =============================================================================
// // radarmodel_aesa.h  —  Public API for the AESA radar simulation model


// // =============================================================================

// #include <array>
// #include <cmath>
// #include <cstdint>
// #include <memory>
// #include <mutex>
// #include <random>           // std::normal_distribution — must be before class defs
// #include <string>
// #include <unordered_map>    // std::unordered_map — must be before class defs
// #include <vector>

// namespace aesa {

// // =============================================================================
// // §1  Enumerations
// // =============================================================================

// enum class RadarMode       { SURVEILLANCE, TWS, LOCK_ON };
// enum class SurfaceType     { AIR, SEA, LAND };
// enum class ModulationType  { NONE, LFM, NLFM, FMCW };
// enum class PRFType         { FIXED, STAGGERED, JITTERED, SWITCHED };
// enum class DetectionCategory { ALL, AIR_ONLY, SURFACE_ONLY };
// enum class SidelobeMode    { NORMAL, LOW_SLL, ULTRA_LOW };

// /// FIX-03  Jammer type
// enum class JammerType      { NOISE, DRFM, STAND_OFF_NOISE };

// /// FIX-07  Swerling target fluctuation model
// enum class SwerlingCase    { CASE_0, CASE_I, CASE_II, CASE_III, CASE_IV };

// /// FIX-06  PRF regime
// enum class WaveformMode    { HPRF, MPRF, LPRF, AUTO };

// /// FIX-04  IFF interrogation mode
// enum class IFFMode         { OFF, MODE_3A, MODE_4, MODE_5 };

// /// FIX-04  IFF response classification
// enum class IFFResponseCode { NO_REPLY, FRIENDLY, UNKNOWN, HOSTILE, CORRUPTED };

// // =============================================================================
// // §2  Sub-structs
// // =============================================================================
// // ============================================================================
// // Occlusion result — three-state shadow model
// // Ref: ITU-R P.526-15 knife-edge diffraction
// //      Ruck et al Radar Cross Section Handbook Ch 5 — shadow boundaries
// // ============================================================================
// struct OcclusionResult {
//     enum class Zone { LIT, PENUMBRA, SHADOW };
//     Zone   zone              = Zone::LIT;
//     double powerReduction    = 1.0;   // linear, 1.0 = full power, 0.0 = fully blocked
//     double diffractionLoss_dB = 0.0;  // additional path loss due to diffraction
// };
// struct NullSteering {
//     bool   active        = false;
//     double azimuth_deg   = 0.0;
//     double elevation_deg = 0.0;
//     float  nullDepth_dB  = -30.0f;
// };

// struct BeamWaveform {
//     ModulationType modulation     = ModulationType::LFM;
//     float          pulseWidth_s   = 50e-6f;
//     float          prf_Hz         = 300.0f;
//     float          bandwidth_Hz   = 5e6f;
//     int            pulsesPerDwell = 10;
//     WaveformMode   mode           = WaveformMode::AUTO;
//     float          prf2_Hz        = 0.0f;   // second PRF for staggered mode; 0 = disabled

// };

// /// FIX-06  One entry in the range-keyed waveform selection table
// struct WaveformEntry {
//     float        maxRange_m = 0.0f;
//     BeamWaveform waveform;
// };

// struct BeamRequest {
//     enum class Task { SEARCH, TRACK, FIRE_CONTROL, HORIZON_SEARCH };

//     Task         task            = Task::SEARCH;
//     double       azimuth_deg    = 0.0;
//     double       elevation_deg  = 0.0;
//     double       dwellTime_ms   = 2.0;
//     uint32_t     targetID       = 0;
//     int          priority       = 0;
//     BeamWaveform waveform;
//     float        spoilFactor    = 1.0f; // FIX-13
// };

// // =============================================================================
// // §3  Noise / jammer structs
// // =============================================================================

// struct NoiseModel {
//     double rangeStdDev     = 30.0;
//     double azimuthStdDev   = 0.1;
//     double elevationStdDev = 0.1;
//     double dopplerStdDev   = 1.0;
// };

// struct JammerConfig {
//     bool       active        = false;
//     JammerType type          = JammerType::NOISE;  // FIX-03
//     double     power_kW      = 0.0;
//     double     gain_dBi      = 0.0;
//     double     bandwidth_Hz  = 1e6;
//     double     range_m       = 0.0;
//     bool       selfScreening = false;

//     // FIX-03  DRFM-specific
//     float  drfmPullOffRate_m_s    = 150.0f;
//     float  drfmVelocityOffset_m_s = 50.0f;
//     bool   gateStealingActive     = false;
//     double pullOffDistance_m      = 0.0;
//     // ADD THESE — RGPO/VGPO
//     bool   rgpoActive          = false;   // Range Gate Pull-Off
//     float  rgpoRate_m_s        = 200.0f;  // pull-off rate
//     float  rgpoMaxOffset_m     = 5000.0f; // max pull-off distance
//     bool   vgpoActive          = false;   // Velocity Gate Pull-Off
//     float  vgpoRate_m_s2       = 50.0f;   // acceleration of false Doppler
//     float  vgpoMaxOffset_m_s   = 300.0f;  // max velocity offset
//     bool   noiseModulation     = false;   // amplitude noise on DRFM return
//     float  jamStrobe_dB        = 0.0f;    // additional strobe power above noise
// };

// // =============================================================================
// // §4  Chaff cloud  (FIX-10)
// // =============================================================================

// struct ChaffCloud {
//     double   x = 0.0, y = 0.0, z = 0.0;
//     double   radius_m    = 200.0;
//     double   rcsTotal    = 1000.0;
//     double   decayTime_s = 60.0;
//     double   birthTime_s = 0.0;
//     uint32_t sourceID    = 0;
// };

// // =============================================================================
// // §5  IFF  (FIX-04)
// // =============================================================================

// struct IFFResult {
//     IFFResponseCode response   = IFFResponseCode::NO_REPLY;
//     uint32_t        squawk     = 0;
//     double          confidence = 0.0;
//     IFFMode         modeUsed   = IFFMode::OFF;
// };
// struct AtmosphericConditions {
//     float temperature_C  = 15.0f;    // °C  — ISA standard = 15°C
//     float humidity_pct   = 60.0f;    // %   — midlatitude standard = 60%
//     float pressure_hPa   = 1013.25f; // hPa — sea-level standard
//     float rainRate_mmph  = 0.0f;     // mm/h — 0=clear, 4=light, 16=moderate, 100=heavy
//     float fogVisibility_m= 0.0f;     // m   — 0=clear, <1000=fog, <200=dense fog
// };
// // =============================================================================
// // §6  RadarConfig
// // =============================================================================

// struct RadarConfig {
//     // ---- Propagation --------------------------------------------------------
//     double earthRadiusFactor  = 1.33;
//     double atmosphericFactor  = 1.0;
//    // double rainRate_mmph      = 0.0;
//     //double fogVisibility_m    = 0.0;
//     AtmosphericConditions atmosphere;   // all weather in here now

//     // ---- Environment (clutter)
//     float  seaState    = 2.0f;   // Douglas sea state 0-6; used by signal processor
//     float  landClutter = 0.0f;   // 0=none, 1=heavy; used by signal processor

//     // ---- AESA array ---------------------------------------------------------
//     int   numElements           = 1000;
//     float peakPowerPerElement_W = 10.0f;
//     float moduleEfficiency      = 0.7f;
//     int   failedModules         = 0;
//     float maxDutyCycle          = 0.25f;  // FIX-08

//     // ---- Antenna / beam -----------------------------------------------------
//     double frequency_Hz          = 10.0e9;
//     float  antennaGain           = 34.0f;
//     double antennaBandwidth      = 100e6;
//     float  beamWidth             = 2.0f;
//     float  maxSteeringAngle_deg  = 60.0f;

//     // ---- Sidelobe control ---------------------------------------------------
//     SidelobeMode sidelobeMode   = SidelobeMode::NORMAL;
//     float peakSidelobeLevel     = -40.0f;
//     float avgSidelobeLevel      = -50.0f;
//     float sidelobeBlanking_dB   = -15.0f; // FIX-11

//     // ---- Null steering
//     NullSteering nullSteering;

//     // ---- FoV ----------------------------------------------------------------
//     float minElevation = -10.0f;
//     float maxElevation =  60.0f;
//     float minAzimuth   = -60.0f;
//     float maxAzimuth   =  60.0f;

//     // ---- Per-task dwell times -----------------------------------------------
//     float searchDwellTime_ms      = 2.0f;
//     float trackDwellTime_ms       = 1.0f;
//     float fireControlDwellTime_ms = 5.0f;

//     // ---- Per-task default waveforms -----------------------------------------
//     BeamWaveform searchWaveform      = { ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF };
//     BeamWaveform trackWaveform       = { ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 10, WaveformMode::MPRF };
//     BeamWaveform fireControlWaveform = { ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 20, WaveformMode::HPRF };

//     // ---- FIX-06  Waveform table (sorted maxRange ascending, 0 = sentinel)
//     WaveformEntry waveformTable[6] = {
//         { 30000.0f,  { ModulationType::NLFM,  5e-6f, 2000.0f, 50e6f, 20, WaveformMode::HPRF } },
//         { 100000.0f, { ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 10, WaveformMode::MPRF } },
//         { 400000.0f, { ModulationType::LFM,  50e-6f,  300.0f,  5e6f, 10, WaveformMode::LPRF } },
//         { 0.0f, {} }, { 0.0f, {} }, { 0.0f, {} }
//     };

//     // ---- PRF ----------------------------------------------------------------
//     float   prfLevels[4] = { 300.0f, 1000.0f, 3000.0f, 0.0f };
//     PRFType prfType      = PRFType::FIXED;

//     // ---- Frequency agility --------------------------------------------------
//     bool  frequencyAgility  = true;
//     float hopStartFrequency = 9.0e9f;
//     float hopStopFrequency  = 11.0e9f;

//     // ---- Receiver -----------------------------------------------------------
//     double systemTemperature_K = 290.0;
//     double noiseFigure_dB      = 4.0;
//     double targetPfa           = 1e-6;

//     // ---- Platform -----------------------------------------------------------
//     double radarHeight        = 10000.0;
//     double minDetectableRange = 100.0;
//     float  platformSpeed_m_s  = 250.0f;   // FIX-01 clutter notch

//     // ---- Target category ---------------------------------------------------
//     DetectionCategory targetCategory = DetectionCategory::AIR_ONLY;

//     // ---- Track lifecycle ---------------------------------------------------
//     int    missedScansToDrop  = 3;
//     double trackCoastSeconds  = 30.0;
//     int    minHitsToValidate  = 2;
//     double maxTrackSpeed      = 3000.0;
//     double manoeuvreThreshold_m = 500.0;

//     // ---- FIX-04  IFF -------------------------------------------------------
//     IFFMode              interrogationMode = IFFMode::MODE_3A;
//     std::vector<uint32_t> friendlySquawks;

//     // ---- FIX-05  JPDA ------------------------------------------------------
//     bool  useJPDA             = true;
//     float jpdaFalseAlarmDensity = 1e-6f;

//     // ---- Measurement noise -------------------------------------------------
//     NoiseModel noise;

//     // ---- Emitter identity --------------------------------------------------
//     std::string emitterID   = "";
//     uint32_t    emitterCode = 0;

//     // ---- Mode --------------------------------------------------------------
//     RadarMode mode           = RadarMode::TWS;
//     uint32_t  lockedTargetID = 0;
// };

// // =============================================================================
// // §7  RadarPose
// // =============================================================================

// struct RadarPose {
//     double x = 0.0, y = 0.0, z = 0.0;
//     float  roll = 0.0f, pitch = 0.0f, heading = 0.0f;
// };
// // ============================================================================
// // Material type — controls surface attenuation factor
// // Values calibrated against measured open-literature RCS data
// // Ref: Knott, Shaeffer, Tuley "Radar Cross Section" 2nd Ed, Table 5.1
// // ============================================================================
// enum class TargetMaterialType {
//     METAL,       //  0 dB reduction — bare aluminium/steel
//     COMPOSITE,   // -3 dB reduction — carbon fibre airframe
//     RAM,         // -15 dB reduction — radar absorbing material coating
//     STEALTHY     // -25 dB reduction — full VLO treatment
// };

// // ============================================================================
// // Shape type — controls surface coherence efficiency per face
// // Accounts for curvature reducing specular return vs flat-plate ideal
// // Ref: Ruck et al "Radar Cross Section Handbook", Plenum 1970, Ch 4
// // ============================================================================
// enum class TargetShapeType {
//     BOX,        // flat sides + corner reflectors — ground vehicle, container
//     AIRCRAFT,   // curved fuselage, blended wing edges
//     SHIP,       // large flat superstructure
//     MISSILE,    // cylindrical body + end caps
//     GENERIC     // fallback
// };

// struct TargetDimensions {
//     double length = 0.0;   // metres — forward axis
//     double height = 0.0;   // metres — vertical axis
//     double width  = 0.0;   // metres — lateral axis
//     bool   valid  = false;
//     TargetMaterialType material = TargetMaterialType::METAL;
//     TargetShapeType    shape    = TargetShapeType::GENERIC;
// };
// // =============================================================================
// // §8  TargetInput
// // =============================================================================

// struct TargetInput {
//     uint32_t    id      = 0;
//     double      x = 0.0, y = 0.0, z = 0.0;
//     double      vx = 0.0, vy = 0.0, vz = 0.0;
//     double      rcs     = 1.0;
//     SurfaceType surface = SurfaceType::AIR;
//     JammerConfig jammer;

//     SwerlingCase swerlingCase = SwerlingCase::CASE_I; // FIX-07

//     std::vector<std::pair<float,float>> rcsTable;  // aspect→RCS pairs
//     std::string platformType = "GENERIC";          // "FIGHTER","BOMBER","UAV","MISSILE","SHIP"
//     TargetDimensions dimensions;
//     // FIX-04  IFF transponder
//     bool     hasIFF    = false;
//     uint32_t iffSquawk = 0;
//     IFFMode  iffMode   = IFFMode::MODE_3A;
// };

// // =============================================================================
// // §9  Signal intercept
// // =============================================================================

// struct SignalIntercept {
//     uint32_t       targetID        = 0;
//     double         frequency_Hz    = 0.0;
//     double         pri_s           = 0.0;
//     double         pulseWidth_s    = 0.0;
//     double         signalLevel_dBW = 0.0;
//     int            priCount = 0, pwCount = 0, freqCount = 0, signalDepth = 0;
//     ModulationType modulation      = ModulationType::NONE;
//     std::string    emitterID       = "";
// };

// struct SignalLibraryEntry {
//     std::string    emitterID         = "";
//     uint32_t       emitterCode       = 0;
//     double         frequency_Hz      = 0.0;
//     double         freqTolerance_Hz  = 1e6;
//     double         pri_s             = 0.0;
//     double         priTolerance_s    = 1e-5;
//     double         pulseWidth_s      = 0.0;
//     double         pwTolerance_s     = 1e-7;
//     ModulationType modulation        = ModulationType::NONE;
//     std::string    description       = "";
// };

// // =============================================================================
// // §10  Output structs
// // =============================================================================

// struct DetectionOutput {
//     uint32_t targetID      = 0;
//     double range           = 0.0;
//     double azimuth         = 0.0;
//     double elevation       = 0.0;
//     double snr             = 0.0;
//     double radialVelocity  = 0.0;
//     double cpa_distance    = 0.0;
//     double time_to_cpa     = 0.0;
//     double Pk              = 0.0;
//     double heading         = 0.0;
//     double speedOverGround = 0.0;
//     double acceleration    = 0.0;
//     double targetAspect    = 0.0;
//     double azError_deg     = 0.0;  // FIX-02 monopulse
//     double elError_deg     = 0.0;  // FIX-02
//     bool   isAmbiguous     = false;
//     bool   lockBroken      = false;
//     bool   isDRFMGhost     = false; // FIX-03
//     bool   inDopplerBlind  = false; // FIX-01
// };

// struct TrackOutput {
//     uint32_t id = 0;
//     double x = 0.0, y = 0.0, z = 0.0;
//     double vx = 0.0, vy = 0.0, vz = 0.0;
//     double range           = 0.0;
//     double azimuth         = 0.0;
//     double elevation       = 0.0;
//     double radialVelocity  = 0.0;
//     double speedOverGround = 0.0;
//     double heading         = 0.0;
//     double targetAspect    = 0.0;
//     double cpa_distance    = 0.0;
//     double time_to_cpa     = 0.0;
//     double Pk              = 0.0;
//     int    hitCount        = 0;
//     int    scanMissCount   = 0;
//     bool   isValidated     = false;
//     bool   wasAmbiguous    = false;
//     bool   isManoeuvring   = false;
//     double trackQuality    = 0.0;
//     bool   isDRFMSuspect   = false; // FIX-03
//     bool   isExternalTrack = false; // FIX-12
//     IFFResult iff;                  // FIX-04
// };

// struct RadarOutput {
//     std::vector<DetectionOutput> detections;
//     std::vector<TrackOutput>     tracks;
//     std::vector<SignalIntercept> intercepts;

//     double    currentAzimuth   = 0.0;
//     double    currentElevation = 0.0;
//     RadarMode mode             = RadarMode::SURVEILLANCE;
//     bool      lockBroken       = false;
//     double    displayRange_km  = 200.0;
//     BeamRequest::Task currentTask = BeamRequest::Task::SEARCH;
//     double    currentDutyCycle = 0.0; // FIX-08
// };

// // =============================================================================
// // §11  TrackFile  — internal Kalman state
// // =============================================================================

// struct TrackFile {
//     uint32_t id = 0;
//     double x = 0.0, y = 0.0, z = 0.0;
//     double vx = 0.0, vy = 0.0, vz = 0.0;
//     double range = 0.0, velocity = 0.0, predictedRange = 0.0;
//     double lastSeenTime = 0.0, lastTrackBeamTime = 0.0;
//     int    hitCount = 0;
//     bool   isValidated = false, isUpdated = false, wasAmbiguous = false;
//     int    missCount = 0, scanMissCount = 0;
//     bool   updatedThisScan = false;
//     bool   isManoeuvring   = false;
//     bool   isDRFMSuspect   = false; // FIX-03
//     bool   isExternalTrack = false; // FIX-12
//     double innovationMagnitude = 0.0;
//     double trackQuality    = 0.0;
//     IFFResult iff;                  // FIX-04

//     std::array<double, 6> X = {};
//     double P[6][6] = {};
//     double Q[6][6] = {};
//     double R[3][3] = {};
//     // ── IMM (2-model: CV-low-Q / CV-high-Q) ──────────────────────────────
//     double imm_mu[2]        = { 0.7, 0.3 };   // model probabilities
//     double imm_X[2][6]      = {};              // per-model state
//     double imm_P[2][6][6]   = {};              // per-model covariance
//     bool   immActive        = false;           // armed after minHitsToValidate
// };

// // =============================================================================
// // §12  Forward declarations
// // =============================================================================

// class RadarSignalProcessor_AESA;
// class RadarAntenna_AESA;
// class RadarScheduler;
// class RadarTracker_AESA;
// class RadarSignalLibrary_AESA;

// // =============================================================================
// // §13  RadarModel_AESA
// // =============================================================================

// class RadarModel_AESA
// {
// public:
//     RadarModel_AESA();
//     ~RadarModel_AESA();

//     // Lifecycle
//     void init (const RadarConfig& cfg);
//     void start();
//     void update(double dt, const RadarPose& pose,
//                 const std::vector<TargetInput>& worldInputs,
//                 double simTime);
//     void end();
//     void reset();

//     // Configuration
//     void        setConfig(const RadarConfig& cfg);
//     RadarConfig getConfig() const;

//     // Mode control
//     void setMode  (RadarMode mode);
//     void lockOn   (uint32_t targetID);
//     void breakLock();

//     // Output
//     RadarOutput getOutput() const;

//     // Utility
//     double computeMaxDetectionRange(double rcs = 3.0) const;
//     double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;

//     // Signal library
//     void loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries);

//     // FIX-10  Chaff
//     void addChaffCloud(const ChaffCloud& cloud);
//     void clearChaffClouds();

//     // FIX-12  External track injection (Link-16 / CEC)
//     void injectExternalTrack(const TrackOutput& ext);

// private:
//     RadarPose  currentPose_;
//     double     currentSimTime_ = 0.0;
//     bool firstScanComplete_ = false;
//     mutable std::mutex mutex_;
//     RadarConfig config_;

//     std::unique_ptr<RadarSignalProcessor_AESA> signal_;
//     std::unique_ptr<RadarAntenna_AESA>         antenna_;
//     std::unique_ptr<RadarScheduler>            scheduler_;
//     std::unique_ptr<RadarTracker_AESA>         tracker_;
//     std::unique_ptr<RadarSignalLibrary_AESA>   library_;

//     bool initialised_ = false, running_ = false;

//     mutable double cachedDisplayRange_km_ = 200.0;
//     mutable bool   displayRangeDirty_     = true;

//     RadarOutput latestOutput_;

//     std::vector<ChaffCloud>              chaffClouds_;    // FIX-10
//     std::unordered_map<uint32_t, double> drfmPullOff_;    // FIX-03 accumulated pull-off per target

//     // Internal pipeline helpers — signatures match implementations exactly
//     bool processTargetDetection(
//         const TargetInput& target,
//         const BeamRequest& beam,
//         double dt, double simTime,
//         double maxUnambiguousRange,
//         double maxUnambiguousRange2,
//         std::vector<DetectionOutput>& scanDetections,
//         std::normal_distribution<double>& rangeNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dopplerNoise);

//     void injectDRFMGhost(
//         const TargetInput& real,
//         const BeamRequest& beam,
//         double simTime,
//         double maxUnambiguousRange,
//         std::vector<DetectionOutput>& scanDetections,
//         std::normal_distribution<double>& rangeNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dopplerNoise);

//     // FIX-04  IFF per-track query
//     IFFResult queryIFF(const TrackFile& track,
//                        const std::vector<TargetInput>& worldInputs) const;

//     double computeMaxDetectionRange_locked(double rcs = 3.0) const;
//     void   rebuildSchedule();
//     void   applyAttitudeToBeam(double bodyAz, double bodyEl,
//                                double& worldAz, double& worldEl) const;
//     std::vector<DetectionOutput> scanDetectionCache_;
//     //std::vector<TrackOutput>     trackOutputCache_;   // ← ADD THIS
//     int lockMissCount_ = 0;
//     // ADD to private members:
//     std::unordered_map<uint32_t, double> rgpoPullOff_;   // range pull-off per target
//     std::unordered_map<uint32_t, double> vgpoPullOff_;   // velocity pull-off per target
//     void injectRGPOVGPO(
//         const TargetInput& real, const BeamRequest& beam,
//         double simTime, double dt, double maxUnambiguousRange,
//         std::vector<DetectionOutput>& scanDets,
//         std::normal_distribution<double>& rNoise,
//         std::normal_distribution<double>& azNoise,
//         std::normal_distribution<double>& elNoise,
//         std::normal_distribution<double>& dvNoise);
//     std::vector<TargetInput> currentWorldInputs_;

//     OcclusionResult computeOcclusion(
//         const TargetInput& candidate,
//         const std::vector<TargetInput>& allTargets,
//         const RadarConfig& cfg) const;

//     double computeKnifeEdgeDiffraction(double nu) const;
// };

// } // namespace aesa

// // =============================================================================
// // §14  C ABI
// // =============================================================================

// extern "C"
// {
// aesa::RadarModel_AESA* aesaradar_create();
// void                   aesaradar_destroy(aesa::RadarModel_AESA* p);
// }

// #endif // RADARMODEL_AESA_H
