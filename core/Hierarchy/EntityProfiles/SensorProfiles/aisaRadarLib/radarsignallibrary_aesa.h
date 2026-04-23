// =============================================================================
// FILE:         radarsignallibrary_aesa.h
// MODULE:       AESA Radar Signal Intercept Library (ESM)
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the RadarSignalLibrary_AESA class which implements
//               the Electronic Support Measures (ESM) signal intercept
//               accumulation and emitter identification subsystem.
//
//               Responsibilities:
//                 1. Accumulate frequency, PRI, pulse width, and signal level
//                    measurements across multiple dwells for each detected
//                    emitter, computing running averages. REQ-AESA-040.
//                 2. Suppress accumulation for DRFM ghost detections — ghost
//                    signals must not corrupt the real emitter's parameter
//                    estimates. REQ-AESA-060.
//                 3. Match accumulated signal parameters against a loaded
//                    emitter library to classify the emitter by name.
//                    REQ-AESA-040.
//                 4. Prune stale intercepts whose last-seen time exceeds the
//                    configured coast period. REQ-AESA-030.
//                 5. Publish the current intercept list to the radar output
//                    at each scan boundary. REQ-AESA-004.
//
// REQUIREMENTS: REQ-AESA-004  Output assembly — intercept publication
//               REQ-AESA-030  Stale intercept pruning
//               REQ-AESA-040  ESM signal accumulation and classification
//               REQ-AESA-060  DRFM ghost suppression in ESM
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-LIB-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic intercept accumulation.
//   Rev 2  15 Feb 2026  Emitter library matching added. REQ-AESA-040.
//   Rev 3  01 Apr 2026  FIX-03: DRFM ghost suppression added. REQ-AESA-060.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARSIGNALLIBRARY_AESA_H
#define RADARSIGNALLIBRARY_AESA_H

#include "radarmodel_aesa.h"
#include <unordered_map>

namespace aesa {

// =============================================================================
// CLASS: RadarSignalLibrary_AESA
//
// DESCRIPTION:  Electronic Support Measures (ESM) signal intercept library.
//               Maintains a per-target accumulator of measured radar emission
//               parameters. Over multiple dwells, the running averages converge
//               to stable estimates which are then matched against a loaded
//               emitter identification library.
//
//               Internal storage:
//                 acc_      — per-target SignalIntercept accumulators.
//                             Key = TargetInput::id.
//                 lastSeen_ — per-target last detection timestamp (seconds).
//                             Key = TargetInput::id.
//                 lib_      — loaded emitter identification library entries.
//
//               All methods are called from RadarModel_AESA::update() which
//               holds the model mutex. This class is NOT independently
//               thread-safe — do not call from multiple threads without
//               external synchronisation.
//
// REQUIREMENTS: REQ-AESA-004, REQ-AESA-030, REQ-AESA-040, REQ-AESA-060
//
// TRACEABILITY:
//   Test suite:  radarSignalLibrary_test (radarsignallibrary_aesa_test.cpp)
//   Test cases:  TC-AESA-LIB-001 through TC-AESA-LIB-025
// =============================================================================
class RadarSignalLibrary_AESA
{
public:

    // =========================================================================
    // FUNCTION:    clear
    //
    // DESCRIPTION: Clears all accumulated intercept data and timestamps.
    //              Called during radar init(), start(), and end() to ensure
    //              no stale intercepts persist across operational cycles.
    //              Safe to call on an already-empty library. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:  None.
    // RETURNS:     void
    //
    // SIDE EFFECTS: Clears acc_ and lastSeen_. Does NOT clear lib_ —
    //               the emitter identification library persists across
    //               operational cycles and is only replaced by loadLibrary().
    //               REQ-AESA-040.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-001  clear() leaves acc_ empty
    //               TC-AESA-LIB-002  clear() leaves lastSeen_ empty
    //               TC-AESA-LIB-003  clear() does not clear lib_
    // =========================================================================
    void clear();

    // =========================================================================
    // FUNCTION:    loadLibrary
    //
    // DESCRIPTION: Replaces the current emitter identification library with
    //              the supplied entries. The library is used by matchLibrary()
    //              to classify accumulated intercepts by emitter name.
    //              An empty entries vector is valid — clears the library,
    //              disabling emitter identification (intercepts will have
    //              empty emitterID). REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   entries  [in]  Vector of emitter library entries. Copied internally.
    //                  May be empty. No ordering requirement — searched
    //                  linearly by matchLibrary(). REQ-AESA-040.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Replaces lib_. Does NOT affect acc_ or lastSeen_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-004  loadLibrary() replaces lib_ contents
    //               TC-AESA-LIB-005  loadLibrary({}) clears library
    //               TC-AESA-LIB-006  loadLibrary() does not affect acc_
    // =========================================================================
    void loadLibrary(const std::vector<SignalLibraryEntry>& entries);

    // =========================================================================
    // FUNCTION:    accumulate
    //
    // DESCRIPTION: Accumulates one dwell's signal measurement into the
    //              running average intercept record for the specified target.
    //              Updates frequency, PRI, pulse width, signal level, and
    //              modulation type estimates. Attempts emitter library
    //              classification after each update. REQ-AESA-040.
    //
    //              DRFM ghost suppression: if isDRFMGhost = true, the
    //              function returns immediately with an empty SignalIntercept
    //              without modifying any accumulator. Ghost detections must
    //              not contaminate real emitter parameter estimates — a DRFM
    //              jammer deliberately falsifies frequency, PRI, and velocity
    //              to deceive the ESM subsystem. REQ-AESA-060.
    //
    //              Averaging algorithm: running mean using the formula:
    //                new_avg = (old_avg * n + new_value) / (n + 1)
    //              where n is the current observation count. This is
    //              numerically stable and does not require storing all
    //              previous measurements. REQ-AESA-040.
    //
    //              Frequency measurement: if frequency agility is active,
    //              the measured frequency is taken as the midpoint of the
    //              hop band (hopStart + hopStop) / 2. This is the best
    //              estimate available when pulse-to-pulse hopping prevents
    //              single-pulse frequency measurement. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040  Signal accumulation and classification
    //              REQ-AESA-060  DRFM ghost suppression
    //
    // PARAMETERS:
    //   targetID       [in]  ID of the emitting target. Must be non-zero.
    //                        Matches TargetInput::id. REQ-AESA-040.
    //
    //   receivedPower  [in]  Received signal power (Watts). Must be > 0 for
    //                        a valid dBW computation. Values <= 1e-30 are
    //                        treated as below the noise floor and recorded
    //                        as -300 dBW (sentinel for unmeasurable).
    //                        REQ-AESA-040.
    //
    //   simTime        [in]  Current simulation time (seconds). Stored as
    //                        last-seen timestamp for stale pruning. Must be
    //                        monotonically increasing. REQ-AESA-030.
    //
    //   cfg            [in]  Radar configuration. Uses frequency_Hz,
    //                        frequencyAgility, hopStartFrequency,
    //                        hopStopFrequency. REQ-AESA-040.
    //
    //   waveform       [in]  Current beam waveform. Uses prf_Hz, pulseWidth_s,
    //                        modulation. REQ-AESA-040.
    //
    //   isDRFMGhost    [in]  true = this detection is a DRFM ghost — suppress
    //                        accumulation entirely and return empty intercept.
    //                        false = real detection — accumulate normally.
    //                        Default: false. REQ-AESA-060.
    //
    // RETURNS:    Updated SignalIntercept for the target after this dwell.
    //             Returns an empty (default-constructed) SignalIntercept if
    //             isDRFMGhost = true. REQ-AESA-040.
    //
    // SIDE EFFECTS: Modifies acc_[targetID] and lastSeen_[targetID].
    //               First call for a new targetID creates a new accumulator
    //               entry (unordered_map insertion — heap allocation, known
    //               MM-01 deviation ICD-AESA-DEVIATION-002). REQ-AESA-040.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-007  accumulate() DRFM ghost returns empty
    //               TC-AESA-LIB-008  accumulate() DRFM ghost does not modify acc_
    //               TC-AESA-LIB-009  accumulate() real detection creates entry
    //               TC-AESA-LIB-010  accumulate() frequency averaged over 3 dwells
    //               TC-AESA-LIB-011  accumulate() PRI averaged correctly
    //               TC-AESA-LIB-012  accumulate() pulseWidth averaged correctly
    //               TC-AESA-LIB-013  accumulate() signalLevel computed in dBW
    //               TC-AESA-LIB-014  accumulate() low power records -300 dBW
    //               TC-AESA-LIB-015  accumulate() frequency agility uses midpoint
    //               TC-AESA-LIB-016  accumulate() updates lastSeen timestamp
    // =========================================================================
    SignalIntercept accumulate(uint32_t targetID,
                               double receivedPower,
                               double simTime,
                               const RadarConfig& cfg,
                               const BeamWaveform& waveform,
                               bool isDRFMGhost = false);

    // =========================================================================
    // FUNCTION:    getIntercepts
    //
    // DESCRIPTION: Copies all current accumulated intercepts into the output
    //              vector. Called by RadarModel_AESA::update() each tick to
    //              populate latestOutput_.intercepts. The output vector is
    //              cleared before filling — stale output from previous ticks
    //              is replaced. REQ-AESA-004.
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // PARAMETERS:
    //   out  [out]  Vector to receive copies of all current intercepts.
    //               Cleared before filling. May receive zero entries if
    //               no targets have been detected. REQ-AESA-004.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies out. Does not modify acc_ or lastSeen_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-017  getIntercepts() returns empty on clear
    //               TC-AESA-LIB-018  getIntercepts() returns one entry after
    //                                one accumulation
    //               TC-AESA-LIB-019  getIntercepts() clears output before fill
    // =========================================================================
    void getIntercepts(std::vector<SignalIntercept>& out) const;

    // =========================================================================
    // FUNCTION:    pruneStale
    //
    // DESCRIPTION: Removes intercept accumulator entries for targets that have
    //              not been detected for longer than coast seconds. This prevents
    //              the accumulator from growing unboundedly and ensures that
    //              emitters which have departed the sensor coverage area do not
    //              continue to appear in the intercept output. REQ-AESA-030.
    //
    //              An entry is pruned when:
    //                (simTime - lastSeen_[id]) > coast
    //
    //              Both acc_ and lastSeen_ entries are removed atomically
    //              for each pruned target. REQ-AESA-030.
    //
    // REQUIREMENT: REQ-AESA-030
    //
    // PARAMETERS:
    //   simTime      [in]  Current simulation time (seconds). Must be the
    //                      same time base used in accumulate() calls.
    //
    //   coastSeconds [in]  Maximum age of an intercept before pruning (seconds).
    //                      Must be > 0. Typically set to cfg.trackCoastSeconds.
    //                      REQ-AESA-030.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Erases entries from acc_ and lastSeen_ for stale targets.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-020  pruneStale() removes expired entry
    //               TC-AESA-LIB-021  pruneStale() retains recent entry
    //               TC-AESA-LIB-022  pruneStale() removes from both acc_ and
    //                                lastSeen_
    // =========================================================================
    void pruneStale(double simTime, double coastSeconds);

private:

    // =========================================================================
    // FUNCTION:    matchLibrary
    //
    // DESCRIPTION: Attempts to classify a signal intercept by matching its
    //              accumulated parameters against the loaded emitter library.
    //              Returns the emitterID of the first matching library entry,
    //              or the intercept's existing emitterID if no match is found.
    //
    //              Matching is performed only when enough observations have
    //              been accumulated for reliable parameter estimates. The
    //              minimum observation count is 3 for frequency (freqCount).
    //              PRI and pulse width matching are only applied if >= 3
    //              observations have been accumulated for those parameters
    //              AND the library entry specifies non-zero reference values.
    //
    //              Matching rules (all must pass for a match):
    //                1. |si.frequency_Hz - e.frequency_Hz| <= e.freqTolerance_Hz
    //                2. If e.pri_s > 0 and si.priCount >= 3:
    //                   |si.pri_s - e.pri_s| <= e.priTolerance_s
    //                3. If e.pulseWidth_s > 0 and si.pwCount >= 3:
    //                   |si.pulseWidth_s - e.pulseWidth_s| <= e.pwTolerance_s
    //                4. If e.modulation != NONE:
    //                   si.modulation == e.modulation
    //
    //              The library is searched linearly from first to last entry.
    //              The first matching entry wins. REQ-AESA-040.
    //
    // REQUIREMENT: REQ-AESA-040
    //
    // PARAMETERS:
    //   si  [in]  The accumulated signal intercept to classify.
    //             Must have freqCount >= 3 for matching to proceed.
    //
    // RETURNS:    emitterID string of the matching library entry, or
    //             si.emitterID (unchanged) if no match found.
    //
    // SIDE EFFECTS: None. Pure query — does not modify si or lib_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-LIB-023  matchLibrary() returns empty before 3 obs
    //               TC-AESA-LIB-024  matchLibrary() identifies known emitter
    //               TC-AESA-LIB-025  matchLibrary() returns unknown for no match
    // =========================================================================
    std::string matchLibrary(const SignalIntercept& si) const;

    // =========================================================================
    // PRIVATE MEMBER VARIABLES
    // =========================================================================

    // Per-target signal intercept accumulators.
    // Key = TargetInput::id (non-zero). Value = running-average intercept.
    // Populated by accumulate(). Cleared by clear() and pruneStale().
    // REQ-AESA-040.
    std::unordered_map<uint32_t, SignalIntercept> acc_;

    // Per-target last-seen simulation timestamps (seconds).
    // Key = TargetInput::id. Value = simTime of most recent accumulate() call.
    // Used by pruneStale() to determine which entries have gone stale.
    // REQ-AESA-030.
    std::unordered_map<uint32_t, double> lastSeen_;

    // Emitter identification library. Populated by loadLibrary().
    // Searched linearly by matchLibrary(). Not cleared by clear().
    // REQ-AESA-040.
    std::vector<SignalLibraryEntry> lib_;
};

} // namespace aesa

#endif // RADARSIGNALLIBRARY_AESA_H
// #pragma once
// #ifndef RADARSIGNALLIBRARY_AESA_H
// #define RADARSIGNALLIBRARY_AESA_H
// // radarsignallibrary_aesa.h  —  Rev 3
// #include "radarmodel_aesa.h"
// #include <unordered_map>

// namespace aesa {

// class RadarSignalLibrary_AESA
// {
// public:
//     void clear();
//     void loadLibrary(const std::vector<SignalLibraryEntry>& entries);

//     // FIX-03: isDRFMGhost suppresses accumulation for ghost detections
//     SignalIntercept accumulate(uint32_t targetID,
//                                double receivedPower,
//                                double simTime,
//                                const RadarConfig& cfg,
//                                const BeamWaveform& waveform,
//                                bool isDRFMGhost = false);

//     void getIntercepts(std::vector<SignalIntercept>& out) const;
//     void pruneStale(double simTime, double coastSeconds);

// private:
//     std::string matchLibrary(const SignalIntercept& si) const;

//     std::unordered_map<uint32_t, SignalIntercept> acc_;
//     std::unordered_map<uint32_t, double>          lastSeen_;
//     std::vector<SignalLibraryEntry>               lib_;
// };

// } // namespace aesa
// #endif
