// =============================================================================
// FILE:        WarfareHandlers.cpp
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "warfarehandlers.h"

#include <dis7/FirePdu.h>
#include <dis7/DetonationPdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>

#include <QDebug>

// =============================================================================
// FireHandler
// =============================================================================
FireHandler::FireHandler(QObject* parent) : QObject(parent) {}

void FireHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void FireHandler::onFireReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::FirePdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    // ── Extract firing entity ─────────────────────────────────────────────────
    const DIS::EntityID& firingID = pdu.getFiringEntityID();
    const DIS::SimulationAddress& firingAddr = firingID.getSimulationAddress();

    std::string firingEngineID = m_mapper
                                     ? m_mapper->getEngineID(firingAddr.getSite(),
                                                             firingAddr.getApplication(),
                                                             firingID.getEntityNumber())
                                     : "";

    // ── Extract target entity ─────────────────────────────────────────────────
    const DIS::EntityID& targetID = pdu.getTargetEntityID();
    const DIS::SimulationAddress& targetAddr = targetID.getSimulationAddress();

    std::string targetEngineID = m_mapper
                                     ? m_mapper->getEngineID(targetAddr.getSite(),
                                                             targetAddr.getApplication(),
                                                             targetID.getEntityNumber())
                                     : "";

    // ── Extract location ──────────────────────────────────────────────────────
    const DIS::Vector3Double& loc = pdu.getLocationInWorldCoordinates();
    ECEFPosition ecef;
    ecef.x = loc.getX(); ecef.y = loc.getY(); ecef.z = loc.getZ();

    double lat, lon, alt_feet;
    CoordConverter::ecefToGeocord(ecef, lat, lon, alt_feet);

    // ── Build DISIncomingFire ─────────────────────────────────────────────────
    DISIncomingFire event;
    event.firingEntityID = firingEngineID;
    event.targetEntityID = targetEngineID;
    event.latitude       = lat;
    event.longitude      = lon;
    event.altitude       = alt_feet;

    emit incomingFire(event);

    qDebug() << "[FireHandler] Fire PDU:"
             << QString::fromStdString(firingEngineID)
             << "→"
             << QString::fromStdString(targetEngineID);
}

// =============================================================================
// DetonationHandler
// =============================================================================
DetonationHandler::DetonationHandler(QObject* parent) : QObject(parent) {}

void DetonationHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void DetonationHandler::onDetonationReceived(QByteArray pduBytes,
                                             QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::DetonationPdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    // ── Extract firing entity ─────────────────────────────────────────────────
    const DIS::EntityID& firingID = pdu.getFiringEntityID();
    const DIS::SimulationAddress& firingAddr = firingID.getSimulationAddress();

    std::string firingEngineID = m_mapper
                                     ? m_mapper->getEngineID(firingAddr.getSite(),
                                                             firingAddr.getApplication(),
                                                             firingID.getEntityNumber())
                                     : "";

    // ── Extract target entity ─────────────────────────────────────────────────
    const DIS::EntityID& targetID = pdu.getTargetEntityID();
    const DIS::SimulationAddress& targetAddr = targetID.getSimulationAddress();

    std::string targetEngineID = m_mapper
                                     ? m_mapper->getEngineID(targetAddr.getSite(),
                                                             targetAddr.getApplication(),
                                                             targetID.getEntityNumber())
                                     : "";

    // ── Extract exploding entity ──────────────────────────────────────────────
    const DIS::EntityID& explodingID = pdu.getExplodingEntityID();
    const DIS::SimulationAddress& explodingAddr = explodingID.getSimulationAddress();

    std::string munitionEngineID = m_mapper
                                       ? m_mapper->getEngineID(explodingAddr.getSite(),
                                                               explodingAddr.getApplication(),
                                                               explodingID.getEntityNumber())
                                       : "";

    // ── Extract location ──────────────────────────────────────────────────────
    const DIS::Vector3Double& loc = pdu.getLocationInWorldCoordinates();
    ECEFPosition ecef;
    ecef.x = loc.getX(); ecef.y = loc.getY(); ecef.z = loc.getZ();

    double lat, lon, alt_feet;
    CoordConverter::ecefToGeocord(ecef, lat, lon, alt_feet);

    // ── Build DISIncomingDetonation ───────────────────────────────────────────
    DISIncomingDetonation event;
    event.firingEntityID    = firingEngineID;
    event.targetEntityID    = targetEngineID;
    event.munitionID        = munitionEngineID;
    event.latitude          = lat;
    event.longitude         = lon;
    event.altitude          = alt_feet;
    event.detonationResult  = pdu.getDetonationResult();
    event.blastRadius       = 0.0f;  // not in DIS PDU — set by local damage model

    emit incomingDetonation(event);

    qDebug() << "[DetonationHandler] Detonation result:"
             << event.detonationResult
             << "at lat=" << lat << "lon=" << lon;
}
