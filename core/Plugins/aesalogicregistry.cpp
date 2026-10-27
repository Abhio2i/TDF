// =============================================================================
// FILE:         aesalogicregistry.cpp
// MODULE:       Plugin SDK — host-side registry of AESA logic hooks
// DESCRIPTION:  See aesalogicregistry.h. REQ-AESA-090.
// =============================================================================

#include "core/Plugins/aesalogicregistry.h"
#include <algorithm>

namespace
{
// Depth of plugin-hook calls on this thread (nesting-safe).
thread_local int t_stageDepth = 0;
}

AesaLogicRegistry& AesaLogicRegistry::instance()
{
    static AesaLogicRegistry registry;
    return registry;
}

bool AesaLogicRegistry::inStage()
{
    return t_stageDepth > 0;
}

AesaLogicRegistry::StageGuard::StageGuard()  { ++t_stageDepth; }
AesaLogicRegistry::StageGuard::~StageGuard() { --t_stageDepth; }

bool AesaLogicRegistry::setHooks(const std::string& owner,
                                 const aesaplugin::LogicHooks& hooks,
                                 std::string* error)
{
    std::lock_guard<std::mutex> lk(mutex_);

    // One owner per replaceable stage.
    for (const auto& entry : owners_)
    {
        if (entry.first == owner) continue;
        const char* clash = nullptr;
        if (hooks.rcs && entry.second.rcs)                       clash = "RCS";
        else if (hooks.detection && entry.second.detection)      clash = "detection";
        else if (hooks.propagation && entry.second.propagation)  clash = "propagation";
        if (clash != nullptr)
        {
            if (error != nullptr)
                *error = std::string(clash) + " stage is already replaced by " + entry.first;
            return false;
        }
    }

    auto it = std::find_if(owners_.begin(), owners_.end(),
                           [&owner](const auto& e) { return e.first == owner; });
    if (it != owners_.end())
        it->second = hooks;
    else
        owners_.emplace_back(owner, hooks);

    rebuildLocked();
    return true;
}

void AesaLogicRegistry::removeOwner(const std::string& owner)
{
    std::lock_guard<std::mutex> lk(mutex_);
    owners_.erase(std::remove_if(owners_.begin(), owners_.end(),
                                 [&owner](const auto& e) { return e.first == owner; }),
                  owners_.end());
    rebuildLocked();
}

std::shared_ptr<const AesaLogicSnapshot> AesaLogicRegistry::snapshot() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return snapshot_;
}

void AesaLogicRegistry::rebuildLocked()
{
    auto s = std::make_shared<AesaLogicSnapshot>();
    for (const auto& entry : owners_)
    {
        const aesaplugin::LogicHooks& h = entry.second;
        if (h.rcs)         s->rcs         = h.rcs;
        if (h.detection)   s->detection   = h.detection;
        if (h.propagation) s->propagation = h.propagation;
        if (h.preFilter)   s->preFilters.push_back(h.preFilter);
        if (h.postFilter)  s->postFilters.push_back(h.postFilter);
    }
    snapshot_ = s;
}
