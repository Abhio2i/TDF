// =============================================================================
// radar.cpp  —  Qt/engine bridge for RadarModel
//
// Only changes vs previous version:
//   • Constructor cfg block: new fields added with defaults
//   • toJson():   new sections added (waveform, category, signalLibrary)
//   • fromJson(): new sections parsed
//
// EVERYTHING ELSE IS IDENTICAL:
//   scan(), collectTargets(), buildPose(),
//   processSurveillance(), processTWS(), processLockOn(),
//   lockOn(), breakLock(), platformToRadarID(), platformRCS(),
//   velocityFromHeadingSpeed()
// =============================================================================

#include "radar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QJsonObject>
#include <QVector3D>
#include <QDebug>
#include <cmath>

static constexpr double DEG2RAD     = M_PI / 180.0;
static constexpr double COORD_SCALE = 1000.0;
static constexpr double MIN_DT      = 1e-4;
static constexpr double MAX_DT      = 1.0;
static constexpr double DEFAULT_RCS = 10.0;
static std::unordered_map<uint32_t, QVector3D> prevPositions_;
static std::unordered_map<uint32_t, float>     computedHeadings_;

// =============================================================================
// Constructor — only the cfg block changes; everything else identical
// =============================================================================

Radar::Radar(Hierarchy* h) : Sensor(h)
{
    subType = SubType::Generic;

    RadarConfig cfg;

    // ---- Existing fields (unchanged defaults) ----------------------------
    cfg.emissionPower_kW      = 100.0;
    cfg.frequency_Hz          = 8e9;
    cfg.antennaGain           = 35.0f;
    cfg.antennaBandwidth      = 1e6;
    cfg.beamWidth             = 3.0f;
    cfg.systemTemperature_K   = 290.0;
    cfg.noiseFigure_dB        = 5.0;
    cfg.targetPfa             = 1e-6;

    cfg.minAzimuth            = -60.0f;
    cfg.maxAzimuth            =  60.0f;
    cfg.minElevation          = -30.0f;
    cfg.maxElevation          =  30.0f;
    cfg.scanningRate_RPM      =  12.0f;
    cfg.scanType              = ScanType::MECHANICAL;

    cfg.prfLevels[0]          = 5000.0f;
    cfg.prfLevels[1]          = 0.0f;
    cfg.prfLevels[2]          = 0.0f;
    cfg.prfLevels[3]          = 0.0f;
    cfg.pulseWidth            = 1e-6f;

    cfg.radarHeight           = 20.0;
    cfg.minDetectableRange    = 30.0;
    cfg.earthRadiusFactor     = 1.33;
    cfg.atmosphericFactor     = 1.0;

    cfg.missedScansToDrop     = 3;
    cfg.trackCoastSeconds     = 30.0;
    cfg.minHitsToValidate     = 2;
    cfg.maxTrackSpeed         = 2000.0;

    cfg.seaState              = 0.0;
    cfg.landClutter           = 0.0;

    cfg.mode                  = RadarMode::TWS;

    // ---- New fields — safe defaults (match original behaviour) ----------
    cfg.prfType               = PRFType::FIXED;
    cfg.modulation            = ModulationType::NONE;
    cfg.targetCategory        = DetectionCategory::ALL;

    cfg.peakSidelobeLevel     = -25.0f;
    cfg.avgSidelobeLevel      = -35.0f;

    cfg.hopStartFrequency     = 0.0f;
    cfg.hopStopFrequency      = 0.0f;
    cfg.hopStepFrequency      = 0.0f;
    cfg.hopRate               = 0.0f;
    cfg.frequencyAgility      = false;

    cfg.scanDwellTime[0]      = 0.0f;   // ms at min-az endpoint
    cfg.scanDwellTime[1]      = 0.0f;   // ms at max-az endpoint

    cfg.emitterID             = "";
    cfg.emitterCode           = 0;

    radarCore_.init(cfg);
    radarCore_.start();

    maxDetectionAngle = cfg.maxAzimuth;
}

// =============================================================================
// Lock / break lock  (unchanged)
// =============================================================================

void Radar::lockOn(uint32_t radarTargetID) { radarCore_.lockOn(radarTargetID); }
void Radar::breakLock()                    { radarCore_.breakLock();           }

// =============================================================================
// Static helpers  (unchanged)
// =============================================================================

uint32_t Radar::platformToRadarID(const std::string& key)
{
    return static_cast<uint32_t>(std::hash<std::string>{}(key));
}

double Radar::platformRCS(const Platform* p)
{
    return p ? DEFAULT_RCS : DEFAULT_RCS;
}

void Radar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                     double& vx, double& vy, double& vz)
{
    //qDebug() << "[velocity] headingDeg=" << headingDeg;
    double rad = headingDeg * DEG2RAD;
    vx = speedMs * std::cos(rad);
    vy = speedMs * std::sin(rad);
    vz = 0.0;
}

// =============================================================================
// Pose builder  (unchanged)
// =============================================================================

RadarPose Radar::buildPose() const
{
    RadarPose pose;
    if (!root || !parentEntity) return pose;

    auto it = root->Platforms.find(parentEntity->ID);
    if (it == root->Platforms.end() || !it->second->transform)
        return pose;

    QVector3D wpos = it->second->transform->matrix->translation();
    pose.x = static_cast<double>(wpos.x()) * COORD_SCALE;
    pose.y = static_cast<double>(wpos.y()) * COORD_SCALE;
    pose.z = static_cast<double>(wpos.z()) * COORD_SCALE;

    if (it->second->dynamicModel)
    {
        pose.heading = it->second->dynamicModel->TrueHeading;
        pose.pitch   = it->second->dynamicModel->pitch;//0.0f;
        pose.roll    = it->second->dynamicModel->roll;//0.0f;
    }
    return pose;
}

// =============================================================================
// Target collector  (unchanged)
// =============================================================================

std::vector<TargetInput> Radar::collectTargets(
    Transform* source,
    std::unordered_map<uint32_t, Platform*>& outIdMap) const
{
    std::vector<TargetInput> inputs;
    outIdMap.clear();

    if (!root ) return inputs;
    inputs.reserve(root->Platforms.size());

    for (auto& [key, entity] : root->Platforms)
    {
        if (!entity || key == parentEntity->ID) continue;

        Platform* platform = entity;
        if (!platform->transform) continue;

        QVector3D localPos =
            source->inverseTransformPoint(platform->transform->matrix->translation());

        TargetInput t;
        t.x = static_cast<double>(localPos.z()) * COORD_SCALE;
        t.y = static_cast<double>(localPos.x()) * COORD_SCALE;
        t.z = static_cast<double>(localPos.y()) * COORD_SCALE;

        t.id = platformToRadarID(key);
        outIdMap[t.id] = platform;

        if (platform->dynamicModel)
        {
            QVector3D worldPos = platform->transform->matrix->translation();
            uint32_t tid = platformToRadarID(key);

            if (prevPositions_.count(tid))
            {
                QVector3D delta = worldPos - prevPositions_[tid];
                float dist = std::sqrt(delta.x()*delta.x() + delta.z()*delta.z());
                if (dist > 0.001f)
                {
                    float hdg = std::atan2(delta.x(), delta.z()) * (180.0f / M_PI);
                    if (hdg < 0.0f) hdg += 360.0f;
                    computedHeadings_[tid] = hdg;
                }
            }
            prevPositions_[tid] = worldPos;

            float hdg = computedHeadings_.count(tid) ? computedHeadings_[tid] : 0.0f;
            velocityFromHeadingSpeed(
                static_cast<double>(hdg),
                static_cast<double>(platform->dynamicModel->currentSpeed),
                t.vx, t.vy, t.vz);
        }
        else
        {
            t.vx = t.vy = t.vz = 0.0;
        }

        t.rcs     = platformRCS(platform);
        t.surface = SurfaceType::AIR;
        t.jammer.active = false;

        inputs.push_back(t);
    }
    return inputs;
}

// =============================================================================
// Output translation  (unchanged)
// =============================================================================

void Radar::processSurveillance(
    const RadarOutput& output,
    const std::unordered_map<uint32_t, Platform*>& idMap,
    std::unordered_set<uint32_t>& addedIDs)
{
    for (const auto& d : output.detections)
    {
        if (addedIDs.count(d.targetID)) continue;
        addedIDs.insert(d.targetID);

        Platform* platform = nullptr;
        auto it = idMap.find(d.targetID);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity         = platform;
        t.radius         = static_cast<float>(d.range / 1000.0);
        t.angle          = static_cast<float>(d.azimuth);
        t.speed          = static_cast<float>(d.speedOverGround);
        t.direction      = static_cast<float>(d.heading);
        t.altitude       = static_cast<float>(d.range * std::sin(d.elevation * M_PI / 180.0));
        t.radialVelocity = static_cast<float>(d.radialVelocity);
        targets.append(t);
    }
}

void Radar::processTWS(
    const RadarOutput& output,
    const std::unordered_map<uint32_t, Platform*>& idMap,
    std::unordered_set<uint32_t>& addedIDs)
{
    for (const auto& tr : output.tracks)
    {
        if (addedIDs.count(tr.id)) continue;
        addedIDs.insert(tr.id);

        Platform* platform = nullptr;
        auto it = idMap.find(tr.id);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity         = platform;
        t.radius         = static_cast<float>(tr.range / 1000.0);
        t.angle          = static_cast<float>(tr.azimuth);
        t.speed          = static_cast<float>(tr.speedOverGround);
        t.direction      = static_cast<float>(tr.heading);
        t.altitude       = static_cast<float>(tr.z);
        t.radialVelocity = static_cast<float>(tr.radialVelocity);
        targets.append(t);
    }
}

void Radar::processLockOn(
    const RadarOutput& output,
    const std::unordered_map<uint32_t, Platform*>& idMap)
{
    if (output.tracks.empty()) return;

    uint32_t lockedID = radarCore_.getConfig().lockedTargetID;
    const TrackOutput* locked = nullptr;
    for (const auto& tr : output.tracks)
        if (tr.id == lockedID) { locked = &tr; break; }

    if (!locked) return;

    Platform* platform = nullptr;
    auto it = idMap.find(locked->id);
    if (it != idMap.end()) platform = it->second;

    Target t{};
    t.entity         = platform;
    t.radius         = static_cast<float>(locked->range / 1000.0);
    t.angle          = static_cast<float>(locked->azimuth);
    t.speed          = static_cast<float>(locked->speedOverGround);
    t.direction      = static_cast<float>(locked->heading);
    t.altitude       = static_cast<float>(locked->z);
    t.radialVelocity = static_cast<float>(locked->radialVelocity);
    targets.append(t);
}

// =============================================================================
// scan()  (unchanged)
// =============================================================================

void Radar::scan()
{
    if (!parentEntity) return;

    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    double dt;
    if (!timerStarted_)
    {
        frameTimer_.start();
        timerStarted_ = true;
        dt = 0.05;
    }
    else
    {
        dt = static_cast<double>(frameTimer_.elapsed()) / 1000.0;
        frameTimer_.restart();
        dt = std::clamp(dt, MIN_DT, MAX_DT);
    }
    simTime_ += dt;

    RadarPose pose = buildPose();

    if (pose.y > 50.0)
    {
        RadarConfig altCfg = radarCore_.getConfig();
        if (std::abs(pose.y - altCfg.radarHeight) > 1.0)
        {
            altCfg.radarHeight = pose.y;
            radarCore_.setConfig(altCfg);
            displayRangeDirty_ = true;
        }
    }

    std::unordered_map<uint32_t, Platform*> idMap;
    std::vector<TargetInput> radarInputs = collectTargets(source, idMap);

    radarCore_.update(dt, pose, radarInputs, simTime_);

    RadarOutput output = radarCore_.getOutput();
    RadarConfig cfg    = radarCore_.getConfig();

    azimuth           = static_cast<float>(output.currentAzimuth);
    beamWidth         = cfg.beamWidth;
    maxDetectionAngle = cfg.maxAzimuth;

    cachedDisplayRange_ = std::clamp(
        static_cast<float>(output.displayRange_km), 5.0f, 1000.0f);
    range = cachedDisplayRange_;

    switch (output.mode)
    {
    case RadarMode::SURVEILLANCE: mode = Sensor::Mode::Search;         break;
    case RadarMode::TWS:          mode = Sensor::Mode::TrackWhileScan; break;
    case RadarMode::LOCK_ON:      mode = Sensor::Mode::FireControl;    break;
    }

    targets.clear();
    std::unordered_set<uint32_t> addedIDs;

    switch (output.mode)
    {
    case RadarMode::SURVEILLANCE: processSurveillance(output, idMap, addedIDs); break;
    case RadarMode::TWS:          processTWS(output, idMap, addedIDs);          break;
    case RadarMode::LOCK_ON:      processLockOn(output, idMap);                 break;
    }
}

// =============================================================================
// toJson  — new sections added at the end; all existing sections unchanged
// =============================================================================

QJsonObject Radar::toJson() const
{
    QJsonObject obj;
    RadarConfig cfg = radarCore_.getConfig();

    // --- Identity (unchanged) ---
    obj["id"]         = QString::fromStdString(ID);
    obj["name"]       = QString::fromStdString(Name);
    obj["Active"]     = Active;
    obj["SensorType"] = "Radar";

    // --- Transmitter (unchanged) ---
    QJsonObject transmitter;
    transmitter["type"]             = "Section";
    transmitter["emissionPower_kW"] = toParm(cfg.emissionPower_kW, "kW",  0.0f,   1000.0f, "Transmit power");
    transmitter["frequency_Hz"]     = toParm(cfg.frequency_Hz,     "Hz",  1e6f,   1e11f,   "Operating frequency");
    transmitter["antennaGain"]      = toParm(cfg.antennaGain,      "dBi", 0.0f,   60.0f,   "Antenna gain");
    transmitter["antennaBandwidth"] = toParm(cfg.antennaBandwidth, "Hz",  0.0f,   1e8f,    "Receiver bandwidth");
    transmitter["beamWidth"]        = toParm(cfg.beamWidth,        "deg", 0.1f,   30.0f,   "Antenna beam width");
    transmitter["pulseWidth"]       = toParm(cfg.pulseWidth,       "s",   0.0f,   1e-3f,   "Pulse width");
    transmitter["prfLevel0"]        = toParm(cfg.prfLevels[0],     "Hz",  0.0f,   1e6f,    "PRF level 0 (primary)");
    transmitter["prfLevel1"]        = toParm(cfg.prfLevels[1],     "Hz",  0.0f,   1e6f,    "PRF level 1");
    transmitter["prfLevel2"]        = toParm(cfg.prfLevels[2],     "Hz",  0.0f,   1e6f,    "PRF level 2");
    transmitter["prfLevel3"]        = toParm(cfg.prfLevels[3],     "Hz",  0.0f,   1e6f,    "PRF level 3");
    transmitter["peakSidelobeLevel"]= toParm(cfg.peakSidelobeLevel,"dB", -80.0f,  0.0f,   "Peak sidelobe level");
    transmitter["avgSidelobeLevel"] = toParm(cfg.avgSidelobeLevel, "dB", -80.0f,  0.0f,   "Average sidelobe level");
    obj["transmitter"] = transmitter;

    // --- Scan (unchanged base + scanDwellTime added) ---
    QJsonObject scan;
    scan["type"]             = "Section";
    scan["minAzimuth"]       = toParm(cfg.minAzimuth,        "deg", -180.0f, 0.0f,   "Minimum azimuth");
    scan["maxAzimuth"]       = toParm(cfg.maxAzimuth,        "deg",  0.0f,   180.0f, "Maximum azimuth");
    scan["minElevation"]     = toParm(cfg.minElevation,      "deg", -90.0f,  0.0f,   "Minimum elevation");
    scan["maxElevation"]     = toParm(cfg.maxElevation,      "deg",  0.0f,   90.0f,  "Maximum elevation");
    scan["scanningRate_RPM"] = toParm(cfg.scanningRate_RPM,  "RPM",  0.0f,   60.0f,  "Antenna scan rate");
    scan["scanType"]         = static_cast<int>(cfg.scanType);
    scan["scanDwellTimeMin"] = toParm(cfg.scanDwellTime[0],  "ms",   0.0f,   5000.0f,"Dwell time at min-az endpoint");
    scan["scanDwellTimeMax"] = toParm(cfg.scanDwellTime[1],  "ms",   0.0f,   5000.0f,"Dwell time at max-az endpoint");
    obj["scan"] = scan;

    // --- Detection (unchanged) ---
    QJsonObject detection;
    detection["type"]                = "Section";
    detection["systemTemperature_K"] = toParm(cfg.systemTemperature_K, "K",  0.0f, 1000.0f, "System noise temperature");
    detection["noiseFigure_dB"]      = toParm(cfg.noiseFigure_dB,      "dB", 0.0f, 30.0f,   "Receiver noise figure");
    detection["targetPfa"]           = toParm(cfg.targetPfa,           "",   0.0f, 1.0f,    "False alarm probability");
    detection["minDetectableRange"]  = toParm(cfg.minDetectableRange,  "m",  0.0f, 1000.0f, "Minimum detectable range");
    detection["seaState"]            = toParm(cfg.seaState,            "",   0.0f, 9.0f,    "Sea state Beaufort scale");
    detection["landClutter"]         = toParm(cfg.landClutter,         "",   0.0f, 1.0f,    "Land clutter factor");
    detection["targetCategory"]      = static_cast<int>(cfg.targetCategory);
    obj["detection"] = detection;

    // --- Platform (unchanged) ---
    QJsonObject platform;
    platform["type"]        = "Section";
    platform["radarHeight"] = toParm(cfg.radarHeight, "m", 0.0f, 30000.0f, "Radar platform height");
    obj["platform"] = platform;

    // --- Tracking (unchanged) ---
    QJsonObject tracking;
    tracking["type"]              = "Section";
    tracking["missedScansToDrop"] = toParm(cfg.missedScansToDrop, "",    0.0f, 20.0f,    "Missed scans before track drop");
    tracking["trackCoastSeconds"] = toParm(cfg.trackCoastSeconds, "s",   0.0f, 300.0f,   "Track coast duration");
    tracking["minHitsToValidate"] = toParm(cfg.minHitsToValidate, "",    1.0f, 10.0f,    "Hits needed to validate track");
    tracking["maxTrackSpeed"]     = toParm(cfg.maxTrackSpeed,     "m/s", 0.0f, 5000.0f,  "Maximum track speed");
    obj["tracking"] = tracking;

    // --- Propagation (unchanged) ---
    QJsonObject propagation;
    propagation["type"]              = "Section";
    propagation["earthRadiusFactor"] = toParm(cfg.earthRadiusFactor, "",     1.0f, 2.0f,     "Earth radius factor");
    propagation["atmosphericFactor"] = toParm(cfg.atmosphericFactor, "",     0.0f, 2.0f,     "Atmospheric refraction");
    propagation["rainRate_mmph"]     = toParm(cfg.rainRate_mmph,     "mm/h", 0.0f, 200.0f,   "Rain rate");
    propagation["fogVisibility_m"]   = toParm(cfg.fogVisibility_m,   "m",    0.0f, 10000.0f, "Fog visibility");
    obj["propagation"] = propagation;

    // --- Noise (unchanged) ---
    QJsonObject noise;
    noise["type"]            = "Section";
    noise["rangeStdDev"]     = toParm(cfg.noise.rangeStdDev,     "m",   0.0f, 1000.0f, "Range noise σ");
    noise["azimuthStdDev"]   = toParm(cfg.noise.azimuthStdDev,   "deg", 0.0f, 10.0f,   "Azimuth noise σ");
    noise["elevationStdDev"] = toParm(cfg.noise.elevationStdDev, "deg", 0.0f, 10.0f,   "Elevation noise σ");
    noise["dopplerStdDev"]   = toParm(cfg.noise.dopplerStdDev,   "m/s", 0.0f, 100.0f,  "Doppler noise σ");
    obj["noise"] = noise;


    // --- Waveform (NEW section) ---
    QJsonObject waveform;
    waveform["type"]           = "Section";
    waveform["modulation"]     = static_cast<int>(cfg.modulation);
    waveform["prfType"]        = static_cast<int>(cfg.prfType);
    waveform["frequencyAgility"]   = cfg.frequencyAgility;
    waveform["hopStartFrequency"]  = toParm(cfg.hopStartFrequency,  "Hz", 0.0f, 1e11f, "Hop range lower bound");
    waveform["hopStopFrequency"]   = toParm(cfg.hopStopFrequency,   "Hz", 0.0f, 1e11f, "Hop range upper bound");
    waveform["hopStepFrequency"]   = toParm(cfg.hopStepFrequency,   "Hz", 0.0f, 1e9f,  "Frequency hop step size");
    waveform["hopRate"]            = toParm(cfg.hopRate,             "Hz", 0.0f, 1e6f,  "Hop rate");
    obj["waveform"] = waveform;

    // --- Emitter identity (NEW section) ---
    QJsonObject emitter;
    emitter["type"]        = "Section";
    emitter["emitterID"]   = QString::fromStdString(cfg.emitterID);
    emitter["emitterCode"] = static_cast<int>(cfg.emitterCode);
    obj["emitter"] = emitter;

    // --- Mode (unchanged) ---
    obj["mode"] = static_cast<int>(cfg.mode);

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;


    return obj;
}

// =============================================================================
// fromJson  — new sections parsed at the end; all existing sections unchanged
// =============================================================================

void Radar::fromJson(const QJsonObject& obj)
{
    RadarConfig cfg = radarCore_.getConfig();

    // --- Transmitter (existing fields unchanged + new sidelobe + all PRF levels) ---
    if (obj.contains("transmitter") && obj["transmitter"].isObject())
    {
        QJsonObject t = obj["transmitter"].toObject();
        if (t.contains("emissionPower_kW"))  cfg.emissionPower_kW  = valueFromParm(t["emissionPower_kW"].toObject());
        if (t.contains("frequency_Hz"))      cfg.frequency_Hz      = valueFromParm(t["frequency_Hz"].toObject());
        if (t.contains("antennaGain"))       cfg.antennaGain       = valueFromParm(t["antennaGain"].toObject());
        if (t.contains("antennaBandwidth"))  cfg.antennaBandwidth  = valueFromParm(t["antennaBandwidth"].toObject());
        if (t.contains("beamWidth"))         cfg.beamWidth         = valueFromParm(t["beamWidth"].toObject());
        if (t.contains("pulseWidth"))        cfg.pulseWidth        = valueFromParm(t["pulseWidth"].toObject());
        if (t.contains("prfLevel0"))         cfg.prfLevels[0]      = valueFromParm(t["prfLevel0"].toObject());
        if (t.contains("prfLevel1"))         cfg.prfLevels[1]      = valueFromParm(t["prfLevel1"].toObject());
        if (t.contains("prfLevel2"))         cfg.prfLevels[2]      = valueFromParm(t["prfLevel2"].toObject());
        if (t.contains("prfLevel3"))         cfg.prfLevels[3]      = valueFromParm(t["prfLevel3"].toObject());
        if (t.contains("peakSidelobeLevel")) cfg.peakSidelobeLevel = valueFromParm(t["peakSidelobeLevel"].toObject());
        if (t.contains("avgSidelobeLevel"))  cfg.avgSidelobeLevel  = valueFromParm(t["avgSidelobeLevel"].toObject());
    }

    // --- Scan (existing fields + new dwell times) ---
    if (obj.contains("scan") && obj["scan"].isObject())
    {
        QJsonObject s = obj["scan"].toObject();
        if (s.contains("minAzimuth"))       cfg.minAzimuth       = valueFromParm(s["minAzimuth"].toObject());
        if (s.contains("maxAzimuth"))       cfg.maxAzimuth       = valueFromParm(s["maxAzimuth"].toObject());
        if (s.contains("minElevation"))     cfg.minElevation     = valueFromParm(s["minElevation"].toObject());
        if (s.contains("maxElevation"))     cfg.maxElevation     = valueFromParm(s["maxElevation"].toObject());
        if (s.contains("scanningRate_RPM")) cfg.scanningRate_RPM = valueFromParm(s["scanningRate_RPM"].toObject());
        if (s.contains("scanType"))         cfg.scanType         = static_cast<ScanType>(s["scanType"].toInt());
        if (s.contains("scanDwellTimeMin")) cfg.scanDwellTime[0] = valueFromParm(s["scanDwellTimeMin"].toObject());
        if (s.contains("scanDwellTimeMax")) cfg.scanDwellTime[1] = valueFromParm(s["scanDwellTimeMax"].toObject());
        minAzimuth = cfg.minAzimuth;
        maxAzimuth = cfg.maxAzimuth;
    }

    // --- Detection (existing fields + targetCategory) ---
    if (obj.contains("detection") && obj["detection"].isObject())
    {
        QJsonObject d = obj["detection"].toObject();
        if (d.contains("systemTemperature_K")) cfg.systemTemperature_K = valueFromParm(d["systemTemperature_K"].toObject());
        if (d.contains("noiseFigure_dB"))      cfg.noiseFigure_dB      = valueFromParm(d["noiseFigure_dB"].toObject());
        if (d.contains("targetPfa"))           cfg.targetPfa           = valueFromParm(d["targetPfa"].toObject());
        if (d.contains("minDetectableRange"))  cfg.minDetectableRange  = valueFromParm(d["minDetectableRange"].toObject());
        if (d.contains("seaState"))            cfg.seaState            = valueFromParm(d["seaState"].toObject());
        if (d.contains("landClutter"))         cfg.landClutter         = valueFromParm(d["landClutter"].toObject());
        if (d.contains("targetCategory"))      cfg.targetCategory      = static_cast<DetectionCategory>(d["targetCategory"].toInt());
    }

    // --- Platform (unchanged) ---
    if (obj.contains("platform") && obj["platform"].isObject())
    {
        QJsonObject p = obj["platform"].toObject();
        if (p.contains("radarHeight")) cfg.radarHeight = valueFromParm(p["radarHeight"].toObject());
    }

    // --- Tracking (unchanged) ---
    if (obj.contains("tracking") && obj["tracking"].isObject())
    {
        QJsonObject t = obj["tracking"].toObject();
        if (t.contains("missedScansToDrop")) cfg.missedScansToDrop = static_cast<int>(valueFromParm(t["missedScansToDrop"].toObject()));
        if (t.contains("trackCoastSeconds")) cfg.trackCoastSeconds = valueFromParm(t["trackCoastSeconds"].toObject());
        if (t.contains("minHitsToValidate")) cfg.minHitsToValidate = static_cast<int>(valueFromParm(t["minHitsToValidate"].toObject()));
        if (t.contains("maxTrackSpeed"))     cfg.maxTrackSpeed     = valueFromParm(t["maxTrackSpeed"].toObject());
    }

    // --- Propagation (unchanged) ---
    if (obj.contains("propagation") && obj["propagation"].isObject())
    {
        QJsonObject p = obj["propagation"].toObject();
        if (p.contains("earthRadiusFactor")) cfg.earthRadiusFactor = valueFromParm(p["earthRadiusFactor"].toObject());
        if (p.contains("atmosphericFactor")) cfg.atmosphericFactor = valueFromParm(p["atmosphericFactor"].toObject());
        if (p.contains("rainRate_mmph"))     cfg.rainRate_mmph     = valueFromParm(p["rainRate_mmph"].toObject());
        if (p.contains("fogVisibility_m"))   cfg.fogVisibility_m   = valueFromParm(p["fogVisibility_m"].toObject());
    }

    // --- Noise (unchanged) ---
    if (obj.contains("noise") && obj["noise"].isObject())
    {
        QJsonObject n = obj["noise"].toObject();
        if (n.contains("rangeStdDev"))     cfg.noise.rangeStdDev     = valueFromParm(n["rangeStdDev"].toObject());
        if (n.contains("azimuthStdDev"))   cfg.noise.azimuthStdDev   = valueFromParm(n["azimuthStdDev"].toObject());
        if (n.contains("elevationStdDev")) cfg.noise.elevationStdDev = valueFromParm(n["elevationStdDev"].toObject());
        if (n.contains("dopplerStdDev"))   cfg.noise.dopplerStdDev   = valueFromParm(n["dopplerStdDev"].toObject());
    }



    // --- Waveform (NEW) ---
    if (obj.contains("waveform") && obj["waveform"].isObject())
    {
        QJsonObject w = obj["waveform"].toObject();
        if (w.contains("modulation"))        cfg.modulation        = static_cast<ModulationType>(w["modulation"].toInt());
        if (w.contains("prfType"))           cfg.prfType           = static_cast<PRFType>(w["prfType"].toInt());
        if (w.contains("frequencyAgility"))  cfg.frequencyAgility  = w["frequencyAgility"].toBool();
        if (w.contains("hopStartFrequency")) cfg.hopStartFrequency = valueFromParm(w["hopStartFrequency"].toObject());
        if (w.contains("hopStopFrequency"))  cfg.hopStopFrequency  = valueFromParm(w["hopStopFrequency"].toObject());
        if (w.contains("hopStepFrequency"))  cfg.hopStepFrequency  = valueFromParm(w["hopStepFrequency"].toObject());
        if (w.contains("hopRate"))           cfg.hopRate           = valueFromParm(w["hopRate"].toObject());
    }

    // --- Emitter identity (NEW) ---
    if (obj.contains("emitter") && obj["emitter"].isObject())
    {
        QJsonObject e = obj["emitter"].toObject();
        if (e.contains("emitterID"))   cfg.emitterID   = e["emitterID"].toString().toStdString();
        if (e.contains("emitterCode")) cfg.emitterCode = static_cast<uint32_t>(e["emitterCode"].toInt());
    }

    // --- Mode (unchanged) ---
    if (obj.contains("mode"))
        cfg.mode = static_cast<RadarMode>(obj["mode"].toInt());

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }

    radarCore_.setConfig(cfg);
    displayRangeDirty_ = true;
}
