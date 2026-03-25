#pragma once
// =============================================================================
// radarsignallibrary.h  —  Signal intercept accumulation and library matching
//
// Responsibility (4th sub-object, owned by RadarModel):
//   Per detected emitter it:
//     • Accumulates measured signal parameters (freq, PRI, PW, mod, level)
//     • Maintains running-average counters: priCount, pwCount, freqCount,
//       signalDepth (consecutive detections above threshold)
//     • Matches the accumulated profile against a loaded SignalLibraryEntry
//       table and writes the emitterID back into the SignalIntercept record
//     • Prunes stale entries at each scan boundary
//
// Design rules:
//   1. Owns only accumulator_ map + library_ table.
//   2. NO MUTEX — only called from RadarModel::update() under the master lock.
//   3. NO Qt types, no engine types.
//   4. radar.cpp never sees this header; RadarModel forward-declares it.
// =============================================================================

#ifndef RADARSIGNALLIBRARY_H
#define RADARSIGNALLIBRARY_H

#include "radarmodel.h"
#include <unordered_map>
#include <vector>

class RadarSignalLibrary
{
public:
    RadarSignalLibrary() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// Discard all accumulated intercepts.
    /// Called by RadarModel::start() and reset().
    void clear();

    /// Replace the loaded emitter reference library.
    /// Call before the mission starts; can be reloaded mid-mission via setConfig.
    void loadLibrary(const std::vector<SignalLibraryEntry>& entries);

    // -------------------------------------------------------------------------
    // Per-detection accumulation
    // -------------------------------------------------------------------------

    /// Update the intercept accumulator for targetID and return the current
    /// SignalIntercept snapshot (with emitterID filled if library match found).
    ///
    /// @param targetID      Target identifier
    /// @param receivedPower Received signal power in Watts (from signal chain)
    /// @param simTime       Current simulation clock (seconds)
    /// @param cfg           Current radar config (freq, PRF, PW, modulation)
    SignalIntercept accumulate(uint32_t           targetID,
                               double             receivedPower,
                               double             simTime,
                               const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // Output / maintenance
    // -------------------------------------------------------------------------

    /// Fill out with a snapshot of all active intercept records.
    void getIntercepts(std::vector<SignalIntercept>& out) const;

    /// Remove entries whose last-seen time is older than (simTime - coastSeconds).
    /// Call once per scan boundary alongside tracker::applyScanMissLogic().
    void pruneStale(double simTime, double coastSeconds);

private:
    // -------------------------------------------------------------------------
    // Library matching
    // -------------------------------------------------------------------------

    /// Compare si against every SignalLibraryEntry.
    /// Returns the first matching emitterID, or "" if no match.
    /// Matching requires at least 3 measurements in each parameter.
    std::string matchLibrary(const SignalIntercept& si) const;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /// Running-average intercept per targetID
    std::unordered_map<uint32_t, SignalIntercept> accumulator_;

    /// Last-seen simulation time per targetID (for pruning)
    std::unordered_map<uint32_t, double> lastSeen_;

    /// Loaded emitter reference table
    std::vector<SignalLibraryEntry> library_;
};

#endif // RADARSIGNALLIBRARY_H
