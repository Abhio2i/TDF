
// =============================================================================
// radarantenna.cpp  —  Antenna scan state machine implementation
//
// All original scan patterns (MECHANICAL, CONICAL, full-360) are preserved
// exactly.  Added:
//   • Dwell timer: beam holds at bar endpoints for cfg.scanDwellTime[0/1] ms
//   • RASTER:   bar-by-bar azimuth sweep with elevation step after each bar
//   • HELICAL:  continuous az rotation with slow el ramp
//   • LISSAJOUS: az + el driven by sinusoids at different rates
// =============================================================================

#include "radarantenna.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Public interface
// =============================================================================

void RadarAntenna::reset(const RadarConfig& cfg)
{
    currentAzimuth_         = static_cast<double>(cfg.minAzimuth);
    currentElevation_       = computeElevation(cfg);
    scanDirection_          = 1.0;
    previousAzimuth_        = currentAzimuth_;
    scanBoundaryOccurred_   = false;

    // Dwell state
    dwellTimer_             = 0.0;
    dwellAtMax_             = false;

    // Raster state — start at top bar
    rasterBarIndex_         = 0;
    rasterCurrentElevation_ = static_cast<double>(cfg.maxElevation);
}

// =============================================================================
// §A  Scanning update
// =============================================================================

void RadarAntenna::update(double dt, const RadarConfig& cfg)
{
    scanBoundaryOccurred_ = false;
    previousAzimuth_      = currentAzimuth_;

    // ------------------------------------------------------------------
    // Dwell timer — freeze azimuth advance while dwelling at an endpoint.
    // elevation and other state are still updated normally.
    // ------------------------------------------------------------------
    if (dwellTimer_ > 0.0)
    {
        dwellTimer_ -= dt;
        if (dwellTimer_ < 0.0) dwellTimer_ = 0.0;
        currentElevation_ = computeElevation(cfg);
        return; // Do not advance azimuth during dwell
    }

    // Scan speed in degrees per second
    double rotSpeed = (static_cast<double>(cfg.scanningRate_RPM) / 60.0) * 360.0;

    switch (cfg.scanType)
    {
    // ------------------------------------------------------------------
    // MECHANICAL  (original logic, unchanged)
    // ------------------------------------------------------------------
    case ScanType::MECHANICAL:
    {
        bool full360 = (cfg.minAzimuth <= -180.0f && cfg.maxAzimuth >= 180.0f);
        if (full360)
        {
            currentAzimuth_ += rotSpeed * dt;
            if (currentAzimuth_ >= 360.0)
                currentAzimuth_ -= 360.0;
        }
        else
        {
            currentAzimuth_ += scanDirection_ * rotSpeed * dt;

            if (currentAzimuth_ > static_cast<double>(cfg.maxAzimuth))
            {
                currentAzimuth_ = static_cast<double>(cfg.maxAzimuth);
                scanDirection_  = -1.0;
                // Start dwell at max-az endpoint if configured
                if (cfg.scanDwellTime[1] > 0.0f)
                {
                    dwellTimer_  = static_cast<double>(cfg.scanDwellTime[1]) * 1e-3;
                    dwellAtMax_  = true;
                }
            }
            if (currentAzimuth_ < static_cast<double>(cfg.minAzimuth))
            {
                currentAzimuth_ = static_cast<double>(cfg.minAzimuth);
                scanDirection_  =  1.0;
                if (cfg.scanDwellTime[0] > 0.0f)
                {
                    dwellTimer_  = static_cast<double>(cfg.scanDwellTime[0]) * 1e-3;
                    dwellAtMax_  = false;
                }
            }
        }
        break;
    }

    // ------------------------------------------------------------------
    // CONICAL  (original logic, unchanged)
    // ------------------------------------------------------------------
    case ScanType::CONICAL:
    {
        bool full360 = (cfg.minAzimuth <= -180.0f && cfg.maxAzimuth >= 180.0f);
        if (full360)
        {
            currentAzimuth_ += rotSpeed * dt;
            if (currentAzimuth_ >= 360.0)
                currentAzimuth_ -= 360.0;
        }
        else
        {
            currentAzimuth_ += scanDirection_ * rotSpeed * dt;
            if (currentAzimuth_ > static_cast<double>(cfg.maxAzimuth))
            {
                currentAzimuth_ = static_cast<double>(cfg.maxAzimuth);
                scanDirection_  = -1.0;
                if (cfg.scanDwellTime[1] > 0.0f)
                    dwellTimer_ = static_cast<double>(cfg.scanDwellTime[1]) * 1e-3;
            }
            if (currentAzimuth_ < static_cast<double>(cfg.minAzimuth))
            {
                currentAzimuth_ = static_cast<double>(cfg.minAzimuth);
                scanDirection_  =  1.0;
                if (cfg.scanDwellTime[0] > 0.0f)
                    dwellTimer_ = static_cast<double>(cfg.scanDwellTime[0]) * 1e-3;
            }
        }
        break;
    }

    // ------------------------------------------------------------------
    // RASTER  —  bar-by-bar azimuth sweep + elevation step per bar
    // ------------------------------------------------------------------
    case ScanType::RASTER:
    {
        currentAzimuth_ += scanDirection_ * rotSpeed * dt;

        if (currentAzimuth_ > static_cast<double>(cfg.maxAzimuth))
        {
            currentAzimuth_ = static_cast<double>(cfg.maxAzimuth);
            scanDirection_  = -1.0;

            // Dwell at right end of this bar
            if (cfg.scanDwellTime[1] > 0.0f)
                dwellTimer_ = static_cast<double>(cfg.scanDwellTime[1]) * 1e-3;

            // Step elevation after each right-to-left reversal
            advanceRasterElevation(cfg);
            scanBoundaryOccurred_ = true;
        }
        if (currentAzimuth_ < static_cast<double>(cfg.minAzimuth))
        {
            currentAzimuth_ = static_cast<double>(cfg.minAzimuth);
            scanDirection_  =  1.0;

            if (cfg.scanDwellTime[0] > 0.0f)
                dwellTimer_ = static_cast<double>(cfg.scanDwellTime[0]) * 1e-3;

            scanBoundaryOccurred_ = true;
        }
        // Elevation is managed by rasterCurrentElevation_ — don't call
        // computeElevation() which would overwrite it.
        currentElevation_ = rasterCurrentElevation_;
        break;
    }

    // ------------------------------------------------------------------
    // HELICAL  —  continuous az rotation, slow el ramp
    // The beam completes one full elevation traverse per cfg.minHitsToValidate
    // azimuth revolutions (we repurpose minHitsToValidate as a multiplier
    // here; a dedicated helicalBars config field can be added if needed).
    // ------------------------------------------------------------------
    case ScanType::HELICAL:
    {
        currentAzimuth_ += rotSpeed * dt;
        if (currentAzimuth_ >= 360.0)
        {
            currentAzimuth_ -= 360.0;
            scanBoundaryOccurred_ = true;
        }
        // Elevation ramps from min → max based on fractional az position
        // One full el sweep per revolution (linear ramp)
        double phase = currentAzimuth_ / 360.0; // [0, 1)
        currentElevation_ = static_cast<double>(cfg.minElevation)
                            + phase * static_cast<double>(cfg.maxElevation - cfg.minElevation);
        break;
    }

    // ------------------------------------------------------------------
    // LISSAJOUS  —  az + el driven by sinusoids at 1:2 frequency ratio
    // Azimuth oscillates between min/max; elevation at twice the rate.
    // ------------------------------------------------------------------
    case ScanType::LISSAJOUS:
    {
        currentAzimuth_ += scanDirection_ * rotSpeed * dt;

        if (currentAzimuth_ > static_cast<double>(cfg.maxAzimuth))
        {
            currentAzimuth_ = static_cast<double>(cfg.maxAzimuth);
            scanDirection_  = -1.0;
            scanBoundaryOccurred_ = true;
        }
        if (currentAzimuth_ < static_cast<double>(cfg.minAzimuth))
        {
            currentAzimuth_ = static_cast<double>(cfg.minAzimuth);
            scanDirection_  =  1.0;
        }
        // Elevation: sinusoid at 2× az sweep rate mapped to [min, max]
        double azRange  = static_cast<double>(cfg.maxAzimuth - cfg.minAzimuth);
        double azNorm   = (azRange > 0.0)
                            ? (currentAzimuth_ - cfg.minAzimuth) / azRange : 0.0;
        double centEl   = (static_cast<double>(cfg.maxElevation)
                         + static_cast<double>(cfg.minElevation)) / 2.0;
        double elAmp    = (static_cast<double>(cfg.maxElevation)
                        - static_cast<double>(cfg.minElevation)) / 2.0;
        currentElevation_ = centEl + elAmp * std::sin(2.0 * M_PI * 2.0 * azNorm);
        break;
    }

    } // end switch

    // For scan types that don't set elevation themselves, use computeElevation()
    if (cfg.scanType == ScanType::MECHANICAL || cfg.scanType == ScanType::CONICAL)
        currentElevation_ = computeElevation(cfg);

    // Boundary detection (MECHANICAL / CONICAL — RASTER/LISSAJOUS set their own)
    if (cfg.scanType == ScanType::MECHANICAL || cfg.scanType == ScanType::CONICAL)
        scanBoundaryOccurred_ = detectScanBoundary(previousAzimuth_,
                                                   currentAzimuth_, cfg);
}

// =============================================================================
// §B  Lock-on update  (original logic, unchanged)
// =============================================================================

bool RadarAntenna::lockOn(
    const std::vector<TargetInput>& worldInputs,
    const RadarConfig& cfg)
{
    scanBoundaryOccurred_ = false;

    for (const auto& t : worldInputs)
    {
        if (t.id != cfg.lockedTargetID) continue;

        double range = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
        if (range < 1e-6) continue;

        currentAzimuth_ = std::atan2(t.y, t.x) * (180.0 / M_PI);
        if (currentAzimuth_ < 0.0) currentAzimuth_ += 360.0;

        double ratio        = std::clamp(t.z / range, -1.0, 1.0);
        currentElevation_   = std::asin(ratio) * (180.0 / M_PI);

        return true;
    }
    return false;
}

// =============================================================================
// §C  Private helpers
// =============================================================================

bool RadarAntenna::detectScanBoundary(
    double prevAz, double newAz, const RadarConfig& cfg) const
{
    bool full360 = (cfg.minAzimuth <= -180.0f && cfg.maxAzimuth >= 180.0f);
    if (full360)
        return (newAz - prevAz) < -180.0;

    return (newAz <= static_cast<double>(cfg.minAzimuth) + 0.1 ||
            newAz >= static_cast<double>(cfg.maxAzimuth) - 0.1);
}

double RadarAntenna::computeElevation(const RadarConfig& cfg) const
{
    double centEl = (static_cast<double>(cfg.minElevation) +
                     static_cast<double>(cfg.maxElevation)) / 2.0;

    if (cfg.scanType == ScanType::CONICAL)
    {
        return centEl + std::sin(currentAzimuth_ * M_PI / 180.0)
        * (static_cast<double>(cfg.beamWidth) / 4.0);
    }

    // MECHANICAL (default): fixed centre elevation
    return centEl;
}

void RadarAntenna::advanceRasterElevation(const RadarConfig& cfg)
{
    double beamStep = static_cast<double>(cfg.beamWidth);
    double elMin    = static_cast<double>(cfg.minElevation);
    double elMax    = static_cast<double>(cfg.maxElevation);

    rasterCurrentElevation_ -= beamStep;

    // Wrap: after bottom bar, restart from top
    if (rasterCurrentElevation_ < elMin)
    {
        rasterBarIndex_         = 0;
        rasterCurrentElevation_ = elMax;
    }
    else
    {
        rasterBarIndex_++;
    }
}// // =============================================================================
// // radarantenna.cpp  —  Antenna scan state machine implementation
// //
// // All scan geometry lives here.  The only state this class owns is the
// // current beam pointing angle and the scan direction flag — nothing else.
// //
// // A new scan pattern (phased-array raster, stacked-beam, …) is added by:
// //   1. Adding an entry to ScanType enum in radarmodel.h
// //   2. Adding a branch in update() / computeElevation() here
// //   No other file needs to change.
// // =============================================================================

// #include "radarantenna.h"

// #include <cmath>
// #include <algorithm>

// #ifndef M_PI
// #define M_PI 3.14159265358979323846
// #endif

// // =============================================================================
// // Public interface
// // =============================================================================

// void RadarAntenna::reset(const RadarConfig& cfg)
// {
//     // Start scan at the leftmost azimuth limit
//     currentAzimuth_       = static_cast<double>(cfg.minAzimuth);
//     currentElevation_     = computeElevation(cfg);
//     scanDirection_        = 1.0;
//     previousAzimuth_      = currentAzimuth_;
//     scanBoundaryOccurred_ = false;
// }

// // =============================================================================
// // §A  Scanning update
// // =============================================================================

// void RadarAntenna::update(double dt, const RadarConfig& cfg)
// {
//     // Clear the one-shot boundary flag at the start of each tick
//     scanBoundaryOccurred_ = false;
//     previousAzimuth_      = currentAzimuth_;

//     // Scan speed in degrees per second
//     double rotSpeed = (static_cast<double>(cfg.scanningRate_RPM) / 60.0) * 360.0;

//     // Determine whether this is a full 360° rotation or a limited sector scan.
//     // Full rotation: minAzimuth ≤ −180 AND maxAzimuth ≥ 180
//     bool full360 = (cfg.minAzimuth <= -180.0f && cfg.maxAzimuth >= 180.0f);

//     if (full360)
//     {
//         // ---- Full rotation — no reversal, just wrap at 360° ----
//         currentAzimuth_ += rotSpeed * dt;
//         if (currentAzimuth_ >= 360.0)
//             currentAzimuth_ -= 360.0;
//     }
//     else
//     {
//         // ---- Sector scan — reverse direction at limits ----
//         currentAzimuth_ += scanDirection_ * rotSpeed * dt;

//         if (currentAzimuth_ > static_cast<double>(cfg.maxAzimuth))
//         {
//             currentAzimuth_ = static_cast<double>(cfg.maxAzimuth);
//             scanDirection_  = -1.0; // reverse
//         }
//         if (currentAzimuth_ < static_cast<double>(cfg.minAzimuth))
//         {
//             currentAzimuth_ = static_cast<double>(cfg.minAzimuth);
//             scanDirection_  =  1.0; // reverse
//         }
//     }

//     // Update elevation for the new azimuth position
//     currentElevation_ = computeElevation(cfg);

//     // Detect boundary crossing (triggers scan-miss logic in the tracker)
//     scanBoundaryOccurred_ = detectScanBoundary(previousAzimuth_, currentAzimuth_, cfg);
// }

// // =============================================================================
// // §B  Lock-on update
// // =============================================================================

// bool RadarAntenna::lockOn(
//     const std::vector<TargetInput>& worldInputs,
//     const RadarConfig& cfg)
// {
//     // Original radarmodel.cpp set scanBoundaryOccurred_ = false explicitly
//     // before calling updateAntennaLockOn().  Replicate that here so a stale
//     // true value from a previous TWS scan cannot leak into LOCK_ON mode and
//     // incorrectly trigger tracker scan-miss logic.
//     scanBoundaryOccurred_ = false;

//     // Search the input list for the locked target
//     for (const auto& t : worldInputs)
//     {
//         if (t.id != cfg.lockedTargetID) continue;

//         double range = std::sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
//         if (range < 1e-6) continue; // Degenerate — skip

//         // Azimuth: atan2 in the horizontal (x-y) plane, wrapped to [0, 360)
//         currentAzimuth_   = std::atan2(t.y, t.x) * (180.0 / M_PI);
//         if (currentAzimuth_ < 0.0) currentAzimuth_ += 360.0;

//         // Elevation: asin of the z/range ratio (z = North/altitude component)
//         double ratio      = std::clamp(t.z / range, -1.0, 1.0);
//         currentElevation_ = std::asin(ratio) * (180.0 / M_PI);

//         return true; // Target found — beam updated
//     }

//     // Target not found in this tick's input list — signal lock-broken
//     return false;
// }

// // =============================================================================
// // §C  Private helpers
// // =============================================================================

// bool RadarAntenna::detectScanBoundary(
//     double prevAz, double newAz, const RadarConfig& cfg) const
// {
//     bool full360 = (cfg.minAzimuth <= -180.0f && cfg.maxAzimuth >= 180.0f);

//     if (full360)
//     {
//         // Full rotation: boundary is when the azimuth wraps (large negative step)
//         return (newAz - prevAz) < -180.0;
//     }

//     // Sector scan: boundary is when the beam is at (or very near) a limit.
//     // The 0.1° tolerance handles floating-point overshoot at the endpoints.
//     return (newAz <= static_cast<double>(cfg.minAzimuth) + 0.1 ||
//             newAz >= static_cast<double>(cfg.maxAzimuth) - 0.1);
// }

// double RadarAntenna::computeElevation(const RadarConfig& cfg) const
// {
//     // Centre elevation is the midpoint of the configured elevation range
//     double centEl = (static_cast<double>(cfg.minElevation) +
//                      static_cast<double>(cfg.maxElevation)) / 2.0;

//     if (cfg.scanType == ScanType::CONICAL)
//     {
//         // Conical scan: sinusoidal elevation modulation at ¼ beam-width amplitude.
//         // The phase is driven by the current azimuth so the nod follows the sweep.
//         return centEl + std::sin(currentAzimuth_ * M_PI / 180.0)
//                             * (static_cast<double>(cfg.beamWidth) / 4.0);
//     }

//     // MECHANICAL (default): fixed centre elevation for the whole sweep
//     return centEl;
// }
