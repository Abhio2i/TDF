// =============================================================================
// radartracker.cpp  —  Kalman track database implementation
//
// All Kalman mathematics (prediction, association, update) lives here.
// The only state this class owns is trackDatabase_ (vector<TrackFile>).
//
// Adding a new tracking algorithm:
//   • Add a method to RadarTracker (e.g. performJPDAUpdate)
//   • Branch in RadarModel::update() to select it via a config flag
//   • TrackFile is the common internal representation — extend it if needed
// =============================================================================

#include "radartracker.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Association gate radius in metres — any detection further than this from
/// the predicted range is rejected before the Mahalanobis test.
static constexpr double RANGE_GATE = 2000.0;

/// χ² threshold for a 3-DOF 99 % confidence gate.
static constexpr double CHI2_GATE_99 = 9.21;

/// Maximum number of tracks — oldest is evicted if exceeded.
static constexpr std::size_t MAX_TRACKS = 2000;

// =============================================================================
// Lifecycle
// =============================================================================

void RadarTracker::clear()
{
    trackDatabase_.clear();
    // Pre-allocate to match original radarmodel.cpp init() which called
    // trackDatabase_.reserve(2048) — avoids repeated heap reallocations
    // as tracks accumulate during a mission.
    trackDatabase_.reserve(2048);
}

// =============================================================================
// §A  Kalman prediction
// =============================================================================

void RadarTracker::predict(double dt)
{
    for (auto& track : trackDatabase_)
    {
        // ---- State transition: constant-velocity model ----
        // State vector X = [x, y, z, vx, vy, vz]
        // F = I₆ with off-diagonal dt terms coupling position to velocity
        double F[6][6] = {
            {1,0,0, dt, 0, 0},
            {0,1,0,  0,dt, 0},
            {0,0,1,  0, 0,dt},
            {0,0,0,  1, 0, 0},
            {0,0,0,  0, 1, 0},
            {0,0,0,  0, 0, 1}
        };

        // X_k|k-1 = F · X_k-1
        std::array<double,6> Xnew = {};
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                Xnew[i] += F[i][j] * track.X[j];
        track.X = Xnew;

        // P_k|k-1 = F · P_k-1 · Fᵀ + Q
        // Step 1: FP = F · P
        double FP[6][6] = {};
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                for (int k = 0; k < 6; ++k)
                    FP[i][j] += F[i][k] * track.P[k][j];

        // Step 2: P = FP · Fᵀ + Q  (note: F is symmetric here so Fᵀ = F)
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
            {
                double tmp = 0.0;
                for (int k = 0; k < 6; ++k)
                    tmp += FP[i][k] * F[j][k]; // F[j][k] = Fᵀ[k][j]
                track.P[i][j] = tmp + track.Q[i][j];
            }

        // Mirror Kalman state back into the convenience position/velocity fields
        track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
        track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];

        track.predictedRange = std::sqrt(
            track.x*track.x + track.y*track.y + track.z*track.z);

        // Clear the "updated this tick" flag so association knows this track
        // has not yet been matched to a detection in the current tick
        track.isUpdated = false;
    }
}

// =============================================================================
// §B  Association
// =============================================================================

double RadarTracker::resolveRangeAmbiguity(
    double measured, double predicted, double Rmax) const
{
    if (Rmax < 1.0) return measured;

    // Test k ∈ [−5, 5]: choose the fold that minimises prediction error
    double best   = measured;
    double minErr = 1e12;
    for (int k = -5; k <= 5; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

double RadarTracker::computeAssociationProbability(
    double measurementRange, double predictedRange, double gateSize) const
{
    // Gaussian probability density — higher = better match
    double err   = measurementRange - predictedRange;
    double sigma = gateSize / 2.0;
    return std::exp(-(err * err) / (2.0 * sigma * sigma));
}

TrackFile* RadarTracker::findBestTrackMatch(
    const DetectionOutput& det,
    double maxUnambiguousRange,
    double& outBestProb)
{
    TrackFile* best = nullptr;
    outBestProb     = 0.0;

    for (auto& track : trackDatabase_)
    {
        // ---- Sanity check: discard tracks with NaN state ----
        if (!std::isfinite(track.x)  || !std::isfinite(track.y)  ||
            !std::isfinite(track.z)  || !std::isfinite(track.vx) ||
            !std::isfinite(track.vy) || !std::isfinite(track.vz))
        {
            // Corrupt track — reset to tentative
            track.hitCount    = 0;
            track.isValidated = false;
            continue;
        }

        // Skip tracks already matched this tick (nearest-neighbour: 1:1)
        if (track.isUpdated) continue;

        // ---- Fast pre-filter: range gate ----
        // Unfold the detection range using this track's prediction, then
        // reject if the residual exceeds RANGE_GATE.
        double candRange = resolveRangeAmbiguity(
            det.range, track.predictedRange, maxUnambiguousRange);

        if (std::abs(candRange - track.predictedRange) > RANGE_GATE) continue;

        // ---- Mahalanobis distance gate (χ², 3-DOF, 99 %) ----
        // Convert detection (range, az, el) to Cartesian measurement z
        double azRad = det.azimuth   * M_PI / 180.0;
        double elRad = det.elevation * M_PI / 180.0;
        double zx = det.range * std::cos(elRad) * std::cos(azRad);
        double zy = det.range * std::cos(elRad) * std::sin(azRad);
        double zz = det.range * std::sin(elRad);

        // Innovation y = z − H·X  (H selects position from state)
        double dx = zx - track.X[0];
        double dy = zy - track.X[1];
        double dz = zz - track.X[2];

        // Innovation covariance S = H·P·Hᵀ + R  (H = I₃ for Cartesian meas.)
        double S[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                S[i][j] = track.P[i][j] + track.R[i][j];

        // 3×3 determinant for matrix inversion
        double detS = S[0][0]*(S[1][1]*S[2][2] - S[1][2]*S[2][1])
                      -S[0][1]*(S[1][0]*S[2][2] - S[1][2]*S[2][0])
                      +S[0][2]*(S[1][0]*S[2][1] - S[1][1]*S[2][0]);
        if (std::abs(detS) < 1e-2) continue; // Near-singular — skip

        // S⁻¹ via cofactor expansion / determinant
        double invS[3][3];
        invS[0][0] = (S[1][1]*S[2][2] - S[1][2]*S[2][1]) / detS;
        invS[0][1] = (S[0][2]*S[2][1] - S[0][1]*S[2][2]) / detS;
        invS[0][2] = (S[0][1]*S[1][2] - S[0][2]*S[1][1]) / detS;
        invS[1][0] = (S[1][2]*S[2][0] - S[1][0]*S[2][2]) / detS;
        invS[1][1] = (S[0][0]*S[2][2] - S[0][2]*S[2][0]) / detS;
        invS[1][2] = (S[0][2]*S[1][0] - S[0][0]*S[1][2]) / detS;
        invS[2][0] = (S[1][0]*S[2][1] - S[1][1]*S[2][0]) / detS;
        invS[2][1] = (S[0][1]*S[2][0] - S[0][0]*S[2][1]) / detS;
        invS[2][2] = (S[0][0]*S[1][1] - S[0][1]*S[1][0]) / detS;

        // Mahalanobis distance squared: d² = yᵀ · S⁻¹ · y
        double d2 = dx*(invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz)
                    + dy*(invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz)
                    + dz*(invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);

        if (d2 > CHI2_GATE_99) continue; // Outside gate

        // Keep the match with the highest range-based association probability
        double prob = computeAssociationProbability(
            candRange, track.predictedRange, RANGE_GATE);

        if (prob > outBestProb) { outBestProb = prob; best = &track; }
    }
    return best;
}

// =============================================================================
// §C  Kalman update
// =============================================================================

void RadarTracker::performKalmanUpdate(
    TrackFile& track,
    const DetectionOutput& det,
    double simTime, double /*dt*/,
    double maxUnambiguousRange,
    const RadarConfig& cfg)
{
    // Unfold the detection range using this track's predicted range
    double bestRange = resolveRangeAmbiguity(
        det.range, track.predictedRange, maxUnambiguousRange);

    // Convert detection to Cartesian measurement z = [zx, zy, zz]
    double azRad = det.azimuth   * M_PI / 180.0;
    double elRad = det.elevation * M_PI / 180.0;
    double z[3] = {
        bestRange * std::cos(elRad) * std::cos(azRad),
        bestRange * std::cos(elRad) * std::sin(azRad),
        bestRange * std::sin(elRad)
    };

    // Innovation: y = z − H·X  (H selects first 3 elements of state)
    double y[3] = { z[0] - track.X[0],
                   z[1] - track.X[1],
                   z[2] - track.X[2] };

    // Innovation covariance: S = H·P·Hᵀ + R
    double S[3][3] = {};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            S[i][j] = track.P[i][j] + track.R[i][j];

    double detS = S[0][0]*(S[1][1]*S[2][2] - S[1][2]*S[2][1])
                  -S[0][1]*(S[1][0]*S[2][2] - S[1][2]*S[2][0])
                  +S[0][2]*(S[1][0]*S[2][1] - S[1][1]*S[2][0]);

    if (std::abs(detS) < 1e-3)
    {
        // S is near-singular — cannot compute gain; penalise but don't crash
        track.hitCount = std::max(0, track.hitCount - 1);
        return;
    }

    double invS[3][3];
    invS[0][0] = (S[1][1]*S[2][2] - S[1][2]*S[2][1]) / detS;
    invS[0][1] = (S[0][2]*S[2][1] - S[0][1]*S[2][2]) / detS;
    invS[0][2] = (S[0][1]*S[1][2] - S[0][2]*S[1][1]) / detS;
    invS[1][0] = (S[1][2]*S[2][0] - S[1][0]*S[2][2]) / detS;
    invS[1][1] = (S[0][0]*S[2][2] - S[0][2]*S[2][0]) / detS;
    invS[1][2] = (S[0][2]*S[1][0] - S[0][0]*S[1][2]) / detS;
    invS[2][0] = (S[1][0]*S[2][1] - S[1][1]*S[2][0]) / detS;
    invS[2][1] = (S[0][1]*S[2][0] - S[0][0]*S[2][1]) / detS;
    invS[2][2] = (S[0][0]*S[1][1] - S[0][1]*S[1][0]) / detS;

    // Kalman gain: K = P · Hᵀ · S⁻¹  (6×3 matrix)
    double K[6][3] = {};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                K[i][j] += track.P[i][k] * invS[k][j];

    // State update: X = X + K·y
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            track.X[i] += K[i][j] * y[j];

    // ---- Joseph-form covariance update: P = (I − K·H)·P·(I − K·H)ᵀ ----
    // More numerically stable than P = (I − K·H)·P for poorly-conditioned P.
    //
    // IKH = I − K·H  (6×6; H = [I₃ | 0₃] so K·H takes first 3 cols of K)
    double IKH[6][6] = {};
    for (int i = 0; i < 6; ++i) IKH[i][i] = 1.0; // start as identity
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j)
            IKH[i][j] -= K[i][j];

    double Pnew[6][6] = {};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            for (int k = 0; k < 6; ++k)
                Pnew[i][j] += IKH[i][k] * track.P[k][j];
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            track.P[i][j] = Pnew[i][j];

    // Clamp velocity estimate to prevent runaway divergence
    for (int i = 3; i < 6; ++i)
        track.X[i] = std::clamp(track.X[i],
                                -cfg.maxTrackSpeed,
                                cfg.maxTrackSpeed);

    // Mirror updated state into convenience fields
    track.x  = track.X[0]; track.y  = track.X[1]; track.z  = track.X[2];
    track.vx = track.X[3]; track.vy = track.X[4]; track.vz = track.X[5];

    track.range = std::sqrt(track.x*track.x + track.y*track.y + track.z*track.z);

    // Radial velocity = dot(v, r̂)
    track.velocity = (track.range > 1e-6)
                         ? (track.vx*track.x + track.vy*track.y + track.vz*track.z) / track.range
                         : 0.0;

    // Update bookkeeping
    track.lastSeenTime    = simTime;
    track.hitCount++;
    track.isUpdated       = true;
    track.updatedThisScan = true;
    track.missCount       = 0;
    track.wasAmbiguous    = det.isAmbiguous;

    // Validate track once it has accumulated enough confirmed detections
    if (track.hitCount >= cfg.minHitsToValidate)
        track.isValidated = true;
}

void RadarTracker::createNewTrack(
    const DetectionOutput& det,
    const TargetInput& target,
    double maxUnambiguousRange,
    double simTime,
    const RadarConfig& cfg)
{
    // Do not create a duplicate track for an ID that already exists
    for (const auto& t : trackDatabase_)
        if (t.id == det.targetID) return;

    TrackFile tr;
    tr.id = det.targetID;

    // Seed position from detection.
    // If the detection was range-ambiguous and the target is closing,
    // add one PRF interval to start the track in the correct range bin.
    double r = det.range;
    if (det.isAmbiguous && det.radialVelocity < 0.0)
        r += maxUnambiguousRange;

    double azRad = det.azimuth   * M_PI / 180.0;
    double elRad = det.elevation * M_PI / 180.0;

    tr.x  = r * std::cos(elRad) * std::cos(azRad);
    tr.y  = r * std::cos(elRad) * std::sin(azRad);
    tr.z  = r * std::sin(elRad);

    // Seed velocity from the TargetInput (engine-provided, not measured by radar)
    tr.vx = target.vx;
    tr.vy = target.vy;
    tr.vz = target.vz;
    tr.X  = { tr.x, tr.y, tr.z, tr.vx, tr.vy, tr.vz };

    // ---- Initial covariance ----
    // Position uncertainty: driven by range standard deviation from noise model
    double posVar = cfg.noise.rangeStdDev * cfg.noise.rangeStdDev;
    // Velocity uncertainty: large initial spread (500 m/s σ) — shrinks quickly
    double velVar = 500.0 * 500.0;

    for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) tr.P[i][j] = 0.0;
    tr.P[0][0] = posVar; tr.P[1][1] = posVar; tr.P[2][2] = posVar;
    tr.P[3][3] = velVar; tr.P[4][4] = velVar; tr.P[5][5] = velVar;

    // ---- Process noise Q ----
    // Small position noise, small velocity noise — constant-velocity assumption
    tr.Q[0][0] = 10.0; tr.Q[1][1] = 10.0; tr.Q[2][2] = 10.0;
    tr.Q[3][3] =  1.0; tr.Q[4][4] =  1.0; tr.Q[5][5] =  1.0;

    // ---- Measurement noise R ----  (3×3 Cartesian position uncertainty)
    for (int i = 0; i < 3; ++i) tr.R[i][i] = 25.0;  // 5 m σ

    // Bookkeeping
    tr.range           = r;
    tr.predictedRange  = r;
    tr.velocity        = det.radialVelocity;
    tr.lastSeenTime    = simTime;
    tr.hitCount        = 1;
    tr.scanMissCount   = 0;
    tr.updatedThisScan = true;
    tr.wasAmbiguous    = det.isAmbiguous;

    // Evict oldest track if database is full.
    // Original used > 2000 (allows 2001 entries before eviction) — preserved exactly.
    if (trackDatabase_.size() > MAX_TRACKS)
        trackDatabase_.erase(trackDatabase_.begin());

    trackDatabase_.push_back(std::move(tr));
}

// =============================================================================
// §D  Scan-miss logic and pruning
// =============================================================================

void RadarTracker::applyScanMissLogic(double simTime, const RadarConfig& cfg)
{
    for (auto& track : trackDatabase_)
    {
        if (!track.updatedThisScan)
        {
            // This track was not matched to any detection during the last scan
            track.scanMissCount++;
            track.missCount++;
        }
        else
        {
            // Reset per-scan miss counter when the track was seen this scan
            track.scanMissCount = 0;
        }

        // Clear the per-scan flag ready for the next scan
        track.updatedThisScan = false;
    }

    // Prune: remove tracks that have been missed too many times or coasted too long
    trackDatabase_.erase(
        std::remove_if(trackDatabase_.begin(), trackDatabase_.end(),
                       [&](const TrackFile& t)
                       {
                           bool tooManyMisses = (t.scanMissCount > cfg.missedScansToDrop);
                           bool coastExpired  = ((simTime - t.lastSeenTime) > cfg.trackCoastSeconds);
                           return tooManyMisses || coastExpired;
                       }),
        trackDatabase_.end());
}

// =============================================================================
// §E  Output assembly
// =============================================================================

TrackOutput RadarTracker::buildTrackOutput(const TrackFile& tr) const
{
    TrackOutput out;
    out.id           = tr.id;

    // Position and velocity from Kalman state
    out.x  = tr.x;  out.y  = tr.y;  out.z  = tr.z;
    out.vx = tr.vx; out.vy = tr.vy; out.vz = tr.vz;

    out.radialVelocity = tr.velocity;
    out.isValidated    = tr.isValidated;
    out.hitCount       = tr.hitCount;
    out.scanMissCount  = tr.scanMissCount;
    out.wasAmbiguous   = tr.wasAmbiguous;

    // Report updated range if measurement was received this tick,
    // otherwise report the Kalman-predicted range (coast)
    double reportRange = tr.isUpdated ? tr.range : tr.predictedRange;
    out.range          = reportRange;

    if (reportRange > 1e-6)
    {
        out.azimuth   = std::atan2(tr.y, tr.x) * (180.0 / M_PI);
        if (out.azimuth < 0.0) out.azimuth += 360.0;

        out.elevation = std::asin(std::clamp(tr.z / reportRange, -1.0, 1.0))
                        * (180.0 / M_PI);
    }


    // out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vz*tr.vz);
    // out.heading = std::atan2(tr.vx, tr.vz) * (180.0 / M_PI);
    // if (out.heading < 0.0) out.heading += 360.0;
    // REPLACE WITH:
    out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vy*tr.vy);
    out.heading = std::atan2(tr.vy, tr.vx) * (180.0 / M_PI);
    if (out.heading < 0.0) out.heading += 360.0;

    // Target aspect: absolute angular difference between heading and azimuth
    out.targetAspect = std::abs(out.heading - out.azimuth);
    if (out.targetAspect > 180.0) out.targetAspect = 360.0 - out.targetAspect;

    // CPA computation from current Kalman state
    double v2 = tr.vx*tr.vx + tr.vy*tr.vy + tr.vz*tr.vz;
    if (v2 > 0.01)
    {
        double t_cpa = -(tr.x*tr.vx + tr.y*tr.vy + tr.z*tr.vz) / v2;
        if (t_cpa > 0.0)
        {
            out.time_to_cpa  = t_cpa;
            double cx = tr.x + tr.vx * t_cpa;
            double cy = tr.y + tr.vy * t_cpa;
            double cz = tr.z + tr.vz * t_cpa;
            out.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
        }
        else
        {
            // Already past CPA
            out.time_to_cpa  = 0.0;
            out.cpa_distance = reportRange;
        }
    }
    else
    {
        out.time_to_cpa  = 0.0;
        out.cpa_distance = reportRange;
    }

    // Probability of kill from Kalman range / radial velocity
    out.Pk = std::min(0.99,
                      0.95 * std::exp(-reportRange / 45000.0) * (tr.velocity < 0.0 ? 1.2 : 0.8));

    return out;
}

DetectionOutput RadarTracker::buildTWSDetection(const TrackFile& tr) const
{
    DetectionOutput out;
    out.targetID = tr.id;

    double reportRange = tr.isUpdated ? tr.range : tr.predictedRange;
    out.range          = reportRange;
    out.radialVelocity = tr.velocity;
    out.isAmbiguous    = false; // TWS detections are already resolved

    if (reportRange > 1e-6)
    {
        out.azimuth   = std::atan2(tr.y, tr.x) * (180.0 / M_PI);
        if (out.azimuth < 0.0) out.azimuth += 360.0;

        out.elevation = std::asin(std::clamp(tr.z / reportRange, -1.0, 1.0))
                        * (180.0 / M_PI);
    }

    // out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vz*tr.vz);

    // out.heading = std::atan2(tr.vx,     tr.vz)     * (180.0 / M_PI);

    // if (out.heading < 0.0) out.heading += 360.0;
    // REPLACE WITH:
    out.speedOverGround = std::sqrt(tr.vx*tr.vx + tr.vy*tr.vy);
    out.heading = std::atan2(tr.vy, tr.vx) * (180.0 / M_PI);
    if (out.heading < 0.0) out.heading += 360.0;

    out.targetAspect = std::abs(out.heading - out.azimuth);
    if (out.targetAspect > 180.0) out.targetAspect = 360.0 - out.targetAspect;

    double v2 = tr.vx*tr.vx + tr.vy*tr.vy + tr.vz*tr.vz;
    if (v2 > 0.01)
    {
        double t_cpa = -(tr.x*tr.vx + tr.y*tr.vy + tr.z*tr.vz) / v2;
        if (t_cpa > 0.0)
        {
            out.time_to_cpa  = t_cpa;
            double cx = tr.x + tr.vx * t_cpa;
            double cy = tr.y + tr.vy * t_cpa;
            double cz = tr.z + tr.vz * t_cpa;
            out.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
        }
        else
        {
            out.time_to_cpa  = 0.0;
            out.cpa_distance = reportRange;
        }
    }
    else
    {
        out.time_to_cpa  = 0.0;
        out.cpa_distance = reportRange;
    }

    out.Pk        = std::min(0.99,
                      0.95 * std::exp(-reportRange / 45000.0) * (tr.velocity < 0.0 ? 1.2 : 0.8));
    out.snr       = 0.0;   // Not recomputed at report time
    out.lockBroken = false;

    return out;
}

void RadarTracker::getValidatedTracks(std::vector<TrackOutput>& out) const
{
    out.clear();
    out.reserve(trackDatabase_.size());
    for (const auto& tr : trackDatabase_)
    {
        if (!tr.isValidated) continue;
        out.push_back(buildTrackOutput(tr));
    }
}
