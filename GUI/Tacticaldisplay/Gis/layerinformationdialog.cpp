

// #include "layerinformationdialog.h"
// #include "GUI/Tacticaldisplay/Gis/custommapdialog.h"

// LayerInformationDialog::LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent)
//     : QDialog(parent), mapLayers(layers), currentLayerIndex(-1)
// {
//     setWindowTitle("Map Layer Information");
//     setMinimumWidth(600);
//     setupUi();
// }

// void LayerInformationDialog::setupUi()
// {
//     QHBoxLayout* mainLayout = new QHBoxLayout(this);

//     // Left side: List of layer names
//     layerList = new QListWidget(this);
//     layerList->setMinimumWidth(200);
//     layerList->setMaximumWidth(250);
//     for (const auto& layer : mapLayers) {
//         layerList->addItem(layer.name);
//     }
//     mainLayout->addWidget(layerList);

//     // Right side: Layer details
//     QWidget* detailsWidget = new QWidget(this);
//     QVBoxLayout* detailsLayout = new QVBoxLayout(detailsWidget);

//     QFormLayout* formLayout = new QFormLayout();

//     nameLabel = new QLabel(this);
//     idLabel = new QLabel(this);
//     zoomMinLabel = new QLabel(this);
//     zoomMaxLabel = new QLabel(this);
//     tileUrlLabel = new QLabel(this);
//     isCustomLabel = new QLabel(this);
//     opacityLabel = new QLabel(this);
//     // resolutionLabel = new QLabel(this);
//     typeLabel = new QLabel(this);

//     formLayout->addRow("Name:", nameLabel);
//     formLayout->addRow("ID:", idLabel);
//     formLayout->addRow("Zoom Min:", zoomMinLabel);
//     formLayout->addRow("Zoom Max:", zoomMaxLabel);
//     formLayout->addRow("Tile URL:", tileUrlLabel);
//     formLayout->addRow("Custom:", isCustomLabel);
//     formLayout->addRow("Opacity:", opacityLabel);
//     // formLayout->addRow("Resolution:", resolutionLabel);
//     formLayout->addRow("Type:", typeLabel);

//     detailsLayout->addLayout(formLayout);

//     editButton = new QPushButton("Edit", this);
//     editButton->setVisible(false); // Hidden by default
//     connect(editButton, &QPushButton::clicked, this, &LayerInformationDialog::editLayer);
//     detailsLayout->addWidget(editButton);
//     detailsLayout->addStretch(); // Push button to the bottom

//     mainLayout->addWidget(detailsWidget);
//     mainLayout->setStretch(1, 1); // Make the details side expandable

//     setLayout(mainLayout);

//     // Connect the list's itemClicked signal to update details
//     connect(layerList, &QListWidget::itemClicked, this, &LayerInformationDialog::updateLayerDetails);

//     // Select the first item by default if the list is not empty
//     if (!mapLayers.isEmpty()) {
//         layerList->setCurrentRow(0);
//         updateLayerDetails(layerList->item(0));
//     }
// }

// void LayerInformationDialog::updateLayerDetails(QListWidgetItem* item)
// {
//     currentLayerIndex = layerList->row(item);
//     if (currentLayerIndex >= 0 && currentLayerIndex < mapLayers.size()) {
//         const auto& layer = mapLayers[currentLayerIndex];
//         nameLabel->setText(layer.name);
//         idLabel->setText(layer.id);
//         zoomMinLabel->setText(QString::number(layer.zoomMin));
//         zoomMaxLabel->setText(QString::number(layer.zoomMax));
//         tileUrlLabel->setText(layer.isCustom ? layer.tileUrl : "N/A");
//         isCustomLabel->setText(layer.isCustom ? "Yes" : "No");
//         opacityLabel->setText(layer.opacity >= 0.0 ? QString::number(layer.opacity, 'f', 2) : "N/A");
//         // resolutionLabel->setText(layer.resolution.isEmpty() ? "N/A" : layer.resolution);
//         typeLabel->setText(layer.type.isEmpty() ? "N/A" : layer.type);
//         editButton->setVisible(layer.isCustom); // Show button only for custom layers
//     }
// }

// void LayerInformationDialog::editLayer()
// {
//     if (currentLayerIndex < 0 || currentLayerIndex >= mapLayers.size()) return;

//     const auto& layer = mapLayers[currentLayerIndex];
//     CustomMapDialog dialog(layer.name, layer.tileUrl, layer.zoomMin, layer.zoomMax,
//                            layer.opacity,  layer.type, this);
//     if (dialog.exec() == QDialog::Accepted) {
//         QString newName = dialog.getMapName();
//         QString newTileUrl = dialog.getTileUrl();
//         if (!newName.isEmpty() && !newTileUrl.isEmpty()) {
//             MapLayerInfo updatedLayer = {
//                 newName,
//                 newName, // ID same as name for custom layers
//                 dialog.getZoomMin(),
//                 dialog.getZoomMax(),
//                 newTileUrl,
//                 true,
//                 dialog.getOpacity(),
//                 // dialog.getResolution().isEmpty() ? "N/A" : dialog.getResolution(),
//                 dialog.getType()
//             };
//             mapLayers[currentLayerIndex] = updatedLayer;
//             layerList->item(currentLayerIndex)->setText(newName);
//             updateLayerDetails(layerList->item(currentLayerIndex));
//             emit layerEdited(currentLayerIndex, updatedLayer);
//         }
//     }
// }
/*
 * LayerInformationDialog Implementation File
 * This file contains the implementation of the LayerInformationDialog class which
 * provides a comprehensive interface for viewing and editing map layer properties
 * with support for both built-in and custom map layers.
 */

#include "layerinformationdialog.h"
#include "GUI/Tacticaldisplay/Gis/custommapdialog.h"

/*
 * Constructor: LayerInformationDialog
 * Initializes the dialog with layer data and sets up the user interface
 */
LayerInformationDialog::LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent)
    : QDialog(parent), mapLayers(layers), currentLayerIndex(-1)
{
    setWindowTitle("Map Layer Information");  // Set dialog window title
    setMinimumWidth(600);                     // Set minimum dialog width for proper layout
    setupUi();                                // Initialize the user interface
}

/*
 * setupUi: Initialize the dialog user interface
 * Creates a two-pane layout with layer list on left and details on right
 */
void LayerInformationDialog::setupUi()
{
    // Create main horizontal layout for two-pane design
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    /*
     * Left Pane: Layer List
     * Displays all available layer names in a scrollable list
     */
    layerList = new QListWidget(this);
    layerList->setMinimumWidth(200);   // Set fixed minimum width
    layerList->setMaximumWidth(250);   // Set fixed maximum width

    // Populate list with layer names from the provided data
    for (const auto& layer : mapLayers) {
        layerList->addItem(layer.name);
    }
    mainLayout->addWidget(layerList);  // Add list to main layout

    /*
     * Right Pane: Layer Details
     * Displays comprehensive information about the selected layer
     */
    QWidget* detailsWidget = new QWidget(this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsWidget);

    // Create form layout for organized property display
    QFormLayout* formLayout = new QFormLayout();

    // Initialize all detail display labels
    nameLabel = new QLabel(this);
    idLabel = new QLabel(this);
    zoomMinLabel = new QLabel(this);
    zoomMaxLabel = new QLabel(this);
    tileUrlLabel = new QLabel(this);
    isCustomLabel = new QLabel(this);
    opacityLabel = new QLabel(this);
    // resolutionLabel = new QLabel(this);  // Commented out as per header
    typeLabel = new QLabel(this);

    // Add all property rows to the form layout with descriptive labels
    formLayout->addRow("Name:", nameLabel);
    formLayout->addRow("ID:", idLabel);
    formLayout->addRow("Zoom Min:", zoomMinLabel);
    formLayout->addRow("Zoom Max:", zoomMaxLabel);
    formLayout->addRow("Tile URL:", tileUrlLabel);
    formLayout->addRow("Custom:", isCustomLabel);
    formLayout->addRow("Opacity:", opacityLabel);
    // formLayout->addRow("Resolution:", resolutionLabel);  // Commented out as per header
    formLayout->addRow("Type:", typeLabel);

    detailsLayout->addLayout(formLayout);  // Add form to details layout

    /*
     * Edit Button
     * Only visible for custom layers, allows editing layer properties
     */
    editButton = new QPushButton("Edit", this);
    editButton->setVisible(false); // Hidden by default, shown only for custom layers
    connect(editButton, &QPushButton::clicked, this, &LayerInformationDialog::editLayer);
    detailsLayout->addWidget(editButton);
    detailsLayout->addStretch(); // Add stretch to push button to bottom

    mainLayout->addWidget(detailsWidget);  // Add details pane to main layout
    mainLayout->setStretch(1, 1); // Make the details side expandable

    setLayout(mainLayout);  // Set the main layout for the dialog

    /*
     * Connect Signals and Set Initial State
     */

    // Connect list selection to details update
    connect(layerList, &QListWidget::itemClicked, this, &LayerInformationDialog::updateLayerDetails);

    // Automatically select and display the first layer if available
    if (!mapLayers.isEmpty()) {
        layerList->setCurrentRow(0);
        updateLayerDetails(layerList->item(0));
    }
}

/*
 * updateLayerDetails: Update the details display for the selected layer
 * Populates all property labels with data from the currently selected layer
 */
void LayerInformationDialog::updateLayerDetails(QListWidgetItem* item)
{
    // Get the index of the selected item
    currentLayerIndex = layerList->row(item);

    // Validate index and update display if valid
    if (currentLayerIndex >= 0 && currentLayerIndex < mapLayers.size()) {
        const auto& layer = mapLayers[currentLayerIndex];

        // Update all display labels with layer properties
        nameLabel->setText(layer.name);
        idLabel->setText(layer.id);
        zoomMinLabel->setText(QString::number(layer.zoomMin));
        zoomMaxLabel->setText(QString::number(layer.zoomMax));
        tileUrlLabel->setText(layer.isCustom ? layer.tileUrl : "N/A");  // Only show URL for custom layers
        isCustomLabel->setText(layer.isCustom ? "Yes" : "No");
        opacityLabel->setText(layer.opacity >= 0.0 ? QString::number(layer.opacity, 'f', 2) : "N/A");
        // resolutionLabel->setText(layer.resolution.isEmpty() ? "N/A" : layer.resolution);  // Commented out
        typeLabel->setText(layer.type.isEmpty() ? "N/A" : layer.type);

        // Only show edit button for custom layers
        editButton->setVisible(layer.isCustom);
    }
}

/*
 * editLayer: Open edit dialog for the currently selected custom layer
 * Uses CustomMapDialog to allow modification of custom layer properties
 */
void LayerInformationDialog::editLayer()
{
    // Validate current layer index
    if (currentLayerIndex < 0 || currentLayerIndex >= mapLayers.size()) return;

    // Get reference to the current layer for editing
    const auto& layer = mapLayers[currentLayerIndex];

    // Create edit dialog with current layer values pre-filled
    CustomMapDialog dialog(layer.name, layer.tileUrl, layer.zoomMin, layer.zoomMax,
                           layer.opacity, layer.type, this);

    // Process dialog result if user clicks OK
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = dialog.getMapName();
        QString newTileUrl = dialog.getTileUrl();

        // Validate that required fields are not empty
        if (!newName.isEmpty() && !newTileUrl.isEmpty()) {
            // Create updated layer configuration
            MapLayerInfo updatedLayer = {
                newName,
                newName, // For custom layers, ID is same as name
                dialog.getZoomMin(),
                dialog.getZoomMax(),
                newTileUrl,
                true,    // Mark as custom layer
                dialog.getOpacity(),
                // dialog.getResolution().isEmpty() ? "N/A" : dialog.getResolution(),  // Commented out
                dialog.getType()
            };

            // Update the layer data
            mapLayers[currentLayerIndex] = updatedLayer;

            // Update the list display
            layerList->item(currentLayerIndex)->setText(newName);

            // Refresh the details display
            updateLayerDetails(layerList->item(currentLayerIndex));

            // Emit signal to notify external components of the change
            emit layerEdited(currentLayerIndex, updatedLayer);
        }
    }
}
