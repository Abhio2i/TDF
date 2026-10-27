// =============================================================================
// FILE:        DISManager.h
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: The central manager that owns and connects all DIS components.
//              Lives entirely on the DIS network thread.
//              Created and moved to thread by DISNetworkPlugin.
//
// OWNS:
//   DISTransport        — UDP socket
//   PDUDispatcher       — routes incoming bytes
//   PDUSender           — sends at 5Hz
//   PDUSerializer       — builds PDU bytes (static, no instance)
//   EntityStateHandler  — processes incoming EntityState
//   FireHandler         — processes incoming Fire
//   DetonationHandler   — processes incoming Detonation
//   ExerciseHandler     — processes Start/Stop/Remove/Create
//   DISNetworkBridge    — reads hierarchy, builds snapshots
//   EntityIDMapper      — entity ID mapping
//   PeerVersionRegistry — peer DIS version tracking
//
// WIRING (all on network thread):
//   DISTransport → PDUDispatcher → Handlers → DISNetworkPlugin queues
//   DISNetworkPlugin signals → PDUSender → PDUSerializer → DISTransport
//
// THREAD MODEL:
//   All objects moved to DIS thread in DISNetworkPlugin::start()
//   QTimer inside PDUSender drives the 5Hz send loop
//   QUdpSocket inside DISTransport drives incoming PDUs
// =============================================================================

#ifndef DISMANAGER_H
#define DISMANAGER_H

#include <QObject>
#include <QString>

#include "../interface/DISConfig.h"
#include "../utils/entityidmapper.h"
#include "../version/pduversionadapter.h"

#include "../interface/DISEntitySnapshot.h"
#include "../interface/DISFireSnapshot.h"
#include "../interface/DISDetonationSnapshot.h"
#include "../interface/DISExerciseControl.h"
#include "../interface/DISConfig.h"
#include "core/DISPlugin/DISNetworkPlugin.h"  // for DISNetworkStatus
#include "../handlers/acknowledgehandler.h"
#include "../handlers/transferownershiphandler.h"
#include "../interface/DISEmissionSnapshot.h"
#include "../interface/DISDesignatorSnapshot.h"
#include "../handlers/designatorhandler.h"
#include "../interface/DISTransmitterSnapshot.h"
#include "../interface/DISSignalSnapshot.h"
#include "../interface/DISDatumSnapshot.h"


// Forward declarations — all owned by DISManager
class DISTransport;
class PDUDispatcher;
class PDUSender;
class EntityStateHandler;
class FireHandler;
class DetonationHandler;
class ExerciseHandler;
class DISNetworkBridge;
class DISNetworkPlugin;
class Hierarchy;
class IFFHandler;
class EmissionHandler;
class DesignatorHandler;\
class TransmitterHandler;
class SignalHandler;
class DataQueryHandler;
class SetDataHandler;
class DataHandler;
class DISManager : public QObject {
    Q_OBJECT

public:
    explicit DISManager(QObject* parent = nullptr);
    ~DISManager() override;

    // ── Setup — called before start() ────────────────────────────────────────
    void setPlugin   (DISNetworkPlugin* plugin);
    void setHierarchy(Hierarchy* hierarchy);

public slots:
    // ── Lifecycle — called on network thread ──────────────────────────────────
    void start(DISConfig config);
    void stop();

    // ── Config reload ─────────────────────────────────────────────────────────
    void onReloadConfig(DISConfig config);

    // ── Outgoing PDU slots — connected from DISNetworkPlugin signals ──────────
    void onSendEntityState (DISEntitySnapshot     snap);
    void onSendFire        (DISFireSnapshot        snap);
    void onSendDetonation  (DISDetonationSnapshot  snap);
    void onSendStartResume (DISStartResumeData     data);
    void onSendStopFreeze  (DISStopFreezeData      data);
    void onSendRemoveEntity(DISRemoveEntityData    data);
    void onSendCreateEntity(DISCreateEntityData    data);

    // ── Entity lifecycle — called from DISNetworkPlugin ───────────────────────
    void onEntityAdded  (QString parentID, QString entityID, QString name);
    void onEntityRemoved(QString parentID, QString entityID, bool profile);
    // Snapshot push — receives from DISNetworkPlugin via QueuedConnection
    void onPushEntitySnapshot  (DISEntitySnapshot snap);
    void onRemoveEntitySnapshot(QString           entityID);
     void onPushIFFSnapshot(DISIFFSnapshot snap);
    void onSendTransferOwnership(DISTransferOwnershipData data);
    void onTakeEntityOwnership  (QString entityID);
    void onGiveUpEntityOwnership(QString entityID);
    void onSendEmission(DISEmissionSnapshot snap);
    void onSendDesignator(DISDesignatorSnapshot snap);
    void onSendTransmitter(DISTransmitterSnapshot snap);
    void onSendSignal(DISSignalSnapshot snap);
    void onSendDataQuery(DISDatumSnapshot snap);
    void onSendSetData(DISDatumSnapshot snap);
    void onSendData(DISDatumSnapshot snap);
    void onSendAcknowledgeDecision(DISAcknowledgeData ack);
signals:
    // ── Status — forwarded to DISNetworkPlugin ────────────────────────────────
    void statusUpdated  (int pdusSent, int pdusRecv, int peerCount);
    void errorOccurred  (QString error);
    void peerJoined     (QString addr, QString name, int version);
    void peerLeft       (QString addr);

private:
    // ── Owned components ──────────────────────────────────────────────────────
    DISTransport*       m_transport       = nullptr;
    PDUDispatcher*      m_dispatcher      = nullptr;
    PDUSender*          m_sender          = nullptr;
    EntityStateHandler* m_entityHandler   = nullptr;
    FireHandler*        m_fireHandler     = nullptr;
    DetonationHandler*  m_detonHandler    = nullptr;
    ExerciseHandler*    m_exerciseHandler = nullptr;
    DISNetworkBridge*   m_bridge          = nullptr;
    IFFHandler* m_iffHandler = nullptr;
    AcknowledgeHandler* m_ackHandler = nullptr;
    TransferOwnershipHandler* m_transferHandler = nullptr;
    EmissionHandler* m_emissionHandler = nullptr;
    DesignatorHandler* m_designatorHandler = nullptr;
    TransmitterHandler* m_transmitterHandler = nullptr;
    SignalHandler* m_signalHandler = nullptr;
    DataQueryHandler* m_dataQueryHandler = nullptr;
    SetDataHandler*   m_setDataHandler   = nullptr;
    DataHandler*      m_dataHandler      = nullptr;
    // ── Shared resources ──────────────────────────────────────────────────────
    EntityIDMapper      m_mapper;
    PeerVersionRegistry m_peerRegistry;

    // ── References ───────────────────────────────────────────────────────────
    DISNetworkPlugin* m_plugin    = nullptr;
    Hierarchy*        m_hierarchy = nullptr;

    // ── Config ───────────────────────────────────────────────────────────────
    DISConfig m_config;
    bool      m_running = false;

    // ── Internal wiring ───────────────────────────────────────────────────────
    void createComponents();
    void connectComponents();
    void connectToPlugin();
    void startStatsTimer();
};

#endif // DISMANAGER_H
