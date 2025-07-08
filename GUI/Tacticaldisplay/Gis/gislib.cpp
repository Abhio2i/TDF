
#include "gislib.h"
#include "gisnetwork.h"
#include "qnetworkreply.h"
#include "qregularexpression.h"
#include "qjsondocument.h"
#include "qjsonarray.h"
#include <QJsonObject>
#include <QtMath>

GISlib::GISlib(QWidget *parent) : QWidget(parent) {
    setStyleSheet("background-color: black;");
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setUpdatesEnabled(true);
    setMouseTracking(true);
    update();
    net = new GISNetwork(this);
    connect(net, &GISNetwork::receiveImage, this, &GISlib::receiveImage);
    connect(net, &GISNetwork::receivePlace, this, &GISlib::receivePlace);
    setLayers(QStringList() << "osm"); // Match first code's default layer
    setCenter(0, 0);
    setZoom(1);
}


void GISlib::addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity) {
    qDebug() << "Adding custom layer: name =" << layerName << ", zoomMin =" << zoomMin << ", zoomMax =" << zoomMax << ", tileUrl =" << tileUrl << ", opacity =" << opacity;
    if (layerName.isEmpty() || tileUrl.isEmpty()) {
        qWarning() << "Invalid layer name or URL: name=" << layerName << ", tileUrl=" << tileUrl;
        return;
    }
    CustomMap customMap;
    customMap.zoomMin = zoomMin;
    customMap.zoomMax = zoomMax;
    customMap.tileUrl = tileUrl;
    customMap.opacity = opacity; // Store opacity
    customMaps[layerName.toLower()] = customMap;
    qDebug() << "customMaps now contains:" << customMaps.keys();
    if (!activeLayers.contains(layerName.toLower())) {
        activeLayers.prepend(layerName.toLower());
        qDebug() << "Added" << layerName << "to activeLayers:" << activeLayers;
        update();
    }
}
void GISlib::receiveImage(QString url, QByteArray data) {
    QRegularExpression rx("/(\\d+)/(\\d+)/(\\d+)\\.png");
    QRegularExpressionMatch match = rx.match(url);

    if (!match.hasMatch()) {
        qDebug() << "Invalid tile URL format:" << url;
        return;
    }

    int z = match.captured(1).toInt();
    int x, y;
    QString layer;

    // Handle ArcGIS (World Imagery) URL format: {z}/{y}/{x}
    if (url.contains("arcgisonline.com")) {
        layer = "World Imagery";
        y = match.captured(2).toInt(); // ArcGIS uses y in second position
        x = match.captured(3).toInt(); // ArcGIS uses x in third position
    } else {
        // Standard {z}/{x}/{y} format
        x = match.captured(2).toInt();
        y = match.captured(3).toInt();
        // Determine layer
        for (auto it = customMaps.constBegin(); it != customMaps.constEnd(); ++it) {
            QString templateUrl = it.value().tileUrl;
            templateUrl.replace("{z}", QString::number(z));
            templateUrl.replace("{x}", QString::number(x));
            templateUrl.replace("{y}", QString::number(y));
            if (url == templateUrl) {
                layer = it.key();
                break;
            }
        }
        if (layer.isEmpty()) {
            if (url.contains("openstreetmap.org")) layer = "osm";
            else if (url.contains("opentopomap.org")) layer = "opentopo";
            else if (url.contains("cartocdn.com")) layer = url.contains("light_all") ? "carto-light" : "carto-dark";
            else {
                qDebug() << "Unknown layer for URL:" << url;
                return;
            }
        }
    }

    QString key = getTileKey(layer, z, x, y);
    pendingTileKeys.remove(key);
    int retries = tileRetries.value(key, 3);
    tileRetries.remove(key);

    if (data.isEmpty()) {
        qDebug() << "Empty data for tile" << key << ", retries left:" << retries - 1;
        if (retries > 0) {
            tileRetries[key] = retries - 1;
            pendingTileKeys.insert(key);
            requestTile(layer, x, y, z, retries - 1);
        } else {
            QPixmap pixmap(256, 256);
            pixmap.fill(Qt::gray);
            tileCache[key] = pixmap;
            pendingTiles = qMax(0, pendingTiles - 1);
            update();
        }
        return;
    }

    if (tileCache.size() >= maxCacheSize) {
        tileCache.remove(tileCache.keys().first());
    }
    QImage img;
    if (!img.loadFromData(data)) {
        qDebug() << "Failed to load image for tile" << key << ", retries left:" << retries - 1;
        if (retries > 0) {
            tileRetries[key] = retries - 1;
            pendingTileKeys.insert(key);
            requestTile(layer, x, y, z, retries - 1);
        } else {
            QPixmap pixmap(256, 256);
            pixmap.fill(Qt::gray);
            tileCache[key] = pixmap;
            pendingTiles = qMax(0, pendingTiles - 1);
            update();
        }
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(img);
    tileCache[key] = pixmap;

    pendingTiles = qMax(0, pendingTiles - 1);
    update();
}



void GISlib::receivePlace(QString url, QByteArray data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    if (!arr.isEmpty()) {
        QJsonObject place = arr.first().toObject();
        double lat = place["lat"].toString().toDouble();
        double lon = place["lon"].toString().toDouble();
        geoOutline = place["geojson"].toObject()["coordinates"].toArray().at(0).toArray();
        marker.lat = lat;
        marker.lon = lon;
        int rank = place["place_rank"].toInt();
        setCenter(lat, lon);
        setZoom(rank);
    }
}

void GISlib::serachPlace(const QString& query) {
    qDebug() << "Searching place:" << query;
    net->requestPlace(query);
}

void GISlib::setLayers(const QStringList& layerNames) {
    qDebug() << "Setting layers:" << layerNames;
    activeLayers = layerNames;
    tileCache.clear();
    pendingTiles = 0;
    update();
}

void GISlib::setCenter(double lat, double lon) {
    centerLat = lat;
    centerLon = lon;
    emit centerChanged(lat, lon);
    update();
}


void GISlib::setZoom(int level) {
    int maxZoom = 19;
    if (activeLayers.contains("opentopo")) maxZoom = 17;
    for (const QString& layer : activeLayers) {
        if (customMaps.contains(layer)) {
            maxZoom = qMin(maxZoom, customMaps[layer].zoomMax);
        }
    }
    zoom = qBound(1, level, maxZoom);
    tileCache.clear();
    pendingTiles = 0;
    emit zoomChanged(zoom);
    update();
}
double GISlib::lonToX(double lon, int zoom) {
    return ((lon + 180.0) / 360.0) * std::pow(2.0, zoom);
}

double GISlib::latToY(double lat, int zoom) {
    double rad = lat * M_PI / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI) / 2.0 * std::pow(2.0, zoom);
}

double GISlib::xToLon(double x, int zoom) {
    return x / std::pow(2.0, zoom) * 360.0 - 180.0;
}

double GISlib::yToLat(double y, int zoom) {
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, zoom);
    return (180.0 / M_PI) * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

QString GISlib::toDMS(double deg, bool isLat) {
    double absolute = std::abs(deg);
    int degrees = static_cast<int>(std::floor(absolute));
    double minutesNotTruncated = (absolute - degrees) * 60.0;
    int minutes = static_cast<int>(std::floor(minutesNotTruncated));
    int seconds = static_cast<int>(std::floor((minutesNotTruncated - minutes) * 60.0));

    QChar direction;
    if (isLat) {
        direction = (deg >= 0) ? 'N' : 'S';
    } else {
        direction = (deg >= 0) ? 'E' : 'W';
    }

    return QString("%1°%2'%3\"%4")
        .arg(degrees)
        .arg(minutes)
        .arg(seconds)
        .arg(direction);
}


void GISlib::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    bool tilesDrawn = false;
    int tileSize = 256;
    int tiles = qPow(2, zoom);
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);
    int tilesX = std::ceil(width() / tileSize) + 2;
    int tilesY = std::ceil(height() / tileSize) + 2;
    int startX = qFloor(centerX - tilesX / 2);
    int startY = qFloor(centerY - tilesY / 2);
    int fallbackCount = 0;
    const int maxFallbacks = 100;

    for (int i = activeLayers.size() - 1; i >= 0; --i) {
        QString layer = activeLayers[i];
        qreal opacity = 1.0; // Default opacity for non-custom layers
        if (customMaps.contains(layer)) {
            opacity = customMaps[layer].opacity; // Get opacity for custom layer
        }
        painter.setOpacity(opacity); // Set opacity for this layer
        for (int x = startX; x < startX + tilesX; ++x) {
            for (int y = startY; y < startY + tilesY; ++y) {
                int wrappedX = (x % tiles + tiles) % tiles;
                QString key = getTileKey(layer, zoom, wrappedX, y);
                if (tileCache.contains(key) && !tileCache[key].isNull()) {
                    int dx = (wrappedX - centerX) * tileSize + width() / 2;
                    int dy = (y - centerY) * tileSize + height() / 2;
                    painter.drawPixmap(dx, dy, tileCache[key]);
                    tilesDrawn = true;
                } else {
                    if (fallbackCount < maxFallbacks) {
                        fallbackCount++;
                        for (int dz = 1; dz <= zoom; ++dz) {
                            int lowerZoom = zoom - dz;
                            int lowerTiles = qPow(2, lowerZoom);
                            int lowerX = wrappedX >> dz;
                            int lowerY = y >> dz;
                            QString lowerKey = getTileKey(layer, lowerZoom, lowerX, lowerY);
                            if (tileCache.contains(lowerKey) && !tileCache[lowerKey].isNull()) {
                                int scale = 1 << dz;
                                int dx = (wrappedX - centerX) * tileSize + width() / 2;
                                int dy = (y - centerY) * tileSize + height() / 2;
                                QRect targetRect(dx, dy, tileSize, tileSize);
                                int srcX = (wrappedX % scale) * (tileSize / scale);
                                int srcY = (y % scale) * (tileSize / scale);
                                QRect srcRect(srcX, srcY, tileSize / scale, tileSize / scale);
                                painter.drawPixmap(targetRect, tileCache[lowerKey], srcRect);
                                tilesDrawn = true;
                                break;
                            }
                        }
                    }
                    requestTile(layer, wrappedX, y, zoom);
                }
            }
        }
        painter.setOpacity(1.0); // Reset opacity for next layer
    }

    if (!tilesDrawn) {
        painter.setPen(Qt::white);
        painter.drawText(rect().center(), QString("Loading tiles... (%1 pending)").arg(pendingTiles));
    }

    double markerX = lonToX(marker.lon, zoom);
    double markerY = latToY(marker.lat, zoom);
    double dx = (markerX - centerX) * tileSize + width() / 2;
    double dy = (markerY - centerY) * tileSize + height() / 2;
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.drawEllipse(QPoint(dx, dy), 5, 5);

    QPolygonF polygon;
    for (const QJsonValue& pointVal : geoOutline) {
        QJsonArray coord = pointVal.toArray();
        double lon = coord.at(0).toDouble();
        double lat = coord.at(1).toDouble();
        double px = (lonToX(lon, zoom) - centerX) * tileSize + width() / 2;
        double py = (latToY(lat, zoom) - centerY) * tileSize + height() / 2;
        polygon << QPointF(px, py);
    }
    painter.setBrush(QColor(255, 0, 0, 50));
    painter.setPen(QPen(Qt::red, 2));
    painter.drawPolygon(polygon);
}

QString GISlib::getTileKey(const QString& layer, int z, int x, int y) {
    bool flip = (layer == "World Imagery");
    return QString("%1/%2/%3/%4").arg(layer).arg(z).arg(flip ? y : x).arg(flip ? x : y);
}

QString GISlib::tileUrl(const QString& layer, int x, int y, int z) {
    qDebug() << "Generating tile URL for layer:" << layer << ", z:" << z << ", x:" << x << ", y:" << y;
    QString lowerLayer = layer.toLower();
    if (customMaps.contains(lowerLayer)) {
        qDebug() << "Found custom layer:" << lowerLayer << ", tileUrl =" << customMaps[lowerLayer].tileUrl;
        QString url = customMaps[lowerLayer].tileUrl;
        url.replace("{z}", QString::number(z));
        url.replace("{x}", QString::number(x));
        url.replace("{y}", QString::number(y));
        QString subdomain = getSubdomain(x, y, lowerLayer);
        if (url.contains("{s}")) {
            url.replace("{s}", subdomain.isEmpty() ? "a" : subdomain);
        }
        qDebug() << "Generated custom layer URL:" << url;
        flipkeyaxis = false;
        return url;
    }
    QString url;
    QString subdomain = getSubdomain(x, y, layer);
    bool flip = false;
    if (layer == "osm") {
        url = QString("http://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(subdomain).arg(z).arg(x).arg(y);
    } else if (layer == "opentopo") {
        url = QString("https://%1.tile.opentopomap.org/%2/%3/%4.png").arg(subdomain).arg(z).arg(x).arg(y);
    } else if (layer == "carto-light") {
        url = QString("https://%1.basemaps.cartocdn.com/light_all/%2/%3/%4.png").arg(subdomain).arg(z).arg(x).arg(y);
    } else if (layer == "carto-dark") {
        url = QString("https://%1.basemaps.cartocdn.com/dark_all/%2/%3/%4.png").arg(subdomain).arg(z).arg(x).arg(y);
    } else if (layer == "World Imagery") {
        url = QString("https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3.png").arg(z).arg(y).arg(x);
        flip = true;
    } else {
        qDebug() << "Unsupported layer:" << layer << ", customMaps keys:" << customMaps.keys();
    }
    flipkeyaxis = flip;
    qDebug() << "Generated tile URL:" << url;
    return url;
}

void GISlib::requestTile(const QString& layer, int x, int y, int z, int retries) {
    int tiles = qPow(2, z);
    x = (x % tiles + tiles) % tiles;
    if (y < 0 || y >= tiles) return;

    if (customMaps.contains(layer)) {
        if (z < customMaps[layer].zoomMin || z > customMaps[layer].zoomMax) return;
    }

    QString key = getTileKey(layer, z, x, y);
    if (tileCache.contains(key) && !tileCache[key].isNull()) return;
    if (pendingTileKeys.contains(key)) return;

    tileCache[key] = QPixmap();
    pendingTileKeys.insert(key);
    tileRetries[key] = retries;
    QString url = tileUrl(layer, x, y, z);
    if (url.isEmpty()) return;

    qDebug() << "Requesting tile for" << layer << ":" << url;
    pendingTiles++;
    net->requestImage(QUrl(url));
}



QString GISlib::getSubdomain(int x, int y, const QString& layer) {
    QStringList subdomains;
    if (layer == "osm" || layer == "opentopo" || layer == "carto-light" || layer == "carto-dark") {
        subdomains << "a" << "b" << "c";
    } else if (customMaps.contains(layer.toLower()) && customMaps[layer.toLower()].tileUrl.contains("{s}")) {
        subdomains << "a" << "b" << "c";
    }
    if (subdomains.isEmpty()) {
        return QString();
    }
    int index = (x + y) % subdomains.size();
    return subdomains[index];
}

void GISlib::mousePressEvent(QMouseEvent* event) {
    emit mousePressed(event);
    if (event->button() == Qt::MiddleButton) {
        dragging = true;
        lastMouse = event->pos();
    }
}

void GISlib::mouseReleaseEvent(QMouseEvent* event) {
    emit mouseReleased(event);
    if (event->button() == Qt::MiddleButton) {
        dragging = false;
    }
}

void GISlib::mouseMoveEvent(QMouseEvent* event) {
    emit mouseMoved(event);
    int tileSize = 256;
    if (dragging) {
        QPointF delta = event->pos() - lastMouse;
        double dx = delta.x();
        double dy = delta.y();
        double cx = lonToX(centerLon, zoom) - dx / tileSize;
        double cy = latToY(centerLat, zoom) - dy / tileSize;
        centerLon = xToLon(cx, zoom);
        centerLat = yToLat(cy, zoom);
        lastMouse = event->pos();
        emit centerChanged(centerLat, centerLon);
        update();
    }
    double mouseX = event->pos().x();
    double mouseY = event->pos().y();
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);
    double dx = (mouseX - width() / 2) / tileSize;
    double dy = (mouseY - height() / 2) / tileSize;
    double tileX = centerX + dx;
    double tileY = centerY + dy;
    double lon = xToLon(tileX, zoom);
    double lat = yToLat(tileY, zoom);
    mouseLat = toDMS(lat, true);
    mouseLon = toDMS(lon, false);
    emit mouseCords(mouseLat, mouseLon);
}

void GISlib::keyPressEvent(QKeyEvent* event) {
    emit keyPressed(event);
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        setZoom(zoom + 1);
    } else if (event->key() == Qt::Key_Minus) {
        setZoom(zoom - 1);
    }
}
