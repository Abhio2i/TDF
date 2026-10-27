#ifndef ESMEMITTERS_H
#define ESMEMITTERS_H
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/ESM/esmrf.h"

/* NotApplicable = sensor is not this kind; Unavailable = it is, but has no usable RF data. */
enum class EsmEmitterQuery { NotApplicable, Unavailable, Ok };

EsmEmitterQuery esmEmitterFromAesa (const Sensor* s, RFEmitterParams& out);  // esmemitter_aesa.cpp
EsmEmitterQuery esmEmitterFromRadar(const Sensor* s, RFEmitterParams& out);  // esmemitter_radar.cpp

inline EsmEmitterQuery esmQueryEmitter(const Sensor* s, RFEmitterParams& out)
{
    if (const IRFEmitter* i = dynamic_cast<const IRFEmitter*>(s))      // future sensors
        return i->rfEmitterParams(out) ? EsmEmitterQuery::Ok : EsmEmitterQuery::Unavailable;
    EsmEmitterQuery q = esmEmitterFromAesa(s, out);
    if (q != EsmEmitterQuery::NotApplicable) return q;
    return esmEmitterFromRadar(s, out);
}
#endif