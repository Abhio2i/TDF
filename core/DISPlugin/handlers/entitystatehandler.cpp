// =============================================================================
// FILE:        EntityStateHandler.cpp
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "entitystatehandler.h"

#include <dis7/EntityStatePdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>

#include <QDebug>

// =============================================================================
// Constructor
// =============================================================================
EntityStateHandler::EntityStateHandler(QObject* parent)
    : QObject(parent)
{
}

EntityStateHandler::~EntityStateHandler() = default;

void EntityStateHandler::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}

// =============================================================================
// entityKey
// Build a unique string key from DIS EntityID triplet
// Used to track DR state per entity
// =============================================================================
QString EntityStateHandler::entityKey(uint16_t site, uint16_t app, uint16_t entity)
{
    return QString("%1:%2:%3").arg(site).arg(app).arg(entity);
}

// =============================================================================
// onEntityStateReceived
// Called from PDUDispatcher on network thread
// Unmarshals PDU → applies dead reckoning → emits incomingTransform
// =============================================================================
void EntityStateHandler::onEntityStateReceived(QByteArray   pduBytes,
                                               QHostAddress sender)
{
    Q_UNUSED(sender)

    // ── Unmarshal using dis7 ──────────────────────────────────────────────────
    DIS::EntityStatePdu pdu;

    // DataStream reads from buffer
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    // ── Extract EntityID ──────────────────────────────────────────────────────
    const DIS::EntityID& entityID = pdu.getEntityID();
    const DIS::SimulationAddress& simAddr = entityID.getSimulationAddress();

    uint16_t site   = simAddr.getSite();
    uint16_t app    = simAddr.getApplication();
    uint16_t entity = entityID.getEntityNumber();

    QString key = entityKey(site, app, entity);

    // ── Look up engine entity ID ──────────────────────────────────────────────
    std::string engineID = m_mapper
                               ? m_mapper->getEngineID(site, app, entity)
                               : "";

    bool isNewEntity = engineID.empty();

    if (isNewEntity) {
        // Entity not yet in our mapper — it is a remote entity
        // Register it so subsequent PDUs can find it
        //if (m_mapper && m_mapper->isOwnApplication(site, app)) return;
        if (m_mapper && m_mapper->isOwnApplication(site, app)) {
            qDebug() << "[EntityStateHandler] DROPPING PDU — own application:"
                     << "site=" << site << "app=" << app
                     << "entity=" << entity;
            return;
        }

        engineID = key.toStdString();
        if (m_mapper) {
            DISEntityID disID;
            disID.site        = site;
            disID.application = app;
            disID.entity      = entity;
            m_mapper->registerRemoteEntity(engineID, disID);
        }
        emit newEntityDiscovered(key, sender);
    }

    // Skip if this is OUR OWN entity
    // (we sent this PDU — no need to apply it to ourselves)
    if (m_mapper && m_mapper->isLocalEntity(engineID)) return;

    // ── Extract position (ECEF) ───────────────────────────────────────────────
    const DIS::Vector3Double& loc = pdu.getEntityLocation();
    double ecefX = loc.getX();
    double ecefY = loc.getY();
    double ecefZ = loc.getZ();

    // ── Extract orientation ───────────────────────────────────────────────────
    const DIS::EulerAngles& ori = pdu.getEntityOrientation();
    float psi   = ori.getPsi();
    float theta = ori.getTheta();
    float phi   = ori.getPhi();

    // ── Extract velocity ──────────────────────────────────────────────────────
    const DIS::Vector3Float& vel = pdu.getEntityLinearVelocity();
    float velX = vel.getX();
    float velY = vel.getY();
    float velZ = vel.getZ();

    // ── Extract DR algorithm ──────────────────────────────────────────────────
    const DIS::DeadReckoningParameters& drParams = pdu.getDeadReckoningParameters();
    uint8_t drAlgo = drParams.getDeadReckoningAlgorithm();

    // ── Update dead reckoning state ───────────────────────────────────────────
    DeadReckoningState& drState = m_drStates[key];
    DeadReckoning::updateState(
        drState,
        ecefX, ecefY, ecefZ,
        velX, velY, velZ,
        psi, theta, phi,
        static_cast<DRAlgorithm>(drAlgo)
        );

    // ── Convert ECEF to geocord for engine ────────────────────────────────────
    ECEFPosition ecef;
    ecef.x = ecefX; ecef.y = ecefY; ecef.z = ecefZ;

    double lat, lon, alt_feet;
    CoordConverter::ecefToGeocord(ecef, lat, lon, alt_feet);

    // ── Convert Euler to heading/pitch/roll ───────────────────────────────────
    DISOrientation euler;
    euler.psi = psi; euler.theta = theta; euler.phi = phi;

    float heading, pitch, roll;
    CoordConverter::eulerToHeading(euler, heading, pitch, roll);

    // ── Convert ECEF velocity to NED ─────────────────────────────────────────
    float northVel, eastVel, vertVel;
    DISVelocity disVel;
    disVel.x = velX; disVel.y = velY; disVel.z = velZ;
    CoordConverter::ecefToNEDVelocity(disVel, lat, lon, northVel, eastVel, vertVel);

    // ── Extract marking ───────────────────────────────────────────────────────
    const DIS::EntityMarking& marking = pdu.getMarking();
    std::string markingStr(marking.getCharacters(), 11);
    // Trim null chars
    markingStr = markingStr.substr(0, markingStr.find('\0'));

    // ── Build DISIncomingTransform ────────────────────────────────────────────
    DISIncomingTransform update;
    update.entityID   = engineID;
    update.latitude   = lat;
    update.longitude  = lon;
    update.altitude   = alt_feet;   // FEET for your geocord
    update.heading    = heading;
    update.pitch      = pitch;
    update.roll       = roll;
    update.northVel   = northVel;
    update.eastVel    = eastVel;
    update.vertVel    = vertVel;
    update.active     = true;
    update.domain     = pdu.getEntityType().getDomain();
    update.category    = pdu.getEntityType().getCategory();     // ← ADD
    update.subcategory = pdu.getEntityType().getSubcategory();  // ← ADD
    update.forceID    = pdu.getForceId();
    update.marking    = markingStr;

    emit incomingTransform(update);
}
