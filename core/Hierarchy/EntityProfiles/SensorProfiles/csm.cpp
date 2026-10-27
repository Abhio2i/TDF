// #include "csm.h"
// #include "CSM/csmdetector.h"
// #include "CSM/csmfrequency.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "core/Hierarchy/EntityProfiles/radio.h"
// #include "core/Simulation/simulation.h"
// #include <cmath>
// #include <string>
// #include <unordered_map>

// namespace {

// const double kRad2Deg = 180.0 / M_PI;

// // Name of a configured modulation scheme (configuration, not a classification).
// std::string modulationName(radio::ModulationScheme m)
// {
//     using M = radio::ModulationScheme;
//     switch (m) {
//     case M::AM:         return "AM";
//     case M::FM:         return "FM";
//     case M::BPSK:       return "BPSK";
//     case M::QPSK:       return "QPSK";
//     case M::PSK8:       return "PSK8";
//     case M::QAM16:      return "QAM16";
//     case M::QAM64:      return "QAM64";
//     case M::FSK2:       return "FSK2";
//     case M::FSK4:       return "FSK4";
//     case M::GMSK:       return "GMSK";
//     case M::OFDM_BPSK:  return "OFDM-BPSK";
//     case M::OFDM_QPSK:  return "OFDM-QPSK";
//     case M::OFDM_QAM16: return "OFDM-QAM16";
//     case M::OFDM_QAM64: return "OFDM-QAM64";
//     }
//     return std::string();
// }

// // Same transform-to-model position mapping as Radio::scan() (metres), so the
// // propagation model sees identical geometry for radios and for the CSM.
// radio::Position toModelPosition(Transform& t)
// {
//     radio::Position p;
//     p.x = t.translation().z() * 1000.0;
//     p.y = t.translation().x() * 1000.0;
//     p.altitude = t.translation().y() * 1000.0;
//     return p;
// }

// std::string emitterKey(const std::string& platformId, const std::string& radioId)
// {
//     return platformId + '\x1f' + radioId;
// }

// struct Candidate {
//     radio::Position pos;
//     radio::RadioConfig cfg;
// };

// } // namespace

// CSM::CSM(Hierarchy* h) : Sensor(h) {
//     subType = SubType::CSM;
//     azimuth = 360;
//     frequency = 0.15f;     // only used when autoMonitor is false
//     bandwidth = 100.0f;
//     noiseFigure = 6.0f;     // dB, typical receiver front end (0 dB is ideal/unrealistic)
// }

// // Drop platform-derived targets; entity-less (external) tracks are not ours.
// void CSM::dropPlatformTargets()
// {
//     for (int i = ewtargets.size() - 1; i >= 0; --i) {
//         if (ewtargets.at(i).entity) ewtargets.removeAt(i);
//     }
//     ewdetects.clear();
// }

// void CSM::clearScanState()
// {
//     lastScan_ = csm::ScanResult();
//     spectrumCache_.clear();
//     held_.clear();
//     lastMonitor_ = csm::monitor::Window();
//     dropPlatformTargets();
// }

// void CSM::scan()
// {
//     if (!Active || !parentEntity || !root) {
//         clearScanState();
//         return;
//     }
//     auto obsIt = root->Platforms.find(parentEntity->ID);
//     if (obsIt == root->Platforms.end() || !obsIt->second) {
//         clearScanState();
//         return;
//     }
//     Platform* observer = obsIt->second;
//     Transform& source = observer->transform;
//     const radio::Position observerPos = toModelPosition(source);

//     // --- Snapshot every radio as a pointer-free EmitterInput ---------------
//     std::vector<csm::EmitterInput> emitters;
//     std::unordered_map<std::string, Candidate> candidates;
//     emitters.reserve(root->Radios.size());

//     for (auto const& kv : root->Radios) {
//         Radio* r = kv.second;
//         if (!r || !r->parentEntity) continue;
//         auto pit = root->Platforms.find(r->parentEntity->ID);
//         if (pit == root->Platforms.end() || !pit->second) continue;
//         Platform* platform = pit->second;

//         csm::EmitterInput e;
//         e.platformId = platform->ID;
//         e.platformName = platform->Name;
//         e.radioId = r->ID;
//         e.radioName = r->Name;
//         e.platformActive = platform->Active;
//         e.radioActive = r->Active;
//         const QVector3D localPos =
//             source.inverseTransformPoint(platform->transform.matrix.translation());
//         e.distanceM = distanceBetween(source.getLatitude(), source.getLongitude(),
//                                       platform->transform.getLatitude(),
//                                       platform->transform.getLongitude());
//         e.bearingDeg = std::atan2(localPos.x(), localPos.z()) * kRad2Deg;
//         e.inCoverage = detectCheck(localPos, static_cast<float>(e.distanceM / 1000.0));
//         radio::RadioConfig cfg;
//         if (r->lib_radio) {
//             cfg = r->lib_radio->getConfiguration();
//             e.frequencyHz = cfg.frequency_hz;
//             e.bandwidthHz = cfg.bandwidth_hz;
//             e.minFreqHz = cfg.min_freq_hz;
//             e.maxFreqHz = cfg.max_freq_hz;
//             e.canTransmit = (cfg.mode != radio::RadioMode::RECEIVER_ONLY) &&
//                             r->lib_radio->isPoweredOn();
//             e.modulation = modulationName(cfg.modulation_scheme);
//             e.txPowerDbm = cfg.tx_power_dbm;
//             e.txPowerDegradationDb = cfg.power_degradation_db;
//             e.txDutyCycle = cfg.tx_duty_cycle;
//             e.txAntennaGainDbi = cfg.antenna.gain_dbi;
//         }

//         candidates[emitterKey(e.platformId, e.radioId)] = { toModelPosition(platform->transform), cfg };
//         emitters.push_back(std::move(e));
//     }
//     const double nowS = Simulation::simulationTime;
//     csm::monitor::Settings ms;
//     ms.autoRange = autoMonitor;
//     ms.mode = scanMode ? csm::monitor::Mode::Scan : csm::monitor::Mode::Monitor;
//     ms.centreHz = csm::freq::ghzToHz(frequency);
//     ms.spanHz = (std::isfinite(bandwidth) && bandwidth > 0.0f)
//                     ? static_cast<double>(bandwidth) * csm::freq::kHzPerMHz
//                     : 0.0;
//     ms.scanStepHz = csm::freq::mhzToHz(scanStepMhz);
//     ms.scanDwellS = scanDwellS;
//     lastMonitor_ = csm::monitor::resolve(ms, nowS);

//     radio::PropagationModel* model = Radio::model;
//     csm::MonitorConfig mon;
//     mon.centreHz = lastMonitor_.valid ? lastMonitor_.centreHz : csm::kNaN;
//     mon.bandwidthHz = lastMonitor_.valid ? lastMonitor_.spanHz : 0.0;
//     mon.rxGainDbi = antennaGain;
//     mon.noiseFigureDb = noiseFigure;
//     mon.temperatureC = model ? model->getConfig().temperature_c : csm::kNaN;
//     mon.sensitivityDbm = sensitivityDbm;   // NaN = unset
//     mon.minSnrDb = minSnrDb;

//     // Path loss from the existing propagation moel
//     const radio::RadioConfig rxCfg;
//     auto pathLoss = [&](const csm::EmitterInput& e) -> double {
//         if (!model) return csm::kNaN;
//         auto it = candidates.find(emitterKey(e.platformId, e.radioId));
//         if (it == candidates.end()) return csm::kNaN;
//         return model->pathLossDb(it->second.pos, observerPos, it->second.cfg, rxCfg);
//     };

//     csm::ScanResult current = csm::Detector::evaluate(emitters, parentEntity->ID, mon, pathLoss,
//                                                       nowS);
//     csm::spectrum::Input specIn;
//     specIn.monitorCentreHz = mon.centreHz;
//     specIn.monitorSpanHz = mon.bandwidthHz;
//     specIn.noiseFigureDb = mon.noiseFigureDb;
//     specIn.temperatureC = mon.temperatureC;
//     for (const csm::Observation& o : current.observations) {
//         csm::spectrum::EmitterSignal es;
//         es.label = (o.platformName.empty() ? o.platformId : o.platformName) + "/" +
//                    (o.radioName.empty() ? o.radioId : o.radioName);
//         if (o.frequencyHz.valid()) es.frequencyHz = o.frequencyHz.value;
//         if (o.bandwidthHz.valid()) es.bandwidthHz = o.bandwidthHz.value;
//         if (o.levelDbm.valid())    es.powerDbm = o.levelDbm.value;
//         if (o.modulationSource == csm::Provenance::Configured) es.modulation = o.modulation;
//         specIn.emitters.push_back(std::move(es));
//     }
//     spectrumCache_.update(specIn);

//     // --- Result for the table / legacy targets ------------------------------
//     if (scanMode && lastMonitor_.valid) {
//         // Hold each emitter's last detection for one sweep + one dwell, then drop it.
//         if (nowS < lastScanTimeS_) held_.clear();            // simulation restarted
//         for (const csm::Observation& o : current.observations)
//             held_[emitterKey(o.platformId, o.radioId)] = o;
//         const double holdS = lastMonitor_.sweepPeriodS + static_cast<double>(scanDwellS);
//         for (auto it = held_.begin(); it != held_.end();) {
//             if (nowS - it->second.observedAtS > holdS) it = held_.erase(it);
//             else ++it;
//         }
//         lastScan_ = csm::ScanResult();
//         lastScan_.stats = current.stats;
//         for (const auto& kv : held_) {
//             const csm::Observation& o = kv.second;
//             lastScan_.observations.push_back(o);
//             if (lastScan_.platforms.empty() || lastScan_.platforms.back().platformId != o.platformId) {
//                 csm::PlatformDetection p;
//                 p.platformId = o.platformId;
//                 p.distanceM = o.distanceM.value;
//                 p.bearingDeg = o.bearingDeg.value;
//                 lastScan_.platforms.push_back(p);
//             }
//             ++lastScan_.platforms.back().radioCount;
//         }
//         lastScan_.stats.detectedRadios = static_cast<int>(lastScan_.observations.size());
//         lastScan_.stats.detectedPlatforms = static_cast<int>(lastScan_.platforms.size());
//     } else {
//         held_.clear();
//         lastScan_ = std::move(current);
//     }
//     lastScanTimeS_ = nowS;

//     // --- Platform-level aggregate for legacy ewtargets/ewdetects ------------
//     dropPlatformTargets();

//     for (const csm::PlatformDetection& pd : lastScan_.platforms) {
//         auto pit = root->Platforms.find(pd.platformId);
//         if (pit == root->Platforms.end() || !pit->second) continue;

//         Target target{};
//         target.entity = pit->second;
//         target.angle = static_cast<float>(pd.bearingDeg);
//         target.radius = static_cast<float>(pd.distanceM / 1000.0);   // km, as before
//         ewdetects.insert(pit->second);
//         ewtargets.append(target);
//     }
// }

// QJsonObject CSM::toJson() const {
//     QJsonObject obj;
//     obj["active"] = Active;
//     obj["name"] = QString::fromStdString(Name);
//     obj["SensorType"] = "CSM";
//     obj["id"] = QString::fromStdString(ID);
//     QJsonObject defaultObj;
//     defaultObj["type"] = "Section";
//     defaultObj["range"] = toParm(range,"km",0,    500);
//     defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1,  100,
//                                      "Manual monitoring centre frequency (used only when monitorAuto = 0). Not a radio operating frequency");
//     defaultObj["azimuth"] = toParm(azimuth,"deg", 0,    360);
//     // Monitor receiver parameters (configured; 0 MHz bandwidth = tuned point).
//     defaultObj["bandwidth"] = toParm(bandwidth,"Mhz", 0, 50000,
//                                      "Manual monitoring span (used only when monitorAuto = 0). Not the signal or noise bandwidth; 0 = tuned point");
//     defaultObj["antennaGain"] = toParm(antennaGain,"dbi", -50, 100);
//     defaultObj["noiseFigure"] = toParm(noiseFigure,"db", 0, 50);
//     // Monitoring mode and detection thresholds (see CSM_README.md).
//     defaultObj["monitorAuto"] = toParm(autoMonitor ? 1.0f : 0.0f, "0/1", 0, 1,
//                                        "1 = automatic receiver range 100 MHz-6 GHz; 0 = use frequency/bandwidth above");
//     defaultObj["scanMode"] = toParm(scanMode ? 1.0f : 0.0f, "0/1", 0, 1,
//                                     "0 = MONITOR (whole range at once); 1 = SCAN (stepped window)");
//     defaultObj["scanStep"] = toParm(scanStepMhz, "Mhz", 1, 5000, "SCAN window width");
//     defaultObj["scanDwell"] = toParm(scanDwellS, "s", 0.05f, 60, "SCAN dwell time per step");
//     defaultObj["minSnr"] = toParm(minSnrDb, "db", -20, 60,
//                                   "Minimum SNR over the signal bandwidth for a detection");
//     defaultObj["sensitivityEnabled"] = toParm(std::isfinite(sensitivityDbm) ? 1.0f : 0.0f, "0/1", 0, 1,
//                                               "1 = apply the CSM sensitivity threshold below");
//     defaultObj["sensitivity"] = toParm(std::isfinite(sensitivityDbm) ? sensitivityDbm : -100.0f, "dBm", -200, 0,
//                                        "Minimum received level for a detection (CSM threshold, not the radio's)");
//     obj["default"] = defaultObj;

//     QJsonObject AddParameters = AdditionalParameters;
//     AddParameters["type"] = "Section";
//     obj["AdditionalParameters"] = AddParameters;

//     return obj;
// }

// void CSM::fromJson(const QJsonObject& obj) {
//     if (obj.contains("id")){
//         ID = obj["id"].toString().toStdString();
//     }
//     if (obj.contains("active"))
//         Active = obj["active"].toBool();
//     if (obj.contains("default") && obj["default"].isObject()) {
//         QJsonObject defaultObj = obj["default"].toObject();
//         if (defaultObj.contains("range"))
//             range = valueFromParm(defaultObj["range"].toObject());

//         if (defaultObj.contains("frequency"))
//             frequency = valueFromParm(defaultObj["frequency"].toObject());

//         if (defaultObj.contains("azimuth"))
//             azimuth = valueFromParm(defaultObj["azimuth"].toObject());

//         if (defaultObj.contains("bandwidth"))
//             bandwidth = valueFromParm(defaultObj["bandwidth"].toObject());

//         if (defaultObj.contains("antennaGain"))
//             antennaGain = valueFromParm(defaultObj["antennaGain"].toObject());

//         if (defaultObj.contains("noiseFigure"))
//             noiseFigure = valueFromParm(defaultObj["noiseFigure"].toObject());

//         // Monitoring mode. Files written before these keys existed: the old
//         // untouched default (10 GHz, 0 MHz span) is treated as unconfigured and
//         // becomes AUTO; any other explicit frequency/span stays MANUAL.
//         if (defaultObj.contains("monitorAuto"))
//             autoMonitor = valueFromParm(defaultObj["monitorAuto"].toObject()) >= 0.5f;
//         else
//             autoMonitor = csm::monitor::isLegacyUnconfigured(frequency, bandwidth);
//         if (defaultObj.contains("scanMode"))
//             scanMode = valueFromParm(defaultObj["scanMode"].toObject()) >= 0.5f;
//         if (defaultObj.contains("scanStep"))
//             scanStepMhz = valueFromParm(defaultObj["scanStep"].toObject());
//         if (defaultObj.contains("scanDwell"))
//             scanDwellS = valueFromParm(defaultObj["scanDwell"].toObject());
//         if (defaultObj.contains("minSnr"))
//             minSnrDb = valueFromParm(defaultObj["minSnr"].toObject());
//         if (defaultObj.contains("sensitivityEnabled")) {
//             const bool en = valueFromParm(defaultObj["sensitivityEnabled"].toObject()) >= 0.5f;
//             sensitivityDbm = (en && defaultObj.contains("sensitivity"))
//                                  ? valueFromParm(defaultObj["sensitivity"].toObject())
//                                  : std::numeric_limits<float>::quiet_NaN();
//         }
//     }

//     if(obj.contains("AdditionalParameters")){
//         AdditionalParameters = obj["AdditionalParameters"].toObject();
//     }
// }





#include "csm.h"
#include "CSM/csmdetector.h"
#include "CSM/csmfrequency.h"
#include "CSM/csmsignalmodel.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Simulation/simulation.h"
#include <cmath>
#include <string>
#include <unordered_map>

namespace {

const double kRad2Deg = 180.0 / M_PI;

// Name of a configured modulation scheme (configuration, not a classification).
std::string modulationName(radio::ModulationScheme m)
{
    using M = radio::ModulationScheme;
    switch (m) {
    case M::AM:         return "AM";
    case M::FM:         return "FM";
    case M::BPSK:       return "BPSK";
    case M::QPSK:       return "QPSK";
    case M::PSK8:       return "PSK8";
    case M::QAM16:      return "QAM16";
    case M::QAM64:      return "QAM64";
    case M::FSK2:       return "FSK2";
    case M::FSK4:       return "FSK4";
    case M::GMSK:       return "GMSK";
    case M::OFDM_BPSK:  return "OFDM-BPSK";
    case M::OFDM_QPSK:  return "OFDM-QPSK";
    case M::OFDM_QAM16: return "OFDM-QAM16";
    case M::OFDM_QAM64: return "OFDM-QAM64";
    }
    return std::string();
}

// Same transform-to-model position mapping as Radio::scan() (metres), so the
// propagation model sees identical geometry for radios and for the CSM.
radio::Position toModelPosition(Transform& t)
{
    radio::Position p;
    p.x = t.translation().z() * 1000.0;
    p.y = t.translation().x() * 1000.0;
    p.altitude = t.translation().y() * 1000.0;
    return p;
}

std::string emitterKey(const std::string& platformId, const std::string& radioId)
{
    return platformId + '\x1f' + radioId;
}

struct Candidate {
    radio::Position pos;
    radio::RadioConfig cfg;
};

} // namespace

CSM::CSM(Hierarchy* h) : Sensor(h) {
    subType = SubType::CSM;
    azimuth = 360;
    frequency = 0.15f;     // only used when autoMonitor is false
    bandwidth = 100.0f;
    noiseFigure = 6.0f;     // dB, typical receiver front end (0 dB is ideal/unrealistic)
}

// Drop platform-derived targets; entity-less (external) tracks are not ours.
void CSM::dropPlatformTargets()
{
    for (int i = ewtargets.size() - 1; i >= 0; --i) {
        if (ewtargets.at(i).entity) ewtargets.removeAt(i);
    }
    ewdetects.clear();
}

void CSM::clearScanState()
{
    lastScan_ = csm::ScanResult();
    spectrumCache_.clear();
    held_.clear();
    lastMonitor_ = csm::monitor::Window();
    dropPlatformTargets();
}

void CSM::setOperatingMode(csm::OperatingMode m)
{
    if (m == opMode_) return;
    opMode_ = m;
    // A switch must not leave receive results (or a spectrum) behind.
    clearScanState();
}

bool CSM::interferenceSource(csm::InterfererInput& out) const
{
    return csm::signal::makeInterferer(isTransmitting(), txFrequencyGhz, txBandwidthMhz,
                                       txPowerDbm, antennaGain, out);
}

void CSM::scan()
{
    if (!Active || !parentEntity || !root) {
        clearScanState();
        return;
    }
    // TRANSMIT: no receiver processing. The source is derived by receivers
    // straight from this object, so there is nothing to publish here and
    // nothing that can go stale.
    if (opMode_ == csm::OperatingMode::Transmit) {
        clearScanState();
        lastMonitor_.reason = "CSM is in TRANSMIT mode (receiver suspended)";
        return;
    }
    auto obsIt = root->Platforms.find(parentEntity->ID);
    if (obsIt == root->Platforms.end() || !obsIt->second) {
        clearScanState();
        return;
    }
    Platform* observer = obsIt->second;
    Transform& source = observer->transform;
    const radio::Position observerPos = toModelPosition(source);

    // --- Snapshot every radio as a pointer-free EmitterInput ---------------
    std::vector<csm::EmitterInput> emitters;
    std::unordered_map<std::string, Candidate> candidates;
    emitters.reserve(root->Radios.size());

    for (auto const& kv : root->Radios) {
        Radio* r = kv.second;
        if (!r || !r->parentEntity) continue;
        auto pit = root->Platforms.find(r->parentEntity->ID);
        if (pit == root->Platforms.end() || !pit->second) continue;
        Platform* platform = pit->second;

        csm::EmitterInput e;
        e.platformId = platform->ID;
        e.platformName = platform->Name;
        e.radioId = r->ID;
        e.radioName = r->Name;
        e.platformActive = platform->Active;
        e.radioActive = r->Active;
        const QVector3D localPos =
            source.inverseTransformPoint(platform->transform.matrix.translation());
        e.distanceM = distanceBetween(source.getLatitude(), source.getLongitude(),
                                      platform->transform.getLatitude(),
                                      platform->transform.getLongitude());
        e.bearingDeg = std::atan2(localPos.x(), localPos.z()) * kRad2Deg;
        e.inCoverage = detectCheck(localPos, static_cast<float>(e.distanceM / 1000.0));
        radio::RadioConfig cfg;
        if (r->lib_radio) {
            cfg = r->lib_radio->getConfiguration();
            e.frequencyHz = cfg.frequency_hz;
            e.bandwidthHz = cfg.bandwidth_hz;
            e.minFreqHz = cfg.min_freq_hz;
            e.maxFreqHz = cfg.max_freq_hz;
            e.canTransmit = (cfg.mode != radio::RadioMode::RECEIVER_ONLY) &&
                            r->lib_radio->isPoweredOn();
            e.modulation = modulationName(cfg.modulation_scheme);
            e.txPowerDbm = cfg.tx_power_dbm;
            e.txPowerDegradationDb = cfg.power_degradation_db;
            e.txDutyCycle = cfg.tx_duty_cycle;
            e.txAntennaGainDbi = cfg.antenna.gain_dbi;
        }

        candidates[emitterKey(e.platformId, e.radioId)] = { toModelPosition(platform->transform), cfg };
        emitters.push_back(std::move(e));
    }
    const double nowS = Simulation::simulationTime;
    csm::monitor::Settings ms;
    ms.autoRange = autoMonitor;
    ms.mode = scanMode ? csm::monitor::Mode::Scan : csm::monitor::Mode::Monitor;
    ms.centreHz = csm::freq::ghzToHz(frequency);
    ms.spanHz = (std::isfinite(bandwidth) && bandwidth > 0.0f)
                    ? static_cast<double>(bandwidth) * csm::freq::kHzPerMHz
                    : 0.0;
    ms.scanStepHz = csm::freq::mhzToHz(scanStepMhz);
    ms.scanDwellS = scanDwellS;
    lastMonitor_ = csm::monitor::resolve(ms, nowS);

    radio::PropagationModel* model = Radio::model;
    csm::MonitorConfig mon;
    mon.centreHz = lastMonitor_.valid ? lastMonitor_.centreHz : csm::kNaN;
    mon.bandwidthHz = lastMonitor_.valid ? lastMonitor_.spanHz : 0.0;
    mon.rxGainDbi = antennaGain;
    mon.noiseFigureDb = noiseFigure;
    mon.temperatureC = model ? model->getConfig().temperature_c : csm::kNaN;
    mon.sensitivityDbm = sensitivityDbm;   // NaN = unset
    mon.minSnrDb = minSnrDb;

    // Path loss from the existing propagation moel
    const radio::RadioConfig rxCfg;
    auto pathLoss = [&](const csm::EmitterInput& e) -> double {
        if (!model) return csm::kNaN;
        auto it = candidates.find(emitterKey(e.platformId, e.radioId));
        if (it == candidates.end()) return csm::kNaN;
        return model->pathLossDb(it->second.pos, observerPos, it->second.cfg, rxCfg);
    };

    // --- Interference from transmitting CSMs (derived live, nothing cached) --
    // Read straight from the hierarchy each scan, so a mode switch, txEnabled
    // off, deactivation, removal or reset takes effect on the next scan.
    std::vector<csm::InterfererInput> interferers;
    for (auto const& kv : root->Sensors) {
        const CSM* j = (kv.second && kv.second != this && kv.second->subType == SubType::CSM)
        ? dynamic_cast<const CSM*>(kv.second) : nullptr;
        if (!j || !j->parentEntity || j->parentEntity->ID == parentEntity->ID) continue;
        csm::InterfererInput in;
        if (!j->interferenceSource(in)) continue;
        auto jp = root->Platforms.find(j->parentEntity->ID);
        if (jp == root->Platforms.end() || !jp->second || !jp->second->Active) continue;
        Platform* jam = jp->second;
        const QVector3D localPos =
            source.inverseTransformPoint(jam->transform.matrix.translation());
        const double distM = distanceBetween(source.getLatitude(), source.getLongitude(),
                                             jam->transform.getLatitude(),
                                             jam->transform.getLongitude());
        in.inCoverage = detectCheck(localPos, static_cast<float>(distM / 1000.0));
        if (!in.inCoverage) continue;
        if (model) {
            radio::RadioConfig txCfg;       // only what the propagation model reads
            txCfg.frequency_hz = in.frequencyHz;
            txCfg.bandwidth_hz = in.bandwidthHz;
            txCfg.tx_power_dbm = in.txPowerDbm;
            txCfg.antenna.gain_dbi = in.txGainDbi;
            in.pathLossDb = model->pathLossDb(toModelPosition(jam->transform), observerPos,
                                              txCfg, rxCfg);
        }
        interferers.push_back(in);
    }

    csm::ScanResult current = csm::Detector::evaluate(emitters, parentEntity->ID, mon, pathLoss,
                                                      nowS, &interferers);
    csm::spectrum::Input specIn;
    specIn.monitorCentreHz = mon.centreHz;
    specIn.monitorSpanHz = mon.bandwidthHz;
    specIn.noiseFigureDb = mon.noiseFigureDb;
    specIn.temperatureC = mon.temperatureC;
    for (const csm::Observation& o : current.observations) {
        csm::spectrum::EmitterSignal es;
        es.label = (o.platformName.empty() ? o.platformId : o.platformName) + "/" +
                   (o.radioName.empty() ? o.radioId : o.radioName);
        if (o.frequencyHz.valid()) es.frequencyHz = o.frequencyHz.value;
        if (o.bandwidthHz.valid()) es.bandwidthHz = o.bandwidthHz.value;
        if (o.levelDbm.valid())    es.powerDbm = o.levelDbm.value;
        if (o.modulationSource == csm::Provenance::Configured) es.modulation = o.modulation;
        specIn.emitters.push_back(std::move(es));
    }
    for (const csm::InterfererInput& in : interferers) {
        const double p = csm::signal::receivedPowerDbm(in.txPowerDbm, 0.0, 1.0, in.txGainDbi,
                                                       mon.rxGainDbi, in.pathLossDb);
        if (!std::isfinite(p)) continue;
        csm::spectrum::InterfererSignal is;
        is.centreHz = in.frequencyHz;
        is.bandwidthHz = in.bandwidthHz;
        is.powerDbm = p;
        specIn.interferers.push_back(is);
    }
    spectrumCache_.update(specIn);

    // --- Result for the table / legacy targets ------------------------------
    if (scanMode && lastMonitor_.valid) {
        // Hold each emitter's last detection for one sweep + one dwell, then drop it.
        if (nowS < lastScanTimeS_) held_.clear();            // simulation restarted
        for (const csm::Observation& o : current.observations)
            held_[emitterKey(o.platformId, o.radioId)] = o;
        const double holdS = lastMonitor_.sweepPeriodS + static_cast<double>(scanDwellS);
        for (auto it = held_.begin(); it != held_.end();) {
            if (nowS - it->second.observedAtS > holdS) it = held_.erase(it);
            else ++it;
        }
        lastScan_ = csm::ScanResult();
        lastScan_.stats = current.stats;
        lastScan_.interference = current.interference;
        for (const auto& kv : held_) {
            const csm::Observation& o = kv.second;
            lastScan_.observations.push_back(o);
            if (lastScan_.platforms.empty() || lastScan_.platforms.back().platformId != o.platformId) {
                csm::PlatformDetection p;
                p.platformId = o.platformId;
                p.distanceM = o.distanceM.value;
                p.bearingDeg = o.bearingDeg.value;
                lastScan_.platforms.push_back(p);
            }
            ++lastScan_.platforms.back().radioCount;
        }
        lastScan_.stats.detectedRadios = static_cast<int>(lastScan_.observations.size());
        lastScan_.stats.detectedPlatforms = static_cast<int>(lastScan_.platforms.size());
    } else {
        held_.clear();
        lastScan_ = std::move(current);
    }
    lastScanTimeS_ = nowS;

    // --- Platform-level aggregate for legacy ewtargets/ewdetects ------------
    dropPlatformTargets();

    for (const csm::PlatformDetection& pd : lastScan_.platforms) {
        auto pit = root->Platforms.find(pd.platformId);
        if (pit == root->Platforms.end() || !pit->second) continue;

        Target target{};
        target.entity = pit->second;
        target.angle = static_cast<float>(pd.bearingDeg);
        target.radius = static_cast<float>(pd.distanceM / 1000.0);   // km, as before
        ewdetects.insert(pit->second);
        ewtargets.append(target);
    }
}

QJsonObject CSM::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["name"] = QString::fromStdString(Name);
    obj["SensorType"] = "CSM";
    obj["id"] = QString::fromStdString(ID);
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",0,    500);
    defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1,  100,
                                     "Manual monitoring centre frequency (used only when monitorAuto = 0). Not a radio operating frequency");
    defaultObj["azimuth"] = toParm(azimuth,"deg", 0,    360);
    // Monitor receiver parameters (configured; 0 MHz bandwidth = tuned point).
    defaultObj["bandwidth"] = toParm(bandwidth,"Mhz", 0, 50000,
                                     "Manual monitoring span (used only when monitorAuto = 0). Not the signal or noise bandwidth; 0 = tuned point");
    defaultObj["antennaGain"] = toParm(antennaGain,"dbi", -50, 100);
    defaultObj["noiseFigure"] = toParm(noiseFigure,"db", 0, 50);
    // Monitoring mode and detection thresholds (see CSM_README.md).
    defaultObj["monitorAuto"] = toParm(autoMonitor ? 1.0f : 0.0f, "0/1", 0, 1,
                                       "1 = automatic receiver range 100 MHz-6 GHz; 0 = use frequency/bandwidth above");
    defaultObj["scanMode"] = toParm(scanMode ? 1.0f : 0.0f, "0/1", 0, 1,
                                    "0 = MONITOR (whole range at once); 1 = SCAN (stepped window)");
    defaultObj["scanStep"] = toParm(scanStepMhz, "Mhz", 1, 5000, "SCAN window width");
    defaultObj["scanDwell"] = toParm(scanDwellS, "s", 0.05f, 60, "SCAN dwell time per step");
    defaultObj["minSnr"] = toParm(minSnrDb, "db", -20, 60,
                                  "Minimum SNR over the signal bandwidth for a detection");
    defaultObj["sensitivityEnabled"] = toParm(std::isfinite(sensitivityDbm) ? 1.0f : 0.0f, "0/1", 0, 1,
                                              "1 = apply the CSM sensitivity threshold below");
    defaultObj["sensitivity"] = toParm(std::isfinite(sensitivityDbm) ? sensitivityDbm : -100.0f, "dBm", -200, 0,
                                       "Minimum received level for a detection (CSM threshold, not the radio's)");
    // Operating mode and TRANSMIT (interference) source. Antenna gain above is shared.
    defaultObj["operatingMode"] = toParm(opMode_ == csm::OperatingMode::Transmit ? 1.0f : 0.0f,
                                         "0/1", 0, 1,
                                         "0 = RECEIVE (electronic-support receiver); 1 = TRANSMIT (interference source, receiver suspended)");
    defaultObj["txEnabled"] = toParm(txEnabled ? 1.0f : 0.0f, "0/1", 0, 1,
                                     "TRANSMIT mode only: 1 = emitting, 0 = silent");
    defaultObj["txFrequency"] = toParm(txFrequencyGhz, "Ghz", 0.001f, 100,
                                       "TRANSMIT mode: interference centre frequency");
    defaultObj["txBandwidth"] = toParm(txBandwidthMhz, "Mhz", 0.001f, 50000,
                                       "TRANSMIT mode: interference bandwidth (power spread uniformly)");
    defaultObj["txPower"] = toParm(txPowerDbm, "dBm", -50, 90,
                                   "TRANSMIT mode: transmit power (antenna gain = CSM antennaGain)");
    obj["default"] = defaultObj;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}

void CSM::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        if (defaultObj.contains("bandwidth"))
            bandwidth = valueFromParm(defaultObj["bandwidth"].toObject());

        if (defaultObj.contains("antennaGain"))
            antennaGain = valueFromParm(defaultObj["antennaGain"].toObject());

        if (defaultObj.contains("noiseFigure"))
            noiseFigure = valueFromParm(defaultObj["noiseFigure"].toObject());

        // Monitoring mode. Files written before these keys existed: the old
        // untouched default (10 GHz, 0 MHz span) is treated as unconfigured and
        // becomes AUTO; any other explicit frequency/span stays MANUAL.
        if (defaultObj.contains("monitorAuto"))
            autoMonitor = valueFromParm(defaultObj["monitorAuto"].toObject()) >= 0.5f;
        else
            autoMonitor = csm::monitor::isLegacyUnconfigured(frequency, bandwidth);
        if (defaultObj.contains("scanMode"))
            scanMode = valueFromParm(defaultObj["scanMode"].toObject()) >= 0.5f;
        if (defaultObj.contains("scanStep"))
            scanStepMhz = valueFromParm(defaultObj["scanStep"].toObject());
        if (defaultObj.contains("scanDwell"))
            scanDwellS = valueFromParm(defaultObj["scanDwell"].toObject());
        if (defaultObj.contains("minSnr"))
            minSnrDb = valueFromParm(defaultObj["minSnr"].toObject());
        if (defaultObj.contains("sensitivityEnabled")) {
            const bool en = valueFromParm(defaultObj["sensitivityEnabled"].toObject()) >= 0.5f;
            sensitivityDbm = (en && defaultObj.contains("sensitivity"))
                                 ? valueFromParm(defaultObj["sensitivity"].toObject())
                                 : std::numeric_limits<float>::quiet_NaN();
        }

        // Operating mode / transmit source. Keys absent (old scenarios, partial
        // runtime updates) leave the member untouched; a freshly constructed CSM
        // is RECEIVE, so old scenarios load as RECEIVE. A non-finite value reads
        // as RECEIVE (NaN >= 0.5 is false).
        if (defaultObj.contains("operatingMode"))
            setOperatingMode(valueFromParm(defaultObj["operatingMode"].toObject()) >= 0.5f
                                 ? csm::OperatingMode::Transmit : csm::OperatingMode::Receive);
        if (defaultObj.contains("txEnabled"))
            txEnabled = valueFromParm(defaultObj["txEnabled"].toObject()) >= 0.5f;
        auto readFinite = [&](const char* key, float& dst) {
            if (!defaultObj.contains(key)) return;
            const float v = valueFromParm(defaultObj[key].toObject());
            if (std::isfinite(v)) dst = v;
        };
        readFinite("txFrequency", txFrequencyGhz);
        readFinite("txBandwidth", txBandwidthMhz);
        readFinite("txPower", txPowerDbm);
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
}
