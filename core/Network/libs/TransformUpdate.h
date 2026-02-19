//Author::Aman Negi
#ifndef TRANSFORMUPDATE_H
#define TRANSFORMUPDATE_H
#pragma once
#include <QVector3D>
#include <core/Hierarchy/entity.h>
#include <QHostAddress>
struct TransformUpdate {
    std::string id;
    QVector3D pos;
    QVector3D rot;
};
struct OwnershipUpdate
{
    QString entityId;
    QString ownerNodeId;
    Entity* entity = nullptr;
};
struct ClientInfo {
    QString nodeId;
    QHostAddress address;
    quint16 port;
    QDateTime connectedAt;
};


// 👇 MUST be in the same header, after the struct
#include <QMetaType>
Q_DECLARE_METATYPE(TransformUpdate)
Q_DECLARE_METATYPE(OwnershipUpdate)
Q_DECLARE_METATYPE(ClientInfo)
#endif // TRANSFORMUPDATE_H
//The macro must be visible to both Runtime & Simulation builds — which it is.
// NetworkTransport Thread
//     |
//     v   (Qt queued signal)
//     NetworkManager::binaryFrameReceived(TransformUpdate)
//     |
//     v
//     SimulationThread::enqueueTransform()
//     |
//     v
//     SimulationThread applies updates during frame()
