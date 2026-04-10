// radarsignalprocessor_aesa.cpp  —  Rev 3
// Fixes vs Rev 2:
//   - cfg.seaState / cfg.landClutter now exist in RadarConfig
//   - unused 'waveform' parameter in calculateSignalStrength resolved
//   - all other physics unchanged
#include "radarsignalprocessor_aesa.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>
//#include <QDebug>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr double BOLTZMANN      = 1.380649e-23;
static constexpr double SPEED_OF_LIGHT = 299792458.0;
static constexpr double MERGE_GATE     = 150.0;

static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

namespace aesa {

// =============================================================================
// §A  Geometry
// =============================================================================

bool RadarSignalProcessor_AESA::isTargetInBeam(
    double beamAz, double beamEl,
    double targetAz, double targetEl,
    const RadarConfig& cfg,
    double& outAzDiff, double& outElDiff,
    double effectiveBeamWidth) const
{
    outAzDiff = std::abs(beamAz - targetAz);
    if (outAzDiff > 180.0) outAzDiff = 360.0 - outAzDiff;
    outElDiff = std::abs(beamEl - targetEl);

    double bw   = (effectiveBeamWidth > 0.0) ? effectiveBeamWidth
                                             : static_cast<double>(cfg.beamWidth);
    double gate = bw * 2.5;
    return (outAzDiff <= gate && outElDiff <= gate);
}

bool RadarSignalProcessor_AESA::checkHorizon(double range, double targetZ,
                                              const RadarConfig& cfg) const
{
    double Re     = 6371000.0 * cfg.earthRadiusFactor * cfg.atmosphericFactor;
    double dRadar = std::sqrt(2.0 * Re * std::max(0.0, cfg.radarHeight));
    double dTgt   = std::sqrt(2.0 * Re * std::max(0.0, targetZ));
    return range <= (dRadar + dTgt);
}

// =============================================================================
// §B  Signal chain
// =============================================================================

double RadarSignalProcessor_AESA::calculateSignalStrength(
    double range, double rcs,
    double arrayGain,
    const BeamWaveform& /*waveform*/,   // parameter retained for API consistency
    const RadarConfig& cfg) const
{
    if (range < 1.0) range = 1.0;

    // FIX-03 LPI: pulse-to-pulse random frequency hop
    double freq = cfg.frequency_Hz;
    if (cfg.frequencyAgility &&
        cfg.hopStopFrequency > cfg.hopStartFrequency &&
        cfg.hopStopFrequency > 0.0f)
    {
        thread_local std::uniform_real_distribution<double> hopDist(0.0, 1.0);
        freq = static_cast<double>(cfg.hopStartFrequency)
             + hopDist(tl_rng) * static_cast<double>(
                   cfg.hopStopFrequency - cfg.hopStartFrequency);
    }
    double lambda = SPEED_OF_LIGHT / freq;

    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt     = static_cast<double>(active)
                  * static_cast<double>(cfg.peakPowerPerElement_W)
                  * static_cast<double>(cfg.moduleEfficiency);

    double Pr = (Pt * arrayGain * arrayGain * lambda * lambda * rcs)
                / (std::pow(4.0 * M_PI, 3.0) * std::pow(range, 4.0));

   // double propLoss = computePropagationLoss(range, cfg);

    return std::max(0.0, Pr * computePropagationLoss(range, cfg));
}

double RadarSignalProcessor_AESA::computeNoisePower(const RadarConfig& cfg,
                                                     double bandwidth_Hz) const
{
    double F = std::pow(10.0, cfg.noiseFigure_dB / 10.0);
    return BOLTZMANN * cfg.systemTemperature_K * std::max(1.0, bandwidth_Hz) * F;
}

double RadarSignalProcessor_AESA::computeClutterPower(double range,
                                                       SurfaceType surface,
                                                       const RadarConfig& cfg) const
{
    if (surface == SurfaceType::AIR || range < 1.0) return 0.0;

    // seaState / landClutter now exist in RadarConfig (added in master header)
    double sigma0 = 0.0;
   // if (surface == SurfaceType::SEA)  sigma0 = static_cast<double>(cfg.seaState)  * 3e-3;
    //if (surface == SurfaceType::LAND) sigma0 = static_cast<double>(cfg.landClutter) * 1e-2;
    // Grazing angle — flat-earth approximation, valid to ~100 km
    // Beyond that the horizon check already gates out the target
    double sinPsi = std::clamp(cfg.radarHeight / std::max(range, 1.0), 1e-4, 1.0);
    double psi_rad = std::asin(sinPsi);

    if (surface == SurfaceType::SEA)
    {
        // GIT sea clutter model (Horst et al. 1978, X-band HH baseline)
        // σ₀(dB) = -61.4 + 40·log10(f_GHz) + 10.1·log10(1 + SS) + 30·log10(sin ψ)
        double f_GHz   = cfg.frequency_Hz / 1.0e9;
        double SS      = std::clamp(static_cast<double>(cfg.seaState), 0.0, 6.0);
        double s0_dB   = -61.4
                       + 40.0  * std::log10(std::max(f_GHz, 0.1))
                       + 10.1  * std::log10(1.0 + SS)
                       + 30.0  * std::log10(sinPsi);
        sigma0 = std::pow(10.0, s0_dB / 10.0);
    }
    if (surface == SurfaceType::LAND)
    {
        // GIT land clutter — Billingsley low-relief terrain baseline (X-band)
        // σ₀(dB) = -25 + 8·log10(sin ψ) + terrain_factor
        // cfg.landClutter is 0–1 terrain roughness scale
        double terrain_dB = static_cast<double>(cfg.landClutter) * 15.0; // 0 dB (smooth) to +15 dB (urban)
        double s0_dB      = -25.0 + 8.0 * std::log10(sinPsi) + terrain_dB;
        sigma0 = std::pow(10.0, s0_dB / 10.0);
    }
    if (sigma0 <= 0.0) return 0.0;

    double tau   = static_cast<double>(cfg.searchWaveform.pulseWidth_s);
    double bwRad = static_cast<double>(cfg.beamWidth) * M_PI / 180.0;
    double patch = (SPEED_OF_LIGHT * tau / 2.0) * (range * bwRad);

    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt     = static_cast<double>(active)
                  * static_cast<double>(cfg.peakPowerPerElement_W)
                  * static_cast<double>(cfg.moduleEfficiency);
    double G      = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);

    double Pc = (Pt * G * G * lambda * lambda * sigma0 * patch)
                / (std::pow(4.0 * M_PI, 3.0) * std::pow(range, 3.0));

    thread_local std::exponential_distribution<double> fluct(1.0);
    return Pc * fluct(tl_rng);
}

double RadarSignalProcessor_AESA::computeJammerPower(double targetRange_m,
                                                      const TargetInput& target,
                                                      const RadarConfig& cfg) const
{
    const auto& j = target.jammer;
    if (!j.active || j.power_kW <= 0.0) return 0.0;

    double Pj  = j.power_kW * 1000.0;
    double Gj  = std::pow(10.0, j.gain_dBi / 10.0);
    double Gr  = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
    double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double Rj  = j.selfScreening ? targetRange_m
                                 : (j.range_m > 1.0 ? j.range_m : targetRange_m);

    double Pr_j = (Pj * Gj * Gr * lam * lam)
                  / (std::pow(4.0 * M_PI, 2.0) * Rj * Rj);

    double B_r = std::max(1.0, cfg.antennaBandwidth);
    double B_j = std::max(1.0, j.bandwidth_Hz);
    return Pr_j * std::min(1.0, B_r / B_j);

}
// ============================================================================
// Water vapour density — Buck (1981) / ITU-R P.836-6
// Returns absolute water vapour density ρ (g/m³)
// from relative humidity and temperature
// Valid: -40°C to +60°C, 0–100% RH
// ============================================================================
double RadarSignalProcessor_AESA::computeWaterVapourDensity(
    const AtmosphericConditions& atm) const
{
    double T   = static_cast<double>(atm.temperature_C);  // °C
    double RH  = static_cast<double>(atm.humidity_pct);   // %
    double T_K = T + 273.15;                              // Kelvin

    // Magnus-Tetens saturation vapour pressure (hPa)
    double e_s = 6.1121 * std::exp((18.678 - T / 234.5)
                                   * (T / (257.14 + T)));

    // Actual vapour pressure (hPa)
    double e_a = (RH / 100.0) * e_s;

    // Convert to absolute density (g/m³)
    // ρ = e_a(Pa) × M_w / (R_u × T_K)
    // M_w = 18.015 g/mol, R_u = 8314.46 J/(kmol·K)
    double rho_w = (e_a * 100.0 * 18.015)
                   / (8314.46 * T_K / 1000.0);

    return std::max(0.0, rho_w);  // g/m³
}

// ============================================================================
// Gaseous attenuation — ITU-R P.676-12 Annex 2
//
// Computes two-way path loss (dB) from:
//   γ_o — oxygen / dry air absorption
//   γ_w — water vapour absorption
//
// Frequency range: 1–350 GHz
// Key features in radar bands:
//   22.235 GHz — water vapour resonance (strongest below 100 GHz)
//   60 GHz     — oxygen complex
// ============================================================================
double RadarSignalProcessor_AESA::computeGaseousAttenuation(
    double frequency_Hz,
    const AtmosphericConditions& atm,
    double range_m) const
{
    double f   = frequency_Hz / 1.0e9;                    // GHz
    double T   = static_cast<double>(atm.temperature_C);  // °C
    double p   = static_cast<double>(atm.pressure_hPa);   // hPa
    double rho = computeWaterVapourDensity(atm);          // g/m³

    // ITU-R P.676-12 reduced variables
    double r_p = p / 1013.25;                             // pressure ratio
    double r_t = 288.15 / (273.15 + T);                   // temperature ratio

    // ----------------------------------------------------------------
    // Oxygen specific attenuation γ_o (dB/km)
    // ITU-R P.676-12 Annex 2, Equation 1
    // ----------------------------------------------------------------
    double gamma_o = 0.0;
    {
        double xi1 = std::pow(r_p, 0.0717) * std::pow(r_t, -1.8132)
        * std::exp(0.1147 * (1.0 - r_p)
                  + 1.4434 * (1.0 - r_t));

        double xi2 = std::pow(r_p, 0.5146) * std::pow(r_t, -4.6368)
                     * std::exp(-0.1217 * (1.0 - r_p)
                                +  2.1441 * (1.0 - r_t));

        double xi3 = std::pow(r_p, 0.3414) * std::pow(r_t, -6.5851)
                     * std::exp(0.2177 * (1.0 - r_p)
                                + 5.4677 * (1.0 - r_t));

        gamma_o = ( 7.2  * std::pow(r_t, 2.8)
                       / (f*f + 0.34 * r_p*r_p * std::pow(r_t, 1.6))
                   + 0.62 * xi3
                         / (std::pow(std::abs(54.0 - f), 1.16 * xi1)
                            + 0.83 * xi2) )
                  * f * f * r_p * r_p * 1.0e-3;

        gamma_o = std::max(0.0, gamma_o);
    }

    // ----------------------------------------------------------------
    // Water vapour specific attenuation γ_w (dB/km)
    // ITU-R P.676-12 Annex 2, Equation 2
    // ----------------------------------------------------------------
    double gamma_w = 0.0;
    {
        double eta1 = 0.955 * r_p * std::pow(r_t, 0.68)
        + 0.006 * rho;

        double eta2 = 0.735 * r_p * std::pow(r_t, 0.5)
                      + 0.0353 * std::pow(r_t, 4.0) * rho;

        // Line shape correction
        auto g = [](double f_val, double f_line) -> double {
            return 1.0 + std::pow((f_val - f_line) / (f_val + f_line), 2.0);
        };

        // Guard against division by zero near exact line frequencies
        auto safe_line = [](double f_val, double f_line,
                            double eta, double width) -> double {
            return eta / (std::pow(f_val - f_line, 2.0)
                          + std::max(width * width, 1.0e-6));
        };

        gamma_w = (
                      // 22.235 GHz — dominant water vapour line in radar band
                      3.98  * eta1 * std::exp(2.23  * (1.0 - r_t))
                          * safe_line(f, 22.235, 1.0, 9.42 * eta1)
                          * g(f, 22.235)

                      // 183.310 GHz
                      + 11.96 * eta1 * std::exp(0.7   * (1.0 - r_t))
                            * safe_line(f, 183.31,  1.0, 11.14 * eta1)

                      // 321.226 GHz
                      + 0.081 * eta1 * std::exp(6.44  * (1.0 - r_t))
                            * safe_line(f, 321.226, 1.0, 6.29 * eta1)

                      // 325.153 GHz
                      + 3.66  * eta1 * std::exp(1.6   * (1.0 - r_t))
                            * safe_line(f, 325.153, 1.0, 9.22 * eta1)

                      // 380 GHz
                      + 25.37 * eta1 * std::exp(1.09  * (1.0 - r_t))
                            * safe_line(f, 380.0,   1.0, 1.0)

                      // 448 GHz
                      + 17.4  * eta1 * std::exp(1.46  * (1.0 - r_t))
                            * safe_line(f, 448.0,   1.0, 1.0)

                      // 557 GHz
                      + 844.6 * eta1 * std::exp(0.17  * (1.0 - r_t))
                            * safe_line(f, 557.0,   1.0, 1.0)
                            * g(f, 557.0)

                      // 752 GHz
                      + 290.0 * eta1 * std::exp(0.41  * (1.0 - r_t))
                            * safe_line(f, 752.0,   1.0, 1.0)
                            * g(f, 752.0)

                      // 1780 GHz
                      + 83328.0 * eta2 * std::exp(0.99 * (1.0 - r_t))
                            * safe_line(f, 1780.0,  1.0, 1.0)
                            * g(f, 1780.0)
                      )
                  * f * f * std::pow(r_t, 2.5) * rho * 1.0e-4;

        gamma_w = std::max(0.0, gamma_w);
    }

    // Two-way total gaseous loss (dB)
    double gamma_total = gamma_o + gamma_w;              // dB/km one-way


    return 2.0 * gamma_total * (range_m / 1000.0);      // dB two-way
}
double RadarSignalProcessor_AESA::computePropagationLoss(
    double range_m, const RadarConfig& cfg) const
{
    double loss_dB = 0.0;

    // ---- Rain (ITU-R P.838-3) -------------------------------------------
    if (cfg.atmosphere.rainRate_mmph > 0.0)
    {
        double gamma_rain = 0.00887
                            * std::pow(static_cast<double>(cfg.atmosphere.rainRate_mmph), 1.255);
        loss_dB += 2.0 * gamma_rain * (range_m / 1000.0);
    }

    // ---- Fog (Kunkel 1984) -----------------------------------------------
    if (cfg.atmosphere.fogVisibility_m > 1.0 &&
        cfg.atmosphere.fogVisibility_m < 2000.0)
    {
        double M_fog     = 0.0367
                       * std::pow(1000.0 / static_cast<double>(
                                      cfg.atmosphere.fogVisibility_m), 1.43);
        double gamma_fog = 0.0157 * std::pow(M_fog, 1.05);
        loss_dB += 2.0 * gamma_fog * (range_m / 1000.0);
    }

    // ---- Gaseous: O2 + H2O (ITU-R P.676-12) ----------------------------
    loss_dB += computeGaseousAttenuation(cfg.frequency_Hz,
                                         cfg.atmosphere,
                                         range_m);

    // ---- Debug (remove when confirmed working) --------------------------


    return std::pow(10.0, -loss_dB / 10.0);
}

// double RadarSignalProcessor_AESA::computePropagationLoss(double range_m,
//                                                           const RadarConfig& cfg) const
// {
//     double loss_dB = 0.0;
//     if (cfg.rainRate_mmph > 0.0)
//     {
//         double gamma = 0.00887 * std::pow(cfg.rainRate_mmph, 1.255);
//         loss_dB += 2.0 * gamma * (range_m / 1000.0);
//     }
//     if (cfg.fogVisibility_m > 1.0 && cfg.fogVisibility_m < 2000.0)
//     {
//         double M     = 0.0367 * std::pow(1000.0 / cfg.fogVisibility_m, 1.43);
//         double gamma = 0.0157 * std::pow(M, 1.05);
//         loss_dB += 2.0 * gamma * (range_m / 1000.0);
//     }
//     return std::pow(10.0, -loss_dB / 10.0);
// }

double RadarSignalProcessor_AESA::computeSINR(
    double receivedPower, double range,
    SurfaceType surface, const TargetInput& target,
    const RadarConfig& cfg, const BeamWaveform& waveform) const
{
    double Pn = computeNoisePower(cfg, static_cast<double>(waveform.bandwidth_Hz));
    double Pc = computeClutterPower(range, surface, cfg);
    double Pj = computeJammerPower(range, target, cfg);

    double pg = computeModulationProcessingGain(waveform);
    double ig = static_cast<double>(std::max(1, waveform.pulsesPerDwell));

    // Null steering jammer suppression
    double jSuppress = 1.0;
    if (cfg.nullSteering.active && target.jammer.active)
    {
        double jAz  = std::atan2(target.y, target.x) * (180.0 / M_PI);
        if (jAz < 0.0) jAz += 360.0;
        double jEl  = (range > 1.0)
            ? std::asin(std::clamp(target.z/range,-1.0,1.0)) * (180.0/M_PI) : 0.0;

        double dAz = std::abs(jAz - cfg.nullSteering.azimuth_deg);
        if (dAz > 180.0) dAz = 360.0 - dAz;
        double dEl = std::abs(jEl - cfg.nullSteering.elevation_deg);

        if (dAz < static_cast<double>(cfg.beamWidth) * 2.0 &&
            dEl < static_cast<double>(cfg.beamWidth) * 2.0)
            jSuppress = std::pow(10.0, static_cast<double>(cfg.nullSteering.nullDepth_dB) / 10.0);
    }

    return std::max(0.0,
        (receivedPower * pg * ig) / (Pn + Pc + Pj * jSuppress));
}

// =============================================================================
// §C  CFAR
// =============================================================================

std::vector<double> RadarSignalProcessor_AESA::generateReferenceCells(
    SurfaceType surface, const RadarConfig& cfg) const
{
    thread_local std::exponential_distribution<double> cellDist(1.0);
    std::vector<double> cells; cells.reserve(16);
    for (int i = 0; i < 16; ++i)
    {
        double c = cellDist(tl_rng);
        if (surface == SurfaceType::SEA)
            c *= (1.0 + static_cast<double>(cfg.seaState)    * 0.3);
        if (surface == SurfaceType::LAND)
            c *= (1.0 + static_cast<double>(cfg.landClutter) * 0.5);
        cells.push_back(c);
    }
    return cells;
}

double RadarSignalProcessor_AESA::computeCFARThreshold(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N     = static_cast<double>(cells.size());
    double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

double RadarSignalProcessor_AESA::computeCFARThresholdRelaxed(
    const std::vector<double>& cells, const RadarConfig& cfg) const
{
    if (cells.empty()) return 1e12;
    double sum = 0.0;
    for (double v : cells) sum += v;
    double N     = static_cast<double>(cells.size());
    double pfa   = std::min(1e-4, cfg.targetPfa * 100.0);
    double alpha = N * (std::pow(pfa, -1.0 / N) - 1.0);
    return (sum / N) * alpha;
}

// =============================================================================
// §D  RCS + FIX-07 Swerling fluctuation
// =============================================================================
// ============================================================================
// §D  Defense-Grade Physical Optics RCS — Rev 4
//
// Method:    6-facet box decomposition, optical regime, incoherent summation
// Fidelity:  NATO STANAG Level 2 — aspect + frequency + material dependent
//
// References:
//  [1] Knott, Shaeffer, Tuley — Radar Cross Section, SciTech 2004, Ch 4-5
//  [2] Ruck et al — Radar Cross Section Handbook, Plenum 1970, Ch 3
//  [3] Skolnik — Introduction to Radar Systems, 3rd Ed, Ch 11
//  [4] IEEE Std 1672-2006 — Radar Cross Section Test Methods
// ============================================================================

// ----------------------------------------------------------------------------
// Material power reduction factor (linear, not dB)
// Validated against open-literature measured RCS reduction data [1, Table 5.1]
// ----------------------------------------------------------------------------
static double rcs_materialFactor(TargetMaterialType mat)
{
    switch (mat)
    {
    case TargetMaterialType::METAL:     return 1.000;  //   0 dB — bare metal
    case TargetMaterialType::COMPOSITE: return 0.500;  //  -3 dB — CFRP airframe
    case TargetMaterialType::RAM:       return 0.032;  // -15 dB — RAM coating
    case TargetMaterialType::STEALTHY:  return 0.003;  // -25 dB — full VLO
    default:                            return 1.000;
    }
}

// ----------------------------------------------------------------------------
// Surface coherence efficiency per face
// Accounts for curvature, surface irregularities, edges reducing specular
// return below the flat-plate Physical Optics ideal
// Calibrated so mean broadside RCS ≈ η × projected_area
// Values from [1] Ch 5 adjusted for entity-level box primitives
// ----------------------------------------------------------------------------
static double rcs_shapeFactor(TargetShapeType shape)
{
    switch (shape)
    {
    case TargetShapeType::BOX:      return 0.40;  // flat faces, dihedral corners
    case TargetShapeType::AIRCRAFT: return 0.08;  // curved surfaces, wing blending
    case TargetShapeType::SHIP:     return 0.50;  // flat superstructure, hard corners
    case TargetShapeType::MISSILE:  return 0.10;  // cylindrical, ogive nose
    case TargetShapeType::GENERIC:  return 0.15;
    default:                        return 0.15;
    }
}

// ----------------------------------------------------------------------------
// Per-face directional efficiency modifier
// Front/rear faces of aircraft are shaped to deflect radar energy —
// they contribute less than simple geometry predicts.
// For BOX/SHIP/GENERIC all faces treated equally.
// ----------------------------------------------------------------------------
static double rcs_faceFactor(TargetShapeType shape, int faceIndex)
{
    // faceIndex: 0=front, 1=rear, 2=right, 3=left, 4=top, 5=bottom
    if (shape == TargetShapeType::AIRCRAFT)
    {
        switch (faceIndex)
        {
        case 0: return 0.08;   // nose — engine intake shaped to reduce RCS
        case 1: return 0.12;   // tail — exhaust signature, slightly higher
        case 2:
        case 3: return 1.00;   // sides — wings dominate, max contributor
        case 4: return 0.60;   // top — fuselage spine
        case 5: return 0.40;   // bottom — smoother lower surface
        default: return 1.00;
        }
    }
    if (shape == TargetShapeType::MISSILE)
    {
        switch (faceIndex)
        {
        case 0: return 0.05;   // ogive nose — very low head-on RCS
        case 1: return 0.10;   // tail — nozzle
        default: return 1.00;  // sides — cylindrical broadside
        }
    }
    return 1.00; // BOX, SHIP, GENERIC — all faces equal
}

// ----------------------------------------------------------------------------
// Frequency regime
// Determines which scattering model applies
// Rayleigh (kD < 0.5): volume scattering dominates, σ ∝ f⁴
// Resonance (kD 0.5–5): Mie region, complex resonances
// Optical   (kD > 5) : surface scattering dominates, σ ≈ f⁰ (flat)
// Ref: [1] Ch 2, [3] Ch 11.2
// ----------------------------------------------------------------------------
enum class ScatteringRegime { RAYLEIGH, RESONANCE, OPTICAL };

static ScatteringRegime rcs_regime(double characteristicDim, double lambda)
{
    double kD = (2.0 * M_PI / lambda) * characteristicDim;
    if (kD < 0.5) return ScatteringRegime::RAYLEIGH;
    if (kD < 5.0) return ScatteringRegime::RESONANCE;
    return ScatteringRegime::OPTICAL;
}

// ----------------------------------------------------------------------------
// Compute single-face RCS contribution
//
// Physics:
//   Optical regime: σ_face = A × cosθ × η_shape × η_face × η_material
//
//   This is the MEAN Physical Optics result, integrated over the sinc²
//   angular pattern of a flat plate. The instantaneous peak (4πA²/λ²)
//   is a specular glint that occupies a solid angle ≈ (λ/D)² — far
//   smaller than any simulation time step illuminates. Using the mean
//   value is correct for entity-level simulation. [1, Sec 4.3], [4]
//
//   Rayleigh regime: σ_face = k⁴ × V² × (geometry factor)
//   Resonance:       interpolate between regimes
//
// Parameters:
//   area      — face area (m²)
//   cosTheta  — illumination angle cosine (dot of radar dir with face normal)
//   lambda    — wavelength (m)
//   volume    — target volume (m³) — used for Rayleigh only
//   eta_shape — shape efficiency for this face
//   eta_mat   — material factor
// ----------------------------------------------------------------------------
static double rcs_faceSigma(double area, double cosTheta,
                            double lambda, double volume,
                            ScatteringRegime regime,
                            double eta_shape, double eta_mat)
{
    if (cosTheta <= 0.0) return 0.0;  // shadowed face

    double sigma = 0.0;

    switch (regime)
    {
    case ScatteringRegime::OPTICAL:
    {
        // Mean Physical Optics — projected area model
        // σ = A × cosθ × η_shape × η_mat
        sigma = area * cosTheta * eta_shape * eta_mat;
        break;
    }
    case ScatteringRegime::RAYLEIGH:
    {
        // Rayleigh scattering — volume-dependent
        // σ = (8π/3) × k⁴ × V² × η_mat
        // k = 2π/λ
        // Ref: [1] Eq 2.14, [3] Sec 11.2a
        double k = 2.0 * M_PI / lambda;
        sigma = (8.0 * M_PI / 3.0)
                * std::pow(k, 4.0)
                * volume * volume
                * eta_mat;
        break;
    }
    case ScatteringRegime::RESONANCE:
    {
        // Mie resonance regime — interpolate between Rayleigh and optical
        // Use geometric mean of the two contributions as a practical
        // approximation. Full Mie series requires spherical geometry.
        // Ref: [1] Sec 2.4 — resonance region approximation
        double k = 2.0 * M_PI / lambda;
        double sigma_ray = (8.0 * M_PI / 3.0) * std::pow(k, 4.0)
                           * volume * volume * eta_mat;
        double sigma_opt = area * cosTheta * eta_shape * eta_mat;
        sigma = std::sqrt(sigma_ray * sigma_opt);  // geometric mean
        break;
    }
    }

    return sigma;
}

// ============================================================================
// Main function
// ============================================================================

double RadarSignalProcessor_AESA::computeEffectiveRCS(
    const TargetInput& target, double range, double frequency_Hz) const
{
    double base = 0.0;

    if (target.dimensions.valid)
    {
        // ----------------------------------------------------------------
        // 1. Extract dimensions and establish body-frame axes
        // ----------------------------------------------------------------
        double L = std::max(target.dimensions.length, 0.01);
        double H = std::max(target.dimensions.height, 0.01);
        double W = std::max(target.dimensions.width,  0.01);

        double lambda = SPEED_OF_LIGHT / std::max(frequency_Hz, 1.0);
        double volume = L * H * W;

        // Body-frame forward axis — derived from velocity vector
        // If target is stationary, use radar line-of-sight as forward
        double speed = std::sqrt(target.vx*target.vx +
                                 target.vy*target.vy +
                                 target.vz*target.vz);

        double fx, fy, fz;  // forward (length axis)
        double rx, ry, rz;  // right   (width axis)
        double ux, uy, uz;  // up      (height axis)

        if (speed > 0.5)
        {
            fx = target.vx / speed;
            fy = target.vy / speed;
            fz = target.vz / speed;
        }
        else
        {
            // Stationary — point forward toward radar
            fx = -target.x / std::max(range, 1.0);
            fy = -target.y / std::max(range, 1.0);
            fz = -target.z / std::max(range, 1.0);
        }

        // World up = (0, 0, 1) in radar frame
        // right = forward × up
        rx = fy * 1.0 - fz * 0.0;
        ry = fz * 0.0 - fx * 1.0;
        rz = fx * 0.0 - fy * 0.0;
        double rmag = std::sqrt(rx*rx + ry*ry + rz*rz);
        if (rmag < 1e-6) { rx = 0; ry = 1; rz = 0; rmag = 1.0; }
        rx /= rmag; ry /= rmag; rz /= rmag;

        // up = right × forward
        ux = ry*fz - rz*fy;
        uy = rz*fx - rx*fz;
        uz = rx*fy - ry*fx;

        // ----------------------------------------------------------------
        // 2. Unit vector from target to radar (illumination direction)
        // ----------------------------------------------------------------
        double ix = -target.x / std::max(range, 1.0);
        double iy = -target.y / std::max(range, 1.0);
        double iz = -target.z / std::max(range, 1.0);

        // ----------------------------------------------------------------
        // 3. Define 6 faces: {normal_x,y,z,  dim_a, dim_b, faceIndex}
        // ----------------------------------------------------------------
        struct Face {
            double nx, ny, nz;
            double a, b;
            int    idx;
        };

        Face faces[6] = {
            {  fx,  fy,  fz,  W, H, 0 },   // front
            { -fx, -fy, -fz,  W, H, 1 },   // rear
            {  rx,  ry,  rz,  L, H, 2 },   // right
            { -rx, -ry, -rz,  L, H, 3 },   // left
            {  ux,  uy,  uz,  L, W, 4 },   // top
            { -ux, -uy, -uz,  L, W, 5 },   // bottom
        };

        // ----------------------------------------------------------------
        // 4. Frequency regime — use smallest dimension as characteristic
        // ----------------------------------------------------------------
        double D_char = std::min({L, H, W});
        ScatteringRegime regime = rcs_regime(D_char, lambda);

        // ----------------------------------------------------------------
        // 5. Per-target factors
        // ----------------------------------------------------------------
        double eta_mat   = rcs_materialFactor(target.dimensions.material);
        double eta_shape = rcs_shapeFactor(target.dimensions.shape);

        // ----------------------------------------------------------------
        // 6. Sum face contributions incoherently
        // Incoherent summation is appropriate because:
        //   a) Frequency agility decorrelates phase between faces
        //   b) Swerling model handles coherent fluctuation separately
        //   c) Entity-level simulation does not resolve face-to-face
        //      phase differences (requires sub-wavelength positioning)
        // Ref: [1] Sec 5.3, [4] Sec 6.2
        // ----------------------------------------------------------------
        double sigma_faces = 0.0;
        for (const auto& f : faces)
        {
            double cosTheta = f.nx*ix + f.ny*iy + f.nz*iz;
            if (cosTheta <= 0.0) continue;

            double A         = f.a * f.b;
            double eta_face  = rcs_faceFactor(target.dimensions.shape, f.idx);

            sigma_faces += rcs_faceSigma(A, cosTheta, lambda, volume,
                                         regime, eta_shape * eta_face, eta_mat);
        }

        // ----------------------------------------------------------------
        // 7. Edge diffraction contribution
        // Corner/edge diffraction adds a frequency-dependent floor RCS
        // even at non-specular angles. Particularly important for low-
        // grazing-angle geometry and box-like shapes with hard edges.
        // Model: σ_edge = λ × perimeter / (8π)
        // Ref: [2] Ch 5 — GTD edge diffraction
        // ----------------------------------------------------------------
        double perimeter_total = 4.0 * (L + H + W);  // sum of all edge lengths
        double sigma_edge = (lambda * perimeter_total) / (8.0 * M_PI)
                            * eta_mat;

        base = sigma_faces + sigma_edge;


    }
    else if (!target.rcsTable.empty())
    {
        // Manual table provided — interpolate aspect angle
        double velMag = std::sqrt(target.vx*target.vx +
                                  target.vy*target.vy +
                                  target.vz*target.vz);
        double aspectAngle_deg = 90.0;
        if (velMag > 0.01 && range > 1.0)
        {
            double dot = std::clamp(
                (target.vx/velMag)*(target.x/range) +
                    (target.vy/velMag)*(target.y/range) +
                    (target.vz/velMag)*(target.z/range), -1.0, 1.0);
            aspectAngle_deg = std::acos(dot) * 180.0 / M_PI;
        }
        base = lookupAspectRCS(target, aspectAngle_deg);
    }
    else
    {
        // Last resort
        base = getPlatformBaseRCS(target.platformType);
    }

    // --------------------------------------------------------------------
    // 8. Swerling fluctuation — temporal decorrelation of RCS
    //    Applied AFTER the geometric mean is computed so that the
    //    physical scattering model sets the mean and Swerling sets
    //    the statistical distribution around it. [1] Ch 2, [3] Ch 2.7
    // --------------------------------------------------------------------
    bool coherent = (target.swerlingCase == SwerlingCase::CASE_II ||
                     target.swerlingCase == SwerlingCase::CASE_IV);
    double finalRCS = computeSwerlingRCS(base, target.swerlingCase, coherent);



    return finalRCS;
}
// double RadarSignalProcessor_AESA::computeEffectiveRCS(
//     const TargetInput& target, double range) const
// {
//     // Compute aspect angle — angle between target velocity and radar LOS
//     double velMag = std::sqrt(target.vx*target.vx +
//                               target.vy*target.vy +
//                               target.vz*target.vz);

//     double aspectAngle_deg = 90.0; // default broadside
//     if (velMag > 0.01 && range > 1.0)
//     {
//         double dot = std::clamp(
//             (target.vx/velMag)*(target.x/range) +
//                 (target.vy/velMag)*(target.y/range) +
//                 (target.vz/velMag)*(target.z/range), -1.0, 1.0);
//         aspectAngle_deg = std::acos(dot) * 180.0 / M_PI;
//     }

//     // Get aspect-dependent base RCS
//     double base = lookupAspectRCS(target, aspectAngle_deg);

//     // Apply Swerling fluctuation
//     bool coherent = (target.swerlingCase == SwerlingCase::CASE_II ||
//                      target.swerlingCase == SwerlingCase::CASE_IV);
//     return computeSwerlingRCS(base, target.swerlingCase, coherent);
// }

double RadarSignalProcessor_AESA::computeSTAPGain(
    double radialVelocity_m_s,
    double platformSpeed_m_s,
    const BeamWaveform& wf,
    const RadarConfig& cfg) const
{
    // STAP improvement factor over MTI
    // Based on number of degrees of freedom = pulsesPerDwell × array elements
    // Simplified: improvement = 10*log10(N_pulses × N_spatial)
    // We model spatial DOF as sqrt(numElements) for single platform
    double N_pulses  = static_cast<double>(std::max(1, wf.pulsesPerDwell));
    double N_spatial = std::sqrt(static_cast<double>(
        std::max(1, cfg.numElements - cfg.failedModules)));
    double stapGain  = N_pulses * N_spatial;

    // Velocity discrimination factor — targets far from clutter
    // notch get full STAP benefit, targets in notch get none
    auto [notchLo, notchHi] = computeClutterNotch(cfg, wf);
    double notchWidth = notchHi - notchLo;
    double distFromNotch = std::abs(radialVelocity_m_s - platformSpeed_m_s);

    if (distFromNotch < notchWidth)
    {
        // Inside notch — partial STAP recovery proportional to distance
        double recovery = distFromNotch / notchWidth;
        return std::max(1.0, stapGain * recovery * 0.3);
    }

    return std::min(stapGain, 1000.0); // cap at +30dB
}

bool RadarSignalProcessor_AESA::isInClutterNotchSTAP(
    double radVel_m_s,
    const RadarConfig& cfg,
    const BeamWaveform& wf) const
{
    // STAP narrows the notch compared to MTI by ~sqrt(N_spatial)
    if (wf.mode == WaveformMode::HPRF) return false;
    if (cfg.platformSpeed_m_s < 1.0f) return false;

    auto [lo, hi] = computeClutterNotch(cfg, wf);
    double N_spatial = std::sqrt(static_cast<double>(
        std::max(1, cfg.numElements - cfg.failedModules)));
    double stapNotchWidth = (hi - lo) / std::max(1.0, std::sqrt(N_spatial));
    double center = (lo + hi) / 2.0;

    return (radVel_m_s >= center - stapNotchWidth &&
            radVel_m_s <= center + stapNotchWidth);
}
double RadarSignalProcessor_AESA::computeSwerlingRCS(double nominalRCS,
                                                      SwerlingCase sc,
                                                      bool /*coherentDwell*/) const
{
    if (nominalRCS <= 0.0) return 0.0;
    switch (sc)
    {
    case SwerlingCase::CASE_0:
        return nominalRCS;
    case SwerlingCase::CASE_I:
    case SwerlingCase::CASE_II: {
        thread_local std::exponential_distribution<double> ed(1.0);
        return nominalRCS * ed(tl_rng);
    }
    case SwerlingCase::CASE_III:
    case SwerlingCase::CASE_IV: {
        thread_local std::exponential_distribution<double> ed(1.0);
        double half = nominalRCS / 2.0;
        return half * (ed(tl_rng) + ed(tl_rng));
    }
    }
    return nominalRCS;
}

// =============================================================================
// §E  Target motion + FIX-07 Albersheim Pd
// =============================================================================

void RadarSignalProcessor_AESA::computeTargetMotionParams(
    DetectionOutput& det, const TargetInput& target, double range) const
{
    det.speedOverGround = std::sqrt(target.vx*target.vx + target.vy*target.vy);
    det.heading = std::atan2(target.vy, target.vx) * (180.0 / M_PI);
    if (det.heading < 0.0) det.heading += 360.0;
    det.acceleration = 0.0;

    if (det.speedOverGround > 0.01 && range > 1e-6)
    {
        double s = det.speedOverGround;
        double dot = std::clamp(
            (target.vx/s)*(target.x/range) +
            (target.vz > 0.001 ? (target.vz/s)*(target.z/range) : 0.0),
            -1.0, 1.0);
        det.targetAspect = std::acos(dot) * 180.0 / M_PI;
    }
    else det.targetAspect = 0.0;
}

double RadarSignalProcessor_AESA::computeRadialVelocity(
    const TargetInput& target, double range,
    std::normal_distribution<double>& noise) const
{
    double dot = target.vx*target.x + target.vy*target.y + target.vz*target.z;
    return (range > 1e-6 ? dot / range : 0.0) + noise(tl_rng);
}

void RadarSignalProcessor_AESA::computeCPA(DetectionOutput& det,
                                            const TargetInput& target,
                                            double range) const
{
    det.cpa_distance = range; det.time_to_cpa = 0.0;
    double v2 = target.vx*target.vx + target.vy*target.vy + target.vz*target.vz;
    if (v2 > 0.01)
    {
        double t = -(target.x*target.vx + target.y*target.vy + target.z*target.vz) / v2;
        det.time_to_cpa = std::max(0.0, t);
        double cx = target.x + target.vx*det.time_to_cpa;
        double cy = target.y + target.vy*det.time_to_cpa;
        double cz = target.z + target.vz*det.time_to_cpa;
        det.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
    }
}

double RadarSignalProcessor_AESA::computeAlbersheimPd(double snr_linear,
                                                       double Pfa, int N,
                                                       SwerlingCase sc) const
{
    if (snr_linear <= 0.0 || N < 1) return 0.0;

    double snr_dB = 10.0 * std::log10(snr_linear);
    double A = std::log(0.62 / std::max(1e-15, Pfa));
    double Nf = static_cast<double>(std::max(1, N));

    double swerlingLoss_dB = 0.0;
    switch (sc)
    {
    case SwerlingCase::CASE_I:   case SwerlingCase::CASE_II:  swerlingLoss_dB = 5.72; break;
    case SwerlingCase::CASE_III: case SwerlingCase::CASE_IV:  swerlingLoss_dB = 2.36; break;
    default: break;
    }
    double effectiveSNR = snr_dB - swerlingLoss_dB;

    double pdLow = 0.001, pdHigh = 0.999;
    for (int iter = 0; iter < 40; ++iter)
    {
        double pdMid = 0.5 * (pdLow + pdHigh);
        double B     = std::log(pdMid / (1.0 - pdMid));
        double snrReq = -5.0 * std::log10(Nf)
                      + (6.2 + 4.54 / std::sqrt(Nf + 0.44))
                        * std::log10(A + 0.12 * A * B + 1.7 * B);
        if (snrReq < effectiveSNR) pdLow  = pdMid;
        else                       pdHigh = pdMid;
    }
    return std::clamp(0.5 * (pdLow + pdHigh), 0.0, 0.99);
}

double RadarSignalProcessor_AESA::computePk(double sinr_linear, double Pfa,
                                             int N, SwerlingCase sc) const
{
    return computeAlbersheimPd(sinr_linear, Pfa, N, sc);
}

// =============================================================================
// §F  Range ambiguity
// =============================================================================

double RadarSignalProcessor_AESA::resolveRangeAmbiguity(double measured,
                                                         double predicted,
                                                         double Rmax) const
{
    if (Rmax < 1.0) return measured;
    double best = measured, minErr = 1e12;
    for (int k = -5; k <= 5; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}
void RadarSignalProcessor_AESA::applyRangeAmbiguity(
    DetectionOutput& det, double range,
    double Rmax, double Rmax2,
    std::normal_distribution<double>& noise) const
{
    // First PRF folded measurement (always computed)
    double noisy1;
    if (range > Rmax)
    { noisy1 = std::fmod(range, Rmax) + noise(tl_rng); det.isAmbiguous = true; }
    else
    { noisy1 = range + noise(tl_rng); det.isAmbiguous = false; }

    if (Rmax2 > 1.0)
    {
        // Second PRF folded measurement — independent noise sample
        double noisy2 = (range > Rmax2)
                            ? std::fmod(range, Rmax2) + noise(tl_rng)
                            : range + noise(tl_rng);
        det.range      = resolveRangeAmbiguityStaggered(noisy1, noisy2,
                                                   Rmax, Rmax2, range);
        det.isAmbiguous = false;   // resolved by coincidence detector
    }
    else
    {
        det.range = noisy1;
        // isAmbiguous already set above
    }
}
// void RadarSignalProcessor_AESA::applyRangeAmbiguity(
//     DetectionOutput& det, double range, double Rmax,
//     std::normal_distribution<double>& noise) const
// {
//     if (range > Rmax)
//     { det.range = std::fmod(range, Rmax) + noise(tl_rng); det.isAmbiguous = true; }
//     else
//     { det.range = range + noise(tl_rng); det.isAmbiguous = false; }
// }
// ADD after the closing brace of resolveRangeAmbiguity():

double RadarSignalProcessor_AESA::resolveRangeAmbiguityStaggered(
    double measured1, double measured2,
    double Rmax1,     double Rmax2,
    double predicted) const
{
    // Coincidence detector: find n1, n2 in [0,4] such that
    //   measured1 + n1*Rmax1  ≈  measured2 + n2*Rmax2
    // The matching candidate is the true unambiguous range.
    // 500 m agreement gate — tighter than RANGE_GATE to avoid false resolves.
    double bestCand = measured1, bestErr = 1e12;
    for (int n1 = 0; n1 <= 4; ++n1)
    {
        double r1 = measured1 + static_cast<double>(n1) * Rmax1;
        if (r1 < 0.0) continue;
        for (int n2 = 0; n2 <= 4; ++n2)
        {
            double r2 = measured2 + static_cast<double>(n2) * Rmax2;
            if (r2 < 0.0) continue;
            if (std::abs(r1 - r2) < 500.0)   // 500 m coincidence gate
            {
                double cand = 0.5 * (r1 + r2);
                double err  = std::abs(cand - predicted);
                if (err < bestErr) { bestErr = err; bestCand = cand; }
            }
        }
    }
    return bestCand;
}

double RadarSignalProcessor_AESA::resolveVelocityStaggered(
    double foldedVel1, double foldedVel2,
    double Vmax1,      double Vmax2,
    double predictedVel) const
{
    // Each PRF has an unambiguous velocity interval [0, Vmax].
    // Fold both measured velocities into their respective [0, Vmax] windows,
    // then find n1, n2 such that foldedVel1 + n1*Vmax1 ≈ foldedVel2 + n2*Vmax2.
    // 2.0 m/s agreement gate — one Doppler bin width at typical fighter PRF.
    auto fold = [](double v, double Vmax) -> double {
        if (Vmax < 1.0) return v;
        v = std::fmod(v, Vmax);
        if (v < 0.0) v += Vmax;
        return v;
    };
    double v1f = fold(foldedVel1, Vmax1);
    double v2f = fold(foldedVel2, Vmax2);

    double bestCand = foldedVel1, bestErr = 1e12;
    for (int n1 = 0; n1 < 8; ++n1)
    {
        double c1 = v1f + static_cast<double>(n1) * Vmax1;
        for (int n2 = 0; n2 < 8; ++n2)
        {
            double c2 = v2f + static_cast<double>(n2) * Vmax2;
            if (std::abs(c1 - c2) < 2.0)   // 2 m/s coincidence gate
            {
                double cand = 0.5 * (c1 + c2);
                double err  = std::abs(cand - predictedVel);
                if (err < bestErr) { bestErr = err; bestCand = cand; }
            }
        }
    }
    return bestCand;
}
void RadarSignalProcessor_AESA::resolveRangeForLockOn(
    DetectionOutput& det, double range, double Rmax,
    uint32_t targetId, const std::vector<TrackFile>& db) const
{
    double predicted = range;
    for (const auto& t : db)
        if (t.id == targetId) { predicted = t.predictedRange; break; }
    det.range       = resolveRangeAmbiguity(det.range, predicted, Rmax);
    det.isAmbiguous = false;
}


double RadarSignalProcessor_AESA::getPlatformBaseRCS(
    const std::string& platformType) const
{
    // Median RCS values in m² — from open literature
    if (platformType == "FIGHTER")  return 3.0;
    if (platformType == "BOMBER")   return 40.0;
    if (platformType == "UAV")      return 0.01;
    if (platformType == "MISSILE")  return 0.1;
    if (platformType == "HELO")     return 3.0;
    if (platformType == "SHIP")     return 10000.0;
    if (platformType == "STEALTH")  return 0.001;
    return 5.0; // GENERIC
}

double RadarSignalProcessor_AESA::lookupAspectRCS(
    const TargetInput& target, double aspectAngle_deg) const
{
    // If no table, use platform type base RCS
    if (target.rcsTable.empty())
        return getPlatformBaseRCS(target.platformType);

    // Linear interpolation between table entries
    const auto& tbl = target.rcsTable;
    if (aspectAngle_deg <= tbl.front().first) return tbl.front().second;
    if (aspectAngle_deg >= tbl.back().first)  return tbl.back().second;

    for (size_t i = 1; i < tbl.size(); ++i)
    {
        if (aspectAngle_deg <= tbl[i].first)
        {
            double t = (aspectAngle_deg - tbl[i-1].first)
            / (tbl[i].first - tbl[i-1].first);
            return tbl[i-1].second + t * (tbl[i].second - tbl[i-1].second);
        }
    }
    return getPlatformBaseRCS(target.platformType);
}
// =============================================================================
// §G  Max detection range
// =============================================================================

double RadarSignalProcessor_AESA::computeMaxDetectionRange(double rcs,
                                                            const RadarConfig& cfg) const
{
    double lam  = SPEED_OF_LIGHT / cfg.frequency_Hz;
    int    active = std::max(0, cfg.numElements - cfg.failedModules);
    double Pt   = static_cast<double>(active)
                * static_cast<double>(cfg.peakPowerPerElement_W)
                * static_cast<double>(cfg.moduleEfficiency);
    double G    = std::pow(10.0, static_cast<double>(cfg.antennaGain) / 10.0);
    double Pn   = computeNoisePower(cfg,
                    static_cast<double>(cfg.searchWaveform.bandwidth_Hz));

    double N     = 16.0;
    double alpha = N * (std::pow(cfg.targetPfa, -1.0 / N) - 1.0);
    double pg    = computeModulationProcessingGain(cfg.searchWaveform);
    double ig    = static_cast<double>(std::max(1, cfg.searchWaveform.pulsesPerDwell));


    // double R_est = 200000.0;
    // for (int iter = 0; iter < 6; ++iter)
    // {
    //     double prop = computePropagationLoss(R_est, cfg);
    //     double num  = Pt * prop * prop * G * G * lam * lam * rcs * pg * ig;
    //     double den  = std::pow(4.0 * M_PI, 3.0) * Pn * alpha;
    //     if (den <= 0.0) break;
    //     R_est = std::pow(num / den, 0.25);
    // }
    // CHANGE TO:
    double R_est = 200000.0;
    double R_prev = 0.0;
    for (int iter = 0; iter < 20; ++iter)
    {
        R_prev = R_est;
        double prop = computePropagationLoss(R_est, cfg);
        double num  = Pt * prop * prop * G * G * lam * lam * rcs * pg * ig;
        double den  = std::pow(4.0 * M_PI, 3.0) * Pn * alpha;
        if (den <= 0.0) break;
        R_est = std::pow(num / den, 0.25);
        if (std::abs(R_est - R_prev) < 10.0) break;
    }
    return std::max(R_est / 1000.0, cfg.minDetectableRange / 1000.0 * 2.0);
}

// =============================================================================
// §H  Detection merge guard
// =============================================================================

bool RadarSignalProcessor_AESA::shouldMergeDetection(
    const DetectionOutput& det,
    const std::vector<DetectionOutput>& existing,
    const RadarConfig& cfg) const
{
    for (const auto& ex : existing)
    {
        double azDiff = std::abs(ex.azimuth - det.azimuth);
        if (azDiff > 180.0) azDiff = 360.0 - azDiff;
        if (std::abs(ex.range - det.range) < MERGE_GATE &&
            azDiff < static_cast<double>(cfg.beamWidth) &&
            std::abs(ex.elevation - det.elevation) < static_cast<double>(cfg.beamWidth))
            return true;
    }
    return false;
}

// =============================================================================
// §I  Beam gain + FIX-11 sidelobe blanking
// =============================================================================

double RadarSignalProcessor_AESA::computeBeamGainFactor(
    double azDiff, double elDiff,
    const RadarConfig& cfg, double effectiveBeamWidth) const
{
    double bw = (effectiveBeamWidth > 0.0) ? effectiveBeamWidth
                                           : static_cast<double>(cfg.beamWidth);
    //if (azDiff <= bw / 2.0 && elDiff <= bw / 2.0) return 1.0;
    if (azDiff <= bw * 2.0 && elDiff <= bw * 2.0) return 1.0;

    float peakSL, avgSL;
    switch (cfg.sidelobeMode)
    {
    case SidelobeMode::LOW_SLL:   peakSL = -45.0f; avgSL = -55.0f; break;
    case SidelobeMode::ULTRA_LOW: peakSL = -55.0f; avgSL = -65.0f; break;
    default:                      peakSL = cfg.peakSidelobeLevel; avgSL = cfg.avgSidelobeLevel; break;
    }

    double dB = (azDiff <= bw * 2.0 && elDiff <= bw * 2.0)
                    ? static_cast<double>(peakSL) : static_cast<double>(avgSL);
    return std::pow(10.0, dB / 10.0);
}

bool RadarSignalProcessor_AESA::isJammerInSidelobe(double azDiff, double elDiff,
                                                    const TargetInput& target,
                                                    const RadarConfig& cfg) const
{
    if (!target.jammer.active) return false;
    double halfBW = static_cast<double>(cfg.beamWidth) / 2.0;
    if (azDiff <= halfBW && elDiff <= halfBW) return false; // in main beam

    if (target.jammer.power_kW <= 0.0) return false;

    double Pj  = target.jammer.power_kW * 1000.0;
    double Gj  = std::pow(10.0, target.jammer.gain_dBi / 10.0);
    double lam = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double Rj  = target.jammer.selfScreening ? 1.0
               : std::max(1.0, target.jammer.range_m);
    double Pr  = (Pj * Gj * lam * lam) / (std::pow(4.0*M_PI,2.0) * Rj*Rj);

    double Pn  = computeNoisePower(cfg, cfg.antennaBandwidth);
    if (Pr <= 0.0 || Pn <= 0.0) return false;

    double excessdB = 10.0 * std::log10(Pr / Pn);
    return excessdB > static_cast<double>(cfg.sidelobeBlanking_dB);
}

// =============================================================================
// §J  Modulation processing gain
// =============================================================================

double RadarSignalProcessor_AESA::computeModulationProcessingGain(
    const BeamWaveform& wf) const
{
    switch (wf.modulation)
    {
    case ModulationType::LFM:
    case ModulationType::NLFM:
    case ModulationType::FMCW:
        return std::max(1.0, static_cast<double>(wf.bandwidth_Hz) *
                             static_cast<double>(wf.pulseWidth_s));
    default: return 1.0;
    }
}

// =============================================================================
// FIX-01  Doppler clutter notch
// =============================================================================
std::pair<double,double> RadarSignalProcessor_AESA::computeClutterNotch(
    const RadarConfig& cfg, const BeamWaveform& wf) const
{
    // Clutter from ground returns at approximately platform speed (m/s)
    // A target is blind only if its radial velocity = clutter velocity ± notch width
    // Notch half-width derived from waveform: λ×PRF/2 divided by dwell pulses
    // gives the minimum resolvable Doppler bin width
    double lambda     = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double prf        = static_cast<double>(wf.prf_Hz);
    double N          = static_cast<double>(std::max(1, wf.pulsesPerDwell));
    double notchWidth = (lambda * prf) / (2.0 * N);   // m/s — Doppler bin width
    notchWidth        = std::clamp(notchWidth, 1.0, 15.0); // physical bounds

    double Vclutter = static_cast<double>(cfg.platformSpeed_m_s);
    return { Vclutter - notchWidth, Vclutter + notchWidth };
}
// std::pair<double,double> RadarSignalProcessor_AESA::computeClutterNotch(
//     const RadarConfig& cfg, const BeamWaveform& /*wf*/) const
// {
//     double Vp = static_cast<double>(cfg.platformSpeed_m_s);
//     return { -Vp, +Vp };
// }

// bool RadarSignalProcessor_AESA::isInDopplerBlindZone(double radVel_m_s,
//                                                       const RadarConfig& cfg,
//                                                       const BeamWaveform& wf) const
// {
//     if (wf.mode == WaveformMode::HPRF) return false;
//     auto [lo, hi] = computeClutterNotch(cfg, wf);
//     return (radVel_m_s >= lo && radVel_m_s <= hi);
// }
bool RadarSignalProcessor_AESA::isInDopplerBlindZone(double radVel_m_s,
                                                     const RadarConfig& cfg,
                                                     const BeamWaveform& wf) const
{
    if (wf.mode == WaveformMode::HPRF) return false;
    if (cfg.platformSpeed_m_s < 1.0f) return false;  // ADD THIS LINE
    auto [lo, hi] = computeClutterNotch(cfg, wf);
    return (radVel_m_s >= lo && radVel_m_s <= hi);
}
// =============================================================================
// FIX-02  Monopulse angle error
// =============================================================================

void RadarSignalProcessor_AESA::computeMonopulseAngleError(
    double azDiff_deg, double elDiff_deg, double sinr,
    const RadarConfig& cfg,
    double& outAzError_deg, double& outElError_deg) const
{
    double bw  = static_cast<double>(cfg.beamWidth);
    double km  = 1.606;
    double sig = (sinr > 0.0) ? bw / (km * std::sqrt(2.0 * sinr)) : bw;

    thread_local std::normal_distribution<double> nd(0.0, 1.0);
    outAzError_deg = azDiff_deg / (km * km) + sig * nd(tl_rng);
    outElError_deg = elDiff_deg / (km * km) + sig * nd(tl_rng);
}

// =============================================================================
// FIX-06  Waveform selection
// =============================================================================

BeamWaveform RadarSignalProcessor_AESA::selectWaveformForRange(
    double range_m, const RadarConfig& cfg) const
{
    for (const auto& entry : cfg.waveformTable)
    {
        if (entry.maxRange_m <= 0.0f) break;
        if (range_m < static_cast<double>(entry.maxRange_m))
            return entry.waveform;
    }
    return cfg.searchWaveform;
}

// =============================================================================
// FIX-09  Two-ray multipath
// =============================================================================

double RadarSignalProcessor_AESA::computeMultipathFactor(
    double range_m, double elevation_deg,
    double targetHeight_m, const RadarConfig& cfg) const
{
    if (elevation_deg > 5.0 || targetHeight_m <= 0.0 || range_m < 1.0) return 1.0;

    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double dphi   = (4.0 * M_PI * cfg.radarHeight * targetHeight_m) / (lambda * range_m);
    double factor = 4.0 * std::pow(std::sin(dphi / 2.0), 2.0);
    return std::clamp(factor, 0.0, 4.0);
}

// =============================================================================
// FIX-10  Chaff return
// =============================================================================

double RadarSignalProcessor_AESA::computeChaffReturn(
    double beamAz, double beamEl,
    const std::vector<ChaffCloud>& clouds,
    double simTime, const RadarConfig& cfg) const
{
    double total = 0.0;
    for (const auto& cloud : clouds)
    {
        double range = std::sqrt(cloud.x*cloud.x + cloud.y*cloud.y + cloud.z*cloud.z);
        if (range < 1.0) continue;

        double cAz = std::atan2(cloud.y, cloud.x) * (180.0/M_PI);
        if (cAz < 0.0) cAz += 360.0;
        double cEl = std::asin(std::clamp(cloud.z/range,-1.0,1.0)) * (180.0/M_PI);

        double dAz = std::abs(beamAz - cAz);
        if (dAz > 180.0) dAz = 360.0 - dAz;
        double dEl = std::abs(beamEl - cEl);
        if (dAz > static_cast<double>(cfg.beamWidth)*3.0 ||
            dEl > static_cast<double>(cfg.beamWidth)*3.0) continue;

        double age    = simTime - cloud.birthTime_s;
        double rcsNow = cloud.rcsTotal * std::exp(-age / std::max(1.0, cloud.decayTime_s));

        double lam  = SPEED_OF_LIGHT / cfg.frequency_Hz;
        int    act  = std::max(0, cfg.numElements - cfg.failedModules);
        double Pt   = static_cast<double>(act)
                    * static_cast<double>(cfg.peakPowerPerElement_W)
                    * static_cast<double>(cfg.moduleEfficiency);
        double G    = std::pow(10.0, static_cast<double>(cfg.antennaGain)/10.0);
        double Pc   = (Pt*G*G*lam*lam*rcsNow)
                    / (std::pow(4.0*M_PI,3.0)*std::pow(range,4.0));
        total += Pc * computePropagationLoss(range, cfg);
    }
    return total;
}

} // namespace aesa
