// =============================================================================
// FILE:        PDUSerializer.cpp
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "pduserializer.h"

// ── dis7 library includes ─────────────────────────────────────────────────────
#include <dis7/EntityStatePdu.h>
#include <dis7/FirePdu.h>
#include <dis7/DetonationPdu.h>
#include <dis7/StartResumePdu.h>
#include <dis7/StopFreezePdu.h>
#include <dis7/RemoveEntityPdu.h>
#include <dis7/CreateEntityPdu.h>
#include <dis7/IFFPdu.h>
#include <dis7/EntityID.h>
#include <dis7/SimulationAddress.h>
#include <dis7/EntityType.h>
#include <dis7/Vector3Float.h>
#include <dis7/Vector3Double.h>
#include <dis7/EulerAngles.h>
#include <dis7/DeadReckoningParameters.h>
#include <dis7/EntityMarking.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>
#include <dis7/utils/PDUType.h>
#include <dis7/MunitionDescriptor.h>
#include <dis7/ClockTime.h>
#include <QDebug>
#include <chrono>
#include <cstring>
#include "core/DISPlugin/utils/dislogger.h"
#include <dis7/AcknowledgePdu.h>
#include <dis7/transferownershippdu.h>
#include <dis7/ElectromagneticEmissionsPdu.h>
#include <dis7/DesignatorPdu.h>
#include <dis7/transmitterpdu.h>
#include <dis7/signalpdu.h>
#include <dis7/DataQueryPdu.h>
#include <dis7/SetDataPdu.h>
#include <dis7/DataPdu.h>
// =============================================================================
// buildDISEntityID
// DIS::EntityID uses SimulationAddress (site+app) + entityNumber
// NOT setSite/setApplication/setEntity directly
// =============================================================================
static DIS::EntityID buildDISEntityID(uint16_t site,
                                      uint16_t application,
                                      uint16_t entityNumber)
{
    DIS::SimulationAddress simAddr;
    simAddr.setSite(site);
    simAddr.setApplication(application);

    DIS::EntityID id;
    id.setSimulationAddress(simAddr);
    id.setEntityNumber(entityNumber);
    return id;
}

// =============================================================================
// marshalToQByteArray
// Marshal any PDU to QByteArray using BIG (network byte order)
// Endian enum: DIS::BIG=0, DIS::LITTLE=1
// =============================================================================
template<typename PDUClass>
static QByteArray marshalToQByteArray(PDUClass& pdu)
{
    DIS::DataStream ds(DIS::BIG);  // BIG not BIG_ENDIAN
    pdu.marshal(ds);

    int size = static_cast<int>(ds.size());
    QByteArray result(size, 0);
    for (int i = 0; i < size; ++i)
        result[i] = ds[static_cast<unsigned int>(i)];
    return result;
}

// =============================================================================
// generateTimestamp
// =============================================================================
uint32_t PDUSerializer::generateTimestamp(bool useAbsolute)
{
    using namespace std::chrono;
    auto now         = system_clock::now();
    auto topOfHour   = floor<hours>(now);   // truncates to current hour boundary
    auto sinceMs     = duration_cast<milliseconds>(now - topOfHour).count();

    // DIS timestamp: upper 31 bits = time units since top of hour
    // 1 unit = 3600s / 2^31 ≈ 1.676 microseconds
    uint32_t ts = static_cast<uint32_t>(
        (static_cast<uint64_t>(sinceMs) * 2147483647ULL) / 3600000ULL
        );

    if (useAbsolute) {
        ts |= 1U;   // bit 0 = 1 → absolute timestamp (IEEE 1278.1)
    } else {
        ts &= ~1U;  // bit 0 = 0 → relative timestamp
    }
    return ts;
}

// =============================================================================
// buildEntityID
// =============================================================================
bool PDUSerializer::buildEntityID(const std::string&    engineID,
                                  const EntityIDMapper& mapper,
                                  uint16_t& site,
                                  uint16_t& app,
                                  uint16_t& entity)
{
    DISEntityID disID = mapper.getDISID(engineID);
    if (!disID.isValid()) {
        DIS_LOG_WARNING("[PDUSerializer] Entity not in mapper:"
                   << QString::fromStdString(engineID));
        return false;
    }
    site   = disID.site;
    app    = disID.application;
    entity = disID.entity;
    return true;
}

// =============================================================================
// serializeEntityState
// =============================================================================
QByteArray PDUSerializer::serializeEntityState(const DISEntitySnapshot& snap,
                                               const DISConfig&         config,
                                               const EntityIDMapper&    mapper)
{
    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.entityID, mapper, site, app, entityNum))
        return QByteArray();

    // if (!CoordConverter::isValidGeocord(snap.latitude, snap.longitude, snap.altitude)) {
    //     qWarning() << "[PDUSerializer] Invalid geocord for entity:"
    //                << QString::fromStdString(snap.entityID)
    //                << "lat=" << snap.latitude
    //                << "lon=" << snap.longitude
    //                << "alt=" << snap.altitude;
    //     return QByteArray();
    // }

    // // Coordinate conversion
    // ECEFPosition ecef = CoordConverter::geocordToECEF(
    //     snap.latitude, snap.longitude, snap.altitude);
    // Submarines (domain=4) operate at negative altitudes — skip the
    // geocord altitude check for subsurface entities only.
    // All other entities go through the full validity check as before.
    if (snap.entityType.domain != 4) {
        if (!CoordConverter::isValidGeocord(snap.latitude, snap.longitude, snap.altitude)) {
            DIS_LOG_WARNING("[PDUSerializer] Invalid geocord for entity:"
                       << QString::fromStdString(snap.entityID)
                       << "lat=" << snap.latitude
                       << "lon=" << snap.longitude
                       << "alt=" << snap.altitude);
            return QByteArray();
        }
    }

    ECEFPosition ecef = CoordConverter::geocordToECEF(
        snap.latitude, snap.longitude, snap.altitude);

    // DISOrientation euler = CoordConverter::headingToEuler(
    //     snap.heading, snap.pitch, snap.roll);
    DISOrientation euler = CoordConverter::headingToEuler(
        snap.heading, snap.pitch, snap.roll,
        snap.latitude, snap.longitude);
    // Correct field names from DISEntitySnapshot:
    // northVel, eastVel, verticalVel
    DISVelocity vel = CoordConverter::nedToECEFVelocity(
        snap.northVel,
        snap.eastVel,
        snap.verticalVel,
        snap.latitude,
        snap.longitude
        );

    DIS::EntityStatePdu pdu;

    // Header
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_ENTITY_STATE);
    pdu.setProtocolFamily(1);
    //pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    // Entity ID — via SimulationAddress
    pdu.setEntityID(buildDISEntityID(site, app, entityNum));

    // Force ID
    pdu.setForceId(snap.forceID);

    // Entity Type — from DISEntityTypeData
    DIS::EntityType entityType;
    entityType.setEntityKind(snap.entityType.kind);
    entityType.setDomain(snap.entityType.domain);
    entityType.setCountry(snap.entityType.country);
    entityType.setCategory(snap.entityType.category);
    entityType.setSubcategory(snap.entityType.subcategory);
    entityType.setSpecific(snap.entityType.specific);
    entityType.setExtra(snap.entityType.extra);
    pdu.setEntityType(entityType);

    // Position
    DIS::Vector3Double location;
    location.setX(ecef.x);
    location.setY(ecef.y);
    location.setZ(ecef.z);
    pdu.setEntityLocation(location);

    // Orientation
    DIS::EulerAngles orientation;
    orientation.setPsi(euler.psi);
    orientation.setTheta(euler.theta);
    orientation.setPhi(euler.phi);
    pdu.setEntityOrientation(orientation);

    // Velocity
    DIS::Vector3Float velocity;
    velocity.setX(vel.x);
    velocity.setY(vel.y);
    velocity.setZ(vel.z);
    pdu.setEntityLinearVelocity(velocity);

    // Dead reckoning
    DIS::DeadReckoningParameters drParams;
    drParams.setDeadReckoningAlgorithm(snap.deadReckoningAlgorithm);
    DIS::Vector3Float zeroVec;
    zeroVec.setX(0.0f); zeroVec.setY(0.0f); zeroVec.setZ(0.0f);
    drParams.setEntityLinearAcceleration(zeroVec);
    drParams.setEntityAngularVelocity(zeroVec);
    pdu.setDeadReckoningParameters(drParams);

    // Marking
    // Marking — IEEE 1278.1 format: character set byte + 11 chars null-padded
    // Character set 1 = ASCII
    // Array must be zero-filled first, then copy at most 10 chars leaving
    // byte[10] always zero. This guarantees null termination on all receivers
    // including strict ones like STAGE and VR-Forces.
    DIS::EntityMarking marking;
    marking.setCharacterSet(1);
    char markingChars[11];
    std::memset(markingChars, 0, sizeof(markingChars));
    std::string markStr = snap.marking.substr(0, 10);
    std::strncpy(markingChars, markStr.c_str(), 10);
    marking.setCharacters(markingChars);
    pdu.setMarking(marking);

    pdu.setEntityAppearance(snap.appearance);
    pdu.setCapabilities(0);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeFire
// DISFireSnapshot fields: firingEntityID, targetEntityID, munitionID,
//                         latitude, longitude, altitude,
//                         velocityX, velocityY, velocityZ, range
// =============================================================================
QByteArray PDUSerializer::serializeFire(const DISFireSnapshot& snap,
                                        const DISConfig&       config,
                                        const EntityIDMapper&  mapper)
{
    DIS::FirePdu pdu;

    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_FIRE);
    pdu.setProtocolFamily(2);
  //  pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    uint16_t site, app, entity;

    if (!buildEntityID(snap.firingEntityID, mapper, site, app, entity))
        return QByteArray();
    pdu.setFiringEntityID(buildDISEntityID(site, app, entity));

    if (!snap.targetEntityID.empty())
        if (buildEntityID(snap.targetEntityID, mapper, site, app, entity))
            pdu.setTargetEntityID(buildDISEntityID(site, app, entity));

    if (!snap.munitionID.empty())
        if (buildEntityID(snap.munitionID, mapper, site, app, entity))
            pdu.setMunitionExpendibleID(buildDISEntityID(site, app, entity));

    ECEFPosition ecef = CoordConverter::geocordToECEF(
        snap.latitude, snap.longitude, snap.altitude);
    DIS::Vector3Double location;
    location.setX(ecef.x); location.setY(ecef.y); location.setZ(ecef.z);
    pdu.setLocationInWorldCoordinates(location);

    // DISFireSnapshot stores velocity as velNorth/velEast/velVertical (NED m/s)
    // Convert to ECEF for DIS wire format
    DISVelocity vel = CoordConverter::nedToECEFVelocity(
        snap.velNorth,
        snap.velEast,
        snap.velVertical,
        snap.latitude,
        snap.longitude
        );
    DIS::Vector3Float velocity;
    velocity.setX(vel.x);
    velocity.setY(vel.y);
    velocity.setZ(vel.z);
    pdu.setVelocity(velocity);

    pdu.setRange(snap.range);

    DIS::MunitionDescriptor descriptor;
    descriptor.setWarhead(0);
    descriptor.setFuse(0);
    descriptor.setQuantity(1);
    descriptor.setRate(0);
    pdu.setDescriptor(descriptor);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeDetonation
// =============================================================================
QByteArray PDUSerializer::serializeDetonation(const DISDetonationSnapshot& snap,
                                              const DISConfig&             config,
                                              const EntityIDMapper&        mapper)
{
    DIS::DetonationPdu pdu;

    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_DETONATION);
    pdu.setProtocolFamily(2);
   // pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));
    uint16_t site, app, entity;

    if (!snap.firingEntityID.empty())
        if (buildEntityID(snap.firingEntityID, mapper, site, app, entity))
            pdu.setFiringEntityID(buildDISEntityID(site, app, entity));

    if (!snap.targetEntityID.empty())
        if (buildEntityID(snap.targetEntityID, mapper, site, app, entity))
            pdu.setTargetEntityID(buildDISEntityID(site, app, entity));

    ECEFPosition ecef = CoordConverter::geocordToECEF(
        snap.latitude, snap.longitude, snap.altitude);
    DIS::Vector3Double location;
    location.setX(ecef.x); location.setY(ecef.y); location.setZ(ecef.z);
    pdu.setLocationInWorldCoordinates(location);

    pdu.setDetonationResult(snap.detonationResult);

    DIS::Vector3Float velocity;
    velocity.setX(0.0f);
    velocity.setY(0.0f);
    velocity.setZ(0.0f);
    pdu.setVelocity(velocity);

    DIS::MunitionDescriptor descriptor;
    descriptor.setWarhead(0);
    descriptor.setFuse(0);
    descriptor.setQuantity(1);
    descriptor.setRate(0);
    pdu.setDescriptor(descriptor);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeStartResume
// =============================================================================
QByteArray PDUSerializer::serializeStartResume(const DISStartResumeData& data,
                                               const DISConfig&          config)
{
    Q_UNUSED(data)

    DIS::StartResumePdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_START_RESUME);
    pdu.setProtocolFamily(5);
    //pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    DIS::EntityID zeroID = buildDISEntityID(config.siteID, config.applicationID, 0);
    pdu.setOriginatingEntityID(zeroID);
    pdu.setReceivingEntityID(zeroID);

    static uint32_t requestID = 0;
    pdu.setRequestID(++requestID);

    DIS::ClockTime zeroTime;
    zeroTime.setHour(0);
    zeroTime.setTimePastHour(0);
    pdu.setRealWorldTime(zeroTime);
    pdu.setSimulationTime(zeroTime);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeStopFreeze
// =============================================================================
QByteArray PDUSerializer::serializeStopFreeze(const DISStopFreezeData& data,
                                              const DISConfig&         config)
{
    DIS::StopFreezePdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_STOP_FREEZE);
    pdu.setProtocolFamily(5);
    //pdu.setTimestamp(generateTimestamp());
   pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));
    DIS::EntityID zeroID = buildDISEntityID(config.siteID, config.applicationID, 0);
    pdu.setOriginatingEntityID(zeroID);
    pdu.setReceivingEntityID(zeroID);

    pdu.setReason(static_cast<unsigned char>(data.reason));
    pdu.setFrozenBehavior(0);

    static uint32_t requestID = 0;
    pdu.setRequestID(++requestID);

    DIS::ClockTime zeroTime;
    zeroTime.setHour(0);
    zeroTime.setTimePastHour(0);
    pdu.setRealWorldTime(zeroTime);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeRemoveEntity
// =============================================================================
QByteArray PDUSerializer::serializeRemoveEntity(const DISRemoveEntityData& data,
                                                const DISConfig&           config,
                                                const EntityIDMapper&      mapper)
{
    DIS::RemoveEntityPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_REMOVE_ENTITY);
    pdu.setProtocolFamily(5);
    //pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(
        buildDISEntityID(config.siteID, config.applicationID, 0));

    uint16_t site, app, entity;
    if (buildEntityID(data.removedEntityID, mapper, site, app, entity))
        pdu.setReceivingEntityID(buildDISEntityID(site, app, entity));

    static uint32_t requestID = 0;
    pdu.setRequestID(++requestID);

    return marshalToQByteArray(pdu);
}

// =============================================================================
// serializeCreateEntity
// =============================================================================
QByteArray PDUSerializer::serializeCreateEntity(const DISCreateEntityData& data,
                                                const DISConfig&           config,
                                                const EntityIDMapper&      mapper)
{
    DIS::CreateEntityPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_CREATE_ENTITY);
    pdu.setProtocolFamily(5);
   // pdu.setTimestamp(generateTimestamp());
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(
        buildDISEntityID(config.siteID, config.applicationID, 0));

    uint16_t site, app, entity;
    if (buildEntityID(data.newEntityID, mapper, site, app, entity))
        pdu.setReceivingEntityID(buildDISEntityID(site, app, entity));

    static uint32_t requestID = 0;
    pdu.setRequestID(++requestID);

    return marshalToQByteArray(pdu);
}
// =============================================================================
// serializeAcknowledge
// IEEE 1278.1 type 15 — sent in response to:
//   CreateEntity (flag=1), RemoveEntity (flag=2),
//   StartResume (flag=3), StopFreeze (flag=4)
//
// Originating entity = who sent the request (from DISAcknowledgeData)
// Receiving entity   = us (our siteID/applicationID)
// No EntityIDMapper needed — uses raw DIS triplets
// =============================================================================
QByteArray PDUSerializer::serializeAcknowledge(const DISAcknowledgeData& data,
                                               const DISConfig&          config)
{
    DIS::AcknowledgePdu pdu;

    // ── Header ────────────────────────────────────────────────────────────────
    // Protocol version hardcoded to 7 (IEEE 1278.1-2012), not config.defaultVersion.
    // DIS::AcknowledgePdu is a single dis7-library class — its body layout is
    // always the 2012 format regardless of what version byte gets stamped here.
    // Stamping anything else (e.g. exercise-wide v6) would label a v7 body as
    // v6, which a strict receiver may reject or misparse.
    pdu.setProtocolVersion(7);
    pdu.setExerciseID     (config.exerciseID);
    pdu.setPduType        (15);  // Acknowledge
    pdu.setProtocolFamily (5);   // Simulation Management Family
    pdu.setTimestamp      (generateTimestamp(config.useAbsTimestamp));

    // ── Originating entity — who sent us the request we are acknowledging ─────
    pdu.setOriginatingEntityID(
        buildDISEntityID(data.originSite, data.originApp, data.originEntity));

    // ── Receiving entity — us ─────────────────────────────────────────────────
    pdu.setReceivingEntityID(
        buildDISEntityID(config.siteID, config.applicationID, 0));

    // ── Acknowledge fields ────────────────────────────────────────────────────
    pdu.setAcknowledgeFlag(data.acknowledgeFlag);
    pdu.setResponseFlag   (data.responseFlag);
    pdu.setRequestID      (data.requestID);

    return marshalToQByteArray(pdu);
}
// =============================================================================
// serializeIFF
// Builds DIS IFF PDU (type 28) from engine IFF state snapshot.
// Maps engine mode codes → FundamentalOperationalData fields.
// System status byte: bit 7 = on/off, bits 0-6 = mode capabilities.
// Mode code layout: upper 12 bits = code, lower 4 bits = status flags.
// =============================================================================
QByteArray PDUSerializer::serializeIFF(const DISIFFSnapshot&  snap,
                                       const DISConfig&        config,
                                       const EntityIDMapper&   mapper)
{
    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.entityID, mapper, site, app, entityNum))
        return QByteArray();

    DIS::IFFPdu pdu;

    // ── Header ────────────────────────────────────────────────────────────────
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(28);  // IFF
    pdu.setProtocolFamily(6);  // Distributed Emissions Family
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    // ── Emitting entity ID ────────────────────────────────────────────────────
    pdu.setEmittingEntityID(buildDISEntityID(site, app, entityNum));

    // ── Event ID — use entity number as event counter ─────────────────────────
    DIS::EventIdentifier eventID;
    DIS::SimulationAddress simAddr;
    simAddr.setSite(site);
    simAddr.setApplication(app);
    eventID.setSimulationAddress(simAddr);
    eventID.setEventNumber(static_cast<unsigned short>(entityNum));
    pdu.setEventID(eventID);

    // ── Relative antenna location — zero offset (antenna at entity origin) ─────
    DIS::Vector3Float antennaLoc;
    antennaLoc.setX(0.0f);
    antennaLoc.setY(0.0f);
    antennaLoc.setZ(0.0f);
    pdu.setRelativeAntennaLocation(antennaLoc);

    // ── System ID ─────────────────────────────────────────────────────────────
    // SystemType: 1 = Mark XII (most common military IFF)
    // SystemMode: 1 = Transponder
    DIS::SystemIdentifier sysID;
    sysID.setSystemType(1);    // Mark XII
    sysID.setSystemName(0);    // Not specified
    sysID.setSystemMode(snap.systemOn ? 1 : 0); // 1=Transponder on
    sysID.setChangeOptions(0);
    pdu.setSystemID(sysID);

    pdu.setSystemDesignator(0);
    pdu.setSystemSpecificData(0);

    // ── Fundamental Operational Data ──────────────────────────────────────────
    // System status byte bit layout (SISO-STD-002):
    //   Bit 7: System On (1) / Off (0)
    //   Bit 6: Param 1 Capable
    //   Bit 5: Param 2 Capable
    //   Bit 4: Mode 1 Capable
    //   Bit 3: Mode 2 Capable
    //   Bit 2: Mode 3/A Capable
    //   Bit 1: Mode C Capable
    //   Bit 0: Mode 4 Capable
    DIS::FundamentalOperationalData fod;

    uint8_t status = 0;
    if (snap.systemOn) {
        status |= 0x80;  // System on
        status |= 0x1C;  // Mode 1, 2, 3A capable
        if (snap.mode4Active) status |= 0x01;  // Mode 4 capable
    }
    fod.setSystemStatus(status);
    fod.setDataField1(0);

    // Mode code layout: bits 15-4 = 12-bit code, bits 3-0 = status flags
    // Status flags: bit 2=online, bit 1=parameter capable, bit 0=mode on
    // informationLayers byte — bit 0 = Layer 1 present
    fod.setInformationLayers(0x01);
    fod.setDataField1(0);
    fod.setDataField2(0);

    // parameter1-6 map to Mode 1/2/3A/4/C/S codes
    fod.setParameter1(static_cast<unsigned short>(snap.mode1Code));
    fod.setParameter2(static_cast<unsigned short>(snap.mode2Code));
    fod.setParameter3(static_cast<unsigned short>(snap.mode3ACode));
    fod.setParameter4(static_cast<unsigned short>(snap.mode4Active ? 1 : 0));
    fod.setParameter5(static_cast<unsigned short>(snap.modeCCode));
    fod.setParameter6(0);

    pdu.setFundamentalOperationalData(fod);

    // ── Layer header — Layer 1 (basic IFF) ───────────────────────────────────
    DIS::LayerHeader layerHdr;
    layerHdr.setLayerNumber(1);
    layerHdr.setLayerSpecificInformation(0);
    pdu.setLayerHeader(layerHdr);

    // ── Beam data — zero (not applicable for basic IFF) ──────────────────────
    DIS::BeamData beamData;
    beamData.setBeamAzimuthCenter(0.0f);
    beamData.setBeamAzimuthSweep(0.0f);
    beamData.setBeamElevationCenter(0.0f);
    beamData.setBeamElevationSweep(0.0f);
    beamData.setBeamSweepSync(0.0f);
    pdu.setBeamData(beamData);

    // ── Secondary operational data — no additional IFF parameters ────────────
    DIS::SecondaryOperationalData secData;
    secData.setOperationalData1(0);
    secData.setOperationalData2(0);
    secData.setNumberOfIFFFundamentalParameterRecords(0);
    pdu.setSecondaryOperationalData(secData);

    return marshalToQByteArray(pdu);
}
QByteArray PDUSerializer::serializeTransferOwnership(
    const DISTransferOwnershipData& data,
    const DISConfig&                config,
    const EntityIDMapper&           mapper)
{
    DISEntityID transferDISID = mapper.getDISID(data.transferEntityID);
    if (!transferDISID.isValid()) return QByteArray();

    DIS::TransferOwnershipPdu pdu;

    // Protocol version hardcoded to 7 (IEEE 1278.1-2012), not config.defaultVersion.
    // The DIS v6 predecessor of this PDU ("Transfer Control Request") had a
    // different structure entirely — renamed and substantially revised in the
    // 2012 standard. DIS::TransferOwnershipPdu (this dis7-library class) only
    // ever produces the 2012-format body, so the header must say so too.
    pdu.setProtocolVersion(7);
    pdu.setExerciseID     (config.exerciseID);
    pdu.setTimestamp      (generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(
        buildDISEntityID(config.siteID, config.applicationID, 0));
    pdu.setReceivingEntityID(
        buildDISEntityID(data.receivingSiteID, data.receivingAppID, 0));

    pdu.setRequestID(data.requestID);
    pdu.setRequiredReliabilityService(0);  // Acknowledged
    pdu.setTransferType(data.transferType);
    pdu.setTransferEntityID(
        buildDISEntityID(transferDISID.site,
                         transferDISID.application,
                         transferDISID.entity));
    pdu.setNumberOfRecordSets(0);

    return marshalToQByteArray(pdu);
}
QByteArray PDUSerializer::serializeEmission(const DISEmissionSnapshot& snap,
                                            const DISConfig&           config,
                                            const EntityIDMapper&      mapper)
{
    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.emittingEntityID, mapper, site, app, entityNum))
        return QByteArray();

    DIS::ElectromagneticEmissionsPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_ELECTRONIC_EMMISIONS);
    pdu.setProtocolFamily(6);  // Distributed Emissions Family
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setEmittingEntityID(buildDISEntityID(site, app, entityNum));

    DIS::EventIdentifier eventID;
    DIS::SimulationAddress simAddr;
    simAddr.setSite(site);
    simAddr.setApplication(app);
    eventID.setSimulationAddress(simAddr);
    eventID.setEventNumber(static_cast<unsigned short>(entityNum));
    pdu.setEventID(eventID);

    // 0 = Heartbeat (full state). We always send full state — no
    // delta-tracking exists yet, so "changed data only" isn't meaningful.
    pdu.setStateUpdateIndicator(0);

    DIS::ElectromagneticEmissionSystemData system;
    system.setEmissionsPadding2(0);

    DIS::EmitterSystem emitterSystem;
    emitterSystem.setEmitterName(0);  // 0 = Other. No SISO-REF-010 emitter
    // name table wired up yet — placeholder
    // until a real emitter database exists.
    emitterSystem.setFunction(snap.emitterFunction);
    emitterSystem.setEmitterIdNumber(snap.systemID);
    system.setEmitterSystem(emitterSystem);

    // Antenna location relative to entity origin. No per-sensor mounting
    // offset is tracked yet — defaults to the entity's own origin.
    DIS::Vector3Float location;
    location.setX(0.0f); location.setY(0.0f); location.setZ(0.0f);
    system.setLocation(location);

    std::vector<DIS::ElectromagneticEmissionBeamData> beamRecords;
    for (const DISEmissionBeam& b : snap.beams) {
        DIS::ElectromagneticEmissionBeamData beam;
        beam.setBeamIDNumber(b.beamID);
        beam.setBeamParameterIndex(0);

        DIS::EEFundamentalParameterData fpd;
        fpd.setFrequency(b.frequency);
        fpd.setFrequencyRange(b.freqRange);
        fpd.setEffectiveRadiatedPower(b.erpPower);
        fpd.setPulseRepetitionFrequency(b.prf);
        fpd.setPulseWidth(b.pulseWidth);
        beam.setFundamentalParameterData(fpd);

        // IEEE 1278.1 BeamData az/el fields are RADIANS on the wire;
        // DISEmissionBeam stores DEGREES — same unit boundary already
        // handled for orientation. Convert here.
        DIS::BeamData beamData;
        beamData.setBeamAzimuthCenter(static_cast<float>(b.beamAzCenter * CoordConverter::DEG_TO_RAD));
        beamData.setBeamAzimuthSweep(static_cast<float>(b.beamAzSweep * CoordConverter::DEG_TO_RAD));
        beamData.setBeamElevationCenter(static_cast<float>(b.beamElCenter * CoordConverter::DEG_TO_RAD));
        beamData.setBeamElevationSweep(static_cast<float>(b.beamElSweep * CoordConverter::DEG_TO_RAD));
        beamData.setBeamSweepSync(0.0f);
        beam.setBeamData(beamData);

        beam.setBeamFunction(b.beamFunction);
        beam.setHighDensityTrackJam(0);
        beam.setBeamStatus(snap.active ? 0 : 1);  // 0 = Active per SISO-REF-010

        DIS::JammingTechnique jamming;
        jamming.setKind(0);
        jamming.setCategory(0);
        jamming.setSubcategory(0);
        jamming.setSpecific(0);
        beam.setJammingTechnique(jamming);

        std::vector<DIS::TrackJamData> trackJam;
        for (const std::string& trackedID : b.trackedEntityIDs) {
            DISEntityID trackedDISID = mapper.getDISID(trackedID);
            if (!trackedDISID.isValid()) continue;
            DIS::TrackJamData tj;
            tj.setEntityID(buildDISEntityID(trackedDISID.site,
                                            trackedDISID.application,
                                            trackedDISID.entity));
            tj.setEmitterNumber(snap.systemID);
            tj.setBeamNumber(b.beamID);
            trackJam.push_back(tj);
        }
        beam.setTrackJamTargets(trackJam);

        beam.setBeamDataLength(0);
        int beamBytes = beam.getMarshalledSize();
        beam.setBeamDataLength(static_cast<unsigned char>((beamBytes + 3) / 4));

        beamRecords.push_back(beam);
    }
    system.setBeamDataRecords(beamRecords);

    system.setSystemDataLength(0);
    int systemBytes = system.getMarshalledSize();
    system.setSystemDataLength(static_cast<unsigned char>((systemBytes + 3) / 4));

    std::vector<DIS::ElectromagneticEmissionSystemData> systems;
    systems.push_back(system);
    pdu.setSystems(systems);

    return marshalToQByteArray(pdu);
}
QByteArray PDUSerializer::serializeDesignator(const DISDesignatorSnapshot& snap,
                                              const DISConfig&              config,
                                              const EntityIDMapper&         mapper)
{
    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.designatingEntityID, mapper, site, app, entityNum))
        return QByteArray();

    if (!CoordConverter::isValidGeocord(snap.latitude, snap.longitude, snap.altitude))
        return QByteArray();

    DIS::DesignatorPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_DESIGNATOR);
    pdu.setProtocolFamily(6);  // Distributed Emissions Family — same as Emission/IFF
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setDesignatingEntityID(buildDISEntityID(site, app, entityNum));
    pdu.setCodeName(snap.codeName);

    // Designated entity optional — skip if not in mapper (no target yet)
    if (!snap.designatedEntityID.empty()) {
        uint16_t dsite, dapp, dentity;
        if (buildEntityID(snap.designatedEntityID, mapper, dsite, dapp, dentity))
            pdu.setDesignatedEntityID(buildDISEntityID(dsite, dapp, dentity));
    }

    pdu.setDesignatorCode(snap.designatorCode);
    pdu.setDesignatorPower(snap.designatorPower);
    pdu.setDesignatorWavelength(snap.designatorWavelength);

    // Offset from designated entity — no per-spot tracking yet, spot
    // assumed centered on the designated entity until that exists.
    DIS::Vector3Float spotOffset;
    spotOffset.setX(0.0f); spotOffset.setY(0.0f); spotOffset.setZ(0.0f);
    pdu.setDesignatorSpotWrtDesignated(spotOffset);

    ECEFPosition ecef = CoordConverter::geocordToECEF(
        snap.latitude, snap.longitude, snap.altitude);
    DIS::Vector3Double spotLocation;
    spotLocation.setX(ecef.x); spotLocation.setY(ecef.y); spotLocation.setZ(ecef.z);
    pdu.setDesignatorSpotLocation(spotLocation);

    pdu.setDeadReckoningAlgorithm(static_cast<char>(snap.deadReckoningAlgorithm));
    pdu.setPadding1(0);
    pdu.setPadding2(0);

    DIS::Vector3Float accel;
    accel.setX(0.0f); accel.setY(0.0f); accel.setZ(0.0f);
    pdu.setEntityLinearAcceleration(accel);

    return marshalToQByteArray(pdu);
}
QByteArray PDUSerializer::serializeTransmitter(const DISTransmitterSnapshot& snap,
                                               const DISConfig&               config,
                                               const EntityIDMapper&          mapper)
{


    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.transmittingEntityID, mapper, site, app, entityNum))
        return QByteArray();

    if (!CoordConverter::isValidGeocord(snap.latitude, snap.longitude, snap.altitude))
        return QByteArray();

    DIS::TransmitterPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_TRANSMITTER);
    pdu.setProtocolFamily(4);  // Radio Communications Family — NOT the same
    // family as Emission/Designator (family 6)
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setEntityID(buildDISEntityID(site, app, entityNum));
    pdu.setRadioID(snap.radioID);

    DIS::RadioType radioType;
    radioType.setEntityKind(snap.radioKind);
    radioType.setDomain(snap.radioDomain);
    radioType.setCountry(snap.radioCountry);
    radioType.setCategory(snap.radioCategory);
    radioType.setSubcategory(snap.radioSubcategory);
    radioType.setSpecific(snap.radioSpecific);
    radioType.setExtra(snap.radioExtra);
    pdu.setRadioType(radioType);

    pdu.setTransmitState(snap.active ? snap.transmitState : 0);
    pdu.setInputSource(snap.inputSource);

    ECEFPosition ecef = CoordConverter::geocordToECEF(
        snap.latitude, snap.longitude, snap.altitude);
    DIS::Vector3Double antennaLoc;
    antennaLoc.setX(ecef.x); antennaLoc.setY(ecef.y); antennaLoc.setZ(ecef.z);
    pdu.setAntennaLocation(antennaLoc);

    // No per-radio mounting offset tracked yet — defaults to entity origin.
    DIS::Vector3Float relLoc;
    relLoc.setX(0.0f); relLoc.setY(0.0f); relLoc.setZ(0.0f);
    pdu.setRelativeAntennaLocation(relLoc);

    pdu.setAntennaPatternType(0);  // 0 = Omnidirectional
    // antennaPatternCount stays 0 — no directional pattern records
    // implemented yet, see TransmitterPdu.h class note.

    pdu.setFrequency(snap.frequency);
    pdu.setTransmitFrequencyBandwidth(snap.transmitFrequencyBandwidth);
    pdu.setPower(snap.power);

    DIS::ModulationType modType;
    modType.setSpreadSpectrum(snap.modSpreadSpectrum);
    modType.setMajorModulation(snap.modMajorModulation);
    modType.setDetail(snap.modDetail);
    modType.setRadioSystem(snap.modRadioSystem);
    pdu.setModulationType(modType);

    pdu.setCryptoSystem(snap.cryptoSystem);
    pdu.setCryptoKeyId(snap.cryptoKeyId);
    // modulationParameterCount stays 0 — no variable modulation-parameter
    // records implemented yet.

    return marshalToQByteArray(pdu);
}

QByteArray PDUSerializer::serializeSignal(const DISSignalSnapshot& snap,
                                          const DISConfig&          config,
                                          const EntityIDMapper&     mapper)
{
    uint16_t site, app, entityNum;
    if (!buildEntityID(snap.transmittingEntityID, mapper, site, app, entityNum))
        return QByteArray();

    DIS::SignalPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_SIGNAL);
    pdu.setProtocolFamily(4);  // Radio Communications Family — must match
    // the Transmitter PDU this Signal references
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setEntityID(buildDISEntityID(site, app, entityNum));
    pdu.setRadioID(snap.radioID);
    pdu.setEncodingScheme(snap.encodingScheme);
    pdu.setTdlType(snap.tdlType);
    pdu.setSampleRate(snap.sampleRate);
    pdu.setSamples(snap.samples);
    pdu.setData(snap.data);   // dataLength is derived automatically inside SignalPdu

    return marshalToQByteArray(pdu);
}

static void buildDatumRecords(const DISDatumSnapshot& snap,
                              std::vector<DIS::FixedDatum>&    outFixed,
                              std::vector<DIS::VariableDatum>& outVariable)
{
    for (const DISFixedDatumEntry& f : snap.fixedDatums) {
        DIS::FixedDatum fd;
        fd.setFixedDatumID(f.datumID);
        fd.setFixedDatumValue(f.datumValue);
        outFixed.push_back(fd);
    }
    for (const DISVariableDatumEntry& v : snap.variableDatums) {
        DIS::VariableDatum vd;
        vd.setVariableDatumID(v.datumID);
        std::vector<uint8_t> bytes(v.data.begin(), v.data.end());
        vd.setPayloadBytes(bytes);   // uses the corrected VariableDatum API
        outVariable.push_back(vd);
    }
}
QByteArray PDUSerializer::serializeDataQuery(const DISDatumSnapshot& snap,
                                             const DISConfig&        config,
                                             const EntityIDMapper&   mapper)
{
    uint16_t osite, oapp, oentity;
    if (!buildEntityID(snap.originatingEntityID, mapper, osite, oapp, oentity))
        return QByteArray();

    DIS::DataQueryPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_DATA_QUERY);
    pdu.setProtocolFamily(5);  // Simulation Management Family
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(buildDISEntityID(osite, oapp, oentity));

    if (!snap.receivingEntityID.empty()) {
        uint16_t rsite, rapp, rentity;
        if (buildEntityID(snap.receivingEntityID, mapper, rsite, rapp, rentity))
            pdu.setReceivingEntityID(buildDISEntityID(rsite, rapp, rentity));
    }

    pdu.setRequestID(snap.requestID);
    pdu.setTimeInterval(snap.timeInterval);

    std::vector<DIS::FixedDatum> fixed;
    std::vector<DIS::VariableDatum> variable;
    buildDatumRecords(snap, fixed, variable);
    pdu.setFixedDatums(fixed);
    pdu.setVariableDatums(variable);

    return marshalToQByteArray(pdu);
}

QByteArray PDUSerializer::serializeSetData(const DISDatumSnapshot& snap,
                                           const DISConfig&        config,
                                           const EntityIDMapper&   mapper)
{
    uint16_t osite, oapp, oentity;
    if (!buildEntityID(snap.originatingEntityID, mapper, osite, oapp, oentity))
        return QByteArray();

    DIS::SetDataPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_SET_DATA);
    pdu.setProtocolFamily(5);
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(buildDISEntityID(osite, oapp, oentity));

    if (!snap.receivingEntityID.empty()) {
        uint16_t rsite, rapp, rentity;
        if (buildEntityID(snap.receivingEntityID, mapper, rsite, rapp, rentity))
            pdu.setReceivingEntityID(buildDISEntityID(rsite, rapp, rentity));
    }

    pdu.setRequestID(snap.requestID);
    pdu.setPadding1(0);

    std::vector<DIS::FixedDatum> fixed;
    std::vector<DIS::VariableDatum> variable;
    buildDatumRecords(snap, fixed, variable);
    pdu.setFixedDatums(fixed);
    pdu.setVariableDatums(variable);

    return marshalToQByteArray(pdu);
}

QByteArray PDUSerializer::serializeData(const DISDatumSnapshot& snap,
                                        const DISConfig&        config,
                                        const EntityIDMapper&   mapper)
{
    uint16_t osite, oapp, oentity;
    if (!buildEntityID(snap.originatingEntityID, mapper, osite, oapp, oentity))
        return QByteArray();

    DIS::DataPdu pdu;
    pdu.setProtocolVersion(config.defaultVersion);
    pdu.setExerciseID(config.exerciseID);
    pdu.setPduType(DIS::PDU_DATA);
    pdu.setProtocolFamily(5);
    pdu.setTimestamp(generateTimestamp(config.useAbsTimestamp));

    pdu.setOriginatingEntityID(buildDISEntityID(osite, oapp, oentity));

    if (!snap.receivingEntityID.empty()) {
        uint16_t rsite, rapp, rentity;
        if (buildEntityID(snap.receivingEntityID, mapper, rsite, rapp, rentity))
            pdu.setReceivingEntityID(buildDISEntityID(rsite, rapp, rentity));
    }

    pdu.setRequestID(snap.requestID);
    pdu.setPadding1(0);

    std::vector<DIS::FixedDatum> fixed;
    std::vector<DIS::VariableDatum> variable;
    buildDatumRecords(snap, fixed, variable);
    pdu.setFixedDatums(fixed);
    pdu.setVariableDatums(variable);

    return marshalToQByteArray(pdu);
}


