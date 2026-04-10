

#include "aesaradar.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>
//#include <QDebug>
#include <cmath>

// ---------------------------------------------------------------------------
// Constants — identical to radar.cpp
// ---------------------------------------------------------------------------

static constexpr double DEG2RAD     = M_PI / 180.0;
static constexpr double COORD_SCALE = 1000.0;
static constexpr double MIN_DT      = 1e-4;
static constexpr double MAX_DT      = 1.0;
static constexpr double DEFAULT_RCS = 5.0;   // m² — AESA air targets

// Persistent velocity helpers — identical pattern to radar.cpp


// =============================================================================
// Constructor
// =============================================================================

AESARadar::AESARadar(Hierarchy* h) : Sensor(h)
{
    subType    = SubType::AESA;
    sensortype = Type::Active;

    aesa::RadarConfig cfg;

    // ---- Array — equivalent to Generic emissionPower_kW = 100 ---------------
    cfg.numElements           = 1000;
    cfg.peakPowerPerElement_W = 100.0f;   // was 10.0f — match Generic 100kW total
    cfg.moduleEfficiency      = 0.70f;
    cfg.failedModules         = 0;
    cfg.maxDutyCycle          = 0.50f;    // was 0.25f — give more headroom

    // ---- Antenna — match Generic exactly ------------------------------------
    cfg.frequency_Hz          = 8.0e9;   // was 10GHz — match Generic 8GHz
    cfg.antennaGain           = 35.0f;   // was 34 — match Generic
    cfg.antennaBandwidth      = 1e6;     // was 500e6 — match Generic 1MHz
    cfg.beamWidth             = 3.0f;    // was 2.0f — match Generic
    cfg.maxSteeringAngle_deg  = 60.0f;

    cfg.sidelobeMode          = aesa::SidelobeMode::LOW_SLL;
    cfg.peakSidelobeLevel     = -25.0f;  // was -45 — match Generic
    cfg.avgSidelobeLevel      = -35.0f;  // was -55 — match Generic
    cfg.sidelobeBlanking_dB   = -15.0f;

    // ---- FoV — match Generic exactly ----------------------------------------
    cfg.minAzimuth   = -60.0f;   cfg.maxAzimuth   = 60.0f;
    cfg.minElevation = -2.0f;   cfg.maxElevation = 15.0f;

    // ---- Dwell times ---------------------------------------------------------
    cfg.searchDwellTime_ms      = 2.0f;  // was 2.0f — longer dwell = more hits
    cfg.trackDwellTime_ms       = 5.0f;
    cfg.fireControlDwellTime_ms = 5.0f;

    // ---- Waveforms — simplified, match Generic pulse width and PRF ----------
    // pulsesPerDwell = PRF × dwellTime
    // search:  5000Hz × 50ms  = 250 pulses
    // track:   5000Hz × 10ms  = 50 pulses
    // fire:    5000Hz × 5ms   = 25 pulses
    cfg.searchWaveform      = {aesa::ModulationType::LFM,  50e-6f, 300.0f,  5e6f, 10, aesa::WaveformMode::LPRF};
    cfg.trackWaveform       = {aesa::ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, aesa::WaveformMode::MPRF};
    cfg.fireControlWaveform = {aesa::ModulationType::NLFM, 5e-6f,  2000.0f, 50e6f, 25, aesa::WaveformMode::HPRF};

    cfg.waveformTable[0] = {  30000.0f, {aesa::ModulationType::NLFM, 5e-6f,  2000.0f, 50e6f, 25, aesa::WaveformMode::HPRF}};
    cfg.waveformTable[1] = { 100000.0f, {aesa::ModulationType::LFM,  10e-6f, 1000.0f, 20e6f, 25, aesa::WaveformMode::MPRF}};
    cfg.waveformTable[2] = { 400000.0f, {aesa::ModulationType::LFM,  50e-6f, 300.0f,  5e6f,  10, aesa::WaveformMode::LPRF}};
    cfg.waveformTable[3] = {}; cfg.waveformTable[4] = {}; cfg.waveformTable[5] = {};

    // Staggered PRF: ratio 9:10 gives LCM Rmax >> any real air target.
    // MPRF:  PRF1=1000Hz (Rmax=150km)  PRF2=1111Hz (Rmax=135km)  → LCM=1350km
    // LPRF:  PRF1=300Hz  (Rmax=500km)  PRF2=333Hz  (Rmax=450km)  → LCM=4500km
    // HPRF has no range ambiguity worth resolving (Rmax=75km, velocity is unambiguous).
    cfg.waveformTable[1].waveform.prf2_Hz = 1111.0f;
    cfg.waveformTable[2].waveform.prf2_Hz =  333.0f;
    cfg.trackWaveform.prf2_Hz             = 1111.0f;
    cfg.searchWaveform.prf2_Hz            =  333.0f;
    cfg.prfType = aesa::PRFType::STAGGERED;

    // ---- Frequency agility OFF — match Generic ------------------------------
    cfg.frequencyAgility  = false;       // was true — Generic has this off
    cfg.hopStartFrequency = 0.0f;
    cfg.hopStopFrequency  = 0.0f;

    // ---- Receiver — match Generic exactly -----------------------------------
    cfg.systemTemperature_K = 290.0;
    cfg.noiseFigure_dB      = 5.0;      // was 4.0 — match Generic
    cfg.targetPfa           = 1e-6;

    // ---- Platform — match Generic exactly -----------------------------------
    cfg.radarHeight           = 20.0;   // was 8000 — match Generic
    cfg.minDetectableRange    = 30.0;   // was 100 — match Generic
    cfg.platformSpeed_m_s     = 0.0f;   // was 250 — CRITICAL, zero until updated by scan()
    cfg.earthRadiusFactor     = 1.33;
    cfg.atmosphericFactor     = 1.0;

    // ---- Clutter — match Generic --------------------------------------------
    cfg.seaState    = 0.0f;
    cfg.landClutter = 0.0f;

    // ---- Track lifecycle — match Generic ------------------------------------
    cfg.targetCategory       = aesa::DetectionCategory::ALL;
    cfg.missedScansToDrop    = 2;
    cfg.trackCoastSeconds    = 8.0;
    cfg.minHitsToValidate    = 2;
    cfg.maxTrackSpeed        = 2000.0;  // was 3000 — match Generic
    cfg.manoeuvreThreshold_m = 500.0;

    // ---- JPDA OFF — keep simple like Generic --------------------------------
    cfg.useJPDA               = false;  // was true — disable for now
    cfg.jpdaFalseAlarmDensity = 1e-6f;

    // ---- IFF ----------------------------------------------------------------
    cfg.interrogationMode = aesa::IFFMode::MODE_3A;

    // ---- Measurement noise — match Generic ----------------------------------
    cfg.noise.rangeStdDev     = 0.0;
    cfg.noise.azimuthStdDev   = 0.0;
    cfg.noise.elevationStdDev = 0.0;
    cfg.noise.dopplerStdDev   = 0.0;

    cfg.mode = aesa::RadarMode::TWS;
    cfg.platformSpeed_m_s = 0.0f;
    cfg.atmosphere.temperature_C   = 30.0f;   // °C
    cfg.atmosphere.humidity_pct    = 30.0f;   // %
    cfg.atmosphere.pressure_hPa    = 1013.25f;// hPa — sea level standard
    cfg.atmosphere.rainRate_mmph   = 0.0f;    // mm/h — clear
    cfg.atmosphere.fogVisibility_m = 0.0f;    // m — clear
    radarCore_.init(cfg);
    radarCore_.start();
    radarCore_.reset();  // clears all pre-fix ghost tracks

    maxDetectionAngle = cfg.maxAzimuth;
}

// =============================================================================
// Lock / break lock
// =============================================================================

void AESARadar::lockOn(uint32_t radarTargetID) { radarCore_.lockOn(radarTargetID); }
void AESARadar::breakLock()                     { radarCore_.breakLock();           }

// =============================================================================
// FIX-12  External track injection
// =============================================================================

void AESARadar::injectExternalTrack(const aesa::TrackOutput& ext)
{
    radarCore_.injectExternalTrack(ext);
    emit externalTrackInjected(ext.id);
}

uint32_t AESARadar::platformToRadarID(const std::string& key)
{
    // FNV-1a 32-bit — deterministic across all platforms and runs
    uint32_t hash = 2166136261u;
    for (unsigned char c : key)
    {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash == 0 ? 1u : hash;  // never return 0, reserved for "no target"
}


void AESARadar::velocityFromHeadingSpeed(double headingDeg, double speedMs,
                                         double& vx, double& vy, double& vz)
{
    double rad = headingDeg * DEG2RAD;
    vx = speedMs * std::cos(rad);
    vy = speedMs * std::sin(rad);
    vz = 0.0;
}

// =============================================================================
// Pose builder — IDENTICAL to radar.cpp
// =============================================================================

aesa::RadarPose AESARadar::buildPose() const
{
    aesa::RadarPose pose;
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
        pose.pitch   = it->second->dynamicModel->pitch;
        pose.roll    = it->second->dynamicModel->roll;
    }
    return pose;
}

// =============================================================================
// Target collector — IDENTICAL coordinate access to radar.cpp
// =============================================================================

std::vector<aesa::TargetInput> AESARadar::collectTargets(
    Transform*                               source,
    std::unordered_map<uint32_t, Platform*>& outIdMap) const
{
    std::vector<aesa::TargetInput> inputs;
    outIdMap.clear();

    if (!root) return inputs;
    inputs.reserve(root->Platforms.size());

    for (auto& [key, entity] : root->Platforms)
    {
        if (!entity || key == parentEntity->ID) continue;

        Platform* platform = entity;
        if (!platform->transform) continue;

        // ---- Same local-coord transform as radar.cpp ----------------------
        QVector3D localPos =
            source->inverseTransformPoint(platform->transform->matrix->translation());

        aesa::TargetInput t;

        // ---- Same axis mapping as radar.cpp: z→x, x→y, y→z ---------------
        t.x = static_cast<double>(localPos.z()) * COORD_SCALE;
        t.y = static_cast<double>(localPos.x()) * COORD_SCALE;
        t.z = static_cast<double>(localPos.y()) * COORD_SCALE;


        t.id = platformToRadarID(key);
        outIdMap[t.id] = platform;

        // ---- Velocity — same finite-difference + dynamicModel as radar.cpp -
        if (platform->dynamicModel)
        {
            QVector3D worldPos = platform->transform->matrix->translation();
            uint32_t  tid      = platformToRadarID(key);

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
                static_cast<double>(platform->dynamicModel->currentSpeed) / 3.6,
                t.vx, t.vy, t.vz);
        }
        else
        {
            t.vx = t.vy = t.vz = 0.0;
        }

        // t.rcs          = platformRCS(platform);
        // t.platformType = "FIGHTER"; // default — ideally read from platform->type
        // // Aspect-dependent RCS table for an F-16-class aircraft
        // t.rcsTable = {
        //     {  0.0f,  0.1f  },   // head-on — very small
        //     { 45.0f,  1.0f  },   // quarter aspect
        //     { 90.0f,  10.0f },   // broadside — largest
        //     {135.0f,  2.0f  },   // rear quarterz
        //     {180.0f,  0.5f  }    // tail-on
        // };
        if (platform->collider)
        {
            t.dimensions.length = static_cast<double>(platform->collider->Length);
            t.dimensions.height = static_cast<double>(platform->collider->Height);
            t.dimensions.width  = static_cast<double>(platform->collider->Width);
            t.dimensions.valid  = true;

            // Set material and shape based on platform type
            // These can later come from platform->type or platform->profile
            t.dimensions.material = aesa::TargetMaterialType::METAL;
            t.dimensions.shape    = aesa::TargetShapeType::GENERIC;


        }


        t.rcs          = 1.0;
        t.rcsTable     = {};
        t.platformType = "GENERIC";
        t.swerlingCase = aesa::SwerlingCase::CASE_I;
        t.surface      = aesa::SurfaceType::AIR;   // AESA = air domain
        t.jammer.active = false;

        inputs.push_back(t);
    }
    return inputs;
}

// =============================================================================
// Output translation — same Target struct population as radar.cpp
// =============================================================================

void AESARadar::processSurveillance(
    const aesa::RadarOutput&                        output,
    const std::unordered_map<uint32_t, Platform*>&  idMap,
    std::unordered_set<uint32_t>&                   addedIDs)
{
    for (const auto& d : output.detections)
    {
        // FIX-03  DRFM ghost → warn, never add to target list
        if (d.isDRFMGhost) {
            emit drfmGhostDetected(d.targetID,
                                   static_cast<float>(d.range),
                                   static_cast<float>(d.azimuth),
                                   static_cast<float>(d.elevation));
            continue;
        }

        if (addedIDs.count(d.targetID)) continue;
        addedIDs.insert(d.targetID);

        Platform* platform = nullptr;
        auto it = idMap.find(d.targetID);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity         = platform;
        t.radius         = static_cast<float>(d.range / 1000.0);       // m → km
        //t.angle          = static_cast<float>(d.azimuth);
        // To:
        float az = static_cast<float>(d.azimuth);
        if (az > 180.0f) az -= 360.0f;
        t.angle = az;
        t.speed          = static_cast<float>(d.speedOverGround);
        t.direction      = static_cast<float>(d.heading);
        //t.altitude       = static_cast<float>(d.range * std::sin(d.elevation * M_PI / 180.0));
        t.altitude = static_cast<float>(
            d.range * std::sin(d.elevation * M_PI / 180.0)
            + radarCore_.getConfig().radarHeight);
        t.radialVelocity = static_cast<float>(d.radialVelocity);
        targets.append(t);
    }
}

void AESARadar::processTWS(
    const aesa::RadarOutput&                        output,
    const std::unordered_map<uint32_t, Platform*>&  idMap,
    std::unordered_set<uint32_t>&                   addedIDs)
{
    // Emit ghost warnings first
    for (const auto& d : output.detections) {
        if (d.isDRFMGhost)
            emit drfmGhostDetected(d.targetID,
                                   static_cast<float>(d.range),
                                   static_cast<float>(d.azimuth),
                                   static_cast<float>(d.elevation));
    }

    for (const auto& tr : output.tracks)
    {
        if (addedIDs.count(tr.id)) continue;
        addedIDs.insert(tr.id);

        // FIX-03  DRFM-suspect tracks are never shown as real targets
        if (tr.isDRFMSuspect) continue;

        Platform* platform = nullptr;
        auto it = idMap.find(tr.id);
        if (it != idMap.end()) platform = it->second;

        Target t{};
        t.entity         = platform;
        t.radius         = static_cast<float>(tr.range / 1000.0);   // m → km
        //t.angle          = static_cast<float>(tr.azimuth);
        // To this — convert body-frame math angle to compass bearing:
        float bodyAz = static_cast<float>(tr.azimuth);
        // tr.azimuth is 0-360 where 0=forward. Convert to -180..+180 for display:
        if (bodyAz > 180.0f) bodyAz -= 360.0f;
        t.angle = bodyAz;
        t.speed          = static_cast<float>(tr.speedOverGround);
        t.direction      = static_cast<float>(tr.heading);
       // t.altitude       = static_cast<float>(tr.z);

        t.altitude = static_cast<float>(tr.z + radarCore_.getConfig().radarHeight);
        t.radialVelocity = static_cast<float>(tr.radialVelocity);
        targets.append(t);

        // FIX-04  IFF result per validated track
        if (tr.isValidated)
            emit iffResult(tr.id,
                           static_cast<int>(tr.iff.response),
                           tr.iff.squawk,
                           static_cast<float>(tr.iff.confidence));

        // FIX-01  Doppler notch warning (radial vel < 30 m/s → near blind zone)
        if (tr.isValidated && std::abs(tr.radialVelocity) < 30.0)
            emit trackBelowDopplerNotch(tr.id);
    }
}

void AESARadar::processLockOn(
    const aesa::RadarOutput&                        output,
    const std::unordered_map<uint32_t, Platform*>&  idMap)
{
    if (output.lockBroken) {
        qDebug() << "[AESARadar] Lock broken";
        return;
    }

    if (output.tracks.empty()) return;

    // Find the locked track — same pattern as radar.cpp
    uint32_t lockedID = radarCore_.getConfig().lockedTargetID;

    const aesa::TrackOutput* locked = nullptr;
    for (const auto& tr : output.tracks)
        if (tr.id == lockedID) { locked = &tr; break; }

    if (!locked) return;

    // FIX-03  Locked track is DRFM suspect — break out, don't fire
    if (locked->isDRFMSuspect) {
        emit drfmGhostDetected(locked->id, 0.0f, 0.0f, 0.0f);
        return;
    }

    Platform* platform = nullptr;
    auto it = idMap.find(locked->id);
    if (it != idMap.end()) platform = it->second;

    Target t{};
    t.entity         = platform;
    t.radius         = static_cast<float>(locked->range / 1000.0);  // m → km
    //t.angle          = static_cast<float>(locked->azimuth);
    float lockAz = static_cast<float>(locked->azimuth);
    if (lockAz > 180.0f) lockAz -= 360.0f;
    t.angle = lockAz;
    t.speed          = static_cast<float>(locked->speedOverGround);
    t.direction      = static_cast<float>(locked->heading);
   // t.altitude       = static_cast<float>(locked->z);
    t.altitude = static_cast<float>(locked->z + radarCore_.getConfig().radarHeight);
    t.radialVelocity = static_cast<float>(locked->radialVelocity);
    targets.append(t);

    // FIX-04  IFF on locked track
    emit iffResult(locked->id,
                   static_cast<int>(locked->iff.response),
                   locked->iff.squawk,
                   static_cast<float>(locked->iff.confidence));
}

// =============================================================================
// scan() — IDENTICAL flow to radar.cpp
// =============================================================================

void AESARadar::scan()
{
    if (!parentEntity) return;

    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if (!source) return;

    // ---- dt — identical to radar.cpp ---------------------------------------
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

    // ---- Pose — identical to radar.cpp -------------------------------------
    aesa::RadarPose pose = buildPose();

    // Update radarHeight from live pose — identical to radar.cpp
    if (pose.y > 50.0)
    {
        aesa::RadarConfig altCfg = radarCore_.getConfig();
        if (std::abs(pose.y - altCfg.radarHeight) > 1.0)
        {
            altCfg.radarHeight = pose.y;
            radarCore_.setConfig(altCfg);
            displayRangeDirty_ = true;
        }
    }

    // {
    //     aesa::RadarConfig altCfg = radarCore_.getConfig();
    //     double liveHeight = std::max(2.0, pose.y);
    //     if (std::abs(liveHeight - altCfg.radarHeight) > 1.0)
    //     {
    //         altCfg.radarHeight = liveHeight;
    //         radarCore_.setConfig(altCfg);
    //         displayRangeDirty_ = true;
    //     }
    // }


   // Update platform speed from dynamicModel if available
    {
        auto it = root->Platforms.find(parentEntity->ID);
        if (it != root->Platforms.end() &&
            it->second->dynamicModel)
        {
            aesa::RadarConfig speedCfg = radarCore_.getConfig();
            speedCfg.platformSpeed_m_s =
                static_cast<float>(it->second->dynamicModel->currentSpeed / 3.6);
            radarCore_.setConfig(speedCfg);   // ← ADD THIS LINE

        }
    }

    // ---- Collect targets — identical access pattern to radar.cpp -----------
    std::unordered_map<uint32_t, Platform*> idMap;
    std::vector<aesa::TargetInput> radarInputs = collectTargets(source, idMap);
    // qDebug() << "[AESA] inputs=" << radarInputs.size()
    //          << "platformSpeed=" << radarCore_.getConfig().platformSpeed_m_s
    //          << "radarHeight="   << radarCore_.getConfig().radarHeight
    //          << "mode="          << static_cast<int>(radarCore_.getConfig().mode);
    // ---- Tick the model ----------------------------------------------------
    radarCore_.update(dt, pose, radarInputs, simTime_);

    // ---- Read output -------------------------------------------------------
    aesa::RadarOutput output = radarCore_.getOutput();

    // ... rest of scan() ...

    // LOG 4 — final target count
    // qDebug() << "[AESA] targets populated=" << targets.size();
    aesa::RadarConfig cfg    = radarCore_.getConfig();

    // ---- Sync Sensor base fields — identical to radar.cpp ------------------
    azimuth           = static_cast<float>(output.currentAzimuth);
    beamWidth         = cfg.beamWidth;
    maxDetectionAngle = cfg.maxAzimuth;

    cachedDisplayRange_ = std::clamp(
        static_cast<float>(output.displayRange_km), 5.0f, 1000.0f);
    range = cachedDisplayRange_;

    // Sync Sensor mode
    switch (output.mode)
    {
    case aesa::RadarMode::SURVEILLANCE: mode = Sensor::Mode::Search;         break;
    case aesa::RadarMode::TWS:          mode = Sensor::Mode::TrackWhileScan; break;
    case aesa::RadarMode::LOCK_ON:      mode = Sensor::Mode::FireControl;    break;
    }

    // ---- Populate targets — identical structure to radar.cpp ---------------
    targets.clear();
    std::unordered_set<uint32_t> addedIDs;

    switch (output.mode)
    {
    case aesa::RadarMode::SURVEILLANCE: processSurveillance(output, idMap, addedIDs); break;
    case aesa::RadarMode::TWS:          processTWS(output, idMap, addedIDs);          break;
    case aesa::RadarMode::LOCK_ON:      processLockOn(output, idMap);                 break;
    }

    // ---- FIX-08  Duty cycle every tick -------------------------------------
    emit schedulerDutyCycle(static_cast<float>(output.currentDutyCycle));

    // ---- Base-class signals — identical to radar.cpp pattern ---------------
    if (!targets.isEmpty())
        emit enemyDetected();
    else
        emit enemyNotFound();
}

// =============================================================================
// toJson — same section / toParm() style as radar.cpp
// =============================================================================

QJsonObject AESARadar::toJson() const
{
    QJsonObject obj;
    aesa::RadarConfig cfg = radarCore_.getConfig();

    // --- Identity ---
    obj["id"]         = QString::fromStdString(ID);
    obj["name"]       = QString::fromStdString(Name);
    obj["Active"]     = Active;
    obj["SensorType"] = "AESARadar";

    // --- Array section (AESA-specific) ---
    QJsonObject array;
    array["type"]                 = "Section";
    array["numElements"]          = toParm(cfg.numElements,            "",    100.0f,  10000.0f, "Number of T/R elements");
    array["peakPowerPerElement_W"]= toParm(cfg.peakPowerPerElement_W,  "W",   0.1f,    100.0f,   "Peak power per element");
    array["moduleEfficiency"]     = toParm(cfg.moduleEfficiency,       "",    0.0f,    1.0f,     "T/R module efficiency");
    array["failedModules"]        = toParm(cfg.failedModules,          "",    0.0f,    1000.0f,  "Failed modules count");
    array["maxDutyCycle"]         = toParm(cfg.maxDutyCycle,           "",    0.0f,    1.0f,     "Max duty cycle");
    obj["array"] = array;

    // --- Transmitter section ---
    QJsonObject transmitter;
    transmitter["type"]             = "Section";
    transmitter["frequency_Hz"]     = toParm(cfg.frequency_Hz,      "Hz",  1e6f,   1e11f,   "Operating frequency");
    transmitter["antennaGain"]      = toParm(cfg.antennaGain,       "dBi", 0.0f,   60.0f,   "Antenna gain");
    transmitter["antennaBandwidth"] = toParm(cfg.antennaBandwidth,  "Hz",  0.0f,   1e9f,    "Receiver bandwidth");
    transmitter["beamWidth"]        = toParm(cfg.beamWidth,         "deg", 0.1f,   30.0f,   "Beam width");
    transmitter["sidelobeMode"]     = static_cast<int>(cfg.sidelobeMode);
    transmitter["peakSidelobeLevel"]= toParm(cfg.peakSidelobeLevel, "dB", -80.0f, 0.0f,    "Peak sidelobe level");
    transmitter["avgSidelobeLevel"] = toParm(cfg.avgSidelobeLevel,  "dB", -80.0f, 0.0f,    "Average sidelobe level");
    transmitter["sidelobeBlanking"] = toParm(cfg.sidelobeBlanking_dB,"dB",-80.0f, 0.0f,    "Guard-horn blanking threshold");
    obj["transmitter"] = transmitter;

    // --- Scan section ---
    QJsonObject scan;
    scan["type"]             = "Section";
    scan["minAzimuth"]       = toParm(cfg.minAzimuth,        "deg", -180.0f, 0.0f,   "Min azimuth");
    scan["maxAzimuth"]       = toParm(cfg.maxAzimuth,        "deg",  0.0f,   180.0f, "Max azimuth");
    scan["minElevation"]     = toParm(cfg.minElevation,      "deg", -90.0f,  0.0f,   "Min elevation");
    scan["maxElevation"]     = toParm(cfg.maxElevation,      "deg",  0.0f,   90.0f,  "Max elevation");
    scan["maxSteeringAngle"] = toParm(cfg.maxSteeringAngle_deg,"deg",0.0f,   90.0f,  "Max electronic steering angle");
    scan["searchDwellTime"]  = toParm(cfg.searchDwellTime_ms,"ms",  0.1f,   100.0f, "Search dwell time");
    scan["trackDwellTime"]   = toParm(cfg.trackDwellTime_ms, "ms",  0.1f,   100.0f, "Track dwell time");
    scan["fcDwellTime"]      = toParm(cfg.fireControlDwellTime_ms,"ms",0.1f,100.0f, "Fire-control dwell time");
    obj["scan"] = scan;

    // --- Waveform section ---
    auto serWF = [](const aesa::BeamWaveform& w) -> QJsonObject {
        QJsonObject o;
        o["modulation"]    = static_cast<int>(w.modulation);
        o["pulseWidth_s"]  = static_cast<double>(w.pulseWidth_s);
        o["prf_Hz"]        = static_cast<double>(w.prf_Hz);
        o["prf2_Hz"]       = static_cast<double>(w.prf2_Hz);   // ← ADD
        o["bandwidth_Hz"]  = static_cast<double>(w.bandwidth_Hz);
        o["pulsesPerDwell"]= w.pulsesPerDwell;
        o["mode"]          = static_cast<int>(w.mode);
        return o;
    };
    QJsonObject waveform;
    waveform["type"]          = "Section";
    waveform["frequencyAgility"]  = cfg.frequencyAgility;
    waveform["hopStartFrequency"] = toParm(cfg.hopStartFrequency, "Hz", 0.0f, 1e11f, "Hop lower bound");
    waveform["hopStopFrequency"]  = toParm(cfg.hopStopFrequency,  "Hz", 0.0f, 1e11f, "Hop upper bound");
    waveform["searchWaveform"]    = serWF(cfg.searchWaveform);
    waveform["trackWaveform"]     = serWF(cfg.trackWaveform);
    waveform["fireControlWaveform"]= serWF(cfg.fireControlWaveform);

    QJsonArray wfTable;
    for (const auto& e : cfg.waveformTable) {
        if (e.maxRange_m <= 0.0f) break;
        QJsonObject we;
        we["maxRange_m"] = static_cast<double>(e.maxRange_m);
        we["waveform"]   = serWF(e.waveform);
        wfTable.append(we);
    }
    waveform["table"] = wfTable;
    obj["waveform"] = waveform;

    // --- Detection section ---
    QJsonObject detection;
    detection["type"]                = "Section";
    detection["systemTemperature_K"] = toParm(cfg.systemTemperature_K, "K",   0.0f,  1000.0f, "System noise temperature");
    detection["noiseFigure_dB"]      = toParm(cfg.noiseFigure_dB,      "dB",  0.0f,  30.0f,   "Receiver noise figure");
    detection["targetPfa"]           = toParm(cfg.targetPfa,           "",    0.0f,  1.0f,    "False alarm probability");
    detection["minDetectableRange"]  = toParm(cfg.minDetectableRange,  "m",   0.0f,  1000.0f, "Min detectable range");
    detection["seaState"]            = toParm(cfg.seaState,            "",    0.0f,  9.0f,    "Sea state");
    detection["landClutter"]         = toParm(cfg.landClutter,         "",    0.0f,  1.0f,    "Land clutter factor");
    detection["targetCategory"]      = static_cast<int>(cfg.targetCategory);
    obj["detection"] = detection;

    // --- Platform section ---
    QJsonObject platform;
    platform["type"]              = "Section";
    platform["radarHeight"]       = toParm(cfg.radarHeight,       "m",   0.0f, 30000.0f, "Radar platform height");
    platform["platformSpeed_m_s"] = toParm(cfg.platformSpeed_m_s, "m/s", 0.0f, 3000.0f,  "Platform speed (FIX-01)");
    platform["minDetectableRange"]= toParm(cfg.minDetectableRange,"m",   0.0f, 10000.0f, "Min detectable range");
    obj["platform"] = platform;

    // --- Tracking section ---
    QJsonObject tracking;
    tracking["type"]               = "Section";
    tracking["missedScansToDrop"]  = toParm(cfg.missedScansToDrop,    "",    0.0f, 20.0f,   "Missed scans before drop");
    tracking["trackCoastSeconds"]  = toParm(cfg.trackCoastSeconds,    "s",   0.0f, 300.0f,  "Coast duration");
    tracking["minHitsToValidate"]  = toParm(cfg.minHitsToValidate,    "",    1.0f, 10.0f,   "Hits to validate");
    tracking["maxTrackSpeed"]      = toParm(cfg.maxTrackSpeed,        "m/s", 0.0f, 5000.0f, "Max track speed");
    tracking["manoeuvreThreshold"] = toParm(cfg.manoeuvreThreshold_m, "m",   0.0f, 5000.0f, "Manoeuvre detection threshold");
    tracking["useJPDA"]            = cfg.useJPDA;
    tracking["jpdaFalseAlarmDensity"] = toParm(cfg.jpdaFalseAlarmDensity,"",0.0f,1.0f,    "JPDA false alarm density");
    obj["tracking"] = tracking;

    // --- Propagation section ---
    QJsonObject propagation;
    propagation["type"]              = "Section";
    propagation["earthRadiusFactor"] = toParm(cfg.earthRadiusFactor, "",     1.0f, 2.0f,     "Earth radius factor");
    propagation["atmosphericFactor"] = toParm(cfg.atmosphericFactor, "",     0.0f, 2.0f,     "Atmospheric refraction");
    propagation["temperature_C"]    = toParm(cfg.atmosphere.temperature_C,   "°C",  -60.0f,  60.0f,    "Ambient temperature");
    propagation["humidity_pct"]     = toParm(cfg.atmosphere.humidity_pct,    "%",    0.0f,   100.0f,   "Relative humidity");
    propagation["pressure_hPa"]     = toParm(cfg.atmosphere.pressure_hPa,    "hPa",  800.0f, 1100.0f,  "Atmospheric pressure");
    propagation["rainRate_mmph"]    = toParm(cfg.atmosphere.rainRate_mmph,   "mm/h", 0.0f,   200.0f,   "Rain rate");
    propagation["fogVisibility_m"]  = toParm(cfg.atmosphere.fogVisibility_m, "m",    0.0f,   10000.0f, "Fog visibility");
    obj["propagation"] = propagation;

    // --- Noise section ---
    QJsonObject noise;
    noise["type"]            = "Section";
    noise["rangeStdDev"]     = toParm(cfg.noise.rangeStdDev,     "m",   0.0f, 1000.0f, "Range noise σ");
    noise["azimuthStdDev"]   = toParm(cfg.noise.azimuthStdDev,   "deg", 0.0f, 10.0f,   "Azimuth noise σ");
    noise["elevationStdDev"] = toParm(cfg.noise.elevationStdDev, "deg", 0.0f, 10.0f,   "Elevation noise σ");
    noise["dopplerStdDev"]   = toParm(cfg.noise.dopplerStdDev,   "m/s", 0.0f, 100.0f,  "Doppler noise σ");
    obj["noise"] = noise;

    // --- IFF section (FIX-04) ---
    QJsonObject iff;
    iff["type"]               = "Section";
    iff["interrogationMode"]  = static_cast<int>(cfg.interrogationMode);
    QJsonArray squawks;
    for (uint32_t sq : cfg.friendlySquawks) squawks.append(static_cast<int>(sq));
    iff["friendlySquawks"]    = squawks;
    obj["iff"] = iff;

    // --- Null steering ---
    QJsonObject nullSteering;
    nullSteering["type"]         = "Section";
    nullSteering["active"]       = cfg.nullSteering.active;
    nullSteering["azimuth_deg"]  = toParm(cfg.nullSteering.azimuth_deg,   "deg", -180.0f, 180.0f,  "Null azimuth");
    nullSteering["elevation_deg"]= toParm(cfg.nullSteering.elevation_deg, "deg",  -90.0f,  90.0f,  "Null elevation");
    nullSteering["nullDepth_dB"] = toParm(cfg.nullSteering.nullDepth_dB,  "dB",  -100.0f,   0.0f,  "Null depth");
    obj["nullSteering"] = nullSteering;

    // --- Mode ---
    obj["mode"] = static_cast<int>(cfg.mode);

    return obj;
}

// =============================================================================
// fromJson — same valueFromParm() style as radar.cpp
// =============================================================================

void AESARadar::fromJson(const QJsonObject& obj)
{
    aesa::RadarConfig cfg = radarCore_.getConfig();

    // --- Array ---
    if (obj.contains("array") && obj["array"].isObject())
    {
        QJsonObject a = obj["array"].toObject();
        if (a.contains("numElements"))           cfg.numElements           = static_cast<int>(valueFromParm(a["numElements"].toObject()));
        if (a.contains("peakPowerPerElement_W")) cfg.peakPowerPerElement_W = valueFromParm(a["peakPowerPerElement_W"].toObject());
        if (a.contains("moduleEfficiency"))      cfg.moduleEfficiency      = valueFromParm(a["moduleEfficiency"].toObject());
        if (a.contains("failedModules"))         cfg.failedModules         = static_cast<int>(valueFromParm(a["failedModules"].toObject()));
        if (a.contains("maxDutyCycle"))          cfg.maxDutyCycle          = valueFromParm(a["maxDutyCycle"].toObject());
    }

    // --- Transmitter ---
    if (obj.contains("transmitter") && obj["transmitter"].isObject())
    {
        QJsonObject t = obj["transmitter"].toObject();
        if (t.contains("frequency_Hz"))      cfg.frequency_Hz      = valueFromParm(t["frequency_Hz"].toObject());
        if (t.contains("antennaGain"))       cfg.antennaGain       = valueFromParm(t["antennaGain"].toObject());
        if (t.contains("antennaBandwidth"))  cfg.antennaBandwidth  = valueFromParm(t["antennaBandwidth"].toObject());
        if (t.contains("beamWidth"))         cfg.beamWidth         = valueFromParm(t["beamWidth"].toObject());
        if (t.contains("sidelobeMode"))      cfg.sidelobeMode      = static_cast<aesa::SidelobeMode>(t["sidelobeMode"].toInt());
        if (t.contains("peakSidelobeLevel")) cfg.peakSidelobeLevel = valueFromParm(t["peakSidelobeLevel"].toObject());
        if (t.contains("avgSidelobeLevel"))  cfg.avgSidelobeLevel  = valueFromParm(t["avgSidelobeLevel"].toObject());
        if (t.contains("sidelobeBlanking"))  cfg.sidelobeBlanking_dB= valueFromParm(t["sidelobeBlanking"].toObject());
    }

    // --- Scan ---
    if (obj.contains("scan") && obj["scan"].isObject())
    {
        QJsonObject s = obj["scan"].toObject();
        if (s.contains("minAzimuth"))       cfg.minAzimuth             = valueFromParm(s["minAzimuth"].toObject());
        if (s.contains("maxAzimuth"))       cfg.maxAzimuth             = valueFromParm(s["maxAzimuth"].toObject());
        if (s.contains("minElevation"))     cfg.minElevation           = valueFromParm(s["minElevation"].toObject());
        if (s.contains("maxElevation"))     cfg.maxElevation           = valueFromParm(s["maxElevation"].toObject());
        if (s.contains("maxSteeringAngle")) cfg.maxSteeringAngle_deg   = valueFromParm(s["maxSteeringAngle"].toObject());
        if (s.contains("searchDwellTime"))  cfg.searchDwellTime_ms     = valueFromParm(s["searchDwellTime"].toObject());
        if (s.contains("trackDwellTime"))   cfg.trackDwellTime_ms      = valueFromParm(s["trackDwellTime"].toObject());
        if (s.contains("fcDwellTime"))      cfg.fireControlDwellTime_ms= valueFromParm(s["fcDwellTime"].toObject());
    }

    // --- Waveform ---
    auto readWF = [](const QJsonObject& o, aesa::BeamWaveform& wf) {
        if (o.isEmpty()) return;
        if (o.contains("modulation"))    wf.modulation    = static_cast<aesa::ModulationType>(o["modulation"].toInt());
        if (o.contains("pulseWidth_s"))  wf.pulseWidth_s  = static_cast<float>(o["pulseWidth_s"].toDouble());
        if (o.contains("prf_Hz"))        wf.prf_Hz        = static_cast<float>(o["prf_Hz"].toDouble());
        if (o.contains("prf2_Hz"))       wf.prf2_Hz       = static_cast<float>(o["prf2_Hz"].toDouble());  // ← ADD
        if (o.contains("bandwidth_Hz"))  wf.bandwidth_Hz  = static_cast<float>(o["bandwidth_Hz"].toDouble());
        if (o.contains("pulsesPerDwell"))wf.pulsesPerDwell= o["pulsesPerDwell"].toInt();
        if (o.contains("mode"))          wf.mode          = static_cast<aesa::WaveformMode>(o["mode"].toInt());
    };
    if (obj.contains("waveform") && obj["waveform"].isObject())
    {
        QJsonObject w = obj["waveform"].toObject();
        if (w.contains("frequencyAgility"))   cfg.frequencyAgility  = w["frequencyAgility"].toBool();
        if (w.contains("hopStartFrequency"))  cfg.hopStartFrequency = valueFromParm(w["hopStartFrequency"].toObject());
        if (w.contains("hopStopFrequency"))   cfg.hopStopFrequency  = valueFromParm(w["hopStopFrequency"].toObject());
        if (w.contains("searchWaveform"))     readWF(w["searchWaveform"].toObject(),      cfg.searchWaveform);
        if (w.contains("trackWaveform"))      readWF(w["trackWaveform"].toObject(),       cfg.trackWaveform);
        if (w.contains("fireControlWaveform"))readWF(w["fireControlWaveform"].toObject(), cfg.fireControlWaveform);
        if (w.contains("table")) {
            QJsonArray arr = w["table"].toArray();
            int idx = 0;
            for (const QJsonValue& v : arr) {
                if (idx >= 6) break;
                QJsonObject we = v.toObject();
                cfg.waveformTable[idx].maxRange_m = static_cast<float>(we.value("maxRange_m").toDouble(0.0));
                if (we.contains("waveform")) readWF(we["waveform"].toObject(), cfg.waveformTable[idx].waveform);
                ++idx;
            }
        }
    }

    // --- Detection ---
    if (obj.contains("detection") && obj["detection"].isObject())
    {
        QJsonObject d = obj["detection"].toObject();
        if (d.contains("systemTemperature_K")) cfg.systemTemperature_K = valueFromParm(d["systemTemperature_K"].toObject());
        if (d.contains("noiseFigure_dB"))      cfg.noiseFigure_dB      = valueFromParm(d["noiseFigure_dB"].toObject());
        if (d.contains("targetPfa"))           cfg.targetPfa           = valueFromParm(d["targetPfa"].toObject());
        if (d.contains("minDetectableRange"))  cfg.minDetectableRange  = valueFromParm(d["minDetectableRange"].toObject());
        if (d.contains("seaState"))            cfg.seaState            = valueFromParm(d["seaState"].toObject());
        if (d.contains("landClutter"))         cfg.landClutter         = valueFromParm(d["landClutter"].toObject());
        if (d.contains("targetCategory"))      cfg.targetCategory      = static_cast<aesa::DetectionCategory>(d["targetCategory"].toInt());
    }

    // --- Platform ---
    if (obj.contains("platform") && obj["platform"].isObject())
    {
        QJsonObject p = obj["platform"].toObject();
        if (p.contains("radarHeight"))        cfg.radarHeight        = valueFromParm(p["radarHeight"].toObject());
        if (p.contains("platformSpeed_m_s"))  cfg.platformSpeed_m_s  = valueFromParm(p["platformSpeed_m_s"].toObject());
        if (p.contains("minDetectableRange")) cfg.minDetectableRange = valueFromParm(p["minDetectableRange"].toObject());
    }

    // --- Tracking ---
    if (obj.contains("tracking") && obj["tracking"].isObject())
    {
        QJsonObject t = obj["tracking"].toObject();
        if (t.contains("missedScansToDrop"))      cfg.missedScansToDrop    = static_cast<int>(valueFromParm(t["missedScansToDrop"].toObject()));
        if (t.contains("trackCoastSeconds"))      cfg.trackCoastSeconds    = valueFromParm(t["trackCoastSeconds"].toObject());
        if (t.contains("minHitsToValidate"))      cfg.minHitsToValidate    = static_cast<int>(valueFromParm(t["minHitsToValidate"].toObject()));
        if (t.contains("maxTrackSpeed"))          cfg.maxTrackSpeed        = valueFromParm(t["maxTrackSpeed"].toObject());
        if (t.contains("manoeuvreThreshold"))     cfg.manoeuvreThreshold_m = valueFromParm(t["manoeuvreThreshold"].toObject());
        if (t.contains("useJPDA"))                cfg.useJPDA              = t["useJPDA"].toBool();
        if (t.contains("jpdaFalseAlarmDensity"))  cfg.jpdaFalseAlarmDensity= valueFromParm(t["jpdaFalseAlarmDensity"].toObject());
    }

    // --- Propagation ---
    if (obj.contains("propagation") && obj["propagation"].isObject())
    {
        QJsonObject p = obj["propagation"].toObject();
        if (p.contains("earthRadiusFactor")) cfg.earthRadiusFactor = valueFromParm(p["earthRadiusFactor"].toObject());
        if (p.contains("atmosphericFactor")) cfg.atmosphericFactor = valueFromParm(p["atmosphericFactor"].toObject());
        if (p.contains("temperature_C"))   cfg.atmosphere.temperature_C   = valueFromParm(p["temperature_C"].toObject());
        if (p.contains("humidity_pct"))    cfg.atmosphere.humidity_pct    = valueFromParm(p["humidity_pct"].toObject());
        if (p.contains("pressure_hPa"))    cfg.atmosphere.pressure_hPa    = valueFromParm(p["pressure_hPa"].toObject());
        if (p.contains("rainRate_mmph"))   cfg.atmosphere.rainRate_mmph   = valueFromParm(p["rainRate_mmph"].toObject());
        if (p.contains("fogVisibility_m")) cfg.atmosphere.fogVisibility_m = valueFromParm(p["fogVisibility_m"].toObject());
    }

    // --- Noise ---
    if (obj.contains("noise") && obj["noise"].isObject())
    {
        QJsonObject n = obj["noise"].toObject();
        if (n.contains("rangeStdDev"))     cfg.noise.rangeStdDev     = valueFromParm(n["rangeStdDev"].toObject());
        if (n.contains("azimuthStdDev"))   cfg.noise.azimuthStdDev   = valueFromParm(n["azimuthStdDev"].toObject());
        if (n.contains("elevationStdDev")) cfg.noise.elevationStdDev = valueFromParm(n["elevationStdDev"].toObject());
        if (n.contains("dopplerStdDev"))   cfg.noise.dopplerStdDev   = valueFromParm(n["dopplerStdDev"].toObject());
    }

    // --- IFF (FIX-04) ---
    if (obj.contains("iff") && obj["iff"].isObject())
    {
        QJsonObject f = obj["iff"].toObject();
        if (f.contains("interrogationMode")) cfg.interrogationMode = static_cast<aesa::IFFMode>(f["interrogationMode"].toInt());
        if (f.contains("friendlySquawks")) {
            cfg.friendlySquawks.clear();
            for (const QJsonValue& v : f["friendlySquawks"].toArray())
                cfg.friendlySquawks.push_back(static_cast<uint32_t>(v.toInt()));
        }
    }

    // --- Null steering ---
    if (obj.contains("nullSteering") && obj["nullSteering"].isObject())
    {
        QJsonObject n = obj["nullSteering"].toObject();
        if (n.contains("active"))        cfg.nullSteering.active        = n["active"].toBool();
        if (n.contains("azimuth_deg"))   cfg.nullSteering.azimuth_deg   = valueFromParm(n["azimuth_deg"].toObject());
        if (n.contains("elevation_deg")) cfg.nullSteering.elevation_deg = valueFromParm(n["elevation_deg"].toObject());
        if (n.contains("nullDepth_dB"))  cfg.nullSteering.nullDepth_dB  = valueFromParm(n["nullDepth_dB"].toObject());
    }

    // --- Mode ---
    if (obj.contains("mode"))
        cfg.mode = static_cast<aesa::RadarMode>(obj["mode"].toInt());

    radarCore_.setConfig(cfg);
    displayRangeDirty_ = true;
}
