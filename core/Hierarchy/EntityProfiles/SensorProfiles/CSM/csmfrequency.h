#pragma once
#include <cmath>
namespace csm {
namespace freq {


constexpr double kHzPerMHz = 1.0e6;
constexpr double kHzPerGHz = 1.0e9;

inline double mhzToHz(double mhz)
{
    return mhz * kHzPerMHz;
}

inline double ghzToHz(double ghz)
{
    return ghz * kHzPerGHz;
}

inline double hzToMhz(double hz)
{
    return hz / kHzPerMHz;
}


enum class Status {
    InBand,
    OutOfBand,
    InvalidEmitter,
    InvalidMonitor,
    OutsideTuningRange
};

struct BandEval {
    Status status = Status::InvalidEmitter;
    double offsetHz = 0.0; // emitter centre - monitor centre
};

double toleranceHz(double monitorCentreHz);

BandEval evaluate(
    double emitterHz,
    double emitterBwHz,
    double minHz,
    double maxHz,
    double monitorHz,
    double monitorBwHz
    );

} // namespace freq
} // namespace csm
