#ifndef AUTOSCRIPTCONTROLLER_H
#define AUTOSCRIPTCONTROLLER_H

#include "autoscriptconfig.h"
#include "autoscriptmanager.h"
#include <QObject>
#include <QPointer>
#include <QString>

class ScriptEngine;
class TcpAutoScriptSource;
class QAction;

// Owns optional config load, TCP source, manager, and toolbar toggle/status.
// RuntimeEditor only constructs this and connects Simulation::sendMode (optional).
class AutoScriptController : public QObject
{
    Q_OBJECT
public:
    explicit AutoScriptController(ScriptEngine* engine, QObject* parent = nullptr);
    ~AutoScriptController() override;

    void attachToggleAction(QAction* action);
    void setSimulationState(AutoScriptManager::SimState state);

    bool isEnabled() const;
    QString statusText() const;

    void resetQueue(const QString& reason);

public slots:
    void setEnabled(bool on);  // start/stop listen only

signals:
    void statusChanged(const QString& text);

private:
    bool applyConfig(const AutoScriptConfig& cfg);
    void refreshStatus();

    QString                m_configPath;
    AutoScriptConfig       m_cfg;
    QPointer<ScriptEngine> m_engine;
    AutoScriptManager*     m_manager = nullptr;
    TcpAutoScriptSource*   m_tcp = nullptr;
    QPointer<QAction>      m_toggleAction;
    QString                m_lastResult;
};

#endif // AUTOSCRIPTCONTROLLER_H
