

// #include "gislib.h"
// #include "gisnetwork.h"
// #include "qnetworkreply.h"
// #include "qregularexpression.h"
// #include "qjsondocument.h"
// #include "qjsonarray.h"
// #include <QJsonObject>
// #include <QtMath>

// GISlib::GISlib(QWidget *parent) : QWidget(parent) {
//     setStyleSheet("background-color: black;");
//     setFocusPolicy(Qt::StrongFocus);
//     //setAttribute(Qt::WA_OpaquePaintEvent);
//     setAttribute(Qt::WA_NoSystemBackground);
//     setAutoFillBackground(false);
//     setUpdatesEnabled(true);
//     setMouseTracking(true);
//     update();
//     net = new GISNetwork(this);
//     connect(net, &GISNetwork::receiveImage, this, &GISlib::receiveImage);
//     connect(net, &GISNetwork::receivePlace, this, &GISlib::receivePlace);
//     net->requestImage(QUrl("https://tile.openstreetmap.org/3/4/3.png"));

//     setLayer("osm");
//     setCenter(0,0);
//     setZoom(1);
//     //serachPlace("india");
// }

// void GISlib::receiveImage(QString url,QByteArray data) {
//     QRegularExpression rx("/(\\d+)/(\\d+)/(\\d+)\\.png$");
//     QRegularExpressionMatch match = rx.match(url);

//     if (!match.hasMatch()) {
//         qDebug() << "Invalid tile URL format:" << url;
//         return;
//     }

//     QString key = match.captured(1) + "/" + match.captured(2) + "/" + match.captured(3);

//     if (tileCache.size() >= maxCacheSize) {
//         qDebug()<<"size  "<<tileCache.size();
//         tileCache.remove(tileCache.keys().first()); // Evict oldest
//     }
//     QImage img;
//     if (!img.loadFromData(data)) {
//         qDebug() << "Failed to load image for tile" << key;
//         qDebug() << "Tile URL was:" << url;
//         qDebug() << "Data size:" << data.size();
//         qDebug() << "Data (partial):" << QString(data.left(100));
//         img = QImage(256, 256, QImage::Format_RGB32);
//         img.fill(Qt::gray);
//     }
//     tileCache[key] = img;

//     pendingTiles = qMax(0, pendingTiles - 1);
//     if (networkImage.loadFromData(data)) {
//         qDebug()<<"chache:"<<tileCache.size();
//         qDebug() << "Image loaded successfully from network!";
//         update(); // trigger repaint
//     } else {
//         qDebug() << "Failed to load image from data!";
//     }
// }

// void GISlib::receivePlace(QString url,QByteArray data) {
//     QJsonDocument doc = QJsonDocument::fromJson(data);
//     if (!doc.isArray()) return;

//     QJsonArray arr = doc.array();
//     if (!arr.isEmpty()) {
//         QJsonObject place = arr.first().toObject();
//         double lat = place["lat"].toString().toDouble();
//         double lon = place["lon"].toString().toDouble();
//         geoOutline = place["geojson"].toObject()["coordinates"].toArray().at(0).toArray();
//         marker.lat = lat;
//         marker.lon = lon;
//         int rank = place["place_rank"].toInt();
//         setCenter(lat,lon);
//         setZoom(rank);
//         qDebug() << "Lat:" << lat << "Lon:" << lon << "Zoom-ish:" << rank;
//         //qDebug() << "cordinate:" << geoOutline;

//         // TODO: update map center, zoom, draw polygon etc.
//     }
// }

// void GISlib::serachPlace(const QString& query){
//     net->requestPlace(query);
// }

// void GISlib::setLayer(const QString& layerName) {
//     currentLayer = layerName;
//     tileCache.clear();
//     pendingTiles = 0;
//     update();
// }

// void GISlib::setCenter(double lat, double lon) {
//     centerLat = lat;
//     centerLon = lon;
//     emit centerChanged(lat, lon);
//     update();
// }

// void GISlib::setZoom(int level) {
//     int maxZoom = 19;// (currentLayer == "opentopo") ? 17 : 19;
//     if(level>=maxZoom)return;
//     zoom = qBound(1, level, maxZoom);
//     if(zoom<=maxZoom && zoom >0){
//         emit zoomChanged(zoom);
//         qDebug()<<"chache:"<<tileCache.size();
//         //tileCache.clear();
//         pendingTiles = 0;
//         //emit zoomChanged(zoom);
//         update();
//     }
// }

// double GISlib::lonToX(double lon, int zoom) {
//     return ((lon + 180.0) / 360.0) * std::pow(2.0, zoom);
// }

// double GISlib::latToY(double lat, int zoom) {
//     double rad = lat * M_PI / 180.0;
//     return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI) / 2.0 * std::pow(2.0, zoom);
// }

// double GISlib::xToLon(double x, int zoom) {
//     return x / std::pow(2.0, zoom) * 360.0 - 180.0;
// }

// double GISlib::yToLat(double y, int zoom) {
//     double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, zoom);
//     return (180.0 / M_PI) * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
// }

// QString GISlib::toDMS(double deg, bool isLat) {
//     double absolute = std::abs(deg);
//     int degrees = static_cast<int>(std::floor(absolute));
//     double minutesNotTruncated = (absolute - degrees) * 60.0;
//     int minutes = static_cast<int>(std::floor(minutesNotTruncated));
//     int seconds = static_cast<int>(std::floor((minutesNotTruncated - minutes) * 60.0));

//     QChar direction;
//     if (isLat) {
//         direction = (deg >= 0) ? 'N' : 'S';
//     } else {
//         direction = (deg >= 0) ? 'E' : 'W';
//     }

//     return QString("%1°%2'%3\"%4")
//         .arg(degrees)
//         .arg(minutes)
//         .arg(seconds)
//         .arg(direction);
// }


// void GISlib::paintEvent(QPaintEvent* event) {
//     emit painted(event); // Emit signal
//     QPainter painter(this);

//     int tiles = qPow(2, zoom);
//     double centerX = lonToX(centerLon, zoom);
//     double centerY = latToY(centerLat, zoom);

//     int tileSize = 256;
//     int tilesX = std::ceil(width() / tileSize) + 2;
//     int tilesY = std::ceil(height() / tileSize) + 2;

//     int startX = qFloor(centerX - tilesX / 2);
//     int startY = qFloor(centerY - tilesY / 2);

//     bool tilesDrawn = false;
//     for (int x = startX; x < startX + tilesX; x++) {
//         for (int y = startY; y < startY + tilesY; ++y) {
//             QString key = getTileKey(zoom, x, y);
//             if (tileCache.contains(key)) {
//                 int dx = (x - centerX) * tileSize + width() / 2;
//                 int dy = (y - centerY) * tileSize + height() / 2;
//                 painter.drawImage(dx, dy, tileCache[key]);
//                 tilesDrawn = true;
//             } else {
//                 requestTile(y, x, zoom, 1);
//             }
//         }
//     }

//     if (true) {
//         double markerX = lonToX(marker.lon, zoom);
//         double markerY = latToY(marker.lat, zoom);

//         double dx = (markerX - centerX) * tileSize + width() / 2;
//         double dy = (markerY - centerY) * tileSize + height() / 2;
//         painter.setBrush(Qt::red);
//         painter.setPen(Qt::white);
//         painter.drawEllipse(QPoint(dx, dy), 5, 5);
//     }
//     if (true) {
//         QPolygonF polygon;
//         double centerX = lonToX(centerLon, zoom);
//         double centerY = latToY(centerLat, zoom);

//         for (const QJsonValue& pointVal : geoOutline) {
//             QJsonArray coord = pointVal.toArray();
//             double lon = coord.at(0).toDouble();
//             double lat = coord.at(1).toDouble();

//             double px = (lonToX(lon, zoom) - centerX) * tileSize + width() / 2;
//             double py = (latToY(lat, zoom) - centerY) * tileSize + height() / 2;

//             polygon << QPointF(px, py);
//         }

//         painter.setBrush(QColor(255, 0, 0, 50));
//         painter.setPen(QPen(Qt::red, 2));
//         painter.drawPolygon(polygon);
//     }
// }
// QString GISlib::getTileKey(int z, int x, int y) {
//     if(flipkeyaxis){
//         return QString("%1/%2/%3").arg(zoom).arg(y).arg(x);
//     }
//     return QString("%1/%2/%3").arg(zoom).arg(x).arg(y);
// }

// QString GISlib::tileUrl(int x, int y, int z) {
//     QString url;
//     if (currentLayer == "osm") {
//         url = QString("http://tile.openstreetmap.org/%1/%2/%3.png").arg(z).arg(y).arg(x);
//         flipkeyaxis = false;
//     } else if (currentLayer == "arcgis") {
//         url = QString("https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3.png").arg(z).arg(y).arg(x);
//         flipkeyaxis = true;
//     } else if (currentLayer == "opentopo") {
//         url = QString("https://a.tile.opentopomap.org/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = false;
//     } else if (currentLayer == "carto-light") {
//         url = QString("https://a.basemaps.cartocdn.com/light_all/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = false;
//     } else if (currentLayer == "carto-dark") {
//         url = QString("https://a.basemaps.cartocdn.com/dark_all/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = false;
//     }else if (currentLayer == "Esri Street Map") {
//         url = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = true;
//     }else if (currentLayer == "Esri Topographic Map") {
//         url = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = true;
//     }else if (currentLayer == "Wikimedia Maps") {
//         url = QString("https://maps.wikimedia.org/osm-intl/%1/%2/%3.png").arg(z).arg(x).arg(y);
//         flipkeyaxis = false;
//     } else {
//         qDebug() << "Unsupported layer:" << currentLayer;
//     }
//     return url;
// }

// void GISlib::requestTile(int x, int y, int z, int retries) {
//     int tiles = qPow(2, z);
//     x = (x % tiles + tiles) % tiles; // Wrap x coordinate
//     if (y < 0 || y >= tiles) {
//         //qDebug() << "Invalid tile coordinates: x =" << x << ", y =" << y << ", zoom =" << z << ", tiles ="<<tiles;
//         return;
//     }

//     QString key = QString("%1/%2/%3").arg(z).arg(y).arg(x);
//     if (tileCache.contains(key)) return;
//     tileCache[key] = QImage();
//     QString url = tileUrl(x,y,z);//QString("http://tile.openstreetmap.org/%1/%2/%3.png").arg(z).arg(y).arg(x);
//     if (url.isEmpty()) return;
//     net->requestImage(QUrl(url));
// }


// void GISlib::mousePressEvent(QMouseEvent* event) {
//     emit mousePressed(event); // Emit signal
//     if (event->button() == Qt::MiddleButton) {
//         dragging = true;
//         lastMouse = event->pos();
//     }
// }
// void GISlib::mouseReleaseEvent(QMouseEvent* event) {
//     emit mouseReleased(event); // Emit signal
//     if (event->button() == Qt::MiddleButton) {
//         dragging = false;
//     }
// }

// void GISlib::mouseMoveEvent(QMouseEvent* event) {
//     emit mouseMoved(event); // Emit signal
//     int tileSize = 256;
//     if (dragging) {
//         QPointF delta = event->pos() - lastMouse;
//         double dx = delta.x();
//         double dy = delta.y();

//         double cx = lonToX(centerLon, zoom) - dx/tileSize;
//         double cy = latToY(centerLat, zoom) - dy/tileSize;

//         centerLon = xToLon(cx, zoom);
//         centerLat = yToLat(cy, zoom);
//         lastMouse = event->pos();
//         emit centerChanged(centerLat, centerLon);
//         update();
//     }
//     double mouseX = event->pos().x();
//     double mouseY = event->pos().y();

//     double centerX = lonToX(centerLon, zoom);
//     double centerY = latToY(centerLat, zoom);

//     double dx = (mouseX - width() / 2) / tileSize;
//     double dy = (mouseY - height() / 2) / tileSize;

//     double tileX = centerX + dx;
//     double tileY = centerY + dy;

//     double lon = xToLon(tileX, zoom);
//     double lat = yToLat(tileY, zoom);

//     QString mouseLat = toDMS(lat, true);
//     QString mouseLon = toDMS(lon, false);
//     qDebug() << "Lat:" << mouseLat << "Lon:" << mouseLon << "Zoom-ish:" << zoom;
//     qDebug() << "CentorLat:" << centerLat << "CentorLon:" << centerLon << "Zoom-ish:" << zoom;
//     emit mouseCords(mouseLat, mouseLon);
// }

// void GISlib::keyPressEvent(QKeyEvent* event) {
//     emit keyPressed(event); // Emit signal
//     if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
//         setZoom(zoom + 1); // Zoom in
//     } else if (event->key() == Qt::Key_Minus) {
//         setZoom(zoom - 1); // Zoom out
//     }
// }




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
    setLayers(QStringList() << "osm"); // Initialize with default layer
    setCenter(0, 0);
    setZoom(1);
}

void GISlib::addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl) {
    CustomMap customMap;
    customMap.zoomMin = zoomMin;
    customMap.zoomMax = zoomMax;
    customMap.tileUrl = tileUrl;
    customMaps[layerName] = customMap;
    // Optionally add to active layers
    if (!activeLayers.contains(layerName)) {
        activeLayers.prepend(layerName);
        update();
    }
}

void GISlib::receiveImage(QString url, QByteArray data) {
    QRegularExpression rx("/(\\d+)/(\\d+)/(\\d+)\\.png$");
    QRegularExpressionMatch match = rx.match(url);

    if (!match.hasMatch()) {
        qDebug() << "Invalid tile URL format:" << url;
        return;
    }

    int z = match.captured(1).toInt();
    int x = match.captured(2).toInt();
    int y = match.captured(3).toInt();

    QString layer;
    // Check custom maps first
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

    // Fallback to predefined layers
    if (layer.isEmpty()) {
        if (url.contains("openstreetmap.org")) layer = "osm";
        else if (url.contains("arcgisonline.com")) layer = url.contains("World_Street_Map") ? "Esri Street Map" : url.contains("World_Topo_Map") ? "Esri Topographic Map" : "arcgis";
        else if (url.contains("opentopomap.org")) layer = "opentopo";
        else if (url.contains("cartocdn.com")) layer = url.contains("light_all") ? "carto-light" : "carto-dark";
        else if (url.contains("wikimedia.org")) layer = "Wikimedia Maps";
        else {
            qDebug() << "Unknown layer for URL:" << url;
            return;
        }
    }

    QString key = getTileKey(layer, z, x, y);
    if (tileCache.size() >= maxCacheSize) {
        tileCache.remove(tileCache.keys().first());
    }
    QImage img;
    if (!img.loadFromData(data)) {
        qDebug() << "Failed to load image for tile" << key;
        img = QImage(256, 256, QImage::Format_RGB32);
        img.fill(Qt::gray);
    }
    tileCache[key] = img;

    pendingTiles = qMax(0, pendingTiles - 1);
    if (networkImage.loadFromData(data)) {
        update();
    }
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
    net->requestPlace(query);
}

void GISlib::setLayers(const QStringList& layerNames) {
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

void GISlib::paintEvent(QPaintEvent* event) {
    emit painted(event);
    QPainter painter(this);
    int tileSize = 256;
    int tiles = qPow(2, zoom);
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);
    int tilesX = std::ceil(width() / tileSize) + 2;
    int tilesY = std::ceil(height() / tileSize) + 2;
    int startX = qFloor(centerX - tilesX / 2);
    int startY = qFloor(centerY - tilesY / 2);

    bool tilesDrawn = false;
    for (int i = activeLayers.size() - 1; i >= 0; --i) {
        QString layer = activeLayers[i];
        for (int x = startX; x < startX + tilesX; ++x) {
            for (int y = startY; y < startY + tilesY; ++y) {
                int wrappedX = (x % tiles + tiles) % tiles;
                QString key = getTileKey(layer, zoom, x, y);
                if (tileCache.contains(key)) {
                    int dx = (x - centerX) * tileSize + width() / 2;
                    int dy = (y - centerY) * tileSize + height() / 2;
                    painter.drawImage(dx, dy, tileCache[key]);
                    tilesDrawn = true;
                } else {
                    requestTile(layer, y, x, zoom);
                }
            }
        }
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
    bool flip = (layer == "arcgis" || layer == "Esri Street Map" || layer == "Esri Topographic Map");
    return QString("%1/%2/%3/%4").arg(layer).arg(z).arg(flip ? y : x).arg(flip ? x : y);
}

QString GISlib::tileUrl(const QString& layer, int x, int y, int z) {
    QString url;
    bool flip = false;
    if (layer == "osm") {
        url = QString("http://tile.openstreetmap.org/%1/%2/%3.png").arg(z).arg(x).arg(y);
    } else if (layer == "arcgis") {
        url = QString("https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3.png").arg(z).arg(y).arg(x);
        flip = true;
    } else if (layer == "opentopo") {
        url = QString("https://a.tile.opentopomap.org/%1/%2/%3.png").arg(z).arg(x).arg(y);
    } else if (layer == "carto-light") {
        url = QString("https://a.basemaps.cartocdn.com/light_all/%1/%2/%3.png").arg(z).arg(x).arg(y);
    } else if (layer == "carto-dark") {
        url = QString("https://a.basemaps.cartocdn.com/dark_all/%1/%2/%3.png").arg(z).arg(x).arg(y);
    } else if (layer == "Esri Street Map") {
        url = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/%1/%2/%3.png").arg(z).arg(y).arg(x);
        flip = true;
    } else if (layer == "Esri Topographic Map") {
        url = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/%1/%2/%3.png").arg(z).arg(y).arg(x);
        flip = true;
    } else if (layer == "Wikimedia Maps") {
        url = QString("https://maps.wikimedia.org/osm-intl/%1/%2/%3.png").arg(z).arg(x).arg(y);
    } else if (customMaps.contains(layer)) {
        url = customMaps[layer].tileUrl;
        url.replace("{z}", QString::number(z));
        url.replace("{x}", QString::number(x));
        url.replace("{y}", QString::number(y));
        flip = false;
    } else {
        qDebug() << "Unsupported layer:" << layer;
    }
    flipkeyaxis = flip;
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
    if (tileCache.contains(key)) return;
    tileCache[key] = QImage();
    QString url = tileUrl(layer, x, y, z);
    if (url.isEmpty()) return;
    pendingTiles++;
    net->requestImage(QUrl(url));
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
