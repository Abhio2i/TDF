#ifndef UIPLUGINMANAGER_H
#define UIPLUGINMANAGER_H

#include <QObject>
#include <QString>
#include "GIS_API/backend/shapes/shape_type_registry.h"
#include "GIS_API/backend/formats/layer_format_registry.h"
#include "GIS_API/backend/api/map_api.h"
#include "GIS_API/backend/api/shape_api.h"
#include "GIS_API/frontend/ui_action_api.h"

class QAction;
class QWidget;


class UIPluginManager : public QObject
{
    Q_OBJECT
public:
    UIPluginManager();

signals:
    void registerSensorPanel(QWidget* panel);

    // DesignToolBar
    void registerDesignToolbarAction(QAction* action, const QString& pluginId);

    // RuntimeToolBar
    void registerRuntimeToolbarAction(QAction* action, const QString& pluginId);
    // NetworkToolBar
    void registerNetworkToolbarAction(QAction* action, const QString& pluginId);

    // MenuBar
    void registerMenuAction(const QString& menuName, QAction* action, const QString& pluginId);

    // GIS
    void registerGisToolbarAction(const QString& id,
                                  const QString& label,
                                  const QString& iconPath,
                                  const QString& category,
                                  bool checkable,
                                  int priority);

    void registerGisContextAction(const QString& id,
                                  const QString& label,
                                  const QString& contextType,
                                  const QString& group,
                                  int priority);
    void registerGisShapeType(IShapeType* type);
    void registerGisLayerFormat(ILayerFormat* format);
    void registerGisTileSource(const QString& id,
                               const QString& folder,
                               const QString& displayName,
                               const QString& iconPath,
                               int priority);

};

#endif
