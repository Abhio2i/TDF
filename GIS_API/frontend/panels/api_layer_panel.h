#ifndef API_LAYER_PANEL_H
#define API_LAYER_PANEL_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QMap>
#include <QKeyEvent>

class ApiLayerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ApiLayerPanel(QWidget* parent = nullptr);
    ~ApiLayerPanel();
    void refresh();
    void selectShape(const QString& shapeId);

signals:
    void layerVisibilityChanged(const QString& layerId, bool visible);
      void shapesSelectionChanged(const QStringList& shapeIds);
    void shapeClicked(const QString& shapeId);
    void requestImport();
    void requestExport(const QString& layerId);
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onContextMenu(const QPoint& pos);
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemChanged(QTreeWidgetItem* item, int column);
        void onSelectionChanged();

private:
    void setupUi();
    void rebuildTree();
    void addLayerItem(const QString& layerId);
    void addShapeItem(QTreeWidgetItem* parent, const QString& shapeId);
    void deleteSelectedItems();
    QTreeWidget* m_tree = nullptr;
    QTreeWidgetItem* m_root = nullptr;
    QMap<QString, QTreeWidgetItem*> m_layerItems;
};

#endif // API_LAYER_PANEL_H
