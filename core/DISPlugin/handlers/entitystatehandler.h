// =============================================================================
// FILE:        EntityStateHandler.h
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Receives raw EntityState PDU bytes from PDUDispatcher.
//              Unmarshals using dis7::EntityStatePdu.
//              Applies dead reckoning for smooth position prediction.
//              Pushes DISIncomingTransform to DISNetworkPlugin queue.
//              DISNetworkPlugin drains the queue on main thread.
//
// THREAD MODEL:
//   Lives on DIS network thread.
//   onEntityStateReceived() called from PDUDispatcher on network thread.
//   Emits incomingTransform signal → DISNetworkPlugin queues it.
// =============================================================================

#ifndef ENTITYSTATEHANDLER_H
#define ENTITYSTATEHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include <QMap>
#include <QString>

#include "../utils/entityidmapper.h"
#include "../utils/deadreckoning.h"
#include "../utils/coordconverter.h"
#include "../interface/disincomingdata.h" // for DISIncomingTransform

class EntityStateHandler : public QObject {
    Q_OBJECT

public:
    explicit EntityStateHandler(QObject* parent = nullptr);
    ~EntityStateHandler() override;

    void setMapper(EntityIDMapper* mapper);

public slots:
    // Connected to PDUDispatcher::entityStateReceived
    void onEntityStateReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    // Emitted for each processed EntityState PDU
    // DISNetworkPlugin::onIncomingTransform connected to this
    void incomingTransform(DISIncomingTransform update);

    // Emitted when a new entity is first seen from this peer
    void newEntityDiscovered(QString entityID, QHostAddress sender);

private:
    EntityIDMapper* m_mapper = nullptr;

    // Dead reckoning state per entity
    QMap<QString, DeadReckoningState> m_drStates;

    // Helper to build entity key from site/app/entity
    static QString entityKey(uint16_t site, uint16_t app, uint16_t entity);
};

#endif // ENTITYSTATEHANDLER_H
