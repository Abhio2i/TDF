#include "ui_action_api.h"
#include "frontend/widgets/map_widget.h"
#include "backend/api/shape_api.h"
#include "backend/api/measure_api.h"
#include "backend/api/airbase_api.h"
#include "backend/api/map_api.h"
#include "backend/api/shape_history_api.h"
#include "backend/api/layer_api.h"
#include <QMessageBox>
#include <QLineEdit>
#include <QDebug>
#include <QInputDialog>
#include <QFileDialog>
#include <algorithm>
#include "backend/api/layer_api.h"
#include "backend/formats/layer_format_registry.h"
#include "backend/formats/builtin_layer_formats.h"
#include "backend/api/elevation_data.h"
#include <QFileInfo>
#include <QImageReader>

static bool ensureGisInteraction(const UiActionContext& ctx)
{
    if (!ctx.map) {
        qWarning() << "ensureGisInteraction: map is null";
        QMessageBox::warning(
            ctx.parentWidget,
            QStringLiteral("GIS Mode"),
            QStringLiteral("Map is not available."));
        return false;
    }
    if (ctx.map->isGisInteractionEnabled())
        return true;
    const auto reply = QMessageBox::warning(
        ctx.parentWidget,
        QStringLiteral("GIS Mode"),
        QStringLiteral("Enable GIS mode to use this tool?"),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Ok);
    if (reply != QMessageBox::Ok)
        return false;
    ctx.map->setGisInteractionEnabled(true);
    return true;
}
UiActionApi* UiActionApi::m_instance = nullptr;
UiActionApi* UiActionApi::instance()
{
    if (!m_instance)
        m_instance = new UiActionApi();
    return m_instance;
}

UiActionApi::UiActionApi(QObject* parent)
    : QObject(parent)
{
}

bool UiActionApi::registerAction(const UiActionDescriptor& desc)
{
    if (desc.id.isEmpty()) {
        qWarning() << "UiActionApi: reject action with empty id";
        return false;
    }
    if (!desc.callback) {
        qWarning() << "UiActionApi: reject action without callback:" << desc.id;
        return false;
    }
    const bool isNew = !m_actions.contains(desc.id);
    m_actions[desc.id] = desc;
    if (isNew)
        emit actionRegistered(desc.id);
    emit registryChanged();
    return true;
}

bool UiActionApi::unregisterAction(const QString& id)
{
    if (!m_actions.contains(id))
        return false;
    m_actions.remove(id);
    emit actionUnregistered(id);
    emit registryChanged();
    return true;
}

void UiActionApi::clearCategory(UiActionCategory category)
{
    QStringList toRemove;
    for (auto it = m_actions.constBegin(); it != m_actions.constEnd(); ++it) {
        if (it.value().category == category)
            toRemove.append(it.key());
    }
    for (const QString& id : toRemove)
        m_actions.remove(id);
    if (!toRemove.isEmpty())
        emit registryChanged();
}

void UiActionApi::clearAll()
{
    m_actions.clear();
    emit registryChanged();
}

QVector<UiActionDescriptor> UiActionApi::actionsFor(UiActionCategory category) const
{
    QVector<UiActionDescriptor> list;
    for (const UiActionDescriptor& d : m_actions) {
        if (d.category == category)
            list.append(d);
    }
    std::sort(list.begin(), list.end(),
              [](const UiActionDescriptor& a, const UiActionDescriptor& b) {
                  if (a.priority != b.priority)
                      return a.priority < b.priority;
                  return a.label < b.label;
              });
    return list;
}

QVector<UiActionDescriptor> UiActionApi::actionsForGroup(UiActionCategory category,
                                                         const QString& group) const
{
    QVector<UiActionDescriptor> list;
    for (const UiActionDescriptor& d : m_actions) {
        if (d.category == category && d.group == group)
            list.append(d);
    }
    std::sort(list.begin(), list.end(),
              [](const UiActionDescriptor& a, const UiActionDescriptor& b) {
                  if (a.priority != b.priority)
                      return a.priority < b.priority;
                  return a.label < b.label;
              });
    return list;
}

QStringList UiActionApi::groupsIn(UiActionCategory category) const
{
    QStringList groups;
    for (const UiActionDescriptor& d : m_actions) {
        if (d.category == category && !d.group.isEmpty() && !groups.contains(d.group))
            groups.append(d.group);
    }
    groups.sort();
    return groups;
}

bool UiActionApi::hasAction(const QString& id) const
{
    return m_actions.contains(id);
}

UiActionDescriptor UiActionApi::action(const QString& id) const
{
    return m_actions.value(id);
}

bool UiActionApi::trigger(const QString& id, const UiActionContext& ctx)
{
    if (!m_actions.contains(id))
        return false;
    const UiActionDescriptor& d = m_actions[id];
    if (d.isEnabled && !d.isEnabled(ctx))
        return false;
    if (d.callback)
        d.callback(ctx);
    return true;
}

// ============================================================
// Core built-in actions
// ============================================================

void UiActionApi::registerCoreActions(MapWidget* map)
{
    if (!map)
        return;
    m_mapForActions = map;
    if (hasAction("core.zoom_in")) {
        if (m_actions.contains(QStringLiteral("core.select_bitmap"))) {
            m_actions[QStringLiteral("core.select_bitmap")].callback =
                [](const UiActionContext& ctx) {
                    if (!ensureGisInteraction(ctx))
                        return;
                    if (!ctx.map) return;
                    QString path = QFileDialog::getOpenFileName(
                        ctx.parentWidget, "Select Bitmap Image", QString(),
                        "Images (*.png *.jpg *.jpeg *.bmp)");
                    if (!path.isEmpty())
                        ctx.map->startPlacingBitmapImage(path);
                };
        }
        const QList<QPair<QString, QString>> shapes = {
                                                       {QStringLiteral("core.shape.line"), QStringLiteral("line")},
                                                       {QStringLiteral("core.shape.circle"), QStringLiteral("circle")},
                                                       {QStringLiteral("core.shape.rectangle"), QStringLiteral("rectangle")},
                                                       {QStringLiteral("core.shape.polygon"), QStringLiteral("polygon")},
                                                       {QStringLiteral("core.shape.point"), QStringLiteral("point")},
                                                       };
        for (const auto& p : shapes) {
            if (!m_actions.contains(p.first))
                continue;
            const QString type = p.second;
            m_actions[p.first].callback = [type](const UiActionContext& ctx) {
                if (!ensureGisInteraction(ctx))
                    return;
                if (!ctx.map) return;
                if (ctx.map->activeDrawingType() == type)
                    ctx.map->cancelDrawing();
                else
                    ctx.map->startDrawing(type);
            };
        }
        const QList<QPair<QString, QString>> bitmaps = {
                                                        {QStringLiteral("core.bitmap.hospital"), QStringLiteral("Hospital")},
                                                        {QStringLiteral("core.bitmap.school"), QStringLiteral("School")},
                                                        {QStringLiteral("core.bitmap.forest"), QStringLiteral("Forest Area")},
                                                        };
        for (const auto& p : bitmaps) {
            if (!m_actions.contains(p.first))
                continue;
            const QString name = p.second;

            m_actions[p.first].callback = [name](const UiActionContext& ctx) {
                if (!ensureGisInteraction(ctx))
                    return;
                if (!ctx.map) return;
                if (ctx.map->activePlacingBitmap() == name)
                    ctx.map->cancelPlacingBitmap();
                else
                    ctx.map->startPlacingBitmap(name);
            };
        }
        syncMapLayerActions();
        return;
    }

    // ---------- ToolbarMain ----------
    registerAction({
        .id = "core.gis_interaction",
        .label = "GIS Interaction",
        .iconPath = ":/icons/images/mapediting.png",
        .category = UiActionCategory::ToolbarMain,
        .checkable = true,
        .priority = 15,
        .callback = [](const UiActionContext& ctx) {
            if (!ctx.map) return;
            bool current = ctx.map->isGisInteractionEnabled();
            ctx.map->setGisInteractionEnabled(!current);
        },
        .isChecked = [](const UiActionContext& ctx) {
            return ctx.map && ctx.map->isGisInteractionEnabled();
        }
    });
    registerAction({
        .id = "core.zoom_in",
        .label = "Zoom In",
        .iconPath = ":/icons/images/zoom-in.png",
        .category = UiActionCategory::ToolbarMain,
        .priority = 10,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->zoomIn();
        }
    });
    registerAction({
        .id = "core.zoom_out",
        .label = "Zoom Out",
        .iconPath = ":/icons/images/zoom-out.png",
        .category = UiActionCategory::ToolbarMain,
        .priority = 20,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->zoomOut();
        }
    });
    registerAction({
        .id = "core.center",
        .label = "Select Center",
        .iconPath = ":/icons/images/centremap.png",
        .category = UiActionCategory::ToolbarMain,
        .priority = 30,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->startSelectCenter();
        }
    });
    registerAction({
        .id = "core.measure",
        .label = "Measure Distance",
        .iconPath = ":/icons/images/measurement.png",
        .category = UiActionCategory::ToolbarMain,
        .checkable = true,
        .priority = 40,
        .callback = [](const UiActionContext& ctx) {
            if (!ctx.map) return;
            if (ctx.map->isMeasuring())
                ctx.map->stopMeasuring();
            else
                ctx.map->startMeasuring();
        },
        .isChecked = [](const UiActionContext& ctx) {
            return ctx.map && ctx.map->isMeasuring();
        }
    });
    registerAction({
        .id = "core.select_bitmap",
        .label = "Select Bitmap Image",
        .iconPath = ":/icons/images/picture.png",
        .category = UiActionCategory::ToolbarMain,
        .priority = 50,
        .callback = [](const UiActionContext& ctx) {
            if (!ensureGisInteraction(ctx))
                return;
            if (!ctx.map) return;
            QString path = QFileDialog::getOpenFileName(
                ctx.parentWidget, "Select Bitmap Image", QString(),
                "Images (*.png *.jpg *.jpeg *.bmp)");
            if (!path.isEmpty())
                ctx.map->startPlacingBitmapImage(path);
        }
    });

    // ---------- ToolbarCoord ----------
    registerAction({
        .id = "core.coord.geo",
        .label = "Geo-detic",
        .category = UiActionCategory::ToolbarCoord,
        .checkable = true,
        .priority = 10,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->setCoordSystem("EPSG:4326");
        },
        .isChecked = [](const UiActionContext& ctx) {
            return ctx.map && ctx.map->coordSystem() == QLatin1String("EPSG:4326");
        }
    });
    registerAction({
        .id = "core.coord.utm",
        .label = "UTM",
        .category = UiActionCategory::ToolbarCoord,
        .checkable = true,
        .priority = 20,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->setCoordSystem("UTM_AUTO");
        },
        .isChecked = [](const UiActionContext& ctx) {
            return ctx.map && ctx.map->coordSystem() == QLatin1String("UTM_AUTO");
        }
    });
    registerAction({
        .id = "core.coord.mgrs",
        .label = "MGRS",
        .category = UiActionCategory::ToolbarCoord,
        .checkable = true,
        .priority = 30,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map) ctx.map->setCoordSystem("MGRS");
        },
        .isChecked = [](const UiActionContext& ctx) {
            return ctx.map && ctx.map->coordSystem() == QLatin1String("MGRS");
        }
    });

    syncMapLayerActions();
    connect(MapApi::instance(), &MapApi::sourcesChanged,
            this, &UiActionApi::syncMapLayerActions);

    // ---------- ToolbarShape ----------
    auto addShape = [this](const QString& id, const QString& label,
                           const QString& type, int prio) {
        registerAction({
            .id = id,
            .label = label,
            .category = UiActionCategory::ToolbarShape,
            .group = "shapes",
            .checkable = true,
            .priority = prio,

            .callback = [type](const UiActionContext& ctx) {
                if (!ensureGisInteraction(ctx))
                    return;
                if (!ctx.map) return;
                if (ctx.map->activeDrawingType() == type)
                    ctx.map->cancelDrawing();
                else
                    ctx.map->startDrawing(type);
            },
            .isChecked = [type](const UiActionContext& ctx) {
                return ctx.map && ctx.map->activeDrawingType() == type;
            }
        });
    };
    addShape("core.shape.line", "Line", "line", 10);
    addShape("core.shape.circle", "Circle", "circle", 20);
    addShape("core.shape.rectangle", "Rectangle", "rectangle", 30);
    addShape("core.shape.polygon", "Polygon", "polygon", 40);
    addShape("core.shape.point", "Points", "point", 50);

    // ---------- ToolbarBitmap ----------
    auto addBitmap = [this](const QString& id, const QString& name,
                            const QString& icon, int prio) {
        registerAction({
            .id = id,
            .label = name,
            .iconPath = icon,
            .category = UiActionCategory::ToolbarBitmap,
            .group = "bitmaps",
            .checkable = true,
            .priority = prio,

            .callback = [name](const UiActionContext& ctx) {
                if (!ensureGisInteraction(ctx))
                    return;
                if (!ctx.map) return;
                if (ctx.map->activePlacingBitmap() == name)
                    ctx.map->cancelPlacingBitmap();
                else
                    ctx.map->startPlacingBitmap(name);
            },

            .isChecked = [name](const UiActionContext& ctx) {
                return ctx.map && ctx.map->activePlacingBitmap() == name;
            }
        });
    };
    addBitmap("core.bitmap.hospital", "Hospital",
              ":/icons/images/hospital.png", 10);
    addBitmap("core.bitmap.school", "School",
              ":/icons/images/school.png", 20);
    addBitmap("core.bitmap.forest", "Forest Area",
              ":/icons/images/forest-area.png", 30);

    // ---------- ToolbarPreset ----------
    registerAction({
        .id = "core.preset.airbase",
        .label = "Airbase",
        .category = UiActionCategory::ToolbarPreset,
        .group = "presets",
        .checkable = true,
        .priority = 10,
        .callback = [](const UiActionContext& ctx) {
            AirbaseApi* api = AirbaseApi::instance();
            api->initialize();
            api->toggleVisibility();
            if (ctx.map) ctx.map->update();
        },
        .isChecked = [](const UiActionContext&) {
            return AirbaseApi::instance()->isVisible();
        }
    });

    // ---------- ContextMap ----------
    auto addCtxShape = [this](const QString& id, const QString& label,
                              const QString& type, int prio) {
        registerAction({
            .id = id,
            .label = label,
            .category = UiActionCategory::ContextMap,
            .group = "Add Shape",
            .priority = prio,
            .callback = [type](const UiActionContext& ctx) {
                if (!ensureGisInteraction(ctx))
                    return;
                if (ctx.map) ctx.map->startDrawing(type);
            }
        });
    };
    addCtxShape("core.ctx.circle", "🔵 Circle", "circle", 10);
    addCtxShape("core.ctx.rectangle", "🟩 Rectangle", "rectangle", 20);
    addCtxShape("core.ctx.polygon", "🔺 Polygon", "polygon", 30);
    addCtxShape("core.ctx.line", "📏 Line", "line", 40);
    addCtxShape("core.ctx.point", "📍 Point", "point", 50);

    registerAction({
        .id = "core.ctx.add_text",
        .label = "📝 Add Text",
        .category = UiActionCategory::ContextMap,
        .priority = 60,
        .callback = [](const UiActionContext& ctx) {
            if (!ensureGisInteraction(ctx))
                return;
            if (ctx.map) ctx.map->addTextAtPosition(ctx.geoPos);
        }
    });
    registerAction({
        .id = "core.ctx.clear_all",
        .label = "🗑️ Clear All",
        .category = UiActionCategory::ContextMap,
        .priority = 200,
        .callback = [](const UiActionContext& ctx) {
            ShapeApi::instance()->deleteAllShapes();
            ShapeHistoryApi::instance()->clearAll();
            if (ctx.map) ctx.map->update();
        }
    });

    // ---------- ContextShape ----------
    auto shapeEnabled = [](const UiActionContext& ctx) {
        if (!ctx.map || ctx.shapeId.isEmpty())
            return false;
        const QString layerId = LayerApi::instance()->getLayerOfShape(ctx.shapeId);
        if (!layerId.isEmpty() && !LayerApi::instance()->isEditable(layerId))
            return false;
        return true;
    };

    registerAction({
        .id = "core.ctx.edit",
        .label = "✏️ Edit",
        .category = UiActionCategory::ContextShape,
        .priority = 10,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->startEditingShape(ctx.shapeId);
        },
        .isEnabled = [](const UiActionContext& ctx) {
            if (!ctx.map || ctx.shapeId.isEmpty())
                return false;
            const QString layerId = LayerApi::instance()->getLayerOfShape(ctx.shapeId);
            if (!layerId.isEmpty() && !LayerApi::instance()->isEditable(layerId))
                return false;
            const QJsonObject props = ShapeApi::instance()->getShapeProperties(ctx.shapeId);
            return props.value(QStringLiteral("type")).toString() != QLatin1String("text");
        }
    });
    registerAction({
        .id = "core.ctx.move",
        .label = "↕️ Move",
        .category = UiActionCategory::ContextShape,
        .priority = 20,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->startMovingShape(ctx.shapeId);
        },
        .isEnabled = shapeEnabled
    });
    registerAction({
        .id = "core.ctx.rotate",
        .label = "🔄 Rotate",
        .category = UiActionCategory::ContextShape,
        .priority = 30,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->startRotatingShape(ctx.shapeId);
        },
        .isEnabled = shapeEnabled
    });
    registerAction({
        .id = "core.ctx.properties",
        .label = "📋 Properties",
        .category = UiActionCategory::ContextShape,
        .priority = 40,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->showProperties(ctx.shapeId);
        },
        .isEnabled = [](const UiActionContext& ctx) {
            if (!ctx.map || ctx.shapeId.isEmpty())
                return false;
            const QString layerId = LayerApi::instance()->getLayerOfShape(ctx.shapeId);
            if (!layerId.isEmpty() && !LayerApi::instance()->isEditable(layerId))
                return false;
            const QJsonObject props = ShapeApi::instance()->getShapeProperties(ctx.shapeId);
            const QString type = props.value(QStringLiteral("type")).toString();
            return type != QLatin1String("bitmap") && type != QLatin1String("raster_image");
        }
    });
    registerAction({
        .id = "core.ctx.copy",
        .label = "📋 Copy",
        .category = UiActionCategory::ContextShape,
        .priority = 50,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->copyShape(ctx.shapeId);
        },
        .isEnabled = [](const UiActionContext& ctx) {
            if (!ctx.map || ctx.shapeId.isEmpty())
                return false;
            const QString layerId = LayerApi::instance()->getLayerOfShape(ctx.shapeId);
            if (!layerId.isEmpty() && !LayerApi::instance()->isEditable(layerId))
                return false;
            const QJsonObject props = ShapeApi::instance()->getShapeProperties(ctx.shapeId);
            const QString type = props.value(QStringLiteral("type")).toString();
            return type != QLatin1String("bitmap") && type != QLatin1String("raster_image");
        }
    });

    auto hasShapeHistory = [](const UiActionContext& ctx) {
        return ctx.map
               && !ctx.shapeId.isEmpty()
               && ShapeHistoryApi::instance()->hasHistory(ctx.shapeId);
    };
    auto canPreview = [](const UiActionContext& ctx) {
        return ctx.map
               && !ctx.shapeId.isEmpty()
               && ShapeHistoryApi::instance()->hasHistory(ctx.shapeId)
               && !ShapeHistoryApi::instance()->isPreviewActive();
    };
    auto canHidePreview = [](const UiActionContext& ctx) {
        return ctx.map
               && !ctx.shapeId.isEmpty()
               && ShapeHistoryApi::instance()->isPreviewActive()
               && ShapeHistoryApi::instance()->previewShapeId() == ctx.shapeId;
    };

    registerAction({
        .id = "core.ctx.preview_history",
        .label = "👁 Preview history",
        .category = UiActionCategory::ContextShape,
        .priority = 55,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.shapeId.isEmpty()) return;
            ShapeHistoryApi::instance()->startPreview(ctx.shapeId);
            if (ctx.map) ctx.map->update();
        },
        .isEnabled = canPreview
    });
    registerAction({
        .id = "core.ctx.restore_history",
        .label = "↩ Restore history",
        .category = UiActionCategory::ContextShape,
        .priority = 56,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.shapeId.isEmpty()) return;
            ShapeHistoryApi::instance()->restoreHistory(ctx.shapeId);
            if (ctx.map) ctx.map->update();
        },
        .isEnabled = hasShapeHistory
    });
    registerAction({
        .id = "core.ctx.hide_preview",
        .label = "🙈 Hide preview",
        .category = UiActionCategory::ContextShape,
        .priority = 57,
        .callback = [](const UiActionContext& ctx) {
            ShapeHistoryApi::instance()->hidePreview();
            if (ctx.map) ctx.map->update();
        },
        .isEnabled = canHidePreview
    });
    registerAction({
        .id = "core.ctx.delete",
        .label = "🗑️ Delete",
        .category = UiActionCategory::ContextShape,
        .priority = 100,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.map && !ctx.shapeId.isEmpty())
                ctx.map->deleteShape(ctx.shapeId);
        },
        .isEnabled = shapeEnabled
    });
    auto layerPanelRootEnabled = [](const UiActionContext& ctx) {
        return ctx.shapeId.isEmpty();
    };
    auto layerRowEnabled = [](const UiActionContext& ctx) {
        return !ctx.layerId.isEmpty() && ctx.shapeId.isEmpty();
    };
    auto shapeRowEnabled = [](const UiActionContext& ctx) {
        if (ctx.shapeId.isEmpty())
            return false;
        const QString layerId = ctx.layerId.isEmpty()
                                    ? LayerApi::instance()->getLayerOfShape(ctx.shapeId)
                                    : ctx.layerId;
        if (!layerId.isEmpty() && !LayerApi::instance()->isEditable(layerId))
            return false;
        return true;
    };
    registerAction({
        .id = "core.layer.add",
        .label = "Add Vector Layer",
        .category = UiActionCategory::ContextLayer,
        .priority = 10,
        .callback = [](const UiActionContext& ctx) {
            QWidget* parent = ctx.parentWidget;
            bool ok = false;
            QString name = QInputDialog::getText(
                parent, QStringLiteral("Add Vector Layer"), QStringLiteral("Layer name:"),
                QLineEdit::Normal, QStringLiteral("New Layer"), &ok);
            if (ok && !name.trimmed().isEmpty())
                LayerApi::instance()->addLayer(name.trimmed());
        },
        .isEnabled = layerPanelRootEnabled
    });
    registerAction({
        .id = "core.layer.add_raster",
        .label = "Add Raster Layer",
        .category = UiActionCategory::ContextLayer,
        .priority = 15,
        .callback = [](const UiActionContext& ctx) {
            QWidget* parent = ctx.parentWidget;
            const QString path = QFileDialog::getOpenFileName(
                parent,
                QStringLiteral("Select Raster Image"),
                QString(),
                QStringLiteral("Images (*.png *.jpg *.jpeg *.tif *.tiff *.bmp);;All Files (*)"));
            if (path.isEmpty())
                return;

            QFileInfo fi(path);
            QString layerName = fi.completeBaseName();
            if (layerName.isEmpty())
                layerName = QStringLiteral("Raster");

            LayerApi* la = LayerApi::instance();

            // Raster add hone se PEHLE jo layer active tha, use yaad rakho
            const QString previousActiveLayerId = la->getActiveLayer();
            // Unique layer name if already exists
            {
                const QString base = layerName;
                int n = 2;
                while (true) {
                    bool exists = false;
                    for (const QString& lid : la->getLayers()) {
                        if (la->getLayerName(lid).compare(layerName, Qt::CaseInsensitive) == 0) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                        break;
                    layerName = base + QStringLiteral(" (%1)").arg(n++);
                }
            }

            const QString layerId = la->addLayer(layerName);
            if (layerId.isEmpty())
                return;

            double lon = 78.9629;
            double lat = 20.5937;
            if (ctx.map) {
                lon = ctx.map->getCenterLon();
                lat = ctx.map->getCenterLat();
            } else if (MapApi::instance()) {
                lon = MapApi::instance()->getCenterLon();
                lat = MapApi::instance()->getCenterLat();
            }

            QImageReader reader(path);
            QSize imgSize = reader.size();
            double aspect = (imgSize.isValid() && imgSize.height() > 0)
                                ? static_cast<double>(imgSize.width()) / imgSize.height()
                                : 1.0;

            const double defaultW = 1.0;
            const double defaultH = defaultW / aspect;

            QJsonObject shape;
            shape[QStringLiteral("type")]      = QStringLiteral("raster_image");
            shape[QStringLiteral("imagePath")] = path;
            shape[QStringLiteral("text")]      = layerName;
            shape[QStringLiteral("width")]     = defaultW;
            shape[QStringLiteral("height")]    = defaultH;
            shape[QStringLiteral("longitude")] = lon;
            shape[QStringLiteral("latitude")]  = lat;
            shape[QStringLiteral("rotation")]  = 0.0;

            const QString sid = ShapeApi::instance()->addShape(shape);
            if (!sid.isEmpty())
                la->assignShapeToLayer(sid, layerId);

            // Active layer wapas restore karo — raster ko active na rehne do
            QString restoreLayerId = previousActiveLayerId;
            if (restoreLayerId.isEmpty() || !la->layerExists(restoreLayerId)) {
                // previous active layer invalid/empty tha (e.g. "Default Layer" kabhi
                // explicitly active nahi kiya gaya tha) — naam se dhoondo
                for (const QString& lid : la->getLayers()) {
                    if (la->getLayerName(lid).compare(QStringLiteral("Default Layer"), Qt::CaseInsensitive) == 0) {
                        restoreLayerId = lid;
                        break;
                    }
                }
            }
            if (!restoreLayerId.isEmpty() && restoreLayerId != layerId)
                la->setActiveLayer(restoreLayerId);

            if (ctx.map)
                ctx.map->update();
        },
        .isEnabled = layerPanelRootEnabled
    });
    registerAction({
        .id = "core.layer.import",
        .label = "Import Layer",
        .category = UiActionCategory::ContextLayer,
        .priority = 20,
        .callback = [](const UiActionContext& ctx) {
            registerBuiltInLayerFormats();
            QStringList allExts;
            QStringList parts;
            for (ILayerFormat* f : LayerFormatRegistry::instance()->allFormats()) {
                if (!f->canImport()) continue;
                parts << f->filter();
                for (const QString& e : f->extensions())
                    allExts << QStringLiteral("*.") + e;
            }
            if (parts.isEmpty()) {
                QMessageBox::warning(ctx.parentWidget, "Import", "No import formats registered.");
                return;
            }
            QString allFilter = QStringLiteral("All supported (%1)")
                                    .arg(allExts.join(QLatin1Char(' ')));
            QString filters = allFilter
                              + QStringLiteral(";;") + parts.join(QStringLiteral(";;"))
                              + QStringLiteral(";;All Files (*)");

            QString selectedFilter = allFilter;
            QString file = QFileDialog::getOpenFileName(
                ctx.parentWidget,
                QStringLiteral("Import Layer"),
                QString(),
                filters,
                &selectedFilter);
            if (file.isEmpty())
                return;
            QString fmtId;
            if (selectedFilter == allFilter
                || selectedFilter.startsWith(QStringLiteral("All "))) {
                if (ILayerFormat* f = LayerFormatRegistry::instance()->formatForPath(file))
                    fmtId = f->id();
            } else {
                fmtId = LayerFormatRegistry::instance()->idFromFilter(selectedFilter);
                if (fmtId.isEmpty()) {
                    if (ILayerFormat* f = LayerFormatRegistry::instance()->formatForPath(file))
                        fmtId = f->id();
                }
            }
            if (fmtId.isEmpty()) {
                QMessageBox::warning(
                    ctx.parentWidget, "Import",
                    QStringLiteral("Unknown file type: *.%1")
                        .arg(QFileInfo(file).suffix()));
                return;
            }
            if (!LayerApi::instance()->importLayer(file, fmtId)) {
                QMessageBox::warning(
                    ctx.parentWidget, QStringLiteral("Import"),
                    QStringLiteral("Failed to import layer.\n"
                                   "Layer may already exist or file is invalid."));
            }
        },
        .isEnabled = layerPanelRootEnabled
    });
    registerAction({
        .id = QStringLiteral("core.layer.add_elevation"),
        .label = QStringLiteral("Add Elevation Data"),
        .category = UiActionCategory::ContextLayer,
        .priority = 16,
        .callback = [](const UiActionContext& ctx) {
            QWidget* parent = ctx.parentWidget;
            const QString path = QFileDialog::getOpenFileName(
                parent,
                QStringLiteral("Select Elevation File"),
                QString(),
                QStringLiteral(
                    "All Elevation ("
                    "*.tif *.tiff *.hgt *.img *.dem *.vrt "
                    "*.dt0 *.dt1 *.dt2 *.dtd "
                    "*.asc *.bil *.bin *.adf "
                    "*.jpg *.jpeg *.png *.bmp);;"
                    "GeoTIFF DEM/DSM (*.tif *.tiff);;"
                    "DTED 0/1/2 (*.dt0 *.dt1 *.dt2 *.dtd);;"
                    "SRTM HGT (*.hgt);;"
                    "USGS / ESRI DEM (*.dem *.img *.adf *.bil);;"
                    "Images (approx) (*.jpg *.jpeg *.png *.bmp);;"
                    "All Files (*)"));
            if (path.isEmpty())
                return;

            if (!ElevationData::instance()->loadFile(path)) {
                QMessageBox::warning(
                    parent,
                    QStringLiteral("Elevation"),
                    QStringLiteral("Failed to load elevation file:\n%1").arg(path));
                return;
            }

            // Same style as Settings status
            const QSize sz = ElevationData::instance()->imageSize();
            QMessageBox::information(
                parent,
                QStringLiteral("Elevation"),
                QStringLiteral("Loaded: %1 × %2 px\n\n%3")
                    .arg(sz.width())
                    .arg(sz.height())
                    .arg(path));

            if (ctx.map)
                ctx.map->update();
        },
        .isEnabled = layerPanelRootEnabled
    });
    registerAction({
        .id = "core.layer.rename",
        .label = "Rename",
        .category = UiActionCategory::ContextLayer,
        .group = QStringLiteral("layer"),
        .priority = 30,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.layerId.isEmpty())
                return;
            QWidget* parent = ctx.parentWidget;
            QString oldName = LayerApi::instance()->getLayerName(ctx.layerId);
            bool ok = false;
            QString name = QInputDialog::getText(
                parent, QStringLiteral("Rename Layer"), QStringLiteral("New name:"),
                QLineEdit::Normal, oldName, &ok);
            if (ok && !name.trimmed().isEmpty())
                LayerApi::instance()->renameLayer(ctx.layerId, name.trimmed());
        },
        .isEnabled = layerRowEnabled
    });
    registerAction({
        .id = "core.layer.export",
        .label = "Export Layer",
        .category = UiActionCategory::ContextLayer,
        .group = "layer",
        .priority = 40,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.layerId.isEmpty()) return;
            registerBuiltInLayerFormats();
            QStringList labels;
            QStringList ids;
            for (ILayerFormat* f : LayerFormatRegistry::instance()->allFormats()) {
                if (!f->canExport()) continue;
                labels << f->filter();
                ids << f->id();
            }
            if (labels.isEmpty()) {
                QMessageBox::warning(ctx.parentWidget, "Export", "No export formats registered.");
                return;
            }
            bool ok = false;
            QString chosen = QInputDialog::getItem(
                ctx.parentWidget,
                QStringLiteral("Export Layer"),
                QStringLiteral("Select format:"),
                labels,
                0,
                false,
                &ok);
            if (!ok || chosen.isEmpty())
                return;
            const int idx = labels.indexOf(chosen);
            QString fmtId = (idx >= 0 && idx < ids.size()) ? ids[idx] : QStringLiteral("geojson");
            ILayerFormat* fmt = LayerFormatRegistry::instance()->format(fmtId);
            QString ext = (fmt && !fmt->extensions().isEmpty())
                              ? fmt->extensions().first()
                              : QStringLiteral("geojson");
            QString layerName = LayerApi::instance()->getLayerName(ctx.layerId);
            QString file = QFileDialog::getSaveFileName(
                ctx.parentWidget,
                QStringLiteral("Export Layer"),
                layerName + QStringLiteral(".") + ext,
                chosen + QStringLiteral(";;All Files (*)"));
            if (file.isEmpty())
                return;
            QFileInfo fi(file);
            if (fi.suffix().isEmpty())
                file += QStringLiteral(".") + ext;
            if (!LayerApi::instance()->exportLayer(ctx.layerId, fmtId, file)) {
                QMessageBox::warning(ctx.parentWidget, "Export",
                                     "Failed to export layer.\nFormat may need a plugin, or layer has no shapes.");
            } else {
                QMessageBox::information(ctx.parentWidget, "Export",
                                         QStringLiteral("Layer exported as %1").arg(chosen));
            }
        },
        .isEnabled = [](const UiActionContext& ctx) {
            if (ctx.layerId.isEmpty() || !ctx.shapeId.isEmpty())
                return false;
            const QStringList shapes = LayerApi::instance()->getShapesOfLayer(ctx.layerId);
            if (shapes.isEmpty())
                return false;   // khaali layer — export ka koi matlab nahi
            for (const QString& sid : shapes) {
                const QString t = ShapeApi::instance()->getShapeType(sid);
                if (t.compare(QLatin1String("bitmap"), Qt::CaseInsensitive) != 0
                    && t.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) != 0)
                    return true;
            }
            return false;
        }
    });
    registerAction({
        .id = "core.layer.delete",
        .label = "Delete",
        .category = UiActionCategory::ContextLayer,
        .group = QStringLiteral("layer"),
        .priority = 50,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.layerId.isEmpty())
                return;
            QWidget* parent = ctx.parentWidget;
            QString name = LayerApi::instance()->getLayerName(ctx.layerId);
            auto reply = QMessageBox::question(
                parent, QStringLiteral("Delete Layer"),
                QStringLiteral("Delete layer \"%1\" and all its shapes?").arg(name));
            if (reply == QMessageBox::Yes)
                LayerApi::instance()->deleteLayer(ctx.layerId);
        },
        .isEnabled = layerRowEnabled
    });
    registerAction({
        .id = "core.layer.shape.rename",
        .label = "Rename",
        .category = UiActionCategory::ContextLayer,
        .group = QStringLiteral("shape"),
        .priority = 60,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.shapeId.isEmpty())
                return;
            QWidget* parent = ctx.parentWidget;
            QJsonObject props = ShapeApi::instance()->getShapeProperties(ctx.shapeId);
            if (props.isEmpty())
                return;
            QString current = props.value(QStringLiteral("text")).toString();
            if (current.isEmpty())
                current = props.value(QStringLiteral("type")).toString();
            bool ok = false;
            QString name = QInputDialog::getText(
                parent, QStringLiteral("Rename Shape"), QStringLiteral("Name:"),
                QLineEdit::Normal, current, &ok);
            if (!ok || name.trimmed().isEmpty())
                return;
            QJsonObject update;
            update[QStringLiteral("text")] = name.trimmed();
            ShapeApi::instance()->updateShape(ctx.shapeId, update);
        },
        .isEnabled = shapeRowEnabled
    });
    registerAction({
        .id = "core.layer.shape.remove",
        .label = "Remove",
        .category = UiActionCategory::ContextLayer,
        .group = QStringLiteral("shape"),
        .priority = 70,
        .callback = [](const UiActionContext& ctx) {
            if (ctx.shapeId.isEmpty())
                return;
            QWidget* parent = ctx.parentWidget;
            QJsonObject props = ShapeApi::instance()->getShapeProperties(ctx.shapeId);
            QString label = props.value(QStringLiteral("text")).toString();
            if (label.isEmpty())
                label = props.value(QStringLiteral("type")).toString();
            auto reply = QMessageBox::question(
                parent, QStringLiteral("Remove Shape"),
                QStringLiteral("Remove shape \"%1\"?").arg(label));
            if (reply != QMessageBox::Yes)
                return;
            ShapeApi::instance()->deleteShape(ctx.shapeId);
        },
        .isEnabled = shapeRowEnabled
    });
}
// ============================================================
// Dynamic Map Layer actions (plugin tile sources)
// ============================================================

void UiActionApi::syncMapLayerActions()
{
    clearCategory(UiActionCategory::ToolbarMapLayer);

    MapApi* mapApi = MapApi::instance();
    if (!mapApi)
        return;

    const QVector<TileSourceInfo> sources = mapApi->allTileSources();
    int prio = 10;
    for (const TileSourceInfo& src : sources) {
        const QString actionId = QStringLiteral("maplayer.") + src.id;

        UiActionDescriptor d;
        d.id        = actionId;
        d.label     = src.displayName;
        d.iconPath  = src.iconPath;
        d.category  = UiActionCategory::ToolbarMapLayer;
        d.group     = QStringLiteral("basemap");
        d.checkable = true;
        d.priority  = src.priority > 0 ? src.priority : prio;
        d.callback  = [this, id = src.id](const UiActionContext& ctx) {
            MapApi::instance()->setActiveTileSource(id);
            if (ctx.map)
                ctx.map->reloadTiles();
            syncMapLayerActions();
        };
        d.isChecked = [id = src.id](const UiActionContext&) {
            return MapApi::instance()->activeTileSource() == id;
        };
        registerAction(d);
        prio += 10;
    }

    // ========== External XYZ Folder ==========
    {
        UiActionDescriptor d;
        d.id        = QStringLiteral("maplayer.external_xyz");
        d.label     = QStringLiteral("External Map (Folder)…");
        d.iconPath  = QStringLiteral(":/icons/images/map.png");
        d.category  = UiActionCategory::ToolbarMapLayer;
        d.group     = QStringLiteral("basemap");
        d.checkable = false;
        d.priority  = 900;
        d.callback  = [](const UiActionContext& ctx) {
            QString folder = QFileDialog::getExistingDirectory(
                ctx.parentWidget,
                QStringLiteral("Select XYZ Tiles Folder (z/x/y.png)"),
                QString(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (folder.isEmpty())
                return;

            QFileInfo fi(folder);
            QString id   = QStringLiteral("ext_xyz_") + fi.fileName().toLower().replace(' ', '_');
            QString name = fi.fileName();

            MapApi* api = MapApi::instance();
            if (api->tileSourceIds().contains(id))
                api->unregisterTileSource(id);

            api->registerTileSource(id, folder, name,
                                    QStringLiteral(":/icons/images/map.png"),
                                    500, 18);
            api->setActiveTileSource(id);
            if (ctx.map)
                ctx.map->reloadTiles();
            UiActionApi::instance()->syncMapLayerActions();
        };
        registerAction(d);
    }

    // ========== External Raster (PNG / TIFF / JPG) ==========
    {
        UiActionDescriptor d;
        d.id        = QStringLiteral("maplayer.external_raster");
        d.label     = QStringLiteral("External Raster (PNG/TIFF)…");
        d.iconPath  = QStringLiteral(":/icons/images/map.png");
        d.category  = UiActionCategory::ToolbarMapLayer;
        d.group     = QStringLiteral("basemap");
        d.checkable = false;
        d.priority  = 910;
        d.callback  = [](const UiActionContext& ctx) {
            QString path = QFileDialog::getOpenFileName(
                ctx.parentWidget,
                QStringLiteral("Select Raster Image (Basemap)"),
                QString(),
                QStringLiteral("Images (*.png *.jpg *.jpeg *.tif *.tiff *.bmp);;All Files (*)"));
            if (path.isEmpty())
                return;

            QFileInfo fi(path);
            QString id   = QStringLiteral("ext_img_") + fi.completeBaseName().toLower().replace(' ', '_');
            QString name = fi.completeBaseName();

            // Special marker: folderName starts with "image:"
            const QString imageKey = QStringLiteral("image:") + path;

            MapApi* api = MapApi::instance();
            if (api->tileSourceIds().contains(id))
                api->unregisterTileSource(id);

            api->registerTileSource(id, imageKey, name,
                                    QStringLiteral(":/icons/images/map.png"),
                                    510, 18);
            api->setActiveTileSource(id);
            if (ctx.map)
                ctx.map->reloadTiles();
            UiActionApi::instance()->syncMapLayerActions();
        };
        registerAction(d);
    }
}
