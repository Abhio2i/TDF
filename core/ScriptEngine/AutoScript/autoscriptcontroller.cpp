#include "autoscriptcontroller.h"
#include "tcpautoscriptsource.h"
#include "core/ScriptEngine/scriptengine.h"

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>

namespace {

QString localIPv4()
{
    for (const QHostAddress& a : QNetworkInterface::allAddresses()) {
        if (a.protocol() == QAbstractSocket::IPv4Protocol
            && a != QHostAddress::LocalHost)
            return a.toString();
    }
    return QStringLiteral("127.0.0.1");
}

} // namespace

AutoScriptController::AutoScriptController(ScriptEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_configPath(AutoScriptConfig::defaultPath())
{
    m_cfg = AutoScriptConfig::load(m_configPath);
    m_manager = new AutoScriptManager(engine, this);
    m_tcp = new TcpAutoScriptSource(this);
    applyConfig(m_cfg);

    connect(m_manager, &AutoScriptManager::scriptQueued, this,
            [this](const AutoScriptRequest&) { refreshStatus(); });

    connect(m_manager, &AutoScriptManager::scriptStarted, this,
            [this](const AutoScriptRequest& r) {
                m_lastResult = QStringLiteral("RUNNING %1").arg(r.id);
                refreshStatus();
            });

    connect(m_manager, &AutoScriptManager::scriptFinished, this,
            [this](const AutoScriptRequest& r, bool ok) {
                m_lastResult = QStringLiteral("%1 %2").arg(ok ? QStringLiteral("DONE")
                                                              : QStringLiteral("ERROR"),
                                                              r.id);
                refreshStatus();
            });

    connect(m_manager, &AutoScriptManager::scriptRejected, this,
            [this](const AutoScriptRequest& r, const QString& reason) {
                m_lastResult = QStringLiteral("REJECTED %1 (%2)").arg(r.id, reason);
                refreshStatus();
            });

    // Always start OFF
    refreshStatus();
}

AutoScriptController::~AutoScriptController()
{
    if (m_tcp)
        m_tcp->stop();
}

bool AutoScriptController::applyConfig(const AutoScriptConfig& cfg)
{
    bool ok = true;
    m_cfg = cfg;

    AutoScriptManager::Policy pol = AutoScriptManager::Policy::Always;
    if (cfg.policy.compare(QStringLiteral("OnlyWhenRunning"), Qt::CaseInsensitive) == 0)
        pol = AutoScriptManager::Policy::OnlyWhenRunning;
    else if (cfg.policy.compare(QStringLiteral("QueueUntilRunning"), Qt::CaseInsensitive) == 0)
        pol = AutoScriptManager::Policy::QueueUntilRunning;
    else if (cfg.policy.compare(QStringLiteral("Always"), Qt::CaseInsensitive) == 0)
        pol = AutoScriptManager::Policy::Always;

    m_manager->setPolicy(pol);
    m_manager->setMaxQueueLength(cfg.maxQueue);
    m_manager->setMaxScriptSize(cfg.maxScriptKB * 1024);

    QString sandbox = cfg.sandboxDir;
    if (sandbox.isEmpty())
        sandbox = QCoreApplication::applicationDirPath() + QStringLiteral("/autoscript_files");
    m_manager->setSandboxDir(sandbox);

    if (m_engine) {
        m_engine->setMaxSliceTime(cfg.maxSliceSeconds > 0 ? cfg.maxSliceSeconds * 1000 : 0);
        m_engine->setMaxTotalTime(cfg.maxTotalSeconds > 0 ? cfg.maxTotalSeconds * 1000 : 0);
    }

    QHostAddress addr = QHostAddress::AnyIPv4;
    if (cfg.listenAddress.compare(QStringLiteral("any"), Qt::CaseInsensitive) != 0
        && !cfg.listenAddress.isEmpty()) {
        addr = QHostAddress(cfg.listenAddress);
        if (addr.isNull()) {
            qWarning() << "[AutoScript] Invalid ListenAddress:" << cfg.listenAddress;
            ok = false;
            addr = QHostAddress::AnyIPv4;
        }
    }

    m_tcp->setListenAddress(addr);
    m_tcp->setListenPort(cfg.port);
    m_tcp->setMaxScriptSize(cfg.maxScriptKB * 1024);

    QList<QHostAddress> hosts;
    for (const QString& h : cfg.allowedHosts) {
        const QString t = h.trimmed();
        if (t.isEmpty())
            continue;
        QHostAddress a(t);
        if (!a.isNull()) {
            hosts.append(a);
        } else {
            qWarning() << "[AutoScript] Invalid AllowedHosts entry:" << h;
            ok = false;
        }
    }
    m_tcp->setAllowedHosts(hosts);

    m_manager->registerSource(m_tcp);
    return ok;
}

void AutoScriptController::attachToggleAction(QAction* action)
{
    m_toggleAction = action;
    if (!action)
        return;

    action->setCheckable(true);
    action->setChecked(isEnabled());
    action->setToolTip(statusText());
    connect(action, &QAction::toggled, this, &AutoScriptController::setEnabled);
    refreshStatus();
}

void AutoScriptController::setSimulationState(AutoScriptManager::SimState state)
{
    if (m_manager)
        m_manager->setSimulationState(state);
    refreshStatus();
}

bool AutoScriptController::isEnabled() const
{
    return m_tcp && m_tcp->isRunning();
}

void AutoScriptController::setEnabled(bool on)
{
    if (on) {
        m_cfg = AutoScriptConfig::load(m_configPath);

        if (m_cfg.listenAddress.isEmpty())
            m_cfg.listenAddress = QStringLiteral("any");
        if (m_cfg.policy.isEmpty())
            m_cfg.policy = QStringLiteral("Always");
        if (m_cfg.port == 0)
            m_cfg.port = AutoScriptDefaults::port;

        if (!applyConfig(m_cfg)) {
            qWarning() << "[AutoScript] Enable refused: optional config errors in"
                       << m_configPath;
            if (m_toggleAction)
                m_toggleAction->setChecked(false);
            refreshStatus();
            return;
        }

        if (!m_tcp->start()) {
            if (m_toggleAction)
                m_toggleAction->setChecked(false);
            refreshStatus();
            return;
        }
        m_manager->setEnabled(true);
    } else {
        m_manager->setEnabled(false);
        m_tcp->stop();
    }

    if (m_toggleAction && m_toggleAction->isChecked() != on)
        m_toggleAction->setChecked(on);
    refreshStatus();
}

QString AutoScriptController::statusText() const
{
    if (!m_tcp || !m_tcp->isRunning())
        return QStringLiteral("AutoScript: OFF");

    const QString addr =
        (m_cfg.listenAddress.compare(QStringLiteral("any"), Qt::CaseInsensitive) == 0)
            ? localIPv4()
            : m_cfg.listenAddress;

    const int q = m_manager ? m_manager->queueSize() : 0;
    QString s = QStringLiteral("AutoScript: ON %1:%2 | queue=%3")
                    .arg(addr)
                    .arg(m_cfg.port)
                    .arg(q);
    if (!m_lastResult.isEmpty())
        s += QStringLiteral(" | last=%1").arg(m_lastResult);
    return s;
}

void AutoScriptController::resetQueue(const QString& reason)
{
    if (m_manager)
        m_manager->resetQueue(reason);
    refreshStatus();
}

void AutoScriptController::refreshStatus()
{
    const QString t = statusText();
    if (m_toggleAction)
        m_toggleAction->setToolTip(t);
    emit statusChanged(t);
}
