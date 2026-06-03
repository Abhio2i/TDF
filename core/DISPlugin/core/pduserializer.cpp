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

    DISOrientation euler = CoordConverter::headingToEuler(
        snap.heading, snap.pitch, snap.roll);

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
