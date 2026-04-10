#pragma once
#ifndef RADARTRACKER_AESA_H
#define RADARTRACKER_AESA_H
// radartracker_aesa.h  —  Rev 3
// FIX-05: full JPDA  |  FIX-12: external track injection

#include "radarmodel_aesa.h"
#include <vector>

namespace aesa {

class RadarTracker_AESA
{
public:
    RadarTracker_AESA() = default;

    void clear();

    // Kalman prediction
    void predict(double dt);

    // NN association (fallback when useJPDA = false)
    TrackFile* findBestTrackMatch(const DetectionOutput& det,
                                  double maxUnambiguousRange,
                                  double& outBestProb);

    // Single-target Kalman update
    void performKalmanUpdate(TrackFile& track,
                             const DetectionOutput& det,
                             double simTime, double dt,
                             double maxUnambiguousRange,
                             const RadarConfig& cfg);

    // FIX-05  JPDA simultaneous update for all tracks vs all detections
    void performJPDAUpdate(const std::vector<DetectionOutput>& detections,
                           double simTime,
                           double maxUnambiguousRange,
                           const RadarConfig& cfg);

    // New track creation
    void createNewTrack(const DetectionOutput& det,
                        const TargetInput& target,
                        double maxUnambiguousRange,
                        double simTime,
                        const RadarConfig& cfg);

    // Scan-miss pruning
    void applyScanMissLogic(double simTime, const RadarConfig& cfg);

    // Output assembly
    TrackOutput     buildTrackOutput  (const TrackFile& track) const;
    DetectionOutput buildTWSDetection (const TrackFile& track) const;
    void            getValidatedTracks(std::vector<TrackOutput>& out) const;

    // Beam requests for scheduler
    void generateTrackBeamRequests(std::vector<BeamRequest>& requests,
                                   double simTime,
                                   const RadarConfig& cfg) const;

    // FIX-12  Link-16 / CEC track injection
    void injectExternalTrack(const TrackOutput& ext, double simTime,
                             const RadarConfig& cfg);

    const std::vector<TrackFile>& database() const { return db_; }
          std::vector<TrackFile>& database()       { return db_; }
    void performIMMPredict(TrackFile& tr, double dt) const noexcept;

private:
    std::vector<TrackFile> db_;

    double resolveRangeAmbiguity(double measured, double predicted, double Rmax) const;
    double computeAdaptiveTrackInterval(const TrackFile& t) const;

    // FIX-05  JPDA matrix helpers
    double gaussianLikelihood(const double z[3], const double x[3],
                               const double S[3][3]) const;
    bool   invertS3(const double S[3][3], double inv[3][3]) const;
    double detS3   (const double S[3][3]) const;
};

} // namespace aesa
#endif
