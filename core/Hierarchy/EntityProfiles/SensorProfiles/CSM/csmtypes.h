// #pragma once
// #include <cmath>
// #include <cstdint>
// #include <limits>
// #include <string>
// #include <vector>

// namespace csm {

// constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// // Where a displayed number comes from. Categories are never converted silently:
// //   Unavailable : source data or processing capability is missing.
// //   Configured  : a stored radio/sensor parameter (not a measurement).
// //   Simulated   : produced by the simulation engine (e.g. receiver samples).
// //                 Reserved: the current engine produces no waveform samples.
// //   Calculated  : computed from identified inputs with a documented relationship
// //                 (e.g. link-budget received level). A model result, not a
// //                 physical measurement.
// //   Classified  : output of an implemented, validated classifier.
// //                 Reserved: no classifier exists.
// enum class Provenance {
//     Unavailable,
//     Configured,
//     Simulated,
//     Calculated,
//     Classified
// };

// struct Quantity {
//     double value = kNaN;
//     Provenance source = Provenance::Unavailable;

//     bool valid() const
//     {
//         return source != Provenance::Unavailable &&
//                std::isfinite(value);
//     }

//     static Quantity make(double v, Provenance p)
//     {
//         Quantity q;

//         if (std::isfinite(v)) {
//             q.value = v;
//             q.source = p;
//         }

//         return q;
//     }
// };

// // Why an emitter was or was not accepted.
// enum class Eligibility {
//     Eligible,
//     ObserverSelf,
//     PlatformInactive,
//     RadioInactive,
//     NotTransmitting,
//     OutOfCoverage,
//     FrequencyInvalid,
//     FrequencyOutsideTuning,
//     FrequencyOutOfBand
// };

// // One radio, as seen by the detector.
// // Contains no pointers, so it cannot retain dangling hierarchy references.
// struct EmitterInput {
//     std::string platformId, platformName;
//     std::string radioId, radioName;

//     bool platformActive = false;
//     bool radioActive = false;
//     bool canTransmit = true;
//     bool inCoverage = false;

//     // Observer-to-emitter geometry.
//     double distanceM = kNaN;
//     double bearingDeg = kNaN;

//     // Radio configuration.
//     // Frequency: Hz
//     // Bandwidth: Hz
//     // Power: dBm
//     // Gains and degradation: dB
//     double frequencyHz = kNaN;
//     double bandwidthHz = kNaN;
//     double minFreqHz = kNaN;
//     double maxFreqHz = kNaN;

//     std::string modulation;

//     double txPowerDbm = kNaN;
//     double txPowerDegradationDb = 0.0;
//     double txDutyCycle = 1.0;
//     double txAntennaGainDbi = kNaN;
// };

// // CSM receiver / monitor configuration.
// struct MonitorConfig {
//     double centreHz = kNaN;

//     // 0 means tuned point; no monitoring span is defined.
//     double bandwidthHz = 0.0;

//     // NaN means "not supplied": dependent results are Unavailable rather than
//     // computed from a guessed default.
//     double rxGainDbi = kNaN;
//     double noiseFigureDb = kNaN;
//     double temperatureC = kNaN;

//     // NaN means no sensitivity threshold is defined (no gating, no margin).
//     double sensitivityDbm = kNaN;

//     // Minimum SNR [dB] (signal power / noise power in the signal's occupied
//     // bandwidth) for a detection. NaN = no SNR gating. Independent of, and
//     // applied in addition to, sensitivityDbm.
//     double minSnrDb = kNaN;
// };

// // Observation for one detected radio.
// struct Observation {
//     std::string platformId, platformName;
//     std::string radioId, radioName;

//     Quantity frequencyHz;
//     Quantity bandwidthHz;

//     // Configured modulation of the emitting radio. NOT a classification result.
//     std::string modulation;
//     Provenance modulationSource = Provenance::Unavailable;

//     Quantity distanceM;
//     Quantity bearingDeg;

//     // Identity note: platformId/radioId/names are SIMULATION METADATA (ISSST
//     // entity identifiers), not identity established by RF evidence. Likewise
//     // frequency/bandwidth/modulation above are the radio's CONFIGURATION.
//     // Only levelDbm/noiseFloorDbm/snrDb/marginDb are model calculations.
//     Quantity levelDbm;
//     // Thermal noise power over the SIGNAL's occupied bandwidth (matched
//     // bandwidth idealisation), not over the monitor span.
//     Quantity noiseFloorDbm;
//     Quantity snrDb;
//     Quantity marginDb;

//     // Why levelDbm / noiseFloorDbm / snrDb are Unavailable (empty when valid).
//     std::string levelNote;
//     std::string noiseNote;

//     double observedAtS = kNaN;
// };

// // Platform-level aggregate.
// // Used to update legacy ewtargets / ewdetects.
// struct PlatformDetection {
//     std::string platformId;

//     double distanceM = kNaN;
//     double bearingDeg = kNaN;

//     int radioCount = 0;
// };

// // Counters used for scan diagnostics and tests.
// struct ScanStats {
//     int emittersSeen = 0;
//     int duplicates = 0;
//     int observerSelf = 0;
//     int platformInactive = 0;
//     int radioInactive = 0;
//     int notTransmitting = 0;
//     int outOfCoverage = 0;
//     int frequencyRejected = 0;
//     int belowSensitivity = 0;
//     int belowSnrThreshold = 0;
//     int detectedRadios = 0;
//     int detectedPlatforms = 0;

//     bool operator==(const ScanStats& o) const
//     {
//         return emittersSeen == o.emittersSeen &&
//                duplicates == o.duplicates &&
//                observerSelf == o.observerSelf &&
//                platformInactive == o.platformInactive &&
//                radioInactive == o.radioInactive &&
//                notTransmitting == o.notTransmitting &&
//                outOfCoverage == o.outOfCoverage &&
//                frequencyRejected == o.frequencyRejected &&
//                belowSensitivity == o.belowSensitivity &&
//                belowSnrThreshold == o.belowSnrThreshold &&
//                detectedRadios == o.detectedRadios &&
//                detectedPlatforms == o.detectedPlatforms;
//     }

//     bool operator!=(const ScanStats& o) const
//     {
//         return !(*this == o);
//     }
// };

// // Complete result of one scan.
// struct ScanResult {
//     std::vector<Observation> observations;
//     std::vector<PlatformDetection> platforms;
//     ScanStats stats;
// };

// } // namespace csm




#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace csm {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// Where a displayed number comes from. Categories are never converted silently:
//   Unavailable : source data or processing capability is missing.
//   Configured  : a stored radio/sensor parameter (not a measurement).
//   Simulated   : produced by the simulation engine (e.g. receiver samples).
//                 Reserved: the current engine produces no waveform samples.
//   Calculated  : computed from identified inputs with a documented relationship
//                 (e.g. link-budget received level). A model result, not a
//                 physical measurement.
//   Classified  : output of an implemented, validated classifier.
//                 Reserved: no classifier exists.
enum class Provenance {
    Unavailable,
    Configured,
    Simulated,
    Calculated,
    Classified
};

struct Quantity {
    double value = kNaN;
    Provenance source = Provenance::Unavailable;

    bool valid() const
    {
        return source != Provenance::Unavailable &&
               std::isfinite(value);
    }

    static Quantity make(double v, Provenance p)
    {
        Quantity q;

        if (std::isfinite(v)) {
            q.value = v;
            q.source = p;
        }

        return q;
    }
};

// Why an emitter was or was not accepted.
enum class Eligibility {
    Eligible,
    ObserverSelf,
    PlatformInactive,
    RadioInactive,
    NotTransmitting,
    OutOfCoverage,
    FrequencyInvalid,
    FrequencyOutsideTuning,
    FrequencyOutOfBand
};

// CSM operating mode. RECEIVE is the existing electronic-support receiver.
// TRANSMIT makes the same CSM an interference (jammer) source and suspends its
// receive processing.
enum class OperatingMode { Receive, Transmit };

inline const char* operatingModeName(OperatingMode m)
{
    return m == OperatingMode::Transmit ? "TRANSMIT" : "RECEIVE";
}

// One interference source, as seen by a receiving CSM.
// Pointer-free AND identity-free by design: no entity ID, name, distance or
// bearing, so none of them can reach the observation table.
// MODEL (documented assumption): a noise (barrage) jammer. The power that
// arrives at the receiver is spread UNIFORMLY over bandwidthHz centred on
// frequencyHz (flat PSD). No waveform, hopping or adaptation is modelled.
struct InterfererInput {
    bool inCoverage = false;          // inside the receiving CSM's range/coverage
    double frequencyHz = kNaN;        // centre [Hz]
    double bandwidthHz = kNaN;        // occupied bandwidth [Hz], must be > 0
    double txPowerDbm = kNaN;         // transmit power [dBm]
    double txGainDbi = kNaN;          // transmit antenna gain [dBi]
    double pathLossDb = kNaN;         // from the existing propagation model [dB]
};

// One radio, as seen by the detector.
// Contains no pointers, so it cannot retain dangling hierarchy references.
struct EmitterInput {
    std::string platformId, platformName;
    std::string radioId, radioName;

    bool platformActive = false;
    bool radioActive = false;
    bool canTransmit = true;
    bool inCoverage = false;

    // Observer-to-emitter geometry.
    double distanceM = kNaN;
    double bearingDeg = kNaN;

    // Radio configuration.
    // Frequency: Hz
    // Bandwidth: Hz
    // Power: dBm
    // Gains and degradation: dB
    double frequencyHz = kNaN;
    double bandwidthHz = kNaN;
    double minFreqHz = kNaN;
    double maxFreqHz = kNaN;

    std::string modulation;

    double txPowerDbm = kNaN;
    double txPowerDegradationDb = 0.0;
    double txDutyCycle = 1.0;
    double txAntennaGainDbi = kNaN;
};

// CSM receiver / monitor configuration.
struct MonitorConfig {
    double centreHz = kNaN;

    // 0 means tuned point; no monitoring span is defined.
    double bandwidthHz = 0.0;

    // NaN means "not supplied": dependent results are Unavailable rather than
    // computed from a guessed default.
    double rxGainDbi = kNaN;
    double noiseFigureDb = kNaN;
    double temperatureC = kNaN;

    // NaN means no sensitivity threshold is defined (no gating, no margin).
    double sensitivityDbm = kNaN;

    // Minimum SNR [dB] (signal power / noise power in the signal's occupied
    // bandwidth) for a detection. NaN = no SNR gating. Independent of, and
    // applied in addition to, sensitivityDbm.
    double minSnrDb = kNaN;
};

// Observation for one detected radio.
struct Observation {
    std::string platformId, platformName;
    std::string radioId, radioName;

    Quantity frequencyHz;
    Quantity bandwidthHz;

    // Configured modulation of the emitting radio. NOT a classification result.
    std::string modulation;
    Provenance modulationSource = Provenance::Unavailable;

    Quantity distanceM;
    Quantity bearingDeg;

    // Identity note: platformId/radioId/names are SIMULATION METADATA (ISSST
    // entity identifiers), not identity established by RF evidence. Likewise
    // frequency/bandwidth/modulation above are the radio's CONFIGURATION.
    // Only levelDbm/noiseFloorDbm/snrDb/marginDb are model calculations.
    Quantity levelDbm;
    // Thermal noise power over the SIGNAL's occupied bandwidth (matched
    // bandwidth idealisation), not over the monitor span.
    Quantity noiseFloorDbm;
    Quantity snrDb;
    Quantity marginDb;

    // Interference-aware quantities. snrDb above keeps its meaning (S/N);
    // these are SEPARATE and are valid only when interference was modelled.
    //   interferenceDbm : interference power inside the signal's occupied band
    //   sinrDb          : S / (N + I), powers summed in LINEAR units
    // With no overlapping interference, interferenceDbm is Unavailable and
    // sinrDb equals snrDb.
    Quantity interferenceDbm;
    Quantity sinrDb;

    // Why levelDbm / noiseFloorDbm / snrDb are Unavailable (empty when valid).
    std::string levelNote;
    std::string noiseNote;

    double observedAtS = kNaN;
};

// Platform-level aggregate.
// Used to update legacy ewtargets / ewdetects.
struct PlatformDetection {
    std::string platformId;

    double distanceM = kNaN;
    double bearingDeg = kNaN;

    int radioCount = 0;
};

// Counters used for scan diagnostics and tests.
struct ScanStats {
    int emittersSeen = 0;
    int duplicates = 0;
    int observerSelf = 0;
    int platformInactive = 0;
    int radioInactive = 0;
    int notTransmitting = 0;
    int outOfCoverage = 0;
    int frequencyRejected = 0;
    int belowSensitivity = 0;
    int belowSnrThreshold = 0;
    int belowSinrThreshold = 0;   // passes S/N gate but fails S/(N+I): caused by interference
    int detectedRadios = 0;
    int detectedPlatforms = 0;

    bool operator==(const ScanStats& o) const
    {
        return emittersSeen == o.emittersSeen &&
               duplicates == o.duplicates &&
               observerSelf == o.observerSelf &&
               platformInactive == o.platformInactive &&
               radioInactive == o.radioInactive &&
               notTransmitting == o.notTransmitting &&
               outOfCoverage == o.outOfCoverage &&
               frequencyRejected == o.frequencyRejected &&
               belowSensitivity == o.belowSensitivity &&
               belowSnrThreshold == o.belowSnrThreshold &&
               belowSinrThreshold == o.belowSinrThreshold &&
               detectedRadios == o.detectedRadios &&
               detectedPlatforms == o.detectedPlatforms;
    }

    bool operator!=(const ScanStats& o) const
    {
        return !(*this == o);
    }
};

// What a receiving CSM knows about interference: that it is present and how
// strong it is inside the monitored window. Deliberately NO source identity,
// count, range or bearing.
struct InterferenceSummary {
    bool present = false;
    // Aggregate interference power inside the monitored window [dBm].
    Quantity inMonitorDbm;
};

// Complete result of one scan.
struct ScanResult {
    InterferenceSummary interference;
    std::vector<Observation> observations;
    std::vector<PlatformDetection> platforms;
    ScanStats stats;
};

} // namespace csm

