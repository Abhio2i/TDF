#include "csmfrequency.h"
#include <algorithm>
namespace csm {
namespace freq {

double toleranceHz(double monitorCentreHz)
{
    const double rel = std::isfinite(monitorCentreHz)
    ? std::fabs(monitorCentreHz) * 1.0e-7
    : 0.0;

    return std::max(1.0, rel);
}

static bool positiveFinite(double v)
{
    return std::isfinite(v) && v > 0.0;
}

BandEval evaluate(
    double emitterHz,
    double emitterBwHz,
    double minHz,
    double maxHz,
    double monitorHz,
    double monitorBwHz)
{
    BandEval r;

    if (!positiveFinite(emitterHz)) {
        r.status = Status::InvalidEmitter;
        return r;
    }

    // A negative or non-finite monitor bandwidth is invalid.
    if (!positiveFinite(monitorHz) ||
        (std::isfinite(monitorBwHz) ? monitorBwHz < 0.0 : true)) {
        r.status = Status::InvalidMonitor;
        return r;
    }

    r.offsetHz = emitterHz - monitorHz;

    const double tol = toleranceHz(monitorHz);

    // Enforce tuning limits only when both limits are configured
    // and meaningful.
    if (std::isfinite(minHz) &&
        std::isfinite(maxHz) &&
        minHz >= 0.0 &&
        maxHz > 0.0 &&
        minHz <= maxHz) {

        if (emitterHz < minHz - tol ||
            emitterHz > maxHz + tol) {
            r.status = Status::OutsideTuningRange;
            return r;
        }
    }

    // Unknown or non-positive emitter bandwidth is treated
    // as a zero-width emitter.
    const double eHalf =
        positiveFinite(emitterBwHz) ? emitterBwHz * 0.5 : 0.0;

    const double mHalf = monitorBwHz * 0.5;

    // Check whether the emitter's occupied band overlaps
    // the monitored frequency band.
    const bool overlap =
        (emitterHz - eHalf) <= (monitorHz + mHalf + tol) &&
        (monitorHz - mHalf) <= (emitterHz + eHalf + tol);

    r.status = overlap ? Status::InBand : Status::OutOfBand;

    return r;
}

} // namespace freq
} // namespace csm
