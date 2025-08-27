#include "tacticaldisplay.h"
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <qgsproject.h>
#include <qgscoordinatetransform.h>

TacticalDisplay::TacticalDisplay(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    installEventFilter(this);
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

    canvas->gislib = mapWidget;

    // Create a container for the coordinate label and CRS selector
    QWidget *coordWidget = new QWidget(mapWidget);
    coordWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *coordLayout = new QHBoxLayout(coordWidget);
    coordLayout->setContentsMargins(2, 2, 2, 2); // Reduced margins
    coordLayout->setSpacing(2); // Reduced spacing for tighter integration
    coordLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // Center vertically

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

    crsComboBox = new QComboBox(coordWidget);
    crsComboBox->addItem("Lat/Lon", "EPSG:4326");
    crsComboBox->addItem("UTM", "UTM_AUTO");
    crsComboBox->setFixedSize(100, 30); // Fixed height to align with label
    crsComboBox->setStyleSheet(
        "QComboBox {"
        "    color: white;"
        "    background-color: rgba(160,160,160,150);"
        "    border: 1px solid rgba(255,255,255,50);"
        "    border-radius: 6px;"
        "    padding: 2px;"
        "    font-family: Arial;"
        "}"
        "QComboBox::drop-down {"
        "    border-left: 1px solid rgba(255,255,255,50);"
        "    width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    color: white;"
        "    background-color: rgba(160,160,160,200);"
        "    selection-background-color: rgba(100,100,100,200);"
        "    border: 1px solid rgba(255,255,255,50);"
        "}"
        );

    coordLayout->addWidget(overlayLabel);
    coordLayout->addWidget(crsComboBox);
    coordWidget->setFixedSize(304, 60); // 200 (label) + 100 (combo) + 4 (margins + spacing)
    coordWidget->setGeometry(20, 20, 304, 60);
    coordWidget->raise();

    connect(mapWidget, &GISlib::mouseCords, this, [=](double lat, double lon, const QString& crsId) {
        QString xLabel = crsId.startsWith("EPSG:326") ? "X" : "Lon";
        QString yLabel = crsId.startsWith("EPSG:326") ? "Y" : "Lat";
        overlayLabel->setText(QString("%1: %2\n%3: %4")
                                  .arg(yLabel).arg(QString::number(lat, 'f', 6))
                                  .arg(xLabel).arg(QString::number(lon, 'f', 6)));
    });

    connect(crsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        QString crsId = crsComboBox->itemData(index).toString();
        mapWidget->setCoordinateSystem(crsId);
    });

    connect(mapWidget, &GISlib::keyPressed, canvas, &CanvasWidget::onGISKeyPressed);
    connect(mapWidget, &GISlib::mousePressed, canvas, &CanvasWidget::onGISMousePressed);
    connect(mapWidget, &GISlib::mouseMoved, canvas, &CanvasWidget::onGISMouseMoved);
    connect(mapWidget, &GISlib::mouseReleased, canvas, &CanvasWidget::onGISMouseReleased);
    connect(mapWidget, &GISlib::painted, canvas, &CanvasWidget::onGISPainted);
    connect(mapWidget, &GISlib::dragEnterEvents, canvas, &CanvasWidget::dragEnterEvents);
    connect(mapWidget, &GISlib::dragMoveEvents, canvas, &CanvasWidget::dragMoveEvents);
    connect(mapWidget, &GISlib::dropEvents, canvas, &CanvasWidget::dropEvents);
    splitter->addWidget(mapCanvasContainer);

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

}

void TacticalDisplay::removeMesh(QString ID) {
    std::string key = ID.toStdString();
    if (canvas->Meshes.find(key) != canvas->Meshes.end()) {
        canvas->Meshes.erase(key);
        canvas->update();
    }
}

void TacticalDisplay::selectedMesh(QString ID) {
    std::string key = ID.toStdString();
    canvas->selectedEntityId = key;
    canvas->update();
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
        mapWidget->addCustomMap(layerName, zoomMin, zoomMax, tileUrl, opacity);
    }
}

bool TacticalDisplay::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        // Forward it to your CanvasWidget
        canvas->wheelEvent(wheelEvent);
        mapWidget->wheelEvents(wheelEvent);
        return true; // event consumed
    }
    return QWidget::eventFilter(obj, event);
}
