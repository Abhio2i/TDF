#ifndef LAYER_API_H
#define LAYER_API_H

#include <QObject>
#include <QString>
#include <QStringList>

class LayerApi : public QObject {
    Q_OBJECT

public:
    static LayerApi* instance();

    Q_INVOKABLE QString addLayer(const QString& name);
    Q_INVOKABLE bool deleteLayer(const QString& id);
    Q_INVOKABLE bool renameLayer(const QString& id, const QString& newName);
    Q_INVOKABLE QStringList getLayers() const;
    Q_INVOKABLE QStringList getLayerNames() const;
    Q_INVOKABLE QString getLayerName(const QString& id) const;
    Q_INVOKABLE bool layerExists(const QString& id) const;

    Q_INVOKABLE bool setVisibility(const QString& id, bool visible);
    Q_INVOKABLE bool isVisible(const QString& id) const;
    // Editable (lock) — imported layers start locked
    Q_INVOKABLE bool setEditable(const QString& id, bool editable);
    Q_INVOKABLE bool isEditable(const QString& id) const;

    Q_INVOKABLE void setActiveLayer(const QString& id);
    Q_INVOKABLE QString getActiveLayer() const;

    Q_INVOKABLE bool assignShapeToLayer(const QString& shapeId, const QString& layerId);
    Q_INVOKABLE bool removeShapeFromLayer(const QString& shapeId);
    Q_INVOKABLE QString getLayerOfShape(const QString& shapeId) const;
    Q_INVOKABLE QStringList getShapesOfLayer(const QString& layerId) const;

    Q_INVOKABLE bool exportLayer(const QString& layerId, const QString& format, const QString& filePath);
    Q_INVOKABLE bool importLayer(const QString& filePath, const QString& format, const QString& layerName = QString());
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

signals:
    void layerAdded(const QString& id);
    void layerDeleted(const QString& id);
    void layerRenamed(const QString& id, const QString& newName);
    void layerVisibilityChanged(const QString& id, bool visible);
    void layerEditableChanged(const QString& id, bool editable);
    void activeLayerChanged(const QString& id);
    void shapeAssigned(const QString& shapeId, const QString& layerId);
    void layersReset();

private:
    explicit LayerApi(QObject* parent = nullptr);
    ~LayerApi();
    static LayerApi* m_instance;
};

#endif
