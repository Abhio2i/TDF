#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmemitters.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include <cmath>

EsmEmitterQuery esmEmitterFromRadar(const Sensor* s, RFEmitterParams& out)
{
    const Radar* r = dynamic_cast<const Radar*>(s);
    if (!r) return EsmEmitterQuery::NotApplicable;

    out = RFEmitterParams();
    const RadarConfig cfg = r->getRadarConfig();
    out.emitting = r->Active;
    if (!(cfg.emissionPower_kW > 0.0) || !(cfg.frequency_Hz > 0.0) || !(cfg.pulseWidth > 0.0f))
        return EsmEmitterQuery::Unavailable;

    out.frequencyMHz   = cfg.frequency_Hz / 1.0e6;
    out.bandwidthHz    = 1.0 / double(cfg.pulseWidth);       // unmodulated-pulse approximation
    out.txPowerDbm     = 10.0 * std::log10(cfg.emissionPower_kW * 1.0e6);  // kW -> mW -> dBm
    out.txGainDbi      = cfg.antennaGain;
    out.sidelobePeakDb = cfg.peakSidelobeLevel;
    out.sidelobeAvgDb  = cfg.avgSidelobeLevel;
    out.emitterId      = cfg.emitterCode;
    out.typeLabel      = QStringLiteral("Radar");
    return out.isValid() ? EsmEmitterQuery::Ok : EsmEmitterQuery::Unavailable;
}