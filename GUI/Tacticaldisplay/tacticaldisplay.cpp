
#include "tacticaldisplay.h"
#include "QVBoxLayout"
#include "QStackedLayout"
#include "QLabel"

TacticalDisplay::TacticalDisplay(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    splitter = new QSplitter(Qt::Horizontal, this);

    QWidget *mapCanvasContainer = new QWidget(this);
    QStackedLayout *containerLayout = new QStackedLayout(mapCanvasContainer);
    containerLayout->setStackingMode(QStackedLayout::StackAll);

    canvas = new CanvasWidget(mapCanvasContainer);
    canvas->setAttribute(Qt::WA_TranslucentBackground);
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    canvas->setStyleSheet("background: transparent;");
    containerLayout->addWidget(canvas);
    canvas->setMinimumWidth(200);

    mapWidget = new GISlib(mapCanvasContainer);
    mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    containerLayout->addWidget(mapWidget);

    QLabel *overlayLabel = new QLabel("Lat: ---, Lon: ---", mapWidget);
    overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlayLabel->setGeometry(20, 20, 200, 60);
    overlayLabel->setStyleSheet(
        "color: white;"
        "background-color: rgba(0,0,0,150);"
        "padding: 5px;"
        "border-radius: 6px;"
        "font-family: Arial;"
        );
    overlayLabel->raise();

    connect(mapWidget, &GISlib::mouseCords, this, [=](QString lat, QString lon) {
        // qDebug() << "Received coordinates:" << lat << lon;
        overlayLabel->setText(QString("Lat: %1\nLon: %2").arg(lat, lon));
    });

    connect(mapWidget, &GISlib::keyPressed, canvas, &CanvasWidget::onGISKeyPressed);
    connect(mapWidget, &GISlib::mousePressed, canvas, &CanvasWidget::onGISMousePressed);
    connect(mapWidget, &GISlib::mouseMoved, canvas, &CanvasWidget::onGISMouseMoved);
    connect(mapWidget, &GISlib::mouseReleased, canvas, &CanvasWidget::onGISMouseReleased);
    connect(mapWidget, &GISlib::painted, canvas, &CanvasWidget::onGISPainted);
    // connect(mapWidget, &GISlib::zoomChanged, canvas, &CanvasWidget::onGISZoomChanged); // New connection
    // connect(mapWidget, &GISlib::centerChanged, canvas, &CanvasWidget::onGISCenterChanged);

    splitter->addWidget(mapCanvasContainer);

    scene3dwidget = new Scene3DWidget();
    splitter->addWidget(scene3dwidget);
    scene3dwidget->setMinimumWidth(200);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
    setMinimumWidth(400);
}
void TacticalDisplay::addMesh(QString ID, MeshData meshData) {
    MeshEntry entity;
    entity.name = meshData.name;
    entity.velocity = new Vector(0, 0, 0);
    entity.position = meshData.transform->position;
    entity.rotation = meshData.transform->rotation;
    entity.size = meshData.transform->size;
    entity.mesh = meshData.Meshes[0];
    entity.collider = meshData.collider;
    entity.trajectory = meshData.trajectory;
    canvas->Meshes[ID.toStdString()] = entity;
    canvas->update();

    Mesh3dEntry entity3d;
    entity3d.name = meshData.name;
    entity3d.velocity = new Vector(0, 0, 0);
    entity3d.position = meshData.transform->position;
    entity3d.rotation = meshData.transform->rotation;
    entity3d.size = meshData.transform->size;
    entity3d.mesh = meshData.Meshes[0];
    entity3d.collider = meshData.collider;
    //scene3dwidget->addEntity(ID, entity3d);
    //scene3dwidget->updateEntities();
}

void TacticalDisplay::removeMesh(QString ID) {
    std::string key = ID.toStdString();
    if (canvas->Meshes.find(key) != canvas->Meshes.end()) {
        canvas->Meshes.erase(key);
        canvas->update();
    }
    //scene3dwidget->removeEntity(ID);
    //scene3dwidget->updateEntities();
}

void TacticalDisplay::selectedMesh(QString ID) {
    std::string key = ID.toStdString();
    canvas->selectedEntityId = key;
    canvas->update();
    //scene3dwidget->selectedEntity(ID);
    //scene3dwidget->updateEntities();
    emit meshSelected(ID);
}


void TacticalDisplay::setMapLayers(const QStringList& layerNames) {
    if (mapWidget) {
        qDebug() << "Setting map layers:" << layerNames;
        mapWidget->setLayers(layerNames);
        mapWidget->update();
    } else {
        qDebug() << "Error: mapWidget is null";
    }
}


void TacticalDisplay::addCustomMap(const QString& layerName, int zoomMin, int zoomMax, const QString& tileUrl, qreal opacity) {
    if (mapWidget) {
        mapWidget->addCustomMap(layerName, zoomMin, zoomMax, tileUrl, opacity); // Pass opacity
    }
}
