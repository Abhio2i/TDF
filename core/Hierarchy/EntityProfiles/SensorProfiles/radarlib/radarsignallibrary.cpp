// =============================================================================
// radarsignallibrary.cpp  —  Signal intercept accumulation and library matching
//
// All methods are called from RadarModel::update() which already holds
// the master mutex — no additional locking needed here.
// =============================================================================

#include "radarsignallibrary.h"
#include <cmath>
#include <algorithm>

// =============================================================================
// Lifecycle
// =============================================================================

void RadarSignalLibrary::clear()
{
    accumulator_.clear();
    lastSeen_.clear();
}

void RadarSignalLibrary::loadLibrary(const std::vector<SignalLibraryEntry>& entries)
{
    library_ = entries;
}

// =============================================================================
// Per-detection accumulation
// =============================================================================

SignalIntercept RadarSignalLibrary::accumulate(
    uint32_t           targetID,
    double             receivedPower,
    double             simTime,
    const RadarConfig& cfg)
{
    // Get or create accumulator entry
    auto& si    = accumulator_[targetID];
    si.targetID = targetID;

    // ------------------------------------------------------------------
    // Frequency measurement
    // Freq agility: the received frequency is within the hop window.
    // When agility is off, it is simply cfg.frequency_Hz.
    // ------------------------------------------------------------------
    double measuredFreq = cfg.frequency_Hz;
    if (cfg.frequencyAgility &&
        cfg.hopStopFrequency > cfg.hopStartFrequency &&
        cfg.hopStopFrequency > 0.0f)
    {
        // The hopped centre frequency: base + step * rate (same formula as
        // radarsignalprocessor calculateSignalStrength uses)
        measuredFreq += static_cast<double>(cfg.hopStepFrequency * cfg.hopRate);
        // Clamp into the declared hop band
        measuredFreq = std::clamp(measuredFreq,
                                  static_cast<double>(cfg.hopStartFrequency),
                                  static_cast<double>(cfg.hopStopFrequency));
    }

    // Running average (incremental)
    si.frequency_Hz = (si.freqCount == 0)
                          ? measuredFreq
                          : (si.frequency_Hz * si.freqCount + measuredFreq) / (si.freqCount + 1);
    si.freqCount++;

    // ------------------------------------------------------------------
    // PRI measurement  (1 / active PRF)
    // Use prfLevels[0] — the library accumulator is not PRF-mode-aware;
    // a multi-PRF waveform will show a blended average, which is correct
    // behaviour for an intercept receiver.
    // ------------------------------------------------------------------
    double measuredPRI = (cfg.prfLevels[0] > 0.0f)
                             ? 1.0 / static_cast<double>(cfg.prfLevels[0]) : 0.0;
    si.pri_s = (si.priCount == 0)
                   ? measuredPRI
                   : (si.pri_s * si.priCount + measuredPRI) / (si.priCount + 1);
    si.priCount++;

    // ------------------------------------------------------------------
    // Pulse width measurement
    // ------------------------------------------------------------------
    double measuredPW = static_cast<double>(cfg.pulseWidth);
    si.pulseWidth_s = (si.pwCount == 0)
                          ? measuredPW
                          : (si.pulseWidth_s * si.pwCount + measuredPW) / (si.pwCount + 1);
    si.pwCount++;

    // ------------------------------------------------------------------
    // Signal level (dBW)
    // ------------------------------------------------------------------
    double powerDBW = (receivedPower > 1e-30)
                          ? 10.0 * std::log10(receivedPower) : -300.0;
    si.signalLevel_dBW = (si.signalDepth == 0)
                             ? powerDBW
                             : (si.signalLevel_dBW * si.signalDepth + powerDBW) / (si.signalDepth + 1);
    si.signalDepth++;

    // ------------------------------------------------------------------
    // Modulation — taken directly from config (operator-known parameter)
    // ------------------------------------------------------------------
    si.modulation = cfg.modulation;

    // ------------------------------------------------------------------
    // Library match — attempted once enough samples are in
    // ------------------------------------------------------------------
    si.emitterID = matchLibrary(si);

    // Update last-seen time for pruning
    lastSeen_[targetID] = simTime;

    return si;
}

// =============================================================================
// Output / maintenance
// =============================================================================

void RadarSignalLibrary::getIntercepts(std::vector<SignalIntercept>& out) const
{
    out.clear();
    out.reserve(accumulator_.size());
    for (const auto& [id, si] : accumulator_)
        out.push_back(si);
}

void RadarSignalLibrary::pruneStale(double simTime, double coastSeconds)
{
    for (auto it = lastSeen_.begin(); it != lastSeen_.end(); )
    {
        if ((simTime - it->second) > coastSeconds)
        {
            accumulator_.erase(it->first);
            it = lastSeen_.erase(it);
        }
        else
            ++it;
    }
}

// =============================================================================
// Library matching (private)
// =============================================================================

std::string RadarSignalLibrary::matchLibrary(const SignalIntercept& si) const
{
    // Require a minimum number of measurements in each dimension before
    // attempting a match — avoids false positives on first detection.
    if (si.freqCount < 3) return si.emitterID; // keep previous match

    for (const auto& entry : library_)
    {
        // --- Frequency ---
        if (std::abs(si.frequency_Hz - entry.frequency_Hz) > entry.freqTolerance_Hz)
            continue;

        // --- PRI (skip check when entry.pri_s == 0 → don't care) ---
        if (entry.pri_s > 0.0 && si.priCount >= 3 &&
            std::abs(si.pri_s - entry.pri_s) > entry.priTolerance_s)
            continue;

        // --- Pulse width (skip check when entry.pulseWidth_s == 0) ---
        if (entry.pulseWidth_s > 0.0 && si.pwCount >= 3 &&
            std::abs(si.pulseWidth_s - entry.pulseWidth_s) > entry.pwTolerance_s)
            continue;

        // --- Modulation (skip check when entry.modulation == NONE) ---
        if (entry.modulation != ModulationType::NONE &&
            si.modulation    != entry.modulation)
            continue;

        // All active criteria passed
        return entry.emitterID;
    }

    // No match — retain previous match if one existed (don't clear a
    // confirmed ID just because a single measurement drifted outside the gate)
    return si.emitterID;
}
