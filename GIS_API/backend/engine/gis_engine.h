#ifndef GIS_ENGINE_H
#define GIS_ENGINE_H

#include <QObject>
#include <QPointF>
#include <QSize>
#include <QMap>
#include <QPixmap>
#include <QStringList>

class GISEngine : public QObject {
    Q_OBJECT
    
public:
    static GISEngine* instance();
    ~GISEngine();
    
    // Map Control
    void setCenter(double lat, double lon);
    void setZoom(int level);
    void zoomIn();
    void zoomOut();
    void setWidgetSize(const QSize& size);
    void setLayers(const QStringList& layers);
    
    // Coordinate Conversion
    QPointF geoToCanvas(double lat, double lon) const;
    QPointF canvasToGeo(const QPointF& pos) const;
    
    // Tile Helpers
    double lonToX(double lon, int zoom) const;
    double latToY(double lat, int zoom) const;
    double xToLon(double x, int zoom) const;
    double yToLat(double y, int zoom) const;
    
    // Tile Management
    void loadOfflineTiles();
    QPixmap getTile(int zoom, int x, int y) const;
    bool hasTile(int zoom, int x, int y) const;
    void clearCache();
    int getTileCount() const;
    QString getTileKey(int z, int x, int y) const;
    void requestTile(int x, int y, int z);
    
    // Getters
    double getCenterLat() const { return m_centerLat; }
    double getCenterLon() const { return m_centerLon; }
    int getZoom() const { return m_zoom; }
    QSize getMapSize() const { return m_widgetSize; }
    
signals:
    void mapChanged();
    void zoomChanged(int newZoom);
    void centerChanged(double lat, double lon);
    void tilesLoaded(int count);
    
private:
    explicit GISEngine(QObject* parent = nullptr);
    static GISEngine* m_instance;
    double m_centerLat;
    double m_centerLon;
    int m_zoom;
    QSize m_widgetSize;
    QStringList m_activeLayers;
    QMap<QString, QPixmap> m_tileCache;
    QString m_tilesPath;
    int m_tileSize;
};

#endif // GIS_ENGINE_H
