#include "autoscriptconfig.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

QString AutoScriptConfig::defaultPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/autoscript.ini");
}

static QString valueOf(const QStringList& lines, const QString& key, const QString& def)
{
    const QRegularExpression re(
        QStringLiteral("^\\s*%1\\s*=\\s*(.*)$").arg(QRegularExpression::escape(key)),
        QRegularExpression::CaseInsensitiveOption);
    for (const QString& line : lines) {
        const QString t = line.trimmed();
        if (t.startsWith('#') || t.startsWith(';'))
            continue;
        const auto m = re.match(t);
        if (m.hasMatch())
            return m.captured(1).trimmed();
    }
    return def;
}

AutoScriptConfig AutoScriptConfig::load(const QString& path)
{
    AutoScriptConfig c; // built-in defaults

    if (!QFileInfo::exists(path))
        return c; // no file required, do not create

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[AutoScript] Cannot read optional config:" << path;
        return c;
    }

    QStringList lines;
    QTextStream in(&f);
    while (!in.atEnd())
        lines.append(in.readLine());

    const QString addr = valueOf(lines, QStringLiteral("ListenAddress"), QString());
    if (!addr.isEmpty())
        c.listenAddress = addr;

    bool okPort = false;
    const int p = valueOf(lines, QStringLiteral("ListenPort"), QString()).toInt(&okPort);
    if (okPort && p > 0 && p <= 65535)
        c.port = static_cast<quint16>(p);

    const QString hosts = valueOf(lines, QStringLiteral("AllowedHosts"), QString());
    if (!hosts.isEmpty()) {
        c.allowedHosts.clear();
        for (const QString& h : hosts.split(',', Qt::SkipEmptyParts)) {
            const QString t = h.trimmed();
            if (!t.isEmpty())
                c.allowedHosts.append(t);
        }
    }

    bool ok = false;
    int v = valueOf(lines, QStringLiteral("MaxScriptKB"), QString()).toInt(&ok);
    if (ok && v >= 1)
        c.maxScriptKB = v;

    v = valueOf(lines, QStringLiteral("MaxQueue"), QString()).toInt(&ok);
    if (ok && v >= 1)
        c.maxQueue = v;

    const QString pol = valueOf(lines, QStringLiteral("Policy"), QString());
    if (!pol.isEmpty())
        c.policy = pol;

    v = valueOf(lines, QStringLiteral("MaxSliceSeconds"), QString()).toInt(&ok);
    if (ok && v >= 0)
        c.maxSliceSeconds = v;

    v = valueOf(lines, QStringLiteral("MaxTotalSeconds"), QString()).toInt(&ok);
    if (ok && v >= 0)
        c.maxTotalSeconds = v;

    const QString sand = valueOf(lines, QStringLiteral("SandboxDir"), QString());
    if (!sand.isEmpty())
        c.sandboxDir = sand;

    return c;
}
