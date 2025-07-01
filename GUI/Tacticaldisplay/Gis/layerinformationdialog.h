

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
    // Structure to hold map layer information
    struct MapLayerInfo {
        QString name;
        QString id;
        int zoomMin = 0;
        int zoomMax = 0;
        QString tileUrl;
        bool isCustom = false;
        qreal opacity = -1.0; // -1.0 indicates not set (display "N/A")
        QString resolution = "N/A"; // Default to "N/A" if not specified
        QString type = ""; // Empty string indicates "N/A"
    };

    explicit LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent = nullptr);

signals:
    void layerEdited(int index, const MapLayerInfo& updatedLayer);

private slots:
    void updateLayerDetails(QListWidgetItem* item);
    void editLayer();

private:
    void setupUi();
    QList<MapLayerInfo>& mapLayers; // Reference to allow updates
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
