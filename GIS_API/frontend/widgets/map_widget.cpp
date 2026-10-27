#include "frontend/widgets/map_widget.h"
#include "backend/api/map_api.h"
#include "backend/api/shape_api.h"
#include "backend/api/measure_api.h"
#include "backend/api/airbase_api.h"
#include "backend/api/layer_api.h"
#include "measure_distance_dialog.h"
#include "backend/api/shape_history_api.h"
#include "backend/shapes/shape_type_registry.h"
#include "backend/shapes/builtin_shape_types.h"
#include "GIS_API/backend/api/elevation_data.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <cmath>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QColorDialog>
#include <QLabel>
#include <QPushButton>
#include <QVector2D>
#include <QJsonArray>
#include "ui_action_api.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

MapWidget::MapWidget(QWidget* parent)
    : QWidget(parent)
    , m_mapApi(MapApi::instance())
    , m_shapeApi(ShapeApi::instance())
    , m_centerLat(20.5937)
    , m_centerLon(78.9629)
    , m_zoom(1)
    , m_dragging(false)
    , m_tileSize(256)
    , m_isDrawing(false)
    , m_drawingType("")
    , m_isDraggingShape(false)
    , m_isMovingShape(false)
    , m_isRotatingShape(false)
    , m_rotateStartAngle(0.0)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #1a1a2e;");
    m_mapApi->setMapSize(width(), height());
    connect(m_mapApi, &MapApi::mapChanged, this, &MapWidget::onMapChanged);
    connect(m_mapApi, &MapApi::tilesLoaded, this, &MapWidget::onTilesLoaded);
    m_mapApi->setCenter(m_centerLat, m_centerLon);
    m_mapApi->setZoom(m_zoom);
    connect(LayerApi::instance(), &LayerApi::layerVisibilityChanged,
            this, [this](const QString&, bool) { update(); });
    m_measureApi = MeasureApi::instance();
    connect(m_measureApi, &MeasureApi::pointsChanged, this, [this]() {
        syncMeasureDialog();
        update();
    });
    connect(m_measureApi, &MeasureApi::measuringChanged, this, [this](bool active) {
        emit measuringChanged(active);
        if (!active && m_measureDialog)
            m_measureDialog->hide();
        setCursor(active ? Qt::CrossCursor : Qt::ArrowCursor);
        update();
    });
    m_markedCenterLat = m_centerLat;
    m_markedCenterLon = m_centerLon;
    registerBuiltInShapeTypes();
    loadOfflineTiles();
    update();
}
MapWidget::~MapWidget() {}

// ============================================================
// MAP CONTROL
// ============================================================

void MapWidget::setCenter(double lat, double lon) {
    m_centerLat = lat;
    m_centerLon = lon;
    m_mapApi->setCenter(lat, lon);
    update();
}

void MapWidget::setZoom(int level) {
    m_zoom = qBound(1, level, 18);
    m_mapApi->setZoom(m_zoom);
    m_tileCache.clear();
    loadOfflineTiles();
    update();
}

void MapWidget::zoomIn() { setZoom(m_zoom + 1); }
void MapWidget::zoomOut() { setZoom(m_zoom - 1); }

double MapWidget::getCenterLat() const { return m_centerLat; }
double MapWidget::getCenterLon() const { return m_centerLon; }
int MapWidget::getZoom() const { return m_zoom; }

QPointF MapWidget::geoToCanvas(double lat, double lon) const {
    return m_mapApi->geoToCanvas(lat, lon);
}

QPointF MapWidget::canvasToGeo(const QPointF& pos) const {
    return m_mapApi->canvasToGeo(pos);
}

// ============================================================
// DRAWING MODE
// ============================================================

void MapWidget::startDrawing(const QString& shapeType)
{
    const QString activeId = LayerApi::instance()->getActiveLayer();
    if (!activeId.isEmpty() && !LayerApi::instance()->isEditable(activeId)) {
        QMessageBox::information(this,
                                 QStringLiteral("Layer Locked"),
                                 QStringLiteral("Active layer is locked.\nUnlock it or select another layer to draw."));
        return;
    }
    if (m_isPlacingBitmap)
        cancelPlacingBitmap();
    if (m_measureApi && m_measureApi->isMeasuring())
        stopMeasuring();
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_currentShapeType = ShapeTypeRegistry::instance()->type(shapeType);
    if (!m_currentShapeType) {
        qWarning() << "Unknown shape type:" << shapeType;
        return;
    }
    m_isDrawing = true;
    m_drawingType = shapeType;
    m_drawingPoints.clear();
    m_isDraggingShape = false;
    m_currentShapeType->onStartDrawing(this);
    setCursor(Qt::CrossCursor);
    update();
    emit drawingTypeChanged(m_drawingType);
}

void MapWidget::cancelDrawing()
{
    m_isDrawing = false;
    m_drawingType = "";
    m_drawingPoints.clear();
    m_currentShapeType = nullptr;
    m_isDraggingShape = false;
    setCursor(Qt::ArrowCursor);
    update();
    emit drawingTypeChanged(QString());
}

// ============================================================
// BITMAP PLACEMENT
// ============================================================

static QString bitmapIconForName(const QString& name)
{
    if (name == QLatin1String("Hospital"))
        return QStringLiteral(":/icons/images/hospital.png");
    if (name == QLatin1String("School"))
        return QStringLiteral(":/icons/images/school.png");
    if (name == QLatin1String("Forest Area"))
        return QStringLiteral(":/icons/images/forest-area.png");
    return QString();
}

void MapWidget::startPlacingBitmap(const QString& name)
{
    cancelDrawing();
    if (m_measureApi && m_measureApi->isMeasuring())
        stopMeasuring();
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_isPlacingBitmap = true;
    m_placingBitmapName = name;
    m_placingBitmapPath = bitmapIconForName(name);
    setCursor(Qt::CrossCursor);
    update();
    emit placingBitmapChanged(m_placingBitmapName);
}

void MapWidget::startPlacingBitmapImage(const QString& imagePath)
{
    cancelDrawing();
    if (m_measureApi && m_measureApi->isMeasuring())
        stopMeasuring();
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_isPlacingBitmap = true;
    m_placingBitmapName = imagePath;
    m_placingBitmapPath = imagePath;
    setCursor(Qt::CrossCursor);
    update();
    emit placingBitmapChanged(m_placingBitmapName);
}

void MapWidget::cancelPlacingBitmap()
{
    m_isPlacingBitmap = false;
    m_placingBitmapName.clear();
    m_placingBitmapPath.clear();
    setCursor(Qt::ArrowCursor);
    update();
    emit placingBitmapChanged(QString());
}

// ============================================================
// MEASURE DISTANCE
// ============================================================

bool MapWidget::isMeasuring() const
{
    return m_measureApi && m_measureApi->isMeasuring();
}

void MapWidget::ensureMeasureDialog()
{
    if (m_measureDialog)
        return;
    m_measureDialog = new ApiMeasureDistanceDialog(this);
    connect(m_measureDialog, &ApiMeasureDistanceDialog::requestNew, this, [this]() {
        clearMeasurePoints();
    });
    connect(m_measureDialog, &ApiMeasureDistanceDialog::closedByUser, this, [this]() {
        stopMeasuring();
    });
}
void MapWidget::syncMeasureDialog()
{
    if (!m_measureDialog || !m_measureApi)
        return;

    QVector<QPointF> allPts;
    QVector<double> allSegs;

    auto appendChain = [&](const QVector<QPointF>& chainPts) {
        for (int i = 0; i < chainPts.size(); ++i) {
            allPts.append(chainPts[i]);
            if (i == 0) {
                allSegs.append(0.0);
            } else {
                const QPointF& a = chainPts[i - 1];
                const QPointF& b = chainPts[i];
                allSegs.append(MeasureApi::distanceMeters(a.y(), a.x(), b.y(), b.x()));
            }
        }
    };

    for (const QVector<QPointF>& chain : m_completedMeasureChains)
        appendChain(chain);
    appendChain(m_measureApi->getPoints());

    m_measureDialog->setPoints(allPts, allSegs);
    emit measurePointsChanged(allPts, allSegs);
}

void MapWidget::startMeasuring()
{
    cancelDrawing();
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    if (!m_measureApi)
        m_measureApi = MeasureApi::instance();
    m_measureApi->startMeasuring();
    ensureMeasureDialog();
    m_measureDialog->clearMeasurement();
    m_measureDialog->show();
    m_measureDialog->raise();
    m_measureDialog->activateWindow();
    setCursor(Qt::CrossCursor);
    update();
}

void MapWidget::stopMeasuring()
{
    if (!m_measureApi)
        return;
    if (m_measureApi->isMeasuring())
        m_measureApi->stopMeasuring();
    m_measureApi->clearPoints();
    m_completedMeasureChains.clear();
    if (m_measureDialog) {
        m_measureDialog->clearMeasurement();
        m_measureDialog->hide();
    }
    setCursor(Qt::ArrowCursor);
    update();
}

void MapWidget::clearMeasurePoints()
{
    if (m_measureApi)
            m_completedMeasureChains.clear();
        m_measureApi->clearPoints();
    update();
}

// ============================================================
// TEXT ADD
// ============================================================

void MapWidget::addTextAtPosition(const QPointF& geoPos)
{
    const QString activeId = LayerApi::instance()->getActiveLayer();
    if (!activeId.isEmpty() && !LayerApi::instance()->isEditable(activeId)) {
        QMessageBox::information(this,
                                 QStringLiteral("Layer Locked"),
                                 QStringLiteral("Active layer is locked.\nUnlock it or select another layer."));
        return;
    }
    bool ok;
    QString text = QInputDialog::getText(this, "Add Text",
                                         "Enter text:",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        QJsonObject shape;
        shape["type"] = "text";
        shape["longitude"] = geoPos.x();
        shape["latitude"] = geoPos.y();
        shape["text"] = text;
        shape["color"] = "#000000";
        shape["fontSize"] = 14;
        m_shapeApi->addShape(shape);
        update();
    }
}

QString MapWidget::findTextAtCanvas(const QPoint& canvasPos) const
{
    QJsonArray shapes = m_shapeApi->getAllShapes();
    QString hitId;

    for (const QJsonValue& val : shapes) {
        QJsonObject obj = val.toObject();
        if (obj["type"].toString() != QLatin1String("text"))
            continue;

        QString text = obj["text"].toString();
        if (text.isEmpty())
            continue;

        int fontSize = obj["fontSize"].toInt(14);
        if (fontSize < 8) fontSize = 14;

        QFont font("Arial", fontSize, QFont::Bold);
        QFontMetrics fm(font);

        double lon = obj["longitude"].toDouble();
        double lat = obj["latitude"].toDouble();
        double rotation = obj["rotation"].toDouble(0.0);
        QPointF pos = geoToCanvas(lat, lon);

        QRect br = fm.boundingRect(text);
        br.moveTo(static_cast<int>(pos.x()),
                  static_cast<int>(pos.y()) - fm.ascent());
        br.adjust(-8, -6, 8, 6);

        QPointF testPt = canvasPos;
        if (std::fabs(rotation) > 0.01) {
            const double rad = -rotation * M_PI / 180.0;
            const double c = std::cos(rad);
            const double s = std::sin(rad);
            const double dx = canvasPos.x() - pos.x();
            const double dy = canvasPos.y() - pos.y();
            testPt = QPointF(pos.x() + dx * c - dy * s,
                             pos.y() + dx * s + dy * c);
        }
        if (br.contains(testPt.toPoint()))
            hitId = obj["id"].toString();
    }
    return hitId;
}
// ============================================================
// SLOTS
// ============================================================

void MapWidget::onMapChanged() {
    m_centerLat = m_mapApi->getCenterLat();
    m_centerLon = m_mapApi->getCenterLon();
    m_zoom = m_mapApi->getZoom();
    loadOfflineTiles();
    update();
    emit mapChanged();
}

void MapWidget::onTilesLoaded(int count) {
    Q_UNUSED(count);
    update();
}

// ============================================================
// TILE LOADING
// ============================================================

void MapWidget::loadOfflineTiles()
{
    m_tileCache.clear();

    // ----- MBTiles path -----
    if (m_hasMbtiles) {
        const int maxNative = m_mbtilesMaxZoom > 0 ? m_mbtilesMaxZoom : 18;
        const int tileZoom  = qMin(m_zoom, maxNative);
        const double scale  = std::pow(2.0, m_zoom - tileZoom);
        const double tilePx = m_tileSize * scale;

        int tiles = 1 << tileZoom;
        double centerX = lonToX(m_centerLon, tileZoom);
        double centerY = latToY(m_centerLat, tileZoom);

        int tilesX = std::ceil(static_cast<double>(width())  / tilePx) + 2;
        int tilesY = std::ceil(static_cast<double>(height()) / tilePx) + 2;
        int startX = std::floor(centerX - tilesX / 2.0);
        int startY = std::floor(centerY - tilesY / 2.0);

        for (int x = startX; x < startX + tilesX; ++x) {
            for (int y = startY; y < startY + tilesY; ++y) {
                int wrappedX = (x % tiles + tiles) % tiles;
                if (y < 0 || y >= tiles)
                    continue;
                if (tileZoom < m_mbtilesMinZoom || tileZoom > m_mbtilesMaxZoom)
                    continue;

                QPixmap pixmap = fetchMbtilesTile(tileZoom, wrappedX, y);
                if (!pixmap.isNull()) {
                    QString key = getTileKey(tileZoom, wrappedX, y);
                    m_tileCache[key] = pixmap;
                }
            }
        }
        return;
    }
    QString tilesPath = m_mapApi->activeTilesPath();
    QDir dir(tilesPath);
    if (!dir.exists()) {
        // qWarning() << "Tiles folder missing:" << tilesPath;
        return;
    }
    const TileSourceInfo srcInfo = m_mapApi->tileSourceInfo(m_mapApi->activeTileSource());
    const int maxNative = srcInfo.maxNativeZoom > 0 ? srcInfo.maxNativeZoom : 8;
    const int tileZoom  = qMin(m_zoom, maxNative);

    int tiles = 1 << tileZoom;
    double centerX = lonToX(m_centerLon, tileZoom);
    double centerY = latToY(m_centerLat, tileZoom);

    const double scale = std::pow(2.0, m_zoom - tileZoom);
    const double tilePx = m_tileSize * scale;
    int tilesX = std::ceil(static_cast<double>(width())  / tilePx) + 2;
    int tilesY = std::ceil(static_cast<double>(height()) / tilePx) + 2;
    int startX = std::floor(centerX - tilesX / 2.0);
    int startY = std::floor(centerY - tilesY / 2.0);

    for (int x = startX; x < startX + tilesX; ++x) {
        for (int y = startY; y < startY + tilesY; ++y) {
            int wrappedX = (x % tiles + tiles) % tiles;
            if (y < 0 || y >= tiles)
                continue;
            QString path = QString("%1/%2/%3/%4.png")
                               .arg(tilesPath).arg(tileZoom).arg(wrappedX).arg(y);
            if (QFile::exists(path)) {
                QPixmap pixmap(path);
                if (!pixmap.isNull()) {
                    QString key = getTileKey(tileZoom, wrappedX, y);
                    m_tileCache[key] = pixmap;
                }
            }
        }
    }
}
// ============================================================
// PAINT EVENT
// ============================================================

void MapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(30, 30, 50));
    drawTiles(painter);
    drawShapes(painter);
    drawHistoryPreview(painter);
    drawAirbases(painter);
    drawEditHandles(painter);
    drawDrawingPreview(painter);
    drawRotateHandle(painter);
    drawMeasureOverlay(painter);
    drawBoxZoomOverlay(painter);
    drawCenterMarker(painter);
    drawSearchMarker(painter);
    drawMapOverlay(painter);
}

// ============================================================
// DRAW ROTATE HANDLE
// ============================================================

void MapWidget::drawRotateHandle(QPainter& painter) {
    if (m_rotatingShapeId.isEmpty()) return;
    QJsonObject props = m_shapeApi->getShapeProperties(m_rotatingShapeId);
    if (props.isEmpty()) return;
    double lon = props["longitude"].toDouble();
    double lat = props["latitude"].toDouble();
    QPointF center = geoToCanvas(lat, lon);
    QPointF handlePos = center + QPointF(60.0, -60.0);
    painter.save();
    painter.setPen(QPen(QColor(100, 200, 255, 150), 1, Qt::DashLine));
    painter.drawLine(center, handlePos);
    painter.setPen(QPen(QColor(100, 200, 255, 80), 3));
    painter.setBrush(QColor(100, 200, 255, 30));
    painter.drawEllipse(handlePos, 20, 20);
    painter.setPen(QPen(Qt::blue, 2));
    painter.setBrush(QColor(100, 200, 255, 180));
    painter.drawEllipse(handlePos, 12, 12);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(Qt::white);
    painter.drawEllipse(handlePos, 4, 4);
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(Qt::NoBrush);
    QRectF arcRect(handlePos.x() - 8, handlePos.y() - 8, 16, 16);
    painter.drawArc(arcRect, 30 * 16, 280 * 16);
    QPointF arrowTip = handlePos + QPointF(8 * cos(-30 * M_PI / 180),
                                           8 * sin(-30 * M_PI / 180));
    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::white, 1));
    QPolygonF arrow;
    arrow << arrowTip << arrowTip + QPointF(-4, -3) << arrowTip + QPointF(-1, 4);
    painter.drawPolygon(arrow);
    painter.setPen(QColor(100, 200, 255));
    painter.setFont(QFont("Arial", 9));
    painter.drawText(handlePos + QPointF(-30, -25), "↻ DRAG");
    double angle = props["rotation"].toDouble(0);
    painter.setPen(Qt::yellow);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(handlePos + QPointF(-20, 40), QString("%1°").arg(angle, 0, 'f', 1));
    painter.restore();
}

// ============================================================
// DRAW AIRBASES
// ============================================================

void MapWidget::drawAirbases(QPainter& painter)
{
    AirbaseApi* api = AirbaseApi::instance();
    if (!api->isVisible())
        return;
    QPixmap icon = api->icon();
    if (icon.isNull()) {
        api->initialize();
        icon = api->icon();
    }
    if (icon.isNull())
        return;
    const int iconSize = 20;
    QPixmap scaled = icon.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QVector<QPointF> pts = api->getPositions();
    for (const QPointF& g : pts) {
        // g = (lon, lat)
        QPointF canvasPos = geoToCanvas(g.y(), g.x());
        painter.drawPixmap(
            static_cast<int>(canvasPos.x() - iconSize / 2),
            static_cast<int>(canvasPos.y() - iconSize / 2),
            scaled);
    }
}

void MapWidget::drawMeasureChain(QPainter& painter, const QVector<QPointF>& pts, bool active)
{
    if (pts.isEmpty())
        return;
    QVector<QPointF> canvasPts;
    canvasPts.reserve(pts.size() + 1);
    for (const QPointF& g : pts)
        canvasPts.append(geoToCanvas(g.y(), g.x()));
    if (active && !canvasPts.isEmpty()) {
        QPointF mouseGeo = canvasToGeo(m_lastMousePos);
        canvasPts.append(geoToCanvas(mouseGeo.y(), mouseGeo.x()));
    }
    if (canvasPts.size() >= 2) {
        painter.setPen(QPen(QColor(255, 220, 0), 2.5));
        for (int i = 1; i < canvasPts.size(); ++i)
            painter.drawLine(canvasPts[i - 1], canvasPts[i]);
    }
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    for (int i = 1; i < pts.size(); ++i) {
        const QPointF& a = pts[i - 1];
        const QPointF& b = pts[i];
        double meters = MeasureApi::distanceMeters(a.y(), a.x(), b.y(), b.x());
        QPointF mid = (geoToCanvas(a.y(), a.x()) + geoToCanvas(b.y(), b.x())) / 2.0;
        const double shown = m_measureApi->convertFromMeters(meters);
        const QString suf = m_measureApi->currentUnitSuffix();
        const int decimals = (suf == QLatin1String("m") || suf == QLatin1String("ft")) ? 1 : 3;
        QString label = QString("%1 %2").arg(shown, 0, 'f', decimals).arg(suf);
        QFontMetrics fm(painter.font());
        QRect br = fm.boundingRect(label).adjusted(-4, -2, 4, 2);
        br.moveCenter(mid.toPoint());
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(20, 20, 40, 200));
        painter.drawRoundedRect(br, 3, 3);
        painter.setPen(QColor(255, 230, 80));
        painter.drawText(br, Qt::AlignCenter, label);
    }
    for (const QPointF& g : pts) {
        QPointF c = geoToCanvas(g.y(), g.x());
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QColor(255, 200, 0));
        painter.drawEllipse(c, 6, 6);
    }
}
void MapWidget::drawMeasureOverlay(QPainter& painter)
{
    if (!m_measureApi)
        return;
    const QVector<QPointF> pts = m_measureApi->getPoints();
    const bool active = m_measureApi->isMeasuring();
    if (!active && pts.isEmpty() && m_completedMeasureChains.isEmpty())
        return;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    for (const QVector<QPointF>& chain : m_completedMeasureChains)
        drawMeasureChain(painter, chain, false);
    drawMeasureChain(painter, pts, active);
    painter.restore();
}

// ============================================================
// DRAW TILES
// ============================================================
void MapWidget::drawTiles(QPainter& painter) {
    if (m_hasRasterBasemap && !m_rasterBasemap.isNull()) {
        const QPointF tl = geoToCanvas(m_rasterNorth, m_rasterWest);
        const QPointF br = geoToCanvas(m_rasterSouth, m_rasterEast);
        QRectF target(tl, br);
        target = target.normalized();
        painter.drawPixmap(target.toRect(), m_rasterBasemap);
        return;
    }
    const TileSourceInfo srcInfo = m_mapApi->tileSourceInfo(m_mapApi->activeTileSource());
    const int maxNative = srcInfo.maxNativeZoom > 0 ? srcInfo.maxNativeZoom : 8;
    const int tileZoom  = qMin(m_zoom, maxNative);
    const double scale  = std::pow(2.0, m_zoom - tileZoom);
    const double tilePx = m_tileSize * scale;

    int tiles = 1 << tileZoom;
    double centerX = lonToX(m_centerLon, tileZoom);
    double centerY = latToY(m_centerLat, tileZoom);
    int tilesX = std::ceil(static_cast<double>(width())  / tilePx) + 2;
    int tilesY = std::ceil(static_cast<double>(height()) / tilePx) + 2;
    int startX = std::floor(centerX - tilesX / 2.0);
    int startY = std::floor(centerY - tilesY / 2.0);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    bool anyTileLoaded = false;
    for (int x = startX; x < startX + tilesX; ++x) {
        for (int y = startY; y < startY + tilesY; ++y) {
            if (y < 0 || y >= tiles)
                continue;
            int wrappedX = (x % tiles + tiles) % tiles;
            QString key = getTileKey(tileZoom, wrappedX, y);

            const double dx = (wrappedX - centerX) * tilePx + width()  / 2.0;
            const double dy = (y        - centerY) * tilePx + height() / 2.0;
            const QRectF dest(dx, dy, tilePx, tilePx);

            if (m_tileCache.contains(key) && !m_tileCache[key].isNull()) {
                painter.drawPixmap(dest, m_tileCache[key], m_tileCache[key].rect());
                anyTileLoaded = true;
            } else {
                painter.fillRect(dest, QColor(40, 40, 60));
                painter.setPen(QColor(70, 70, 90));
                painter.drawRect(dest);
                if (tilePx > 40) {
                    painter.setPen(QColor(100, 100, 140));
                    painter.setFont(QFont("Arial", qMax(7, static_cast<int>(8 * scale))));
                    painter.drawText(dest.adjusted(6, 4, -4, -4),
                                     Qt::AlignLeft | Qt::AlignTop,
                                     QString("%1,%2").arg(wrappedX).arg(y));
                }
            }
        }
    }
    if (!anyTileLoaded) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(rect(), Qt::AlignCenter, "No tiles loaded\nPlace tiles in testOSM/ folder");
    }
}

// ============================================================
// DRAW SHAPES
// ============================================================
static void drawShapeFallback(QPainter& painter, const QJsonObject& obj, MapWidget* map)
{
    if (!map)
        return;
    const QColor color(obj.value(QStringLiteral("color")).toString(QStringLiteral("#00cc88")));
    const int lineWidth = qMax(1, obj.value(QStringLiteral("lineWidth")).toInt(2));
    const QString type = obj.value(QStringLiteral("type")).toString().toLower();
    const double rotation = obj.value(QStringLiteral("rotation")).toDouble(0.0);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, lineWidth));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 50));

    // 1) points outline (star / freehand / etc.)
    if (obj.contains(QStringLiteral("points")) && obj.value(QStringLiteral("points")).isArray()) {
        const QJsonArray pts = obj.value(QStringLiteral("points")).toArray();
        QPolygonF poly;
        poly.reserve(pts.size());
        for (const QJsonValue& v : pts) {
            const QJsonArray pt = v.toArray();
            if (pt.size() < 2)
                continue;
            poly << map->geoToCanvas(pt.at(1).toDouble(), pt.at(0).toDouble());
        }
        if (poly.size() >= 2) {
            if (type.contains(QLatin1String("line")) || type.contains(QLatin1String("path"))) {
                painter.setBrush(Qt::NoBrush);
                painter.drawPolyline(poly);
            } else {
                painter.drawPolygon(poly);
            }
            painter.restore();
            return;
        }
    }

    // 2) center + width/height in degrees (ellipse plugin style)
    const double lon = obj.value(QStringLiteral("longitude")).toDouble();
    const double lat = obj.value(QStringLiteral("latitude")).toDouble();
    const QPointF center = map->geoToCanvas(lat, lon);

    const double w = obj.value(QStringLiteral("width")).toDouble();
    const double h = obj.value(QStringLiteral("height")).toDouble();

    double rx = 0.0, ry = 0.0;
    if (w > 0.0 && h > 0.0) {
        const QPointF edgeX = map->geoToCanvas(lat, lon + w / 2.0);
        const QPointF edgeY = map->geoToCanvas(lat + h / 2.0, lon);
        rx = QVector2D(edgeX - center).length();
        ry = QVector2D(edgeY - center).length();
    }

    if (rx <= 0.0) {
        rx = 8.0;
        ry = 8.0;
    }

    if (std::fabs(rotation) > 0.01) {
        painter.translate(center);
        painter.rotate(rotation);
        painter.translate(-center);
    }

    if (type.contains(QLatin1String("rect")))
        painter.drawRect(QRectF(center.x() - rx, center.y() - ry, rx * 2.0, ry * 2.0));
    else
        painter.drawEllipse(center, rx, ry);

    painter.restore();
}

void MapWidget::drawShapes(QPainter& painter)
{
    QJsonArray shapes = m_shapeApi->getAllShapes();
    LayerApi* layerApi = LayerApi::instance();
    for (const QJsonValue& val : shapes) {
        QJsonObject obj = val.toObject();
        QString id = obj["id"].toString();
        QString layerId = layerApi->getLayerOfShape(id);
        if (!layerId.isEmpty() && !layerApi->isVisible(layerId))
            continue;
        IShapeType* st = ShapeTypeRegistry::instance()->type(obj["type"].toString());
        if (st) {
            st->draw(painter, obj, this);
if (m_selectedShapeIds.contains(id) || id == m_selectedShapeId) {
                painter.save();
                st->drawSelection(painter, obj, this);
                painter.restore();
            }
        } else {
            drawShapeFallback(painter, obj, this);
if (m_selectedShapeIds.contains(id) || id == m_selectedShapeId) {
                painter.save();
                painter.setPen(QPen(QColor(0, 180, 255), 1.5, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                if (obj.contains(QStringLiteral("points")) &&
                    obj.value(QStringLiteral("points")).isArray()) {
                    const QJsonArray pts = obj.value(QStringLiteral("points")).toArray();
                    QPolygonF poly;
                    for (const QJsonValue& v : pts) {
                        const QJsonArray pt = v.toArray();
                        if (pt.size() < 2)
                            continue;
                        poly << geoToCanvas(pt.at(1).toDouble(), pt.at(0).toDouble());
                    }
                    if (poly.size() >= 2)
                        painter.drawPolygon(poly);
                } else {
                    const QPointF c = geoToCanvas(
                        obj.value(QStringLiteral("latitude")).toDouble(),
                        obj.value(QStringLiteral("longitude")).toDouble());
                    painter.drawEllipse(c, 10, 10);
                }
                painter.restore();
            }
        }
    }
}
// ============================================================
// DRAW DRAWING PREVIEW
// ============================================================

void MapWidget::drawDrawingPreview(QPainter& painter)
{
    if (!m_isDrawing || !m_currentShapeType || m_drawingPoints.isEmpty())
        return;
    m_currentShapeType->drawPreview(painter, m_drawingPoints, this);
}

// ============================================================
// MOUSE PRESS EVENT
// ============================================================
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_dragging = true;
        m_lastDragPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    m_lastMousePos = event->pos();
    if (m_measureApi && m_measureApi->isMeasuring()) {
        if (event->button() == Qt::LeftButton) {
            m_measureApi->addPoint(canvasToGeo(event->pos()));
            update();
        }
        return;
    }
    if (!m_gisInteractionEnabled) {
        if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
            event->ignore();
            return;
        }
    }
    if (m_isBoxZooming && event->button() == Qt::LeftButton) {
        m_boxZoomFollowMouse = false;
        setCursor(Qt::CrossCursor);
        update();
        return;
    }

    // Legacy pending path
    if (m_boxZoomPending && event->button() == Qt::LeftButton) {
        m_boxZoomPending = false;
        m_isBoxZooming = true;
        m_boxZoomFollowMouse = false;
        m_boxZoomStart = event->pos();
        m_boxZoomCurrent = event->pos();
        setCursor(Qt::CrossCursor);
        update();
        return;
    }
    if (m_boxZoomPending) {
        m_boxZoomPending = false;
        setCursor(Qt::ArrowCursor);
    }
    // ============================================================
    // 1. Measuring mode
    // ============================================================
    if (m_measureApi && m_measureApi->isMeasuring()) {
        if (event->button() == Qt::LeftButton) {
            m_measureApi->addPoint(canvasToGeo(event->pos()));
            update();
        }
        return;
    }
    // ============================================================
    // 2. Bitmap placement mode
    // ============================================================
    if (m_isPlacingBitmap) {
        if (event->button() == Qt::LeftButton) {
            QPointF geo = canvasToGeo(event->pos());
            const double sizeDeg = pixelSizeToDegrees(28);
            QJsonObject shape;
            shape["type"]      = "bitmap";
            shape["longitude"] = geo.x();
            shape["latitude"]  = geo.y();
            shape["width"]     = sizeDeg;
            shape["height"]    = sizeDeg;
            shape["imagePath"] = m_placingBitmapPath;
            shape["rotation"]  = 0.0;
            m_shapeApi->addShape(shape);
            update();
        }
        return;
    }
    // ============================================================
    // 3. Drawing mode (plugin system)
    // ============================================================
    if (m_isDrawing && m_currentShapeType) {
        bool finished = m_currentShapeType->onMousePress(this, event, m_drawingPoints);
        if (finished) {
            QJsonObject shape = m_currentShapeType->finishDrawing(m_drawingPoints);
            if (!shape.isEmpty())
                m_shapeApi->addShape(shape);
            cancelDrawing();
        }
        update();
        return;
    }
    // ============================================================
    // 4. Select Center pick mode
    // ============================================================
    if (m_isSelectingCenter && event->button() == Qt::LeftButton) {
        QPointF geo = canvasToGeo(event->pos());
        m_markedCenterLon = geo.x();
        m_markedCenterLat = geo.y();
        setCenter(m_markedCenterLat, m_markedCenterLon);
        m_isSelectingCenter = false;
        setCursor(Qt::ArrowCursor);
        update();
        return;
    }

    // ============================================================
    // 5. Right click → Context menu
    // ============================================================
    if (event->button() == Qt::RightButton) {
        showContextMenu(event);
        return;
    }

    // ============================================================
    // 6. Middle button → Start pan
    // ============================================================
    if (event->button() == Qt::MiddleButton) {
        m_dragging = true;
        m_lastDragPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // ============================================================
    // 6. Left button
    // ============================================================
    if (event->button() == Qt::LeftButton) {
        if (!m_editingShapeId.isEmpty()) {
            int idx = findNearestEditPoint(m_editingShapeId, event->pos());
            if (idx >= 0) {
                m_editPointIndex = idx;
                ShapeHistoryApi::instance()->pushSnapshot(m_editingShapeId);
                setCursor(Qt::CrossCursor);
                update();
                return;
            }
            m_editingShapeId.clear();
            m_editPointIndex = -1;
        }
        if (!m_movingShapeId.isEmpty()) {
            QString hitForMove = findTextAtCanvas(event->pos());
            if (hitForMove.isEmpty())
                hitForMove = findBitmapAtCanvas(event->pos());
            if (hitForMove.isEmpty()) {
                QPointF geo = canvasToGeo(event->pos());
                hitForMove = m_shapeApi->findShapeAt(geo);
            }
            if (hitForMove == m_movingShapeId) {
                m_isMovingShape = true;
                m_moveStartPos = event->pos();
                m_shapeDataAtStart = m_shapeApi->getShapeProperties(m_movingShapeId);
                ShapeHistoryApi::instance()->pushSnapshot(m_movingShapeId);
                setCursor(Qt::ClosedHandCursor);
                update();
                return;
            }
            m_movingShapeId.clear();
            m_isMovingShape = false;
        }
        if (!m_rotatingShapeId.isEmpty()) {
            QJsonObject props = m_shapeApi->getShapeProperties(m_rotatingShapeId);
            if (!props.isEmpty()) {
                QPointF center = geoToCanvas(props["latitude"].toDouble(),
                                             props["longitude"].toDouble());
                QPointF handle = center + QPointF(60.0, -60.0);
                if (QVector2D(event->pos() - handle).length() < 25.0) {
                    m_isRotatingShape = true;
                    m_rotateStartPos = event->pos();
                    m_rotateStartAngle = props["rotation"].toDouble(0.0);
                    ShapeHistoryApi::instance()->pushSnapshot(m_rotatingShapeId);
                    setCursor(Qt::ClosedHandCursor);
                    update();
                    return;
                }
            }
            m_rotatingShapeId.clear();
            m_isRotatingShape = false;
        }
        QString hitId = findTextAtCanvas(event->pos());
        if (hitId.isEmpty())
            hitId = findBitmapAtCanvas(event->pos());
        if (hitId.isEmpty()) {
            QPointF geo = canvasToGeo(event->pos());
            hitId = m_shapeApi->findShapeAt(geo);
            if (!hitId.isEmpty()) {
                QJsonObject p = m_shapeApi->getShapeProperties(hitId);
            }
        }
        if (!hitId.isEmpty()) {
            selectShape(hitId);
            if (m_editingShapeId != hitId)  m_editingShapeId.clear();
            if (m_movingShapeId  != hitId)  m_movingShapeId.clear();
            if (m_rotatingShapeId != hitId) m_rotatingShapeId.clear();
            m_editPointIndex = -1;
            m_isMovingShape = false;
            m_isRotatingShape = false;
            update();
            return;
        }
        selectShape(QString());
        m_editingShapeId.clear();
        m_movingShapeId.clear();
        m_rotatingShapeId.clear();
        m_editPointIndex = -1;
        m_isMovingShape = false;
        m_isRotatingShape = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    if (ElevationData::instance()->isLoaded()) {
        static qint64 s_lastAltLogMs = 0;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - s_lastAltLogMs >= 200) {
            s_lastAltLogMs = now;
            const QPointF geo = canvasToGeo(event->pos());
            const double lat = geo.y();
            const double lon = geo.x();
            const double alt = ElevationData::instance()->getAltitude(lat, lon);
            if (!std::isnan(alt)) {
                qDebug().nospace()
                    << "[Elevation] Lat: " << lat
                    << "  Lon: " << lon
                    << "  Alt: " << alt << " m";
            }
        }
    }
    if (m_isBoxZooming) {
        if (m_boxZoomFollowMouse || (event->buttons() & Qt::LeftButton)) {
            m_boxZoomCurrent = event->pos();
            update();
        }
        return;
    }

    if (m_measureApi && m_measureApi->isMeasuring()) {
        update();
        return;
    }
    if (m_isDrawing && m_currentShapeType) {
        if (m_drawingPoints.isEmpty()) {
        } else if (m_drawingPoints.size() == 1) {
            m_drawingPoints.append(canvasToGeo(event->pos()));
        } else {
            m_drawingPoints.last() = canvasToGeo(event->pos());
        }
        m_currentShapeType->onMouseMove(this, event, m_drawingPoints);
        update();
        return;
    }

    // ---------- Edit point drag ----------
    if (m_editPointIndex >= 0 && !m_editingShapeId.isEmpty()) {
        QJsonObject props = m_shapeApi->getShapeProperties(m_editingShapeId);
        const QString type = props.value(QStringLiteral("type")).toString();
        IShapeType* st = ShapeTypeRegistry::instance()->type(type);
        if (st) {
            QPointF geo = canvasToGeo(event->pos());
            st->updateEditPoint(props, m_editPointIndex, geo);
            m_shapeApi->updateShape(m_editingShapeId, props);
        }
        update();
        return;
    }

    // ---------- Move shape ----------
    if (m_isMovingShape && !m_movingShapeId.isEmpty()) {
        QPointF startGeo = canvasToGeo(m_moveStartPos);
        QPointF currGeo  = canvasToGeo(event->pos());
        double dLon = currGeo.x() - startGeo.x();
        double dLat = currGeo.y() - startGeo.y();
        QJsonObject props = m_shapeDataAtStart;
        props["longitude"] = m_shapeDataAtStart["longitude"].toDouble() + dLon;
        props["latitude"]  = m_shapeDataAtStart["latitude"].toDouble()  + dLat;
        if (props.contains("points") && props["points"].isArray()) {
            QJsonArray oldPts = m_shapeDataAtStart["points"].toArray();
            QJsonArray newPts;
            for (const QJsonValue& v : oldPts) {
                QJsonArray pt = v.toArray();
                if (pt.size() < 2) continue;
                QJsonArray np;
                np.append(pt[0].toDouble() + dLon);
                np.append(pt[1].toDouble() + dLat);
                newPts.append(np);
            }
            props["points"] = newPts;
        }
        m_shapeApi->updateShape(m_movingShapeId, props);
        update();
        return;
    }

    // ---------- Rotate shape ----------
    if (m_isRotatingShape && !m_rotatingShapeId.isEmpty()) {
        QJsonObject props = m_shapeApi->getShapeProperties(m_rotatingShapeId);
        QPointF center = geoToCanvas(props["latitude"].toDouble(),
                                     props["longitude"].toDouble());
        QPointF v1 = m_rotateStartPos - center.toPoint();
        QPointF v2 = event->pos() - center.toPoint();
        double a1 = std::atan2(v1.y(), v1.x());
        double a2 = std::atan2(v2.y(), v2.x());
        double delta = (a2 - a1) * 180.0 / M_PI;
        props["rotation"] = m_rotateStartAngle + delta;
        m_shapeApi->updateShape(m_rotatingShapeId, props);
        update();
        return;
    }

    // ---------- Middle-button Pan ----------
    if (m_dragging) {
        if (!(event->buttons() & Qt::MiddleButton)) {
            m_dragging = false;
            setCursor(Qt::ArrowCursor);
            update();
            return;
        }
        QPoint delta = event->pos() - m_lastDragPos;
        m_lastDragPos = event->pos();
        QPointF centerCanvas(width() / 2.0, height() / 2.0);
        QPointF newCenterCanvas = centerCanvas - delta;
        QPointF newGeo = canvasToGeo(newCenterCanvas);
        setCenter(newGeo.y(), newGeo.x());
        return;
    }
    update();
}
// ============================================================
// MOUSE RELEASE / WHEEL / RESIZE
// ============================================================


void MapWidget::wheelEvent(QWheelEvent* event) {

    const QPointF mousePos = event->pos();
    const QPointF geoUnderMouse = canvasToGeo(mousePos);
    const int oldZoom = m_zoom;
    int newZoom = oldZoom + (event->angleDelta().y() > 0 ? 1 : -1);
    newZoom = qBound(1, newZoom, 18);
    if (newZoom == oldZoom) {
        event->accept();
        return;
    }
    setZoom(newZoom);
    const QPointF canvasAfter = geoToCanvas(geoUnderMouse.y(), geoUnderMouse.x());
    const QPointF centerCanvas(width() / 2.0, height() / 2.0);
    const QPointF delta = mousePos - canvasAfter;
    const QPointF newCenterCanvas = centerCanvas - delta;
    const QPointF newGeo = canvasToGeo(newCenterCanvas);
    setCenter(newGeo.y(), newGeo.x());
    event->accept();
}

void MapWidget::resizeEvent(QResizeEvent* event) {
    m_mapApi->setMapSize(width(), height());
    QWidget::resizeEvent(event);
    update();
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isBoxZooming && event->button() == Qt::LeftButton) {
        QRect zoomBox = QRect(m_boxZoomStart, m_boxZoomCurrent).normalized();
        m_isBoxZooming = false;
        m_boxZoomPending = false;
        m_boxZoomFollowMouse = false;
        setCursor(Qt::ArrowCursor);
        if (zoomBox.width() > 10 && zoomBox.height() > 10)
            zoomToCanvasRect(zoomBox);
        update();
        return;
    }
    if (m_isDrawing && m_currentShapeType) {
        bool finished = m_currentShapeType->onMouseRelease(this, event, m_drawingPoints);
        if (finished) {
            QJsonObject shape = m_currentShapeType->finishDrawing(m_drawingPoints);
            if (!shape.isEmpty())
                m_shapeApi->addShape(shape);
            cancelDrawing();
        }
        update();
        return;
    }

    if (m_editPointIndex >= 0) {
        m_editPointIndex = -1;
        setCursor(Qt::SizeAllCursor);
        update();
        return;
    }

    if (m_isMovingShape) {
        m_isMovingShape = false;
        setCursor(Qt::OpenHandCursor);
        update();
        return;
    }

    // End rotate
    if (m_isRotatingShape) {
        m_isRotatingShape = false;
        setCursor(Qt::PointingHandCursor);
        update();
        return;
    }

    // End pan
    if (event->button() == Qt::MiddleButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
}

// ============================================================
// DRAWING CLICK / KEY / FINISH
// ============================================================
void MapWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_isDrawing && m_currentShapeType && m_drawingPoints.size() >= 2) {
            QJsonObject shape = m_currentShapeType->finishDrawing(m_drawingPoints);
            if (!shape.isEmpty())
                m_shapeApi->addShape(shape);
            cancelDrawing();
            return;
        }
        if (m_measureApi && m_measureApi->isMeasuring()) {
            const QVector<QPointF> pts = m_measureApi->getPoints();
            if (pts.size() >= 2)
                m_completedMeasureChains.append(pts);
            m_measureApi->clearPoints();
            update();
            return;
        }
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (!m_selectedShapeId.isEmpty()) {
            deleteShape(m_selectedShapeId);
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_Escape) {
        if (m_isBoxZooming || m_boxZoomPending || m_boxZoomFollowMouse) {
            m_isBoxZooming = false;
            m_boxZoomPending = false;
            m_boxZoomFollowMouse = false;
            setCursor(Qt::ArrowCursor);
            update();
            return;
        }
        if (m_isSelectingCenter) {
            m_isSelectingCenter = false;
            setCursor(Qt::ArrowCursor);
            update();
            return;
        }
        if (m_measureApi && m_measureApi->isMeasuring()) {
            stopMeasuring();
            return;
        }
        if (m_isDrawing) {
            cancelDrawing();
            return;
        }

        if (m_isPlacingBitmap) {
            cancelPlacingBitmap();
            return;
        }

        m_isMovingShape = false;
        m_isRotatingShape = false;
        m_editPointIndex = -1;
        m_movingShapeId.clear();
        m_editingShapeId.clear();
        m_rotatingShapeId.clear();
        m_selectedShapeId.clear();
        setCursor(Qt::ArrowCursor);
        update();
        return;
    }
    QWidget::keyPressEvent(event);
}
void MapWidget::finishDrawing()
{
    if (!m_currentShapeType || m_drawingPoints.isEmpty()) {
        cancelDrawing();
        return;
    }
    QJsonObject shape = m_currentShapeType->finishDrawing(m_drawingPoints);
    if (!shape.isEmpty()) {
        m_shapeApi->addShape(shape);
    }
    cancelDrawing();
}
// ============================================================
// CONTEXT MENU / SHAPE OPS / TILE HELPERS
// ============================================================
void MapWidget::showContextMenu(QMouseEvent* event)
{
    QMenu menu(this);
    const QString menuStyle = R"(
        QMenu {
            background-color: #1A3652;
            color: white;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 4px 0px;
            font-size: 13px;
        }
        QMenu::item {
            background-color: transparent;
            color: white;
            padding: 8px 28px 8px 20px;
            margin: 2px 4px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #0078D4;
            color: white;
        }
        QMenu::item:disabled {
            color: #808080;
        }
        QMenu::separator {
            height: 1px;
            background-color: #27446d;
            margin: 4px 10px;
        }
        QMenu::right-arrow {
            width: 10px;
            height: 10px;
        }
    )";
    menu.setStyleSheet(menuStyle);
    QPointF geoPos = canvasToGeo(event->pos());
    QString shapeId = findTextAtCanvas(event->pos());
    if (shapeId.isEmpty())
        shapeId = findBitmapAtCanvas(event->pos());
    if (shapeId.isEmpty())
        shapeId = m_shapeApi->findShapeAt(geoPos);
    UiActionContext ctx;
    ctx.map = this;
    ctx.parentWidget = this;
    ctx.geoPos = geoPos;
    ctx.shapeId = shapeId;
    UiActionApi* ui = UiActionApi::instance();

    if (!shapeId.isEmpty()) {
        if (m_selectedShapeId != shapeId) {
            m_selectedShapeId = shapeId;
            emit shapeSelectionChanged(m_selectedShapeId);
        }
        for (const UiActionDescriptor& d : ui->actionsFor(UiActionCategory::ContextShape)) {
            if (d.isEnabled && !d.isEnabled(ctx))
                continue;
            if (d.id == QLatin1String("core.ctx.properties")
                || d.id == QLatin1String("core.ctx.copy")
                || d.id == QLatin1String("core.ctx.preview_history")
                || d.id == QLatin1String("core.ctx.delete"))
                menu.addSeparator();
            QAction* a = menu.addAction(d.label);
            a->setData(d.id);
            if (d.isEnabled)
                a->setEnabled(d.isEnabled(ctx));
        }
        QAction* selected = menu.exec(event->globalPos());
        if (!selected)
            return;
        const QString id = selected->data().toString();
        ui->trigger(id, ctx);
        update();
        return;
    }

    // ---------- Empty map ----------
    QAction* pasteAction = nullptr;
    if (!m_clipboardShape.isEmpty()) {
        pasteAction = menu.addAction("📋 Paste");
        menu.addSeparator();
    }

    QStringList groups = ui->groupsIn(UiActionCategory::ContextMap);
    QMap<QString, QMenu*> groupMenus;

    for (const QString& g : groups) {
        QMenu* sub = menu.addMenu("➕ " + g);
        sub->setStyleSheet(menuStyle);
        groupMenus[g] = sub;
    }

    QVector<UiActionDescriptor> mapActions = ui->actionsFor(UiActionCategory::ContextMap);
    QAction* lastSepCandidate = nullptr;

    for (const UiActionDescriptor& d : mapActions) {
        QMenu* target = &menu;
        if (!d.group.isEmpty() && groupMenus.contains(d.group))
            target = groupMenus[d.group];
        if (d.priority >= 150 && target == &menu && lastSepCandidate == nullptr) {
            menu.addSeparator();
            lastSepCandidate = reinterpret_cast<QAction*>(1);
        }
        QAction* a = target->addAction(d.label);
        a->setData(d.id);
        if (d.isEnabled)
            a->setEnabled(d.isEnabled(ctx));
    }
    QAction* selected = menu.exec(event->globalPos());
    if (!selected)
        return;
    if (selected == pasteAction) {
        pasteShapeAt(geoPos);
        return;
    }
    const QString id = selected->data().toString();
    ui->trigger(id, ctx);
    update();
}

void MapWidget::deleteShape(const QString& shapeId) {
    if (QMessageBox::question(this, "Delete Shape",
                              "Are you sure you want to delete this shape?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (ShapeHistoryApi::instance()->previewShapeId() == shapeId)
        ShapeHistoryApi::instance()->hidePreview();
    ShapeHistoryApi::instance()->clear(shapeId);
    m_shapeApi->deleteShape(shapeId);
    m_selectedShapeId = m_movingShapeId = m_editingShapeId = m_rotatingShapeId = "";
    m_isMovingShape = m_isRotatingShape = false;
    setCursor(Qt::ArrowCursor);
    update();
}

void MapWidget::showProperties(const QString& shapeId)
{
    if (shapeId.isEmpty())
        return;
    QJsonObject props = m_shapeApi->getShapeProperties(shapeId);
    if (props.isEmpty())
        return;
    const QString type = props["type"].toString();

    // ============================================================
    // TEXT PROPERTIES
    // ============================================================
    if (type == QLatin1String("text")) {
        QString currentText = props["text"].toString();
        QColor currentColor = QColor(props["color"].toString("#000000"));
        int currentSize = props["fontSize"].toInt(14);
        if (currentSize < 8) currentSize = 14;
        QDialog dialog(this);
        dialog.setWindowTitle("Text Properties");
        dialog.setFixedWidth(340);
        dialog.setStyleSheet(
            "QDialog { background-color: #2b2b2b; }"
            "QLabel { color: white; font-size: 13px; font-weight: bold; }"
            "QLineEdit { background-color: #3c3c3c; color: white; padding: 8px;"
            " border: 1px solid #555; border-radius: 4px; font-size: 13px; }"
            "QSpinBox { background-color: #3c3c3c; color: white; padding: 8px;"
            " border: 1px solid #555; border-radius: 4px; font-size: 13px; }"
            "QPushButton#closeBtn, QPushButton#deleteBtn {"
            " background-color: #3c3c3c; color: white; padding: 10px;"
            " border: none; border-radius: 4px; font-size: 13px; }"
            "QPushButton#closeBtn:hover, QPushButton#deleteBtn:hover {"
            " background-color: #4a4a4a; }"
            "QPushButton#applyBtn { background-color: #0078d7; color: white; padding: 10px;"
            " border: none; border-radius: 4px; font-size: 13px; font-weight: bold; }"
            "QPushButton#applyBtn:hover { background-color: #005a9e; }");

        QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        mainLayout->addWidget(new QLabel("Text Content:", &dialog));
        QLineEdit* textEdit = new QLineEdit(currentText, &dialog);
        textEdit->setFixedHeight(36);
        mainLayout->addWidget(textEdit);

        mainLayout->addWidget(new QLabel("Text Color:", &dialog));
        QColor pickedColor = currentColor;
        QPushButton* colorButton = new QPushButton("Select Color", &dialog);
        colorButton->setFixedHeight(40);
        colorButton->setCursor(Qt::PointingHandCursor);

        auto updateColorButton = [colorButton](const QColor& c) {
            int brightness = (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000;
            QString textColor = brightness > 140 ? "black" : "white";
            colorButton->setStyleSheet(
                QString("QPushButton { background-color: %1; color: %2; border: none; "
                        "border-radius: 4px; font-size: 13px; font-weight: bold; }")
                    .arg(c.name(), textColor));
        };
        updateColorButton(pickedColor);
        QObject::connect(colorButton, &QPushButton::clicked,
                         [&dialog, &pickedColor, updateColorButton]() {
                             QColor chosen = QColorDialog::getColor(pickedColor, &dialog, "Select Color");
                             if (chosen.isValid()) {
                                 pickedColor = chosen;
                                 updateColorButton(pickedColor);
                             }
                         });
        mainLayout->addWidget(colorButton);
        mainLayout->addWidget(new QLabel("Font Size:", &dialog));
        QSpinBox* sizeSpin = new QSpinBox(&dialog);
        sizeSpin->setRange(8, 72);
        sizeSpin->setValue(currentSize);
        sizeSpin->setFixedHeight(36);
        mainLayout->addWidget(sizeSpin);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* applyButton = new QPushButton("Apply", &dialog);
        applyButton->setObjectName("applyBtn");
        QPushButton* deleteButton = new QPushButton("Delete Text", &dialog);
        deleteButton->setObjectName("deleteBtn");
        QPushButton* closeButton = new QPushButton("Close", &dialog);
        closeButton->setObjectName("closeBtn");
        buttonLayout->addWidget(applyButton);
        buttonLayout->addWidget(deleteButton);
        buttonLayout->addWidget(closeButton);
        mainLayout->addLayout(buttonLayout);
        QObject::connect(applyButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        QObject::connect(deleteButton, &QPushButton::clicked, &dialog, [&dialog]() {
            dialog.done(2);
        });

        const int result = dialog.exec();
        if (result == QDialog::Accepted) {
            ShapeHistoryApi::instance()->pushSnapshot(shapeId);
            QJsonObject updateData;
            updateData["text"] = textEdit->text();
            updateData["color"] = pickedColor.name();
            updateData["fontSize"] = sizeSpin->value();
            m_shapeApi->updateShape(shapeId, updateData);
            update();
        } else if (result == 2) {
            deleteShape(shapeId);
        }
        return;
    }

    // ============================================================
    // NORMAL SHAPE PROPERTIES (color + line width)
    // ============================================================
    QColor currentColor = QColor(props["color"].toString("#ff4444"));
    int currentWidth = props["lineWidth"].toInt(2);

    QDialog dialog(this);
    dialog.setWindowTitle("Shape Properties");
    dialog.setFixedWidth(320);
    dialog.setStyleSheet(
        "QDialog { background-color: #1e1e1e; }"
        "QLabel { color: white; font-size: 13px; font-weight: bold; }"
        "QSpinBox { background-color: #3c3c3c; color: white; padding: 8px;"
        " border: 1px solid #555; border-radius: 4px; font-size: 13px; }"
        "QPushButton#cancelBtn { background-color: #3c3c3c; color: white; padding: 10px;"
        " border: none; border-radius: 4px; font-size: 13px; }"
        "QPushButton#cancelBtn:hover { background-color: #4a4a4a; }"
        "QPushButton#applyBtn { background-color: #0078d7; color: white; padding: 10px;"
        " border: none; border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton#applyBtn:hover { background-color: #005a9e; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addWidget(new QLabel("Shape Color:", &dialog));

    QColor pickedColor = currentColor;
    QPushButton* colorButton = new QPushButton("Select Color", &dialog);
    colorButton->setFixedHeight(40);
    colorButton->setCursor(Qt::PointingHandCursor);
    auto updateColorButton = [colorButton](const QColor& c) {
        int brightness = (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000;
        QString textColor = brightness > 140 ? "black" : "white";
        colorButton->setStyleSheet(
            QString("QPushButton { background-color: %1; color: %2; border: none; "
                    "border-radius: 4px; font-size: 13px; font-weight: bold; }")
                .arg(c.name(), textColor));
    };
    updateColorButton(pickedColor);
    QObject::connect(colorButton, &QPushButton::clicked, [&dialog, &pickedColor, updateColorButton]() {
        QColor chosen = QColorDialog::getColor(pickedColor, &dialog, "Select Color");
        if (chosen.isValid()) { pickedColor = chosen; updateColorButton(pickedColor); }
    });
    mainLayout->addWidget(colorButton);

    mainLayout->addWidget(new QLabel("Border Thickness:", &dialog));
    QSpinBox* widthSpin = new QSpinBox(&dialog);
    widthSpin->setRange(1, 20);
    widthSpin->setValue(currentWidth);
    widthSpin->setFixedHeight(36);
    mainLayout->addWidget(widthSpin);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* applyButton = new QPushButton("Apply", &dialog);
    applyButton->setObjectName("applyBtn");
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    cancelButton->setObjectName("cancelBtn");
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    QObject::connect(applyButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        ShapeHistoryApi::instance()->pushSnapshot(shapeId);
        m_shapeApi->updateShapeColor(shapeId, pickedColor);
        m_shapeApi->updateShapeWidth(shapeId, widthSpin->value());
        update();
    }
}
QString MapWidget::getTileKey(int z, int x, int y) const {
    return QString("%1_%2_%3").arg(z).arg(x).arg(y);
}
double MapWidget::lonToX(double lon, int z) const {
    return ((lon + 180.0) / 360.0) * std::pow(2.0, z);
}
double MapWidget::latToY(double lat, int z) const {
    if (lat > 85.051128) lat = 85.051128;
    if (lat < -85.051128) lat = -85.051128;
    double rad = lat * M_PI / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI) / 2.0 * std::pow(2.0, z);
}
double MapWidget::xToLon(double x, int z) const {
    return x / std::pow(2.0, z) * 360.0 - 180.0;
}
double MapWidget::yToLat(double y, int z) const {
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, z);
    return (180.0 / M_PI) * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}
int MapWidget::findNearestEditPoint(const QString& shapeId, const QPointF& canvasPos) const
{
    if (shapeId.isEmpty())
        return -1;

    QJsonObject props = m_shapeApi->getShapeProperties(shapeId);
    if (props.isEmpty())
        return -1;

    const QString type = props[QStringLiteral("type")].toString();

    IShapeType* st = ShapeTypeRegistry::instance()->type(type);
    if (st)
        return st->findNearestEditPoint(props, canvasPos, this);
    return -1;
}
void MapWidget::drawEditHandles(QPainter& painter)
{
    if (m_editingShapeId.isEmpty()) return;
    QJsonObject props = m_shapeApi->getShapeProperties(m_editingShapeId);
    if (props.isEmpty()) return;

    const QString type = props.value(QStringLiteral("type")).toString();

    IShapeType* st = ShapeTypeRegistry::instance()->type(type);
    if (st)
        st->drawEditHandles(painter, props, this);
}
void MapWidget::selectShape(const QString& shapeId)
{
    m_selectedShapeId = shapeId;
    m_selectedShapeIds.clear();
    if (!shapeId.isEmpty())
        m_selectedShapeIds.insert(shapeId);
    emit shapeSelectionChanged(m_selectedShapeId);
    update();
}

void MapWidget::setSelectedShapes(const QStringList& shapeIds)
{
    m_selectedShapeIds = QSet<QString>(shapeIds.begin(), shapeIds.end());
    m_selectedShapeId = (shapeIds.size() == 1) ? shapeIds.first() : QString();
    update();
}

QStringList MapWidget::selectedShapeIds() const
{
    return m_selectedShapeIds.values();
}


void MapWidget::centerOnShape(const QString& shapeId)
{
    if (shapeId.isEmpty() || !m_shapeApi)
        return;
    const QJsonObject props = m_shapeApi->getShapeProperties(shapeId);
    if (props.isEmpty())
        return;
    const double lat = props.value(QStringLiteral("latitude")).toDouble();
    const double lon = props.value(QStringLiteral("longitude")).toDouble();
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        return;
    setCenter(lat, lon);
}
void MapWidget::copyShape(const QString& shapeId)
{
    QJsonObject props = m_shapeApi->getShapeProperties(shapeId);
    if (props.isEmpty())
        return;
    m_clipboardShape = props;
    m_clipboardShape.remove("id");
}

void MapWidget::pasteShapeAt(const QPointF& geoPos)
{
    if (m_clipboardShape.isEmpty())
        return;

    QJsonObject shape = m_clipboardShape;
    shape.remove("id");

    const double oldLon = shape["longitude"].toDouble();
    const double oldLat = shape["latitude"].toDouble();
    const double dLon = geoPos.x() - oldLon;
    const double dLat = geoPos.y() - oldLat;

    shape["longitude"] = geoPos.x();
    shape["latitude"] = geoPos.y();

    if (shape.contains("points") && shape["points"].isArray()) {
        QJsonArray pts = shape["points"].toArray();
        QJsonArray newPts;
        for (const QJsonValue& v : pts) {
            QJsonArray pt = v.toArray();
            if (pt.size() < 2)
                continue;
            QJsonArray np;
            np.append(pt[0].toDouble() + dLon);
            np.append(pt[1].toDouble() + dLat);
            newPts.append(np);
        }
        shape["points"] = newPts;
    }
    m_shapeApi->addShape(shape);
    update();
}
void MapWidget::setCoordSystem(const QString& sys)
{
    m_coordSystem = sys;
    update();
    emit coordSystemChanged(m_coordSystem);
}

QString MapWidget::formatCoord(double lat, double lon) const
{
    if (m_coordSystem == QLatin1String("UTM_AUTO")) {
        int zone = static_cast<int>(std::floor((lon + 180.0) / 6.0)) + 1;
        double easting = (lon - ((zone - 1) * 6 - 180 + 3))
                         * 111320.0 * std::cos(lat * M_PI / 180.0);
        double northing = lat * 110540.0;
        if (northing < 0)
            northing += 10000000.0;
        easting += 500000.0;
        return QString("Y: %1\nX: %2")
            .arg(northing, 0, 'f', 6)
            .arg(easting, 0, 'f', 6);
    }

    if (m_coordSystem == QLatin1String("MGRS")) {
        int zone = static_cast<int>(std::floor((lon + 180.0) / 6.0)) + 1;
        static const char bands[] = "CDEFGHJKLMNPQRSTUVWX";
        int bandIdx = static_cast<int>((lat + 80.0) / 8.0);
        if (bandIdx < 0) bandIdx = 0;
        if (bandIdx > 19) bandIdx = 19;
        char band = bands[bandIdx];
        double easting = (lon - ((zone - 1) * 6 - 180 + 3))
                             * 111320.0 * std::cos(lat * M_PI / 180.0) + 500000.0;
        double northing = lat * 110540.0;
        if (northing < 0) northing += 10000000.0;
        int e100 = static_cast<int>(easting) % 100000;
        int n100 = static_cast<int>(northing) % 100000;
        char col = 'A' + (static_cast<int>(easting / 100000) % 24);
        char row = 'A' + (static_cast<int>(northing / 100000) % 20);

        return QString("%1%2%3%4 %5 %6")
            .arg(zone)
            .arg(band)
            .arg(col)
            .arg(row)
            .arg(e100, 5, 10, QChar('0'))
            .arg(n100, 5, 10, QChar('0'));
    }
    return QString("Lat: %1\nLon: %2")
        .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6);

}
void MapWidget::startEditingShape(const QString& shapeId)
{
    if (!shapeLayerIsEditable(shapeId))
        return;
    if (shapeId.isEmpty())
        return;
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_selectedShapeId = shapeId;
    m_editingShapeId = shapeId;
    m_movingShapeId.clear();
    m_rotatingShapeId.clear();
    m_isMovingShape = false;
    m_isRotatingShape = false;
    m_editPointIndex = -1;
    setCursor(Qt::SizeAllCursor);
    emit shapeSelectionChanged(m_selectedShapeId);
    update();
}

void MapWidget::startMovingShape(const QString& shapeId)
{
    if (!shapeLayerIsEditable(shapeId))
        return;
    if (shapeId.isEmpty())
        return;
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_selectedShapeId = shapeId;
    m_movingShapeId = shapeId;
    m_editingShapeId.clear();
    m_rotatingShapeId.clear();
    m_isMovingShape = false;
    m_isRotatingShape = false;
    setCursor(Qt::OpenHandCursor);
    emit shapeSelectionChanged(m_selectedShapeId);
    update();
}

void MapWidget::startRotatingShape(const QString& shapeId)
{
    if (!shapeLayerIsEditable(shapeId))
        return;
    if (shapeId.isEmpty())
        return;
    if (m_isSelectingCenter) {
        m_isSelectingCenter = false;
    }
    m_selectedShapeId = shapeId;
    m_rotatingShapeId = shapeId;
    m_editingShapeId.clear();
    m_movingShapeId.clear();
    m_isMovingShape = false;
    m_isRotatingShape = false;
    setCursor(Qt::PointingHandCursor);
    emit shapeSelectionChanged(m_selectedShapeId);
    update();
}
void MapWidget::drawHistoryPreview(QPainter& painter)
{
    ShapeHistoryApi* hist = ShapeHistoryApi::instance();
    if (!hist->isPreviewActive())
        return;
    QJsonObject obj = hist->previewGeometry();
    if (obj.isEmpty())
        return;
    IShapeType* st = ShapeTypeRegistry::instance()->type(obj["type"].toString());
    if (!st)
        return;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 200, 80, 220),
                        qMax(2, obj["lineWidth"].toInt(2)),
                        Qt::DashLine));
    painter.setBrush(QColor(0, 200, 80, 55));
    st->draw(painter, obj, this);
    QPointF pos = geoToCanvas(obj["latitude"].toDouble(),
                              obj["longitude"].toDouble());
    painter.resetTransform();
    painter.setPen(QColor(0, 180, 70));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(pos + QPointF(12, -12), QStringLiteral("PREVIEW"));
    painter.restore();
}
QString MapWidget::findBitmapAtCanvas(const QPoint& canvasPos) const
{
    QJsonArray shapes = m_shapeApi->getAllShapes();
    QString hitId;
    for (int i = shapes.size() - 1; i >= 0; --i) {
        QJsonObject obj = shapes.at(i).toObject();
        if (obj["type"].toString() != QLatin1String("bitmap"))
            continue;

        const double lon = obj["longitude"].toDouble();
        const double lat = obj["latitude"].toDouble();
        const double rotation = obj["rotation"].toDouble(0.0);
        const double w = obj["width"].toDouble();
        const double h = obj.contains("height") ? obj["height"].toDouble() : w;

        const QPointF tl = geoToCanvas(lat + h / 2.0, lon - w / 2.0);
        const QPointF br = geoToCanvas(lat - h / 2.0, lon + w / 2.0);
        QRectF rect = QRectF(tl, br).normalized();
        rect.adjust(-4, -4, 4, 4);

        const QPointF pos = geoToCanvas(lat, lon);
        QPointF testPt = canvasPos;
        if (std::fabs(rotation) > 0.01) {
            const double rad = -rotation * M_PI / 180.0;
            const double c = std::cos(rad);
            const double sIn = std::sin(rad);
            const double dx = canvasPos.x() - pos.x();
            const double dy = canvasPos.y() - pos.y();
            testPt = QPointF(pos.x() + dx * c - dy * sIn,
                             pos.y() + dx * sIn + dy * c);
        }
        if (rect.contains(testPt)) {
            hitId = obj["id"].toString();
            break;
        }
    }
    return hitId;
}double MapWidget::metersPerPixel() const
{
    const double latRad = m_centerLat * M_PI / 180.0;
    return 156543.03392 * std::cos(latRad) / std::pow(2.0, m_zoom);
}

void MapWidget::drawMapOverlay(QPainter& painter)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    QPointF mouseGeo = canvasToGeo(m_lastMousePos);
    const double lat = mouseGeo.y();
    const double lon = mouseGeo.x();
    QString displayText = formatCoord(lat, lon);
    if (ElevationData::instance()->isLoaded()) {
        const double alt = ElevationData::instance()->getAltitude(lat, lon);
        if (!std::isnan(alt)) {
            displayText += QString("\nAlt: %1 m").arg(alt, 0, 'f', 1);
        } else {
            displayText += QStringLiteral("\nAlt: N/A");
        }
    }
    QFont infoFont("Arial", 10);
    painter.setFont(infoFont);
    QFontMetrics fm(infoFont);
    QRect textRect = fm.boundingRect(QRect(0, 0, 320, 120),
                                     Qt::AlignLeft | Qt::AlignTop,
                                     displayText);
    textRect.adjust(-10, -6, 10, 6);
    textRect.moveTo(12, 12);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 40, 50, 160));
    painter.drawRoundedRect(textRect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(textRect.adjusted(10, 6, -10, -6),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     displayText);
    const double mpp = metersPerPixel();
    if (mpp > 0.0 && std::isfinite(mpp)) {
        const double targetPx = 100.0;
        static const double nice[] = {
            1, 2, 5, 10, 20, 50, 100, 200, 500,
            1000, 2000, 5000, 10000, 20000, 50000,
            100000, 200000, 500000, 1000000, 2000000, 5000000
        };
        double best = nice[0];
        double bestScore = 1e18;
        for (double n : nice) {
            const double barPxCandidate = n / mpp;
            if (barPxCandidate < 40.0 || barPxCandidate > 180.0)
                continue;
            const double score = std::abs(barPxCandidate - targetPx);
            if (score < bestScore) {
                bestScore = score;
                best = n;
            }
        }

        if (bestScore > 1e17) {
            const double rawMeters = mpp * targetPx;
            best = nice[0];
            for (double n : nice) {
                if (n <= rawMeters)
                    best = n;
                else
                    break;
            }
        }

        const double barPx = best / mpp;
        QString label;
        if (best >= 1000.0)
            label = QString("%1 km").arg(best / 1000.0, 0, 'f', best >= 10000 ? 0 : 1);
        else
            label = QString("%1 m").arg(best, 0, 'f', 0);
        const int barH = 8;
        const int margin = 16;
        const int boxPad = 10;
        QFont scaleFont("Arial", 10, QFont::Bold);
        painter.setFont(scaleFont);
        QFontMetrics sfm(scaleFont);
        QRect labelBr = sfm.boundingRect(label);
        const int boxW = qMax(static_cast<int>(barPx) + boxPad * 2,
                              labelBr.width() + boxPad * 2);
        const int boxH = barH + labelBr.height() + boxPad * 2 + 4;
        QRect box(width() - boxW - margin,
                  margin,
                  boxW, boxH);
        painter.setPen(QPen(QColor(180, 180, 180), 1));
        painter.setBrush(QColor(255, 255, 255, 230));
        painter.drawRoundedRect(box, 4, 4);
        const int barX = box.left() + (box.width() - static_cast<int>(barPx)) / 2;
        const int barY = box.bottom() - boxPad - barH;
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.drawRect(barX, barY, static_cast<int>(barPx), barH);
        painter.setBrush(Qt::white);
        painter.drawRect(barX, barY, 2, barH);
        painter.drawRect(barX + static_cast<int>(barPx) - 2, barY, 2, barH);
        painter.setPen(Qt::black);
        painter.drawText(box.left(), box.top() + boxPad,
                         box.width(), labelBr.height(),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         label);
    }
    painter.restore();
}

void MapWidget::reloadTiles()
{
    m_tileCache.clear();
    m_hasRasterBasemap = false;
    m_hasMbtiles = false;
    m_rasterBasemap = QPixmap();

    const TileSourceInfo info = m_mapApi->tileSourceInfo(m_mapApi->activeTileSource());
    const QString key = info.folderName;

    if (key.startsWith(QStringLiteral("image:"))) {
        loadRasterBasemap(key.mid(6));
    }
    else if (key.startsWith(QStringLiteral("mbtiles:"))) {
        loadMbtilesBasemap(key.mid(8));
        loadOfflineTiles();
    }
    else if (key.startsWith(QStringLiteral("gdal:"))) {
        loadGdalBasemap(key.mid(5));          // ← NAYA
    }
    else {
        loadOfflineTiles();
    }
    update();
}
void MapWidget::startSelectCenter()
{
    cancelDrawing();
    if (m_measureApi && m_measureApi->isMeasuring())
        stopMeasuring();
    if (m_isPlacingBitmap)
        cancelPlacingBitmap();
    m_boxZoomPending     = false;
    m_isBoxZooming       = false;
    m_boxZoomFollowMouse = false;
    m_hasSearchMarker = false;
    m_isSelectingCenter = false;
    setCenter(m_markedCenterLat, m_markedCenterLon);
    setCursor(Qt::ArrowCursor);
    update();
}
void MapWidget::drawCenterMarker(QPainter& painter)
{
    const QPointF c = geoToCanvas(m_markedCenterLat, m_markedCenterLon);
    if (!rect().adjusted(-40, -40, 40, 40).contains(c.toPoint()))
    return;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::white, 1.5));
    painter.setBrush(QColor(220, 30, 30));
    painter.drawEllipse(c, 6, 6);
    painter.restore();
}
void MapWidget::setGisInteractionEnabled(bool enabled)
{
    if (m_gisInteractionEnabled == enabled)
        return;
    m_gisInteractionEnabled = enabled;
    emit gisInteractionEnabledChanged(enabled);
    update();
}
void MapWidget::drawBoxZoomOverlay(QPainter& painter)
{
    if (!m_isBoxZooming)
        return;
    QRect r = QRect(m_boxZoomStart, m_boxZoomCurrent).normalized();
    if (r.width() < 2 || r.height() < 2)
        return;
    painter.save();
    painter.setPen(QPen(QColor(0, 180, 255), 1.5, Qt::DashLine));
    painter.setBrush(QColor(0, 180, 255, 40));
    painter.drawRect(r);
    painter.restore();
}
void MapWidget::zoomToCanvasRect(const QRect& canvasRect)
{
    QRect r = canvasRect.normalized();
    if (r.width() < 10 || r.height() < 10)
        return;

    QPointF tl = canvasToGeo(r.topLeft());
    QPointF br = canvasToGeo(r.bottomRight());
    const double minLon = qMin(tl.x(), br.x());
    const double maxLon = qMax(tl.x(), br.x());
    const double minLat = qMin(tl.y(), br.y());
    const double maxLat = qMax(tl.y(), br.y());

    const double centerLat = (minLat + maxLat) * 0.5;
    const double centerLon = (minLon + maxLon) * 0.5;

    const double scaleX = static_cast<double>(width())  / qMax(1, r.width());
    const double scaleY = static_cast<double>(height()) / qMax(1, r.height());
    const double scale  = qMin(scaleX, scaleY);
    int newZoom = m_zoom + static_cast<int>(std::floor(std::log2(scale)));
    newZoom = qBound(1, newZoom, 18);
    setCenter(centerLat, centerLon);
    setZoom(newZoom);
}
void MapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    if (!m_gisInteractionEnabled) {
        event->ignore();
        return;
    }
    if ((m_measureApi && m_measureApi->isMeasuring()) ||
        m_isDrawing || m_isPlacingBitmap || m_isSelectingCenter) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    m_boxZoomPending     = false;
    m_isBoxZooming       = true;
    m_boxZoomFollowMouse = true;
    m_boxZoomStart       = event->pos();
    m_boxZoomCurrent     = event->pos();
    setCursor(Qt::CrossCursor);
    update();
    event->accept();
}
double MapWidget::pixelSizeToDegrees(int pixels) const
{
    const double mpp = metersPerPixel();
    if (mpp <= 0.0 || !std::isfinite(mpp))
        return 0.01;
    const double meters = pixels * mpp;
    return meters / 110540.0;
}

void MapWidget::drawSearchMarker(QPainter& painter)
{
    if (!m_hasSearchMarker)
        return;
    const QPointF c = geoToCanvas(m_searchMarkerLat, m_searchMarkerLon);
    if (!rect().adjusted(-20, -20, 20, 20).contains(c.toPoint()))
        return;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::white, 1.5));
    painter.setBrush(QColor(220, 30, 30));
    painter.drawEllipse(c, 6, 6);
    painter.restore();
}
void MapWidget::clearSearchMarker()
{
    if (!m_hasSearchMarker)
        return;
    m_hasSearchMarker = false;
    update();
}

void MapWidget::goToLocation(double lat, double lon, bool showMarker)
{
    if (lat < -90.0)  lat = -90.0;
    if (lat >  90.0)  lat =  90.0;
    if (lon < -180.0) lon = -180.0;
    if (lon >  180.0) lon =  180.0;

    if (showMarker) {
        m_searchMarkerLat = lat;
        m_searchMarkerLon = lon;
        m_hasSearchMarker = true;
    } else {
        m_hasSearchMarker = false;
    }

    setCenter(lat, lon);
    update();
}
void MapWidget::loadRasterBasemap(const QString& imagePath)
{
    m_rasterBasemap = QPixmap();
    m_hasRasterBasemap = false;
    m_rasterBasemapPath.clear();

    if (imagePath.isEmpty() || !QFile::exists(imagePath))
        return;

    QPixmap px(imagePath);
    if (px.isNull()) {
        qWarning() << "Failed to load raster basemap:" << imagePath;
        return;
    }

    m_rasterBasemap = px;
    m_rasterBasemapPath = imagePath;
    m_hasRasterBasemap = true;

    const double span = 10.0;
    m_rasterWest  = m_centerLon - span;
    m_rasterEast  = m_centerLon + span;
    m_rasterSouth = m_centerLat - span;
    m_rasterNorth = m_centerLat + span;

    m_rasterWest  = qBound(-180.0, m_rasterWest, 180.0);
    m_rasterEast  = qBound(-180.0, m_rasterEast, 180.0);
    m_rasterSouth = qBound(-85.0,  m_rasterSouth, 85.0);
    m_rasterNorth = qBound(-85.0,  m_rasterNorth, 85.0);
}

void MapWidget::loadMbtilesBasemap(const QString& path)
{
    m_hasMbtiles = false;
    m_mbtilesPath.clear();
    m_mbtilesMinZoom = 0;
    m_mbtilesMaxZoom = 18;

    if (path.isEmpty() || !QFile::exists(path))
        return;

    // Unique connection name
    const QString connName = QStringLiteral("mbtiles_%1")
                                 .arg(reinterpret_cast<quintptr>(this));

    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(path);
        if (!db.open()) {
            qWarning() << "MBTiles open failed:" << db.lastError().text() << path;
            return;
        }

        // metadata
        QSqlQuery q(db);
        if (q.exec(QStringLiteral("SELECT name, value FROM metadata"))) {
            while (q.next()) {
                const QString key = q.value(0).toString().toLower();
                const QString val = q.value(1).toString();
                if (key == QLatin1String("minzoom"))
                    m_mbtilesMinZoom = val.toInt();
                else if (key == QLatin1String("maxzoom"))
                    m_mbtilesMaxZoom = val.toInt();
            }
        }

        // tiles table check
        if (!q.exec(QStringLiteral(
                "SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles LIMIT 1"))) {
            qWarning() << "MBTiles: no tiles table" << path;
            db.close();
            return;
        }
        db.close();
    }

    m_mbtilesPath = path;
    m_hasMbtiles = true;
    qDebug() << "MBTiles loaded:" << path
             << "zoom" << m_mbtilesMinZoom << "-" << m_mbtilesMaxZoom;
}

QPixmap MapWidget::fetchMbtilesTile(int z, int x, int y) const
{
    if (!m_hasMbtiles || m_mbtilesPath.isEmpty())
        return {};

    // MBTiles uses TMS Y (flipped)
    const int tmsY = (1 << z) - 1 - y;

    const QString connName = QStringLiteral("mbtiles_%1")
                                 .arg(reinterpret_cast<quintptr>(this));

    if (!QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(m_mbtilesPath);
        if (!db.open())
            return {};
    }

    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen())
        return {};

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT tile_data FROM tiles "
        "WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?"));
    q.addBindValue(z);
    q.addBindValue(x);
    q.addBindValue(tmsY);

    if (!q.exec() || !q.next())
        return {};

    const QByteArray blob = q.value(0).toByteArray();
    QPixmap px;
    px.loadFromData(blob);
    return px;
}
static void ensureGdalInitialized()
{
    static bool done = false;
    if (!done) {
        GDALAllRegister();
        done = true;
    }
}

bool MapWidget::extractGeoBoundsFromGdal(const QString& path,
                                         double& west, double& east,
                                         double& south, double& north)
{
    ensureGdalInitialized();

    GDALDataset* ds = static_cast<GDALDataset*>(
        GDALOpen(path.toUtf8().constData(), GA_ReadOnly));
    if (!ds) return false;

    double gt[6];
    if (ds->GetGeoTransform(gt) != CE_None) {
        GDALClose(ds);
        return false;
    }

    const int w = ds->GetRasterXSize();
    const int h = ds->GetRasterYSize();

    // Affine transform
    // gt[0] = top-left X (lon)
    // gt[3] = top-left Y (lat)
    // gt[1] = pixel width
    // gt[5] = pixel height (usually negative)
    west  = gt[0];
    east  = gt[0] + w * gt[1] + h * gt[2];
    north = gt[3];
    south = gt[3] + w * gt[4] + h * gt[5];

    // Normalize
    if (west > east)  std::swap(west, east);
    if (south > north) std::swap(south, north);

    const char* proj = ds->GetProjectionRef();
    if (proj && *proj) {
        OGRSpatialReference src, dst;
        const char* wkt = proj;
        src.importFromWkt(&wkt);
        dst.SetWellKnownGeogCS("WGS84");
        src.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
        dst.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

        if (!src.IsSame(&dst)) {
            OGRCoordinateTransformation* ct = OGRCreateCoordinateTransformation(&src, &dst);
            if (ct) {
                double xs[4] = { west, east, east, west };
                double ys[4] = { south, south, north, north };
                if (ct->Transform(4, xs, ys)) {
                    west  = *std::min_element(xs, xs + 4);
                    east  = *std::max_element(xs, xs + 4);
                    south = *std::min_element(ys, ys + 4);
                    north = *std::max_element(ys, ys + 4);
                }
                OCTDestroyCoordinateTransformation(ct);
            }
        }
    }

    GDALClose(ds);
    return true;
}

bool MapWidget::loadGdalBasemap(const QString& path)
{
    ensureGdalInitialized();

    m_rasterBasemap = QPixmap();
    m_hasRasterBasemap = false;
    m_rasterBasemapPath.clear();

    if (path.isEmpty() || !QFile::exists(path))
        return false;

    GDALDataset* ds = static_cast<GDALDataset*>(
        GDALOpen(path.toUtf8().constData(), GA_ReadOnly));
    if (!ds) {
        qWarning() << "GDAL failed to open:" << path;
        return false;
    }

    // ---- Geo bounds ----
    double west, east, south, north;
    if (!extractGeoBoundsFromGdal(path, west, east, south, north)) {
        // fallback – current center ± span
        const double span = 5.0;
        west  = m_centerLon - span;
        east  = m_centerLon + span;
        south = m_centerLat - span;
        north = m_centerLat + span;
    }

    m_rasterWest  = west;
    m_rasterEast  = east;
    m_rasterSouth = south;
    m_rasterNorth = north;

    // ---- Image data (overview / full) ----
    const int srcW = ds->GetRasterXSize();
    const int srcH = ds->GetRasterYSize();
    const int bands = ds->GetRasterCount();

    // Limit size for memory (adjust as needed)
    const int maxDim = 4096;
    int dstW = srcW;
    int dstH = srcH;
    if (dstW > maxDim || dstH > maxDim) {
        const double scale = qMin(double(maxDim)/dstW, double(maxDim)/dstH);
        dstW = int(dstW * scale);
        dstH = int(dstH * scale);
    }

    QImage img(dstW, dstH, QImage::Format_RGB32);
    img.fill(Qt::black);

    // Read RGB (or first 3 bands)
    for (int b = 1; b <= qMin(3, bands); ++b) {
        GDALRasterBand* band = ds->GetRasterBand(b);
        if (!band) continue;

        QVector<quint8> buf(dstW * dstH);
        CPLErr err = band->RasterIO(GF_Read,
                                    0, 0, srcW, srcH,
                                    buf.data(), dstW, dstH,
                                    GDT_Byte, 0, 0);
        if (err != CE_None) continue;

        for (int y = 0; y < dstH; ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < dstW; ++x) {
                quint8 v = buf[y * dstW + x];
                QRgb pixel = line[x];
                if (b == 1) line[x] = qRgb(v, qGreen(pixel), qBlue(pixel));
                else if (b == 2) line[x] = qRgb(qRed(pixel), v, qBlue(pixel));
                else if (b == 3) line[x] = qRgb(qRed(pixel), qGreen(pixel), v);
            }
        }
    }

    // Single band → grayscale
    if (bands == 1) {
        GDALRasterBand* band = ds->GetRasterBand(1);
        QVector<quint8> buf(dstW * dstH);
        band->RasterIO(GF_Read, 0, 0, srcW, srcH,
                       buf.data(), dstW, dstH, GDT_Byte, 0, 0);
        for (int y = 0; y < dstH; ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < dstW; ++x) {
                quint8 v = buf[y * dstW + x];
                line[x] = qRgb(v, v, v);
            }
        }
    }

    GDALClose(ds);

    m_rasterBasemap = QPixmap::fromImage(img);
    m_rasterBasemapPath = path;
    m_hasRasterBasemap = !m_rasterBasemap.isNull();

    qDebug() << "GDAL basemap loaded:" << path
             << "bounds" << west << east << south << north
             << "size" << dstW << "x" << dstH;

    return m_hasRasterBasemap;

}
bool MapWidget::shapeLayerIsEditable(const QString& shapeId) const
{
    if (shapeId.isEmpty())
        return false;
    const QString layerId = LayerApi::instance()->getLayerOfShape(shapeId);
    if (layerId.isEmpty())
        return true; // no layer = treat as editable
    return LayerApi::instance()->isEditable(layerId);
}
