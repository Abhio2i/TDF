
#ifndef MAP_API_H
#define MAP_API_H

#include <QObject>
#include <QPointF>
#include <QSize>
#include <QPixmap>
#include <QStringList>
#include <QMap>
#include <QString>
#include <QVector>

class GISEngine;


struct TileSourceInfo {
    QString id;
    QString folderName;
    QString displayName;
    QString iconPath;
    int     priority = 100;
    int     maxNativeZoom = 18;
};

class MapApi : public QObject {
    Q_OBJECT

public:
    static MapApi* instance();

    // Map Control
    void setCenter(double lat, double lon);
    void setZoom(int level);
    void zoomIn();
    void zoomOut();
    void setMapSize(int width, int height);
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

    // ---------- Tile sources ----------
    Q_INVOKABLE void registerTileSource(const QString& id,
                                        const QString& folderName,
                                        const QString& displayName = QString(),
                                        const QString& iconPath = QString(),
                                        int priority = 100,
                                        int maxNativeZoom = 18);

    Q_INVOKABLE bool unregisterTileSource(const QString& id);

    Q_INVOKABLE bool setActiveTileSource(const QString& id);
    Q_INVOKABLE QString activeTileSource() const;
    Q_INVOKABLE QString activeTilesPath() const;
    Q_INVOKABLE QStringList tileSourceIds() const;
    Q_INVOKABLE TileSourceInfo tileSourceInfo(const QString& id) const;
    Q_INVOKABLE QVector<TileSourceInfo> allTileSources() const;

    // Getters
    double getCenterLat() const;
    double getCenterLon() const;
    int getZoom() const;
    QSize getMapSize() const;

signals:
    void mapChanged();
    void zoomChanged(int newZoom);
    void centerChanged(double lat, double lon);
    void tilesLoaded(int count);
    void tileSourceChanged(const QString& id);
    void tileSourceRegistered(const QString& id);
    void tileSourceUnregistered(const QString& id);
    void sourcesChanged();

private:
    explicit MapApi(QObject* parent = nullptr);
    ~MapApi();

    static MapApi* m_instance;
    GISEngine* m_engine;

    QMap<QString, TileSourceInfo> m_tileSources;
    QString m_activeTileSource;
};

#endif // MAP_API_H
