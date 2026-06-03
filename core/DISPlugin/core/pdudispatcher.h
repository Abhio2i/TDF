// =============================================================================
// FILE:        PDUDispatcher.h
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Receives raw UDP bytes from DISTransport.
//              Validates the PDU header.
//              Reads the PDU type byte.
//              Unmarshals using your dis7 library.
//              Emits typed signals for each PDU type.
//              DISManager connects these signals to handlers.
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   datagramReceived() slot called from DISTransport on network thread.
//   All signals emitted on network thread.
//   DISManager receives on network thread and pushes to queues.
// =============================================================================

#ifndef PDUDISPATCHER_H
#define PDUDISPATCHER_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

#include "../version/pduversionadapter.h"
#include "../interface/DISConfig.h"

// Forward declare dis7 PDU types — no include needed in header
namespace DIS {
class EntityStatePdu;
class FirePdu;
class DetonationPdu;
class StartResumePdu;
class StopFreezePdu;
class RemoveEntityPdu;
class CreateEntityPdu;
class ElectromagneticEmissionsPdu;
}

// =============================================================================
// PDUDispatcher
// Receives raw bytes, validates, unmarshals, emits typed signals
// =============================================================================
class PDUDispatcher : public QObject {
    Q_OBJECT

public:
    explicit PDUDispatcher(QObject* parent = nullptr);
    ~PDUDispatcher() override;

    // Configure with exercise ID to filter foreign exercises
    void configure(const DISConfig& config);

    // Connect to PeerVersionRegistry for peer tracking
    void setPeerRegistry(PeerVersionRegistry* registry);

public slots:
    // ── Connected to DISTransport::datagramReceived ───────────────────────────
    // Called on network thread when UDP datagram arrives
    void onDatagramReceived(QByteArray data,
                            QHostAddress sender,
                            quint16 senderPort);

signals:
    // ── Typed PDU signals — connect to handlers ───────────────────────────────
    // All emitted on network thread
    void entityStateReceived    (QByteArray pduBytes, QHostAddress sender);
    void fireReceived           (QByteArray pduBytes, QHostAddress sender);
    void detonationReceived     (QByteArray pduBytes, QHostAddress sender);
    void startResumeReceived    (QByteArray pduBytes, QHostAddress sender);
    void stopFreezeReceived     (QByteArray pduBytes, QHostAddress sender);
    void removeEntityReceived   (QByteArray pduBytes, QHostAddress sender);
    void createEntityReceived   (QByteArray pduBytes, QHostAddress sender);
    void emissionReceived       (QByteArray pduBytes, QHostAddress sender);

    // ── Stats signals ─────────────────────────────────────────────────────────
    void unknownPDUReceived     (uint8_t pduType, QHostAddress sender);
    void invalidPDUReceived     (QHostAddress sender);
    void peerDiscovered         (QHostAddress addr, uint8_t version,
                        uint16_t site, uint16_t app);
    void iffReceived(QByteArray data, QHostAddress sender, quint16 port);


private:
    DISConfig            m_config;
    PeerVersionRegistry* m_peerRegistry = nullptr;

    // Stats
    int m_totalReceived  = 0;
    int m_totalDropped   = 0;
    int m_entityStateCount = 0;

    // Dispatch based on PDU type byte
    void dispatch(const QByteArray& pduBytes, const QHostAddress& sender);

    // Extract site/app from EntityState PDU for peer tracking
    // Returns false if PDU too short
    bool extractSimulationAddress(const QByteArray& pduBytes,
                                  uint16_t& site,
                                  uint16_t& app) const;
};

#endif // PDUDISPATCHER_H
