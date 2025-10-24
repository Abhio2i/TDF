

// #ifndef LAYERINFORMATIONDIALOG_H
// #define LAYERINFORMATIONDIALOG_H

// #include <QDialog>
// #include <QListWidget>
// #include <QLabel>
// #include <QFormLayout>
// #include <QHBoxLayout>
// #include <QPushButton>

// class LayerInformationDialog : public QDialog
// {
//     Q_OBJECT

// public:
//     struct MapLayerInfo {
//         QString name;
//         QString id;
//         int zoomMin = 0;
//         int zoomMax = 0;
//         QString tileUrl;
//         bool isCustom = false;
//         qreal opacity = -1.0;
//         QString resolution = "N/A";
//         QString type = "";
//     };

//     explicit LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent = nullptr);

// signals:
//     void layerEdited(int index, const MapLayerInfo& updatedLayer);

// private slots:
//     void updateLayerDetails(QListWidgetItem* item);
//     void editLayer();

// private:
//     void setupUi();
//     QList<MapLayerInfo>& mapLayers;
//     QListWidget* layerList;
//     QLabel* nameLabel;
//     QLabel* idLabel;
//     QLabel* zoomMinLabel;
//     QLabel* zoomMaxLabel;
//     QLabel* tileUrlLabel;
//     QLabel* isCustomLabel;
//     QLabel* opacityLabel;
//     QLabel* resolutionLabel;
//     QLabel* typeLabel;
//     QPushButton* editButton;
//     int currentLayerIndex;


// };

// #endif // LAYERINFORMATIONDIALOG_H
/*
 * LayerInformationDialog Header File
 * This file defines the LayerInformationDialog class which provides a user interface
 * for viewing and editing detailed information about map layers including both
 * built-in and custom map layer configurations.
 */

#ifndef LAYERINFORMATIONDIALOG_H
#define LAYERINFORMATIONDIALOG_H

// Include necessary Qt classes
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>

/*
 * LayerInformationDialog Class
 * A dialog that displays comprehensive information about available map layers
 * and allows editing of custom layer properties through an integrated interface.
 */
class LayerInformationDialog : public QDialog
{
    Q_OBJECT

public:
    /*
     * MapLayerInfo Structure
     * Contains all properties and metadata for a map layer configuration
     */
    struct MapLayerInfo {
        QString name;        // Display name of the layer
        QString id;          // Unique identifier for the layer
        int zoomMin = 0;     // Minimum zoom level where layer is visible
        int zoomMax = 0;     // Maximum zoom level where layer is visible
        QString tileUrl;     // URL template for tile server (for custom layers)
        bool isCustom = false; // Flag indicating if this is a user-added custom layer
        qreal opacity = -1.0; // Layer transparency (0.0 = transparent, 1.0 = opaque)
        QString resolution = "N/A"; // Map resolution information
        QString type = "";   // Layer type category (Raster, Vector, etc.)
    };

    /*
     * Constructor
     * Creates a layer information dialog populated with the provided layer data
     */
    explicit LayerInformationDialog(QList<MapLayerInfo>& layers, QWidget *parent = nullptr);

signals:
    /*
     * layerEdited Signal
     * Emitted when a layer's properties are modified through the dialog
     */
    void layerEdited(int index, const MapLayerInfo& updatedLayer);

private slots:
    /*
     * Private Slots for UI Interaction
     */

    /* Update the details display when a different layer is selected */
    void updateLayerDetails(QListWidgetItem* item);

    /* Open edit dialog for the currently selected custom layer */
    void editLayer();

private:
    /*
     * Private Setup Method
     */
    void setupUi();

    /*
     * Data and UI Component Members
     */

    QList<MapLayerInfo>& mapLayers;  // Reference to the layer data (external storage)

    // UI Components
    QListWidget* layerList;          // List widget showing all layer names
    QLabel* nameLabel;               // Display for layer name
    QLabel* idLabel;                 // Display for layer ID
    QLabel* zoomMinLabel;            // Display for minimum zoom level
    QLabel* zoomMaxLabel;            // Display for maximum zoom level
    QLabel* tileUrlLabel;            // Display for tile URL
    QLabel* isCustomLabel;           // Display for custom layer flag
    QLabel* opacityLabel;            // Display for opacity value
    QLabel* resolutionLabel;         // Display for resolution information
    QLabel* typeLabel;               // Display for layer type
    QPushButton* editButton;         // Button to edit custom layers

    int currentLayerIndex;           // Index of currently selected layer

};

#endif // LAYERINFORMATIONDIALOG_H
