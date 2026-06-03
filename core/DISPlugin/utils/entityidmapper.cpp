// =============================================================================
// FILE:        EntityIDMapper.cpp
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "entityidmapper.h"
#include <QDebug>
#include <QReadLocker>
#include <QWriteLocker>

// =============================================================================
// configure
// Set site and application IDs from dis_config.json
// Call once before registering any entities
// =============================================================================
void EntityIDMapper::configure(uint16_t siteID, uint16_t applicationID)
{
    QWriteLocker lock(&m_lock);
    m_siteID        = siteID;
    m_applicationID = applicationID;
    m_nextEntityNum = 1;

    qDebug() << "[EntityIDMapper] Configured:"
             << "site=" << siteID
             << "app="  << applicationID;
}

// =============================================================================
// registerLocalEntity
// Register an entity owned by this machine
// Auto-assigns the next available entity number
// Returns the assigned DISEntityID
// =============================================================================
DISEntityID EntityIDMapper::registerLocalEntity(const std::string& engineID)
{
    QWriteLocker lock(&m_lock);

    // Already registered — return existing
    auto it = m_engineToDIS.find(engineID);
    if (it != m_engineToDIS.end()) {
        return it->second;
    }

    // Assign next entity number
    DISEntityID disID;
    disID.site        = m_siteID;
    disID.application = m_applicationID;
    disID.entity      = m_nextEntityNum++;

    // Store both directions
    m_engineToDIS[engineID] = disID;
    m_disToEngine[disID]    = engineID;
    m_isLocal[engineID]     = true;

    qDebug() << "[EntityIDMapper] Local entity registered:"
             << QString::fromStdString(engineID)
             << "→ site=" << disID.site
             << "app="    << disID.application
             << "entity=" << disID.entity;

    return disID;
}

// =============================================================================
// registerRemoteEntity
// Register an entity received from the network
// Uses the exact DIS triplet from the incoming PDU
// =============================================================================
void EntityIDMapper::registerRemoteEntity(const std::string& engineID,
                                          const DISEntityID& disID)
{
    QWriteLocker lock(&m_lock);

    // Already registered — skip
    if (m_engineToDIS.find(engineID) != m_engineToDIS.end()) return;

    m_engineToDIS[engineID] = disID;
    m_disToEngine[disID]    = engineID;
    m_isLocal[engineID]     = false;

    qDebug() << "[EntityIDMapper] Remote entity registered:"
             << QString::fromStdString(engineID)
             << "← site=" << disID.site
             << "app="    << disID.application
             << "entity=" << disID.entity;
}

// =============================================================================
// registerEntity
// Register with explicit DIS triplet
// Used for pre-configured entities or loading from scenario
// =============================================================================
void EntityIDMapper::registerEntity(const std::string& engineID,
                                    uint16_t site,
                                    uint16_t application,
                                    uint16_t entity)
{
    DISEntityID disID;
    disID.site        = site;
    disID.application = application;
    disID.entity      = entity;

    bool isLocal = (site == m_siteID && application == m_applicationID);

    QWriteLocker lock(&m_lock);

    m_engineToDIS[engineID] = disID;
    m_disToEngine[disID]    = engineID;
    m_isLocal[engineID]     = isLocal;

    // Keep nextEntityNum ahead of any manually assigned numbers
    if (isLocal && entity >= m_nextEntityNum) {
        m_nextEntityNum = entity + 1;
    }
}

// =============================================================================
// unregisterEntity
// Remove entity from both maps
// Called when entity is removed from hierarchy
// =============================================================================
void EntityIDMapper::unregisterEntity(const std::string& engineID)
{
    QWriteLocker lock(&m_lock);

    auto it = m_engineToDIS.find(engineID);
    if (it == m_engineToDIS.end()) return;

    DISEntityID disID = it->second;

    m_disToEngine.erase(disID);
    m_engineToDIS.erase(engineID);
    m_isLocal.erase(engineID);

    qDebug() << "[EntityIDMapper] Entity unregistered:"
             << QString::fromStdString(engineID);
}

// =============================================================================
// getDISID
// Look up DIS triplet from engine string ID
// Returns invalid DISEntityID (all zeros) if not found
// =============================================================================
DISEntityID EntityIDMapper::getDISID(const std::string& engineID) const
{
    QReadLocker lock(&m_lock);

    auto it = m_engineToDIS.find(engineID);
    if (it != m_engineToDIS.end()) {
        return it->second;
    }

    // Not found — return invalid
    return DISEntityID{};
}

// =============================================================================
// getEngineID (from DISEntityID)
// Look up engine string ID from DIS triplet
// Returns empty string if not found
// =============================================================================
std::string EntityIDMapper::getEngineID(const DISEntityID& disID) const
{
    QReadLocker lock(&m_lock);

    auto it = m_disToEngine.find(disID);
    if (it != m_disToEngine.end()) {
        return it->second;
    }

    return "";
}

// =============================================================================
// getEngineID (from three uint16 values)
// Convenience overload — builds DISEntityID then looks up
// =============================================================================
std::string EntityIDMapper::getEngineID(uint16_t site,
                                        uint16_t application,
                                        uint16_t entity) const
{
    DISEntityID disID;
    disID.site        = site;
    disID.application = application;
    disID.entity      = entity;
    return getEngineID(disID);
}

// =============================================================================
// isRegistered
// =============================================================================
bool EntityIDMapper::isRegistered(const std::string& engineID) const
{
    QReadLocker lock(&m_lock);
    return m_engineToDIS.find(engineID) != m_engineToDIS.end();
}

bool EntityIDMapper::isRegistered(const DISEntityID& disID) const
{
    QReadLocker lock(&m_lock);
    return m_disToEngine.find(disID) != m_disToEngine.end();
}

// =============================================================================
// isLocalEntity
// =============================================================================
bool EntityIDMapper::isLocalEntity(const std::string& engineID) const
{
    QReadLocker lock(&m_lock);
    auto it = m_isLocal.find(engineID);
    return (it != m_isLocal.end()) && it->second;
}
bool EntityIDMapper::isOwnApplication(uint16_t site, uint16_t app) const
{
    return (site == m_siteID && app == m_applicationID);
}
bool EntityIDMapper::isLocalEntity(const DISEntityID& disID) const
{
    QReadLocker lock(&m_lock);
    auto it = m_disToEngine.find(disID);
    if (it == m_disToEngine.end()) return false;
    auto jt = m_isLocal.find(it->second);
    return (jt != m_isLocal.end()) && jt->second;
}

// =============================================================================
// clear
// Remove all mappings — called on scenario reset
// =============================================================================
void EntityIDMapper::clear()
{
    QWriteLocker lock(&m_lock);
    m_engineToDIS.clear();
    m_disToEngine.clear();
    m_isLocal.clear();
    m_nextEntityNum = 1;

    qDebug() << "[EntityIDMapper] Cleared all entity mappings";
}

// =============================================================================
// count
// =============================================================================
int EntityIDMapper::count() const
{
    QReadLocker lock(&m_lock);
    return static_cast<int>(m_engineToDIS.size());
}

// =============================================================================
// debugDump
// Print all registered entities to debug output
// =============================================================================
void EntityIDMapper::debugDump() const
{
    QReadLocker lock(&m_lock);

    qDebug() << "[EntityIDMapper] ── Entity Map Dump ──";
    qDebug() << "[EntityIDMapper] Total entities:" << m_engineToDIS.size();

    for (const auto& pair : m_engineToDIS) {
        const DISEntityID& id = pair.second;
        bool local = false;
        auto lit = m_isLocal.find(pair.first);
        if (lit != m_isLocal.end()) local = lit->second;

        qDebug() << "[EntityIDMapper]"
                 << QString::fromStdString(pair.first)
                 << "→"
                 << id.site << "/" << id.application << "/" << id.entity
                 << (local ? "[LOCAL]" : "[REMOTE]");
    }
    qDebug() << "[EntityIDMapper] ────────────────────";
}
