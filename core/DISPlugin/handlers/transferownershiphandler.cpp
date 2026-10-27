// =============================================================================
// FILE:        transferownershiphandler.cpp
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// Manual byte parsing — TransferOwnershipPdu.h not in this dis7 build.
// Wire offsets per IEEE 1278.1 Transfer Ownership PDU (type 35):
//   [12-17] Originating Entity ID
//   [18-23] Receiving Entity ID
//   [24-27] Request ID (uint32 BE)
//   [28]    Required Reliability Service
//   [29]    Transfer Type
//   [30-35] Transfer Entity ID
//   [36-37] Number of Record Sets
//   [38-39] Padding
// Total: 40 bytes
// =============================================================================
#include "transferownershiphandler.h"
#include "core/DISPlugin/utils/dislogger.h"
#include <dis7/transferownershippdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>
#include <dis7/transferownershippdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>
// =============================================================================
// Constructor
// =============================================================================
TransferOwnershipHandler::TransferOwnershipHandler(QObject* parent)
    : QObject(parent)
{
}

// =============================================================================
// setMapper
// =============================================================================
void TransferOwnershipHandler::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}

// =============================================================================
// onTransferOwnershipReceived
// Called from PDUDispatcher when a type-35 PDU arrives on network thread.
// 1. Parse bytes manually
// 2. Emit acknowledgeNeeded immediately (DirectConnection to PDUSender)
// 3. Emit incomingTransferOwnership (QueuedConnection to plugin main thread)
// =============================================================================
void TransferOwnershipHandler::onTransferOwnershipReceived(QByteArray   pduBytes,
                                                           QHostAddress sender)
{
    Q_UNUSED(sender)

    if (pduBytes.size() < 40) {
        DIS_LOG_WARNING("[TransferOwnershipHandler] PDU too short:"
                        << pduBytes.size() << "bytes — expected 40");
        return;
    }

    // ── Parse using DIS::TransferOwnershipPdu ────────────────────────────────
    DIS::TransferOwnershipPdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    // ── Ignore transfers not addressed to us ──────────────────────────────────
    // Filters bystander machines on a shared multicast group, and — critically —
    // filters our OWN broadcast looping back to us via MulticastLoopbackOption,
    // which was causing this machine to re-take ownership of an entity it had
    // just given up (giveUpOwnership immediately followed by takeOwnership).
    const DIS::EntityID& receivingID = pdu.getReceivingEntityID();
    const DIS::SimulationAddress& receivingAddr = receivingID.getSimulationAddress();
    uint16_t receivingSite = receivingAddr.getSite();
    uint16_t receivingApp  = receivingAddr.getApplication();

    if (m_mapper && !m_mapper->isOwnApplication(receivingSite, receivingApp)) {
        return;
    }

    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    uint16_t originSite   = simAddr.getSite();
    uint16_t originApp    = simAddr.getApplication();
    uint16_t originEntity = originID.getEntityNumber();

    const DIS::EntityID& transferID = pdu.getTransferEntityID();
    const DIS::SimulationAddress& transferAddr = transferID.getSimulationAddress();

    uint16_t transferSite   = transferAddr.getSite();
    uint16_t transferApp    = transferAddr.getApplication();
    uint16_t transferEntity = transferID.getEntityNumber();

    uint32_t requestID    = pdu.getRequestID();
    uint8_t  transferType = pdu.getTransferType();

    // ── Resolve engine ID from mapper ─────────────────────────────────────────
    std::string engineID = m_mapper
                               ? m_mapper->getEngineID(transferSite, transferApp, transferEntity)
                               : "";

    // If not in mapper, build the key EntityStateHandler would use
    if (engineID.empty()) {
        engineID = QString("%1:%2:%3")
        .arg(transferSite)
            .arg(transferApp)
            .arg(transferEntity)
            .toStdString();
    }

    DIS_LOG_BASIC("[TransferOwnershipHandler] Transfer Ownership received"
                  << "entity=" << QString::fromStdString(engineID)
                  << "type="   << transferType
                  << "requestID=" << requestID
                  << "from site=" << originSite
                  << "app="       << originApp);

    // ── Step 1: Send Acknowledge (AbleToComply) immediately ──────────────────
    // DirectConnection — fires on same network thread before returning
    // DISAcknowledgeData ack;
    // ack.originSite      = originSite;
    // ack.originApp       = originApp;
    // ack.originEntity    = originEntity;
    // ack.acknowledgeFlag = DISAcknowledgeFlag::TransferOwnership;
    // ack.responseFlag    = DISResponseFlag::AbleToComply;
    // ack.requestID       = requestID;
    // emit acknowledgeNeeded(ack);

    // ── Step 2: Notify plugin to take ownership ───────────────────────────────
    // QueuedConnection — crosses to main thread
    DISIncomingTransferOwnership data;
    data.transferEntityID     = engineID;
    data.transferEntitySite   = transferSite;
    data.transferEntityApp    = transferApp;
    data.transferEntityEntity = transferEntity;
    data.originSite           = originSite;
    data.originApp            = originApp;
    data.originEntity         = originEntity;
    data.transferType         = transferType;
    data.requestID            = requestID;
    emit incomingTransferOwnership(data);
}
