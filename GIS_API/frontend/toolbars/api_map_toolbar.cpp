#include "api_map_toolbar.h"
#include "frontend/widgets/map_widget.h"
#include "ui_action_api.h"
#include "backend/api/airbase_api.h"
#include "GUI/Toolbars/designtoolbar-styles.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>
#include <QActionGroup>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QLineEdit>
#include <QWidgetAction>
#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>
#include "backend/api/map_api.h"
#include <QSettings>   // ARTI CHANGE: last folder yaad rakhne ke liye
#include <QDir>

static const QSize ICON_SIZE(20, 20);
QPixmap ApiMapToolbar::withWhiteBg(const QString& iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) return QPixmap();
    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);
    QPainter p(&newPixmap);
    p.drawPixmap(0, 0, pixmap);
    p.end();
    return newPixmap;
}

ApiMapToolbar::ApiMapToolbar(MapWidget* map, QWidget* parent)
    : QToolBar(parent)
    , m_map(map)
{
    setWindowTitle("API GIS ToolBar");
    setStyleSheet(DesignToolbarStyles::Toolbar);
    setIconSize(ICON_SIZE);
    setMovable(false);
    if (m_map)
        UiActionApi::instance()->registerCoreActions(m_map);
    rebuildFromRegistry();
    connect(UiActionApi::instance(), &UiActionApi::registryChanged,
            this, &ApiMapToolbar::onRegistryChanged);
    if (m_map) {
        connect(m_map, &MapWidget::measuringChanged, this, [this](bool active) {
            if (m_measureAction)
                m_measureAction->setChecked(active);
        });
    }
    if (m_map) {
        connect(m_map, &MapWidget::gisInteractionEnabledChanged,
                this, [this](bool enabled) {
                    if (m_gisInteractionAction)
                        m_gisInteractionAction->setChecked(enabled);
                });
        setGisToolsEnabled(m_map->isGisInteractionEnabled());

        connect(m_map, &MapWidget::placingBitmapChanged,
                this, &ApiMapToolbar::onPlacingBitmapChanged);

        connect(m_map, &MapWidget::drawingTypeChanged,
                this, &ApiMapToolbar::onDrawingTypeChanged);
        connect(m_map, &MapWidget::coordSystemChanged,
                this, &ApiMapToolbar::onCoordSystemChanged);
    } else {
        setGisToolsEnabled(false);
    }
}

void ApiMapToolbar::onPlacingBitmapChanged(const QString& activeName)
{
    for (auto it = m_bitmapActions.constBegin(); it != m_bitmapActions.constEnd(); ++it)
        it.value()->setChecked(it.key() == activeName);
}

void ApiMapToolbar::onDrawingTypeChanged(const QString& activeType)
{
    for (auto it = m_shapeActions.constBegin(); it != m_shapeActions.constEnd(); ++it)
        it.value()->setChecked(it.key() == activeType);
}

void ApiMapToolbar::onRegistryChanged()
{
    rebuildFromRegistry();
}

QAction* ApiMapToolbar::makeActionFromDescriptor(const UiActionDescriptor& d,
                                                 const UiActionContext& ctx)
{
    QAction* a = new QAction(this);
    a->setText(d.label);
    a->setData(d.id);
    a->setCheckable(d.checkable);
    if (!d.iconPath.isEmpty())
        a->setIcon(QIcon(withWhiteBg(d.iconPath)));
    if (d.isChecked)
        a->setChecked(d.isChecked(ctx));
    if (d.isEnabled)
        a->setEnabled(d.isEnabled(ctx));

    if (d.id == QLatin1String("core.zoom_in")) {
        a->setShortcuts({
            QKeySequence(Qt::Key_Plus),
            QKeySequence(Qt::Key_Equal)
        });
        a->setShortcutContext(Qt::WindowShortcut);
    } else if (d.id == QLatin1String("core.zoom_out")) {
        a->setShortcuts({
            QKeySequence(Qt::Key_Minus),
            QKeySequence(Qt::Key_Underscore)
        });
        a->setShortcutContext(Qt::WindowShortcut);
    }
    connect(a, &QAction::triggered, this, [this, id = d.id, ctx](bool) {
        UiActionContext c = ctx;
        c.parentWidget = this;
        c.map = m_map;
        UiActionApi::instance()->trigger(id, c);
    });
    return a;
}

void ApiMapToolbar::addSearchPopup()
{
    QAction* searchAction = new QAction(
        QIcon(withWhiteBg(":/icons/images/search.png")), tr("Search Place"), this);
    searchAction->setCheckable(true);
    QMenu* searchMenu = new QMenu(this);
    searchMenu->setStyleSheet(DesignToolbarStyles::Menu);
    QWidgetAction* searchWa = new QWidgetAction(this);
    QLineEdit* searchInput = new QLineEdit();
    searchInput->setPlaceholderText("Enter coordinates (lat,lon)...");
    searchInput->setMinimumWidth(200);
    searchInput->setStyleSheet(DesignToolbarStyles::LineEdit);
    connect(searchInput, &QLineEdit::returnPressed, this, [this, searchInput]() {
        QString input = searchInput->text().trimmed();
        QStringList parts = input.split(",");
        if (parts.size() == 2) {
            bool latOk = false, lonOk = false;
            double lat = parts[0].trimmed().toDouble(&latOk);
            double lon = parts[1].trimmed().toDouble(&lonOk);
            if (latOk && lonOk && m_map)
                m_map->goToLocation(lat, lon);
        }
        searchInput->clear();
    });
    searchWa->setDefaultWidget(searchInput);
    searchMenu->addAction(searchWa);
    searchAction->setMenu(searchMenu);
    QToolButton* btn = new QToolButton(this);
    btn->setDefaultAction(searchAction);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    addWidget(btn);
}
QToolButton* ApiMapToolbar::addCategoryPopup(const QString& label,
                                             const QString& iconPath,
                                             UiActionCategory category,
                                             bool exclusive)
{
    QAction* parentAct = new QAction(
        QIcon(withWhiteBg(iconPath)), label, this);
    parentAct->setCheckable(true);
    QMenu* menu = new QMenu(this);
    const QString menuStyle = DesignToolbarStyles::Menu + QStringLiteral(R"(
        QMenu::item {
            padding: 8px 28px 8px 8px;
        }
        QMenu::indicator {
            width: 12px;
            height: 12px;
            margin-left: 8px;
            margin-right: 8px;
            subcontrol-position: left center;
            subcontrol-origin: padding;
            border: 2px solid #4A6A8A;
            border-radius: 4px;
            background-color: #0A1A2A;
        }
        QMenu::indicator:checked {
            image: url(:/icons/images/check.png);
            background-color: #4CD137;
        }
        QMenu::indicator:checked:hover {
            image: url(:/icons/images/check.png);
        }
        QMenu::icon {
            padding-left: 4px;
            padding-right: 4px;
        }
    )");
    menu->setStyleSheet(menuStyle);
    parentAct->setMenu(menu);

    QActionGroup* group = exclusive ? new QActionGroup(menu) : nullptr;
    if (group)
        group->setExclusive(false);

    UiActionContext popupCtx;
    popupCtx.map = m_map;
    popupCtx.parentWidget = this;

    for (const UiActionDescriptor& d :
         UiActionApi::instance()->actionsFor(category)) {

        // External actions yahan mat dikhao
        if (d.id == QLatin1String("maplayer.external_xyz") ||
            d.id == QLatin1String("maplayer.external_raster"))
            continue;

        QAction* a = menu->addAction(d.label);
        a->setData(d.id);
        a->setCheckable(d.checkable);
        if (d.isChecked)
            a->setChecked(d.isChecked(popupCtx));
        if (category == UiActionCategory::ToolbarBitmap)
            m_bitmapActions[d.label] = a;
        if (category == UiActionCategory::ToolbarShape)
            m_shapeActions[d.id.section('.', -1)] = a;
        if (category == UiActionCategory::ToolbarCoord)
            m_coordActions[d.id] = a;
        if (group)
            group->addAction(a);
        connect(a, &QAction::triggered, this, [this, id = d.id](bool) {
            UiActionContext c;
            c.parentWidget = this;
            c.map = m_map;
            if (!c.map) {
                qWarning() << "ApiMapToolbar: map null for action" << id;
                return;
            }
            UiActionApi::instance()->trigger(id, c);
        });
    }

    // ----- External Map submenu (sirf Map Layer) -----
    // ARTI CHANGE: 4 entries (XYZ/Raster/MBTiles/GDAL) -> 2 entries (Open Map File / Open XYZ Folder).
    // Extension se khud decide hota hai: .mbtiles -> "mbtiles:", baaki sab -> "gdal:".
    if (category == UiActionCategory::ToolbarMapLayer) {
        menu->addSeparator();
        QMenu* extMenu = menu->addMenu(tr("External Map"));
        extMenu->setToolTipsVisible(true);
        extMenu->setStyleSheet(menuStyle);

        // ARTI CHANGE: common function - register + activate + reload
        auto activateSource = [this](const QString& id, const QString& name,
                                     const QString& key, int prio) {
            MapApi* api = MapApi::instance();
            if (api->tileSourceIds().contains(id))
                api->unregisterTileSource(id);
            api->registerTileSource(id, key, name,
                                    QStringLiteral(":/icons/images/map.png"), prio, 18);
            api->setActiveTileSource(id);
            if (m_map)
                m_map->reloadTiles();
            UiActionApi::instance()->syncMapLayerActions();
        };

        // ----- Open Map File -----
  QAction* fileAct = extMenu->addAction(tr("Open Map File…"));
        fileAct->setToolTip(tr(
            "Georeferenced imagery: GeoTIFF, JPEG2000, JPEG/PNG + world file, ERDAS .img\n"
            "Defence: NITF, CADRG/ADRG, GeoPDF\n"
            "Tiles: MBTiles\n"
            "Virtual: VRT"));
        connect(fileAct, &QAction::triggered, this, [this, activateSource]() {
            const QString startDir = QSettings().value(
                                                    QStringLiteral("externalMaps/lastDir"), QDir::homePath()).toString();
            const QString path = QFileDialog::getOpenFileName(
                this, tr("Open Map File"), startDir,
                tr("All supported maps (*.tif *.tiff *.jp2 *.jpg *.jpeg *.png *.bmp *.gif "
                   "*.img *.ntf *.gen *.pdf *.vrt *.mbtiles);;"
                   "Georeferenced imagery (*.tif *.tiff *.jp2 *.jpg *.jpeg *.png *.bmp *.gif *.img);;"
                   "Defence (*.ntf *.gen *.pdf);;"
                   "Tile-based (*.mbtiles);;"
                   "Virtual (*.vrt);;"
                   "All files (*)"));
            if (path.isEmpty())
                return;

            const QFileInfo fi(path);
            if (!fi.isFile()) {
                QMessageBox::warning(this, tr("External Map"),
                                     tr("File not found:\n%1").arg(path));
                return;
            }
            QSettings().setValue(QStringLiteral("externalMaps/lastDir"), fi.absolutePath());

            const bool isMbt = fi.suffix().compare(QLatin1String("mbtiles"),
                                                   Qt::CaseInsensitive) == 0;
            // fileName() (extension ke saath) taaki test.jpg aur test.png ka id na takraye
            const QString id = (isMbt ? QStringLiteral("ext_mbt_") : QStringLiteral("ext_gdal_"))
                               + fi.fileName().toLower().replace(QLatin1Char(' '), QLatin1Char('_'));
            const QString key = (isMbt ? QStringLiteral("mbtiles:") : QStringLiteral("gdal:")) + path;
            activateSource(id, fi.completeBaseName(), key, isMbt ? 520 : 530);
        });

        // ----- Open XYZ Folder -----
      QAction* xyzAct = extMenu->addAction(tr("Open XYZ Tiles Folder…"));
        xyzAct->setToolTip(tr("Folder with tiles in z/x/y.png layout"));
        connect(xyzAct, &QAction::triggered, this, [this, activateSource]() {
            const QString folder = QFileDialog::getExistingDirectory(
                this, tr("Select XYZ Tiles Folder (z/x/y.png)"), QString(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (folder.isEmpty())
                return;
            const QFileInfo fi(folder);
            const QString id = QStringLiteral("ext_xyz_") +
                               fi.fileName().toLower().replace(QLatin1Char(' '), QLatin1Char('_'));
            activateSource(id, fi.fileName(), folder, 500);
        });
    }

    QToolButton* btn = new QToolButton(this);
    btn->setDefaultAction(parentAct);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    addWidget(btn);
    return btn;
}
void ApiMapToolbar::rebuildFromRegistry()
{
    clear();
    m_measureAction = nullptr;
    m_bitmapActions.clear();
    m_shapeActions.clear();
     m_coordActions.clear();

    if (!m_gisInteractionAction) {
        for (const UiActionDescriptor& d :
             UiActionApi::instance()->actionsFor(UiActionCategory::ToolbarMain)) {
            if (d.id != QLatin1String("core.gis_interaction"))
                continue;
            UiActionContext ctx;
            ctx.map = m_map;
            ctx.parentWidget = this;
            m_gisInteractionAction = makeActionFromDescriptor(d, ctx);
            break;
        }
    }

    UiActionContext ctx;
    ctx.map = m_map;
    ctx.parentWidget = this;

    for (const UiActionDescriptor& d :
         UiActionApi::instance()->actionsFor(UiActionCategory::ToolbarMain)) {
        if (d.id == QLatin1String("core.gis_interaction"))
            continue;

        QAction* a = makeActionFromDescriptor(d, ctx);
        addAction(a);

        if (d.id == QLatin1String("core.measure"))
            m_measureAction = a;

        if (QWidget* w = widgetForAction(a))
            w->setStyleSheet(DesignToolbarStyles::ToolbarButton);
    }

    addSearchPopup();
    addCategoryPopup(tr("Coordinate System"),
                     ":/icons/images/coordinate-system.png",
                     UiActionCategory::ToolbarCoord, true);
    addCategoryPopup(tr("Map Layer"),
                     ":/icons/images/map.png",
                     UiActionCategory::ToolbarMapLayer, true);
    addCategoryPopup(tr("Shape"),
                     ":/icons/images/shapes.png",
                     UiActionCategory::ToolbarShape, true);
    addCategoryPopup(tr("Bitmaps"),
                     ":/icons/images/photo.png",
                     UiActionCategory::ToolbarBitmap, true);
    addCategoryPopup(tr("Preset Layers"),
                     ":/icons/images/preset.png",
                     UiActionCategory::ToolbarPreset, false);

}
void ApiMapToolbar::setGisInteractionChecked(bool checked)
{
    if (m_gisInteractionAction)
        m_gisInteractionAction->setChecked(checked);
}
void ApiMapToolbar::setGisToolsEnabled(bool enabled)
{
    m_gisToolsEnabled = enabled;
    applyGisToolsEnabled();
}
void ApiMapToolbar::applyGisToolsEnabled()
{
    Q_UNUSED(m_gisToolsEnabled);
    setVisible(true);
}
void ApiMapToolbar::onCoordSystemChanged(const QString&)
{
    UiActionContext ctx;
    ctx.map = m_map;
    for (auto it = m_coordActions.constBegin(); it != m_coordActions.constEnd(); ++it) {
        const UiActionDescriptor d = UiActionApi::instance()->action(it.key());
        it.value()->setChecked(d.isChecked && d.isChecked(ctx));
    }
}
