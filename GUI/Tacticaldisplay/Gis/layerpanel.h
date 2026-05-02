/* ========================================================================= */
/* File: layerpanel.cpp                                                     */
/* Written by: Waris Ali                                                   */
/* ========================================================================= */

#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QMap>
#include <QCheckBox>
#include <QPushButton>
#include <QProcess>
#include <QPixmap>
#include <QPainter>
#include <QLabel>

// Forward declaration
class CanvasWidget;
class GISlib;
struct MeshEntry;

// %%% Raster Layer Data Structure %%%
struct RasterLayer {
    QString  name;
    QString  filePath;
    QString shapeId;
    mutable int nativeHeight = 0;
    QPixmap  pixmap;
    mutable QPixmap displayPixmap;
    mutable int     lastRenderZoom   = -1;
    mutable int     lastCanvasWidth  = -1;
    mutable int     lastCanvasHeight = -1;
    double   minLon = 0.0;
    double   minLat = 0.0;
    double   maxLon = 0.0;
    double   maxLat = 0.0;
    bool     visible = true;
    double   opacity = 1.0;

};

// %%% Class Definition %%%
/* Panel for managing layers with visibility toggle */
class LayerPanel : public QDockWidget
{
    Q_OBJECT

public:
    // Initialize layer panel

    explicit LayerPanel(QWidget *parent = nullptr);
    // Clean up resources
    ~LayerPanel();
    // static void runUnitTestsOnce();
    // Get tree widget
    QTreeWidget* getTreeWidget() const { return layerTree; }

    // Set canvas widget reference for export / raster-draw functionality
    void setCanvasWidget(CanvasWidget* canvas) { m_canvasWidget = canvas; }

    // %%% Active Layer Management %%%
    // Get currently active layer name
    QString getActiveLayer() const { return activeLayerName; }

    // Set active layer by name
    void setActiveLayer(const QString& layerName);

    // Get active layer item
    QTreeWidgetItem* getActiveLayerItem() const;

    // %%% Shape Management %%%
    // Add shape to specified layer (or active layer if empty)
    void addShapeToLayer(const QString& shapeId, const QString& shapeType,
                         const QString& layerName = "");

    // Remove shape from layer
    void removeShapeFromLayer(const QString& shapeId);

    // Get layer name for a shape
    QString getLayerForShape(const QString& shapeId) const;

    // Get/Set custom display name for a shape
    QString getShapeDisplayName(const QString& shapeId) const;
    void setShapeDisplayName(const QString& shapeId, const QString& displayName);

    // Update shape count display for a layer
    void updateLayerShapeCount(const QString& layerName);

    // Select and scroll to a shape item in the tree by its shapeId
    void selectShapeInPanel(const QString& shapeId);

    // Move a shape from its current layer to a different layer
    void moveShapeToLayer(const QString& shapeId, const QString& targetLayerName);

    // %%% Layer Queries %%%
    // Check if layer exists
    bool layerExists(const QString& layerName) const;

    // Get all shapes in a layer
    QStringList getShapesInLayer(const QString& layerName) const;

    // %%% Visibility Management %%%
    // Check if layer is visible
    bool isLayerVisible(const QString& layerName) const;

    // Set layer visibility
    void setLayerVisibility(const QString& layerName, bool visible);

    // %%% Raster Layer Management %%%
    // Draw all visible raster layers onto painter using GISlib for projection
    void drawRasterLayers(QPainter& painter, GISlib* gislib) const;

    // %%% Serialization Methods %%%
    // Serialize layer data to JSON for persistence across editors
    QJsonObject toJson() const;

    // Deserialize layer data from JSON
    void fromJson(const QJsonObject& json);

    bool exportToFlatGeobuf(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToCSV(const QString& layerName, const QStringList& shapeIds, const QString& filePath);

    // script use by amjad
    void addLayerFromScript(const QString& name);
    QString generateFriendlyShapeName(const QString& shapeId, const QString& shapeType) const;
     void selectRasterInPanel(const QString& shapeId);
    void setSuppressCenter(bool suppress) { m_suppressCenter = suppress; }

signals:
    // Emitted when active layer changes
    void activeLayerChanged(const QString& newLayerName);

    // Emitted when a layer is added
    void layerAdded(const QString& layerName);

    // Emitted when a layer is removed
    void layerRemoved(const QString& layerName);

    // Emitted just before a layer is removed, carries all shapeIds to delete from canvas
    void layerWithShapesRemoved(const QStringList& shapeIds);

    // Emitted when a shape is moved from one layer to another
    void shapeMovedToLayer(const QString& shapeId,
                           const QString& fromLayer,
                           const QString& toLayer);

    // Emitted when layer visibility changes
    void layerVisibilityChanged(const QString& layerName, bool visible);

    // Emitted when a raster layer is added or its visibility changes
    // (canvas connects to this to trigger a repaint)
    void rasterLayerChanged();
    void geoJsonLayerRemoved(const QString& layerName);
    void shapeClicked(const QString& shapeId);
protected:
    // Intercepts double-click on layer name labels for inline rename
    bool eventFilter(QObject* obj, QEvent* event) override;
public slots:
    void updateRasterLayerFromShape(const QString& shapeId);

private slots:
    // Show context menu on right-click
    void showContextMenu(const QPoint &pos);
    // Add new vector layer under selected layer
    void addLayer();
    // Add new raster layer (file dialog → metadata read → add to panel)
    void addRasterLayer();
    // Remove selected layer
    void removeLayer();
    // Rename selected layer
    void renameLayer();
    // Export layer to GIS formats
    void exportLayer();
    // Rename a layer by name (shared by menu action + double-click)
    void renameLayerByName(const QString& targetName);
    // Apply confirmed rename across all internal maps + UI
    void applyLayerRename(const QString& oldName, const QString& newName, bool isRaster);
    // Handle layer selection change
    void onLayerSelectionChanged();
    // Handle visibility toggle clicked
    void onVisibilityToggleClicked(const QString& layerName);
    // Rename a shape item (triggered from context menu)
    void renameShape(const QString& shapeId);
    void onShapeItemClicked(QTreeWidgetItem* item, int column);
private:
    // %%% UI Components %%%
    bool m_suppressCenter = false;
    QTreeWidget *layerTree = nullptr;
    CanvasWidget *m_canvasWidget = nullptr;
    QMenu *contextMenu = nullptr;
    QMap<QString, QPushButton*> expandButtons;
    // Name labels for vector layers — updated directly on rename so the
    // item widget shows the new name without rebuilding the whole widget.
    QMap<QString, QLabel*>      layerNameLabels;
    // Name labels for raster layers
    QMap<QString, QLabel*>      rasterNameLabels;

    // Menu actions
    QAction *addLayerAction       = nullptr;
    QAction *addRasterLayerAction = nullptr;
    QAction *removeLayerAction    = nullptr;
    QAction *renameLayerAction    = nullptr;
    QAction *exportLayerAction    = nullptr;

    // %%% Vector Layer Data %%%
    QString activeLayerName;
    QMap<QString, QStringList>      layerShapes;
    QMap<QString, QString>          shapeToLayer;
    QMap<QString, QTreeWidgetItem*> layerItems;
    QMap<QString, bool>             layerVisibility;
    QMap<QString, QWidget*>         visibilityToggleWidgets;
    QMap<QString, QString>          shapeDisplayNames;  // shapeId → custom display name

    // %%% Raster Layer Data %%%
    // Ordered list of raster layer names (insertion order = draw order, bottom-to-top)
    QList<QString>                  rasterLayerOrder;
    // Map: raster layer name → RasterLayer data
    QMap<QString, RasterLayer>      rasterLayers;
    // Map: raster layer name → tree widget item
    QMap<QString, QTreeWidgetItem*> rasterLayerItems;
    // Map: raster layer name → visibility toggle widget
    QMap<QString, QWidget*>         rasterVisibilityWidgets;
    // Map: raster layer name → expand button
    QMap<QString, QPushButton*>     rasterExpandButtons;

    // Root "Layers" item
    QTreeWidgetItem *rootLayersItem = nullptr;

    // %%% Setup Methods %%%
    void setupUI();
    void setupContextMenu();

    // %%% Vector Helper Methods %%%
    QTreeWidgetItem* findLayerItem(const QString& layerName) const;
    QString getFullLayerName(QTreeWidgetItem* item) const;
    void updateActiveLayerVisual(QTreeWidgetItem* item);
    void clearActiveLayerVisual();
    void addShapeItemToTree(const QString& layerName, const QString& shapeId,
                            const QString& shapeType);
    void removeShapeItemFromTree(const QString& shapeId);
    void createVisibilityToggle(const QString& layerName, QTreeWidgetItem* item);
    void updateVisibilityToggleIcon(const QString& layerName, bool visible);
    QString getShapeTypeFromId(const QString& shapeId) const;
    bool isLayerItem(QTreeWidgetItem* item) const;
    bool isShapeItem(QTreeWidgetItem* item) const;
    bool isValidShapeToLayerDrop(QTreeWidgetItem* dragged, QTreeWidgetItem* target) const;
    QTreeWidgetItem* resolveDropTargetLayer(const QPoint& viewportPos) const;

    // %%% Raster Helper Methods %%%
    // Create a tree item + visibility toggle for a raster layer
    void createRasterLayerItem(const QString& layerName);
    // Update the raster visibility toggle button icon
    void updateRasterVisibilityIcon(const QString& layerName, bool visible);
    // Toggle raster visibility when its button is clicked
    void onRasterVisibilityToggleClicked(const QString& layerName);
    // Try to read georeferencing from a world-file (.tfw / .jgw / .pgw etc.)
    bool readWorldFile(const QString& imagePath, RasterLayer& out);
    // Try to parse a GeoTIFF's geographic extents using Qt/GDAL fallback
    bool readGeoTiffExtents(const QString& imagePath, RasterLayer& out);

    // %%% Export Helper Methods %%%
    bool exportToGeoJSON(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToKML(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToGML(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToShapefile(const QString& layerName, const QStringList& shapeIds, const QString& filePath);

    QJsonObject createGeoJSONFeature(const MeshEntry& entry, const QString& shapeId);
    QString createKMLPlacemark(const MeshEntry& entry, const QString& shapeId);
    QString createGMLFeature(const MeshEntry& entry, const QString& shapeId);

    QJsonObject getGeometryAsGeoJSON(const MeshEntry& entry);
    QString getGeometryAsKML(const MeshEntry& entry);
    QString getGeometryAsGML(const MeshEntry& entry);
    QString getShapeType(const MeshEntry& entry);

    void writeShapefileHeader(QDataStream& stream, qint32 fileLength,
                              int shapeType, double minX, double minY,
                              double maxX, double maxY);
    QByteArray createShapefileRecord(const MeshEntry* entry, int shapeType,
                                     double& minX, double& minY,
                                     double& maxX, double& maxY);
    bool writeDBF(const QString& filePath, const QVector<const MeshEntry*>& shapes);
    void writeDBFFieldDescriptor(QDataStream& stream, const QString& name,
                                 char type, int length);
    bool exportShapesByType(const QVector<const MeshEntry*>& shapes,
                            const QString& filePath,
                            const QString& layerName,
                            int shapeType);
    QString createCSVLine(const MeshEntry& entry);
    QString createWKTGeometry(const MeshEntry& entry);
    QMap<QString, QString> shapeToRasterLayer;
    void removeRasterLayer(const QString& layerName);
};

#endif // LAYERPANEL_H
