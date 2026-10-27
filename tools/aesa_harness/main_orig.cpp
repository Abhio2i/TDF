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

int main(int argc, char** argv)
{
    const int scenario = (argc > 1) ? std::atoi(argv[1]) : 1;

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
