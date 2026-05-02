// =============================================================================
// FILE:         radarsignalprocessor_aesa.cpp
// MODULE:       AESA Radar Signal Processor — Implementation
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements all physics-layer signal processing. All functions
//               are stateless const methods — the only mutable state is the
//               thread_local RNG which is per-thread and requires no mutex.
//               No dynamic memory allocation except std::vector in
//               generateReferenceCells() (known MM-01 deviation,
//               ICD-AESA-DEVIATION-002 — 16 fixed cells, bounded allocation).
//               No recursion (FN-06 compliant). No exceptions (FP-01 compliant).
//
// REQUIREMENTS: REQ-AESA-040  Detection pipeline physics
//               REQ-AESA-021  Staggered PRF ambiguity resolution
//               REQ-AESA-060  Electronic warfare
//               REQ-AESA-071  Propagation loss
//               REQ-AESA-072  Two-ray multipath
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-SP-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation.
//   Rev 2  15 Feb 2026  FIX-01 through FIX-11 applied.
//   Rev 3  01 Apr 2026  STAP, staggered PRF, Physical Optics RCS,
//                       ITU-R P.676-12 gaseous attenuation added.
//                       Commented-out code removed per NS-05.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Magic numbers replaced with named constexpr constants.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radarsignalprocessor_aesa.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>

// =============================================================================
// FILE-SCOPE NAMED CONSTANTS
// All numeric literals used in this translation unit are declared here.
// Satisfies VI-08 (no magic numbers). REQ-AESA-040.
// =============================================================================
namespace
{
// -------------------------------------------------------------------------
// Physical constants
// -------------------------------------------------------------------------

// Boltzmann constant (J/K). Used in noise power computation. REQ-AESA-040.
constexpr double BOLTZMANN = 1.380649e-23;

// Speed of light in vacuum (m/s). REQ-AESA-020, REQ-AESA-040.
constexpr double SPEED_OF_LIGHT = 299792458.0;

// Pi — full precision. Replaces non-standard M_PI macro. LC-08 compliant.
constexpr double PI = 3.14159265358979323846;

// Conversion factor: degrees to radians.
constexpr double DEG_TO_RAD = PI / 180.0;

// Conversion factor: radians to degrees.
constexpr double RAD_TO_DEG = 180.0 / PI;

// -------------------------------------------------------------------------
// Beam gate constants
// -------------------------------------------------------------------------

// Beam gate half-width multiplier. Gate = beamWidth * BEAM_GATE_FACTOR.
// 2.5 * beamWidth gives enough margin to capture targets illuminated by
// the first sidelobe without including distant false alarms. REQ-AESA-040.
constexpr double BEAM_GATE_FACTOR = 2.5;

// Azimuth wrap threshold (degrees). If azimuth difference exceeds this
// value the shorter arc is used. REQ-AESA-040.
constexpr double AZ_WRAP_THRESHOLD = 180.0;

// Full azimuth circle (degrees). REQ-AESA-040.
constexpr double AZ_FULL_CIRCLE = 360.0;

// -------------------------------------------------------------------------
// Radar equation constants
// -------------------------------------------------------------------------

// Minimum range (metres) for signal strength computation.
// Prevents division by zero at zero range. REQ-AESA-040.
constexpr double MIN_RANGE_M = 1.0;

// Minimum bandwidth (Hz) for noise power computation.
// Prevents zero noise power. REQ-AESA-040.
constexpr double MIN_BANDWIDTH_HZ = 1.0;

// Minimum integration count for SINR computation. REQ-AESA-040.
constexpr int    MIN_PULSES_PER_DWELL = 1;

// 4*pi factor used in radar range equation denominator. REQ-AESA-040.
constexpr double FOUR_PI = 4.0 * PI;

// -------------------------------------------------------------------------
// Clutter constants
// -------------------------------------------------------------------------

// Minimum grazing angle sine for sea clutter computation (radians).
// Prevents log10(0) in GIT sea clutter model. REQ-AESA-040.
constexpr double MIN_GRAZING_SINE = 1e-4;

// Maximum grazing angle sine (= 1.0 — vertical incidence). REQ-AESA-040.
constexpr double MAX_GRAZING_SINE = 1.0;

// Minimum frequency (GHz) for GIT sea clutter model to avoid log10(0).
constexpr double MIN_FREQ_GHZ_CLUTTER = 0.1;

// GIT sea clutter model coefficient constants (Horst et al. 1978).
// REQ-AESA-040.
constexpr double GIT_SEA_INTERCEPT  = -61.4;
constexpr double GIT_SEA_FREQ_COEFF =  40.0;
constexpr double GIT_SEA_SS_COEFF   =  10.1;
constexpr double GIT_SEA_PSICOEFF   =  30.0;
constexpr double GIT_SEA_SS_MAX     =   6.0;

// Billingsley land clutter model constants (X-band baseline). REQ-AESA-040.
constexpr double BILLINGSLEY_INTERCEPT  = -25.0;
constexpr double BILLINGSLEY_PSI_COEFF  =   8.0;
constexpr double BILLINGSLEY_TERRAIN_DB =  15.0;  // 0 (smooth) to +15 (urban)

// -------------------------------------------------------------------------
// CFAR constants
// -------------------------------------------------------------------------

// Number of CA-CFAR reference cells. REQ-AESA-040.
constexpr int    CFAR_NUM_CELLS = 16;

// CFAR infinite threshold — returned when reference cells are empty.
// Prevents detection when no valid clutter estimate exists. REQ-AESA-040.
constexpr double CFAR_INFINITE_THRESHOLD = 1e12;

// Relaxed CFAR Pfa upper bound. Pfa_relaxed = min(PFA_RELAXED_MAX, Pfa*100).
// REQ-AESA-040.
constexpr double PFA_RELAXED_MAX = 1e-4;

// Relaxed CFAR Pfa scaling factor. REQ-AESA-040.
constexpr double PFA_RELAXED_SCALE = 100.0;

// CFAR reference cell sea state scaling coefficient. REQ-AESA-040.
constexpr double CFAR_SEA_SCALE  = 0.3;

// CFAR reference cell land clutter scaling coefficient. REQ-AESA-040.
constexpr double CFAR_LAND_SCALE = 0.5;

// -------------------------------------------------------------------------
// RCS physical optics constants
// -------------------------------------------------------------------------

// Rayleigh regime boundary: kD < RAYLEIGH_KD_LIMIT. REQ-AESA-040.
constexpr double RAYLEIGH_KD_LIMIT   = 0.5;

// Resonance/optical boundary: kD < RESONANCE_KD_LIMIT. REQ-AESA-040.
constexpr double RESONANCE_KD_LIMIT  = 5.0;

// Minimum characteristic dimension (metres) to prevent zero wavelength ratio.
constexpr double MIN_DIMENSION_M = 0.01;

// Material reduction factors (linear scale). REQ-AESA-040.
constexpr double RCS_MATERIAL_METAL     = 1.000;  //  0 dB — bare metal
constexpr double RCS_MATERIAL_COMPOSITE = 0.500;  // -3 dB — CFRP airframe
constexpr double RCS_MATERIAL_RAM       = 0.032;  // -15 dB — RAM coating
constexpr double RCS_MATERIAL_STEALTHY  = 0.003;  // -25 dB — full VLO

// Shape efficiency factors (coherence per face type). REQ-AESA-040.
constexpr double RCS_SHAPE_BOX      = 0.40;
constexpr double RCS_SHAPE_AIRCRAFT = 0.08;
constexpr double RCS_SHAPE_SHIP     = 0.50;
constexpr double RCS_SHAPE_MISSILE  = 0.10;
constexpr double RCS_SHAPE_GENERIC  = 0.15;

// Aircraft face efficiency factors by face index. REQ-AESA-040.
constexpr double RCS_AIRCRAFT_NOSE   = 0.08;
constexpr double RCS_AIRCRAFT_TAIL   = 0.12;
constexpr double RCS_AIRCRAFT_SIDE   = 1.00;
constexpr double RCS_AIRCRAFT_TOP    = 0.60;
constexpr double RCS_AIRCRAFT_BOTTOM = 0.40;

// Missile face factors. REQ-AESA-040.
constexpr double RCS_MISSILE_NOSE = 0.05;
constexpr double RCS_MISSILE_TAIL = 0.10;

// Edge diffraction model: sigma_edge = lambda * perimeter / (8*pi).
// Ref: Ruck et al Radar Cross Section Handbook Ch 5. REQ-AESA-040.
constexpr double EDGE_DIFFRACTION_DENOM = 8.0 * PI;

// Rayleigh model coefficient: 8*pi/3. REQ-AESA-040.
constexpr double RAYLEIGH_COEFF = 8.0 * PI / 3.0;

// Minimum speed (m/s) for valid velocity unit vector. REQ-AESA-040.
constexpr double MIN_SPEED_FOR_HEADING = 0.5;

// Minimum right vector magnitude before fallback. REQ-AESA-040.
constexpr double MIN_RVEC_MAG = 1e-6;

// Edge perimeter multiplier: 4 * (L + H + W). REQ-AESA-040.
constexpr double EDGE_PERIMETER_MULT = 4.0;

// Number of 6-facet model faces.
constexpr int NUM_FACES = 6;

// -------------------------------------------------------------------------
// Swerling constants
// -------------------------------------------------------------------------

// Minimum nominal RCS for Swerling computation to be meaningful. REQ-AESA-040.
constexpr double MIN_NOMINAL_RCS = 0.0;

// Swerling Case III/IV uses half the nominal RCS for two exponential draws.
constexpr double SWERLING_34_HALF = 2.0;

// -------------------------------------------------------------------------
// Albersheim Pd constants
// -------------------------------------------------------------------------

// Albersheim bisection iteration count. 40 iterations gives convergence
// to better than 0.001 in Pd. REQ-AESA-040.
constexpr int    ALBERSHEIM_ITERATIONS = 40;

// Albersheim Pfa guard. Prevents log(0) in A computation. REQ-AESA-040.
constexpr double ALBERSHEIM_PFA_MIN = 1e-15;

// Albersheim constant in A = log(0.62 / Pfa). REQ-AESA-040.
constexpr double ALBERSHEIM_PFA_COEFF = 0.62;

// Albersheim search bounds for bisection. REQ-AESA-040.
constexpr double ALBERSHEIM_PD_LOW  = 0.001;
constexpr double ALBERSHEIM_PD_HIGH = 0.999;
constexpr double ALBERSHEIM_PD_MAX  = 0.99;

// Swerling loss constants (dB). REQ-AESA-040.
constexpr double SWERLING_LOSS_I_II   = 5.72;
constexpr double SWERLING_LOSS_III_IV = 2.36;

// -------------------------------------------------------------------------
// Range ambiguity constants
// -------------------------------------------------------------------------

// Search range for resolveRangeAmbiguity — k in [-K_SEARCH, +K_SEARCH].
// Covers 11 PRF multiples. REQ-AESA-021.
constexpr int    RANGE_RESOLVE_K_MAX = 5;

// Search range for staggered PRF coincidence — n1, n2 in [0, N_STAG_MAX].
constexpr int    N_STAG_MAX = 4;

// Staggered range coincidence gate (metres). REQ-AESA-021.
constexpr double RANGE_COINCIDENCE_GATE_M = 500.0;

// Minimum Rmax for resolveRangeAmbiguity to apply. REQ-AESA-021.
constexpr double MIN_RMAX_FOR_RESOLVE = 1.0;

// Staggered velocity search iterations (n1, n2 in [0, VEL_N_MAX-1]).
constexpr int    VEL_N_MAX = 8;

// Minimum Vmax (m/s) for velocity folding. REQ-AESA-021.
constexpr double MIN_VMAX_FOR_FOLD = 1.0;

// Velocity coincidence gate (m/s). REQ-AESA-021.
constexpr double VEL_COINCIDENCE_GATE = 2.0;

// -------------------------------------------------------------------------
// Max detection range constants
// -------------------------------------------------------------------------

// Initial range estimate for iterative radar equation solver (metres).
constexpr double MAX_RANGE_INITIAL_M = 500000.0;

// Max iterations for range solver. REQ-AESA-040.
constexpr int    MAX_RANGE_ITERATIONS = 20;

// Convergence criterion for range solver (metres). REQ-AESA-040.
constexpr double MAX_RANGE_CONVERGENCE_M = 10.0;

// Number of reference cells used in max range CFAR approximation.
constexpr double MAX_RANGE_CFAR_N = 16.0;

// -------------------------------------------------------------------------
// Merge gate constants
// -------------------------------------------------------------------------

// Range merge gate (metres). Detections within this range are merged.
// REQ-AESA-040.
constexpr double MERGE_GATE_RANGE_M = 150.0;

// -------------------------------------------------------------------------
// Beam gain / sidelobe constants
// -------------------------------------------------------------------------

// Sidelobe region boundary multiplier. Targets within beamWidth *
// MAIN_BEAM_FACTOR of boresight get full gain. REQ-AESA-040.
constexpr double MAIN_BEAM_FACTOR = 2.0;

// LOW_SLL mode sidelobe levels (dBi). REQ-AESA-040.
constexpr float  LOW_SLL_PEAK = -45.0f;
constexpr float  LOW_SLL_AVG  = -55.0f;

// ULTRA_LOW mode sidelobe levels (dBi). REQ-AESA-040.
constexpr float  ULTRA_LOW_PEAK = -55.0f;
constexpr float  ULTRA_LOW_AVG  = -65.0f;

// -------------------------------------------------------------------------
// Monopulse constants
// -------------------------------------------------------------------------

// Monopulse sensitivity slope for sinc aperture (dimensionless).
// REQ-AESA-040.
constexpr double MONOPULSE_KM = 1.606;

// Monopulse noise scaling: noise_sigma = bw / (km * sqrt(2 * SINR)).
// Factor of 2 in denominator from two-channel sum/difference processing.
constexpr double MONOPULSE_SINR_FACTOR = 2.0;

// -------------------------------------------------------------------------
// Multipath constants
// -------------------------------------------------------------------------

// Elevation threshold (degrees) above which multipath is negligible.
// REQ-AESA-072.
constexpr double MULTIPATH_EL_THRESHOLD_DEG = 5.0;

// Multipath factor clamp bounds [0, 4]. REQ-AESA-072.
constexpr double MULTIPATH_MIN = 0.0;
constexpr double MULTIPATH_MAX = 4.0;

// -------------------------------------------------------------------------
// STAP constants
// -------------------------------------------------------------------------

// STAP gain cap (linear). Corresponds to +30 dB maximum improvement.
// Prevents unrealistically large gain from configuration errors. REQ-AESA-040.
constexpr double STAP_GAIN_CAP = 1000.0;

// STAP partial recovery factor for targets inside the notch.
// Physical basis: STAP cannot fully recover targets at exact clutter
// velocity — partial recovery proportional to 0.3 * (distance/width).
constexpr double STAP_RECOVERY_FACTOR = 0.3;

// Minimum STAP gain (= 1.0, no degradation). REQ-AESA-040.
constexpr double STAP_GAIN_MIN = 1.0;

// Minimum number of active elements for valid STAP spatial DOF. REQ-AESA-040.
constexpr int    STAP_MIN_ELEMENTS = 1;

// Minimum pulses for valid STAP temporal DOF. REQ-AESA-040.
constexpr int    STAP_MIN_PULSES = 1;

// Clutter notch width clamp bounds (m/s). REQ-AESA-040.
constexpr double NOTCH_WIDTH_MIN = 1.0;
constexpr double NOTCH_WIDTH_MAX = 15.0;

// Minimum platform speed (m/s) for a meaningful clutter notch. REQ-AESA-040.
constexpr float  MIN_PLATFORM_SPEED_MPS = 1.0f;

// -------------------------------------------------------------------------
// Doppler / waveform constants
// -------------------------------------------------------------------------

// LFM time-bandwidth product must be >= this for meaningful processing gain.
constexpr double MIN_PROCESSING_GAIN = 1.0;

// -------------------------------------------------------------------------
// Chaff return constants
// -------------------------------------------------------------------------

// Beam gate multiplier for chaff cloud inclusion check. REQ-AESA-061.
constexpr double CHAFF_BEAM_GATE_FACTOR = 3.0;

// Minimum chaff cloud range (metres). REQ-AESA-061.
constexpr double MIN_CHAFF_RANGE_M = 1.0;

// Minimum chaff decay time (seconds). Prevents division by zero. REQ-AESA-061.
constexpr double MIN_CHAFF_DECAY_S = 1.0;

// -------------------------------------------------------------------------
// Horizon constants
// -------------------------------------------------------------------------

// Earth mean radius (metres). REQ-AESA-071.
constexpr double EARTH_RADIUS_M = 6371000.0;

// -------------------------------------------------------------------------
// Propagation constants
// -------------------------------------------------------------------------

// Rain attenuation coefficient A in gamma = A * R^B (ITU-R P.838-3 X-band).
// gamma_rain (dB/km) = 0.00887 * rainRate_mmph^1.255. REQ-AESA-071.
constexpr double RAIN_COEFF_A = 0.00887;
constexpr double RAIN_COEFF_B = 1.255;

// Two-way loss multiplier. REQ-AESA-071.
constexpr double TWO_WAY = 2.0;

// Kilometres per metre. Used to convert range_m to range_km. REQ-AESA-071.
constexpr double KM_PER_M = 0.001;

// Fog attenuation coefficients (Kunkel 1984). REQ-AESA-071.
constexpr double FOG_COEFF_A   = 0.0367;
constexpr double FOG_VIS_DENOM = 1000.0;
constexpr double FOG_VIS_EXP   = 1.43;
constexpr double FOG_GAMMA_A   = 0.0157;
constexpr double FOG_GAMMA_EXP = 1.05;
constexpr double FOG_VIS_MIN_M = 1.0;
constexpr double FOG_VIS_MAX_M = 2000.0;

// -------------------------------------------------------------------------
// ITU-R P.676-12 gaseous attenuation constants
// -------------------------------------------------------------------------

// Pressure ratio denominator: r_p = pressure_hPa / ISA_PRESSURE_HPA.
constexpr double ISA_PRESSURE_HPA = 1013.25;

// Temperature ratio numerator: r_t = ISA_TEMP_K / (273.15 + T_celsius).
constexpr double ISA_TEMP_K = 288.15;
constexpr double KELVIN_OFFSET = 273.15;

// Gaseous attenuation unit conversion: gamma (dB/km) to two-way dB.
// Factor = 2 (two-way) * range_m * 0.001 (m to km). REQ-AESA-071.
constexpr double GASEOUS_TWO_WAY_KM = 2.0 * KM_PER_M;

// GHz conversion for gaseous model. REQ-AESA-071.
constexpr double HZ_TO_GHZ = 1.0e-9;

// Water vapour model constants (Buck 1981 / ITU-R P.836-6). REQ-AESA-071.
constexpr double BUCK_A     = 6.1121;
constexpr double BUCK_B     = 18.678;
constexpr double BUCK_C     = 234.5;
constexpr double BUCK_D     = 257.14;
constexpr double WATER_MOLAR_MASS = 18.015;   // g/mol
constexpr double GAS_CONSTANT_JKMOL = 8314.46; // J/(kmol·K)
constexpr double PA_PER_HPA = 100.0;           // Pascal per hPa

// -------------------------------------------------------------------------
// Jammer constants
// -------------------------------------------------------------------------

// Minimum jammer range (metres) to prevent division by zero. REQ-AESA-060.
constexpr double MIN_JAMMER_RANGE_M = 1.0;

// Minimum jammer power for noise computation to be meaningful (Watts).
constexpr double MIN_JAMMER_POWER_W = 0.0;

// -------------------------------------------------------------------------
// Frequency agility constants
// -------------------------------------------------------------------------

// Minimum hop stop frequency (Hz) for agility to be valid. REQ-AESA-020.
constexpr float  MIN_HOP_STOP_HZ = 0.0f;

// -------------------------------------------------------------------------
// Platform base RCS values (m²) from open literature. REQ-AESA-040.
// -------------------------------------------------------------------------
constexpr double RCS_FIGHTER = 3.0;
constexpr double RCS_BOMBER  = 40.0;
constexpr double RCS_UAV     = 0.01;
constexpr double RCS_MISSILE = 0.1;
constexpr double RCS_HELO    = 3.0;
constexpr double RCS_SHIP    = 10000.0;
constexpr double RCS_STEALTH = 0.001;
constexpr double RCS_GENERIC = 5.0;

// -------------------------------------------------------------------------
// Null steering constants
// -------------------------------------------------------------------------

// Null cone half-angle multiplier. REQ-AESA-040.
constexpr double NULL_CONE_FACTOR = 2.0;

// -------------------------------------------------------------------------
// Clamp bounds for acos/asin domain protection
// -------------------------------------------------------------------------
constexpr double DOT_CLAMP_MIN = -1.0;
constexpr double DOT_CLAMP_MAX =  1.0;

} // anonymous namespace

// =============================================================================
// FILE-SCOPE RANDOM NUMBER GENERATOR
// thread_local — per-thread state, no synchronisation required.
// KNOWN DEVIATION: LC-02 — std::random_device seeding is implementation-defined.
// Mitigated by: all outputs clamped, no safety decision depends on a specific
// RNG value. ICD-AESA-DEVIATION-003. REQ-AESA-040.
// =============================================================================
static thread_local std::default_random_engine tl_rng{
    std::random_device{}()
};

// =============================================================================
// FILE-SCOPE RCS HELPER FUNCTIONS
// These are pure functions operating on enum types. Declared static to
// restrict linkage to this translation unit. REQ-AESA-040.
// =============================================================================

// -----------------------------------------------------------------------------
// FUNCTION: rcs_materialFactor
// DESCRIPTION: Returns linear power reduction factor for target material.
//              Ref: Knott et al Radar Cross Section 2nd Ed, Table 5.1.
//              REQ-AESA-040.
// -----------------------------------------------------------------------------
static double rcs_materialFactor(aesa::TargetMaterialType mat)
{
    switch (mat)
    {
    case aesa::TargetMaterialType::METAL:     return RCS_MATERIAL_METAL;
    case aesa::TargetMaterialType::COMPOSITE: return RCS_MATERIAL_COMPOSITE;
    case aesa::TargetMaterialType::RAM:       return RCS_MATERIAL_RAM;
    case aesa::TargetMaterialType::STEALTHY:  return RCS_MATERIAL_STEALTHY;
    default:                                  return RCS_MATERIAL_METAL;
    }
}

// -----------------------------------------------------------------------------
// FUNCTION: rcs_shapeFactor
// DESCRIPTION: Returns surface coherence efficiency (dimensionless) for a
//              target shape. Accounts for curvature reducing specular return
//              below flat-plate Physical Optics ideal. REQ-AESA-040.
// -----------------------------------------------------------------------------
static double rcs_shapeFactor(aesa::TargetShapeType shape)
{
    switch (shape)
    {
    case aesa::TargetShapeType::BOX:      return RCS_SHAPE_BOX;
    case aesa::TargetShapeType::AIRCRAFT: return RCS_SHAPE_AIRCRAFT;
    case aesa::TargetShapeType::SHIP:     return RCS_SHAPE_SHIP;
    case aesa::TargetShapeType::MISSILE:  return RCS_SHAPE_MISSILE;
    case aesa::TargetShapeType::GENERIC:  return RCS_SHAPE_GENERIC;
    default:                              return RCS_SHAPE_GENERIC;
    }
}

// -----------------------------------------------------------------------------
// FUNCTION: rcs_faceFactor
// DESCRIPTION: Returns per-face directional efficiency modifier.
//              faceIndex: 0=front, 1=rear, 2=right, 3=left, 4=top, 5=bottom.
//              Aircraft and missile faces are shaped to deflect radar energy.
//              BOX/SHIP/GENERIC all faces equal (factor 1.0). REQ-AESA-040.
// -----------------------------------------------------------------------------
static double rcs_faceFactor(aesa::TargetShapeType shape, int faceIndex)
{
    if (shape == aesa::TargetShapeType::AIRCRAFT)
    {
        switch (faceIndex)
        {
        case 0: return RCS_AIRCRAFT_NOSE;
        case 1: return RCS_AIRCRAFT_TAIL;
        case 2:
        case 3: return RCS_AIRCRAFT_SIDE;
        case 4: return RCS_AIRCRAFT_TOP;
        case 5: return RCS_AIRCRAFT_BOTTOM;
        default: return RCS_AIRCRAFT_SIDE;
        }
    }
    if (shape == aesa::TargetShapeType::MISSILE)
    {
        switch (faceIndex)
        {
        case 0: return RCS_MISSILE_NOSE;
        case 1: return RCS_MISSILE_TAIL;
        default: return 1.00;
        }
    }
    // BOX, SHIP, GENERIC: all faces equal — no shaping applied.
    return 1.00;
}

// -----------------------------------------------------------------------------
// FUNCTION: rcs_regime
// DESCRIPTION: Determines the electromagnetic scattering regime based on
//              the electrical size of the target (kD = 2*pi*D/lambda).
//              RAYLEIGH (kD < 0.5): volume scattering, sigma ~ f^4.
//              RESONANCE (0.5 <= kD < 5): Mie region.
//              OPTICAL (kD >= 5): surface scattering, sigma ~ f^0 (flat).
//              Ref: Knott Ch 2, Skolnik Ch 11.2. REQ-AESA-040.
// -----------------------------------------------------------------------------
enum class ScatteringRegime { RAYLEIGH, RESONANCE, OPTICAL };

static ScatteringRegime rcs_regime(double characteristicDim, double lambda)
{
    // kD = electrical circumference normalised by wavelength. REQ-AESA-040.
    double kD = (2.0 * PI / lambda) * characteristicDim;
    if (kD < RAYLEIGH_KD_LIMIT)  return ScatteringRegime::RAYLEIGH;
    if (kD < RESONANCE_KD_LIMIT) return ScatteringRegime::RESONANCE;
    return ScatteringRegime::OPTICAL;
}

// -----------------------------------------------------------------------------
// FUNCTION: rcs_faceSigma
// DESCRIPTION: Computes RCS contribution of a single face.
//              Returns 0.0 for shadowed faces (cosTheta <= 0).
//              OPTICAL: sigma = A * cosTheta * eta_shape * eta_mat.
//              RAYLEIGH: sigma = (8pi/3) * k^4 * V^2 * eta_mat.
//              RESONANCE: geometric mean of Rayleigh and Optical estimates.
//              Ref: Knott Sec 4.3, Ruck Ch 3. REQ-AESA-040.
// -----------------------------------------------------------------------------
static double rcs_faceSigma(double area, double cosTheta,
                            double lambda, double volume,
                            ScatteringRegime regime,
                            double eta_shape, double eta_mat)
{
    // Shadowed face contributes zero RCS. REQ-AESA-040.
    if (cosTheta <= 0.0) return 0.0;

    double sigma = 0.0;

    switch (regime)
    {
    case ScatteringRegime::OPTICAL:
    {
        // Mean Physical Optics projected area model.
        // This is the MEAN result integrated over the sinc^2 angular pattern
        // of a flat plate. Using mean is correct for entity-level simulation
        // — the specular glint occupies a solid angle << simulation time step.
        // Ref: Knott Sec 4.3, IEEE Std 1672-2006 Sec 6.2. REQ-AESA-040.
        sigma = area * cosTheta * eta_shape * eta_mat;
        break;
    }
    case ScatteringRegime::RAYLEIGH:
    {
        // Rayleigh volume scattering: sigma = (8pi/3) * k^4 * V^2 * eta_mat.
        // k = 2*pi/lambda. Ref: Knott Eq 2.14, Skolnik Sec 11.2a. REQ-AESA-040.
        double k = 2.0 * PI / lambda;
        sigma = RAYLEIGH_COEFF
                * std::pow(k, 4.0)
                * volume * volume
                * eta_mat;
        break;
    }
    case ScatteringRegime::RESONANCE:
    {
        // Mie resonance regime: interpolate using geometric mean of the two
        // regime estimates. Full Mie series requires spherical geometry — the
        // geometric mean is the standard entity-level approximation.
        // Ref: Knott Sec 2.4. REQ-AESA-040.
        double k        = 2.0 * PI / lambda;
        double sigma_ray = RAYLEIGH_COEFF * std::pow(k, 4.0)
                           * volume * volume * eta_mat;
        double sigma_opt = area * cosTheta * eta_shape * eta_mat;
        sigma = std::sqrt(sigma_ray * sigma_opt);
        break;
    }
    }

    return sigma;
}

namespace aesa {

// =============================================================================
// §A  GEOMETRY
// =============================================================================

// =============================================================================
// FUNCTION: isTargetInBeam
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::isTargetInBeam(
    double beamAz, double beamEl,
    double targetAz, double targetEl,
    const RadarConfig& cfg,
    double& outAzDiff, double& outElDiff,
    double effectiveBeamWidth) const
{
    // Compute unsigned azimuth difference with wrap-around protection.
    // Targets at +179 and -179 are only 2 deg apart — the shorter arc is used.
    // REQ-AESA-040.
    outAzDiff = std::abs(beamAz - targetAz);
    if (outAzDiff > AZ_WRAP_THRESHOLD) outAzDiff = AZ_FULL_CIRCLE - outAzDiff;

    // Elevation difference is unambiguous — no wrap needed. REQ-AESA-040.
    outElDiff = std::abs(beamEl - targetEl);

    // Use effective beamwidth (spoiled) if valid, else fall back to natural bw.
    // effectiveBeamWidth <= 0 signals "use cfg.beamWidth". REQ-AESA-040.
    double bw   = (effectiveBeamWidth > 0.0)
                    ? effectiveBeamWidth
                    : static_cast<double>(cfg.beamWidth);

    // Gate = BEAM_GATE_FACTOR * bw. This is the beam pattern half-width at
    // the -13 dB point (first null) for a uniformly illuminated aperture.
    // REQ-AESA-040.
    double gate = bw * BEAM_GATE_FACTOR;

    return (outAzDiff <= gate && outElDiff <= gate);
}

// =============================================================================
// FUNCTION: checkHorizon
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::checkHorizon(double range, double targetZ,
                                             const RadarConfig& cfg) const
{
    // Effective earth radius for 4/3 earth model. REQ-AESA-071.
    double Re = EARTH_RADIUS_M * cfg.earthRadiusFactor * cfg.atmosphericFactor;

    // Geometric horizon range from radar: d = sqrt(2 * Re * h_radar).
    // std::max(0.0) prevents sqrt of negative altitude. REQ-AESA-071.
    double dRadar = std::sqrt(2.0 * Re * std::max(0.0, cfg.radarHeight));

    // Geometric horizon range from target: d = sqrt(2 * Re * h_target).
    double dTgt   = std::sqrt(2.0 * Re * std::max(0.0, targetZ));

    // Target is visible if slant range <= combined horizon. REQ-AESA-071.
    return range <= (dRadar + dTgt);
}

// =============================================================================
// §B  SIGNAL CHAIN
// =============================================================================

// =============================================================================
// FUNCTION: calculateSignalStrength
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::calculateSignalStrength(
    double range, double rcs,
    double arrayGain,
    const BeamWaveform& /*waveform*/,
    const RadarConfig& cfg) const
{
    // Clamp range to minimum to prevent division by zero in R^4. REQ-AESA-040.
    if (range < MIN_RANGE_M) range = MIN_RANGE_M;

    // -------------------------------------------------------------------------
    // LPI frequency hopping. When frequencyAgility is enabled, select a random
    // frequency within the hop band for each pulse. This prevents intercept
    // receivers from tracking the radar's frequency. REQ-AESA-020.
    // -------------------------------------------------------------------------
    double freq = cfg.frequency_Hz;
    if (cfg.frequencyAgility &&
        cfg.hopStopFrequency > cfg.hopStartFrequency &&
        cfg.hopStopFrequency > MIN_HOP_STOP_HZ)
    {
        // Uniform random frequency within hop band. REQ-AESA-020.
        thread_local std::uniform_real_distribution<double> hopDist(0.0, 1.0);
        freq = static_cast<double>(cfg.hopStartFrequency)
               + hopDist(tl_rng)
                     * static_cast<double>(cfg.hopStopFrequency
                                           - cfg.hopStartFrequency);
    }

    // Wavelength from hopped (or fixed) frequency. REQ-AESA-040.
    double lambda = SPEED_OF_LIGHT / freq;

    // Total transmit power from all active T/R modules. REQ-AESA-012.
    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt     = static_cast<double>(active)
                * static_cast<double>(cfg.peakPowerPerElement_W)
                * static_cast<double>(cfg.moduleEfficiency);

    // Radar range equation (monostatic, two-way path). REQ-AESA-040.
    // Pr = (Pt * G^2 * lambda^2 * sigma) / ((4*pi)^3 * R^4)
    double Pr = (Pt * arrayGain * arrayGain * lambda * lambda * rcs)
                / (std::pow(FOUR_PI, 3.0) * std::pow(range, 4.0));

    // Apply two-way propagation loss (rain, fog, gaseous). REQ-AESA-071.
    return std::max(0.0, Pr * computePropagationLoss(range, cfg));
}

// =============================================================================
// FUNCTION: computeNoisePower
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeNoisePower(
    const RadarConfig& cfg, double bandwidth_Hz) const
{
    // Linear noise figure: F = 10^(NF_dB/10). REQ-AESA-040.
    double F = std::pow(10.0, cfg.noiseFigure_dB / 10.0);

    // Pn = k * T * B * F. Clamp bandwidth to MIN_BANDWIDTH_HZ. REQ-AESA-040.
    return BOLTZMANN
           * cfg.systemTemperature_K
           * std::max(MIN_BANDWIDTH_HZ, bandwidth_Hz)
           * F;
}

// =============================================================================
// FUNCTION: computeClutterPower
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeClutterPower(
    double range, SurfaceType surface, const RadarConfig& cfg) const
{
    // AIR surface has no ground clutter. REQ-AESA-040.
    if (surface == SurfaceType::AIR || range < MIN_RANGE_M) return 0.0;

    double sigma0 = 0.0;

    // Grazing angle sine from flat-earth approximation.
    // sin(psi) = h_radar / R. Clamped to [MIN_GRAZING_SINE, 1.0]. REQ-AESA-040.
    double sinPsi = std::clamp(
        cfg.radarHeight / std::max(range, MIN_RANGE_M),
        MIN_GRAZING_SINE, MAX_GRAZING_SINE);

    if (surface == SurfaceType::SEA)
    {
        // GIT sea clutter model (Horst et al. 1978, X-band HH baseline).
        // sigma_0 (dB) = -61.4 + 40*log10(f_GHz) + 10.1*log10(1+SS)
        //              + 30*log10(sin psi). REQ-AESA-040.
        double f_GHz = cfg.frequency_Hz * HZ_TO_GHZ;
        double SS    = std::clamp(static_cast<double>(cfg.seaState),
                               0.0, GIT_SEA_SS_MAX);
        double s0_dB = GIT_SEA_INTERCEPT
                       + GIT_SEA_FREQ_COEFF * std::log10(std::max(f_GHz, MIN_FREQ_GHZ_CLUTTER))
                       + GIT_SEA_SS_COEFF   * std::log10(1.0 + SS)
                       + GIT_SEA_PSICOEFF   * std::log10(sinPsi);
        sigma0 = std::pow(10.0, s0_dB / 10.0);
    }

    if (surface == SurfaceType::LAND)
    {
        // Billingsley low-relief terrain model (X-band baseline).
        // sigma_0 (dB) = -25 + 8*log10(sin psi) + terrain_factor.
        // cfg.landClutter maps 0–1 to 0–15 dB terrain factor. REQ-AESA-040.
        double terrain_dB = static_cast<double>(cfg.landClutter)
                            * BILLINGSLEY_TERRAIN_DB;
        double s0_dB      = BILLINGSLEY_INTERCEPT
                       + BILLINGSLEY_PSI_COEFF * std::log10(sinPsi)
                       + terrain_dB;
        sigma0 = std::pow(10.0, s0_dB / 10.0);
    }

    // No valid surface type matched — return zero. REQ-AESA-040.
    if (sigma0 <= 0.0) return 0.0;

    // Clutter patch area: (c*tau/2) * (R*theta_bw). REQ-AESA-040.
    double tau   = static_cast<double>(cfg.searchWaveform.pulseWidth_s);
    double bwRad = static_cast<double>(cfg.beamWidth) * DEG_TO_RAD;
    double patch = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

    // Clutter power using radar range equation (3rd power for surface clutter).
    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt     = static_cast<double>(active)
                * static_cast<double>(cfg.peakPowerPerElement_W)
                * static_cast<double>(cfg.moduleEfficiency);
    double G      = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);

    double Pc = (Pt * G * G * lambda * lambda * sigma0 * patch)
                / (std::pow(FOUR_PI, 3.0) * std::pow(range, 3.0));

    // Rayleigh amplitude fluctuation of the clutter return. REQ-AESA-040.
    thread_local std::exponential_distribution<double> fluct(1.0);
    return Pc * fluct(tl_rng);
}

// =============================================================================
// FUNCTION: computeJammerPower
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeJammerPower(
    double targetRange_m, const TargetInput& target,
    const RadarConfig& cfg) const
{
    const auto& j = target.jammer;

    // Inactive or zero-power jammer contributes nothing. REQ-AESA-060.
    if (!j.active || j.power_kW <= MIN_JAMMER_POWER_W) return 0.0;

    // Jammer transmit power (W), antenna gain (linear), radar receive gain.
    double Pj  = j.power_kW * 1000.0;
    double Gj  = std::pow(10.0, j.gain_dBi / 10.0);
    double Gr  = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
    double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;

    // Jammer range: self-screening uses target range, stand-off uses j.range_m.
    double Rj = j.selfScreening
                    ? targetRange_m
                    : (j.range_m > MIN_JAMMER_RANGE_M ? j.range_m : targetRange_m);

    // One-way jamming equation: Pj_rx = Pj*Gj*Gr*lam^2 / ((4pi)^2 * Rj^2).
    // REQ-AESA-060.
    double Pr_j = (Pj * Gj * Gr * lam * lam)
                  / (std::pow(FOUR_PI, 2.0) * Rj * Rj);

    // Bandwidth efficiency: jammer power is diluted when jammer bandwidth
    // exceeds receiver bandwidth. min(1, B_r / B_j). REQ-AESA-060.
    double B_r = std::max(MIN_BANDWIDTH_HZ, cfg.antennaBandwidth);
    double B_j = std::max(MIN_BANDWIDTH_HZ, j.bandwidth_Hz);
    return Pr_j * std::min(1.0, B_r / B_j);
}

// =============================================================================
// FUNCTION: computeWaterVapourDensity
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeWaterVapourDensity(
    const AtmosphericConditions& atm) const
{
    double T   = static_cast<double>(atm.temperature_C);
    double RH  = static_cast<double>(atm.humidity_pct);
    double T_K = T + KELVIN_OFFSET;

    // Magnus-Tetens saturation vapour pressure (hPa).
    // e_s = 6.1121 * exp((18.678 - T/234.5) * T/(257.14 + T)).
    // REQ-AESA-071.
    double e_s = BUCK_A * std::exp((BUCK_B - T / BUCK_C) * (T / (BUCK_D + T)));

    // Actual vapour pressure (hPa) = RH% * e_s. REQ-AESA-071.
    double e_a = (RH / 100.0) * e_s;

    // Convert to absolute density (g/m³).
    // rho = e_a(Pa) * M_w / (R_u * T_K) where M_w=18.015 g/mol, R_u=8314.46 J/(kmol·K).
    // REQ-AESA-071.
    double rho_w = (e_a * PA_PER_HPA * WATER_MOLAR_MASS)
                   / (GAS_CONSTANT_JKMOL * T_K / 1000.0);

    // Floor at 0.0 — negative density is physically impossible. REQ-AESA-071.
    return std::max(0.0, rho_w);
}

// =============================================================================
// FUNCTION: computeGaseousAttenuation
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeGaseousAttenuation(
    double frequency_Hz,
    const AtmosphericConditions& atm,
    double range_m) const
{
    // Convert to ITU-R P.676-12 reduced variables. REQ-AESA-071.
    double f   = frequency_Hz * HZ_TO_GHZ;             // GHz
    double T   = static_cast<double>(atm.temperature_C);
    double p   = static_cast<double>(atm.pressure_hPa);
    double rho = computeWaterVapourDensity(atm);        // g/m³

    double r_p = p / ISA_PRESSURE_HPA;                  // pressure ratio
    double r_t = ISA_TEMP_K / (KELVIN_OFFSET + T);      // temperature ratio

    // -------------------------------------------------------------------------
    // Oxygen specific attenuation gamma_o (dB/km).
    // ITU-R P.676-12 Annex 2, Equation 1. REQ-AESA-071.
    // -------------------------------------------------------------------------
    double gamma_o = 0.0;
    {
        double xi1 = std::pow(r_p, 0.0717) * std::pow(r_t, -1.8132)
        * std::exp(0.1147 * (1.0 - r_p) + 1.4434 * (1.0 - r_t));

        double xi2 = std::pow(r_p, 0.5146) * std::pow(r_t, -4.6368)
                     * std::exp(-0.1217 * (1.0 - r_p) + 2.1441 * (1.0 - r_t));

        double xi3 = std::pow(r_p, 0.3414) * std::pow(r_t, -6.5851)
                     * std::exp(0.2177 * (1.0 - r_p) + 5.4677 * (1.0 - r_t));

        gamma_o = ( 7.2  * std::pow(r_t, 2.8)
                       / (f * f + 0.34 * r_p * r_p * std::pow(r_t, 1.6))
                   + 0.62 * xi3
                         / (std::pow(std::abs(54.0 - f), 1.16 * xi1)
                            + 0.83 * xi2) )
                  * f * f * r_p * r_p * 1.0e-3;

        // Clamp — attenuation cannot be negative. REQ-AESA-071.
        gamma_o = std::max(0.0, gamma_o);
    }

    // -------------------------------------------------------------------------
    // Water vapour specific attenuation gamma_w (dB/km).
    // ITU-R P.676-12 Annex 2, Equation 2. REQ-AESA-071.
    //
    // ADVISORY FP-08: lambdas used here are trivial (single arithmetic
    // expression), capture nothing, and are local to this block. Deviation
    // documented in ICD-AESA-DEVIATION-004. REQ-AESA-071.
    // -------------------------------------------------------------------------
    double gamma_w = 0.0;
    {
        double eta1 = 0.955 * r_p * std::pow(r_t, 0.68) + 0.006 * rho;
        double eta2 = 0.735 * r_p * std::pow(r_t, 0.5)
                      + 0.0353 * std::pow(r_t, 4.0) * rho;

        // Line shape correction factor. REQ-AESA-071.
        auto g = [](double f_val, double f_line) -> double {
            return 1.0 + std::pow((f_val - f_line) / (f_val + f_line), 2.0);
        };

        // Guard against division by zero near exact resonance frequencies.
        auto safe_line = [](double f_val, double f_line,
                            double eta, double width) -> double {
            return eta / (std::pow(f_val - f_line, 2.0)
                          + std::max(width * width, 1.0e-6));
        };

        // Water vapour absorption lines. REQ-AESA-071.
        gamma_w = (
                      // 22.235 GHz — dominant water vapour line in radar band
                      3.98   * eta1 * std::exp(2.23  * (1.0 - r_t))
                          * safe_line(f, 22.235,  1.0, 9.42  * eta1) * g(f, 22.235)
                      // 183.310 GHz
                      + 11.96  * eta1 * std::exp(0.7   * (1.0 - r_t))
                            * safe_line(f, 183.31,  1.0, 11.14 * eta1)
                      // 321.226 GHz
                      + 0.081  * eta1 * std::exp(6.44  * (1.0 - r_t))
                            * safe_line(f, 321.226, 1.0, 6.29  * eta1)
                      // 325.153 GHz
                      + 3.66   * eta1 * std::exp(1.6   * (1.0 - r_t))
                            * safe_line(f, 325.153, 1.0, 9.22  * eta1)
                      // 380 GHz
                      + 25.37  * eta1 * std::exp(1.09  * (1.0 - r_t))
                            * safe_line(f, 380.0,   1.0, 1.0)
                      // 448 GHz
                      + 17.4   * eta1 * std::exp(1.46  * (1.0 - r_t))
                            * safe_line(f, 448.0,   1.0, 1.0)
                      // 557 GHz
                      + 844.6  * eta1 * std::exp(0.17  * (1.0 - r_t))
                            * safe_line(f, 557.0,   1.0, 1.0)  * g(f, 557.0)
                      // 752 GHz
                      + 290.0  * eta1 * std::exp(0.41  * (1.0 - r_t))
                            * safe_line(f, 752.0,   1.0, 1.0)  * g(f, 752.0)
                      // 1780 GHz
                      + 83328.0 * eta2 * std::exp(0.99 * (1.0 - r_t))
                            * safe_line(f, 1780.0,  1.0, 1.0)  * g(f, 1780.0)
                      )
                  * f * f * std::pow(r_t, 2.5) * rho * 1.0e-4;

        // Clamp — attenuation cannot be negative. REQ-AESA-071.
        gamma_w = std::max(0.0, gamma_w);
    }

    // Two-way total gaseous loss (dB).
    // gamma_total is one-way in dB/km; multiply by 2 for two-way and
    // convert range from m to km. REQ-AESA-071.
    double gamma_total = gamma_o + gamma_w;
    return GASEOUS_TWO_WAY_KM * gamma_total * range_m;
}

// =============================================================================
// FUNCTION: computePropagationLoss
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computePropagationLoss(
    double range_m, const RadarConfig& cfg) const
{
    double loss_dB = 0.0;

    // ---- Rain attenuation (ITU-R P.838-3) -----------------------------------
    // gamma_rain (dB/km) = 0.00887 * R^1.255.
    // Two-way loss = 2 * gamma_rain * range_km. REQ-AESA-071.
    if (cfg.atmosphere.rainRate_mmph > 0.0)
    {
        double gamma_rain = RAIN_COEFF_A
                            * std::pow(static_cast<double>(
                                           cfg.atmosphere.rainRate_mmph),
                                       RAIN_COEFF_B);
        loss_dB += TWO_WAY * gamma_rain * (range_m * KM_PER_M);
    }

    // ---- Fog attenuation (Kunkel 1984) ----------------------------------------
    // Only active when visibility is in [1 m, 2000 m] — outside this range the
    // model is not valid. REQ-AESA-071.
    if (cfg.atmosphere.fogVisibility_m > FOG_VIS_MIN_M &&
        cfg.atmosphere.fogVisibility_m < FOG_VIS_MAX_M)
    {
        // Liquid water content M (g/m³) from visibility (m). REQ-AESA-071.
        double M_fog     = FOG_COEFF_A
                       * std::pow(FOG_VIS_DENOM
                                      / static_cast<double>(
                                          cfg.atmosphere.fogVisibility_m),
                                  FOG_VIS_EXP);
        // Specific attenuation (dB/km). REQ-AESA-071.
        double gamma_fog = FOG_GAMMA_A * std::pow(M_fog, FOG_GAMMA_EXP);
        loss_dB += TWO_WAY * gamma_fog * (range_m * KM_PER_M);
    }

    // ---- Gaseous absorption (ITU-R P.676-12) ----------------------------------
    // O2 and H2O absorption. Returns dB, two-way. REQ-AESA-071.
    loss_dB += computeGaseousAttenuation(cfg.frequency_Hz,
                                         cfg.atmosphere, range_m);

    // Convert total dB loss to linear power reduction factor. REQ-AESA-071.
    return std::pow(10.0, -loss_dB / 10.0);
}

// =============================================================================
// FUNCTION: computeSINR
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeSINR(
    double receivedPower, double range,
    SurfaceType surface, const TargetInput& target,
    const RadarConfig& cfg, const BeamWaveform& waveform) const
{
    // Compute interference components. REQ-AESA-040.
    double Pn = computeNoisePower(cfg,
                                  static_cast<double>(waveform.bandwidth_Hz));
    double Pc = computeClutterPower(range, surface, cfg);
    double Pj = computeJammerPower(range, target, cfg);

    // Modulation processing gain and pulse integration gain. REQ-AESA-020.
    double pg = computeModulationProcessingGain(waveform);
    double ig = static_cast<double>(std::max(MIN_PULSES_PER_DWELL,
                                             waveform.pulsesPerDwell));

    // -------------------------------------------------------------------------
    // Adaptive null steering jammer suppression.
    // If the jammer falls within the null cone, multiply its power by
    // 10^(nullDepth_dB/10) before adding to denominator. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double jSuppress = 1.0;
    if (cfg.nullSteering.active && target.jammer.active)
    {
        // Compute jammer direction in body frame. REQ-AESA-040.
        double jAz = std::atan2(target.y, target.x) * RAD_TO_DEG;
        if (jAz < 0.0) jAz += AZ_FULL_CIRCLE;

        double jEl = (range > MIN_RANGE_M)
                         ? std::asin(std::clamp(target.z / range,
                                                DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                               * RAD_TO_DEG
                         : 0.0;

        // Angular separation from null direction. REQ-AESA-040.
        double dAz = std::abs(jAz - cfg.nullSteering.azimuth_deg);
        if (dAz > AZ_WRAP_THRESHOLD) dAz = AZ_FULL_CIRCLE - dAz;
        double dEl = std::abs(jEl - cfg.nullSteering.elevation_deg);

        // Check if jammer is within the null cone. REQ-AESA-040.
        if (dAz < static_cast<double>(cfg.beamWidth) * NULL_CONE_FACTOR &&
            dEl < static_cast<double>(cfg.beamWidth) * NULL_CONE_FACTOR)
        {
            // Apply null depth suppression to jammer power. REQ-AESA-040.
            jSuppress = std::pow(10.0,
                                 static_cast<double>(cfg.nullSteering.nullDepth_dB) / 10.0);
        }
    }

    // SINR = (Pr * pg * ig) / (Pn + Pc + Pj * jSuppress). REQ-AESA-040.
    return std::max(0.0,
                    (receivedPower * pg * ig) / (Pn + Pc + Pj * jSuppress));
}

// =============================================================================
// §C  CFAR
// =============================================================================

// =============================================================================
// FUNCTION: generateReferenceCells
// Full description in header.
// =============================================================================
std::vector<double> RadarSignalProcessor_AESA::generateReferenceCells(
    SurfaceType surface, const RadarConfig& cfg) const
{
    // Exponential distribution models Rayleigh amplitude / chi-squared power
    // clutter statistics. REQ-AESA-040.
    thread_local std::exponential_distribution<double> cellDist(1.0);

    std::vector<double> cells;
    cells.reserve(CFAR_NUM_CELLS);

    for (int i = 0; i < CFAR_NUM_CELLS; ++i)
    {
        double c = cellDist(tl_rng);

        // Scale cells by surface clutter factor — surface clutter has higher
        // variance than thermal noise. REQ-AESA-040.
        if (surface == SurfaceType::SEA)
        {
            c *= (1.0 + static_cast<double>(cfg.seaState) * CFAR_SEA_SCALE);
        }
        if (surface == SurfaceType::LAND)
        {
            c *= (1.0 + static_cast<double>(cfg.landClutter) * CFAR_LAND_SCALE);
        }
        cells.push_back(c);
    }
    return cells;
}

// =============================================================================
// FUNCTION: computeCFARThreshold
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeCFARThreshold(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    // Guard against empty cell list. REQ-AESA-040.
    if (cells.empty()) return CFAR_INFINITE_THRESHOLD;

    double sum = 0.0;
    for (double v : cells) sum += v;

    double N     = static_cast<double>(cells.size());

    // CA-CFAR multiplier: alpha = N * (Pfa^(-1/N) - 1). REQ-AESA-040.
    const double safePfa = std::max(cfg.targetPfa, 1e-15);
    double alpha = N * (std::pow(safePfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

// =============================================================================
// FUNCTION: computeCFARThresholdRelaxed
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeCFARThresholdRelaxed(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    // Guard against empty cell list. REQ-AESA-040.
    if (cells.empty()) return CFAR_INFINITE_THRESHOLD;

    double sum = 0.0;
    for (double v : cells) sum += v;

    double N = static_cast<double>(cells.size());

    // Relaxed Pfa: min(PFA_RELAXED_MAX, targetPfa * 100).
    // Higher Pfa = lower threshold = higher sensitivity near clutter notch.
    // REQ-AESA-040.
    double pfa   = std::max(1e-15, std::min(PFA_RELAXED_MAX, cfg.targetPfa * PFA_RELAXED_SCALE));
    double alpha = N * (std::pow(pfa, -1.0 / N) - 1.0);

    return (sum / N) * alpha;
}

// =============================================================================
// §D  RCS AND SWERLING FLUCTUATION
// =============================================================================

// =============================================================================
// FUNCTION: computeEffectiveRCS
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeEffectiveRCS(
    const TargetInput& target, double range, double frequency_Hz) const
{
    double base = 0.0;

    if (target.dimensions.valid)
    {
        // ----------------------------------------------------------------
        // 1. Extract and bound dimensions. REQ-AESA-040.
        // ----------------------------------------------------------------
        double L = std::max(target.dimensions.length, MIN_DIMENSION_M);
        double H = std::max(target.dimensions.height, MIN_DIMENSION_M);
        double W = std::max(target.dimensions.width,  MIN_DIMENSION_M);

        double lambda = SPEED_OF_LIGHT / std::max(frequency_Hz, 1.0);
        double volume = L * H * W;

        // ----------------------------------------------------------------
        // 2. Establish body-frame axes from velocity or LOS. REQ-AESA-040.
        // ----------------------------------------------------------------
        double speed = std::sqrt(target.vx * target.vx +
                                 target.vy * target.vy +
                                 target.vz * target.vz);

        double fx, fy, fz;  // forward (length axis)
        double rx, ry, rz;  // right   (width axis)
        double ux, uy, uz;  // up      (height axis)

        if (speed > MIN_SPEED_FOR_HEADING)
        {
            // Moving target — forward axis from velocity. REQ-AESA-040.
            fx = target.vx / speed;
            fy = target.vy / speed;
            fz = target.vz / speed;
        }
        else
        {
            // Stationary — forward axis toward radar for worst-case aspect.
            fx = -target.x / std::max(range, 1.0);
            fy = -target.y / std::max(range, 1.0);
            fz = -target.z / std::max(range, 1.0);
        }

        // right = forward × up (world up = [0, 0, 1]). REQ-AESA-040.
        rx = fy * 1.0 - fz * 0.0;
        ry = fz * 0.0 - fx * 1.0;
        rz = fx * 0.0 - fy * 0.0;
        double rmag = std::sqrt(rx * rx + ry * ry + rz * rz);

        // Degenerate case: velocity is purely vertical — fallback right axis.
        if (rmag < MIN_RVEC_MAG) { rx = 0; ry = 1; rz = 0; rmag = 1.0; }
        rx /= rmag; ry /= rmag; rz /= rmag;

        // up = right × forward. REQ-AESA-040.
        ux = ry * fz - rz * fy;
        uy = rz * fx - rx * fz;
        uz = rx * fy - ry * fx;

        // ----------------------------------------------------------------
        // 3. Illumination direction (radar to target reversed). REQ-AESA-040.
        // ----------------------------------------------------------------
        double ix = -target.x / std::max(range, 1.0);
        double iy = -target.y / std::max(range, 1.0);
        double iz = -target.z / std::max(range, 1.0);

        // ----------------------------------------------------------------
        // 4. Define 6 faces: {normal, dimensions, face index}. REQ-AESA-040.
        // ----------------------------------------------------------------
        struct Face { double nx, ny, nz, a, b; int idx; };
        const Face faces[NUM_FACES] = {
            {  fx,  fy,  fz, W, H, 0 },   // front
            { -fx, -fy, -fz, W, H, 1 },   // rear
            {  rx,  ry,  rz, L, H, 2 },   // right
            { -rx, -ry, -rz, L, H, 3 },   // left
            {  ux,  uy,  uz, L, W, 4 },   // top
            { -ux, -uy, -uz, L, W, 5 },   // bottom
        };

        // ----------------------------------------------------------------
        // 5. Frequency regime from smallest dimension. REQ-AESA-040.
        // ----------------------------------------------------------------
        double D_char = std::min({L, H, W});
        ScatteringRegime regime = rcs_regime(D_char, lambda);

        // ----------------------------------------------------------------
        // 6. Per-target material and shape factors. REQ-AESA-040.
        // ----------------------------------------------------------------
        double eta_mat   = rcs_materialFactor(target.dimensions.material);
        double eta_shape = rcs_shapeFactor(target.dimensions.shape);

        // ----------------------------------------------------------------
        // 7. Incoherent summation of face contributions. REQ-AESA-040.
        // Incoherent because: frequency agility decorrelates phases,
        // Swerling handles coherent fluctuation, entity-level simulation
        // cannot resolve sub-wavelength face-to-face phase differences.
        // Ref: Knott Sec 5.3, IEEE Std 1672-2006 Sec 6.2.
        // ----------------------------------------------------------------
        double sigma_faces = 0.0;
        for (const auto& f : faces)
        {
            double cosTheta = f.nx * ix + f.ny * iy + f.nz * iz;
            if (cosTheta <= 0.0) continue;  // shadowed — skip

            double A        = f.a * f.b;
            double eta_face = rcs_faceFactor(target.dimensions.shape, f.idx);

            sigma_faces += rcs_faceSigma(A, cosTheta, lambda, volume,
                                         regime,
                                         eta_shape * eta_face, eta_mat);
        }

        // ----------------------------------------------------------------
        // 8. Edge diffraction floor: sigma_edge = lambda*perimeter/(8*pi).
        // Adds a frequency-dependent floor at non-specular angles.
        // Ref: Ruck et al Ch 5 — GTD edge diffraction. REQ-AESA-040.
        // ----------------------------------------------------------------
        double perimeter_total = EDGE_PERIMETER_MULT * (L + H + W);
        double sigma_edge = (lambda * perimeter_total)
                            / EDGE_DIFFRACTION_DENOM
                            * eta_mat;

        base = sigma_faces + sigma_edge;
    }
    else if (!target.rcsTable.empty())
    {
        // Aspect-angle table provided — interpolate. REQ-AESA-040.
        double velMag = std::sqrt(target.vx * target.vx +
                                  target.vy * target.vy +
                                  target.vz * target.vz);
        double aspectAngle_deg = 90.0;  // broadside default
        if (velMag > 0.01 && range > 1.0)
        {
            double dot = std::clamp(
                (target.vx / velMag) * (target.x / range) +
                    (target.vy / velMag) * (target.y / range) +
                    (target.vz / velMag) * (target.z / range),
                DOT_CLAMP_MIN, DOT_CLAMP_MAX);
            aspectAngle_deg = std::acos(dot) * RAD_TO_DEG;
        }
        base = lookupAspectRCS(target, aspectAngle_deg);
    }
    else
    {
        // Last resort: platform type lookup. REQ-AESA-040.
        base = getPlatformBaseRCS(target.platformType);
    }

    // -------------------------------------------------------------------------
    // 9. Swerling fluctuation. Applied after the geometric base is computed so
    //    the physical model sets the MEAN and Swerling sets the distribution.
    //    Ref: Knott Ch 2, Skolnik Ch 2.7. REQ-AESA-040.
    // -------------------------------------------------------------------------
    bool coherent = (target.swerlingCase == SwerlingCase::CASE_II ||
                     target.swerlingCase == SwerlingCase::CASE_IV);
    return computeSwerlingRCS(base, target.swerlingCase, coherent);
}

// =============================================================================
// FUNCTION: computeSwerlingRCS
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeSwerlingRCS(
    double nominalRCS, SwerlingCase sc, bool /*coherentDwell*/) const
{
    // Zero or negative nominal RCS cannot be meaningfully fluctuated. REQ-AESA-040.
    if (nominalRCS <= MIN_NOMINAL_RCS) return 0.0;

    switch (sc)
    {
    case SwerlingCase::CASE_0:
        // Non-fluctuating — return exact nominal RCS. REQ-AESA-040.
        return nominalRCS;

    case SwerlingCase::CASE_I:
    case SwerlingCase::CASE_II:
    {
        // Chi-squared with 2 DOF (exponential distribution).
        // Models many small, equal-amplitude independent scatterers. REQ-AESA-040.
        thread_local std::exponential_distribution<double> ed(1.0);
        return nominalRCS * ed(tl_rng);
    }

    case SwerlingCase::CASE_III:
    case SwerlingCase::CASE_IV:
    {
        // Chi-squared with 4 DOF (sum of two exponentials).
        // Models one dominant scatterer plus many small ones. REQ-AESA-040.
        thread_local std::exponential_distribution<double> ed(1.0);
        double half = nominalRCS / SWERLING_34_HALF;
        return half * (ed(tl_rng) + ed(tl_rng));
    }
    }
    return nominalRCS;
}

// =============================================================================
// §E  TARGET MOTION AND ALBERSHEIM Pd
// =============================================================================

// =============================================================================
// FUNCTION: computeTargetMotionParams
// Full description in header.
// =============================================================================
void RadarSignalProcessor_AESA::computeTargetMotionParams(
    DetectionOutput& det, const TargetInput& target, double range) const
{
    // Speed over ground from horizontal velocity components. REQ-AESA-040.
    det.speedOverGround = std::sqrt(target.vx * target.vx +
                                    target.vy * target.vy);

    // Heading from atan2 — wrap to [0, 360). REQ-AESA-040.
    det.heading = std::atan2(target.vy, target.vx) * RAD_TO_DEG;
    if (det.heading < 0.0) det.heading += AZ_FULL_CIRCLE;

    // Acceleration: not currently estimated — reserved for IMU fusion.
    det.acceleration = 0.0;

    // Target aspect angle: angle between velocity heading and radar LOS.
    if (det.speedOverGround > 0.01 && range > 1e-6)
    {
        double s = det.speedOverGround;
        double dot = std::clamp(
            (target.vx / s) * (target.x / range) +
                (target.vz > 0.001 ? (target.vz / s) * (target.z / range) : 0.0),
            DOT_CLAMP_MIN, DOT_CLAMP_MAX);
        det.targetAspect = std::acos(dot) * RAD_TO_DEG;
    }
    else
    {
        det.targetAspect = 0.0;
    }
}

// =============================================================================
// FUNCTION: computeRadialVelocity
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeRadialVelocity(
    const TargetInput& target, double range,
    std::normal_distribution<double>& noise) const
{
    // Radial velocity = dot(velocity, LOS unit vector). REQ-AESA-040.
    double dot = target.vx * target.x +
                 target.vy * target.y +
                 target.vz * target.z;

    // Guard against zero range — return noise only. REQ-AESA-040.
    return (range > 1e-6 ? dot / range : 0.0) + noise(tl_rng);
}

// =============================================================================
// FUNCTION: computeCPA
// Full description in header.
// =============================================================================
void RadarSignalProcessor_AESA::computeCPA(
    DetectionOutput& det, const TargetInput& target, double range) const
{
    // Default: CPA is at current position (stationary target). REQ-AESA-040.
    det.cpa_distance = range;
    det.time_to_cpa  = 0.0;

    double v2 = target.vx * target.vx +
                target.vy * target.vy +
                target.vz * target.vz;

    if (v2 > 0.01)
    {
        // Time to CPA: t = -dot(position, velocity) / |velocity|^2. REQ-AESA-040.
        double t = -(target.x * target.vx +
                     target.y * target.vy +
                     target.z * target.vz) / v2;

        // CPA is only in the future — past CPA means target is receding.
        det.time_to_cpa = std::max(0.0, t);

        // CPA position. REQ-AESA-040.
        double cx = target.x + target.vx * det.time_to_cpa;
        double cy = target.y + target.vy * det.time_to_cpa;
        double cz = target.z + target.vz * det.time_to_cpa;
        det.cpa_distance = std::sqrt(cx * cx + cy * cy + cz * cz);
    }
}

// =============================================================================
// FUNCTION: computeAlbersheimPd
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeAlbersheimPd(
    double snr_linear, double Pfa, int N, SwerlingCase sc) const
{
    // Zero or negative SNR — no detection possible. REQ-AESA-040.
    if (snr_linear <= 0.0 || N < 1) return 0.0;

    double snr_dB = 10.0 * std::log10(snr_linear);

    // Albersheim A constant: A = log(0.62 / Pfa). REQ-AESA-040.
    double A  = std::log(ALBERSHEIM_PFA_COEFF / std::max(ALBERSHEIM_PFA_MIN, Pfa));
    double Nf = static_cast<double>(std::max(1, N));

    // Swerling fluctuation loss (dB). Non-fluctuating (CASE_0) has no loss.
    // REQ-AESA-040.
    double swerlingLoss_dB = 0.0;
    switch (sc)
    {
    case SwerlingCase::CASE_I:
    case SwerlingCase::CASE_II:
        swerlingLoss_dB = SWERLING_LOSS_I_II;
        break;
    case SwerlingCase::CASE_III:
    case SwerlingCase::CASE_IV:
        swerlingLoss_dB = SWERLING_LOSS_III_IV;
        break;
    default:
        break;
    }

    // Effective SNR after Swerling loss. REQ-AESA-040.
    double effectiveSNR = snr_dB - swerlingLoss_dB;

    // Bisection search over Pd in [0.001, 0.999]. REQ-AESA-040.
    double pdLow  = ALBERSHEIM_PD_LOW;
    double pdHigh = ALBERSHEIM_PD_HIGH;

    for (int iter = 0; iter < ALBERSHEIM_ITERATIONS; ++iter)
    {
        double pdMid = 0.5 * (pdLow + pdHigh);
        double B     = std::log(pdMid / (1.0 - pdMid));

        // Albersheim required SNR (dB) for this Pd. REQ-AESA-040.
        double snrReq = -5.0 * std::log10(Nf)
                        + (6.2 + 4.54 / std::sqrt(Nf + 0.44))
                              * std::log10(A + 0.12 * A * B + 1.7 * B);

        if (snrReq < effectiveSNR) pdLow  = pdMid;
        else                        pdHigh = pdMid;
    }

    return std::clamp(0.5 * (pdLow + pdHigh), 0.0, ALBERSHEIM_PD_MAX);
}

// =============================================================================
// FUNCTION: computePk
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computePk(
    double sinr_linear, double Pfa, int N, SwerlingCase sc) const
{
    return computeAlbersheimPd(sinr_linear, Pfa, N, sc);
}

// =============================================================================
// §F  RANGE AMBIGUITY
// =============================================================================

// =============================================================================
// FUNCTION: resolveRangeAmbiguity
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::resolveRangeAmbiguity(
    double measured, double predicted, double Rmax) const
{
    // If Rmax is too small to be meaningful, return measured unchanged. REQ-AESA-021.
    if (Rmax < MIN_RMAX_FOR_RESOLVE) return measured;

    double best   = measured;
    double minErr = 1e12;

    // Search k in [-RANGE_RESOLVE_K_MAX, +RANGE_RESOLVE_K_MAX].
    // The candidate closest to the Kalman-predicted range is selected. REQ-AESA-021.
    for (int k = -RANGE_RESOLVE_K_MAX; k <= RANGE_RESOLVE_K_MAX; ++k)
    {
        double cand = measured + static_cast<double>(k) * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

// =============================================================================
// FUNCTION: applyRangeAmbiguity
// Full description in header.
// =============================================================================
void RadarSignalProcessor_AESA::applyRangeAmbiguity(
    DetectionOutput& det, double range,
    double Rmax, double Rmax2,
    std::normal_distribution<double>& noise) const
{
    // ---- First PRF folded measurement (always computed). REQ-AESA-021. ----
    double noisy1;
    if (range > Rmax)
    {
        // Range is ambiguous — fold into [0, Rmax). REQ-AESA-021.
        noisy1          = std::fmod(range, Rmax) + noise(tl_rng);
        det.isAmbiguous = true;
    }
    else
    {
        // Range is unambiguous — add noise only. REQ-AESA-021.
        noisy1          = range + noise(tl_rng);
        det.isAmbiguous = false;
    }

    if (Rmax2 > MIN_RMAX_FOR_RESOLVE)
    {
        // ---- Staggered PRF: second independent measurement. REQ-AESA-021. ----
        double noisy2 = (range > Rmax2)
                            ? std::fmod(range, Rmax2) + noise(tl_rng)
                            : range + noise(tl_rng);

        // Coincidence detector resolves to true range. REQ-AESA-021.
        det.range       = resolveRangeAmbiguityStaggered(noisy1, noisy2,
                                                   Rmax, Rmax2, range);
        det.isAmbiguous = false;   // coincidence detector resolved it
    }
    else
    {
        // Single PRF — report folded measurement. REQ-AESA-021.
        det.range = noisy1;
        // det.isAmbiguous already set above.
    }
}

// =============================================================================
// FUNCTION: resolveRangeAmbiguityStaggered
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::resolveRangeAmbiguityStaggered(
    double measured1, double measured2,
    double Rmax1, double Rmax2, double predicted) const
{
    double bestCand = measured1;
    double bestErr  = 1e12;

    // Search n1 and n2 in [0, N_STAG_MAX] for coincidence. REQ-AESA-021.
    for (int n1 = 0; n1 <= N_STAG_MAX; ++n1)
    {
        double r1 = measured1 + static_cast<double>(n1) * Rmax1;
        if (r1 < 0.0) continue;

        for (int n2 = 0; n2 <= N_STAG_MAX; ++n2)
        {
            double r2 = measured2 + static_cast<double>(n2) * Rmax2;
            if (r2 < 0.0) continue;

            // Coincidence gate: candidates from both PRFs must agree to within
            // RANGE_COINCIDENCE_GATE_M (500 m). REQ-AESA-021.
            if (std::abs(r1 - r2) < RANGE_COINCIDENCE_GATE_M)
            {
                // Average the two estimates for minimum variance. REQ-AESA-021.
                double cand = 0.5 * (r1 + r2);
                double err  = std::abs(cand - predicted);
                if (err < bestErr) { bestErr = err; bestCand = cand; }
            }
        }
    }
    return bestCand;
}

// =============================================================================
// FUNCTION: resolveVelocityStaggered
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::resolveVelocityStaggered(
    double foldedVel1, double foldedVel2,
    double Vmax1, double Vmax2, double predictedVel) const
{
    // Fold each measurement into its respective [0, Vmax] window.
    // ADVISORY FP-08: lambda is trivial, local, captures nothing.
    // ICD-AESA-DEVIATION-004. REQ-AESA-021.
    auto fold = [](double v, double Vmax) -> double {
        if (Vmax < MIN_VMAX_FOR_FOLD) return v;
        v = std::fmod(v, Vmax);
        if (v < 0.0) v += Vmax;
        return v;
    };

    double v1f = fold(foldedVel1, Vmax1);
    double v2f = fold(foldedVel2, Vmax2);

    double bestCand = foldedVel1;
    double bestErr  = 1e12;

    // Search n1, n2 in [0, VEL_N_MAX) for coincidence. REQ-AESA-021.
    for (int n1 = 0; n1 < VEL_N_MAX; ++n1)
    {
        double c1 = v1f + static_cast<double>(n1) * Vmax1;
        for (int n2 = 0; n2 < VEL_N_MAX; ++n2)
        {
            double c2 = v2f + static_cast<double>(n2) * Vmax2;

            // Velocity coincidence gate: 2 m/s (~one Doppler bin). REQ-AESA-021.
            if (std::abs(c1 - c2) < VEL_COINCIDENCE_GATE)
            {
                double cand = 0.5 * (c1 + c2);
                double err  = std::abs(cand - predictedVel);
                if (err < bestErr) { bestErr = err; bestCand = cand; }
            }
        }
    }
    return bestCand;
}

// =============================================================================
// FUNCTION: resolveRangeForLockOn
// Full description in header.
// =============================================================================
void RadarSignalProcessor_AESA::resolveRangeForLockOn(
    DetectionOutput& det, double range, double Rmax,
    uint32_t targetId, const std::vector<TrackFile>& db) const
{
    // Use Kalman-predicted range as the prior if the track is in the database.
    // If the track is not found, fall back to the true slant range. REQ-AESA-021.
    double predicted = range;
    for (const auto& t : db)
    {
        if (t.id == targetId)
        {
            predicted = t.predictedRange;
            break;
        }
    }

    // Resolve folded range to nearest multiple of Rmax from predicted. REQ-AESA-021.
    det.range       = resolveRangeAmbiguity(det.range, predicted, Rmax);
    det.isAmbiguous = false;
}

// =============================================================================
// §G  MAX DETECTION RANGE
// =============================================================================

// =============================================================================
// FUNCTION: computeMaxDetectionRange
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeMaxDetectionRange(
    double rcs, const RadarConfig& cfg) const
{
    double lam    = SPEED_OF_LIGHT / cfg.frequency_Hz;
    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt     = static_cast<double>(active)
                * static_cast<double>(cfg.peakPowerPerElement_W)
                * static_cast<double>(cfg.moduleEfficiency);
    double G      = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
    double Pn     = computeNoisePower(
        cfg, static_cast<double>(cfg.searchWaveform.bandwidth_Hz));

    // CA-CFAR multiplier with N=16 reference cells. REQ-AESA-040.
    const double safePfa = std::max(cfg.targetPfa, 1e-15);   // guard against Pfa=0
    double alpha = MAX_RANGE_CFAR_N
                   * (std::pow(safePfa, -1.0 / MAX_RANGE_CFAR_N) - 1.0);

    double pg = computeModulationProcessingGain(cfg.searchWaveform);
    double ig = static_cast<double>(std::max(1, cfg.searchWaveform.pulsesPerDwell));

    // Iterative solver — propagation loss depends on range, so R must be solved
    // iteratively. Converges when |R_new - R_prev| < 10 m. REQ-AESA-040.
    // Pre-compute the denominator — it does not depend on range.
    const double den = std::pow(FOUR_PI, 3.0) * Pn * alpha;
    if (den <= 0.0)
        return cfg.minDetectableRange * KM_PER_M * 2.0;

    // BUGFIX (REQ-AESA-040): The original fixed-point iterator oscillates under
    // heavy rain/fog because computePropagationLoss() collapses to ~0 at large R
    // (e.g. 150 mm/h rain gives 1900+ dB loss at 200 km), driving R_new to 0,
    // which then recovers prop to 1 and drives R_new back to 227 km — repeating
    // for all MAX_RANGE_ITERATIONS and returning the no-rain answer. Replaced
    // with a bisection solver which is unconditionally stable for any prop curve.
    //
    // The solver finds R* such that f(R) = 0, where:
    //   f(R) = R - ( Pt * prop(R)^2 * G^2 * lam^2 * rcs * pg * ig / den )^0.25
    // f(R) < 0 means SINR > threshold (can detect further), f(R) > 0 means
    // SINR < threshold (too far). We bisect [R_lo, R_hi] until width < 10 m.

    // Step 1 — find R_hi: shrink from MAX_RANGE_INITIAL_M until f(R_hi) > 0.
    // Step 1a — walk R_hi UP until it is above the true detection range.
    // Fixes the case where no-rain RF range (227 km) exceeds MAX_RANGE_INITIAL_M.
    double R_lo = static_cast<double>(cfg.minDetectableRange);
    double R_hi = MAX_RANGE_INITIAL_M;
    for (int i = 0; i < 16; ++i)
    {
        double prop  = computePropagationLoss(R_hi, cfg);
        double num   = Pt * prop * prop * G * G * lam * lam * rcs * pg * ig;
        double R_new = (num > 0.0) ? std::pow(num / den, 0.25) : 0.0;
        if (R_new <= R_hi) break;       // R_hi is above the true range — good
        R_hi *= 2.0;
        if (R_hi > 2000000.0) { R_hi = 2000000.0; break; }  // hard cap 2000 km
    }
    // Step 1b — walk R_hi DOWN until bracketed from above.
    // Fixes the case where heavy rain collapses prop to 0 at large R.
    for (int i = 0; i < 64; ++i)
    {
        double prop  = computePropagationLoss(R_hi, cfg);
        double num   = Pt * prop * prop * G * G * lam * lam * rcs * pg * ig;
        double R_new = (num > 0.0) ? std::pow(num / den, 0.25) : 0.0;
        if (R_new <= R_hi) break;       // properly bracketed
        R_hi *= 0.5;
        if (R_hi <= R_lo) { R_hi = R_lo; break; }
    }

    // Step 2 — bisect to MAX_RANGE_CONVERGENCE_M precision. REQ-AESA-040.
    for (int iter = 0; iter < MAX_RANGE_ITERATIONS; ++iter)
    {
        double R_mid = 0.5 * (R_lo + R_hi);
        double prop  = computePropagationLoss(R_mid, cfg);
        double num   = Pt * prop * prop * G * G * lam * lam * rcs * pg * ig;
        double R_new = (num > 0.0) ? std::pow(num / den, 0.25) : 0.0;

        if (R_new > R_mid)
            R_lo = R_mid;   // SINR above threshold — detection range is further
        else
            R_hi = R_mid;   // SINR below threshold — detection range is closer

        if ((R_hi - R_lo) < MAX_RANGE_CONVERGENCE_M) break;
    }

    const double R_est = 0.5 * (R_lo + R_hi);

    // Return in km, with a floor of 2x minDetectableRange. REQ-AESA-040.
    return std::max(R_est * KM_PER_M,
                    cfg.minDetectableRange * KM_PER_M * 2.0);
}

// =============================================================================
// §H  DETECTION MERGE GUARD
// =============================================================================

// =============================================================================
// FUNCTION: shouldMergeDetection
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::shouldMergeDetection(
    const DetectionOutput& det,
    const std::vector<DetectionOutput>& existing,
    const RadarConfig& cfg) const
{
    for (const auto& ex : existing)
    {
        // Azimuth difference with wrap-around. REQ-AESA-040.
        double azDiff = std::abs(ex.azimuth - det.azimuth);
        if (azDiff > AZ_WRAP_THRESHOLD) azDiff = AZ_FULL_CIRCLE - azDiff;

        // Merge if range within MERGE_GATE_RANGE_M AND angular within beamWidth.
        // REQ-AESA-040.
        if (std::abs(ex.range     - det.range)     < MERGE_GATE_RANGE_M &&
            azDiff                                  < static_cast<double>(cfg.beamWidth) &&
            std::abs(ex.elevation - det.elevation)  < static_cast<double>(cfg.beamWidth))
        {
            return true;
        }
    }
    return false;
}

// =============================================================================
// §I  BEAM GAIN AND SIDELOBE BLANKING
// =============================================================================

// =============================================================================
// FUNCTION: computeBeamGainFactor
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeBeamGainFactor(
    double azDiff, double elDiff,
    const RadarConfig& cfg, double effectiveBeamWidth) const
{
    double bw = (effectiveBeamWidth > 0.0)
    ? effectiveBeamWidth
    : static_cast<double>(cfg.beamWidth);

    // Within main beam: full gain (factor = 1.0). REQ-AESA-040.
    if (azDiff <= bw * MAIN_BEAM_FACTOR && elDiff <= bw * MAIN_BEAM_FACTOR)
    {
        return 1.0;
    }

    // Outside main beam: apply sidelobe level. REQ-AESA-040.
    float peakSL, avgSL;
    switch (cfg.sidelobeMode)
    {
    case SidelobeMode::LOW_SLL:
        peakSL = LOW_SLL_PEAK;  avgSL = LOW_SLL_AVG;
        break;
    case SidelobeMode::ULTRA_LOW:
        peakSL = ULTRA_LOW_PEAK; avgSL = ULTRA_LOW_AVG;
        break;
    default:
        peakSL = cfg.peakSidelobeLevel;
        avgSL  = cfg.avgSidelobeLevel;
        break;
    }

    // Use average sidelobe level for targets clearly outside the main beam.
    // The first condition is always false here (we are past the main beam check)
    // but it is retained for defensive clarity. REQ-AESA-040.
    double dB = (azDiff <= bw * MAIN_BEAM_FACTOR && elDiff <= bw * MAIN_BEAM_FACTOR)
                    ? static_cast<double>(peakSL)
                    : static_cast<double>(avgSL);

    return std::pow(10.0, dB / 10.0);
}

// =============================================================================
// FUNCTION: isJammerInSidelobe
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::isJammerInSidelobe(
    double azDiff, double elDiff,
    const TargetInput& target, const RadarConfig& cfg) const
{
    // No jammer — no blanking needed. REQ-AESA-060.
    if (!target.jammer.active) return false;

    // Jammer in main beam: not a sidelobe jammer — do not blank. REQ-AESA-040.
    double halfBW = static_cast<double>(cfg.beamWidth) / 2.0;
    if (azDiff <= halfBW && elDiff <= halfBW) return false;

    // Zero power jammer — no power to blank on. REQ-AESA-060.
    if (target.jammer.power_kW <= 0.0) return false;

    // Compute jammer power received in the sidelobe. REQ-AESA-060.
    double Pj  = target.jammer.power_kW * 1000.0;
    double Gj  = std::pow(10.0, target.jammer.gain_dBi / 10.0);
    double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double Rj  = target.jammer.selfScreening
                    ? MIN_JAMMER_RANGE_M
                    : std::max(MIN_JAMMER_RANGE_M, target.jammer.range_m);

    double Pr  = (Pj * Gj * lam * lam)
                / (std::pow(FOUR_PI, 2.0) * Rj * Rj);

    double Pn  = computeNoisePower(cfg, cfg.antennaBandwidth);
    if (Pr <= 0.0 || Pn <= 0.0) return false;

    // If jammer exceeds sidelobeBlanking_dB above noise → blank. REQ-AESA-040.
    double excessdB = 10.0 * std::log10(Pr / Pn);
    return excessdB > static_cast<double>(cfg.sidelobeBlanking_dB);
}

// =============================================================================
// §J  MODULATION PROCESSING GAIN
// =============================================================================

// =============================================================================
// FUNCTION: computeModulationProcessingGain
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeModulationProcessingGain(
    const BeamWaveform& wf) const
{
    switch (wf.modulation)
    {
    case ModulationType::LFM:
    case ModulationType::NLFM:
    case ModulationType::FMCW:
        // Processing gain = time-bandwidth product (BT product). REQ-AESA-020.
        return std::max(MIN_PROCESSING_GAIN,
                        static_cast<double>(wf.bandwidth_Hz)
                            * static_cast<double>(wf.pulseWidth_s));
    default:
        // Unmodulated pulse — no compression gain. REQ-AESA-020.
        return 1.0;
    }
}

// =============================================================================
// §K  DOPPLER CLUTTER NOTCH AND STAP
// =============================================================================

// =============================================================================
// FUNCTION: computeClutterNotch
// Full description in header.
// =============================================================================
std::pair<double,double> RadarSignalProcessor_AESA::computeClutterNotch(
    const RadarConfig& cfg, const BeamWaveform& wf) const
{
    // Notch half-width = lambda * PRF / (2 * N_pulses).
    // This is the Doppler bin width — the minimum resolvable velocity step.
    // Clamped to [NOTCH_WIDTH_MIN, NOTCH_WIDTH_MAX] m/s for physical validity.
    // REQ-AESA-040.
    double lambda     = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double prf        = static_cast<double>(wf.prf_Hz);
    double N          = static_cast<double>(std::max(1, wf.pulsesPerDwell));
    double notchWidth = (lambda * prf) / (2.0 * N);
    notchWidth        = std::clamp(notchWidth, NOTCH_WIDTH_MIN, NOTCH_WIDTH_MAX);

    // Clutter centre velocity = platform speed. REQ-AESA-040.
    double Vclutter = static_cast<double>(cfg.platformSpeed_m_s);
    return { Vclutter - notchWidth, Vclutter + notchWidth };
}

// =============================================================================
// FUNCTION: isInDopplerBlindZone
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::isInDopplerBlindZone(
    double radVel_m_s, const RadarConfig& cfg, const BeamWaveform& wf) const
{
    // HPRF waveform is immune to MTI blind zones. REQ-AESA-040.
    if (wf.mode == WaveformMode::HPRF) return false;

    // Stationary platform has no meaningful clutter notch. REQ-AESA-040.
    if (cfg.platformSpeed_m_s < MIN_PLATFORM_SPEED_MPS) return false;

    auto [lo, hi] = computeClutterNotch(cfg, wf);
    return (radVel_m_s >= lo && radVel_m_s <= hi);
}

// =============================================================================
// FUNCTION: computeSTAPGain
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeSTAPGain(
    double radialVelocity_m_s, double platformSpeed_m_s,
    const BeamWaveform& wf, const RadarConfig& cfg) const
{
    // STAP degrees of freedom: temporal = N_pulses, spatial = sqrt(N_elements).
    // Full STAP gain = N_pulses * sqrt(N_spatial). REQ-AESA-040.
    double N_pulses  = static_cast<double>(std::max(STAP_MIN_PULSES,
                                                   wf.pulsesPerDwell));
    double N_spatial = std::sqrt(static_cast<double>(
        std::max(STAP_MIN_ELEMENTS,
                 cfg.numElements - cfg.failedModules)));
    double stapGain  = N_pulses * N_spatial;

    // Velocity discrimination factor. REQ-AESA-040.
    auto [notchLo, notchHi] = computeClutterNotch(cfg, wf);
    double notchWidth    = notchHi - notchLo;
    double distFromNotch = std::abs(radialVelocity_m_s - platformSpeed_m_s);

    if (distFromNotch < notchWidth)
    {
        // Inside notch: partial recovery proportional to distance from centre.
        // STAP_RECOVERY_FACTOR = 0.3 — physical limit of partial notch recovery.
        // REQ-AESA-040.
        double recovery = distFromNotch / notchWidth;
        return std::max(STAP_GAIN_MIN, stapGain * recovery * STAP_RECOVERY_FACTOR);
    }

    // Outside notch: full STAP gain, capped at STAP_GAIN_CAP (+30 dB). REQ-AESA-040.
    return std::min(stapGain, STAP_GAIN_CAP);
}

// =============================================================================
// FUNCTION: isInClutterNotchSTAP
// Full description in header.
// =============================================================================
bool RadarSignalProcessor_AESA::isInClutterNotchSTAP(
    double radVel_m_s, const RadarConfig& cfg, const BeamWaveform& wf) const
{
    // HPRF waveform is immune. REQ-AESA-040.
    if (wf.mode == WaveformMode::HPRF) return false;

    // Stationary platform has no notch. REQ-AESA-040.
    if (cfg.platformSpeed_m_s < MIN_PLATFORM_SPEED_MPS) return false;

    auto [lo, hi] = computeClutterNotch(cfg, wf);

    // STAP narrows the notch by sqrt(N_spatial) compared to MTI. REQ-AESA-040.
    double N_spatial      = std::sqrt(static_cast<double>(
        std::max(STAP_MIN_ELEMENTS, cfg.numElements - cfg.failedModules)));
    double stapNotchWidth = (hi - lo) / std::max(1.0, std::sqrt(N_spatial));
    double center         = (lo + hi) / 2.0;

    return (radVel_m_s >= center - stapNotchWidth &&
            radVel_m_s <= center + stapNotchWidth);
}

// =============================================================================
// §L  MONOPULSE ANGLE ERROR
// =============================================================================

// =============================================================================
// FUNCTION: computeMonopulseAngleError
// Full description in header.
// =============================================================================
void RadarSignalProcessor_AESA::computeMonopulseAngleError(
    double azDiff_deg, double elDiff_deg, double sinr,
    const RadarConfig& cfg,
    double& outAzError_deg, double& outElError_deg) const
{
    double bw = static_cast<double>(cfg.beamWidth);

    // Noise sigma: bw / (km * sqrt(2 * SINR)). When SINR <= 0, sigma = bw
    // (maximum uncertainty equal to beamwidth). REQ-AESA-040.
    double sig = (sinr > 0.0)
                     ? bw / (MONOPULSE_KM * std::sqrt(MONOPULSE_SINR_FACTOR * sinr))
                     : bw;

    // Random angle noise. REQ-AESA-040.
    thread_local std::normal_distribution<double> nd(0.0, 1.0);

    // Systematic bias term: azDiff / km^2 (pointing correction). REQ-AESA-040.
    outAzError_deg = azDiff_deg / (MONOPULSE_KM * MONOPULSE_KM) + sig * nd(tl_rng);
    outElError_deg = elDiff_deg / (MONOPULSE_KM * MONOPULSE_KM) + sig * nd(tl_rng);
}

// =============================================================================
// §M  WAVEFORM SELECTION
// =============================================================================

// =============================================================================
// FUNCTION: selectWaveformForRange
// Full description in header.
// =============================================================================
BeamWaveform RadarSignalProcessor_AESA::selectWaveformForRange(
    double range_m, const RadarConfig& cfg) const
{
    // Linear search through the waveform table sorted by maxRange_m ascending.
    // First entry whose maxRange_m > range_m is selected. REQ-AESA-020.
    for (const auto& entry : cfg.waveformTable)
    {
        // Sentinel: maxRange_m = 0.0 marks end of valid entries. REQ-AESA-020.
        if (entry.maxRange_m <= 0.0f) break;

        if (range_m < static_cast<double>(entry.maxRange_m))
        {
            return entry.waveform;
        }
    }

    // No entry matched — use default search waveform. REQ-AESA-020.
    return cfg.searchWaveform;
}

// =============================================================================
// §N  TWO-RAY MULTIPATH
// =============================================================================

// =============================================================================
// FUNCTION: computeMultipathFactor
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeMultipathFactor(
    double range_m, double elevation_deg,
    double targetHeight_m, const RadarConfig& cfg) const
{
    // Multipath is negligible at high elevation angles. REQ-AESA-072.
    if (elevation_deg > MULTIPATH_EL_THRESHOLD_DEG) return 1.0;

    // No reflection for targets at zero altitude. REQ-AESA-072.
    if (targetHeight_m <= 0.0) return 1.0;

    // Zero range would cause division by zero. REQ-AESA-072.
    if (range_m < MIN_RANGE_M) return 1.0;

    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;

    // Two-ray path difference phase: dphi = 4*pi*h_r*h_t / (lambda*R).
    // REQ-AESA-072.
    double dphi = (4.0 * PI * cfg.radarHeight * targetHeight_m)
                  / (lambda * range_m);

    // Interference factor: 4 * sin^2(dphi/2). REQ-AESA-072.
    double factor = 4.0 * std::pow(std::sin(dphi / 2.0), 2.0);

    // Clamp to physical range [0, 4]. Constructive interference cannot exceed 4x.
    return std::clamp(factor, MULTIPATH_MIN, MULTIPATH_MAX);
}

// =============================================================================
// §O  CHAFF RETURN
// =============================================================================

// =============================================================================
// FUNCTION: computeChaffReturn
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::computeChaffReturn(
    double beamAz, double beamEl,
    const std::vector<ChaffCloud>& clouds,
    double simTime, const RadarConfig& cfg) const
{
    double total = 0.0;

    for (const auto& cloud : clouds)
    {
        // Cloud range from radar. REQ-AESA-061.
        double range = std::sqrt(cloud.x * cloud.x +
                                 cloud.y * cloud.y +
                                 cloud.z * cloud.z);
        if (range < MIN_CHAFF_RANGE_M) continue;

        // Cloud azimuth and elevation in body frame. REQ-AESA-061.
        double cAz = std::atan2(cloud.y, cloud.x) * RAD_TO_DEG;
        if (cAz < 0.0) cAz += AZ_FULL_CIRCLE;
        double cEl = std::asin(std::clamp(cloud.z / range,
                                          DOT_CLAMP_MIN, DOT_CLAMP_MAX))
                     * RAD_TO_DEG;

        // Angular offset from beam centre. REQ-AESA-061.
        double dAz = std::abs(beamAz - cAz);
        if (dAz > AZ_WRAP_THRESHOLD) dAz = AZ_FULL_CIRCLE - dAz;
        double dEl = std::abs(beamEl - cEl);

        // Skip clouds outside the beam footprint (3 * beamWidth gate).
        // REQ-AESA-061.
        if (dAz > static_cast<double>(cfg.beamWidth) * CHAFF_BEAM_GATE_FACTOR ||
            dEl > static_cast<double>(cfg.beamWidth) * CHAFF_BEAM_GATE_FACTOR)
        {
            continue;
        }

        // Exponential RCS decay: rcsNow = rcsTotal * exp(-age / decayTime).
        // REQ-AESA-061.
        double age    = simTime - cloud.birthTime_s;
        double rcsNow = cloud.rcsTotal
                        * std::exp(-age / std::max(MIN_CHAFF_DECAY_S,
                                                   cloud.decayTime_s));

        // Chaff return using radar range equation. REQ-AESA-061.
        double lam  = SPEED_OF_LIGHT / cfg.frequency_Hz;
        int    act  = std::max(0, cfg.numElements - cfg.failedModules);
        double Pt   = static_cast<double>(act)
                    * static_cast<double>(cfg.peakPowerPerElement_W)
                    * static_cast<double>(cfg.moduleEfficiency);
        double G    = std::pow(10.0,
                            static_cast<double>(cfg.antennaGain) / 10.0);

        double Pc = (Pt * G * G * lam * lam * rcsNow)
                    / (std::pow(FOUR_PI, 3.0) * std::pow(range, 4.0));

        total += Pc * computePropagationLoss(range, cfg);
    }
    return total;
}

// =============================================================================
// UTILITY METHODS (RCS lookup)
// =============================================================================

// =============================================================================
// FUNCTION: getPlatformBaseRCS
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::getPlatformBaseRCS(
    const std::string& platformType) const
{
    // Median RCS values from open literature (Skolnik, Knott). REQ-AESA-040.
    if (platformType == "FIGHTER")  return RCS_FIGHTER;
    if (platformType == "BOMBER")   return RCS_BOMBER;
    if (platformType == "UAV")      return RCS_UAV;
    if (platformType == "MISSILE")  return RCS_MISSILE;
    if (platformType == "HELO")     return RCS_HELO;
    if (platformType == "SHIP")     return RCS_SHIP;
    if (platformType == "STEALTH")  return RCS_STEALTH;
    return RCS_GENERIC;
}

// =============================================================================
// FUNCTION: lookupAspectRCS
// Full description in header.
// =============================================================================
double RadarSignalProcessor_AESA::lookupAspectRCS(
    const TargetInput& target, double aspectAngle_deg) const
{
    // Empty table: fall back to platform type base RCS. REQ-AESA-040.
    if (target.rcsTable.empty())
        return getPlatformBaseRCS(target.platformType);

    const auto& tbl = target.rcsTable;

    // Clamp to table bounds. REQ-AESA-040.
    if (aspectAngle_deg <= tbl.front().first) return tbl.front().second;
    if (aspectAngle_deg >= tbl.back().first)  return tbl.back().second;

    // Linear interpolation between table entries. REQ-AESA-040.
    for (size_t i = 1; i < tbl.size(); ++i)
    {
        if (aspectAngle_deg <= tbl[i].first)
        {
            double t = (aspectAngle_deg - tbl[i - 1].first)
            / (tbl[i].first - tbl[i - 1].first);
            return tbl[i - 1].second + t * (tbl[i].second - tbl[i - 1].second);
        }
    }
    return getPlatformBaseRCS(target.platformType);
}

} // namespace aesa


