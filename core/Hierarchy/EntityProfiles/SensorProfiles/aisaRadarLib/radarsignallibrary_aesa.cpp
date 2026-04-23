// =============================================================================
// FILE:         radarsignallibrary_aesa.cpp
// MODULE:       AESA Radar Signal Intercept Library (ESM) — Implementation
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the RadarSignalLibrary_AESA class. Provides ESM
//               signal intercept accumulation, running-average parameter
//               estimation, emitter library matching, stale entry pruning,
//               and DRFM ghost suppression.
//
//               Design principles:
//                 - No dynamic allocation in the steady-state operational loop
//                   except for unordered_map insertion on first detection of
//                   a new target (known MM-01 deviation, ICD-AESA-DEVIATION-002).
//                 - No recursion (FN-06 compliant).
//                 - No exceptions (FP-01 compliant).
//                 - All lambdas are short, single-expression, and restricted
//                   to the scope of accumulate() (FP-08 advisory deviation —
//                   lambda used for inline averaging formula readability).
//
// REQUIREMENTS: REQ-AESA-004  Output assembly — intercept publication
//               REQ-AESA-030  Stale intercept pruning
//               REQ-AESA-040  ESM signal accumulation and classification
//               REQ-AESA-060  DRFM ghost suppression
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-LIB-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation.
//   Rev 2  15 Feb 2026  Emitter library matching added.
//   Rev 3  01 Apr 2026  FIX-03: DRFM ghost suppression added. REQ-AESA-060.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Magic numbers replaced with named constexpr constants.
//                       Commented-out code removed per NS-05.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radarsignallibrary_aesa.h"
#include <cmath>

// =============================================================================
// FILE-SCOPE NAMED CONSTANTS
//
// All numeric literals used in this translation unit are declared here.
// Satisfies VI-08 (no magic numbers). REQ-AESA-040.
// =============================================================================
namespace
{
// Minimum received power (Watts) for a valid dBW signal level measurement.
// Powers at or below this threshold are at or below the thermal noise floor
// and cannot be meaningfully expressed in dBW. Recorded as FLOOR_DBW.
// REQ-AESA-040.
constexpr double MIN_POWER_W = 1e-30;

// Signal level floor value (dBW). Recorded when receivedPower <= MIN_POWER_W.
// -300 dBW is far below any physically realisable signal and serves as a
// sentinel indicating the measurement is noise-limited. REQ-AESA-040.
constexpr double FLOOR_DBW = -300.0;

// Minimum frequency observations required before emitter library matching
// is attempted. Below this count the frequency estimate is unreliable.
// REQ-AESA-040.
constexpr int    MIN_FREQ_COUNT_FOR_MATCH = 3;

// Minimum PRI observations required before PRI-based library matching
// is applied. REQ-AESA-040.
constexpr int    MIN_PRI_COUNT_FOR_MATCH = 3;

// Minimum pulse width observations required before PW-based matching.
// REQ-AESA-040.
constexpr int    MIN_PW_COUNT_FOR_MATCH = 3;

// Frequency agility midpoint divisor.
// measFreq = (hopStart + hopStop) / AGILITY_MIDPOINT_DIV. REQ-AESA-040.
constexpr double AGILITY_MIDPOINT_DIV = 2.0;

// Minimum hop stop frequency (Hz) for agility midpoint to be valid.
// If hopStopFrequency <= this value, agility midpoint is not used.
// REQ-AESA-040.
constexpr float  MIN_HOP_STOP_HZ = 0.0f;

// Minimum PRF (Hz) for a valid PRI computation.
// If prf_Hz <= 0, PRI is recorded as 0.0 (undefined). REQ-AESA-040.
constexpr float  MIN_PRF_FOR_PRI = 0.0f;

// Reference PRI value threshold in library entry (seconds).
// If e.pri_s <= this value, PRI matching is skipped for that entry.
// REQ-AESA-040.
constexpr double LIBRARY_PRI_MATCH_THRESHOLD = 0.0;

// Reference pulse width threshold in library entry (seconds).
// If e.pulseWidth_s <= this value, PW matching is skipped. REQ-AESA-040.
constexpr double LIBRARY_PW_MATCH_THRESHOLD = 0.0;

} // anonymous namespace

namespace aesa {

// =============================================================================
// SECTION 1: LIFECYCLE
// REQ-AESA-040
// =============================================================================

// =============================================================================
// FUNCTION: clear
// Full description in header.
// =============================================================================
void RadarSignalLibrary_AESA::clear()
{
    // Clear all accumulated intercepts. After this call, getIntercepts()
    // will return an empty vector. REQ-AESA-040.
    acc_.clear();

    // Clear all last-seen timestamps. Required so pruneStale() does not
    // try to prune entries that no longer exist in acc_. REQ-AESA-030.
    lastSeen_.clear();

    // lib_ is intentionally NOT cleared here.
    // The emitter identification library persists across radar init/start
    // cycles and is loaded once via loadLibrary(). Clearing it here would
    // require re-loading after every radar reset. REQ-AESA-040.
}

// =============================================================================
// FUNCTION: loadLibrary
// Full description in header.
// =============================================================================
void RadarSignalLibrary_AESA::loadLibrary(
    const std::vector<SignalLibraryEntry>& entries)
{
    // Replace the entire library with the new entries. Linear assignment.
    // acc_ and lastSeen_ are not affected — existing intercepts continue
    // to be matched against the new library on subsequent calls. REQ-AESA-040.
    lib_ = entries;
}

// =============================================================================
// SECTION 2: ACCUMULATION
// REQ-AESA-040, REQ-AESA-060
// =============================================================================

// =============================================================================
// FUNCTION: accumulate
// Full description in header.
// =============================================================================
SignalIntercept RadarSignalLibrary_AESA::accumulate(
    uint32_t targetID,
    double receivedPower,
    double simTime,
    const RadarConfig& cfg,
    const BeamWaveform& waveform,
    bool isDRFMGhost)
{
    // -------------------------------------------------------------------------
    // DRFM ghost suppression. REQ-AESA-060.
    // Ghost detections must not contaminate real emitter parameter estimates.
    // A DRFM jammer deliberately falsifies frequency, PRI, and velocity to
    // confuse the ESM subsystem. Returning an empty intercept immediately
    // ensures the ghost contributes nothing to acc_. REQ-AESA-060.
    // -------------------------------------------------------------------------
    if (isDRFMGhost) return SignalIntercept{};

    // -------------------------------------------------------------------------
    // Retrieve or create the accumulator entry for this target.
    // First access for a new targetID creates a default-constructed entry
    // (unordered_map insertion — heap allocation, known MM-01 deviation,
    // ICD-AESA-DEVIATION-002). REQ-AESA-040.
    // -------------------------------------------------------------------------
    auto& si      = acc_[targetID];
    si.targetID   = targetID;

    // -------------------------------------------------------------------------
    // Frequency measurement.
    // If frequency agility is active and a valid hop band is configured,
    // use the midpoint of the hop band as the measured frequency.
    // Rationale: pulse-to-pulse frequency hopping prevents measuring the
    // exact instantaneous frequency. The midpoint of the hop band is the
    // best estimate available from the waveform configuration. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double measFreq = cfg.frequency_Hz;

    if (cfg.frequencyAgility &&
        cfg.hopStopFrequency > cfg.hopStartFrequency &&
        cfg.hopStopFrequency > MIN_HOP_STOP_HZ)
    {
        // Agility midpoint: (hopStart + hopStop) / 2. REQ-AESA-040.
        measFreq = (static_cast<double>(cfg.hopStartFrequency)
                    + static_cast<double>(cfg.hopStopFrequency))
                   / AGILITY_MIDPOINT_DIV;
    }

    // -------------------------------------------------------------------------
    // Running average helper.
    // Computes the running mean: new_avg = (old * n + new) / (n + 1).
    // Defined as a local lambda for single-use inline averaging.
    // ADVISORY: FP-08 recommends avoiding lambdas in operational code.
    // This lambda is trivial (single arithmetic expression), captures nothing,
    // and is limited to this function scope. Deviation documented in
    // ICD-AESA-DEVIATION-004. REQ-AESA-040.
    // -------------------------------------------------------------------------
    auto avg = [](double old, double newV, int n) -> double
    {
        return (old * static_cast<double>(n) + newV)
        / static_cast<double>(n + 1);
    };

    // -------------------------------------------------------------------------
    // Accumulate frequency. First observation initialises; subsequent
    // observations are running-averaged. REQ-AESA-040.
    // -------------------------------------------------------------------------
    si.frequency_Hz = (si.freqCount == 0)
                          ? measFreq
                          : avg(si.frequency_Hz, measFreq, si.freqCount);
    si.freqCount++;

    // -------------------------------------------------------------------------
    // Accumulate PRI (pulse repetition interval, seconds).
    // PRI = 1 / prf_Hz. Zero PRF is not physically valid — record PRI as 0.0
    // to indicate the measurement is undefined. REQ-AESA-040.
    // -------------------------------------------------------------------------
    double pri = (waveform.prf_Hz > MIN_PRF_FOR_PRI)
                     ? 1.0 / static_cast<double>(waveform.prf_Hz)
                     : 0.0;

    si.pri_s = (si.priCount == 0)
                   ? pri
                   : avg(si.pri_s, pri, si.priCount);
    si.priCount++;

    // -------------------------------------------------------------------------
    // Accumulate pulse width (seconds). REQ-AESA-040.
    // -------------------------------------------------------------------------
    double pw = static_cast<double>(waveform.pulseWidth_s);

    si.pulseWidth_s = (si.pwCount == 0)
                          ? pw
                          : avg(si.pulseWidth_s, pw, si.pwCount);
    si.pwCount++;

    // -------------------------------------------------------------------------
    // Accumulate signal level (dBW).
    // Powers at or below MIN_POWER_W are noise-floor limited — record the
    // floor sentinel FLOOR_DBW rather than log10(0) which is undefined.
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    double pdBW = (receivedPower > MIN_POWER_W)
                      ? 10.0 * std::log10(receivedPower)
                      : FLOOR_DBW;

    si.signalLevel_dBW = (si.signalDepth == 0)
                             ? pdBW
                             : avg(si.signalLevel_dBW, pdBW, si.signalDepth);
    si.signalDepth++;

    // -------------------------------------------------------------------------
    // Record modulation type from current waveform.
    // Latest observation overwrites previous — modulation type is not averaged.
    // REQ-AESA-040.
    // -------------------------------------------------------------------------
    si.modulation = waveform.modulation;

    // -------------------------------------------------------------------------
    // Attempt emitter library classification.
    // matchLibrary() returns a non-empty emitterID only if freqCount >= 3.
    // The result replaces the intercept's emitterID field. REQ-AESA-040.
    // -------------------------------------------------------------------------
    si.emitterID = matchLibrary(si);

    // -------------------------------------------------------------------------
    // Update last-seen timestamp for stale pruning. REQ-AESA-030.
    // -------------------------------------------------------------------------
    lastSeen_[targetID] = simTime;

    return si;
}

// =============================================================================
// SECTION 3: OUTPUT
// REQ-AESA-004
// =============================================================================

// =============================================================================
// FUNCTION: getIntercepts
// Full description in header.
// =============================================================================
void RadarSignalLibrary_AESA::getIntercepts(
    std::vector<SignalIntercept>& out) const
{
    // Clear output vector before filling — caller must see only current
    // intercepts, not residuals from previous calls. REQ-AESA-004.
    out.clear();

    // Pre-allocate to avoid repeated reallocation. REQ-AESA-004.
    out.reserve(acc_.size());

    // Copy all current accumulator entries into the output vector.
    // Order is unspecified (unordered_map iteration). Consumers must not
    // rely on any particular ordering. REQ-AESA-004.
    for (const auto& [id, si] : acc_)
    {
        out.push_back(si);
    }
}

// =============================================================================
// SECTION 4: STALE PRUNING
// REQ-AESA-030
// =============================================================================

// =============================================================================
// FUNCTION: pruneStale
// Full description in header.
// =============================================================================
void RadarSignalLibrary_AESA::pruneStale(double simTime, double coastSeconds)
{
    // Iterate lastSeen_ and erase entries whose age exceeds coastSeconds.
    // Both acc_ and lastSeen_ entries are removed to keep the two maps
    // consistent — an entry in lastSeen_ without a corresponding acc_ entry
    // (or vice versa) would indicate a data integrity error. REQ-AESA-030.
    for (auto it = lastSeen_.begin(); it != lastSeen_.end(); )
    {
        // Age = current time minus last detection time. REQ-AESA-030.
        double age = simTime - it->second;

        if (age > coastSeconds)
        {
            // Erase the corresponding accumulator entry. REQ-AESA-040.
            acc_.erase(it->first);

            // Erase the lastSeen_ entry and advance the iterator.
            // erase() returns the iterator to the next valid element.
            it = lastSeen_.erase(it);
        }
        else
        {
            // Entry is still fresh — advance iterator without erasing.
            ++it;
        }
    }
}

// =============================================================================
// SECTION 5: EMITTER IDENTIFICATION
// REQ-AESA-040
// =============================================================================

// =============================================================================
// FUNCTION: matchLibrary
// Full description in header.
// =============================================================================
std::string RadarSignalLibrary_AESA::matchLibrary(
    const SignalIntercept& si) const
{
    // Require minimum frequency observations before attempting classification.
    // Below this count the frequency estimate is too noisy to be reliable.
    // Return the existing emitterID unchanged (empty string on first few dwells).
    // REQ-AESA-040.
    if (si.freqCount < MIN_FREQ_COUNT_FOR_MATCH) return si.emitterID;

    // Linear search through the library. First matching entry wins. REQ-AESA-040.
    for (const auto& e : lib_)
    {
        // ---- Rule 1: Frequency match (mandatory for all entries) -----------
        // Reject if measured frequency is outside the entry's tolerance band.
        // REQ-AESA-040.
        if (std::abs(si.frequency_Hz - e.frequency_Hz) > e.freqTolerance_Hz)
        {
            continue;
        }

        // ---- Rule 2: PRI match (optional — only if library entry specifies)
        // Apply PRI matching only when:
        //   - The library entry has a non-zero reference PRI (e.pri_s > 0).
        //   - We have accumulated enough PRI observations (priCount >= 3).
        // REQ-AESA-040.
        if (e.pri_s > LIBRARY_PRI_MATCH_THRESHOLD &&
            si.priCount >= MIN_PRI_COUNT_FOR_MATCH)
        {
            if (std::abs(si.pri_s - e.pri_s) > e.priTolerance_s)
            {
                continue;
            }
        }

        // ---- Rule 3: Pulse width match (optional) ---------------------------
        // Apply PW matching only when:
        //   - The library entry has a non-zero reference PW (e.pulseWidth_s > 0).
        //   - We have accumulated enough PW observations (pwCount >= 3).
        // REQ-AESA-040.
        if (e.pulseWidth_s > LIBRARY_PW_MATCH_THRESHOLD &&
            si.pwCount >= MIN_PW_COUNT_FOR_MATCH)
        {
            if (std::abs(si.pulseWidth_s - e.pulseWidth_s) > e.pwTolerance_s)
            {
                continue;
            }
        }

        // ---- Rule 4: Modulation type match (optional) ----------------------
        // Apply modulation matching only when the library entry specifies
        // a non-NONE modulation type. NONE means "match any modulation".
        // REQ-AESA-040.
        if (e.modulation != ModulationType::NONE &&
            si.modulation != e.modulation)
        {
            continue;
        }

        // All applicable rules passed — this entry matches. Return emitter ID.
        // REQ-AESA-040.
        return e.emitterID;
    }

    // No library entry matched — return the existing emitterID unchanged.
    // This preserves any previously assigned ID and returns an empty string
    // for a new, unclassified emitter. REQ-AESA-040.
    return si.emitterID;
}

} // namespace aesa
