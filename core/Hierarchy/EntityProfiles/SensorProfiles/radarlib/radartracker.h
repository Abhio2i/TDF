#pragma once
// =============================================================================
// radartracker.h  —  Kalman track database and data association
//
// Responsibility:
//   Owns the track database (std::vector<TrackFile>) and all operations on it:
//     • Constant-velocity Kalman prediction (F·X, F·P·Fᵀ + Q)
//     • Mahalanobis-gated nearest-neighbour association
//     • Joseph-form Kalman measurement update
//     • New track initialisation
//     • Scan-miss logic and track pruning
//     • TrackOutput / DetectionOutput assembly from internal TrackFile state
//
// Design rules:
//   1. Owns trackDatabase_ (vector<TrackFile>) — nothing else does.
//   2. NO MUTEX — only called from RadarModel::update() which holds the lock.
//   3. Receives const RadarConfig& on each call — never stores a copy.
//   4. NO Qt types, no engine types.
//
// Future tracking algorithms (JPDA, IMM, multi-hypothesis) can be added by:
//   • Adding new methods here
//   • Branching in RadarModel::update() to select the algorithm
//   The TrackFile struct remains the common internal representation.
// =============================================================================

#ifndef RADARTRACKER_H
#define RADARTRACKER_H

#include "radarmodel.h"
#include <vector>

class RadarTracker
{
public:
    // -------------------------------------------------------------------------
    // Construction / lifecycle
    // -------------------------------------------------------------------------

    RadarTracker() = default;

    /// Discard all tracks.  Called by RadarModel::start() and reset().
    void clear();

    // -------------------------------------------------------------------------
    // §A  Kalman prediction  (called every tick before detection)
    // -------------------------------------------------------------------------

    /// Propagate all track states forward by dt seconds using a
    /// constant-velocity motion model (F·X, F·P·Fᵀ + Q).
    /// Also resets per-tick isUpdated flags so association can tell which
    /// tracks have not yet been matched this tick.
    ///
    /// @param dt  Tick duration (seconds)
    void predict(double dt);

    // -------------------------------------------------------------------------
    // §B  Association  (nearest-neighbour, Mahalanobis gated)
    // -------------------------------------------------------------------------

    /// Find the best existing track to associate with det.
    /// Uses a χ² gate (3-DOF, 99 %) and picks the highest association
    /// probability if multiple tracks pass the gate.
    ///
    /// @param det                  Raw detection to associate
    /// @param maxUnambiguousRange  PRF Rmax for range-fold correction
    /// @param outBestProb          [out] Association probability of winner
    /// @return Pointer into trackDatabase_, or nullptr if no match found.
    TrackFile* findBestTrackMatch(const DetectionOutput& det,
                                  double maxUnambiguousRange,
                                  double& outBestProb);

    // -------------------------------------------------------------------------
    // §C  Kalman update
    // -------------------------------------------------------------------------

    /// Update track with a matched detection (Joseph-form covariance update).
    /// Increments hitCount; validates track once hitCount ≥ minHitsToValidate.
    ///
    /// @param track                Track to update (from findBestTrackMatch)
    /// @param det                  Matched detection
    /// @param simTime              Simulation clock (seconds)
    /// @param dt                   Tick duration (seconds)
    /// @param maxUnambiguousRange  PRF Rmax for range unfolding
    /// @param cfg                  Config (minHitsToValidate, maxTrackSpeed, …)
    void performKalmanUpdate(TrackFile& track,
                             const DetectionOutput& det,
                             double simTime,
                             double dt,
                             double maxUnambiguousRange,
                             const RadarConfig& cfg);

    /// Initialise a new track from a detection that found no match.
    /// Does nothing if a track with det.targetID already exists.
    ///
    /// @param det                  Unmatched detection
    /// @param target               Corresponding TargetInput (for velocity seed)
    /// @param maxUnambiguousRange  PRF Rmax for initial range correction
    /// @param simTime              Simulation clock (seconds)
    /// @param cfg                  Config (noise.rangeStdDev for P initialisation)
    void createNewTrack(const DetectionOutput& det,
                        const TargetInput& target,
                        double maxUnambiguousRange,
                        double simTime,
                        const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // §D  Scan-miss logic and pruning
    // -------------------------------------------------------------------------

    /// Called once per scan boundary (when antenna reverses / completes a rev).
    /// For each track that was not updated this scan:
    ///   • increments scanMissCount and missCount
    ///   • resets updatedThisScan flag
    /// Prunes tracks that exceed missedScansToDrop or have coasted past
    /// trackCoastSeconds.
    ///
    /// @param simTime  Simulation clock (seconds)
    /// @param cfg      Config (missedScansToDrop, trackCoastSeconds)
    void applyScanMissLogic(double simTime, const RadarConfig& cfg);

    // -------------------------------------------------------------------------
    // §E  Output assembly
    // -------------------------------------------------------------------------

    /// Build a TrackOutput from a validated TrackFile for the output bundle.
    TrackOutput buildTrackOutput(const TrackFile& track) const;

    /// Build a DetectionOutput from a validated TrackFile.
    /// Used when the display layer needs detection-style records for TWS tracks.
    DetectionOutput buildTWSDetection(const TrackFile& track) const;

    /// Fill out with TrackOutput for all validated tracks.
    void getValidatedTracks(std::vector<TrackOutput>& out) const;

    // -------------------------------------------------------------------------
    // §F  Direct database access  (for diagnostics / RadarModel internals)
    // -------------------------------------------------------------------------

    /// Read-only access to the raw track database.
    const std::vector<TrackFile>& database() const { return trackDatabase_; }

private:
    // -------------------------------------------------------------------------
    // Track database — the only member data in this class
    // -------------------------------------------------------------------------
    std::vector<TrackFile> trackDatabase_;

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /// Unfold a measured range using the Kalman prediction as a reference.
    /// Searches k ∈ [−5, 5] for the offset that minimises |measured + k·Rmax − predicted|.
    double resolveRangeAmbiguity(double measuredRange,
                                 double predictedRange,
                                 double Rmax) const;

    /// Gaussian association probability used to rank competing track matches.
    double computeAssociationProbability(double measurementRange,
                                         double predictedRange,
                                         double gateSize) const;
};

#endif // RADARTRACKER_H
