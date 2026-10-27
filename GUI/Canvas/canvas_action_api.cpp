#include "canvas_action_api.h"
#include "overlay_canvas.h"
#include "map_widget.h"
#include "GUI/Hierarchytree/additemdialog.h"
#include "GUI/Hierarchytree/contextmenu.h"
#include "GUI/Hierarchytree/hierarchytree.h"
#include "GUI/Editors/runtimeeditor.h"
#include "GUI/Editors/scenarioeditor.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "GUI/mainwindow.h"
#include <QDebug>
#include <QProgressDialog>
#include <QApplication>
#include <algorithm>

CanvasActionApi* CanvasActionApi::m_instance = nullptr;
CanvasActionApi* CanvasActionApi::instance()
{
    if (!m_instance)
        m_instance = new CanvasActionApi();
    return m_instance;
}

CanvasActionApi::CanvasActionApi(QObject* parent)
    : QObject(parent)
{
    registerDefaultActions();
}

void CanvasActionApi::registerDefaultActions()
{
    registerAction({
        .id       = "canvas.add_entity",
        .label    = "➕ Add Entity",
        .priority = 5,
        .callback = [](const CanvasActionContext& ctx) {
            if (!ctx.map || !ctx.canvas)
                return;
            ScenarioEditor* se = nullptr;
            RuntimeEditor*  re = nullptr;
            QWidget* w = ctx.canvas;
            while (w) {
                if (!se) se = qobject_cast<ScenarioEditor*>(w);
                if (!re) re = qobject_cast<RuntimeEditor*>(w);
                w = w->parentWidget();
            }
            if (!se && !re)
                return;
            Hierarchy* library = nullptr;
            if (MainWindow* mw = MainWindow::instance())
                library = mw->getDatabaseHierarchy();
            Hierarchy* hierarchy = se ? se->hierarchy : re->hierarchy;
            HierarchyTree* treeView = se ? se->treeView : re->treeView;
            if (!library || !hierarchy || !treeView || !treeView->getContextMenu())
                return;
            AddItemDialog dialog(AddItemDialog::EntityType,
                                 "Platform",
                                 AddItemDialog::NormalMode,
                                 library,
                                 ctx.parent ? ctx.parent : ctx.canvas,
                                 se ? QStringLiteral("scenario") : QStringLiteral("runtime"));
            dialog.setWindowTitle("Add Entity");
            if (dialog.exec() != QDialog::Accepted || dialog.getName().isEmpty())
                return;
            const bool isProfileParent = true;
            QProgressDialog progressDialog(
                QStringLiteral("Adding Entities..."),
                QStringLiteral("Cancel"),
                0,
                dialog.getNumber(),
                ctx.parent ? ctx.parent : ctx.canvas);
            progressDialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            progressDialog.setWindowModality(Qt::WindowModal);
            progressDialog.setMinimumDuration(0);
            progressDialog.setValue(0);
            progressDialog.show();
            QApplication::processEvents();
            ProfileCategaory* profile = hierarchy->getProfileByName("Platform");
            const QString parentId = profile
                                         ? QString::fromStdString(profile->ID)
                                         : QString();

            double initLat = 190.0;
            double initLon = 0.0;
            if (!dialog.isScenarioconfigEnabled()) {
                initLat = ctx.geoPos.y();
                initLon = ctx.geoPos.x();
            }
            const QString baseName = dialog.getName();
            const int count = dialog.getNumber();
            ContextMenu* ctxMenu = treeView->getContextMenu();
            for (int i = 0; i < count; ++i) {
                if (progressDialog.wasCanceled())
                    break;
                const QString entityName =
                    (count == 1) ? baseName
                                 : QString("%1-%2").arg(baseName).arg(i + 1);
                emit ctxMenu->addEntityRequested(
                    parentId,
                    entityName,
                    isProfileParent,
                    dialog.getComponents(),
                    &dialog,
                    QString(),
                    initLat,
                    initLon,
                    0.0f
                    );
                progressDialog.setValue(i + 1);
                QApplication::processEvents();
            }
            progressDialog.close();
        }
    });

    // registerAction({
    //     .id       = "canvas.draw_point",
    //     .label    = "📍 Draw Point",
    //     .priority = 10,
    //     .callback = [](const CanvasActionContext& ctx) {
    //         Q_UNUSED(ctx)
    //         // if (ctx.canvas)
    //         //     ctx.canvas->startDrawPointMode();
    //     }
    // });

    // registerAction({
    //     .id       = "canvas.clear_points",
    //     .label    = "🗑️ Clear All Points",
    //     .priority = 100,
    //     .callback = [](const CanvasActionContext& ctx) {
    //         Q_UNUSED(ctx)
    //         // if (ctx.canvas)
    //         //     ctx.canvas->clearPoints();
    //     }
    // });
}
bool CanvasActionApi::registerAction(const CanvasActionDescriptor& desc)
{
    if (desc.id.isEmpty() || !desc.callback) {
        qWarning() << "CanvasActionApi: invalid action" << desc.id;
        return false;
    }
    m_actions[desc.id] = desc;
    emit registryChanged();
    return true;
}

bool CanvasActionApi::unregisterAction(const QString& id)
{
    if (!m_actions.contains(id))
        return false;
    m_actions.remove(id);
    emit registryChanged();
    return true;
}

void CanvasActionApi::clearAll()
{
    m_actions.clear();
    emit registryChanged();
}

QVector<CanvasActionDescriptor> CanvasActionApi::actions() const
{
    QVector<CanvasActionDescriptor> list;
    for (const auto& d : m_actions)
        list.append(d);
    std::sort(list.begin(), list.end(),
              [](const CanvasActionDescriptor& a, const CanvasActionDescriptor& b) {
                  if (a.priority != b.priority)
                      return a.priority < b.priority;
                  return a.label < b.label;
              });
    return list;
}

bool CanvasActionApi::trigger(const QString& id, const CanvasActionContext& ctx)
{
    if (!m_actions.contains(id))
    return false;
    const auto& d = m_actions[id];
    if (d.isEnabled && !d.isEnabled(ctx))
    return false;
    d.callback(ctx);
    return true;
}
