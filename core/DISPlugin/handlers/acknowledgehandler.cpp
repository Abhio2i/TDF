// =============================================================================
// FILE:        acknowledgehandler.cpp
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "acknowledgehandler.h"

#include <dis7/AcknowledgePdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>

#include "core/DISPlugin/utils/dislogger.h"

// =============================================================================
// Constructor
// =============================================================================
AcknowledgeHandler::AcknowledgeHandler(QObject* parent)
    : QObject(parent)
{
}

// =============================================================================
// setMapper
// =============================================================================
void AcknowledgeHandler::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}

// =============================================================================
// onAcknowledgeReceived
// Called from PDUDispatcher when a type-15 PDU arrives
// Unmarshal → resolve engine ID → emit incomingAcknowledge
// =============================================================================
void AcknowledgeHandler::onAcknowledgeReceived(QByteArray   pduBytes,
                                               QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::AcknowledgePdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    // ── Extract originating entity ID (who sent the original request) ─────────
    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    uint16_t site   = simAddr.getSite();
    uint16_t app    = simAddr.getApplication();
    uint16_t entity = originID.getEntityNumber();

    // Resolve to engine ID if known
    std::string engineID = m_mapper
                               ? m_mapper->getEngineID(site, app, entity)
                               : "";

    DISIncomingAcknowledge ack;
    ack.originatingEntityID = engineID;
    ack.acknowledgeFlag     = pdu.getAcknowledgeFlag();
    ack.responseFlag        = pdu.getResponseFlag();
    ack.requestID           = pdu.getRequestID();

    DIS_LOG_BASIC("[AcknowledgeHandler] Acknowledge received"
                  << "flag="      << ack.acknowledgeFlag
                  << "response="  << ack.responseFlag
                  << "requestID=" << ack.requestID
                  << "from site=" << site
                  << "app="       << app);

    emit incomingAcknowledge(ack);
}
