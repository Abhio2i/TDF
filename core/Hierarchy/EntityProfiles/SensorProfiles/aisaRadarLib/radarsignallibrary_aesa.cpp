// radarsignallibrary_aesa.cpp  —  Rev 3
#include "radarsignallibrary_aesa.h"
#include <cmath>

namespace aesa {

void RadarSignalLibrary_AESA::clear()
{
    acc_.clear(); lastSeen_.clear();
}

void RadarSignalLibrary_AESA::loadLibrary(const std::vector<SignalLibraryEntry>& entries)
{
    lib_ = entries;
}

SignalIntercept RadarSignalLibrary_AESA::accumulate(
    uint32_t targetID, double receivedPower, double simTime,
    const RadarConfig& cfg, const BeamWaveform& waveform,
    bool isDRFMGhost)
{
    if (isDRFMGhost) return SignalIntercept{};

    auto& si = acc_[targetID];
    si.targetID = targetID;

    double measFreq = cfg.frequency_Hz;
    if (cfg.frequencyAgility &&
        cfg.hopStopFrequency > cfg.hopStartFrequency &&
        cfg.hopStopFrequency > 0.0f)
    {
        measFreq = 0.5 * (static_cast<double>(cfg.hopStartFrequency)
                        + static_cast<double>(cfg.hopStopFrequency));
    }

    auto avg = [](double old, double newV, int n) { return (old*n + newV)/(n+1); };

    si.frequency_Hz  = si.freqCount == 0 ? measFreq : avg(si.frequency_Hz,  measFreq,  si.freqCount); si.freqCount++;

    double pri = (waveform.prf_Hz > 0.0f) ? 1.0/static_cast<double>(waveform.prf_Hz) : 0.0;
    si.pri_s         = si.priCount == 0 ? pri : avg(si.pri_s, pri, si.priCount); si.priCount++;

    double pw = static_cast<double>(waveform.pulseWidth_s);
    si.pulseWidth_s  = si.pwCount == 0 ? pw : avg(si.pulseWidth_s, pw, si.pwCount); si.pwCount++;

    double pdBW = (receivedPower > 1e-30) ? 10.0*std::log10(receivedPower) : -300.0;
    si.signalLevel_dBW = si.signalDepth == 0 ? pdBW : avg(si.signalLevel_dBW, pdBW, si.signalDepth);
    si.signalDepth++;

    si.modulation = waveform.modulation;
    si.emitterID  = matchLibrary(si);
    lastSeen_[targetID] = simTime;
    return si;
}

void RadarSignalLibrary_AESA::getIntercepts(std::vector<SignalIntercept>& out) const
{
    out.clear(); out.reserve(acc_.size());
    for (const auto& [id, si] : acc_) out.push_back(si);
}

void RadarSignalLibrary_AESA::pruneStale(double simTime, double coast)
{
    for (auto it = lastSeen_.begin(); it != lastSeen_.end(); )
    {
        if ((simTime - it->second) > coast)
        { acc_.erase(it->first); it = lastSeen_.erase(it); }
        else ++it;
    }
}

std::string RadarSignalLibrary_AESA::matchLibrary(const SignalIntercept& si) const
{
    if (si.freqCount < 3) return si.emitterID;
    for (const auto& e : lib_)
    {
        if (std::abs(si.frequency_Hz - e.frequency_Hz) > e.freqTolerance_Hz) continue;
        if (e.pri_s > 0.0 && si.priCount >= 3 && std::abs(si.pri_s - e.pri_s) > e.priTolerance_s) continue;
        if (e.pulseWidth_s > 0.0 && si.pwCount >= 3 && std::abs(si.pulseWidth_s - e.pulseWidth_s) > e.pwTolerance_s) continue;
        if (e.modulation != ModulationType::NONE && si.modulation != e.modulation) continue;
        return e.emitterID;
    }
    return si.emitterID;
}

} // namespace aesa
