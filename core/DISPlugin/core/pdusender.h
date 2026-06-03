// =============================================================================
// FILE:        PDUSender.h
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Sends EntityState PDUs at 5Hz for all locally owned entities.
//              Also sends event-driven PDUs immediately (fire, detonation etc).
//              Uses shouldSendUpdate() from DeadReckoning to skip unnecessary
//              sends (entity not moved enough since last PDU).
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   QTimer fires every 200ms on network thread.
//   snapshots are pushed in via thread-safe queue from bridge.
//   Event PDUs (fire, detonation) sent immediately when received.
// =============================================================================

#ifndef PDUSENDER_H
#define PDUSENDER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QByteArray>

#include "../interface/DISEntitySnapshot.h"
#include "../interface/DISFireSnapshot.h"
#include "../interface/DISDetonationSnapshot.h"
#include "../interface/DISExerciseControl.h"
#include "../interface/DISConfig.h"
#include "../utils/entityidmapper.h"
#include "../utils/deadreckoning.h"
#include "../version/pduversionadapter.h"
#include "pduserializer.h"
#include "core/DISPlugin/bridge/disnetworkbridge.h"
#include "core/DISPlugin/interface/DISIFFSnapshot.h"

class DISNetworkPlugin;

// Forward declare transport
class DISTransport;

// =============================================================================
// PDUSender
// Owns the 5Hz send timer and all outgoing PDU logic
// =============================================================================
class PDUSender : public QObject {
    Q_OBJECT

public:
    explicit PDUSender(QObject* parent = nullptr);
    ~PDUSender() override;

    // ── Setup ────────────────────────────────────────────────────────────────
    void configure(const DISConfig& config);
    void setTransport(DISTransport* transport);
    void setMapper(EntityIDMapper* mapper);
    void setBridge(DISNetworkBridge* bridge);
    void setPlugin(DISNetworkPlugin* plugin);
    void setPeerRegistry(PeerVersionRegistry* registry);

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void start();
    void stop();

    // ── Entity snapshot updates ───────────────────────────────────────────────
    // Called by bridge every frame for each local entity
    // Thread safe — uses mutex
    void updateEntitySnapshot(const DISEntitySnapshot& snap);
    void removeEntitySnapshot(const QString& entityID);   // ← ADD THIS

    // ── Event PDUs — sent immediately ─────────────────────────────────────────
    void sendFire        (const DISFireSnapshot&       snap);
    void sendDetonation  (const DISDetonationSnapshot& snap);
    void sendStartResume (const DISStartResumeData&    data);
    void sendStopFreeze  (const DISStopFreezeData&     data);
    void sendRemoveEntity(const DISRemoveEntityData&   data);
    void sendCreateEntity(const DISCreateEntityData&   data);

    // ── Stats ─────────────────────────────────────────────────────────────────
    int pdusSentPerSecond() const { return m_pdusSentSec; }
    void updateIFFSnapshot(const DISIFFSnapshot& snap);
    void sendIFF(const DISIFFSnapshot& snap);

signals:
    void pduSent(QString pduType, QString entityID);
    void errorOccurred(QString error);

private slots:
    // Called by QTimer every 200ms
    void onSendTimer();

    // Called every 1 second to reset per-second stats
    void onStatsTimer();

private:
    // Config
    DISConfig            m_config;
    DISTransport*        m_transport    = nullptr;
    EntityIDMapper*      m_mapper       = nullptr;
    PeerVersionRegistry* m_peerRegistry = nullptr;
DISNetworkPlugin* m_plugin = nullptr;
    // Timers
    QTimer* m_sendTimer  = nullptr;  // 200ms = 5Hz
    QTimer* m_statsTimer = nullptr;  // 1000ms stats

    // Latest snapshot for each entity — keyed by entityID string
    // Updated by bridge, read by send timer
    QMutex m_snapshotMutex;
    QMap<QString, DISEntitySnapshot> m_snapshots;

    // Dead reckoning state per entity — for shouldSendUpdate()
    QMap<QString, DeadReckoningState> m_drStates;
    QHash<QString, qint64>             m_lastSentMs;
    DISNetworkBridge* m_bridge = nullptr;
    // Stats
    int m_pdusSentCount = 0;
    int m_pdusSentSec   = 0;

    // Internal helpers
    void sendEntityState(const DISEntitySnapshot& snap);
    void sendRawPDU(const QByteArray& pduBytes, const QString& pduType,
                    const QString& entityID = "");
    QByteArray adaptForPeers(const QByteArray& pduBytes);
    QMap<QString, DISIFFSnapshot> m_iffSnapshots;
};

#endif // PDUSENDER_H
