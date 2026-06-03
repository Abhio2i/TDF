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

    // RemoveEntity is handled by the absence of EntityState heartbeats
    // We log here for now — entity timeout handled by EntityStateHandler
}

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

    // New entity will announce itself via EntityState PDU shortly
    // EntityStateHandler will register it when first EntityState arrives
}
