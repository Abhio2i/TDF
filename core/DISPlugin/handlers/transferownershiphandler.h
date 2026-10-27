// =============================================================================
// FILE:        transferownershiphandler.h
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Handles incoming Transfer Ownership PDU (IEEE 1278.1 type 35).
//              When received, automatically sends Acknowledge (AbleToComply)
//              then emits incomingTransferOwnership for the plugin to process.
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   onTransferOwnershipReceived() called via DirectConnection from dispatcher.
//   acknowledgeNeeded wired directly to PDUSender (DirectConnection).
//   incomingTransferOwnership emitted via QueuedConnection to plugin.
// =============================================================================

#ifndef TRANSFEROWNERSHIPHANDLER_H
#define TRANSFEROWNERSHIPHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

#include "core/DISPlugin/interface/disincomingdata.h"
#include "core/DISPlugin/interface/DISExerciseControl.h"
#include "core/DISPlugin/utils/entityidmapper.h"

class TransferOwnershipHandler : public QObject
{
    Q_OBJECT

public:
    explicit TransferOwnershipHandler(QObject* parent = nullptr);

    void setMapper(EntityIDMapper* mapper);

public slots:
    void onTransferOwnershipReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    // Wired directly to PDUSender on same thread — fires immediately
    void acknowledgeNeeded(DISAcknowledgeData ack);

    // Crosses thread boundary to DISNetworkPlugin — QueuedConnection
    void incomingTransferOwnership(DISIncomingTransferOwnership data);

private:
    EntityIDMapper* m_mapper = nullptr;
};

#endif // TRANSFEROWNERSHIPHANDLER_H
