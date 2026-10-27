
#ifndef UI_ACTION_API_H
#define UI_ACTION_API_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QIcon>
#include <QPointF>
#include <QVariantMap>
#include <QVector>
#include <QMap>
#include <functional>
#include <QPointer>

class MapWidget;
class QWidget;

enum class UiActionCategory {
    ToolbarMain,
    ToolbarShape,
    ToolbarBitmap,
    ToolbarPreset,
    ToolbarMapLayer,
    ToolbarCoord,
    ContextMap,
    ContextShape,
    ContextLayer,
    Custom
};

struct UiActionContext {
    MapWidget* map = nullptr;
    QWidget*   parentWidget = nullptr;
    QPointF    geoPos;
    QString    shapeId;
    QString    layerId;
    QVariantMap extra;
};

struct UiActionDescriptor {
    QString id;
    QString label;
    QString iconPath;
    UiActionCategory category = UiActionCategory::Custom;
    QString group;
    bool checkable = false;
    bool enabled = true;
    int  priority = 100;
    std::function<void(const UiActionContext&)> callback;
    std::function<bool(const UiActionContext&)> isEnabled;
    std::function<bool(const UiActionContext&)> isChecked;
};

class UiActionApi : public QObject {
    Q_OBJECT

public:
    static UiActionApi* instance();
    bool registerAction(const UiActionDescriptor& desc);
    bool unregisterAction(const QString& id);
    void clearCategory(UiActionCategory category);
    void clearAll();
    QVector<UiActionDescriptor> actionsFor(UiActionCategory category) const;
    QVector<UiActionDescriptor> actionsForGroup(UiActionCategory category, const QString& group) const;
    QStringList groupsIn(UiActionCategory category) const;
    bool hasAction(const QString& id) const;
    UiActionDescriptor action(const QString& id) const;
    bool trigger(const QString& id, const UiActionContext& ctx = UiActionContext());
    void registerCoreActions(MapWidget* map);
    void syncMapLayerActions();


signals:
    void actionRegistered(const QString& id);
    void actionUnregistered(const QString& id);
    void registryChanged();

private:
    explicit UiActionApi(QObject* parent = nullptr);
    static UiActionApi* m_instance;
    QMap<QString, UiActionDescriptor> m_actions;
    QPointer<MapWidget> m_mapForActions;
};

#endif // UI_ACTION_API_H
