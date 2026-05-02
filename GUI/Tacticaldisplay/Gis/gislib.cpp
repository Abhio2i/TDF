
#include "gislib.h"
#include "gisnetwork.h"
#include <QNetworkReply>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtMath>
#include <QMessageBox>
#include <qgscoordinatetransform.h>
#include <qgsproject.h>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <core/Debug/console.h>
#include <qgsjsonutils.h>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgsjsonutils.h>
#include <qgswkbtypes.h>
#include <QFileInfo>
#include <qgis.h>

// =========================================================================
// MGRS Conversion Utility Functions (Relies on QGIS for robust UTM conversion)
// =========================================================================

namespace MGRS_Utils {
// Latitude Band Letters (C-H, J-N, P-X, excluding I, O)
const char* latBandLetters = "CDEFGHJKLMNPQRSTUVWXX";
// 100k Square Column Letters (A-Z, excluding I, O)
const char* colLetters = "ABCDEFGHJKLMNPQRSTUVWXYZ";
// 100k Square Row Letters (A-V, excluding I, O)
const char* rowLetters = "ABCDEFGHJKLMNPQRSTUV";

// Gets the MGRS latitude band letter (C-X)
char getLatBand(double lat) {
    if (lat < -80 || lat >= 84) return 'Z';
    // Handle special case for band X (72N to 84N)
    if (lat >= 72.0 && lat <= 84.0) return 'X';

    int index = qFloor((lat + 80.0) / 8.0);
    return latBandLetters[qBound(0, index, 20)];
}

/*
     * latLonToUtm: Convert WGS84 Lat/Lon to UTM Easting/Northing
     * Uses QGIS coordinate transformation to correctly determine and transform to the correct UTM zone.
     */
QPointF latLonToUtm(double lat, double lon, int& zone) {
    // 1. Determine UTM Zone
    zone = qFloor((lon + 180.0) / 6.0) + 1;

    QgsCoordinateReferenceSystem srcCrs("EPSG:4326");
    QgsCoordinateReferenceSystem destCrs;

    // 2. Define destination CRS based on the zone (326XX for North, 327XX for South)
    if (lat >= 0) {
        // Northern Hemisphere
        destCrs = QgsCoordinateReferenceSystem(QString("EPSG:326%1").arg(zone));
    } else {
        // Southern Hemisphere
        destCrs = QgsCoordinateReferenceSystem(QString("EPSG:327%1").arg(zone));
    }

    if (!destCrs.isValid()) {
        qWarning() << "Could not create valid UTM CRS for zone" << zone;
        return QPointF(0, 0);
    }

    // 3. Perform the transformation
    QgsCoordinateTransform transform(srcCrs, destCrs, QgsProject::instance());
    QgsPointXY point(lon, lat);
    QgsPointXY transformed = transform.transform(point);

    return QPointF(transformed.x(), transformed.y());
}
}

/*
 * latLonToMGRS: Convert latitude/longitude to MGRS string
 * Takes WGS84 decimal degrees and returns a 10-digit MGRS string (1-meter precision).
 * NOTE: This function's complexity is why we rely on QGIS for the Lat/Lon -> UTM step.
 */
QString GISlib::latLonToMGRS(double lat, double lon) {
    if (lat < -80 || lat >= 84) return "POLAR";

    int zone;
    QPointF utm = MGRS_Utils::latLonToUtm(lat, lon, zone);
    double easting = utm.x();
    double northing = utm.y();

    // 1. Get GZD (Grid Zone Designation)
    QString gzd = QString::number(zone) + MGRS_Utils::getLatBand(lat);

    // 2. Get 100,000-Meter Square Identifier (Letters)

    // Lookup table initialization for 100k square letters
    // Easting band sequence starts at a specific offset based on (zone-1) mod 3
    int colOffset = (zone - 1) % 3;
    int colIndex = qFloor(easting / 100000.0) - 1;
    char colLetter = MGRS_Utils::colLetters[(colIndex + colOffset * 8) % 24];

    // Northing band sequence (row letters) cycles every 2000 km (20 rows)
    double northingInCycle = northing;
    if (lat >= 0) {
        // Northern Hemisphere starts cycle at 0
        northingInCycle = fmod(northing, 2000000.0);
    } else {
        // Southern Hemisphere reference is 10,000,000m (equator to pole distance)
        // Northing is relative to the equator, but the cycle must be calculated from a fixed point.
        northingInCycle = fmod(northing + 10000000.0, 2000000.0);
    }

    // Starting row letter index for the cycle (complex: determined by latitude band)
    // The index for the C-band (80S) starts at 'H' (index 7).
    int rowOffset = 0;
    char latBand = MGRS_Utils::getLatBand(lat);
    if (latBand >= 'C' && latBand <= 'H') rowOffset = 7;
    else if (latBand >= 'J' && latBand <= 'N') rowOffset = 0; // J-N starts at 'A' relative to the cycle
    else if (latBand >= 'P' && latBand <= 'X') rowOffset = 7;

    int rowIndex = qFloor(northingInCycle / 100000.0);
    char rowLetter = MGRS_Utils::rowLetters[(rowIndex + rowOffset) % 20];

    // 3. Get Numerical Location (5 digits for 1m precision)
    QString numEasting = QString::number(qRound(fmod(easting, 100000.0))).rightJustified(5, '0');
    QString numNorthing = QString::number(qRound(fmod(northing, 100000.0))).rightJustified(5, '0');

    // Combine: GZD + 100k Letters + Numerical Location (e.g., 31SDA 12345 67890)
    return QString("%1%2%3 %4 %5")
        .arg(gzd)
        .arg(colLetter)
        .arg(rowLetter)
        .arg(numEasting)
        .arg(numNorthing);
}

/*
 * Constructor: GISlib
 * Initializes the GIS library with default settings, network components,
 * coordinate systems, and vector layer support.
 */
GISlib::GISlib(QWidget *parent) : QWidget(parent) {
    // Set widget styling and properties
    setStyleSheet("background-color: black;");  // Black background for map
    setFocusPolicy(Qt::StrongFocus);            // Accept keyboard focus
    setAttribute(Qt::WA_NoSystemBackground);    // Optimize rendering
    setAutoFillBackground(false);               // Disable auto-fill for custom painting
    setUpdatesEnabled(true);                    // Enable widget updates
    setMouseTracking(true);                     // Enable mouse tracking for coordinates
    setAcceptDrops(true);                       // Accept drag and drop operations

    // Initialize network component for tile downloads
    net = new GISNetwork(this);
    connect(net, &GISNetwork::receiveImage, this, &GISlib::receiveImage);
    connect(net, &GISNetwork::receivePlace, this, &GISlib::receivePlace);

    // Set default map configuration
    setLayers(QStringList() << "osm");              // Default OpenStreetMap layer
    setCenter(20.5937, 78.9629);                   // Center on India (near Nagpur)
    zoom = 4;                                       // Initialize zoom
    effectiveTileZoom = 4;                          // Initialize effective tile zoom
    setZoom(4);                                     // Set default zoom level to 4 (will update both)
    setInitialOffset(QPointF(100, -50));           // Initial map offset

    // Initialize coordinate reference systems
    currentCrs.createFromString("EPSG:4326");      // Default to WGS84 Lat/Lon
    customCrs.createFromProj4("+proj=tmerc +lat_0=28.6139 +lon_0=77.2090 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +units=m +no_defs");

    // Initialize vector layers for GeoJSON support
    vectorLayers.clear();
    qDebug() << "GISlib initialized with vector layer support";

    airbasePixmap = QPixmap(airbaseIconPath);
    if (airbasePixmap.isNull()) {
        Console::error("Failed to load airbase icon: " + airbaseIconPath.toStdString());
    }

}

/*
 * Drag and Drop Event Handlers
 * Forward drag and drop events to connected components
 */
void GISlib::dragEnterEvent(QDragEnterEvent *event)
{
    emit dragEnterEvents(event);  // Forward drag enter event
}

void GISlib::dragMoveEvent(QDragMoveEvent *event)
{
    emit dragMoveEvents(event);   // Forward drag move event
}

void GISlib::dropEvent(QDropEvent *event)
{
    emit dropEvents(event);       // Forward drop event
}

/*
 * setEllipsoidal: Configure distance calculation mode
 * Toggles between planar and ellipsoidal distance calculations
 */
void GISlib::setEllipsoidal(bool e) {
    ellipsoidal = e;  // Set ellipsoidal calculation flag
}

/*
 * addCustomMap: Add custom map layer with specified parameters
 * Supports custom tile servers with zoom ranges and opacity control
 */
void GISlib::addCustomMap(const QString& layerName, int zoomMin, int zoomMax,
                          const QString& tileUrl, qreal opacity) {
    qDebug() << "Adding custom layer: name =" << layerName << ", zoomMin =" << zoomMin
             << ", zoomMax =" << zoomMax << ", tileUrl =" << tileUrl << ", opacity =" << opacity;

    // Validate input parameters
    if (layerName.isEmpty() || tileUrl.isEmpty()) {
        qWarning() << "Invalid layer name or URL: name=" << layerName << ", tileUrl=" << tileUrl;
        return;
    }

    // Configure custom map layer
    CustomMap customMap;
    customMap.zoomMin = zoomMin;      // Minimum zoom level for display
    customMap.zoomMax = zoomMax;      // Maximum zoom level for display
    customMap.tileUrl = tileUrl;      // Tile server URL template
    customMap.opacity = opacity;      // Layer opacity for rendering

    // Store custom map configuration
    customMaps[layerName.toLower()] = customMap;
    qDebug() << "customMaps now contains:" << customMaps.keys();

    // Activate layer if not already active
    if (!activeLayers.contains(layerName.toLower())) {
        activeLayers.prepend(layerName.toLower());
        qDebug() << "Added" << layerName << "to activeLayers:" << activeLayers;
        update();  // Refresh display
    }
}

/*
 * receiveImage: Handle received tile image data from network
 * Processes downloaded map tiles and manages tile caching
 */
void GISlib::receiveImage(QString url, QByteArray data) {
    // Extract tile coordinates from URL using regular expression
    QRegularExpression rx("/(\\d+)/(\\d+)/(\\d+)\\.png");
    QRegularExpressionMatch match = rx.match(url);

    if (!match.hasMatch()) {
        qDebug() << "Invalid tile URL format:" << url;
        return;
    }

    // Parse tile coordinates from URL
    int z = match.captured(1).toInt();  // Zoom level
    int x = match.captured(2).toInt();  // X coordinate
    int y = match.captured(3).toInt();  // Y coordinate
    QString layer;                      // Layer identifier

    // Determine layer based on URL pattern
    QString appDir = QCoreApplication::applicationDirPath();
    QString osmPath = QDir(appDir).filePath("testOSM");
    QString satPath = QDir(appDir).filePath("satMAP");
    QString tarPath = QDir(appDir).filePath("tarMAP");

    if (url.startsWith(QUrl::fromLocalFile(osmPath).toString())) {
        layer = "osm";
    } else if (url.startsWith(QUrl::fromLocalFile(satPath).toString())) {
        layer = "satellite";
    } else if (url.startsWith(QUrl::fromLocalFile(tarPath).toString())) {
        // layer = "tarrine";
        layer = "terrain";
    } else if (url.contains("openstreetmap.org")) {
        layer = "osm";
    } else if (url.contains("opentopomap.org")) {
        layer = "opentopo";
    } else if (url.contains("cartocdn.com")) {
        layer = url.contains("light_all") ? "carto-light" : "carto-dark";
    } else if (url.contains("arcgisonline.com")) {
        layer = "World Imagery";
        x = match.captured(3).toInt(); // ArcGIS uses {z}/{y}/{x} format
        y = match.captured(2).toInt();
    } else {
        // Check custom maps for matching URL pattern
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
            qDebug() << "Unknown layer for URL:" << url;
            return;
        }
    }

    // Generate unique tile key for caching
    QString key = getTileKey(layer, z, x, y);
    pendingTileKeys.remove(key);  // Remove from pending requests

    // Handle tile download retries
    int retries = tileRetries.value(key, 3);
    tileRetries.remove(key);

    // Process empty data (download failure)
    if (data.isEmpty()) {
        // qDebug() << "Empty data for tile" << key << ", retries left:" << retries - 1;
        if (retries > 0) {
            // Retry download with decreased retry count
            tileRetries[key] = retries - 1;
            pendingTileKeys.insert(key);
            requestTile(layer, x, y, z, retries - 1);
        } else {
            // Create gray placeholder tile after max retries
            QPixmap pixmap(256, 256);
            pixmap.fill(Qt::gray);
            tileCache[key] = pixmap;
            pendingTiles = qMax(0, pendingTiles - 1);
            update();  // Refresh display
        }
        return;
    }

    // Manage tile cache size
    if (tileCache.size() >= maxCacheSize) {
        tileCache.remove(tileCache.keys().first());  // Remove oldest tile
    }

    // Load image data into QImage
    QImage img;
    if (!img.loadFromData(data)) {
        qDebug() << "Failed to load image for tile" << key << ", retries left:" << retries - 1;
        if (retries > 0) {
            // Retry image loading
            tileRetries[key] = retries - 1;
            pendingTileKeys.insert(key);
            requestTile(layer, x, y, z, retries - 1);
        } else {
            // Create gray placeholder after max retries
            QPixmap pixmap(256, 256);
            pixmap.fill(Qt::gray);
            tileCache[key] = pixmap;
            pendingTiles = qMax(0, pendingTiles - 1);
            update();  // Refresh display
        }
        return;
    }

    // Store successful tile in cache
    QPixmap pixmap = QPixmap::fromImage(img);
    tileCache[key] = pixmap;

    pendingTiles = qMax(0, pendingTiles - 1);  // Decrement pending count
    update();  // Refresh display with new tile
}

/* Set coordinate reference system by CRS ID */
void GISlib::setCoordinateSystem(const QString& crsId) {
    // 1. Update the string flag to track the display mode
    currentCoordinateSystem = crsId;

    // 2. Set the QgsCoordinateReferenceSystem object.
    // For MGRS/UTM_AUTO/4326, we reset the currentCrs to WGS84.
    // The dynamic UTM calculation will happen in mouseMoveEvent.
    if (crsId == "EPSG:4326" || crsId == "MGRS" || crsId == "UTM_AUTO") {
        currentCrs.createFromString("EPSG:4326");
    }

    qDebug() << "GISlib: CRS updated to" << crsId;

    // 3. Force coordinate display update
    // Simulate a mouse move event to instantly update the overlay label
    QPoint currentMousePos = mapFromGlobal(QCursor::pos());
    if (rect().contains(currentMousePos)) {
        QMouseEvent fakeMove(QEvent::MouseMove, currentMousePos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        mouseMoveEvent(&fakeMove);
    }
    update();
}


/*
 * receivePlace: Process received place search results
 * Updates map marker and outline based on search results
 */
void GISlib::receivePlace(QString url, QByteArray data) {
    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    if (!arr.isEmpty()) {
        QJsonObject place = arr.first().toObject();
        double lat = place["lat"].toString().toDouble();  // Extract latitude
        double lon = place["lon"].toString().toDouble();  // Extract longitude

        // Extract GeoJSON outline for polygon rendering
        geoOutline = place["geojson"].toObject()["coordinates"].toArray().at(0).toArray();
        marker.lat = lat;  // Set marker position
        marker.lon = lon;

        // Determine appropriate zoom level based on place rank
        int placeRank = place["place_rank"].toInt();
        int zoomLevel;

        // Map place rank to zoom levels for appropriate detail
        if (placeRank >= 30) zoomLevel = 6;      // Country level
        else if (placeRank >= 25) zoomLevel = 7;  // State level
        else if (placeRank >= 20) zoomLevel = 7; // Region level
        else if (placeRank >= 16) zoomLevel = 8; // City level
        else if (placeRank >= 12) zoomLevel = 8; // Town level
        else if (placeRank >= 8) zoomLevel = 9;  // Village level
        else zoomLevel = 6;                      // Default level

        // Update map view to show search result
        setCenter(lat, lon);
        setZoom(zoomLevel);
    }
}

//for bound cheking
void GISlib::fitToBounds(double minLat, double minLon,
                         double maxLat, double maxLon, int zoomOffset)
{
    // Center point
    double centerLat = (minLat + maxLat) / 2.0;
    double centerLon = (minLon + maxLon) / 2.0;

    // Move map to center
    setCenter(centerLat, centerLon);

    // --- AUTO ZOOM LOGIC ---
    double latSpan = std::abs(maxLat - minLat);

    // ---- base zoom (auto) ----
    int baseZoom;
    if (latSpan > 12)      baseZoom = 5;   // country
    else if (latSpan > 6)  baseZoom = 6;   // big state
    else if (latSpan > 3)  baseZoom = 7;   // state
    else if (latSpan > 1.5)baseZoom = 8;
    else if (latSpan > 0.7)baseZoom = 9;
    else                   baseZoom = 10; // city

    // ---- APPLY OFFSET ----
    int finalZoom = baseZoom + zoomOffset;

    // ---- clamp ----
    finalZoom = std::clamp(finalZoom, 4, 10);

    setZoom(finalZoom);
}

/*
 * serachPlace: Initiate place search by query string
 * Forwards search request to network component
 */
void GISlib::serachPlace(const QString& query) {
    qDebug() << "Searching place:" << query;
    net->requestPlace(query);  // Forward to network handler
}

/*
 * setLayers: Configure active map layers for rendering
 * Clears tile cache and updates display
 */
void GISlib::setLayers(const QStringList& layerNames) {
    qDebug() << "Setting layers:" << layerNames;
    activeLayers = layerNames;  // Update active layers
    tileCache.clear();          // Clear tile cache
    pendingTiles = 0;           // Reset pending tile count
    update();                   // Refresh display
}

/*
 * setCenter: Set map center to specified geographic coordinates
 * Updates center position and emits change signal
 */
void GISlib::setCenter(double lat, double lon) {
    centerLat = lat;            // Update latitude
    centerLon = lon;            // Update longitude
    emit centerChanged(lat, lon); // Notify center change
    update();                   // Refresh display
}

/*
 * searchByCoordinates: Search by geographic coordinates
 * Sets marker at specified coordinates and adjusts zoom level
 */
void GISlib::searchByCoordinates(double lat, double lon) {
    qDebug() << "Searching by coordinates: Lat =" << lat << ", Lon =" << lon;

    // Clear previous geographic data
    geoOutline = QJsonArray();  // Clear outline
    marker.lat = lat;           // Set marker latitude
    marker.lon = lon;           // Set marker longitude

    int zoomLevel = 4; // Medium zoom level for coordinate search

    // Update map view
    setCenter(lat, lon);
    setZoom(zoomLevel);
    update();  // Refresh display

    Console::log("Coordinate search: " + std::to_string(lat) + ", " + std::to_string(lon));
}

/*
 * setZoom: Set map zoom level with validation
 * Adjusts zoom based on layer capabilities and clears tile cache
 * Visual zoom can go up to 19, but tiles are only requested up to maxZoom
 */
// void GISlib::setZoom(int level) {
//     int maxTileZoom = 13;  // Default maximum tile zoom for most layers

//     // Adjust max tile zoom based on active layers
//     if (activeLayers.contains("opentopo")) maxTileZoom = 17;
//     for (const QString& layer : activeLayers) {
//         if (customMaps.contains(layer)) {
//             maxTileZoom = qMin(maxTileZoom, customMaps[layer].zoomMax);
//         }
//     }

//     // Clamp visual zoom level to absolute maximum (19)
//     zoom = qBound(1, level, ABSOLUTE_MAX_ZOOM);

//     // Effective tile zoom is capped at the layer's maximum tile availability
//     effectiveTileZoom = qMin(zoom, maxTileZoom);

//     tileCache.clear();          // Clear tile cache for new zoom
//     pendingTiles = 0;           // Reset pending tiles
//     emit zoomChanged(zoom);     // Notify zoom change (visual zoom)
//     update();                   // Refresh display
// }
void GISlib::setZoom(int level) {
    int maxTileZoom = 8;  // Default maximum tile zoom for most layers

    // Adjust max tile zoom based on active layers
    if (activeLayers.contains("opentopo")) maxTileZoom = 17;
    for (const QString& layer : activeLayers) {
        if (customMaps.contains(layer)) {
            maxTileZoom = qMin(maxTileZoom, customMaps[layer].zoomMax);
        }
    }

    // Clamp visual zoom level to absolute maximum (19)
    zoom = qBound(1, level, ABSOLUTE_MAX_ZOOM);

    // Effective tile zoom is capped at the layer's maximum tile availability
    effectiveTileZoom = qMin(zoom, maxTileZoom);

    // qDebug() << "Visual Zoom:" << zoom << "| Effective Tile Zoom:" << effectiveTileZoom;  // <-- ADD THIS LINE

    tileCache.clear();          // Clear tile cache for new zoom
    pendingTiles = 0;           // Reset pending tiles
    emit zoomChanged(zoom);     // Notify zoom change (visual zoom)
    update();                   // Refresh display
}

/*
 * Coordinate Transformation Functions
 * Convert between geographic coordinates and tile coordinates
 */

/* Convert longitude to tile X coordinate at specified zoom level */
double GISlib::lonToX(double lon, int zoom) {
    return ((lon + 180.0) / 360.0) * std::pow(2.0, zoom);
}

/* Convert latitude to tile Y coordinate at specified zoom level */
double GISlib::latToY(double lat, int zoom) {
    double rad = lat * M_PI / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI) / 2.0 * std::pow(2.0, zoom);
}

/* Convert tile X coordinate to longitude */
double GISlib::xToLon(double x, int zoom) {
    return x / std::pow(2.0, zoom) * 360.0 - 180.0;
}

/* Convert tile Y coordinate to latitude */
double GISlib::yToLat(double y, int zoom) {
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, zoom);
    return (180.0 / M_PI) * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

/*
 * toDMS: Convert decimal degrees to Degrees-Minutes-Seconds format
 * Used for coordinate display in traditional format
 */
QString GISlib::toDMS(double deg, bool isLat) {
    double absolute = std::abs(deg);
    int degrees = static_cast<int>(std::floor(absolute));
    double minutesNotTruncated = (absolute - degrees) * 60.0;
    int minutes = static_cast<int>(std::floor(minutesNotTruncated));
    int seconds = static_cast<int>(std::floor((minutesNotTruncated - minutes) * 60.0));

    // Determine direction character based on coordinate type and sign
    QChar direction;
    if (isLat) {
        direction = (deg >= 0) ? 'N' : 'S';  // North/South for latitude
    } else {
        direction = (deg >= 0) ? 'E' : 'W';  // East/West for longitude
    }

    // Format as D°M'S"Direction
    return QString("%1°%2'%3\"%4")
        .arg(degrees)
        .arg(minutes)
        .arg(seconds)
        .arg(direction);
}

/*
 * paintEvent: Main rendering function
 * Draws map tiles, vector layers, markers, and measurement tools
 * Scales tiles when visual zoom exceeds available tile zoom
 */
void GISlib::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    // drawAirbases(painter);

    painter.setRenderHint(QPainter::Antialiasing);  // Enable smooth rendering

    bool tilesDrawn = false;    // Track if any tiles were rendered
    int tileSize = 256;         // Standard tile size
    int tiles = qPow(2, zoom);  // Number of tiles at current zoom (visual)

    // Calculate scale factor when zoomed beyond tile availability
    int zoomDiff = zoom - effectiveTileZoom;
    double scaleFactor = qPow(2.0, zoomDiff);

    // Calculate center in tile coordinates (using visual zoom)
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);

    // Determine visible tile range (using visual zoom)
    int tilesX = std::ceil(width() / tileSize) + 2;
    int tilesY = std::ceil(height() / tileSize) + 2;
    int startX = qFloor(centerX - tilesX / 2);
    int startY = qFloor(centerY - tilesY / 2);

    int fallbackCount = 0;      // Counter for fallback tile attempts
    const int maxFallbacks = 100; // Maximum fallback attempts

    // Render map layers from bottom to top
    for (int i = activeLayers.size() - 1; i >= 0; --i) {
        QString layer = activeLayers[i];
        qreal opacity = 1.0;

        // Apply layer-specific opacity
        if (customMaps.contains(layer)) {
            opacity = customMaps[layer].opacity;
        }
        painter.setOpacity(opacity);

        // Render tiles for current layer
        for (int x = startX; x < startX + tilesX; ++x) {
            for (int y = startY; y < startY + tilesY; ++y) {
                int wrappedX = (x % tiles + tiles) % tiles;  // Wrap X coordinate

                // If we're zoomed beyond tile availability, request tiles at effectiveTileZoom
                // and scale them up
                int tileX, tileY, tileZoom;
                if (zoomDiff > 0) {
                    // Calculate which tile from effectiveTileZoom contains this visual position
                    tileX = wrappedX >> zoomDiff;  // Divide by 2^zoomDiff
                    tileY = y >> zoomDiff;
                    tileZoom = effectiveTileZoom;
                } else {
                    tileX = wrappedX;
                    tileY = y;
                    tileZoom = zoom;
                }

                int effectiveTiles = qPow(2, tileZoom);
                int wrappedTileX = (tileX % effectiveTiles + effectiveTiles) % effectiveTiles;
                QString key = getTileKey(layer, tileZoom, wrappedTileX, tileY);

                // Render cached tile if available
                if (tileCache.contains(key) && !tileCache[key].isNull()) {
                    int dx = (wrappedX - centerX) * tileSize + width() / 2;
                    int dy = (y - centerY) * tileSize + height() / 2;

                    if (zoomDiff > 0) {
                        // Scale the tile up when zoomed beyond tile availability
                        // Calculate which portion of the scaled tile to draw
                        int subTileX = wrappedX % (1 << zoomDiff);
                        int subTileY = y % (1 << zoomDiff);
                        int subTileSize = tileSize / (1 << zoomDiff);

                        QRect srcRect(subTileX * subTileSize, subTileY * subTileSize,
                                      subTileSize, subTileSize);
                        QRect dstRect(dx, dy, tileSize, tileSize);

                        painter.drawPixmap(dstRect, tileCache[key], srcRect);
                    } else {
                        // Normal rendering at available zoom level
                        painter.drawPixmap(dx, dy, tileCache[key]);
                    }
                    tilesDrawn = true;
                } else {
                    // Attempt to use lower zoom level tiles as fallback
                    if (fallbackCount < maxFallbacks) {
                        fallbackCount++;
                        for (int dz = 1; dz <= tileZoom; ++dz) {
                            int lowerZoom = tileZoom - dz;
                            int lowerTiles = qPow(2, lowerZoom);
                            int lowerX = wrappedTileX >> dz;  // Divide by 2^dz
                            int lowerY = tileY >> dz;
                            QString lowerKey = getTileKey(layer, lowerZoom, lowerX, lowerY);

                            if (tileCache.contains(lowerKey) && !tileCache[lowerKey].isNull()) {
                                int scale = 1 << dz;  // 2^dz
                                int dx = (wrappedX - centerX) * tileSize + width() / 2;
                                int dy = (y - centerY) * tileSize + height() / 2;
                                QRect targetRect(dx, dy, tileSize, tileSize);
                                int srcX = (wrappedTileX % scale) * (tileSize / scale);
                                int srcY = (tileY % scale) * (tileSize / scale);
                                QRect srcRect(srcX, srcY, tileSize / scale, tileSize / scale);
                                painter.drawPixmap(targetRect, tileCache[lowerKey], srcRect);
                                tilesDrawn = true;
                                break;
                            }
                        }
                    }
                    // Request missing tile (only at effectiveTileZoom)
                    requestTile(layer, wrappedTileX, tileY, tileZoom);
                }
            }
        }
        painter.setOpacity(1.0); // Reset opacity for next layer
    }

    // NEW: Render vector layers (GeoJSON data)
    for (auto it = vectorLayers.constBegin(); it != vectorLayers.constEnd(); ++it) {
        const VectorLayer& layer = it.value();
        renderVectorLayer(painter, layer);
    }

    // Draw distance measurement line and label if active
    if (measuringDistance && measureStartPoint != QPointF(0, 0)) {
        painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);

        // Convert geographic points to canvas coordinates
        QPointF startCanvas = geoToCanvas(measureStartPoint.y(), measureStartPoint.x());
        QPointF endCanvas = geoToCanvas(measureEndPoint.y(), measureEndPoint.x());
        painter.drawLine(startCanvas, endCanvas);

        // Draw measurement points
        painter.setBrush(Qt::yellow);
        painter.drawEllipse(startCanvas, 5, 5);
        painter.drawEllipse(endCanvas, 5, 5);

        // Calculate and display distance
        double distanceMeters = calculateDistance(measureStartPoint, measureEndPoint);
        double distanceKilometers = distanceMeters / 1000.0;
        QString distanceText = QString("%1 m (%2 km)")
                                   .arg(distanceMeters, 0, 'f', 2)
                                   .arg(distanceKilometers, 0, 'f', 2);

        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 10));
        QPointF textPos = (startCanvas + endCanvas) / 2.0;
        painter.drawText(textPos + QPointF(10, -10), distanceText);
    }

    // Display loading message if no tiles rendered
    if (!tilesDrawn) {
        painter.setPen(Qt::white);
        painter.drawText(rect().center(), QString("Loading tiles... (%1 pending)").arg(pendingTiles));
    }

    // Draw search marker
    double markerX = lonToX(marker.lon, zoom);
    double markerY = latToY(marker.lat, zoom);
    double dx = (markerX - centerX) * tileSize + width() / 2;
    double dy = (markerY - centerY) * tileSize + height() / 2;
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.drawEllipse(QPoint(dx, dy), 5, 5);

    // Draw geographic outline (polygon)
    QPolygonF polygon;
    for (const QJsonValue& pointVal : geoOutline) {
        QJsonArray coord = pointVal.toArray();
        double lon = coord.at(0).toDouble();
        double lat = coord.at(1).toDouble();
        double px = (lonToX(lon, zoom) - centerX) * tileSize + width() / 2;
        double py = (latToY(lat, zoom) - centerY) * tileSize + height() / 2;
        polygon << QPointF(px, py);
    }
    painter.setBrush(QColor(255, 0, 0, 50));  // Semi-transparent red
    painter.setPen(QPen(Qt::red, 2));
    painter.drawPolygon(polygon);
}

/*
 * getTileKey: Generate unique key for tile caching
 * Format: "layer/zoom/x/y" with optional coordinate flipping
 */
QString GISlib::getTileKey(const QString& layer, int z, int x, int y) {
    bool flip = (layer == "World Imagery");  // ArcGIS uses flipped coordinates
    return QString("%1/%2/%3/%4").arg(layer).arg(z).arg(flip ? y : x).arg(flip ? x : y);
}

/*
 * tileUrl: Generate tile URL for specific layer and coordinates
 * Supports multiple tile servers and custom map layers
 */
QString GISlib::tileUrl(const QString& layer, int x, int y, int z) {
    //qDebug() << "Generating tile URL for layer:" << layer << ", z:" << z << ", x:" << x << ", y:" << y;
    QString lowerLayer = layer.toLower();

    // Local OSM tiles
    if (lowerLayer == "osm") {
        QString appDir = QCoreApplication::applicationDirPath();
        QString osmPath = QDir(appDir).filePath("testOSM");
        QString filePath = QString("file://%1/%2/%3/%4.png").arg(osmPath).arg(z).arg(x).arg(y);
        //qDebug() << "Generated local tile path for OSM:" << filePath;
        flipkeyaxis = false;
        return filePath;
    }

    // Local satellite tiles
    if (lowerLayer == "satellite") {
        QString appDir = QCoreApplication::applicationDirPath();
        QString satPath = QDir(appDir).filePath("satMAP");
        QString filePath = QString("file://%1/%2/%3/%4.png").arg(satPath).arg(z).arg(x).arg(y);
        //qDebug() << "Generated local tile path for Satellite Map:" << filePath;
        flipkeyaxis = false;
        return filePath;
    }

    // Local terrain tiles
    if (lowerLayer == "terrain") {
        QString appDir = QCoreApplication::applicationDirPath();
        QString tarPath = QDir(appDir).filePath("tarMAP");
        QString filePath = QString("file://%1/%2/%3/%4.png").arg(tarPath).arg(z).arg(x).arg(y);
        //qDebug() << "Generated local tile path for Terrain Map:" << filePath;
        flipkeyaxis = false;
        return filePath;
    }

    // Custom map layers
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

    // Online tile servers
    QString url;
    QString subdomain = getSubdomain(x, y, layer);
    bool flip = false;

    if (layer == "opentopo") {
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

/*
 * requestTile: Request tile from network with validation and retry logic
 * Manages tile requests, caching, and coordinate validation
 * Does not request tiles beyond the effectiveTileZoom level
 */
void GISlib::requestTile(const QString& layer, int x, int y, int z, int retries) {
    int tiles = qPow(2, z);
    x = (x % tiles + tiles) % tiles;  // Wrap X coordinate

    // Validate Y coordinate range
    if (y < 0 || y >= tiles) return;

    // Check zoom level validity for custom maps
    if (customMaps.contains(layer)) {
        if (z < customMaps[layer].zoomMin || z > customMaps[layer].zoomMax) return;
    }

    // Don't request tiles beyond effectiveTileZoom - they should be scaled instead
    if (z > effectiveTileZoom) return;

    QString key = getTileKey(layer, z, x, y);

    // Skip if tile already cached or pending
    if (tileCache.contains(key) && !tileCache[key].isNull()) return;
    if (pendingTileKeys.contains(key)) return;

    // Initialize tile request
    tileCache[key] = QPixmap();          // Placeholder entry
    pendingTileKeys.insert(key);          // Mark as pending
    tileRetries[key] = retries;           // Set retry count

    QString url = tileUrl(layer, x, y, z);
    if (url.isEmpty()) return;

    //qDebug() << "Requesting tile for" << layer << ":" << url;
    pendingTiles++;                       // Increment pending count
    net->requestImage(QUrl(url));         // Send network request
}

/*
 * geoToCanvas: Convert geographic coordinates to canvas pixel coordinates
 * Used for positioning markers and vector features on the map
 */
QPointF GISlib::geoToCanvas(double lat, double lon) {
    int tileSize = 256;

    // Calculate center in tile coordinates
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);

    // Calculate point in tile coordinates
    double pointX = lonToX(lon, zoom);
    double pointY = latToY(lat, zoom);

    // Convert to canvas pixel coordinates
    double dx = (pointX - centerX) * tileSize + width() / 2;
    double dy = (pointY - centerY) * tileSize + height() / 2;

    return QPointF(dx, dy);
}

/*
 * canvasToGeo: Convert canvas pixel coordinates to geographic coordinates
 * Used for interpreting mouse clicks and measurements
 */
QPointF GISlib::canvasToGeo(QPointF p) {
    int tileSize = 256;

    // Calculate center in tile coordinates
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);

    // Convert canvas coordinates to tile coordinates
    double tileX = centerX + (p.x() - width() / 2.0) / tileSize;
    double tileY = centerY + (p.y() - height() / 2.0) / tileSize;

    // Convert tile coordinates to geographic coordinates
    double lon = xToLon(tileX, zoom);
    double lat = yToLat(tileY, zoom);

    return QPointF(lon, lat); // Return as (longitude, latitude)
}

/*
 * getSubdomain: Get subdomain for tile URL load balancing
 * Distributes requests across multiple subdomains for better performance
 */
QString GISlib::getSubdomain(int x, int y, const QString& layer) {
    QStringList subdomains;

    // Define subdomains for different tile servers
    if (layer == "osm" || layer == "opentopo" || layer == "carto-light" || layer == "carto-dark") {
        subdomains << "a" << "b" << "c";
    } else if (customMaps.contains(layer.toLower()) && customMaps[layer.toLower()].tileUrl.contains("{s}")) {
        subdomains << "a" << "b" << "c";
    }

    if (subdomains.isEmpty()) {
        return QString();  // No subdomains needed
    }

    // Select subdomain based on tile coordinates for even distribution
    int index = (x + y) % subdomains.size();
    return subdomains[index];
}

/*
 * Mouse Event Handlers
 * Handle map interaction including dragging and measurement
 */

void GISlib::mousePressEvent(QMouseEvent* event) {
    emit mousePressed(event);  // Forward event

    // Middle button for map dragging
    if (event->button() == Qt::MiddleButton) {
        dragging = true;
        lastMouse = event->pos();  // Store initial position
    }

    // Left button for distance measurement start
    if (measuringDistance && event->button() == Qt::LeftButton) {
        measureStartPoint = canvasToGeo(event->pos());  // Convert to geographic
        measureEndPoint = measureStartPoint;
        qDebug() << "Measurement start point set to (lon:" << measureStartPoint.x()
                 << ", lat:" << measureStartPoint.y() << ")";
        update();  // Refresh display
    }
}

void GISlib::mouseReleaseEvent(QMouseEvent* event) {
    emit mouseReleased(event);  // Forward event

    // End map dragging
    if (event->button() == Qt::MiddleButton) {
        dragging = false;
    }

    // Complete distance measurement
    if (measuringDistance && event->button() == Qt::LeftButton) {
        endDistanceMeasurement();
    }
}
// In gislib.cpp
void GISlib::mouseMoveEvent(QMouseEvent* event) {
    // 1. Event Forwarding
    emit mouseMoved(event);

    int tileSize = 256;

    // 2. Map Dragging/Panning Logic (RETAINED)
    if (dragging) {
        QPointF delta = event->pos() - lastMouse;
        double dx = delta.x();
        double dy = delta.y();

        // Calculate new center in tile coordinates
        double cx = lonToX(centerLon, zoom) - dx / tileSize;
        double cy = latToY(centerLat, zoom) - dy / tileSize;

        // Convert back to geographic coordinates
        centerLon = xToLon(cx, zoom);
        centerLat = yToLat(cy, zoom);

        lastMouse = event->pos();
        emit centerChanged(centerLat, centerLon);
        update();
    }

    // 3. Update measurement end point (RETAINED)
    if (measuringDistance) {
        measureEndPoint = canvasToGeo(event->pos());
        qDebug() << "Measurement end point updated to (lon:" << measureEndPoint.x()
                 << ", lat:" << measureEndPoint.y() << ")";
        update();
    }

    // 4. Calculate mouse coordinates in WGS84 (EPSG:4326)
    QPointF geo = canvasToGeo(event->pos());
    double lon = geo.x();
    double lat = geo.y();

    double displayLat = lat;
    double displayLon = lon;
    QString displayCrsId = "EPSG:4326"; // Default WGS84

    // 5. Transformation Logic (FIXED for robust switching)

    if (currentCoordinateSystem == "MGRS") {
        // MGRS: Emit WGS84 coordinates and let TacticalDisplay slot handle conversion
        displayCrsId = "MGRS";

    } else if (currentCoordinateSystem == "UTM_AUTO") {
        // UTM_AUTO: Dynamically determine UTM zone and transform

        int zone = qFloor((lon + 180.0) / 6.0) + 1;
        // Determine UTM EPSG code (326XX for North, 327XX for South)
        QString utmEpsg = (lat >= 0) ? QString("EPSG:326%1").arg(zone) : QString("EPSG:327%1").arg(zone);

        QgsCoordinateReferenceSystem srcCrs("EPSG:4326");
        QgsCoordinateReferenceSystem utmCrs(utmEpsg);

        if (utmCrs.isValid()) {
            QgsCoordinateTransform transform(srcCrs, utmCrs, QgsProject::instance());
            try {
                QgsPointXY point(lon, lat);
                QgsPointXY transformedPoint = transform.transform(point);

                // Set the UTM Easting (X) and Northing (Y) for display
                displayLat = transformedPoint.y();
                displayLon = transformedPoint.x();
                displayCrsId = utmEpsg; // e.g., "EPSG:32644"

                // Update currentCrs to the dynamic UTM zone
                currentCrs = utmCrs;

            } catch (QgsCsException &e) {
                qWarning() << "UTM Transformation error:" << e.what();
                // Fallback to WGS84
            }
        } else {
            qWarning() << "Invalid UTM CRS:" << utmEpsg;
            // Fallback to WGS84
        }

    } else {
        // Default: EPSG:4326 (Lat/Lon). No transformation needed.
        displayCrsId = "EPSG:4326";
    }

    // 6. Emit the signal with the coordinates and CRS ID
    emit mouseCords(displayLat, displayLon, displayCrsId);
}

/*
 * keyPressEvent: Handle keyboard input for map navigation
 * Supports zoom in/out with plus/minus keys
 */
void GISlib::keyPressEvent(QKeyEvent* event) {
    emit keyPressed(event);  // Forward event

    // Zoom in with plus/equal key
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        setZoom(zoom + 1);
    }
    // Zoom out with minus key
    else if (event->key() == Qt::Key_Minus) {
        setZoom(zoom - 1);
    }
}

/*
 * wheelEvents: Handle mouse wheel events for zooming
 * Zooms in/out based on wheel direction with bounds checking
 */
void GISlib::wheelEvents(QWheelEvent *event) {
    // 1. Capture the geographic coordinate currently under the mouse cursor
    QPointF mousePos = event->position();
    QPointF geoPointBefore = canvasToGeo(mousePos);

    // 2. Determine new zoom level
    int delta = event->angleDelta().y();
    int newZoom = zoom;
    if (delta > 0) {
        newZoom++;
    } else {
        newZoom--;
    }

    // 3. Apply the zoom (this updates the internal 'zoom' variable)
    setZoom(newZoom);

    // 4. Calculate where that same geographic point is NOW after zooming
    // Since center haven't changed yet, geoToCanvas will show where it shifted to.
    QPointF canvasPointAfter = geoToCanvas(geoPointBefore.y(), geoPointBefore.x());

    // 5. Calculate the pixel displacement (offset)
    // We want the point to be at 'mousePos', but it's currently at 'canvasPointAfter'
    double dx = canvasPointAfter.x() - mousePos.x();
    double dy = canvasPointAfter.y() - mousePos.y();

    // 6. Convert that pixel displacement back to geographic coordinates
    // to find the new center that compensates for the shift
    int tileSize = 256;
    double centerX = lonToX(centerLon, zoom);
    double centerY = latToY(centerLat, zoom);

    double newTileCenterX = centerX + dx / tileSize;
    double newTileCenterY = centerY + dy / tileSize;

    // 7. Set the new center
    setCenter(yToLat(newTileCenterY, zoom), xToLon(newTileCenterX, zoom));
}
/*
 * setInitialOffset: Set initial map offset for positioning
 * Used for initial map alignment and calibration
 */
void GISlib::setInitialOffset(QPointF offset) {
    initialOffset = offset;
    update();  // Refresh display
}

/*
 * calculateDistance: Calculate distance between two geographic points
 * Supports both planar approximation and ellipsoidal (Vincenty) calculations
 */
double GISlib::calculateDistance(QPointF point1, QPointF point2) {
    // Planar distance calculation (faster, less accurate)
    if (!ellipsoidal) {
        const double a = 6378137.0; // Earth's equatorial radius in meters

        // Convert to radians
        double lat1 = point1.y() * M_PI / 180.0;
        double lat2 = point2.y() * M_PI / 180.0;
        double lon1 = point1.x() * M_PI / 180.0;
        double lon2 = point2.x() * M_PI / 180.0;

        // Average latitude for longitude scaling
        double avgLat = (lat1 + lat2) / 2.0;

        // Calculate differences in meters
        double dx = (lon2 - lon1) * a * cos(avgLat); // Longitude difference
        double dy = (lat2 - lat1) * a;               // Latitude difference

        return sqrt(dx * dx + dy * dy);  // Euclidean distance
    }

    // Ellipsoidal distance using Vincenty formula (more accurate)
    const double a = 6378137.0;    // Equatorial radius in meters
    const double f = 1.0 / 298.257223563; // Flattening of the ellipsoid
    const double b = a * (1.0 - f); // Polar radius

    // Convert to radians
    double lat1 = point1.y() * M_PI / 180.0;
    double lon1 = point1.x() * M_PI / 180.0;
    double lat2 = point2.y() * M_PI / 180.0;
    double lon2 = point2.x() * M_PI / 180.0;

    // Difference in longitude
    double L = lon2 - lon1;

    // Reduced latitudes
    double U1 = atan((1.0 - f) * tan(lat1));
    double U2 = atan((1.0 - f) * tan(lat2));

    double sinU1 = sin(U1), cosU1 = cos(U1);
    double sinU2 = sin(U2), cosU2 = cos(U2);

    // Iterative Vincenty calculation
    double lambda = L;
    double lambdaP;
    int maxIterations = 100;
    int iter = 0;

    double sinSigma, cosSigma, sigma, cos2Alpha, cos2SigmaM;
    double C, deltaLambda;

    do {
        lambdaP = lambda;
        sinSigma = sqrt(pow(cosU2 * sin(lambda), 2) +
                        pow(cosU1 * sinU2 - sinU1 * cosU2 * cos(lambda), 2));
        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cos(lambda);
        sigma = atan2(sinSigma, cosSigma);
        double sinAlpha = cosU1 * cosU2 * sin(lambda) / sinSigma;
        cos2Alpha = 1.0 - sinAlpha * sinAlpha;
        cos2SigmaM = cosSigma - 2.0 * sinU1 * sinU2 / cos2Alpha;

        // Handle edge cases to prevent division by zero
        if (cos2Alpha < 1e-12) cos2Alpha = 0.0;
        if (cos2Alpha == 0.0) cos2SigmaM = 0.0;

        C = f / 16.0 * cos2Alpha * (4.0 + f * (4.0 - 3.0 * cos2Alpha));
        deltaLambda = L + (1.0 - C) * f * sinAlpha *
                              (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));
        lambda = deltaLambda;
        iter++;
    } while (fabs(lambda - lambdaP) > 1e-12 && iter < maxIterations);

    // Check for convergence failure
    if (iter >= maxIterations) {
        qDebug() << "Vincenty formula did not converge";
        return 0.0; // Fallback for non-convergence
    }

    // Final distance calculation
    double u2 = cos2Alpha * (a * a - b * b) / (b * b);
    double A = 1.0 + u2 / 16384.0 * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
    double B = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
    double deltaSigma = B * sinSigma * (cos2SigmaM + B / 4.0 * (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
                                                                B / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma) *
                                                                    (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));

    double s = b * A * (sigma - deltaSigma); // Distance in meters
    return s;
}

/*
 * startDistanceMeasurement: Begin distance measurement mode
 * Resets measurement points and enables measurement state
 */
void GISlib::startDistanceMeasurement() {
    measuringDistance = true;           // Enable measurement mode
    measureStartPoint = QPointF(0, 0);  // Reset start point
    measureEndPoint = QPointF(0, 0);    // Reset end point
    qDebug() << "Distance measurement started";
    update();  // Refresh display
}

/*
 * endDistanceMeasurement: Complete distance measurement
 * Calculates final distance and emits measurement signal
 */
void GISlib::endDistanceMeasurement() {
    if (measuringDistance) {
        // Calculate final distance
        double distance = calculateDistance(measureStartPoint, measureEndPoint);
        emit distanceMeasured(distance, measureStartPoint, measureEndPoint);

        // Reset measurement state
        measuringDistance = false;
        measureStartPoint = QPointF(0, 0);
        measureEndPoint = QPointF(0, 0);
        qDebug() << "Distance measurement ended";
        update();  // Refresh display
    }
}

// ============================================================================
// GeoJSON Layer Management Functions
// ============================================================================

/*
 * importGeoJsonLayer: Import GeoJSON layer from file
 * Parses GeoJSON file and creates vector layer for rendering
 */
void GISlib::importGeoJsonLayer(const QString &filePath) {
    qDebug() << "Importing GeoJSON from:" << filePath;

    // Open and read GeoJSON file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open GeoJSON file:" << filePath;
        Console::error("Failed to open GeoJSON file: " + filePath.toStdString());
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // Parse JSON document
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || parseError.error != QJsonParseError::NoError) {
        qWarning() << "Invalid GeoJSON format:" << parseError.errorString();
        Console::error("Invalid GeoJSON format: " + parseError.errorString().toStdString());
        return;
    }

    // Validate GeoJSON structure
    QJsonObject geoJson = doc.object();
    if (!geoJson.contains("type") || geoJson["type"].toString() != "FeatureCollection" ||
        !geoJson.contains("features") || !geoJson["features"].isArray()) {
        qWarning() << "Invalid GeoJSON structure (expected FeatureCollection):" << filePath;
        Console::error("Invalid GeoJSON structure - expected FeatureCollection");
        return;
    }

    QJsonArray features = geoJson["features"].toArray();
    if (features.isEmpty()) {
        qDebug() << "No features found in GeoJSON:" << filePath;
        Console::log("GeoJSON file is empty - no features to import");
        return;
    }

    // Generate unique layer name from filename
    QString layerName = QFileInfo(filePath).completeBaseName();
    if (vectorLayers.contains(layerName)) {
        // Append number if layer name already exists
        int counter = 1;
        QString originalName = layerName;
        while (vectorLayers.contains(layerName)) {
            layerName = QString("%1_%2").arg(originalName).arg(counter++);
        }
    }

    // Create new vector layer configuration
    VectorLayer newLayer;
    newLayer.name = layerName;
    newLayer.features = features;
    newLayer.visible = true;
    newLayer.color = QColor(255, 0, 0, 180); // Red with transparency
    newLayer.opacity = 0.8;
    newLayer.width = 2.0;

    vectorLayers[layerName] = newLayer;

    qDebug() << "Added vector layer:" << layerName << "with" << features.size() << "features";
    Console::log("GeoJSON layer imported successfully: " + layerName.toStdString() +
                 " (" + std::to_string(features.size()) + " features)");

    update();  // Refresh display with new layer
}

/*
 * addVectorLayer: Add vector layer with specified features
 * Creates vector layer from provided GeoJSON features array
 */
void GISlib::addVectorLayer(const QString &layerName, const QJsonArray &features) {
    VectorLayer newLayer;
    newLayer.name = layerName;
    newLayer.features = features;
    newLayer.visible = true;
    newLayer.color = QColor(255, 0, 0, 180);
    newLayer.opacity = 0.8;
    newLayer.width = 2.0;

    vectorLayers[layerName] = newLayer;
    qDebug() << "Added vector layer:" << layerName << "with" << features.size() << "features";
    update();  // Refresh display
}

/*
 * clearVectorLayers: Remove all vector layers
 * Clears all GeoJSON layers from the map
 */
void GISlib::clearVectorLayers() {
    vectorLayers.clear();
    qDebug() << "Cleared all vector layers";
    update();  // Refresh display
}

/*
 * toggleVectorLayerVisibility: Toggle visibility of specific vector layer
 * Shows/hides vector layer without removing it
 */
void GISlib::toggleVectorLayerVisibility(const QString &layerName, bool visible) {
    if (vectorLayers.contains(layerName)) {
        vectorLayers[layerName].visible = visible;
        qDebug() << "Vector layer visibility toggled:" << layerName << visible;
        update();  // Refresh display
    } else {
        qWarning() << "Vector layer not found:" << layerName;
    }
}

/*
 * getVectorLayerNames: Get list of all vector layer names
 * Returns keys from vectorLayers map
 */
QList<QString> GISlib::getVectorLayerNames() const {
    return vectorLayers.keys();
}

/*
 * renderVectorLayer: Render vector layer to painter
 * Processes all features in the layer and renders their geometries
 */
void GISlib::renderVectorLayer(QPainter &painter, const VectorLayer &layer) {
    if (!layer.visible || layer.features.isEmpty()) return;

    painter.save();
    painter.setOpacity(layer.opacity);
    painter.setPen(QPen(layer.color, layer.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(layer.color));

    // Render each feature in the layer
    for (const QJsonValue &featureVal : layer.features) {
        if (!featureVal.isObject()) continue;

        QJsonObject feature = featureVal.toObject();
        if (!feature.contains("geometry") || !feature["geometry"].isObject()) continue;

        QJsonObject geometry = feature["geometry"].toObject();
        renderGeometry(painter, geometry, layer.color, layer.width);
    }

    painter.restore();
}

/*
 * renderGeometry: Render specific GeoJSON geometry type
 * Handles Point, LineString, Polygon, and Multi* geometry types
 */
/*
 * renderGeometry: Render specific GeoJSON geometry type
 * Handles Point, LineString, Polygon, and Multi* geometry types
 * FIXED: Correctly distinguishes between single and multi geometries
 */
void GISlib::renderGeometry(QPainter &painter, const QJsonObject &geometry,
                            const QColor &color, double width) {
    QString type = geometry["type"].toString().toLower();

    // Point geometry (single point)
    if (type == "point") {
        QJsonValue coordinatesValue = geometry["coordinates"];
        if (coordinatesValue.isArray()) {
            QJsonArray coordinates = coordinatesValue.toArray();
            if (coordinates.size() >= 2) {
                QPointF canvasPos = jsonCoordinateToCanvas(coordinates);
                painter.setBrush(color);
                painter.setPen(QPen(color, 2));
                painter.drawEllipse(canvasPos, 4, 4);  // Draw point marker
            }
        }
    }
    // MultiPoint geometry (multiple points)
    else if (type == "multipoint") {
        QJsonValue coordinatesValue = geometry["coordinates"];
        if (coordinatesValue.isArray()) {
            QJsonArray points = coordinatesValue.toArray();
            for (const QJsonValue &pointVal : points) {
                if (pointVal.isArray()) {
                    QPointF canvasPos = jsonCoordinateToCanvas(pointVal.toArray());
                    painter.setBrush(color);
                    painter.setPen(QPen(color, 2));
                    painter.drawEllipse(canvasPos, 4, 4);  // Draw point marker
                }
            }
        }
    }
    // LineString geometry (single line)
    else if (type == "linestring") {
        QJsonValue coordArrayValue = geometry["coordinates"];
        if (coordArrayValue.isArray()) {
            QJsonArray coords = coordArrayValue.toArray();

            QPolygonF polyline;
            for (const QJsonValue &coordVal : coords) {
                if (coordVal.isArray()) {
                    QPointF canvasPos = jsonCoordinateToCanvas(coordVal.toArray());
                    polyline << canvasPos;
                }
            }

            if (polyline.size() > 1) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(color, width));
                painter.drawPolyline(polyline);  // Draw line
            }
        }
    }
    // MultiLineString geometry (multiple lines)
    else if (type == "multilinestring") {
        QJsonValue coordArraysValue = geometry["coordinates"];
        if (coordArraysValue.isArray()) {
            QJsonArray lineStrings = coordArraysValue.toArray();

            for (const QJsonValue &lineStringVal : lineStrings) {
                if (!lineStringVal.isArray()) continue;
                QJsonArray coords = lineStringVal.toArray();

                QPolygonF polyline;
                for (const QJsonValue &coordVal : coords) {
                    if (coordVal.isArray()) {
                        QPointF canvasPos = jsonCoordinateToCanvas(coordVal.toArray());
                        polyline << canvasPos;
                    }
                }

                if (polyline.size() > 1) {
                    painter.setBrush(Qt::NoBrush);
                    painter.setPen(QPen(color, width));
                    painter.drawPolyline(polyline);  // Draw line
                }
            }
        }
    }
    else if (type == "polygon") {
        QJsonValue coordArraysValue = geometry["coordinates"];
        if (coordArraysValue.isArray()) {
            QJsonArray rings = coordArraysValue.toArray();
            for (const QJsonValue &ringVal : rings) {
                if (!ringVal.isArray()) continue;
                QJsonArray coords = ringVal.toArray();

                QPolygonF polygon;
                for (const QJsonValue &coordVal : coords) {
                    if (coordVal.isArray()) {
                        QPointF canvasPos = jsonCoordinateToCanvas(coordVal.toArray());
                        polygon << canvasPos;
                    }
                }
                if (polygon.size() > 2) {
                    painter.setBrush(QBrush(color, Qt::Dense4Pattern));
                    painter.setPen(QPen(color, width));
                    painter.drawPolygon(polygon);
                }
            }
        }
    }
    // MultiPolygon geometry (multiple polygons)
    else if (type == "multipolygon") {
        QJsonValue coordArraysValue = geometry["coordinates"];
        if (coordArraysValue.isArray()) {
            QJsonArray polygons = coordArraysValue.toArray();

            for (const QJsonValue &polygonVal : polygons) {
                if (!polygonVal.isArray()) continue;
                QJsonArray rings = polygonVal.toArray();

                for (const QJsonValue &ringVal : rings) {
                    if (!ringVal.isArray()) continue;
                    QJsonArray coords = ringVal.toArray();

                    QPolygonF polygon;
                    for (const QJsonValue &coordVal : coords) {
                        if (coordVal.isArray()) {
                            QPointF canvasPos = jsonCoordinateToCanvas(coordVal.toArray());
                            polygon << canvasPos;
                        }
                    }

                    if (polygon.size() > 2) {
                        painter.setBrush(QBrush(color, Qt::Dense4Pattern));  // Pattern fill
                        painter.setPen(QPen(color, width));
                        painter.drawPolygon(polygon);  // Draw polygon
                    }
                }
            }
        }
    }
}

/*
 * jsonCoordinateToCanvas: Convert JSON coordinate to canvas coordinates
 * Extracts longitude/latitude from JSON array and converts to pixel coordinates
 */
QPointF GISlib::jsonCoordinateToCanvas(const QJsonArray &coord) {
    if (coord.size() < 2) return QPointF(0, 0);

    double lon = coord[0].toDouble(); // First element is longitude
    double lat = coord[1].toDouble(); // Second element is latitude
    return geoToCanvas(lat, lon);     // Convert to canvas coordinates
}

/*
 * latLonToCustomXY: Convert latitude/longitude to custom coordinate system
 * Uses QGIS coordinate transformation for custom projections
 */
QPointF GISlib::latLonToCustomXY(double lat, double lon) {
    QgsCoordinateReferenceSystem srcCrs("EPSG:4326");  // WGS84 source
    QgsCoordinateTransform transform(srcCrs, customCrs, QgsProject::instance());
    try {
        QgsPointXY point(lon, lat);
        QgsPointXY transformed = transform.transform(point);
        return QPointF(transformed.x(), transformed.y());  // Return custom coordinates
    } catch (QgsCsException &e) {
        qDebug() << "Transformation error:" << e.what();
        return QPointF(0, 0);  // Return origin on error
    }
}

/*
 * customXYToLatLon: Convert custom coordinates to latitude/longitude
 * Inverse transformation from custom coordinate system to WGS84
 */
QPointF GISlib::customXYToLatLon(double x, double y) {
    QgsCoordinateReferenceSystem destCrs("EPSG:4326");  // WGS84 destination
    QgsCoordinateTransform transform(customCrs, destCrs, QgsProject::instance());
    try {
        QgsPointXY point(x, y);
        QgsPointXY transformed = transform.transform(point);
        return QPointF(transformed.y(), transformed.x()); // Return as (latitude, longitude)
    } catch (QgsCsException &e) {
        qDebug() << "Reverse transformation error:" << e.what();
        return QPointF(0, 0);  // Return origin on error
    }
}
void GISlib::loadAirbaseData() {
    if (!airbasePositions.empty()) return;

    airbasePositions = {
        {77.4301, 28.7172},  // Hindon
        {76.7770, 30.3020},  // Ambala
        {75.3800, 31.5400},  // Adampur
        {70.0420, 22.5270},  // Jamnagar
        {73.0219, 26.2842},  // Jodhpur
        {78.1670, 26.2530},  // Gwalior
        {78.0133, 27.1556},  // Agra
        {79.4200, 28.3670},  // Bareilly
        {74.9830, 33.8830},  // Awantipur
        {77.6270, 34.0827},  // Leh
        {74.7833, 34.0833},  // Srinagar
        {73.9200, 18.5800},  // Pune
        {77.1500, 10.6750},  // Sulur
        {76.9167, 8.4833},   // Thiruvananthapuram
        {94.4670, 26.6170},  // Jorhat
        {87.2830, 22.1500},  // Kalaikunda
        {88.3500, 22.7500},  // Barrackpore
        {89.3500, 26.7500},  // Hasimara
        {92.8167, 9.1500},   // Car Nicobar
        {70.0420, 22.5270},   // Jamnagar (example duplicate)

        // Pakistan Airbases [web:1, web:3]
        {73.0992, 33.6027},  // PAF Base Mushaf, Sargodha, Punjab
        {71.9789, 33.8730},  // PAF Base Minhas, Kamra, Punjab
        {73.0500, 31.3667},  // PAF Base Rafiqui, Shorkot, Punjab
        {71.5200, 33.9667},  // PAF Base Peshawar, Khyber Pakhtunkhwa
        {67.0333, 24.8936},  // PAF Base Masroor, Karachi, Sindh
        {72.2667, 30.1833},  // PAF Base Multan, Punjab
        {74.4036, 31.6119},  // PAF Base Lahore, Punjab
        {70.8967, 30.2033},  // PAF Base Dera Ghazi Khan, Punjab
        {66.9375, 25.2764},  // PAF Base Samungli, Quetta, Balochistan
        {69.2667, 28.9667}   // PAF Base Shahbaz, Jacobabad, Sindh
    };

    Console::log("Loaded " + std::to_string(airbasePositions.size()) + " airbases");
}
void GISlib::drawAirbases(QPainter& painter) {
    if (!showAirbases || airbasePositions.empty() || airbasePixmap.isNull())
        return;

    painter.save();

    const int iconSize = 20;
    QPixmap scaledIcon = airbasePixmap.scaled(
        iconSize, iconSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    for (const auto& pos : airbasePositions) {
        double lon = pos.first;
        double lat = pos.second;

        QPointF canvasPos = geoToCanvas(lat, lon);
        painter.drawPixmap(
            canvasPos.x() - iconSize / 2.0,
            canvasPos.y() - iconSize / 2.0,
            scaledIcon
            );
    }

    painter.restore();
}
void GISlib::toggleAirbaseLayer() {
    if (!showAirbases) {
        loadAirbaseData();
        showAirbases = true;
        Console::log("Airbase layer enabled");
    } else {
        showAirbases = false;
        Console::log("Airbase layer disabled");
    }
    update();
}
void GISlib::renderAirbases(QPainter& painter)
{
    if (!showAirbases || airbasePositions.empty() || airbasePixmap.isNull())
        return;

    const int iconSize = 20;
    static QPixmap cachedScaledIcon =
        airbasePixmap.scaled(iconSize, iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);

    for (const auto& pos : airbasePositions) {
        QPointF canvasPos = geoToCanvas(pos.second, pos.first);
        painter.drawPixmap(
            canvasPos.x() - iconSize / 2,
            canvasPos.y() - iconSize / 2,
            cachedScaledIcon
            );
    }
}
void GISlib::removeVectorLayer(const QString& layerName) {
    // remove from whatever map/list stores imported geojson layers
    vectorLayers.remove(layerName);   // adjust to match your actual variable name
    update(); // or repaint if needed
}
void GISlib::importVectorLayer(const QString &filePath)
{
    QgsVectorLayer *layer = new QgsVectorLayer(filePath, "temp", "ogr");
    if (!layer->isValid()) {
        Console::error("Failed to load: " + filePath.toStdString());
        delete layer;
        return;
    }

    QJsonArray featuresArray;
    QgsFeatureIterator it = layer->getFeatures();
    QgsFeature feat;
    while (it.nextFeature(feat)) {
        QJsonObject featureObj;
        featureObj["type"] = "Feature";

        QgsGeometry geom = feat.geometry();
        if (geom.isNull()) {
            continue; // skip features without geometry
        }

        // Get geometry type as Qgis::GeometryType
        Qgis::GeometryType geomType = geom.type();  // simpler: geom.type()
        QJsonObject geomObj;

        if (geomType == Qgis::GeometryType::Point) {
            geomObj["type"] = "Point";
            QgsPointXY pt = geom.asPoint();
            QJsonArray coords;
            coords.append(pt.x());
            coords.append(pt.y());
            geomObj["coordinates"] = coords;
        }
        else if (geomType == Qgis::GeometryType::Line) {
            geomObj["type"] = "LineString";
            QgsPolylineXY line = geom.asPolyline();
            QJsonArray coordsArray;
            for (const QgsPointXY &p : line) {
                QJsonArray coord;
                coord.append(p.x());
                coord.append(p.y());
                coordsArray.append(coord);
            }
            geomObj["coordinates"] = coordsArray;
        }
        else if (geomType == Qgis::GeometryType::Polygon) {
            geomObj["type"] = "Polygon";
            QgsPolygonXY poly = geom.asPolygon();
            QJsonArray ringsArray;
            for (const QgsPolylineXY &ring : poly) {
                QJsonArray ringCoords;
                for (const QgsPointXY &p : ring) {
                    QJsonArray coord;
                    coord.append(p.x());
                    coord.append(p.y());
                    ringCoords.append(coord);
                }
                ringsArray.append(ringCoords);
            }
            geomObj["coordinates"] = ringsArray;
        }
        else {
            // unsupported geometry type (MultiPoint, MultiLineString, MultiPolygon, etc.)
            continue;
        }

        featureObj["geometry"] = geomObj;

        // Add attributes as properties
        QJsonObject properties;
        const QgsFields fields = layer->fields();
        for (int i = 0; i < fields.count(); ++i) {
            QVariant value = feat.attribute(i);
            if (value.isValid()) {
                properties[fields[i].name()] = QJsonValue::fromVariant(value);
            }
        }
        featureObj["properties"] = properties;

        featuresArray.append(featureObj);
    }

    delete layer;

    if (!featuresArray.isEmpty()) {
        QString layerName = QFileInfo(filePath).completeBaseName();
        addVectorLayer(layerName, featuresArray);
    }

    update();
}
