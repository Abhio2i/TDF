// =============================================================================
// FILE:         radartracker_aesa.cpp
// MODULE:       AESA Radar Track Management
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements multi-target track management for the AESA radar
//               simulation. Provides Kalman-filter prediction (CV + IMM),
//               nearest-neighbour (NN) and Joint Probabilistic Data Association
//               (JPDA) measurement-to-track association, scan-miss pruning,
//               track output assembly, and beam request generation.
//
//               All functions are stateless computations or direct state
//               updates on db_. No external I/O, no dynamic memory beyond
//               STL containers, no recursion.
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
//                       replaced with named constexpr constants.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#include "radartracker_aesa.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

// =============================================================================
// NAMED CONSTANTS
// All numeric literals used in tracking computations are declared here.
// This satisfies VI-08 (no magic numbers) and ensures single-point change
// if physical model parameters or thresholds are revised.
// =============================================================================

namespace
{
// Pi — full precision, replaces non-standard M_PI macro (LC-08 compliance).
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Range gate width (metres). Detections further than this from the track's
// predicted range (after ambiguity resolution) are rejected before the
// chi-squared gate. This pre-filter reduces the cost of the matrix inversion
// in the chi-squared gate. REQ-AESA-022.
static constexpr double RANGE_GATE = 5000.0;

// Chi-squared gate threshold for 99% probability ellipsoid in 3 dimensions
// (3 DOF chi-squared at 99% confidence = 9.21). Detections with Mahalanobis
// distance squared d² > CHI2_GATE_99 are rejected. REQ-AESA-022.
static constexpr double CHI2_GATE_99 = 9.21;

// Maximum number of track entries in db_. When this limit is reached,
// the oldest entry (db_.front()) is erased before a new track is inserted.
// Prevents unbounded memory growth in long-duration simulations. REQ-AESA-020.
static constexpr std::size_t MAX_TRACKS = 2000;

// IMM transition probability matrix π[i][j] = P(model j | model i).
// Model 0: constant-velocity (CV). Model 1: manoeuvre (high process noise).
// Row 0: from CV:       95% stay CV, 5% switch to manoeuvre.
// Row 1: from manoeuvre: 10% revert to CV, 90% stay manoeuvre.
// Values selected per Blackman & Popoli, "Design and Analysis of Modern
// Tracking Systems", Ch. 6. REQ-AESA-024.
static constexpr double IMM_PI[2][2] = { { 0.95, 0.05 },
                                        { 0.10, 0.90 } };

// IMM velocity process noise variance Q_vel for each model (m²/s²).
// Model 0 (CV):         1.0  m²/s²  — low noise, smooth trajectory.
// Model 1 (Manoeuvre):  2500.0 m²/s² — high noise, ~50 m/s² acceleration.
// REQ-AESA-024.
static constexpr double IMM_Q_VEL[2] = { 1.0, 2500.0 };

// Vertical velocity process noise cap for IMM manoeuvre model (m²/s²).
// Kept lower than IMM_Q_VEL[1] because vertical acceleration is physically
// limited for most air targets. REQ-AESA-024.
static constexpr double IMM_Q_VZ_MANOEUVRE = 25.0;

// Vertical velocity process noise for CV model (m²/s²). Matches base Q.
// REQ-AESA-024.
static constexpr double IMM_Q_VZ_CV = 1.0;

// Initial track position variance when rangeStdDev² < this value (m²).
// Floors the initial covariance P_pos to avoid over-confidence on first
// hit. Value = 500² = 250000 m². REQ-AESA-020.
static constexpr double INIT_POS_VAR_FLOOR = 500.0 * 500.0;

// Initial track velocity variance (m²/s²). Value = 500² = 250000 m²/s².
// Wide to reflect complete uncertainty about target velocity at birth.
// REQ-AESA-020.
static constexpr double INIT_VEL_VAR = 500.0 * 500.0;

// Initial track position process noise Q_pos for new track entries (m²).
// REQ-AESA-020.
static constexpr double INIT_Q_POS = 10.0;

// Initial track velocity process noise Q_vel for new track entries (m²/s²).
// REQ-AESA-020.
static constexpr double INIT_Q_VEL = 1.0;

// Default x/y measurement noise variance R_xy (m²). Value = 25 m² = 5 m RMS.
// REQ-AESA-023.
static constexpr double MEAS_R_XY = 25.0;

// Minimum z measurement noise standard deviation (metres). Used to floor
// the elevation-angle-derived z noise at long range. REQ-AESA-020.
static constexpr double MEAS_R_Z_MIN_STD = 50.0;

// External track measurement noise variance for all axes (m²).
// Large value reflects reduced accuracy of datalink-derived position.
// Value = 300² m² per axis. REQ-AESA-027.
static constexpr double EXT_MEAS_R = 9.0e4;

// External track initial position variance (m²). Value = 300² = 90000 m².
// REQ-AESA-027.
static constexpr double EXT_P_POS = 300.0 * 300.0;

// External track initial velocity variance (m²/s²). Value = 50² = 2500 m²/s².
// REQ-AESA-027.
static constexpr double EXT_P_VEL = 50.0 * 50.0;

// Adaptive track interval — manoeuvring target (seconds). REQ-AESA-026.
static constexpr double TRACK_INTERVAL_MANOUVRE = 0.2;

// Adaptive track interval — high-speed target (speed > 500 m/s). REQ-AESA-026.
static constexpr double TRACK_INTERVAL_HIGH_SPEED = 0.3;

// Speed threshold for high-speed classification (m/s). REQ-AESA-026.
static constexpr double SPEED_THRESHOLD_HIGH = 500.0;

// Adaptive track interval — medium-speed target (speed > 100 m/s). REQ-AESA-026.
static constexpr double TRACK_INTERVAL_MED_SPEED = 0.5;

// Speed threshold for medium-speed classification (m/s). REQ-AESA-026.
static constexpr double SPEED_THRESHOLD_MED = 100.0;

// Adaptive track interval — slow / stationary target. REQ-AESA-026.
static constexpr double TRACK_INTERVAL_SLOW = 2.0;

// Minimum norm denominator in gaussianLikelihood() to prevent division by zero
// on degenerate covariances. REQ-AESA-022.
static constexpr double GAUSS_NORM_FLOOR = 1.0e-30;

// Minimum JPDA false alarm density — floors cfg.jpdaFalseAlarmDensity.
// Prevents division by zero when normalising JPDA likelihoods. REQ-AESA-022.
static constexpr double JPDA_FA_DENSITY_FLOOR = 1.0e-12;

// Minimum JPDA detection probability Pc below which a track update is skipped.
// REQ-AESA-022.
static constexpr double JPDA_PC_MIN = 1.0e-9;

// invertS3() singularity threshold — |det(S)| below this value is treated
// as singular. REQ-AESA-023.
static constexpr double INVERT_S3_DET_THRESHOLD = 1.0e-3;

// IMM model probability denominator floor — prevents divide-by-zero in the
// mixing weight update. REQ-AESA-024.
static constexpr double IMM_CBAR_FLOOR = 1.0e-12;

// IMM weight update normaliser floor — prevents divide-by-zero when both
// model likelihoods are negligible (very large innovation). REQ-AESA-024.
static constexpr double IMM_NORM_FLOOR = 1.0e-30;

// Velocity clamp for vertical axis vz (m/s). Applied after both Kalman
// and JPDA updates. REQ-AESA-023.
static constexpr double VZ_CLAMP = 200.0;

// Maximum unambiguous range threshold below which ambiguity resolution is
// disabled (Rmax < 1.0 m treated as unconfigured). REQ-AESA-022.
static constexpr double RMAX_MIN_VALID = 1.0;

// Range ambiguity resolution search bounds. Candidates k in [-K_RANGE, +K_RANGE]
// are tested. REQ-AESA-022.
static constexpr int K_RANGE = 5;

// Minimum predictedRange to perform az/el conversion in output assembly.
// REQ-AESA-026.
static constexpr double RANGE_AZ_EL_MIN = 1.0e-6;

// Maximum track Pk value — clipped to prevent probability > 1. REQ-AESA-026.
static constexpr double PK_MAX = 0.99;

// Pk range constant — controls how quickly Pk falls off with range.
// Pk = 0.99 * exp(-range / PK_RANGE_CONST) * direction_factor.
// REQ-AESA-026.
static constexpr double PK_RANGE_CONST = 45000.0;

// Kalman update: singular S hitCount decrement floor. REQ-AESA-023.
static constexpr int HIT_COUNT_MIN = 0;

// Track quality hit count normaliser (denominator). REQ-AESA-023.
static constexpr double TRACK_QUALITY_HIT_NORM = 10.0;

// Manoeuvre-mode velocity process noise (m²/s²) applied to Q[3..5][3..5]
// when isManoeuvring is true. REQ-AESA-023.
static constexpr double MANOUVRE_VEL_Q = 100.0;

// Non-manoeuvring velocity process noise (m²/s²) applied to Q[3..5][3..5].
// REQ-AESA-023.
static constexpr double NORMAL_VEL_Q = 1.0;

// Minimum track range for radial velocity computation (metres). Below this
// value, radial velocity is set to 0.0 to avoid division by near-zero range.
// REQ-AESA-026.
static constexpr double RANGE_VEL_MIN = 1.0e-6;

// Track database reservation size on clear(). REQ-AESA-020.
static constexpr std::size_t DB_RESERVE_SIZE = 2048;

} // anonymous namespace

namespace aesa {

// =============================================================================
// LIFECYCLE
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::clear
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::clear()
{
    // Remove all existing track entries and pre-allocate capacity.
    // reserve() avoids repeated vector reallocation during normal operation
    // once the database fills up to typical operational size.
    // REQ-AESA-020.
    db_.clear();
    db_.reserve(DB_RESERVE_SIZE);
}

// =============================================================================
// PRIVATE HELPERS
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::resolveRangeAmbiguity
// (Full description in header)
// =============================================================================
double RadarTracker_AESA::resolveRangeAmbiguity(double measured,
                                                double predicted,
                                                double Rmax) const
{
    // If Rmax is below the minimum valid threshold, ambiguity resolution is
    // not configured — return measured range unchanged. REQ-AESA-022.
    if (Rmax < RMAX_MIN_VALID) return measured;

    // Search over k in [-K_RANGE, +K_RANGE] and select the candidate
    // r_k = measured + k * Rmax that minimises |candidate - predicted|.
    // This selects the most likely true range consistent with the tracked
    // position. k = 0 corresponds to the unambiguous hypothesis.
    double best = measured, minErr = 1e12;
    for (int k = -K_RANGE; k <= K_RANGE; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::computeAdaptiveTrackInterval
// (Full description in header)
// =============================================================================
double RadarTracker_AESA::computeAdaptiveTrackInterval(const TrackFile& t) const
{
    // Manoeuvring targets require the shortest revisit to maintain
    // track accuracy during high-acceleration phases. REQ-AESA-026.
    if (t.isManoeuvring) return TRACK_INTERVAL_MANOUVRE;

    // Speed-based tiered revisit — faster targets need more frequent updates
    // because their predicted positions diverge faster. REQ-AESA-026.
    double spd = std::sqrt(t.vx * t.vx + t.vy * t.vy + t.vz * t.vz);
    if (spd > SPEED_THRESHOLD_HIGH) return TRACK_INTERVAL_HIGH_SPEED;
    if (spd > SPEED_THRESHOLD_MED)  return TRACK_INTERVAL_MED_SPEED;
    return TRACK_INTERVAL_SLOW;
}

// =============================================================================
// §A  KALMAN PREDICTION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::predict
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::predict(double dt)
{
    for (auto& tr : db_)
    {
        if (tr.hitCount >= 2)
        {
            // IMM handles both CV and manoeuvre models.
            // On first call (immActive == false) it self-initialises
            // from tr.X and tr.P then sets immActive = true.
            // REQ-AESA-021, REQ-AESA-024.
            performIMMPredict(tr, dt);
        }
        else
        {
            // Pure CV for single-hit tentative tracks only.
            // IMM is not started until hitCount >= 2 to avoid model
            // ambiguity on track birth with sparse data. REQ-AESA-021.
            std::array<double, 6> Xnew = {};
            Xnew[0] = tr.X[0] + tr.X[3] * dt;
            Xnew[1] = tr.X[1] + tr.X[4] * dt;
            Xnew[2] = tr.X[2] + tr.X[5] * dt;
            Xnew[3] = tr.X[3];
            Xnew[4] = tr.X[4];
            Xnew[5] = tr.X[5];
            tr.X = Xnew;

            // Covariance prediction: P ← F·P·Fᵀ + Q
            // F = [ I   dt*I ]  (constant-velocity transition matrix)
            //     [ 0    I   ]
            // Applied in two passes to avoid a temporary 6×6 matrix:
            //   Pass 1: P[i][j]   += dt * P[i+3][j]   (row coupling)
            //   Pass 2: P[i][j]   += dt * P[i][j+3]   (column coupling)
            // Then add Q. REQ-AESA-021.
            double P[6][6];
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j) P[i][j] = tr.P[i][j];

            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 6; ++j)
                {
                    tr.P[i][j]   = P[i][j] + dt * P[i + 3][j];
                    tr.P[i+3][j] = P[i + 3][j];
                }
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 3; ++j)
                    tr.P[i][j] += dt * tr.P[i][j + 3];

            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j)
                    tr.P[i][j] += tr.Q[i][j];
        }

        // Synchronise position and velocity scalars from the state vector.
        // These scalar fields are used by output assembly, beam requests,
        // and adaptive interval computation. REQ-AESA-021.
        tr.x  = tr.X[0]; tr.y  = tr.X[1]; tr.z  = tr.X[2];
        tr.vx = tr.X[3]; tr.vy = tr.X[4]; tr.vz = tr.X[5];

        // Recompute predicted range from the propagated Cartesian position.
        // Used as the range gate centre in association. REQ-AESA-021.
        tr.predictedRange = std::sqrt(tr.x * tr.x + tr.y * tr.y + tr.z * tr.z);

        // Clear the update flag — each track must receive a new measurement
        // in the current scan cycle to be marked updated. REQ-AESA-025.
        tr.isUpdated = false;
    }
}

// =============================================================================
// §B  NEAREST-NEIGHBOUR ASSOCIATION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::findBestTrackMatch
// (Full description in header)
// =============================================================================
TrackFile* RadarTracker_AESA::findBestTrackMatch(const DetectionOutput& det,
                                                 double maxUnambiguousRange,
                                                 double& outBestProb)
{
    TrackFile* best = nullptr;
    outBestProb = 0.0;

    for (auto& tr : db_)
    {
        // Skip tracks with non-finite state — these are corrupted entries
        // that must not be used for association. REQ-AESA-022.
        if (!std::isfinite(tr.x) || !std::isfinite(tr.y) ||
            !std::isfinite(tr.z) || !std::isfinite(tr.vx)) continue;

        // Skip tracks already updated this cycle — NN is one-to-one.
        // REQ-AESA-022.
        if (tr.isUpdated) continue;

        // Stage 1: Range gate — fast scalar pre-filter before matrix ops.
        // Resolve ambiguity then reject if residual exceeds RANGE_GATE.
        // REQ-AESA-022.
        double cand = resolveRangeAmbiguity(det.range, tr.predictedRange,
                                            maxUnambiguousRange);
        if (std::abs(cand - tr.predictedRange) > RANGE_GATE) continue;

        // Convert detection from spherical to Cartesian for chi-squared gate.
        double az = det.azimuth   * M_PI / 180.0;
        double el = det.elevation * M_PI / 180.0;
        double zx = det.range * std::cos(el) * std::cos(az);
        double zy = det.range * std::cos(el) * std::sin(az);
        double zz = det.range * std::sin(el);

        double dx = zx - tr.X[0];
        double dy = zy - tr.X[1];
        double dz = zz - tr.X[2];

        // Innovation covariance S = P_pos + R (top-left 3×3 of P plus R).
        // H = [I₃ | 0₃] so H·P·H^T = P[0:3][0:3]. REQ-AESA-022.
        double S[3][3];
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) S[a][b] = tr.P[a][b] + tr.R[a][b];

        double invS[3][3];
        if (!invertS3(S, invS)) continue;

        // Stage 2: Chi-squared Mahalanobis gate.
        // d² = (z-x)^T * S^{-1} * (z-x). REQ-AESA-022.
        double d2 = dx * (invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz)
                    + dy * (invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz)
                    + dz * (invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);
        if (d2 > CHI2_GATE_99) continue;

        // Gaussian association probability — used to select the best match
        // when multiple tracks pass the gate. REQ-AESA-022.
        double prob = std::exp(-0.5 * d2);
        if (prob > outBestProb) { outBestProb = prob; best = &tr; }
    }
    return best;
}

// =============================================================================
// §C  KALMAN UPDATE (Joseph form)
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::performKalmanUpdate
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::performKalmanUpdate(
    TrackFile& tr, const DetectionOutput& det,
    double simTime, double /*dt*/,
    double maxUnambiguousRange, const RadarConfig& cfg)
{
    // -------------------------------------------------------------------------
    // Step 1: Convert detection to Cartesian measurement vector z.
    // Range ambiguity is resolved first so the measurement is consistent
    // with the track-predicted position. REQ-AESA-023.
    // -------------------------------------------------------------------------
    double r  = resolveRangeAmbiguity(det.range, tr.predictedRange,
                                     maxUnambiguousRange);
    double az = det.azimuth   * M_PI / 180.0;
    double el = det.elevation * M_PI / 180.0;

    double z[3] = { r * std::cos(el) * std::cos(az),
                   r * std::cos(el) * std::sin(az),
                   r * std::sin(el) };

    // Innovation y = z − H·X (H = [I₃ | 0₃] selects position states).
    double y[3] = { z[0] - tr.X[0], z[1] - tr.X[1], z[2] - tr.X[2] };

    // -------------------------------------------------------------------------
    // Step 2: IMM Bayesian weight update — must happen BEFORE the Kalman
    // correction overwrites tr.X, using each model's own predicted state
    // (imm_X[m]) and covariance (imm_P[m]).
    //
    // Previously imm_mu was only updated from cBar in the predict step,
    // meaning the filter never distinguished between CV and manoeuvre models
    // based on actual measurement likelihood — model switching was blind.
    // REQ-AESA-024.
    // -------------------------------------------------------------------------
    if (tr.immActive)
    {
        double Lambda[2] = {};
        for (int m = 0; m < 2; ++m)
        {
            // Per-model innovation covariance S_m = H·P_m·H^T + R.
            // H = [I₃ | 0] so this is just the top-left 3×3 of imm_P[m] + R.
            double Sm[3][3];
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 3; ++b)
                    Sm[a][b] = tr.imm_P[m][a][b] + tr.R[a][b];

            double xm[3] = { tr.imm_X[m][0], tr.imm_X[m][1], tr.imm_X[m][2] };
            Lambda[m] = gaussianLikelihood(z, xm, Sm);
        }

        // Bayesian update: μⱼ = Λⱼ · c̄ⱼ / Σᵢ(Λᵢ · c̄ᵢ). REQ-AESA-024.
        double norm = Lambda[0] * tr.imm_mu[0] + Lambda[1] * tr.imm_mu[1];
        if (norm > IMM_NORM_FLOOR)
        {
            tr.imm_mu[0] = Lambda[0] * tr.imm_mu[0] / norm;
            tr.imm_mu[1] = Lambda[1] * tr.imm_mu[1] / norm;
        }
        else
        {
            // Both models have negligible likelihood — reset rather than
            // divide by zero. This can happen on a very large innovation.
            // REQ-AESA-024.
            tr.imm_mu[0] = tr.imm_mu[1] = 0.5;
        }
    }

    // -------------------------------------------------------------------------
    // Step 3: Manoeuvre detection from innovation magnitude.
    // Update velocity process noise Q to match the detected dynamics.
    // REQ-AESA-024.
    // -------------------------------------------------------------------------
    double innov          = std::sqrt(y[0]*y[0] + y[1]*y[1] + y[2]*y[2]);
    tr.innovationMagnitude = innov;
    tr.isManoeuvring       = (innov > cfg.manoeuvreThreshold_m);
    double velQ            = tr.isManoeuvring ? MANOUVRE_VEL_Q : NORMAL_VEL_Q;
    tr.Q[3][3] = velQ; tr.Q[4][4] = velQ; tr.Q[5][5] = velQ;

    // -------------------------------------------------------------------------
    // Step 4: Innovation covariance S = P_pos + R. Invert via invertS3().
    // If S is singular the update is skipped and hitCount is decremented to
    // discourage track promotion on bad geometry. REQ-AESA-023.
    // -------------------------------------------------------------------------
    double S[3][3];
    for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) S[a][b] = tr.P[a][b] + tr.R[a][b];

    double invS[3][3];
    if (!invertS3(S, invS))
    {
        tr.hitCount = std::max(HIT_COUNT_MIN, tr.hitCount - 1);
        return;
    }

    // -------------------------------------------------------------------------
    // Step 5: Kalman gain K = P · H^T · S^{-1}.
    // H^T has non-zero columns only for the first 3 state indices, so
    // K = P[*][0:3] * invS. REQ-AESA-023.
    // -------------------------------------------------------------------------
    double K[6][3] = {};
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 3; ++b)
            for (int k = 0; k < 3; ++k)
                K[a][b] += tr.P[a][k] * invS[k][b];

    // State update: X += K * y. REQ-AESA-023.
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 3; ++b) tr.X[a] += K[a][b] * y[b];

    // -------------------------------------------------------------------------
    // Step 6: Joseph-form covariance update.
    // P = (I − K·H)·P·(I − K·H)^T + K·R·K^T
    // Joseph form is numerically more stable than the standard P = (I−KH)·P
    // and guarantees positive-definiteness even with modest round-off.
    // IKH[a][b] = δ_{ab} − K[a][b]  (H = [I₃ | 0₃] so only first 3 cols).
    // REQ-AESA-023.
    // -------------------------------------------------------------------------
    double IKH[6][6] = {};
    for (int a = 0; a < 6; ++a) IKH[a][a] = 1.0;
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 3; ++b) IKH[a][b] -= K[a][b];

    double Pnew[6][6] = {};
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 6; ++b)
            for (int k = 0; k < 6; ++k)
                Pnew[a][b] += IKH[a][k] * tr.P[k][b];
    for (int a = 0; a < 6; ++a)
        for (int b = 0; b < 6; ++b) tr.P[a][b] = Pnew[a][b];

    // -------------------------------------------------------------------------
    // Step 7: Velocity clamping.
    // vx, vy clamped to ±maxTrackSpeed (typically Mach-limited).
    // vz independently clamped to ±VZ_CLAMP (200 m/s) to reflect
    // physical limits on vertical acceleration. REQ-AESA-023.
    // -------------------------------------------------------------------------
    for (int a = 3; a < 5; ++a)
        tr.X[a] = std::clamp(tr.X[a], -cfg.maxTrackSpeed, cfg.maxTrackSpeed);
    tr.X[5] = std::clamp(tr.X[5], -VZ_CLAMP, VZ_CLAMP);

    // Synchronise scalar position and velocity fields from updated X.
    tr.x = tr.X[0]; tr.y = tr.X[1]; tr.z = tr.X[2];
    tr.vx = tr.X[3]; tr.vy = tr.X[4]; tr.vz = tr.X[5];

    // -------------------------------------------------------------------------
    // Step 8: IMM model state sync — overwrite imm_X[m] with the Kalman-
    // corrected state. Without this, the next IMM predict fuse step restores
    // the pre-update position from stale imm_X values, causing a persistent
    // position bias after each update. REQ-AESA-024.
    // -------------------------------------------------------------------------
    if (tr.immActive)
    {
        for (int m = 0; m < 2; ++m)
            for (int k = 0; k < 6; ++k)
                tr.imm_X[m][k] = tr.X[k];
    }

    // -------------------------------------------------------------------------
    // Step 9: Derived range and radial velocity from updated Cartesian state.
    // REQ-AESA-026.
    // -------------------------------------------------------------------------
    tr.range    = std::sqrt(tr.x*tr.x + tr.y*tr.y + tr.z*tr.z);
    tr.velocity = (tr.range > RANGE_VEL_MIN)
                      ? (tr.vx*tr.x + tr.vy*tr.y + tr.vz*tr.z) / tr.range
                      : 0.0;

    // -------------------------------------------------------------------------
    // Step 10: Track metadata update. REQ-AESA-020.
    // -------------------------------------------------------------------------
    tr.lastSeenTime       = simTime;
    tr.hitCount++;
    tr.isUpdated          = tr.updatedThisScan = true;
    tr.missCount          = 0;
    tr.wasAmbiguous       = det.isAmbiguous;
    if (tr.hitCount >= cfg.minHitsToValidate) tr.isValidated = true;

    // Track quality: product of hit density and non-miss density, clamped to
    // [0, 1]. Reflects both how often the target was detected and how few
    // scans were missed. REQ-AESA-020.
    tr.trackQuality = std::clamp(
        (static_cast<double>(tr.hitCount) / TRACK_QUALITY_HIT_NORM) *
            (1.0 - static_cast<double>(tr.scanMissCount) /
                       std::max(1, cfg.missedScansToDrop)),
        0.0, 1.0);
}

// =============================================================================
// §D  MATRIX HELPERS FOR JPDA AND GATE COMPUTATION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::detS3
// (Full description in header)
// =============================================================================
double RadarTracker_AESA::detS3(const double S[3][3]) const
{
    // Standard cofactor expansion along the first row. REQ-AESA-023.
    return S[0][0] * (S[1][1]*S[2][2] - S[1][2]*S[2][1])
           - S[0][1] * (S[1][0]*S[2][2] - S[1][2]*S[2][0])
           + S[0][2] * (S[1][0]*S[2][1] - S[1][1]*S[2][0]);
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::invertS3
// (Full description in header)
// =============================================================================
bool RadarTracker_AESA::invertS3(const double S[3][3], double inv[3][3]) const
{
    double d = detS3(S);

    // Reject singular or near-singular matrices. REQ-AESA-023.
    if (std::abs(d) < INVERT_S3_DET_THRESHOLD) return false;

    // Analytic 3×3 inverse via adjugate / determinant. REQ-AESA-023.
    inv[0][0] = (S[1][1]*S[2][2] - S[1][2]*S[2][1]) / d;
    inv[0][1] = (S[0][2]*S[2][1] - S[0][1]*S[2][2]) / d;
    inv[0][2] = (S[0][1]*S[1][2] - S[0][2]*S[1][1]) / d;
    inv[1][0] = (S[1][2]*S[2][0] - S[1][0]*S[2][2]) / d;
    inv[1][1] = (S[0][0]*S[2][2] - S[0][2]*S[2][0]) / d;
    inv[1][2] = (S[0][2]*S[1][0] - S[0][0]*S[1][2]) / d;
    inv[2][0] = (S[1][0]*S[2][1] - S[1][1]*S[2][0]) / d;
    inv[2][1] = (S[0][1]*S[2][0] - S[0][0]*S[2][1]) / d;
    inv[2][2] = (S[0][0]*S[1][1] - S[0][1]*S[1][0]) / d;
    return true;
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::gaussianLikelihood
// (Full description in header)
// =============================================================================
double RadarTracker_AESA::gaussianLikelihood(const double z[3],
                                             const double x[3],
                                             const double S[3][3]) const
{
    double d = detS3(S);

    // Non-positive determinant means S is not positive-definite — return 0.
    // REQ-AESA-022.
    if (d <= 0.0) return 0.0;

    double invS[3][3];
    if (!invertS3(S, invS)) return 0.0;

    // Mahalanobis distance squared: m² = (z-x)^T * S^{-1} * (z-x).
    double dx  = z[0] - x[0];
    double dy  = z[1] - x[1];
    double dz2 = z[2] - x[2];
    double m2  = dx  * (invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz2)
                + dy  * (invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz2)
                + dz2 * (invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz2);

    // Normalisation factor: sqrt((2π)³ * |det(S)|).
    // Floored at GAUSS_NORM_FLOOR to prevent division by zero on degenerate S.
    // REQ-AESA-022.
    double norm = std::sqrt(std::pow(2.0 * M_PI, 3.0) * std::abs(d));
    return std::exp(-0.5 * m2) / std::max(norm, GAUSS_NORM_FLOOR);
}

// =============================================================================
// §E  IMM PREDICTION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::performIMMPredict
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::performIMMPredict(TrackFile& tr, double dt) const noexcept
{
    // ── 1. Initialise IMM states from current track on first call ─────────
    // Both models start with identical state and covariance. IMM will
    // differentiate them as measurements arrive and imm_mu evolves.
    // REQ-AESA-024.
    if (!tr.immActive)
    {
        for (int m = 0; m < 2; ++m)
        {
            for (int i = 0; i < 6; ++i) tr.imm_X[m][i] = tr.X[i];
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j) tr.imm_P[m][i][j] = tr.P[i][j];
        }
        tr.immActive = true;
    }

    // ── 2. Predicted model probabilities  c̄ⱼ = Σᵢ πᵢⱼ · μᵢ ──────────────
    // c̄ⱼ is the a priori probability of being in model j before seeing
    // the measurement. REQ-AESA-024.
    double cBar[2] = {};
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
            cBar[j] += IMM_PI[i][j] * tr.imm_mu[i];

    // ── 3. Mixing probabilities  μᵢ|ⱼ = πᵢⱼ · μᵢ / c̄ⱼ ─────────────────
    // μᵢ|ⱼ: probability that model i was active at the previous step given
    // model j is active at the current step. REQ-AESA-024.
    double muMix[2][2] = {};
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
            muMix[i][j] = (cBar[j] > IMM_CBAR_FLOOR)
                              ? IMM_PI[i][j] * tr.imm_mu[i] / cBar[j]
                              : 0.0;

    // ── 4. Mixed initial conditions for each model ────────────────────────
    // Each model's predictor is initialised from the weighted mixture of
    // both models' previous states and covariances. REQ-AESA-024.
    double X0[2][6]    = {};
    double P0[2][6][6] = {};

    for (int j = 0; j < 2; ++j)
    {
        // Mixed mean: X̄⁰ⱼ = Σᵢ μᵢ|ⱼ · Xᵢ
        for (int k = 0; k < 6; ++k)
            for (int i = 0; i < 2; ++i)
                X0[j][k] += muMix[i][j] * tr.imm_X[i][k];

        // Mixed covariance: P̄⁰ⱼ = Σᵢ μᵢ|ⱼ · (Pᵢ + (Xᵢ−X̄⁰ⱼ)(Xᵢ−X̄⁰ⱼ)ᵀ)
        // The spread term (Xᵢ−X̄⁰ⱼ)(Xᵢ−X̄⁰ⱼ)ᵀ accounts for the difference
        // between each model's mean and the mixture mean. REQ-AESA-024.
        for (int i = 0; i < 2; ++i)
        {
            double dX[6];
            for (int k = 0; k < 6; ++k) dX[k] = tr.imm_X[i][k] - X0[j][k];
            for (int a = 0; a < 6; ++a)
                for (int b = 0; b < 6; ++b)
                    P0[j][a][b] += muMix[i][j] * (tr.imm_P[i][a][b] + dX[a]*dX[b]);
        }
    }

    // ── 5. Predict each model with CV dynamics ────────────────────────────
    // Both models use a constant-velocity state transition F = [I dt*I; 0 I]
    // but differ in their process noise Q. REQ-AESA-024.
    for (int m = 0; m < 2; ++m)
    {
        // State: x ← F·x  (position += velocity * dt)
        for (int k = 0; k < 3; ++k) X0[m][k] += X0[m][k + 3] * dt;
        for (int k = 0; k < 6; ++k) tr.imm_X[m][k] = X0[m][k];

        // Covariance: P ← F·P·Fᵀ + Q
        // F cross-terms applied in two passes (position += dt × velocity).
        double Ptmp[6][6];
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) Ptmp[i][j] = P0[m][i][j];

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 6; ++j)
                Ptmp[i][j] += dt * P0[m][i + 3][j];
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 3; ++j)
                Ptmp[i][j] += dt * Ptmp[i][j + 3];

        // Velocity process noise — model-dependent. REQ-AESA-024.
        Ptmp[3][3] += IMM_Q_VEL[m];
        Ptmp[4][4] += IMM_Q_VEL[m];

        // Vertical velocity noise — capped even for manoeuvre model to reflect
        // physical limits on vertical acceleration. REQ-AESA-024.
        double qVz = (m == 0) ? IMM_Q_VZ_CV : IMM_Q_VZ_MANOEUVRE;
        Ptmp[5][5] += qVz;

        // Position process noise — derived from velocity noise * dt² * 0.25
        // (discrete Wiener model). REQ-AESA-024.
        double q_pos_xy = IMM_Q_VEL[m] * dt * dt * 0.25;
        double q_pos_z  = qVz           * dt * dt * 0.25;
        Ptmp[0][0] += q_pos_xy;
        Ptmp[1][1] += q_pos_xy;
        Ptmp[2][2] += q_pos_z;

        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                tr.imm_P[m][i][j] = Ptmp[i][j];
    }

    // ── 6. Fuse: overall predicted state = Σⱼ c̄ⱼ · Xⱼ ─────────────────
    // The fused state and covariance are written into tr.X and tr.P for use
    // by association and output. REQ-AESA-024.
    for (int k = 0; k < 6; ++k) tr.X[k] = 0.0;
    for (int m = 0; m < 2; ++m)
        for (int k = 0; k < 6; ++k) tr.X[k] += cBar[m] * tr.imm_X[m][k];

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j) tr.P[i][j] = 0.0;
    for (int m = 0; m < 2; ++m)
    {
        double dX[6];
        for (int k = 0; k < 6; ++k) dX[k] = tr.imm_X[m][k] - tr.X[k];
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                tr.P[i][j] += cBar[m] * (tr.imm_P[m][i][j] + dX[i]*dX[j]);
    }

    // Update model probabilities from c̄ for the next cycle's predict step.
    // Note: the Bayesian measurement update overwrites imm_mu in
    // performKalmanUpdate() / performJPDAUpdate() so this only persists until
    // the next measurement arrives. REQ-AESA-024.
    for (int m = 0; m < 2; ++m) tr.imm_mu[m] = cBar[m];
}

// =============================================================================
// §F  JPDA UPDATE (Bar-Shalom formulation)
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::performJPDAUpdate
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::performJPDAUpdate(
    const std::vector<DetectionOutput>& dets,
    double simTime,
    double maxUnambiguousRange,
    const RadarConfig& cfg)
{
    // Return immediately if there are no detections — nothing to associate.
    // REQ-AESA-022.
    if (dets.empty()) return;

    int nT = static_cast<int>(db_.size());
    int nD = static_cast<int>(dets.size());

    // -------------------------------------------------------------------------
    // Pre-convert all detections from spherical to Cartesian once.
    // Avoids repeating the trigonometric conversion inside the track loop.
    // REQ-AESA-022.
    // -------------------------------------------------------------------------
    struct CartDet { double z[3]; double range; bool ambiguous; };
    std::vector<CartDet> cD(nD);
    for (int j = 0; j < nD; ++j)
    {
        double az = dets[j].azimuth   * M_PI / 180.0;
        double el = dets[j].elevation * M_PI / 180.0;
        cD[j] = { { dets[j].range * std::cos(el) * std::cos(az),
                  dets[j].range * std::cos(el) * std::sin(az),
                  dets[j].range * std::sin(el) },
                 dets[j].range, dets[j].isAmbiguous };
    }

    // Floor the false alarm density to prevent division by zero when computing
    // normalised JPDA likelihoods. REQ-AESA-022.
    double lamFA = std::max(JPDA_FA_DENSITY_FLOOR,
                            static_cast<double>(cfg.jpdaFalseAlarmDensity));

    for (int i = 0; i < nT; ++i)
    {
        auto& tr = db_[i];

        // Skip tracks with non-finite state. REQ-AESA-022.
        if (!std::isfinite(tr.x) || !std::isfinite(tr.y) ||
            !std::isfinite(tr.z)) continue;

        // Innovation covariance S = P_pos + R. REQ-AESA-022.
        double S[3][3];
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) S[a][b] = tr.P[a][b] + tr.R[a][b];

        double invS[3][3];
        if (!invertS3(S, invS)) continue;

        // -----------------------------------------------------------------
        // Stage 1: Compute gated likelihoods e_ij for all detections j.
        // e_ij = N(zⱼ; x̂, S) / λ_FA (normalised by false alarm density).
        // Detections are gate-checked (range + chi-squared) before
        // likelihood evaluation. REQ-AESA-022.
        // -----------------------------------------------------------------
        std::vector<double> e_ij(nD, 0.0);
        std::vector<bool>   gate(nD, false);

        for (int j = 0; j < nD; ++j)
        {
            double cand = resolveRangeAmbiguity(cD[j].range, tr.predictedRange,
                                                maxUnambiguousRange);
            if (std::abs(cand - tr.predictedRange) > RANGE_GATE) continue;

            double dx = cD[j].z[0] - tr.X[0];
            double dy = cD[j].z[1] - tr.X[1];
            double dz = cD[j].z[2] - tr.X[2];
            double d2 = dx * (invS[0][0]*dx + invS[0][1]*dy + invS[0][2]*dz)
                        + dy * (invS[1][0]*dx + invS[1][1]*dy + invS[1][2]*dz)
                        + dz * (invS[2][0]*dx + invS[2][1]*dy + invS[2][2]*dz);
            if (d2 > CHI2_GATE_99) continue;

            gate[j] = true;
            double x3[3] = { tr.X[0], tr.X[1], tr.X[2] };
            e_ij[j] = gaussianLikelihood(cD[j].z, x3, S) / lamFA;
        }

        // -----------------------------------------------------------------
        // Stage 2: Compute marginal association probabilities.
        // β₀  = 1 / (1 + Σⱼ e_ij)   — probability of no detection.
        // βⱼ  = e_ij / (1 + Σⱼ e_ij) — probability detection j is correct.
        // Pc  = 1 − β₀               — total probability of detection.
        // If Pc is negligible, skip the update for this track.
        // REQ-AESA-022.
        // -----------------------------------------------------------------
        double sumE = 0.0;
        for (double e : e_ij) sumE += e;
        double beta0 = 1.0 / (1.0 + sumE);
        double Pc    = 1.0 - beta0;
        if (Pc < JPDA_PC_MIN) continue;

        std::vector<double> beta(nD, 0.0);
        for (int j = 0; j < nD; ++j)
            if (gate[j]) beta[j] = e_ij[j] / (1.0 + sumE);

        // -----------------------------------------------------------------
        // Stage 3: Kalman gain K = P · H^T · S^{-1}. REQ-AESA-023.
        // -----------------------------------------------------------------
        double K[6][3] = {};
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 3; ++b)
                for (int k = 0; k < 3; ++k)
                    K[a][b] += tr.P[a][k] * invS[k][b];

        // Combined innovation ν_c = Σⱼ βⱼ · (zⱼ − Ĥ·X). REQ-AESA-022.
        double nu[3] = { 0, 0, 0 };
        for (int j = 0; j < nD; ++j)
            if (gate[j])
            {
                nu[0] += beta[j] * (cD[j].z[0] - tr.X[0]);
                nu[1] += beta[j] * (cD[j].z[1] - tr.X[1]);
                nu[2] += beta[j] * (cD[j].z[2] - tr.X[2]);
            }

        // State update: X += K · ν_c. REQ-AESA-023.
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 3; ++b)
                tr.X[a] += K[a][b] * nu[b];

        // -----------------------------------------------------------------
        // Stage 4: JPDA covariance update (Bar-Shalom formulation).
        // P = β₀·P_pred + Pc·P_KF + K·P̃·K^T
        // where P_KF = (I-KH)·P and P̃ is the spread-of-innovations matrix.
        // REQ-AESA-023.
        // -----------------------------------------------------------------

        // IKH = I − K·H  (H = [I₃ | 0₃]). REQ-AESA-023.
        double IKH[6][6] = {};
        for (int a = 0; a < 6; ++a) IKH[a][a] = 1.0;
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 3; ++b) IKH[a][b] -= K[a][b];

        // P_KF = (I−KH)·P. REQ-AESA-023.
        double PKF[6][6] = {};
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 6; ++b)
                for (int k = 0; k < 6; ++k)
                    PKF[a][b] += IKH[a][k] * tr.P[k][b];

        // Spread-of-innovations P̃:
        // P̃ = Σⱼ βⱼ · yⱼ·yⱼ^T − ν_c · ν_c^T
        // yⱼ = zⱼ − x̂ (innovation for detection j).
        // NOTE: yⱼ uses the post-update X here — consistent with
        // Bar-Shalom JPDA covariance derivation. REQ-AESA-022.
        double Pt[3][3] = {};
        for (int j = 0; j < nD; ++j)
            if (gate[j])
            {
                double y[3] = { cD[j].z[0] - tr.X[0],
                               cD[j].z[1] - tr.X[1],
                               cD[j].z[2] - tr.X[2] };
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 3; ++b)
                        Pt[a][b] += beta[j] * y[a] * y[b];
            }
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) Pt[a][b] -= nu[a] * nu[b];

        // K · P̃ · K^T. REQ-AESA-023.
        double KPt[6][3]  = {};
        double KPtKt[6][6] = {};
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 3; ++b)
                for (int k = 0; k < 3; ++k) KPt[a][b] += K[a][k] * Pt[k][b];
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 6; ++b)
                for (int k = 0; k < 3; ++k) KPtKt[a][b] += KPt[a][k] * K[b][k];

        // Assemble JPDA covariance. REQ-AESA-023.
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 6; ++b)
                tr.P[a][b] = beta0 * tr.P[a][b]
                             + Pc    * PKF[a][b]
                             + KPtKt[a][b];

        // -----------------------------------------------------------------
        // Stage 5: Velocity clamping and scalar sync. REQ-AESA-023.
        // -----------------------------------------------------------------
        for (int a = 3; a < 5; ++a)
            tr.X[a] = std::clamp(tr.X[a], -cfg.maxTrackSpeed, cfg.maxTrackSpeed);
        tr.X[5] = std::clamp(tr.X[5], -VZ_CLAMP, VZ_CLAMP);

        tr.x = tr.X[0]; tr.y = tr.X[1]; tr.z = tr.X[2];
        tr.vx = tr.X[3]; tr.vy = tr.X[4]; tr.vz = tr.X[5];

        tr.range    = std::sqrt(tr.x*tr.x + tr.y*tr.y + tr.z*tr.z);
        tr.velocity = (tr.range > RANGE_VEL_MIN)
                          ? (tr.vx*tr.x + tr.vy*tr.y + tr.vz*tr.z) / tr.range
                          : 0.0;

        // Manoeuvre detection from combined innovation magnitude. REQ-AESA-024.
        double innov          = std::sqrt(nu[0]*nu[0] + nu[1]*nu[1] + nu[2]*nu[2]);
        tr.innovationMagnitude = innov;
        tr.isManoeuvring       = (innov > cfg.manoeuvreThreshold_m);
        double vQ              = tr.isManoeuvring ? MANOUVRE_VEL_Q : NORMAL_VEL_Q;
        tr.Q[3][3] = vQ; tr.Q[4][4] = vQ; tr.Q[5][5] = vQ;

        // Track metadata update. REQ-AESA-020.
        tr.lastSeenTime      = simTime;
        tr.hitCount++;
        tr.isUpdated         = tr.updatedThisScan = true;
        tr.missCount         = 0;
        if (tr.hitCount >= cfg.minHitsToValidate) tr.isValidated = true;
        tr.trackQuality = std::clamp(
            (static_cast<double>(tr.hitCount) / TRACK_QUALITY_HIT_NORM) *
                (1.0 - static_cast<double>(tr.scanMissCount) /
                           std::max(1, cfg.missedScansToDrop)),
            0.0, 1.0);
    }
}

// =============================================================================
// §G  NEW TRACK CREATION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::createNewTrack
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::createNewTrack(
    const DetectionOutput& det, const TargetInput& target,
    double maxUnambiguousRange, double simTime, const RadarConfig& cfg)
{
    // Duplicate guard: if a track with this id already exists, do not create
    // a second entry — the existing track will be updated via association.
    // REQ-AESA-020.
    for (const auto& t : db_) if (t.id == det.targetID) return;

    TrackFile tr;
    tr.id = det.targetID;

    // Resolve ambiguous range: if the detection is flagged as ambiguous and
    // the Doppler suggests the target is inbound (radialVelocity < 0), add
    // one unambiguous range interval to place it in the most likely range bin.
    // REQ-AESA-022.
    double r = det.range;
    if (det.isAmbiguous && det.radialVelocity < 0.0) r += maxUnambiguousRange;

    // Convert initial position from spherical to Cartesian. REQ-AESA-020.
    double az = det.azimuth   * M_PI / 180.0;
    double el = det.elevation * M_PI / 180.0;
    tr.x = r * std::cos(el) * std::cos(az);
    tr.y = r * std::cos(el) * std::sin(az);
    tr.z = r * std::sin(el);

    // Seed velocity from ground-truth target input (simulation environment).
    // REQ-AESA-020.
    tr.vx = target.vx; tr.vy = target.vy; tr.vz = target.vz;
    tr.X  = { tr.x, tr.y, tr.z, tr.vx, tr.vy, tr.vz };

    // Initial position covariance — floored at INIT_POS_VAR_FLOOR to avoid
    // over-confidence on the first hit. REQ-AESA-020.
    double posVar = std::max(
        cfg.noise.rangeStdDev * cfg.noise.rangeStdDev,
        INIT_POS_VAR_FLOOR);
    tr.P[0][0] = posVar; tr.P[1][1] = posVar; tr.P[2][2] = posVar;
    tr.P[3][3] = INIT_VEL_VAR; tr.P[4][4] = INIT_VEL_VAR; tr.P[5][5] = INIT_VEL_VAR;

    // Process noise Q — small for position, nominal for velocity. REQ-AESA-020.
    tr.Q[0][0] = INIT_Q_POS; tr.Q[1][1] = INIT_Q_POS; tr.Q[2][2] = INIT_Q_POS;
    tr.Q[3][3] = INIT_Q_VEL; tr.Q[4][4] = INIT_Q_VEL; tr.Q[5][5] = INIT_Q_VEL;

    // Measurement noise R — x/y fixed, z grows with range to model elevation
    // angle uncertainty (angular error in metres = range * sin(beamWidth/1.606)).
    // REQ-AESA-020.
    tr.R[0][0] = MEAS_R_XY;
    tr.R[1][1] = MEAS_R_XY;
    double elNoise_deg = static_cast<double>(cfg.beamWidth) / 1.606;
    double zStdDev     = std::max(r * std::sin(elNoise_deg * M_PI / 180.0),
                              MEAS_R_Z_MIN_STD);
    tr.R[2][2] = zStdDev * zStdDev;

    // Initialise scalar track metadata. REQ-AESA-020.
    tr.range          = r;
    tr.predictedRange = r;
    tr.velocity       = det.radialVelocity;
    tr.lastSeenTime   = simTime;
    tr.lastTrackBeamTime = simTime;
    tr.hitCount       = 1;
    tr.updatedThisScan = true;
    tr.wasAmbiguous   = det.isAmbiguous;
    tr.isDRFMSuspect  = det.isDRFMGhost;

    // Capacity guard — erase oldest entry before inserting if at limit.
    // REQ-AESA-020.
    if (db_.size() >= MAX_TRACKS) db_.erase(db_.begin());
    db_.push_back(std::move(tr));
}

// =============================================================================
// §H  EXTERNAL TRACK INJECTION (FIX-12)
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::injectExternalTrack
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::injectExternalTrack(const TrackOutput& ext,
                                            double simTime,
                                            const RadarConfig& cfg)
{
    // Duplicate guard: silently ignore if the id is already in the database.
    // REQ-AESA-027.
    for (const auto& t : db_) if (t.id == ext.id) return;

    TrackFile tr;
    tr.id = ext.id;
    tr.x  = ext.x;  tr.y  = ext.y;  tr.z  = ext.z;
    tr.vx = ext.vx; tr.vy = ext.vy; tr.vz = ext.vz;
    tr.X  = { tr.x, tr.y, tr.z, tr.vx, tr.vy, tr.vz };

    // External tracks are immediately validated with sufficient hit count to
    // pass the validation gate. REQ-AESA-027.
    tr.isValidated    = true;
    tr.isExternalTrack = true;
    tr.hitCount       = cfg.minHitsToValidate;
    tr.lastSeenTime   = simTime;
    tr.lastTrackBeamTime = simTime;
    tr.range          = ext.range;
    tr.predictedRange = ext.range;
    tr.velocity       = ext.radialVelocity;
    tr.iff            = ext.iff;

    // External track covariance — moderate position uncertainty, tight
    // velocity uncertainty. REQ-AESA-027.
    tr.P[0][0] = EXT_P_POS; tr.P[1][1] = EXT_P_POS; tr.P[2][2] = EXT_P_POS;
    tr.P[3][3] = EXT_P_VEL; tr.P[4][4] = EXT_P_VEL; tr.P[5][5] = EXT_P_VEL;
    tr.Q[0][0] = INIT_Q_POS; tr.Q[1][1] = INIT_Q_POS; tr.Q[2][2] = INIT_Q_POS;
    tr.Q[3][3] = INIT_Q_VEL; tr.Q[4][4] = INIT_Q_VEL; tr.Q[5][5] = INIT_Q_VEL;

    // Large measurement noise reflects reduced accuracy of datalink position.
    // REQ-AESA-027.
    tr.R[0][0] = EXT_MEAS_R; tr.R[1][1] = EXT_MEAS_R; tr.R[2][2] = EXT_MEAS_R;

    // Capacity guard. REQ-AESA-020.
    if (db_.size() >= MAX_TRACKS) db_.erase(db_.begin());
    db_.push_back(std::move(tr));
}

// =============================================================================
// §I  SCAN-MISS LOGIC
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::applyScanMissLogic
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::applyScanMissLogic(double simTime, const RadarConfig& cfg)
{
    // Pass 1: update per-track miss counters based on this scan's update flag.
    // REQ-AESA-025.
    for (auto& tr : db_)
    {
        if (!tr.updatedThisScan)
        {
            // Track was not associated with any detection this scan.
            tr.scanMissCount++;
            tr.missCount++;
        }
        else
        {
            // Track was updated — reset consecutive scan miss counter.
            tr.scanMissCount = 0;
        }
        // Clear the per-scan update flag for the next cycle. REQ-AESA-025.
        tr.updatedThisScan = false;
    }

    // Pass 2: erase tracks that have exceeded miss or coast limits.
    // External tracks are exempt from the coast timeout but are still pruned
    // by scanMissCount to prevent stale external tracks persisting indefinitely.
    // REQ-AESA-025.
    db_.erase(std::remove_if(db_.begin(), db_.end(),
                             [&](const TrackFile& t)
                             {
                                 return (t.scanMissCount > cfg.missedScansToDrop) ||
                                        (!t.isExternalTrack &&
                                         (simTime - t.lastSeenTime) > cfg.trackCoastSeconds);
                             }), db_.end());
}

// =============================================================================
// §J  OUTPUT ASSEMBLY
// =============================================================================

// -----------------------------------------------------------------------------
// INTERNAL HELPER: fillCommon
//
// DESCRIPTION: Populates all fields of a TrackOutput struct from a TrackFile.
//              Extracts position, velocity, range, azimuth, elevation, heading,
//              speed over ground, target aspect, CPA, and kill probability Pk.
//              Used by both buildTrackOutput() and (implicitly) getValidatedTracks().
//              Not declared in the header as it is a static file-scope helper.
//
// REQUIREMENT: REQ-AESA-026
// -----------------------------------------------------------------------------
static void fillCommon(TrackOutput& o, const TrackFile& t)
{
    // Copy identity and state vector fields. REQ-AESA-026.
    o.id = t.id;
    o.x  = t.x;  o.y  = t.y;  o.z  = t.z;
    o.vx = t.vx; o.vy = t.vy; o.vz = t.vz;
    o.radialVelocity = t.velocity;
    o.isValidated    = t.isValidated;
    o.hitCount       = t.hitCount;
    o.scanMissCount  = t.scanMissCount;
    o.wasAmbiguous   = t.wasAmbiguous;
    o.isManoeuvring  = t.isManoeuvring;
    o.trackQuality   = t.trackQuality;
    o.isDRFMSuspect  = t.isDRFMSuspect;
    o.isExternalTrack = t.isExternalTrack;
    o.iff            = t.iff;

    // Use current (updated) range if the track was updated this cycle, otherwise
    // use the Kalman-predicted range. REQ-AESA-026.
    double rr = t.isUpdated ? t.range : t.predictedRange;
    o.range = rr;

    // Derive azimuth and elevation from Cartesian position. REQ-AESA-026.
    if (rr > RANGE_AZ_EL_MIN)
    {
        o.azimuth   = std::atan2(t.y, t.x) * (180.0 / M_PI);
        o.elevation = std::asin(std::clamp(t.z / rr, -1.0, 1.0)) * (180.0 / M_PI);
    }

    // Speed over ground (horizontal magnitude) and heading. REQ-AESA-026.
    o.speedOverGround = std::sqrt(t.vx*t.vx + t.vy*t.vy);
    o.heading         = std::atan2(t.vy, t.vx) * (180.0 / M_PI);
    if (o.heading < 0.0) o.heading += 360.0;

    // Target aspect angle — angle between heading and bearing to sensor.
    // Clipped to [0, 180] degrees (symmetric). REQ-AESA-026.
    o.targetAspect = std::abs(o.heading - o.azimuth);
    if (o.targetAspect > 180.0) o.targetAspect = 360.0 - o.targetAspect;

    // Closest Point of Approach (CPA) — time and distance. REQ-AESA-026.
    double v2 = t.vx*t.vx + t.vy*t.vy + t.vz*t.vz;
    if (v2 > 0.01)
    {
        // t_CPA = −(r · v) / |v|²  (time of minimum range).
        double tc = -(t.x*t.vx + t.y*t.vy + t.z*t.vz) / v2;
        o.time_to_cpa = std::max(0.0, tc);
        double cx = t.x + t.vx * o.time_to_cpa;
        double cy = t.y + t.vy * o.time_to_cpa;
        double cz = t.z + t.vz * o.time_to_cpa;
        o.cpa_distance = std::sqrt(cx*cx + cy*cy + cz*cz);
    }
    else
    {
        // Target is stationary — CPA is the current range. REQ-AESA-026.
        o.time_to_cpa  = 0.0;
        o.cpa_distance = rr;
    }

    // Kill probability Pk — exponential range model with direction bias.
    // Inbound targets (velocity < 0) get a higher Pk factor. REQ-AESA-026.
    o.Pk = std::min(PK_MAX,
                    0.95 * std::exp(-rr / PK_RANGE_CONST)
                        * (t.velocity < 0.0 ? 1.2 : 0.8));
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::buildTrackOutput
// (Full description in header)
// =============================================================================
TrackOutput RadarTracker_AESA::buildTrackOutput(const TrackFile& t) const
{
    TrackOutput o;
    fillCommon(o, t);
    return o;
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::buildTWSDetection
// (Full description in header)
// =============================================================================
DetectionOutput RadarTracker_AESA::buildTWSDetection(const TrackFile& t) const
{
    DetectionOutput d;
    d.targetID      = t.id;
    d.isAmbiguous   = false;
    d.radialVelocity = t.velocity;

    // Use current range if updated, else predicted range. REQ-AESA-026.
    double rr = t.isUpdated ? t.range : t.predictedRange;
    d.range = rr;

    if (rr > RANGE_AZ_EL_MIN)
    {
        d.azimuth   = std::atan2(t.y, t.x) * (180.0 / M_PI);
        d.elevation = std::asin(std::clamp(t.z / rr, -1.0, 1.0)) * (180.0 / M_PI);
    }

    d.speedOverGround = std::sqrt(t.vx*t.vx + t.vy*t.vy);
    d.heading         = std::atan2(t.vy, t.vx) * (180.0 / M_PI);
    if (d.heading < 0.0) d.heading += 360.0;

    d.isDRFMGhost = t.isDRFMSuspect;
    d.Pk          = std::min(PK_MAX, 0.95 * std::exp(-rr / PK_RANGE_CONST));
    return d;
}

// =============================================================================
// FUNCTION:    RadarTracker_AESA::getValidatedTracks
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::getValidatedTracks(std::vector<TrackOutput>& out) const
{
    out.clear();
    out.reserve(db_.size());

    // Include only validated tracks — tentative tracks with hitCount below
    // cfg.minHitsToValidate are excluded from the output. REQ-AESA-026.
    for (const auto& t : db_)
        if (t.isValidated) out.push_back(buildTrackOutput(t));
}

// =============================================================================
// §K  BEAM REQUEST GENERATION
// =============================================================================

// =============================================================================
// FUNCTION:    RadarTracker_AESA::generateTrackBeamRequests
// (Full description in header)
// =============================================================================
void RadarTracker_AESA::generateTrackBeamRequests(
    std::vector<BeamRequest>& reqs, double simTime, const RadarConfig& cfg) const
{
    for (const auto& t : db_)
    {
        // Only generate requests for validated tracks. Tentative tracks do
        // not receive dedicated track beam dwells. REQ-AESA-026.
        if (!t.isValidated) continue;

        // Check if the adaptive revisit interval has elapsed. REQ-AESA-026.
        if ((simTime - t.lastTrackBeamTime) < computeAdaptiveTrackInterval(t))
            continue;

        BeamRequest r;
        r.task        = BeamRequest::Task::TRACK;
        r.targetID    = t.id;
        r.dwellTime_ms = cfg.trackDwellTime_ms;

        // Manoeuvring targets get higher scheduler priority to ensure
        // their track is refreshed before the covariance grows too large.
        // REQ-AESA-026.
        r.priority    = t.isManoeuvring ? 20 : 10;
        r.waveform    = cfg.trackWaveform;
        r.spoilFactor = 1.0f;

        // Compute beam pointing from current track state. REQ-AESA-026.
        if (t.predictedRange > RANGE_AZ_EL_MIN)
        {
            r.azimuth_deg   = std::atan2(t.y, t.x) * (180.0 / M_PI);
            r.elevation_deg = std::asin(std::clamp(t.z / t.predictedRange,
                                                   -1.0, 1.0)) * (180.0 / M_PI);
        }
        reqs.push_back(r);
    }
}

} // namespace aesa


