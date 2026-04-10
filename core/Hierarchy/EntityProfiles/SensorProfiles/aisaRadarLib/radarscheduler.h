#pragma once
#ifndef RADARSCHEDULER_H
#define RADARSCHEDULER_H
// radarscheduler.h  —  Rev 3  FIX-08 duty-cycle enforcement
#include "radarmodel_aesa.h"
#include <vector>

namespace aesa {

class RadarScheduler
{
public:
    RadarScheduler() = default;

    void reset();
    void buildSchedule(const RadarConfig& cfg, const std::vector<TrackFile>& tracks);
    void insertFireControlBeam(uint32_t targetID, const TrackFile* track,
                               const RadarConfig& cfg);

    const BeamRequest& currentBeam() const;
    void advance(double dt_s);

    bool   scanCompleted()    const { return scanComplete_;      }
    int    searchGridSize()   const { return totalSearchBeams_;  }
    int    currentIndex()     const { return currentIndex_;      }
    int    scheduleSize()     const { return static_cast<int>(schedule_.size()); }
    double dwellElapsed_ms()  const { return dwellElapsed_ms_;   }
    double currentDutyCycle() const { return currentDutyCycle_;  } // FIX-08

private:
    void buildSearchGrid    (const RadarConfig& cfg);
    void insertTrackBeams   (const std::vector<TrackFile>& tracks, const RadarConfig& cfg);
    void interleaveSchedule ();

    BeamRequest makeTrackBeam(const TrackFile& track,
                              const RadarConfig& cfg, bool manoeuvring) const;

    static double      computeDutyCycle  (const BeamWaveform& wf);
    static BeamWaveform degradeWaveform  (const BeamWaveform& wf, double targetDuty);

    std::vector<BeamRequest> schedule_, searchGrid_, pendingTrack_;

    int    currentIndex_      = 0;
    double dwellElapsed_ms_   = 0.0;
    int    totalSearchBeams_  = 0;
    int    searchBeamsServed_ = 0;
    bool   scanComplete_      = false;
    double currentDutyCycle_  = 0.0;

    BeamRequest fallbackBeam_;
};

} // namespace aesa
#endif