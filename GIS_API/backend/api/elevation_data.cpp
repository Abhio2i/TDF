/* =============================================================================
 * FILE:         elevation_data.cpp
 * MODULE:       Elevation / Altitude API
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 *
 * PLACEMENT:    GIS_API/backend/elevation/elevation_data.cpp
 *
 * DEM:   GDAL GeoTIFF / HGT / etc. → real metres
 * Image: JPG/PNG world ramp → approximate (demo only)
 * =============================================================================
 */
#include "elevation_data.h"

#include <QFileInfo>
#include <QDebug>
#include <limits>
#include <cmath>
#include <algorithm>

#include <gdal_priv.h>
#include <cpl_conv.h>

static void ensureGdal()
{
    static bool done = false;
    if (!done) {
        GDALAllRegister();
        done = true;
    }
}

static bool isDemExtension(const QString& path)
{
    const QString lower = path.toLower();
    return lower.endsWith(QLatin1String(".tif"))
           || lower.endsWith(QLatin1String(".tiff"))
           || lower.endsWith(QLatin1String(".hgt"))
           || lower.endsWith(QLatin1String(".img"))
           || lower.endsWith(QLatin1String(".dem"))
           || lower.endsWith(QLatin1String(".vrt"))
           || lower.endsWith(QLatin1String(".asc"))
           || lower.endsWith(QLatin1String(".bil"))
           || lower.endsWith(QLatin1String(".dt0"))
           || lower.endsWith(QLatin1String(".dt1"))
           || lower.endsWith(QLatin1String(".dt2"))
           || lower.endsWith(QLatin1String(".dtd"))
           || lower.endsWith(QLatin1String(".adf"))
           || lower.endsWith(QLatin1String(".bin"));
}

ElevationData* ElevationData::m_instance = nullptr;

ElevationData* ElevationData::instance()
{
    if (!m_instance)
        m_instance = new ElevationData();
    return m_instance;
}

ElevationData::ElevationData(QObject* parent)
    : QObject(parent)
{

}

QSize ElevationData::imageSize() const
{
    if (m_isDem)
        return QSize(m_cols, m_rows);
    return m_image.size();
}

bool ElevationData::loadFile(const QString& path)
{
    clear();

    if (path.isEmpty() || !QFileInfo::exists(path)) {
        qWarning() << "ElevationData: file not found" << path;
        return false;
    }

    if (isDemExtension(path)) {
        if (loadWithGdal(path))
            return true;
        qWarning() << "ElevationData: GDAL load failed for" << path;
        return false;
    }

    return loadWithImage(path);
}

bool ElevationData::loadWithGdal(const QString& path)
{
    ensureGdal();

    GDALDataset* ds = static_cast<GDALDataset*>(
        GDALOpen(path.toUtf8().constData(), GA_ReadOnly));
    if (!ds) {
        qWarning() << "ElevationData: GDALOpen failed" << path
                   << CPLGetLastErrorMsg();
        return false;
    }

    double gt[6];
    if (ds->GetGeoTransform(gt) != CE_None) {
        qWarning() << "ElevationData: no geotransform" << path;
        GDALClose(ds);
        return false;
    }

    const int srcCols = ds->GetRasterXSize();
    const int srcRows = ds->GetRasterYSize();
    if (srcCols <= 0 || srcRows <= 0) {
        qWarning() << "ElevationData: invalid raster size" << srcCols << srcRows;
        GDALClose(ds);
        return false;
    }

    // Geographic bounds from full source geotransform
    m_west  = gt[0];
    m_north = gt[3];
    m_east  = gt[0] + srcCols * gt[1] + srcRows * gt[2];
    m_south = gt[3] + srcCols * gt[4] + srcRows * gt[5];
    if (m_west > m_east)   std::swap(m_west, m_east);
    if (m_south > m_north) std::swap(m_south, m_north);

    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) {
        qWarning() << "ElevationData: no raster band 1";
        GDALClose(ds);
        return false;
    }

    int hasNoData = FALSE;
    m_noData = band->GetNoDataValue(&hasNoData);
    m_hasNoData = (hasNoData != 0);

    constexpr qint64 kMaxPixels = 50LL * 1000 * 1000;
    const qint64 srcPixels = static_cast<qint64>(srcCols) * static_cast<qint64>(srcRows);

    int dstCols = srcCols;
    int dstRows = srcRows;
    if (srcPixels > kMaxPixels) {
        const double scale = std::sqrt(static_cast<double>(kMaxPixels) / static_cast<double>(srcPixels));
        dstCols = std::max(1, static_cast<int>(srcCols * scale));
        dstRows = std::max(1, static_cast<int>(srcRows * scale));
        qWarning() << "ElevationData: DEM too large" << srcCols << "x" << srcRows
                   << "→ downsampling to" << dstCols << "x" << dstRows;
    }

    const qint64 dstPixels = static_cast<qint64>(dstCols) * static_cast<qint64>(dstRows);
    if (dstPixels <= 0 || dstPixels > kMaxPixels * 2) {
        qWarning() << "ElevationData: refused alloc pixels" << dstPixels;
        GDALClose(ds);
        return false;
    }

    QVector<float> buffer;
    try {
        buffer.resize(static_cast<int>(dstPixels));
    } catch (...) {
        qWarning() << "ElevationData: out of memory for" << dstCols << "x" << dstRows;
        GDALClose(ds);
        return false;
    }
    if (buffer.size() != static_cast<int>(dstPixels)) {
        qWarning() << "ElevationData: QVector resize failed" << dstPixels;
        GDALClose(ds);
        return false;
    }

    // RasterIO can resample src → dst size
    const CPLErr err = band->RasterIO(
        GF_Read,
        0, 0, srcCols, srcRows,
        buffer.data(),
        dstCols, dstRows,
        GDT_Float32,
        0, 0);

    GDALClose(ds);

    if (err != CE_None) {
        qWarning() << "ElevationData: RasterIO failed" << path;
        return false;
    }

    m_heights = std::move(buffer);
    m_cols   = dstCols;
    m_rows   = dstRows;
    m_path   = path;
    m_loaded = true;
    m_isDem  = true;
    m_image  = QImage();

    qDebug() << "ElevationData: DEM loaded" << path
             << "size" << m_cols << "x" << m_rows
             << "(source" << srcCols << "x" << srcRows << ")"
             << "bounds W/E/S/N" << m_west << m_east << m_south << m_north;

    emit elevationLoaded(path);
    return true;
}

bool ElevationData::loadWithImage(const QString& path)
{
    QImage img(path);
    if (img.isNull()) {
        qWarning() << "ElevationData: failed to load image" << path;
        return false;
    }

    if (img.format() != QImage::Format_RGB32
        && img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_RGB32);
    }

    m_image  = img;
    m_path   = path;
    m_loaded = true;
    m_isDem  = false;
    m_heights.clear();
    m_cols = m_rows = 0;

    qDebug() << "ElevationData: image (approx) loaded" << path
             << "size" << m_image.width() << "x" << m_image.height();

    emit elevationLoaded(path);
    return true;
}

void ElevationData::clear()
{
    m_image = QImage();
    m_heights.clear();
    m_path.clear();
    m_loaded = false;
    m_isDem  = false;
    m_cols = m_rows = 0;
    m_hasNoData = false;
    emit elevationCleared();
}

bool ElevationData::latLonToPixelDem(double lat, double lon, int& x, int& y) const
{
    if (!m_loaded || !m_isDem || m_heights.isEmpty() || m_cols <= 0 || m_rows <= 0)
        return false;

    if (lon < m_west || lon > m_east || lat < m_south || lat > m_north)
        return false;

    const double fx = (lon - m_west) / (m_east - m_west) * (m_cols - 1);
    const double fy = (m_north - lat) / (m_north - m_south) * (m_rows - 1);

    x = static_cast<int>(fx + 0.5);
    y = static_cast<int>(fy + 0.5);

    if (x < 0) x = 0;
    if (x >= m_cols) x = m_cols - 1;
    if (y < 0) y = 0;
    if (y >= m_rows) y = m_rows - 1;
    return true;
}

bool ElevationData::latLonToPixelImage(double lat, double lon, int& x, int& y) const
{
    if (!m_loaded || m_isDem || m_image.isNull())
        return false;

    if (lat < -90.0)  lat = -90.0;
    if (lat >  90.0)  lat =  90.0;
    if (lon < -180.0) lon = -180.0;
    if (lon >  180.0) lon =  180.0;

    const int w = m_image.width();
    const int h = m_image.height();

    x = static_cast<int>(((lon / 180.0) + 1.0) * 0.5 * w);
    y = static_cast<int>(((90.0 - lat) / 180.0) * h);

    if (x < 0) x = 0;
    if (x >= w) x = w - 1;
    if (y < 0) y = 0;
    if (y >= h) y = h - 1;
    return true;
}

double ElevationData::colorToHeight(QRgb pixel) const
{
    const int gray = qGray(pixel);
    constexpr double minH = 0.0;
    constexpr double maxH = 9000.0;
    return minH + (gray / 255.0) * (maxH - minH);
}

double ElevationData::getAltitude(double lat, double lon) const
{
    if (!m_loaded)
        return std::numeric_limits<double>::quiet_NaN();

    if (m_isDem) {
        int x = 0, y = 0;
        if (!latLonToPixelDem(lat, lon, x, y))
            return std::numeric_limits<double>::quiet_NaN();

        const float h = m_heights[y * m_cols + x];
        if (m_hasNoData && (h == static_cast<float>(m_noData) || std::isnan(h)))
            return std::numeric_limits<double>::quiet_NaN();
        return static_cast<double>(h);
    }

    int x = 0, y = 0;
    if (!latLonToPixelImage(lat, lon, x, y))
        return std::numeric_limits<double>::quiet_NaN();
    return colorToHeight(m_image.pixel(x, y));
}

unsigned int ElevationData::getPixel(double lat, double lon) const
{
    if (!m_loaded || m_isDem)
        return 0;
    int x = 0, y = 0;
    if (!latLonToPixelImage(lat, lon, x, y))
        return 0;
    return static_cast<unsigned int>(m_image.pixel(x, y));
}
