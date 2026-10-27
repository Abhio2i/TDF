// =============================================================================
// FILE:        DISNetworkPlugin.h
// MODULE:      DIS Network Plugin
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: The single entry point for all DIS network functionality.
//              Drop this plugin into any TDF-compatible engine with 4 lines.
//              Manages its own thread internally.
//              Engine never touches internal DIS classes directly.
//
// USAGE (in your main setup):
//   DISNetworkPlugin* dis = new DISNetworkPlugin();
//   dis->attachHierarchy(hierarchy);
//   dis->attachSimulation(simulation);
//   dis->start("dis_config.json");
//   simulation->registerPlugin(dis);
//
// THREAD MODEL:
//   Main thread  → applyPendingUpdates() drains incoming queues
//   DIS thread   → sends EntityState PDUs at 5Hz
//                  receives incoming PDUs
//                  pushes results to thread-safe queues
//
// SIGNALS (connect UI to these):
//   statusUpdated()    → TX/RX rates, peer count, entity count
//   errorOccurred()    → network errors for UI display
//   peerJoined/Left()  → peer list updates for UI
// =============================================================================

#ifndef DISNETWORKPLUGIN_H
#define DISNETWORKPLUGIN_H

#include "SimulationPlugin.h"
#include "interface/DISConfig.h"
#include "interface/DISEntitySnapshot.h"
#include "interface/DISFireSnapshot.h"
#include "interface/DISDetonationSnapshot.h"
#include "interface/DISEmissionSnapshot.h"
#include "interface/DISExerciseControl.h"
#include "utils/deadreckoning.h"
#include "utils/coordconverter.h"
#include "core/DISPlugin/interface/DISIFFSnapshot.h"
#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QHostAddress>
#include <atomic>
#include "interface/disincomingdata.h"
#include <QSet>
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/Components/dynamicmodel.h"
#include "core/Hierarchy/Struct/geocords.h"
#include "interface/DISIncomingEmission.h"
#include "interface/DISDesignatorSnapshot.h"
#include "interface/DISIncomingDesignator.h"
#include "interface/DISTransmitterSnapshot.h"
#include "interface/DISIncomingTransmitter.h"
#include "interface/DISSignalSnapshot.h"
#include "interface/DISIncomingSignal.h"
#include "interface/DISDatumSnapshot.h"
#include "interface/DISIncomingDatum.h"
#include "core/DISPlugin/utils/disdatumregistry.h"   // new include
struct DataQuerySubscription {                        // new struct
    DISIncomingDatum request;
    int64_t lastSentMs = 0;
};
// Forward declarations — internal classes, engine never sees these
class DISManager;
class DISNetworkBridge;
class Simulation;
class Hierarchy;

// =============================================================================
// DISNetworkStatus
// Runtime status emitted every second to UI
// =============================================================================
struct DISNetworkStatus {
    bool    connected       = false;
    int     pdusSentPerSec  = 0;
    int     pdusRecvPerSec  = 0;
    int     activePeers     = 0;
    int     landEntities    = 0;
    int     airEntities     = 0;
    int     navalEntities   = 0;
    int     subEntities     = 0;
    QString lastError       = "";
};
Q_DECLARE_METATYPE(DISNetworkStatus)

class DISNetworkPlugin : public SimulationPlugin {
    Q_OBJECT

public:
    explicit DISNetworkPlugin(QObject* parent = nullptr);
    ~DISNetworkPlugin() override;

    // ── SimulationPlugin interface ────────────────────────────────────────────
    void    attachHierarchy (Hierarchy*   hierarchy)  override;
    void    attachSimulation(Simulation*  simulation) override;
    void    start           (const QString& configPath) override;
    void    stop            ()                          override;
    void    applyPendingUpdates()                       override;
    QString pluginName      () const override { return "DISNetworkPlugin"; }
    bool    isRunning       () const override { return m_running.load(); }

    // ── Manual PDU publish (called by bridge, not by engine directly) ─────────
    void publishEntityState (const DISEntitySnapshot&     snap);
    void publishFire        (const DISFireSnapshot&       snap);
    void publishDetonation  (const DISDetonationSnapshot& snap);
    void publishEmission    (const DISEmissionSnapshot&   snap);
    void publishStartResume (const DISStartResumeData&    data);
    void publishStopFreeze  (const DISStopFreezeData&     data);
    void publishRemoveEntity(const DISRemoveEntityData&   data);
    void publishCreateEntity(const DISCreateEntityData&   data);
    void publishDesignator(const DISDesignatorSnapshot& snap);
    void publishTransmitter(const DISTransmitterSnapshot& snap);
    void publishSignal(const DISSignalSnapshot& snap);
    void publishDataQuery(const DISDatumSnapshot& snap);
    void publishSetData(const DISDatumSnapshot& snap);
    void publishData(const DISDatumSnapshot& snap);
    // Called by DISManager to check before registering an entity as local
    bool isRemoteEntity(const QString& entityID) const;
    // ── Config reload (from UI via DISUIBridge) ────────────────────────────────
    void reloadConfig(const DISConfig& newConfig);

    // ── Read current config (for UI) ──────────────────────────────────────────
    DISConfig       currentConfig()  const;
    DISNetworkStatus currentStatus() const;
    // Initiate transfer — called from UI Transfer tab
    // entityID: engine ID of locally owned entity
    // targetSiteID/targetAppID: the receiving machine's DIS address
    void transferEntityOwnership(const QString& entityID,
                                 uint16_t targetSiteID,
                                 uint16_t targetAppID);
    // Called by UI after the person answers the accept/reject prompt shown
    // in response to transferOwnershipRequested(). entityKey must match the
    // value passed in that signal.
    void respondToTransferOwnership(const QString& entityKey, bool accept);
    // Returns list of {engineID, name} pairs for locally owned entities
    // Used by Transfer tab to populate entity dropdown
    QList<QPair<QString,QString>> getLocalEntityList() const;
    QList<QPair<QString,QString>> getRemoteEntityList() const;

public slots:
    // ── Connected to Weapon::missileDetonated signal ──────────────────────────
    // Qt::QueuedConnection handles thread safety automatically
    void onMissileDetonated(const QString& weaponID,
                            double lat, double lon, double alt);

    // ── Connected to Simulation signals ──────────────────────────────────────
    void onSimulationStarted();
    void onSimulationStopped();
    void onSimulationPaused();

    // ── Connected to Hierarchy signals ────────────────────────────────────────
    void onEntityAdded  (const QString& parentID, const QString& entityID,
                       const QString& name);
    void onEntityRemoved(const QString& parentID, const QString& entityID,
                         bool profile);
    void onIncomingTransform   (DISIncomingTransform   update);
    void onIncomingFire        (DISIncomingFire        event);
    void onIncomingDetonation  (DISIncomingDetonation  event);
    void onIncomingExercise    (DISIncomingExercise    event);
    void onIncomingIFF(DISIncomingIFF iff);
    void onIncomingAcknowledge(DISIncomingAcknowledge ack);
    void onIncomingTransferOwnership(DISIncomingTransferOwnership data);
    void onIncomingEmission(DISIncomingEmission data);
    void onIncomingDesignator(DISIncomingDesignator data);
    void onIncomingTransmitter(DISIncomingTransmitter data);
    void onIncomingSignal(DISIncomingSignal data);
    void onIncomingDataQuery(DISIncomingDatum data);
    void onIncomingSetData(DISIncomingDatum data);
    void onIncomingData(DISIncomingDatum data);
signals:
    // ── Status signals (connect UI to these) ──────────────────────────────────
    void statusUpdated    (DISNetworkStatus status);
    void errorOccurred    (QString errorMessage);
    void connectionChanged(bool connected);

    // ── Peer events ───────────────────────────────────────────────────────────
    void peerJoined(QString address, QString name, int disVersion);
    void peerLeft  (QString address);

    // ── PDU log (for UI PDU monitor) ──────────────────────────────────────────
    void pduSent    (QString pduType, QString entityID);
    void pduReceived(QString pduType, QString sourceAddr);

    // ── Internal signals (engine to DIS thread via QueuedConnection) ──────────
    // These are emitted by plugin, received by DISManager on network thread
    void _internalSendEntityState (DISEntitySnapshot     snap);
    void _internalSendFire        (DISFireSnapshot        snap);
    void _internalSendDetonation  (DISDetonationSnapshot  snap);
    void _internalSendEmission    (DISEmissionSnapshot    snap);
    void _internalSendStartResume (DISStartResumeData     data);
    void _internalSendStopFreeze  (DISStopFreezeData      data);
    void _internalSendRemoveEntity(DISRemoveEntityData    data);
    void _internalSendCreateEntity(DISCreateEntityData    data);
    void _internalReloadConfig    (DISConfig              config);
    void _internalPushSnapshot    (DISEntitySnapshot snap);
    void _internalRemoveSnapshot  (QString           entityID);
    void _internalSimulationStart ();
    void _internalSimulationPause ();
    void _internalSimulationStop  ();
    void addRemoteDISEntity(QString parentID, QString entityID,
                            QString entityName, bool profile);
    void removeRemoteDISEntity(QString parentID, QString entityID, bool profile);
    void _internalPushIFFSnapshot(DISIFFSnapshot snap);
    void _internalSendTransferOwnership (DISTransferOwnershipData data);
    void _internalTakeEntityOwnership   (QString entityID);
    void _internalSendDesignator(DISDesignatorSnapshot snap);
    void _internalGiveUpEntityOwnership (QString entityID);
    void _internalSendTransmitter(DISTransmitterSnapshot snap);
    void _internalSendSignal(DISSignalSnapshot snap);
    void _internalSendDataQuery(DISDatumSnapshot snap);
    void _internalSendSetData(DISDatumSnapshot snap);
    void _internalSendData(DISDatumSnapshot snap);
    void _internalSendAcknowledgeDecision(DISAcknowledgeData ack);
    void transferOwnershipRequested(uint16_t originSite, uint16_t originApp,
                                    QString entityKey, QString entityName);
    void dataResponseReceived(QString summary);

private slots:
    // ── Received FROM DISManager (network thread → main thread) ───────────────

    void onIncomingStatus      (DISNetworkStatus       status);
    void onIncomingError       (QString                error);
    void onIncomingPeerJoined  (QString addr, QString name, int version);
    void onIncomingPeerLeft    (QString addr);

private:
    // ── Engine references ─────────────────────────────────────────────────────
    Hierarchy*   m_hierarchy   = nullptr;
    Simulation*  m_simulation  = nullptr;
    QSet<QString> m_pendingRemoteEntities;
    // ── DIS internals ────────────────────────────────────────────────────────
    QThread*     m_disThread   = nullptr;
    DISManager*  m_disManager  = nullptr;
    DISNetworkBridge* m_bridge = nullptr;

    // ── Config ───────────────────────────────────────────────────────────────
    DISConfig    m_config;
    QString      m_configPath;

    // ── State ────────────────────────────────────────────────────────────────
    std::atomic<bool> m_running{false};
    DISNetworkStatus  m_status;
    mutable QMutex    m_statusMutex;

    // ── Thread-safe incoming queues ───────────────────────────────────────────
    // DIS thread pushes → main thread drains in applyPendingUpdates()
    QMutex                       m_transformQueueMutex;
    QQueue<DISIncomingTransform> m_transformQueue;

    QMutex                   m_fireQueueMutex;
    QQueue<DISIncomingFire>  m_fireQueue;

    QMutex                        m_detonationQueueMutex;
    QQueue<DISIncomingDetonation> m_detonationQueue;

    QMutex                      m_exerciseQueueMutex;
    QQueue<DISIncomingExercise> m_exerciseQueue;
    bool m_signalsConnected = false;
    std::atomic<bool> m_simulationIsPlaying{false};
    QSet<QString>     m_destroyedEntityIDs;
    bool              m_wasPaused = false;      // ← ADD THIS
    bool m_disTriggeredPause = false;

    bool buildIFFSnapshot(Platform* platform, DISIFFSnapshot& snap) const;
    void applyIFFEvents();
    void applyAcknowledgeEvents();
    void applyDataQuerySubscriptions();
    void pushAllIFFSnapshots();
    void pushAllLocalSnapshots();
    bool buildEntitySnapshot(Platform* platform, DISEntitySnapshot& snap) const;
    // ── Internal helpers ─────────────────────────────────────────────────────
    void connectSimulationSignals();
    void connectHierarchySignals();
    void applyTransformUpdates();
    void applyFireEvents();
    void applyDetonationEvents();
    void applyExerciseEvents();
    void startRemoteEntityTimeoutChecker();
    void applyDeadReckoningToRemoteEntities();
    QMap<QString, DeadReckoningState> m_remoteEntityDRStates;
    QMap<QString, int64_t> m_remoteEntityTimestamps;
    QMap<QString, QString> m_remoteEntityParentIDs;
    QString m_remoteEntitiesFolderID;
    QTimer* m_timeoutTimer = nullptr;
    QTimer* m_drainTimer    = nullptr;
     QSet<QString>     m_preExistingEntityIDs;
    void scheduleSimulationResume();
    QTimer* m_resumeTimer = nullptr;
    QMap<QString, uint8_t> m_remoteEntityDomain;
    QMap<QString, uint8_t> m_remoteEntityCategory;
    // ── Entity Type Registry ──────────────────────────────────────────────────
    struct DISEntityTypeProfile {
        uint8_t     domain      = 0;
        uint8_t     category    = 0;
        std::string spritePath;
        float       collideRadius = 200.0f;
        float       warningRadius = 400.0f;
    };
    QList<DISEntityTypeProfile>  m_entityTypeRegistry;
    void        buildEntityTypeRegistry();
    std::string lookupSprite  (uint8_t domain, uint8_t category) const;
    void        lookupCollider(uint8_t domain, uint8_t category,
                        float& collideRadius, float& warningRadius) const;
    QMutex                   m_iffQueueMutex;
    QQueue<DISIncomingIFF>   m_iffQueue;
    QMutex                           m_acknowledgeQueueMutex;
    QQueue<DISIncomingAcknowledge>   m_acknowledgeQueue;
    QMutex                               m_transferQueueMutex;
    QQueue<DISIncomingTransferOwnership> m_transferQueue;
    QMap<uint32_t, QString>              m_pendingTransfers;   // requestID → entityID
    uint32_t                             m_nextTransferRequestID = 1;

    void applyTransferOwnershipEvents();
    QString describeDatumResponse(const DISIncomingDatum& data) const;
    QSet<QString> m_awaitingTransferDecision;
    bool m_transferDecisionInProgress = false;
    // Holds the in-flight request data between transferOwnershipRequested()
    // being emitted and respondToTransferOwnership() being called back.
    // Keyed by entityKey — only one entry at a time in practice, since
    // applyTransferOwnershipEvents() won't dequeue a new item while
    // m_transferDecisionInProgress is true.
    QMap<QString, DISIncomingTransferOwnership> m_pendingTransferDecisions;
    QMap<uint32_t, DataQuerySubscription> m_dataQuerySubscriptions;

};

#endif // DISNETWORKPLUGIN_H
