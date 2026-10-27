// =============================================================================
// FILE:        DISExerciseControl.h
// MODULE:      DIS Network Plugin — Interface
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Plain data structs for DIS exercise control PDUs.
//              StartResumePdu (type 13) — sent when simulation starts
//              StopFreezePdu  (type 14) — sent when simulation stops/pauses
//              RemoveEntityPdu (type 31) — sent when entity is destroyed
//              CreateEntityPdu (type 32) — sent when entity joins mid-exercise
//              No Qt. No engine types. No DIS library types.
//
// USAGE:       Bridge sends StartResume when Simulation::start() called
//              Bridge sends StopFreeze when Simulation::stop/pause() called
//              Bridge sends RemoveEntity when entity->Active = false
//              Bridge sends CreateEntity when new entity added during exercise
// =============================================================================

#ifndef DISEXERCISECONTROL_H
#define DISEXERCISECONTROL_H

#include <string>
#include <cstdint>

// =============================================================================
// StopFreeze reason codes — IEEE 1278.1
// =============================================================================
namespace DISStopReason {
    constexpr uint8_t Other            = 0;
    constexpr uint8_t Recess           = 1;  // temporary pause
    constexpr uint8_t Termination      = 2;  // exercise over
    constexpr uint8_t SystemFailure    = 3;
    constexpr uint8_t SecurityViolation= 4;
    constexpr uint8_t EntityReconst    = 5;
}

// =============================================================================
// StopFreeze frozen behavior — IEEE 1278.1
// =============================================================================
namespace DISFrozenBehavior {
    constexpr uint8_t RunSimClock           = 0;
    constexpr uint8_t TransmitPDUs          = 1;
    constexpr uint8_t UpdateSimModels       = 2;
    constexpr uint8_t TransmitAndUpdate     = 3;
}

// =============================================================================
// DISStartResumeData
// Sent when exercise starts or resumes from pause
// Bridge sends this from Simulation::start() and Simulation::startf()
// =============================================================================
struct DISStartResumeData {
    // Originating and receiving entity IDs
    // Leave empty to broadcast to all participants
    std::string originatingEntityID;
    std::string receivingEntityID;

    // Real world time at start (seconds since epoch)
    // Set to current system time by PDUSerializer
    double realWorldStartTimeSec = 0.0;

    // Simulation time at start (seconds)
    float simulationStartTimeSec = 0.0f;

    // Request identifier (for acknowledge)
    uint32_t requestID = 0;
};

// =============================================================================
// DISStopFreezeData
// Sent when exercise stops or pauses
// Bridge sends this from Simulation::stop() and Simulation::pause()
// =============================================================================
struct DISStopFreezeData {
    std::string originatingEntityID;
    std::string receivingEntityID;

    // Real world time at stop
    double realWorldTimeSec = 0.0;

    // Why stopping
    uint8_t reason          = DISStopReason::Recess;
    uint8_t frozenBehavior  = DISFrozenBehavior::RunSimClock;

    // Request identifier
    uint32_t requestID = 0;
};

// =============================================================================
// DISRemoveEntityData
// Sent when an entity is destroyed or removed from simulation
// Bridge sends this when entity->Active becomes false
// or when Simulation::entityRemoved() is called
// =============================================================================
struct DISRemoveEntityData {
    std::string originatingEntityID;  // us (the sender)
    std::string removedEntityID;      // entity being removed

    // Request identifier (for acknowledge)
    uint32_t requestID = 0;
};

// =============================================================================
// DISCreateEntityData
// Sent when a new entity joins mid-exercise
// Bridge sends this from Simulation::entityAdded()
// when simulation is already running
// =============================================================================
struct DISCreateEntityData {
    std::string originatingEntityID;  // us (the sender)
    std::string newEntityID;          // entity being created

    // Request identifier
    uint32_t requestID = 0;
};
// =============================================================================
// Acknowledge flag codes — IEEE 1278.1 Table B.47
// Identifies which PDU type is being acknowledged
// =============================================================================
namespace DISAcknowledgeFlag {
constexpr uint16_t CreateEntity      = 1;
constexpr uint16_t RemoveEntity      = 2;
constexpr uint16_t StartResume       = 3;
constexpr uint16_t StopFreeze        = 4;
constexpr uint16_t TransferOwnership = 5;
}

// =============================================================================
// Acknowledge response flag — IEEE 1278.1 Table B.48
// =============================================================================
namespace DISResponseFlag {
constexpr uint16_t Other          = 0;
constexpr uint16_t AbleToComply   = 1;
constexpr uint16_t UnableToComply = 2;
}

// =============================================================================
// DISAcknowledgeData
// Sent automatically in response to CreateEntity, RemoveEntity,
// StartResume, StopFreeze, and (future) TransferOwnership PDUs.
// Uses raw DIS triplets — no engine ID mapping needed.
// ExerciseHandler fills this from the received PDU and emits acknowledgeNeeded.
// PDUSender serializes and sends it immediately.
// =============================================================================
struct DISAcknowledgeData {
    // Originator of the request we are acknowledging (extracted from received PDU)
    uint16_t originSite      = 0;
    uint16_t originApp       = 0;
    uint16_t originEntity    = 0;

    // Which PDU type we are acknowledging — DISAcknowledgeFlag::*
    uint16_t acknowledgeFlag = 0;

    // Response — always AbleToComply for now
    uint16_t responseFlag    = DISResponseFlag::AbleToComply;

    // Must match the requestID from the PDU being acknowledged
    uint32_t requestID       = 0;
};
// =============================================================================
// Transfer type codes — IEEE 1278.1 Table 70
// =============================================================================
namespace DISTransferType {
constexpr uint8_t Other                          = 1;
constexpr uint8_t ControllingAppRequestsTransfer = 2;  // owner initiates
constexpr uint8_t RequestingAppDesiresControl    = 3;  // receiver requests
constexpr uint8_t MutualConsent                  = 4;
constexpr uint8_t AutomaticTransfer              = 7;
}

// =============================================================================
// DISTransferOwnershipData
// Sent by the current owner to transfer an entity to another machine.
// The entity keeps its original DIS ID after transfer — all receivers
// see continuity with no ID change.
// requestID must be set before sending — used to match Acknowledge response.
// =============================================================================
struct DISTransferOwnershipData {
    std::string transferEntityID;  // engine ID for local lookup in mapper

    uint16_t receivingSiteID = 0;  // target machine's site ID
    uint16_t receivingAppID  = 0;  // target machine's app ID

    uint8_t  transferType = DISTransferType::ControllingAppRequestsTransfer;
    uint32_t requestID    = 0;     // set by DISNetworkPlugin before sending
};
#endif // DISEXERCISECONTROL_H
