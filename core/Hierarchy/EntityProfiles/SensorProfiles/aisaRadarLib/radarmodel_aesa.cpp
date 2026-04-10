// =============================================================================
// radarmodel_aesa.cpp  —  Rev 3
// All 13 audit fixes wired. assembleFinalOutput() stub removed.
// =============================================================================

#include "radarmodel_aesa.h"
#include "radarantenna_aesa.h"
#include "radarsignalprocessor_aesa.h"
#include "radarscheduler.h"
#include "radartracker_aesa.h"
#include "radarsignallibrary_aesa.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr double SPEED_OF_LIGHT = 299792458.0;
static thread_local std::default_random_engine tl_rng{ std::random_device{}() };

namespace aesa {

// =============================================================================
// Construction
// =============================================================================

RadarModel_AESA::RadarModel_AESA()
    : signal_   (std::make_unique<RadarSignalProcessor_AESA>())
    , antenna_  (std::make_unique<RadarAntenna_AESA>())
    , scheduler_(std::make_unique<RadarScheduler>())
    , tracker_  (std::make_unique<RadarTracker_AESA>())
    , library_  (std::make_unique<RadarSignalLibrary_AESA>())
{}

RadarModel_AESA::~RadarModel_AESA() = default;

// =============================================================================
// §1  Lifecycle
// =============================================================================

void RadarModel_AESA::init(const RadarConfig& cfg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_ = cfg;  displayRangeDirty_ = true;
    latestOutput_ = RadarOutput{}; latestOutput_.mode = cfg.mode;
    tracker_->clear(); antenna_->reset();
    library_->clear(); scheduler_->reset();
    chaffClouds_.clear(); drfmPullOff_.clear();
    scanDetectionCache_.clear();   // ← ADD
    initialised_ = true; running_ = false;
    lockMissCount_ = 0;
    rgpoPullOff_.clear();
    vgpoPullOff_.clear();
}

void RadarModel_AESA::start()
{
    std::lock_guard<std::mutex> lk(mutex_);
    tracker_->clear(); antenna_->reset();
    library_->clear(); chaffClouds_.clear(); drfmPullOff_.clear();
    firstScanComplete_ = false;
    scanDetectionCache_.clear();   // ← ADD
    scheduler_->buildSchedule(config_, {});
    latestOutput_ = RadarOutput{}; latestOutput_.mode = config_.mode;
    displayRangeDirty_ = true; running_ = true;

}

void RadarModel_AESA::end()
{
    std::lock_guard<std::mutex> lk(mutex_);
    running_ = false; tracker_->clear(); library_->clear();
    scheduler_->reset(); chaffClouds_.clear(); drfmPullOff_.clear();
    latestOutput_ = RadarOutput{};
}

void RadarModel_AESA::reset()
{
    RadarConfig saved;
    { std::lock_guard<std::mutex> lk(mutex_); saved = config_; }
    firstScanComplete_ = false;
    init(saved); start();
    lockMissCount_ = 0;
    rgpoPullOff_.clear();
    vgpoPullOff_.clear();
}

// =============================================================================
// §2  Config + Mode
// =============================================================================

void RadarModel_AESA::setConfig(const RadarConfig& cfg)
{
    //std::lock_guard<std::mutex> lk(mutex_); config_ = cfg; displayRangeDirty_ = true;
    std::lock_guard<std::mutex> lk(mutex_);
    if (cfg.mode == RadarMode::SURVEILLANCE && config_.mode != RadarMode::SURVEILLANCE)
    {
        tracker_->clear();
        latestOutput_.tracks.clear();
        latestOutput_.detections.clear();
    }
    config_ = cfg;
    displayRangeDirty_ = true;
}

RadarConfig RadarModel_AESA::getConfig() const
{
    std::lock_guard<std::mutex> lk(mutex_); return config_;
}

void RadarModel_AESA::setMode(RadarMode mode)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode = mode; displayRangeDirty_ = true;
    scheduler_->buildSchedule(config_, tracker_->database());
}

void RadarModel_AESA::lockOn(uint32_t targetID)
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode = RadarMode::LOCK_ON; config_.lockedTargetID = targetID;
    lockMissCount_         = 0;          // ADD

    // ADD — clear stale TWS detections from display
    scanDetectionCache_.clear();
    latestOutput_.detections.clear();
    latestOutput_.tracks.clear();
    firstScanComplete_ = false;

    const TrackFile* locked = nullptr;
    for (const auto& t : tracker_->database())
        if (t.id == targetID) { locked = &t; break; }
    scheduler_->buildSchedule(config_, tracker_->database());

   // scheduler_->insertFireControlBeam(targetID, locked, config_);
}

// void RadarModel_AESA::breakLock()
// {
//     std::lock_guard<std::mutex> lk(mutex_);
//     config_.mode = RadarMode::SURVEILLANCE; config_.lockedTargetID = 0;
//     scheduler_->buildSchedule(config_, tracker_->database());
// }
void RadarModel_AESA::breakLock()
{
    std::lock_guard<std::mutex> lk(mutex_);
    config_.mode = RadarMode::SURVEILLANCE; config_.lockedTargetID = 0;

    // Clear the scan detection cache. In LOCK_ON mode all beam time was
    // concentrated on one target at one azimuth. If that biased cache were
    // published as the first "complete scan" after returning to surveillance
    // the display would show a ghost detection at the old lock azimuth until
    // the next full scan boundary flushed it naturally.
    scanDetectionCache_.clear();
    latestOutput_.detections.clear();
    latestOutput_.tracks.clear();
    firstScanComplete_ = false;
    scheduler_->buildSchedule(config_, tracker_->database());
}
RadarOutput RadarModel_AESA::getOutput() const
{
    std::lock_guard<std::mutex> lk(mutex_); return latestOutput_;
}

// =============================================================================
// §3  Chaff / external track
// =============================================================================

void RadarModel_AESA::addChaffCloud(const ChaffCloud& cloud)
{
    std::lock_guard<std::mutex> lk(mutex_); chaffClouds_.push_back(cloud);
}
void RadarModel_AESA::clearChaffClouds()
{
    std::lock_guard<std::mutex> lk(mutex_); chaffClouds_.clear();
}
void RadarModel_AESA::injectExternalTrack(const TrackOutput& ext)
{
    std::lock_guard<std::mutex> lk(mutex_);
    tracker_->injectExternalTrack(ext, currentSimTime_, config_);
}
void RadarModel_AESA::loadSignalLibrary(const std::vector<SignalLibraryEntry>& entries)
{
    std::lock_guard<std::mutex> lk(mutex_); library_->loadLibrary(entries);
}

// =============================================================================
// §4  Attitude compensation
// =============================================================================

void RadarModel_AESA::applyAttitudeToBeam(double bodyAz, double bodyEl,
                                           double& worldAz, double& worldEl) const
{
    if (std::abs(currentPose_.roll) < 0.1f && std::abs(currentPose_.pitch) < 0.1f)
    {
        worldAz = bodyAz + static_cast<double>(currentPose_.heading);
        if (worldAz >= 360.0) worldAz -= 360.0;
        if (worldAz <    0.0) worldAz += 360.0;
        worldEl = bodyEl;
        return;
    }
    double azR=bodyAz*M_PI/180.0, elR=bodyEl*M_PI/180.0;
    double bx=std::cos(elR)*std::cos(azR);
    double by=std::cos(elR)*std::sin(azR);
    double bz=std::sin(elR);

    double rr=static_cast<double>(currentPose_.roll)*M_PI/180.0;
    double bx1=bx, by1=by*std::cos(rr)-bz*std::sin(rr), bz1=by*std::sin(rr)+bz*std::cos(rr);

    double pp=static_cast<double>(currentPose_.pitch)*M_PI/180.0;
    double bx2=bx1*std::cos(pp)+bz1*std::sin(pp), by2=by1;
    double bz2=-bx1*std::sin(pp)+bz1*std::cos(pp);

    double hh=static_cast<double>(currentPose_.heading)*M_PI/180.0;
    double bx3=bx2*std::cos(hh)-by2*std::sin(hh);
    double by3=bx2*std::sin(hh)+by2*std::cos(hh);
    double bz3=bz2;

    worldAz=std::atan2(by3,bx3)*(180.0/M_PI); if(worldAz<0.0) worldAz+=360.0;
    worldEl=std::asin(std::clamp(bz3,-1.0,1.0))*(180.0/M_PI);
}

// =============================================================================
// §5  FIX-04  IFF query
// =============================================================================

IFFResult RadarModel_AESA::queryIFF(const TrackFile& track,
                                     const std::vector<TargetInput>& world) const
{
    IFFResult res; res.modeUsed = config_.interrogationMode;
    if (config_.interrogationMode == IFFMode::OFF) return res;

    for (const auto& t : world)
    {
        if (t.id != track.id) continue;
        if (!t.hasIFF || t.iffMode == IFFMode::OFF) { res.response=IFFResponseCode::NO_REPLY; return res; }
        res.squawk = t.iffSquawk;
        bool friendly = false;
        for (uint32_t sq : config_.friendlySquawks) if (sq == t.iffSquawk) { friendly=true; break; }
        if (friendly)          { res.response=IFFResponseCode::FRIENDLY; res.confidence=0.95; }
        else if (t.iffSquawk)  { res.response=IFFResponseCode::UNKNOWN;  res.confidence=0.70; }
        else                   { res.response=IFFResponseCode::NO_REPLY; res.confidence=0.0;  }
        return res;
    }
    return res;
}

// =============================================================================
// §6  Main update  (5.0 – 5.12)
// =============================================================================

void RadarModel_AESA::update(double dt, const RadarPose& pose,
                              const std::vector<TargetInput>& worldInputs,
                              double simTime)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (!running_) return;

    currentSimTime_ = simTime; currentPose_ = pose;
    currentWorldInputs_ = worldInputs;
    latestOutput_.lockBroken = false;

    // Update radar height from platform pose
    if (pose.y > 50.0 && std::abs(pose.y - config_.radarHeight) > 1.0)
    { config_.radarHeight = pose.y; displayRangeDirty_ = true; }

    // FIX-10  Prune expired chaff
    chaffClouds_.erase(
        std::remove_if(chaffClouds_.begin(), chaffClouds_.end(),
            [&](const ChaffCloud& c)
            { return (simTime - c.birthTime_s) > c.decayTime_s * 5.0; }),
        chaffClouds_.end());

    // 5.2  PRF
    // const BeamRequest& schedBeam = scheduler_->currentBeam();
    // double prf = static_cast<double>(std::max(1.0f, schedBeam.waveform.prf_Hz));
    // double Rmax = SPEED_OF_LIGHT / (2.0 * prf);

    // // 5.3  Kalman prediction
    // if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
    //     tracker_->predict(dt);

    // // 5.4  Beam pointing (FIX-13)
    // scheduler_->advance(dt);
    // const BeamRequest& beam = scheduler_->currentBeam();
    // 5.3  Kalman prediction
    if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
        tracker_->predict(dt);

    // 5.4  Beam pointing — advance FIRST, then read beam so that
    // Rmax and the detection pipeline both use the same waveform.
    // Previously Rmax was computed from the pre-advance beam, causing
    // range ambiguity checks to use the wrong PRF when the scheduler
    // stepped to a new beam in the same tick.
    scheduler_->advance(dt);
    const BeamRequest& beam = scheduler_->currentBeam();
    // double prf  = static_cast<double>(std::max(1.0f, beam.waveform.prf_Hz));
    // double Rmax = SPEED_OF_LIGHT / (2.0 * prf);
    double prf  = static_cast<double>(std::max(1.0f, beam.waveform.prf_Hz));
    double Rmax = SPEED_OF_LIGHT / (2.0 * prf);
    // Staggered PRF second Rmax — zero when prf2_Hz not set
    double prf2  = static_cast<double>(beam.waveform.prf2_Hz);
    double Rmax2 = (prf2 > 1.0) ? SPEED_OF_LIGHT / (2.0 * prf2) : 0.0;
   // double worldAz, worldEl;
    //applyAttitudeToBeam(beam.azimuth_deg, beam.elevation_deg, worldAz, worldEl);
    //antenna_->pointBeam(worldAz, worldEl, config_, beam.spoilFactor);
    // Keep body frame — targets are already in body frame from inverseTransformPoint
    //antenna_->pointBeam(beam.azimuth_deg, beam.elevation_deg, config_, beam.spoilFactor);
    // In LOCK_ON, re-point beam from Kalman-predicted track position every tick
    // so the beam follows a moving target rather than staying at the baked-in
    // lock position.
    if (config_.mode == RadarMode::LOCK_ON && config_.lockedTargetID != 0)
    {
        bool pointed = false;
        for (const auto& tr : tracker_->database())
        {
            if (tr.id == config_.lockedTargetID && tr.predictedRange > 1.0)
            {
                double fcAz = std::atan2(tr.y, tr.x) * (180.0 / M_PI);
                double fcEl = std::asin(std::clamp(tr.z / tr.predictedRange,
                                                   -1.0, 1.0)) * (180.0 / M_PI);
                antenna_->pointBeam(fcAz, fcEl, config_, 1.0f);
                pointed = true;
                break;
            }
        }
        // Fallback if track not yet in db (first tick after lockOn)
        if (!pointed)
            antenna_->pointBeam(beam.azimuth_deg, beam.elevation_deg,
                                config_, beam.spoilFactor);
    }
    else
    {
        antenna_->pointBeam(beam.azimuth_deg, beam.elevation_deg,
                            config_, beam.spoilFactor);
    }
    bool scanBnd = scheduler_->scanCompleted();
    if (scanBnd) antenna_->setScanBoundary(); else antenna_->clearScanBoundary();

    // 5.5  Per-target detection
    std::normal_distribution<double> rNoise  (0.0, config_.noise.rangeStdDev);
    std::normal_distribution<double> azNoise (0.0, config_.noise.azimuthStdDev);
    std::normal_distribution<double> elNoise (0.0, config_.noise.elevationStdDev);
    std::normal_distribution<double> dvNoise (0.0, config_.noise.dopplerStdDev);

    std::vector<DetectionOutput> scanDets;
    scanDets.reserve(worldInputs.size() * 2);

    bool lockedSeen = false;
    for (const auto& t : worldInputs)
    {
        if (config_.mode == RadarMode::LOCK_ON && t.id != config_.lockedTargetID) continue;

        // bool got = processTargetDetection(
        //     t, beam, dt, simTime, Rmax,
        //     scanDets, rNoise, azNoise, elNoise, dvNoise);
        bool got = processTargetDetection(
            t, beam, dt, simTime, Rmax, Rmax2,
            scanDets, rNoise, azNoise, elNoise, dvNoise);
        if (got && config_.mode == RadarMode::LOCK_ON &&
            t.id == config_.lockedTargetID)
            lockedSeen = true;
    }

    // 5.6  Track association
    if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
    {
        if (config_.useJPDA)
        {
            tracker_->performJPDAUpdate(scanDets, simTime, Rmax, config_);
            // New tracks for unmatched detections
            for (const auto& det : scanDets)
            {
                bool hasTrack = false;
                for (const auto& tr : tracker_->database())
                    if (tr.id == det.targetID && tr.isUpdated) { hasTrack=true; break; }
                if (!hasTrack)
                    for (const auto& t : worldInputs)
                        if (t.id == det.targetID)
                        { tracker_->createNewTrack(det, t, Rmax, simTime, config_); break; }
            }
        }
        else
        {
            for (const auto& det : scanDets)
            {
                double prob = 0.0;
                TrackFile* tr = tracker_->findBestTrackMatch(det, Rmax, prob);
                if (tr)
                    tracker_->performKalmanUpdate(*tr, det, simTime, dt, Rmax, config_);
                else
                    for (const auto& t : worldInputs)
                        if (t.id == det.targetID)
                        { tracker_->createNewTrack(det, t, Rmax, simTime, config_); break; }
            }
        }
    }

    // 5.7  Scan-miss
    // if (scanBnd)
    // {
    //     tracker_->applyScanMissLogic(simTime, config_);
    //     library_->pruneStale(simTime, config_.trackCoastSeconds);
    // }
    // REPLACE WITH:
    if (scanBnd && config_.mode != RadarMode::LOCK_ON)
    {
        tracker_->applyScanMissLogic(simTime, config_);
        library_->pruneStale(simTime, config_.trackCoastSeconds);
    }
    // 5.8  Break-lock
    // if (config_.mode == RadarMode::LOCK_ON && !lockedSeen)
    // {
    //     config_.mode = RadarMode::SURVEILLANCE; config_.lockedTargetID = 0;
    //     latestOutput_.lockBroken = true;
    //     scheduler_->buildSchedule(config_, tracker_->database());
    // }
    // 5.8  Break-lock — tolerate up to 10 consecutive missed dwells
    // before dropping lock. Prevents Swerling fluctuation or a single
    // bad SINR frame from immediately breaking the lock.
    if (config_.mode == RadarMode::LOCK_ON)
    {
        if (!lockedSeen)
        {
            if (++lockMissCount_ > 10)
            {
                lockMissCount_         = 0;
                config_.mode           = RadarMode::SURVEILLANCE;
                config_.lockedTargetID = 0;
                latestOutput_.lockBroken = true;
                scanDetectionCache_.clear();
                scheduler_->buildSchedule(config_, tracker_->database());
            }
        }
        else
        {
            lockMissCount_ = 0;
        }
    }
    // 5.9  FIX-04  IFF per validated track
    for (auto& tr : tracker_->database())
        if (tr.isValidated) tr.iff = queryIFF(tr, worldInputs);

    // 5.10  Display range
    // if (displayRangeDirty_)
    // {
    //     cachedDisplayRange_km_ = std::max(5.0, std::min(1000.0,
    //                                       computeMaxDetectionRange_locked()));
    //     displayRangeDirty_ = false;
    // }
    // 5.10  Display range — cap by both RF physics AND horizon geometry
    if (displayRangeDirty_)
    {
        double rfRange = computeMaxDetectionRange_locked();

        // Horizon range — same formula as checkHorizon()
        double Re      = 6371000.0 * config_.earthRadiusFactor
                    * config_.atmosphericFactor;
        double dRadar  = std::sqrt(2.0 * Re * std::max(0.0, config_.radarHeight));
        double horizonRange_km = dRadar / 1000.0;

        cachedDisplayRange_km_ = std::max(5.0, std::min(1000.0,
                                                        std::min(rfRange, horizonRange_km)));
        displayRangeDirty_ = false;
    }
    // 5.11  Assemble output
    //latestOutput_.detections       = std::move(scanDets);
    // REPLACE WITH:
    // Accumulate detections from every beam position across the scan.
    // Publish the complete scan at the scan boundary so both targets
    // always appear together once the beam has visited them both.
    // for (auto& d : scanDets)
    //     if (!signal_->shouldMergeDetection(d, scanDetectionCache_, config_))
    //         scanDetectionCache_.push_back(d);

    // latestOutput_.detections = scanDetectionCache_;
    // if (scanBnd)
    // {
    //     //latestOutput_.detections = scanDetectionCache_;
    //     scanDetectionCache_.clear();
    // }
    // Accumulate per-beam detections, deduplicating by targetID not just position.
    // Position-only merge guard allows the same target to re-enter the cache
    // after moving between dwells. ID-based dedup prevents this.
    for (auto& d : scanDets)
    {
        bool alreadyHave = false;
        for (const auto& existing : scanDetectionCache_)
            if (existing.targetID == d.targetID) { alreadyHave = true; break; }
        if (!alreadyHave)
            scanDetectionCache_.push_back(d);
        else
        {
            // Update position of existing entry with latest detection
            for (auto& existing : scanDetectionCache_)
                if (existing.targetID == d.targetID) { existing = d; break; }
        }
    }

    latestOutput_.detections = scanDetectionCache_;
    if (scanBnd)
        scanDetectionCache_.clear();
    latestOutput_.currentAzimuth   = antenna_->currentAzimuth();
    latestOutput_.currentElevation = antenna_->currentElevation();
    latestOutput_.mode             = config_.mode;
    latestOutput_.displayRange_km  = cachedDisplayRange_km_;
    latestOutput_.currentTask      = beam.task;
    latestOutput_.currentDutyCycle = scheduler_->currentDutyCycle(); // FIX-08

   // tracker_->getValidatedTracks(latestOutput_.tracks);
    // REPLACE WITH:
    // if (config_.mode == RadarMode::TWS || config_.mode == RadarMode::LOCK_ON)
    //     tracker_->getValidatedTracks(latestOutput_.tracks);
    // else
    //     latestOutput_.tracks.clear();
    // Tracker updates internally every beam tick — that is correct and
    // intentional (more frequent Kalman updates = better estimates).
    //
    // Display output cadence is deliberately split by mode:
    //
    //   SURVEILLANCE — no tracker running, clear every tick.
    //
    //   TWS — detections and tracks are both published at the scan
    //         boundary only. Between boundaries the display holds the
    //         last complete scan picture. This keeps detections and
    //         tracks temporally consistent — you never see a track
    //         from beam N+1 alongside detections from beam N-3.
    //
    //   LOCK_ON — fire-control loop needs the latest track state every
    //             tick so the weapons system sees minimum latency.
    //             Detections are also published every tick in this mode
    //             (fire-control beam revisits the same target repeatedly
    //             so scanBnd rarely fires).

    switch (config_.mode)
    {
    case RadarMode::SURVEILLANCE:
        latestOutput_.tracks.clear();
        break;

    // case RadarMode::TWS:
    //     // Publish latest Kalman-predicted track state every tick so
    //     // heading, altitude and position are always current on the display.
    //     // Detections are still held in scanDetectionCache_ until scan
    //     // boundary — that is what prevents the two-blip problem.
    //     tracker_->getValidatedTracks(latestOutput_.tracks);
    //     break;
    case RadarMode::TWS:
        if (scanBnd) firstScanComplete_ = true;  // ADD THIS
        if (firstScanComplete_)                   // ADD THIS
            tracker_->getValidatedTracks(latestOutput_.tracks);
        // Before first scan completes → hold tracks, show nothing
        break;
    case RadarMode::LOCK_ON:
        // Publish every tick for minimum fire-control latency.
        tracker_->getValidatedTracks(latestOutput_.tracks);
        // Also flush detections every tick in lock-on — the scan
        // boundary rarely fires when all beam time is on one target.
        if (!scanDets.empty())
            latestOutput_.detections = scanDets;
        break;
    }
    library_->getIntercepts(latestOutput_.intercepts);

    // 5.12  Rebuild schedule at scan boundary
    if (scanBnd) rebuildSchedule();
}

// =============================================================================
// §7  Per-target detection pipeline
// =============================================================================

bool RadarModel_AESA::processTargetDetection(
    const TargetInput& target, const BeamRequest& beam,
    double /*dt*/, double simTime,
    double maxUnambiguousRange, double maxUnambiguousRange2,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    // Category filter
    if (config_.targetCategory == DetectionCategory::AIR_ONLY &&
        target.surface != SurfaceType::AIR) return false;
    if (config_.targetCategory == DetectionCategory::SURFACE_ONLY &&
        target.surface == SurfaceType::AIR) return false;

    double range = std::sqrt(target.x*target.x + target.y*target.y + target.z*target.z);
    if (range < config_.minDetectableRange) return false;
    if (!signal_->checkHorizon(range, target.z, config_)) return false;

    // ---- Occlusion — ITU-R P.526-15 knife-edge diffraction ------------------
    OcclusionResult occlusion = computeOcclusion(
        target, currentWorldInputs_, config_);
    if (occlusion.zone != OcclusionResult::Zone::LIT)
    {
        qDebug() << "[OCCLUSION] Target:" << target.id
                 << "Zone:" << static_cast<int>(occlusion.zone)
                 << "Loss_dB:" << occlusion.diffractionLoss_dB
                 << "PowerFactor:" << occlusion.powerReduction;
    }
    if (occlusion.zone == OcclusionResult::Zone::SHADOW)
        return false;


    double tAz = std::atan2(target.y, target.x) * (180.0/M_PI);
    //if (tAz < 0.0) tAz += 360.0;
    double tEl = std::asin(std::clamp(target.z/range,-1.0,1.0)) * (180.0/M_PI);

    // Beam gate (FIX-13: effective beamwidth)
    double azDiff, elDiff;
    if (!signal_->isTargetInBeam(antenna_->currentAzimuth(), antenna_->currentElevation(),
                                  tAz, tEl, config_, azDiff, elDiff,
                                  antenna_->effectiveBeamWidth()))
        return false;

    // FIX-11  Sidelobe blanking
    if (signal_->isJammerInSidelobe(azDiff, elDiff, target, config_)) return false;

    // FIX-01  Doppler blind zone
    double radVelRaw = (target.vx*target.x + target.vy*target.y + target.vz*target.z)
                      / std::max(range, 1.0);
    bool inBlind = false;
    // if (config_.mode != RadarMode::LOCK_ON &&
    //     signal_->isInDopplerBlindZone(radVelRaw, config_, beam.waveform))
    // {
    //     if (beam.waveform.mode != WaveformMode::HPRF) return false;
    //     inBlind = true;
    // }
    if (config_.mode != RadarMode::LOCK_ON)
    {
        bool inStapNotch = signal_->isInClutterNotchSTAP(
            radVelRaw, config_, beam.waveform);
        bool inMtiNotch  = signal_->isInDopplerBlindZone(
            radVelRaw, config_, beam.waveform);

        if (inMtiNotch && !inStapNotch)
        {
            // STAP recovers this target — apply STAP gain to SINR instead
            // of rejecting. Only works if we have enough elements for STAP.
            if (config_.numElements < 100)
            {
                if (beam.waveform.mode != WaveformMode::HPRF) return false;
                inBlind = true;
            }
            // else: fall through — STAP gain applied below
        }
        else if (inMtiNotch && inStapNotch)
        {
            if (beam.waveform.mode != WaveformMode::HPRF) return false;
            inBlind = true;
        }
    }

    // Array gain (FIX-13)
    double steer  = antenna_->computeSteeringAngle(
        antenna_->currentAzimuth(), antenna_->currentElevation(), tAz, tEl);
    double gain   = antenna_->computeArrayGain(steer, config_, antenna_->currentSpoilFactor())
                  * signal_->computeBeamGainFactor(azDiff, elDiff, config_,
                                                    antenna_->effectiveBeamWidth());

    // FIX-06  Range-based waveform selection
    BeamWaveform wf = signal_->selectWaveformForRange(range, config_);
    if (beam.task != BeamRequest::Task::SEARCH) wf = beam.waveform;

    // FIX-07  Swerling RCS
    // TargetInput tgt = target;
    // bool coherent = (beam.task==BeamRequest::Task::TRACK ||
    //                  beam.task==BeamRequest::Task::FIRE_CONTROL);
    // tgt.rcs = signal_->computeSwerlingRCS(target.rcs, target.swerlingCase, coherent);
    // computeEffectiveRCS applies aspect-angle weighting AND Swerling fluctuation
    // internally. Pre-fluctuating rcs here caused RCS to be fluctuated twice,
    // producing a silent 3-10 dB SNR error on every detection.
   // double effRCS = signal_->computeEffectiveRCS(target, range);
    double effRCS = signal_->computeEffectiveRCS(target, range, config_.frequency_Hz);
    effRCS *= signal_->computeMultipathFactor(range, tEl, target.z, config_);
    effRCS *= occlusion.powerReduction;  // penumbra reduces effective RCS

    // FIX-10  Chaff clutter in SINR denominator
    double chaffPwr = signal_->computeChaffReturn(
        antenna_->currentAzimuth(), antenna_->currentElevation(),
        chaffClouds_, simTime, config_);

    double Pr   = signal_->calculateSignalStrength(range, effRCS, gain, wf, config_);
    double sinr = signal_->computeSINR(Pr, range, target.surface, target, config_, wf);
    double stapGain = signal_->computeSTAPGain(
        radVelRaw,
        static_cast<double>(config_.platformSpeed_m_s),
        wf, config_);
    sinr *= stapGain;
    // Reduce SINR by chaff
    if (chaffPwr > 0.0)
    {
        double Pn = signal_->computeNoisePower(config_, static_cast<double>(wf.bandwidth_Hz));
        if (Pn + chaffPwr > 0.0) sinr *= Pn / (Pn + chaffPwr);
    }

    auto   cells = signal_->generateReferenceCells(target.surface, config_);
    double radVel= signal_->computeRadialVelocity(target, range, dvNoise);
    double thresh= (std::abs(radVel) < 5.0)
                       ? signal_->computeCFARThresholdRelaxed(cells, config_)
                       : signal_->computeCFARThreshold(cells, config_);

    if (sinr <= thresh) return false;

    // Build DetectionOutput
    DetectionOutput det;
    det.targetID       = target.id;
    det.azimuth        = tAz  + azNoise(tl_rng);
    det.elevation      = tEl  + elNoise(tl_rng);
    det.snr            = sinr;
    det.radialVelocity = radVel;
    det.inDopplerBlind = inBlind;

    // FIX-02  Monopulse
    signal_->computeMonopulseAngleError(azDiff, elDiff, sinr, config_,
                                         det.azError_deg, det.elError_deg);
    det.azimuth   += det.azError_deg;
    det.elevation += det.elError_deg;

    signal_->computeTargetMotionParams(det, target, range);
    signal_->computeCPA(det, target, range);

    // FIX-07  Albersheim Pk
    det.Pk = signal_->computePk(sinr, config_.targetPfa,
                                 wf.pulsesPerDwell, target.swerlingCase);

    //signal_->applyRangeAmbiguity(det, range, maxUnambiguousRange, rNoise);
    signal_->applyRangeAmbiguity(det, range, maxUnambiguousRange,
                                 maxUnambiguousRange2, rNoise);
    // ADD after applyRangeAmbiguity call, before the existing det.isAmbiguous check:
    // Staggered velocity resolver — only runs when prf2_Hz is set
    if (maxUnambiguousRange2 > 1.0 && wf.prf2_Hz > 1.0f)
    {
        double lambda = SPEED_OF_LIGHT / config_.frequency_Hz;
        double Vmax1  = lambda * static_cast<double>(wf.prf_Hz)  / 2.0;
        double Vmax2  = lambda * static_cast<double>(wf.prf2_Hz) / 2.0;
        // Second independent Doppler measurement from the interleaved PRF
        double radVel2 = signal_->computeRadialVelocity(target, range, dvNoise);
        det.radialVelocity = signal_->resolveVelocityStaggered(
            det.radialVelocity, radVel2, Vmax1, Vmax2, radVelRaw);
    }
    // Resolve folded range using true slant range (already computed above)
    if (det.isAmbiguous)
    {
        det.range       = signal_->resolveRangeAmbiguity(det.range, range, maxUnambiguousRange);
        det.isAmbiguous = false;
    }

    if (config_.mode == RadarMode::LOCK_ON)
        signal_->resolveRangeForLockOn(det, range, maxUnambiguousRange,
                                        target.id, tracker_->database());
    else if (!det.isAmbiguous && det.range < config_.minDetectableRange)
        det.range = config_.minDetectableRange;

    if (signal_->shouldMergeDetection(det, scanDets, config_)) return false;

    scanDets.push_back(det);
    library_->accumulate(target.id, Pr, simTime, config_, wf, false);

    // FIX-03  DRFM ghost
    if (target.jammer.active && target.jammer.type == JammerType::DRFM)
        injectDRFMGhost(target, beam, simTime, maxUnambiguousRange,
                        scanDets, rNoise, azNoise, elNoise, dvNoise);
    // ADD THIS:
    if (target.jammer.active &&
        (target.jammer.rgpoActive || target.jammer.vgpoActive))
        injectRGPOVGPO(target, beam, simTime, 0.05, maxUnambiguousRange,
                       scanDets, rNoise, azNoise, elNoise, dvNoise);
    return true;
}

// =============================================================================
// FIX-03  DRFM ghost injection
// =============================================================================

void RadarModel_AESA::injectDRFMGhost(
    const TargetInput& real, const BeamRequest& beam,
    double simTime, double maxUnambiguousRange,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    const auto& j = real.jammer;
    if (!j.gateStealingActive) return;

    drfmPullOff_[real.id] += static_cast<double>(j.drfmPullOffRate_m_s) * 0.05;
    drfmPullOff_[real.id]  = std::min(drfmPullOff_[real.id], 3.0 * maxUnambiguousRange);

    double range = std::sqrt(real.x*real.x + real.y*real.y + real.z*real.z);
    if (range < 1.0) return;

    double gAz = std::atan2(real.y, real.x) * (180.0/M_PI);
    //if (gAz < 0.0) gAz += 360.0;
    double gEl = std::asin(std::clamp(real.z/range,-1.0,1.0)) * (180.0/M_PI);

    DetectionOutput ghost;
    ghost.targetID       = real.id;
    ghost.range          = range + drfmPullOff_[real.id] + rNoise(tl_rng);
    ghost.azimuth        = gAz + azNoise(tl_rng);
    ghost.elevation      = gEl + elNoise(tl_rng);
    ghost.radialVelocity = -(real.vx*real.x+real.vy*real.y+real.vz*real.z)/range
                           + static_cast<double>(j.drfmVelocityOffset_m_s)
                           + dvNoise(tl_rng);
    ghost.snr            = scanDets.empty() ? 1.0 : 0.6 * scanDets.back().snr;
    ghost.isDRFMGhost    = true;
    ghost.isAmbiguous    = (ghost.range > maxUnambiguousRange);
    ghost.Pk             = 0.0;

    if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
    {
        scanDets.push_back(ghost);
        library_->accumulate(real.id, 0.0, simTime, config_, beam.waveform, true);
    }
}
double RadarModel_AESA::computeKnifeEdgeDiffraction(double nu) const
{
    if (nu < -0.78) return 0.0;

    double J_dB;
    if (nu <= 2.4)
        J_dB = 6.02 + 9.11*nu + 1.27*nu*nu;
    else
        J_dB = 20.0 * std::log10(nu) + 13.0;

    return std::max(0.0, J_dB);
}

OcclusionResult RadarModel_AESA::computeOcclusion(
    const TargetInput& candidate,
    const std::vector<TargetInput>& allTargets,
    const RadarConfig& cfg) const
{
    OcclusionResult result;

    double cx = candidate.x;
    double cy = candidate.y;
    double cz = candidate.z;
    double d_total = std::sqrt(cx*cx + cy*cy + cz*cz);
    if (d_total < 1.0) return result;

    double ux = cx / d_total;
    double uy = cy / d_total;
    double uz = cz / d_total;

    double lambda = SPEED_OF_LIGHT / cfg.frequency_Hz;
    double totalLoss_dB = 0.0;

    for (const auto& other : allTargets)
    {
        if (other.id == candidate.id) continue;

        double ox = other.x;
        double oy = other.y;
        double oz = other.z;

        double d1 = std::sqrt(ox*ox + oy*oy + oz*oz);
        if (d1 < 1.0 || d1 >= d_total) continue;

        double d2 = d_total - d1;
        if (d2 < 1.0) continue;

        double dot  = ox*ux + oy*uy + oz*uz;
        double perpX = ox - dot*ux;
        double perpY = oy - dot*uy;
        double perpZ = oz - dot*uz;
        double perpDist = std::sqrt(perpX*perpX + perpY*perpY + perpZ*perpZ);

        double occluderRadius;
        if (other.dimensions.valid)
            occluderRadius = 0.5 * std::min(other.dimensions.width,
                                            other.dimensions.height);
        else if (other.platformType == "SHIP")    occluderRadius = 15.0;
        else if (other.platformType == "BOMBER")  occluderRadius = 12.0;
        else                                      occluderRadius =  5.0;

        double h  = occluderRadius - perpDist;
        double r1 = std::sqrt(lambda * d1 * d2 / (d1 + d2));

        if (-h > 0.577 * r1) continue;

        double nu      = h * std::sqrt(2.0 * (d1 + d2) / (lambda * d1 * d2));
        double loss_dB = computeKnifeEdgeDiffraction(nu);
        totalLoss_dB  += 2.0 * loss_dB;  // two-way
    }

    result.diffractionLoss_dB = totalLoss_dB;
    result.powerReduction     = std::pow(10.0, -totalLoss_dB / 10.0);

    if      (totalLoss_dB >= 40.0) result.zone = OcclusionResult::Zone::SHADOW;
    else if (totalLoss_dB >=  6.0) result.zone = OcclusionResult::Zone::PENUMBRA;
    else                           result.zone = OcclusionResult::Zone::LIT;

    return result;
}
// =============================================================================
// §8  Utilities
// =============================================================================

void RadarModel_AESA::rebuildSchedule()
{
    scheduler_->buildSchedule(config_, tracker_->database());
}

double RadarModel_AESA::computeMaxDetectionRange(double rcs) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return computeMaxDetectionRange_locked(rcs);
}

double RadarModel_AESA::computeMaxDetectionRange_locked(double rcs) const
{
    return signal_->computeMaxDetectionRange(rcs, config_);
}

double RadarModel_AESA::resolveRangeAmbiguity(double measured,
                                               double predicted,
                                               double Rmax) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return signal_->resolveRangeAmbiguity(measured, predicted, Rmax);
}
void RadarModel_AESA::injectRGPOVGPO(
    const TargetInput& real, const BeamRequest& beam,
    double simTime, double dt, double maxUnambiguousRange,
    std::vector<DetectionOutput>& scanDets,
    std::normal_distribution<double>& rNoise,
    std::normal_distribution<double>& azNoise,
    std::normal_distribution<double>& elNoise,
    std::normal_distribution<double>& dvNoise)
{
    const auto& j = real.jammer;

    double range = std::sqrt(real.x*real.x + real.y*real.y + real.z*real.z);
    if (range < 1.0) return;

    double gAz = std::atan2(real.y, real.x) * (180.0 / M_PI);
    double gEl = std::asin(std::clamp(real.z/range, -1.0, 1.0)) * (180.0/M_PI);

    // ── RGPO — false target walks away in range ───────────────────────────
    if (j.rgpoActive)
    {
        rgpoPullOff_[real.id] += static_cast<double>(j.rgpoRate_m_s) * dt;
        rgpoPullOff_[real.id]  = std::min(rgpoPullOff_[real.id],
                                         static_cast<double>(j.rgpoMaxOffset_m));

        DetectionOutput ghost;
        ghost.targetID       = real.id;
        ghost.range          = range + rgpoPullOff_[real.id] + rNoise(tl_rng);
        ghost.azimuth        = gAz + azNoise(tl_rng);
        ghost.elevation      = gEl + elNoise(tl_rng);
        ghost.radialVelocity = (real.vx*real.x + real.vy*real.y + real.vz*real.z)
                                   / range + dvNoise(tl_rng);
        ghost.snr            = scanDets.empty() ? 1.0 : scanDets.back().snr * 1.2;
        ghost.isDRFMGhost    = true;
        ghost.isAmbiguous    = (ghost.range > maxUnambiguousRange);
        ghost.Pk             = 0.0;

        if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
            scanDets.push_back(ghost);
    }

    // ── VGPO — false Doppler walks away from real velocity ────────────────
    if (j.vgpoActive)
    {
        vgpoPullOff_[real.id] += static_cast<double>(j.vgpoRate_m_s2) * dt;
        vgpoPullOff_[real.id]  = std::min(vgpoPullOff_[real.id],
                                         static_cast<double>(j.vgpoMaxOffset_m_s));

        DetectionOutput ghost;
        ghost.targetID       = real.id;
        ghost.range          = range + rNoise(tl_rng);
        ghost.azimuth        = gAz + azNoise(tl_rng);
        ghost.elevation      = gEl + elNoise(tl_rng);
        ghost.radialVelocity = (real.vx*real.x + real.vy*real.y + real.vz*real.z)
                                   / range + vgpoPullOff_[real.id] + dvNoise(tl_rng);
        ghost.snr            = scanDets.empty() ? 1.0 : scanDets.back().snr * 1.1;
        ghost.isDRFMGhost    = true;
        ghost.isAmbiguous    = false;
        ghost.Pk             = 0.0;

        if (!signal_->shouldMergeDetection(ghost, scanDets, config_))
            scanDets.push_back(ghost);
    }
}
} // namespace aesa

// =============================================================================
// C ABI
// =============================================================================

extern "C"
{
aesa::RadarModel_AESA* aesaradar_create() { return new aesa::RadarModel_AESA(); }
void aesaradar_destroy(aesa::RadarModel_AESA* p) { delete p; }
}
