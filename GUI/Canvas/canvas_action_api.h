#ifndef CANVAS_ACTION_API_H
#define CANVAS_ACTION_API_H
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <functional>
#include <QPointF>

class OverlayCanvas;
class MapWidget;
class QWidget;

struct CanvasActionContext {
    OverlayCanvas* canvas = nullptr;
    MapWidget*     map    = nullptr;
    QWidget*       parent = nullptr;
    QPointF        geoPos;
    QPoint         canvasPos;
};

struct CanvasActionDescriptor {
    QString id;
    QString label;
    QString icon;
    int     priority = 100;
    bool    enabled  = true;
    std::function<void(const CanvasActionContext&)> callback;
    std::function<bool(const CanvasActionContext&)> isEnabled;
};

class CanvasActionApi : public QObject
{
    Q_OBJECT
public:
    static CanvasActionApi* instance();
    bool registerAction(const CanvasActionDescriptor& desc);
    bool unregisterAction(const QString& id);
    void clearAll();
    QVector<CanvasActionDescriptor> actions() const;
    bool trigger(const QString& id, const CanvasActionContext& ctx);

signals:
    void registryChanged();

private:
    explicit CanvasActionApi(QObject* parent = nullptr);
    static CanvasActionApi* m_instance;
    void registerDefaultActions();
    QMap<QString, CanvasActionDescriptor> m_actions;
};

#endif // CANVAS_ACTION_API_H
