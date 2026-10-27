#include "csmmonitor.h"
#include <algorithm>
#include <cmath>

namespace csm {
namespace monitor {

namespace {
bool positiveFinite(double v) { return std::isfinite(v) && v > 0.0; }
Window invalid(const Settings& s, const char* why)
{
    Window w;
    w.reason = why;
    w.autoRange = s.autoRange;
    w.mode = s.mode;
    return w;
}
} // namespace

const char* modeName(Mode m) { return m == Mode::Scan ? "SCAN" : "MONITOR"; }

bool isLegacyUnconfigured(double centreGhz, double spanMhz)
{
    return std::isfinite(centreGhz) && std::isfinite(spanMhz) &&
           std::fabs(centreGhz - kLegacyDefaultCentreGhz) < 1e-6 && spanMhz == 0.0;
}

Window resolve(const Settings& s, double simTimeS)
{
    double lo, hi;
    if (s.autoRange) {
        lo = kAutoMinHz;
        hi = kAutoMaxHz;
    } else {
        if (!positiveFinite(s.centreHz))
            return invalid(s, "monitor centre frequency invalid");
        if (!std::isfinite(s.spanHz) || s.spanHz < 0.0)
            return invalid(s, "monitor bandwidth invalid");
        lo = s.centreHz - 0.5 * s.spanHz;
        hi = s.centreHz + 0.5 * s.spanHz;
    }

    Window w;
    w.valid = true;
    w.autoRange = s.autoRange;
    w.mode = s.mode;
    w.rangeLoHz = lo;
    w.rangeHiHz = hi;
    w.centreHz = 0.5 * (lo + hi);
    w.spanHz = hi - lo;

    if (s.mode == Mode::Monitor) return w;

    // SCAN: step a narrower window across the range.
    const double range = hi - lo;
    if (!(range > 0.0))
        return invalid(s, "SCAN needs a monitor bandwidth > 0");
    if (!positiveFinite(s.scanStepHz))
        return invalid(s, "scan step invalid");
    if (!positiveFinite(s.scanDwellS))
        return invalid(s, "scan dwell invalid");

    const double step = std::min(s.scanStepHz, range);
    const int n = std::max(1, static_cast<int>(std::ceil(range / step - 1e-9)));
    const double t = std::isfinite(simTimeS) ? std::max(0.0, simTimeS) : 0.0;
    const long long tick = static_cast<long long>(std::floor(t / s.scanDwellS));
    const int k = static_cast<int>(tick % n);
    const double wlo = lo + k * step;
    const double whi = std::min(hi, wlo + step);

    w.stepIndex = k;
    w.stepCount = n;
    w.sweepPeriodS = n * s.scanDwellS;
    w.centreHz = 0.5 * (wlo + whi);
    w.spanHz = whi - wlo;
    return w;
}

} // namespace monitor
} // namespace csm
