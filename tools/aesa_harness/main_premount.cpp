// AESA regression harness v3. Scenarios:
//  1-4: own velocity zero, level, North -> MUST be byte-identical 2a vs 2b
//  5:   own 250 m/s North (constant)   -> track absolute velocity check
//  6:   own 250 m/s, turning 0 -> 45 deg at 0.75 deg/s -> track heading check
#include "radarmodel_aesa.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

extern "C" void aesa_test_seed_model(unsigned seed);
extern "C" void aesa_test_seed_signal(unsigned seed);

using namespace aesa;

static constexpr double PI_ = 3.14159265358979323846;
static constexpr double D2R = PI_ / 180.0;

static RadarConfig makeConfig(int scenario)
{
    RadarConfig c;
    c.numElements           = 1000;
    c.peakPowerPerElement_W = 100.0f;
    c.moduleEfficiency      = 0.70f;
    c.maxDutyCycle          = 0.50f;
    c.frequency_Hz          = 8.0e9;
    c.antennaGain           = 35.0f;
    c.antennaBandwidth      = 1e6;
    c.beamWidth             = 3.0f;
    c.maxSteeringAngle_deg  = 60.0f;
    c.minAzimuth   = -60.0f;  c.maxAzimuth   = 60.0f;
    c.minElevation =  -2.0f;  c.maxElevation = 15.0f;
    c.noiseFigure_dB      = 5.0;
    c.radarHeight         = 20.0;
    c.minDetectableRange  = 30.0;
    c.targetCategory      = DetectionCategory::ALL;
    c.missedScansToDrop   = 2;
    c.trackCoastSeconds   = 8.0;
    c.minHitsToValidate   = 2;
    c.maxTrackSpeed       = 2000.0;
    c.useJPDA             = (scenario == 2);
    c.mode                = RadarMode::TWS;
    return c;
}

static TargetInput makeTarget(uint32_t id, double azDeg, double elDeg, double r)
{
    TargetInput t;
    t.id = id;
    const double az = azDeg * D2R, el = elDeg * D2R;
    t.x = r * std::cos(el) * std::cos(az);
    t.y = r * std::cos(el) * std::sin(az);
    t.z = r * std::sin(el);
    t.surface      = SurfaceType::AIR;
    t.swerlingCase = SwerlingCase::CASE_I;
    t.platformType = "FIGHTER";
    return t;
}

static void addOccluded(std::vector<TargetInput>& v, size_t idx, uint32_t id)
{
    TargetInput t = v[idx];
    t.id = id;
    const double r = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
    const double k = (r + 5000.0) / r;
    t.x *= k; t.y *= k; t.z *= k;
    v.push_back(t);
}

static std::vector<TargetInput> makeTargets(int scenario)
{
    std::vector<TargetInput> v;
    if (scenario == 4)
    {
        for (int i = 0; i < 20; ++i)
        {
            TargetInput t = makeTarget(1000u + static_cast<uint32_t>(i),
                                       -50.0 + 100.0 * i / 19.0,
                                       8.0 + (i % 4) * 2.0,
                                       30000.0 + 5000.0 * i);
            const double spd = 50.0 + 2.5 * i;
            const double h   = ((i * 37) % 360) * D2R;
            t.vx = -spd * std::cos(h);
            t.vy =  spd * std::sin(h);
            t.platformType = (i % 2 == 0) ? "FIGHTER" : "GENERIC";
            if (i % 3 == 0)
            {
                t.dimensions.length = 15.0; t.dimensions.height = 5.0;
                t.dimensions.width  = 10.0; t.dimensions.valid  = true;
            }
            v.push_back(t);
        }
        addOccluded(v, 3, 1020u);
        return v;
    }
    if (scenario == 5)
    {
        TargetInput a = makeTarget(2001u, -10.0, 8.0, 40000.0);
        TargetInput b = makeTarget(2002u,   0.0, 8.0, 45000.0);
        const double rb = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
        b.vx = 50.0 * b.x / rb; b.vy = 50.0 * b.y / rb; b.vz = 50.0 * b.z / rb;
        TargetInput c = makeTarget(2003u,  10.0, 8.0, 50000.0);
        const double ca = 10.0 * D2R;
        c.vx = -150.0 * std::sin(ca); c.vy = 150.0 * std::cos(ca);
        v.push_back(a); v.push_back(b); v.push_back(c);
        return v;
    }
    if (scenario == 6)
    {
        // 3001: flying East 200 m/s (true heading 90).
        // 3002: flying North 150 m/s (true heading 0).
        // 3003: stationary.
        TargetInput a = makeTarget(3001u,  20.0, 6.0, 50000.0); a.vy = 200.0;
        TargetInput b = makeTarget(3002u,  40.0, 6.0, 60000.0); b.vx = 150.0;
        TargetInput c = makeTarget(3003u, -20.0, 6.0, 45000.0);
        v.push_back(a); v.push_back(b); v.push_back(c);
        return v;
    }
    // Scenarios 1-3: unchanged.
    for (int i = 0; i < 20; ++i)
    {
        TargetInput t = makeTarget(1000u + static_cast<uint32_t>(i),
                                   -50.0 + 100.0 * i / 19.0,
                                   0.5 + (i % 5) * 1.5,
                                   15000.0 + 5500.0 * i);
        const double spd = 150.0 + 10.0 * i;
        const double h   = ((i * 37) % 360) * D2R;
        t.vx = -spd * std::cos(h);
        t.vy =  spd * std::sin(h);
        t.platformType = (i % 2 == 0) ? "FIGHTER" : "GENERIC";
        if (i % 3 == 0)
        {
            t.dimensions.length = 15.0; t.dimensions.height = 5.0;
            t.dimensions.width  = 10.0; t.dimensions.valid  = true;
        }
        v.push_back(t);
    }
    addOccluded(v, 3, 1020u);
    if (scenario == 3)
    {
        JammerConfig& j = v[7].jammer;
        j.active = true; j.type = JammerType::NOISE; j.power_kW = 0.1;
        j.gain_dBi = 10.0; j.bandwidth_Hz = 1e6; j.selfScreening = true;
    }
    return v;
}

#ifdef HARNESS_P1PLUGIN
// =============================================================================
// Scenario 7 (local only, never committed): AESA plugin SDK hook wiring.
// The glue below marked VERBATIM is copied unmodified from
// AESA_API_P1_TASK.md (P1-3). Everything else is harness scaffolding.
// =============================================================================
#include "core/Plugins/aesalogicregistry.h"
#include <QDebug>
#include <QVector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

// Stand-ins for the TDF engine types the copied glue refers to.
struct Platform { std::string ID; };
struct Target
{
    Platform* entity = nullptr;
    float radius = 0.0f, angle = 0.0f, speed = 0.0f, direction = 0.0f, altitude = 0.0f;
    float radialVelocity = 0.0f;
    float trueBearing    = 0.0f;
};
struct HarnessEntity { std::string ID; };

// ==== BEGIN VERBATIM COPY: AESA_API_P1_TASK.md lines 926-1042 (P1-3 3.2) ====
namespace
{
struct AesaStageContext
{
    const AesaLogicSnapshot*                       hooks = nullptr;
    const std::unordered_map<uint32_t, Platform*>* idMap = nullptr;
    aesaplugin::RadarInfo                          radar;
};

int aesaModeToInt(aesa::RadarMode m)
{
    switch (m)
    {
    case aesa::RadarMode::SURVEILLANCE: return 0;
    case aesa::RadarMode::TWS:          return 1;
    case aesa::RadarMode::LOCK_ON:      return 2;
    }
    return 0;
}

aesaplugin::TargetGeometry stageGeometry(const AesaStageContext& c,
                                         const aesa::TargetInput& t,
                                         double range_m)
{
    aesaplugin::TargetGeometry g;
    g.targetId = t.id;
    if (c.idMap != nullptr)
    {
        auto it = c.idMap->find(t.id);
        if (it != c.idMap->end() && it->second != nullptr)
            g.targetEntityId = it->second->ID;
    }
    g.x = t.x;   g.y = t.y;   g.z = t.z;
    g.vx = t.vx; g.vy = t.vy; g.vz = t.vz;
    g.range_m = range_m;
    return g;
}

bool stageMeanRcs(void* ctx, const aesa::TargetInput& t, double range_m,
                  double frequency_Hz, double& meanRcs_m2)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->rcs) return false;

    aesaplugin::RcsQuery q;
    q.radar           = c->radar;
    q.target          = stageGeometry(*c, t, range_m);
    q.platformType    = t.platformType;
    q.length_m        = t.dimensions.length;
    q.width_m         = t.dimensions.width;
    q.height_m        = t.dimensions.height;
    q.dimensionsValid = t.dimensions.valid;
    q.frequency_Hz    = frequency_Hz;

    bool ok = false;
    {
        AesaLogicRegistry::StageGuard guard;
        try { ok = c->hooks->rcs(q, meanRcs_m2); } catch (...) { ok = false; }
    }
    if (ok && !(std::isfinite(meanRcs_m2) && meanRcs_m2 >= 0.0))
    {
        static bool warned = false;
        if (!warned) { warned = true; qWarning() << "[AESA plugin] invalid RCS value; built-in RCS used"; }
        return false;
    }
    return ok;
}

bool stageDetect(void* ctx, const aesa::TargetInput& t, double range_m,
                 const aesa::DetectionStageInput& in, bool& detected)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->detection) return false;

    aesaplugin::DetectionQuery q;
    q.radar              = c->radar;
    q.target             = stageGeometry(*c, t, range_m);
    q.sinr               = in.sinr;
    q.cfarThreshold      = in.cfarThreshold;
    q.radialVelocity_m_s = in.radialVelocity_m_s;
    q.mode               = aesaModeToInt(in.mode);
    q.builtInDetected    = in.builtInDetected;

    AesaLogicRegistry::StageGuard guard;
    try { return c->hooks->detection(q, detected); } catch (...) { return false; }
}

bool stagePropagation(void* ctx, double range_m, const aesa::RadarConfig& cfg,
                      double& lossFactor)
{
    const auto* c = static_cast<const AesaStageContext*>(ctx);
    if (c == nullptr || c->hooks == nullptr || !c->hooks->propagation) return false;

    aesaplugin::PropagationQuery q;
    q.radar           = c->radar;
    q.range_m         = range_m;
    q.frequency_Hz    = cfg.frequency_Hz;
    q.temperature_C   = cfg.atmosphere.temperature_C;
    q.humidity_pct    = cfg.atmosphere.humidity_pct;
    q.pressure_hPa    = cfg.atmosphere.pressure_hPa;
    q.rainRate_mmph   = cfg.atmosphere.rainRate_mmph;
    q.fogVisibility_m = cfg.atmosphere.fogVisibility_m;

    bool ok = false;
    {
        AesaLogicRegistry::StageGuard guard;
        try { ok = c->hooks->propagation(q, lossFactor); } catch (...) { ok = false; }
    }
    if (ok && !(std::isfinite(lossFactor) && lossFactor > 0.0 && lossFactor <= 1.0))
    {
        static bool warned = false;
        if (!warned) { warned = true; qWarning() << "[AESA plugin] invalid propagation loss; built-in model used"; }
        return false;
    }
    return ok;
}
} // namespace
// ==== END VERBATIM COPY: lines 926-1042 ====

struct ScanRun
{
    long totalDets = 0;
    long totalTrks = 0;
    long ticksModelOutput   = 0;   // ticks with dets+trks > 0
    long ticksTargetsFilled = 0;   // ticks with targets non-empty after post-filter
};

// One full scenario-1 run (same config, targets, pose, speed and lock
// schedule as scenarios 1-4 in main()) through the copied scan() glue.
// Reseeds both RNGs and builds a fresh model every call.
static ScanRun runScan(FILE* log)
{
    aesa_test_seed_model(12345u);
    aesa_test_seed_signal(12345u);

    ScanRun res;
    RadarModel_AESA model;
    model.init(makeConfig(1));
    model.start();
    model.reset();
    RadarModel_AESA& radarCore_ = model;

    std::vector<TargetInput> tg = makeTargets(1);
    const double dt = 0.05;
    const uint32_t lockId = 1005u;
    double simTime = 0.0;

    // Identities used by the copied glue.
    const std::string ID   = "harness-radar";
    const std::string Name = "AESA-H";
    HarnessEntity ownEntity; ownEntity.ID = "harness-platform";
    HarnessEntity* parentEntity = &ownEntity;
    std::vector<Platform> platforms(tg.size());
    std::unordered_map<uint32_t, Platform*> idMap;
    for (size_t i = 0; i < tg.size(); ++i)
    {
        platforms[i].ID = "entity-" + std::to_string(tg[i].id);
        idMap[tg[i].id] = &platforms[i];
    }
    QVector<Target> targets;

    for (int tick = 0; tick < 1200; ++tick)
    {
        simTime += dt;
        const double ovx = 0.0, ovy = 0.0;
        for (auto& t : tg)
        {
            t.x += (t.vx - ovx) * dt;
            t.y += (t.vy - ovy) * dt;
            t.z += t.vz * dt;
        }

        RadarPose pose;
        pose.y = 2000.0 + 100.0 * std::sin(0.01 * tick);
#ifdef HARNESS_2A
        pose.vx = ovx;
        pose.vy = ovy;
#endif
        const float kmh = 720.0f + 36.0f * std::sin(0.1f * static_cast<float>(tick));
        const float spd = static_cast<float>(kmh / 3.6);
        model.setPlatformSpeed(spd);

        if (tick == 600)  model.lockOn(lockId);
        if (tick == 900)  model.breakLock();
        if (tick == 1000) { RadarConfig c = model.getConfig(); c.mode = RadarMode::TWS; model.setConfig(c); }

        std::vector<TargetInput> radarInputs = tg;
        const double simTime_ = simTime;

// ==== BEGIN VERBATIM COPY: AESA_API_P1_TASK.md lines 1064-1118 (P1-3 3.3) ====
    // ---- Plugin logic (AESA plugin SDK). REQ-AESA-090. ----------------------
    // One snapshot per scan. Empty registry = nothing below runs and the
    // model uses its built-in logic only.
    const std::shared_ptr<const AesaLogicSnapshot> pluginHooks =
        AesaLogicRegistry::instance().snapshot();
    AesaStageContext stageCtx;
    if (pluginHooks && !pluginHooks->empty())
    {
        stageCtx.hooks          = pluginHooks.get();
        stageCtx.idMap          = &idMap;
        stageCtx.radar.radarId  = ID;
        stageCtx.radar.entityId = parentEntity->ID;
        stageCtx.radar.name     = Name;
    }

    // Pre-filter: plugins may remove targets before the radar sees them.
    if (stageCtx.hooks != nullptr && !stageCtx.hooks->preFilters.empty())
    {
        radarInputs.erase(
            std::remove_if(radarInputs.begin(), radarInputs.end(),
                [&stageCtx](const aesa::TargetInput& t)
                {
                    const double r = std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z);
                    const aesaplugin::TargetGeometry g = stageGeometry(stageCtx, t, r);
                    for (const auto& keepFn : stageCtx.hooks->preFilters)
                    {
                        bool keep = true;
                        {
                            AesaLogicRegistry::StageGuard guard;
                            try { keep = keepFn(stageCtx.radar, g); } catch (...) { keep = true; }
                        }
                        if (!keep) return true;   // remove
                    }
                    return false;
                }),
            radarInputs.end());
    }

    // Replaceable stages: installed for this update() only.
    const bool pluginStages = (stageCtx.hooks != nullptr && stageCtx.hooks->hasStages());
    if (pluginStages)
    {
        aesa::LogicOverrides ov;
        ov.context = &stageCtx;
        if (stageCtx.hooks->rcs)         ov.meanRcs         = &stageMeanRcs;
        if (stageCtx.hooks->detection)   ov.detect          = &stageDetect;
        if (stageCtx.hooks->propagation) ov.propagationLoss = &stagePropagation;
        radarCore_.setLogicOverrides(ov);
    }

    radarCore_.update(dt, pose, radarInputs, simTime_);

    // stageCtx lives on this stack frame: clear before it goes out of scope.
    if (pluginStages)
        radarCore_.setLogicOverrides(aesa::LogicOverrides{});
// ==== END VERBATIM COPY: lines 1064-1118 ====

        const RadarOutput o = model.getOutput();

        // Harness stand-in for processSurveillance/processTWS/processLockOn:
        // one engine-visible entry per detection.
        targets.clear();
        for (const auto& d : o.detections)
        {
            Target t;
            auto it = idMap.find(d.targetID);
            t.entity         = (it != idMap.end()) ? it->second : nullptr;
            t.radius         = static_cast<float>(d.range / 1000.0);
            t.angle          = static_cast<float>(d.azimuth);
            t.radialVelocity = static_cast<float>(d.radialVelocity);
            targets.append(t);
        }

// ==== BEGIN VERBATIM COPY: AESA_API_P1_TASK.md lines 1146-1174 (P1-3 3.4) ====
    if (stageCtx.hooks != nullptr && !stageCtx.hooks->postFilters.empty())
    {
        decltype(targets) kept;
        kept.reserve(targets.size());
        for (const auto& t : targets)
        {
            aesaplugin::OutputTarget o;
            o.targetEntityId     = (t.entity != nullptr) ? t.entity->ID : std::string();
            o.range_km           = t.radius;
            o.bearingBody_deg    = t.angle;
            o.trueBearing_deg    = t.trueBearing;
            o.speed_m_s          = t.speed;
            o.heading_deg        = t.direction;
            o.altitude_m         = t.altitude;
            o.radialVelocity_m_s = t.radialVelocity;

            bool keep = true;
            for (const auto& keepFn : stageCtx.hooks->postFilters)
            {
                {
                    AesaLogicRegistry::StageGuard guard;
                    try { keep = keepFn(stageCtx.radar, o); } catch (...) { keep = true; }
                }
                if (!keep) break;
            }
            if (keep) kept.append(t);
        }
        targets = kept;
    }
// ==== END VERBATIM COPY: lines 1146-1174 ====

        res.totalDets += static_cast<long>(o.detections.size());
        res.totalTrks += static_cast<long>(o.tracks.size());
        if (!o.detections.empty() || !o.tracks.empty()) ++res.ticksModelOutput;
        if (!targets.isEmpty()) ++res.ticksTargetsFilled;

        if (log != nullptr)
        {
            std::fprintf(log, "TICK %d mode=%d lockBroken=%d az=%.17g el=%.17g dr=%.17g duty=%.17g task=%d dets=%zu trks=%zu\n",
                        tick, static_cast<int>(o.mode), static_cast<int>(o.lockBroken),
                        o.currentAzimuth, o.currentElevation, o.displayRange_km,
                        o.currentDutyCycle, static_cast<int>(o.currentTask),
                        o.detections.size(), o.tracks.size());
            for (const auto& d : o.detections)
                std::fprintf(log, "  D %u %.17g %.17g %.17g %.17g %.17g %d %d %.17g\n",
                            d.targetID, d.range, d.azimuth, d.elevation, d.snr,
                            d.radialVelocity, static_cast<int>(d.isAmbiguous),
                            static_cast<int>(d.isDRFMGhost), d.Pk);
            for (const auto& t : o.tracks)
                std::fprintf(log, "  T %u %.17g %.17g %.17g %.17g %.17g %.17g %d %d\n",
                            t.id, t.x, t.y, t.z, t.vx, t.vy, t.vz,
                            static_cast<int>(t.isValidated), t.hitCount);
        }
    }
    return res;
}

// Host-style callback: refuses (as the P1-4 host callbacks do) while a
// plugin hook is running on this thread.
static long g_hostCalls   = 0;
static long g_hostRefused = 0;
static bool hostStyleCallback()
{
    ++g_hostCalls;
    if (AesaLogicRegistry::inStage()) { ++g_hostRefused; return false; }
    return true;
}

static int g_fail = 0;
static void report(const char* name, bool pass, const std::string& detail)
{
    std::printf("CHECK %-28s %s  %s\n", name, pass ? "PASS" : "FAIL", detail.c_str());
    if (!pass) ++g_fail;
}

static std::string counts(const ScanRun& r)
{
    return "dets=" + std::to_string(r.totalDets) + " trks=" + std::to_string(r.totalTrks) +
           " modelOutTicks=" + std::to_string(r.ticksModelOutput) +
           " targetsTicks=" + std::to_string(r.ticksTargetsFilled);
}

static int runScenario7(const char* gLogPath)
{
    AesaLogicRegistry& reg = AesaLogicRegistry::instance();

    // Built-in baseline (empty registry).
    const ScanRun base = runScan(nullptr);
    std::printf("BASE (built-in, empty registry): %s\n", counts(base).c_str());

    // a: detection hook -> never detected.
    {
        aesaplugin::LogicHooks h;
        h.detection = [](const aesaplugin::DetectionQuery&, bool& detected) { detected = false; return true; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("a detection=false", ok && r.totalDets == 0, counts(r));
    }
    // b1: RCS hook mean 0.0.
    {
        aesaplugin::LogicHooks h;
        h.rcs = [](const aesaplugin::RcsQuery&, double& m) { m = 0.0; return true; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("b1 rcs mean=0", ok && r.totalDets == 0, counts(r));
    }
    // b2: RCS hook mean 1e6.
    {
        aesaplugin::LogicHooks h;
        h.rcs = [](const aesaplugin::RcsQuery&, double& m) { m = 1e6; return true; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("b2 rcs mean=1e6", ok && r.totalDets >= base.totalDets,
               counts(r) + " (base dets=" + std::to_string(base.totalDets) + ")");
    }
    // c: propagation hook 1e-12.
    {
        aesaplugin::LogicHooks h;
        h.propagation = [](const aesaplugin::PropagationQuery&, double& f) { f = 1e-12; return true; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("c propagation=1e-12", ok && r.totalDets == 0, counts(r));
    }
    // d: pre-filter removes everything.
    {
        aesaplugin::LogicHooks h;
        h.preFilter = [](const aesaplugin::RadarInfo&, const aesaplugin::TargetGeometry&) { return false; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("d preFilter=false", ok && r.totalDets == 0, counts(r));
    }
    // e: post-filter removes everything from targets; model unaffected.
    {
        aesaplugin::LogicHooks h;
        h.postFilter = [](const aesaplugin::RadarInfo&, const aesaplugin::OutputTarget&) { return false; };
        const bool ok = reg.setHooks("harness", h);
        const ScanRun r = runScan(nullptr);
        report("e postFilter=false",
               ok && base.ticksTargetsFilled > 0 && r.ticksTargetsFilled == 0 &&
               r.ticksModelOutput > 0 && r.totalDets > 0 && r.totalTrks > 0,
               counts(r) + " (base targetsTicks=" + std::to_string(base.ticksTargetsFilled) + ")");
    }
    // f: host-style callback inside a hook sees inStage() == true.
    {
        g_hostCalls = 0; g_hostRefused = 0;
        const bool outsideBefore = !AesaLogicRegistry::inStage() && hostStyleCallback();
        aesaplugin::LogicHooks h;
        h.detection = [](const aesaplugin::DetectionQuery&, bool&) { hostStyleCallback(); return false; };
        const bool ok = reg.setHooks("harness", h);
        g_hostCalls = 0; g_hostRefused = 0;
        const ScanRun r = runScan(nullptr);
        const long inHookCalls = g_hostCalls, inHookRefused = g_hostRefused;
        const bool outsideAfter = !AesaLogicRegistry::inStage() && hostStyleCallback();
        report("f inStage inside hook",
               ok && outsideBefore && outsideAfter && inHookCalls > 0 && inHookRefused == inHookCalls,
               "hookCalls=" + std::to_string(inHookCalls) + " refused=" + std::to_string(inHookRefused) +
               " outsideOK=" + std::to_string(outsideBefore && outsideAfter));
        std::printf("INFO f: detection hook returning false (built-in used): %s; same as base: %s\n",
                    counts(r).c_str(),
                    (r.totalDets == base.totalDets && r.totalTrks == base.totalTrks) ? "yes" : "no");
    }
    // g: removeOwner -> output identical to scenario 1 (checked by cmp with cd2_A_1.txt).
    {
        reg.removeOwner("harness");
        const bool emptyReg = reg.snapshot()->empty();
        FILE* f = std::fopen(gLogPath, "w");
        if (f == nullptr) { report("g removeOwner", false, "cannot open log"); return 1; }
        const ScanRun r = runScan(f);
        std::fclose(f);
        report("g removeOwner (registry)", emptyReg, counts(r) + " log=" + gLogPath +
               " (byte-compare vs scenario 1 log done outside)");
    }

    std::printf("SCENARIO7 %s (%d failed)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    return g_fail == 0 ? 0 : 2;
}
#endif // HARNESS_P1PLUGIN

int main(int argc, char** argv)
{
    const int scenario = (argc > 1) ? std::atoi(argv[1]) : 1;
#ifdef HARNESS_P1PLUGIN
    if (scenario == 7) return runScenario7((argc > 2) ? argv[2] : "p1_7g.txt");
#endif

    aesa_test_seed_model(12345u);
    aesa_test_seed_signal(12345u);

    RadarModel_AESA model;
    model.init(makeConfig(scenario));
    model.start();
    model.reset();

    std::vector<TargetInput> tg = makeTargets(scenario);
    const double dt = 0.05;
    const uint32_t lockId = (scenario == 5) ? 2002u : (scenario == 6) ? 3001u : 1005u;
    double simTime = 0.0;

    for (int tick = 0; tick < 1200; ++tick)
    {
        simTime += dt;

        // Own motion (stabilised frame): heading psi, speed 250 m/s.
        double psiDeg = 0.0, ownSpd = 0.0;
        if (scenario == 5) { ownSpd = 250.0; }
        if (scenario == 6) { ownSpd = 250.0; psiDeg = 0.75 * simTime; }
        const double psi = psiDeg * D2R;
        const double ovx = ownSpd * std::cos(psi);
        const double ovy = ownSpd * std::sin(psi);

        // Relative positions: target motion minus own motion.
        for (auto& t : tg)
        {
            t.x += (t.vx - ovx) * dt;
            t.y += (t.vy - ovy) * dt;
            t.z += t.vz * dt;
        }

        RadarPose pose;
        pose.y = 2000.0 + 100.0 * std::sin(0.01 * tick);
#ifdef HARNESS_2A
        pose.vx = ovx;
        pose.vy = ovy;
        if (scenario == 6)
        {
            // Level, heading psi: rows = forward, right, up in (N, E, U).
            const double c = std::cos(psi), s = std::sin(psi);
            const double R[3][3] = { {  c,   s,  0.0 },
                                     { -s,   c,  0.0 },
                                     { 0.0, 0.0, 1.0 } };
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) pose.bodyFromStab[i][j] = R[i][j];
        }
#endif

        float kmh;
        if      (scenario == 4)                  kmh = 0.0f;
        else if (scenario == 5 || scenario == 6) kmh = 900.0f;   // 250 m/s
        else kmh = 720.0f + 36.0f * std::sin(0.1f * static_cast<float>(tick));
        const float spd = static_cast<float>(kmh / 3.6);
        model.setPlatformSpeed(spd);

        if (tick == 600)  model.lockOn(lockId);
        if (tick == 900)  model.breakLock();
        if (tick == 1000) { RadarConfig c = model.getConfig(); c.mode = RadarMode::TWS; model.setConfig(c); }

        model.update(dt, pose, tg, simTime);
        const RadarOutput o = model.getOutput();

        std::printf("TICK %d mode=%d lockBroken=%d az=%.17g el=%.17g dr=%.17g duty=%.17g task=%d dets=%zu trks=%zu\n",
                    tick, static_cast<int>(o.mode), static_cast<int>(o.lockBroken),
                    o.currentAzimuth, o.currentElevation, o.displayRange_km,
                    o.currentDutyCycle, static_cast<int>(o.currentTask),
                    o.detections.size(), o.tracks.size());
        for (const auto& d : o.detections)
            std::printf("  D %u %.17g %.17g %.17g %.17g %.17g %d %d %.17g\n",
                        d.targetID, d.range, d.azimuth, d.elevation, d.snr,
                        d.radialVelocity, static_cast<int>(d.isAmbiguous),
                        static_cast<int>(d.isDRFMGhost), d.Pk);
        for (const auto& t : o.tracks)
            std::printf("  T %u %.17g %.17g %.17g %.17g %.17g %.17g %d %d\n",
                        t.id, t.x, t.y, t.z, t.vx, t.vy, t.vz,
                        static_cast<int>(t.isValidated), t.hitCount);
    }
    return 0;
}
