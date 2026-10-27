#pragma once
#include <string>
namespace csm {
namespace monitor {
constexpr double kAutoMinHz = 100.0e6;
constexpr double kAutoMaxHz = 6.0e6;
constexpr double kDefaultScanStepHz = 100.0e6;
constexpr double kDefaultScanDwellS = 0.5;
constexpr double kLegacyDefaultCentreGhz = 10.0;
enum class Mode { Monitor, Scan };
struct Settings {
    bool autoRange = true;
    Mode mode = Mode::Monitor;
    double centreHz = 0.0;        // manual centre [Hz]
    double spanHz = 0.0;          // manual span [Hz]; 0 = tuned point
    double scanStepHz = kDefaultScanStepHz;
    double scanDwellS = kDefaultScanDwellS;
};

struct Window {
    bool valid = false;
    std::string reason;           // when !valid
    bool autoRange = true;
    Mode mode = Mode::Monitor;
    double rangeLoHz = 0.0;       // full monitored range
    double rangeHiHz = 0.0;
    double centreHz = 0.0;        // window being monitored now
    double spanHz = 0.0;
    int stepIndex = 0;            // SCAN only (0-based)
    int stepCount = 1;
    double sweepPeriodS = 0.0;    // SCAN only: stepCount * dwell
};

Window resolve(const Settings& s, double simTimeS);
bool isLegacyUnconfigured(double centreGhz, double spanMhz);
const char* modeName(Mode m);
} // namespace monitor
} // namespace csm
