

#ifndef LAYERINFORMATIONDIALOG_H
#define LAYERINFORMATIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>

class LayerInformationDialog : public QDialog
{
    Q_OBJECT

public:
    struct MapLayerInfo {
        QString name;
        QString id;
        int zoomMin = 0;
        int zoomMax = 0;
        QString tileUrl;
        bool isCustom = false;
        qreal opacity = -1.0;
        QString resolution = "N/A";
        QString type = "";
    };

    explicit LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent = nullptr);

signals:
    void layerEdited(int index, const MapLayerInfo& updatedLayer);

private slots:
    void updateLayerDetails(QListWidgetItem* item);
    void editLayer();

private:
    void setupUi();
    QList<MapLayerInfo>& mapLayers;
    QListWidget* layerList;
    QLabel* nameLabel;
    QLabel* idLabel;
    QLabel* zoomMinLabel;
    QLabel* zoomMaxLabel;
    QLabel* tileUrlLabel;
    QLabel* isCustomLabel;
    QLabel* opacityLabel;
    QLabel* resolutionLabel;
    QLabel* typeLabel;
    QPushButton* editButton;
    int currentLayerIndex;


};

#endif // LAYERINFORMATIONDIALOG_H
