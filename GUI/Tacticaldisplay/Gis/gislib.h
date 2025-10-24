

/*
 * GISlib Header File
 * This file defines the GISlib class which provides geographic information system
 * functionality including map rendering, coordinate transformations, GeoJSON support,
 * and distance measurement capabilities.
 */

#ifndef GISLIB_H
#define GISLIB_H

// Include necessary Qt and GIS libraries
#include "gisnetwork.h"
#include "qjsonarray.h"
#include "qwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <qgscoordinatereferencesystem.h>

// Declare metatypes for event forwarding between threads
Q_DECLARE_METATYPE(QMouseEvent*)
Q_DECLARE_METATYPE(QKeyEvent*)

/*
 * GISlib Class
 * A QWidget-based GIS library that provides map rendering, coordinate transformations,
 * GeoJSON support, and various GIS operations including distance measurement.
 */
class GISlib : public QWidget {
    Q_OBJECT

public:
    /*
     * Constructor
     * Initializes the GIS library with default settings and network components
     */
    GISlib(QWidget *parent = nullptr);

    // Public member variables for mouse coordinates
    QString mouseLat, mouseLon;

    /*
     * Core GIS Functionality Methods
     */

    /* Search for a place by query string */
    void serachPlace(const QString& query);

    /* Set active map layers for rendering */
    void setLayers(const QStringList& layerNames);

    /* Set map center to specified geographic coordinates */
    void setCenter(double lat, double lon);

    /* Set zoom level for map display */
    void setZoom(int level);

    /* Add custom map layer with specified parameters and opacity */
    void addCustomMap(const QString& layerName, int zoomMin, int zoomMax,
                      const QString& tileUrl, qreal opacity = 1.0);

    /*
     * Coordinate Transformation Methods
     */

    /* Convert longitude to tile X coordinate at specified zoom level */
    double lonToX(double lon, int zoom);

    /* Convert latitude to tile Y coordinate at specified zoom level */
    double latToY(double lat, int zoom);

    /* Convert geographic coordinates to canvas pixel coordinates */
    QPointF geoToCanvas(double lat, double lon);

    /* Convert canvas pixel coordinates to geographic coordinates */
    QPointF canvasToGeo(QPointF p);

    /* Handle mouse wheel events for zooming */
    void wheelEvents(QWheelEvent *event);

    /* Set initial offset for map positioning */
    void setInitialOffset(QPointF offset);

    /* Set coordinate reference system by CRS ID */
    void setCoordinateSystem(const QString& crsId);

    /*
     * Distance Measurement Methods
     */

    /* Start distance measurement mode */
    void startDistanceMeasurement();

    /* End distance measurement mode */
    void endDistanceMeasurement();

    /* Calculate distance between two geographic points (in lon, lat) */
    double calculateDistance(QPointF point1, QPointF point2);

    /* Check if currently in distance measurement mode */
    bool isMeasuringDistance() const { return measuringDistance; }

    /*
     * GeoJSON Layer Management Methods
     */

    /* Import GeoJSON layer from file path */
    void importGeoJsonLayer(const QString &filePath);

    /* Add vector layer with name and GeoJSON features */
    void addVectorLayer(const QString &layerName, const QJsonArray &features);

    /* Clear all vector layers */
    void clearVectorLayers();

    /* Toggle visibility of specific vector layer */
    void toggleVectorLayerVisibility(const QString &layerName, bool visible);

    /* Get list of all vector layer names */
    QList<QString> getVectorLayerNames() const;

    /*
     * Advanced Coordinate System Methods
     */

    /* Set ellipsoidal calculation mode for distance measurements */
    void setEllipsoidal(bool e);

    /* Convert latitude/longitude to custom coordinate system XY */
    QPointF latLonToCustomXY(double lat, double lon);

    /* Convert custom coordinate system XY to latitude/longitude */
    QPointF customXYToLatLon(double x, double y);

    /* Search by geographic coordinates */
    void searchByCoordinates(double latitude, double longitude);

protected:
    /*
     * Drag and Drop Event Handlers
     */
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    /*
     * GIS Library Signals
     * These signals communicate state changes and user interactions to connected components
     */

    /* Emitted when mouse coordinates change with CRS information */
    void mouseCords(double lat, double lon, const QString& crsId);

    /* Emitted when map center changes */
    void centerChanged(double lat, double lon);

    /* Emitted when zoom level changes */
    void zoomChanged(int zoom);

    /* Event forwarding signals for unified event handling */
    void keyPressed(QKeyEvent *event);
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);
    void mouseReleased(QMouseEvent *event);
    void painted(QPaintEvent *event);

    /* Emitted when distance measurement is completed */
    void distanceMeasured(double distance, QPointF startPoint, QPointF endPoint);

    /* Drag and drop event forwarding signals */
    void dragEnterEvents(QDragEnterEvent *event);
    void dragMoveEvents(QDragMoveEvent *event);
    void dropEvents(QDropEvent *event);

private:
    /*
     * Tile Management Private Methods
     */

    /* Get subdomain for tile URL based on tile coordinates */
    QString getSubdomain(int x, int y, const QString& layer);

    /* Generate tile URL for specific layer and coordinates */
    QString tileUrl(const QString& layer, int x, int y, int z);

    /* Generate unique key for tile caching */
    QString getTileKey(const QString& layer, int z, int x, int y);

    /* Request tile from network with retry mechanism */
    void requestTile(const QString& layer, int x, int y, int z, int retries = 3);

    /*
     * Vector Layer Structure and Methods
     */
    struct VectorLayer {
        QString name;           // Layer identifier name
        QJsonArray features;    // GeoJSON feature collection
        bool visible;           // Layer visibility flag
        QColor color;           // Rendering color
        qreal opacity;          // Layer opacity
        double width;           // Line width for rendering
    };

    /* Map of vector layers by name */
    QMap<QString, VectorLayer> vectorLayers;

    /* Render vector layer to painter */
    void renderVectorLayer(QPainter &painter, const VectorLayer &layer);

    /* Parse GeoJSON file into features array */
    void parseGeoJson(const QString &filePath, QJsonArray &features);

    /* Render GeoJSON geometry to painter */
    void renderGeometry(QPainter &painter, const QJsonObject &geometry,
                        const QColor &color, double width);

    /* Convert JSON coordinate to canvas coordinates */
    QPointF jsonCoordinateToCanvas(const QJsonArray &coord);

private:
    /*
     * Internal Data Structures
     */

    /* Marker structure for point annotations */
    struct Marker {
        double lat, lon;    // Geographic coordinates
        QColor color;       // Display color
    };

    /* Custom map layer configuration */
    struct CustomMap {
        int zoomMin;        // Minimum zoom level for display
        int zoomMax;        // Maximum zoom level for display
        QString tileUrl;    // Tile server URL template
        qreal opacity;      // Layer opacity
    };

    /*
     * Private Member Variables
     */

    bool flipkeyaxis = false;           // Coordinate axis flipping flag

    GISNetwork* net;                    // Network manager for tile downloads
    QPixmap networkImage;               // Cached network status image

    bool dragging = false;              // Map dragging state flag
    QStringList activeLayers;           // List of currently active map layers
    QPoint lastMouse;                   // Last mouse position for dragging

    Marker marker;                      // Current marker for display
    QJsonArray geoOutline;              // Geographic outline data
    QList<Marker> markers;              // Collection of map markers

    QHash<QString, QPixmap> tileCache;  // Cache for map tiles
    QSet<QString> pendingTileKeys;      // Set of tiles currently being downloaded
    QHash<QString, int> tileRetries;    // Retry count for failed tile downloads

    QMap<QString, CustomMap> customMaps; // Custom map layer configurations

    double centerLat, centerLon;        // Current map center coordinates
    int pendingTiles = 0;               // Count of pending tile downloads
    int zoom;                           // Current zoom level

    QPointF initialOffset = QPointF(0, 0); // Initial map offset in pixels

    /* Convert tile X coordinate to longitude */
    double xToLon(double x, int zoom);

    /* Convert tile Y coordinate to latitude */
    double yToLat(double y, int zoom);

    /* Convert decimal degrees to Degrees-Minutes-Seconds format */
    QString toDMS(double deg, bool isLat);

    static constexpr int maxCacheSize = 1000; // Maximum tile cache size

    QgsCoordinateReferenceSystem currentCrs; // Current coordinate reference system

    bool measuringDistance = false;     // Distance measurement mode flag
    QPointF measureStartPoint;          // Start point for measurement (lon, lat)
    QPointF measureEndPoint;            // End point for measurement (lon, lat)

    bool ellipsoidal = true;            // Use ellipsoidal calculations flag

    QgsCoordinateReferenceSystem customCrs; // Custom coordinate reference system

public slots:
    /*
     * Public Slots for Network Responses
     */

    /* Handle received tile image data */
    void receiveImage(QString url, QByteArray data);

    /* Handle received place search results */
    void receivePlace(QString url, QByteArray data);

protected:
    /*
     * Protected Event Handlers
     */
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // GISLIB_H
