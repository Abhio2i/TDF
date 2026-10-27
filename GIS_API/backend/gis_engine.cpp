#include "gis_engine.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <cmath>

GISEngine* GISEngine::m_instance = nullptr;

GISEngine* GISEngine::instance() {
    if (!m_instance) {
        m_instance = new GISEngine();
    }
    return m_instance;
}

GISEngine::GISEngine(QObject* parent) 
    : QObject(parent)
    , m_centerLat(20.5937)
    , m_centerLon(78.9629)
    , m_zoom(4)
    , m_widgetSize(800, 600)
    , m_tileSize(256)
{
    QString appDir = QCoreApplication::applicationDirPath();
    m_tilesPath = QDir(appDir).filePath("testOSM");
    m_activeLayers << "osm";
}
GISEngine::~GISEngine() {}

void GISEngine::setCenter(double lat, double lon) {
    m_centerLat = lat;
    m_centerLon = lon;
    emit centerChanged(lat, lon);
    emit mapChanged();
}

void GISEngine::setZoom(int level) {
    m_zoom = qBound(1, level, 18);
    emit zoomChanged(m_zoom);
    emit mapChanged();
}

void GISEngine::zoomIn() {
    setZoom(m_zoom + 1);
}

void GISEngine::zoomOut() {
    setZoom(m_zoom - 1);
}

void GISEngine::setWidgetSize(const QSize& size) {
    m_widgetSize = size;
    emit mapChanged();
}

void GISEngine::setLayers(const QStringList& layers) {
    m_activeLayers = layers;
    m_tileCache.clear();
    emit mapChanged();
}

// ============================================
// Coordinate Conversion
// ============================================

double GISEngine::lonToX(double lon, int zoom) const {
    return ((lon + 180.0) / 360.0) * std::pow(2.0, zoom);
}

double GISEngine::latToY(double lat, int zoom) const {
    if (lat > 85.051128) lat = 85.051128;
    if (lat < -85.051128) lat = -85.051128;
    double rad = lat * M_PI / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI) / 2.0 * std::pow(2.0, zoom);
}

double GISEngine::xToLon(double x, int zoom) const {
    return x / std::pow(2.0, zoom) * 360.0 - 180.0;
}

double GISEngine::yToLat(double y, int zoom) const {
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, zoom);
    return (180.0 / M_PI) * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

QPointF GISEngine::geoToCanvas(double lat, double lon) const {
    const int tileSize = 256;
    double px = lonToX(lon, m_zoom);
    double py = latToY(lat, m_zoom);
    double cx = lonToX(m_centerLon, m_zoom);
    double cy = latToY(m_centerLat, m_zoom);
    double x = (px - cx) * tileSize + m_widgetSize.width() / 2.0;
    double y = (py - cy) * tileSize + m_widgetSize.height() / 2.0;
    return QPointF(x, y);
}

QPointF GISEngine::canvasToGeo(const QPointF& pos) const {
    const int tileSize = 256;
    double cx = lonToX(m_centerLon, m_zoom);
    double cy = latToY(m_centerLat, m_zoom);
    double tileX = cx + (pos.x() - m_widgetSize.width() / 2.0) / tileSize;
    double tileY = cy + (pos.y() - m_widgetSize.height() / 2.0) / tileSize;
    double lon = xToLon(tileX, m_zoom);
    double lat = yToLat(tileY, m_zoom);
    return QPointF(lon, lat);
}

// ============================================
// Tile Management
// ============================================

QString GISEngine::getTileKey(int z, int x, int y) const {
    return QString("osm/%1/%2/%3").arg(z).arg(x).arg(y);
}

void GISEngine::requestTile(int x, int y, int z) {
    // Like GISlib - check if tile exists
    QString path = QString("%1/%2/%3/%4.png").arg(m_tilesPath).arg(z).arg(x).arg(y);
    if (QFile::exists(path)) {
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            QString key = getTileKey(z, x, y);
            m_tileCache[key] = pixmap;
        }
    }
}

void GISEngine::loadOfflineTiles() {
    m_tileCache.clear();
    QDir dir(m_tilesPath);
    if (!dir.exists()) {
        emit tilesLoaded(0);
        return;
    }
    int tiles = 1 << m_zoom;
    double centerX = lonToX(m_centerLon, m_zoom);
    double centerY = latToY(m_centerLat, m_zoom);
    int tilesX = std::ceil(m_widgetSize.width() / m_tileSize) + 2;
    int tilesY = std::ceil(m_widgetSize.height() / m_tileSize) + 2;
    int startX = std::floor(centerX - tilesX / 2.0);
    int startY = std::floor(centerY - tilesY / 2.0);
    int loaded = 0;
    for (int x = startX; x < startX + tilesX; ++x) {
        for (int y = startY; y < startY + tilesY; ++y) {
            int wrappedX = (x % tiles + tiles) % tiles;
            QString path = QString("%1/%2/%3/%4.png").arg(m_tilesPath).arg(m_zoom).arg(wrappedX).arg(y);
            if (QFile::exists(path)) {
                QPixmap pixmap(path);
                if (!pixmap.isNull()) {
                    QString key = getTileKey(m_zoom, wrappedX, y);
                    m_tileCache[key] = pixmap;
                    loaded++;
                }
            }
        }
    }
    emit tilesLoaded(loaded);
}

QPixmap GISEngine::getTile(int zoom, int x, int y) const {
    QString key = getTileKey(zoom, x, y);
    return m_tileCache.value(key, QPixmap());
}

bool GISEngine::hasTile(int zoom, int x, int y) const {
    QString key = getTileKey(zoom, x, y);
    return m_tileCache.contains(key) && !m_tileCache[key].isNull();
}

void GISEngine::clearCache() {
    m_tileCache.clear();
}

int GISEngine::getTileCount() const {
    return m_tileCache.size();
}
