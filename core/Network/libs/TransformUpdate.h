#ifndef TRANSFORMUPDATE_H
#define TRANSFORMUPDATE_H
#pragma once
#include <QVector3D>

struct TransformUpdate {
    std::string id;
    QVector3D pos;
    QVector3D rot;
};

// 👇 MUST be in the same header, after the struct
#include <QMetaType>
    Q_DECLARE_METATYPE(TransformUpdate)
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
