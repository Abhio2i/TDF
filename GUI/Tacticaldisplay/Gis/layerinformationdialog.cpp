#include "layerinformationdialog.h"
#include "GUI/Tacticaldisplay/Gis/custommapdialog.h"

LayerInformationDialog::LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent)
    : QDialog(parent), mapLayers(layers), currentLayerIndex(-1)
{
    setWindowTitle("Map Layer Information");
    setMinimumWidth(600);
    setupUi();
}

void LayerInformationDialog::setupUi()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Left side: List of layer names
    layerList = new QListWidget(this);
    layerList->setMinimumWidth(200);
    layerList->setMaximumWidth(250);
    for (const auto& layer : mapLayers) {
        layerList->addItem(layer.name);
    }
    mainLayout->addWidget(layerList);

    // Right side: Layer details
    QWidget* detailsWidget = new QWidget(this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsWidget);
    QFormLayout* formLayout = new QFormLayout();
    nameLabel = new QLabel(this);
    idLabel = new QLabel(this);
    zoomMinLabel = new QLabel(this);
    zoomMaxLabel = new QLabel(this);
    tileUrlLabel = new QLabel(this);
    isCustomLabel = new QLabel(this);
    opacityLabel = new QLabel(this);
    // resolutionLabel = new QLabel(this);
    typeLabel = new QLabel(this);

    formLayout->addRow("Name:", nameLabel);
    formLayout->addRow("ID:", idLabel);
    formLayout->addRow("Zoom Min:", zoomMinLabel);
    formLayout->addRow("Zoom Max:", zoomMaxLabel);
    formLayout->addRow("Tile URL:", tileUrlLabel);
    formLayout->addRow("Custom:", isCustomLabel);
    formLayout->addRow("Opacity:", opacityLabel);
    formLayout->addRow("Type:", typeLabel);

    detailsLayout->addLayout(formLayout);

    editButton = new QPushButton("Edit", this);
    editButton->setVisible(false); // Hidden by default
    connect(editButton, &QPushButton::clicked, this, &LayerInformationDialog::editLayer);
    detailsLayout->addWidget(editButton);
    detailsLayout->addStretch(); // Push button to the bottom

    mainLayout->addWidget(detailsWidget);
    mainLayout->setStretch(1, 1); // Make the details side expandable

    setLayout(mainLayout);

    // Connect the list's itemClicked signal to update details
    connect(layerList, &QListWidget::itemClicked, this, &LayerInformationDialog::updateLayerDetails);

    // Select the first item by default if the list is not empty
    if (!mapLayers.isEmpty()) {
        layerList->setCurrentRow(0);
        updateLayerDetails(layerList->item(0));
    }
}

void LayerInformationDialog::updateLayerDetails(QListWidgetItem* item)
{
    currentLayerIndex = layerList->row(item);
    if (currentLayerIndex >= 0 && currentLayerIndex < mapLayers.size()) {
        const auto& layer = mapLayers[currentLayerIndex];
        nameLabel->setText(layer.name);
        idLabel->setText(layer.id);
        zoomMinLabel->setText(QString::number(layer.zoomMin));
        zoomMaxLabel->setText(QString::number(layer.zoomMax));
        tileUrlLabel->setText(layer.isCustom ? layer.tileUrl : "N/A");
        isCustomLabel->setText(layer.isCustom ? "Yes" : "No");
        opacityLabel->setText(layer.opacity >= 0.0 ? QString::number(layer.opacity, 'f', 2) : "N/A");
        // resolutionLabel->setText(layer.resolution.isEmpty() ? "N/A" : layer.resolution);
        typeLabel->setText(layer.type.isEmpty() ? "N/A" : layer.type);
        editButton->setVisible(layer.isCustom);
    }
}

void LayerInformationDialog::editLayer()
{
    if (currentLayerIndex < 0 || currentLayerIndex >= mapLayers.size()) return;

    const auto& layer = mapLayers[currentLayerIndex];
    CustomMapDialog dialog(layer.name, layer.tileUrl, layer.zoomMin, layer.zoomMax,
                           layer.opacity,  layer.type, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = dialog.getMapName();
        QString newTileUrl = dialog.getTileUrl();
        if (!newName.isEmpty() && !newTileUrl.isEmpty()) {
            MapLayerInfo updatedLayer = {
                newName,
                newName,
                dialog.getZoomMin(),
                dialog.getZoomMax(),
                newTileUrl,
                true,
                dialog.getOpacity(),
                dialog.getType()
            };
            mapLayers[currentLayerIndex] = updatedLayer;
            layerList->item(currentLayerIndex)->setText(newName);
            updateLayerDetails(layerList->item(currentLayerIndex));
            emit layerEdited(currentLayerIndex, updatedLayer);
        }
    }
}
