// =============================================================================
// FILE:        DISManager.cpp
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "dismanager.h"

#include "../transport/DISTransport.h"
#include "pdudispatcher.h"
#include "pdusender.h"
#include "../handlers/entitystatehandler.h"
#include "../handlers/warfarehandlers.h"
#include "../handlers/exercisehandler.h"
#include "../bridge/disnetworkbridge.h"
#include "../DISNetworkPlugin.h"
#include "core/DISPlugin/utils/dislogger.h"
#include <QDebug>
#include <QTimer>
#include "../handlers/iffhandler.h"
#include "../handlers/emissionhandler.h"
#include "../handlers/transmitterhandler.h"
#include "../handlers/signalhandler.h"
#include "../handlers/dataqueryhandler.h"
#include "../handlers/setdatahandler.h"
#include "../handlers/datahandler.h"
// =============================================================================
// Constructor
// =============================================================================
DISManager::DISManager(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[DISManager] Created";
}

// =============================================================================
// Destructor
// =============================================================================
DISManager::~DISManager()
{
    stop();
    qDebug() << "[DISManager] Destroyed";
}

// =============================================================================
// setPlugin / setHierarchy
// =============================================================================
void DISManager::setPlugin(DISNetworkPlugin* plugin)
{
    m_plugin = plugin;
}

void DISManager::setHierarchy(Hierarchy* hierarchy)
{
    m_hierarchy = hierarchy;
}

// =============================================================================
// start
// Called on network thread when DISNetworkPlugin starts
// Creates all components, wires them together, starts transport
// =============================================================================
void DISManager::start(DISConfig config)
{
    if (m_running) {
        DIS_LOG_WARNING("[DISManager] Already running");
        return;
    }

    m_config = config;

    // ── Configure EntityIDMapper ───────────────────────────────────────────────
    m_mapper.configure(config.siteID, config.applicationID);

    // ── Create all components ─────────────────────────────────────────────────
    createComponents();

    // ── Wire components together ──────────────────────────────────────────────
    connectComponents();

    // ── Wire to DISNetworkPlugin ──────────────────────────────────────────────
    connectToPlugin();

    // ── Start transport ───────────────────────────────────────────────────────
    bool ok = m_transport->start();
    if (!ok) {
        emit errorOccurred("Failed to start UDP transport on port "
                           + QString::number(config.port));
        return;
    }

    // ── Start sender ──────────────────────────────────────────────────────────
    m_sender->start();

    startStatsTimer();  // ← ADD


    m_running = true;

    DIS_LOG_BASIC("[DISManager] Started"
             << "multicast=" << QString::fromStdString(config.multicastGroup)
             << "port="      << config.port
             << "exercise="  << config.exerciseID
             << "site="      << config.siteID
             << "app="       << config.applicationID);
}

// =============================================================================
// stop
// Clean shutdown — stops sender first, then transport
// =============================================================================
void DISManager::stop()
{
    if (!m_running) return;
    m_running = false;

    if (m_sender)    m_sender->stop();
    if (m_transport) m_transport->stop();

    DIS_LOG_BASIC("[DISManager] Stopped");
}

// =============================================================================
// createComponents
// Instantiates all owned objects
// All parented to this so they live on the same thread
// =============================================================================
void DISManager::createComponents()
{
    // Transport — UDP socket
    m_transport = new DISTransport(this);
   /* m_transport->init(QString::fromStdString(m_config.multicastGroup),
                      m_config.port,
                      "");*/ // empty = auto-select interface
    // m_transport->init(QString::fromStdString(m_config.multicastGroup),
    //                   m_config.port,
    //                   QString::fromStdString(m_config.networkInterface));
    m_transport->init(QString::fromStdString(m_config.multicastGroup),
                      m_config.port,
                      QString::fromStdString(m_config.networkInterface),
                      QString::fromStdString(m_config.connectionMode));

    // Dispatcher — routes incoming bytes
    m_dispatcher = new PDUDispatcher(this);
    m_dispatcher->configure(m_config);
    m_dispatcher->setPeerRegistry(&m_peerRegistry);

    // Sender — 5Hz outgoing
    m_sender = new PDUSender(this);
    m_sender->configure(m_config);
    m_sender->setTransport(m_transport);
    m_sender->setMapper(&m_mapper);
    m_sender->setPeerRegistry(&m_peerRegistry);

    // Handlers — incoming PDU processing
    m_entityHandler = new EntityStateHandler(this);
    m_entityHandler->setMapper(&m_mapper);

    m_fireHandler = new FireHandler(this);
    m_fireHandler->setMapper(&m_mapper);

    m_detonHandler = new DetonationHandler(this);
    m_detonHandler->setMapper(&m_mapper);

    m_exerciseHandler = new ExerciseHandler(this);

    // Bridge — reads hierarchy, builds snapshots
    m_bridge = new DISNetworkBridge(this);
    m_bridge->setMapper(&m_mapper);
    m_bridge->configure(m_config);
    m_sender->setBridge(m_bridge);
    m_iffHandler = new IFFHandler(this);
    m_iffHandler->setMapper(&m_mapper);
    // Acknowledge handler — processes incoming Acknowledge PDUs (type 15)
    m_emissionHandler = new EmissionHandler(this);
    m_emissionHandler->setMapper(&m_mapper);

    m_designatorHandler = new DesignatorHandler(this);
    m_designatorHandler->setMapper(&m_mapper);

    m_ackHandler = new AcknowledgeHandler(this);
    m_ackHandler->setMapper(&m_mapper);
    // Transfer Ownership handler — processes incoming type-35 PDUs
    m_transferHandler = new TransferOwnershipHandler(this);
    m_transferHandler->setMapper(&m_mapper);

    m_transmitterHandler = new TransmitterHandler(this);
    m_transmitterHandler->setMapper(&m_mapper);
    m_signalHandler = new SignalHandler(this);
    m_signalHandler->setMapper(&m_mapper);

    m_dataQueryHandler = new DataQueryHandler(this);
    m_dataQueryHandler->setMapper(&m_mapper);
    m_setDataHandler = new SetDataHandler(this);
    m_setDataHandler->setMapper(&m_mapper);
    m_dataHandler = new DataHandler(this);
    m_dataHandler->setMapper(&m_mapper);
    DIS_LOG_BASIC("[DISManager] All components created");
}

// =============================================================================
// connectComponents
// Wires all internal signals/slots
// All connections are direct (same thread)
// =============================================================================
void DISManager::connectComponents()
{
    // ── Transport → Dispatcher ────────────────────────────────────────────────
    connect(m_transport, &DISTransport::datagramReceived,
            m_dispatcher, &PDUDispatcher::onDatagramReceived,
            Qt::DirectConnection);

    // ── Dispatcher → Handlers ─────────────────────────────────────────────────
    connect(m_dispatcher, &PDUDispatcher::entityStateReceived,
            m_entityHandler, &EntityStateHandler::onEntityStateReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::fireReceived,
            m_fireHandler, &FireHandler::onFireReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::detonationReceived,
            m_detonHandler, &DetonationHandler::onDetonationReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::startResumeReceived,
            m_exerciseHandler, &ExerciseHandler::onStartResumeReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::stopFreezeReceived,
            m_exerciseHandler, &ExerciseHandler::onStopFreezeReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::removeEntityReceived,
            m_exerciseHandler, &ExerciseHandler::onRemoveEntityReceived,
            Qt::DirectConnection);

    connect(m_dispatcher, &PDUDispatcher::createEntityReceived,
            m_exerciseHandler, &ExerciseHandler::onCreateEntityReceived,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::iffReceived,
            m_iffHandler,  &IFFHandler::onIFFReceived,
            Qt::DirectConnection);
    // ── Dispatcher → AcknowledgeHandler ──────────────────────────────────────────
    connect(m_dispatcher, &PDUDispatcher::acknowledgeReceived,
            m_ackHandler, &AcknowledgeHandler::onAcknowledgeReceived,
            Qt::DirectConnection);
    // ── Dispatcher → TransferOwnershipHandler ────────────────────────────────────
    connect(m_dispatcher, &PDUDispatcher::transferOwnershipReceived,
            m_transferHandler, &TransferOwnershipHandler::onTransferOwnershipReceived,
            Qt::DirectConnection);

    // ── TransferOwnershipHandler → PDUSender (auto-acknowledge, same thread) ─────
    connect(m_transferHandler, &TransferOwnershipHandler::acknowledgeNeeded,
            m_sender, &PDUSender::sendAcknowledge,
            Qt::DirectConnection);
    // ── ExerciseHandler → PDUSender (auto-acknowledge on CreateEntity/RemoveEntity)
    // DirectConnection — both on network thread — fires immediately before reply
    connect(m_exerciseHandler, &ExerciseHandler::acknowledgeNeeded,
            m_sender, &PDUSender::sendAcknowledge,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::emissionReceived,
            m_emissionHandler, &EmissionHandler::onEmissionReceived,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::designatorReceived,
            m_designatorHandler, &DesignatorHandler::onDesignatorReceived,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::transmitterReceived,
            m_transmitterHandler, &TransmitterHandler::onTransmitterReceived,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::signalReceived,
            m_signalHandler, &SignalHandler::onSignalReceived,
            Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::dataQueryReceived,
            m_dataQueryHandler, &DataQueryHandler::onDataQueryReceived, Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::setDataReceived,
            m_setDataHandler, &SetDataHandler::onSetDataReceived, Qt::DirectConnection);
    connect(m_dispatcher, &PDUDispatcher::dataReceived,
            m_dataHandler, &DataHandler::onDataReceived, Qt::DirectConnection);
    DIS_LOG_BASIC("[DISManager] Components wired");
}

// =============================================================================
// connectToPlugin
// Wires handler outputs → DISNetworkPlugin queues
// These cross the thread boundary → Qt::QueuedConnection
// =============================================================================
void DISManager::connectToPlugin()
{
    if (!m_plugin) return;
    m_sender->setPlugin(m_plugin);  // ADD THIS LINE

    // ── Incoming transforms → plugin queue ────────────────────────────────────
    connect(m_entityHandler, &EntityStateHandler::incomingTransform,
            m_plugin, &DISNetworkPlugin::onIncomingTransform,
            Qt::QueuedConnection);

    // ── Incoming fire → plugin queue ──────────────────────────────────────────
    connect(m_fireHandler, &FireHandler::incomingFire,
            m_plugin, &DISNetworkPlugin::onIncomingFire,
            Qt::QueuedConnection);

    // ── Incoming detonation → plugin queue ────────────────────────────────────
    connect(m_detonHandler, &DetonationHandler::incomingDetonation,
            m_plugin, &DISNetworkPlugin::onIncomingDetonation,
            Qt::QueuedConnection);

    // ── Incoming exercise → plugin queue ──────────────────────────────────────
    connect(m_exerciseHandler, &ExerciseHandler::incomingExercise,
            m_plugin, &DISNetworkPlugin::onIncomingExercise,
            Qt::QueuedConnection);

    // ── Plugin send signals → sender slots ────────────────────────────────────
    // These cross thread boundary → QueuedConnection
    connect(m_plugin, &DISNetworkPlugin::_internalSendEntityState,
            this, &DISManager::onSendEntityState,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendFire,
            this, &DISManager::onSendFire,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendDetonation,
            this, &DISManager::onSendDetonation,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendStartResume,
            this, &DISManager::onSendStartResume,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendStopFreeze,
            this, &DISManager::onSendStopFreeze,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendRemoveEntity,
            this, &DISManager::onSendRemoveEntity,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendCreateEntity,
            this, &DISManager::onSendCreateEntity,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalReloadConfig,
            this, &DISManager::onReloadConfig,
            Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalPushSnapshot,
            this, &DISManager::onPushEntitySnapshot,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalRemoveSnapshot,
            this, &DISManager::onRemoveEntitySnapshot,
            Qt::QueuedConnection);
    // ── Peer discovery → plugin ───────────────────────────────────────────────
    connect(m_dispatcher, &PDUDispatcher::peerDiscovered,
            this, [this](QHostAddress addr, uint8_t version,
                   uint16_t site, uint16_t app) {
                Q_UNUSED(site) Q_UNUSED(app)
                emit peerJoined(addr.toString(), "", static_cast<int>(version));
            }, Qt::DirectConnection);

    // ── Errors → plugin ───────────────────────────────────────────────────────
    connect(m_transport, &DISTransport::errorOccurred,
            this, &DISManager::errorOccurred,
            Qt::DirectConnection);
    connect(this, &DISManager::statusUpdated,
            this, [this](int sent, int recv, int peers) {
                if (!m_plugin) return;
                DISNetworkStatus status;
                status.connected      = m_running;
                status.pdusSentPerSec = sent;
                status.pdusRecvPerSec = recv;
                status.activePeers    = peers;
                QMetaObject::invokeMethod(
                    m_plugin, "onIncomingStatus",
                    Qt::QueuedConnection,
                    Q_ARG(DISNetworkStatus, status));
            }, Qt::DirectConnection);
    DIS_LOG_BASIC("[DISManager] Plugin connections made");
    connect(m_iffHandler, &IFFHandler::incomingIFF,
            m_plugin, &DISNetworkPlugin::onIncomingIFF,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalPushIFFSnapshot,
            this, &DISManager::onPushIFFSnapshot,
            Qt::QueuedConnection);
    // ── Incoming transfer → plugin queue ─────────────────────────────────────────
    connect(m_transferHandler, &TransferOwnershipHandler::incomingTransferOwnership,
            m_plugin, &DISNetworkPlugin::onIncomingTransferOwnership,
            Qt::QueuedConnection);

    // ── Plugin → DISManager for ownership operations ─────────────────────────────
    connect(m_plugin, &DISNetworkPlugin::_internalSendTransferOwnership,
            this, &DISManager::onSendTransferOwnership,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalTakeEntityOwnership,
            this, &DISManager::onTakeEntityOwnership,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalGiveUpEntityOwnership,
            this, &DISManager::onGiveUpEntityOwnership,
            Qt::QueuedConnection);
    // ── Incoming acknowledge → plugin queue ───────────────────────────────────────
    connect(m_ackHandler, &AcknowledgeHandler::incomingAcknowledge,
            m_plugin, &DISNetworkPlugin::onIncomingAcknowledge,
            Qt::QueuedConnection);
    connect(m_emissionHandler, &EmissionHandler::incomingEmission,
            m_plugin, &DISNetworkPlugin::onIncomingEmission,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendEmission,
            this, &DISManager::onSendEmission,
            Qt::QueuedConnection);
    connect(m_designatorHandler, &DesignatorHandler::incomingDesignator,
            m_plugin, &DISNetworkPlugin::onIncomingDesignator,
            Qt::QueuedConnection);

    connect(m_plugin, &DISNetworkPlugin::_internalSendDesignator,
            this, &DISManager::onSendDesignator,
            Qt::QueuedConnection);

    connect(m_transmitterHandler, &TransmitterHandler::incomingTransmitter,
            m_plugin, &DISNetworkPlugin::onIncomingTransmitter,
            Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendTransmitter,
            this, &DISManager::onSendTransmitter,
            Qt::QueuedConnection);

    connect(m_signalHandler, &SignalHandler::incomingSignal,
            m_plugin, &DISNetworkPlugin::onIncomingSignal,
            Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendSignal,
            this, &DISManager::onSendSignal,
            Qt::QueuedConnection);
    connect(m_dataQueryHandler, &DataQueryHandler::incomingDataQuery,
            m_plugin, &DISNetworkPlugin::onIncomingDataQuery, Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendDataQuery,
            this, &DISManager::onSendDataQuery, Qt::QueuedConnection);

    connect(m_setDataHandler, &SetDataHandler::incomingSetData,
            m_plugin, &DISNetworkPlugin::onIncomingSetData, Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendSetData,
            this, &DISManager::onSendSetData, Qt::QueuedConnection);

    connect(m_dataHandler, &DataHandler::incomingData,
            m_plugin, &DISNetworkPlugin::onIncomingData, Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendData,
            this, &DISManager::onSendData, Qt::QueuedConnection);
    connect(m_plugin, &DISNetworkPlugin::_internalSendAcknowledgeDecision,
            this, &DISManager::onSendAcknowledgeDecision,
            Qt::QueuedConnection);
}

// =============================================================================
// Outgoing PDU slots
// Receive from plugin (via QueuedConnection from main thread)
// Forward to sender (same thread — direct)
// =============================================================================
void DISManager::onSendEntityState(DISEntitySnapshot snap)
{
    if (m_sender) m_sender->updateEntitySnapshot(snap);
}

void DISManager::onSendFire(DISFireSnapshot snap)
{
    if (m_sender) m_sender->sendFire(snap);
}

void DISManager::onSendDetonation(DISDetonationSnapshot snap)
{
    if (m_sender) m_sender->sendDetonation(snap);
}

void DISManager::onSendStartResume(DISStartResumeData data)
{
    if (m_sender) m_sender->sendStartResume(data);
}

void DISManager::onSendStopFreeze(DISStopFreezeData data)
{
    if (m_sender) m_sender->sendStopFreeze(data);
}

void DISManager::onSendRemoveEntity(DISRemoveEntityData data)
{
    if (m_sender) m_sender->sendRemoveEntity(data);
}

void DISManager::onSendCreateEntity(DISCreateEntityData data)
{
    if (m_sender) m_sender->sendCreateEntity(data);
}

// =============================================================================
// Entity lifecycle
// =============================================================================
void DISManager::onEntityAdded(QString parentID, QString entityID, QString name)
{
    Q_UNUSED(parentID) Q_UNUSED(name)
    if (!m_bridge) return;

    // Do not register ghost entities as local — they are owned by a remote machine.
    // Check against the remote tracking maps in the plugin.
    if (m_plugin && m_plugin->isRemoteEntity(entityID)) return;

    m_bridge->registerEntity(entityID);
}
void DISManager::onPushEntitySnapshot(DISEntitySnapshot snap)
{
    if (m_bridge) m_bridge->pushSnapshot(snap);
}

void DISManager::onRemoveEntitySnapshot(QString entityID)
{
    if (m_bridge) m_bridge->removeSnapshot(entityID);
    if (m_sender)  m_sender->removeEntitySnapshot(entityID);  // ← ADD THIS

}
void DISManager::onEntityRemoved(QString parentID, QString entityID, bool profile)
{
    Q_UNUSED(parentID) Q_UNUSED(profile)
    if (m_bridge) m_bridge->unregisterEntity(entityID);
}

// =============================================================================
// onReloadConfig
// =============================================================================
void DISManager::onReloadConfig(DISConfig config)
{
    m_config = config;
    if (m_dispatcher) m_dispatcher->configure(config);
    if (m_sender)     m_sender->configure(config);
    if (m_bridge)     m_bridge->configure(config);
    DIS_LOG_BASIC("[DISManager] Config reloaded");
}
void DISManager::startStatsTimer()
{
    QTimer* statsTimer = new QTimer(this);
    statsTimer->setInterval(1000);

    // Stale peer cleanup counter — removeStale runs every 60 ticks (60 seconds)
    // A peer is stale if no PDU received from it for 60 seconds.
    // This prevents disconnected machines from staying in the peer list forever.
    int* cleanupTick = new int(0);

    connect(statsTimer, &QTimer::timeout, this, [this, cleanupTick]() {
        if (!m_running) return;

        int sent  = m_transport ? m_transport->pdusSentSec() : 0;
        int recv  = m_transport ? m_transport->pdusRecvSec() : 0;
        int peers = static_cast<int>(m_peerRegistry.getActivePeers().size());
        emit statusUpdated(sent, recv, peers);

        // Remove peers not seen for 60 seconds — runs every 60 ticks
        ++(*cleanupTick);
        if (*cleanupTick >= 60) {
            *cleanupTick = 0;
            m_peerRegistry.removeStale(60);
        }
    });

    // Clean up the counter when the timer is destroyed
    connect(statsTimer, &QObject::destroyed, this, [cleanupTick]() {
        delete cleanupTick;
    });

    statsTimer->start();
}
void DISManager::onPushIFFSnapshot(DISIFFSnapshot snap)
{
    if (m_sender) m_sender->updateIFFSnapshot(snap);
}
// =============================================================================
// onSendTransferOwnership
// Forwards from plugin (main thread) to PDUSender (network thread)
// =============================================================================
void DISManager::onSendAcknowledgeDecision(DISAcknowledgeData ack)
{
    if (m_sender) m_sender->sendAcknowledge(ack);
}
void DISManager::onSendTransferOwnership(DISTransferOwnershipData data)
{
    if (m_sender) m_sender->sendTransferOwnership(data);
}

// =============================================================================
// onTakeEntityOwnership
// Called after Machine B accepts a Transfer Ownership PDU.
// Flips entity to local in mapper — PDUSender starts sending EntityState
// automatically on the next 5Hz tick via pushAllLocalSnapshots().
// =============================================================================
void DISManager::onTakeEntityOwnership(QString entityID)
{
    m_mapper.takeOwnership(entityID.toStdString());
    DIS_LOG_BASIC("[DISManager] Ownership taken for entity:" << entityID);
}

// =============================================================================
// onGiveUpEntityOwnership
// Called after Machine A receives AbleToComply Acknowledge.
// Removes entity from PDUSender so EntityState stops broadcasting.
// =============================================================================
void DISManager::onGiveUpEntityOwnership(QString entityID)
{
    DIS_LOG_BASIC("[DISManager] onGiveUpEntityOwnership:"
                  << entityID);

    // Clear from bridge cache — prevents re-insertion via collectSnapshots()
    if (m_bridge) m_bridge->removeSnapshot(entityID);

    // Blacklist in sender — prevents EntityState from being sent
    if (m_sender) m_sender->removeEntitySnapshot(entityID);

    // Flip isLocal to false in mapper — EntityStateHandler will now
    // accept incoming EntityState from the new owner (Machine B)
    // instead of dropping it at line 101 of entitystatehandler.cpp
    m_mapper.giveUpOwnership(entityID.toStdString());
}
void DISManager::onSendEmission(DISEmissionSnapshot snap)
{
    if (m_sender) m_sender->updateEmissionSnapshot(snap);

    //if (m_sender) m_sender->sendEmission(snap);
}

void DISManager::onSendDesignator(DISDesignatorSnapshot snap)
{
    if (m_sender) m_sender->sendDesignator(snap);
}
void DISManager::onSendTransmitter(DISTransmitterSnapshot snap)
{
    // if (m_sender) m_sender->sendTransmitter(snap);
    if (m_sender) m_sender->updateTransmitterSnapshot(snap);

}
void DISManager::onSendSignal(DISSignalSnapshot snap)
{
    if (m_sender) m_sender->sendSignal(snap);
}
void DISManager::onSendDataQuery(DISDatumSnapshot snap) { if (m_sender) m_sender->sendDataQuery(snap); }
void DISManager::onSendSetData(DISDatumSnapshot snap)   { if (m_sender) m_sender->sendSetData(snap); }
void DISManager::onSendData(DISDatumSnapshot snap)      { if (m_sender) m_sender->sendData(snap); }
