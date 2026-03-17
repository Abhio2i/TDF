// =============================================================================
// radar.cpp  —  Qt/engine bridge for RadarModel
//
// This file is the ONLY place that knows about both Qt/engine types AND
// RadarModel.  It translates between them.
//
// Design rules (enforced here):
//   1. No radar physics in this file.  Physics lives in radarmodel.cpp only.
//   2. No Qt types cross the radarmodel.h boundary.
//   3. RadarConfig is built once here and handed to the model via init().
//      Hot-reload goes through setConfig() / individual mode setters.
//   4. Each scan() call provides the model with a fresh RadarPose and a
//      fresh TargetInput list — the model decides what to do with them.
//   5. Output is read as a single RadarOutput snapshot via getOutput().
// =============================================================================

#include "radar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QJsonObject>
#include <QVector3D>
#include <QDebug>
#include <cmath>

// ---------------------------------------------------------------------------
// Constants — bridge layer only
// ---------------------------------------------------------------------------
static constexpr double DEG2RAD     = M_PI / 180.0;
static constexpr double COORD_SCALE = 1000.0;   // engine units → metres
static constexpr double MIN_DT      = 1e-4;     // seconds
static constexpr double MAX_DT      = 1.0;      // seconds
static constexpr double DEFAULT_RCS = 10.0;     // m² fallback

// =============================================================================
// Constructor — build default config, init + start the model
// =============================================================================

Radar::Radar(Hierarchy* h) : Sensor(h)
{
    subType = SubType::Generic;

    // ------------------------------------------------------------------
    // Build initial RadarConfig — single source of truth for defaults
    // ------------------------------------------------------------------
    RadarConfig cfg;
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

    cfg.jammer.active         = true;
    cfg.jammer.power_kW       = 0.0;
    cfg.jammer.gain_dBi       = 20.0;
    cfg.jammer.selfScreening  = true;

    // ------------------------------------------------------------------
    // Initialise the model
    // ------------------------------------------------------------------
    radarCore_.init(cfg);
    radarCore_.start();

    // Sync sensor base-class fields
    maxDetectionAngle = cfg.maxAzimuth;
}

// =============================================================================
// Lock / break lock
// =============================================================================

void Radar::lockOn(uint32_t radarTargetID)
{
    radarCore_.lockOn(radarTargetID);
}

void Radar::breakLock()
{
    radarCore_.breakLock();
}

// =============================================================================
// Static helpers (bridge-private, no physics)
// =============================================================================

uint32_t Radar::platformToRadarID(const std::string& key)
{
    return static_cast<uint32_t>(std::hash<std::string>{}(key));
}

double Radar::platformRCS(const Platform* p)
{
    return p ? DEFAULT_RCS : DEFAULT_RCS;
    // TODO: read p->profile->rcs when profile is ready
}

/// Convert platform heading + speed → velocity vector in radar-local frame
/// Convention: X = East, Y = Up, Z = North
void Radar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                     double& vx, double& vy, double& vz)
{
    double rad = headingDeg * DEG2RAD;
    vx = speedMs * std::sin(rad);   // East
    vy = 0.0;                       // Up (horizontal motion)
    vz = speedMs * std::cos(rad);   // North
}

// =============================================================================
// Pose builder — reads own platform transform
// =============================================================================

RadarPose Radar::buildPose() const
{
    RadarPose pose;

    if (!root || !parentEntity) return pose;

    auto it = root->Platforms->find(parentEntity->ID);
    if (it == root->Platforms->end() || !it->second->transform)
        return pose;

    QVector3D wpos = it->second->transform->matrix->translation();

    // Engine Y = altitude; scale to metres
    pose.x = static_cast<double>(wpos.z()) * COORD_SCALE;   // forward → North
    pose.y = static_cast<double>(wpos.y()) * COORD_SCALE;   // up → altitude
    pose.z = static_cast<double>(wpos.x()) * COORD_SCALE;   // right → East

    if (it->second->dynamicModel)
    {
        pose.heading = it->second->dynamicModel->TrueHeading;
        pose.pitch   = 0.0f;
        pose.roll    = 0.0f;
    }

    return pose;
}

// =============================================================================
// Target collector — transforms engine platforms into TargetInput list
// =============================================================================

std::vector<TargetInput> Radar::collectTargets(
    Transform* source,
    std::unordered_map<uint32_t, Platform*>& outIdMap) const
{
    std::vector<TargetInput> inputs;
    outIdMap.clear();

    if (!root || !root->Platforms) return inputs;
    inputs.reserve(root->Platforms->size());

    for (auto& [key, entity] : *root->Platforms)
    {
        if (!entity || key == parentEntity->ID) continue;

        Platform* platform = entity;
        if (!platform->transform) continue;

        // Local position relative to our platform
        QVector3D localPos =
            source->inverseTransformPoint(platform->transform->matrix->translation());

        TargetInput t;
        // Engine local axes → radar model axes (X=East, Y=Up, Z=North)
        t.x = static_cast<double>(localPos.z()) * COORD_SCALE;
        t.y = static_cast<double>(localPos.x()) * COORD_SCALE;
        t.z = static_cast<double>(localPos.y()) * COORD_SCALE;

        t.id = platformToRadarID(key);
        outIdMap[t.id] = platform;

        if (platform->dynamicModel)
        {
            velocityFromHeadingSpeed(
                static_cast<double>(platform->dynamicModel->TrueHeading),
                static_cast<double>(platform->dynamicModel->currentSpeed),
                t.vx, t.vy, t.vz);
        }
        else
        {
            t.vx = t.vy = t.vz = 0.0;
        }

        t.rcs     = platformRCS(platform);
        t.surface = SurfaceType::SEA;

        inputs.push_back(t);
    }

    return inputs;
}

// =============================================================================
// Output → Sensor targets  (translate RadarOutput into engine Target structs)
// =============================================================================

/// Populate sensor->targets from surveillance detections
void Radar::processSurveillance(
    const RadarOutput& output,
    const std::unordered_map<uint32_t, Platform*>& idMap,
    std::unordered_set<uint32_t>& addedIDs)
{
    qDebug().noquote()
    << QString("[ %1 ]  SURV — %2 detection(s)")
            .arg(QString::fromStdString(parentEntity->Name))
            .arg(output.detections.size());

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

/// Populate sensor->targets from TWS validated tracks
void Radar::processTWS(
    const RadarOutput& output,
    const std::unordered_map<uint32_t, Platform*>& idMap,
    std::unordered_set<uint32_t>& addedIDs)
{
    qDebug().noquote()
    << QString("[ %1 ]  TWS — %2 track(s)")
            .arg(QString::fromStdString(parentEntity->Name))
            .arg(output.tracks.size());

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

/// Populate sensor->targets from the single locked track
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
// scan()  —  called once per simulation tick by the engine
// =============================================================================

void Radar::scan()
{
    if (!parentEntity) return;

    Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    // ------------------------------------------------------------------
    // dt measurement
    // ------------------------------------------------------------------
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

    // ------------------------------------------------------------------
    // Build pose + world inputs
    // ------------------------------------------------------------------
    // RadarPose pose = buildPose();

    // std::unordered_map<uint32_t, Platform*> idMap;
    RadarPose pose = buildPose();

    // Sync altitude — mark range dirty if height changed
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

    // ------------------------------------------------------------------
    // Tick the model
    // ------------------------------------------------------------------
    radarCore_.update(dt, pose, radarInputs, simTime_);

    // ------------------------------------------------------------------
    // Read output snapshot + config in one consistent pair
    // (two separate getOutput/getConfig calls would each take the mutex
    //  independently — a setConfig() in between could make them inconsistent)
    // ------------------------------------------------------------------
    RadarOutput output = radarCore_.getOutput();
    RadarConfig cfg    = radarCore_.getConfig();

    // ------------------------------------------------------------------
    // Sync sensor base-class fields from output
    // ------------------------------------------------------------------
    azimuth           = static_cast<float>(output.currentAzimuth);
    beamWidth         = cfg.beamWidth;
    maxDetectionAngle = cfg.maxAzimuth;

    cachedDisplayRange_ = std::clamp(
        static_cast<float>(output.displayRange_km), 5.0f, 1000.0f);
    range = cachedDisplayRange_;
    // if (displayRangeDirty_)
    // {
    //     cachedDisplayRange_ = static_cast<float>(output.displayRange_km);
    //     cachedDisplayRange_ = std::clamp(cachedDisplayRange_, 5.0f, 1000.0f);
    //     displayRangeDirty_  = false;
    // }
    // range = cachedDisplayRange_;

    // Sync Sensor::Mode from model mode
    switch (output.mode)
    {
    case RadarMode::SURVEILLANCE: mode = Sensor::Mode::Search;         break;
    case RadarMode::TWS:          mode = Sensor::Mode::TrackWhileScan; break;
    case RadarMode::LOCK_ON:      mode = Sensor::Mode::FireControl;    break;
    }

    // ------------------------------------------------------------------
    // Translate model output → engine Target list
    // ------------------------------------------------------------------
    targets.clear();
    std::unordered_set<uint32_t> addedIDs;

    switch (output.mode)
    {
    case RadarMode::SURVEILLANCE:
        processSurveillance(output, idMap, addedIDs);
        break;
    case RadarMode::TWS:
        processTWS(output, idMap, addedIDs);
        break;
    case RadarMode::LOCK_ON:
        processLockOn(output, idMap);
        break;
    }
}

// =============================================================================
// Serialisation
// =============================================================================

QJsonObject Radar::toJson() const
{
    QJsonObject obj;
    RadarConfig cfg = radarCore_.getConfig();

    obj["id"]      = QString::fromStdString(ID);
    obj["name"]    = QString::fromStdString(Name);
    obj["Active"]  = Active;
    obj["SensorType"] = "Radar";

    obj["emissionPower_kW"]    = cfg.emissionPower_kW;
    obj["frequency_Hz"]        = cfg.frequency_Hz;
    obj["antennaGain"]         = cfg.antennaGain;
    obj["antennaBandwidth"]    = cfg.antennaBandwidth;
    obj["beamWidth"]           = cfg.beamWidth;
    obj["scanningRate_RPM"]    = cfg.scanningRate_RPM;
    obj["minAzimuth"]          = cfg.minAzimuth;
    obj["maxAzimuth"]          = cfg.maxAzimuth;
    obj["minElevation"]        = cfg.minElevation;
    obj["maxElevation"]        = cfg.maxElevation;
    obj["radarHeight"]         = cfg.radarHeight;
    obj["minDetectableRange"]  = cfg.minDetectableRange;
    obj["seaState"]            = cfg.seaState;
    obj["landClutter"]         = cfg.landClutter;
    obj["maxTrackSpeed"]       = cfg.maxTrackSpeed;
    obj["systemTemperature_K"] = cfg.systemTemperature_K;
    obj["noiseFigure_dB"]      = cfg.noiseFigure_dB;
    obj["targetPfa"]           = cfg.targetPfa;
    obj["missedScansToDrop"]   = cfg.missedScansToDrop;
    obj["trackCoastSeconds"]   = cfg.trackCoastSeconds;
    obj["minHitsToValidate"]   = cfg.minHitsToValidate;
    obj["prfLevel0"]           = cfg.prfLevels[0];
    obj["pulseWidth"]          = cfg.pulseWidth;
    obj["earthRadiusFactor"]   = cfg.earthRadiusFactor;
    obj["atmosphericFactor"]   = cfg.atmosphericFactor;
    obj["rainRate_mmph"]       = cfg.rainRate_mmph;
    obj["fogVisibility_m"]     = cfg.fogVisibility_m;
    obj["scanType"]            = static_cast<int>(cfg.scanType);
    obj["mode"]                = static_cast<int>(cfg.mode);

    obj["noise_rangeStdDev"]   = cfg.noise.rangeStdDev;
    obj["noise_azimuthStdDev"] = cfg.noise.azimuthStdDev;
    obj["noise_elevStdDev"]    = cfg.noise.elevationStdDev;
    obj["noise_dopplerStdDev"] = cfg.noise.dopplerStdDev;

    obj["jammer_active"]        = cfg.jammer.active;
    obj["jammer_power_kW"]      = cfg.jammer.power_kW;
    obj["jammer_gain_dBi"]      = cfg.jammer.gain_dBi;
    obj["jammer_bandwidth_Hz"]  = cfg.jammer.bandwidth_Hz;
    obj["jammer_range_m"]       = cfg.jammer.range_m;
    obj["jammer_selfScreening"] = cfg.jammer.selfScreening;

    return obj;
}

void Radar::fromJson(const QJsonObject& obj)
{
    RadarConfig cfg = radarCore_.getConfig();

    auto getDouble = [&](const QString& k, double  d) { return obj.contains(k) ? obj[k].toDouble(d)           : d; };
    auto getFloat  = [&](const QString& k, float   d) { return obj.contains(k) ? static_cast<float>(obj[k].toDouble(d)) : d; };
    auto getInt    = [&](const QString& k, int     d) { return obj.contains(k) ? obj[k].toInt(d)              : d; };
    auto getBool   = [&](const QString& k, bool    d) { return obj.contains(k) ? obj[k].toBool(d)             : d; };

    cfg.emissionPower_kW      = getDouble("emissionPower_kW",    cfg.emissionPower_kW);
    cfg.frequency_Hz          = getDouble("frequency_Hz",        cfg.frequency_Hz);
    cfg.antennaGain           = getFloat ("antennaGain",         cfg.antennaGain);
    cfg.antennaBandwidth      = getDouble("antennaBandwidth",    cfg.antennaBandwidth);
    cfg.beamWidth             = getFloat ("beamWidth",           cfg.beamWidth);
    cfg.scanningRate_RPM      = getFloat ("scanningRate_RPM",    cfg.scanningRate_RPM);
    cfg.minAzimuth            = getFloat ("minAzimuth",          cfg.minAzimuth);
    cfg.maxAzimuth            = getFloat ("maxAzimuth",          cfg.maxAzimuth);
    cfg.minElevation          = getFloat ("minElevation",        cfg.minElevation);
    cfg.maxElevation          = getFloat ("maxElevation",        cfg.maxElevation);
    cfg.radarHeight           = getDouble("radarHeight",         cfg.radarHeight);
    cfg.minDetectableRange    = getDouble("minDetectableRange",  cfg.minDetectableRange);
    cfg.seaState              = getDouble("seaState",            cfg.seaState);
    cfg.landClutter           = getDouble("landClutter",         cfg.landClutter);
    cfg.maxTrackSpeed         = getDouble("maxTrackSpeed",       cfg.maxTrackSpeed);
    cfg.systemTemperature_K   = getDouble("systemTemperature_K", cfg.systemTemperature_K);
    cfg.noiseFigure_dB        = getDouble("noiseFigure_dB",      cfg.noiseFigure_dB);
    cfg.targetPfa             = getDouble("targetPfa",           cfg.targetPfa);
    cfg.missedScansToDrop     = getInt   ("missedScansToDrop",   cfg.missedScansToDrop);
    cfg.trackCoastSeconds     = getDouble("trackCoastSeconds",   cfg.trackCoastSeconds);
    cfg.minHitsToValidate     = getInt   ("minHitsToValidate",   cfg.minHitsToValidate);
    cfg.prfLevels[0]          = getFloat ("prfLevel0",           cfg.prfLevels[0]);
    cfg.pulseWidth            = getFloat ("pulseWidth",          cfg.pulseWidth);
    cfg.earthRadiusFactor     = getDouble("earthRadiusFactor",   cfg.earthRadiusFactor);
    cfg.atmosphericFactor     = getDouble("atmosphericFactor",   cfg.atmosphericFactor);
    cfg.rainRate_mmph         = getDouble("rainRate_mmph",       cfg.rainRate_mmph);
    cfg.fogVisibility_m       = getDouble("fogVisibility_m",     cfg.fogVisibility_m);
    cfg.scanType              = static_cast<ScanType>(getInt("scanType", static_cast<int>(cfg.scanType)));
    cfg.mode                  = static_cast<RadarMode>(getInt("mode",    static_cast<int>(cfg.mode)));

    cfg.noise.rangeStdDev     = getDouble("noise_rangeStdDev",   cfg.noise.rangeStdDev);
    cfg.noise.azimuthStdDev   = getDouble("noise_azimuthStdDev", cfg.noise.azimuthStdDev);
    cfg.noise.elevationStdDev = getDouble("noise_elevStdDev",    cfg.noise.elevationStdDev);
    cfg.noise.dopplerStdDev   = getDouble("noise_dopplerStdDev", cfg.noise.dopplerStdDev);

    cfg.jammer.active         = getBool  ("jammer_active",       false);
    cfg.jammer.power_kW       = getDouble("jammer_power_kW",     0.0);
    cfg.jammer.gain_dBi       = getDouble("jammer_gain_dBi",     0.0);
    cfg.jammer.bandwidth_Hz   = getDouble("jammer_bandwidth_Hz", cfg.jammer.bandwidth_Hz);
    cfg.jammer.range_m        = getDouble("jammer_range_m",      cfg.jammer.range_m);
    cfg.jammer.selfScreening  = getBool  ("jammer_selfScreening",false);

    radarCore_.setConfig(cfg);
    displayRangeDirty_ = true;
}
// #include "radar.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
// #include <QJsonDocument>
// #include <QVector3D>
// #include <QJsonObject>
// #include <QJsonValue>
// #include <cmath>

// // ---------------------------------------------------------------------------
// // Constants
// // ---------------------------------------------------------------------------
// static constexpr double RAD2DEG = 180.0 / M_PI;
// static constexpr double DEG2RAD = M_PI / 180.0;
// static constexpr double DEFAULT_RCS  = 10.0;   // broadside surface target ~10m² — replace per-platform later
// static constexpr double COORD_SCALE  = 1000.0;
// static constexpr double MIN_DT       = 1e-4;
// static constexpr double MAX_DT       = 1.0;

// // ===========================================================================
// // Constructor
// // ===========================================================================
// Radar::Radar(Hierarchy* h) : Sensor(h)
// {
//     subType = SubType::Generic;

//     RadarAttributes cfg;

//     cfg.emissionPower_kW    = 100.0;
//     cfg.frequency_Hz        = 8e9;
//     cfg.antennaGain         = 35.0;
//     cfg.antennaBandwidth    = 1e6;
//     cfg.beamWidth           = 3.0;
//     cfg.systemTemperature_K = 290.0;
//     cfg.noiseFigure_dB      = 5.0;
//     cfg.targetPfa           = 1e-6;
//     cfg.minAzimuth          = -60.0f;
//     cfg.maxAzimuth          =  60.0f;
//     cfg.minElevation        = -30.0f;
//     cfg.maxElevation        =  30.0f;
//     cfg.scanningRate_RPM    = 12.0f;
//     cfg.scanType            = ScanType::MECHANICAL;
//     cfg.prfLevels[0]        = 5000.0f;
//     cfg.radarHeight         = 20.0;
//     cfg.minDetectableRange  = 30.0;
//     cfg.earthRadiusFactor   = 1.33;
//     cfg.atmosphericFactor   = 1.0;   // standard atmosphere — raise for ducting
//     cfg.missedScansToDrop   = 3;
//     cfg.trackCoastSeconds   = 30.0;
//     cfg.minHitsToValidate   = 2;//3 ideal
//     cfg.maxTrackSpeed       = 2000.0;
//     cfg.pulseWidth          = 1e-6f;
//     cfg.seaState            = 0.0;
//     cfg.landClutter         = 0.0;
//     cfg.mode                = RadarMode::TWS;

//     cfg.jammer.active       = true;
//     cfg.jammer.power_kW     = 0.0;
//     cfg.jammer.gain_dBi     = 20.0;
//     cfg.jammer.selfScreening = true;

//     radarCore.setConfiguration(cfg);
//     maxDetectionAngle = cfg.maxAzimuth;   // ← MISSING
//     radarCore.reset();
// }

// // ===========================================================================
// // Lock-on / break-lock
// // ===========================================================================
// void Radar::lockOn(uint32_t radarTargetID)
// {
//     RadarAttributes cfg  = radarCore.getConfiguration();
//     cfg.mode             = RadarMode::LOCK_ON;
//     cfg.lockedTargetID   = radarTargetID;
//     radarCore.setConfiguration(cfg);
// }

// void Radar::breakLock()
// {
//     RadarAttributes cfg  = radarCore.getConfiguration();
//     cfg.mode             = RadarMode::SURVEILLANCE;
//     cfg.lockedTargetID   = 0;
//     radarCore.setConfiguration(cfg);
// }

// // ===========================================================================
// // Static helpers
// // ===========================================================================
// uint32_t Radar::platformToRadarID(const std::string& key)
// {
//     return static_cast<uint32_t>(std::hash<std::string>{}(key));
// }

// double Radar::platformRCS(const Platform* platform)
// {
//     if (!platform) return DEFAULT_RCS;
//    return DEFAULT_RCS;   // TODO: read from platform->profile->rcs when profile is ready
// }

// void Radar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
//                                      double& vx, double& vy, double& vz)
// {
//     double rad = headingDeg * DEG2RAD;
//     vx = speedMs * std::sin(rad);   // East  (lateral)
//     vy = 0.0;                       // Up    (vertical — zero)
//     vz = speedMs * std::cos(rad);   // North (forward)
// }
// // void Radar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
// //                                      double& vx, double& vy, double& vz)
// // {
// //     double rad = headingDeg * DEG2RAD;
// //     vx = speedMs * std::cos(rad);
// //     vy = 0.0;
// //     vz = speedMs * std::sin(rad);
// // }

// // ===========================================================================
// // collectTargets
// // ===========================================================================
// std::vector<TargetInput> Radar::collectTargets(
//     Transform* source,
//     std::unordered_map<uint32_t, Platform*>& outIdMap) const
// {
//     std::vector<TargetInput> inputs;
//     outIdMap.clear();

//     if (!root || !root->Platforms) return inputs;
//     inputs.reserve(root->Platforms->size());

//     for (auto& [key, entity] : *root->Platforms)
//     {
//         if (!entity || key == parentEntity->ID) continue;

//         Platform* platform = entity;
//         if (!platform->transform) continue;

//         QVector3D localPos =
//             source->inverseTransformPoint(
//                 platform->transform->matrix->translation());

//         TargetInput t;
//         t.x = static_cast<double>(localPos.z()) * COORD_SCALE;
//         t.y = static_cast<double>(localPos.x()) * COORD_SCALE;
//         t.z = static_cast<double>(localPos.y()) * COORD_SCALE;

//         t.id      = platformToRadarID(key);
//         outIdMap[t.id] = platform;

//         if (platform->dynamicModel)
//         {
//             velocityFromHeadingSpeed(
//                 static_cast<double>(platform->dynamicModel->TrueHeading),
//                 static_cast<double>(platform->dynamicModel->currentSpeed),
//                 t.vx, t.vy, t.vz);
//         }
//         else
//         {
//             t.vx = t.vy = t.vz = 0.0;
//         }

//         t.rcs     = platformRCS(platform);
//         t.surface = SurfaceType::SEA;

//         inputs.push_back(t);
//     }

//     return inputs;
// }

// // ===========================================================================
// // processSurveillance
// // ===========================================================================
// void Radar::processSurveillance(
//     const std::vector<DetectionOutput>& detections,
//     const std::unordered_map<uint32_t, Platform*>& idMap,
//     std::unordered_set<uint32_t>& addedIDs)
// {
//     QString radarName = QString::fromStdString(parentEntity->Name);

//     qDebug().noquote()
//         << QString("[ %1 ]  SURVEILLANCE — %2 detection(s)")
//                .arg(radarName).arg(detections.size());

//     for (const auto& d : detections)
//     {
//         if (addedIDs.count(d.targetID)) continue;
//         addedIDs.insert(d.targetID);

//         // Resolve detected entity name
//         QString targetName = QString("id:%1").arg(d.targetID);
//         Platform* platform = nullptr;
//         auto it = idMap.find(d.targetID);
//         if (it != idMap.end()) {
//             platform   = it->second;
//             targetName = QString::fromStdString(platform->Name);
//         }



//         Target t{};
//         if (platform) t.entity = platform;
//         t.radius    = static_cast<float>(d.range / 1000.0);
//         t.angle     = static_cast<float>(d.azimuth);
//         t.speed     = static_cast<float>(d.speedOverGround);
//         t.direction = static_cast<float>(d.heading);
//         //t.altitude       = static_cast<float>(d.elevation);   // elevation from detection
//         t.altitude = static_cast<float>(d.range * std::sin(d.elevation * M_PI / 180.0));
//         t.radialVelocity = static_cast<float>(d.radialVelocity);    // radial velocity from detection

//         targets.append(t);
//     }
// }

// // ===========================================================================
// // processTWS
// // ===========================================================================
// void Radar::processTWS(
//     const std::vector<TrackFile>& tracks,
//     const std::unordered_map<uint32_t, Platform*>& idMap,
//     std::unordered_set<uint32_t>& addedIDs)
// {
//     QString radarName = QString::fromStdString(parentEntity->Name);

//     qDebug().noquote()
//         << QString("[ %1 ]  TWS — %2 active track(s)")
//                .arg(radarName).arg(tracks.size());

//     for (const auto& tr : tracks)
//     {
//         if (addedIDs.count(tr.id)) continue;
//         addedIDs.insert(tr.id);

//         // Resolve tracked entity name
//         QString targetName = QString("id:%1").arg(tr.id);
//         Platform* platform = nullptr;
//         auto it = idMap.find(tr.id);
//         if (it != idMap.end()) {
//             platform   = it->second;
//             targetName = QString::fromStdString(platform->Name);
//         }

//         double az    = std::atan2(tr.y, tr.x) * RAD2DEG;
//         if (az < 0.0) az += 360.0;
//         double speed = std::sqrt(tr.vx*tr.vx + tr.vy*tr.vy + tr.vz*tr.vz);

//         // Status: CONFIRMED or TENTATIVE, with coast count if coasting
//         QString status = tr.isValidated ? "CONFIRMED" : "TENTATIVE";
//         if (tr.scanMissCount > 0)
//             status += QString(" [coast %1]").arg(tr.scanMissCount);

//         // qDebug().noquote()
//         //     << QString("  ├─ %1  range=%2 km  az=%3°  speed=%4 m/s  radVel=%5 m/s  %6")
//         //            .arg(targetName, -20)
//         //            .arg(tr.range / 1000.0, 7, 'f', 2)
//         //            .arg(az,                6, 'f', 1)
//         //            .arg(speed,             6, 'f', 1)
//         //            .arg(tr.velocity,       6, 'f', 1)
//         //            .arg(status);

//         Target t{};
//         if (platform) t.entity = platform;
//         t.radius    = static_cast<float>(tr.range / 1000.0);
//         t.angle     = static_cast<float>(az);
//         t.speed     = static_cast<float>(speed);
//         // t.direction = static_cast<float>(std::atan2(tr.vy, tr.vx) * RAD2DEG);
//         // CORRECT:
//         t.direction = static_cast<float>(std::atan2(tr.vx, tr.vz) * RAD2DEG);
//         if (t.direction < 0.0f) t.direction += 360.0f;
//         t.radialVelocity = static_cast<float>(tr.velocity);
//         t.altitude = static_cast<float>(tr.z);
//         // qDebug().noquote()
//         //     << QString("  [TWS_DEBUG] %1")
//         //            .arg(targetName, -20)
//         //     << "\n    INPUT  (from TrackFile):"
//         //     << QString("    vx=%1  vy=%2  vz=%3  range=%4m  z=%5m  tr.velocity=%6m/s")
//         //            .arg(tr.vx,       0, 'f', 3)
//         //            .arg(tr.vy,       0, 'f', 3)
//         //            .arg(tr.vz,       0, 'f', 3)
//         //            .arg(tr.range,    0, 'f', 1)
//         //            .arg(tr.z,        0, 'f', 1)
//         //            .arg(tr.velocity, 0, 'f', 3)
//         //     << "\n    OUTPUT (to Target/display):"
//         //     << QString("    S=%1m/s(%2kt)  A=%3m  H=%4°  RV=%5m/s")
//         //            .arg(t.speed,          0, 'f', 2)
//         //            .arg(t.speed * 1.94384f, 0, 'f', 1)
//         //            .arg(t.altitude,       0, 'f', 1)
//         //            .arg(t.direction,      0, 'f', 1)
//         //            .arg(t.radialVelocity, 0, 'f', 2);
//         targets.append(t);
//     }
// }

// // ===========================================================================
// // processLockOn
// // ===========================================================================
// void Radar::processLockOn(
//     const std::vector<TrackFile>& tracks,
//     const std::unordered_map<uint32_t, Platform*>& idMap)
// {
//     QString radarName = QString::fromStdString(parentEntity->Name);

//     if (tracks.empty())
//     {
//         qDebug().noquote()
//         << QString("[ %1 ]  LOCK LOST — no validated tracks").arg(radarName);
//         return;
//     }

//     uint32_t lockedID     = radarCore.getConfiguration().lockedTargetID;
//     const TrackFile* locked = nullptr;
//     for (const auto& tr : tracks)
//         if (tr.id == lockedID) { locked = &tr; break; }

//     if (!locked)
//     {
//         qDebug().noquote()
//         << QString("[ %1 ]  LOCK LOST — target not in validated tracks").arg(radarName);
//         return;
//     }

//     // Resolve locked entity name
//     QString targetName = QString("id:%1").arg(locked->id);
//     Platform* platform = nullptr;
//     auto it = idMap.find(locked->id);
//     if (it != idMap.end()) {
//         platform   = it->second;
//         targetName = QString::fromStdString(platform->Name);
//     }

//     double az    = std::atan2(locked->y, locked->x) * RAD2DEG;
//     if (az < 0.0) az += 360.0;
//     double speed = std::sqrt(
//         locked->vx*locked->vx + locked->vy*locked->vy + locked->vz*locked->vz);

//     Target t{};
//     if (platform) t.entity = platform;
//     t.radius = static_cast<float>(locked->range / 1000.0);
//     t.angle  = static_cast<float>(az);
//     t.speed  = static_cast<float>(speed);
//     // ── These three were missing — caused the "frozen" appearance ──
//     t.direction = static_cast<float>(std::atan2(locked->vx, locked->vz) * RAD2DEG);
//     if (t.direction < 0.0f) t.direction += 360.0f;

//     t.radialVelocity = static_cast<float>(locked->velocity);
//     t.altitude       = static_cast<float>(locked->z);

//     targets.append(t);
// }

// // ===========================================================================
// // logModeBanner
// // ===========================================================================
// void Radar::logModeBanner(RadarMode mode) const
// {
//     QString radarName = QString::fromStdString(parentEntity->Name);

//     QString modeStr;
//     switch (mode)
//     {
//     case RadarMode::SURVEILLANCE: modeStr = "SURVEILLANCE";      break;
//     case RadarMode::TWS:          modeStr = "TRACK-WHILE-SCAN";  break;
//     case RadarMode::LOCK_ON:      modeStr = "FIRE CONTROL LOCK"; break;
//     }

//     qDebug().noquote()
//         << QString("\n[ %1 ]  %2  |  Az=%3°")
//                .arg(radarName)
//                .arg(modeStr)
//                .arg(radarCore.getCurrentAzimuth(), 0, 'f', 1);
// }

// // ===========================================================================
// // scan()
// // ===========================================================================
// void Radar::scan()
// {
//     if (!parentEntity) return;

//     Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
//     if (!source) return;

//     double dt;
//     if (!timerStarted)
//     {
//         frameTimer.start();
//         timerStarted = true;
//         dt = 0.05;
//     }
//     else
//     {
//         dt = static_cast<double>(frameTimer.elapsed()) / 1000.0;
//         frameTimer.restart();
//         dt = std::clamp(dt, MIN_DT, MAX_DT);
//     }

//     simTime += dt;

//     std::unordered_map<uint32_t, Platform*> idMap;
//     // Sync radarHeight from actual platform elevation
//     // Surface/land platforms stay at their fixed mast height (config default)
//     // Air platforms must use real-time altitude for correct horizon calculation
//     {
//         RadarAttributes cfg = radarCore.getConfiguration();

//         float platformAltitude = static_cast<float>(
//                                      (*root->Platforms)[parentEntity->ID]
//                                          ->transform->matrix->translation().y()
//                                      ) * static_cast<float>(COORD_SCALE);

//         qDebug() << "[ Radar ] platform raw_y="
//                  << (*root->Platforms)[parentEntity->ID]
//                         ->transform->matrix->translation().y()
//                  << "  altitude_m=" << platformAltitude;

//         // Only override if platform is airborne (above 50m threshold)
//         // Below 50m treat as surface platform and keep fixed radarHeight
//         if (platformAltitude > 50.0f) {
//             if (static_cast<double>(platformAltitude) != cfg.radarHeight) {
//                 cfg.radarHeight = static_cast<double>(platformAltitude);
//                 radarCore.setConfiguration(cfg);
//                 displayRangeDirty = true;  // altitude changed — recalculate display range
//             }
//         }
//     }
//     // }
//     auto radarInputs = collectTargets(source, idMap);

//     radarCore.update(dt, radarInputs, simTime);
//     azimuth   = static_cast<float>(radarCore.getCurrentAzimuth());
//     beamWidth = radarCore.getConfiguration().beamWidth;
//     //range     = 100.0f;
//     // With:
//     // FIX 4: only recompute when config has changed, not every frame
//     if (displayRangeDirty) {
//         cachedDisplayRange = static_cast<float>(radarCore.computeMaxDetectionRange());
//         cachedDisplayRange = std::clamp(cachedDisplayRange, 5.0f, 1000.0f);  // FIX 7
//         displayRangeDirty  = false;
//     }
//     range = cachedDisplayRange;    // ADD THESE — keep sensor base class in sync with radar config
//     maxDetectionAngle = radarCore.getConfiguration().maxAzimuth;
//     RadarMode currentMode = radarCore.getMode();  // ← ONE declaration, no shadow

//     switch (currentMode) {
//     case RadarMode::SURVEILLANCE: mode = Sensor::Mode::Search;         break;
//     case RadarMode::TWS:          mode = Sensor::Mode::TrackWhileScan; break;
//     case RadarMode::LOCK_ON:      mode = Sensor::Mode::FireControl;    break;
//     }

//     qDebug() << "SCAN_DEBUG rpm=" << radarCore.getConfiguration().scanningRate_RPM
//              << "az=" << radarCore.getCurrentAzimuth()
//              << "dt=" << dt;

//     logModeBanner(currentMode);   // ← called ONCE

//     targets.clear();
//     std::unordered_set<uint32_t> addedIDs;

//     switch (currentMode)
//     {
//     case RadarMode::SURVEILLANCE:
//         processSurveillance(radarCore.getActiveDetections(), idMap, addedIDs);
//         break;
//     case RadarMode::TWS:
//         processTWS(radarCore.getActiveTracks(), idMap, addedIDs);
//         break;
//     case RadarMode::LOCK_ON:
//         processLockOn(radarCore.getActiveTracks(), idMap);
//         break;
//     }
// }

// // ===========================================================================
// // Serialisation
// // ===========================================================================
// QJsonObject Radar::toJson() const
// {
//     QJsonObject obj;
//    // QJsonObject obj;
//     RadarAttributes cfg = radarCore.getConfiguration();

//     obj["id"]   = QString::fromStdString(ID);    // ← ADD THIS
//     obj["name"] = QString::fromStdString(Name);  // ← ADD THIS
//     obj["Active"] = Active;                       // ← ADD THIS
//     obj["SensorType"] = "Radar";

//     obj["emissionPower_kW"]    = cfg.emissionPower_kW;
//     obj["frequency_Hz"]        = cfg.frequency_Hz;
//     obj["antennaGain"]         = cfg.antennaGain;
//     obj["antennaBandwidth"]    = cfg.antennaBandwidth;
//     obj["beamWidth"]           = cfg.beamWidth;
//     obj["scanningRate_RPM"]    = cfg.scanningRate_RPM;
//     obj["minAzimuth"]          = cfg.minAzimuth;
//     obj["maxAzimuth"]          = cfg.maxAzimuth;
//     obj["minElevation"]        = cfg.minElevation;
//     obj["maxElevation"]        = cfg.maxElevation;
//     obj["radarHeight"]         = cfg.radarHeight;
//     obj["minDetectableRange"]  = cfg.minDetectableRange;
//     obj["seaState"]            = cfg.seaState;
//     obj["landClutter"]         = cfg.landClutter;
//     obj["maxTrackSpeed"]       = cfg.maxTrackSpeed;
//     obj["systemTemperature_K"] = cfg.systemTemperature_K;
//     obj["noiseFigure_dB"]      = cfg.noiseFigure_dB;
//     obj["targetPfa"]           = cfg.targetPfa;
//     obj["missedScansToDrop"]   = cfg.missedScansToDrop;
//     obj["trackCoastSeconds"]   = cfg.trackCoastSeconds;
//     obj["minHitsToValidate"]   = cfg.minHitsToValidate;
//     obj["prfLevel0"]           = cfg.prfLevels[0];
//     obj["mode"]                = static_cast<int>(cfg.mode);
//     obj["noise_rangeStdDev"]   = cfg.noise.rangeStdDev;
//     obj["noise_azimuthStdDev"] = cfg.noise.azimuthStdDev;
//     obj["noise_elevStdDev"]    = cfg.noise.elevationStdDev;
//     obj["noise_dopplerStdDev"] = cfg.noise.dopplerStdDev;
//     // toJson()
//     obj["jammer_active"]       = cfg.jammer.active;
//     obj["jammer_power_kW"]     = cfg.jammer.power_kW;
//     obj["jammer_gain_dBi"]     = cfg.jammer.gain_dBi;
//     obj["jammer_selfScreening"]= cfg.jammer.selfScreening;
//     obj["atmosphericFactor"]   = cfg.atmosphericFactor;
//     // toJson() — add these:
//     obj["pulseWidth"]          = cfg.pulseWidth;
//     obj["earthRadiusFactor"]   = cfg.earthRadiusFactor;
//     obj["rainRate_mmph"]       = cfg.rainRate_mmph;
//     obj["fogVisibility_m"]     = cfg.fogVisibility_m;
//     obj["jammer_bandwidth_Hz"] = cfg.jammer.bandwidth_Hz;
//     obj["jammer_range_m"]      = cfg.jammer.range_m;
//     obj["scanType"]            = static_cast<int>(cfg.scanType);

//     return obj;
// }

// void Radar::fromJson(const QJsonObject& obj)
// {
//     RadarAttributes cfg = radarCore.getConfiguration();

//     auto getDouble = [&](const QString& k, double d) { return obj.contains(k) ? obj[k].toDouble(d) : d; };
//     auto getFloat  = [&](const QString& k, float  d) { return obj.contains(k) ? static_cast<float>(obj[k].toDouble(d)) : d; };
//     auto getInt    = [&](const QString& k, int    d) { return obj.contains(k) ? obj[k].toInt(d) : d; };

//     cfg.emissionPower_kW      = getDouble("emissionPower_kW",    cfg.emissionPower_kW);
//     cfg.frequency_Hz          = getDouble("frequency_Hz",        cfg.frequency_Hz);
//     cfg.antennaGain           = getFloat ("antennaGain",         cfg.antennaGain);
//     cfg.antennaBandwidth      = getDouble("antennaBandwidth",    cfg.antennaBandwidth);
//     cfg.beamWidth             = getFloat ("beamWidth",           cfg.beamWidth);
//     cfg.scanningRate_RPM      = getFloat ("scanningRate_RPM",    cfg.scanningRate_RPM);
//     cfg.minAzimuth            = getFloat ("minAzimuth",          cfg.minAzimuth);
//     cfg.maxAzimuth            = getFloat ("maxAzimuth",          cfg.maxAzimuth);
//     cfg.minElevation          = getFloat ("minElevation",        cfg.minElevation);
//     cfg.maxElevation          = getFloat ("maxElevation",        cfg.maxElevation);
//     cfg.radarHeight           = getDouble("radarHeight",         cfg.radarHeight);
//     cfg.minDetectableRange    = getDouble("minDetectableRange",  cfg.minDetectableRange);
//     cfg.seaState              = getDouble("seaState",            cfg.seaState);
//     cfg.landClutter           = getDouble("landClutter",         cfg.landClutter);
//     cfg.maxTrackSpeed         = getDouble("maxTrackSpeed",       cfg.maxTrackSpeed);
//     cfg.systemTemperature_K   = getDouble("systemTemperature_K", cfg.systemTemperature_K);
//     cfg.noiseFigure_dB        = getDouble("noiseFigure_dB",      cfg.noiseFigure_dB);
//     cfg.targetPfa             = getDouble("targetPfa",           cfg.targetPfa);
//     cfg.missedScansToDrop     = getInt   ("missedScansToDrop",   cfg.missedScansToDrop);
//     cfg.trackCoastSeconds     = getDouble("trackCoastSeconds",   cfg.trackCoastSeconds);
//     cfg.minHitsToValidate     = getInt   ("minHitsToValidate",   cfg.minHitsToValidate);
//     cfg.prfLevels[0]          = getFloat ("prfLevel0",           cfg.prfLevels[0]);
//     cfg.mode                  = static_cast<RadarMode>(getInt("mode", static_cast<int>(cfg.mode)));
//     cfg.noise.rangeStdDev     = getDouble("noise_rangeStdDev",   cfg.noise.rangeStdDev);
//     cfg.noise.azimuthStdDev   = getDouble("noise_azimuthStdDev", cfg.noise.azimuthStdDev);
//     cfg.noise.elevationStdDev = getDouble("noise_elevStdDev",    cfg.noise.elevationStdDev);
//     cfg.noise.dopplerStdDev   = getDouble("noise_dopplerStdDev", cfg.noise.dopplerStdDev);
//     // fromJson()
//     cfg.jammer.active        = obj["jammer_active"].toBool(false);
//     cfg.jammer.power_kW      = getDouble("jammer_power_kW",      0.0);
//     cfg.jammer.gain_dBi      = getDouble("jammer_gain_dBi",      0.0);
//     cfg.jammer.selfScreening = obj["jammer_selfScreening"].toBool(false);
//     cfg.atmosphericFactor    = getDouble("atmosphericFactor",     1.0);
//     // fromJson() — add these:
//     cfg.pulseWidth          = getFloat ("pulseWidth",          cfg.pulseWidth);
//     cfg.earthRadiusFactor   = getDouble("earthRadiusFactor",   cfg.earthRadiusFactor);
//     cfg.rainRate_mmph       = getDouble("rainRate_mmph",       cfg.rainRate_mmph);
//     cfg.fogVisibility_m     = getDouble("fogVisibility_m",     cfg.fogVisibility_m);
//     cfg.jammer.bandwidth_Hz = getDouble("jammer_bandwidth_Hz", cfg.jammer.bandwidth_Hz);
//     cfg.jammer.range_m      = getDouble("jammer_range_m",      cfg.jammer.range_m);
//     cfg.scanType            = static_cast<ScanType>(getInt("scanType", static_cast<int>(cfg.scanType)));
//     radarCore.setConfiguration(cfg);
//     displayRangeDirty = true;  // FIX 4: config changed — recalculate display range

// }
// #include "radar.h"
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/hierarchy.h"
// #include "core/Hierarchy/EntityProfiles/sensor.h"
// const float RAD2DEG = 180.0f / M_PI;
// Radar::Radar(Hierarchy* h) : Sensor(h)  {
//     subType = SubType::Generic;
// }

// void Radar::scan(){
//     if(!Active)return;
//      if(!parentEntity) return;
//     Transform* source = (*root->Platforms)[parentEntity->ID]->transform;
//     if(!source) return;
//     for (auto& [key, entity] : *root->Platforms)
//     {
//         if(!entity || key == parentEntity->ID) continue;
//         Platform* platform = entity;
//         if (platform && platform->transform) {
//             QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
//             //float distance = localPos.length();
//             float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;
//             float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
//             if (detectCheck(localPos,metredis,2)&&platform->Active) // .position() is assumed
//             {
//                 if (detects.count(platform) == 0)
//                 {
//                     detects.insert(platform);
//                     Target target;
//                     target.entity = platform;
//                     if(platform->dynamicModel)
//                         target.speed = platform->dynamicModel->currentSpeed;
//                     target.angle = yAngle;
//                     target.radius = metredis;
//                     targets.append(target);
//                 }else{
//                     for (int i = 0; i < targets.size(); ++i) {
//                         if (targets.at(i).entity == platform) {
//                             targets[i].angle = yAngle;
//                             if(platform->dynamicModel)
//                                 targets[i].speed = platform->dynamicModel->currentSpeed;
//                             targets[i].radius = metredis;
//                             break;
//                         }
//                     }
//                 }
//             }
//             else
//             {
//                 // C# detects.Contains(tr) -> C++ detects.count(tr) > 0
//                 if (detects.count(platform) > 0)
//                 {
//                     // C# detects.Remove(tr) -> C++ detects.erase(tr)
//                     for (int i = 0; i < targets.size(); ++i) {
//                         if (targets.at(i).entity == platform) {
//                             targets.removeAt(i);
//                             break;
//                         }
//                     }
//                     detects.erase(platform);
//                     // qDebug()<< "vanish :"<<&entity->Name;
//                 }
//             }
//         }
//     }
//     // qDebug()<<targets.size();
// }

// QJsonObject Radar::toJson() const {
//     QJsonObject obj;
//     obj["id"] = QString::fromStdString(ID);
//     obj["name"] = QString::fromStdString(Name);
//     obj["Active"] = Active;
//     obj["SensorType"] = "Radar";

//     QJsonObject capabilitiesObj;
//     capabilitiesObj["type"] = "option";
//     QJsonArray optionsArray;
//     for (const QString& opt : DetectionCapabilitiesTypeOptions())
//         optionsArray.append(opt);
//     capabilitiesObj["options"] = optionsArray;
//     capabilitiesObj["value"] = detectionCapabilitiesToString(capabilities);

//     QJsonObject Emmison;
//     Emmison["type"] = "Section";
//     Emmison["power"] = toParm(power,"kw",  0,    1000);
//     Emmison["frequency"] = toParm(frequency,"Ghz", 0.1,  100);
//     obj["Emmison"] = Emmison;

//     QJsonObject Envolope;
//     Envolope["type"] = "Section";
//     Envolope["minAzimuth"] = toParm(minAzimuth,"deg", -180, 0);
//     Envolope["maxAzimuth"] = toParm(maxAzimuth,"deg", 0,    180);
//     Envolope["minElevation"] = toParm(minElevation,"deg", -90,  0);
//     Envolope["maxElevation"] = toParm(maxElevation,"deg", 0,    90);
//     obj["Envolope"] = Envolope;

//     QJsonObject Scanning;
//     Scanning["type"] = "Section";
//     Scanning["rate"] = toParm(rate,"hz", 0, 100);
//     Scanning["hits"] = toParm(hits,"",   0, 100);
//     obj["Scanning"] = Scanning;

//     QJsonObject Antenna;
//     Antenna["type"] = "Section";
//     Antenna["AntennaGain"] = toParm(AntennaGain,"db",  0,    60);
//     Antenna["AntennaBandwidth"] = toParm(AntennaBandwidth,"ghz", 0,    10);
//     Antenna["beamWidth"] = toParm(beamWidth,"deg", 0,    360);
//     Antenna["scanType"] = toParm(scanType,"");
//     Antenna["scanTime1"] = toParm(scanTime1,"");
//     Antenna["scanTime2"] = toParm(scanTime2,"");
//     Antenna["peakSideLobLevel"] = toParm(peakSideLobLevel,"");
//     Antenna["avgSideLobLevel"] = toParm(avgSideLobLevel,"");
//     obj["Antenna"] = Antenna;

//     QJsonObject Pulse;
//     Pulse["type"] = "Section";
//     Pulse["pulseWidth"] = toParm(pulseWidth,"us", 0, 1000);
//     obj["Pulse"] = Pulse;

//     QJsonObject defaultObj;
//     defaultObj["type"] = "Section";
//     defaultObj["range"] = toParm(range,"km", 0, 1000);
//     defaultObj["frequency"] = toParm(frequency,"Ghz", 0, 1000);
//     defaultObj["azimuth"] = toParm(azimuth,"deg", 0,   360);
//     defaultObj["DetectionCapabilities"] = capabilitiesObj;
//     obj["default"] = defaultObj;
//     return obj;
// }

// void Radar::fromJson(const QJsonObject& obj) {
//     if (obj.contains("id")){
//         ID = obj["id"].toString().toStdString();
//     }
//     if (obj.contains("Active"))
//         Active = obj["Active"].toBool();

//     if (obj.contains("Emmison") && obj["Emmison"].isObject()) {
//         QJsonObject Emmison = obj["Emmison"].toObject();
//         if (Emmison.contains("power"))
//             power = valueFromParm(Emmison["power"].toObject());
//         if (Emmison.contains("frequency"))
//             frequency = valueFromParm(Emmison["frequency"].toObject());
//     }

//     if (obj.contains("Envolope") && obj["Envolope"].isObject()) {
//         QJsonObject Envolope = obj["Envolope"].toObject();
//         if (Envolope.contains("minAzimuth"))
//             minAzimuth = valueFromParm(Envolope["minAzimuth"].toObject());
//         if (Envolope.contains("maxAzimuth"))
//             maxAzimuth = valueFromParm(Envolope["maxAzimuth"].toObject());
//         if (Envolope.contains("minElevation"))
//             minElevation = valueFromParm(Envolope["minElevation"].toObject());
//         if (Envolope.contains("maxElevation"))
//             maxElevation = valueFromParm(Envolope["maxElevation"].toObject());
//     }

//     if (obj.contains("Scanning") && obj["Scanning"].isObject()) {
//         QJsonObject Scanning = obj["Scanning"].toObject();
//         if (Scanning.contains("rate"))
//             rate = valueFromParm(Scanning["rate"].toObject());
//         if (Scanning.contains("hits"))
//             hits = valueFromParm(Scanning["hits"].toObject());
//     }

//     if (obj.contains("Antenna") && obj["Antenna"].isObject()) {
//         QJsonObject Antenna = obj["Antenna"].toObject();
//         if (Antenna.contains("AntennaGain"))
//             AntennaGain = valueFromParm(Antenna["AntennaGain"].toObject());
//         if (Antenna.contains("AntennaBandwidth"))
//             AntennaBandwidth = valueFromParm(Antenna["AntennaBandwidth"].toObject());
//         if (Antenna.contains("beamWidth"))
//             beamWidth = valueFromParm(Antenna["beamWidth"].toObject());
//         if (Antenna.contains("scanType"))
//             scanType = valueFromParm(Antenna["scanType"].toObject());
//         if (Antenna.contains("scanTime1"))
//             scanTime1 = valueFromParm(Antenna["scanTime1"].toObject());
//         if (Antenna.contains("scanTime2"))
//             scanTime2 = valueFromParm(Antenna["scanTime2"].toObject());
//         if (Antenna.contains("peakSideLobLevel"))
//             peakSideLobLevel = valueFromParm(Antenna["peakSideLobLevel"].toObject());
//         if (Antenna.contains("avgSideLobLevel"))
//             avgSideLobLevel = valueFromParm(Antenna["avgSideLobLevel"].toObject());
//     }

//     if (obj.contains("Pulse") && obj["Pulse"].isObject()) {
//         QJsonObject Pulse = obj["Pulse"].toObject();
//         if (Pulse.contains("pulseWidth"))
//             pulseWidth = valueFromParm(Pulse["pulseWidth"].toObject());
//     }

//     if (obj.contains("default") && obj["default"].isObject()) {
//         QJsonObject defaultObj = obj["default"].toObject();
//         if (defaultObj.contains("range"))
//             range = valueFromParm(defaultObj["range"].toObject());

//         if (defaultObj.contains("frequency"))
//             frequency = valueFromParm(defaultObj["frequency"].toObject());

//         if (defaultObj.contains("azimuth"))
//             azimuth = valueFromParm(defaultObj["azimuth"].toObject());

//         if (defaultObj.contains("DetectionCapabilities") && defaultObj["DetectionCapabilities"].isObject()) {
//             QJsonObject capabilitiesObj = defaultObj["DetectionCapabilities"].toObject();
//             if (capabilitiesObj.contains("value"))
//                 capabilities = stringTodetectionCapabilities(capabilitiesObj["value"].toString());
//         }
//     }
// }
