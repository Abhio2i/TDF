// =============================================================================
// FILE:         aesaRadarInterface.h
// MODULE:       Plugin SDK — built-in AESA radar access and logic replacement
// PROJECT:      Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:     RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Interface a plugin implements to access and extend the
//               built-in AESA radar (customer requirement 5, REQ-AESA-090):
//                 - read radar list, configuration and live status;
//                 - change radar configuration (JSON, same format as
//                   AESARadar::toJson / fromJson; partial objects allowed);
//                 - replace simulation-logic stages: mean RCS, detection
//                   decision, propagation loss;
//                 - pre-filter (remove targets before the radar sees them)
//                   and post-filter (remove entries from the engine-visible
//                   target list).
//
// RULES FOR PLUGIN AUTHORS
//   1. Build with the same compiler, C++ standard library and Qt 5.15 as the
//      TDF application (std::function / std::string / QJsonObject cross the
//      library boundary).
//   2. Hook functions (LogicHooks) run on the simulation thread inside the
//      radar update, also during replay. They must be fast, must not block,
//      must not touch widgets, and must not call any host callback below:
//      host callbacks called from inside a hook are refused (empty / false).
//   3. A radar ID changes on every scenario load. Identify radars by
//      entityId + name if you need to recognise them across loads.
//   4. Positions are radar-centred, stabilised frame (x = North, y = East,
//      z = Up), metres. Velocities are absolute, m/s, same axes.
//      NOTE: this is the radar model frame, NOT the TDF engine frame
//      (engine: x = East, y = Up, z = North). Mapping:
//      plugin.x = engine.z, plugin.y = engine.x, plugin.z = engine.y.
//   5. Return false from a stage function to use the built-in logic for that
//      call. Invalid values (NaN, negative RCS, loss outside (0,1]) also fall
//      back to the built-in logic.
//   6. Only one plugin can replace a given stage. setLogicHooks() returns
//      false if another loaded plugin already replaces it.
//   7. Implement only this interface in the plugin library (the host matches
//      one interface per plugin).
// =============================================================================

#ifndef AESARADARINTERFACE_H
#define AESARADARINTERFACE_H

#include <QtPlugin>
#include <QJsonObject>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace aesaplugin {

// SDK version. Incremented whenever a struct below changes.
constexpr int API_VERSION = 1;

struct RadarInfo
{
    std::string radarId;    // runtime sensor ID (changes on scenario load)
    std::string entityId;   // owning platform entity ID
    std::string name;       // radar name in the scenario
};

struct RadarStatus
{
    bool   valid             = false;
    int    mode              = 0;     // 0 = SURVEILLANCE, 1 = TWS, 2 = LOCK_ON
    double beamAzimuth_deg   = 0.0;   // body frame
    double beamElevation_deg = 0.0;   // body frame
    double dutyCycle         = 0.0;   // 0..1
    double displayRange_km   = 0.0;
};

struct TargetGeometry
{
    uint32_t    targetId = 0;    // radar-internal target ID
    std::string targetEntityId;  // platform entity ID (empty if unknown)
    double x  = 0.0, y  = 0.0, z  = 0.0;   // m, stabilised N/E/U, radar-centred
    double vx = 0.0, vy = 0.0, vz = 0.0;   // m/s, absolute, N/E/U
    double range_m = 0.0;
};

// ---- Stage: mean RCS --------------------------------------------------------
struct RcsQuery
{
    RadarInfo      radar;
    TargetGeometry target;
    std::string    platformType;          // "FIGHTER", ..., "GENERIC"
    double length_m = 0.0, width_m = 0.0, height_m = 0.0;
    bool   dimensionsValid = false;
    double frequency_Hz    = 0.0;
};
// Set meanRcs_m2 (finite, >= 0) and return true to replace the built-in MEAN
// RCS. The radar still applies the target's Swerling fluctuation.
using RcsFn = std::function<bool(const RcsQuery&, double& meanRcs_m2)>;

// ---- Stage: detection decision ---------------------------------------------
struct DetectionQuery
{
    RadarInfo      radar;
    TargetGeometry target;
    double sinr               = 0.0;    // linear
    double cfarThreshold      = 0.0;    // linear, built-in threshold
    double radialVelocity_m_s = 0.0;    // relative, negative = closing
    int    mode               = 0;      // as RadarStatus::mode
    bool   builtInDetected    = false;  // sinr > cfarThreshold
};
using DetectionFn = std::function<bool(const DetectionQuery&, bool& detected)>;

// ---- Stage: propagation loss -----------------------------------------------
// Used for target returns, chaff returns and the display-range estimate.
struct PropagationQuery
{
    RadarInfo radar;
    double range_m         = 0.0;       // one-way path length
    double frequency_Hz    = 0.0;
    double temperature_C   = 15.0;
    double humidity_pct    = 60.0;
    double pressure_hPa    = 1013.25;
    double rainRate_mmph   = 0.0;
    double fogVisibility_m = 0.0;
};
// lossFactor: two-way linear power factor in (0, 1]; 1 = no loss.
using PropagationFn = std::function<bool(const PropagationQuery&, double& lossFactor)>;

// ---- Filters ----------------------------------------------------------------
// Pre-filter: return false to remove the target from this radar for this tick
// (the radar does not see it at all).
using TargetFilterFn = std::function<bool(const RadarInfo&, const TargetGeometry&)>;

struct OutputTarget
{
    std::string targetEntityId;     // empty if unknown
    double range_km           = 0.0;
    double bearingBody_deg    = 0.0;
    double trueBearing_deg    = 0.0;
    double speed_m_s          = 0.0;
    double heading_deg        = 0.0;
    double altitude_m         = 0.0;
    double radialVelocity_m_s = 0.0;
};
// Post-filter: return false to remove the entry from the target list the rest
// of the engine sees (display, weapons, decision logic). The radar's internal
// tracks and DIS emission are not affected.
using OutputFilterFn = std::function<bool(const RadarInfo&, const OutputTarget&)>;

struct LogicHooks
{
    RcsFn          rcs;          // empty = built-in
    DetectionFn    detection;    // empty = built-in
    PropagationFn  propagation;  // empty = built-in
    TargetFilterFn preFilter;    // empty = keep all
    OutputFilterFn postFilter;   // empty = keep all
};

} // namespace aesaplugin

// =============================================================================
// CLASS: AesaRadarPluginInterface
// =============================================================================
class AesaRadarPluginInterface
{
public:
    virtual ~AesaRadarPluginInterface() = default;

    // SDK version the plugin was built against. Do not override.
    virtual int apiVersion() const { return aesaplugin::API_VERSION; }

    // Called once after all host callbacks below are set.
    virtual void init() = 0;

    // Called every rendered frame (Simulation::Render).
    virtual void update(float delta) = 0;

    // Called for every existing radar at load, then for each new radar.
    virtual void radarAdded(const aesaplugin::RadarInfo& info) { (void)info; }

    // Called when a radar announced by radarAdded() is removed.
    virtual void radarRemoved(const std::string& radarId) { (void)radarId; }

    // Called before the plugin library is unloaded. The host removes this
    // plugin's hooks right after this call.
    virtual void shutdown() = 0;

    // ---- Host callbacks (set by the host before init()) ---------------------
    using ListRadarsFn = std::function<std::vector<aesaplugin::RadarInfo>()>;
    using GetConfigFn  = std::function<QJsonObject(const std::string& radarId)>;
    using SetConfigFn  = std::function<bool(const std::string& radarId, const QJsonObject& partialConfig)>;
    using GetStatusFn  = std::function<aesaplugin::RadarStatus(const std::string& radarId)>;
    using SetHooksFn   = std::function<bool(const aesaplugin::LogicHooks& hooks)>;
    using ClearHooksFn = std::function<void()>;

    ListRadarsFn listRadars      = nullptr;
    GetConfigFn  getRadarConfig  = nullptr;
    SetConfigFn  setRadarConfig  = nullptr;
    GetStatusFn  getRadarStatus  = nullptr;
    SetHooksFn   setLogicHooks   = nullptr;   // replaces this plugin's hooks
    ClearHooksFn clearLogicHooks = nullptr;

    void registerListRadarsCallback(ListRadarsFn cb)     { listRadars = cb; }
    void registerGetConfigCallback(GetConfigFn cb)       { getRadarConfig = cb; }
    void registerSetConfigCallback(SetConfigFn cb)       { setRadarConfig = cb; }
    void registerGetStatusCallback(GetStatusFn cb)       { getRadarStatus = cb; }
    void registerSetLogicHooksCallback(SetHooksFn cb)    { setLogicHooks = cb; }
    void registerClearLogicHooksCallback(ClearHooksFn cb){ clearLogicHooks = cb; }
};

#define AesaRadarPluginInterface_iid "com.n2i.AesaRadarPluginInterface/1.0"
Q_DECLARE_INTERFACE(AesaRadarPluginInterface, AesaRadarPluginInterface_iid)

#endif // AESARADARINTERFACE_H
