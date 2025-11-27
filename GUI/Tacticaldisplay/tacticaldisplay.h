// /* ========================================================================= */
// /* File: tacticaldisplay.h                                                  */
// /* Purpose: Defines widget for tactical display with 2D and 3D views         */
// /* ========================================================================= */

// #ifndef TACTICALDISPLAY_H
// #define TACTICALDISPLAY_H

// #include "core/Render/scenerenderer.h"            // For scene renderer
// #include <QDockWidget>                            // For dock widget
// #include <QWidget>                                // For widget base class
// #include <GUI/Tacticaldisplay/canvaswidget.h>     // For canvas widget
// #include <core/Hierarchy/Components/transform.h>  // For transform component
// #include <core/Hierarchy/Components/mesh.h>       // For mesh component
// #include <QSplitter>                              // For splitter widget
// #include "canvaswidget.h"                         // For canvas widget
// #include "GUI/scene3dwidget/scene3dwidget.h"      // For 3D scene widget
// #include "GUI/Tacticaldisplay/Gis/gislib.h"       // For GIS library
// #include "QStackedWidget"                         // For stacked widget

// // %%% Forward Declarations %%%
// class QComboBox;
// class QLabel;

// // %%% Class Definition %%%
// /* Widget for tactical display visualization */
// class TacticalDisplay : public QWidget {
//     Q_OBJECT

// public:
//     // Initialize tactical display
//     explicit TacticalDisplay(QWidget *parent = nullptr);
//     // Show 3D view
//     void show3DView();
//     // Hide 3D view
//     void hide3DView();

// public slots:
//     // Add mesh with ID and data
//     void addMesh(QString ID, MeshData meshData);
//     // Remove mesh by ID
//     void removeMesh(QString ID);
//     // Select mesh by ID
//     void selectedMesh(QString ID);
//     // Set map layers
//     void setMapLayers(const QStringList& layerNames);
//     // Add custom map layer
//     void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity = 1.0);
//     // Slot to handle coordinate system changes from the toolbar
//     void onCoordinateSystemChanged(const QString& crsId); // <-- NEW SLOT

// signals:
//     // Signal mesh selection
//     void meshSelected(QString ID);

// protected:
//     // Handle event filtering
//     bool eventFilter(QObject *obj, QEvent *event) override;

// public:
//     // %%% UI Components %%%
//     // Canvas widget
//     CanvasWidget *canvas;
//     // Canvas dock widget
//     QDockWidget *canvasDock;
//     // 3D scene dock widget
//     QDockWidget *scene3dDock;
//     // Splitter for layout
//     QSplitter *splitter;
//     // 3D scene widget
//     Scene3DWidget *scene3dwidget;
//     // Stacked widget for views
//     QStackedWidget* stackedWidget;
//     // GIS map widget
//     GISlib* mapWidget;
//     // Zoom in on map
//     void zoomIn() {
//         if (mapWidget) {
//             currentZoom++;
//             mapWidget->setZoom(currentZoom);
//         }
//     }
//     // Zoom out on map
//     void zoomOut() {
//         if (mapWidget) {
//             currentZoom--;
//             mapWidget->setZoom(currentZoom);
//         }
//     }

// private:
//     // %%% Data Members %%%
//     // Current zoom level
//     int currentZoom = 3;
//     // 3D view visibility flag
//     bool is3DViewVisible = false;
//     // Overlay label
//     QLabel *overlayLabel;
//     // Coordinate system combo box
//     QComboBox *crsComboBox;
// };

// #endif // TACTICALDISPLAY_H
#ifndef TACTICALDISPLAY_H
#define TACTICALDISPLAY_H

#include "core/Render/scenerenderer.h"            // For scene renderer
#include <QDockWidget>                            // For dock widget
#include <QWidget>                                // For widget base class
#include <GUI/Tacticaldisplay/canvaswidget.h>     // For canvas widget
#include <core/Hierarchy/Components/transform.h>  // For transform component
#include <core/Hierarchy/Components/mesh.h>       // For mesh component
#include <QSplitter>                              // For splitter widget
#include "canvaswidget.h"                         // For canvas widget
#include "GUI/scene3dwidget/scene3dwidget.h"      // For 3D scene widget
#include "GUI/Tacticaldisplay/Gis/gislib.h"       // For GIS library
#include "QStackedWidget"                         // For stacked widget
#include "GUI/Tacticaldisplay/scalebar.h"         // Add this line for scale bar

// %%% Forward Declarations %%%
class QComboBox;
class QLabel;

// %%% Class Definition %%%
/* Widget for tactical display visualization */
class TacticalDisplay : public QWidget {
    Q_OBJECT

public:
    // Initialize tactical display
    explicit TacticalDisplay(QWidget *parent = nullptr);
    // Show 3D view
    void show3DView();
    // Hide 3D view
    void hide3DView();

public slots:
    // Add mesh with ID and data
    void addMesh(QString ID, MeshData meshData);
    // Remove mesh by ID
    void removeMesh(QString ID);
    // Select mesh by ID
    void selectedMesh(QString ID);
    // Set map layers
    void setMapLayers(const QStringList& layerNames);
    // Add custom map layer
    void addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity = 1.0);
    // Slot to handle coordinate system changes from the toolbar
    void onCoordinateSystemChanged(const QString& crsId);

signals:
    // Signal mesh selection
    void meshSelected(QString ID);

protected:
    // Handle event filtering
    bool eventFilter(QObject *obj, QEvent *event) override;
    // Handle resize events - ADD THIS LINE
    void resizeEvent(QResizeEvent *event) override;

public:
    // %%% UI Components %%%
    // Canvas widget
    CanvasWidget *canvas;
    // Canvas dock widget
    QDockWidget *canvasDock;
    // 3D scene dock widget
    QDockWidget *scene3dDock;
    // Splitter for layout
    QSplitter *splitter;
    // 3D scene widget
    Scene3DWidget *scene3dwidget;
    // Stacked widget for views
    QStackedWidget* stackedWidget;
    // GIS map widget
    GISlib* mapWidget;
    // Scale bar widget
    ScaleBar *scaleBar;  // Add this line

    // Zoom in on map
    void zoomIn() {
        if (mapWidget) {
            currentZoom++;
            mapWidget->setZoom(currentZoom);
            updateScaleBar();  // Add this call
        }
    }
    // Zoom out on map
    void zoomOut() {
        if (mapWidget) {
            currentZoom--;
            mapWidget->setZoom(currentZoom);
            updateScaleBar();  // Add this call
        }
    }

private:
    // %%% Data Members %%%
    // Current zoom level
    int currentZoom = 3;
    // 3D view visibility flag
    bool is3DViewVisible = false;
    // Overlay label
    QLabel *overlayLabel;
    // Coordinate system combo box
    QComboBox *crsComboBox;

    // Private method to update scale bar
    void updateScaleBar();  // Add this line
};

#endif // TACTICALDISPLAY_H
