#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmemitters.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include <algorithm>
#include <cmath>
#include <QDebug>
#include <QString>

EsmEmitterQuery esmEmitterFromAesa(const Sensor* s, RFEmitterParams& out)
{
    const AESARadar* a = dynamic_cast<const AESARadar*>(s);
    if (!a) return EsmEmitterQuery::NotApplicable;

    out = RFEmitterParams();
    const aesa::RadarConfig cfg = a->getRadarConfig();
    //Vansh
    qDebug() << "[ESM-AESA]"
             << "Emitter:" << QString::fromStdString(a->Name)
             << "frequency_Hz:" << cfg.frequency_Hz
             << "frequency_MHz:" << cfg.frequency_Hz / 1.0e6;

    out.emitting = a->Active;
    //Vansh

    // Same power maths as computeERP_dBm() in aesaradar.cpp (peak, total).
    const int    numActive = std::max(0, cfg.numElements - cfg.failedModules);
    const double ptW = double(numActive) * cfg.peakPowerPerElement_W * cfg.moduleEfficiency;

    // Waveform currently in use (same mapping as currentWaveformFor()).
    const aesa::BeamWaveform* wf = &cfg.searchWaveform;
    switch (a->getMode()) {
    case aesa::RadarMode::LOCK_ON: wf = &cfg.fireControlWaveform; break;
    case aesa::RadarMode::TWS:     wf = &cfg.trackWaveform;       break;
    default: break;
    }
    double bwHz = wf->bandwidth_Hz;
    if (!(bwHz > 0.0) && wf->pulseWidth_s > 0.0f) bwHz = 1.0 / double(wf->pulseWidth_s);

    if (!(ptW > 0.0) || !(cfg.frequency_Hz > 0.0) || !(bwHz > 0.0))
        return EsmEmitterQuery::Unavailable;                 // never invent data

    out.frequencyMHz   = cfg.frequency_Hz / 1.0e6;           // nominal (agility not modelled)
    out.bandwidthHz    = bwHz;
    out.txPowerDbm     = 10.0 * std::log10(ptW * 1000.0);    // W -> mW -> dBm
    out.txGainDbi      = cfg.antennaGain;
    out.sidelobePeakDb = cfg.peakSidelobeLevel;              // dB relative to main beam
    out.sidelobeAvgDb  = cfg.avgSidelobeLevel;
    out.emitterId      = cfg.emitterCode;                    // 0 -> ESM derives an ID
    out.typeLabel      = QStringLiteral("AESA");
    return out.isValid() ? EsmEmitterQuery::Ok : EsmEmitterQuery::Unavailable;
}