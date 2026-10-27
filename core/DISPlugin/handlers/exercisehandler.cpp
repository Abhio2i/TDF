// =============================================================================
// FILE:        ExerciseHandler.cpp
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
// =============================================================================

#include "exercisehandler.h"

#include <dis7/StartResumePdu.h>
#include <dis7/StopFreezePdu.h>
#include <dis7/RemoveEntityPdu.h>
#include <dis7/CreateEntityPdu.h>
#include <dis7/utils/DataStream.h>
#include <dis7/utils/Endian.h>

#include <QDebug>

ExerciseHandler::ExerciseHandler(QObject* parent) : QObject(parent) {}

// =============================================================================
// onStartResumeReceived
// DRDO STAGE sends this when exercise starts
// =============================================================================
void ExerciseHandler::onStartResumeReceived(QByteArray   pduBytes,
                                            QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::StartResumePdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    qDebug() << "[ExerciseHandler] Start/Resume PDU received"
             << "requestID=" << pdu.getRequestID();

    DISIncomingExercise event;
    event.type = DISIncomingExercise::StartResume;
    emit incomingExercise(event);
    // ── Send Acknowledge per IEEE 1278.1 ──────────────────────────────────────
    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    DISAcknowledgeData ack;
    ack.originSite      = simAddr.getSite();
    ack.originApp       = simAddr.getApplication();
    ack.originEntity    = originID.getEntityNumber();
    ack.acknowledgeFlag = DISAcknowledgeFlag::StartResume;
    ack.responseFlag    = DISResponseFlag::AbleToComply;
    ack.requestID       = pdu.getRequestID();
    emit acknowledgeNeeded(ack);
}

// =============================================================================
// onStopFreezeReceived
// DRDO STAGE sends this when exercise pauses or stops
// =============================================================================
void ExerciseHandler::onStopFreezeReceived(QByteArray   pduBytes,
                                           QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::StopFreezePdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    qDebug() << "[ExerciseHandler] Stop/Freeze PDU received"
             << "reason=" << pdu.getReason();

    DISIncomingExercise event;
    event.type       = DISIncomingExercise::StopFreeze;
    event.stopReason = pdu.getReason();
    emit incomingExercise(event);
    // ── Send Acknowledge per IEEE 1278.1 ──────────────────────────────────────
    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    DISAcknowledgeData ack;
    ack.originSite      = simAddr.getSite();
    ack.originApp       = simAddr.getApplication();
    ack.originEntity    = originID.getEntityNumber();
    ack.acknowledgeFlag = DISAcknowledgeFlag::StopFreeze;
    ack.responseFlag    = DISResponseFlag::AbleToComply;
    ack.requestID       = pdu.getRequestID();
    emit acknowledgeNeeded(ack);
}

// =============================================================================
// onRemoveEntityReceived
// Another DIS node removed an entity from the exercise
// =============================================================================
void ExerciseHandler::onRemoveEntityReceived(QByteArray   pduBytes,
                                             QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::RemoveEntityPdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    qDebug() << "[ExerciseHandler] Remove Entity PDU received"
             << "requestID=" << pdu.getRequestID();

    // ── Send Acknowledge per IEEE 1278.1 ──────────────────────────────────────
    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    DISAcknowledgeData ack;
    ack.originSite      = simAddr.getSite();
    ack.originApp       = simAddr.getApplication();
    ack.originEntity    = originID.getEntityNumber();
    ack.acknowledgeFlag = DISAcknowledgeFlag::RemoveEntity;
    ack.responseFlag    = DISResponseFlag::AbleToComply;
    ack.requestID       = pdu.getRequestID();
    emit acknowledgeNeeded(ack);

    // Entity removal tracked by EntityState heartbeat timeout
    // 12-second timeout in DISNetworkPlugin removes ghost entity
}
// void ExerciseHandler::onRemoveEntityReceived(QByteArray   pduBytes,
//                                              QHostAddress sender)
// {
//     Q_UNUSED(sender)

//     DIS::RemoveEntityPdu pdu;
//     DIS::DataStream ds(pduBytes.constData(),
//                        static_cast<size_t>(pduBytes.size()),
//                        DIS::BIG);
//     pdu.unmarshal(ds);

//     qDebug() << "[ExerciseHandler] Remove Entity PDU received"
//              << "requestID=" << pdu.getRequestID();

//     // RemoveEntity is handled by the absence of EntityState heartbeats
//     // We log here for now — entity timeout handled by EntityStateHandler
// }

// =============================================================================
// onCreateEntityReceived
// Another DIS node added a new entity to the exercise
// =============================================================================
void ExerciseHandler::onCreateEntityReceived(QByteArray   pduBytes,
                                             QHostAddress sender)
{
    Q_UNUSED(sender)

    DIS::CreateEntityPdu pdu;
    DIS::DataStream ds(pduBytes.constData(),
                       static_cast<size_t>(pduBytes.size()),
                       DIS::BIG);
    pdu.unmarshal(ds);

    qDebug() << "[ExerciseHandler] Create Entity PDU received"
             << "requestID=" << pdu.getRequestID();

    // ── Send Acknowledge per IEEE 1278.1 ──────────────────────────────────────
    // IEEE requires Acknowledge PDU in response to CreateEntity.
    // Extract originator from PDU and emit — PDUSender handles serialization.
    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();

    DISAcknowledgeData ack;
    ack.originSite      = simAddr.getSite();
    ack.originApp       = simAddr.getApplication();
    ack.originEntity    = originID.getEntityNumber();
    ack.acknowledgeFlag = DISAcknowledgeFlag::CreateEntity;
    ack.responseFlag    = DISResponseFlag::AbleToComply;
    ack.requestID       = pdu.getRequestID();
    emit acknowledgeNeeded(ack);

    // New entity will announce itself via EntityState PDU shortly
    // EntityStateHandler registers it on first EntityState arrival
}
// void ExerciseHandler::onCreateEntityReceived(QByteArray   pduBytes,
//                                              QHostAddress sender)
// {
//     Q_UNUSED(sender)

//     DIS::CreateEntityPdu pdu;
//     DIS::DataStream ds(pduBytes.constData(),
//                        static_cast<size_t>(pduBytes.size()),
//                        DIS::BIG);
//     pdu.unmarshal(ds);

//     qDebug() << "[ExerciseHandler] Create Entity PDU received"
//              << "requestID=" << pdu.getRequestID();

//     // New entity will announce itself via EntityState PDU shortly
//     // EntityStateHandler will register it when first EntityState arrives
// }
