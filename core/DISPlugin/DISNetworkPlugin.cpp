// =============================================================================
// FILE:        DISNetworkPlugin.cpp
// MODULE:      DIS Network Plugin
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "DISNetworkPlugin.h"

// These will be included once written — stubbed for now
// #include "core/DISManager.h"
// #include "bridge/DISNetworkBridge.h"

// Engine includes
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"
#include "core/DISPlugin/utils/dislogger.h"
//#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "config/disconfigloader.h"
#include "core/dismanager.h"
#include "bridge/disnetworkbridge.h"
#include <cmath>
#include <core/Hierarchy/Utils/entityutils.h>
// =============================================================================
// Register metatypes for cross-thread signal/slot
// Must be done before any signal/slot connections across threads
// =============================================================================
static bool registerMetaTypes() {
    qRegisterMetaType<DISNetworkStatus>    ("DISNetworkStatus");
    qRegisterMetaType<DISEntitySnapshot>   ("DISEntitySnapshot");
    qRegisterMetaType<DISFireSnapshot>     ("DISFireSnapshot");
    qRegisterMetaType<DISDetonationSnapshot>("DISDetonationSnapshot");
    qRegisterMetaType<DISEmissionSnapshot> ("DISEmissionSnapshot");
    qRegisterMetaType<DISStartResumeData>  ("DISStartResumeData");
    qRegisterMetaType<DISStopFreezeData>   ("DISStopFreezeData");
    qRegisterMetaType<DISRemoveEntityData> ("DISRemoveEntityData");
    qRegisterMetaType<DISCreateEntityData> ("DISCreateEntityData");
    qRegisterMetaType<DISIncomingTransform>("DISIncomingTransform");
    qRegisterMetaType<DISIncomingFire>     ("DISIncomingFire");
    qRegisterMetaType<DISIncomingDetonation>("DISIncomingDetonation");
    qRegisterMetaType<DISIncomingExercise> ("DISIncomingExercise");
    qRegisterMetaType<DISConfig>           ("DISConfig");
    qRegisterMetaType<DISIncomingIFF>("DISIncomingIFF");
    qRegisterMetaType<DISIFFSnapshot>("DISIFFSnapshot");
    qRegisterMetaType<DISIncomingAcknowledge>("DISIncomingAcknowledge");
    qRegisterMetaType<DISIncomingTransferOwnership>("DISIncomingTransferOwnership");
    qRegisterMetaType<DISTransferOwnershipData>    ("DISTransferOwnershipData");
    qRegisterMetaType<DISIncomingEmission>("DISIncomingEmission");
    qRegisterMetaType<DISIncomingDesignator>("DISIncomingDesignator");
    qRegisterMetaType<DISDesignatorSnapshot>("DISDesignatorSnapshot");
    qRegisterMetaType<DISTransmitterSnapshot>("DISTransmitterSnapshot");
    qRegisterMetaType<DISIncomingTransmitter>("DISIncomingTransmitter");
    qRegisterMetaType<DISSignalSnapshot>("DISSignalSnapshot");
    qRegisterMetaType<DISIncomingSignal>("DISIncomingSignal");
    qRegisterMetaType<DISDatumSnapshot>("DISDatumSnapshot");
    qRegisterMetaType<DISIncomingDatum>("DISIncomingDatum");
    qRegisterMetaType<DISAcknowledgeData>("DISAcknowledgeData");
    return true;
}
static bool s_metaTypesRegistered = registerMetaTypes();

// =============================================================================
// Constructor
// =============================================================================
DISNetworkPlugin::DISNetworkPlugin(QObject* parent)
    : SimulationPlugin(parent)
{
    // Load STAGE-compatible defaults
    m_config = DISDefaultConfig();

    DIS_LOG_BASIC("[DISNetworkPlugin] Created");
}

// =============================================================================
// Destructor
// =============================================================================
DISNetworkPlugin::~DISNetworkPlugin()
{
    stop();
    DIS_LOG_BASIC("[DISNetworkPlugin] Destroyed");
}

// =============================================================================
// attachHierarchy
// Store reference to engine hierarchy for reading entity state
// =============================================================================
void DISNetworkPlugin::attachHierarchy(Hierarchy* hierarchy)
{
    m_hierarchy = hierarchy;
   DIS_LOG_BASIC("[DISNetworkPlugin] Hierarchy attached");
}

// =============================================================================
// attachSimulation
// Store reference to simulation and connect to its signals
// =============================================================================
void DISNetworkPlugin::attachSimulation(Simulation* simulation)
{
    m_simulation = simulation;
    DIS_LOG_BASIC("[DISNetworkPlugin] Simulation attached");
}
// =============================================================================
// buildEntityTypeRegistry
// Maps DIS domain+category to local sprite and collider values.
// One entry per engine entity subtype.
// Domain:  1=Land  2=Air  3=Surface  4=Subsurface
// Category matches DISNetworkBridge::subCategoryToDISCategory output.
// =============================================================================
void DISNetworkPlugin::buildEntityTypeRegistry()
{
    m_entityTypeRegistry.clear();

    auto add = [&](uint8_t domain, uint8_t category,
                   const char* sprite,
                   float collideR, float warnR) {
        DISEntityTypeProfile p;
        p.domain        = domain;
        p.category      = category;
        p.spritePath    = sprite;
        p.collideRadius = collideR;
        p.warningRadius = warnR;
        m_entityTypeRegistry.append(p);
    };

    // ── Air (domain=2) — all values confirmed against real SISO-REF-010 ────────
    add(2,  1, ":/texture/images/Texture/DIS_PIC/fighter_jet.png",      50,  200); // Fighter/Air Defense
    add(2,  2, ":/texture/images/Texture/DIS_PIC/fighter_jet.png",      50,  200); // Attack/Strike
    add(2,  3, ":/texture/images/Texture/DIS_PIC/bomber.png",           60,  250); // Bomber
    add(2,  4, ":/texture/images/Texture/DIS_PIC/cargo_transport.png",  50,  200); // Cargo/Tanker
    add(2, 14, ":/texture/images/Texture/DIS_PIC/uav.png",              20,  100); // UAV
    add(2, 20, ":/texture/images/Texture/DIS_PIC/helicopter.png",       30,  150); // Attack Helicopter
    add(2, 21, ":/texture/images/Texture/DIS_PIC/helicopter.png",       30,  150); // Utility Helicopter

    // ── Land (domain=1) — confirmed; no real Artillery/AFV top-level category exists ──
    add(1,  1, ":/texture/images/Texture/DIS_PIC/tank.png",             10,   50); // Tank
    add(1,  3, ":/texture/images/Texture/DIS_PIC/ground_radar.png",     15,   60); // (kept — existing GroundRadar mapping)
    add(1,  4, ":/texture/images/Texture/DIS_PIC/tank.png",              5,   30); // Human (kept — existing mapping)
    add(1, 84, ":/texture/images/Texture/DIS_PIC/utility_vehicle.png",   8,   40); // Single Unit Utility/Emergency Truck

    // ── Surface (domain=3) — confirmed against real SISO-REF-010 ───────────────
    add(3,  1, ":/texture/images/Texture/DIS_PIC/ship.png",            100,  500); // Carrier
    add(3,  2, ":/texture/images/Texture/DIS_PIC/destroyer.png",        90,  450); // Command Ship/Cruiser
    add(3,  3, ":/texture/images/Texture/DIS_PIC/destroyer.png",        90,  450); // Guided Missile Cruiser
    add(3,  4, ":/texture/images/Texture/DIS_PIC/destroyer.png",        85,  420); // Guided Missile Destroyer
    add(3,  5, ":/texture/images/Texture/DIS_PIC/destroyer.png",        85,  420); // Destroyer
    add(3,  6, ":/texture/images/Texture/DIS_PIC/frigate.png",          80,  400); // Guided-missile Frigate
    add(3,  7, ":/texture/images/Texture/DIS_PIC/patrol_craft.png",     30,  150); // Light/Patrol Craft
    add(3,  8, ":/texture/images/Texture/DIS_PIC/auxiliary_ship.png",   60,  300); // Mine Countermeasure Ship
    add(3,  9, ":/texture/images/Texture/DIS_PIC/landing_ship.png",     70,  350); // Dock Landing Ship

    // ── Subsurface (domain=4) ────────────────────────────────────────────────
    // ── Subsurface (domain=4) — confirmed against real SISO-REF-010 ────────────
    add(4,  1, ":/texture/images/Texture/DIS_PIC/submarine.png",        60,  300); // SSBN
    add(4,  2, ":/texture/images/Texture/DIS_PIC/submarine.png",        60,  300); // SSGN
    add(4,  3, ":/texture/images/Texture/DIS_PIC/submarine.png",        60,  300); // SSN
    add(4,  4, ":/texture/images/Texture/DIS_PIC/submarine.png",        60,  300); // SSG
    add(4,  5, ":/texture/images/Texture/DIS_PIC/submarine.png",        60,  300); // SS
    add(4,  8, ":/texture/images/Texture/DIS_PIC/uuv.png",              20,  100); // UUV


    DIS_LOG_BASIC("[DISNetworkPlugin] Entity type registry:" << m_entityTypeRegistry.size() << "entries");
}
// void DISNetworkPlugin::buildEntityTypeRegistry()
// {
//     m_entityTypeRegistry.clear();

//     auto add = [&](uint8_t domain, uint8_t category,
//                    const char* sprite,
//                    float collideR, float warnR) {
//         DISEntityTypeProfile p;
//         p.domain        = domain;
//         p.category      = category;
//         p.spritePath    = sprite;
//         p.collideRadius = collideR;
//         p.warningRadius = warnR;
//         m_entityTypeRegistry.append(p);
//     };

//     // ── Air ───────────────────────────────────────────────────────────────────
//     add(2,  1, ":/texture/images/Texture/DIS_PIC/fighter_jet.png",   50,  200); // Aircraft
//     add(2,  2, ":/texture/images/Texture/DIS_PIC/helicopter.png",    30,  150); // Helicopter
//     add(2, 14, ":/texture/images/Texture/DIS_PIC/uav.png",           20,  100); // UAV

//     // ── Land ──────────────────────────────────────────────────────────────────
//     add(1,  1, ":/texture/images/Texture/DIS_PIC/tank.png",          10,   50); // Tank
//     add(1,  3, ":/texture/images/Texture/DIS_PIC/ground_radar.png",  15,   60); // GroundRadar
//     add(1,  4, ":/texture/images/Texture/DIS_PIC/tank.png",       5,   30); // Human

//     // ── Surface ───────────────────────────────────────────────────────────────
//     add(3,  1, ":/texture/images/Texture/DIS_PIC/ship.png",         100,  500); // Ship
//     add(3,  6, ":/texture/images/Texture/DIS_PIC/frigate.png",       80,  400); // Frigate

//     // ── Subsurface ────────────────────────────────────────────────────────────
//     add(4,  1, ":/texture/images/Texture/DIS_PIC/submarine.png",     60,  300); // Submarine

//     DIS_LOG_BASIC("[DISNetworkPlugin] Entity type registry:" << m_entityTypeRegistry.size() << "entries");
// }
std::string DISNetworkPlugin::lookupSprite(uint8_t domain,
                                           uint8_t category) const
{
    for (const DISEntityTypeProfile& p : m_entityTypeRegistry) {
        if (p.domain == domain && p.category == category)
            return p.spritePath;
    }
    // Domain-aware fallback — a category we haven't explicitly mapped still
    // gets a correct-domain icon, never the wrong domain (e.g. never shows
    // an unmapped ship as an aircraft).
    switch (domain) {
    case 1: return ":/texture/images/Texture/DIS_PIC/tank.png";          // Land
    case 2: return ":/texture/images/Texture/DIS_PIC/fighter_jet.png";   // Air
    case 3: return ":/texture/images/Texture/DIS_PIC/ship.png";          // Surface
    case 4: return ":/texture/images/Texture/DIS_PIC/submarine.png";     // Subsurface
    default: return ":/texture/images/Texture/DIS_PIC/fighter_jet.png"; // Other/Space — no dedicated asset
    }
}
// std::string DISNetworkPlugin::lookupSprite(uint8_t domain,
//                                            uint8_t category) const
// {
//     for (const DISEntityTypeProfile& p : m_entityTypeRegistry) {
//         if (p.domain == domain && p.category == category)
//             return p.spritePath;
//     }
//     return ":/texture/images/Texture/DIS_PIC/fighter_jet.png";  // fallback
// }
void DISNetworkPlugin::lookupCollider(uint8_t domain, uint8_t category,
                                      float& collideRadius,
                                      float& warningRadius) const
{
    for (const DISEntityTypeProfile& p : m_entityTypeRegistry) {
        if (p.domain == domain && p.category == category) {
            collideRadius = p.collideRadius;
            warningRadius = p.warningRadius;
            return;
        }
    }
    // Domain-aware fallback sizes, roughly matching each domain's typical scale.
    switch (domain) {
    case 1: collideRadius = 10.0f;  warningRadius = 50.0f;  break; // Land
    case 2: collideRadius = 50.0f;  warningRadius = 200.0f; break; // Air
    case 3: collideRadius = 100.0f; warningRadius = 500.0f; break; // Surface
    case 4: collideRadius = 60.0f;  warningRadius = 300.0f; break; // Subsurface
    default: collideRadius = 200.0f; warningRadius = 400.0f; break;
    }
}
// void DISNetworkPlugin::lookupCollider(uint8_t domain, uint8_t category,
//                                       float& collideRadius,
//                                       float& warningRadius) const
// {
//     for (const DISEntityTypeProfile& p : m_entityTypeRegistry) {
//         if (p.domain == domain && p.category == category) {
//             collideRadius = p.collideRadius;
//             warningRadius = p.warningRadius;
//             return;
//         }
//     }
//     collideRadius = 200.0f;
//     warningRadius = 400.0f;
// }
// =============================================================================
// start
// Load config, create DIS thread, start network
// =============================================================================
void DISNetworkPlugin::start(const QString& configPath)
{
    if (m_running.load()) {
        DIS_LOG_WARNING("[DISNetworkPlugin] Already running, stop first");
        return;
    }

    m_configPath = configPath;

    // Load config from JSON file
    // (DISConfigLoader will handle this properly in Day 9)
    // For now use defaults
    DIS_LOG_BASIC("[DISNetworkPlugin] Loading config from" << configPath);
    m_config = DISConfigLoader::load(configPath);
    buildEntityTypeRegistry();
    DISLogger::instance().setLevelFromString(m_config.globalTraceLevel);
    DIS_LOG_BASIC("[DISNetworkPlugin] Log level:" << QString::fromStdString(m_config.globalTraceLevel));
    if (m_config.siteID == 0 || m_config.applicationID == 0) {
        DIS_LOG_ERROR("[DISNetworkPlugin] INVALID CONFIG: siteID and applicationID must be non-zero");
        return;
    }

    DIS_LOG_BASIC("[DISNetworkPlugin] Identity: site=" << m_config.siteID << "app=" << m_config.applicationID);

    // Connect simulation and hierarchy signals
    // if (m_simulation) connectSimulationSignals();
    // if (m_hierarchy)  connectHierarchySignals();
    if (m_simulation && !m_signalsConnected) connectSimulationSignals();
    if (m_hierarchy  && !m_signalsConnected) connectHierarchySignals();
    m_signalsConnected = true;

    // ── Create DIS network thread ─────────────────────────────────────────────
    m_disThread = new QThread(this);
    m_disThread->setObjectName("DISNetworkThread");

    // ── Create DISManager and move to thread ──────────────────────────────────
    m_disManager = new DISManager();
    m_disManager->setPlugin(this);
    m_disManager->setHierarchy(m_hierarchy);
    m_disManager->moveToThread(m_disThread);

    connect(m_disThread, &QThread::finished,
            m_disManager, &QObject::deleteLater);
    // Connect hierarchy entity added → DISManager → bridge registration
    connect(m_hierarchy, &Hierarchy::entityAdded,
            m_disManager, &DISManager::onEntityAdded,
            Qt::QueuedConnection);

    // connect(m_hierarchy, &Hierarchy::entityRemovedfull,
    //         m_disManager, &DISManager::onEntityRemoved,
    //         Qt::BlockingQueuedConnection);

    connect(m_disManager, &DISManager::errorOccurred,
            this, &DISNetworkPlugin::onIncomingError,
            Qt::QueuedConnection);

    connect(m_disManager, &DISManager::peerJoined,
            this, &DISNetworkPlugin::onIncomingPeerJoined,
            Qt::QueuedConnection);
    connect(this, &DISNetworkPlugin::_internalSimulationStart,
            m_simulation, &Simulation::start,
            Qt::QueuedConnection);

    connect(this, &DISNetworkPlugin::_internalSimulationPause,
            m_simulation, &Simulation::pause,
            Qt::QueuedConnection);

    connect(this, &DISNetworkPlugin::_internalSimulationStop,
            m_simulation, &Simulation::stop,
            Qt::QueuedConnection);
    connect(m_disThread, &QThread::started, this, [this]() {
        QMetaObject::invokeMethod(
            m_disManager, "start",
            Qt::QueuedConnection,
            Q_ARG(DISConfig, m_config));

        // Re-register existing entities AFTER manager starts
        if (m_hierarchy) {
            for (auto& pair : m_hierarchy->Entities) {
                Entity* entity = pair.second;
                if (!entity) continue;
                if (entity->isRemoteDISEntity) continue;
                QString entityID = QString::fromStdString(entity->ID);
                QString entityName = QString::fromStdString(entity->Name);
                QMetaObject::invokeMethod(m_disManager, "onEntityAdded",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, ""),
                                          Q_ARG(QString, entityID),
                                          Q_ARG(QString, entityName));
            }
        }
    });
    m_preExistingEntityIDs.clear();
    if (m_hierarchy) {
        for (const auto& pair : m_hierarchy->Entities) {
            if (pair.second && !pair.second->isRemoteDISEntity)
                m_preExistingEntityIDs.insert(
                    QString::fromStdString(pair.first));
        }
    }
    m_disThread->start();







    m_running.store(true);

    // ── Incoming PDU drain timer ──────────────────────────────────────────────
    // applyPendingUpdates() is called by Simulation::frame() which only ticks
    // when updateTimer is running (simulation playing).
    // PDUs arriving before Play is pressed, or while paused, would pile up.
    // This timer drains all incoming queues at 20Hz regardless of sim state.
    // Both this timer and frame() run on the main thread — Qt event loop
    // serialises them, overlap is impossible by design.
    // pushAllLocalSnapshots() is intentionally excluded — it reads entity
    // positions from dynamic models which only advance during sim play.
    m_drainTimer = new QTimer(this);
    m_drainTimer->setInterval(50);  // 20Hz — above DIS 5Hz send rate
    connect(m_drainTimer, &QTimer::timeout, this, [this]() {
        if (!m_running.load()) return;
        applyTransformUpdates();
        applyFireEvents();
        applyDetonationEvents();
        applyExerciseEvents();
        applyIFFEvents();
        applyAcknowledgeEvents();
        applyTransferOwnershipEvents();
        applyDataQuerySubscriptions();   // ← add this line


    });
    m_drainTimer->start();

    startRemoteEntityTimeoutChecker();

    DIS_LOG_BASIC( "[DISNetworkPlugin] Started on thread"
             << m_disThread->objectName());
}

// =============================================================================
// stop
// Shut down thread cleanly
// =============================================================================
void DISNetworkPlugin::stop()
{
    if (!m_running.load()) return;
    m_running.store(false);
    m_disTriggeredPause = false;  // ← ADD THIS
    m_wasPaused = false;          // ← ADD THIS
    m_signalsConnected = false;   // ← ADD THIS
    // Step 1 — remove all remote entities from hierarchy
    if (m_hierarchy && !m_remoteEntityTimestamps.isEmpty()) {
        for (const QString& entityKey : m_remoteEntityTimestamps.keys()) {
            QString parentID = m_remoteEntityParentIDs.value(entityKey, "");
            if (!parentID.isEmpty())
                emit removeRemoteDISEntity(parentID, entityKey, false);
        }
    }
    // // Step 2 — remove remote entities folder to prevent duplicate on reconnect
    if (!m_remoteEntitiesFolderID.isEmpty() && m_hierarchy) {
        m_hierarchy->removeFolderViaNetwork(m_remoteEntitiesFolderID);
    }
    m_remoteEntitiesFolderID = "";
    m_pendingRemoteEntities.clear();
    m_remoteEntityTimestamps.clear();
    m_remoteEntityParentIDs.clear();
    m_remoteEntityDRStates.clear();
    m_destroyedEntityIDs.clear();
    m_preExistingEntityIDs.clear();
    m_remoteEntityDomain.clear();    // ← ADD
    m_remoteEntityCategory.clear();  // ← ADD
    if (m_timeoutTimer) m_timeoutTimer->stop();
    if (m_drainTimer) {          // ADD
        m_drainTimer->stop();    // ADD
        m_drainTimer = nullptr;  // ADD
    }

    // DISStopFreezeData stopData;
    // stopData.reason = DISStopReason::Termination;
    // publishStopFreeze(stopData);

    if (m_disThread && m_disThread->isRunning()) {
        if (m_disManager) {
            QMetaObject::invokeMethod(m_disManager, "stop",
                                      Qt::BlockingQueuedConnection);
        }
        m_disThread->quit();
        m_disThread->wait(3000);
        if (m_disThread->isRunning()) {
            DIS_LOG_WARNING("[DISNetworkPlugin] Thread did not stop cleanly");
            m_disThread->terminate();
        }
        m_disThread->deleteLater();
        m_disThread  = nullptr;
        m_disManager = nullptr;
    }
    DIS_LOG_BASIC("[DISNetworkPlugin] Stopped");
}

// =============================================================================
// applyPendingUpdates
// Called by Simulation::frame() on MAIN THREAD every frame
// Drains all incoming queues and applies to engine
// Must be fast — runs on sim tick
// =============================================================================
void DISNetworkPlugin::applyPendingUpdates()
{
    if (!m_running.load()) return;
    // pushAllLocalSnapshots reads entity positions from dynamic models.
    // Dynamic models only advance during simulation play — must stay here,
    // called from Simulation::frame() which is sim-gated.
    // applyTransformUpdates and all event draining are handled by
    // m_drainTimer at 20Hz, independent of simulation play state.

    pushAllLocalSnapshots();
    pushAllIFFSnapshots();
}
void DISNetworkPlugin::pushAllLocalSnapshots()
{
    if (!m_hierarchy) return;


    for (const auto& pair : m_hierarchy->Entities) {
        Entity* entity = pair.second;
        if (!entity) continue;

        Platform* platform = dynamic_cast<Platform*>(entity);


        if (entity->isRemoteDISEntity) continue;
        if (!platform)  continue;

        QString entityKey = QString::fromStdString(entity->ID);

        if (!entity->Active || entity->Health <= 0.0f) {
            DIS_LOG_WARNING("[DIS DESTROYED] Entity stopped broadcasting:"
                            << entityKey
                            << "Active=" << entity->Active
                            << "Health=" << entity->Health);
            if (!m_destroyedEntityIDs.contains(entityKey)) {
                DISEntitySnapshot snap;
                if (buildEntitySnapshot(platform, snap)) {
                    snap.active     = false;
                    snap.health     = 0.0f;
                    snap.appearance = (3u << 3);
                    emit _internalPushSnapshot(snap);
                }
                m_destroyedEntityIDs.insert(entityKey);
            }
            continue;
        }
        // Entity is active — remove from destroyed set if it was there
        // This allows recovery if entity health was restored or active was reset
        m_destroyedEntityIDs.remove(entityKey);
        DISEntitySnapshot snap;
        if (buildEntitySnapshot(platform, snap))
            emit _internalPushSnapshot(snap);
    }
}
bool DISNetworkPlugin::buildEntitySnapshot(Platform*          platform,
                                           DISEntitySnapshot& snap) const
{
    if (!platform )       return false;
    if (!platform->Active) return false;

    Entity* entity = platform;

    snap.entityID   = entity->ID;
    snap.parentID   = entity->parentID;
    snap.active     = entity->Active;
    snap.marking    = entity->Name.substr(0, 10);
    snap.forceID    = DISNetworkBridge::teamToForceID(static_cast<int>(entity->team));
    snap.health     = entity->Health;
    snap.appearance = DISNetworkBridge::healthToAppearance(entity->Health);

    if (!platform->disType.useOverride) {
        uint8_t domain, kind;
        DISNetworkBridge::categoryToDISType(static_cast<int>(entity->category),
                                            domain, kind);
        // Fix submarine domain: Marine/Submarine → Subsurface (4), not Surface (3)
        // DIS standard domain values: Other=0 Land=1 Air=2 Surface=3 Subsurface=4
        if (entity->category == Entity::Category::Marine &&
            platform->marineCategory == Entity::SubMarineCategory::Submarine) {
            domain = static_cast<uint8_t>(4);  // Subsurface
        }
        snap.entityType.kind        = kind;
        snap.entityType.domain      = domain;
        snap.entityType.country     = m_config.countryCode;
        int subCat = 0;
        switch (entity->category) {
        case Entity::Category::Air:
            subCat = static_cast<int>(platform->airCategory);
            break;
        case Entity::Category::Ground:
            subCat = static_cast<int>(platform->groundCategory);
            break;
        case Entity::Category::Marine:
            subCat = static_cast<int>(platform->marineCategory);
            break;
        }
        snap.entityType.category    = DISNetworkBridge::subCategoryToDISCategory(
            static_cast<int>(entity->category), subCat);
        // subcategory=1 is the generic first subtype for every category.
        // subcategory=0 means "other/undefined" in SISO-REF-010 — avoid it.
        // specific and extra require a full SISO type registry — leave as 0.
        snap.entityType.subcategory = 1;
        snap.entityType.specific    = 0;
        snap.entityType.extra       = 0;
    } else {
        // Explicit per-entity SISO-REF-010 override — set via the DIS Type
        // panel. Sent verbatim, bypassing all derivation logic above.
        snap.entityType.kind        = platform->disType.kind;
        snap.entityType.domain      = platform->disType.domain;
        snap.entityType.country     = platform->disType.country;
        snap.entityType.category    = platform->disType.category;
        snap.entityType.subcategory = platform->disType.subcategory;
        snap.entityType.specific    = platform->disType.specific;
        snap.entityType.extra       = platform->disType.extra;
    }

    Geocords& geo  = platform->transform.geocord;
    snap.latitude  = geo.latitude;
    snap.longitude = geo.longitude;
    snap.altitude  = geo.altitude;
    snap.heading   = static_cast<float>(geo.Heading);
    snap.pitch     = platform->transform.pitch();
    snap.roll      = platform->transform.roll();

    snap.northVel    = platform->dynamicModel.NorthVelocity;
    snap.eastVel     = platform->dynamicModel.EastVelocity;
    snap.verticalVel = platform->dynamicModel.VerticalVelocity;
    snap.deadReckoningAlgorithm = DISDeadReckoning::DRM_FPW;
    return true;
}
// =============================================================================
// applyTransformUpdates
// Drain incoming entity state queue
// Apply positions to matching entities in hierarchy
// =============================================================================
void DISNetworkPlugin::applyTransformUpdates()
{
    // ── Drain incoming PDU queue ──────────────────────────────────────────────
    {
        QMutexLocker lock(&m_transformQueueMutex);

        while (!m_transformQueue.isEmpty()) {
            DISIncomingTransform update = m_transformQueue.dequeue();
            if (!m_hierarchy) continue;

            QString entityKey = QString::fromStdString(update.entityID);
            auto it = m_hierarchy->Entities.find(update.entityID);

            if (it == m_hierarchy->Entities.end()) {

                // Already pending creation — another PDU arrived before hierarchy
                // confirmed the entity. Don't create a second ghost.
                if (m_pendingRemoteEntities.contains(entityKey)) {
                    m_remoteEntityTimestamps[entityKey] =
                        QDateTime::currentMSecsSinceEpoch();
                    continue;
                }

                // ── Ghost entity creation ─────────────────────────────────────────
                if (m_remoteEntitiesFolderID.isEmpty()) {
                    QString platformProfileID;
                    for (const auto& [key, profile] :
                         m_hierarchy->ProfileCategories) {
                        if (QString::fromStdString(profile->Name)
                                .contains("Platform")) {
                            platformProfileID = QString::fromStdString(key);
                            break;
                        }
                    }
                    if (!platformProfileID.isEmpty()) {
                        Folder* folder = m_hierarchy->addFolder(
                            platformProfileID, "Remote DIS Entities", true);
                        if (folder) {
                            m_remoteEntitiesFolderID =
                                QString::fromStdString(folder->ID);
                            scheduleSimulationResume();  // addFolder → status → pause
                        }
                    }
                }

                if (!m_remoteEntitiesFolderID.isEmpty()) {
                    QString entityName = update.marking.empty()
                    ? entityKey
                    : QString::fromStdString(update.marking).trimmed();
                    if (entityName.isEmpty()) entityName = entityKey;
                    m_pendingRemoteEntities.insert(entityKey);
                    m_remoteEntityTimestamps[entityKey] =
                        QDateTime::currentMSecsSinceEpoch();
                    m_remoteEntityParentIDs[entityKey] =
                        m_remoteEntitiesFolderID;
                    m_remoteEntityDomain[entityKey]      = update.domain;      // ← ADD THIS
                    m_remoteEntityCategory[entityKey]    = update.category;    // ← ADD THIS
                    if (m_hierarchy)
                        m_hierarchy->m_pendingRemoteDISEntityIDs.insert(entityKey);
                    if (Simulation::isPlay)
                        m_disTriggeredPause = true;
                    emit addRemoteDISEntity(
                        m_remoteEntitiesFolderID, entityKey, entityName, false);
                }
                continue;
            }

            // ── Entity exists ─────────────────────────────────────────────────
            Entity* entity = it->second;

            // GAP 3 FIX: Never update LOCAL entities from network PDUs
            if (!entity->isRemoteDISEntity) continue;

            // Platform* platform = dynamic_cast<Platform*>(entity);
            // if (!platform)  continue;

            // // GAP 2 FIX: Update DR state from received PDU
            // DeadReckoningState& drState = m_remoteEntityDRStates[entityKey];
            Platform* platform = dynamic_cast<Platform*>(entity);
            if (!platform)  continue;

            // ── Populate DynamicModel telemetry for UI display (EntityInfoDialog,
            // tactical display speed labels/tooltips, sensor closing-speed math) ──
            // updateDynamics()/Aircraft::FixedUpdate() never runs for remote
            // entities (guarded in Platform::update(), DynamicModel::Update(),
            // and Simulation::calculatePhysics() — all three unchanged by this),
            // so nothing else ever writes these fields for a remote entity.
            // This is a plain data copy of values already received over the
            // wire — it does not run physics and does not affect position,
            // which stays owned entirely by applyDeadReckoningToRemoteEntities().
            // update.northVel/eastVel/vertVel are already NED m/s (converted in
            // entitystatehandler.cpp before this point). Altitude needs feet→km,
            // matching Transform::setGeoCord()'s existing use of FTtoKM.
            float groundSpeedMs = std::sqrt(update.northVel * update.northVel +
                                            update.eastVel  * update.eastVel);
            platform->dynamicModel.currentSpeed     = groundSpeedMs * 3.6f;
            platform->dynamicModel.currentAltitude  = static_cast<float>(update.altitude) * FTtoKM;
            platform->dynamicModel.NorthVelocity    = update.northVel;
            platform->dynamicModel.EastVelocity     = update.eastVel;
            platform->dynamicModel.VerticalVelocity = update.vertVel;

            // GAP 2 FIX: Update DR state from received PDU
            DeadReckoningState& drState = m_remoteEntityDRStates[entityKey];

            ECEFPosition ecef = CoordConverter::geocordToECEF(
                update.latitude, update.longitude, update.altitude);

            // DISOrientation euler = CoordConverter::headingToEuler(
            //     update.heading, update.pitch, update.roll);
            DISOrientation euler = CoordConverter::headingToEuler(
                update.heading, update.pitch, update.roll,
                update.latitude, update.longitude);
            // ── DEBUG HEADING MYSTERY ─────────────────────────────────────────────────

            // ─────────────────────────────────────────────────────────────────────────

            DISVelocity vel = CoordConverter::nedToECEFVelocity(
                update.northVel, update.eastVel, update.vertVel,
                update.latitude, update.longitude);

            DeadReckoning::updateState(
                drState,
                ecef.x, ecef.y, ecef.z,
                vel.x, vel.y, vel.z,
                euler.psi, euler.theta, euler.phi,
                DRAlgorithm::FPW);

            m_remoteEntityTimestamps[entityKey] =
                QDateTime::currentMSecsSinceEpoch();
        }
    }

    // GAP 2 FIX: Apply DR every frame for smooth movement
    applyDeadReckoningToRemoteEntities();
}

void DISNetworkPlugin::applyDeadReckoningToRemoteEntities()
{
    if (!m_hierarchy) return;

    int64_t nowMs = DeadReckoning::millisecondsSinceEpoch();

    for (auto it = m_remoteEntityDRStates.begin();
         it != m_remoteEntityDRStates.end(); ++it)
    {
        const QString& entityKey = it.key();
        const DeadReckoningState& drState = it.value();

        if (!drState.initialized) continue;

        auto eit = m_hierarchy->Entities.find(entityKey.toStdString());
        if (eit == m_hierarchy->Entities.end()) continue;

        Entity* entity = eit->second;
        if (!entity->isRemoteDISEntity) continue;
        if (!entity->Active) continue;

        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform) continue;

        double dtSeconds =
            static_cast<double>(nowMs - drState.lastUpdateMs) / 1000.0;

        if (dtSeconds > 10.0) continue;

        PredictedState predicted = DeadReckoning::predict(drState, dtSeconds);

        platform->transform.setGeoCord(
            static_cast<float>(predicted.latitude),
            static_cast<float>(predicted.longitude),
            static_cast<float>(predicted.altitude),
            static_cast<float>(predicted.heading)
            );
    }
}
void DISNetworkPlugin::startRemoteEntityTimeoutChecker()
{
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(6000);  // check every 6 seconds
    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        int64_t now = QDateTime::currentMSecsSinceEpoch();
        QList<QString> toRemove;

        for (auto it = m_remoteEntityTimestamps.begin();
             it != m_remoteEntityTimestamps.end(); ++it) {
            int64_t age = now - it.value();
            if (age > 12000) {  // 12 seconds = 2x DIS heartbeat
                toRemove.append(it.key());
            }
        }

        for (const QString& entityKey : toRemove) {
            QString parentID = m_remoteEntityParentIDs.value(entityKey, "");

            DIS_LOG_BASIC("[DISNetworkPlugin] Remote entity timed out:"
                     << entityKey);

            // Clear pending state BEFORE emitting removal so that if
            // the entity returns and sends a new PDU immediately, the
            // ghost creation path is not blocked by a stale pending entry.
            m_pendingRemoteEntities.remove(entityKey);              // ADD
            if (m_hierarchy)
                m_hierarchy->m_pendingRemoteDISEntityIDs.remove(entityKey); // ADD

            // Remove from hierarchy
            emit removeRemoteDISEntity(parentID, entityKey, false);

            // Clear all tracking state so entity can reappear cleanly
            m_remoteEntityTimestamps.remove(entityKey);
            m_remoteEntityParentIDs.remove(entityKey);
            m_remoteEntityDRStates.remove(entityKey);
            m_remoteEntityDomain.remove(entityKey);     // ADD — refreshed on return
            m_remoteEntityCategory.remove(entityKey);   // ADD — refreshed on return
        }
        if (!toRemove.isEmpty())
            scheduleSimulationResume();
    });
    m_timeoutTimer->start();
}
// =============================================================================
// applyFireEvents
// Drain incoming fire event queue
// Log fire events, trigger any local responses
// =============================================================================
void DISNetworkPlugin::applyFireEvents()
{
    QMutexLocker lock(&m_fireQueueMutex);

    while (!m_fireQueue.isEmpty()) {
        DISIncomingFire event = m_fireQueue.dequeue();

        DIS_LOG_BASIC("[DISNetworkPlugin] Fire received:" << QString::fromStdString(event.firingEntityID));

        // TODO: Notify UI, trigger visual effect
        // For now just log
    }
}

// =============================================================================
// applyDetonationEvents
// Drain incoming detonation queue
// Apply damage to target entity if owned locally
// =============================================================================
void DISNetworkPlugin::applyDetonationEvents()
{
    QMutexLocker lock(&m_detonationQueueMutex);

    while (!m_detonationQueue.isEmpty()) {
        DISIncomingDetonation event = m_detonationQueue.dequeue();

        if (!m_hierarchy) continue;

        // Find target entity
        auto it = m_hierarchy->Entities.find(event.targetEntityID);
        if (it == m_hierarchy->Entities.end()) continue;

        Entity* entity = it->second;

        // Cast to Platform to access networkObject
        Platform* platform = dynamic_cast<Platform*>(entity);

        // Only apply damage if WE own this entity
        // if (platform && platform->networkObject
        //     && !platform->networkObject->isOwner) continue;
        // Never apply incoming detonation damage to remote entities
        // isRemoteDISEntity is reliable — networkObject may be null on local platforms
        if (entity->isRemoteDISEntity) continue;
        // Apply damage based on blast radius and result
        if (event.detonationResult == DISDetonationResult::EntityImpact ||
            event.detonationResult == DISDetonationResult::EntityProximateDetonation) {

            // Simple damage model — reduce health
            // More sophisticated model in future
            float damage = (event.blastRadius > 0.0f) ? 50.0f : 100.0f;
            entity->Health = qMax(0.0f, entity->Health - damage);

            DIS_LOG_BASIC("[DISNetworkPlugin] Detonation hit:" << QString::fromStdString(event.targetEntityID));

            if (entity->Health <= 0.0f) {
                entity->Active = false;
                DIS_LOG_WARNING("[DISNetworkPlugin] Entity destroyed:" << QString::fromStdString(event.targetEntityID));
            }
        }
    }
}

// =============================================================================
// applyExerciseEvents
// Drain exercise control queue
// Handle start/stop/pause from remote DIS nodes
// =============================================================================
void DISNetworkPlugin::applyExerciseEvents()
{
    QMutexLocker lock(&m_exerciseQueueMutex);
    while (!m_exerciseQueue.isEmpty()) {
        DISIncomingExercise event = m_exerciseQueue.dequeue();
        switch (event.type) {

        case DISIncomingExercise::StartResume: {
            if (!Simulation::isPlay) {
                bool receiveEnabled = true;
                for (const auto& cfg : m_config.pduConfigs) {
                    if (cfg.pduType == DISPDUType::StartResume) {
                        receiveEnabled = cfg.receive;
                        break;
                    }
                }
                if (receiveEnabled) {
                    DIS_LOG_BASIC("[DIS] Remote Start/Resume received — resuming");
                    emit _internalSimulationStart();
                }
            }
            break;
        }

        case DISIncomingExercise::StopFreeze: {
            if (Simulation::isPlay) {
                bool receiveEnabled = true;
                for (const auto& cfg : m_config.pduConfigs) {
                    if (cfg.pduType == DISPDUType::StopFreeze) {
                        receiveEnabled = cfg.receive;
                        break;
                    }
                }
                if (receiveEnabled) {
                    DIS_LOG_BASIC("[DIS] Remote Stop/Freeze received — pausing");
                    emit _internalSimulationPause();
                }
            }
            break;
        }

        }
    }
}


// =============================================================================
// Publish methods
// Called by bridge to send PDUs
// Emit internal signals → received by DISManager on network thread
// =============================================================================

void DISNetworkPlugin::publishEntityState(const DISEntitySnapshot& snap)
{
    emit _internalSendEntityState(snap);
}

void DISNetworkPlugin::publishFire(const DISFireSnapshot& snap)
{
    emit pduSent("Fire", QString::fromStdString(snap.firingEntityID));
    emit _internalSendFire(snap);
}

void DISNetworkPlugin::publishDetonation(const DISDetonationSnapshot& snap)
{
    emit pduSent("Detonation", QString::fromStdString(snap.munitionID));
    emit _internalSendDetonation(snap);
}

void DISNetworkPlugin::publishEmission(const DISEmissionSnapshot& snap)
{
    emit _internalSendEmission(snap);
}
void DISNetworkPlugin::publishDesignator(const DISDesignatorSnapshot& snap)
{
    emit _internalSendDesignator(snap);
}
void DISNetworkPlugin::publishStartResume(const DISStartResumeData& data)
{
    emit pduSent("Start/Resume", "");
    emit _internalSendStartResume(data);
}

void DISNetworkPlugin::publishStopFreeze(const DISStopFreezeData& data)
{
    emit pduSent("Stop/Freeze", "");
    emit _internalSendStopFreeze(data);
}

void DISNetworkPlugin::publishRemoveEntity(const DISRemoveEntityData& data)
{
    emit pduSent("RemoveEntity", QString::fromStdString(data.removedEntityID));
    emit _internalSendRemoveEntity(data);
}

void DISNetworkPlugin::publishCreateEntity(const DISCreateEntityData& data)
{
    emit pduSent("CreateEntity", QString::fromStdString(data.newEntityID));
    emit _internalSendCreateEntity(data);
}

// =============================================================================
// onMissileDetonated
// Connected to Weapon::missileDetonated signal
// Qt::QueuedConnection handles thread safety
// =============================================================================
void DISNetworkPlugin::onMissileDetonated(const QString& weaponID,
                                          double lat, double lon, double alt)
{
    if (!m_hierarchy) return;

    DISDetonationSnapshot snap;
    snap.munitionID  = weaponID.toStdString();
    snap.latitude    = lat;
    snap.longitude   = lon;
    snap.altitude    = alt;

    // Find the weapon to get target and firing entity
    auto it = m_hierarchy->Entities.find(weaponID.toStdString());
    if (it != m_hierarchy->Entities.end()) {
        Weapon* weapon = dynamic_cast<Weapon*>(it->second);
        if (weapon) {
            snap.blastRadius = weapon->blastRadius;
            if (weapon->parentEntity)
                snap.firingEntityID = weapon->parentEntity->ID;
            if (weapon->m_targetplatform)
                snap.targetEntityID = weapon->m_targetplatform->ID;

            // Determine result
            snap.detonationResult =
                snap.targetEntityID.empty()
                    ? DISDetonationResult::Detonation
                    : DISDetonationResult::EntityImpact;
        }
    }

    publishDetonation(snap);
}
void DISNetworkPlugin::onSimulationStarted()
{
    m_wasPaused = false;
    m_disTriggeredPause = false;

    DISStartResumeData data;
    publishStartResume(data);
    DIS_LOG_BASIC ("[DISNetworkPlugin] Start/Resume PDU sent");
}


void DISNetworkPlugin::onSimulationPaused()
{
    // GUARD: multiple pausef() calls fire sendMode(PAUSE) for one
    // logical pause (from multiple Hierarchy::status emissions during
    // entity creation). Only send ONE Stop/Freeze PDU per pause event.
    if (m_wasPaused) return;
    m_wasPaused = true;

    DISStopFreezeData data;
    data.reason = DISStopReason::Recess;
    publishStopFreeze(data);
    DIS_LOG_BASIC("[DISNetworkPlugin] Stop/Freeze PDU sent (pause)");
}
void DISNetworkPlugin::onSimulationStopped()
{
    m_wasPaused = true;
    DISStopFreezeData data;
    data.reason = DISStopReason::Termination;
    publishStopFreeze(data);
    DIS_LOG_BASIC("[DISNetworkPlugin] Stop/Freeze PDU sent (stop)");
}


// =============================================================================
// Hierarchy signal handlers
// =============================================================================
void DISNetworkPlugin::onEntityAdded(const QString& parentID,
                                     const QString& entityID,
                                     const QString& name)
{    /*if (m_simulationIsPlaying.load())
        m_disTriggeredPause = true;*/
    // Only set disTriggeredPause for remote DIS entities being integrated.
    // Setting it for ALL entity additions keeps it permanently true during
    // exercises, which blocks user-initiated pause PDUs from being sent.
    if (m_simulationIsPlaying.load() &&
        (m_pendingRemoteEntities.contains(entityID) || isRemoteEntity(entityID))) {
        m_disTriggeredPause = true;
    }
    Q_UNUSED(parentID) Q_UNUSED(name)

    DIS_LOG_FULL("[DIS onEntityAdded] parentID=" << parentID
             << "entityID=" << entityID << "name=" << name);  // ← ADD THIS
    // ── Remote entity integration path ────────────────────────────────────────
    if (m_pendingRemoteEntities.contains(entityID) || isRemoteEntity(entityID)) {
        auto it = m_hierarchy->Entities.find(entityID.toStdString());
        if (it != m_hierarchy->Entities.end()) {
            Entity* entity = it->second;
            entity->isRemoteDISEntity = true;

            Platform* platform = dynamic_cast<Platform*>(entity);
            if (platform) {
                platform->addComponent("transform");
                platform->addComponent("collider");
                platform->addComponent("bitmap");

                // ── Set correct sprite based on received SISO entity type ─────────────
                uint8_t domain   = m_remoteEntityDomain.value(entityID, 2);
                uint8_t category = m_remoteEntityCategory.value(entityID, 1);
                // if (platform->meshRenderer2d && platform->meshRenderer2d->Sprite) {
                //     std::string sprite = lookupSprite(domain, category);
                //     *platform->meshRenderer2d->Sprite = sprite;
                //     DIS_LOG_FULL("[DIS] Remote entity sprite:"
                //              << entityID
                //              << "domain=" << domain
                //              << "category=" << category
                //              << "->" << QString::fromStdString(sprite));
                // }

                // if (platform->collider) {
                //     float collideR = 200.0f;
                //     float warnR    = 400.0f;
                //     lookupCollider(domain, category, collideR, warnR);
                //     platform->collider->CollideRadius = collideR;
                //     platform->collider->WarningRadius = warnR;
                //     DIS_LOG_FULL("[DIS] Remote entity collider:"
                //              << entityID
                //              << "collide=" << collideR
                //              << "warn=" << warnR);
                // }
                // NEW
                if (platform->meshRenderer2d.Sprite) {
                    std::string sprite = lookupSprite(domain, category);
                    platform->meshRenderer2d.setSprite(sprite);
                }

                {
                    float collideR = 200.0f;
                    float warnR    = 400.0f;
                    lookupCollider(domain, category, collideR, warnR);
                    platform->collider.CollideRadius = collideR;
                    platform->collider.WarningRadius = warnR;
                }

                emit m_hierarchy->entityMeshAdded(
                    QString::fromStdString(entity->parentID), platform);

            }
        }
        m_pendingRemoteEntities.remove(entityID);
        if (m_hierarchy)
            m_hierarchy->m_pendingRemoteDISEntityIDs.remove(entityID);
        scheduleSimulationResume();
        DIS_LOG_BASIC("[DIS] Integrated remote entity:" << entityID);
        return;
    }

    // ── Pre-existing entity (was present when DIS started) ────────────────────
    if (m_preExistingEntityIDs.contains(entityID)) {
        m_preExistingEntityIDs.remove(entityID);
        return;
    }

    // ── New local entity — only send CreateEntity for Platforms ───────────────
    // Weapons, sensors, radios are sub-entities, not standalone DIS entities
    if (m_hierarchy) {
        auto it = m_hierarchy->Entities.find(entityID.toStdString());
        if (it != m_hierarchy->Entities.end()) {
            if (it->second->type != Constants::EntityType::Platform)
                return;
        }
    }

    // if (m_simulationIsPlaying.load()) {
    //     DISCreateEntityData data;
    //     data.newEntityID = entityID.toStdString();
    //     publishCreateEntity(data);
    // }
    if (m_simulationIsPlaying.load()) {
        m_disTriggeredPause = true;
        DISCreateEntityData data;
        data.newEntityID = entityID.toStdString();
        publishCreateEntity(data);
        QTimer::singleShot(300, this, [this]() {
            m_disTriggeredPause = false;
        });
    }
}


bool DISNetworkPlugin::isRemoteEntity(const QString& entityID) const
{
    return m_remoteEntityTimestamps.contains(entityID)
    || m_remoteEntityDRStates.contains(entityID);
}

void DISNetworkPlugin::onEntityRemoved(const QString& parentID,
                                       const QString& entityID,
                                       bool profile)
{
    if (m_simulationIsPlaying.load())
        m_disTriggeredPause = true;
    Q_UNUSED(parentID) Q_UNUSED(profile)
    if (!m_running.load()) return;

    m_destroyedEntityIDs.remove(entityID);
    emit _internalRemoveSnapshot(entityID);

    if (!isRemoteEntity(entityID)) {
        DISRemoveEntityData data;
        data.removedEntityID = entityID.toStdString();
        publishRemoveEntity(data);
    }
    // entityRemoved does NOT pause the sim — just clear the flag
    // so future Start/Resume and Stop/Freeze PDUs aren't suppressed
    QTimer::singleShot(100, this, [this]() {
        m_disTriggeredPause = false;
    });
}

// =============================================================================
// Incoming slot handlers
// Called via QueuedConnection from network thread → main thread
// Push to queues for applyPendingUpdates() to drain
// =============================================================================

void DISNetworkPlugin::onIncomingTransform(DISIncomingTransform update)
{
    //qDebug() << "[DISNetworkPlugin] INCOMING TRANSFORM:" << QString::fromStdString(update.entityID);
    // Stamp NOW — don't wait for applyTransformUpdates.
    // If sim is paused, applyPendingUpdates stops running and timestamps
    // freeze, causing false 12-second evictions of remote entities.
    m_remoteEntityTimestamps[QString::fromStdString(update.entityID)] =
        QDateTime::currentMSecsSinceEpoch();               // ← ADD
    QMutexLocker lock(&m_transformQueueMutex);
    m_transformQueue.enqueue(update);
}

void DISNetworkPlugin::onIncomingFire(DISIncomingFire event)
{
    QMutexLocker lock(&m_fireQueueMutex);
    m_fireQueue.enqueue(event);
}

void DISNetworkPlugin::onIncomingDetonation(DISIncomingDetonation event)
{
    QMutexLocker lock(&m_detonationQueueMutex);
    m_detonationQueue.enqueue(event);
}

void DISNetworkPlugin::onIncomingExercise(DISIncomingExercise event)
{
    QMutexLocker lock(&m_exerciseQueueMutex);
    m_exerciseQueue.enqueue(event);
}

void DISNetworkPlugin::onIncomingStatus(DISNetworkStatus status)
{
    {
        QMutexLocker lock(&m_statusMutex);
        m_status = status;
    }
    emit statusUpdated(status);
}

void DISNetworkPlugin::onIncomingError(QString error)
{
    DIS_LOG_WARNING("[DISNetworkPlugin] Error:" << error);
    emit errorOccurred(error);
}

void DISNetworkPlugin::onIncomingPeerJoined(QString addr, QString name, int version)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] Peer joined:" << addr << name << "DIS v" << version);
    emit peerJoined(addr, name, version);
}

void DISNetworkPlugin::onIncomingPeerLeft(QString addr)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] Peer left:" << addr);
    emit peerLeft(addr);
}

// =============================================================================
// connectSimulationSignals
// Connect to simulation lifecycle signals
// =============================================================================
void DISNetworkPlugin::connectSimulationSignals()
{
    disconnect(m_simulation, &Simulation::sendMode, this, nullptr);

    connect(m_simulation, &Simulation::sendMode,
            this, [this](SimulationStateNS::State state) {
                switch (state) {
                case SimulationStateNS::START:
                    DIS_LOG_FULL("[DIS sendMode] START m_wasPaused=" << m_wasPaused << "m_disTriggeredPause=" << m_disTriggeredPause);
                    m_simulationIsPlaying.store(true);
                    m_wasPaused = false;
                    if (!m_disTriggeredPause)
                        onSimulationStarted();
                    break;

                case SimulationStateNS::UPDATE:
                    m_simulationIsPlaying.store(true);
                    break;

                // case SimulationStateNS::PAUSE:
                //     DIS_LOG_FULL("[DIS sendMode] PAUSE m_wasPaused=" << m_wasPaused << "m_disTriggeredPause=" << m_disTriggeredPause);
                //     m_simulationIsPlaying.store(false);
                //     onSimulationPaused();  // m_wasPaused guard inside handles deduplication
                //     break;
                case SimulationStateNS::PAUSE:
                    DIS_LOG_FULL("[DIS sendMode] PAUSE m_wasPaused=" << m_wasPaused << "m_disTriggeredPause=" << m_disTriggeredPause);
                    m_simulationIsPlaying.store(false);
                    if (!m_disTriggeredPause)
                        onSimulationPaused();
                    break;
                case SimulationStateNS::STOP:
                    DIS_LOG_FULL("[DIS sendMode] STOP m_wasPaused=" << m_wasPaused << "m_disTriggeredPause=" << m_disTriggeredPause);
                    m_simulationIsPlaying.store(false);
                    break;

                case SimulationStateNS::REINITIALIZE:
                    DIS_LOG_FULL("[DIS sendMode] REINITIALIZE");
                    m_simulationIsPlaying.store(false);
                    break;

                default:
                    break;
                }
            },
            Qt::QueuedConnection);
}

// =============================================================================
// connectHierarchySignals
// Connect to hierarchy entity lifecycle signals
// =============================================================================
void DISNetworkPlugin::connectHierarchySignals()
{
    // connect(m_hierarchy, &Hierarchy::entityAdded,
    //         this, &DISNetworkPlugin::onEntityAdded,
    //         Qt::QueuedConnection);

    // Note: entityRemoved in hierarchy takes different params
    // Will align in bridge layer
}

// =============================================================================
// reloadConfig — called by DISUIBridge when user clicks Apply
// =============================================================================
void DISNetworkPlugin::reloadConfig(const DISConfig& newConfig)
{
    m_config = newConfig;
    emit _internalReloadConfig(newConfig);
    DIS_LOG_BASIC("[DISNetworkPlugin] Config reloaded");
}

// =============================================================================
// Getters for UI
// =============================================================================
DISConfig DISNetworkPlugin::currentConfig() const
{
    return m_config;
}

DISNetworkStatus DISNetworkPlugin::currentStatus() const
{
    QMutexLocker lock(&m_statusMutex);
    return m_status;
}
// =============================================================================
// scheduleSimulationResume
//
// ROOT CAUSE: runtime.cpp connects Hierarchy::status → simulation->pause()
// with QThread::msleep(200). Every DIS-triggered hierarchy operation fires
// status with no corresponding auto-resume (unlike local entity additions
// which have wasPlaying/startf logic in Simulation::entityAdded).
//
// WHY THE TIMER IS RELIABLE: msleep(200) blocks the main thread's event loop.
// No timer can fire during the block. When msleep ends, all queued pausef
// events process first (they were queued before our timer was set). By the
// time our timer fires, simulation is already paused and ready to resume.
// =============================================================================
void DISNetworkPlugin::scheduleSimulationResume()
{
    if (Simulation::isPlay)
        m_disTriggeredPause = true;

    if (!m_simulationIsPlaying.load()) return;

    if (m_resumeTimer) {
        m_resumeTimer->stop();
        m_resumeTimer->deleteLater();
        m_resumeTimer = nullptr;
    }

    m_resumeTimer = new QTimer(this);
    m_resumeTimer->setSingleShot(true);
    m_resumeTimer->setInterval(250);

    connect(m_resumeTimer, &QTimer::timeout, this, [this]() {
        m_resumeTimer->deleteLater();
        m_resumeTimer = nullptr;

        if (!m_simulation) return;
        if (!m_running.load()) return;
        if (!m_disTriggeredPause) return;

        if (Simulation::isPlay) {
            // Already playing — no start needed, just clear the flag
            QTimer::singleShot(50, this, [this]() {
                m_disTriggeredPause = false;
            });
            return;
        }

        DIS_LOG_BASIC("[DISNetworkPlugin] Resuming simulation after DIS hierarchy operation");
        m_simulation->start();

        QTimer::singleShot(50, this, [this]() {
            m_disTriggeredPause = false;
        });
    });

    m_resumeTimer->start();
}
bool DISNetworkPlugin::buildIFFSnapshot(Platform*       platform,
                                        DISIFFSnapshot& snap) const
{
    if (!platform) return false;
    // if (!platform->iffs || !platform->iffs->Active) return false;
    // if (!platform->iffs->iffs || platform->iffs->iffs->empty()) return false;

    // // Use the first active IFF transponder
    // IFF* iff = nullptr;
    // for (auto& pair : *platform->iffs->iffs) {
    if (!platform->iffs.Active) return false;
    if (platform->iffs.iffs.empty()) return false;

    IFF* iff = nullptr;
    for (auto& pair : platform->iffs.iffs) {
        if (pair.second &&
            pair.second->operationalMode == IFF::OperationalMode::Active) {
            iff = pair.second;
            break;
        }
    }
    if (!iff) return false;

    Entity* entity = platform;
    snap.entityID     = entity->ID;
    snap.systemOn     = (iff->operationalMode == IFF::OperationalMode::Active);
    snap.forceID      = DISNetworkBridge::teamToForceID(
        static_cast<int>(entity->team));
    snap.mode4Active  = (iff->encryptionType != IFF::EncryptionType::None);

    // Parse mode codes from string — stored as decimal digits
    auto parseCode = [](const std::string& s) -> uint16_t {
        try { return static_cast<uint16_t>(std::stoi(s)); }
        catch (...) { return 0; }
    };

    snap.mode1Code  = parseCode(iff->modeConfiguration.mode1);
    snap.mode2Code  = parseCode(iff->modeConfiguration.mode2);
    snap.mode3ACode = parseCode(iff->modeConfiguration.mode3A);
    snap.modeCCode  = parseCode(iff->modeConfiguration.modeC);

    snap.siteID       = static_cast<uint16_t>(m_config.siteID);
    snap.applicationID = static_cast<uint16_t>(m_config.applicationID);

    return true;
}
void DISNetworkPlugin::pushAllIFFSnapshots()
{
    if (!m_hierarchy) return;

    for (const auto& pair : m_hierarchy->Entities) {
        Entity* entity = pair.second;
        if (!entity || entity->isRemoteDISEntity) continue;

        Platform* platform = dynamic_cast<Platform*>(entity);
        if (!platform) continue;


        DISIFFSnapshot snap;
        if (buildIFFSnapshot(platform, snap))
            emit _internalPushIFFSnapshot(snap);
    }
}
void DISNetworkPlugin::applyIFFEvents()
{
    QMutexLocker lock(&m_iffQueueMutex);
    while (!m_iffQueue.isEmpty()) {
        DISIncomingIFF event = m_iffQueue.dequeue();

        DIS_LOG_BASIC("[DISNetworkPlugin] IFF received from:"
                      << QString::fromStdString(event.emittingEntityID)
                      << "Mode3A=" << event.mode3ACode
                      << "On=" << event.systemOn);

        // Future: apply received IFF codes to remote entity IFF component
    }
}
void DISNetworkPlugin::onIncomingIFF(DISIncomingIFF iff)
{
    QMutexLocker lock(&m_iffQueueMutex);
    m_iffQueue.enqueue(iff);
}
// =============================================================================
// onIncomingAcknowledge
// Called via QueuedConnection from AcknowledgeHandler on network thread
// Pushes to queue for drain timer to process on main thread
// =============================================================================
void DISNetworkPlugin::onIncomingAcknowledge(DISIncomingAcknowledge ack)
{
    QMutexLocker lock(&m_acknowledgeQueueMutex);
    m_acknowledgeQueue.enqueue(ack);
}

// =============================================================================
// applyAcknowledgeEvents
// Called by drain timer at 20Hz on main thread
// Logs incoming acknowledges
// Future: match requestID against pending request tracking map
// =============================================================================
void DISNetworkPlugin::applyAcknowledgeEvents()
{
    QMutexLocker lock(&m_acknowledgeQueueMutex);
    while (!m_acknowledgeQueue.isEmpty()) {
        DISIncomingAcknowledge ack = m_acknowledgeQueue.dequeue();

        const char* flagName = "Unknown";
        switch (ack.acknowledgeFlag) {
        case 1: flagName = "CreateEntity";      break;
        case 2: flagName = "RemoveEntity";      break;
        case 3: flagName = "StartResume";       break;
        case 4: flagName = "StopFreeze";        break;
        case 5: flagName = "TransferOwnership"; break;
        }

        const char* responseName = "Unknown";
        switch (ack.responseFlag) {
        case 0: responseName = "Other";           break;
        case 1: responseName = "AbleToComply";    break;
        case 2: responseName = "UnableToComply";  break;
        }

        DIS_LOG_BASIC("[DISNetworkPlugin] Acknowledge received:"
                      << "for="       << flagName
                      << "response="  << responseName
                      << "requestID=" << ack.requestID
                      << "from="      << QString::fromStdString(ack.originatingEntityID));

        // ── Handle TransferOwnership acknowledge ──────────────────────────────────
        if (ack.acknowledgeFlag == DISAcknowledgeFlag::TransferOwnership) {

            if (ack.responseFlag == DISResponseFlag::AbleToComply) {
                auto it = m_pendingTransfers.find(ack.requestID);


                if (it != m_pendingTransfers.end()) {
                    QString entityID = it.value();

                    m_pendingTransfers.erase(it);


                    // Flip to remote on main thread
                    if (m_hierarchy) {
                        auto eit = m_hierarchy->Entities.find(entityID.toStdString());
                        if (eit != m_hierarchy->Entities.end()) {
                            eit->second->isRemoteDISEntity = true;

                            // Stop trajectory — position now driven by DR from Machine B
                            // Platform* platform = dynamic_cast<Platform*>(eit->second);
                            // if (platform && platform->trajectory) {
                            //     platform->trajectory->Active = false;
                            //     platform->trajectory->Trajectories.clear();
                            // }
                            Platform* platform = dynamic_cast<Platform*>(eit->second);
                            if (platform) {
                                platform->trajectory.Active = false;
                                platform->trajectory.Trajectories.clear();
                            }
                        }
                        emit _internalGiveUpEntityOwnership(entityID);
                        DIS_LOG_BASIC("[DISNetworkPlugin] Ownership transferred — trajectory stopped:"
                                      << entityID);
                    }
                    DIS_LOG_BASIC("[DISNetworkPlugin] Ownership successfully transferred:"
                                  << entityID);
                }
            } else {
                DIS_LOG_WARNING("[DISNetworkPlugin] Transfer Ownership refused — UnableToComply"
                                << "requestID=" << ack.requestID);
                // Remove from pending — transfer cancelled
                m_pendingTransfers.remove(ack.requestID);
            }
        }

        // Future: if requestID is in pending map → mark as confirmed
    }
}
// =============================================================================
// onIncomingTransferOwnership
// Called via QueuedConnection from TransferOwnershipHandler on network thread
// =============================================================================
void DISNetworkPlugin::onIncomingTransferOwnership(DISIncomingTransferOwnership data)
{
    QMutexLocker lock(&m_transferQueueMutex);
    m_transferQueue.enqueue(data);
}

// =============================================================================
// applyTransferOwnershipEvents
// Called by drain timer at 20Hz on main thread.
// Takes ownership of the entity: flips isRemoteDISEntity, removes from
// remote tracking maps, tells network thread to update mapper.
// After this, pushAllLocalSnapshots() picks up the entity automatically
// on the next frame and PDUSender starts broadcasting EntityState for it.
// =============================================================================
void DISNetworkPlugin::applyTransferOwnershipEvents()
{
    // Re-entrancy guard — stays relevant: while a decision is awaiting
    // the UI's response, don't dequeue and emit a second request.
    if (m_transferDecisionInProgress) return;

    DISIncomingTransferOwnership data;
    bool haveOne = false;
    {
        QMutexLocker lock(&m_transferQueueMutex);
        if (m_transferQueue.isEmpty()) return;
        data = m_transferQueue.dequeue();
        haveOne = true;
    }
    if (!haveOne) return;

    if (!m_hierarchy) return;
    std::string engineID = data.transferEntityID;
    QString entityKey = QString::fromStdString(engineID);

    auto it = m_hierarchy->Entities.find(engineID);
    if (it == m_hierarchy->Entities.end()) {
        DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: entity not found:"
                        << entityKey);
        return;
    }
    Entity* entity = it->second;

    // ── Reject if we're already deciding on a transfer for this entity ──────
    if (m_awaitingTransferDecision.contains(entityKey)) {
        DISAcknowledgeData ack;
        ack.originSite      = data.originSite;
        ack.originApp       = data.originApp;
        ack.originEntity    = data.originEntity;
        ack.acknowledgeFlag = DISAcknowledgeFlag::TransferOwnership;
        ack.requestID       = data.requestID;
        ack.responseFlag    = DISResponseFlag::UnableToComply;
        emit _internalSendAcknowledgeDecision(ack);
        DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: rejected — "
                        "already awaiting decision for entity:" << entityKey);
        return;
    }

    m_awaitingTransferDecision.insert(entityKey);
    m_transferDecisionInProgress = true;
    m_pendingTransferDecisions[entityKey] = data;

    QString entityName = QString::fromStdString(entity->Name);
    emit transferOwnershipRequested(data.originSite, data.originApp,
                                    entityKey, entityName);
    // Execution returns here immediately. The UI will call
    // respondToTransferOwnership() once the person answers.
}

// =============================================================================
// respondToTransferOwnership
// Called by the UI (NetworkToolbar) after the person answers the
// accept/reject prompt shown in response to transferOwnershipRequested().
// Sends the Acknowledge with the chosen ResponseFlag, and on accept,
// runs the same ownership-taking logic that used to follow the dialog
// inline in applyTransferOwnershipEvents().
// =============================================================================
void DISNetworkPlugin::respondToTransferOwnership(const QString& entityKey, bool accept)
{
    auto pendingIt = m_pendingTransferDecisions.find(entityKey);
    if (pendingIt == m_pendingTransferDecisions.end()) {
        DIS_LOG_WARNING("[DISNetworkPlugin] respondToTransferOwnership: "
                        "no pending decision for entity:" << entityKey);
        return;
    }
    DISIncomingTransferOwnership data = pendingIt.value();
    m_pendingTransferDecisions.erase(pendingIt);

    m_transferDecisionInProgress = false;
    m_awaitingTransferDecision.remove(entityKey);

    DISAcknowledgeData ack;
    ack.originSite      = data.originSite;
    ack.originApp       = data.originApp;
    ack.originEntity    = data.originEntity;
    ack.acknowledgeFlag = DISAcknowledgeFlag::TransferOwnership;
    ack.requestID       = data.requestID;

    if (!accept) {
        ack.responseFlag = DISResponseFlag::UnableToComply;
        emit _internalSendAcknowledgeDecision(ack);
        DIS_LOG_BASIC("[DISNetworkPlugin] TransferOwnership: rejected by user:"
                      << entityKey);
        return;
    }

    if (!m_hierarchy) return;
    std::string engineID = data.transferEntityID;
    auto it = m_hierarchy->Entities.find(engineID);
    if (it == m_hierarchy->Entities.end()) {
        DIS_LOG_WARNING("[DISNetworkPlugin] respondToTransferOwnership: "
                        "entity vanished before accept could be applied:" << entityKey);
        ack.responseFlag = DISResponseFlag::UnableToComply;
        emit _internalSendAcknowledgeDecision(ack);
        return;
    }
    Entity* entity = it->second;

    ack.responseFlag = DISResponseFlag::AbleToComply;
    emit _internalSendAcknowledgeDecision(ack);

    // ── Take ownership — flip from remote to local ─────────────────────
    entity->isRemoteDISEntity = false;
    Platform* platform = dynamic_cast<Platform*>(entity);
    if (platform && m_remoteEntityDRStates.contains(entityKey)) {
        const DeadReckoningState& lastState = m_remoteEntityDRStates[entityKey];
        {
            double lat = 0.0, lon = 0.0, alt = 0.0;
            ECEFPosition ecef;
            ecef.x = lastState.posX;
            ecef.y = lastState.posY;
            ecef.z = lastState.posZ;
            CoordConverter::ecefToGeocord(ecef, lat, lon, alt);
            platform->transform.geocord.latitude  = lat;
            platform->transform.geocord.longitude = lon;
            platform->transform.geocord.altitude  = alt;

            DISOrientation euler;
            euler.psi   = lastState.psi;
            euler.theta = lastState.theta;
            euler.phi   = lastState.phi;
            float heading = 0.0f, pitch = 0.0f, roll = 0.0f;
            CoordConverter::eulerToHeading(euler, lat, lon, heading, pitch, roll);
            platform->transform.setHeading(heading);
        }
    }
    if (platform) {
        platform->addComponent("transform");
        platform->addComponent("crossSection");
        platform->addComponent("trajectory");
        platform->addComponent("rigidbody");
        platform->addComponent("dynamicModel");
        platform->addComponent("collider");
        platform->addComponent("bitmap");
        platform->addComponent("sensors");
        platform->addComponent("iffs");
        platform->addComponent("radios");
        platform->addComponent("weapons");
        DIS_LOG_BASIC("[DISNetworkPlugin] Components added for transferred entity:"
                      << entityKey);
    }
    if (m_simulation) {
        m_simulation->promoteToLocalPhysics(engineID);
    }

    m_remoteEntityTimestamps.remove(entityKey);
    m_remoteEntityDRStates.remove(entityKey);
    m_remoteEntityParentIDs.remove(entityKey);
    m_remoteEntityDomain.remove(entityKey);
    m_remoteEntityCategory.remove(entityKey);
    m_pendingRemoteEntities.remove(entityKey);

    emit _internalTakeEntityOwnership(entityKey);
    DIS_LOG_BASIC("[DISNetworkPlugin] Ownership taken — full control:"
                  << entityKey
                  << "from site=" << data.originSite
                  << "app="       << data.originApp);
}
// void DISNetworkPlugin::applyTransferOwnershipEvents()
// {
//     QMutexLocker lock(&m_transferQueueMutex);
//     while (!m_transferQueue.isEmpty()) {
//         DISIncomingTransferOwnership data = m_transferQueue.dequeue();

//         if (!m_hierarchy) continue;

//         std::string engineID = data.transferEntityID;

//         auto it = m_hierarchy->Entities.find(engineID);
//         if (it == m_hierarchy->Entities.end()) {
//             DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: entity not found:"
//                             << QString::fromStdString(engineID));
//             continue;
//         }

//         Entity* entity = it->second;

//         // ── Take ownership — flip from remote to local ─────────────────────
//         // ── Take ownership — flip from remote to local ─────────────────────
//         entity->isRemoteDISEntity = false;
//         QString entityKey = QString::fromStdString(engineID);
//         Platform* platform = dynamic_cast<Platform*>(entity);


//         if (platform && m_remoteEntityDRStates.contains(entityKey)) {
//             const DeadReckoningState& lastState = m_remoteEntityDRStates[entityKey];
//             {
//                 double lat = 0.0, lon = 0.0, alt = 0.0;
//                 ECEFPosition ecef;
//                 ecef.x = lastState.posX;
//                 ecef.y = lastState.posY;
//                 ecef.z = lastState.posZ;
//                 CoordConverter::ecefToGeocord(ecef, lat, lon, alt);
//                 platform->transform.geocord.latitude  = lat;
//                 platform->transform.geocord.longitude = lon;
//                 platform->transform.geocord.altitude  = alt;

//                 DISOrientation euler;
//                 euler.psi   = lastState.psi;
//                 euler.theta = lastState.theta;
//                 euler.phi   = lastState.phi;
//                 float heading = 0.0f, pitch = 0.0f, roll = 0.0f;
//                 //CoordConverter::eulerToHeading(euler, heading, pitch, roll);
//                 CoordConverter::eulerToHeading(euler, lat, lon, heading, pitch, roll);
//                 platform->transform.setHeading(heading);
//             }
//         }


//         if (platform) {
//             platform->addComponent("transform");
//             platform->addComponent("crossSection");
//             platform->addComponent("trajectory");
//             platform->addComponent("rigidbody");
//             platform->addComponent("dynamicModel");
//             platform->addComponent("collider");
//             platform->addComponent("bitmap");
//             platform->addComponent("sensors");
//             platform->addComponent("iffs");
//             platform->addComponent("radios");
//             platform->addComponent("weapons");
//             DIS_LOG_BASIC("[DISNetworkPlugin] Components added for transferred entity:"
//                           << entityKey);
//         }
//         if (m_simulation) {
//             m_simulation->promoteToLocalPhysics(engineID);
//         }
//         // ── Remove from all remote tracking maps ─────────────────────────
//         m_remoteEntityTimestamps.remove(entityKey);
//         m_remoteEntityDRStates.remove(entityKey);
//         m_remoteEntityParentIDs.remove(entityKey);
//         m_remoteEntityDomain.remove(entityKey);
//         m_remoteEntityCategory.remove(entityKey);
//         m_pendingRemoteEntities.remove(entityKey);

//         // ── Tell network thread to flip EntityIDMapper isLocal flag ───────
//         emit _internalTakeEntityOwnership(entityKey);

//         DIS_LOG_BASIC("[DISNetworkPlugin] Ownership taken — full control:"
//                       << entityKey
//                       << "from site=" << data.originSite
//                       << "app="       << data.originApp);

//     }
// }

// =============================================================================
// transferEntityOwnership
// Public method called from UI Transfer tab.
// Validates entity is locally owned, builds transfer data, sends PDU.
// =============================================================================
void DISNetworkPlugin::transferEntityOwnership(const QString& entityID,
                                               uint16_t targetSiteID,
                                               uint16_t targetAppID)
{
    if (!m_running.load()) return;
    if (!m_hierarchy) return;

    // Verify entity exists
    auto it = m_hierarchy->Entities.find(entityID.toStdString());
    if (it == m_hierarchy->Entities.end()) {
        DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: entity not found:"
                        << entityID);
        return;
    }

    // Verify we own it
    if (it->second->isRemoteDISEntity) {
        DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: entity is not local:"
                        << entityID);
        return;
    }

    // Build and send
    DISTransferOwnershipData data;
    data.transferEntityID = entityID.toStdString();
    data.receivingSiteID  = targetSiteID;
    data.receivingAppID   = targetAppID;
    data.transferType     = DISTransferType::ControllingAppRequestsTransfer;
    data.requestID        = m_nextTransferRequestID++;

    if (m_pendingTransfers.values().contains(entityID)) {
        DIS_LOG_WARNING("[DISNetworkPlugin] TransferOwnership: already pending for entity:"
                        << entityID);
        return;
    }
    // Track so we can process the Acknowledge
    m_pendingTransfers[data.requestID] = entityID;

    emit _internalSendTransferOwnership(data);

    DIS_LOG_BASIC("[DISNetworkPlugin] Transfer initiated"
                  << "entity=" << entityID
                  << "to site=" << targetSiteID
                  << "app="     << targetAppID
                  << "requestID=" << data.requestID);
}

// =============================================================================
// getLocalEntityList
// Returns {engineID, name} pairs for locally owned Platform entities.
// Used by Transfer tab to populate entity dropdown.
// =============================================================================
QList<QPair<QString,QString>> DISNetworkPlugin::getLocalEntityList() const
{
    QList<QPair<QString,QString>> result;
    if (!m_hierarchy) return result;

    for (const auto& pair : m_hierarchy->Entities) {
        Entity* entity = pair.second;
        if (!entity) continue;
        if (entity->isRemoteDISEntity) continue;
        if (entity->type != Constants::EntityType::Platform) continue;

        result.append({
            QString::fromStdString(entity->ID),
            QString::fromStdString(entity->Name)
        });
    }
    return result;
}
// =============================================================================
// onIncomingEmission
// Called via QueuedConnection from EmissionHandler on network thread.
// Logged only for now — no ESM/EW model exists yet to consume this data.
// Revisit once radar/sensor testing is far enough along to act on it.
// =============================================================================
void DISNetworkPlugin::onIncomingEmission(DISIncomingEmission data)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] Emission received from:"
                  << QString::fromStdString(data.emittingEntityID)
                  << "function=" << data.emitterFunction
                  << "beams="    << data.beams.size());
}
// =============================================================================
// onIncomingDesignator
// Called via QueuedConnection from DesignatorHandler on network thread.
// Logged only for now — no weapon-guidance consumer exists yet.
// =============================================================================
void DISNetworkPlugin::onIncomingDesignator(DISIncomingDesignator data)
{


    DIS_LOG_BASIC("[DISNetworkPlugin] Designator received from:"
                  << QString::fromStdString(data.designatingEntityID)
                  << "code=" << data.designatorCode
                  << "power=" << data.designatorPower);
}
void DISNetworkPlugin::publishTransmitter(const DISTransmitterSnapshot& snap)
{
    emit _internalSendTransmitter(snap);
}
void DISNetworkPlugin::publishSignal(const DISSignalSnapshot& snap)
{
    emit _internalSendSignal(snap);
}

void DISNetworkPlugin::onIncomingTransmitter(DISIncomingTransmitter data)
{

    qDebug()<<"[DISNetworkPlugin] Transmitter received from:"
                  << QString::fromStdString(data.transmittingEntityID)
                  << "radioID=" << data.radioID
                  << "state=" << data.transmitState
                  << "freq=" << data.frequency;
    DIS_LOG_BASIC("[DISNetworkPlugin] Transmitter received from:"
                  << QString::fromStdString(data.transmittingEntityID)
                  << "radioID=" << data.radioID
                  << "state=" << data.transmitState
                  << "freq=" << data.frequency);
}

void DISNetworkPlugin::onIncomingSignal(DISIncomingSignal data)
{
    qDebug()<<"[DISNetworkPlugin] Signal received from:"
                  << QString::fromStdString(data.transmittingEntityID)
                  << "radioID=" << data.radioID
                  << "bytes=" << data.data.size();
    DIS_LOG_BASIC("[DISNetworkPlugin] Signal received from:"
                  << QString::fromStdString(data.transmittingEntityID)
                  << "radioID=" << data.radioID
                  << "bytes=" << data.data.size());
}
void DISNetworkPlugin::publishDataQuery(const DISDatumSnapshot& snap) { emit _internalSendDataQuery(snap); }
void DISNetworkPlugin::publishSetData(const DISDatumSnapshot& snap)   { emit _internalSendSetData(snap); }
void DISNetworkPlugin::publishData(const DISDatumSnapshot& snap)      { emit _internalSendData(snap); }

void DISNetworkPlugin::onIncomingDataQuery(DISIncomingDatum data)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] DataQuery received from:"
                  << QString::fromStdString(data.originatingEntityID)
                  << "requestID=" << data.requestID
                  << "fixed=" << data.fixedDatums.size()
                  << "variable=" << data.variableDatums.size());

    if (!m_hierarchy) return;
    if (data.receivingEntityID.empty()) return; // no wildcard/broadcast-query handling yet

    auto it = m_hierarchy->Entities.find(data.receivingEntityID);
    if (it == m_hierarchy->Entities.end()) return;
    Platform* platform = dynamic_cast<Platform*>(it->second);
    if (!platform) return;

    DISDatumSnapshot response;
    response.originatingEntityID = data.receivingEntityID;   // we're answering, so we're the originator now
    response.receivingEntityID   = data.originatingEntityID; // back to whoever asked
    response.requestID           = data.requestID;           // ties response to query — IEEE 1278.1 §5.3.6.10

    for (const DISFixedDatumEntry& req : data.fixedDatums) {
        uint32_t value = 0;
        if (DISDatumRegistry::getFixed(req.datumID, platform,
                                       static_cast<uint32_t>(Simulation::simulationTime), value))
            response.fixedDatums.push_back({req.datumID, value});
    }
    for (const DISVariableDatumEntry& req : data.variableDatums) {
        QByteArray value;
        if (DISDatumRegistry::getVariable(req.datumID, platform, m_hierarchy, value))
            response.variableDatums.push_back({req.datumID, value});
    }

    if (response.fixedDatums.empty() && response.variableDatums.empty()) return;

    publishData(response); // §5.3.6.10: a Data Query is answered with a Data PDU

    // Non-zero Time Interval = standing subscription (§5.3.6.8): keep
    // answering this query at that cadence. Drained by the existing
    // m_drainTimer alongside every other periodic event — no new timer.
    if (data.timeInterval > 0)
        m_dataQuerySubscriptions[data.requestID] = { data, QDateTime::currentMSecsSinceEpoch() };
    else
        m_dataQuerySubscriptions.remove(data.requestID);
}

void DISNetworkPlugin::onIncomingSetData(DISIncomingDatum data)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] SetData received from:"
                  << QString::fromStdString(data.originatingEntityID)
                  << "requestID=" << data.requestID
                  << "fixed=" << data.fixedDatums.size()
                  << "variable=" << data.variableDatums.size());

    if (!m_hierarchy) return;
    if (data.receivingEntityID.empty()) return;

    auto it = m_hierarchy->Entities.find(data.receivingEntityID);
    if (it == m_hierarchy->Entities.end()) return;
    Platform* platform = dynamic_cast<Platform*>(it->second);
    if (!platform) return;

    DISDatumSnapshot response;
    response.originatingEntityID = data.receivingEntityID;
    response.receivingEntityID   = data.originatingEntityID;
    response.requestID           = data.requestID;

    for (const DISFixedDatumEntry& req : data.fixedDatums) {
        DISDatumRegistry::setFixed(req.datumID, platform, req.datumValue);
        uint32_t current = 0;
        if (DISDatumRegistry::getFixed(req.datumID, platform,
                                       static_cast<uint32_t>(Simulation::simulationTime), current))
            response.fixedDatums.push_back({req.datumID, current});
    }
    for (const DISVariableDatumEntry& req : data.variableDatums) {
        QByteArray current;
        if (DISDatumRegistry::getVariable(req.datumID, platform, m_hierarchy, current))
            response.variableDatums.push_back({req.datumID, current});
    }

    publishData(response); // §5.3.6.10: Set Data is always answered with a Data PDU
}
// =============================================================================
// applyDataQuerySubscriptions
// Called by the 20Hz drain timer. Resends the Data PDU answer for every
// standing (non-zero Time Interval) DataQuery, at its requested cadence.
// =============================================================================
void DISNetworkPlugin::applyDataQuerySubscriptions()
{
    if (m_dataQuerySubscriptions.isEmpty() || !m_hierarchy) return;

    int64_t nowMs = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_dataQuerySubscriptions.begin(); it != m_dataQuerySubscriptions.end(); ++it) {
        DataQuerySubscription& sub = it.value();

        // NOTE: Time Interval unit assumed to be seconds (common DIS field
        // convention) — not independently confirmed against the standard
        // this session. Adjust here if a real peer test shows otherwise.
        int64_t intervalMs = static_cast<int64_t>(sub.request.timeInterval) * 1000;
        if (intervalMs <= 0 || (nowMs - sub.lastSentMs) < intervalMs) continue;

        auto eit = m_hierarchy->Entities.find(sub.request.receivingEntityID);
        if (eit == m_hierarchy->Entities.end()) continue;
        Platform* platform = dynamic_cast<Platform*>(eit->second);
        if (!platform) continue;

        DISDatumSnapshot response;
        response.originatingEntityID = sub.request.receivingEntityID;
        response.receivingEntityID   = sub.request.originatingEntityID;
        response.requestID           = sub.request.requestID;

        for (const DISFixedDatumEntry& req : sub.request.fixedDatums) {
            uint32_t value = 0;
            if (DISDatumRegistry::getFixed(req.datumID, platform,
                                           static_cast<uint32_t>(Simulation::simulationTime), value))
                response.fixedDatums.push_back({req.datumID, value});
        }
        for (const DISVariableDatumEntry& req : sub.request.variableDatums) {
            QByteArray value;
            if (DISDatumRegistry::getVariable(req.datumID, platform, m_hierarchy, value))
                response.variableDatums.push_back({req.datumID, value});
        }

        if (!response.fixedDatums.empty() || !response.variableDatums.empty())
            publishData(response);

        sub.lastSentMs = nowMs;
    }
}
// void DISNetworkPlugin::onIncomingDataQuery(DISIncomingDatum data)
// {

//     // Logged only for now — a real implementation would inspect
//     // data.fixedDatums/variableDatums for which fields were requested and
//     // respond with a Data PDU carrying the actual values. That response
//     // logic doesn't exist yet; deferred until a concrete DRDO query need
//     // is known (same "wire proven, consumption deferred" scope as Stage 2).
//     DIS_LOG_BASIC("[DISNetworkPlugin] DataQuery received from:"
//                   << QString::fromStdString(data.originatingEntityID)
//                   << "requestID=" << data.requestID
//                   << "fixed=" << data.fixedDatums.size()
//                   << "variable=" << data.variableDatums.size());
// }

// void DISNetworkPlugin::onIncomingSetData(DISIncomingDatum data)
// {

//     DIS_LOG_BASIC("[DISNetworkPlugin] SetData received from:"
//                   << QString::fromStdString(data.originatingEntityID)
//                   << "requestID=" << data.requestID
//                   << "fixed=" << data.fixedDatums.size()
//                   << "variable=" << data.variableDatums.size());
// }
// =============================================================================
// getRemoteEntityList
// Returns {engineID, name} pairs for known remote Platform entities.
// Used by the Data Exchange tab so a target can be picked, not typed blind.
// =============================================================================
QList<QPair<QString,QString>> DISNetworkPlugin::getRemoteEntityList() const
{
    QList<QPair<QString,QString>> result;
    if (!m_hierarchy) return result;

    for (const auto& pair : m_hierarchy->Entities) {
        Entity* entity = pair.second;
        if (!entity) continue;
        if (!entity->isRemoteDISEntity) continue;
        if (entity->type != Constants::EntityType::Platform) continue;

        result.append({
            QString::fromStdString(entity->ID),
            QString::fromStdString(entity->Name)
        });
    }
    return result;
}

// =============================================================================
// describeDatumResponse
// Decodes a Data PDU into a human-readable line for the Data Exchange tab's
// response log. Mirrors DISDatumRegistry's own Datum IDs and layouts exactly.
// =============================================================================
QString DISNetworkPlugin::describeDatumResponse(const DISIncomingDatum& data) const
{
    static const char* roiNames[7] = {
        "HOLD_FIRE", "RETURN_FIRE_ONLY", "DEFENSIVE_ONLY", "FIRE_ON_DETECTION",
        "FIRE_ON_IDENTIFICATION", "FREE_FIRE", "COMMAND_AUTHORIZATION_REQUIRED"
    };

    QStringList parts;
    parts << QString("RequestID=%1 from=%2")
                 .arg(data.requestID)
                 .arg(QString::fromStdString(data.originatingEntityID));

    for (const DISFixedDatumEntry& f : data.fixedDatums) {
        switch (f.datumID) {
        case DISDatumID::WeaponSystemStatus:
            parts << QString("WeaponSystemStatus: %1 armed").arg(f.datumValue);
            break;
        case DISDatumID::EngagementData: {
            QString name = (f.datumValue < 7) ? roiNames[f.datumValue] : "Unknown";
            parts << QString("EngagementData/ROI: %1 (%2)").arg(name).arg(f.datumValue);
            break;
        }
        case DISDatumID::TimeSynchronization:
            parts << QString("TimeSync: %1 s").arg(f.datumValue);
            break;
        default:
            parts << QString("FixedDatum[%1]=%2").arg(f.datumID).arg(f.datumValue);
        }
    }

    for (const DISVariableDatumEntry& v : data.variableDatums) {
        switch (v.datumID) {
        case DISDatumID::EntityTrackIdentity:
            parts << QString("Identity: %1").arg(QString::fromUtf8(v.data));
            break;
        case DISDatumID::EnvironmentalData: {
            QDataStream ds(v.data);
            float temperatureC = 0, humidityPct = 0, rainRateMmph = 0, fogVisibilityM = 0;
            ds >> temperatureC >> humidityPct >> rainRateMmph >> fogVisibilityM;
            parts << QString("Environmental: %1°C, %2%% humidity, %3 mm/hr rain, %4 m visibility")
                         .arg(temperatureC, 0, 'f', 1)
                         .arg(humidityPct, 0, 'f', 1)
                         .arg(rainRateMmph, 0, 'f', 1)
                         .arg(fogVisibilityM, 0, 'f', 0);
            break;
        }
        default:
            parts << QString("VariableDatum[%1]= %2 bytes").arg(v.datumID).arg(v.data.size());
        }
    }

    return parts.join(" | ");
}

// Replace the existing onIncomingData() body:
void DISNetworkPlugin::onIncomingData(DISIncomingDatum data)
{
    DIS_LOG_BASIC("[DISNetworkPlugin] Data received from:"
                  << QString::fromStdString(data.originatingEntityID)
                  << "requestID=" << data.requestID
                  << "fixed=" << data.fixedDatums.size()
                  << "variable=" << data.variableDatums.size());

    emit dataResponseReceived(describeDatumResponse(data));
}
// void DISNetworkPlugin::onIncomingData(DISIncomingDatum data)
// {

//     DIS_LOG_BASIC("[DISNetworkPlugin] Data received from:"
//                   << QString::fromStdString(data.originatingEntityID)
//                   << "requestID=" << data.requestID
//                   << "fixed=" << data.fixedDatums.size()
//                   << "variable=" << data.variableDatums.size());
// }
