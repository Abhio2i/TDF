#include "autoscriptmanager.h"
#include "core/ScriptEngine/scriptengine.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>

AutoScriptManager::AutoScriptManager(ScriptEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
    qRegisterMetaType<AutoScriptRequest>("AutoScriptRequest");

    m_sandboxDir = QCoreApplication::applicationDirPath() + "/autoscript_files";

    if (m_engine) {
        connect(m_engine, &ScriptEngine::scriptFinished,
                this, &AutoScriptManager::onEngineScriptFinished);
    }
}

void AutoScriptManager::setPolicy(Policy policy)
{
    m_policy = policy;
    scheduleNext();
}

void AutoScriptManager::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (m_enabled)
        scheduleNext();
    else
        resetQueue("Auto-script execution disabled");
}

void AutoScriptManager::registerSource(AutoScriptSource* source)
{
    if (!source || m_sources.contains(source))
        return;

    m_sources.append(source);

    connect(source, &AutoScriptSource::scriptReceived,
            this, &AutoScriptManager::onScriptReceived);

    connect(source, &AutoScriptSource::sourceError,
            this, [](const QString& msg) {
                qWarning().noquote() << "[AutoScript] source error:" << msg;
            });

    connect(source, &QObject::destroyed, this, [this](QObject* o) {
        m_sources.removeAll(static_cast<AutoScriptSource*>(o));
    });
}

void AutoScriptManager::submit(const AutoScriptRequest& request)
{
    onScriptReceived(request);
}

// ---------------------------------------------------------------- receive
void AutoScriptManager::onScriptReceived(const AutoScriptRequest& in)
{
    AutoScriptRequest request = in;

    if (request.id.isEmpty())
        request.id = QString("auto-%1").arg(QDateTime::currentMSecsSinceEpoch());

    const QByteArray bytes = request.content.toUtf8();
    request.meta.insert("sha256",
                        QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()));

    audit("RECEIVED", request, "OK");

    if (!m_enabled)
        return reject(request, "Auto-script execution is disabled");

    if (bytes.trimmed().isEmpty())
        return reject(request, "Empty script content");

    if (bytes.size() > m_maxScriptBytes)
        return reject(request, "Script exceeds size limit");

    if (m_policy == Policy::OnlyWhenRunning && m_simState != SimState::Running)
        return reject(request, "Simulation is not running");

    if (m_queue.size() >= m_maxQueue)
        return reject(request, "Queue is full");

    // 'replace' is a privilege: only authenticated senders may abort a running script.
    const bool replace = request.replaceRunning && request.trusted;
    QString note;
    if (request.replaceRunning && !request.trusted) {
        request.replaceRunning = false;
        note = "replace ignored (sender not authenticated), queued normally";
    }

    if (replace)
        m_queue.prepend(request);
    else
        m_queue.enqueue(request);

    audit("QUEUED", request, "QUEUED", note);
    notify(request, "QUEUED", note);
    emit scriptQueued(request);

    if (replace && m_engine && m_engine->isScriptRunning())
        m_engine->stopScript();     // emits ABORTED -> onEngineScriptFinished -> scheduleNext

    scheduleNext();
}

void AutoScriptManager::reject(const AutoScriptRequest& request, const QString& reason)
{
    audit("REJECTED", request, "REJECTED", reason);
    notify(request, "REJECTED", reason);
    emit scriptRejected(request, reason);
}

// ------------------------------------------------------- simulation state
void AutoScriptManager::setSimulationState(SimState state)
{
    m_simState = state;

    if (state == SimState::Stopped) {
        // Scenario ended/reset: queued scripts may reference entities that no longer exist.
        resetQueue("Simulation stopped");
        return;
    }
    // Paused: running script continues, nothing new starts. Running: release queue.
    if (state == SimState::Running)
        scheduleNext();
}

void AutoScriptManager::resetQueue(const QString& reason)
{
    // Cancel queued first, so the abort below cannot start the next one.
    const QList<AutoScriptRequest> pending(m_queue.begin(), m_queue.end());
    m_queue.clear();
    for (const AutoScriptRequest& r : pending) {
        audit("CANCELLED", r, "CANCELLED", reason);
        notify(r, "CANCELLED", reason);
    }

    if (m_busy && m_engine)
        m_engine->stopScript();     // -> ABORTED notification for the running one
}

// -------------------------------------------------------------- execution
bool AutoScriptManager::canExecuteNow() const
{
    if (!m_enabled || !m_engine)
        return false;
    if (m_busy)
        return false;
    if (m_engine->isScriptRunning())     // a manual script is running: never kill it
        return false;

    switch (m_policy) {
    case Policy::Always:
        return true;
    case Policy::OnlyWhenRunning:
    case Policy::QueueUntilRunning:
        return m_simState == SimState::Running;
    }
    return false;
}

void AutoScriptManager::scheduleNext()
{
    if (m_nextScheduled)
        return;
    m_nextScheduled = true;
    QMetaObject::invokeMethod(this, [this]() {
        m_nextScheduled = false;
        tryExecuteNext();
    }, Qt::QueuedConnection);
}

void AutoScriptManager::tryExecuteNext()
{
    if (m_queue.isEmpty() || !canExecuteNow())
        return;

    const AutoScriptRequest req = m_queue.dequeue();

    m_current = req;
    m_busy = true;

    // Auto scripts get a restricted file area (ReadFile/WriteFile/AppendFile).
    m_engine->setFileSandbox(true, m_sandboxDir);

    audit("STARTED", req, "RUNNING");
    notify(req, "RUNNING", QString());
    emit scriptStarted(req);

    // May finish synchronously: scriptFinished is then delivered before this returns.
    const bool started = m_engine->loadAndCompileScript(req.content);

    if (!started && m_busy)   // safety net, engine normally already emitted ERROR
        onEngineScriptFinished(false, "ERROR", "Script could not be started");
}

void AutoScriptManager::onEngineScriptFinished(bool ok, const QString& status, const QString& message)
{
    if (!m_busy) {
        // A manual script ended: auto queue may continue.
        scheduleNext();
        return;
    }

    const AutoScriptRequest req = m_current;
    m_busy = false;
    m_current = AutoScriptRequest();

    if (m_engine)
        m_engine->setFileSandbox(false);

    audit("FINISHED", req, status, message);
    notify(req, status, message);
    emit scriptFinished(req, ok);

    scheduleNext();
}

// ---------------------------------------------------------------- helpers
void AutoScriptManager::notify(const AutoScriptRequest& request,
                               const QString& status, const QString& message)
{
    if (auto* src = qobject_cast<AutoScriptSource*>(request.origin.data()))
        src->sendStatus(request, status, message);
}

void AutoScriptManager::audit(const QString& stage, const AutoScriptRequest& r,
                              const QString& status, const QString& reason)
{
    const QString hash = r.meta.value("sha256").toString();

    qInfo().noquote()
        << "[AutoScript]" << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
        << stage << "id=" + r.id << "src=" + r.sourceName
        << "trusted=" + QString::number(r.trusted ? 1 : 0)
        << "sha256=" + hash << status << reason;

    if (m_engine) {
        m_engine->logEvent(ReportCategory::SYSTEM,
                           "AutoScript " + stage, r.id, r.sourceName,
                           "sha256=" + hash, QString(), status, reason);
    }
}
