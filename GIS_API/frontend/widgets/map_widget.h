#ifndef MAP_WIDGET_H
#define MAP_WIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QPoint>
#include <QVector>
#include <QKeyEvent>
#include <QJsonObject>
#include "backend/shapes/ishape_type.h"
#include <QSet>


class MapApi;
class ShapeApi;
class MeasureApi;
class ApiMeasureDistanceDialog;
class MapWidget : public QWidget {
    Q_OBJECT

public:
    explicit MapWidget(QWidget* parent = nullptr);
    ~MapWidget();
    void setCenter(double lat, double lon);
    void setZoom(int level);
    void zoomIn();
    void zoomOut();
    double getCenterLat() const;
    double getCenterLon() const;
    double pixelSizeToDegrees(int pixels) const;
    int getZoom() const;
    QPointF geoToCanvas(double lat, double lon) const;
    QPointF canvasToGeo(const QPointF& pos) const;
    void startDrawing(const QString& shapeType);
    void cancelDrawing();
    bool isDrawing() const { return m_isDrawing; }

    QString activeDrawingType() const { return m_isDrawing ? m_drawingType : QString(); }
    void startMeasuring();
    void stopMeasuring();
    void clearMeasurePoints();
    bool isMeasuring() const;
    void addTextAtPosition(const QPointF& geoPos);
    void startPlacingBitmap(const QString& name);
    void startPlacingBitmapImage(const QString& imagePath);
    void cancelPlacingBitmap();
    bool isPlacingBitmap() const { return m_isPlacingBitmap; }

    QString activePlacingBitmap() const { return m_isPlacingBitmap ? m_placingBitmapName : QString(); }
    void selectShape(const QString& shapeId);
    QString selectedShapeId() const { return m_selectedShapeId; }
    void centerOnShape(const QString& shapeId);
    void setCoordSystem(const QString& sys);
    QString coordSystem() const { return m_coordSystem; }
    void startEditingShape(const QString& shapeId);
    void startMovingShape(const QString& shapeId);
    void startRotatingShape(const QString& shapeId);
    void showProperties(const QString& shapeId);
    void deleteShape(const QString& shapeId);
    void copyShape(const QString& shapeId);
    void reloadTiles();
    void startSelectCenter();
    void setGisInteractionEnabled(bool enabled);
    bool isGisInteractionEnabled() const { return m_gisInteractionEnabled; }
    void zoomToCanvasRect(const QRect& canvasRect);
    void setSelectedShapes(const QStringList& shapeIds);
    QStringList selectedShapeIds() const;
    // void goToLocation(double lat, double lon);
    void clearSearchMarker();
    void goToLocation(double lat, double lon, bool showMarker = true);



signals:
    void shapeSelectionChanged(const QString& shapeId);
    void measurePointsChanged(const QVector<QPointF>& geoPoints,
                              const QVector<double>& segmentMeters);
    void measuringChanged(bool active);
    void mapChanged();
    void gisInteractionEnabledChanged(bool enabled);
    void placingBitmapChanged(const QString& activeName);
    void drawingTypeChanged(const QString& activeType);
    void coordSystemChanged(const QString& sys);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onMapChanged();
    void onTilesLoaded(int count);

private:
    MapApi* m_mapApi;
    ShapeApi* m_shapeApi;
    double m_centerLat;
    double m_centerLon;
    int m_zoom;
    bool m_dragging;
    QPoint m_lastDragPos;
    QPoint m_lastMousePos;
    int m_tileSize;
    QMap<QString, QPixmap> m_tileCache;
    bool m_isDrawing;
    QString m_drawingType;
    QVector<QPointF> m_drawingPoints;
    bool m_isDraggingShape;

    QString m_selectedShapeId;
    QSet<QString> m_selectedShapeIds;
    QString m_editingShapeId;
    QString m_movingShapeId;
    QString m_rotatingShapeId;

    bool m_isMovingShape;
    QPoint m_moveStartPos;

    QJsonObject m_shapeDataAtStart;

    bool m_isRotatingShape;
    QPoint m_rotateStartPos;
    double m_rotateStartAngle;

    MeasureApi* m_measureApi = nullptr;
    ApiMeasureDistanceDialog* m_measureDialog = nullptr;
    QVector<QVector<QPointF>> m_completedMeasureChains;
    void drawTiles(QPainter& painter);
    void drawShapes(QPainter& painter);
    void drawDrawingPreview(QPainter& painter);
    void drawRotateHandle(QPainter& painter);
    void drawMeasureOverlay(QPainter& painter);
    void drawMeasureChain(QPainter& painter, const QVector<QPointF>& pts, bool active);
    void drawAirbases(QPainter& painter);
    void loadOfflineTiles();
    void finishDrawing();
    void handleDrawingClick(QMouseEvent* event);
    void showContextMenu(QMouseEvent* event);
    QString getTileKey(int z, int x, int y) const;
    double lonToX(double lon, int z) const;
    double latToY(double lat, int z) const;
    double xToLon(double x, int z) const;
    double yToLat(double y, int z) const;
    int m_editPointIndex = -1;
    int findNearestEditPoint(const QString& shapeId, const QPointF& geoPos) const;
    void drawEditHandles(QPainter& painter);
    void ensureMeasureDialog();
    void syncMeasureDialog();
    bool m_isPlacingBitmap = false;
    QString m_placingBitmapName;
    QString m_placingBitmapPath;
    QString findTextAtCanvas(const QPoint& canvasPos) const;
    void pasteShapeAt(const QPointF& geoPos);
    QJsonObject m_clipboardShape;
    QString m_coordSystem = "EPSG:4326";
    QString formatCoord(double lat, double lon) const;
    void drawHistoryPreview(QPainter& painter);
    QString findBitmapAtCanvas(const QPoint& canvasPos) const;
    void drawMapOverlay(QPainter& painter);
    double metersPerPixel() const;
    IShapeType* m_currentShapeType = nullptr;
    bool m_isSelectingCenter = false;
    // double m_markedCenterLat = 20.5937;
    // double m_markedCenterLon = 78.9629;

    // bool   m_hasSearchMarker = false;
    // double m_searchMarkerLat = 0.0;
    // double m_searchMarkerLon = 0.0;

    // void drawCenterMarker(QPainter& painter);

    double m_markedCenterLat = 20.5937;   // sirf Select Center (official)
    double m_markedCenterLon = 78.9629;

    // Search temporary — Select Center se bilkul alag
    bool   m_hasSearchMarker = false;
    double m_searchMarkerLat = 0.0;
    double m_searchMarkerLon = 0.0;

    void drawCenterMarker(QPainter& painter);
    void drawSearchMarker(QPainter& painter);
    // void drawSearchMarker(QPainter& painter);
    bool m_gisInteractionEnabled = false;
    bool  m_boxZoomPending = false;
    bool  m_isBoxZooming   = false;
    QPoint m_boxZoomStart;
    QPoint m_boxZoomCurrent;
    void drawBoxZoomOverlay(QPainter& painter);
    bool m_boxZoomFollowMouse = false;
    // existing private section mein:
    QPixmap m_rasterBasemap;
    QString m_rasterBasemapPath;
    bool    m_hasRasterBasemap = false;
    // default geo bounds for image (lon/lat)
    double  m_rasterWest  = -180.0;
    double  m_rasterEast  =  180.0;
    double  m_rasterSouth =  -85.0;
    double  m_rasterNorth =   85.0;
    void loadRasterBasemap(const QString& imagePath);
    // MBTiles
    bool    m_hasMbtiles = false;
    QString m_mbtilesPath;
    int     m_mbtilesMinZoom = 0;
    int     m_mbtilesMaxZoom = 18;

    void loadMbtilesBasemap(const QString& path);
    QPixmap fetchMbtilesTile(int z, int x, int y) const;
    bool loadGdalBasemap(const QString& path);
    bool extractGeoBoundsFromGdal(const QString& path,
                                  double& west, double& east,
                                  double& south, double& north);
    bool shapeLayerIsEditable(const QString& shapeId) const;


};

#endif // MAP_WIDGET_H
