// =============================================================================
// FILE:        PDUSender.cpp
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "pdusender.h"
#include "../transport/DISTransport.h"

#include <QDebug>
#include <QMutexLocker>
#include "../DISNetworkPlugin.h"
#include "core/DISPlugin//utils/dislogger.h"
// =============================================================================
// Constructor
// =============================================================================
PDUSender::PDUSender(QObject* parent)
    : QObject(parent)
{
    // 5Hz send timer
    m_sendTimer = new QTimer(this);
    m_sendTimer->setInterval(200);  // 200ms = 5Hz
    connect(m_sendTimer, &QTimer::timeout, this, &PDUSender::onSendTimer);

    // 1Hz stats timer
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(1000);
    connect(m_statsTimer, &QTimer::timeout, this, &PDUSender::onStatsTimer);
}

PDUSender::~PDUSender()
{
    stop();
}
// =============================================================================
// isSendEnabled (file-scope helper)
// Checks per-PDU send flag from config pduConfigs list.
// Uses DISPDUType constants from DISConfig.h — these match pduType values
// stored in pduConfigs by DISDefaultConfig().
// Default is allow if type not found in list.
// =============================================================================
static bool isSendEnabled(const DISConfig& config, uint8_t pduType)
{
    for (const DISPDUConfig& cfg : config.pduConfigs) {
        if (cfg.pduType == pduType)
            return cfg.send;
    }
    return true;  // not found in config — allow by default
}
// =============================================================================
// configure / setters
// =============================================================================
void PDUSender::configure(const DISConfig& config)
{
    m_config = config;
}

void PDUSender::setTransport(DISTransport* transport)
{
    m_transport = transport;
}

void PDUSender::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}
void PDUSender::setBridge(DISNetworkBridge* bridge)
{
    m_bridge = bridge;
}
void PDUSender::setPlugin(DISNetworkPlugin* plugin)
{
    m_plugin = plugin;
}
void PDUSender::setPeerRegistry(PeerVersionRegistry* registry)
{
    m_peerRegistry = registry;
}

// =============================================================================
// start / stop
// =============================================================================
void PDUSender::start()
{
    m_sendTimer->start();
    m_statsTimer->start();
    DIS_LOG_BASIC("[PDUSender] Started — sending at 5Hz");
}

void PDUSender::stop()
{
    if (m_sendTimer)  m_sendTimer->stop();
    if (m_statsTimer) m_statsTimer->stop();
    DIS_LOG_BASIC("[PDUSender] Stopped");
}

// =============================================================================
// updateEntitySnapshot
// Called by bridge every frame for each locally owned entity
// Thread safe — bridge may call from main thread
// =============================================================================
void PDUSender::updateEntitySnapshot(const DISEntitySnapshot& snap)
{
    QMutexLocker lock(&m_snapshotMutex);
    m_snapshots[QString::fromStdString(snap.entityID)] = snap;
}

// =============================================================================
// onSendTimer
// Called every 200ms (5Hz) on the DIS network thread
// Iterates all registered local entities
// Sends EntityState PDU for each one that needs an update
// =============================================================================
void PDUSender::onSendTimer()
{
    DIS_LOG_FULL("[Sender tick] snapshots=" << m_snapshots.size()
     << "transport=" << (m_transport && m_transport->isRunning() ? "OK" : "DEAD"));
    if (!m_transport || !m_mapper) return;
    if (!m_bridge) return;  // ADD THIS LINE

    // Collect fresh snapshots from bridge every tick
    // Collect fresh snapshots on the main thread to avoid racing
    // with entity deletion. BlockingQueuedConnection blocks this
    // DIS thread until the main thread finishes the call.
    // if (m_bridge && m_plugin) {
    //     QList<DISEntitySnapshot> snapshots;
    //     QMetaObject::invokeMethod(m_plugin, [this, &snapshots]() {
    //         snapshots = m_bridge->collectSnapshots();
    //     }, Qt::BlockingQueuedConnection);
    //     QMutexLocker bridgeLock(&m_snapshotMutex);
    //     for (const DISEntitySnapshot& snap : snapshots) {
    //         m_snapshots[QString::fromStdString(snap.entityID)] = snap;
    //     }
    // }
    if (m_bridge) {
        QList<DISEntitySnapshot> snapshots = m_bridge->collectSnapshots();

        QMutexLocker bridgeLock(&m_snapshotMutex);
        for (const DISEntitySnapshot& snap : snapshots) {
            m_snapshots[QString::fromStdString(snap.entityID)] = snap;
        }
    }

    { QMutexLocker lock(&m_snapshotMutex);
    for (const DISEntitySnapshot& snap : m_snapshots) {
        if (!snap.active) continue;

        QString entityKey = QString::fromStdString(snap.entityID);

        // ── Dead reckoning check ──────────────────────────────────────────────
        // Check if receivers' dead reckoning is still accurate enough
        // If yes — skip this entity this tick (save bandwidth)
        // ── Dead reckoning check (IEEE 1278.1 compliant) ──────────────────────────
        // Two independent conditions trigger a send — OR logic per standard:
        //   1. DR threshold exceeded  — position/orientation error grown too large
        //   2. Heartbeat due          — 5 seconds elapsed since last send (mandatory)
        //
        // IEEE 1278.1-1995 Section 5.2.1:
        // "An Entity State PDU shall be issued any time the error in the DR
        //  algorithms has grown beyond acceptable levels OR at a minimum of
        //  once every five seconds."
        {
            qint64 nowMs        = QDateTime::currentMSecsSinceEpoch();
            qint64 lastSent     = m_lastSentMs.value(entityKey, 0);
            //bool heartbeatDue   = (nowMs - lastSent) >= 5000;  // 5 s mandatory
            qint64 heartbeatMs  = static_cast<qint64>(m_config.heartbeatSec * 1000.0f);
            bool heartbeatDue   = (nowMs - lastSent) >= heartbeatMs;

            bool drThresholdExceeded = false;
            if (m_drStates.contains(entityKey)) {
                const DeadReckoningState& drState = m_drStates[entityKey];

                ECEFPosition ecef = CoordConverter::geocordToECEF(
                    snap.latitude, snap.longitude, snap.altitude);

                DISOrientation euler = CoordConverter::headingToEuler(
                    snap.heading, snap.pitch, snap.roll);

                // Convert angle threshold from degrees (config) to radians (DR function)
                const float kDegToRad = 3.14159265358979f / 180.0f;
                drThresholdExceeded = DeadReckoning::shouldSendUpdate(
                    drState,
                    ecef.x, ecef.y, ecef.z,
                    euler.psi, euler.theta, euler.phi,
                    m_config.positionThresholdM,
                    m_config.angleThresholdDeg * kDegToRad);
            //     drThresholdExceeded = DeadReckoning::shouldSendUpdate(
            //         drState,
            //         ecef.x, ecef.y, ecef.z,
            //         euler.psi, euler.theta, euler.phi);
             }

            // Skip ONLY if both conditions are false
            if (!heartbeatDue && !drThresholdExceeded) continue;
        }

        // ── Send EntityState PDU ──────────────────────────────────────────────
        sendEntityState(snap);

        m_lastSentMs[entityKey] = QDateTime::currentMSecsSinceEpoch();


        // ── Update DR state after sending ────────────────────────────────────
        ECEFPosition ecef = CoordConverter::geocordToECEF(
            snap.latitude, snap.longitude, snap.altitude);
        DISOrientation euler = CoordConverter::headingToEuler(
            snap.heading, snap.pitch, snap.roll);
        DISVelocity vel = CoordConverter::nedToECEFVelocity(
            snap.northVel, snap.eastVel, snap.verticalVel,
            snap.latitude, snap.longitude);

        DeadReckoningState& drState = m_drStates[entityKey];
        DeadReckoning::updateState(
            drState,
            ecef.x, ecef.y, ecef.z,
            vel.x, vel.y, vel.z,
            euler.psi, euler.theta, euler.phi,
            static_cast<DRAlgorithm>(m_config.defaultDRAlgorithm)
            );
    }
    }
    // ── Send IFF PDUs ─────────────────────────────────────────────────────────
    {
        QMutexLocker lock(&m_snapshotMutex);
        for (const DISIFFSnapshot& snap : m_iffSnapshots) {
            if (snap.systemOn)
                sendIFF(snap);
        }
    }
}

// =============================================================================
// sendEntityState
// Serializes and sends one EntityState PDU
// =============================================================================
// void PDUSender::sendEntityState(const DISEntitySnapshot& snap)
// {
//     if (!m_mapper) {
//         DIS_LOG_BASIC("[DIAG Sender] sendEntityState: mapper is null");
//         return;
//     }
//     if (!isSendEnabled(m_config, DISPDUType::EntityState)) {
//         DIS_LOG_BASIC("[DIAG Sender] sendEntityState: EntityState DISABLED in config");
//         return;
//     }

//     DIS_LOG_BASIC("[DIAG Sender] serializing entity="
//                   << QString::fromStdString(snap.entityID)
//                   << "lat=" << snap.latitude
//                   << "lon=" << snap.longitude
//                   << "alt=" << snap.altitude
//                   << "active=" << snap.active);

//     QByteArray pduBytes = PDUSerializer::serializeEntityState(
//         snap, m_config, *m_mapper);

//     DIS_LOG_BASIC("[DIAG Sender] pduBytes size=" << pduBytes.size());

//     if (pduBytes.isEmpty()) return;

//     sendRawPDU(pduBytes, "EntityState",
//                QString::fromStdString(snap.entityID));
// }
void PDUSender::sendEntityState(const DISEntitySnapshot& snap)
{
    if (!m_mapper) return;
    if (!isSendEnabled(m_config, DISPDUType::EntityState)) return;  // ADD


    QByteArray pduBytes = PDUSerializer::serializeEntityState(
        snap, m_config, *m_mapper);

    if (pduBytes.isEmpty()) return;

    sendRawPDU(pduBytes, "EntityState",
               QString::fromStdString(snap.entityID));
}

// =============================================================================
// Event PDU senders — called immediately (not on timer)
// =============================================================================
void PDUSender::sendFire(const DISFireSnapshot& snap)
{
    if (!m_mapper) return;
    if (!isSendEnabled(m_config, DISPDUType::Fire)) return;  // ADD

    QByteArray pduBytes = PDUSerializer::serializeFire(snap, m_config, *m_mapper);
    sendRawPDU(pduBytes, "Fire", QString::fromStdString(snap.firingEntityID));
}

void PDUSender::sendDetonation(const DISDetonationSnapshot& snap)
{
    if (!m_mapper) return;
    if (!isSendEnabled(m_config, DISPDUType::Detonation)) return;  // ADD

    QByteArray pduBytes = PDUSerializer::serializeDetonation(snap, m_config, *m_mapper);
    sendRawPDU(pduBytes, "Detonation", QString::fromStdString(snap.munitionID));
}

void PDUSender::sendStartResume(const DISStartResumeData& data)
{
    if (!isSendEnabled(m_config, DISPDUType::StartResume)) return;  // ADD
    QByteArray pduBytes = PDUSerializer::serializeStartResume(data, m_config);
    sendRawPDU(pduBytes, "StartResume");
}

void PDUSender::sendStopFreeze(const DISStopFreezeData& data)
{
    if (!isSendEnabled(m_config, DISPDUType::StopFreeze)) return;  // ADD

    QByteArray pduBytes = PDUSerializer::serializeStopFreeze(data, m_config);
    sendRawPDU(pduBytes, "StopFreeze");
}

void PDUSender::sendRemoveEntity(const DISRemoveEntityData& data)
{
    if (!m_mapper) return;
    if (!isSendEnabled(m_config, DISPDUType::RemoveEntity)) return;  // ADD

    QByteArray pduBytes = PDUSerializer::serializeRemoveEntity(data, m_config, *m_mapper);
    sendRawPDU(pduBytes, "RemoveEntity",
               QString::fromStdString(data.removedEntityID));
}
void PDUSender::removeEntitySnapshot(const QString& entityID)
{
    QMutexLocker lock(&m_snapshotMutex);
    m_snapshots.remove(entityID);
    m_drStates.remove(entityID);    // also clear DR state so it doesn't linger
    m_lastSentMs.remove(entityID);
}
void PDUSender::sendCreateEntity(const DISCreateEntityData& data)
{
    if (!m_mapper) return;
    if (!isSendEnabled(m_config, DISPDUType::CreateEntity)) return;  // ADD
    QByteArray pduBytes = PDUSerializer::serializeCreateEntity(data, m_config, *m_mapper);
    sendRawPDU(pduBytes, "CreateEntity",
               QString::fromStdString(data.newEntityID));
}

// =============================================================================
// sendRawPDU
// Final step — adapts for peer versions and sends via transport
// =============================================================================
void PDUSender::sendRawPDU(const QByteArray& pduBytes,
                           const QString&    pduType,
                           const QString&    entityID)
{
    if (!m_transport || pduBytes.isEmpty()) return;

    // Adapt version for all known peers then send multicast
    // For now send multicast with default version
    // Per-peer unicast with V6 adaptation handled here when peers known
    QByteArray toSend = pduBytes;

    if (m_peerRegistry) {
        QList<PeerInfo> peers = m_peerRegistry->getActivePeers();
        for (const PeerInfo& peer : peers) {
            if (peer.disVersion != m_config.defaultVersion) {
                // This peer needs a different version — send unicast
                QByteArray adapted = PDUVersionAdapter::adaptForPeer(
                    pduBytes, peer.disVersion);
                m_transport->sendUnicast(adapted,
                                         QHostAddress(peer.address));
            }
        }
    }

    // Send multicast for all V7 peers and unknown peers
   // m_transport->sendMulticast(toSend);
    // Send multicast for all V7 peers and unknown peers
    // If unicast peer address is configured, send directly to that IP instead
    if (m_config.connectionMode == "Unicast") {
        // Unicast mode — send individually to each peer in list
        for (const DISConfig::UnicastPeer& peer : m_config.unicastPeers) {
            m_transport->sendUnicast(
                toSend,
                QHostAddress(QString::fromStdString(peer.ip)),
                peer.port);
        }
    } else {
        // Multicast mode — default
        m_transport->sendMulticast(toSend);
    }

    m_pdusSentCount++;
    emit pduSent(pduType, entityID);
}

// =============================================================================
// onStatsTimer — reset per-second counters
// =============================================================================
void PDUSender::onStatsTimer()
{
    m_pdusSentSec   = m_pdusSentCount;
    m_pdusSentCount = 0;
}
void PDUSender::updateIFFSnapshot(const DISIFFSnapshot& snap)
{
    QMutexLocker lock(&m_snapshotMutex);
    m_iffSnapshots[QString::fromStdString(snap.entityID)] = snap;
}
void PDUSender::sendIFF(const DISIFFSnapshot& snap)
{


    if (!m_mapper) {
        return;
    }
    if (!isSendEnabled(m_config, DISPDUType::IFF)) {
        return;
    }

    QByteArray pduBytes = PDUSerializer::serializeIFF(
        snap, m_config, *m_mapper);


    if (pduBytes.isEmpty()) return;

    sendRawPDU(pduBytes, "IFF",
               QString::fromStdString(snap.entityID));

}
// void PDUSender::sendIFF(const DISIFFSnapshot& snap)
// {
//     if (!m_mapper) return;
//     if (!isSendEnabled(m_config, DISPDUType::IFF)) return;

//     QByteArray pduBytes = PDUSerializer::serializeIFF(
//         snap, m_config, *m_mapper);

//     if (pduBytes.isEmpty()) return;
//     sendRawPDU(pduBytes, "IFF",
//                QString::fromStdString(snap.entityID));

//     DIS_LOG_BASIC("[PDUSender] IFF PDU sent for:"
//                   << QString::fromStdString(snap.entityID));
// }
