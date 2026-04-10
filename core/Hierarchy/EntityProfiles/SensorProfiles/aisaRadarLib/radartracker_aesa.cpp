// radartracker_aesa.cpp  —  Rev 3
#include "radartracker_aesa.h"
#include <algorithm>
#include <cmath>
#include <QDebug>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr double RANGE_GATE    = 5000.0;
static constexpr double CHI2_GATE_99  = 9.21;
static constexpr std::size_t MAX_TRACKS = 2000;
static constexpr double IMM_PI[2][2]  = { { 0.95, 0.05 },
                                        { 0.10, 0.90 } };
static constexpr double IMM_Q_VEL[2]  = { 1.0, 2500.0 };
namespace aesa {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RadarTracker_AESA::clear()
{
    db_.clear();
    db_.reserve(2048);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

double RadarTracker_AESA::resolveRangeAmbiguity(double measured,
                                                 double predicted,
                                                 double Rmax) const
{
    if (Rmax < 1.0) return measured;
    double best = measured, minErr = 1e12;
    for (int k = -5; k <= 5; ++k)
    {
        double cand = measured + k * Rmax;
        double err  = std::abs(cand - predicted);
        if (err < minErr) { minErr = err; best = cand; }
    }
    return best;
}

double RadarTracker_AESA::computeAdaptiveTrackInterval(const TrackFile& t) const
{
    if (t.isManoeuvring) return 0.2;
    double spd = std::sqrt(t.vx*t.vx + t.vy*t.vy + t.vz*t.vz);
    if (spd > 500.0) return 0.3;
    if (spd > 100.0) return 0.5;
    return 2.0;
}

// ---------------------------------------------------------------------------
// §A  Kalman prediction
// ---------------------------------------------------------------------------

void RadarTracker_AESA::predict(double dt)
{
    for (auto& tr : db_)
    {
        if (tr.hitCount >= 2)
        {
            // IMM handles both CV and manoeuvre models.
            // On first call (immActive=false) it self-initialises
            // from tr.X and tr.P then sets immActive=true.
            performIMMPredict(tr, dt);
        }
        else
        {
            // Pure CV for single-hit tentative tracks only.
            std::array<double,6> Xnew = {};
            Xnew[0]=tr.X[0]+tr.X[3]*dt; Xnew[1]=tr.X[1]+tr.X[4]*dt;
            Xnew[2]=tr.X[2]+tr.X[5]*dt; Xnew[3]=tr.X[3];
            Xnew[4]=tr.X[4]; Xnew[5]=tr.X[5];
            tr.X = Xnew;

            double P[6][6];
            for (int i=0;i<6;++i) for (int j=0;j<6;++j) P[i][j]=tr.P[i][j];
            for (int i=0;i<3;++i) for (int j=0;j<6;++j)
                {
                    tr.P[i][j] = P[i][j] + dt*P[i+3][j];
                    tr.P[i+3][j] = P[i+3][j];
                }
            for (int i=0;i<6;++i) for (int j=0;j<3;++j)
                    tr.P[i][j] += dt*tr.P[i][j+3];
            for (int i=0;i<6;++i) for (int j=0;j<6;++j)
                    tr.P[i][j] += tr.Q[i][j];
        }

        tr.x=tr.X[0]; tr.y=tr.X[1]; tr.z=tr.X[2];
        tr.vx=tr.X[3]; tr.vy=tr.X[4]; tr.vz=tr.X[5];
        tr.predictedRange = std::sqrt(tr.x*tr.x+tr.y*tr.y+tr.z*tr.z);
        tr.isUpdated = false;
    }
}
// void RadarTracker_AESA::predict(double dt)
// {
//     for (auto& tr : db_)
//     {
//         if (tr.hitCount >= 2 && tr.immActive)
//         {
//             performIMMPredict(tr, dt);
//         }
//         else
//         {
//             std::array<double,6> Xnew = {};
//             Xnew[0]=tr.X[0]+tr.X[3]*dt; Xnew[1]=tr.X[1]+tr.X[4]*dt;
//             Xnew[2]=tr.X[2]+tr.X[5]*dt; Xnew[3]=tr.X[3];
//             Xnew[4]=tr.X[4]; Xnew[5]=tr.X[5];
//             tr.X = Xnew;

//             double P[6][6];
//             for (int i=0;i<6;++i) for (int j=0;j<6;++j) P[i][j]=tr.P[i][j];
//             for (int i=0;i<3;++i) for (int j=0;j<6;++j)
//                 {
//                     tr.P[i][j] = P[i][j] + dt*P[i+3][j];
//                     tr.P[i+3][j] = P[i+3][j];
//                 }
//             for (int i=0;i<6;++i) for (int j=0;j<3;++j)
//                     tr.P[i][j] += dt*tr.P[i][j+3];
//             for (int i=0;i<6;++i) for (int j=0;j<6;++j)
//                     tr.P[i][j] += tr.Q[i][j];

//             if (tr.hitCount >= 2)
//                 performIMMPredict(tr, dt);
//         }

//         tr.x=tr.X[0]; tr.y=tr.X[1]; tr.z=tr.X[2];
//         tr.vx=tr.X[3]; tr.vy=tr.X[4]; tr.vz=tr.X[5];
//         tr.predictedRange = std::sqrt(tr.x*tr.x+tr.y*tr.y+tr.z*tr.z);
//         tr.isUpdated = false;
//     }
// }

// ---------------------------------------------------------------------------
// §B  NN association
// ---------------------------------------------------------------------------

TrackFile* RadarTracker_AESA::findBestTrackMatch(const DetectionOutput& det,
                                                   double maxUnambiguousRange,
                                                   double& outBestProb)
{
    TrackFile* best = nullptr;
    outBestProb = 0.0;

    for (auto& tr : db_)
    {
        if (!std::isfinite(tr.x)||!std::isfinite(tr.y)||
            !std::isfinite(tr.z)||!std::isfinite(tr.vx)) continue;
        if (tr.isUpdated) continue;

        double cand = resolveRangeAmbiguity(det.range, tr.predictedRange, maxUnambiguousRange);
        if (std::abs(cand - tr.predictedRange) > RANGE_GATE) continue;

        double az=det.azimuth*M_PI/180.0, el=det.elevation*M_PI/180.0;
        double zx=det.range*std::cos(el)*std::cos(az);
        double zy=det.range*std::cos(el)*std::sin(az);
        double zz=det.range*std::sin(el);
        double dx=zx-tr.X[0], dy=zy-tr.X[1], dz=zz-tr.X[2];

        double S[3][3];
        for (int a=0;a<3;++a) for (int b=0;b<3;++b) S[a][b]=tr.P[a][b]+tr.R[a][b];

        double invS[3][3];
        if (!invertS3(S,invS)) continue;

        double d2 = dx*(invS[0][0]*dx+invS[0][1]*dy+invS[0][2]*dz)
                   +dy*(invS[1][0]*dx+invS[1][1]*dy+invS[1][2]*dz)
                   +dz*(invS[2][0]*dx+invS[2][1]*dy+invS[2][2]*dz);
        if (d2 > CHI2_GATE_99) continue;

        double prob = std::exp(-0.5*d2);
        if (prob > outBestProb) { outBestProb=prob; best=&tr; }
    }
    return best;
}

// ---------------------------------------------------------------------------
// §C  Kalman update (Joseph form)
// ---------------------------------------------------------------------------

void RadarTracker_AESA::performKalmanUpdate(
    TrackFile& tr, const DetectionOutput& det,
    double simTime, double /*dt*/,
    double maxUnambiguousRange, const RadarConfig& cfg)
{
    double r = resolveRangeAmbiguity(det.range, tr.predictedRange, maxUnambiguousRange);
    double az=det.azimuth*M_PI/180.0, el=det.elevation*M_PI/180.0;
    // double z[3]={ r*std::cos(el)*std::cos(az), r*std::cos(el)*std::sin(az), r*std::sin(el) };
    // double y[3]={ z[0]-tr.X[0], z[1]-tr.X[1], z[2]-tr.X[2] };

    // double innov = std::sqrt(y[0]*y[0]+y[1]*y[1]+y[2]*y[2]);
    double z[3]={ r*std::cos(el)*std::cos(az), r*std::cos(el)*std::sin(az), r*std::sin(el) };
    double y[3]={ z[0]-tr.X[0], z[1]-tr.X[1], z[2]-tr.X[2] };

    // IMM Bayesian weight update — must happen here, BEFORE the Kalman
    // correction overwrites tr.X, using each model's own predicted state
    // and covariance (imm_X[m], imm_P[m]).
    // Previously imm_mu was only updated from cBar in the predict step,
    // meaning the filter never distinguished between CV and manoeuvre models
    // based on actual measurement likelihood — model switching was blind.
    if (tr.immActive)
    {
        double Lambda[2] = {};
        for (int m = 0; m < 2; ++m)
        {
            // Per-model innovation covariance S_m = H*P_m*H^T + R
            // H = [I3 | 0] so this is just the top-left 3x3 of imm_P[m] + R
            double Sm[3][3];
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 3; ++b)
                    Sm[a][b] = tr.imm_P[m][a][b] + tr.R[a][b];

            double xm[3] = { tr.imm_X[m][0], tr.imm_X[m][1], tr.imm_X[m][2] };
            Lambda[m] = gaussianLikelihood(z, xm, Sm);
        }

        // μⱼ = Λⱼ · c̄ⱼ / Σᵢ(Λᵢ · c̄ᵢ)
        double norm = Lambda[0]*tr.imm_mu[0] + Lambda[1]*tr.imm_mu[1];
        if (norm > 1e-30)
        {
            tr.imm_mu[0] = Lambda[0] * tr.imm_mu[0] / norm;
            tr.imm_mu[1] = Lambda[1] * tr.imm_mu[1] / norm;
        }
        else
        {
            // Both models have negligible likelihood — reset rather than
            // divide by zero. This can happen on a very large innovation.
            tr.imm_mu[0] = tr.imm_mu[1] = 0.5;
        }
    }

    double innov = std::sqrt(y[0]*y[0]+y[1]*y[1]+y[2]*y[2]);
    tr.innovationMagnitude = innov;
    tr.isManoeuvring       = (innov > cfg.manoeuvreThreshold_m);
    double velQ = tr.isManoeuvring ? 100.0 : 1.0;
    tr.Q[3][3]=velQ; tr.Q[4][4]=velQ; tr.Q[5][5]=velQ;

    double S[3][3];
    for (int a=0;a<3;++a) for (int b=0;b<3;++b) S[a][b]=tr.P[a][b]+tr.R[a][b];

    double invS[3][3];
    if (!invertS3(S,invS)) { tr.hitCount=std::max(0,tr.hitCount-1); return; }

    double K[6][3]={};
    for (int a=0;a<6;++a) for (int b=0;b<3;++b) for (int k=0;k<3;++k)
        K[a][b] += tr.P[a][k]*invS[k][b];

    for (int a=0;a<6;++a) for (int b=0;b<3;++b) tr.X[a] += K[a][b]*y[b];

    // Joseph form: P = (I-KH)*P*(I-KH)^T + K*R*K^T
    double IKH[6][6]={};
    for (int a=0;a<6;++a) IKH[a][a]=1.0;
    for (int a=0;a<6;++a) for (int b=0;b<3;++b) IKH[a][b] -= K[a][b];

    double Pnew[6][6]={};
    for (int a=0;a<6;++a) for (int b=0;b<6;++b) for (int k=0;k<6;++k)
        Pnew[a][b] += IKH[a][k]*tr.P[k][b];
    for (int a=0;a<6;++a) for (int b=0;b<6;++b) tr.P[a][b]=Pnew[a][b];

    //for (int a=3;a<6;++a) tr.X[a]=std::clamp(tr.X[a],-cfg.maxTrackSpeed,cfg.maxTrackSpeed);
    for (int a=3;a<5;++a) tr.X[a]=std::clamp(tr.X[a],-cfg.maxTrackSpeed,cfg.maxTrackSpeed);
    tr.X[5] = std::clamp(tr.X[5], -200.0, 200.0);  // vz max 200 m/s vertical
    tr.x=tr.X[0]; tr.y=tr.X[1]; tr.z=tr.X[2];
    tr.vx=tr.X[3]; tr.vy=tr.X[4]; tr.vz=tr.X[5];
    // ADD THIS — sync IMM model states with the Kalman-corrected state
    // Without this, the IMM fuse step on the next predict() call
    // restores the pre-update position from stale imm_X values
    if (tr.immActive)
    {
        for (int m = 0; m < 2; ++m)
            for (int k = 0; k < 6; ++k)
                tr.imm_X[m][k] = tr.X[k];
    }

    tr.range = std::sqrt(tr.x*tr.x+tr.y*tr.y+tr.z*tr.z);
    //tr.range = std::sqrt(tr.x*tr.x+tr.y*tr.y+tr.z*tr.z);
    tr.velocity = (tr.range>1e-6) ? (tr.vx*tr.x+tr.vy*tr.y+tr.vz*tr.z)/tr.range : 0.0;

    tr.lastSeenTime = simTime;
    tr.hitCount++;
    tr.isUpdated = tr.updatedThisScan = true;
    tr.missCount = 0; tr.wasAmbiguous = det.isAmbiguous;
    if (tr.hitCount >= cfg.minHitsToValidate) tr.isValidated = true;
    tr.trackQuality = std::clamp(
        (static_cast<double>(tr.hitCount)/10.0) *
        (1.0 - static_cast<double>(tr.scanMissCount)/std::max(1,cfg.missedScansToDrop)),
        0.0, 1.0);
}

// ---------------------------------------------------------------------------
// FIX-05  JPDA  (Bar-Shalom formulation)
// ---------------------------------------------------------------------------

double RadarTracker_AESA::detS3(const double S[3][3]) const
{
    return S[0][0]*(S[1][1]*S[2][2]-S[1][2]*S[2][1])
          -S[0][1]*(S[1][0]*S[2][2]-S[1][2]*S[2][0])
          +S[0][2]*(S[1][0]*S[2][1]-S[1][1]*S[2][0]);
}

bool RadarTracker_AESA::invertS3(const double S[3][3], double inv[3][3]) const
{
    double d = detS3(S);
    if (std::abs(d) < 1e-3) return false;
    inv[0][0]=(S[1][1]*S[2][2]-S[1][2]*S[2][1])/d;
    inv[0][1]=(S[0][2]*S[2][1]-S[0][1]*S[2][2])/d;
    inv[0][2]=(S[0][1]*S[1][2]-S[0][2]*S[1][1])/d;
    inv[1][0]=(S[1][2]*S[2][0]-S[1][0]*S[2][2])/d;
    inv[1][1]=(S[0][0]*S[2][2]-S[0][2]*S[2][0])/d;
    inv[1][2]=(S[0][2]*S[1][0]-S[0][0]*S[1][2])/d;
    inv[2][0]=(S[1][0]*S[2][1]-S[1][1]*S[2][0])/d;
    inv[2][1]=(S[0][1]*S[2][0]-S[0][0]*S[2][1])/d;
    inv[2][2]=(S[0][0]*S[1][1]-S[0][1]*S[1][0])/d;
    return true;
}

double RadarTracker_AESA::gaussianLikelihood(const double z[3], const double x[3],
                                              const double S[3][3]) const
{
    double d = detS3(S);
    if (d <= 0.0) return 0.0;
    double invS[3][3];
    if (!invertS3(S,invS)) return 0.0;

    double dx=z[0]-x[0], dy=z[1]-x[1], dz2=z[2]-x[2];
    double m2 = dx*(invS[0][0]*dx+invS[0][1]*dy+invS[0][2]*dz2)
               +dy*(invS[1][0]*dx+invS[1][1]*dy+invS[1][2]*dz2)
               +dz2*(invS[2][0]*dx+invS[2][1]*dy+invS[2][2]*dz2);
    double norm = std::sqrt(std::pow(2.0*M_PI,3.0)*std::abs(d));
    return std::exp(-0.5*m2) / std::max(norm, 1e-30);
}
void RadarTracker_AESA::performIMMPredict(TrackFile& tr, double dt) const noexcept
{
    // ── 1. Initialise IMM states from current track on first call ─────────
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
    double cBar[2] = {};
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
            cBar[j] += IMM_PI[i][j] * tr.imm_mu[i];

    // ── 3. Mixing probabilities  μᵢ|ⱼ = πᵢⱼ · μᵢ / c̄ⱼ ─────────────────
    double muMix[2][2] = {};
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
            muMix[i][j] = (cBar[j] > 1e-12)
                              ? IMM_PI[i][j] * tr.imm_mu[i] / cBar[j] : 0.0;

    // ── 4. Mixed initial conditions for each model ────────────────────────
    double X0[2][6]     = {};
    double P0[2][6][6]  = {};

    for (int j = 0; j < 2; ++j)
    {
        // Mixed mean
        for (int k = 0; k < 6; ++k)
            for (int i = 0; i < 2; ++i)
                X0[j][k] += muMix[i][j] * tr.imm_X[i][k];

        // Mixed covariance  Σᵢ μᵢ|ⱼ · (Pᵢ + (Xᵢ−X̄ⱼ)(Xᵢ−X̄ⱼ)ᵀ)
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
    for (int m = 0; m < 2; ++m)
    {
        // State: x ← F·x
        for (int k = 0; k < 3; ++k) X0[m][k] += X0[m][k+3] * dt;
        for (int k = 0; k < 6; ++k) tr.imm_X[m][k] = X0[m][k];

        // Covariance: P ← F·P·Fᵀ + Q
        // F cross-terms (position += dt × velocity)
        double Ptmp[6][6];
        for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) Ptmp[i][j] = P0[m][i][j];
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 6; ++j)
                Ptmp[i][j] += dt * P0[m][i+3][j];
        for (int i = 0; i < 6; ++i) for (int j = 0; j < 3; ++j)
                Ptmp[i][j] += dt * Ptmp[i][j+3];

        // Add process noise Q (velocity block only, model-specific)
        // for (int k = 3; k < 6; ++k)
        //     Ptmp[k][k] += IMM_Q_VEL[m];
        // // Position process noise (dt⁴/4 · σ_a²)
        // double q_pos = IMM_Q_VEL[m] * dt*dt * 0.25;
        // for (int k = 0; k < 3; ++k)
        //     Ptmp[k][k] += q_pos;
        // Horizontal velocity noise
        Ptmp[3][3] += IMM_Q_VEL[m];
        Ptmp[4][4] += IMM_Q_VEL[m];
        // Vertical velocity noise — capped even for manoeuvre model
        double qVz = (m == 0) ? 1.0 : 25.0;
        Ptmp[5][5] += qVz;
        // Position process noise
        double q_pos_xy = IMM_Q_VEL[m] * dt * dt * 0.25;
        double q_pos_z  = qVz            * dt * dt * 0.25;
        Ptmp[0][0] += q_pos_xy;
        Ptmp[1][1] += q_pos_xy;
        Ptmp[2][2] += q_pos_z;

        for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j)
                tr.imm_P[m][i][j] = Ptmp[i][j];
    }

    // ── 6. Fuse: overall predicted state = Σⱼ c̄ⱼ · Xⱼ ─────────────────
    for (int k = 0; k < 6; ++k) tr.X[k] = 0.0;
    for (int m = 0; m < 2; ++m)
        for (int k = 0; k < 6; ++k) tr.X[k] += cBar[m] * tr.imm_X[m][k];

    for (int i = 0; i < 6; ++i) for (int j = 0; j < 6; ++j) tr.P[i][j] = 0.0;
    for (int m = 0; m < 2; ++m)
    {
        double dX[6];
        for (int k = 0; k < 6; ++k) dX[k] = tr.imm_X[m][k] - tr.X[k];
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j)
                tr.P[i][j] += cBar[m] * (tr.imm_P[m][i][j] + dX[i]*dX[j]);
    }

    // Update c̄ → μ for next cycle
    for (int m = 0; m < 2; ++m) tr.imm_mu[m] = cBar[m];
}
void RadarTracker_AESA::performJPDAUpdate(
    const std::vector<DetectionOutput>& dets,
    double simTime,
    double maxUnambiguousRange,
    const RadarConfig& cfg)
{
    if (dets.empty()) return;

    int nT = static_cast<int>(db_.size());
    int nD = static_cast<int>(dets.size());

    // Pre-convert detections to Cartesian
    struct CartDet { double z[3]; double range; bool ambiguous; };
    std::vector<CartDet> cD(nD);
    for (int j=0;j<nD;++j)
    {
        double az=dets[j].azimuth*M_PI/180.0, el=dets[j].elevation*M_PI/180.0;
        cD[j] = { { dets[j].range*std::cos(el)*std::cos(az),
                    dets[j].range*std::cos(el)*std::sin(az),
                    dets[j].range*std::sin(el) },
                  dets[j].range, dets[j].isAmbiguous };
    }

    double lamFA = std::max(1e-12, static_cast<double>(cfg.jpdaFalseAlarmDensity));

    for (int i=0;i<nT;++i)
    {
        auto& tr = db_[i];
        if (!std::isfinite(tr.x)||!std::isfinite(tr.y)||!std::isfinite(tr.z)) continue;

        double S[3][3];
        for (int a=0;a<3;++a) for (int b=0;b<3;++b) S[a][b]=tr.P[a][b]+tr.R[a][b];
        double invS[3][3];
        if (!invertS3(S,invS)) continue;

        std::vector<double> e_ij(nD, 0.0);
        std::vector<bool>   gate(nD, false);

        for (int j=0;j<nD;++j)
        {
            double cand = resolveRangeAmbiguity(cD[j].range, tr.predictedRange, maxUnambiguousRange);
            if (std::abs(cand - tr.predictedRange) > RANGE_GATE) continue;
            double dx=cD[j].z[0]-tr.X[0], dy=cD[j].z[1]-tr.X[1], dz=cD[j].z[2]-tr.X[2];
            double d2 = dx*(invS[0][0]*dx+invS[0][1]*dy+invS[0][2]*dz)
                       +dy*(invS[1][0]*dx+invS[1][1]*dy+invS[1][2]*dz)
                       +dz*(invS[2][0]*dx+invS[2][1]*dy+invS[2][2]*dz);
            if (d2 > CHI2_GATE_99) continue;
            gate[j]=true;
            double x3[3]={tr.X[0],tr.X[1],tr.X[2]};
            e_ij[j] = gaussianLikelihood(cD[j].z, x3, S) / lamFA;
        }

        double sumE=0.0;
        for (double e : e_ij) sumE+=e;
        double beta0 = 1.0/(1.0+sumE);
        double Pc    = 1.0 - beta0;
        if (Pc < 1e-9) continue;

        std::vector<double> beta(nD,0.0);
        for (int j=0;j<nD;++j) if (gate[j]) beta[j]=e_ij[j]/(1.0+sumE);

        // Kalman gain
        double K[6][3]={};
        for (int a=0;a<6;++a) for (int b=0;b<3;++b) for (int k=0;k<3;++k)
            K[a][b] += tr.P[a][k]*invS[k][b];

        // Combined innovation ν_c
        double nu[3]={0,0,0};
        for (int j=0;j<nD;++j) if (gate[j])
        {
            nu[0]+=beta[j]*(cD[j].z[0]-tr.X[0]);
            nu[1]+=beta[j]*(cD[j].z[1]-tr.X[1]);
            nu[2]+=beta[j]*(cD[j].z[2]-tr.X[2]);
        }
        for (int a=0;a<6;++a)
            for (int b=0;b<3;++b)
                tr.X[a] += K[a][b]*nu[b];

        // IKH = I - K*H
        double IKH[6][6]={};
        for (int a=0;a<6;++a) IKH[a][a]=1.0;
        for (int a=0;a<6;++a) for (int b=0;b<3;++b) IKH[a][b]-=K[a][b];

        // P_KF
        double PKF[6][6]={};
        for (int a=0;a<6;++a) for (int b=0;b<6;++b) for (int k=0;k<6;++k)
            PKF[a][b] += IKH[a][k]*tr.P[k][b];

        // Spread-of-innovations P_tilde
        double Pt[3][3]={};
        for (int j=0;j<nD;++j) if (gate[j])
        {
            double y[3]={cD[j].z[0]-tr.X[0],cD[j].z[1]-tr.X[1],cD[j].z[2]-tr.X[2]};
            for (int a=0;a<3;++a) for (int b=0;b<3;++b) Pt[a][b]+=beta[j]*y[a]*y[b];
        }
        for (int a=0;a<3;++a) for (int b=0;b<3;++b) Pt[a][b]-=nu[a]*nu[b];

        // K*P_tilde*K^T
        double KPt[6][3]={}, KPtKt[6][6]={};
        for (int a=0;a<6;++a) for (int b=0;b<3;++b) for (int k=0;k<3;++k) KPt[a][b]+=K[a][k]*Pt[k][b];
        for (int a=0;a<6;++a) for (int b=0;b<6;++b) for (int k=0;k<3;++k) KPtKt[a][b]+=KPt[a][k]*K[b][k];

        for (int a=0;a<6;++a) for (int b=0;b<6;++b)
            tr.P[a][b] = beta0*tr.P[a][b] + Pc*PKF[a][b] + KPtKt[a][b];

        //for (int a=3;a<6;++a) tr.X[a]=std::clamp(tr.X[a],-cfg.maxTrackSpeed,cfg.maxTrackSpeed);
        for (int a=3;a<5;++a) tr.X[a]=std::clamp(tr.X[a],-cfg.maxTrackSpeed,cfg.maxTrackSpeed);
        tr.X[5] = std::clamp(tr.X[5], -200.0, 200.0);  // vz max 200 m/s vertical
        tr.x=tr.X[0]; tr.y=tr.X[1]; tr.z=tr.X[2];
        tr.vx=tr.X[3]; tr.vy=tr.X[4]; tr.vz=tr.X[5];
        tr.range = std::sqrt(tr.x*tr.x+tr.y*tr.y+tr.z*tr.z);
        tr.velocity=(tr.range>1e-6)?(tr.vx*tr.x+tr.vy*tr.y+tr.vz*tr.z)/tr.range:0.0;

        double innov=std::sqrt(nu[0]*nu[0]+nu[1]*nu[1]+nu[2]*nu[2]);
        tr.innovationMagnitude=innov;
        tr.isManoeuvring=(innov>cfg.manoeuvreThreshold_m);
        double vQ=tr.isManoeuvring?100.0:1.0;
        tr.Q[3][3]=vQ; tr.Q[4][4]=vQ; tr.Q[5][5]=vQ;

        tr.lastSeenTime=simTime; tr.hitCount++;
        tr.isUpdated=tr.updatedThisScan=true; tr.missCount=0;
        if (tr.hitCount>=cfg.minHitsToValidate) tr.isValidated=true;
        tr.trackQuality=std::clamp(
            (static_cast<double>(tr.hitCount)/10.0)*
            (1.0-static_cast<double>(tr.scanMissCount)/std::max(1,cfg.missedScansToDrop)),
            0.0, 1.0);
    }
}

// ---------------------------------------------------------------------------
// §C  New track creation
// ---------------------------------------------------------------------------

void RadarTracker_AESA::createNewTrack(
    const DetectionOutput& det, const TargetInput& target,
    double maxUnambiguousRange, double simTime, const RadarConfig& cfg)
{
    for (const auto& t : db_) if (t.id == det.targetID) return;

    TrackFile tr;
    tr.id = det.targetID;

    double r = det.range;
    if (det.isAmbiguous && det.radialVelocity < 0.0) r += maxUnambiguousRange;

    double az=det.azimuth*M_PI/180.0, el=det.elevation*M_PI/180.0;
    tr.x=r*std::cos(el)*std::cos(az); tr.y=r*std::cos(el)*std::sin(az);
    tr.z=r*std::sin(el);

    tr.vx=target.vx; tr.vy=target.vy; tr.vz=target.vz;
    tr.X={tr.x,tr.y,tr.z,tr.vx,tr.vy,tr.vz};

   // double posVar = cfg.noise.rangeStdDev * cfg.noise.rangeStdDev;
    double posVar = std::max(
        cfg.noise.rangeStdDev * cfg.noise.rangeStdDev,
        500.0 * 500.0
        );
    double velVar = 500.0*500.0;
    tr.P[0][0]=posVar; tr.P[1][1]=posVar; tr.P[2][2]=posVar;
    tr.P[3][3]=velVar; tr.P[4][4]=velVar; tr.P[5][5]=velVar;
    tr.Q[0][0]=10.0; tr.Q[1][1]=10.0; tr.Q[2][2]=10.0;
    tr.Q[3][3]=1.0;  tr.Q[4][4]=1.0;  tr.Q[5][5]=1.0;
   // tr.R[0][0]=25.0; tr.R[1][1]=25.0; tr.R[2][2]=25.0;
    tr.R[0][0] = 25.0;
    tr.R[1][1] = 25.0;
    // Z measurement noise grows with range — elevation angle error at range
    double elNoise_deg = static_cast<double>(cfg.beamWidth) / 1.606;
    double zStdDev     = std::max(r * std::sin(elNoise_deg * M_PI / 180.0), 50.0);
    tr.R[2][2]         = zStdDev * zStdDev;

    tr.range=r; tr.predictedRange=r; tr.velocity=det.radialVelocity;
    tr.lastSeenTime=simTime; tr.lastTrackBeamTime=simTime;
    tr.hitCount=1; tr.updatedThisScan=true; tr.wasAmbiguous=det.isAmbiguous;
    tr.isDRFMSuspect=det.isDRFMGhost;

    if (db_.size() >= MAX_TRACKS) db_.erase(db_.begin());
    db_.push_back(std::move(tr));
}

// ---------------------------------------------------------------------------
// FIX-12  External track injection
// ---------------------------------------------------------------------------

void RadarTracker_AESA::injectExternalTrack(const TrackOutput& ext,
                                             double simTime,
                                             const RadarConfig& cfg)
{
    for (const auto& t : db_) if (t.id == ext.id) return;

    TrackFile tr;
    tr.id=ext.id; tr.x=ext.x; tr.y=ext.y; tr.z=ext.z;
    tr.vx=ext.vx; tr.vy=ext.vy; tr.vz=ext.vz;
    tr.X={tr.x,tr.y,tr.z,tr.vx,tr.vy,tr.vz};
    tr.isValidated=true; tr.isExternalTrack=true;
    tr.hitCount=cfg.minHitsToValidate;
    tr.lastSeenTime=simTime; tr.lastTrackBeamTime=simTime;
    tr.range=ext.range; tr.predictedRange=ext.range;
    tr.velocity=ext.radialVelocity; tr.iff=ext.iff;

    double pV=300.0*300.0, vV=50.0*50.0;
    tr.P[0][0]=pV; tr.P[1][1]=pV; tr.P[2][2]=pV;
    tr.P[3][3]=vV; tr.P[4][4]=vV; tr.P[5][5]=vV;
    tr.Q[0][0]=10.0; tr.Q[1][1]=10.0; tr.Q[2][2]=10.0;
    tr.Q[3][3]=1.0; tr.Q[4][4]=1.0; tr.Q[5][5]=1.0;
    tr.R[0][0]=9e4; tr.R[1][1]=9e4; tr.R[2][2]=9e4;

    if (db_.size() >= MAX_TRACKS) db_.erase(db_.begin());
    db_.push_back(std::move(tr));
}

// ---------------------------------------------------------------------------
// §D  Scan-miss logic
// ---------------------------------------------------------------------------

void RadarTracker_AESA::applyScanMissLogic(double simTime, const RadarConfig& cfg)
{
    for (auto& tr : db_)
    {
        if (!tr.updatedThisScan) { tr.scanMissCount++; tr.missCount++; }
        else                     { tr.scanMissCount=0; }
        tr.updatedThisScan=false;
    }
    db_.erase(std::remove_if(db_.begin(), db_.end(),
        [&](const TrackFile& t)
        {
            return (t.scanMissCount > cfg.missedScansToDrop) ||
                   (!t.isExternalTrack &&
                    (simTime - t.lastSeenTime) > cfg.trackCoastSeconds);
        }), db_.end());
}

// ---------------------------------------------------------------------------
// §E  Output assembly
// ---------------------------------------------------------------------------

static void fillCommon(TrackOutput& o, const TrackFile& t)
{
    o.id=t.id; o.x=t.x; o.y=t.y; o.z=t.z;
    o.vx=t.vx; o.vy=t.vy; o.vz=t.vz;
    o.radialVelocity=t.velocity; o.isValidated=t.isValidated;
    o.hitCount=t.hitCount; o.scanMissCount=t.scanMissCount;
    o.wasAmbiguous=t.wasAmbiguous; o.isManoeuvring=t.isManoeuvring;
    o.trackQuality=t.trackQuality; o.isDRFMSuspect=t.isDRFMSuspect;
    o.isExternalTrack=t.isExternalTrack; o.iff=t.iff;

    double rr = t.isUpdated ? t.range : t.predictedRange;
    o.range = rr;
    if (rr > 1e-6)
    {
        o.azimuth  = std::atan2(t.y,t.x)*(180.0/M_PI);
        //if (o.azimuth<0.0) o.azimuth+=360.0;
        o.elevation= std::asin(std::clamp(t.z/rr,-1.0,1.0))*(180.0/M_PI);
    }
    o.speedOverGround=std::sqrt(t.vx*t.vx+t.vy*t.vy);
    o.heading=std::atan2(t.vy,t.vx)*(180.0/M_PI);
    if (o.heading<0.0) o.heading+=360.0;
    o.targetAspect=std::abs(o.heading-o.azimuth);
    if (o.targetAspect>180.0) o.targetAspect=360.0-o.targetAspect;

    double v2=t.vx*t.vx+t.vy*t.vy+t.vz*t.vz;
    if (v2>0.01)
    {
        double tc=-(t.x*t.vx+t.y*t.vy+t.z*t.vz)/v2;
        o.time_to_cpa=std::max(0.0,tc);
        double cx=t.x+t.vx*o.time_to_cpa,cy=t.y+t.vy*o.time_to_cpa,cz=t.z+t.vz*o.time_to_cpa;
        o.cpa_distance=std::sqrt(cx*cx+cy*cy+cz*cz);
    }
    else { o.time_to_cpa=0.0; o.cpa_distance=rr; }

    o.Pk=std::min(0.99, 0.95*std::exp(-rr/45000.0)*(t.velocity<0.0?1.2:0.8));
}

TrackOutput RadarTracker_AESA::buildTrackOutput(const TrackFile& t) const
{
    TrackOutput o; fillCommon(o,t); return o;
}

DetectionOutput RadarTracker_AESA::buildTWSDetection(const TrackFile& t) const
{
    DetectionOutput d;
    d.targetID=t.id; d.isAmbiguous=false; d.radialVelocity=t.velocity;
    double rr=t.isUpdated?t.range:t.predictedRange; d.range=rr;
    if (rr>1e-6)
    {
        d.azimuth  =std::atan2(t.y,t.x)*(180.0/M_PI); //if(d.azimuth<0.0) d.azimuth+=360.0;
        d.elevation=std::asin(std::clamp(t.z/rr,-1.0,1.0))*(180.0/M_PI);
    }
    d.speedOverGround=std::sqrt(t.vx*t.vx+t.vy*t.vy);
    d.heading=std::atan2(t.vy,t.vx)*(180.0/M_PI); if(d.heading<0.0) d.heading+=360.0;
    d.isDRFMGhost=t.isDRFMSuspect;
    d.Pk=std::min(0.99, 0.95*std::exp(-rr/45000.0));
    return d;
}

void RadarTracker_AESA::getValidatedTracks(std::vector<TrackOutput>& out) const
{
    out.clear(); out.reserve(db_.size());
    for (const auto& t : db_) if (t.isValidated) out.push_back(buildTrackOutput(t));
}

// ---------------------------------------------------------------------------
// §F  Beam requests
// ---------------------------------------------------------------------------

void RadarTracker_AESA::generateTrackBeamRequests(
    std::vector<BeamRequest>& reqs, double simTime, const RadarConfig& cfg) const
{
    for (const auto& t : db_)
    {
        if (!t.isValidated) continue;
        if ((simTime-t.lastTrackBeamTime) < computeAdaptiveTrackInterval(t)) continue;

        BeamRequest r;
        r.task=BeamRequest::Task::TRACK; r.targetID=t.id;
        r.dwellTime_ms=cfg.trackDwellTime_ms;
        r.priority=t.isManoeuvring?20:10;
        r.waveform=cfg.trackWaveform; r.spoilFactor=1.0f;

        if (t.predictedRange>1.0)
        {
            r.azimuth_deg=std::atan2(t.y,t.x)*(180.0/M_PI);
            //if (r.azimuth_deg<0.0) r.azimuth_deg+=360.0;
            r.elevation_deg=std::asin(std::clamp(t.z/t.predictedRange,-1.0,1.0))*(180.0/M_PI);
        }
        reqs.push_back(r);
    }
}

} // namespace aesa
