/* =============================================================================
 * FILE:         elevation_data.h
 * MODULE:       Elevation / Altitude API
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 *
 * DESCRIPTION:  Loads elevation data and provides getAltitude(lat, lon).
 *               - Preferred: GeoTIFF / DEM via GDAL (real height in metres)
 *               - Fallback:  world color/gray image (approximate)
 *
 * PLACEMENT:    GIS_API/backend/elevation/elevation_data.h
 * =============================================================================
 */
#ifndef ELEVATION_DATA_H
#define ELEVATION_DATA_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QSize>
#include <QVector>

class ElevationData : public QObject
{
    Q_OBJECT

public:
    static ElevationData* instance();

    /** Load elevation file. DEM (.tif/.tiff/.hgt/…) via GDAL; else image fallback. */
    Q_INVOKABLE bool loadFile(const QString& path);

    Q_INVOKABLE void clear();

    Q_INVOKABLE bool isLoaded() const { return m_loaded; }
    Q_INVOKABLE bool isDem() const { return m_isDem; }
    Q_INVOKABLE QString filePath() const { return m_path; }
    Q_INVOKABLE QSize imageSize() const;

    /** DEM geographic bounds (degrees). Valid when isDem() is true. */
    Q_INVOKABLE double west()  const { return m_west; }
    Q_INVOKABLE double east()  const { return m_east; }
    Q_INVOKABLE double south() const { return m_south; }
    Q_INVOKABLE double north() const { return m_north; }

    /**
     * Altitude in metres (WGS84 lat/lon).
     * DEM mode: real pixel height from GDAL.
     * Image mode: approximate from brightness.
     * Returns NaN if not loaded or out of bounds.
     */
    Q_INVOKABLE double getAltitude(double lat, double lon) const;

    /** Debug: raw pixel (image mode) or 0. */
    Q_INVOKABLE unsigned int getPixel(double lat, double lon) const;

signals:
    void elevationLoaded(const QString& path);
    void elevationCleared();

private:
    explicit ElevationData(QObject* parent = nullptr);
    static ElevationData* m_instance;

    bool loadWithGdal(const QString& path);
    bool loadWithImage(const QString& path);

    bool latLonToPixelImage(double lat, double lon, int& x, int& y) const;
    bool latLonToPixelDem(double lat, double lon, int& x, int& y) const;
    double colorToHeight(QRgb pixel) const;

    QString m_path;
    bool    m_loaded = false;
    bool    m_isDem  = false;

    QImage  m_image;

    QVector<float> m_heights;
    int     m_cols = 0;
    int     m_rows = 0;
    double  m_west  = -180.0;
    double  m_east  =  180.0;
    double  m_south =  -90.0;
    double  m_north =   90.0;
    double  m_noData = -9999.0;
    bool    m_hasNoData = false;
};

#endif // ELEVATION_DATA_H
