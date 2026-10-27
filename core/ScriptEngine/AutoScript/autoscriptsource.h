#ifndef AUTOSCRIPTSOURCE_H
#define AUTOSCRIPTSOURCE_H

#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>

// One script received from a remote/local source.
struct AutoScriptRequest
{
    QString id;                    // sender id (auto-generated if empty)
    QString content;               // full AngelScript source (content only, never a path)
    QString sourceName;            // e.g. "tcp:192.168.1.20:51234"
    bool    replaceRunning = false;// ask to abort the running script (honoured only if trusted)
    bool    trusted = false;       // sender authenticated (token matched)

    QPointer<QObject> origin;      // source that received it (for status replies)
    quint64 connectionId = 0;      // source specific connection handle

    QVariantMap meta;              // sha256 etc.
};
Q_DECLARE_METATYPE(AutoScriptRequest)

// Interface for every script provider (TCP now, folder/serial/... later).
// A source ONLY receives scripts. It never touches ScriptEngine.
class AutoScriptSource : public QObject
{
    Q_OBJECT
public:
    explicit AutoScriptSource(QObject* parent = nullptr) : QObject(parent) {}
    ~AutoScriptSource() override = default;

    virtual QString name() const = 0;
    virtual bool    start() = 0;
    virtual void    stop() = 0;
    virtual bool    isRunning() const = 0;

    // Manager calls this to report progress back to the sender.
    // status: QUEUED | RUNNING | DONE | ERROR | ABORTED | REJECTED | CANCELLED
    virtual void sendStatus(const AutoScriptRequest& request,
                            const QString& status,
                            const QString& message)
    {
        Q_UNUSED(request) Q_UNUSED(status) Q_UNUSED(message)
    }

signals:
    void scriptReceived(const AutoScriptRequest& request);
    void sourceError(const QString& message);
};

#endif // AUTOSCRIPTSOURCE_H
