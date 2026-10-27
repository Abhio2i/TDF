# AESA Radar Plugin SDK — Guide and Example

The AESA plugin SDK lets a plugin access and extend the built-in AESA radar
(customer requirement 5): read and change radar parameters, observe radar
results, and replace parts of the simulation logic.

Interface header: `core/Plugins/Interfaces/aesaRadarInterface.h`
(SDK version `aesaplugin::API_VERSION`, currently 1).

## What a plugin can do

| Capability | How |
|---|---|
| List radars | `listRadars()` → radarId, entityId, name |
| Read live status | `getRadarStatus(radarId)` → mode, beam az/el, duty cycle, display range |
| Read parameters | `getRadarConfig(radarId)` → JSON (same as scenario save) |
| Change parameters | `setRadarConfig(radarId, partialJson)` → only the keys given change |
| Replace mean RCS | `LogicHooks::rcs` (Swerling fluctuation still applied) |
| Replace detection decision | `LogicHooks::detection` (gets SINR, built-in CFAR threshold, built-in result) |
| Replace propagation loss | `LogicHooks::propagation` (two-way factor in (0, 1]) |
| Remove targets before the radar | `LogicHooks::preFilter` |
| Remove entries from the engine-visible target list | `LogicHooks::postFilter` |
| Be told about radars | `radarAdded()` / `radarRemoved()` |

## Rules

1. Build with the same compiler, standard library and Qt 5.15 as TDF.
2. Hooks run inside the radar update on the simulation thread (also during
   replay): keep them fast, never block, never touch widgets, and never call
   host callbacks from inside a hook (they are refused).
3. Radar IDs change on every scenario load: use entityId + name to recognise
   a radar across loads.
4. Geometry is radar-centred, stabilised: x = North, y = East, z = Up (m).
   Velocities are absolute (m/s), same axes.
5. Return false from a stage function to use the built-in logic for that
   call. Invalid values and exceptions also fall back to the built-in logic.
6. One plugin per replaced stage; `setLogicHooks()` returns false otherwise.
7. Implement only `AesaRadarPluginInterface` in the plugin library.
8. In `shutdown()`, call `clearLogicHooks()` (the host also removes them).
9. Send only the keys you want to change to setRadarConfig(). Writing back
   the full getRadarConfig() result re-applies every value through its toParm
   range, so a value outside that range is silently clamped (the same happens
   on scenario save/load).

## Minimal plugin

    class MyPlugin : public QObject, public AesaRadarPluginInterface {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID AesaRadarPluginInterface_iid)
        Q_INTERFACES(AesaRadarPluginInterface)
    public:
        void init() override {
            aesaplugin::LogicHooks h;
            h.rcs = [](const aesaplugin::RcsQuery& q, double& mean) {
                mean = (q.target.range_m < 20000.0) ? 2.0 : 5.0;
                return true;
            };
            setLogicHooks(h);
        }
        void update(float) override {}
        void shutdown() override { clearLogicHooks(); }
    };

## The example plugin in this folder

Build (outside the repo):

    mkdir -p ~/Desktop/work/aesa_example_plugin_build
    cd ~/Desktop/work/aesa_example_plugin_build
    /home/o2i/Qt/5.15.2/gcc_64/bin/qmake TDF_SRC=<TDF repo> <TDF repo>/sdk/examples/aesa_example_plugin/aesa_example_plugin.pro
    make -j8

Choose behaviour with `AESA_EXAMPLE_MODE` before starting TDF, e.g.
`AESA_EXAMPLE_MODE=log,rcs`. Modes: `log` (default), `rcs`, `blind`, `fog`,
`prefilter`, `filter`, `config`.

## Known limitation (v1)

The radar bridge currently sets `platformType = "GENERIC"` for every target,
so `RcsQuery::platformType` is always `"GENERIC"`. Use `targetEntityId` to
tell targets apart.
