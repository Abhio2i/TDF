
#ifndef DESIGNTOOLBAR_H
#define DESIGNTOOLBAR_H

#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <core/Components/transform.h>



class StayOpenMenu : public QMenu {
public:
    explicit StayOpenMenu(QWidget* parent = nullptr) : QMenu(parent) {}

protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        QAction* action = activeAction();
        if (action && action->isEnabled()) {
            action->trigger();
        } else {
            QMenu::mouseReleaseEvent(e);
        }
    }
};
class DesignToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit DesignToolBar(QWidget *parent = nullptr);
    QAction* getLoadAction() { return loadJsonAction; }
    QAction* getSaveAction() { return saveJsonAction; }
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *mapSelectLayerAction;
    QAction *selectCenterAction;
    QAction *searchPlaceAction;
signals:
    void viewTriggered();
    void moveTriggered();
    void rotateTriggered();
    void scaleTriggered();
    void zoomInTriggered();
    void zoomOutTriggered();
    void gridVisibilityToggled(bool);
    void gridSnappingToggled(bool);
    void layerSelectTriggered();
    void measureAreaTriggered();
    void drawTriggered();
    void databaseTriggered();
    // New signals for grid control
    void gridPlaneXToggled(bool);
    void gridPlaneYToggled(bool);
    void gridPlaneZToggled(bool);
    void gridOpacityChanged(int);
    //==============================
    void snapGridSizeXToggled(bool);
    void snapGridSizeYToggled(bool);
    void snapGridSizeZToggled(bool);
    void snapRotateToggled(bool);
    void snapScaleToggled(bool);
    void modeChanged(TransformMode mode);
    void transformModeChanged(const Transform& mode);

    void gridPlaneXOpacityChanged(int value);
    void gridPlaneYOpacityChanged(int value);
    void gridPlaneZOpacityChanged(int value);
    void layerOptionToggled(const QString& option, bool checked);

    void searchPlaceTriggered(const QString& location);
    void mapLayerChanged(const QString& layer);

    void selectCenterTriggered();
public slots:
    void onModeChanged(int mode);
private:
    QAction *viewAction;
    QAction *moveAction;
    QAction *rotateAction;
    QAction *scaleAction;
    // QAction *zoomInAction;
    // QAction *zoomOutAction;
    QAction *gridToggleAction;
    QAction *snappingToggleAction;
    QAction *layerSelectAction;
    QAction *measureAreaAction;
    QAction *drawAction;
    QAction *databaseAction;
    // New actions for grid control
    QAction *gridPlaneXAction;
    QAction *gridPlaneYAction;
    QAction *gridPlaneZAction;

    void createActions();
    void setupToolBar();
    void highlightAction(QAction *action);
    QPixmap withWhiteBg(const QString &iconPath);
    QAction* loadJsonAction;
    QAction* saveJsonAction;

    QAction *snapGridSizeXAction;
    QAction *snapGridSizeYAction;
    QAction *snapGridSizeZAction;
    QAction *snapRotateAction;
    QAction *snapScaleAction;



};

#endif
