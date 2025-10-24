/* ========================================================================= */
/* File: tacticaldisplay.cpp                                              */
/* Purpose: Implements tactical display with map and 3D scene integration   */
/* ========================================================================= */

#include "tacticaldisplay.h"                       // For tactical display class
#include <QVBoxLayout>                             // For vertical layout
#include <QStackedLayout>                          // For stacked layout
#include <QLabel>                                  // For label widget
#include <QComboBox>                               // For combo box widget
#include <QHBoxLayout>                             // For horizontal layout
#include <qgsproject.h>                            // For QGIS project management
#include <qgscoordinatetransform.h>                // For coordinate transforms

// %%% Constructor %%%
/* Initialize tactical display with map and 3D widgets */
TacticalDisplay::TacticalDisplay(QWidget *parent)
    : QWidget(parent)
{
    // Set up main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    installEventFilter(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    // Create splitter
    splitter = new QSplitter(Qt::Horizontal, this);
    // Set up map canvas container
    QWidget *mapCanvasContainer = new QWidget(this);
    QStackedLayout *containerLayout = new QStackedLayout(mapCanvasContainer);
    containerLayout->setStackingMode(QStackedLayout::StackAll);
    // Configure canvas widget
    canvas = new CanvasWidget(mapCanvasContainer);
    canvas->setAttribute(Qt::WA_TranslucentBackground);
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    canvas->setStyleSheet("background: transparent;");
    containerLayout->addWidget(canvas);
    canvas->setMinimumWidth(200);
    // Configure GIS map widget
    mapWidget = new GISlib(mapCanvasContainer);
    mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    containerLayout->addWidget(mapWidget);
    canvas->gislib = mapWidget;
    // Create coordinate display widget
    QWidget *coordWidget = new QWidget(mapWidget);
    coordWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *coordLayout = new QHBoxLayout(coordWidget);
    coordLayout->setContentsMargins(2, 2, 2, 2);
    coordLayout->setSpacing(2);
    coordLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Configure coordinate label
    overlayLabel = new QLabel("Lat: ---, Lon: ---", coordWidget);
    overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlayLabel->setStyleSheet(
        "color: white;"
        "background-color: rgba(160,160,160,150);"
        "padding: 5px;"
        "border-radius: 6px;"
        "font-family: Arial;"
        );
    overlayLabel->setFixedSize(200, 60);
    // Configure CRS combo box
    crsComboBox = new QComboBox(coordWidget);
    crsComboBox->addItem("Lat/Lon", "EPSG:4326");
    crsComboBox->addItem("UTM", "UTM_AUTO");
    crsComboBox->setFixedSize(100, 30);
    crsComboBox->setStyleSheet(
        "QComboBox {"
        " color: white;"
        " background-color: rgba(160,160,160,150);"
        " border: 1px solid rgba(255,255,255,50);"
        " border-radius: 6px;"
        " padding: 2px;"
        " font-family: Arial;"
        "}"
        "QComboBox::drop-down {"
        " border-left: 1px solid rgba(255,255,255,50);"
        " width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        " color: white;"
        " background-color: rgba(160,160,160,200);"
        " selection-background-color: rgba(100,100,100,200);"
        " border: 1px solid rgba(255,255,255,50);"
        "}"
        );
    // Add coordinate widgets to layout
    coordLayout->addWidget(overlayLabel);
    coordLayout->addWidget(crsComboBox);
    coordWidget->setFixedSize(304, 60);
    coordWidget->setGeometry(20, 20, 304, 60);
    coordWidget->raise();
    // Connect coordinate display signal
    connect(mapWidget, &GISlib::mouseCords, this, [=](double lat, double lon, const QString& crsId) {
        QString xLabel = crsId.startsWith("EPSG:326") ? "X" : "Lon";
        QString yLabel = crsId.startsWith("EPSG:326") ? "Y" : "Lat";
        overlayLabel->setText(QString("%1: %2\n%3: %4")
                                  .arg(yLabel).arg(QString::number(lat, 'f', 6))
                                  .arg(xLabel).arg(QString::number(lon, 'f', 6)));
    });
    // Connect CRS selection signal
    connect(crsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        QString crsId = crsComboBox->itemData(index).toString();
        mapWidget->setCoordinateSystem(crsId);
    });
    // Connect GIS event signals to canvas
    connect(mapWidget, &GISlib::keyPressed, canvas, &CanvasWidget::onGISKeyPressed);
    connect(mapWidget, &GISlib::mousePressed, canvas, &CanvasWidget::onGISMousePressed);
    connect(mapWidget, &GISlib::mouseMoved, canvas, &CanvasWidget::onGISMouseMoved);
    connect(mapWidget, &GISlib::mouseReleased, canvas, &CanvasWidget::onGISMouseReleased);
    connect(mapWidget, &GISlib::painted, canvas, &CanvasWidget::onGISPainted);
    connect(mapWidget, &GISlib::dragEnterEvents, canvas, &CanvasWidget::dragEnterEvents);
    connect(mapWidget, &GISlib::dragMoveEvents, canvas, &CanvasWidget::dragMoveEvents);
    connect(mapWidget, &GISlib::dropEvents, canvas, &CanvasWidget::dropEvents);
    // Add map container to splitter
    splitter->addWidget(mapCanvasContainer);
    // Add and hide 3D widget
    scene3dwidget = new Scene3DWidget();
    splitter->addWidget(scene3dwidget);
    scene3dwidget->setMinimumWidth(200);
    scene3dwidget->setVisible(false);
    // Configure splitter stretch factors
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);
    // Set minimum width
    setMinimumWidth(400);
}

// %%% Mesh Management %%%
/* Add a mesh to the canvas */
void TacticalDisplay::addMesh(QString ID, MeshData meshData)
{
    // Create mesh entry
    MeshEntry entity;
    entity.name = meshData.name;
    entity.velocity = new QVector3D(0, 0, 0);
    entity.transform = meshData.transform->matrix;
    entity.position = new QVector3D(0, 0, 0);
    entity.rotation = new QQuaternion();
    entity.size = new QVector3D(0, 0, 0);
    entity.mesh = meshData.Meshes[0];
    entity.collider = meshData.collider;
    entity.trajectory = meshData.trajectory;
    // Add to canvas and update
    canvas->Meshes[ID.toStdString()] = entity;
    canvas->update();
}

/* Remove a mesh from the canvas */
void TacticalDisplay::removeMesh(QString ID)
{
    std::string key = ID.toStdString();
    if (canvas->Meshes.find(key) != canvas->Meshes.end()) {
        canvas->Meshes.erase(key);
        canvas->update();
    }
}

/* Select a mesh and update display */
void TacticalDisplay::selectedMesh(QString ID)
{
    std::string key = ID.toStdString();
    canvas->selectedEntityId = key;
    canvas->update();
    emit meshSelected(ID);
}

/* Set map layers for display */
void TacticalDisplay::setMapLayers(const QStringList& layerNames)
{
    if (mapWidget) {
        qDebug() << "Setting map layers:" << layerNames;
        mapWidget->setLayers(layerNames);
        mapWidget->update();
    } else {
        qDebug() << "Error: mapWidget is null";
    }
}

/* Add a custom map layer */
void TacticalDisplay::addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity)
{
    if (mapWidget) {
        mapWidget->addCustomMap(layerName, zoomMin, zoomMax, tileUrl, opacity);
    }
}

// %%% Event Handling %%%
/* Handle mouse wheel events */
bool TacticalDisplay::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        // Forward wheel event to canvas and map
        canvas->wheelEvent(wheelEvent);
        mapWidget->wheelEvents(wheelEvent);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
