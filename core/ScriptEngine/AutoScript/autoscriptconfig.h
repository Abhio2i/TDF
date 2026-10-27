#ifndef AUTOSCRIPTCONFIG_H
#define AUTOSCRIPTCONFIG_H

#include <QString>
#include <QStringList>

namespace AutoScriptDefaults {
constexpr quint16 port = 9090;
}

struct AutoScriptConfig
{
    bool        enabled = false;
    QString     listenAddress = QStringLiteral("any");
    quint16     port = AutoScriptDefaults::port;
    QStringList allowedHosts; // empty = any host
    int         maxScriptKB = 512;
    int         maxQueue = 32;
    QString     policy = QStringLiteral("Always");
    int         maxSliceSeconds = 10;
    int         maxTotalSeconds = 0;
    QString     sandboxDir;

    static QString          defaultPath();
    static AutoScriptConfig load(const QString& path);
};

#endif
