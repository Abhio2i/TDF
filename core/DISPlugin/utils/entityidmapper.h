// =============================================================================
// FILE:        EntityIDMapper.h
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Maps your engine's string entity IDs to DIS EntityID triplets
//              (site/application/entity) and back.
//
// WHY THIS IS NEEDED:
//   Your engine identifies entities by string ID (e.g. "INS_DELHI")
//   DIS identifies entities by three uint16 numbers:
//     Site        — which DIS site (your machine's site ID from config)
//     Application — which application on that site (from config)
//     Entity      — unique number per entity on this application
//
//   This mapper owns the entity number assignment and lookup in both
//   directions.
//
// THREAD SAFETY:
//   All public methods are protected by QReadWriteLock.
//   Safe to call from both main thread and DIS network thread.
//
// USAGE:
//   // Register entity when it is added to hierarchy
//   mapper.registerEntity("INS_DELHI", 1, 1001, 1);
//
//   // Look up DIS ID when sending PDU
//   DISEntityID id = mapper.getDISID("INS_DELHI");
//
//   // Look up engine ID when receiving PDU
//   std::string name = mapper.getEngineID(1, 1001, 1);
// =============================================================================

#ifndef ENTITYIDMAPPER_H
#define ENTITYIDMAPPER_H

#include <QString>
#include <QReadWriteLock>
#include <unordered_map>
#include <string>
#include <cstdint>

// =============================================================================
// DISEntityID
// The three numbers that uniquely identify an entity on a DIS network
// =============================================================================
struct DISEntityID {
    uint16_t site        = 0;
    uint16_t application = 0;
    uint16_t entity      = 0;

    bool isValid() const {
        return site != 0 || application != 0 || entity != 0;
    }

    bool operator==(const DISEntityID& other) const {
        return site        == other.site
               && application == other.application
               && entity      == other.entity;
    }
};

// =============================================================================
// Hash for DISEntityID so it can be used as unordered_map key
// =============================================================================
struct DISEntityIDHash {
    std::size_t operator()(const DISEntityID& id) const {
        // Pack three uint16 into one uint64 for hashing
        uint64_t packed = (static_cast<uint64_t>(id.site)        << 32)
                          | (static_cast<uint64_t>(id.application) << 16)
                          | (static_cast<uint64_t>(id.entity));
        return std::hash<uint64_t>()(packed);
    }
};

// =============================================================================
// EntityIDMapper
// Thread-safe bidirectional map between engine string IDs and DIS triplets
// =============================================================================
class EntityIDMapper {
public:
    EntityIDMapper() = default;
    ~EntityIDMapper() = default;

    // ── Configuration ─────────────────────────────────────────────────────────

    // Set this machine's site and application IDs from DISConfig
    // Call once before any registration
    void configure(uint16_t siteID, uint16_t applicationID);

    // ── Registration ─────────────────────────────────────────────────────────

    // Register a locally owned entity (entity we control)
    // Auto-assigns the next available entity number
    // Returns the assigned DISEntityID
    DISEntityID registerLocalEntity(const std::string& engineID);

    // Register a remote entity (received from network)
    // Uses the exact DIS triplet from the PDU
    void registerRemoteEntity(const std::string& engineID,
                              const DISEntityID& disID);

    // Register with explicit DIS triplet (for pre-configured entities)
    void registerEntity(const std::string& engineID,
                        uint16_t site,
                        uint16_t application,
                        uint16_t entity);

    // Remove entity from both maps
    void unregisterEntity(const std::string& engineID);

    // ── Lookup ───────────────────────────────────────────────────────────────

    // Get DIS triplet from engine string ID
    // Returns invalid DISEntityID (all zeros) if not found
    DISEntityID getDISID(const std::string& engineID) const;

    // Get engine string ID from DIS triplet
    // Returns empty string if not found
    std::string getEngineID(const DISEntityID& disID) const;

    // Convenience overload
    std::string getEngineID(uint16_t site,
                            uint16_t application,
                            uint16_t entity) const;

    // Check if entity is registered
    bool isRegistered(const std::string& engineID) const;
    bool isRegistered(const DISEntityID& disID) const;

    // Check if entity is locally owned (sent by us)
    bool isLocalEntity(const std::string& engineID) const;
    bool isLocalEntity(const DISEntityID& disID) const;
bool isOwnApplication(uint16_t site, uint16_t app) const;
    // ── Bulk operations ───────────────────────────────────────────────────────

    // Clear all mappings (e.g. on scenario reset)
    void clear();

    // Total number of registered entities
    int count() const;

    // ── Debug ────────────────────────────────────────────────────────────────
    void debugDump() const;

private:
    // Config
    uint16_t m_siteID        = 1;
    uint16_t m_applicationID = 1;
    uint16_t m_nextEntityNum = 1;  // auto-increment for local entities

    // Bidirectional maps
    // engineID → DISEntityID
    std::unordered_map<std::string, DISEntityID> m_engineToDIS;

    // DISEntityID → engineID
    std::unordered_map<DISEntityID, std::string, DISEntityIDHash> m_disToEngine;

    // Track which entities are local (owned by this machine)
    std::unordered_map<std::string, bool> m_isLocal;

    // Thread safety
    mutable QReadWriteLock m_lock;
};

#endif // ENTITYIDMAPPER_H
