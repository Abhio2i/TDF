// =============================================================================
// FILE:        WarfareHandlers.h
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Handlers for Fire and Detonation PDUs.
//              Fire PDU    → emits incomingFire signal
//              Detonation  → emits incomingDetonation signal
//              Both feed into DISNetworkPlugin queues
// =============================================================================

#ifndef WARFAREHANDLERS_H
#define WARFAREHANDLERS_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

#include "../utils/entityidmapper.h"
#include "../utils/coordconverter.h"
#include "core/DISPlugin/DISNetworkPlugin.h"  // for DISIncomingFire, DISIncomingDetonation

// =============================================================================
// FireHandler
// =============================================================================
class FireHandler : public QObject {
    Q_OBJECT
public:
    explicit FireHandler(QObject* parent = nullptr);
    void setMapper(EntityIDMapper* mapper);

public slots:
    void onFireReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    void incomingFire(DISIncomingFire event);

private:
    EntityIDMapper* m_mapper = nullptr;
};

// =============================================================================
// DetonationHandler
// =============================================================================
class DetonationHandler : public QObject {
    Q_OBJECT
public:
    explicit DetonationHandler(QObject* parent = nullptr);
    void setMapper(EntityIDMapper* mapper);

public slots:
    void onDetonationReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    void incomingDetonation(DISIncomingDetonation event);

private:
    EntityIDMapper* m_mapper = nullptr;
};

#endif // WARFAREHANDLERS_H
