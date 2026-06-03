// =============================================================================
// FILE:        iffhandler.cpp
// =============================================================================
#include "iffhandler.h"
#include <dis7/IFFPdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>
#include "core/DISPlugin/utils/dislogger.h"
#include <QHostAddress>

IFFHandler::IFFHandler(QObject* parent) : QObject(parent) {}

void IFFHandler::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}

void IFFHandler::onIFFReceived(const QByteArray& data,
                               const QHostAddress& sender,
                               quint16 senderPort)
{
    Q_UNUSED(sender) Q_UNUSED(senderPort)

    try {
        DIS::DataStream ds(
            const_cast<char*>(data.constData()),
            data.size(),
            DIS::BIG);

        DIS::IFFPdu pdu;
        pdu.unmarshal(ds);

        // Get emitting entity ID
        const DIS::EntityID& eid = pdu.getEmittingEntityID();
        if (!m_mapper) return;

        std::string engineID = m_mapper->getEngineID(
            DISEntityID{
                eid.getSimulationAddress().getSite(),
                eid.getSimulationAddress().getApplication(),
                eid.getEntityNumber()
            });

        if (engineID.empty()) {
            // Entity not yet known — use DIS ID as string key
            engineID = QString("%1-%2-%3")
                           .arg(eid.getSimulationAddress().getSite())
                           .arg(eid.getSimulationAddress().getApplication())
                           .arg(eid.getEntityNumber())
                           .toStdString();
        }

        DISIncomingIFF incoming;
        incoming.emittingEntityID = engineID;

        // Extract system status — bit 7 = system on
        const DIS::FundamentalOperationalData& fod =
            pdu.getFundamentalOperationalData();
        incoming.systemOn = (fod.getSystemStatus() & 0x80) != 0;

        // Extract mode codes
        // InformationLayer = Mode 1, InformationLayer2 = Mode 2,
        // IffMode3 = Mode 3A
        incoming.mode1Code   = static_cast<uint16_t>(fod.getParameter1());
        incoming.mode2Code   = static_cast<uint16_t>(fod.getParameter2());
        incoming.mode3ACode  = static_cast<uint16_t>(fod.getParameter3());
        incoming.mode4Active = (fod.getParameter4() & 0x0001) != 0;
        DIS_LOG_BASIC("[IFFHandler] Incoming IFF from:"
                      << QString::fromStdString(engineID)
                      << "Mode3A=" << incoming.mode3ACode
                      << "On=" << incoming.systemOn);

        emit incomingIFF(incoming);

    } catch (...) {
        DIS_LOG_WARNING("[IFFHandler] Failed to parse IFF PDU");
    }
}
