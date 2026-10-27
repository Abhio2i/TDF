// #include "csmdetector.h"
// #include "csmfrequency.h"
// #include "csmsignalmodel.h"

// #include <algorithm>
// #include <cmath>
// #include <set>
// #include <tuple>
// #include <utility>

// namespace csm {

// Eligibility Detector::classify(
//     const EmitterInput& e,
//     const std::string& observerId,
//     const MonitorConfig& m)
// {
//     // Perform basic eligibility checks first.
//     if (e.platformId == observerId)
//         return Eligibility::ObserverSelf;

//     if (!e.platformActive)
//         return Eligibility::PlatformInactive;

//     if (!e.radioActive)
//         return Eligibility::RadioInactive;

//     if (!e.canTransmit)
//         return Eligibility::NotTransmitting;

//     if (!e.inCoverage)
//         return Eligibility::OutOfCoverage;

//     // Check emitter frequency against the monitored band
//     // and the emitter radio's own tuning limits.
//     const freq::BandEval fe = freq::evaluate(
//         e.frequencyHz,
//         e.bandwidthHz,
//         e.minFreqHz,
//         e.maxFreqHz,
//         m.centreHz,
//         m.bandwidthHz);

//     switch (fe.status) {
//     case freq::Status::InBand:
//         return Eligibility::Eligible;

//     case freq::Status::OutsideTuningRange:
//         return Eligibility::FrequencyOutsideTuning;

//     case freq::Status::OutOfBand:
//         return Eligibility::FrequencyOutOfBand;

//     default:
//         return Eligibility::FrequencyInvalid;
//     }
// }

// ScanResult Detector::evaluate(
//     const std::vector<EmitterInput>& emitters,
//     const std::string& observerId,
//     const MonitorConfig& m,
//     const PathLossFn& pathLoss,
//     double simTimeS)
// {
//     ScanResult out;

//     // Sort emitters to ensure deterministic results.
//     std::vector<const EmitterInput*> sorted;
//     sorted.reserve(emitters.size());

//     for (const auto& e : emitters)
//         sorted.push_back(&e);

//     std::stable_sort(
//         sorted.begin(),
//         sorted.end(),
//         [](const EmitterInput* a, const EmitterInput* b) {
//             return std::tie(a->platformId, a->radioId)
//             < std::tie(b->platformId, b->radioId);
//         });

//     // Track duplicate platform-radio combinations.
//     std::set<std::pair<std::string, std::string>> seen;

//     ScanStats& st = out.stats;

//     for (const EmitterInput* ep : sorted) {
//         const EmitterInput& e = *ep;

//         ++st.emittersSeen;

//         if (!seen.insert({e.platformId, e.radioId}).second) {
//             ++st.duplicates;
//             continue;
//         }

//         // Reject emitters that fail eligibility checks.
//         switch (classify(e, observerId, m)) {
//         case Eligibility::ObserverSelf:
//             ++st.observerSelf;
//             continue;

//         case Eligibility::PlatformInactive:
//             ++st.platformInactive;
//             continue;

//         case Eligibility::RadioInactive:
//             ++st.radioInactive;
//             continue;

//         case Eligibility::NotTransmitting:
//             ++st.notTransmitting;
//             continue;

//         case Eligibility::OutOfCoverage:
//             ++st.outOfCoverage;
//             continue;

//         case Eligibility::FrequencyInvalid:
//         case Eligibility::FrequencyOutsideTuning:
//         case Eligibility::FrequencyOutOfBand:
//             ++st.frequencyRejected;
//             continue;

//         case Eligibility::Eligible:
//             break;
//         }

//         // Create an observation for this specific radio.
//         Observation o;

//         o.platformId = e.platformId;
//         o.platformName = e.platformName;

//         o.radioId = e.radioId;
//         o.radioName = e.radioName;

//         // Configured values: the radio's stored settings, not CSM measurements.
//         o.frequencyHz = Quantity::make(
//             e.frequencyHz,
//             Provenance::Configured);

//         o.bandwidthHz =
//             (std::isfinite(e.bandwidthHz) && e.bandwidthHz > 0.0)
//                 ? Quantity::make(
//                       e.bandwidthHz,
//                       Provenance::Configured)
//                 : Quantity();

//         // Configured modulation, not an independently classified modulation.
//         o.modulation = e.modulation;
//         o.modulationSource = e.modulation.empty()
//                                  ? Provenance::Unavailable
//                                  : Provenance::Configured;

//         // Geometry is computed from entity positions.
//         o.distanceM = Quantity::make(
//             e.distanceM,
//             Provenance::Calculated);

//         o.bearingDeg = Quantity::make(
//             e.bearingDeg,
//             Provenance::Calculated);

//         o.observedAtS = simTimeS;

//         // Received level: link-budget calculation using the path loss supplied
//         // by the caller (the existing propagation model). Model result, not a
//         // measurement. Unavailable if any input is missing.
//         const double pl = pathLoss ? pathLoss(e) : kNaN;

//         const double level = signal::receivedPowerDbm(
//             e.txPowerDbm,
//             e.txPowerDegradationDb,
//             e.txDutyCycle,
//             e.txAntennaGainDbi,
//             m.rxGainDbi,
//             pl);

//         o.levelDbm = Quantity::make(level, Provenance::Calculated);
//         if (!o.levelDbm.valid()) {
//             o.levelNote = !pathLoss || !std::isfinite(pl)
//             ? "path loss unavailable"
//             : "tx power/antenna gain or rx gain unavailable";
//         }

//         // Noise power over the signal's OCCUPIED bandwidth (matched-bandwidth
//         // idealisation: the detection filter is assumed to match the signal).
//         // The monitor span is NOT used: it is a coverage range, not the
//         // receiver's noise bandwidth. Without a configured signal bandwidth
//         // the noise/SNR are unavailable rather than guessed.
//         o.noiseFloorDbm = Quantity::make(
//             signal::thermalNoiseFloorDbm(
//                 e.bandwidthHz,
//                 m.noiseFigureDb,
//                 m.temperatureC),
//             Provenance::Calculated);
//         if (!o.noiseFloorDbm.valid()) {
//             o.noiseNote = !(std::isfinite(e.bandwidthHz) && e.bandwidthHz > 0.0)
//             ? "signal bandwidth not configured"
//             : "noise figure or temperature unavailable";
//         }

//         // SNR only when both level and noise floor are valid.
//         if (o.levelDbm.valid() && o.noiseFloorDbm.valid()) {
//             o.snrDb = Quantity::make(
//                 o.levelDbm.value - o.noiseFloorDbm.value,
//                 Provenance::Calculated);
//         }

//         // Sensitivity gating/margin only when a threshold has been configured.
//         if (std::isfinite(m.sensitivityDbm) && o.levelDbm.valid()) {
//             o.marginDb = Quantity::make(
//                 o.levelDbm.value - m.sensitivityDbm,
//                 Provenance::Calculated);
//             if (o.marginDb.value < 0.0) {
//                 ++st.belowSensitivity;
//                 continue;
//             }
//         }

//         // SNR gate (only when a minimum SNR is configured and SNR is known).
//         if (std::isfinite(m.minSnrDb) && o.snrDb.valid() &&
//             o.snrDb.value < m.minSnrDb) {
//             ++st.belowSnrThreshold;
//             continue;
//         }

//         out.observations.push_back(o);
//         ++st.detectedRadios;

//         // Aggregate matching radios under their platform.
//         // The input is sorted by platformId, so each platform
//         // occupies one consecutive group.
//         if (out.platforms.empty() ||
//             out.platforms.back().platformId != e.platformId) {

//             PlatformDetection p;

//             p.platformId = e.platformId;
//             p.distanceM = e.distanceM;
//             p.bearingDeg = e.bearingDeg;

//             out.platforms.push_back(p);
//             ++st.detectedPlatforms;
//         }

//         ++out.platforms.back().radioCount;
//     }

//     return out;
// }

// } // namespace csm








#include "csmdetector.h"
#include "csmfrequency.h"
#include "csmsignalmodel.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>
#include <utility>

namespace csm {

Eligibility Detector::classify(
    const EmitterInput& e,
    const std::string& observerId,
    const MonitorConfig& m)
{
    // Perform basic eligibility checks first.
    if (e.platformId == observerId)
        return Eligibility::ObserverSelf;

    if (!e.platformActive)
        return Eligibility::PlatformInactive;

    if (!e.radioActive)
        return Eligibility::RadioInactive;

    if (!e.canTransmit)
        return Eligibility::NotTransmitting;

    if (!e.inCoverage)
        return Eligibility::OutOfCoverage;

    // Check emitter frequency against the monitored band
    // and the emitter radio's own tuning limits.
    const freq::BandEval fe = freq::evaluate(
        e.frequencyHz,
        e.bandwidthHz,
        e.minFreqHz,
        e.maxFreqHz,
        m.centreHz,
        m.bandwidthHz);

    switch (fe.status) {
    case freq::Status::InBand:
        return Eligibility::Eligible;

    case freq::Status::OutsideTuningRange:
        return Eligibility::FrequencyOutsideTuning;

    case freq::Status::OutOfBand:
        return Eligibility::FrequencyOutOfBand;

    default:
        return Eligibility::FrequencyInvalid;
    }
}

ScanResult Detector::evaluate(
    const std::vector<EmitterInput>& emitters,
    const std::string& observerId,
    const MonitorConfig& m,
    const PathLossFn& pathLoss,
    double simTimeS,
    const std::vector<InterfererInput>* interferers)
{
    ScanResult out;

    const bool haveJam = interferers && !interferers->empty();

    // Anonymous summary: interference inside the monitored window. Needs a
    // finite monitor span; a tuned point (span 0) has no band to integrate.
    if (haveJam && std::isfinite(m.centreHz) && std::isfinite(m.bandwidthHz) &&
        m.bandwidthHz > 0.0) {
        const double mw = signal::totalInterferenceInBandMw(
            *interferers, m.rxGainDbi,
            m.centreHz - 0.5 * m.bandwidthHz, m.centreHz + 0.5 * m.bandwidthHz);
        if (mw > 0.0) {
            out.interference.present = true;
            out.interference.inMonitorDbm =
                Quantity::make(signal::mwToDbm(mw), Provenance::Calculated);
        }
    }

    // Sort emitters to ensure deterministic results.
    std::vector<const EmitterInput*> sorted;
    sorted.reserve(emitters.size());

    for (const auto& e : emitters)
        sorted.push_back(&e);

    std::stable_sort(
        sorted.begin(),
        sorted.end(),
        [](const EmitterInput* a, const EmitterInput* b) {
            return std::tie(a->platformId, a->radioId)
            < std::tie(b->platformId, b->radioId);
        });

    // Track duplicate platform-radio combinations.
    std::set<std::pair<std::string, std::string>> seen;

    ScanStats& st = out.stats;

    for (const EmitterInput* ep : sorted) {
        const EmitterInput& e = *ep;

        ++st.emittersSeen;

        if (!seen.insert({e.platformId, e.radioId}).second) {
            ++st.duplicates;
            continue;
        }

        // Reject emitters that fail eligibility checks.
        switch (classify(e, observerId, m)) {
        case Eligibility::ObserverSelf:
            ++st.observerSelf;
            continue;

        case Eligibility::PlatformInactive:
            ++st.platformInactive;
            continue;

        case Eligibility::RadioInactive:
            ++st.radioInactive;
            continue;

        case Eligibility::NotTransmitting:
            ++st.notTransmitting;
            continue;

        case Eligibility::OutOfCoverage:
            ++st.outOfCoverage;
            continue;

        case Eligibility::FrequencyInvalid:
        case Eligibility::FrequencyOutsideTuning:
        case Eligibility::FrequencyOutOfBand:
            ++st.frequencyRejected;
            continue;

        case Eligibility::Eligible:
            break;
        }

        // Create an observation for this specific radio.
        Observation o;

        o.platformId = e.platformId;
        o.platformName = e.platformName;

        o.radioId = e.radioId;
        o.radioName = e.radioName;

        // Configured values: the radio's stored settings, not CSM measurements.
        o.frequencyHz = Quantity::make(
            e.frequencyHz,
            Provenance::Configured);

        o.bandwidthHz =
            (std::isfinite(e.bandwidthHz) && e.bandwidthHz > 0.0)
                ? Quantity::make(
                      e.bandwidthHz,
                      Provenance::Configured)
                : Quantity();

        // Configured modulation, not an independently classified modulation.
        o.modulation = e.modulation;
        o.modulationSource = e.modulation.empty()
                                 ? Provenance::Unavailable
                                 : Provenance::Configured;

        // Geometry is computed from entity positions.
        o.distanceM = Quantity::make(
            e.distanceM,
            Provenance::Calculated);

        o.bearingDeg = Quantity::make(
            e.bearingDeg,
            Provenance::Calculated);

        o.observedAtS = simTimeS;

        // Received level: link-budget calculation using the path loss supplied
        // by the caller (the existing propagation model). Model result, not a
        // measurement. Unavailable if any input is missing.
        const double pl = pathLoss ? pathLoss(e) : kNaN;

        const double level = signal::receivedPowerDbm(
            e.txPowerDbm,
            e.txPowerDegradationDb,
            e.txDutyCycle,
            e.txAntennaGainDbi,
            m.rxGainDbi,
            pl);

        o.levelDbm = Quantity::make(level, Provenance::Calculated);
        if (!o.levelDbm.valid()) {
            o.levelNote = !pathLoss || !std::isfinite(pl)
            ? "path loss unavailable"
            : "tx power/antenna gain or rx gain unavailable";
        }

        // Noise power over the signal's OCCUPIED bandwidth (matched-bandwidth
        // idealisation: the detection filter is assumed to match the signal).
        // The monitor span is NOT used: it is a coverage range, not the
        // receiver's noise bandwidth. Without a configured signal bandwidth
        // the noise/SNR are unavailable rather than guessed.
        o.noiseFloorDbm = Quantity::make(
            signal::thermalNoiseFloorDbm(
                e.bandwidthHz,
                m.noiseFigureDb,
                m.temperatureC),
            Provenance::Calculated);
        if (!o.noiseFloorDbm.valid()) {
            o.noiseNote = !(std::isfinite(e.bandwidthHz) && e.bandwidthHz > 0.0)
            ? "signal bandwidth not configured"
            : "noise figure or temperature unavailable";
        }

        // SNR only when both level and noise floor are valid.
        if (o.levelDbm.valid() && o.noiseFloorDbm.valid()) {
            o.snrDb = Quantity::make(
                o.levelDbm.value - o.noiseFloorDbm.value,
                Provenance::Calculated);
        }

        // Interference inside THIS signal's occupied band (linear sum), and the
        // effective metric S/(N+I). snrDb above is left untouched. Needs a
        // configured signal bandwidth to define the band.
        double sinrGate = o.snrDb.valid() ? o.snrDb.value : kNaN;
        if (o.snrDb.valid()) o.sinrDb = o.snrDb;      // I = 0  =>  SINR == SNR
        if (haveJam && o.levelDbm.valid() && o.noiseFloorDbm.valid() &&
            std::isfinite(e.bandwidthHz) && e.bandwidthHz > 0.0) {
            const double iMw = signal::totalInterferenceInBandMw(
                *interferers, m.rxGainDbi,
                e.frequencyHz - 0.5 * e.bandwidthHz, e.frequencyHz + 0.5 * e.bandwidthHz);
            if (iMw > 0.0) {
                o.interferenceDbm = Quantity::make(signal::mwToDbm(iMw), Provenance::Calculated);
                o.sinrDb = Quantity::make(
                    signal::sinrDb(o.levelDbm.value, o.noiseFloorDbm.value, iMw),
                    Provenance::Calculated);
                sinrGate = o.sinrDb.value;
            }
        }

        // Sensitivity gating/margin only when a threshold has been configured.
        if (std::isfinite(m.sensitivityDbm) && o.levelDbm.valid()) {
            o.marginDb = Quantity::make(
                o.levelDbm.value - m.sensitivityDbm,
                Provenance::Calculated);
            if (o.marginDb.value < 0.0) {
                ++st.belowSensitivity;
                continue;
            }
        }

        // SNR gate (only when a minimum SNR is configured and SNR is known).
        // The gate uses the effective metric: SINR when interference overlaps
        // this signal, otherwise SNR (identical to the previous behaviour).
        if (std::isfinite(m.minSnrDb) && o.snrDb.valid() &&
            std::isfinite(sinrGate) && sinrGate < m.minSnrDb) {
            if (o.snrDb.value < m.minSnrDb) ++st.belowSnrThreshold;
            else                            ++st.belowSinrThreshold;   // jam-caused
            continue;
        }

        out.observations.push_back(o);
        ++st.detectedRadios;

        // Aggregate matching radios under their platform.
        // The input is sorted by platformId, so each platform
        // occupies one consecutive group.
        if (out.platforms.empty() ||
            out.platforms.back().platformId != e.platformId) {

            PlatformDetection p;

            p.platformId = e.platformId;
            p.distanceM = e.distanceM;
            p.bearingDeg = e.bearingDeg;

            out.platforms.push_back(p);
            ++st.detectedPlatforms;
        }

        ++out.platforms.back().radioCount;
    }

    return out;
}

} // namespace csm

