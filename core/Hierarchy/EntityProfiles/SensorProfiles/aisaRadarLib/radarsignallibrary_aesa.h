#pragma once
#ifndef RADARSIGNALLIBRARY_AESA_H
#define RADARSIGNALLIBRARY_AESA_H
// radarsignallibrary_aesa.h  —  Rev 3
#include "radarmodel_aesa.h"
#include <unordered_map>

namespace aesa {

class RadarSignalLibrary_AESA
{
public:
    void clear();
    void loadLibrary(const std::vector<SignalLibraryEntry>& entries);

    // FIX-03: isDRFMGhost suppresses accumulation for ghost detections
    SignalIntercept accumulate(uint32_t targetID,
                               double receivedPower,
                               double simTime,
                               const RadarConfig& cfg,
                               const BeamWaveform& waveform,
                               bool isDRFMGhost = false);

    void getIntercepts(std::vector<SignalIntercept>& out) const;
    void pruneStale(double simTime, double coastSeconds);

private:
    std::string matchLibrary(const SignalIntercept& si) const;

    std::unordered_map<uint32_t, SignalIntercept> acc_;
    std::unordered_map<uint32_t, double>          lastSeen_;
    std::vector<SignalLibraryEntry>               lib_;
};

} // namespace aesa
#endif