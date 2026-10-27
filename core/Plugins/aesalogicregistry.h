// =============================================================================
// FILE:         aesalogicregistry.h
// MODULE:       Plugin SDK — host-side registry of AESA logic hooks
// PROJECT:      Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:     RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Holds the logic hooks registered by loaded AESA plugins,
//               tagged with the owning plugin path. Every AESARadar takes a
//               snapshot once per scan(), so hooks apply to radars however
//               they were created (UI, scenario load, replay, DIS).
//               Unloading a plugin removes its hooks in one call.
//               Thread-safe: mutex-protected, immutable snapshots.
//               REQ-AESA-090.
// =============================================================================

#ifndef AESALOGICREGISTRY_H
#define AESALOGICREGISTRY_H

#include "core/Plugins/Interfaces/aesaRadarInterface.h"
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// Merged view of all registered hooks. Immutable once published.
struct AesaLogicSnapshot
{
    aesaplugin::RcsFn                        rcs;
    aesaplugin::DetectionFn                  detection;
    aesaplugin::PropagationFn                propagation;
    std::vector<aesaplugin::TargetFilterFn>  preFilters;    // plugin load order
    std::vector<aesaplugin::OutputFilterFn>  postFilters;   // plugin load order

    bool hasStages() const
    {
        return static_cast<bool>(rcs) || static_cast<bool>(detection) ||
               static_cast<bool>(propagation);
    }
    bool empty() const
    {
        return !hasStages() && preFilters.empty() && postFilters.empty();
    }
};

class AesaLogicRegistry
{
public:
    static AesaLogicRegistry& instance();

    // Sets (or replaces) the hooks owned by 'owner' (plugin path). Fails if
    // another owner already replaces one of the same stages.
    bool setHooks(const std::string& owner, const aesaplugin::LogicHooks& hooks,
                  std::string* error = nullptr);

    // Removes every hook owned by 'owner'. Safe if owner has none.
    void removeOwner(const std::string& owner);

    // Current merged hooks. Never null.
    std::shared_ptr<const AesaLogicSnapshot> snapshot() const;

    // True while a plugin hook is running on this thread. Host callbacks
    // refuse to run then (they could deadlock the radar model).
    static bool inStage();

    // RAII marker placed around every call into a plugin hook.
    class StageGuard
    {
    public:
        StageGuard();
        ~StageGuard();
        StageGuard(const StageGuard&) = delete;
        StageGuard& operator=(const StageGuard&) = delete;
    };

private:
    AesaLogicRegistry() = default;
    void rebuildLocked();

    mutable std::mutex mutex_;
    std::vector<std::pair<std::string, aesaplugin::LogicHooks>> owners_;
    std::shared_ptr<const AesaLogicSnapshot> snapshot_ =
        std::make_shared<const AesaLogicSnapshot>();
};

#endif // AESALOGICREGISTRY_H
