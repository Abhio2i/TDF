
#include "map_api.h"
#include "backend/engine/gis_engine.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <algorithm>

MapApi* MapApi::m_instance = nullptr;

MapApi* MapApi::instance()
{
    if (!m_instance)
        m_instance = new MapApi();
    return m_instance;
}

MapApi::MapApi(QObject* parent)
    : QObject(parent)
{
    m_engine = GISEngine::instance();
    connect(m_engine, &GISEngine::mapChanged, this, &MapApi::mapChanged);
    connect(m_engine, &GISEngine::zoomChanged, this, &MapApi::zoomChanged);
    connect(m_engine, &GISEngine::centerChanged, this, &MapApi::centerChanged);
    connect(m_engine, &GISEngine::tilesLoaded, this, &MapApi::tilesLoaded);
    registerTileSource(QStringLiteral("osm"),
                       QStringLiteral("testOSM"),
                       QStringLiteral("OpenStreetMap"),
                       QStringLiteral(":/icons/images/map.png"),
                       10,
                       8);
    registerTileSource(QStringLiteral("satellite"),
                       QStringLiteral("satMAP"),
                       QStringLiteral("Satellite Map"),
                       QStringLiteral(":/icons/images/map.png"),
                       20,
                       8);
    registerTileSource(QStringLiteral("terrain"),
                       QStringLiteral("tarMAP"),
                       QStringLiteral("Terrain Map"),
                       QStringLiteral(":/icons/images/map.png"),
                       30,
                       8);
    m_activeTileSource = QStringLiteral("osm");
}

MapApi::~MapApi() {}
void MapApi::setCenter(double lat, double lon)
{
    m_engine->setCenter(lat, lon);
}

void MapApi::setZoom(int level)
{
    m_engine->setZoom(level);
}

void MapApi::zoomIn()
{
    m_engine->zoomIn();
}

void MapApi::zoomOut()
{
    m_engine->zoomOut();
}

void MapApi::setMapSize(int width, int height)
{
    m_engine->setWidgetSize(QSize(width, height));
}

void MapApi::setLayers(const QStringList& layers)
{
    m_engine->setLayers(layers);
}

QPointF MapApi::geoToCanvas(double lat, double lon) const
{
    return m_engine->geoToCanvas(lat, lon);
}

QPointF MapApi::canvasToGeo(const QPointF& pos) const
{
    return m_engine->canvasToGeo(pos);
}

double MapApi::lonToX(double lon, int zoom) const
{
    return m_engine->lonToX(lon, zoom);
}

double MapApi::latToY(double lat, int zoom) const
{
    return m_engine->latToY(lat, zoom);
}

double MapApi::xToLon(double x, int zoom) const
{
    return m_engine->xToLon(x, zoom);
}

double MapApi::yToLat(double y, int zoom) const
{
    return m_engine->yToLat(y, zoom);
}

void MapApi::loadOfflineTiles()
{
    m_engine->loadOfflineTiles();
}

QPixmap MapApi::getTile(int zoom, int x, int y) const
{
    return m_engine->getTile(zoom, x, y);
}

bool MapApi::hasTile(int zoom, int x, int y) const
{
    return m_engine->hasTile(zoom, x, y);
}

void MapApi::clearCache()
{
    m_engine->clearCache();
}

int MapApi::getTileCount() const
{
    return m_engine->getTileCount();
}

double MapApi::getCenterLat() const
{
    return m_engine->getCenterLat();
}

double MapApi::getCenterLon() const
{
    return m_engine->getCenterLon();
}

int MapApi::getZoom() const
{
    return m_engine->getZoom();
}

QSize MapApi::getMapSize() const
{
    return m_engine->getMapSize();
}

// ============================================================
// Tile sources (plugin API)
// ============================================================

void MapApi::registerTileSource(const QString& id,
                                const QString& folderName,
                                const QString& displayName,
                                const QString& iconPath,
                                int priority,
                                int maxNativeZoom)
{
    if (id.isEmpty() || folderName.isEmpty()) {
        qWarning() << "MapApi::registerTileSource: empty id or folderName";
        return;
    }

    const bool isNew = !m_tileSources.contains(id);

    TileSourceInfo info;
    info.id             = id;
    info.folderName     = folderName;
    info.displayName    = displayName.isEmpty() ? id : displayName;
    info.iconPath       = iconPath;
    info.priority       = priority;
    info.maxNativeZoom  = qBound(1, maxNativeZoom, 22);

    m_tileSources[id] = info;

    if (isNew)
        emit tileSourceRegistered(id);
    emit sourcesChanged();
}

bool MapApi::unregisterTileSource(const QString& id)
{
    if (!m_tileSources.contains(id))
        return false;

    m_tileSources.remove(id);

    if (m_activeTileSource == id) {
        if (!m_tileSources.isEmpty()) {
            m_activeTileSource = allTileSources().first().id;
            clearCache();
            emit tileSourceChanged(m_activeTileSource);
            emit mapChanged();
        } else {
            m_activeTileSource.clear();
        }
    }
    emit tileSourceUnregistered(id);
    emit sourcesChanged();
    return true;
}

bool MapApi::setActiveTileSource(const QString& id)
{
    if (!m_tileSources.contains(id)) {
        qWarning() << "MapApi: unknown tile source" << id;
        return false;
    }
    if (m_activeTileSource == id)
        return true;

    m_activeTileSource = id;
    clearCache();
    emit tileSourceChanged(id);
    emit mapChanged();
    return true;
}

QString MapApi::activeTileSource() const
{
    return m_activeTileSource;
}

QString MapApi::activeTilesPath() const
{
    const TileSourceInfo info = m_tileSources.value(m_activeTileSource);
    if (info.folderName.isEmpty())
        return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("testOSM"));

    // image: prefix → as-is return (MapWidget handle karega)
    if (info.folderName.startsWith(QStringLiteral("image:")))
        return info.folderName;

    if (QFileInfo(info.folderName).isAbsolute())
        return info.folderName;

    return QDir(QCoreApplication::applicationDirPath()).filePath(info.folderName);
}

QStringList MapApi::tileSourceIds() const
{
    return m_tileSources.keys();
}

TileSourceInfo MapApi::tileSourceInfo(const QString& id) const
{
    return m_tileSources.value(id);
}

QVector<TileSourceInfo> MapApi::allTileSources() const
{
    QVector<TileSourceInfo> list;
    list.reserve(m_tileSources.size());
    for (const TileSourceInfo& info : m_tileSources)
    list.append(info);
    std::sort(list.begin(), list.end(),
              [](const TileSourceInfo& a, const TileSourceInfo& b) {
                  if (a.priority != b.priority)
                      return a.priority < b.priority;
                  return a.displayName < b.displayName;
              });
    return list;
}
