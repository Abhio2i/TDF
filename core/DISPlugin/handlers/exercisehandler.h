// =============================================================================
// FILE:        ExerciseHandler.h
// MODULE:      DIS Network Plugin — Handlers
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Handles simulation management PDUs:
//              StartResume  → simulation should start
//              StopFreeze   → simulation should pause/stop
//              RemoveEntity → entity left the exercise
//              CreateEntity → new entity joined the exercise
// =============================================================================

#ifndef EXERCISEHANDLER_H
#define EXERCISEHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

#include "core/DISPlugin/DISNetworkPlugin.h"  // for DISIncomingExercise

class ExerciseHandler : public QObject {
    Q_OBJECT
public:
    explicit ExerciseHandler(QObject* parent = nullptr);

public slots:
    void onStartResumeReceived (QByteArray pduBytes, QHostAddress sender);
    void onStopFreezeReceived  (QByteArray pduBytes, QHostAddress sender);
    void onRemoveEntityReceived(QByteArray pduBytes, QHostAddress sender);
    void onCreateEntityReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    void incomingExercise(DISIncomingExercise event);
};

#endif // EXERCISEHANDLER_H
