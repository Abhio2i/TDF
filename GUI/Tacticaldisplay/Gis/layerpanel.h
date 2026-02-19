
/* ========================================================================= */
/* File: layerpanel.h                                                       */
/* Purpose: Enhanced layer panel with visibility toggle for layers          */
/* Written by: Waris Ali                                                */
/* Modified by: Added visibility toggle feature                            */
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
#include <QProcess>

// Forward declaration
class CanvasWidget;
struct MeshEntry;

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

    // Get tree widget
    QTreeWidget* getTreeWidget() const { return layerTree; }

    // Set canvas widget reference for export functionality
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

    // Update shape count display for a layer
    void updateLayerShapeCount(const QString& layerName);

    // %%% Layer Queries %%%
    // Check if layer exists
    bool layerExists(const QString& layerName) const;

    // Get all shapes in a layer
    QStringList getShapesInLayer(const QString& layerName) const;

    // %%% NEW: Visibility Management %%%
    // Check if layer is visible
    bool isLayerVisible(const QString& layerName) const;

    // Set layer visibility
    void setLayerVisibility(const QString& layerName, bool visible);

    // %%% Serialization Methods %%%
    // Serialize layer data to JSON for persistence across editors
    QJsonObject toJson() const;

    // Deserialize layer data from JSON
    void fromJson(const QJsonObject& json);
    // bool exportToTopoJSON(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToFlatGeobuf(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToCSV(const QString& layerName, const QStringList& shapeIds, const QString& filePath);

signals:
    // Emitted when active layer changes
    void activeLayerChanged(const QString& newLayerName);

    // Emitted when a layer is added
    void layerAdded(const QString& layerName);

    // Emitted when a layer is removed
    void layerRemoved(const QString& layerName);

    // NEW: Emitted when layer visibility changes
    void layerVisibilityChanged(const QString& layerName, bool visible);

private slots:
    // Show context menu on right-click
    void showContextMenu(const QPoint &pos);
    // Add new layer under selected layer
    void addLayer();
    // Remove selected layer
    void removeLayer();
    // Rename selected layer
    void renameLayer();
    // Export layer to GIS formats
    void exportLayer();
    // Handle layer selection change
    void onLayerSelectionChanged();

    // NEW: Handle visibility toggle clicked
    void onVisibilityToggleClicked(const QString& layerName);

private:
    // %%% UI Components %%%
    // Tree widget for layer hierarchy
    QTreeWidget *layerTree = nullptr;

    // Canvas widget reference for shape data access
    CanvasWidget *m_canvasWidget = nullptr;

    // Context menu for layer operations
    QMenu *contextMenu = nullptr;
  QMap<QString, QPushButton*> expandButtons;
    // Menu actions
    QAction *addLayerAction = nullptr;
    QAction *removeLayerAction = nullptr;
    QAction *renameLayerAction = nullptr;
    QAction *exportLayerAction = nullptr;

    // %%% Layer Data %%%
    // Currently active layer name
    QString activeLayerName;

    // Map: layer name -> list of shape IDs
    QMap<QString, QStringList> layerShapes;

    // Map: shape ID -> layer name (reverse lookup)
    QMap<QString, QString> shapeToLayer;

    // Map: layer name -> tree widget item
    QMap<QString, QTreeWidgetItem*> layerItems;

    // NEW: Map: layer name -> visibility state
    QMap<QString, bool> layerVisibility;

    // NEW: Map: layer name -> visibility toggle widget
    QMap<QString, QWidget*> visibilityToggleWidgets;

    // Root "Layers" item
    QTreeWidgetItem *rootLayersItem = nullptr;

    // %%% Setup Methods %%%
    // Initialize UI components
    void setupUI();
    // Initialize context menu
    void setupContextMenu();

    // %%% Helper Methods %%%
    // Find layer item by name
    QTreeWidgetItem* findLayerItem(const QString& layerName) const;

    // Get full layer name (handles hierarchical layers)
    QString getFullLayerName(QTreeWidgetItem* item) const;

    // Update visual indicator for active layer
    void updateActiveLayerVisual(QTreeWidgetItem* item);

    // Clear active layer visual indicator
    void clearActiveLayerVisual();

    // Add shape item under layer in tree
    void addShapeItemToTree(const QString& layerName, const QString& shapeId,
                            const QString& shapeType);

    // Remove shape item from tree
    void removeShapeItemFromTree(const QString& shapeId);

    // NEW: Create visibility toggle for a layer
    void createVisibilityToggle(const QString& layerName, QTreeWidgetItem* item);

    // NEW: Update visibility toggle icon
    void updateVisibilityToggleIcon(const QString& layerName, bool visible);

    // Extract shape type from shape ID (for restoration)
    QString getShapeTypeFromId(const QString& shapeId) const;

    // %%% Export Helper Methods %%%
    // Export to different GIS formats
    bool exportToGeoJSON(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToKML(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToGML(const QString& layerName, const QStringList& shapeIds, const QString& filePath);
    bool exportToShapefile(const QString& layerName, const QStringList& shapeIds, const QString& filePath);

    // Create format-specific features
    QJsonObject createGeoJSONFeature(const MeshEntry& entry, const QString& shapeId);
    QString createKMLPlacemark(const MeshEntry& entry, const QString& shapeId);
    QString createGMLFeature(const MeshEntry& entry, const QString& shapeId);

    // Geometry conversion helpers
    QJsonObject getGeometryAsGeoJSON(const MeshEntry& entry);
    QString getGeometryAsKML(const MeshEntry& entry);
    QString getGeometryAsGML(const MeshEntry& entry);
    QString getShapeType(const MeshEntry& entry);

    // Write shapefile header (100 bytes)
    void writeShapefileHeader(QDataStream& stream, qint32 fileLength,
                              int shapeType, double minX, double minY,
                              double maxX, double maxY);

    // Create a single shape record in shapefile binary format
    QByteArray createShapefileRecord(const MeshEntry* entry, int shapeType,
                                     double& minX, double& minY,
                                     double& maxX, double& maxY);

    // Write DBF (dBASE) attribute file
    bool writeDBF(const QString& filePath, const QVector<const MeshEntry*>& shapes);

    // Write DBF field descriptor (32 bytes)
    void writeDBFFieldDescriptor(QDataStream& stream, const QString& name,
                                 char type, int length);
    bool exportShapesByType(const QVector<const MeshEntry*>& shapes,
                            const QString& filePath,
                            const QString& layerName,
                            int shapeType);
    // // TopoJSON helpers
    // QJsonObject createTopoJSONGeometry(const MeshEntry& entry, const QString& shapeId);

    // bool createManualTopoJSON(const QString& layerName, const QString& geoJsonPath, const QString& topoJsonPath);


    // CSV helpers
    QString createCSVLine(const MeshEntry& entry);
    QString createWKTGeometry(const MeshEntry& entry);
};

#endif // LAYERPANEL_H
