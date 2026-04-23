// =============================================================================
// FILE:        networkobject.h
// MODULE:      Network Synchronisation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the NetworkObject class, which enables networked
//              replication of entity state. Handles ownership (server/client),
//              spawning, updates, and synchronisation of Transform and
//              parameters across the simulation network.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added ownership flags and parameter map.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

#include <QObject>
#include "./transform.h"
#include <core/Hierarchy/Struct/parameter.h>

// =============================================================================
// CLASS: NetworkObject
//
// DESCRIPTION: Component that enables an entity to participate in network
//              replication. Manages ownership (server, client, owner),
//              spawn/update flags, and synchronises Transform and custom
//              parameters over the network.
// =============================================================================
class NetworkObject: public QObject, public Component
{
    Q_OBJECT
public:
    NetworkObject();
    ComponentType Typo() const override { return ComponentType::NetworkObject; }

    // =========================================================================
    // SECTION: Network State Flags
    // DESCRIPTION: Role and ownership indicators for this networked object.
    // =========================================================================
    bool Active;                    //!< Whether network replication is active
    bool isOwner;                   //!< Does this instance own the object?
    bool isOwnByServer;             //!< Is the object owned by the server?
    bool isServer;                  //!< Is this instance a server?
    bool isClient;                  //!< Is this instance a client?
    bool isSpawn;                   //!< Flag indicating a spawn operation
    bool isUpdate;                  //!< Flag indicating an update operation
    std::string lastUpdateStamp;    //!< Timestamp of last network update

    // =========================================================================
    // SECTION: Networked Data
    // DESCRIPTION: References to synchronised components and parameters.
    // =========================================================================
    Transform *transform;                                   //!< Transform to replicate
    std::unordered_map<std::string, Parameter> *parameters; //!< Custom parameters map

    void recieveUpdate();           //!< Handle incoming network update

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

signals:
    void netWorkSignal();           //!< Emitted when network activity occurs
};

#endif // NETWORKOBJECT_H
