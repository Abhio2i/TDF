#ifndef AUTOSCRIPTMANAGER_H
#define AUTOSCRIPTMANAGER_H

#include "autoscriptsource.h"
#include <QList>
#include <QObject>
#include <QPointer>
#include <QQueue>

class ScriptEngine;

// Queue + simulation gate + security checks between AutoScriptSource(s) and ScriptEngine.
// All of this lives on the GUI thread (QTcpServer signals arrive on the GUI thread).
class AutoScriptManager : public QObject
{
    Q_OBJECT
public:
    enum class Policy {
        Always,            // run regardless of simulation state
        OnlyWhenRunning,   // reject scripts that arrive while simulation is not running
        QueueUntilRunning  // accept anytime, run when simulation is RUNNING (default)
    };

    enum class SimState { Stopped, Running, Paused };

    explicit AutoScriptManager(ScriptEngine* engine, QObject* parent = nullptr);

    void   setPolicy(Policy policy);
    Policy policy() const { return m_policy; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setMaxQueueLength(int n)        { m_maxQueue = qMax(1, n); }
    void setMaxScriptSize(int bytes)     { m_maxScriptBytes = qMax(1024, bytes); }
    void setSandboxDir(const QString& d) { m_sandboxDir = d; }

    bool isBusy() const    { return m_busy; }
    int  queueSize() const { return m_queue.size(); }

    void registerSource(AutoScriptSource* source);   // no ownership
    void submit(const AutoScriptRequest& request);   // internal / test use

    // Abort running auto script + cancel everything queued
    // (call on scenario load / reset).
    void resetQueue(const QString& reason);

public slots:
    void onScriptReceived(const AutoScriptRequest& request);
    void setSimulationState(SimState state);

private slots:
    void onEngineScriptFinished(bool ok, const QString& status, const QString& message);

signals:
    void scriptQueued(const AutoScriptRequest& request);
    void scriptStarted(const AutoScriptRequest& request);
    void scriptFinished(const AutoScriptRequest& request, bool success);
    void scriptRejected(const AutoScriptRequest& request, const QString& reason);

private:
    bool canExecuteNow() const;
    void tryExecuteNext();
    void scheduleNext();
    void reject(const AutoScriptRequest& request, const QString& reason);
    void notify(const AutoScriptRequest& request, const QString& status, const QString& message);
    void audit(const QString& stage, const AutoScriptRequest& request,
               const QString& status, const QString& reason = QString());

    QPointer<ScriptEngine>  m_engine;
    Policy                  m_policy = Policy::QueueUntilRunning;
    SimState                m_simState = SimState::Stopped;
    bool                    m_enabled = true;

    bool                    m_busy = false;           // an AUTO script is running
    bool                    m_nextScheduled = false;
    AutoScriptRequest       m_current;

    int                     m_maxQueue = 32;
    int                     m_maxScriptBytes = 512 * 1024;
    QString                 m_sandboxDir;

    QQueue<AutoScriptRequest> m_queue;
    QList<AutoScriptSource*>  m_sources;
};

#endif // AUTOSCRIPTMANAGER_H
