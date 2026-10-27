#ifndef LAYER_MANAGER_H
#define LAYER_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

struct LayerInfo {
    QString id;
    QString name;
    bool visible = true;
    bool editable = true;
    QStringList shapeIds;
};

class LayerManager : public QObject {
    Q_OBJECT

public:
    static LayerManager* instance();

    // Layer CRUD
    QString addLayer(const QString& name);
    bool deleteLayer(const QString& id);
    bool renameLayer(const QString& id, const QString& newName);
    QStringList getLayerIds() const;
    QStringList getLayerNames() const;
    LayerInfo getLayer(const QString& id) const;
    bool layerExists(const QString& id) const;
    QString findLayerIdByName(const QString& name) const;

    // Visibility
    bool setLayerVisibility(const QString& id, bool visible);
    bool isLayerVisible(const QString& id) const;
    // EDITABLE
    bool setLayerEditable(const QString& id, bool editable);
    bool isLayerEditable(const QString& id) const;

    // Shape ↔ Layer
    bool addShapeToLayer(const QString& shapeId, const QString& layerId);
    bool removeShapeFromLayer(const QString& shapeId);
    QString getLayerForShape(const QString& shapeId) const;
    QStringList getShapesInLayer(const QString& layerId) const;
    void clearShapesOfLayer(const QString& layerId);

    // Active layer
    void setActiveLayer(const QString& id);
    QString getActiveLayer() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
    void clearAllLayers();


signals:
    void layerAdded(const QString& id);
    void layerDeleted(const QString& id);
    void layerRenamed(const QString& id, const QString& newName);
    void layerVisibilityChanged(const QString& id, bool visible);
    void layerEditableChanged(const QString& id, bool editable);
    void shapeAssignedToLayer(const QString& shapeId, const QString& layerId);
    void shapeRemovedFromLayer(const QString& shapeId, const QString& layerId);
    void activeLayerChanged(const QString& id);

private:
    explicit LayerManager(QObject* parent = nullptr);
    static LayerManager* m_instance;
    QMap<QString, LayerInfo> m_layers;
    QVector<QString> m_layerOrder;
    QMap<QString, QString> m_shapeToLayer;
    QString m_activeLayerId;
    int m_counter = 0;
    QString generateId();
};

#endif // LAYER_MANAGER_H
