#include <QElapsedTimer>
#include "canvaswidget.h"
#include "GUI/Hierarchytree/additemdialog.h"
#include "GUI/Hierarchytree/contextmenu.h"
#include "qapplication.h"
#include "qjsondocument.h"
#include "qprogressdialog.h"
#include "qquaternion.h"
#include "qvector3d.h"
#include <QTimer>
#include <QDebug>
#include <QMouseEvent>
#include <cmath>
#include <core/Debug/console.h>
#include <QtMath>
#include <QUuid>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <algorithm>
#include <QMimeData>
#include <QInputDialog>
#include <string>
#include <QFile>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QColorDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QFontMetrics>
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include <GUI/mainwindow.h>
#include <core/Config/scenarioconfig.h>
#include "core/Debug/profiler.h"
#include <cmath>
#include <tuple>
#include <GUI/Tacticaldisplay/entityinfodialog.h>
#include "GUI/Tacticaldisplay/Shape/shapes_feature.h"
#include "Shape/shapeeventhandler.h"
#include "GUI/Tacticaldisplay/contextmenu/contextmenu_handler.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include <core/Hierarchy/Components/transform.h>
#include "GUI/Tacticaldisplay/Layer/geojson.h"
#include "GUI/Tacticaldisplay/Layer/csv.h"
#include "GUI/Tacticaldisplay/Layer/kml.h"
#include "GUI/Tacticaldisplay/Layer/gml.h"
#include "GUI/Tacticaldisplay/Layer/shapefile.h"
#include "GUI/Tacticaldisplay/Layer/flatgeobuf.h"
#include "core/Hierarchy/EntityProfiles/weapons/sonobuoy.h"
#include <GUI/Tacticaldisplay/waypointeditdialog.h>
#include <QDialogButtonBox>
#include <QToolTip>
#include <GUI/Tacticaldisplay/tooltiphelper.h>
#include <QHBoxLayout>
#include <QWidgetAction>

// ============================================================================
// Constructor / Destructor
// ============================================================================

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget(parent)
    , pointPen(Qt::cyan)
    , m_shapeEventHandler(nullptr)
    , m_shapesFeature(nullptr)
    , m_contextMenuHandler(nullptr)

{
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(MainWindow::scenarioconfig->getSavedImageSize());
    ImageScale = match.captured(1).toInt();
    pointPen.setWidth(pointRadius * 2);
    cacheimg = new QPixmap(":/texture/images/Texture/fighterjet.png");
    scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                 Qt::IgnoreAspectRatio, Qt::FastTransformation);
    gislib = new GISlib(parent);
    setMinimumSize(300, 300);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
    setFocusPolicy(Qt::StrongFocus);
    setUpdatesEnabled(true);
    setMouseTracking(true);
    simulate = false;
    frameCount = 0;
    fps = 0;
    zoomLevel = 1.0f;
    fpsTimer.start();
    selectedWaypointIndex = -1;
    isDraggingWaypoint = false;
    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount;
        frameCount = 0;
    });

    // ========================================================================
    // Create DECOUPLED shape system - No CanvasWidget dependency
    // ========================================================================
    m_shapesFeature = new ShapesFeature(this);
    m_shapeEventHandler = new ShapeEventHandler(this);

    // Initialize Measurement Module
    m_measurementModule = new Measurement(gislib, this);
    connect(m_measurementModule, &Measurement::measurementUpdated, this, &CanvasWidget::Refresh);

    connect(m_measurementModule->getDialog(), &QDialog::rejected, this, [this]() {
        setTransformMode(TransformMode::Translate);
    });

    connect(this, &CanvasWidget::pointsUpdated, m_measurementModule->getDialog(), [this](const QList<QPointF>& points) {
        MeasureDistanceDialog* dlg = m_measurementModule->getDialog();
        if (dlg->isVisible()) {
            m_measurementModule->clearMeasurementPoints();
            for (int i = 1; i < points.size(); ++i) {
                double dist = gislib->calculateDistance(points[i-1], points[i]);
                dlg->addMeasurement(points[i].x(), points[i].y(), dist);
            }
        }
    });



    // ========================================================================
    // Create Context Menu Handler
    // ========================================================================
    m_contextMenuHandler = new ContextMenuHandler(this);
    setupContextMenuCallbacks();

    activeTooltipOptions = {"Name", "Speed", "Altitude", "Latitude", "Longitude", "Trajectory ETA"};

    entityInfoDialog = new EntityInfoDialog(parentWidget());
    entityInfoDialog->setAttribute(Qt::WA_DeleteOnClose, false);

    connect(&m_tooltipTimer, &QTimer::timeout, this, &CanvasWidget::updateHoverTooltip);
    connect(entityInfoDialog, &EntityInfoDialog::update, this, [=]{
        Refresh();
    });

    connect(gislib, &GISlib::distanceMeasured, this, &CanvasWidget::onDistanceMeasured);
    gislib->addOverlayCallback([this](QPainter* painter) {
        if (currentMode == TransformMode::MeasureDistance && m_measurementModule) {
            m_measurementModule->drawRubberBand(*painter);
            m_measurementModule->drawCommittedSegments(*painter);
        }
        if (m_shapeEventHandler) {
            m_shapeEventHandler->paintEvent(*painter);
            m_shapeEventHandler->handleBitmapsPaint(*painter);
        }
        if (gislib) {
            gislib->renderAirbases(*painter);
        }
    });

    fpsUpdateTimer->start(100);
    Refresh();
}

CanvasWidget::~CanvasWidget() {
    delete cacheimg;
    cacheimg = nullptr;
    delete m_shapeEventHandler;
    m_shapeEventHandler = nullptr;
    delete m_shapesFeature;
    m_shapesFeature = nullptr;
    delete m_contextMenuHandler;
    m_contextMenuHandler = nullptr;
}

// ============================================================================
// Context Menu Callbacks Setup
// ============================================================================

void CanvasWidget::setupContextMenuCallbacks()
{
    if (!m_contextMenuHandler) return;

    // Existing callbacks
    m_contextMenuHandler->setShapeFinalizeCallback([this](const QString& shapeType, const QPointF& geoPos) {
        if (m_shapesFeature) {
            m_shapesFeature->handleShapeDrawing(shapeType, geoPos, true);
        }
    });

    m_contextMenuHandler->setShapeDrawingCallback([this](bool enabled, const QString& shapeType) {
        setShapeDrawingMode(enabled, shapeType);
        selectedShape = shapeType;
    });

    m_contextMenuHandler->setTrajectoryCallback([this](bool enabled) {
        setTrajectoryDrawingMode(enabled);
    });

    m_contextMenuHandler->setEntityCallback([this](const QString& entityId) {
        // Remove entity (existing logic)
        auto it = Meshes.find(entityId.toStdString());
        if (it != Meshes.end() && it->second.entity) {
            QString parentId = QString::fromStdString(it->second.entity->parentID);
            ScenarioEditor* se = nullptr;
            RuntimeEditor* re = nullptr;
            QWidget* w = this;
            while (w) {
                if (!se) se = qobject_cast<ScenarioEditor*>(w);
                if (!re) re = qobject_cast<RuntimeEditor*>(w);
                w = w->parentWidget();
            }
            if (se && se->hierarchy)
                se->hierarchy->removeEntity(parentId, entityId);
            else if (re && re->hierarchy)
                re->hierarchy->removeEntity(parentId, entityId);
            Meshes.erase(entityId.toStdString());
            Refresh();
        }
    });

    // === NEW CALLBACKS FOR ENTITY ACTIONS ===
    m_contextMenuHandler->setRenameCallback([this](const QString& entityId) {
        auto it = Meshes.find(entityId.toStdString());
        if (it == Meshes.end() || !it->second.entity) return;

        QString currentName = QString::fromStdString(it->second.entity->Name);
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename Entity", "Enter new name:",
                                                QLineEdit::Normal, currentName, &ok);
        if (ok && !newName.trimmed().isEmpty() && newName != currentName) {
            ScenarioEditor* se = nullptr;
            RuntimeEditor* re = nullptr;
            QWidget* w = this;
            while (w) {
                if (!se) se = qobject_cast<ScenarioEditor*>(w);
                if (!re) re = qobject_cast<RuntimeEditor*>(w);
                w = w->parentWidget();
            }
            if (se && se->hierarchy)
                se->hierarchy->renameEntity(entityId, newName);
            else if (re && re->hierarchy)
                re->hierarchy->renameEntity(entityId, newName);
            Refresh();
        }
    });

    m_contextMenuHandler->setResizeCallback([this](const QString& entityId) {
        auto it = Meshes.find(entityId.toStdString());
        if (it == Meshes.end()) return;
        int currentSize = (it->second.individualImageSize > 0) ? it->second.individualImageSize : ImageScale;
        QDialog dialog(this);
        dialog.setWindowTitle("Change Entity Size");
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        QLabel* label = new QLabel("Set image size (pixels):", &dialog);
        QSpinBox* spin = new QSpinBox(&dialog);
        spin->setRange(10, 1000);
        spin->setValue(currentSize);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(label);
        layout->addWidget(spin);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            it->second.individualImageSize = spin->value();
            Refresh();
        }
    });

    m_contextMenuHandler->setTrajectoryColorCallback([this](const QString& entityId) {
        auto it = Meshes.find(entityId.toStdString());
        if (it == Meshes.end()) return;

        QColor newColor = QColorDialog::getColor(it->second.trajectoryColor, this,
                                                 "Select Trajectory Color", QColorDialog::DontUseNativeDialog);
        if (newColor.isValid()) {
            it->second.trajectoryColor = newColor;
            Refresh();
        }
    });
    m_contextMenuHandler->setEditWaypointCallback([this](int index) {
        if (index < 0 || index >= (int)currentTrajectory.size()) {
            return;
        }
        Waypoints* wp = currentTrajectory[index];
        if (!wp || !wp->position) return;
        double latitude = wp->position->x;
        double longitude = wp->position->z;
        double altitude = wp->position->y;
        double speed = wp->speed;
        QWidget* mainWindow = this->window();
        if (WaypointEditDialog::editWaypoint(mainWindow, latitude, longitude, altitude, speed)) {
            wp->position->x = latitude;
            wp->position->z = longitude;
            wp->position->y = altitude;
            wp->speed = speed;
            updateTrajectoryData();
            Refresh();
        }
    });
    m_contextMenuHandler->setDeleteWaypointCallback([this](int /*index*/) {
        if (selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {

            Waypoints* wp = currentTrajectory[selectedWaypointIndex];
            if (wp) {
                if (wp->position) {
                    delete wp->position;
                    wp->position = nullptr;
                }
                delete wp;
            }
            currentTrajectory.erase(currentTrajectory.begin() + selectedWaypointIndex);
            deselectWaypoint();
            updateTrajectoryData();
            Refresh();
        }
    });

    m_contextMenuHandler->setDeleteAllWaypointsCallback([this]() {
        for (Waypoints* wp : currentTrajectory) {
            if (wp) {
                if (wp->position) {
                    delete wp->position;
                    wp->position = nullptr;
                }
                delete wp;
            }
        }
        currentTrajectory.clear();
        deselectWaypoint();
        saveTrajectory();
        Refresh();
    });

    m_contextMenuHandler->setCancelTrajectoryCallback([this]() {

        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end()) {
                MeshEntry& entry = it->second;
                for (Waypoints* wp : currentTrajectory) {
                    delete wp->position;
                    delete wp;
                }
                currentTrajectory.clear();
                if (entry.trajectory) {
                    for (const Waypoints* wp : entry.trajectory->Trajectories) {
                        Waypoints* newWaypoint = new Waypoints();
                        newWaypoint->position = new Vector(
                            wp->position->x, wp->position->y, wp->position->z);
                        newWaypoint->speed = wp->speed;
                        currentTrajectory.push_back(newWaypoint);
                    }
                }
            }
        }
        setTrajectoryDrawingMode(false);
        deselectWaypoint();
        Refresh();
    });

    m_contextMenuHandler->setSendMessageCallback([this](const QString& entityId) {
        auto it = Meshes.find(entityId.toStdString());
        if (it == Meshes.end() || !it->second.platform /*|| !it->second.platform->radios*/) return;

        bool ok;
        QString text = QInputDialog::getText(this, "Send Message", "Enter message:",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            for (auto& pair : it->second.platform->radios.radios) {
                if (pair.second) pair.second->sendMsg(text.toStdString());
            }
        }
    });

    m_contextMenuHandler->setAddEntityCallback([this](const QPoint& mousePos) {
        ScenarioEditor* se = nullptr;
        RuntimeEditor* re = nullptr;
        QWidget* w = this;
        while (w) {
            if (!se) se = qobject_cast<ScenarioEditor*>(w);
            if (!re) re = qobject_cast<RuntimeEditor*>(w);
            w = w->parentWidget();
        }
        if (!se && !re) return;

        AddItemDialog dialog(AddItemDialog::EntityType,
                             "Platform",
                             AddItemDialog::NormalMode,
                             MainWindow::instance() ? MainWindow::instance()->getDatabaseHierarchy() : nullptr,
                             nullptr,
                             "Runtime");
        dialog.setWindowTitle("Add Entity");
        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            bool isProfileParent = true;
            QString progressText = "Adding Entities...";
            QProgressDialog progressDialog(progressText,
                                           "Cancel", 0, dialog.getNumber(), this);
            progressDialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            progressDialog.setWindowModality(Qt::WindowModal);
            progressDialog.setMinimumDuration(0);
            progressDialog.setValue(0);
            progressDialog.show();
            progressDialog.move(this->parentWidget()->mapToGlobal(this->parentWidget()->rect().center()) -
                                progressDialog.rect().center());
            QApplication::processEvents();

            ProfileCategaory* profile = (se ? se->hierarchy : re->hierarchy)->getProfileByName("Platform");
            QString parentId = profile ? QString::fromStdString(profile->ID) : "";
            double initLat = 190;
            double initLon = 0;
            if (!dialog.isScenarioconfigEnabled()) {
                QPointF geocords = gislib->canvasToGeo(mousePos);
                initLat = geocords.y();
                initLon = geocords.x();
            }

            for (int i = 0; i < dialog.getNumber(); ++i) {
                if (progressDialog.wasCanceled()) break;
                if (se && se->treeView && se->treeView->getContextMenu()) {
                    emit se->treeView->getContextMenu()->addEntityRequested(
                        parentId,
                        dialog.getName() + QString::number(i),
                        isProfileParent,
                        dialog.getComponents(),
                        &dialog,
                        "",
                        initLat,
                        initLon
                        );
                } else if (re && re->treeView && re->treeView->getContextMenu()) {
                    emit re->treeView->getContextMenu()->addEntityRequested(
                        parentId,
                        dialog.getName() + QString::number(i),
                        isProfileParent,
                        dialog.getComponents(),
                        &dialog,
                        "",
                        initLat,
                        initLon
                        );
                }
                progressDialog.setValue(i + 1);
                QApplication::processEvents();
            }
            progressDialog.close();
        }
    });

    m_contextMenuHandler->setAddShapeCallback([this](const QString& shapeType) {
        setShapeDrawingMode(true, shapeType);
        selectedShape = shapeType;
    });

    m_contextMenuHandler->setAddTextCallback([this](const QPoint& mousePos) {
        if (!gislib) return;

        // Check if layer exists first
        if (m_layerPanel && m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
            QWidget* parentWidget = dynamic_cast<QWidget*>(this);
            if (!parentWidget && m_layerPanel) {
                parentWidget = dynamic_cast<QWidget*>(m_layerPanel->parent());
            }

            QMessageBox msgBox(parentWidget);
            msgBox.setWindowTitle("No Layer Found");
            msgBox.setText("No layers exist!\n\nPlease create a layer first to add text.");
            msgBox.setIcon(QMessageBox::Warning);
            QPushButton* createBtn = msgBox.addButton("Create New Layer", QMessageBox::AcceptRole);
            msgBox.addButton("Cancel", QMessageBox::RejectRole);
            msgBox.exec();

            if (msgBox.clickedButton() == createBtn) {
                m_layerPanel->addLayer();
                if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                    return;
                }
            } else {
                return;
            }
        }

        bool ok;
        QString text = QInputDialog::getText(nullptr, "Add Text", "Enter text:",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            QPointF geoPos = gislib->canvasToGeo(mousePos);
            addTextAtGeo(text, geoPos);
        }
    });

    m_contextMenuHandler->setPasteShapeCallback([this]() {
        pasteShape();
    });

    m_contextMenuHandler->setBoxSelectionCallback([this]() {
        if (m_shapeEventHandler) {
            m_shapeEventHandler->toggleBoxSelectionMode();
            Refresh();
        }
    });

    m_contextMenuHandler->setIsBoxSelectionModeCallback([this]() {
        return m_shapeEventHandler ? m_shapeEventHandler->isBoxSelectionMode() : false;
    });
}

// ============================================================================
// Getters - Shape Delegation
// ============================================================================


MeshEntry* CanvasWidget::getEntityMeshEntry(const QString& entityId) {
    auto it = Meshes.find(entityId.toStdString());
    if (it != Meshes.end()) {
        return &it->second;
    }
    return nullptr;
}
ShapesFeature* CanvasWidget::getShapesFeature() const {
    return m_shapesFeature;
}

std::vector<MeshEntry>& CanvasWidget::getTempMeshes() {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempMeshes() : m_dummyMeshes;
}

const std::vector<MeshEntry>& CanvasWidget::getTempMeshes() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempMeshes() : m_dummyMeshes;
}

std::vector<Vector*>& CanvasWidget::getTempPolygonVertices() {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempPolygonVertices() : m_dummyVectors;
}

const std::vector<Vector*>& CanvasWidget::getTempPolygonVertices() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempPolygonVertices() : m_dummyVectors;
}

std::vector<Vector*>& CanvasWidget::getTempLineVertices() {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempLineVertices() : m_dummyVectors;
}

const std::vector<Vector*>& CanvasWidget::getTempLineVertices() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempLineVertices() : m_dummyVectors;
}

std::vector<QPointF>& CanvasWidget::getTempPolygonCanvasPoints() {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempPolygonCanvasPoints() : m_dummyPoints;
}

const std::vector<QPointF>& CanvasWidget::getTempPolygonCanvasPoints() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempPolygonCanvasPoints() : m_dummyPoints;
}

std::vector<QPointF>& CanvasWidget::getTempLineCanvasPoints() {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempLineCanvasPoints() : m_dummyPoints;
}

const std::vector<QPointF>& CanvasWidget::getTempLineCanvasPoints() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getTempLineCanvasPoints() : m_dummyPoints;
}

const QString& CanvasWidget::getHighlightedShapeId() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getHighlightedShapeId() : empty;
}

void CanvasWidget::setHighlightedShapeId(const QString& id) {
    if (m_shapeEventHandler) m_shapeEventHandler->setHighlightedShapeId(id);
}

bool CanvasWidget::isDraggingShape() const {
    return m_shapeEventHandler ? m_shapeEventHandler->isDraggingShape() : false;
}

const QString& CanvasWidget::getDraggingShapeId() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getDraggingShapeId() : empty;
}

bool CanvasWidget::isResizingShape() const {
    return m_shapeEventHandler ? m_shapeEventHandler->isResizingShape() : false;
}

int CanvasWidget::getSelectedHandleIndex() const {
    return m_shapeEventHandler ? m_shapeEventHandler->getSelectedHandleIndex() : -1;
}

const std::vector<QPointF>& CanvasWidget::getResizeHandles() const {
    static std::vector<QPointF> empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getResizeHandles() : empty;
}

const QString& CanvasWidget::getEditingShapeId() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getEditingShapeId() : empty;
}

std::vector<QString>& CanvasWidget::getSelectedShapeIds() {
    return m_shapeEventHandler ? m_shapeEventHandler->getSelectedShapeIds() : m_dummyShapeIds;
}

bool CanvasWidget::isMultiSelecting() const
{

    // IMPORTANT:
    // Single selected shape bhi box-selection/group context ka part ho
    // sakta hai, isliye yahan > 1 ki condition nahi laga rahe.

    const bool hasSelectedEntities =
        !selectedEntityIds.empty();

    const bool hasSelectedShapes =
        m_shapeEventHandler &&
        !m_shapeEventHandler->getSelectedShapeIds().empty();

    return hasSelectedEntities || hasSelectedShapes;
}

bool CanvasWidget::isRotatingBitmap() const {
    return m_shapeEventHandler ? m_shapeEventHandler->isRotatingBitmap() : false;
}

const QString& CanvasWidget::getRotatingBitmapId() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getRotatingBitmapId() : empty;
}

void CanvasWidget::setActiveRotateId(const QString& id) {
    if (m_shapeEventHandler) m_shapeEventHandler->setActiveRotateId(id);
}

const QString& CanvasWidget::getActiveRotateId() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getActiveRotateId() : empty;
}

void CanvasWidget::clearActiveRotateId() {
    if (m_shapeEventHandler) m_shapeEventHandler->clearActiveRotateId();
}

void CanvasWidget::clearSelectedShapeIds() {
    if (m_shapeEventHandler) m_shapeEventHandler->clearSelectedShapeIds();
}

bool CanvasWidget::isPlacingBitmap() const {
    return m_shapeEventHandler ? m_shapeEventHandler->isPlacingBitmap() : false;
}

const QString& CanvasWidget::getSelectedBitmapType() const {
    static QString empty;
    return m_shapeEventHandler ? m_shapeEventHandler->getSelectedBitmapType() : empty;
}

// ============================================================================
// Shape API - Delegated to ShapeEventHandler
// ============================================================================

void CanvasWidget::setShapeDrawingMode(bool enabled, const QString& shapeType) {
    if (m_shapeEventHandler) m_shapeEventHandler->setShapeDrawingMode(enabled, shapeType);
}

void CanvasWidget::onBitmapImageSelected(const QString& filePath) {
    if (m_shapeEventHandler) m_shapeEventHandler->onBitmapImageSelected(filePath);
}

QString CanvasWidget::getBitmapImagePath(const QString& bitmapType) {
    return m_shapeEventHandler ? m_shapeEventHandler->getBitmapImagePath(bitmapType) : QString();
}

void CanvasWidget::onBitmapSelected(const QString& bitmapType) {
    if (m_shapeEventHandler) m_shapeEventHandler->onBitmapSelected(bitmapType);
}

void CanvasWidget::onBitmapImageSelectedAtGeo(const QString& path, const QPointF& geo) {
    if (m_shapeEventHandler) m_shapeEventHandler->onBitmapImageSelectedAtGeo(path, geo);
}

void CanvasWidget::onBitmapSelectedAtGeo(const QString& bitmapType, const QPointF& geo) {
    if (m_shapeEventHandler) m_shapeEventHandler->onBitmapSelectedAtGeo(bitmapType, geo);
}

void CanvasWidget::addTextAtGeo(const QString& text, const QPointF& geo) {
    if (m_shapeEventHandler) m_shapeEventHandler->addTextAtGeo(text, geo);
}

void CanvasWidget::copySelectedShape() {
    if (m_shapeEventHandler) m_shapeEventHandler->copySelectedShape();
}

void CanvasWidget::pasteShape() {
    if (m_shapeEventHandler) m_shapeEventHandler->pasteShape();
}

void CanvasWidget::deleteText(const QString& textId) {
    if (m_shapeEventHandler) m_shapeEventHandler->deleteText(textId);
}

void CanvasWidget::updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness) {
    if (m_shapeEventHandler) m_shapeEventHandler->updateShapeProperties(shapeId, color, borderThickness);
}

void CanvasWidget::showShapePropertiesDialog(const QString& shapeId) {
    if (m_shapeEventHandler) m_shapeEventHandler->showShapePropertiesDialog(shapeId);
}

bool CanvasWidget::moveShapeByName(const std::string& shapeName, const QPointF& geoPos) {
    return m_shapeEventHandler ? m_shapeEventHandler->moveShapeByName(shapeName, geoPos) : false;
}

bool CanvasWidget::deleteObjectById(const QString& id) {
    return m_shapeEventHandler ? m_shapeEventHandler->deleteObjectById(id) : false;
}

void CanvasWidget::centerOnShape(const QString& shapeId) {
    if (m_shapeEventHandler) m_shapeEventHandler->centerOnShape(shapeId);
}

void CanvasWidget::placePastedShape(const QPointF& geoPos) {
    if (m_shapeEventHandler) m_shapeEventHandler->placePastedShape(geoPos);
}

bool CanvasWidget::isShape(const QString& shapeId) const {
    return m_shapeEventHandler ? m_shapeEventHandler->isShape(shapeId) : false;
}

bool CanvasWidget::shouldDrawShape(const QString& shapeId) const {
    return m_shapeEventHandler ? m_shapeEventHandler->shouldDrawShape(shapeId) : false;
}

void CanvasWidget::startShapeEdit(const QString& shapeId)
{
    if (m_shapeEventHandler) {
        m_shapeEventHandler->startShapeEdit(shapeId);
    }
}


void CanvasWidget::startShapeRotate(const QString& shapeId)
{
    if (m_shapeEventHandler) {
        m_shapeEventHandler->startShapeRotate(shapeId);
    }
}


void CanvasWidget::cancelActiveShapeOperation()
{
    if (m_shapeEventHandler) {
        m_shapeEventHandler->cancelActiveShapeOperation();
    }
}

void CanvasWidget::addImportedMesh(
    const MeshEntry& entry)
{
    getTempMeshes().push_back(entry);
    update();
}
// ============================================================================
// CanvasWidget Core Methods
// ============================================================================

void CanvasWidget::ReInit() {
    QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(MainWindow::scenarioconfig->getSavedImageSize());
    ImageScale = match.captured(1).toInt();
    scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                 Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void CanvasWidget::setImageScale(int value) {
    ImageScale = value;
    scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                 Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void CanvasWidget::selectWaypoint(int index) {
    if (index >= 0 && index < (int)currentTrajectory.size()) {
        selectedWaypointIndex = index;
        Refresh();
    } else {
        deselectWaypoint();
    }
}

void CanvasWidget::Render(float) {
    angle += 2.0f;
    if (angle >= 360.0f) angle = 0;
    update();
}

void CanvasWidget::Refresh() {
    if (!simulate) {
        update();
    }
}

void CanvasWidget::simulation() {
    simulate = true;
}

void CanvasWidget::setLayerPanel(LayerPanel* panel) {
    m_layerPanel = panel;
    if (m_shapesFeature) {
        m_shapesFeature->setLayerPanel(panel);
    }
    if (m_shapeEventHandler) {
        m_shapeEventHandler->setLayerPanel(panel);
    }
    if (m_layerPanel) {
        connect(m_layerPanel, &LayerPanel::layerVisibilityChanged,
                this, [this](const QString& layerName, bool visible) {
                    if (geoJsonLayers.contains(layerName)) {
                        geoJsonLayers[layerName] = visible;
                    }
                    update();
                });

        connect(m_layerPanel, &LayerPanel::geoJsonLayerRemoved,
                this, [this](const QString& layerName) {
                    if (gislib && geoJsonLayers.contains(layerName)) {
                        gislib->removeVectorLayer(layerName);
                        geoJsonLayers.remove(layerName);
                        geoJsonLayerFilePaths.remove(layerName);
                    }

                    geoJsonLayerFilePaths.remove(layerName);
                    update();
                });

        connect(m_layerPanel, &LayerPanel::rasterLayerChanged,
                this, [this]() {
                    update();
                });

        connect(m_layerPanel, &LayerPanel::layerWithShapesRemoved,
                this, [this](const QStringList& shapeIds) {
                    auto& meshes = getTempMeshes();
                    meshes.erase(
                        std::remove_if(meshes.begin(), meshes.end(),
                                       [&shapeIds](const MeshEntry& e) {
                                           if (shapeIds.contains(e.name)) {
                                               if (e.mesh) {
                                                   for (auto v : e.mesh->polygen) delete v;
                                                   delete e.mesh->color;
                                                   delete e.mesh;
                                               }
                                               return true;
                                           }
                                           return false;
                                       }),
                        meshes.end()
                        );

                    if (m_shapeEventHandler) {
                        m_shapeEventHandler->syncCountersFromLoadedShapes(meshes);
                    }

                    update();
                });

        connect(m_layerPanel, &LayerPanel::shapeRemovedFromLayer,
                this, [this](const QString& shapeId) {
                    auto& meshes = getTempMeshes();
                    meshes.erase(
                        std::remove_if(meshes.begin(), meshes.end(),
                                       [&shapeId](const MeshEntry& e) {
                                           if (e.name == shapeId) {
                                               if (e.mesh) {
                                                   for (auto v : e.mesh->polygen) delete v;
                                                   delete e.mesh->color;
                                                   delete e.mesh;
                                               }
                                               return true;
                                           }
                                           return false;
                                       }),
                        meshes.end()
                        );

                    if (m_shapeEventHandler) {
                        m_shapeEventHandler->syncCountersFromLoadedShapes(meshes);
                    }

                    update();
                });

        connect(m_layerPanel, &LayerPanel::shapeClicked,
                this, [this](const QString& shapeId)
                {
                    if (!m_shapeEventHandler)
                        return;

                    // Clear ALL previous canvas shape selections.
                    m_shapeEventHandler->clearSelectedShapeIds();

                    // Clear old single highlighted shape.
                    setHighlightedShapeId("");

                    // Select only the shape clicked from LayerPanel.
                    m_shapeEventHandler->getSelectedShapeIds();

                    // Set it as the active/highlighted shape.
                    setHighlightedShapeId(shapeId);

                    // Center map on selected shape.
                    centerOnShape(shapeId);

                    // Redraw canvas.
                    Refresh();
                });
    }
}

// ============================================================================
// Drag and Drop
// ============================================================================

void CanvasWidget::dragEnterEvents(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasWidget::dragMoveEvents(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
        event->mimeData()->hasFormat("application/x-entity")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasWidget::dropEvents(QDropEvent *event) {
    if (!gislib) return;
    const QMimeData *mimeData = event->mimeData();
    bool accepted = false;

    if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;
            QString text = roleDataMap.value(Qt::DisplayRole).toString();
            QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();

            if (customData["type"].toString() == "entity" &&
                customData.contains("name") && !customData["name"].toString().isEmpty() &&
                customData.contains("ID") && !customData["ID"].toString().isEmpty()) {
                QPoint dropPos = event->pos();
                QPointF geoPos = gislib->canvasToGeo(dropPos);
                QString entityId = customData["ID"].toString();
                if (Meshes.find(entityId.toStdString()) != Meshes.end()) {
                    auto& entry = Meshes[entityId.toStdString()];
                    entry.coreTransform->setGeoCord(geoPos.y(), geoPos.x());
                    if (entry.position) {
                        entry.position->setZ(geoPos.x());
                        entry.position->setX(geoPos.y());
                    }
                    accepted = true;
                }
            }
        }
    }

    if (!accepted && mimeData->hasFormat("application/x-entity")) {
        QByteArray entityData = mimeData->data("application/x-entity");
        QDataStream stream(&entityData, QIODevice::ReadOnly);
    }

    if (accepted) {
        event->acceptProposedAction();
        Refresh();
    } else {
        event->ignore();
    }
}

void CanvasWidget::editor() {
    simulate = false;
}

// ============================================================================
// Transform Modes
// ============================================================================

void CanvasWidget::setTransformMode(TransformMode mode) {
    currentMode = mode;
    if (mode == TransformMode::MeasureDistance) {
        MeasureDistanceDialog* dlg = m_measurementModule->getDialog();
        if (!dlg->isVisible()) {
            dlg->show();
            dlg->raise();
            m_measurementModule->startDistanceMeasurement();
        }
    } else {
        MeasureDistanceDialog* dlg = m_measurementModule->getDialog();
        if (dlg && dlg->isVisible()) {
            dlg->hide();
            m_measurementModule->endDistanceMeasurement();
        }
    }
    setCursor(Qt::ArrowCursor);
    Refresh();
}

void CanvasWidget::setTrajectoryDrawingMode(bool enabled) {
    if (!enabled && !currentTrajectory.empty() && !selectedEntityId.empty()) {
        saveTrajectory();
    }
    isDrawingTrajectory = enabled;

    if (enabled) {
        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end() && it->second.trajectory == nullptr) return;
            if (it->second.trajectory) {
                currentMode = TransformMode::DrawTrajectory;
                setCursor(Qt::CrossCursor);
                for (Waypoints* wp : currentTrajectory) {
                    delete wp->position;
                    delete wp;
                }
                currentTrajectory.clear();
                for (const Waypoints* wp : it->second.trajectory->Trajectories) {
                    Waypoints* newWaypoint = new Waypoints();
                    newWaypoint->fromJson(wp->toJson());
                    currentTrajectory.push_back(newWaypoint);
                }
            } else {
                currentMode = TransformMode::DrawTrajectory;
                setCursor(Qt::CrossCursor);
                currentTrajectory.clear();
            }
        } else {
            currentTrajectory.clear();
        }
    } else {
        currentMode = TransformMode::Translate;
        setCursor(Qt::ArrowCursor);
        deselectWaypoint();
    }
    Refresh();
}
void CanvasWidget::saveTrajectory() {
    if (!isDrawingTrajectory || selectedEntityId.empty()) {
        return;
    }
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) {
        return;
    }
    MeshEntry& entry = it->second;
    if (!entry.trajectory) {
        if (entry.platform /*&& entry.platform->trajectory*/) {
            entry.trajectory = &entry.platform->trajectory;
        } else {
            entry.trajectory = new Trajectory();
            entry.trajectory->ID = selectedEntityId;
        }
    }
    for (Waypoints* wp : entry.trajectory->Trajectories) {
        if (wp) {
            delete wp->position;
            delete wp;
        }
    }
    entry.trajectory->Trajectories.clear();
    for (const Waypoints* waypoint : currentTrajectory) {
        if (!waypoint) continue;
        Waypoints* newWaypoint = new Waypoints();
        QJsonObject wpJson = waypoint->toJson();
        newWaypoint->fromJson(wpJson);
        entry.trajectory->addTrajectory(newWaypoint);
    }
    entry.trajectory->Active = !entry.trajectory->Trajectories.empty();
    QJsonArray waypointsArray;
    for (const Waypoints* wp : entry.trajectory->Trajectories) {
        if (wp) {
            waypointsArray.append(wp->toJson());
        }
    }
    emit trajectoryUpdatedforLogger(QString::fromStdString(selectedEntityId),
                                    entry.trajectory->Trajectories);
    emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
    Refresh();
}

void CanvasWidget::updateWaypointsFromInspector(QString entityId, QJsonArray waypoints) {
    auto it = Meshes.find(entityId.toStdString());
    if (it == Meshes.end()) {
        return;
    }
    MeshEntry& entry = it->second;
    if (!entry.trajectory) {
        entry.trajectory = new Trajectory();
        entry.trajectory->ID = entityId.toStdString();
    }
    for (Waypoints* wp : entry.trajectory->Trajectories) {
        delete wp->position;
        delete wp;
    }
    entry.trajectory->Trajectories.clear();
    for (const QJsonValue& val : waypoints) {
        QJsonObject wpObj = val.toObject();
        if (!wpObj.contains("position")) {
            continue;
        }
        QJsonObject posObj = wpObj["position"].toObject();
        Waypoints* newWaypoint = new Waypoints();
        newWaypoint->position = new Vector(
            posObj["x"].toDouble(),
            posObj["y"].toDouble(),
            posObj["z"].toDouble()
            );
        entry.trajectory->addTrajectory(newWaypoint);
    }
    entry.trajectory->Active = !waypoints.isEmpty();
    if (isDrawingTrajectory && selectedEntityId == entityId.toStdString()) {
        for (Waypoints* wp : currentTrajectory) {
            delete wp->position;
            delete wp;
        }
        currentTrajectory.clear();
        for (const Waypoints* wp : entry.trajectory->Trajectories) {
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->fromJson(wp->toJson());
            currentTrajectory.push_back(newWaypoint);
        }
        deselectWaypoint();
    }
    Refresh();
}

void CanvasWidget::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        zoomLevel += 1;
    } else {
        zoomLevel -= 1;
    }
    if (zoomLevel > 19) {
        zoomLevel = 19;
    } else if (zoomLevel < 1) {
        zoomLevel = 1;
    }
    Refresh();
}

// ============================================================================
// Mouse Event Handlers
// ============================================================================

void CanvasWidget::handleMousePress(QMouseEvent *event)
{
    if (!gislib)
        return;

    // ========================================================================
    // BASIC LEFT CLICK STATE
    // ========================================================================
    if (event->button() == Qt::LeftButton)
    {
        m_lastLeftClickGeo = gislib->canvasToGeo(event->pos());

        // IMPORTANT:
        // Do not clear highlighted shape yet if we are about to start
        // a multi-selection drag. This avoids Line/Polyline highlight
        // behaving differently from other selected shapes.
        //
        // Normal clicks will clear it below after the multi-drag check.
    }


    // ========================================================================
    // DOUBLE CLICK HANDLING
    // ========================================================================
    if (event->button() == Qt::LeftButton &&
        event->type() == QEvent::MouseButtonDblClick)
    {
        // --------------------------------------------------------------------
        // Finalize Line / Polygon drawing
        // --------------------------------------------------------------------
        if (m_shapeEventHandler &&
            currentMode == TransformMode::DrawShape)
        {
            QString selShape = selectedShape;

            if (selShape == "Line" ||
                selShape == "Polygon")
            {
                m_shapeEventHandler->mouseDoubleClickEvent(event);
                return;
            }
        }

        m_tooltipTimer.stop();
        QToolTip::hideText();

        // --------------------------------------------------------------------
        // Runtime Editor Entity Double Click
        // --------------------------------------------------------------------
        bool isRuntimeEditor = false;

        QWidget* wCheck = this;

        while (wCheck)
        {
            if (wCheck->metaObject()->className() ==
                QString("RuntimeEditor"))
            {
                isRuntimeEditor = true;
                break;
            }

            wCheck = wCheck->parentWidget();
        }

        if (isRuntimeEditor)
        {
            for (auto& [id, entry] : Meshes)
            {
                if (!entry.coreTransform /*||
                    !entry.coreTransform->geocord*/)
                {
                    continue;
                }

                QPointF entityPos =
                    gislib->geoToCanvas(
                        entry.coreTransform->getLatitude(),
                        entry.coreTransform->getLongitude()
                        );

                float imageSize = 20.0f;

                if (entry.individualImageSize > 0 ||
                    ImageScale > 0)
                {
                    imageSize =
                        (entry.individualImageSize > 0
                             ? entry.individualImageSize
                             : ImageScale) /
                        2.0f;
                }

                if (QVector2D(event->pos() - entityPos).length()
                    < imageSize)
                {
                    selectedEntityId = id;

                    if (entry.entity)
                    {
                        showEntityInfo(
                            QString::fromStdString(id)
                            );
                    }

                    return;
                }
            }
        }


        // --------------------------------------------------------------------
        // Trajectory Double Click Detection
        // --------------------------------------------------------------------
        for (auto& [id, entry] : Meshes)
        {
            if (!entry.coreTransform /*||
                !entry.coreTransform->geocord*/)
            {
                continue;
            }

            if (!entry.trajectory ||
                entry.trajectory->Trajectories.empty())
            {
                continue;
            }

            QPointF entityPos =
                gislib->geoToCanvas(
                    entry.coreTransform->getLatitude(),
                    entry.coreTransform->getLongitude()
                    );

            float imageSize = 20.0f;

            if (entry.individualImageSize > 0 ||
                ImageScale > 0)
            {
                imageSize =
                    (entry.individualImageSize > 0
                         ? entry.individualImageSize
                         : ImageScale) /
                    2.0f;
            }

            if (QVector2D(event->pos() - entityPos).length()
                < imageSize)
            {
                continue;
            }


            bool trajectoryClicked = false;


            // ----------------------------------------------------------------
            // Check Waypoints
            // ----------------------------------------------------------------
            for (size_t i = 0;
                 i < entry.trajectory->Trajectories.size();
                 ++i)
            {
                Waypoints* waypoint =
                    entry.trajectory->Trajectories[i];

                if (!waypoint ||
                    !waypoint->position)
                {
                    continue;
                }

                QPointF wpCanvas =
                    gislib->geoToCanvas(
                        waypoint->position->x,
                        waypoint->position->z
                        );

                if (QVector2D(event->pos() - wpCanvas).length()
                    < 10.0f)
                {
                    trajectoryClicked = true;
                    break;
                }
            }


            // ----------------------------------------------------------------
            // Check Trajectory Lines
            // ----------------------------------------------------------------
            if (!trajectoryClicked &&
                entry.trajectory->Trajectories.size() >= 2)
            {
                const auto& traj =
                    entry.trajectory->Trajectories;

                for (size_t i = 0;
                     i < traj.size() - 1;
                     ++i)
                {
                    if (!traj[i] ||
                        !traj[i + 1] ||
                        !traj[i]->position ||
                        !traj[i + 1]->position)
                    {
                        continue;
                    }

                    QPointF p1 =
                        gislib->geoToCanvas(
                            traj[i]->position->x,
                            traj[i]->position->z
                            );

                    QPointF p2 =
                        gislib->geoToCanvas(
                            traj[i + 1]->position->x,
                            traj[i + 1]->position->z
                            );

                    QPointF lineVec =
                        p2 - p1;

                    QPointF pointVec =
                        event->pos() - p1;

                    float lineLengthSquared =
                        lineVec.x() * lineVec.x() +
                        lineVec.y() * lineVec.y();

                    if (lineLengthSquared < 1e-6)
                    {
                        continue;
                    }

                    float t =
                        (pointVec.x() * lineVec.x() +
                         pointVec.y() * lineVec.y()) /
                        lineLengthSquared;

                    if (t < 0.0f)
                        t = 0.0f;

                    if (t > 1.0f)
                        t = 1.0f;

                    QPointF projection =
                        p1 + t * lineVec;

                    if (QVector2D(
                            event->pos() - projection
                            ).length() < 8.0f)
                    {
                        trajectoryClicked = true;
                        break;
                    }
                }
            }


            if (trajectoryClicked)
            {
                selectedEntityId = id;

                clearSonobuoyHighlight();

                emit selectEntitybyCursor(
                    QString::fromStdString(id)
                    );

                return;
            }
        }


        // --------------------------------------------------------------------
        // Box Zoom on Empty Canvas
        // --------------------------------------------------------------------
        if (isClickOnEmptyCanvas(event->pos()))
        {
            boxZoomPending = true;
            boxZoomStart = event->pos();
            boxZoomCurrent = event->pos();

            doubleClickTimer.start();

            return;
        }

        boxZoomPending = false;
        return;
    }


    // ========================================================================
    // RIGHT CLICK
    // ========================================================================
    if (event->button() == Qt::RightButton)
    {
        handleRightClick(event);
        return;
    }


    // ========================================================================
    // MIDDLE BUTTON PANNING
    // ========================================================================
    if (event->button() == Qt::MiddleButton)
    {
        isPanning = true;
        lastMousePos = event->pos();

        setCursor(Qt::ClosedHandCursor);

        return;
    }


    // ========================================================================
    // MEASURE DISTANCE
    // ========================================================================
    if (currentMode == TransformMode::MeasureDistance)
    {
        m_measurementModule->handleMousePress(event);

        Refresh();

        return;
    }


    // ========================================================================
    // CRITICAL FIX:
    // MULTI-SELECTION GROUP DRAG GETS PRIORITY
    //
    // This MUST execute before:
    //
    // handleTextMousePress()
    // handleBitmapsMousePress()
    // handleShapeSelection()
    //
    // IMPORTANT:
    // isClickOnAnySelectedItem() now performs REAL geometry hit testing.
    // It will not start group drag merely because another imported layer
    // contains an object with a similar/duplicate legacy Temp name.
    // ========================================================================
    if (event->button() == Qt::LeftButton &&
        currentMode == TransformMode::Translate &&
        !isDrawingTrajectory &&
        isMultiSelecting())
    {
        const auto& selectedShapes =
            getSelectedShapeIds();

        const size_t entityCount =
            selectedEntityIds.size();

        const size_t shapeCount =
            selectedShapes.size();

        const bool genuineMultiSelect =
            (entityCount + shapeCount) > 1;


        if (genuineMultiSelect)
        {
            const bool clickedSelectedItem =
                isClickOnAnySelectedItem(
                    event->pos()
                    );


            if (clickedSelectedItem)
            {
                m_tooltipTimer.stop();
                QToolTip::hideText();

                // ------------------------------------------------------------
                // Start group drag
                // ------------------------------------------------------------
                isMultiDrag = true;

                multiDragStartGeo =
                    gislib->canvasToGeo(
                        event->pos()
                        );


                // ------------------------------------------------------------
                // Disable single entity transform drag
                // ------------------------------------------------------------
                activeDragAxis.clear();


                // ------------------------------------------------------------
                // IMPORTANT:
                //
                // Do NOT call:
                //
                // setHighlightedShapeId("")
                // handleTextMousePress()
                // handleBitmapsMousePress()
                // handleShapeSelection()
                //
                // Otherwise individual item drag will override group drag.
                // ------------------------------------------------------------
                setCursor(
                    Qt::ClosedHandCursor
                    );

                event->accept();

                Refresh();

                return;
            }
        }
    }


    // ========================================================================
    // NORMAL LEFT CLICK
    //
    // We only clear single-shape hover/highlight AFTER checking multi-drag.
    // This is important for selected Line/Polyline visual state.
    // ========================================================================
    if (event->button() == Qt::LeftButton)
    {
        setHighlightedShapeId("");
    }


    // ========================================================================
    // INDIVIDUAL TEXT / BITMAP HANDLING
    //
    // MUST remain AFTER multi-drag check.
    // ========================================================================
    if (event->button() == Qt::LeftButton &&
        !isDrawingTrajectory &&
        currentMode != TransformMode::DrawShape)
    {
        // --------------------------------------------------------------------
        // Text Individual Mouse Press
        // --------------------------------------------------------------------
        if (m_shapeEventHandler &&
            m_shapeEventHandler->handleTextMousePress(event))
        {
            return;
        }


        // --------------------------------------------------------------------
        // Bitmap Individual Mouse Press
        // --------------------------------------------------------------------
        if (m_shapeEventHandler &&
            m_shapeEventHandler->handleBitmapsMousePress(event))
        {
            return;
        }
    }


    // ========================================================================
    // MAIN LEFT CLICK HANDLING
    // ========================================================================
    if (event->button() == Qt::LeftButton)
    {
        m_tooltipTimer.stop();
        QToolTip::hideText();


        // ====================================================================
        // BITMAP HANDLING
        //
        // Note:
        // Keeping this because it existed in your original function.
        // If already consumed above, execution never reaches here.
        // ====================================================================
        if (m_shapeEventHandler &&
            m_shapeEventHandler->handleBitmapsMousePress(event))
        {
            return;
        }


        // ====================================================================
        // INDIVIDUAL SHAPE SELECTION / DRAG
        //
        // This executes only when Group Drag did NOT consume the click.
        // ====================================================================
        if (m_shapeEventHandler &&
            m_shapeEventHandler->handleShapeSelection(event))
        {
            return;
        }


        // ====================================================================
        // SHAPE EVENT HANDLER
        // ====================================================================
        if (m_shapeEventHandler)
        {
            bool boxSelectTriggered = false;


            // ----------------------------------------------------------------
            // Box Selection Start
            // ----------------------------------------------------------------
            if (m_shapeEventHandler->isBoxSelectionMode() &&
                isClickOnEmptyCanvas(event->pos()))
            {
                boxSelectTriggered = true;
            }


            if (boxSelectTriggered)
            {
                m_shapeEventHandler->mousePressEvent(event);

                return;
            }
            else
            {
                // ------------------------------------------------------------
                // Temporarily disable Box Selection for normal shape handling
                // ------------------------------------------------------------
                bool
                    Mode =
                    m_shapeEventHandler->isBoxSelectionMode();


                if (Mode)
                {
                    m_shapeEventHandler
                        ->setBoxSelectionMode(false);
                }


                m_shapeEventHandler
                    ->mousePressEvent(event);


                if (Mode)
                {
                    m_shapeEventHandler
                        ->setBoxSelectionMode(true);
                }
            }


            // ----------------------------------------------------------------
            // Shape Handler Consumed Operation
            // ----------------------------------------------------------------
            if (currentMode ==
                    TransformMode::DrawShape ||

                    currentMode ==
                    TransformMode::EditShape ||

                    currentMode ==
                    TransformMode::PlaceBitmap ||

                m_shapeEventHandler
                    ->isDraggingShape() ||

                m_shapeEventHandler
                    ->isResizingShape() ||

                !m_shapeEventHandler
                    ->getEditingShapeId()
                    .isEmpty() ||

                !m_shapeEventHandler
                    ->getActiveRotateId()
                    .isEmpty() ||

                m_shapeEventHandler
                    ->isPlacingBitmap())
            {
                return;
            }
        }
    }


    // ========================================================================
    // MEASURE DISTANCE GUARD
    // ========================================================================
    if (currentMode ==
        TransformMode::MeasureDistance)
    {
        return;
    }


    // ========================================================================
    // DRAW TRAJECTORY
    // ========================================================================
    if (currentMode ==
            TransformMode::DrawTrajectory &&

        isDrawingTrajectory &&

        !selectedEntityId.empty())
    {
        QPointF geoPos =
            gislib->canvasToGeo(
                event->pos()
                );


        int nearestIndex =
            findNearestWaypoint(
                event->pos()
                );


        if (nearestIndex >= 0)
        {
            selectWaypoint(
                nearestIndex
                );

            isDraggingWaypoint = true;
        }
        else
        {
            Waypoints* waypoint =
                new Waypoints();


            waypoint->position =
                new Vector(
                    geoPos.y(),
                    0,
                    geoPos.x()
                    );


            currentTrajectory
                .push_back(
                    waypoint
                    );


            selectWaypoint(
                currentTrajectory.size() - 1
                );


            updateTrajectoryData();
        }


        Refresh();

        return;
    }


    // ========================================================================
    // ENTITY TRANSFORM GIZMO
    // ========================================================================
    if (!selectedEntityId.empty())
    {
        auto it =
            Meshes.find(
                selectedEntityId
                );


        if (it != Meshes.end())
        {
            auto& entry =
                it->second;


            if (entry.coreTransform /*&&
                entry.coreTransform->geocord*/)
            {
                QPointF basePos =
                    gislib->geoToCanvas(
                        entry.coreTransform
                            ->getLatitude(),

                        entry.coreTransform
                            ->getLongitude()
                        );


                const float handleTolerance =
                    15.0f;


                // ============================================================
                // TRANSLATE
                // ============================================================
                if (currentMode ==
                    TransformMode::Translate)
                {
                    QPointF xAxisHandle =
                        basePos +
                        QPointF(50, 0);


                    QPointF yAxisHandle =
                        basePos +
                        QPointF(0, -50);


                    if (QVector2D(
                            event->pos() -
                            xAxisHandle
                            ).length()
                        < handleTolerance)
                    {
                        activeDragAxis = "x";

                        dragStartPos =
                            event->pos();


                        emit MoveEntity(
                            QString::fromStdString(
                                selectedEntityId
                                )
                            );


                        Refresh();

                        return;
                    }


                    if (QVector2D(
                            event->pos() -
                            yAxisHandle
                            ).length()
                        < handleTolerance)
                    {
                        activeDragAxis = "y";

                        dragStartPos =
                            event->pos();


                        emit MoveEntity(
                            QString::fromStdString(
                                selectedEntityId
                                )
                            );


                        Refresh();

                        return;
                    }
                }


                // ============================================================
                // ROTATE
                // ============================================================
                else if (currentMode ==
                         TransformMode::Rotate)
                {
                    float distFromCenter =
                        QVector2D(
                            event->pos() -
                            basePos
                            ).length();


                    if (qAbs(
                            distFromCenter -
                            40.0f
                            ) < handleTolerance)
                    {
                        activeDragAxis =
                            "rotate";


                        dragStartPos =
                            event->pos();


                        emit MoveEntity(
                            QString::fromStdString(
                                selectedEntityId
                                )
                            );


                        Refresh();

                        return;
                    }
                }


                // ============================================================
                // SCALE
                // ============================================================
                else if (currentMode ==
                         TransformMode::Scale)
                {
                    QPointF xScaleHandle =
                        basePos +
                        QPointF(50, 0);


                    QPointF yScaleHandle =
                        basePos +
                        QPointF(0, 50);


                    if (QVector2D(
                            event->pos() -
                            xScaleHandle
                            ).length()
                        < handleTolerance)
                    {
                        activeDragAxis =
                            "scale-x";


                        dragStartPos =
                            event->pos();


                        emit MoveEntity(
                            QString::fromStdString(
                                selectedEntityId
                                )
                            );


                        Refresh();

                        return;
                    }


                    if (QVector2D(
                            event->pos() -
                            yScaleHandle
                            ).length()
                        < handleTolerance)
                    {
                        activeDragAxis =
                            "scale-y";


                        dragStartPos =
                            event->pos();


                        emit MoveEntity(
                            QString::fromStdString(
                                selectedEntityId
                                )
                            );


                        Refresh();

                        return;
                    }
                }
            }
        }
    }


    bool entityWasClicked = false;
    for (auto& [id, entry] : Meshes) {
        if (!entry.coreTransform /*|| !entry.coreTransform->geocord*/) continue;
        if (!entry.entity || !entry.entity->Active) continue;

        QPointF entityPos = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude());

        if (QVector2D(event->pos() - entityPos).length() < 20.0f) {
            if (selectedEntityId != id) {
                selectedEntityId = id;
                m_highlightedWaypointIndex = -1;
                m_highlightedWaypointEntityId = "";
                emit selectEntitybyCursor(QString::fromStdString(id));
            }
            selectedEntityIds.clear();
            getSelectedShapeIds().clear();
            dragStartPos = event->pos();
            activeDragAxis = "both";
            entityWasClicked = true;
            Refresh();
            return;
        }
    }

    if (!entityWasClicked && currentMode != TransformMode::DrawTrajectory &&
        currentMode != TransformMode::DrawShape && currentMode != TransformMode::PlaceBitmap) {
        clearSelectedShapeIds();
        setHighlightedShapeId("");
        selectedEntityId = "";
        selectedEntityIds.clear();
        activeDragAxis = "";
        m_highlightedWaypointIndex = -1;
        m_highlightedWaypointEntityId = "";
        clearSonobuoyHighlight();
        Refresh();
    }
}

void CanvasWidget::handleMouseMove(QMouseEvent *event)
{
    if (!gislib)
        return;

    m_lastMousePos = event->pos();


    // ========================================================================
    // FIX: MULTI-SELECTION GROUP DRAG — HIGHEST PRIORITY
    //
    // Agar group drag active hai aur Left Mouse Button pressed hai,
    // to selected Entity + Shape + Line + Point + Text sab ek saath move honge.
    //
    // Individual handlers ko execute nahi hone dena.
    // ========================================================================
    if (isMultiDrag)
    {
        // Agar left mouse button ab pressed nahi hai,
        // accidental/stuck multi-drag prevent karo.
        if (!(event->buttons() & Qt::LeftButton))
        {
            isMultiDrag = false;
            setCursor(Qt::ArrowCursor);
            Refresh();
            return;
        }

        QPointF currentGeo =
            gislib->canvasToGeo(event->pos());

        QPointF deltaGeo =
            currentGeo - multiDragStartGeo;


        // Tiny/no movement ko ignore karo
        if (!qFuzzyIsNull(deltaGeo.x()) ||
            !qFuzzyIsNull(deltaGeo.y()))
        {
            // Full selected group move karo
            moveSelectedItems(deltaGeo);

            // Next mouse move ke liye current position save karo.
            // Isse accumulated delta/jumping nahi hogi.
            multiDragStartGeo = currentGeo;
        }

        setCursor(Qt::ClosedHandCursor);

        Refresh();

        event->accept();

        // CRITICAL:
        // Neeche ke individual handlers execute nahi hone chahiye:
        //
        // handleTextMouseMove()
        // handleBitmapsMouseMove()
        // mouseMoveEvent()
        // Entity drag
        //
        return;
    }


    // ========================================================================
    // ENTITY HOVER
    // ========================================================================

    checkEntityHover(event->pos());


    // ========================================================================
    // BOX ZOOM PENDING
    // ========================================================================

    if (boxZoomPending)
    {
        QPoint delta =
            event->pos() - boxZoomStart;

        if (qAbs(delta.x()) > 5 ||
            qAbs(delta.y()) > 5)
        {
            isBoxZooming = true;
            boxZoomPending = false;
        }
    }


    // ========================================================================
    // BOX ZOOM ACTIVE
    // ========================================================================

    if (isBoxZooming)
    {
        boxZoomCurrent =
            event->pos();

        Refresh();
        return;
    }


    // ========================================================================
    // SHAPE / TEXT / BITMAP INDIVIDUAL MOUSE MOVE
    //
    // Ye block ONLY tab chalega jab Multi Drag active nahi hai.
    // ========================================================================

    if (m_shapeEventHandler)
    {
        // Text individual dragging
        m_shapeEventHandler->handleTextMouseMove(event);

        // Bitmap individual dragging
        m_shapeEventHandler->handleBitmapsMouseMove(event);

        // Normal Shape / Point / Line handling
        m_shapeEventHandler->mouseMoveEvent(event);


        // Agar ShapeEventHandler ne event consume kar liya hai
        // ya koi individual operation active hai,
        // to Canvas Entity handling tak mat jao.
        if (m_shapeEventHandler->isDraggingShape() ||
            m_shapeEventHandler->isResizingShape() ||
            m_shapeEventHandler->isPlacingBitmap() ||
            !m_shapeEventHandler->getEditingShapeId().isEmpty() ||
            !m_shapeEventHandler->getActiveRotateId().isEmpty())
        {
            return;
        }
    }


    // ========================================================================
    // CANVAS PANNING
    // ========================================================================

    if (isPanning)
    {
        QPoint delta =
            event->pos() - lastMousePos;

        canvasOffset += delta;

        lastMousePos =
            event->pos();

        Refresh();
        return;
    }


    // ========================================================================
    // MEASURE DISTANCE MODE
    // ========================================================================

    if (currentMode == TransformMode::MeasureDistance)
    {
        m_measurementModule->handleMouseMove(event);

        Refresh();
        return;
    }


    // ========================================================================
    // TRAJECTORY WAYPOINT DRAGGING
    // ========================================================================

    if (isDrawingTrajectory &&
        isDraggingWaypoint &&
        selectedWaypointIndex >= 0 &&
        selectedWaypointIndex < static_cast<int>(currentTrajectory.size()))
    {
        QPointF geoPos =
            gislib->canvasToGeo(event->pos());

        Waypoints* wp =
            currentTrajectory[selectedWaypointIndex];

        if (wp && wp->position)
        {
            wp->position->z =
                geoPos.x();

            wp->position->x =
                geoPos.y();
        }

        Refresh();
        return;
    }


    // ========================================================================
    // SINGLE ENTITY TRANSFORM
    //
    // IMPORTANT:
    // Multi Drag already top par handle ho chuka hai.
    // Isliye yahan sirf normal single entity operation aayega.
    // ========================================================================

    if (!selectedEntityId.empty() &&
        !activeDragAxis.isEmpty())
    {
        // Left button release ho gaya to drag stop karo
        if (!(event->buttons() & Qt::LeftButton))
        {
            activeDragAxis.clear();
            return;
        }


        auto it =
            Meshes.find(selectedEntityId);

        if (it == Meshes.end())
            return;


        MeshEntry& entry =
            it->second;


        if (!entry.coreTransform /*||
            !entry.coreTransform->geocord*/)
        {
            return;
        }


        // ====================================================================
        // TRANSLATE
        // ====================================================================

        if (currentMode == TransformMode::Translate)
        {
            QPointF newGeoPos =
                gislib->canvasToGeo(event->pos());


            // ----------------------------------------------------------------
            // X AXIS
            // ----------------------------------------------------------------

            if (activeDragAxis == "x")
            {
                entry.coreTransform->setLongitude(
                    newGeoPos.x()
                    );
            }


            // ----------------------------------------------------------------
            // Y AXIS
            // ----------------------------------------------------------------

            else if (activeDragAxis == "y")
            {
                entry.coreTransform->setLatitude(
                    newGeoPos.y()
                    );
            }


            // ----------------------------------------------------------------
            // FREE MOVEMENT
            // ----------------------------------------------------------------

            else if (activeDragAxis == "both")
            {
                bool isBoxSelectMode =
                    m_shapeEventHandler
                        ? m_shapeEventHandler->isBoxSelectionMode()
                        : false;


                // ============================================================
                // BOX SELECTION MODE
                //
                // Normally ye case Multi Drag ke top block mein handle hoga.
                // Ye fallback single-entity behavior ke liye rakha hai.
                // ============================================================

                if (isBoxSelectMode)
                {
                    double oldLat =
                        entry.coreTransform->getLatitude();

                    double oldLon =
                        entry.coreTransform->getLongitude();


                    entry.coreTransform->setGeoCord(
                        newGeoPos.y(),
                        newGeoPos.x()
                        );


                    double deltaLat =
                        newGeoPos.y() - oldLat;

                    double deltaLon =
                        newGeoPos.x() - oldLon;


                    // --------------------------------------------------------
                    // Move entity trajectory
                    // --------------------------------------------------------

                    if (entry.trajectory &&
                        !entry.trajectory->Trajectories.empty())
                    {
                        for (Waypoints* wp :
                             entry.trajectory->Trajectories)
                        {
                            if (wp &&
                                wp->position)
                            {
                                wp->position->x +=
                                    deltaLat;

                                wp->position->z +=
                                    deltaLon;
                            }
                        }
                    }


                    // --------------------------------------------------------
                    // Move current trajectory
                    // --------------------------------------------------------

                    for (Waypoints* wp :
                         currentTrajectory)
                    {
                        if (wp &&
                            wp->position)
                        {
                            wp->position->x +=
                                deltaLat;

                            wp->position->z +=
                                deltaLon;
                        }
                    }
                }

                else
                {
                    entry.coreTransform->setGeoCord(
                        newGeoPos.y(),
                        newGeoPos.x()
                        );
                }
            }


            emit MoveEntity(
                QString::fromStdString(
                    selectedEntityId
                    )
                );
        }


        // ====================================================================
        // ROTATE
        // ====================================================================

        else if (currentMode == TransformMode::Rotate)
        {
            QPointF basePos =
                gislib->geoToCanvas(
                    entry.coreTransform->getLatitude(),
                    entry.coreTransform->getLongitude()
                    );


            qreal angle_new =
                qAtan2(
                    event->pos().y() - basePos.y(),
                    event->pos().x() - basePos.x()
                    );


            qreal angle_old =
                qAtan2(
                    dragStartPos.y() - basePos.y(),
                    dragStartPos.x() - basePos.x()
                    );


            float angle_change =
                -qRadiansToDegrees(
                    angle_new - angle_old
                    );


            entry.coreTransform->setRotation(
                QQuaternion::fromEulerAngles(
                    QVector3D(
                        0,
                        entry.coreTransform
                                ->rotation()
                                .toEulerAngles()
                                .y() - angle_change,
                        0
                        )
                    )
                );


            emit MoveEntity(
                QString::fromStdString(
                    selectedEntityId
                    )
                );
        }


        // ====================================================================
        // SCALE
        // ====================================================================

        else if (currentMode == TransformMode::Scale)
        {
            QPointF basePos =
                gislib->geoToCanvas(
                    entry.coreTransform->getLatitude(),
                    entry.coreTransform->getLongitude()
                    );


            qreal dist_new =
                QVector2D(
                    event->pos() - basePos
                    ).length();


            qreal dist_old =
                QVector2D(
                    dragStartPos - basePos
                    ).length();


            // Division by zero protection
            if (dist_old <= 0.0001)
                return;


            qreal scaleFactor =
                dist_new / dist_old;


            // ----------------------------------------------------------------
            // SCALE X
            // ----------------------------------------------------------------

            if (activeDragAxis == "x")
            {
                QVector3D sc(
                    0,
                    0,
                    entry.coreTransform
                            ->scale3D()
                            .z() * scaleFactor
                    );

                entry.coreTransform->setScale3D(sc);
            }


            // ----------------------------------------------------------------
            // SCALE Y
            // ----------------------------------------------------------------

            else if (activeDragAxis == "y")
            {
                QVector3D sc(
                    entry.coreTransform
                            ->scale3D()
                            .x() * scaleFactor,
                    0,
                    0
                    );

                entry.coreTransform->setScale3D(sc);
            }


            // ----------------------------------------------------------------
            // SCALE BOTH
            // ----------------------------------------------------------------

            else if (activeDragAxis == "both")
            {
                QVector3D sc(
                    entry.coreTransform
                            ->scale3D()
                            .x() * scaleFactor,

                    0,

                    entry.coreTransform
                            ->scale3D()
                            .z() * scaleFactor
                    );

                entry.coreTransform->setScale3D(sc);
            }


            emit MoveEntity(
                QString::fromStdString(
                    selectedEntityId
                    )
                );
        }


        // ====================================================================
        // Update drag position
        // ====================================================================

        dragStartPos =
            event->pos();

        Refresh();
    }
}

void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
    if (!gislib) return;
    if (event->button() == Qt::LeftButton && isBoxZooming) {
        QRect zoomBox = QRect(boxZoomStart, boxZoomCurrent).normalized();
        if (zoomBox.width() > 10 && zoomBox.height() > 10) {
            QPointF topLeftGeo = gislib->canvasToGeo(zoomBox.topLeft());
            QPointF topRightGeo = gislib->canvasToGeo(zoomBox.topRight());
            QPointF bottomLeftGeo = gislib->canvasToGeo(zoomBox.bottomLeft());
            QPointF bottomRightGeo = gislib->canvasToGeo(zoomBox.bottomRight());
            double minLon = qMin(qMin(topLeftGeo.x(), topRightGeo.x()),
                                 qMin(bottomLeftGeo.x(), bottomRightGeo.x()));
            double maxLon = qMax(qMax(topLeftGeo.x(), topRightGeo.x()),
                                 qMax(bottomLeftGeo.x(), bottomRightGeo.x()));
            double minLat = qMin(qMin(topLeftGeo.y(), topRightGeo.y()),
                                 qMin(bottomLeftGeo.y(), bottomRightGeo.y()));
            double maxLat = qMax(qMax(topLeftGeo.y(), topRightGeo.y()),
                                 qMax(bottomLeftGeo.y(), bottomRightGeo.y()));
            gislib->fitToBounds(minLat, minLon, maxLat, maxLon, 0);
        }
        isBoxZooming = false;
        boxZoomPending = false;
        Refresh();
        return;
    }

    if (boxZoomPending) {
        boxZoomPending = false;
    }

    if (event->button() == Qt::LeftButton) {
        // Box selection release removed here, delegated to ShapeEventHandler
        if (m_shapeEventHandler) {
            m_shapeEventHandler->handleBitmapsMouseRelease(event);
            m_shapeEventHandler->mouseReleaseEvent(event);
            m_shapeEventHandler->handleTextMouseRelease(event);
        }
        activeDragAxis = "";
        isDraggingWaypoint = false;
        selectEntity = false;
        updateTrajectoryData();
        if (isMultiDrag) {
            isMultiDrag = false;
            setCursor(Qt::ArrowCursor);
            return;
        }
    }
    if (currentMode == TransformMode::MeasureDistance) {
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    Refresh();
}

// ============================================================================
// Key Event Handler
// ============================================================================

void CanvasWidget::handleKeyPress(QKeyEvent *event) {
    if (m_shapeEventHandler) {
        m_shapeEventHandler->handleBitmapsKeyPress(event);
        m_shapeEventHandler->keyPressEvent(event);
    }

    if (event->key() == Qt::Key_Delete && !selectedEntityId.empty() && currentMode != TransformMode::DrawTrajectory) {
        auto it = Meshes.find(selectedEntityId);
        if (it != Meshes.end() && it->second.entity) {
            QString entityId = QString::fromStdString(selectedEntityId);
            QString parentId = QString::fromStdString(it->second.entity->parentID);
            std::string idToErase = selectedEntityId;
            selectedEntityId.clear();
            selectedEntityIds.clear();

            ScenarioEditor* se = nullptr;
            RuntimeEditor* re = nullptr;
            QWidget* w = this;
            while (w) {
                if (!se) se = qobject_cast<ScenarioEditor*>(w);
                if (!re) re = qobject_cast<RuntimeEditor*>(w);
                w = w->parentWidget();
            }
            if (se && se->hierarchy)
                se->hierarchy->removeEntity(parentId, entityId);
            else if (re && re->hierarchy)
                re->hierarchy->removeEntity(parentId, entityId);

            auto it2 = Meshes.find(idToErase);
            if (it2 != Meshes.end())
                Meshes.erase(it2);
            Refresh();
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F2 && !selectedEntityId.empty()) {
        auto it = Meshes.find(selectedEntityId);
        if (it != Meshes.end() && it->second.entity) {
            QString currentName = QString::fromStdString(it->second.entity->Name);
            QInputDialog dialog;
            dialog.setWindowTitle("Rename");
            dialog.setLabelText("Enter new name:");
            dialog.setTextValue(currentName);
            dialog.setWindowModality(Qt::ApplicationModal);
            dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

            if (dialog.exec() == QDialog::Accepted) {
                QString newName = dialog.textValue();
                if (!newName.trimmed().isEmpty() && newName != currentName) {
                    ScenarioEditor* se = nullptr;
                    RuntimeEditor* re = nullptr;
                    QWidget* w = this;
                    while (w) {
                        if (!se) se = qobject_cast<ScenarioEditor*>(w);
                        if (!re) re = qobject_cast<RuntimeEditor*>(w);
                        w = w->parentWidget();
                    }
                    if (se && se->hierarchy)
                        se->hierarchy->renameEntity(QString::fromStdString(selectedEntityId), newName);
                    else if (re && re->hierarchy)
                        re->hierarchy->renameEntity(QString::fromStdString(selectedEntityId), newName);
                    Refresh();
                }
            }
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F) {
        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end()) {
                MeshEntry& entry = it->second;
                if (entry.platform && entry.platform->Active) {
                    entry.platform->fireMissile();
                }
            }
        }
        return;
    }

    if (event->key() == Qt::Key_M) {
        if (!selectedEntityId.empty()) {
            auto it = Meshes.find(selectedEntityId);
            if (it != Meshes.end()) {
                MeshEntry& entry = it->second;
                if (entry.platform && entry.platform->Active) {
                    Platform* entity = entry.platform;
                    if (!entry.radioVisible) return;
                    if (!entity) return;
                    // if (!entity->radios) return;
                    bool ok;
                    QString text = QInputDialog::getText(this, tr("Input Box Title"),
                                                         tr("Enter your name:"), QLineEdit::Normal,
                                                         "Default Text", &ok);
                    if (ok && !text.isEmpty()) {
                        for (auto const& pair : entity->radios.radios) {
                            Radio* s = pair.second;
                            s->sendMsg(text.toStdString());
                        }
                    }
                }
            }
        }
        return;
    }

    if (event->key() == Qt::Key_Escape && currentMode == TransformMode::MeasureDistance) {
        setTransformMode(TransformMode::Translate);
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        if (currentMode == TransformMode::DrawTrajectory && isDrawingTrajectory) {
            if (!selectedEntityId.empty()) {
                auto it = Meshes.find(selectedEntityId);
                if (it != Meshes.end()) {
                    MeshEntry& entry = it->second;
                    for (Waypoints* wp : currentTrajectory) {
                        delete wp->position;
                        delete wp;
                    }
                    currentTrajectory.clear();
                    if (entry.trajectory) {
                        for (const Waypoints* wp : entry.trajectory->Trajectories) {
                            Waypoints* newWaypoint = new Waypoints();
                            newWaypoint->fromJson(wp->toJson());
                            currentTrajectory.push_back(newWaypoint);
                        }
                    }
                }
            }
            setTrajectoryDrawingMode(false);
            deselectWaypoint();
            return;
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (currentMode == TransformMode::DrawTrajectory && isDrawingTrajectory) {
            saveTrajectory();
            setTrajectoryDrawingMode(false);
            deselectWaypoint();
            return;
        }
    } else if (event->key() == Qt::Key_5) {
        setTransformMode(TransformMode::MeasureDistance);
    } else if (event->key() == Qt::Key_1) {
        setTransformMode(TransformMode::Translate);
    } else if (event->key() == Qt::Key_2) {
        setTransformMode(TransformMode::Rotate);
    } else if (event->key() == Qt::Key_3) {
        setTransformMode(TransformMode::Scale);
    } else if (event->key() == Qt::Key_4) {
        simulate = !simulate;
    } else if (event->key() == Qt::Key_Delete && currentMode == TransformMode::DrawTrajectory &&
               selectedWaypointIndex >= 0 && selectedWaypointIndex < (int)currentTrajectory.size()) {
        delete currentTrajectory[selectedWaypointIndex]->position;
        delete currentTrajectory[selectedWaypointIndex];
        currentTrajectory.erase(currentTrajectory.begin() + selectedWaypointIndex);
        deselectWaypoint();
        updateTrajectoryData();
        Refresh();
    }
    Refresh();
}

// ============================================================================
// Right-Click Handler - Simplified
// ============================================================================

void CanvasWidget::handleRightClick(QMouseEvent *event) {
    if (!gislib) return;
    m_tooltipTimer.stop();
    QToolTip::hideText();
    activeDragAxis = "";
    isDraggingWaypoint = false;

    if (currentMode == TransformMode::DrawShape &&
        (selectedShape == "Line" || selectedShape == "Polygon")) {
        bool canFinalize = (selectedShape == "Line" && getTempLineVertices().size() >= 2) ||
                           (selectedShape == "Polygon" && getTempPolygonVertices().size() >= 3);
        if (canFinalize) {
            QPointF lastGeoPos;
            if (selectedShape == "Line" && !getTempLineVertices().empty()) {
                Vector* v = getTempLineVertices().back();
                lastGeoPos = QPointF(v->x, v->y);
            } else if (selectedShape == "Polygon" && !getTempPolygonVertices().empty()) {
                Vector* v = getTempPolygonVertices().back();
                lastGeoPos = QPointF(v->x, v->y);
            }
            if (m_shapesFeature) {
                m_shapesFeature->handleShapeDrawing(selectedShape, lastGeoPos, true);
            }
            return;
        }
        return;
    }

    // ========================================================================
    // CHECK WHAT WAS CLICKED
    // ========================================================================

    bool isShapeHit = false;
    QString hitShapeId;
    bool isEntityHit = false;
    QString hitEntityId;
    bool isTrajectoryHit = false;
    int hitWaypointIndex = -1;
    bool isOnEmptyCanvas = isClickOnEmptyCanvas(event->pos());

    // Check shape hit
    if (m_shapeEventHandler) {
        for (const auto& entry : getTempMeshes()) {
            if (entry.name.startsWith("Temp") ||
                entry.name.startsWith("Text") ||
                entry.name.startsWith("UserImage") ||
                entry.shapeType.compare("Text", Qt::CaseInsensitive) == 0 ||
                !entry.bitmapPath.isEmpty()) {
                bool isHit = false;
                if (entry.name.startsWith("Text") && !entry.text.isEmpty()) {
                    QPointF entityPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                    QFontMetrics fm(entry.textFont);
                    QRect textRect = fm.boundingRect(entry.text);
                    textRect.moveTo(entityPos.x(), entityPos.y() - fm.ascent());
                    QRect innerRect = textRect.adjusted(
                        textRect.width() * 0.125f, textRect.height() * 0.125f,
                        -textRect.width() * 0.125f, -textRect.height() * 0.125f
                        );
                    isHit = innerRect.contains(event->pos());
                } else {
                    QPolygonF rotatedPoly = m_shapeEventHandler->getRotatedShapePolygon(entry);
                    if (rotatedPoly.isEmpty()) continue;
                    if (entry.name.startsWith("TempPoint")) {
                        QPointF centerCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                        float distance = QVector2D(event->pos() - centerCanvas).length();
                        isHit = (distance <= 6.0f);
                    } else if (entry.name.startsWith("TempPolyline")) {
                        qreal lineTolerance = 6.0f;
                        for (int i = 0; i < rotatedPoly.size() - 1; ++i) {
                            if (isPointNearLineSegment(event->pos(), rotatedPoly[i], rotatedPoly[i+1], lineTolerance)) {
                                isHit = true;
                                break;
                            }
                        }
                    } else {
                        QPointF center(0, 0);
                        for (const QPointF& p : rotatedPoly) center += p;
                        center /= rotatedPoly.size();
                        QPolygonF scaledPoly;
                        for (const QPointF& p : rotatedPoly) {
                            QPointF offset = p - center;
                            scaledPoly << (center + offset * 1.0f);
                        }
                        isHit = scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill);
                    }
                }
                if (isHit) {
                    isShapeHit = true;
                    hitShapeId = entry.name;
                    break;
                }
            }
        }
    }

    bool isEditingTrajectory = (currentMode == TransformMode::DrawTrajectory && isDrawingTrajectory);
    if (!isEditingTrajectory) {
        for (auto& [id, entry] : Meshes) {
            if (!entry.coreTransform) continue;
            QPointF entityPos = gislib->geoToCanvas(
                entry.coreTransform->getLatitude(),
                entry.coreTransform->getLongitude()
                );
            if (QVector2D(event->pos() - entityPos).length() < 20.0f) {
                isEntityHit = true;
                hitEntityId = QString::fromStdString(id);
                selectedEntityId = id;
                break;
            }
        }
    }
    // Check trajectory hit
    if (isEditingTrajectory) {
        hitWaypointIndex = findNearestWaypoint(event->pos());
        isTrajectoryHit = true;   // true for the whole edit session, waypoint hit optional
        if (hitWaypointIndex >= 0) {
            selectWaypoint(hitWaypointIndex);
        }
    }
    // ========================================================================
    // DELEGATE TO CONTEXT MENU HANDLER OR SHAPE SYSTEM
    // ========================================================================
    if (isShapeHit) {
        if (m_shapeEventHandler) {
            m_shapeEventHandler->handleShapeRightClick(event);
        }
        return;
    }
    if (m_contextMenuHandler) {
        m_contextMenuHandler->showContextMenu(
            event->globalPos(),
            event->pos(),
            isShapeHit,
            hitShapeId,
            isEntityHit,
            hitEntityId,
            isTrajectoryHit,
            hitWaypointIndex,
            isOnEmptyCanvas,
            currentMode,
            selectedShape
            );
    }
    Refresh();
}
void CanvasWidget::handleTrajectoryRightClick(QMouseEvent * /*event*/) {
    // This is now handled by ContextMenuHandler

    // Keep for compatibility if needed
}

// ============================================================================
// Paint Event
// ============================================================================

void CanvasWidget::handlePaint(QPaintEvent * /*event*/) {
    if (!gislib) return;
    QElapsedTimer timer;
    timer.start();
    if (!selectedEntityId.empty()) {
        auto it = Meshes.find(selectedEntityId);
        bool valid = (it != Meshes.end()) &&
                     (it->second.entity != nullptr) &&
                     (it->second.entity->Active) /*&&
                     (it->second.coreTransform != nullptr)*/ /*&&
                     (it->second.coreTransform->geocord != nullptr)*/;
        if (valid) {
            entityInfoDialog->updateEntityInfo();
        } else {
            hideEntityInfo();
            selectedEntityId.clear();
        }
    }
    QPainter painter(this);

    std::vector<std::string> staleIds;
    for (auto& [id, entry] : Meshes) {
        if (!entry.entity || !entry.coreTransform /*|| !entry.coreTransform->geocord*/) {
            staleIds.push_back(id);
        }
    }
    for (const auto& sid : staleIds) {
        Meshes.erase(sid);
    }

    for (auto& [id, entry] : Meshes) {
        if (!entry.entity || !entry.entity->Active || !entry.coreTransform /*|| !entry.coreTransform->geocord*/) continue;
        drawTrajectory(painter, id, entry);
        drawTrail(painter, id, entry);
        drawRadar(painter, id, entry);
        drawRadio(painter, id, entry);
        drawImage(painter, id, entry);
        drawCollision(painter, id, entry);
        drawCollider(painter, id, entry);
        drawSonobuoyHighlight(painter);
        drawSelectionOutline(painter);
    }

    drawMesh(painter);
    drawSelectionOutline(painter);
    gislib->renderAirbases(painter);

    if (m_shapeEventHandler) {
        m_shapeEventHandler->paintEvent(painter);
        m_shapeEventHandler->handleBitmapsPaint(painter);
    }

    if (m_shapesFeature && m_shapesFeature->isDraggingShape()) {
        m_shapesFeature->drawDragPreview(painter, gislib);
    }


    if (isDrawingTrajectory && !currentTrajectory.empty()) {
        painter.save();
        QColor drawColor = Qt::magenta;
        painter.setPen(QPen(drawColor, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;

        for (size_t i = 0; i < currentTrajectory.size(); ++i) {
            Waypoints* wp = currentTrajectory[i];
            if (!wp || !wp->position) continue;
            QPointF screenPos = gislib->geoToCanvas(wp->position->x, wp->position->z);
            polyline << screenPos;
            painter.setBrush(drawColor);
            int ps = ((int)i == selectedWaypointIndex) ? 6 : 4;
            painter.setPen(((int)i == selectedWaypointIndex) ? QPen(Qt::yellow, 2) : QPen(drawColor, 1));
            painter.drawEllipse(screenPos, ps, ps);
            painter.setBrush(Qt::NoBrush);
        }

        if (polyline.size() > 1) {
            painter.setPen(QPen(drawColor, 2, Qt::DashLine));
            painter.drawPolyline(polyline);

            float arrowSize = 7.0f;
            painter.setBrush(drawColor);
            painter.setPen(QPen(drawColor, 1, Qt::SolidLine));
            for (int i = 0; i < polyline.size() - 1; ++i) {
                QPointF p1 = polyline[i];
                QPointF p2 = polyline[i + 1];
                QPointF mid = (p1 + p2) / 2.0f;
                float angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
                QPointF tip = mid + QPointF(std::cos(angle) * arrowSize, std::sin(angle) * arrowSize);
                QPointF left = mid + QPointF(
                                   std::cos(angle + M_PI * 0.75f) * arrowSize * 0.6f,
                                   std::sin(angle + M_PI * 0.75f) * arrowSize * 0.6f
                                   );
                QPointF right = mid + QPointF(
                                    std::cos(angle - M_PI * 0.75f) * arrowSize * 0.6f,
                                    std::sin(angle - M_PI * 0.75f) * arrowSize * 0.6f
                                    );
                QPolygonF arr;
                arr << tip << left << right;
                painter.drawPolygon(arr);
            }
        }
        painter.restore();
    }

    drawSceneInformation(painter);
    drawEntityInformation(painter);
    drawTransformGizmo(painter);

    if (isBoxZooming) {
        painter.save();
        QRect zoomBox = QRect(boxZoomStart, boxZoomCurrent).normalized();
        painter.fillRect(zoomBox, QColor(0, 120, 215, 30));
        painter.setPen(QPen(QColor(0, 120, 215, 200), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(zoomBox);
        int hs = 6;
        painter.setBrush(QColor(0, 120, 215, 200));
        auto drawHandle = [&](QPoint p) {
            painter.drawRect(p.x() - hs/2, p.y() - hs/2, hs, hs);
        };
        drawHandle(zoomBox.topLeft());
        drawHandle(zoomBox.topRight());
        drawHandle(zoomBox.bottomLeft());
        drawHandle(zoomBox.bottomRight());
        painter.restore();
    }

    frameCount++;
    if (Profiler::currentFrame)
        Profiler::currentFrame->canvasTime = timer.elapsed();
}

// ============================================================================
// Qt Event Overrides
// ============================================================================

void CanvasWidget::keyPressEvent(QKeyEvent *event) {
    handleKeyPress(event);
}

void CanvasWidget::paintEvent(QPaintEvent *event) {
    handlePaint(event);
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    handleMousePress(event);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    handleMouseMove(event);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    handleMouseRelease(event);
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_shapeEventHandler) {
        m_shapeEventHandler->mouseDoubleClickEvent(event);
    }
}

// ============================================================================
// Drawing Methods - Grid
// ============================================================================

void CanvasWidget::drawGridLines(QPainter& painter) {
    int alpha = (gridOpacity * 255) / 100;
    if (showXGrid) {
        painter.setPen(QColor(128, 128, 128, alpha));
        for (int x = 0; x < width(); x += 20)
            painter.drawLine(x, 0, x, height());
    }
    if (showYGrid) {
        painter.setPen(QColor(128, 128, 128, alpha));
        for (int y = 0; y < height(); y += 20)
            painter.drawLine(0, y, width(), y);
    }
}

// ============================================================================
// Drawing Methods - Scene Information
// ============================================================================

void CanvasWidget::drawSceneInformation(QPainter& painter) {
    if (!showInformation) {
        return;
    }
    painter.save();
    painter.resetTransform();
    const QFont infoFont("Arial", 10, QFont::Bold);
    painter.setFont(infoFont);
    painter.setPen(QColor(5, 5, 5, 200));
    const QPointF startPos(width() - 110, 35);
    int yOffset = 0;

    const auto currentFrame = Profiler::currentFrame;
    const QString modeText = QString(" %1").arg(simulate ? "Simulation" : "Editor");
    painter.drawText(QPointF(width() - 100, 20), modeText);

    struct PerfMetric {
        const char* label;
        int timeValue;
    };
    const PerfMetric metrics[] = {
        {"Exc Time", currentFrame->excutionTime},
        {"Can Time", currentFrame->canvasTime},
        {"Phy Time", currentFrame->physicsTime},
        {"Dym Time", currentFrame->dynamicTime},
        {"Sen Time", currentFrame->SensorTime},
        {"Rdr Time", currentFrame->RadarTime},
        {"EW Time",  currentFrame->EWTime},
        {"CSM Time", currentFrame->CSMTime},
        {"ESM Time", currentFrame->ESMTime},
        {"IFF Time", currentFrame->IFFTime},
        {"Rdo Time", currentFrame->RadioTime},
        {"CSM UI", currentFrame->CSMTime},
        {"ESM UI", currentFrame->ESMTime},
        {"GUI Time", currentFrame->GUITime}
    };

    for (const auto& metric : metrics) {
        QString text = QString("%1 %2ms")
        .arg(metric.label)
            .arg(metric.timeValue);
        painter.drawText(startPos + QPointF(0, yOffset), text);
        yOffset += 15;
    }
    if (showFPS) {
        QString fpsText = QString("FPS: %1").arg(fps*10);
        painter.drawText(QPointF(10, 20), fpsText);
    }
    painter.restore();
}

// ============================================================================
// Drawing Methods - Entity Information
// ============================================================================

void CanvasWidget::drawEntityInformation(QPainter& painter) {
    if (!showInformation || simulate || selectedEntityId.empty()) return;
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) return;

    painter.save();
    painter.resetTransform();

    QString name = it->second.name;
    auto& pos = it->second.position;
    QString text;

    if (currentMode == TransformMode::Translate || currentMode == TransformMode::DrawTrajectory) {
        text = QString("pos: lon %1, lat %2")
        .arg(pos->x(), 0, 'f', 4)
            .arg(pos->y(), 0, 'f', 4);
    } else if (currentMode == TransformMode::Rotate) {
        auto& rot = Meshes[selectedEntityId].rotation;
        text = QString("rot: x %1, y %2, z %3")
                   .arg(rot->toEulerAngles().x())
                   .arg(rot->toEulerAngles().y())
                   .arg(rot->toEulerAngles().z());
    } else if (currentMode == TransformMode::Scale) {
        auto& size = Meshes[selectedEntityId].size;
        text = QString("size: x %1, y %2, z %3")
                   .arg(size->x())
                   .arg(size->y())
                   .arg(size->z());
    }
    if (currentMode == TransformMode::DrawTrajectory) {
        text += QString(" | Waypoints: %1").arg(currentTrajectory.size());
    }
    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(255, 255, 255, 200));
    painter.drawText(QPointF(10, height() - 50), name);
    painter.drawText(QPointF(10, height() - 30), text);
    painter.drawText(QPointF(10, height() - 10), QString("Mode: %1")
                                                     .arg(currentMode == TransformMode::Translate ? "Translate" :
                                                              currentMode == TransformMode::Rotate ? "Rotate" :
                                                              currentMode == TransformMode::Scale ? "Scale" : "Draw Trajectory"));
    painter.restore();
}

// ============================================================================
// Drawing Methods - Transform Gizmo
// ============================================================================

void CanvasWidget::drawTransformGizmo(QPainter& painter) {
    if(!gislib) return;
    if (simulate || currentMode == TransformMode::DrawTrajectory || selectedEntityId.empty()) return;
    auto it = Meshes.find(selectedEntityId);
    if (it == Meshes.end()) return;
    auto& entry = it->second;
    if (!entry.coreTransform /*|| !entry.coreTransform->geocord*/) return;
    QPointF base = gislib->geoToCanvas(
        entry.coreTransform->getLatitude(),
        entry.coreTransform->getLongitude());
    QPointF xAxisHandlePos = base + QPointF(50, 0);
    QPointF yAxisHandlePos = base + QPointF(0, -50);
    if (currentMode == TransformMode::Translate) {
        painter.setPen(QPen(Qt::red, (activeDragAxis == "x") ? 4 : 2));
        painter.drawLine(base, xAxisHandlePos);
        painter.setPen(QPen(Qt::blue, (activeDragAxis == "y") ? 4 : 2));
        painter.drawLine(base, yAxisHandlePos);
    } else if (currentMode == TransformMode::Rotate) {
        painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(base, 40, 40);
        float radius = 40;
        float rad = 0.0f;
        if (entry.coreTransform) {
            rad = qDegreesToRadians(entry.coreTransform->rotation().toEulerAngles().y() + 90);
        }
        QPointF endpoint(base.x() + radius * sin(rad), base.y() - radius * cos(rad));
        painter.drawLine(base, endpoint);
    } else if (currentMode == TransformMode::Scale) {
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::red);
        painter.drawRect(QRectF(base + QPointF(45, -3), QSizeF(10, 6)));
        painter.setBrush(Qt::green);
        painter.drawRect(QRectF(base + QPointF(-3, 45), QSizeF(6, 10)));
    }
}

// ============================================================================
// Drawing Methods - Entity Components (Stubs - Full implementations exist)
// ============================================================================

void CanvasWidget::drawTrajectory(QPainter& painter, const std::string& id, MeshEntry &entry) {
    if (!showTrajectories) return;
    if (!entry.trajectory) {
        if(entry.platform /*&& entry.platform->trajectory*/){
            entry.trajectory = &entry.platform->trajectory;
            if(!entry.trajectory->Active || entry.trajectory->Trajectories.empty()){
                return;
            }
        }else{
            return;
        }
    }else{
        if(!entry.trajectory->Active || entry.trajectory->Trajectories.empty()){
            return;
        }
    }
    double lat = entry.coreTransform->getLatitude();
    double lon = entry.coreTransform->getLongitude();
    bool show = true;
    if(entry.platform /*&& entry.platform->dynamicModel*/){
        show = entry.platform->dynamicModel.followEntity==nullptr;
    }
    int current = entry.trajectory->current;
    QPointF point = gislib->geoToCanvas(lat,lon);
    painter.save();
    bool isSelected = (id == selectedEntityId) ||
                      (std::find(selectedEntityIds.begin(), selectedEntityIds.end(), id) != selectedEntityIds.end());
    QColor trajectoryColor = isSelected
                                 ? QColor(0, 128, 128)
                                 : entry.trajectoryColor;
    QColor pointColor = trajectoryColor;

    QPen trajectoryPen(trajectoryColor, 2, Qt::DashLine);
    painter.setPen(trajectoryPen);
    painter.setBrush(Qt::NoBrush);
    entry.polyline.clear();
    entry.pointsToDraw.clear();
    if(entry.polyline.isEmpty())
    {
        for (const Waypoints* waypoint : entry.trajectory->Trajectories) {
            if (!waypoint || !waypoint->position) {
                continue;
            }
            QPointF point = gislib->geoToCanvas(waypoint->position->x, waypoint->position->z);
            entry.polyline << point;
            entry.pointsToDraw.append(point);
        }
    }
    if (entry.polyline.size() > 1) {
        QPen polyPen(trajectoryColor, 2, Qt::DashLine);
        painter.setPen(polyPen);
        if(current==0 && show){
            float lat = entry.coreTransform->getLatitude();
            lat = lat>180?180:(lat<-180?-180:lat);
            float lon = entry.coreTransform->getLongitude();
            lon = lon>180?180:(lon<-180?-180:lon);

            QPointF startPoint = gislib->geoToCanvas(lat, lon);
            painter.drawLine(startPoint, entry.polyline[0]);
        }

        painter.drawPolyline(entry.polyline);
        painter.setBrush(trajectoryColor);
        QPen arrowPen(trajectoryColor, 2, Qt::SolidLine);
        painter.setPen(arrowPen);
        float arrowSize = 7.0f;
        // entry.polyline directly use karo - entity pos ADD MAT KARO
        for (int i = 0; i < entry.polyline.size() - 1; ++i) {
            QPointF p1 = entry.polyline[i];
            QPointF p2 = entry.polyline[i + 1];
            QPointF mid = (p1 + p2) / 2.0f;
            float dx = p2.x() - p1.x();
            float dy = p2.y() - p1.y();
            float angle = std::atan2(dy, dx);

            QPointF tip = mid + QPointF(std::cos(angle) * arrowSize, std::sin(angle) * arrowSize);
            QPointF left = mid + QPointF(
                               std::cos(angle + M_PI * 0.75f) * arrowSize * 0.6f,
                               std::sin(angle + M_PI * 0.75f) * arrowSize * 0.6f
                               );
            QPointF right = mid + QPointF(
                                std::cos(angle - M_PI * 0.75f) * arrowSize * 0.6f,
                                std::sin(angle - M_PI * 0.75f) * arrowSize * 0.6f
                                );
            QPolygonF arrowHead;
            arrowHead << tip << left << right;
            painter.drawPolygon(arrowHead);
        }
    }
    if (!entry.pointsToDraw.empty()) {
        // QColor pointColor = isSelected ? QColor(0, 128, 128) : Qt::blue;
        const QPen peen(pointColor, 5);
        painter.setPen(peen);
        painter.drawPoints(entry.pointsToDraw);
    }
    if(isSelected){
        int wpnum = 0;
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        float distance = 0;
        QVector3D ini(0,0,0);
        bool st = false;

        for (const Waypoints* waypoint : entry.trajectory->Trajectories) {
            if (!waypoint || !waypoint->position) {
                wpnum++;
                continue;
            }
            QPointF point = gislib->geoToCanvas(waypoint->position->x, waypoint->position->z);
            FlatXYZ xyz = geoToFlatXYZ(waypoint->position->x,waypoint->position->z,0);
            QVector3D ne(xyz.x,xyz.y,xyz.z);
            if(!st){
                st = true;
                ini.setX(ne.x());
                ini.setY(ne.y());
                ini.setZ(ne.z());
            }
            distance +=ini.distanceToPoint(ne);
            ini.setX(ne.x());
            ini.setY(ne.y());
            ini.setZ(ne.z());

            painter.drawText(point + QPointF(7, -7),
                             QString("WP%1 %2km").arg(wpnum + 1).arg(QString::number(distance, 'f', 0)));
            wpnum++;
        }
    }

    QString entityIdStr = QString::fromStdString(id);
    if (m_highlightedWaypointIndex >= 0 &&
        m_highlightedWaypointEntityId == entityIdStr &&
        m_highlightedWaypointIndex < (int)entry.trajectory->Trajectories.size())
    {
        Waypoints* wp = entry.trajectory->Trajectories[m_highlightedWaypointIndex];
        if (wp && wp->position) {
            QPointF wpCanvas = gislib->geoToCanvas(wp->position->x, wp->position->z);
            painter.save();
            painter.setPen(QPen(QColor(255, 255, 0, 180), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(wpCanvas, 14, 14);
            painter.setPen(QPen(Qt::yellow, 2));
            painter.setBrush(QColor(255, 255, 0, 120));
            painter.drawEllipse(wpCanvas, 8, 8);
            painter.restore();
        }
    }
    painter.restore();
}

void CanvasWidget::drawTrail(QPainter& painter, std::string id, MeshEntry entry) {
    // ↓↓↓ Only show trail for selected entity ↓↓↓
    if (id != selectedEntityId) return;
    if(entry.coreTransform->trailData.capacity() > 2) {
        QVector3D v = entry.coreTransform->trailData.at(0);
        QPointF lastv = gislib->geoToCanvas(v.x(), v.z());
        painter.setPen(QPen(Qt::magenta, 2));
        for (const auto& position : entry.coreTransform->trailData) {
            QPointF point = gislib->geoToCanvas(position.x(), position.z());
            painter.drawLine(lastv.x(), lastv.y(), point.x(), point.y());
            lastv.setX(point.x());
            lastv.setY(point.y());
        }
    }
}

void CanvasWidget::drawRadar(QPainter& painter, std::string id, MeshEntry entry) {
    Platform* entity = entry.platform;
    if (!entry.detection) return;
    if (!entity) return;
    // if (!entity->sensors) return;
    for (auto const& pair : entity->sensors.sensors) {
        Sensor* s = pair.second;
        if (s->Active && (s->subType == Sensor::SubType::Generic || s->subType == Sensor::SubType::AESA|| s->subType == Sensor::SubType::EO || s->subType == Sensor::SubType::IR)) {
            // Get entity position in GIS coordinates
            float lat = entry.coreTransform->getLatitude();
            float lon = entry.coreTransform->getLongitude();
            // Convert entity position to canvas coordinates
            QPointF centerPoint = gislib->geoToCanvas(lat, lon);
            float centerX = centerPoint.x();
            float centerY = centerPoint.y();
            float platformHeading = entry.transform->rotation().toEulerAngles().y();
            float minAz = s->minAzimuth;
            float maxAz = s->maxAzimuth;
            float worldLeftEdge  = platformHeading + minAz;
            float worldRightEdge = platformHeading + maxAz;
            float sectorSpan     = maxAz - minAz;
            auto [latAtRadius, lonAtRadius] = calculateNewLatLong(
                lat, lon,
                worldLeftEdge,
                s->range
                );
            QPointF radiusPoint = gislib->geoToCanvas(latAtRadius, lonAtRadius);
            float pixelRadius = std::sqrt(
                std::pow(radiusPoint.x() - centerX, 2) +
                std::pow(radiusPoint.y() - centerY, 2)
                );
            float startAngle = 90.0f - worldRightEdge;
            float sweepAngle = sectorSpan;

            painter.setPen(QPen(Qt::black, 1));
            QRectF boundingRect(
                centerX - pixelRadius,
                centerY - pixelRadius,
                pixelRadius * 2,
                pixelRadius * 2
                );
            painter.drawPie(boundingRect,
                            static_cast<int>(startAngle * 16),
                            static_cast<int>(sweepAngle * 16));

        }
    }
}

void CanvasWidget::drawRadio(QPainter& painter, std::string id, MeshEntry entry) {
    Platform* entity = entry.platform;
    if(!entry.radioVisible) return;
    if (!entity) return;
    // if (!entity->radios) return;

    for (auto const& pair : entity->radios.radios) {
        Radio* s = pair.second;
        QPointF point = gislib->geoToCanvas(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude());

        float centerX = point.x();
        float centerY = point.y();
        QString  msg = QString::fromStdString(s->msg);
        if(!msg.isEmpty()){
            QFont font("Arial", 10, QFont::Bold);
            painter.setFont(font);
            painter.setPen(QColor(0, 255, 0, 255));
            // Fixed screen coordinates ka upyog karein
            painter.drawText(QPointF(centerX, centerY),msg);
        }

        QPen pen(Qt::black, 1);
        QVector<qreal> dashes;
        dashes << 4 << 10;
        pen.setDashPattern(dashes);
        painter.setPen(pen);

        for (int i = 0; i < s->targets.size(); ++i) {
            try {
                if(s->targets[i].entity /*&& s->targets[i].entity->transform*/){
                    QPointF points2 = gislib->geoToCanvas(s->targets[i].entity->transform.getLatitude(), s->targets[i].entity->transform.getLongitude());
                    painter.drawLine(centerX, centerY, points2.x(), points2.y());
                }
            }catch(...){

            }
        }
    }
}

void CanvasWidget::drawImage(QPainter& painter, std::string id, MeshEntry entry) {
    if (!showImage) return;
    if (!entry.entity || !entry.entity->Active) return;
    if (!entry.coreTransform) return;
    // if (!entry.coreTransform->geocord) return;
    if (!cacheimg || cacheimg->isNull()) return;

    double lat = entry.coreTransform->geocord.latitude;
    double lon = entry.coreTransform->geocord.longitude;

    QPointF point = gislib->geoToCanvas(lat, lon);
    float angle = 0.0f;
    if (entry.transform) {
        angle = entry.transform->rotation().toEulerAngles().y();
    } else if (entry.coreTransform) {
        angle = entry.coreTransform->geocord.Heading;
    }

    int finalScale = (entry.individualImageSize > 0) ? entry.individualImageSize : ImageScale;

    QPixmap pixToUse;
    if (entry.mesh) {
        pixToUse = entry.mesh->getPixmap(finalScale);
    }
    painter.save();
    painter.translate(point.x(), point.y());
    painter.rotate(angle);
    if (!pixToUse || pixToUse.isNull()) {
        painter.drawPixmap(QPointF(-scaledimg.width() / 2.0f, -scaledimg.height() / 2.0f), scaledimg);
    }else{
        painter.drawPixmap(QPointF(-pixToUse.width() / 2.0f, -pixToUse.height() / 2.0f), pixToUse);
    }
    painter.restore();
}

void CanvasWidget::drawCollision(QPainter& painter, std::string id, MeshEntry entry) {
    return;
    if (!entry.entity) return;
    if (entry.entity->collisionWarning){
        QPointF point = gislib->geoToCanvas(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude());

        float x = point.x();
        float y = point.y();
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(QBrush(QColor(255, 0, 0, 40)));
        QRectF ring(x-25, y-25, 50, 50);
        painter.drawEllipse(ring);
    }
}

void CanvasWidget::drawCollider(QPainter& painter, std::string id, MeshEntry entry) {
    if (!showColliders) return;
    // for (const auto& [id, entry] : Meshes) {
    if (!entry.collider) return;
    float rad = entry.collider->CollideRadius*2;
    QPointF point = gislib->geoToCanvas(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude());
    auto [newLat, newLon] = calculateNewLatLong(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude(), 90,rad/1000);
    QPointF points = gislib->geoToCanvas(newLat, newLon);
    float distance = QVector2D(point).distanceToPoint(QVector2D(points));
    float x = point.x();
    float y = point.y();
    painter.setPen(QPen(QColor(0, 55, 0, 100), 2));
    painter.setBrush(QBrush(QColor(0, 255, 0, 40)));
    QRectF ring(x-(distance/2), y-(distance/2), distance,distance);
    painter.drawEllipse(ring);
    painter.setBrush(QBrush(QColor(0, 255, 0, 0)));
    if(entry.collider->blinkRadius>0){
        painter.setPen(QPen(QColor(200, 0, 0, 200), 2));
        painter.setBrush(QBrush(QColor(200, 0, 0, 200)));
        QRectF ring(x-(entry.collider->blinkRadius/2), y-(entry.collider->blinkRadius/2), entry.collider->blinkRadius,entry.collider->blinkRadius);
        painter.drawEllipse(ring);
    }
}

void CanvasWidget::drawSelectionOutline(QPainter& painter) {
    // Full implementation exists
}

void CanvasWidget::drawMesh(QPainter& painter) {
    // Stays empty as shape drawing was moved to ShapeEventHandler
}

void CanvasWidget::drawFormation(QPainter& painter, std::string /*id*/, MeshEntry entry) {
    if (!showImage) return;
    if(!entry.platform)return;
    DynamicModel& dynamic = entry.platform->dynamicModel;
    if (/*dynamic && */dynamic.formation) {
        QPointF point = gislib->geoToCanvas(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude());
        float x = point.x();
        float y = point.y();
        float angle = entry.transform->rotation().toEulerAngles().y();
        QPen pen(Qt::blue);      // Border ka color
        pen.setWidth(4);         // Border ki motai (thickness)
        painter.setPen(pen);
        int radius = 5;
        // Kyunki circle hai, x-radius aur y-radius dono same rahenge
        painter.drawEllipse(QPoint(x, y), radius, radius);

        for (const auto& pair : *dynamic.formation->formationPositions) {
            Vector offset = *pair.second->Offset;
            QVector3D real = entry.coreTransform->transformPoint(QVector3D(offset.x*30.f,offset.y,offset.z*30.f));
            GeoPos cord = flatXYZToGeo(real.x(),real.y(),real.z());
            QPointF point = gislib->geoToCanvas(cord.lat, cord.lon);
            // painter.drawEllipse(QPoint(x+(offset.x*30.f), y+(offset.z*30.f)), radius, radius);
            painter.drawEllipse(QPoint(point.x(), point.y()), radius, radius);


        }
    }
}

// ============================================================================
// Trajectory Helper Methods
// ============================================================================

void CanvasWidget::deselectWaypoint() {
    selectedWaypointIndex = -1;
    isDraggingWaypoint = false;
    Refresh();
}

int CanvasWidget::findNearestWaypoint(QPointF canvasPos) {
    const float tolerance = 10.0f;
    int nearestIndex = -1;
    float minDistance = std::numeric_limits<float>::max();
    for (size_t i = 0; i < currentTrajectory.size(); ++i) {
        Waypoints* wp = currentTrajectory[i];
        QPointF wpCanvas = gislib->geoToCanvas(wp->position->x, wp->position->z);
        float distance = QVector2D(canvasPos - wpCanvas).length();
        if (distance < tolerance && distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    return nearestIndex;
}

void CanvasWidget::updateTrajectoryData() {
    if (selectedEntityId.empty()) return;
    QJsonArray waypointsArray;
    for (const Waypoints* wp : currentTrajectory) {
        waypointsArray.append(wp->toJson());
    }
    emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
}

// ============================================================================
// Serialization
// ============================================================================

QJsonObject CanvasWidget::toJson() const {
    QJsonObject json;
    json["selectedBitmapType"] = getSelectedBitmapType();
    json["isPlacingBitmap"] = isPlacingBitmap();
    if (m_shapeEventHandler) {
        QJsonArray polygonVerticesArray;
        for (const Vector* vertex : getTempPolygonVertices()) {
            QJsonObject vObj;
            vObj["x"] = vertex->x;
            vObj["y"] = vertex->y;
            vObj["z"] = vertex->z;
            polygonVerticesArray.append(vObj);
        }
        json["tempPolygonVertices"] = polygonVerticesArray;
        QJsonArray polygonCanvasPointsArray;
        for (const QPointF& point : getTempPolygonCanvasPoints()) {
            QJsonObject pObj;
            pObj["x"] = point.x();
            pObj["y"] = point.y();
            polygonCanvasPointsArray.append(pObj);
        }
        json["tempPolygonCanvasPoints"] = polygonCanvasPointsArray;
        QJsonArray lineVerticesArray;
        for (const Vector* vertex : getTempLineVertices()) {
            QJsonObject vObj;
            vObj["x"] = vertex->x;
            vObj["y"] = vertex->y;
            vObj["z"] = vertex->z;
            lineVerticesArray.append(vObj);
        }
        json["tempLineVertices"] = lineVerticesArray;
        QJsonArray lineCanvasPointsArray;
        for (const QPointF& point : getTempLineCanvasPoints()) {
            QJsonObject pObj;
            pObj["x"] = point.x();
            pObj["y"] = point.y();
            lineCanvasPointsArray.append(pObj);
        }
        json["tempLineCanvasPoints"] = lineCanvasPointsArray;
        QJsonArray tempMeshesArray;
        for (const MeshEntry& entry : getTempMeshes()) {
            QJsonObject meshObj;
            meshObj["name"] = entry.name;
            meshObj["text"] = entry.text;
            if (entry.name.startsWith("Text")) {
                QJsonObject textColorObj;
                textColorObj["r"] = entry.textColor.red();
                textColorObj["g"] = entry.textColor.green();
                textColorObj["b"] = entry.textColor.blue();
                textColorObj["a"] = entry.textColor.alpha();
                meshObj["textColor"] = textColorObj;

                QJsonObject fontObj;
                fontObj["family"] = entry.textFont.family();
                fontObj["size"] = entry.textSize;
                fontObj["bold"] = entry.textFont.bold();
                fontObj["italic"] = entry.textFont.italic();
                meshObj["textFont"] = fontObj;
            }

            QJsonObject posObj;
            posObj["x"] = entry.position->x();
            posObj["y"] = entry.position->y();
            posObj["z"] = entry.position->z();
            meshObj["position"] = posObj;

            QJsonObject rotObj;
            rotObj["z_deg"] = entry.rotation->z() * (180.0 / M_PI);
            meshObj["rotation"] = rotObj;

            QJsonObject sizeObj;
            sizeObj["x"] = entry.size->x();
            sizeObj["y"] = entry.size->y();
            sizeObj["z"] = entry.size->z();
            meshObj["size"] = sizeObj;

            QJsonObject velObj;
            velObj["x"] = entry.velocity->x();
            velObj["y"] = entry.velocity->y();
            velObj["z"] = entry.velocity->z();
            meshObj["velocity"] = velObj;

            if (!entry.bitmapPath.isEmpty()) {
                meshObj["bitmapPath"] = entry.bitmapPath;
            }

            if (entry.mesh) {
                QJsonObject meshData;
                meshData["lineWidth"] = entry.mesh->lineWidth;
                meshData["closePath"] = entry.mesh->closePath;
                if (entry.mesh->color) {
                    QJsonObject colorObj;
                    colorObj["r"] = entry.mesh->color->red();
                    colorObj["g"] = entry.mesh->color->green();
                    colorObj["b"] = entry.mesh->color->blue();
                    colorObj["a"] = entry.mesh->color->alpha();
                    meshData["color"] = colorObj;
                }
                QJsonArray polygenArray;
                for (const Vector* point : entry.mesh->polygen) {
                    QJsonObject pointObj;
                    pointObj["x"] = point->x;
                    pointObj["y"] = point->y;
                    pointObj["z"] = point->z;
                    polygenArray.append(pointObj);
                }
                meshData["polygen"] = polygenArray;
                meshObj["mesh"] = meshData;
            }
            tempMeshesArray.append(meshObj);
        }
        json["tempMeshes"] = tempMeshesArray;
    }

    QJsonObject meshesColorData;
    for (const auto& [id, entry] : Meshes) {
        QJsonObject colorObj;
        colorObj["r"] = entry.trajectoryColor.red();
        colorObj["g"] = entry.trajectoryColor.green();
        colorObj["b"] = entry.trajectoryColor.blue();
        colorObj["a"] = entry.trajectoryColor.alpha();
        meshesColorData[QString::fromStdString(id)] = colorObj;
    }
    json["trajectoryColors"] = meshesColorData;

    if (m_layerPanel) {
        QJsonObject layersData = m_layerPanel->toJson();
        json["layers"] = layersData;
    }

    QJsonArray geoJsonArray;
    for (auto it = geoJsonLayerFilePaths.constBegin(); it != geoJsonLayerFilePaths.constEnd(); ++it) {
        QJsonObject layerObj;
        layerObj["name"] = it.key();
        layerObj["filePath"] = it.value();
        layerObj["visible"] = geoJsonLayers.value(it.key(), true);
        geoJsonArray.append(layerObj);
    }
    json["geoJsonImportedLayers"] = geoJsonArray;
    return json;
}

void CanvasWidget::fromJson(const QJsonObject& json) {
    for (Waypoints* wp : currentTrajectory) {
        if (wp) {
            if (wp->position) delete wp->position;
            delete wp;
        }
    }
    currentTrajectory.clear();
    geoJsonLayers.clear();
    geoJsonLayerFilePaths.clear();
    if (m_shapeEventHandler) {
        auto& polygonVerts = getTempPolygonVertices();
        for (Vector* v : polygonVerts) delete v;
        polygonVerts.clear();
        getTempPolygonCanvasPoints().clear();

        auto& lineVerts = getTempLineVertices();
        for (Vector* v : lineVerts) delete v;
        lineVerts.clear();
        getTempLineCanvasPoints().clear();

        auto& meshes = getTempMeshes();
        for (MeshEntry& entry : meshes) {
            if (entry.mesh) {
                for (Vector* v : entry.mesh->polygen) delete v;
                if (entry.mesh->color) delete entry.mesh->color;
                delete entry.mesh;
            }
            if (entry.collider) delete entry.collider;
            if (entry.trajectory) {
                for (Waypoints* wp : entry.trajectory->Trajectories) {
                    if (wp) {
                        if (wp->position) delete wp->position;
                        delete wp;
                    }
                }
                delete entry.trajectory;
            }
        }
        meshes.clear();
    }

    if (json.contains("tempPolygonVertices") && json["tempPolygonVertices"].isArray()) {
        QJsonArray verticesArray = json["tempPolygonVertices"].toArray();
        auto& verts = getTempPolygonVertices();
        for (const QJsonValue& val : verticesArray) {
            QJsonObject vObj = val.toObject();
            verts.push_back(new Vector(
                vObj["x"].toDouble(),
                vObj["y"].toDouble(),
                vObj["z"].toDouble()
                ));
        }
    }

    if (json.contains("tempPolygonCanvasPoints") && json["tempPolygonCanvasPoints"].isArray()) {
        QJsonArray pointsArray = json["tempPolygonCanvasPoints"].toArray();
        auto& points = getTempPolygonCanvasPoints();
        for (const QJsonValue& val : pointsArray) {
            QJsonObject pObj = val.toObject();
            points.push_back(QPointF(pObj["x"].toDouble(), pObj["y"].toDouble()));
        }
    }

    if (json.contains("tempLineVertices") && json["tempLineVertices"].isArray()) {
        QJsonArray verticesArray = json["tempLineVertices"].toArray();
        auto& verts = getTempLineVertices();
        for (const QJsonValue& val : verticesArray) {
            QJsonObject vObj = val.toObject();
            verts.push_back(new Vector(
                vObj["x"].toDouble(),
                vObj["y"].toDouble(),
                vObj["z"].toDouble()
                ));
        }
    }

    if (json.contains("tempLineCanvasPoints") && json["tempLineCanvasPoints"].isArray()) {
        QJsonArray pointsArray = json["tempLineCanvasPoints"].toArray();
        auto& points = getTempLineCanvasPoints();
        for (const QJsonValue& val : pointsArray) {
            QJsonObject pObj = val.toObject();
            points.push_back(QPointF(pObj["x"].toDouble(), pObj["y"].toDouble()));
        }
    }

    if (json.contains("tempMeshes") && json["tempMeshes"].isArray()) {
        QJsonArray tempMeshesArray = json["tempMeshes"].toArray();
        auto& meshes = getTempMeshes();
        for (const QJsonValue& val : tempMeshesArray) {
            QJsonObject meshObj = val.toObject();
            MeshEntry entry;

            entry.name = meshObj["name"].toString();
            entry.text = meshObj["text"].toString();

            if (entry.name.startsWith("Text")) {
                if (meshObj.contains("textColor")) {
                    QJsonObject colorObj = meshObj["textColor"].toObject();
                    entry.textColor = QColor(
                        colorObj["r"].toInt(),
                        colorObj["g"].toInt(),
                        colorObj["b"].toInt(),
                        colorObj["a"].toInt()
                        );
                } else {
                    entry.textColor = Qt::black;
                }

                if (meshObj.contains("textFont")) {
                    QJsonObject fontObj = meshObj["textFont"].toObject();
                    entry.textFont = QFont(fontObj["family"].toString());
                    entry.textSize = fontObj["size"].toInt();
                    entry.textFont.setPointSize(entry.textSize);
                    entry.textFont.setBold(fontObj["bold"].toBool());
                    entry.textFont.setItalic(fontObj["italic"].toBool());
                } else {
                    entry.textFont = QFont("Arial", 12);
                    entry.textSize = 12;
                }
                entry.isTextSelected = false;
            }

            if (meshObj.contains("position")) {
                QJsonObject posObj = meshObj["position"].toObject();
                entry.position = new QVector3D(
                    posObj["x"].toDouble(),
                    posObj["y"].toDouble(),
                    posObj["z"].toDouble()
                    );
            } else {
                entry.position = new QVector3D(0, 0, 0);
            }

            entry.rotation = new QQuaternion();
            if (meshObj.contains("rotation")) {
                QJsonObject rotObj = meshObj["rotation"].toObject();
                qreal deg = rotObj["z_deg"].toDouble();
                entry.rotation->setZ(deg * (M_PI / 180.0));
            }

            if (meshObj.contains("size")) {
                QJsonObject sizeObj = meshObj["size"].toObject();
                entry.size = new QVector3D(
                    sizeObj["x"].toDouble(),
                    sizeObj["y"].toDouble(),
                    sizeObj["z"].toDouble()
                    );
            } else {
                entry.size = new QVector3D(1, 1, 1);
            }

            if (meshObj.contains("velocity")) {
                QJsonObject velObj = meshObj["velocity"].toObject();
                entry.velocity = new QVector3D(
                    velObj["x"].toDouble(),
                    velObj["y"].toDouble(),
                    velObj["z"].toDouble()
                    );
            } else {
                entry.velocity = new QVector3D(0, 0, 0);
            }

            if (meshObj.contains("bitmapPath")) {
                entry.bitmapPath = meshObj["bitmapPath"].toString();
            }

            if (meshObj.contains("mesh")) {
                QJsonObject meshData = meshObj["mesh"].toObject();
                entry.mesh = new Mesh();
                entry.mesh->lineWidth = meshData["lineWidth"].toInt();
                entry.mesh->closePath = meshData["closePath"].toBool();
                if (meshData.contains("color")) {
                    QJsonObject colorObj = meshData["color"].toObject();
                    entry.mesh->color = new QColor(
                        colorObj["r"].toInt(),
                        colorObj["g"].toInt(),
                        colorObj["b"].toInt(),
                        colorObj["a"].toInt()
                        );
                } else {
                    entry.mesh->color = new QColor(Qt::red);
                }
                if (meshData.contains("polygen")) {
                    QJsonArray polygenArray = meshData["polygen"].toArray();
                    for (const QJsonValue& pointVal : polygenArray) {
                        QJsonObject pointObj = pointVal.toObject();
                        entry.mesh->polygen.push_back(new Vector(
                            pointObj["x"].toDouble(),
                            pointObj["y"].toDouble(),
                            pointObj["z"].toDouble()
                            ));
                    }
                }
            } else {
                entry.mesh = nullptr;
            }
            entry.collider = nullptr;
            entry.trajectory = nullptr;

            meshes.push_back(entry);
        }
    }

    if (json.contains("layers") && m_layerPanel) {
        QJsonObject layersData = json["layers"].toObject();
        m_layerPanel->fromJson(layersData);
    }

    if (json.contains("geoJsonImportedLayers") && gislib) {
        QJsonArray geoJsonArray = json["geoJsonImportedLayers"].toArray();
        for (const QJsonValue& val : geoJsonArray) {
            QJsonObject layerObj = val.toObject();
            QString layerName = layerObj["name"].toString();
            QString filePath = layerObj["filePath"].toString();
            bool visible = layerObj["visible"].toBool(true);

            if (filePath.isEmpty() || !QFile::exists(filePath)) {
                continue;
            }
            // Fix: Do not call gislib->importGeoJsonLayer(filePath) here.
            // In the decoupled architecture, imported layers are fully managed by
            // the canvas via m_tempMeshes. Loading them into the QGIS map here
            // causes the shapes to render twice (once on canvas, once on map).
            geoJsonLayers[layerName] = visible;
            geoJsonLayerFilePaths[layerName] = filePath;
        }
    }

    if (m_shapesFeature) {
        m_shapesFeature->syncCountersFromLoadedShapes(getTempMeshes());
    }

    if (m_shapeEventHandler) {
        m_shapeEventHandler->syncCountersFromLoadedShapes(getTempMeshes());
    }
    m_importCounter = 0;
    for (const MeshEntry& e : getTempMeshes()) {
        if (e.name.contains("_import_")) {
            QString suffix = e.name.section('_', -1);
            bool ok = false;
            int idx = suffix.toInt(&ok);
            if (ok && idx >= m_importCounter) m_importCounter = idx + 1;
        }
    }
    Refresh();
}

// ============================================================================
// Additional Methods
// ============================================================================

void CanvasWidget::onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint) {
    double distanceKilometers = distance / 1000.0;
    QString msg = QString("Measured distance: %1 meters (%2 km) from (lon: %3, lat: %4) to (lon: %5, lat: %6)")
                      .arg(distance, 0, 'f', 2)
                      .arg(distanceKilometers, 0, 'f', 2)
                      .arg(startPoint.x(), 0, 'f', 6)
                      .arg(startPoint.y(), 0, 'f', 6)
                      .arg(endPoint.x(), 0, 'f', 6)
                      .arg(endPoint.y(), 0, 'f', 6);
    MeasureDistanceDialog* dlg = m_measurementModule->getDialog();
    if (dlg) {
        dlg->raise();
    }
    QMessageBox::information(this, "Distance Measured", msg);
}

void CanvasWidget::onPresetLayerSelected(const QString& preset) {
    if (preset == "Airbase") {
        gislib->toggleAirbaseLayer();
        update();
    }
}

// void CanvasWidget::importGeoJsonLayer(const QString &filePath) {
//     if (!gislib) {
//         return;
//     }

//     for (auto it = geoJsonLayerFilePaths.constBegin(); it != geoJsonLayerFilePaths.constEnd(); ++it) {
//         if (it.value() == filePath) {
//             return;
//         }
//     }

//     gislib->importGeoJsonLayer(filePath);
//     QString layerName = QFileInfo(filePath).completeBaseName();
//     geoJsonLayers[layerName] = true;
//     geoJsonLayerFilePaths[layerName] = filePath;

//     if (m_layerPanel) {
//         if (!m_layerPanel->layerExists(layerName)) {
//             m_layerPanel->addLayerFromScript(layerName);
//         }
//     }

//     emit geoJsonLayerAdded(layerName);
//     Refresh();
// }

void CanvasWidget::onGeoJsonLayerToggled(const QString &layerName, bool visible) {
    if (gislib) {
        gislib->toggleVectorLayerVisibility(layerName, visible);
        geoJsonLayers[layerName] = visible;
        Refresh();
    } else {
    }
}


void CanvasWidget::toggleLayerVisibility(const QString& layer, bool visible) {
    if (layer == "ToolTip") {
        showTooltip = visible;
        if (!visible) {
            m_tooltipTimer.stop();
            QToolTip::hideText();
        }
    } else if (layer == "Collider") {
        showColliders = visible;
    } else if (layer == "Mesh") {
        showMesh = visible;
    } else if (layer == "Outline") {
        showOutline = visible;
    } else if (layer == "Information") {
        showInformation = visible;
    } else if (layer == "FPS") {
        showFPS = visible;
    } else if (layer == "Image") {
        showImage = visible;
    } else if (layer == "Sensors") {
        for (auto& [id, entry] : Meshes) {
            entry.detection = visible;
        }
    } else if (layer == "Radio") {
        for (auto& [id, entry] : Meshes) {
            entry.radioVisible = visible;
        }
    } else if (layer == "Trajectories") {
        showTrajectories = visible;
    }
    Refresh();
}

// ============================================================================
// Entity Info
// ============================================================================

void CanvasWidget::showEntityInfo(const QString& entityId)
{
    Q_UNUSED(entityId);
    // Deprecated: Entity info ab RuntimeEditor + EditorCanvas se open hota hai.
    // Purana MeshEntry path band.
}
void CanvasWidget::hideEntityInfo()
{
    if (entityInfoDialog && entityInfoDialog->isVisible())
        entityInfoDialog->hide();
}

void CanvasWidget::resetEntityInfoDialog()
{
    if (entityInfoDialog)
        entityInfoDialog->onHierarchyReset();
}

// ============================================================================
// Import Layer
// ============================================================================

void CanvasWidget::importLayer(const QString& filePath)
{
    if (!gislib)
        return;

    QFileInfo info(filePath);

    QString base = info.completeBaseName();
    QString ext = info.suffix().toLower();
    QString layerName = base;

    // =====================================================================
    // Prevent importing exact same file twice
    // =====================================================================
    for (auto it = geoJsonLayerFilePaths.constBegin();
         it != geoJsonLayerFilePaths.constEnd();
         ++it)
    {
        if (it.value() == filePath)
        {
            QMessageBox msgBox(parentWidget());

            msgBox.setWindowTitle(
                "Layer Already Imported"
                );

            msgBox.setText(
                QString(
                    "This layer is already imported as \"%1\".\n"
                    "Duplicate layers are not allowed."
                    ).arg(it.key())
                );

            msgBox.setIcon(
                QMessageBox::Warning
                );

            msgBox.exec();

            return;
        }
    }

    // =====================================================================
    // IMPORTANT:
    // Make layer name unique as well.
    //
    // Example:
    // roads
    // roads_2
    // roads_3
    //
    // Normally duplicate file path is already blocked, but two different
    // files from different folders may have same base filename.
    // =====================================================================
    QString originalLayerName = layerName;

    int layerSuffix = 2;

    while (geoJsonLayers.contains(layerName) ||
           geoJsonLayerFilePaths.contains(layerName) ||
           (m_layerPanel && m_layerPanel->layerExists(layerName)))
    {
        layerName =
            QString("%1_%2")
                .arg(originalLayerName)
                .arg(layerSuffix);

        ++layerSuffix;
    }

    // =====================================================================
    // Register layer
    // =====================================================================
    geoJsonLayers[layerName] = true;
    geoJsonLayerFilePaths[layerName] = filePath;

    if (m_layerPanel &&
        !m_layerPanel->layerExists(layerName))
    {
        m_layerPanel->addLayerFromScript(
            layerName
            );
    }

    bool success = false;

    // =====================================================================
    // Import according to extension
    // =====================================================================
    if (ext == "geojson" ||
        ext == "json")
    {
        success =
            GeoJSON::importLayer(
                this,
                filePath,
                layerName
                );
    }
    else if (ext == "csv")
    {
        success =
            CSV::importLayer(
                this,
                filePath,
                layerName
                );
    }
    else if (ext == "kml" ||
             ext == "kmz")
    {
        success =
            KML::importLayer(
                this,
                filePath,
                layerName
                );
    }
    else if (ext == "gml")
    {
        success =
            GML::importLayer(
                this,
                filePath,
                layerName
                );
    }
    else if (ext == "shp")
    {
        success =
            Shapefile::importLayer(
                this,
                filePath,
                layerName
                );
    }
    else if (ext == "fgb")
    {
        success =
            FlatGeobuf::importLayer(
                this,
                filePath,
                layerName
                );
    }

    // =====================================================================
    // Import successful
    // =====================================================================
    if (success)
    {
        emit geoJsonLayerAdded(
            layerName
            );

        Refresh();
    }
    else
    {
        // -------------------------------------------------------------
        // Import failed - rollback layer registration
        // -------------------------------------------------------------
        geoJsonLayers.remove(
            layerName
            );

        geoJsonLayerFilePaths.remove(
            layerName
            );

        // NOTE:
        // If LayerPanel has removeLayer() function,
        // you should also call it here.
        //
        // Example:
        //
        // if (m_layerPanel)
        //     m_layerPanel->removeLayer(layerName);
    }
}
void CanvasWidget::registerImportedShape(
    const QString& layerName,
    const QString& shapeType,
    const QString& shapeName,
    const QVector3D& position,
    const std::vector<QVector3D>& polygenOffsets,
    const QVector3D& size,
    bool closePath)
{
    qDebug() << "Register Shape:";
    qDebug() << "Layer =" << layerName;
    qDebug() << "Type =" << shapeType;
    qDebug() << "Name =" << shapeName;
    qDebug() << "Position =" << position;
    qDebug() << "Offsets =" << polygenOffsets.size();

    QVector3D fixedPos = position;
    std::vector<QVector3D> fixedOffsets = polygenOffsets;
    QVector3D fixedSize = size;

    QString finalShapeType =
        shapeType.trimmed();


    // =====================================================================
    // Convert EPSG:3857 coordinates to EPSG:4326 if required
    // =====================================================================
    if (qAbs(fixedPos.x()) > 360.0 ||
        qAbs(fixedPos.y()) > 180.0)
    {
        auto convert3857to4326 =
            [](double x, double y) -> QPointF
        {
            double lon =
                (x / 20037508.34) *
                180.0;

            double lat =
                (y / 20037508.34) *
                180.0;

            lat =
                180.0 / M_PI *
                (
                    2 *
                        std::atan(
                            std::exp(
                                lat *
                                M_PI /
                                180.0
                                )
                            )
                    -
                    M_PI / 2.0
                    );

            return QPointF(
                lon,
                lat
                );
        };


        QPointF wgsCenter =
            convert3857to4326(
                fixedPos.x(),
                fixedPos.y()
                );


        // =================================================================
        // Convert Polygon / Polyline offsets
        // =================================================================
        for (size_t i = 0;
             i < fixedOffsets.size();
             ++i)
        {
            double absX =
                fixedPos.x() +
                fixedOffsets[i].x();

            double absY =
                fixedPos.y() +
                fixedOffsets[i].y();


            QPointF wgsPt =
                convert3857to4326(
                    absX,
                    absY
                    );


            fixedOffsets[i].setX(
                wgsPt.x() -
                wgsCenter.x()
                );

            fixedOffsets[i].setY(
                wgsPt.y() -
                wgsCenter.y()
                );
        }


        // =================================================================
        // Convert Circle radius
        // =================================================================
        if (finalShapeType.compare(
                "Circle",
                Qt::CaseInsensitive) == 0)
        {
            double absX =
                fixedPos.x() +
                fixedSize.x();


            QPointF wgsPt =
                convert3857to4326(
                    absX,
                    fixedPos.y()
                    );


            fixedSize.setX(
                qAbs(
                    wgsPt.x() -
                    wgsCenter.x()
                    )
                );
        }


        fixedPos.setX(
            wgsCenter.x()
            );

        fixedPos.setY(
            wgsCenter.y()
            );
    }


    // =====================================================================
    // Detect circular Polygon and convert to Circle
    // =====================================================================
    if (finalShapeType.compare(
            "Polygon",
            Qt::CaseInsensitive) == 0 &&
        fixedOffsets.size() >= 16)
    {
        double cosLat =
            std::cos(
                fixedPos.y() *
                M_PI /
                180.0
                );


        if (cosLat < 0.01)
        {
            cosLat = 0.01;
        }


        double sumRadius = 0.0;
        double minRadius = 1e9;
        double maxRadius = -1.0;


        for (const QVector3D& offset :
             fixedOffsets)
        {
            double dx =
                offset.x() *
                cosLat;

            double dy =
                offset.y();


            double r =
                std::sqrt(
                    dx * dx +
                    dy * dy
                    );


            sumRadius += r;

            if (r < minRadius)
                minRadius = r;

            if (r > maxRadius)
                maxRadius = r;
        }


        double meanRadius =
            sumRadius /
            static_cast<double>(
                fixedOffsets.size()
                );


        if (meanRadius > 0.0 &&
            (maxRadius - minRadius) <
                meanRadius * 0.15)
        {
            finalShapeType =
                "Circle";


            fixedSize.setX(
                meanRadius /
                cosLat
                );


            fixedOffsets.clear();
        }
    }


    // =====================================================================
    // Normalize Shape Type
    // =====================================================================
    if (finalShapeType.compare(
            "Line",
            Qt::CaseInsensitive) == 0)
    {
        finalShapeType =
            "Polyline";
    }


    if (finalShapeType.compare(
            "Polygon",
            Qt::CaseInsensitive) == 0)
    {
        closePath = true;
    }


    // =====================================================================
    // DISPLAY NAME
    //
    // This is what LayerPanel should show.
    // =====================================================================
    QString displayName =
        shapeName.trimmed();


    if (displayName.isEmpty())
    {
        displayName =
            finalShapeType;
    }


    // =====================================================================
    // CRITICAL FIX:
    // CREATE UNIQUE INTERNAL SHAPE ID
    //
    // DO NOT use shapeName directly as entry.name.
    //
    // Different imported layers can contain:
    //
    // Polygon 1
    // Polygon 1
    //
    // If entry.name is same, selection/edit/delete/drag can target
    // shape from another layer.
    //
    // Keep Temp<Type> prefix because existing Canvas/ShapeEventHandler
    // code uses startsWith("TempPolygon"), etc.
    // =====================================================================

    QString typePrefix;


    if (finalShapeType.compare(
            "Circle",
            Qt::CaseInsensitive) == 0)
    {
        typePrefix =
            "TempCircle";
    }
    else if (finalShapeType.compare(
                 "Rectangle",
                 Qt::CaseInsensitive) == 0)
    {
        typePrefix =
            "TempRectangle";
    }
    else if (finalShapeType.compare(
                 "Polygon",
                 Qt::CaseInsensitive) == 0)
    {
        typePrefix =
            "TempPolygon";
    }
    else if (finalShapeType.compare(
                 "Polyline",
                 Qt::CaseInsensitive) == 0)
    {
        typePrefix =
            "TempPolyline";
    }
    else if (finalShapeType.compare(
                 "Point",
                 Qt::CaseInsensitive) == 0)
    {
        typePrefix =
            "TempPoint";
    }
    else
    {
        typePrefix =
            "Temp" +
            finalShapeType;
    }


    // =====================================================================
    // Generate globally unique ID
    //
    // Example:
    //
    // TempPolygon__LayerA__import_0
    // TempPolygon__LayerB__import_1
    //
    // IMPORTANT:
    // m_importCounter is global for CanvasWidget.
    // =====================================================================
    QString safeLayerName =
        layerName;

    safeLayerName.replace(
        " ",
        "_"
        );

    safeLayerName.replace(
        "/",
        "_"
        );

    safeLayerName.replace(
        "\\",
        "_"
        );


    QString uniqueInternalId;


    // =====================================================================
    // Extra safety:
    // Even if counter somehow resets, check existing m_tempMeshes.
    // =====================================================================
    while (true)
    {
        uniqueInternalId =
            QString(
                "%1__%2__import_%3"
                )
                .arg(typePrefix)
                .arg(safeLayerName)
                .arg(m_importCounter++);


        bool alreadyExists =
            false;


        for (const MeshEntry& existing :
             getTempMeshes())
        {
            if (existing.name ==
                uniqueInternalId)
            {
                alreadyExists =
                    true;

                break;
            }
        }


        if (!alreadyExists)
        {
            break;
        }
    }


    // =====================================================================
    // Create MeshEntry
    // =====================================================================
    MeshEntry entry;


    // INTERNAL UNIQUE ID
    entry.name =
        uniqueInternalId;


    // ACTUAL SHAPE TYPE
    entry.shapeType =
        finalShapeType;


    entry.position =
        new QVector3D(
            fixedPos
            );


    entry.rotation =
        new QQuaternion();


    entry.velocity =
        new QVector3D(
            0,
            0,
            0
            );


    entry.size =
        new QVector3D(
            fixedSize
            );


    entry.mesh =
        new Mesh();


    if (!entry.mesh)
    {
        delete entry.position;
        delete entry.rotation;
        delete entry.velocity;
        delete entry.size;
        return;
    }
    entry.mesh->closePath =
        closePath;
    entry.mesh->color =
        new QColor(
            Qt::red
            );
    entry.mesh->lineWidth =
        2;


    // =====================================================================
    // Add Polygon / Polyline Vertices
    // =====================================================================
    for (const QVector3D& v :
         fixedOffsets)
    {
        entry.mesh
            ->polygen
            .push_back(
                new Vector(
                    v.x(),
                    v.y(),
                    v.z()
                    )
                );
    }


    // =====================================================================
    // Add to Canvas Meshes
    // =====================================================================
    getTempMeshes()
        .push_back(
            entry
            );


    // =====================================================================
    // Register with LayerPanel
    //
    // IMPORTANT:
    //
    // Parameter 1 = UNIQUE INTERNAL ID
    // Parameter 2 = DISPLAY NAME
    // Parameter 3 = LAYER NAME
    //
    // This is the MOST IMPORTANT part.
    //
    // OLD:
    // addShapeToLayer(entry.name, entry.name, layerName)
    //
    // NEW:
    // addShapeToLayer(entry.name, displayName, layerName)
    // =====================================================================
    if (m_layerPanel)
    {
        m_layerPanel
            ->addShapeToLayer(
                entry.name,
                displayName,
                layerName
                );
    }


    qDebug()
        << "Imported Shape Registered:"
        << "ID =" << entry.name
        << "Display =" << displayName
        << "Layer =" << layerName
        << "Type =" << entry.shapeType;


    update();
}


// ============================================================================
// CHAITANYA JAIN CHANGE:
// Why: Import Text feature from GeoJSON.
// IMPORTANT: This function is OUTSIDE registerImportedShape().
// ============================================================================

void CanvasWidget::registerImportedText(
    const QString& layerName,
    const QString& featureName,
    const QVector3D& position,
    const QString& text)
{
    if (!m_shapeEventHandler) {
        qWarning() << "Cannot import Text: ShapeEventHandler is null";
        return;
    }

    m_shapeEventHandler->addImportedText(
        layerName,
        featureName,
        position,
        text
        );

    update();
}


// ============================================================================
// CHAITANYA JAIN CHANGE:
// Why: Import Bitmap feature from GeoJSON.
// IMPORTANT: This function is OUTSIDE registerImportedShape().
// ============================================================================

void CanvasWidget::registerImportedBitmap(
    const QString& layerName,
    const QString& featureName,
    const QVector3D& position,
    const QString& bitmapType,
    const QVector3D& size,
    float rotation)
{
    // =====================================================================
    // 1. Resolve bitmap type to internal resource path
    // =====================================================================
    QString bitmapPath;

    if (bitmapType.compare("Hospital", Qt::CaseInsensitive) == 0)
    {
        bitmapPath = ":/icons/images/hospital.png";
    }
    else if (bitmapType.compare("School", Qt::CaseInsensitive) == 0)
    {
        bitmapPath = ":/icons/images/school.png";
    }
    else if (bitmapType.compare("Forest Area", Qt::CaseInsensitive) == 0)
    {
        bitmapPath = ":/icons/images/forest-area.png";
    }
    else
    {
        qWarning()
        << "Unknown imported bitmap type:"
        << bitmapType;

        return;
    }

    // =====================================================================
    // 2. Generate a valid unique name if featureName is empty
    // =====================================================================
    QString finalName = featureName;

    if (finalName.trimmed().isEmpty())
    {
        finalName = QString("TempBitmap_%1")
        .arg(getTempMeshes().size());
    }

    // =====================================================================
    // 3. Create MeshEntry
    // =====================================================================
    MeshEntry entry;

    entry.name = finalName;

    // Geographic position: X = longitude, Y = latitude
    entry.position = new QVector3D(position);

    // Qt QQuaternion does NOT have setEulerAngles().
    // Create quaternion directly from Euler angles.
    // fromEulerAngles() expects angles in degrees.
    entry.rotation = new QQuaternion(
        QQuaternion::fromEulerAngles(
            0.0f,
            0.0f,
            rotation
            )
        );

    entry.velocity = new QVector3D(0.0f, 0.0f, 0.0f);

    // Protect against invalid/zero size values
    QVector3D finalSize = size;

    if (finalSize.x() <= 0.0f)
        finalSize.setX(1.0f);

    if (finalSize.y() <= 0.0f)
        finalSize.setY(1.0f);

    if (finalSize.z() <= 0.0f)
        finalSize.setZ(1.0f);

    entry.size = new QVector3D(finalSize);

    // =====================================================================
    // 4. Create mesh
    // =====================================================================
    entry.mesh = new Mesh();

    entry.mesh->closePath = false;
    entry.mesh->color = new QColor(Qt::white);
    entry.mesh->lineWidth = 1;

    // =====================================================================
    // 5. Set bitmap resource path
    // =====================================================================
    entry.bitmapPath = bitmapPath;

    // =====================================================================
    // 6. Add bitmap to CanvasWidget temporary meshes
    // =====================================================================
    getTempMeshes().push_back(entry);

    // =====================================================================
    // 7. Register bitmap inside LayerPanel
    // =====================================================================
    if (m_layerPanel)
    {
        m_layerPanel->addShapeToLayer(
            entry.name,
            bitmapType,
            layerName
            );
    }

    // =====================================================================
    // 8. Refresh canvas
    // =====================================================================
    update();

    // =====================================================================
    // 9. Debug information
    // =====================================================================
    qDebug()
        << "Imported bitmap registered:"
        << "Layer =" << layerName
        << "Name =" << entry.name
        << "Type =" << bitmapType
        << "Path =" << bitmapPath
        << "Position =" << position
        << "Size =" << finalSize
        << "Rotation =" << rotation;
}



// ============================================================================
// Hover and Tooltip
// ============================================================================

QString CanvasWidget::checkEntityHover(const QPoint& mousePos)
{
    if (!showTooltip)
        return QString();

    QString newHoverId;

    for (const auto& [id, entry] : Meshes) {

        if (!entry.coreTransform ||
            !entry.entity ||
            !entry.entity->Active)
        {
            continue;
        }

        QPointF entityPos =
            gislib->geoToCanvas(
                entry.coreTransform->getLatitude(),
                entry.coreTransform->getLongitude()
                );

        float imageSize = 25.0f;

        if (entry.individualImageSize > 0 ||
            ImageScale > 0)
        {
            imageSize =
                (entry.individualImageSize > 0
                     ? entry.individualImageSize
                     : ImageScale)
                / 2.0f;
        }

        imageSize += 5.0f;

        if (QVector2D(mousePos - entityPos).length()
            < imageSize)
        {
            newHoverId =
                QString::fromStdString(id);

            break;
        }
    }

    if (newHoverId != m_hoveredEntityId) {

        m_hoveredEntityId =
            newHoverId;

        if (m_hoveredEntityId.isEmpty()) {

            QToolTip::hideText();
            m_tooltipTimer.stop();

        } else {

            m_tooltipTimer.start(700);
        }

        if (!m_hoveredEntityId.isEmpty()) {

            setCursor(
                Qt::PointingHandCursor);

        } else if (
            currentMode ==
            TransformMode::Translate)
        {
            setCursor(
                Qt::ArrowCursor);
        }
    }

    return m_hoveredEntityId;
}
void CanvasWidget::updateHoverTooltip() {
    if (m_hoveredEntityId.isEmpty() || !isVisible()) return;
    auto it = Meshes.find(m_hoveredEntityId.toStdString());
    if (it == Meshes.end()) return;
    const MeshEntry& entry = it->second;
    QString tooltipText = TooltipHelper::generateEntityTooltip(entry, activeTooltipOptions);
    if (!tooltipText.isEmpty()) {
        TooltipHelper::showTooltip(tooltipText, nullptr);
    }
}

void CanvasWidget::setTooltipOptions(const QSet<QString>& options) {
    activeTooltipOptions = options;
    if (!m_hoveredEntityId.isEmpty()) {
        updateHoverTooltip();
    }
}

// ============================================================================
// Multi Selection
// ============================================================================

void CanvasWidget::clearMultiSelection() {
    isMultiDrag = false;
    clearSelectedShapeIds();
    selectedEntityIds.clear();
    selectedEntityId = "";
    setHighlightedShapeId("");
    setCursor(Qt::ArrowCursor);
    Refresh();
}

// performBoxSelection removed, delegated to ShapeEventHandler
void CanvasWidget::moveSelectedItems(const QPointF& deltaGeo)
{
    if (!isMultiDrag)
        return;


    // ==============================================================
    // 1. MOVE ALL SELECTED ENTITIES
    // ==============================================================
    for (const std::string& id : selectedEntityIds)
    {
        auto it = Meshes.find(id);

        if (it == Meshes.end())
            continue;

        MeshEntry& entry = it->second;


        // ----------------------------------------------------------
        // Move Entity
        // ----------------------------------------------------------
        if (entry.coreTransform)
        {
            double newLat =
                entry.coreTransform->getLatitude()
                + deltaGeo.y();

            double newLon =
                entry.coreTransform->getLongitude()
                + deltaGeo.x();

            entry.coreTransform->setGeoCord(
                newLat,
                newLon
                );
        }


        // ----------------------------------------------------------
        // Move Entity Trajectory Together
        // ----------------------------------------------------------
        if (entry.trajectory &&
            !entry.trajectory->Trajectories.empty())
        {
            for (Waypoints* wp :
                 entry.trajectory->Trajectories)
            {
                if (!wp || !wp->position)
                    continue;

                wp->position->x += deltaGeo.y();
                wp->position->z += deltaGeo.x();
            }
        }
    }


    // ==============================================================
    // 2. MOVE ALL SELECTED SHAPES
    // ==============================================================

    if (m_shapeEventHandler)
    {
        auto& meshes =
            m_shapeEventHandler->getTempMeshes();

        const auto& selectedShapes =
            m_shapeEventHandler->getSelectedShapeIds();

        for (const QString& selectedId : selectedShapes)
        {
            for (auto& entry : meshes)
            {
                if (entry.name != selectedId)
                    continue;

                if (!entry.position)
                    continue;

                entry.position->setX(
                    entry.position->x()
                    + deltaGeo.x()
                    );

                entry.position->setY(
                    entry.position->y()
                    + deltaGeo.y()
                    );

                break;
            }
        }
    }


    // ==============================================================
    // Refresh Canvas
    // ==============================================================

    Refresh();
}

bool CanvasWidget::isClickOnAnySelectedItem(const QPoint& pos)
{
    if (!gislib)
        return false;

    const QPointF mousePos(pos);

    // ========================================================================
    // 1. CHECK SELECTED ENTITIES
    // ========================================================================
    for (const std::string& id : selectedEntityIds)
    {
        auto it = Meshes.find(id);

        if (it == Meshes.end())
            continue;

        const auto& entry = it->second;

        if (!entry.coreTransform /*||
            !entry.coreTransform->geocord*/)
        {
            continue;
        }

        QPointF entityPos =
            gislib->geoToCanvas(
                entry.coreTransform->getLatitude(),
                entry.coreTransform->getLongitude()
                );

        if (QVector2D(mousePos - entityPos).length() <= 20.0f)
            return true;
    }


    // ========================================================================
    // 2. CHECK SELECTED SHAPES
    // ========================================================================
    if (!m_shapeEventHandler)
        return false;

    const auto& selectedShapes =
        m_shapeEventHandler->getSelectedShapeIds();

    const auto& tempMeshes =
        m_shapeEventHandler->getTempMeshes();


    // IMPORTANT:
    // Do NOT assume that the first MeshEntry having selectedId is the
    // correct object.
    //
    // Imported layers may contain duplicate legacy names such as:
    //
    // TempRectangle...
    // TempPolygon...
    //
    // Therefore every matching MeshEntry must be geometrically tested.
    // ========================================================================
    for (const QString& selectedId : selectedShapes)
    {
        for (const auto& entry : tempMeshes)
        {
            if (entry.name != selectedId)
                continue;

            if (!m_shapeEventHandler->shouldDrawShape(entry.name))
                continue;

            if (!entry.position)
                continue;


            // =================================================================
            // POINT
            // =================================================================
            if (entry.name.startsWith("TempPoint"))
            {
                QPointF centerCanvas =
                    gislib->geoToCanvas(
                        entry.position->y(),
                        entry.position->x()
                        );

                const qreal distance =
                    QVector2D(mousePos - centerCanvas).length();

                if (distance <= 8.0)
                    return true;

                continue;
            }


            // =================================================================
            // TEXT
            // =================================================================
            if (entry.name.startsWith("TempText") ||
                entry.name.startsWith("Text"))
            {
                if (entry.text.isEmpty())
                    continue;

                QPointF textCanvas =
                    gislib->geoToCanvas(
                        entry.position->y(),
                        entry.position->x()
                        );

                QFontMetrics fm(entry.textFont);

                QRectF textRect =
                    fm.boundingRect(entry.text);

                // Same coordinate convention as actual text hit testing.
                textRect.moveTopLeft(
                    QPointF(
                        textCanvas.x(),
                        textCanvas.y() - fm.ascent()
                        )
                    );

                // Small tolerance only.
                textRect.adjust(
                    -4.0,
                    -4.0,
                    4.0,
                    4.0
                    );

                if (textRect.contains(mousePos))
                    return true;

                continue;
            }


            // =================================================================
            // GET ACTUAL CANVAS GEOMETRY
            // =================================================================
            QPolygonF poly =
                m_shapeEventHandler
                    ->getRotatedShapePolygon(entry);

            if (poly.isEmpty())
                continue;


            // =================================================================
            // LINE / POLYLINE
            // =================================================================
            if (entry.name.startsWith("TempPolyline"))
            {
                if (poly.size() < 2)
                    continue;

                const qreal lineTolerance = 7.0;

                for (int i = 0;
                     i < poly.size() - 1;
                     ++i)
                {
                    const QPointF& a = poly[i];
                    const QPointF& b = poly[i + 1];

                    QPointF ab = b - a;
                    QPointF ap = mousePos - a;

                    const qreal lengthSquared =
                        ab.x() * ab.x() +
                        ab.y() * ab.y();

                    if (lengthSquared <= 0.000001)
                        continue;

                    qreal t =
                        (ap.x() * ab.x() +
                         ap.y() * ab.y()) /
                        lengthSquared;

                    t = qBound(
                        0.0,
                        t,
                        1.0
                        );

                    QPointF closest =
                        a + ab * t;

                    const qreal distance =
                        QVector2D(
                            mousePos - closest
                            ).length();

                    if (distance <= lineTolerance)
                        return true;
                }

                continue;
            }


            // =================================================================
            // CIRCLE / RECTANGLE / POLYGON / BITMAP
            //
            // IMPORTANT:
            // Exact polygon only.
            // No oversized artificial hit rectangle.
            // No boundingRect-only hit detection.
            // =================================================================
            if (poly.size() >= 3 &&
                poly.containsPoint(
                    mousePos,
                    Qt::OddEvenFill
                    ))
            {
                return true;
            }
        }
    }

    return false;
}
void CanvasWidget::selectMultipleEntities(const QList<QString>& entityIds) {
    selectedEntityIds.clear();
    selectedEntityId.clear();
    for (const QString& id : entityIds) {
        selectedEntityIds.push_back(id.toStdString());
    }
    if (!selectedEntityIds.empty()) {
        selectedEntityId = selectedEntityIds[0];
    }
    update();
}

void CanvasWidget::clearSelection() {
    selectedEntityIds.clear();
    selectedEntityId.clear();
    update();
}

// ============================================================================
// Center on Entity
// ============================================================================

void CanvasWidget::centerOnEntity(const QString& entityId, bool adjustZoom) {
    auto it = Meshes.find(entityId.toStdString());
    if (it == Meshes.end()) return;
    MeshEntry& entry = it->second;
    if (!entry.coreTransform) return;
    double latitude = entry.coreTransform->getLatitude();
    double longitude = entry.coreTransform->getLongitude();
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) return;
    if (gislib) {
        gislib->setCenter(latitude, longitude);
        if (adjustZoom) {
            gislib->setZoom(9);
        }
        update();
    }
}

void CanvasWidget::centerOnEntityWithZoom(const QString& entityId, int zoomLevel) {
    if (zoomLevel < 0 || zoomLevel > 18) zoomLevel = 10;

    auto it = Meshes.find(entityId.toStdString());
    if (it == Meshes.end()) return;
    MeshEntry& entry = it->second;
    if (!entry.coreTransform) return;

    double latitude = entry.coreTransform->getLatitude();
    double longitude = entry.coreTransform->getLongitude();
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) return;

    if (gislib) {
        gislib->setCenter(latitude, longitude);
        gislib->setZoom(zoomLevel);
        update();
    }
}

double CanvasWidget::calculateTrajectoryCompletionTime(const MeshEntry& entry) const {
    if (!entry.trajectory || entry.trajectory->Trajectories.empty()) return -1.0;
    if (!entry.platform /*|| !entry.platform->dynamicModel*/) return -1.0;
    double totalTime = 0.0;
    double currentSpeed = entry.platform->dynamicModel.currentSpeed;
    if (currentSpeed <= 0) currentSpeed = entry.platform->dynamicModel.moveSpeed;
    if (currentSpeed <= 0) return -1.0;
    QPointF currentPos(entry.coreTransform->getLongitude(), entry.coreTransform->getLatitude());
    if (!entry.trajectory->Trajectories.empty()) {
        Waypoints* firstWp = entry.trajectory->Trajectories[0];
        QPointF wpPos(firstWp->position->z, firstWp->position->x);
        double distanceToFirst = gislib->calculateDistance(currentPos, wpPos) / 1000.0;
        totalTime += (distanceToFirst / currentSpeed) * 3600.0;
    }
    for (size_t i = 0; i < entry.trajectory->Trajectories.size() - 1; ++i) {
        Waypoints* wp1 = entry.trajectory->Trajectories[i];
        Waypoints* wp2 = entry.trajectory->Trajectories[i + 1];
        QPointF pos1(wp1->position->z, wp1->position->x);
        QPointF pos2(wp2->position->z, wp2->position->x);
        double distance = gislib->calculateDistance(pos1, pos2) / 1000.0;
        double speedForSegment = (wp1->speed > 0) ? wp1->speed : currentSpeed;
        totalTime += (distance / speedForSegment) * 3600.0;
    }
    return totalTime;
}

// ============================================================================
// Reset
// ============================================================================

void CanvasWidget::fullCanvasReset() {
    auto& meshes = getTempMeshes();
    for (auto& entry : meshes) {
        if (entry.mesh) {
            for (Vector* v : entry.mesh->polygen) delete v;
            if (entry.mesh->color) delete entry.mesh->color;
            delete entry.mesh;
        }
        delete entry.collider;
        if (entry.trajectory) {
            for (Waypoints* wp : entry.trajectory->Trajectories) {
                delete wp->position;
                delete wp;
            }
            delete entry.trajectory;
        }
    }
    meshes.clear();

    std::vector<std::string> toRemove;
    for (auto& [id, entry] : Meshes) {
        if (entry.entity && entry.entity->type == Constants::EntityType::Weapon) {
            toRemove.push_back(id);
        }
    }
    for (const auto& id : toRemove) {
        Meshes.erase(id);
    }

    clearMultiSelection();
    selectedEntityIds.clear();
    getSelectedShapeIds().clear();
    selectedEntityId.clear();
    setHighlightedShapeId("");
    clearActiveRotateId();

    for (Waypoints* wp : currentTrajectory) {
        if (wp) { delete wp->position; delete wp; }
    }
    currentTrajectory.clear();
    geoJsonLayers.clear();
    geoJsonLayerFilePaths.clear();
    m_importCounter = 0;
    if (m_shapesFeature) {
        m_shapesFeature->resetCounters();
    }
    if (m_shapeEventHandler) {
        m_shapeEventHandler->resetCounters();
        m_shapeEventHandler->clearStates();
    }

    Refresh();
}

// ============================================================================
// Sonobuoy Highlight
// ============================================================================

void CanvasWidget::drawSonobuoyHighlight(QPainter& painter) {
    if (m_highlightedSonobuoyId.isEmpty()) return;

    for (auto& [id, entry] : Meshes) {
        if (!entry.platform /*|| !entry.platform->weapons*/) continue;
        auto wIt = entry.platform->weapons.weapons->find(m_highlightedSonobuoyId.toStdString());
        if (wIt == entry.platform->weapons.weapons->end()) continue;
        Weapon* wb = wIt->second;
        if (!wb /*|| !wb->transform*/ /*|| !wb->transform->geocord*/) break;

        double lat = wb->transform.getLatitude();
        double lon = wb->transform.getLongitude();
        if (lat > 180.0 || lat < -180.0) break;

        QPointF pos = gislib->geoToCanvas(lat, lon);
        painter.save();
        painter.setPen(QPen(Qt::red, 3, Qt::DashLine));
        painter.drawEllipse(pos, 20, 20);
        painter.restore();
        break;
    }
}

void CanvasWidget::highlightSonobuoy(const QString& sonobuoyId) {
    m_highlightedSonobuoyId = sonobuoyId;
    selectedEntityId.clear();
    selectedEntityIds.clear();
    activeDragAxis.clear();
    Refresh();
}

void CanvasWidget::clearSonobuoyHighlight() {
    if (!m_highlightedSonobuoyId.isEmpty()) {
        m_highlightedSonobuoyId.clear();
        Refresh();
    }
}

void CanvasWidget::onWaypointSelectedFromInspector(int waypointIndex) {
    m_highlightedWaypointIndex = waypointIndex;
    m_highlightedWaypointEntityId = QString::fromStdString(selectedEntityId);
    Refresh();
}

bool CanvasWidget::isClickOnEmptyCanvas(const QPoint& pos) {
    for (auto& [id, entry] : Meshes) {
        if (!entry.coreTransform) continue;
        QPointF entityPos = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude()
            );
        if (QVector2D(pos - entityPos).length() < 20.0f) return false;
        if (entry.trajectory && !entry.trajectory->Trajectories.empty()) {
            for (size_t i = 0; i < entry.trajectory->Trajectories.size(); ++i) {
                QPointF wpCanvas = gislib->geoToCanvas(
                    entry.trajectory->Trajectories[i]->position->x,
                    entry.trajectory->Trajectories[i]->position->z
                    );
                if (QVector2D(pos - wpCanvas).length() < 10.0f) return false;
            }
            if (entry.trajectory->Trajectories.size() >= 2) {
                const auto& traj = entry.trajectory->Trajectories;
                for (size_t i = 0; i < traj.size() - 1; ++i) {
                    QPointF p1 = gislib->geoToCanvas(traj[i]->position->x, traj[i]->position->z);
                    QPointF p2 = gislib->geoToCanvas(traj[i+1]->position->x, traj[i+1]->position->z);
                    QPointF lineVec = p2 - p1;
                    QPointF pointVec = pos - p1;
                    float lineLengthSquared = lineVec.x() * lineVec.x() + lineVec.y() * lineVec.y();
                    if (lineLengthSquared < 1e-6) continue;
                    float t = (pointVec.x() * lineVec.x() + pointVec.y() * lineVec.y()) / lineLengthSquared;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    QPointF projection = p1 + t * lineVec;
                    if (QVector2D(pos - projection).length() < 8.0f) return false;
                }
            }
        }
    }

    for (const auto& entry : getTempMeshes()) {
        if (!entry.position || !entry.size) continue;
        QPointF centerGeo(entry.position->x(), entry.position->y());
        QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

        //  if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
        if (entry.name.startsWith("Temp") ||
            entry.name.startsWith("UserImage") ||
            !entry.bitmapPath.isEmpty() ||
            entry.shapeType.compare("Text", Qt::CaseInsensitive) == 0) {
            QFont font = entry.textFont;
            font.setPointSize(entry.textSize);
            QFontMetrics fm(font);
            QRect textRect = fm.boundingRect(entry.text);
            textRect.moveTo(centerCanvas.x(), centerCanvas.y() - fm.ascent());
            if (textRect.contains(pos)) return false;
        }

        if (!entry.bitmapPath.isEmpty()) {
            QPointF sizePointGeo(entry.position->x() + entry.size->x(), entry.position->y() + entry.size->y());
            QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
            float halfWidth = qAbs(sizePointCanvas.x() - centerCanvas.x());
            float halfHeight = qAbs(sizePointCanvas.y() - centerCanvas.y());
            QRectF bitmapRect(centerCanvas.x() - halfWidth, centerCanvas.y() - halfHeight,
                              halfWidth * 2, halfHeight * 2);
            if (bitmapRect.contains(pos)) return false;
        }

        if (entry.name.startsWith("TempCircle")) {
            QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
            QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
            float radius = QVector2D(radiusPointCanvas - centerCanvas).length();
            if (QVector2D(pos - centerCanvas).length() <= radius) return false;
        }

        if (entry.name.startsWith("TempRectangle") || entry.name.startsWith("TempPolygon") ||
            entry.name.startsWith("TempPolyline")) {
            if (entry.mesh && !entry.mesh->polygen.empty()) {
                QPolygonF polygon;
                float rotationRad = entry.rotation->z();
                float cosA = std::cos(rotationRad);
                float sinA = std::sin(rotationRad);
                for (Vector* point : entry.mesh->polygen) {
                    if (!point) continue;
                    float localX = point->x;
                    float localY = point->y;
                    float rotatedX = localX * cosA - localY * sinA;
                    float rotatedY = localX * sinA + localY * cosA;
                    float worldGeoX = centerGeo.x() + rotatedX;
                    float worldGeoY = centerGeo.y() + rotatedY;
                    QPointF canvasPoint = gislib->geoToCanvas(worldGeoY, worldGeoX);
                    polygon << canvasPoint;
                }
                if (polygon.containsPoint(pos, Qt::OddEvenFill)) return false;
            }
        }
        if (entry.name.startsWith("TempPoint")) {
            QPointF pointCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            if (QVector2D(pos - pointCanvas).length() <= 5.0f) return false;
        }
    }
    return true;
}

bool CanvasWidget::isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance) {
    QPointF vec = v2 - v1;
    QPointF vp = p - v1;
    qreal lenSquared = vec.x() * vec.x() + vec.y() * vec.y();
    if (lenSquared < 1e-6) {
        return QVector2D(p - v1).length() < tolerance;
    }
    qreal t = std::max<qreal>(0.0, std::min<qreal>(1.0, (vp.x() * vec.x() + vp.y() * vec.y()) / lenSquared));
    QPointF projection = v1 + t * vec;
    return QVector2D(p - projection).length() < tolerance;
}
void CanvasWidget::selectEntitiesInBox(const QRectF& boxRect)
{
    if (!gislib) return;

    selectedEntityIds.clear();

    for (auto& [id, entry] : Meshes) {
        if (!entry.coreTransform /*|| !entry.coreTransform->geocord*/) continue;

        QPointF entityPos = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude()
            );
        bool isSelected = boxRect.contains(entityPos);
        if (!isSelected && entry.trajectory && !entry.trajectory->Trajectories.empty()) {
            for (const Waypoints* wp : entry.trajectory->Trajectories) {
                if (!wp || !wp->position) continue;
                QPointF wpCanvas = gislib->geoToCanvas(wp->position->x, wp->position->z);
                if (boxRect.contains(wpCanvas)) {
                    isSelected = true;
                    break;
                }
            }
        }
        if (isSelected) {
            selectedEntityIds.push_back(id);
        }
    }

    if (!selectedEntityIds.empty()) {
        selectedEntityId = selectedEntityIds[0];
    } else {
        selectedEntityId.clear();
    }

    Refresh();
}

bool CanvasWidget::deleteObjectFromCanvas(const QString& shapeId) {
    return deleteObjectById(shapeId);
}

void CanvasWidget::addRasterMesh(const MeshEntry& entry) {
    getTempMeshes().push_back(entry);
    update();
}

std::vector<MeshEntry> CanvasWidget::getMeshEntriesForShapes(const QStringList& shapeIds)
{
    std::vector<MeshEntry> result;

    // Permanent / simulation meshes
    for (const auto& pair : Meshes)
    {
        if (shapeIds.contains(pair.second.name) ||
            shapeIds.contains(QString::fromStdString(pair.first)))
        {
            result.push_back(pair.second);
        }
    }

    // Temporary GIS shapes
    for (const auto& entry : getTempMeshes())
    {
        if (shapeIds.contains(entry.name))
        {
            result.push_back(entry);
        }
    }

    return result;
}
ViewportInfo CanvasWidget::getViewportInfo() const {
    ViewportInfo info;
    if (gislib) {
        QPointF centreCanvas(gislib->width() / 2.0, gislib->height() / 2.0);
        info.centerGeo = gislib->canvasToGeo(centreCanvas);

        QPointF p1 = gislib->canvasToGeo(QPointF(gislib->width() / 2.0,       gislib->height() / 2.0));
        QPointF p2 = gislib->canvasToGeo(QPointF(gislib->width() / 2.0 + 1.0, gislib->height() / 2.0));
        QPointF p3 = gislib->canvasToGeo(QPointF(gislib->width() / 2.0,       gislib->height() / 2.0 + 1.0));

        info.degPerPixLon = std::abs(p2.x() - p1.x());
        info.degPerPixLat = std::abs(p3.y() - p1.y());
    }
    return info;
}
void CanvasWidget::fitMapBounds(double minLon, double minLat, double maxLon, double maxLat) {
    if (gislib) {
        gislib->fitToBounds(minLat, minLon, maxLat, maxLon, 0);
        update();
    }
}
