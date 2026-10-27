// // CSMDetector.h - per-radio CSM evaluation with platform aggregation.
// // Pure logic: takes a snapshot of emitters and returns observations.

// #pragma once

// #include "csmtypes.h"
// #include <functional>

// namespace csm {

// class Detector {
// public:
//     // Returns path loss in dB for an eligible emitter.
//     // NaN means the value is unavailable.
//     using PathLossFn = std::function<double(const EmitterInput&)>;

//     // Evaluate each radio independently and aggregate results
//     // by platform.
//     static ScanResult evaluate(
//         const std::vector<EmitterInput>& emitters,
//         const std::string& observerPlatformId,
//         const MonitorConfig& monitor,
//         const PathLossFn& pathLoss,
//         double simTimeS);

//     // Classify an individual emitter for detection eligibility.
//     static Eligibility classify(
//         const EmitterInput& emitter,
//         const std::string& observerPlatformId,
//         const MonitorConfig& monitor);
// };

// } // namespace csm






// CSMDetector.h - per-radio CSM evaluation with platform aggregation.
// Pure logic: takes a snapshot of emitters and returns observations.

#pragma once

#include "csmtypes.h"
#include <functional>

namespace csm {

class Detector {
public:
    // Returns path loss in dB for an eligible emitter.
    // NaN means the value is unavailable.
    using PathLossFn = std::function<double(const EmitterInput&)>;

    // Evaluate each radio independently and aggregate results
    // by platform.
    static ScanResult evaluate(
        const std::vector<EmitterInput>& emitters,
        const std::string& observerPlatformId,
        const MonitorConfig& monitor,
        const PathLossFn& pathLoss,
        double simTimeS,
        // Optional interference sources (already path-loss resolved, identity-
        // free). nullptr or empty = no interference: results are identical to
        // the interference-free behaviour.
        const std::vector<InterfererInput>* interferers = nullptr);

    // Classify an individual emitter for detection eligibility.
    static Eligibility classify(
        const EmitterInput& emitter,
        const std::string& observerPlatformId,
        const MonitorConfig& monitor);
};

} // namespace csm
