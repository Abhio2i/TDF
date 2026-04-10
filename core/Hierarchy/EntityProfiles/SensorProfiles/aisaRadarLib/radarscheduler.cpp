// radarscheduler.cpp  —  Rev 3
#include "radarscheduler.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace aesa {

// ---------------------------------------------------------------------------
// FIX-08  Duty cycle helpers
// ---------------------------------------------------------------------------

double RadarScheduler::computeDutyCycle(const BeamWaveform& wf)
{
    return std::clamp(
        static_cast<double>(wf.pulseWidth_s) * static_cast<double>(wf.prf_Hz),
        0.0, 1.0);
}

BeamWaveform RadarScheduler::degradeWaveform(const BeamWaveform& wf, double targetDuty)
{
    BeamWaveform out = wf;
    double tau = static_cast<double>(wf.pulseWidth_s);
    if (tau > 1e-9)
    {
        float newPRF = static_cast<float>(targetDuty / tau);
        out.prf_Hz = std::min(wf.prf_Hz, std::max(1.0f, newPRF));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RadarScheduler::reset()
{
    schedule_.clear(); searchGrid_.clear(); pendingTrack_.clear();
    currentIndex_ = 0; dwellElapsed_ms_ = 0.0;
    totalSearchBeams_ = searchBeamsServed_ = 0;
    scanComplete_ = false; currentDutyCycle_ = 0.0;
}

// ---------------------------------------------------------------------------
// Build
// ---------------------------------------------------------------------------

void RadarScheduler::buildSchedule(const RadarConfig& cfg,
                                    const std::vector<TrackFile>& tracks)
{
    schedule_.clear(); searchGrid_.clear(); pendingTrack_.clear();

    currentIndex_ = 0; dwellElapsed_ms_ = 0.0; scanComplete_ = false;

    // ADD THIS BLOCK — FC-only schedule, no search grid sweeping
    if (cfg.mode == RadarMode::LOCK_ON && cfg.lockedTargetID != 0)
    {
        const TrackFile* locked = nullptr;
        for (const auto& t : tracks)
            if (t.id == cfg.lockedTargetID) { locked = &t; break; }
        insertFireControlBeam(cfg.lockedTargetID, locked, cfg);
        totalSearchBeams_  = 1;
        searchBeamsServed_ = 0;
        return;
    }
    buildSearchGrid(cfg);
    insertTrackBeams(tracks, cfg);
    interleaveSchedule();

    if (cfg.mode == RadarMode::LOCK_ON && cfg.lockedTargetID != 0)
    {
        const TrackFile* locked = nullptr;
        for (const auto& t : tracks)
            if (t.id == cfg.lockedTargetID) { locked = &t; break; }
        insertFireControlBeam(cfg.lockedTargetID, locked, cfg);
    }

    // FIX-08  Enforce duty budget across whole schedule
    double maxDuty = static_cast<double>(cfg.maxDutyCycle);
    for (auto& b : schedule_)
        if (computeDutyCycle(b.waveform) > maxDuty)
            b.waveform = degradeWaveform(b.waveform, maxDuty);

    totalSearchBeams_  = static_cast<int>(searchGrid_.size());
    searchBeamsServed_ = 0;
    currentIndex_      = 0;
    dwellElapsed_ms_   = 0.0;
    scanComplete_      = false;

    if (schedule_.empty())
    {
        fallbackBeam_.task          = BeamRequest::Task::SEARCH;
        fallbackBeam_.azimuth_deg   = 0.0;
        fallbackBeam_.elevation_deg = 0.0;
        fallbackBeam_.dwellTime_ms  = cfg.searchDwellTime_ms;
        fallbackBeam_.waveform      = cfg.searchWaveform;
        schedule_.push_back(fallbackBeam_);
    }
}

void RadarScheduler::insertFireControlBeam(uint32_t targetID,
                                            const TrackFile* track,
                                            const RadarConfig& cfg)
{
    BeamRequest fc;
    fc.task         = BeamRequest::Task::FIRE_CONTROL;
    fc.targetID     = targetID;
    fc.dwellTime_ms = cfg.fireControlDwellTime_ms;
    fc.priority     = 100;
    fc.waveform     = cfg.fireControlWaveform;
    fc.spoilFactor  = 1.0f;

    if (track && track->predictedRange > 1.0)
    {
        fc.azimuth_deg   = std::atan2(track->y, track->x) * (180.0 / M_PI);
        //if (fc.azimuth_deg < 0.0) fc.azimuth_deg += 360.0;
        fc.elevation_deg = std::asin(
            std::clamp(track->z / track->predictedRange, -1.0, 1.0)) * (180.0 / M_PI);
    }
    schedule_.insert(schedule_.begin(), fc);
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

const BeamRequest& RadarScheduler::currentBeam() const
{
    if (schedule_.empty()) return fallbackBeam_;
    return schedule_[currentIndex_];
}

void RadarScheduler::advance(double dt_s)
{
    if (schedule_.empty()) return;
    scanComplete_ = false;

    const BeamRequest& curr = schedule_[currentIndex_];
    currentDutyCycle_ = computeDutyCycle(curr.waveform);

    dwellElapsed_ms_ += dt_s * 1000.0;
    if (dwellElapsed_ms_ < curr.dwellTime_ms) return;

    dwellElapsed_ms_ = 0.0;

    // if (curr.task == BeamRequest::Task::SEARCH)
    // {
    //     if (++searchBeamsServed_ >= totalSearchBeams_)
    //     { searchBeamsServed_ = 0; scanComplete_ = true; }
    // }
    if (curr.task == BeamRequest::Task::SEARCH)
    {
        if (++searchBeamsServed_ >= totalSearchBeams_)
        { searchBeamsServed_ = 0; scanComplete_ = true; }

        if (!pendingTrack_.empty())
        {
            BeamRequest tb = pendingTrack_.front();
            pendingTrack_.erase(pendingTrack_.begin());
            int pos = std::min(currentIndex_ + 1,
                               static_cast<int>(schedule_.size()));
            schedule_.insert(schedule_.begin() + pos, tb);
        }
    }
    if (curr.task == BeamRequest::Task::FIRE_CONTROL)
    {
       // schedule_.erase(schedule_.begin() + currentIndex_);
        //if (!schedule_.empty())
            currentIndex_ = currentIndex_ % static_cast<int>(schedule_.size());
        return;
    }

    currentIndex_ = (currentIndex_ + 1) % static_cast<int>(schedule_.size());
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void RadarScheduler::buildSearchGrid(const RadarConfig& cfg)
{
    double azStep = static_cast<double>(cfg.beamWidth) *2.0;
    double elStep = static_cast<double>(cfg.beamWidth) *2.0;
    for (double el = cfg.maxElevation; el >= cfg.minElevation - 0.01; el -= elStep)
        for (double az = cfg.minAzimuth; az <= cfg.maxAzimuth + 0.01; az += azStep)
        {
            BeamRequest r;
            r.task          = BeamRequest::Task::SEARCH;
            r.azimuth_deg   = az; r.elevation_deg = el;
            r.dwellTime_ms  = cfg.searchDwellTime_ms;
            r.priority      = 0;
            r.waveform      = cfg.searchWaveform;
            r.spoilFactor   = 1.0f;
            searchGrid_.push_back(r);
        }
}

void RadarScheduler::insertTrackBeams(const std::vector<TrackFile>& tracks,
                                       const RadarConfig& cfg)
{
    for (const auto& t : tracks)
    {
        if (!t.isValidated) continue;
        BeamRequest tb = makeTrackBeam(t, cfg, t.isManoeuvring);
        pendingTrack_.push_back(tb);
        if (t.isManoeuvring) pendingTrack_.push_back(tb); // double-rate for manoeuvring
    }
}

void RadarScheduler::interleaveSchedule()
{
    schedule_ = searchGrid_;
    if (pendingTrack_.empty()) return;

    int ns = static_cast<int>(schedule_.size());
    int nt = static_cast<int>(pendingTrack_.size());
    int interval = std::max(1, ns / nt);
    int offset   = interval;

    for (int ti = 0; ti < nt; ++ti)
    {
        int pos = std::min(offset, static_cast<int>(schedule_.size()));
        schedule_.insert(schedule_.begin() + pos, pendingTrack_[ti]);
        offset += interval + 1;
    }
}

BeamRequest RadarScheduler::makeTrackBeam(const TrackFile& t,
                                           const RadarConfig& cfg,
                                           bool manoeuvring) const
{
    BeamRequest r;
    r.task        = BeamRequest::Task::TRACK;
    r.targetID    = t.id;
    r.dwellTime_ms= cfg.trackDwellTime_ms;
    r.priority    = manoeuvring ? 20 : 10;
    r.waveform    = cfg.trackWaveform;
    r.spoilFactor = 1.0f;

    if (t.predictedRange > 1.0)
    {
        r.azimuth_deg  = std::atan2(t.y, t.x) * (180.0 / M_PI);
        //if (r.azimuth_deg < 0.0) r.azimuth_deg += 360.0;
        r.elevation_deg = std::asin(
            std::clamp(t.z / t.predictedRange, -1.0, 1.0)) * (180.0 / M_PI);
    }
    return r;
}

} // namespace aesa
