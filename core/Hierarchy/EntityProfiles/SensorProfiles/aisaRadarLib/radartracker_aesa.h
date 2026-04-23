// =============================================================================
// FILE:         radartracker_aesa.h
// MODULE:       AESA Radar Track Management
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares the RadarTracker_AESA class which implements
//               multi-target track management for an Active Electronically
//               Scanned Array (AESA) radar model. Provides Kalman-filter
//               prediction, nearest-neighbour (NN) and Joint Probabilistic
//               Data Association (JPDA) measurement-to-track association,
//               Interacting Multiple Model (IMM) manoeuvre handling, scan-miss
//               pruning, track output assembly, and beam request generation.
//
//               All methods operate on an internal track database (db_)
//               represented as a std::vector<TrackFile>. No external I/O,
//               no dynamic memory beyond STL containers, no recursion.
//
// REQUIREMENTS: REQ-AESA-020  Track initialisation and lifecycle management
//               REQ-AESA-021  Kalman filter prediction (CV model + IMM)
//               REQ-AESA-022  Measurement-to-track association (NN and JPDA)
//               REQ-AESA-023  Track update — Joseph-form Kalman and JPDA
//               REQ-AESA-024  IMM manoeuvre detection and model mixing
//               REQ-AESA-025  Scan-miss and track coast / deletion logic
//               REQ-AESA-026  Track output and beam request generation
//               REQ-AESA-027  External track injection (Link-16 / CEC)
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-TRACKER-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic NN association,
//                       constant-velocity Kalman filter.
//   Rev 2  15 Feb 2026  FIX-05: Full JPDA simultaneous update added.
//                       FIX-12: External track injection (Link-16 / CEC).
//   Rev 3  01 Apr 2026  IMM predict/update integrated. Range ambiguity
//                       resolution added. Adaptive track interval added.
//                       Joseph-form covariance update. JPDA spread-of-
//                       innovations term added. IMM Bayesian weight update
//                       moved to measurement update step.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//                       Commented-out code removed per NS-05. Magic numbers
//                       replaced with named constexpr constants in .cpp.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#pragma once
#ifndef RADARTRACKER_AESA_H
#define RADARTRACKER_AESA_H

#include "radarmodel_aesa.h"
#include <vector>

namespace aesa {

// =============================================================================
// CLASS: RadarTracker_AESA
//
// DESCRIPTION:  Implements multi-target track management for the AESA radar
//               simulation. Maintains an internal database of TrackFile
//               objects, each representing one tracked entity with its full
//               6-DOF Kalman state (position + velocity), covariance,
//               process noise, measurement noise, IMM model states, and
//               track quality metadata.
//
//               The processing pipeline per radar scan cycle is:
//                 1. predict()                — propagate all tracks by dt
//                 2. performJPDAUpdate() or   — associate + update tracks
//                    performKalmanUpdate()       (JPDA preferred, NN fallback)
//                 3. createNewTrack()         — initialise unmatched detections
//                 4. applyScanMissLogic()     — prune coasted / dropped tracks
//                 5. getValidatedTracks()     — assemble output for consumers
//                 6. generateTrackBeamRequests() — request track-mode dwells
//
// REQUIREMENTS: REQ-AESA-020 through REQ-AESA-027
//
// THREAD SAFETY: Not thread-safe. The owning RadarModel_AESA serialises
//                all access via its internal mutex. Do not call from multiple
//                threads without external synchronisation.
//
// TRACEABILITY:
//   Test suite:  test_radartracker_aesa (radartracker_aesa_test.cpp)
//   Test cases:  TC-AESA-TRK-001 through TC-AESA-TRK-NNN
// =============================================================================
class RadarTracker_AESA
{
public:

    // =========================================================================
    // CONSTRUCTOR: RadarTracker_AESA (default)
    //
    // DESCRIPTION: Default constructor. The track database db_ is empty on
    //              construction. Call clear() before reuse across radar
    //              init/start cycles to guarantee a clean database state.
    //
    // REQUIREMENT: REQ-AESA-020
    // =========================================================================
    RadarTracker_AESA() = default;

    // =========================================================================
    // FUNCTION:    clear
    //
    // DESCRIPTION: Removes all track entries from the internal database and
    //              reserves capacity for 2048 entries to avoid repeated
    //              reallocation during normal operation. Must be called
    //              during radar initialisation and restart.
    //
    // REQUIREMENT: REQ-AESA-020
    //
    // PARAMETERS:  None.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Clears db_. Reserves 2048 slots in db_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-001  clear() empties the track database
    //               TC-AESA-TRK-002  clear() post-condition: db_.size() == 0
    // =========================================================================
    void clear();

    // =========================================================================
    // FUNCTION:    predict
    //
    // DESCRIPTION: Propagates all tracks forward by dt seconds using either
    //              the IMM predictor (for tracks with hitCount >= 2) or a
    //              simple constant-velocity (CV) predictor (for single-hit
    //              tentative tracks). After prediction, each track's position
    //              scalars (x, y, z, vx, vy, vz) are synchronised from the
    //              state vector X, and predictedRange is recomputed.
    //              The isUpdated flag is cleared to false for all tracks,
    //              ready for the association step.
    //
    //              IMM self-initialises on first call (immActive == false):
    //              both model states and covariances are copied from tr.X /
    //              tr.P, then immActive is set to true.
    //
    // REQUIREMENT: REQ-AESA-021  Kalman filter prediction
    //              REQ-AESA-024  IMM manoeuvre model propagation
    //
    // PARAMETERS:
    //   dt  [in]  Prediction interval in seconds. Must be positive and finite.
    //             Non-positive dt produces no meaningful prediction — caller
    //             must validate before calling. Range: (0.0, 10.0] seconds
    //             for normal operation.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies tr.X, tr.P, tr.x, tr.y, tr.z, tr.vx, tr.vy,
    //               tr.vz, tr.predictedRange, tr.isUpdated for every track
    //               in db_. For IMM-active tracks also modifies tr.imm_X,
    //               tr.imm_P, tr.imm_mu.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-010  predict() advances position by vx*dt
    //               TC-AESA-TRK-011  predict() clears isUpdated on all tracks
    //               TC-AESA-TRK-012  predict() recomputes predictedRange
    //               TC-AESA-TRK-013  IMM self-initialises on first predict
    // =========================================================================
    void predict(double dt);

    // =========================================================================
    // FUNCTION:    findBestTrackMatch
    //
    // DESCRIPTION: Nearest-neighbour (NN) gated association. Searches the
    //              track database for the single best match to a given
    //              detection, using a two-stage gate: a range gate
    //              (RANGE_GATE = 5000 m) followed by a chi-squared gate
    //              (CHI2_GATE_99 = 9.21, equivalent to 99% probability
    //              ellipsoid in 3-D). Returns a pointer to the best matching
    //              TrackFile and the associated Gaussian probability.
    //
    //              Tracks with non-finite state or already updated this cycle
    //              (isUpdated == true) are skipped. Range ambiguity is
    //              resolved before gating via resolveRangeAmbiguity().
    //
    //              Used as the fallback association path when useJPDA is false.
    //              For multi-target dense environments, prefer performJPDAUpdate().
    //
    // REQUIREMENT: REQ-AESA-022  Measurement-to-track association
    //
    // PARAMETERS:
    //   det                  [in]   Detection to associate. Must contain
    //                               finite range, azimuth, elevation.
    //   maxUnambiguousRange  [in]   PRF-dependent maximum unambiguous range
    //                               (metres). Used by resolveRangeAmbiguity().
    //                               Pass 0.0 or negative to disable ambiguity
    //                               resolution (raw measured range used).
    //   outBestProb          [out]  Gaussian association probability of the
    //                               returned track. Set to 0.0 if no match
    //                               found. Range: [0.0, 1.0].
    //
    // RETURNS:    Pointer to the best matching TrackFile in db_, or nullptr
    //             if no track passes both gates.
    //             The returned pointer is valid only until the next call that
    //             modifies db_ (e.g. createNewTrack, applyScanMissLogic).
    //
    // SIDE EFFECTS: None. Read-only access to db_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-020  NN returns nullptr on empty database
    //               TC-AESA-TRK-021  NN returns nullptr when det outside gate
    //               TC-AESA-TRK-022  NN returns correct track when det in gate
    //               TC-AESA-TRK-023  NN skips already-updated tracks
    // =========================================================================
    TrackFile* findBestTrackMatch(const DetectionOutput& det,
                                  double maxUnambiguousRange,
                                  double& outBestProb);

    // =========================================================================
    // FUNCTION:    performKalmanUpdate
    //
    // DESCRIPTION: Applies a single-target Joseph-form Kalman update to one
    //              track using one detection. Implements the following steps:
    //
    //              1. IMM Bayesian weight update — per-model likelihoods are
    //                 computed from the pre-update model states (imm_X, imm_P)
    //                 and measurement z, then imm_mu is updated. This step
    //                 must occur before tr.X is overwritten by the Kalman
    //                 correction to use the genuine predicted states.
    //              2. Innovation computation and manoeuvre detection.
    //              3. Innovation covariance S = P(0:3,0:3) + R. Inversion via
    //                 invertS3(). If singular, hitCount is decremented and
    //                 the function returns without updating.
    //              4. Kalman gain K = P * H^T * S^{-1}  (H = [I_3 | 0_3]).
    //              5. State update:  X += K * y.
    //              6. Joseph-form covariance: P = (I-KH)*P*(I-KH)^T + K*R*K^T.
    //              7. Velocity clamping: vx, vy clamped to ±maxTrackSpeed;
    //                 vz clamped to ±200 m/s.
    //              8. IMM model state sync — imm_X[m] overwritten with the
    //                 corrected X so the next IMM predict does not restore
    //                 the stale pre-update position.
    //              9. Track metadata update: hitCount, isValidated, quality.
    //
    // REQUIREMENT: REQ-AESA-023  Kalman measurement update
    //              REQ-AESA-024  IMM weight update at measurement time
    //
    // PARAMETERS:
    //   track                [in/out]  TrackFile to update. Must be a valid
    //                                  reference into db_ (or a copy thereof).
    //   det                  [in]      Detection to fuse. Must contain finite
    //                                  range, azimuth, elevation, radialVelocity.
    //   simTime              [in]      Current simulation time (seconds).
    //                                  Stored in tr.lastSeenTime.
    //   dt                   [in]      Time since last prediction (seconds).
    //                                  Reserved for future use — currently
    //                                  unused in the update equations.
    //   maxUnambiguousRange  [in]      PRF-dependent unambiguous range (metres).
    //   cfg                  [in]      Radar configuration. Key fields:
    //                                    cfg.manoeuvreThreshold_m — innovation
    //                                      magnitude above which isManoeuvring
    //                                      is set true.
    //                                    cfg.maxTrackSpeed — vx/vy clamp limit.
    //                                    cfg.minHitsToValidate — validation gate.
    //                                    cfg.missedScansToDrop — quality denom.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies track.X, track.P, track.x/y/z, track.vx/vy/vz,
    //               track.range, track.velocity, track.hitCount, track.isUpdated,
    //               track.updatedThisScan, track.lastSeenTime, track.missCount,
    //               track.isManoeuvring, track.innovationMagnitude,
    //               track.isValidated, track.trackQuality, track.wasAmbiguous,
    //               track.imm_X (if immActive), track.imm_mu (if immActive),
    //               track.Q[3..5][3..5].
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-030  Kalman update advances hitCount
    //               TC-AESA-TRK-031  Kalman update sets isUpdated true
    //               TC-AESA-TRK-032  State converges toward measurement
    //               TC-AESA-TRK-033  Singular S skips update, decrements hit
    // =========================================================================
    void performKalmanUpdate(TrackFile& track,
                             const DetectionOutput& det,
                             double simTime,
                             double dt,
                             double maxUnambiguousRange,
                             const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    performJPDAUpdate
    //
    // DESCRIPTION: Joint Probabilistic Data Association (JPDA) simultaneous
    //              update for all tracks against all detections in one call.
    //              Implements the Bar-Shalom formulation:
    //
    //              For each track i:
    //                1. Compute Gaussian likelihoods e_ij for all gated
    //                   detections j (same two-stage gate as NN).
    //                2. Compute marginal association probabilities:
    //                     β₀  = 1 / (1 + Σⱼ e_ij)   (no-detection prob)
    //                     βⱼ  = e_ij / (1 + Σⱼ e_ij) (detection j prob)
    //                     Pc  = 1 - β₀               (detection prob)
    //                3. Combined innovation:  ν_c = Σⱼ βⱼ · (zⱼ - Ĥ·X)
    //                4. State update:          X  += K · ν_c
    //                5. Covariance:
    //                     P = β₀·P_pred + Pc·P_KF + K·P̃·K^T
    //                   where P̃ is the spread-of-innovations matrix.
    //                6. Velocity clamping, metadata update (same as
    //                   performKalmanUpdate).
    //
    //              If no detection is gated to a track (Pc < 1e-9) that
    //              track is skipped — its covariance grows on the next
    //              predict() call.
    //
    //              False alarm density λ_FA = cfg.jpdaFalseAlarmDensity,
    //              floored at 1e-12 to prevent division by zero.
    //
    // REQUIREMENT: REQ-AESA-022  JPDA data association
    //              REQ-AESA-023  JPDA covariance update
    //
    // PARAMETERS:
    //   detections           [in]  All detections from the current scan.
    //                              May be empty — function returns immediately.
    //                              Each detection must contain finite range,
    //                              azimuth, elevation.
    //   simTime              [in]  Current simulation time (seconds).
    //   maxUnambiguousRange  [in]  PRF-dependent unambiguous range (metres).
    //   cfg                  [in]  Radar configuration. Key fields same as
    //                              performKalmanUpdate, plus:
    //                                cfg.jpdaFalseAlarmDensity — λ_FA.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies all fields listed under performKalmanUpdate
    //               for every track that has at least one gated detection.
    //               Tracks with no gated detections are not modified.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-040  JPDA returns early on empty detections
    //               TC-AESA-TRK-041  JPDA skips track with non-finite state
    //               TC-AESA-TRK-042  JPDA updates track with single detection
    //               TC-AESA-TRK-043  JPDA skips track when Pc < threshold
    // =========================================================================
    void performJPDAUpdate(const std::vector<DetectionOutput>& detections,
                           double simTime,
                           double maxUnambiguousRange,
                           const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    createNewTrack
    //
    // DESCRIPTION: Initialises a new TrackFile for a detection that could not
    //              be associated with any existing track. The initial state
    //              vector is built from the detection's spherical coordinates
    //              (range, azimuth, elevation) converted to Cartesian, with
    //              velocity taken from the TargetInput ground-truth (simulation
    //              use only — operational builds replace this with Doppler).
    //
    //              Duplicate guard: if a track with the same id (det.targetID)
    //              already exists in db_, no new track is created and the
    //              function returns immediately.
    //
    //              Capacity guard: if db_.size() >= MAX_TRACKS (2000), the
    //              oldest track (front of vector) is erased before insertion
    //              to bound memory usage.
    //
    //              Initial covariance:
    //                P_pos  = max(rangeStdDev², 500²) metres² on diagonal.
    //                P_vel  = 500² m²/s² on velocity diagonal.
    //              Measurement noise R[2][2] (z-axis) grows with range to
    //              model elevation angle uncertainty at long range.
    //
    // REQUIREMENT: REQ-AESA-020  Track initialisation
    //
    // PARAMETERS:
    //   det                  [in]  Detection to initialise from. Must contain
    //                              a unique targetID and finite range/az/el.
    //   target               [in]  Ground-truth velocity input. Fields vx, vy,
    //                              vz used to seed the velocity state.
    //   maxUnambiguousRange  [in]  Used to correct range if det.isAmbiguous
    //                              and det.radialVelocity < 0.
    //   simTime              [in]  Current simulation time (seconds). Stored
    //                              in tr.lastSeenTime and tr.lastTrackBeamTime.
    //   cfg                  [in]  Radar configuration. Key fields:
    //                                cfg.noise.rangeStdDev — initial position
    //                                  variance floor.
    //                                cfg.beamWidth — elevation noise scaling.
    //                                cfg.minHitsToValidate — for future use.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Appends one TrackFile to db_ (after optional front erase).
    //               Does not modify any existing TrackFile.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-050  createNewTrack adds entry to db_
    //               TC-AESA-TRK-051  Duplicate ID does not create second track
    //               TC-AESA-TRK-052  Capacity guard evicts oldest when full
    //               TC-AESA-TRK-053  Ambiguous detection range corrected
    // =========================================================================
    void createNewTrack(const DetectionOutput& det,
                        const TargetInput& target,
                        double maxUnambiguousRange,
                        double simTime,
                        const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    applyScanMissLogic
    //
    // DESCRIPTION: Applies end-of-scan miss accounting and pruning to the
    //              track database. For each track:
    //                - If updatedThisScan is false: scanMissCount++ and
    //                  missCount++.
    //                - If updatedThisScan is true:  scanMissCount reset to 0.
    //                - updatedThisScan is reset to false for the next scan.
    //
    //              Tracks are then erased if either:
    //                - scanMissCount > cfg.missedScansToDrop, or
    //                - The track is not external AND
    //                  (simTime - lastSeenTime) > cfg.trackCoastSeconds.
    //
    //              External tracks (isExternalTrack == true) are exempt from
    //              the coast timeout but not from the scan-miss count limit.
    //
    // REQUIREMENT: REQ-AESA-025  Track coast and deletion
    //
    // PARAMETERS:
    //   simTime  [in]  Current simulation time (seconds).
    //   cfg      [in]  Radar configuration. Key fields:
    //                    cfg.missedScansToDrop — scan-miss threshold.
    //                    cfg.trackCoastSeconds — coast timeout (seconds).
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies tr.scanMissCount, tr.missCount, tr.updatedThisScan
    //               for all tracks. Erases entries from db_ that meet the
    //               deletion criteria.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-060  Miss increments scanMissCount
    //               TC-AESA-TRK-061  Hit resets scanMissCount to 0
    //               TC-AESA-TRK-062  Track dropped after missedScansToDrop
    //               TC-AESA-TRK-063  External track exempt from coast timeout
    // =========================================================================
    void applyScanMissLogic(double simTime, const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    buildTrackOutput
    //
    // DESCRIPTION: Constructs a TrackOutput struct from a TrackFile for
    //              consumption by downstream modules (display, weapons, Link-16).
    //              Computes derived fields: azimuth, elevation, speedOverGround,
    //              heading, targetAspect, time_to_cpa, cpa_distance, and Pk.
    //              Uses the track's current range if isUpdated, otherwise the
    //              predicted range.
    //
    // REQUIREMENT: REQ-AESA-026  Track output assembly
    //
    // PARAMETERS:
    //   track  [in]  TrackFile to convert. Must be a valid, initialised entry.
    //
    // RETURNS:    Fully populated TrackOutput by value.
    //
    // SIDE EFFECTS: None. Pure const computation.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-070  buildTrackOutput copies id/position/vel
    //               TC-AESA-TRK-071  buildTrackOutput computes correct azimuth
    //               TC-AESA-TRK-072  buildTrackOutput computes correct heading
    // =========================================================================
    TrackOutput buildTrackOutput(const TrackFile& track) const;

    // =========================================================================
    // FUNCTION:    buildTWSDetection
    //
    // DESCRIPTION: Constructs a DetectionOutput struct from a TrackFile for
    //              Track-While-Scan (TWS) forwarding. Used to pass track
    //              position estimates back to the signal processor or display
    //              as if they were detections. Uses predicted range when
    //              isUpdated is false.
    //
    // REQUIREMENT: REQ-AESA-026  TWS detection output
    //
    // PARAMETERS:
    //   track  [in]  TrackFile to convert. Must be a valid, initialised entry.
    //
    // RETURNS:    Fully populated DetectionOutput by value.
    //
    // SIDE EFFECTS: None. Pure const computation.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-073  buildTWSDetection copies id and range
    // =========================================================================
    DetectionOutput buildTWSDetection(const TrackFile& track) const;

    // =========================================================================
    // FUNCTION:    getValidatedTracks
    //
    // DESCRIPTION: Assembles TrackOutput entries for all validated tracks
    //              (isValidated == true) in the database and appends them
    //              to the provided output vector. Output vector is cleared
    //              before filling.
    //
    // REQUIREMENT: REQ-AESA-026  Validated track output
    //
    // PARAMETERS:
    //   out  [out]  Vector to receive validated track outputs. Cleared on
    //               entry. Capacity reserved to db_.size() to avoid
    //               reallocation.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Clears and repopulates out. No modification to db_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-074  getValidatedTracks returns empty when db empty
    //               TC-AESA-TRK-075  getValidatedTracks excludes tentative tracks
    //               TC-AESA-TRK-076  getValidatedTracks includes validated tracks
    // =========================================================================
    void getValidatedTracks(std::vector<TrackOutput>& out) const;

    // =========================================================================
    // FUNCTION:    generateTrackBeamRequests
    //
    // DESCRIPTION: Generates BeamRequest entries (Task::TRACK) for all
    //              validated tracks whose adaptive revisit interval has elapsed.
    //              The revisit interval is determined by
    //              computeAdaptiveTrackInterval(): 0.2 s for manoeuvring
    //              targets, 0.3 s for high-speed, 0.5 s for medium-speed,
    //              2.0 s for slow targets.
    //
    //              Manoeuvring tracks receive priority 20; non-manoeuvring
    //              tracks receive priority 10.
    //
    //              Beam pointing is computed from the current track state:
    //              azimuth = atan2(y, x), elevation = asin(z / predictedRange).
    //
    // REQUIREMENT: REQ-AESA-026  Track beam scheduling
    //
    // PARAMETERS:
    //   reqs     [out]  Vector to receive beam requests. Not cleared on entry —
    //                   requests are appended. Caller clears before calling if
    //                   needed.
    //   simTime  [in]   Current simulation time (seconds). Compared against
    //                   tr.lastTrackBeamTime to determine if revisit is due.
    //   cfg      [in]   Radar configuration. Key fields:
    //                     cfg.trackDwellTime_ms — dwell duration per request.
    //                     cfg.trackWaveform     — waveform type for track mode.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Appends to reqs. No modification to db_.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-080  No request generated before interval
    //               TC-AESA-TRK-081  Request generated after interval elapsed
    //               TC-AESA-TRK-082  Manoeuvring track gets priority 20
    // =========================================================================
    void generateTrackBeamRequests(std::vector<BeamRequest>& requests,
                                   double simTime,
                                   const RadarConfig& cfg) const;

    // =========================================================================
    // FUNCTION:    injectExternalTrack
    //
    // DESCRIPTION: Creates a new TrackFile from an externally provided
    //              TrackOutput (e.g. received via Link-16 / CEC). The injected
    //              track is marked isExternalTrack = true and isValidated = true
    //              with hitCount set to cfg.minHitsToValidate.
    //
    //              Duplicate guard: if a track with the same id already exists
    //              in db_, the injection is silently ignored.
    //
    //              Capacity guard: same as createNewTrack (MAX_TRACKS = 2000).
    //
    //              External track measurement noise is set large (R = 9e4)
    //              to reflect the lower accuracy of datalink-derived positions.
    //              Coast timeout does not apply to external tracks.
    //
    // REQUIREMENT: REQ-AESA-027  External track injection
    //
    // PARAMETERS:
    //   ext      [in]  External track data. Must contain a unique id and
    //                  finite position/velocity.
    //   simTime  [in]  Current simulation time (seconds). Stored in
    //                  lastSeenTime and lastTrackBeamTime.
    //   cfg      [in]  Radar configuration. Key field:
    //                    cfg.minHitsToValidate — initial hitCount value.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Appends one TrackFile to db_ (after optional front erase).
    //               Does not modify any existing TrackFile.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-090  injectExternalTrack adds entry to db_
    //               TC-AESA-TRK-091  Duplicate injection ignored
    //               TC-AESA-TRK-092  Injected track is marked isExternalTrack
    //               TC-AESA-TRK-093  Injected track is marked isValidated
    // =========================================================================
    void injectExternalTrack(const TrackOutput& ext,
                             double simTime,
                             const RadarConfig& cfg);

    // =========================================================================
    // FUNCTION:    performIMMPredict
    //
    // DESCRIPTION: Performs one IMM prediction step for a single track.
    //              Implements the standard five-step IMM algorithm
    //              (Bar-Shalom, Li, Kirubarajan, "Estimation with Applications
    //              to Tracking and Navigation", Ch. 11):
    //
    //              1. Initialise model states from tr.X / tr.P on first call
    //                 (immActive == false), then set immActive = true.
    //              2. Predicted model probabilities:
    //                   c̄ⱼ = Σᵢ πᵢⱼ · μᵢ
    //              3. Mixing probabilities:
    //                   μᵢ|ⱼ = πᵢⱼ · μᵢ / c̄ⱼ
    //              4. Mixed initial conditions per model:
    //                   X̄⁰ⱼ  = Σᵢ μᵢ|ⱼ · Xᵢ
    //                   P̄⁰ⱼ  = Σᵢ μᵢ|ⱼ · (Pᵢ + (Xᵢ−X̄⁰ⱼ)(Xᵢ−X̄⁰ⱼ)ᵀ)
    //              5. CV predict each model:
    //                   Xⱼ ← F·X̄⁰ⱼ,  Pⱼ ← F·P̄⁰ⱼ·Fᵀ + Qⱼ
    //                 Model 0 (CV): Q_vel = 1.0 m²/s², Q_vz = 1.0 m²/s²
    //                 Model 1 (Manoeuvre): Q_vel = 2500.0 m²/s², Q_vz = 25.0
    //              6. Fuse: overall state = Σⱼ c̄ⱼ · Xⱼ, update μ ← c̄.
    //
    //              Declared public to allow direct unit testing. Called from
    //              predict() for all tracks with hitCount >= 2.
    //
    // REQUIREMENT: REQ-AESA-024  IMM predict step
    //
    // PARAMETERS:
    //   tr  [in/out]  TrackFile to predict. Modified in place.
    //   dt  [in]      Prediction interval (seconds). Must be positive.
    //
    // RETURNS:     void
    //
    // SIDE EFFECTS: Modifies tr.X, tr.P, tr.imm_X, tr.imm_P, tr.imm_mu,
    //               tr.immActive.
    //
    // EXCEPTIONS:  noexcept — no exceptions thrown.
    //
    // TRACEABILITY:
    //   Test cases: TC-AESA-TRK-013  IMM self-initialises on first predict
    //               TC-AESA-TRK-014  IMM fused state matches CV at boresight
    //               TC-AESA-TRK-015  IMM model probs sum to 1.0 after predict
    // =========================================================================
    void performIMMPredict(TrackFile& tr, double dt) const noexcept;

    // =========================================================================
    // ACCESSORS — direct access to internal track database
    //
    // DESCRIPTION: Provide read-only and read-write access to db_ for use by
    //              the owning RadarModel_AESA (e.g. for scan-miss book-keeping
    //              and external track updates). Not for general use — prefer
    //              the specific API methods above.
    //
    // REQUIREMENT: REQ-AESA-020
    // =========================================================================

    // Returns const reference to the internal track database.
    const std::vector<TrackFile>& database() const { return db_; }

    // Returns mutable reference to the internal track database.
    // Use only for direct state manipulation during test or initialisation.
    std::vector<TrackFile>& database()       { return db_; }

private:

    // =========================================================================
    // PRIVATE MEMBER VARIABLES
    // =========================================================================

    // Internal track database. Each entry is one active TrackFile.
    // Maximum size bounded by MAX_TRACKS (2000) — oldest entry evicted
    // when capacity is exceeded. Reserved to 2048 on clear().
    // REQ-AESA-020.
    std::vector<TrackFile> db_;

    // =========================================================================
    // PRIVATE HELPER FUNCTIONS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    resolveRangeAmbiguity
    //
    // DESCRIPTION: Resolves PRF-induced range ambiguity by selecting the
    //              candidate range r_k = measured + k * Rmax (k in [-5, +5])
    //              that minimises |candidate - predicted|. If Rmax < 1.0,
    //              the measured range is returned unchanged.
    //
    // REQUIREMENT: REQ-AESA-022
    //
    // PARAMETERS:
    //   measured   [in]  Ambiguous measured range (metres).
    //   predicted  [in]  Track-predicted range (metres).
    //   Rmax       [in]  Maximum unambiguous range (metres). Disable if < 1.0.
    //
    // RETURNS:    Best candidate range (metres).
    //
    // SIDE EFFECTS: None. Pure const computation.
    // =========================================================================
    double resolveRangeAmbiguity(double measured,
                                 double predicted,
                                 double Rmax) const;

    // =========================================================================
    // FUNCTION:    computeAdaptiveTrackInterval
    //
    // DESCRIPTION: Returns the recommended track beam revisit interval
    //              (seconds) based on target speed and manoeuvre state.
    //              Manoeuvring: 0.2 s. Speed > 500 m/s: 0.3 s.
    //              Speed > 100 m/s: 0.5 s. Otherwise: 2.0 s.
    //
    // REQUIREMENT: REQ-AESA-026
    //
    // PARAMETERS:
    //   t  [in]  TrackFile whose speed and manoeuvre state are queried.
    //
    // RETURNS:    Revisit interval in seconds.
    //
    // SIDE EFFECTS: None. Pure const computation.
    // =========================================================================
    double computeAdaptiveTrackInterval(const TrackFile& t) const;

    // =========================================================================
    // FUNCTION:    gaussianLikelihood
    //
    // DESCRIPTION: Evaluates the 3-D Gaussian likelihood N(z; x, S) at
    //              measurement z with mean x and covariance S.
    //              Returns 0.0 if det(S) <= 0 or if S is singular.
    //              Norm denominator floored at 1e-30 to prevent division
    //              by zero on degenerate covariances.
    //
    // REQUIREMENT: REQ-AESA-022  JPDA likelihood computation
    //
    // PARAMETERS:
    //   z    [in]  3-element measurement vector (Cartesian, metres).
    //   x    [in]  3-element predicted state vector (Cartesian, metres).
    //   S    [in]  3×3 innovation covariance matrix. Must be positive-definite.
    //
    // RETURNS:    Gaussian likelihood value. Range: [0.0, +inf).
    //             Returns 0.0 on degenerate S.
    //
    // SIDE EFFECTS: None. Pure const computation.
    // =========================================================================
    double gaussianLikelihood(const double z[3],
                              const double x[3],
                              const double S[3][3]) const;

    // =========================================================================
    // FUNCTION:    invertS3
    //
    // DESCRIPTION: Computes the inverse of a 3×3 matrix S using the analytic
    //              cofactor / determinant formula. Returns false (and leaves
    //              inv unchanged) if |det(S)| < 1e-3, indicating a singular
    //              or near-singular matrix. Caller must check the return value
    //              and skip the Kalman update on failure.
    //
    // REQUIREMENT: REQ-AESA-023  Innovation covariance inversion
    //
    // PARAMETERS:
    //   S    [in]   3×3 matrix to invert.
    //   inv  [out]  3×3 result matrix. Written only if return is true.
    //
    // RETURNS:    true  — inversion successful; inv contains S^{-1}.
    //             false — matrix is singular; inv is not written.
    //
    // SIDE EFFECTS: Writes inv on success. No other side effects.
    // =========================================================================
    bool invertS3(const double S[3][3], double inv[3][3]) const;

    // =========================================================================
    // FUNCTION:    detS3
    //
    // DESCRIPTION: Computes the determinant of a 3×3 matrix S using the
    //              standard cofactor expansion along the first row.
    //
    // REQUIREMENT: REQ-AESA-023
    //
    // PARAMETERS:
    //   S  [in]  3×3 matrix.
    //
    // RETURNS:    Scalar determinant value (double).
    //
    // SIDE EFFECTS: None. Pure const computation.
    // =========================================================================
    double detS3(const double S[3][3]) const;
};

} // namespace aesa

#endif // RADARTRACKER_AESA_H


// #pragma once
// #ifndef RADARTRACKER_AESA_H
// #define RADARTRACKER_AESA_H
// // radartracker_aesa.h  —  Rev 3
// // FIX-05: full JPDA  |  FIX-12: external track injection

// #include "radarmodel_aesa.h"
// #include <vector>

// namespace aesa {

// class RadarTracker_AESA
// {
// public:
//     RadarTracker_AESA() = default;

//     void clear();

//     // Kalman prediction
//     void predict(double dt);

//     // NN association (fallback when useJPDA = false)
//     TrackFile* findBestTrackMatch(const DetectionOutput& det,
//                                   double maxUnambiguousRange,
//                                   double& outBestProb);

//     // Single-target Kalman update
//     void performKalmanUpdate(TrackFile& track,
//                              const DetectionOutput& det,
//                              double simTime, double dt,
//                              double maxUnambiguousRange,
//                              const RadarConfig& cfg);

//     // FIX-05  JPDA simultaneous update for all tracks vs all detections
//     void performJPDAUpdate(const std::vector<DetectionOutput>& detections,
//                            double simTime,
//                            double maxUnambiguousRange,
//                            const RadarConfig& cfg);

//     // New track creation
//     void createNewTrack(const DetectionOutput& det,
//                         const TargetInput& target,
//                         double maxUnambiguousRange,
//                         double simTime,
//                         const RadarConfig& cfg);

//     // Scan-miss pruning
//     void applyScanMissLogic(double simTime, const RadarConfig& cfg);

//     // Output assembly
//     TrackOutput     buildTrackOutput  (const TrackFile& track) const;
//     DetectionOutput buildTWSDetection (const TrackFile& track) const;
//     void            getValidatedTracks(std::vector<TrackOutput>& out) const;

//     // Beam requests for scheduler
//     void generateTrackBeamRequests(std::vector<BeamRequest>& requests,
//                                    double simTime,
//                                    const RadarConfig& cfg) const;

//     // FIX-12  Link-16 / CEC track injection
//     void injectExternalTrack(const TrackOutput& ext, double simTime,
//                              const RadarConfig& cfg);

//     const std::vector<TrackFile>& database() const { return db_; }
//           std::vector<TrackFile>& database()       { return db_; }
//     void performIMMPredict(TrackFile& tr, double dt) const noexcept;

// private:
//     std::vector<TrackFile> db_;

//     double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;
//     double computeAdaptiveTrackInterval(const TrackFile& t) const;

//     // FIX-05  JPDA matrix helpers
//     double gaussianLikelihood(const double z[3], const double x[3],
//                                const double S[3][3]) const;
//     bool   invertS3(const double S[3][3], double inv[3][3]) const;
//     double detS3   (const double S[3][3]) const;
// };

// } // namespace aesa
// #endif
