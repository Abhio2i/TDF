#pragma once
// =============================================================================
// radarantenna.h  —  Antenna scan state machine
//
// Responsibility:
//   Owns and advances the current beam pointing (azimuth + elevation) each
//   tick.  Handles:
//     • Mechanical sector scan  (back-and-forth between minAzimuth/maxAzimuth)
//     • Full 360° rotation      (when minAzimuth ≤ −180 && maxAzimuth ≥ 180)
//     • Conical scan            (sinusoidal elevation modulation)
//     • Raster scan             (bar-by-bar: az sweep + el step per bar)
//     • Helical scan            (az rotates, el ramps slowly)
//     • Lissajous scan          (az + el at different sinusoidal rates)
//     • LOCK_ON slaving         (beam slaves to the target's current position)
//     • Scan-boundary detection (triggers tracker miss logic)
//     • Dwell timer             (beam holds at bar endpoints for scanDwellTime)
//
// Design rules (unchanged):
//   1. Owns ONLY scan-angle state.
//   2. NO MUTEX.
//   3. NO Qt types, no engine types.
//   4. Adding a new pattern = add a branch in update() / computeElevation().
//
// Change log vs previous version:
//   + dwellTimer_     — counts down during endpoint dwell (seconds)
//   + dwellAtMax_     — true when dwelling at max-az endpoint (else min-az)
//   + rasterBarIndex_ — which elevation bar the raster scan is on
//   + advanceRasterElevation() private helper
// =============================================================================

#ifndef RADARANTENNA_H
#define RADARANTENNA_H

#include "radarmodel.h"
#include <vector>

class RadarAntenna
{
public:
    RadarAntenna() = default;

    /// Reset scan state to starting position.
    void reset(const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // §A  Scanning update  (SURVEILLANCE and TWS)
    // -------------------------------------------------------------------------
    void update(double dt, const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // §B  Lock-on update  (LOCK_ON mode)
    // -------------------------------------------------------------------------
    bool lockOn(const std::vector<TargetInput>& worldInputs,
                const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // §C  Read-only state accessors (unchanged)
    // -------------------------------------------------------------------------
    double currentAzimuth()       const { return currentAzimuth_;       }
    double currentElevation()     const { return currentElevation_;      }
    bool   scanBoundaryOccurred() const { return scanBoundaryOccurred_;  }

private:
    // ---- Core scan state (unchanged) ------------------------------------
    double currentAzimuth_        = 0.0;
    double currentElevation_      = 0.0;
    double scanDirection_         = 1.0;
    double previousAzimuth_       = 0.0;
    bool   scanBoundaryOccurred_  = false;

    // ---- Dwell timer state ----------------------------------------------
    double dwellTimer_            = 0.0;   ///< Seconds remaining in current dwell
    bool   dwellAtMax_            = false; ///< true = max-az endpoint, false = min-az

    // ---- Raster / multi-bar scan state ----------------------------------
    int    rasterBarIndex_        = 0;     ///< Current elevation bar index
    double rasterCurrentElevation_= 0.0;  ///< Raster bar centre elevation

    // ---- Private helpers ------------------------------------------------
    bool   detectScanBoundary(double prevAz, double newAz,
                            const RadarConfig& cfg) const;

    double computeElevation(const RadarConfig& cfg) const;

    /// Step raster elevation by one beam width; wrap at max → min.
    void   advanceRasterElevation(const RadarConfig& cfg);
};

#endif // RADARANTENNA_H
// #pragma once
// // =============================================================================
// // radarantenna.h  —  Antenna scan state machine
// //
// // Responsibility:
// //   Owns and advances the current beam pointing (azimuth + elevation) each
// //   tick.  Handles:
// //     • Mechanical sector scan  (back-and-forth between minAzimuth/maxAzimuth)
// //     • Full 360° rotation      (when minAzimuth ≤ −180 && maxAzimuth ≥ 180)
// //     • Conical scan            (sinusoidal elevation modulation)
// //     • LOCK_ON slaving         (beam slaves to the target's current position)
// //     • Scan-boundary detection (used by tracker to trigger miss logic)
// //
// // Design rules:
// //   1. Owns ONLY scan-angle state.  No config copy — receives const RadarConfig&
// //      on every call so the orchestrator's config_ is always authoritative.
// //   2. NO MUTEX — only called from RadarModel::update() which holds the lock.
// //   3. NO Qt types, no engine types.
// //   4. Adding a new scan pattern (phased-array raster, Lissajous, …) means
// //      adding a method here and a branch in update().  Nothing else changes.
// // =============================================================================

// #ifndef RADARANTENNA_H
// #define RADARANTENNA_H

// #include "radarmodel.h"
// #include <vector>

// class RadarAntenna
// {
// public:
//     // -------------------------------------------------------------------------
//     // Construction
//     // -------------------------------------------------------------------------

//     RadarAntenna() = default;

//     /// Reset scan state to starting position defined in cfg.
//     /// Called by RadarModel::start() and RadarModel::reset().
//     void reset(const RadarConfig& cfg);

//     // -------------------------------------------------------------------------
//     // §A  Scanning update  (SURVEILLANCE and TWS modes)
//     // -------------------------------------------------------------------------

//     /// Advance the scan angle by dt seconds according to cfg.scanningRate_RPM.
//     /// Sets the internal scanBoundaryOccurred_ flag when the beam reverses
//     /// direction or completes a full 360° revolution.
//     ///
//     /// @param dt   Tick duration (seconds)
//     /// @param cfg  Current radar config (scan limits, rate, beam width, type)
//     void update(double dt, const RadarConfig& cfg);

//     // -------------------------------------------------------------------------
//     // §B  Lock-on update  (LOCK_ON mode)
//     // -------------------------------------------------------------------------

//     /// Slave the beam to the locked target's current position.
//     /// Searches worldInputs for cfg.lockedTargetID and points the beam there.
//     ///
//     /// @param worldInputs  All targets visible this tick (radar-local coords)
//     /// @param cfg          Current radar config (provides lockedTargetID)
//     /// @return True if the locked target was found and beam was updated.
//     ///         False if the target is not in worldInputs (caller interprets
//     ///         this as lock-broken and falls back to SURVEILLANCE).
//     bool lockOn(const std::vector<TargetInput>& worldInputs,
//                 const RadarConfig& cfg);

//     // -------------------------------------------------------------------------
//     // §C  Read-only state accessors
//     // -------------------------------------------------------------------------

//     double currentAzimuth()          const { return currentAzimuth_;          }
//     double currentElevation()        const { return currentElevation_;         }
//     bool   scanBoundaryOccurred()    const { return scanBoundaryOccurred_;     }

// private:
//     // -------------------------------------------------------------------------
//     // Scan state  (the only member data in this class)
//     // -------------------------------------------------------------------------

//     double currentAzimuth_        = 0.0;  ///< degrees
//     double currentElevation_      = 0.0;  ///< degrees
//     double scanDirection_         = 1.0;  ///< +1 = increasing az, −1 = decreasing
//     double previousAzimuth_       = 0.0;  ///< For boundary detection
//     bool   scanBoundaryOccurred_  = false;///< Set each tick; cleared by next update()

//     // -------------------------------------------------------------------------
//     // Private helpers
//     // -------------------------------------------------------------------------

//     /// Returns true if the azimuth sweep has crossed a boundary this tick.
//     /// Works for both full-360 (wrap-around) and sector (reversal) modes.
//     bool detectScanBoundary(double prevAz, double newAz,
//                             const RadarConfig& cfg) const;

//     /// Compute the elevation for the current azimuth.
//     /// MECHANICAL: centred between min/max elevation.
//     /// CONICAL:    sinusoidally modulated by ¼ beam width.
//     double computeElevation(const RadarConfig& cfg) const;
// };

// #endif // RADARANTENNA_H
