// =============================================================================
// FILE:        PDUDispatcher.cpp
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "pdudispatcher.h"
#include <dis7/utils/PDUType.h>
#include <QDebug>

// =============================================================================
// Constructor
// =============================================================================
PDUDispatcher::PDUDispatcher(QObject* parent)
    : QObject(parent)
{
}

PDUDispatcher::~PDUDispatcher() = default;

// =============================================================================
// configure
// =============================================================================
void PDUDispatcher::configure(const DISConfig& config)
{
    m_config = config;
}

// =============================================================================
// isReceiveEnabled (file-scope helper)
// Checks per-PDU receive flag from config pduConfigs list.
// Uses the raw PDU type byte from the wire — must match pduType stored
// in DISPDUConfig entries. Default is allow if type not found in list.
// NOTE: DISPDUType::RemoveEntity(31) and CreateEntity(32) do not match
//       DIS library wire values (12, 11) — those two always pass through.
//       This is a pre-existing mismatch in DISConfig.h, not a bug here.
// =============================================================================
static bool isReceiveEnabled(const DISConfig& config, uint8_t pduType)
{
    for (const DISPDUConfig& cfg : config.pduConfigs) {
        if (cfg.pduType == pduType)
            return cfg.receive;
    }
    return true;  // not found in config — allow by default
}
// =============================================================================
// setPeerRegistry
// =============================================================================
void PDUDispatcher::setPeerRegistry(PeerVersionRegistry* registry)
{
    m_peerRegistry = registry;
}

// =============================================================================
// onDatagramReceived
// Entry point — called by DISTransport::datagramReceived signal
// Validates header then dispatches
// =============================================================================
void PDUDispatcher::onDatagramReceived(QByteArray   data,
                                       QHostAddress sender,
                                       quint16      senderPort)
{
    Q_UNUSED(senderPort)
    m_totalReceived++;

    // ── Step 1: Validate ──────────────────────────────────────────────────────
    // ── Step 1: Validate ──────────────────────────────────────────────────────
    if (!PDUVersionAdapter::isValidDISPDU(data, m_config.exerciseID)) {
        m_totalDropped++;
        emit invalidPDUReceived(sender);
        return;
    }

    // ── Step 1.5: Discard late messages ───────────────────────────────────────
    // Only absolute timestamps (bit 0 = 1) are checked.
    // Zero and relative timestamps are always accepted.
    // This matches STAGE 22.0 "Discard Late Messages" behaviour.
    if (m_config.discardLateMessages) {
        if (PDUVersionAdapter::isLateMessage(data, m_config.lateMessageThresholdMs)) {
            m_totalDropped++;
            qDebug() << "[PDUDispatcher] Late PDU discarded from"
                     << sender.toString();
            return;
        }
    }

    // ── Step 2: Track peer version ────────────────────────────────────────────

    // ── Step 2: Track peer version ────────────────────────────────────────────
    uint8_t version = PDUVersionAdapter::extractVersion(data);

    if (m_peerRegistry) {
        uint16_t site = 0, app = 0;
        extractSimulationAddress(data, site, app);
        bool isNew = !m_peerRegistry->hasPeer(sender.toString());
        m_peerRegistry->updatePeer(sender.toString(), version, site, app);
        if (isNew && (site != 0 || app != 0)) {
            emit peerDiscovered(sender, version, site, app);
        }
    }

    // ── Step 3: Normalize to V7 for internal processing ───────────────────────
    QByteArray normalized = PDUVersionAdapter::normalizeToV7(data);

    // ── Step 4: Dispatch by PDU type ──────────────────────────────────────────
    dispatch(normalized, sender);
}

// =============================================================================
// dispatch
// Reads PDU type byte and emits the correct typed signal
// Passes raw normalized bytes — handlers unmarshal themselves
// =============================================================================
void PDUDispatcher::dispatch(const QByteArray& pduBytes,
                             const QHostAddress& sender)
{
    uint8_t pduType = PDUVersionAdapter::extractPDUType(pduBytes);
    switch (pduType) {

    case DIS::PDU_ENTITY_STATE:
        if (!isReceiveEnabled(m_config, DIS::PDU_ENTITY_STATE)) break;
        m_entityStateCount++;
        emit entityStateReceived(pduBytes, sender);
        break;

    case DIS::PDU_FIRE:
        if (!isReceiveEnabled(m_config, DIS::PDU_FIRE)) break;
        emit fireReceived(pduBytes, sender);
        break;

    case DIS::PDU_DETONATION:
        if (!isReceiveEnabled(m_config, DIS::PDU_DETONATION)) break;
        emit detonationReceived(pduBytes, sender);
        break;

    case DIS::PDU_START_RESUME:
        if (!isReceiveEnabled(m_config, DIS::PDU_START_RESUME)) break;
        emit startResumeReceived(pduBytes, sender);
        break;

    case DIS::PDU_STOP_FREEZE:
        if (!isReceiveEnabled(m_config, DIS::PDU_STOP_FREEZE)) break;
        emit stopFreezeReceived(pduBytes, sender);
        break;

    case DIS::PDU_REMOVE_ENTITY:
        // NOTE: DIS wire type 12 does not match DISPDUType::RemoveEntity(31)
        // Filter lookup will not find entry — defaults to allow.
        // To be fixed when DISConfig.h type constants are corrected.
        if (!isReceiveEnabled(m_config, DIS::PDU_REMOVE_ENTITY)) break;
        emit removeEntityReceived(pduBytes, sender);
        break;

    case DIS::PDU_CREATE_ENTITY:
        // NOTE: DIS wire type 11 does not match DISPDUType::CreateEntity(32)
        // Filter lookup will not find entry — defaults to allow.
        if (!isReceiveEnabled(m_config, DIS::PDU_CREATE_ENTITY)) break;
        emit createEntityReceived(pduBytes, sender);
        break;
    case DIS::PDU_IFF:
        if (isReceiveEnabled(m_config, DIS::PDU_IFF))
            emit iffReceived(pduBytes, sender, 0);
        break;
    case DIS::PDU_ELECTRONIC_EMMISIONS:
        if (!isReceiveEnabled(m_config, DIS::PDU_ELECTRONIC_EMMISIONS)) break;
        emit emissionReceived(pduBytes, sender);
        break;

    default:
        // PDU type we do not handle — not an error, just log
        emit unknownPDUReceived(pduType, sender);
        break;
    }
    // switch (pduType) {

    // case DIS::PDU_ENTITY_STATE:
    //     m_entityStateCount++;
    //     emit entityStateReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_FIRE:
    //     emit fireReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_DETONATION:
    //     emit detonationReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_START_RESUME:
    //     emit startResumeReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_STOP_FREEZE:
    //     emit stopFreezeReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_REMOVE_ENTITY:
    //     emit removeEntityReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_CREATE_ENTITY:
    //     emit createEntityReceived(pduBytes, sender);
    //     break;

    // case DIS::PDU_ELECTRONIC_EMMISIONS:
    //     emit emissionReceived(pduBytes, sender);
    //     break;

    // default:
    //     // PDU type we do not handle — not an error, just log
    //     emit unknownPDUReceived(pduType, sender);
    //     break;
    // }
}

// =============================================================================
// extractSimulationAddress
// Reads site and application IDs from EntityState PDU
// Used for peer tracking — only EntityState has guaranteed site/app
// at a known offset after the 12-byte header
//
// EntityStatePdu wire layout after header (12 bytes):
//   bytes 12-13: site ID        (uint16 big-endian)
//   bytes 14-15: application ID (uint16 big-endian)
//   bytes 16-17: entity number  (uint16 big-endian)
// =============================================================================
bool PDUDispatcher::extractSimulationAddress(const QByteArray& pduBytes,
                                             uint16_t& site,
                                             uint16_t& app) const
{
    // Need at least 16 bytes: 12 header + 4 for site+app
    if (pduBytes.size() < 16) return false;

    uint8_t pduType = PDUVersionAdapter::extractPDUType(pduBytes);

    // Only extract from PDU types that start with EntityID
    // EntityState, Fire, Detonation all have EntityID as first field after header
    if (pduType != DIS::PDU_ENTITY_STATE &&
        pduType != DIS::PDU_FIRE &&
        pduType != DIS::PDU_DETONATION) {
        return false;
    }

    // Big-endian read of site (bytes 12-13) and app (bytes 14-15)
    site = static_cast<uint16_t>(
        (static_cast<uint8_t>(pduBytes[12]) << 8) |
        static_cast<uint8_t>(pduBytes[13]));

    app = static_cast<uint16_t>(
        (static_cast<uint8_t>(pduBytes[14]) << 8) |
        static_cast<uint8_t>(pduBytes[15]));

    return true;
}
