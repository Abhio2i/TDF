// =============================================================================
// FILE:        acknowledgehandler.h
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Handles incoming Acknowledge PDU (IEEE 1278.1 type 15).
//              Called by PDUDispatcher when another machine acknowledges
//              our CreateEntity, RemoveEntity, StartResume, or StopFreeze PDU.
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   onAcknowledgeReceived() called via DirectConnection from PDUDispatcher.
//   incomingAcknowledge emitted on network thread → DISNetworkPlugin
//   via QueuedConnection.
// =============================================================================

#ifndef ACKNOWLEDGEHANDLER_H
#define ACKNOWLEDGEHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

#include "core/DISPlugin/interface/disincomingdata.h"
#include "core/DISPlugin/utils/entityidmapper.h"

class AcknowledgeHandler : public QObject
{
    Q_OBJECT

public:
    explicit AcknowledgeHandler(QObject* parent = nullptr);

    void setMapper(EntityIDMapper* mapper);

public slots:
    // Connected from PDUDispatcher::acknowledgeReceived
    void onAcknowledgeReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    // Forwarded to DISNetworkPlugin via QueuedConnection
    void incomingAcknowledge(DISIncomingAcknowledge ack);

private:
    EntityIDMapper* m_mapper = nullptr;
};

#endif // ACKNOWLEDGEHANDLER_H
