#ifndef API_MAP_TOOLBAR_H
#define API_MAP_TOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QMap>
#include "ui_action_api.h"

class MapWidget;
class QToolButton;

class ApiMapToolbar : public QToolBar {
    Q_OBJECT
public:
    explicit ApiMapToolbar(MapWidget* map, QWidget* parent = nullptr);
    void rebuildFromRegistry();
    void setGisInteractionChecked(bool checked);
    void setGisToolsEnabled(bool enabled);
    QAction* gisInteractionAction() const { return m_gisInteractionAction; }

private slots:
    void onRegistryChanged();
    void onPlacingBitmapChanged(const QString& activeName);
    void onDrawingTypeChanged(const QString& activeType);
    void onCoordSystemChanged(const QString& activeSys);
private:
    void addSearchPopup();
    QToolButton* addCategoryPopup(const QString& label,
                                  const QString& iconPath,
                                  UiActionCategory category,
                                  bool exclusive);
    QAction* makeActionFromDescriptor(const UiActionDescriptor& d,
                                      const UiActionContext& ctx);
    void applyGisToolsEnabled();
    QPixmap withWhiteBg(const QString& iconPath);
    MapWidget* m_map = nullptr;
    QAction* m_measureAction = nullptr;
    QAction* m_gisInteractionAction = nullptr;
    bool m_gisToolsEnabled = false;
    QMap<QString, QAction*> m_bitmapActions;
    QMap<QString, QAction*> m_coordActions;
    QMap<QString, QAction*> m_shapeActions;
};

#endif
