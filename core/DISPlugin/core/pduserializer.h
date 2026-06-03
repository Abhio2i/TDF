
// =============================================================================
// FILE:        PDUSerializer.h
// MODULE:      DIS Network Plugin — Core
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Builds raw DIS PDU bytes using your dis7 library classes.
//              This is the FIRST file that directly uses dis7.
//
// WHAT IT DOES:
//   Takes your engine snapshots (DISEntitySnapshot etc)
//   Fills dis7 PDU objects (EntityStatePdu, FirePdu etc)
//   Calls pdu.marshal(dataStream) to get raw bytes
//   Returns QByteArray ready to send via DISTransport
//
// USES FROM dis7:
//   DIS::EntityStatePdu    — entity position/state
//   DIS::FirePdu           — weapon fired
//   DIS::DetonationPdu     — weapon detonated
//   DIS::StartResumePdu    — simulation start
//   DIS::StopFreezePdu     — simulation stop/pause
//   DIS::RemoveEntityPdu   — entity removed
//   DIS::CreateEntityPdu   — entity created
//   DIS::DataStream        — serialization buffer
//   DIS::Endian            — byte order (BIG_ENDIAN for network)
//
// THREAD SAFETY:
//   All methods are stateless and static.
//   Safe to call from any thread.
// =============================================================================
#ifndef PDUSERIALIZER_H
#define PDUSERIALIZER_H

#include <QByteArray>

#include "../interface/DISEntitySnapshot.h"
#include "../interface/DISFireSnapshot.h"
#include "../interface/DISDetonationSnapshot.h"
#include "core/DISPlugin/interface/DISEmissionSnapshot.h"
#include "core/DISPlugin/interface/DISExerciseControl.h"
#include "core/DISPlugin/interface/DISConfig.h"
#include "core/DISPlugin/utils/entityidmapper.h"
#include "core/DISPlugin/utils/coordconverter.h"
#include "core/DISPlugin/interface/DISIFFSnapshot.h"



class PDUSerializer {
public:

    // =========================================================================
    // serializeEntityState
    // Builds EntityStatePdu bytes from your engine entity snapshot
    //
    // Called by PDUSender at 5Hz for each locally owned entity
    // Also called immediately when shouldSendUpdate() returns true
    //
    // Returns empty QByteArray on failure
    // =========================================================================
    static QByteArray serializeEntityState(const DISEntitySnapshot& snap,
                                           const DISConfig&         config,
                                           const EntityIDMapper&    mapper);

    // =========================================================================
    // serializeFire
    // Builds FirePdu bytes from weapon fire event
    // =========================================================================
    static QByteArray serializeFire(const DISFireSnapshot& snap,
                                    const DISConfig&       config,
                                    const EntityIDMapper&  mapper);

    // =========================================================================
    // serializeDetonation
    // Builds DetonationPdu bytes from detonation event
    // =========================================================================
    static QByteArray serializeDetonation(const DISDetonationSnapshot& snap,
                                          const DISConfig&             config,
                                          const EntityIDMapper&        mapper);

    // =========================================================================
    // serializeStartResume
    // Builds StartResumePdu bytes
    // =========================================================================
    static QByteArray serializeStartResume(const DISStartResumeData& data,
                                           const DISConfig&          config);

    // =========================================================================
    // serializeStopFreeze
    // Builds StopFreezePdu bytes
    // =========================================================================
    static QByteArray serializeStopFreeze(const DISStopFreezeData& data,
                                          const DISConfig&         config);

    // =========================================================================
    // serializeRemoveEntity
    // Builds RemoveEntityPdu bytes
    // =========================================================================
    static QByteArray serializeRemoveEntity(const DISRemoveEntityData& data,
                                            const DISConfig&           config,
                                            const EntityIDMapper&      mapper);

    // =========================================================================
    // serializeCreateEntity
    // Builds CreateEntityPdu bytes
    // =========================================================================
    static QByteArray serializeCreateEntity(const DISCreateEntityData& data,
                                            const DISConfig&           config,
                                            const EntityIDMapper&      mapper);
    // ── IFF PDU ───────────────────────────────────────────────────────────────────
    static QByteArray serializeIFF(const DISIFFSnapshot&  snap,
                                   const DISConfig&        config,
                                   const EntityIDMapper&   mapper);
private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    // Fill common PDU header fields (protocol version, exercise ID, timestamp)
    // Called by all serialize methods
    template<typename PDUType>
    static void fillHeader(PDUType& pdu, const DISConfig& config, uint8_t pduType);

    // Build EntityID from mapper lookup
    // Returns false if entity not found in mapper
    static bool buildEntityID(const std::string& engineID,
                              const EntityIDMapper& mapper,
                              uint16_t& site,
                              uint16_t& app,
                              uint16_t& entity);

    // Generate DIS timestamp from current time
    // DIS uses absolute timestamp (seconds since midnight) or relative
    //static uint32_t generateTimestamp();
    static uint32_t generateTimestamp(bool useAbsolute = false);
};

#endif // PDUSERIALIZER_H
