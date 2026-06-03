/*
* CanvasWidget Implementation File
* This file contains the implementation of the CanvasWidget class which provides
* a interactive canvas for rendering entities, trajectories, shapes, and handling
* various user interactions including drag-drop, drawing, and measurement.
*/

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
#include "GUI/Tacticaldisplay/Gis/shapes_feature.h"
#include <core/Hierarchy/Utils/entityutils.h>
#include <core/Hierarchy/Components/transform.h>
#include <GUI/Tacticaldisplay/waypointeditdialog.h>
#include <QDialogButtonBox>
#include <QToolTip>
#include <GUI/Tacticaldisplay/tooltiphelper.h>
#include <QHBoxLayout>
#include <QWidgetAction>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) ,pointPen(Qt::cyan){
   QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
   QRegularExpressionMatch match = re.match(MainWindow::scenarioconfig->getSavedImageSize());
   ImageScale = match.captured(1).toInt();
   pointPen.setWidth(pointRadius * 2);
   cacheimg = new QPixmap(":/texture/images/Texture/fighterjet.png");
   scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                Qt::IgnoreAspectRatio, Qt::FastTransformation);
   gislib = new GISlib();
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
   selectedHandleIndex = -1;
   isResizingShape = false;
   QTimer* fpsUpdateTimer = new QTimer(this);
   connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
       fps = frameCount;
       frameCount = 0;
   });
   shapesFeature = new ShapesFeature(this);
  activeTooltipOptions = {"Name", "Speed", "Altitude", "Latitude", "Longitude","Trajectory ETA"};
   // Initialize entity info dialog
   entityInfoDialog = new EntityInfoDialog(parentWidget());
   entityInfoDialog->setAttribute(Qt::WA_DeleteOnClose, false);

 connect(&m_tooltipTimer, &QTimer::timeout, this, &CanvasWidget::updateHoverTooltip);
   connect(entityInfoDialog, &EntityInfoDialog::update, this,[=]{
       Refresh();
   });
   // connect(this, &CanvasWidget::selectEntitybyCursor, this, &CanvasWidget::showEntityInfo);
   connect(gislib, &GISlib::distanceMeasured, this, &CanvasWidget::onDistanceMeasured);
   connect(gislib, &GISlib::keyPressed, this, &CanvasWidget::keyPressEvent);
   connect(gislib, &GISlib::mousePressed, this, &CanvasWidget::mousePressEvent);
   connect(gislib, &GISlib::mouseMoved, this, &CanvasWidget::mouseMoveEvent);
   connect(gislib, &GISlib::mouseReleased, this, &CanvasWidget::mouseReleaseEvent);
   connect(gislib, &GISlib::painted, this, &CanvasWidget::paintEvent);
   fpsUpdateTimer->start(100);
   Refresh();
}
// getter return shape feature
ShapesFeature* CanvasWidget::getShapesFeature() const
{
   return shapesFeature;

}
void CanvasWidget::ReInit(){
   QRegularExpression re("(\\d+)(?:\\s*px)?", QRegularExpression::CaseInsensitiveOption);
   QRegularExpressionMatch match = re.match(MainWindow::scenarioconfig->getSavedImageSize());
   ImageScale = match.captured(1).toInt();
   scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                Qt::IgnoreAspectRatio, Qt::FastTransformation);
}
void CanvasWidget::setImageScale(int value){
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

void CanvasWidget::Render(float ) {
   angle += 2.0f;
   if (angle >= 360.0f) angle = 0;
   update();
}

void CanvasWidget::Refresh() {
   if(!simulate){
       update();
   }
}

void CanvasWidget::simulation() {
   simulate = true;
}

void CanvasWidget::setLayerPanel(LayerPanel* panel) {
    m_layerPanel = panel;
    if (shapesFeature) {
        shapesFeature->setLayerPanel(panel);
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
            }
            update();
        });

        connect(m_layerPanel, &LayerPanel::rasterLayerChanged,
                this, [this]() {
            update();
        });
        connect(m_layerPanel, &LayerPanel::layerWithShapesRemoved,
                this, [this](const QStringList& shapeIds) {
            tempMeshes.erase(
                std::remove_if(tempMeshes.begin(), tempMeshes.end(),
                    [&shapeIds](const MeshEntry& e) {
                        return shapeIds.contains(e.name);
                    }),
                tempMeshes.end()
            );
            update();
        });
        connect(m_layerPanel, &LayerPanel::shapeClicked,
                     this, [this](const QString& shapeId) {
                 m_highlightedShapeId = shapeId;
                 centerOnShape(shapeId);
             });
    }
}
void CanvasWidget::dragEnterEvents(QDragEnterEvent *event)
{
   if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
       event->mimeData()->hasFormat("application/x-entity")) {
       event->acceptProposedAction();
   } else {
       event->ignore();
   }
}

void CanvasWidget::dragMoveEvents(QDragMoveEvent *event)
{
   if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist") ||
       event->mimeData()->hasFormat("application/x-entity")) {
       event->acceptProposedAction();
   } else {
       event->ignore();
   }
}
void CanvasWidget::dropEvents(QDropEvent *event)
{
   const QMimeData *mimeData = event->mimeData();
   bool accepted = false;
   // Try both MIME types
   if (mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
       //qDebug() << "Processing application/x-qabstractitemmodeldatalist";
       QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
       QDataStream stream(&encoded, QIODevice::ReadOnly);
       while (!stream.atEnd()) {
           int row, col;
           QMap<int, QVariant> roleDataMap;
           stream >> row >> col >> roleDataMap;
           QString text = roleDataMap.value(Qt::DisplayRole).toString();
           QVariantMap customData = roleDataMap.value(Qt::UserRole).toMap();
           // FIX: Remove the always-true condition
           if (customData["type"].toString() == "entity" &&
               customData.contains("name") && !customData["name"].toString().isEmpty() &&
               customData.contains("ID") && !customData["ID"].toString().isEmpty()) {
               QPoint dropPos = event->pos();
               QPointF geoPos = gislib->canvasToGeo(dropPos);
               QString entityId = customData["ID"].toString();
               if (Meshes.find(entityId.toStdString()) != Meshes.end()) {
                   auto& entry = Meshes[entityId.toStdString()];
                   entry.coreTransform->setGeoCord(geoPos.y(),geoPos.x());
                   if (entry.position) {
                       entry.position->setZ(geoPos.x());
                       entry.position->setX(geoPos.y());
                   }
                   //qDebug() << "Entity successfully placed at:" << geoPos;
                   accepted = true;
               } else {

               }
           }
       }
   }

   // Also check for application/x-entity format
   if (!accepted && mimeData->hasFormat("application/x-entity")) {
       //qDebug() << "Processing application/x-entity";
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
void CanvasWidget::setTransformMode(TransformMode mode) {
   currentMode = mode;
   if (mode == MeasureDistance) {
       if (!measureDialog) {
           measureDialog = new MeasureDistanceDialog(parentWidget());
           measureDialog->setAttribute(Qt::WA_DeleteOnClose, false);
           measureDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint |
                                         Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
           connect(measureDialog, &MeasureDistanceDialog::newMeasurementRequested, this, [&]() {
               measurePoints.clear();
               Refresh();
           });
           connect(measureDialog, &MeasureDistanceDialog::newMeasurementRequested, this, [&]() {
               measurePoints.clear();
               Refresh();
           });
           connect(measureDialog, &MeasureDistanceDialog::measurementTypeChanged, this, &CanvasWidget::onMeasurementTypeChanged);
           connect(measureDialog, &QDialog::rejected, this, [&]() {
               setTransformMode(Translate);
           });
           connect(this, &CanvasWidget::pointsUpdated, measureDialog, [this](const QList<QPointF>& points) {
               if (measureDialog->isVisible()) {
                   measureDialog->clearMeasurements();
                   for (int i = 1; i < points.size(); ++i) {
                       double dist = gislib->calculateDistance(points[i-1], points[i]);
                       measureDialog->addMeasurement(points[i].x(), points[i].y(), dist);
                   }
               }
           });
           measureDialog->setModal(false);
       }
       if (!measureDialog->isVisible()) {
           measureDialog->show();
           measureDialog->raise();
           measureDialog->clearMeasurements();
       }
       measurePoints.clear();
       setCursor(Qt::CrossCursor);

   } else {
       if (measureDialog && measureDialog->isVisible()) {
           measureDialog->hide();
       }
       setCursor(Qt::ArrowCursor);
   }
   Refresh();
}

void CanvasWidget::setTrajectoryDrawingMode(bool enabled) {
   isDrawingTrajectory = enabled;
   if (enabled) {
       if (!selectedEntityId.empty()) {
           auto it = Meshes.find(selectedEntityId);
           if (it != Meshes.end() && it->second.trajectory == nullptr) return;
           if (it->second.trajectory) {
               currentMode = DrawTrajectory;
               setCursor(Qt::CrossCursor);
               for (Waypoints* wp : currentTrajectory) {
                   delete wp->position;
                   delete wp;
               }

               currentTrajectory.clear();
               for (const Waypoints* wp : it->second.trajectory->Trajectories) {
                   Waypoints* newWaypoint = new Waypoints();
                   newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
                   newWaypoint->speed = wp->speed;
                   newWaypoint->formation = wp->formation;
                   newWaypoint->sensor = wp->sensor;
                   currentTrajectory.push_back(newWaypoint);
               }
           } else {
               currentMode = DrawTrajectory;
               setCursor(Qt::CrossCursor);
               currentTrajectory.clear();
           }
       } else {
           currentTrajectory.clear();
       }
   } else {
       currentMode = Translate;
       setCursor(Qt::ArrowCursor);
       deselectWaypoint();
       if (!currentTrajectory.empty() && !selectedEntityId.empty()) {
           saveTrajectory();
       }
   }
   Refresh();
}
void CanvasWidget::saveTrajectory() {
   //Console::log("saveTrajectory called");
   if (!isDrawingTrajectory) {
       return;
   }
   if (selectedEntityId.empty()) {
       return;
   }
   auto it = Meshes.find(selectedEntityId);
   if (it == Meshes.end()) {
       return;
   }
   MeshEntry& entry = it->second;
   if (!entry.trajectory) {
       if(entry.platform && entry.platform->trajectory){
           entry.trajectory = entry.platform->trajectory;
       }else{
           entry.trajectory = new Trajectory();
           entry.trajectory->ID = selectedEntityId;
       }
   }
   if(!entry.trajectory){
       if(entry.platform && entry.platform->trajectory){
       }
   }
   for (Waypoints* wp : entry.trajectory->Trajectories) {
       delete wp->position;
       delete wp;
   }
   entry.trajectory->Trajectories.clear();
   for (Waypoints* waypoint : currentTrajectory) {
       Waypoints* newWaypoint = new Waypoints();
       newWaypoint->position = new Vector(waypoint->position->x, waypoint->position->y, waypoint->position->z);
       newWaypoint->speed = waypoint->speed;
       newWaypoint->formation = waypoint->formation;
       newWaypoint->sensor = waypoint->sensor;
       entry.trajectory->addTrajectory(newWaypoint);
   }

   entry.trajectory->Active = !currentTrajectory.empty();
   // Prepare JSON data for emission
   QJsonArray waypointsArray;
   for (const Waypoints* wp : entry.trajectory->Trajectories) {
       QJsonObject wpObj;
       QJsonObject posObj;
       posObj["type"] = "vector";
       posObj["x"] = wp->position->x;
       posObj["y"] = wp->position->y;
       posObj["z"] = wp->position->z;
       wpObj["position"] = posObj;
       waypointsArray.append(wpObj);
   }
   emit trajectoryUpdatedforLogger(QString::fromStdString(selectedEntityId), entry.trajectory->Trajectories);
   emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
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
   // Clear existing trajectory waypoints
   for (Waypoints* wp : entry.trajectory->Trajectories) {
       delete wp->position;
       delete wp;
   }
   entry.trajectory->Trajectories.clear();
   // Update trajectory waypoints
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
           newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
           newWaypoint->speed = wp->speed;
           newWaypoint->formation = wp->formation;
           newWaypoint->sensor = wp->sensor;
           currentTrajectory.push_back(newWaypoint);
       }
       deselectWaypoint();
   }
   Refresh();
}

void CanvasWidget::wheelEvent(QWheelEvent *event)
{
   if (event->angleDelta().y() > 0) {
       zoomLevel +=1;
   } else {
       zoomLevel -=1;
   }
   if(zoomLevel > 19){
       zoomLevel = 19;
   }else
       if(zoomLevel < 1){
           zoomLevel = 1;
       }
   Refresh();
}
void CanvasWidget::handleMousePress(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Clear highlight at the start of every left click
        // It will be re-set if something is actually clicked
        m_highlightedShapeId = "";
    }

    if (event->button() == Qt::LeftButton &&
          event->type() == QEvent::MouseButtonDblClick &&
          currentMode == DrawShape &&
          (selectedShape == "Line" || selectedShape == "Polygon"))
      {
          QPointF geoPos = gislib->canvasToGeo(event->pos());
          shapesFeature->handleShapeDrawing(selectedShape, geoPos, true);
          return;
      }
   if (event->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonDblClick) {
       if (isClickOnEmptyCanvas(event->pos())) {
           boxZoomPending = true;
           boxZoomStart = event->pos();
           boxZoomCurrent = event->pos();
           doubleClickTimer.start();
           return;
       } else {
           boxZoomPending = false;
       }
   }
   if (event->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonDblClick) {
       m_tooltipTimer.stop();
               QToolTip::hideText();
       bool isRuntimeEditor = !window()->windowTitle().contains("Scenario Editor", Qt::CaseInsensitive);
       if (isRuntimeEditor) {
           for (auto& [id, entry] : Meshes) {
               if (!entry.coreTransform || !entry.coreTransform->geocord) continue;  // ADD

               if (!entry.coreTransform) continue;
               QPointF entityPos = gislib->geoToCanvas(

                   entry.coreTransform->getLatitude(),
                   entry.coreTransform->getLongitude()
               );
               float imageSize = 20.0f;
               if (entry.individualImageSize > 0 || ImageScale > 0) {
                   imageSize = (entry.individualImageSize > 0 ? entry.individualImageSize : ImageScale) / 2.0f;
               }
               if (QVector2D(event->pos() - entityPos).length() < imageSize) {
                   selectedEntityId = id;
                   if (entry.entity) {
                       showEntityInfo(QString::fromStdString(id));
                   }
                   return;
               }
           }
       }
       for (auto& [id, entry] : Meshes) {
            if (!entry.coreTransform || !entry.coreTransform->geocord) continue;
           if (!entry.trajectory || entry.trajectory->Trajectories.empty()) continue;
           QPointF entityPos = gislib->geoToCanvas(
               entry.coreTransform->getLatitude(),
               entry.coreTransform->getLongitude()
           );
           float imageSize = 20.0f;
           if (entry.individualImageSize > 0 || ImageScale > 0) {
               imageSize = (entry.individualImageSize > 0 ? entry.individualImageSize : ImageScale) / 2.0f;
           }
           if (QVector2D(event->pos() - entityPos).length() < imageSize) {
               continue;
           }
           // Now check trajectory
           bool trajectoryClicked = false;
           // Check waypoints
           for (size_t i = 0; i < entry.trajectory->Trajectories.size(); ++i) {
               QPointF wpCanvas = gislib->geoToCanvas(
                   entry.trajectory->Trajectories[i]->position->x,
                   entry.trajectory->Trajectories[i]->position->z
               );
               if (QVector2D(event->pos() - wpCanvas).length() < 10.0f) {
                   trajectoryClicked = true;
                   break;
               }
           }

           // Check trajectory lines
           if (!trajectoryClicked && entry.trajectory->Trajectories.size() >= 2) {
               const auto& traj = entry.trajectory->Trajectories;
               for (size_t i = 0; i < traj.size() - 1; ++i) {
                   QPointF p1 = gislib->geoToCanvas(traj[i]->position->x, traj[i]->position->z);
                   QPointF p2 = gislib->geoToCanvas(traj[i+1]->position->x, traj[i+1]->position->z);
                   QPointF lineVec = p2 - p1;
                   QPointF pointVec = event->pos() - p1;
                   float lineLengthSquared = lineVec.x() * lineVec.x() + lineVec.y() * lineVec.y();
                   if (lineLengthSquared < 1e-6) continue;
                   float t = (pointVec.x() * lineVec.x() + pointVec.y() * lineVec.y()) / lineLengthSquared;
                   if (t < 0.0f) t = 0.0f;
                   if (t > 1.0f) t = 1.0f;
                   QPointF projection = p1 + t * lineVec;
                   if (QVector2D(event->pos() - projection).length() < 8.0f) {
                       trajectoryClicked = true;
                       break;
                   }
               }
           }
           if (trajectoryClicked) {
               selectedEntityId = id;
               setTrajectoryDrawingMode(true);
               if (isDrawingTrajectory) {
                   auto& entry = Meshes[id];
                   currentTrajectory.clear();
                   for (const Waypoints* wp : entry.trajectory->Trajectories) {
                       Waypoints* newWaypoint = new Waypoints();
                       newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
                       newWaypoint->speed = wp->speed;
                       newWaypoint->formation = wp->formation;
                       newWaypoint->sensor = wp->sensor;
                       currentTrajectory.push_back(newWaypoint);
                   }
               }
               emit selectEntitybyCursor(QString::fromStdString(id));
               return;
           }
       }
       if (isClickOnEmptyCanvas(event->pos())) {
           boxZoomPending = true;
           boxZoomStart = event->pos();
           boxZoomCurrent = event->pos();
           doubleClickTimer.start();
           return;
       }
       boxZoomPending = false;
       return;
   }
   if (event->button() == Qt::RightButton) {
       handleRightClick(event);
       return;
   }
   if (event->button() == Qt::MiddleButton) {
       isPanning = true;
       lastMousePos = event->pos();
       setCursor(Qt::ClosedHandCursor);
       return;
   }
   // if (event->button() == Qt::LeftButton && !isDrawingTrajectory && currentMode != DrawShape) {
   //     // Call the new text handling function
   //     handleTextMousePress(event);

   //     // Call the new bitmap/image handling function
   //     handleBitmapsMousePress(event);
   if (event->button() == Qt::LeftButton && !isDrawingTrajectory && currentMode != DrawShape) {
       // Handle bitmap placement, selection, rotation, and text selection
       if (handleBitmapsMousePress(event)) return;
       handleTextMousePress(event);
   }
   if (currentMode == MeasureDistance && event->button() == Qt::LeftButton) {
       QPointF geo = gislib->canvasToGeo(event->pos());
       measurePoints.append(geo);
       if (measurePoints.size() >= 2) {
           QPointF prev = measurePoints[measurePoints.size() - 2];
           QPointF curr = measurePoints.last();
           double dist = gislib->calculateDistance(prev, curr);
           if (measureDialog) {
               measureDialog->addMeasurement(curr.x(), curr.y(), dist);
           }
       }
       Refresh();
       return;
   }
   if (event->button() == Qt::LeftButton) {
       m_tooltipTimer.stop();
       QToolTip::hideText();

       if (isBoxSelectionMode && isClickOnEmptyCanvas(event->pos())) {
           isBoxSelecting = true;
           boxStartPos = event->pos();
           boxCurrentPos = event->pos();
           clearMultiSelection();
           Refresh();
           return;
       }

       // Multi-selection drag
       if (isMultiSelecting() && isClickOnAnySelectedItem(event->pos())) {
           isMultiDrag = true;
           multiDragStartGeo = gislib->canvasToGeo(event->pos());
           setCursor(Qt::ClosedHandCursor);
           return;
       }
       if (handleBitmapsMousePress(event)) return;

        if (handleShapeSelection(event)) { return; }
        handleShapesMousePress(event);


       // Then other handlers
       handleShapesMousePress(event);
   }
   if (currentMode == MeasureDistance && event->button() == Qt::LeftButton) {
       QPointF geo = gislib->canvasToGeo(event->pos());
       measurePoints.append(geo);
       if (measurePoints.size() >= 2) {
           QPointF prev = measurePoints[measurePoints.size() - 2];
           QPointF curr = measurePoints.last();
           double dist = gislib->calculateDistance(prev, curr);
           if (measureDialog) {
               measureDialog->addMeasurement(curr.x(), curr.y(), dist);
           }
       }
       Refresh();
       return;
   }

   if (currentMode == MeasureDistance) {
       return;
   }
   if (currentMode == DrawTrajectory && isDrawingTrajectory && !selectedEntityId.empty()) {
       QPointF geoPos = gislib->canvasToGeo(event->pos());
       int nearestIndex = findNearestWaypoint(event->pos());
       if (nearestIndex >= 0) {
           selectWaypoint(nearestIndex);
           isDraggingWaypoint = true;
       } else {
           Waypoints* waypoint = new Waypoints();
           waypoint->position = new Vector(geoPos.y(), 0 ,geoPos.x());
           currentTrajectory.push_back(waypoint);
           selectWaypoint(currentTrajectory.size() - 1);
           updateTrajectoryData();
       }
       Refresh();
       return;
   }
   if (!selectedEntityId.empty()) {
       auto it = Meshes.find(selectedEntityId);
       if (it != Meshes.end()) {
           auto& entry = it->second;
           if (entry.coreTransform && entry.coreTransform->geocord) {
                      QPointF basePos = gislib->geoToCanvas(
                          entry.coreTransform->getLatitude(),
                          entry.coreTransform->getLongitude());
           const float handleTolerance = 15.0f;
           if (currentMode == Translate) {
               QPointF xAxisHandle = basePos + QPointF(50, 0);
               QPointF yAxisHandle = basePos + QPointF(0, -50);
               if (QVector2D(event->pos() - xAxisHandle).length() < handleTolerance) {
                   activeDragAxis = "x";
                   dragStartPos = event->pos();
                   emit MoveEntity(QString::fromStdString(selectedEntityId));
                   Refresh();
                   return;
               }
               if (QVector2D(event->pos() - yAxisHandle).length() < handleTolerance) {
                   activeDragAxis = "y";
                   dragStartPos = event->pos();
                   emit MoveEntity(QString::fromStdString(selectedEntityId));
                   Refresh();
                   return;
               }
           } else if (currentMode == Rotate) {
               float distFromCenter = QVector2D(event->pos() - basePos).length();
               if (qAbs(distFromCenter - 40.0f) < handleTolerance) {
                   activeDragAxis = "rotate";
                   dragStartPos = event->pos();
                   emit MoveEntity(QString::fromStdString(selectedEntityId));
                   Refresh();
                   return;
               }
           } else if (currentMode == Scale) {
               QPointF xScaleHandle = basePos + QPointF(50, 0);
               QPointF yScaleHandle = basePos + QPointF(0, 50);
               if (QVector2D(event->pos() - xScaleHandle).length() < handleTolerance) {
                   activeDragAxis = "scale-x";
                   dragStartPos = event->pos();
                   emit MoveEntity(QString::fromStdString(selectedEntityId));
                   Refresh();
                   return;
               }
               if (QVector2D(event->pos() - yScaleHandle).length() < handleTolerance) {
                   activeDragAxis = "scale-y";
                   dragStartPos = event->pos();
                   emit MoveEntity(QString::fromStdString(selectedEntityId));
                   Refresh();
                   return;
               }
           }
       }
   }
}
   // Is loop ko replace karo (function ke end ke paas wala)
   bool entityWasClicked = false;
   for (auto& [id, entry] : Meshes) {
       // ← NULL CHECKS ADD KIYE
       if (!entry.coreTransform || !entry.coreTransform->geocord) continue;
       if (!entry.entity || !entry.entity->Active) continue;

       QPointF entityPos = gislib->geoToCanvas(
           entry.coreTransform->getLatitude(),
           entry.coreTransform->getLongitude());

       if (QVector2D(event->pos() - entityPos).length() < 20.0f) {
           if (selectedEntityId != id) {
               selectedEntityId = id;
               emit selectEntitybyCursor(QString::fromStdString(id));
           }

           selectedEntityIds.clear();
           selectedShapeIds.clear();

           dragStartPos = event->pos();
           activeDragAxis = "both";
           entityWasClicked = true;
           Refresh();
           return;
       }
   }

   if (!entityWasClicked
       && currentMode != DrawTrajectory
       && currentMode != DrawShape
       && currentMode != PlaceBitmap)
   {
       isDraggingShape     = false;
       draggingShapeId     = "";
       isDraggingBitmap    = false;
       draggingBitmapId    = "";
       isDraggingUserImage = false;
       draggingUserImageId = "";
       selectedShapeIds.clear();        // ← YE IMPORTANT HAI
       m_highlightedShapeId = "";

       selectedEntityId = "";
       selectedEntityIds.clear();
       activeDragAxis = "";
       Refresh();
   }
}
void CanvasWidget::handleShapesMousePress(QMouseEvent *event) {

   // ========================================================================
   // NEW: RIGHT-CLICK to finalize Line/Polygon
   // ========================================================================
    if (currentMode == DrawShape || currentMode == PlaceBitmap) {
        if (m_layerPanel && m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
            setTransformMode(Translate);
            return;
        }
    }
   if (event->button() == Qt::RightButton && currentMode == DrawShape &&
       (selectedShape == "Line" || selectedShape == "Polygon")) {

       // Check if we have enough vertices to finalize
       bool canFinalize = false;
       if (selectedShape == "Line" && tempLineVertices.size() >= 2) {
           canFinalize = true;
       } else if (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3) {
           canFinalize = true;
       }

       if (canFinalize) {
           // Finalize the shape without adding current position as vertex
           // Use the last vertex position
           QPointF lastGeoPos;
           if (selectedShape == "Line" && !tempLineVertices.empty()) {
               Vector* lastVertex = tempLineVertices.back();
               lastGeoPos = QPointF(lastVertex->x, lastVertex->y);
           } else if (selectedShape == "Polygon" && !tempPolygonVertices.empty()) {
               Vector* lastVertex = tempPolygonVertices.back();
               lastGeoPos = QPointF(lastVertex->x, lastVertex->y);
           }

           shapesFeature->handleShapeDrawing(selectedShape, lastGeoPos, true);

           Console::log("Finalized " + selectedShape.toStdString() + " with RIGHT-CLICK");
           return;
       } else {
           // Not enough vertices - just show warning
           QString minVertices = (selectedShape == "Line") ? "2" : "3";
           Console::warning("Need at least " + minVertices.toStdString() +
                          " vertices to finalize " + selectedShape.toStdString());
           return;
       }
   }

   // ========================================================================
   // Handle drag-to-draw for Circle and Rectangle
   // ========================================================================
   if (currentMode == DrawShape && (selectedShape == "Circle" || selectedShape == "Rectangle")) {
       QPointF geoPos = gislib->canvasToGeo(event->pos());
       shapesFeature->startDragShape(selectedShape, geoPos);
       return;
   }

   // ========================================================================
   // Handle multi-point shapes (Line/Polygon)
   // LEFT-CLICK: Add vertex
   // DOUBLE-CLICK: Add vertex AND finalize
   // ========================================================================
   if (currentMode == DrawShape && (selectedShape == "Line" || selectedShape == "Polygon")) {
       QPointF geoPos = gislib->canvasToGeo(event->pos());
       bool finalize = (event->type() == QEvent::MouseButtonDblClick);
       shapesFeature->handleShapeDrawing(selectedShape, geoPos, finalize);

       if (finalize) {
           Console::log("Finalized " + selectedShape.toStdString() + " with DOUBLE-CLICK");
       }
       return;
   }

   // ========================================================================
   // Handle shape dragging selection
   // ========================================================================
   if (currentMode == Translate && !isDrawingTrajectory && currentMode != PlaceBitmap) {
       if (handleShapeSelection(event)) {
           return;
       }
   }

   // ========================================================================
   // Resize handle selection
   // ========================================================================
   if (currentMode == EditShape && !editingShapeId.isEmpty()) {
       const qreal handleTolerance = 10.0f;
       selectedHandleIndex = -1;
       for (size_t i = 0; i < resizeHandles.size(); ++i) {
           if (QVector2D(event->pos() - resizeHandles[i]).length() < handleTolerance) {
               selectedHandleIndex = i;
               isResizingShape = true;
               dragStartPos = event->pos();
               Refresh();
               return;
           }
       }
       currentMode = Translate;
       editingShapeId = "";
       selectedHandleIndex = -1;
       isResizingShape = false;
       resizeHandles.clear();
       setCursor(Qt::ArrowCursor);
       Refresh();
       return;
   }

   // ========================================================================
   // FALLBACK: DrawShape handling
   // ========================================================================
   if (currentMode == DrawShape) {
       QPointF geoPos = gislib->canvasToGeo(event->pos());
       bool finalize = (event->type() == QEvent::MouseButtonDblClick);
       shapesFeature->handleShapeDrawing(selectedShape, geoPos, finalize);
       return;
   }
}
//============================================================================
// Written by: Waris
//============================================================================
//============================================================================
// FIXED: Bitmap Selection (Waris + Grok)
//============================================================================
bool CanvasWidget::handleBitmapsMousePress(QMouseEvent *event) {
    // User Imported Images
    if (handleUserImageSelection(event)) {
        return true;
    }

    // Preset Bitmaps (Hospital, School, etc.)
    if (handleBitmapSelection(event)) {
        return true;
    }

    // Rotation Handle Click
    if (currentMode == Translate && !activeRotateId.isEmpty()) {
        const qreal tolerance = 18.0;
        for (auto& entry : tempMeshes) {
            if (entry.name != activeRotateId || entry.bitmapPath.isEmpty()) continue;
            if (!entry.position || !entry.size) continue;

            QPointF center = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            QPointF sizePoint = gislib->geoToCanvas(
                entry.position->y() + entry.size->y(),
                entry.position->x() + entry.size->x()
            );
            float w = qAbs(sizePoint.x() - center.x()) * 2;
            float h = qAbs(sizePoint.y() - center.y()) * 2;
            QRectF bbox(center.x() - w/2, center.y() - h/2, w, h);
            QPointF handle = bbox.bottomRight() + QPointF(35, 35);

            if (QVector2D(event->pos() - handle).length() < tolerance) {
                isRotatingBitmap = true;
                rotatingBitmapId = entry.name;
                rotateHandleCenter = center;
                rotateHandleStartPos = event->pos();
                initialBitmapAngle = std::atan2(event->pos().y() - center.y(),
                                              event->pos().x() - center.x());
                setCursor(Qt::PointingHandCursor);
                Refresh();
                return true;
            }
        }
    }

    // Handle bitmap placement when in PlaceBitmap mode
    if (currentMode == PlaceBitmap && !selectedBitmapType.isEmpty()) {
        QPointF geoPos = gislib->canvasToGeo(event->pos());
        QString bitmapPath = getBitmapImagePath(selectedBitmapType);
        if (bitmapPath.isEmpty()) {
            return false;
        }
        // Create a new bitmap mesh entry
        static int bitmapCounter = 0;
        MeshEntry entry;
        entry.name = QString("TempBitmap_%1").arg(bitmapCounter++);
        entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D(0.2, 0.2, 1);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.trajectory = nullptr;
        entry.collider = nullptr;
        entry.bitmapPath = bitmapPath;
        entry.text = "";
        entry.mesh = new Mesh();
        if (!entry.mesh) {
            return false;
        }
        entry.mesh->color = new QColor(Qt::white);
        entry.mesh->lineWidth = 1;
        entry.mesh->closePath = false;
        tempMeshes.push_back(entry);

        // Reset placement state and return to Translate mode
        isPlacingBitmap = false;
        selectedBitmapType = "";
        currentMode = Translate;
        setCursor(Qt::ArrowCursor);
        Refresh();
        return true;
    }

    return false;   // nothing handled
}
//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextMousePress(QMouseEvent *event) {
   // Text selection handling
   if (event->button() == Qt::LeftButton && !isDrawingTrajectory && currentMode != DrawShape) {
       if (handleTextSelection(event)) {
           return;
       }
   }
   if (currentMode == EditShape && !editingShapeId.isEmpty()) {
       const qreal handleTolerance = 10.0f;
       selectedHandleIndex = -1;
       // Find resize handles
       for (size_t i = 0; i < resizeHandles.size(); ++i) {
           if (QVector2D(event->pos() - resizeHandles[i]).length() < handleTolerance) {
               selectedHandleIndex = i;
               isResizingShape = true;
               dragStartPos = event->pos();
               Refresh();
               return;
           }
       }
       currentMode = Translate;
       editingShapeId = "";
       selectedHandleIndex = -1;
       isResizingShape = false;
       resizeHandles.clear();
       setCursor(Qt::ArrowCursor);
       Refresh();
       return;
   }
}
//============================================================================
// Written by: Waris
//============================================================================
// Checks whether a point lies within a given tolerance distance
static bool isPointNearLineSegment(const QPointF& p, const QPointF& v1, const QPointF& v2, qreal tolerance) {
   // Direction vector of the line segment
   QPointF vec = v2 - v1;
   QPointF vp = p - v1;
   qreal lenSquared = vec.x() * vec.x() + vec.y() * vec.y();
   // Squared length of the line segment
   if (lenSquared < 1e-6) {
       return QVector2D(p - v1).length() < tolerance;
   }
   // Project point onto the line segment (clamped between 0 and 1)
   qreal t = std::max<qreal>(0.0, std::min<qreal>(1.0, (vp.x() * vec.x() + vp.y() * vec.y()) / lenSquared));
   QPointF projection = v1 + t * vec;
   // Check distance from the point to the projected point
   return QVector2D(p - projection).length() < tolerance;
}

void CanvasWidget::handleRightClick(QMouseEvent *event) {
   m_tooltipTimer.stop();
   QToolTip::hideText();
   if (currentMode == DrawShape &&
          (selectedShape == "Line" || selectedShape == "Polygon"))
      {
          bool canFinalize =
              (selectedShape == "Line"    && tempLineVertices.size()    >= 2) ||
              (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3);
          if (canFinalize) {
              // Use the last placed vertex as the final geo position
              QPointF lastGeoPos;
              if (selectedShape == "Line" && !tempLineVertices.empty()) {
                  Vector* v = tempLineVertices.back();
                  lastGeoPos = QPointF(v->x, v->y);
              } else if (selectedShape == "Polygon" && !tempPolygonVertices.empty()) {
                  Vector* v = tempPolygonVertices.back();
                  lastGeoPos = QPointF(v->x, v->y);
              }
              shapesFeature->handleShapeDrawing(selectedShape, lastGeoPos, true);
          } else {
              QString minV = (selectedShape == "Line") ? "2" : "3";
              Console::warning("Need at least " + minV.toStdString() +
                               " vertices to finalize " + selectedShape.toStdString());
          }
          return;   // always consume the right-click here
      }
   // FIRST: Check if right-click is on ANY ENTITY
   // FIRST: Check if right-click is on ANY ENTITY
   bool entityClicked = false;
   for (auto& [id, entry] : Meshes) {
       if (!entry.coreTransform) {
           continue;
       }
       QPointF entityPos = gislib->geoToCanvas(entry.coreTransform->getLatitude(),
                                               entry.coreTransform->getLongitude());
       if (QVector2D(event->pos() - entityPos).length() < 20.0f) {
           selectedEntityId = id;
           entityClicked = true;
           // Create entity context menu
           QMenu contextMenu(this);
           contextMenu.setStyleSheet(
               "QMenu {"
               "    background-color: white;"
               "    color: black;"
               "    border: 1px solid #cccccc;"
               "}"
               "QMenu::item {"
               "    background-color: white;"
               "    color: black;"
               "    padding: 5px 20px;"
               "}"
               "QMenu::item:selected {"
               "    background-color: #e6e6e6;"
               "    color: black;"
               "}"
               "QMenu::item:hover {"
               "    background-color: #f0f0f0;"
               "    color: black;"
               "}"
           );
           // Check if we're in Scenario Editor or Runtime Editor
           bool isScenarioEditor = window()->windowTitle().contains("Scenario Editor", Qt::CaseInsensitive);
           if (isScenarioEditor) {
               // Scenario Editor menu items
               QAction* removeAction = contextMenu.addAction("Remove");
               connect(removeAction, &QAction::triggered, this, [=]() {
                   ScenarioEditor* editor = nullptr;
                   QWidget* w = this;
                   while (w && !editor) {
                       editor = qobject_cast<ScenarioEditor*>(w);
                       w = w->parentWidget();
                   }
                   if (editor && editor->hierarchy) {
                       auto it = editor->hierarchy->Entities.find(id);
                       if (it != editor->hierarchy->Entities.end()) {
                           QString parentId = QString::fromStdString(it->second->parentID);
                           editor->hierarchy->removeEntity(parentId, QString::fromStdString(id));
                           Meshes.erase(id);
                           Refresh();
                       }
                   }
               });
               QAction* renameAction = contextMenu.addAction("Rename");
               connect(renameAction, &QAction::triggered, this, [=]() {
                   ScenarioEditor* editor = nullptr;
                   QWidget* w = this;
                   while (w && !editor) {
                       editor = qobject_cast<ScenarioEditor*>(w);
                       w = w->parentWidget();
                   }
                   if (editor && editor->hierarchy) {
                       auto it = editor->hierarchy->Entities.find(id);
                       if (it != editor->hierarchy->Entities.end()) {
                           Entity* entity = it->second;
                           QString currentName = QString::fromStdString(entity->Name);
                           bool ok;
                           QString newName = QInputDialog::getText(nullptr, "Rename",
                                                                  "Enter new name:",
                                                                  QLineEdit::Normal,
                                                                  currentName, &ok);
                           if (ok && !newName.trimmed().isEmpty() && newName != currentName) {
                               editor->hierarchy->renameEntity(QString::fromStdString(id), newName);
                               Refresh();
                           }
                       }
                   }
               });

               // ✅ Platform-specific actions only if this is a Platform entity
               if (entry.platform) {
                   QAction* editTrajectoryAction = contextMenu.addAction("Edit Trajectory");
                   connect(editTrajectoryAction, &QAction::triggered, this, [=]() {
                       setTrajectoryDrawingMode(true);
                   });

                   QAction* setSizeAction = contextMenu.addAction("Change Entity Size");
                   connect(setSizeAction, &QAction::triggered, this, [this, id]() {
                       static bool isDialogOpen = false;
                       if (isDialogOpen) return;
                       isDialogOpen = true;
                       int currentSize = (Meshes[id].individualImageSize > 0) ?
                                         Meshes[id].individualImageSize : ImageScale;
                       QDialog* dialog = new QDialog(this->window());
                       dialog->setWindowTitle("Image Size");
                       dialog->setStyleSheet("QDialog { background-color: #0F2636; border: 2px solid #27446d; }");
                       dialog->resize(100, 10);
                       QVBoxLayout* layout = new QVBoxLayout(dialog);
                       layout->setContentsMargins(20, 20, 20, 20);
                       QLabel* label = new QLabel("Set size for this entity:", dialog);
                       QFont labelFont = label->font();
                       labelFont.setPointSize(10);
                       label->setFont(labelFont);
                       layout->addWidget(label);
                       QSpinBox* spinBox = new QSpinBox(dialog);
                       spinBox->setMinimum(10);
                       spinBox->setMaximum(1000);
                       spinBox->setValue(currentSize);
                       spinBox->setSingleStep(1);
                       spinBox->setMinimumHeight(30);
                       spinBox->setStyleSheet("QSpinBox { min-width: 200px; padding: 5px; font-size: 12px; }");
                       layout->addWidget(spinBox);
                       layout->addSpacing(10);
                       QDialogButtonBox* buttonBox = new QDialogButtonBox(
                           QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
                       layout->addWidget(buttonBox);
                       connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
                       connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
                       dialog->setLayout(layout);
                       int result = dialog->exec();
                       int newSize = spinBox->value();
                       delete dialog;
                       isDialogOpen = false;
                       if (result == QDialog::Accepted && Meshes.find(id) != Meshes.end()) {
                           Meshes[id].individualImageSize = newSize;
                           QTimer::singleShot(100, this, [this]() { this->update(); });
                       }
                   });

                   QAction* changeTrajectoryColorAction = contextMenu.addAction("Change Trajectory Color");
                   connect(changeTrajectoryColorAction, &QAction::triggered, this, [this, id]() {
                       auto it = Meshes.find(id);
                       if (it == Meshes.end()) return;
                       QColor newColor = QColorDialog::getColor(
                           it->second.trajectoryColor,
                           this->window(),
                           "Select Trajectory Color",
                           QColorDialog::DontUseNativeDialog
                       );
                       if (newColor.isValid()) {
                           it->second.trajectoryColor = newColor;
                           Refresh();
                       }
                   });
               }
           } else {
               // Runtime Editor menu items
               QAction* removeAction = contextMenu.addAction("Remove");
               connect(removeAction, &QAction::triggered, this, [=]() {
                   RuntimeEditor* editor = nullptr;
                   QWidget* w = this;
                   while (w && !editor) {
                       editor = qobject_cast<RuntimeEditor*>(w);
                       w = w->parentWidget();
                   }
                   if (editor && editor->hierarchy) {
                       auto it = editor->hierarchy->Entities.find(id);
                       if (it != editor->hierarchy->Entities.end()) {
                           QString parentId = QString::fromStdString(it->second->parentID);
                           editor->hierarchy->removeEntity(parentId, QString::fromStdString(id));
                           Meshes.erase(id);
                           Refresh();
                       }
                   }
               });
               QAction* renameAction = contextMenu.addAction("Rename");
               connect(renameAction, &QAction::triggered, this, [=]() {
                   RuntimeEditor* editor = nullptr;
                   QWidget* w = this;
                   while (w && !editor) {
                       editor = qobject_cast<RuntimeEditor*>(w);
                       w = w->parentWidget();
                   }
                   if (editor && editor->hierarchy) {
                       auto it = editor->hierarchy->Entities.find(id);
                       if (it != editor->hierarchy->Entities.end()) {
                           Entity* entity = it->second;
                           QString currentName = QString::fromStdString(entity->Name);
                           bool ok;
                           QString newName = QInputDialog::getText(nullptr, "Rename",
                                                                  "Enter new name:",
                                                                  QLineEdit::Normal,
                                                                  currentName, &ok);
                           if (ok && !newName.trimmed().isEmpty() && newName != currentName) {
                               editor->hierarchy->renameEntity(QString::fromStdString(id), newName);
                               Refresh();
                           }
                       }
                   }
               });

               // ✅ Platform-specific actions only if this is a Platform entity
               if (entry.platform) {
                   QAction* editTrajectoryAction = contextMenu.addAction("Edit Trajectory");
                   connect(editTrajectoryAction, &QAction::triggered, this, [=]() {
                       setTrajectoryDrawingMode(true);
                   });

                   QAction* setSizeAction = contextMenu.addAction("Change Entity Size");
                   connect(setSizeAction, &QAction::triggered, this, [this, id]() {
                       static bool isDialogOpen = false;
                       if (isDialogOpen) return;
                       isDialogOpen = true;
                       int currentSize = (Meshes[id].individualImageSize > 0) ?
                                         Meshes[id].individualImageSize : ImageScale;
                       QDialog* dialog = new QDialog(this->window());
                       dialog->setWindowTitle("Image Size");
                       dialog->setStyleSheet("QDialog { background-color: #0F2636; border: 2px solid #27446d; }");
                       dialog->resize(100, 10);
                       QVBoxLayout* layout = new QVBoxLayout(dialog);
                       layout->setContentsMargins(20, 20, 20, 20);
                       QLabel* label = new QLabel("Set size for this entity:", dialog);
                       QFont labelFont = label->font();
                       labelFont.setPointSize(10);
                       label->setFont(labelFont);
                       layout->addWidget(label);
                       QSpinBox* spinBox = new QSpinBox(dialog);
                       spinBox->setMinimum(10);
                       spinBox->setMaximum(1000);
                       spinBox->setValue(currentSize);
                       spinBox->setSingleStep(1);
                       spinBox->setMinimumHeight(30);
                       spinBox->setStyleSheet("QSpinBox { min-width: 200px; padding: 5px; font-size: 12px; }");
                       layout->addWidget(spinBox);
                       layout->addSpacing(10);
                       QDialogButtonBox* buttonBox = new QDialogButtonBox(
                           QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
                       layout->addWidget(buttonBox);
                       connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
                       connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
                       dialog->setLayout(layout);
                       int result = dialog->exec();
                       int newSize = spinBox->value();
                       delete dialog;
                       isDialogOpen = false;
                       if (result == QDialog::Accepted && Meshes.find(id) != Meshes.end()) {
                           Meshes[id].individualImageSize = newSize;
                           QTimer::singleShot(100, this, [this]() { this->update(); });
                       }
                   });

                   QAction* changeTrajectoryColorAction = contextMenu.addAction("Change Trajectory Color");
                   connect(changeTrajectoryColorAction, &QAction::triggered, this, [this, id]() {
                       auto it = Meshes.find(id);
                       if (it == Meshes.end()) return;
                       QColor newColor = QColorDialog::getColor(
                           it->second.trajectoryColor,
                           this->window(),
                           "Select Trajectory Color",
                           QColorDialog::DontUseNativeDialog
                       );
                       if (newColor.isValid()) {
                           it->second.trajectoryColor = newColor;
                           Refresh();
                       }
                   });


               }
           }

           // Show the entity menu and return
           contextMenu.exec(event->globalPos());
           return;
       }
   }
   // SECOND: Check if right-click is on TRAJECTORY (in DrawTrajectory mode)
   if (currentMode == DrawTrajectory && isDrawingTrajectory) {
       handleTrajectoryRightClick(event);
       return;
   }
   // THIRD: Check if right-click is on TEXT
   for (auto& entry : tempMeshes) {
       if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QPointF textPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
           QFontMetrics fm(entry.textFont);
           QRect textRect = fm.boundingRect(entry.text);
           // textRect.moveTo(textPos.x(), textPos.y());
           textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());
           QRectF expandedRect = textRect.adjusted(-5, -5, 5, 5);
           if (expandedRect.contains(event->pos())) {
               QMenu contextMenu(this);
               contextMenu.setStyleSheet(
                   "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
                   "QMenu::item { background-color: white; color: black; padding: 5px 20px; }"
                   "QMenu::item:selected { background-color: #e6e6e6; color: black; }"
                   "QMenu::item:hover { background-color: #f0f0f0; color: black; }"
               );
               QAction* editTextAction = contextMenu.addAction("Edit Text Properties");
               connect(editTextAction, &QAction::triggered, this, [=]() {
                   showTextPropertiesDialog(entry.name);
               });
               QAction* deleteAction = contextMenu.addAction("Delete Text");
               connect(deleteAction, &QAction::triggered, this, [=]() {
                   deleteText(entry.name);
               });
               contextMenu.exec(event->globalPos());
               return;
           }
       }
   }
   // FOURTH: Check if right-click is on SHAPES or BITMAPS
   bool clickedOnShape = false;
   QPointF geoPos = gislib->canvasToGeo(event->pos());
   for (const auto& entry : tempMeshes) {
       if ((entry.name.startsWith("Temp") && !entry.name.startsWith("TempText")) ||
           !entry.bitmapPath.isEmpty()) {
           // Check if click is inside the shape/bitmap
           bool isHit = false;
           // TEXT - use text bounds
           if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
               QPointF entityPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
               QFontMetrics fm(entry.textFont);
               QRect textRect = fm.boundingRect(entry.text);
               // textRect.moveTo(entityPos.x(), entityPos.y());
               textRect.moveTo(entityPos.x(), entityPos.y() - fm.ascent());
               QRect innerRect = textRect.adjusted(
                   textRect.width() * 0.125f, textRect.height() * 0.125f,
                   -textRect.width() * 0.125f, -textRect.height() * 0.125f
               );
               isHit = innerRect.contains(event->pos());
           }
           // SHAPES & BITMAPS - use rotated polygon
           else {
               QPolygonF rotatedPoly = getRotatedShapePolygon(entry);

               if (rotatedPoly.isEmpty()) continue;

               // POINT
               if (entry.name.startsWith("TempPoint")) {
                   QPointF centerCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                   float distance = QVector2D(event->pos() - centerCanvas).length();
                   isHit = (distance <= 6.0f);
               }
               // POLYLINE
               else if (entry.name.startsWith("TempPolyline")) {
                   qreal lineTolerance = 6.0f;
                   for (int i = 0; i < rotatedPoly.size() - 1; ++i) {
                       if (isPointNearLineSegment(event->pos(), rotatedPoly[i], rotatedPoly[i+1], lineTolerance)) {
                           isHit = true;
                           break;
                       }
                   }
               }
               else {
                   QPointF center(0, 0);
                   for (const QPointF& p : rotatedPoly) {
                       center += p;
                   }
                   center /= rotatedPoly.size();
                   QPolygonF scaledPoly;
                   for (const QPointF& p : rotatedPoly) {
                       QPointF offset = p - center;
                       scaledPoly << (center + offset * 0.75f);
                   }
                   isHit = scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill);
               }
           }
           if (isHit) {
               clickedOnShape = true;
               handleShapeRightClick(event);
               return;
           }
       }
   }
   // FIFTH: If nothing was clicked, show GENERAL CONTEXT MENU
   // Only show general menu if NOT on entity, trajectory, text, shape, or bitmap
   if (!entityClicked && !clickedOnShape) {
       QMenu contextMenu(this);
       contextMenu.setStyleSheet(
           "QMenu {"
           "    background-color: white;"
           "    color: black;"
           "    border: 1px solid #cccccc;"
           "}"
           "QMenu::item {"
           "    background-color: white;"
           "    color: black;"
           "    padding: 5px 20px;"
           "}"
           "QMenu::item:selected {"
           "    background-color: #e6e6e6;"
           "    color: black;"
           "}"
           "QMenu::item:hover {"
           "    background-color: #f0f0f0;"
           "    color: black;"
           "}"
       );
       QAction* boxSelectAction = contextMenu.addAction(
                  isBoxSelectionMode ? "✓ Disable Box Selection Mode" : "Enable Box Selection Mode"
              );
              connect(boxSelectAction, &QAction::triggered, this, [this]() {
                  isBoxSelectionMode = !isBoxSelectionMode;
                  setCursor(isBoxSelectionMode ? Qt::CrossCursor : Qt::ArrowCursor);
                  Console::log(isBoxSelectionMode ?
                               "Box Selection Mode ENABLED (Left Drag to select)" :
                               "Box Selection Mode DISABLED");
                  Refresh();
              });
              contextMenu.addSeparator();
       // Check if in Scenario Editor
       bool isScenarioEditor = window()->windowTitle().contains("Scenario Editor", Qt::CaseInsensitive);
       bool isRuntimeEditor = window()->windowTitle().contains("Runtime Editor", Qt::CaseInsensitive);
       if (isScenarioEditor) {
           QAction* addEntityAction = contextMenu.addAction("Add Entity");
           connect(addEntityAction, &QAction::triggered, this, [=]() {
               ScenarioEditor* editor = nullptr;
               QWidget* w = this;
               while (w && !editor) {
                   editor = qobject_cast<ScenarioEditor*>(w);
                   w = w->parentWidget();
               }
               if (editor && editor->hierarchy) {
                   AddItemDialog dialog(AddItemDialog::EntityType,
                                        "Platform",
                                        AddItemDialog::NormalMode,
                                        editor->library,
                                        nullptr,
                                        "Runtime");
                   dialog.setWindowTitle("Add Entity");
                   if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                       bool isProfileParent = true;
                       QString selectedEntityId;
                       QString progressText;
                       progressText = "Adding Entities...";
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
                       ProfileCategaory* profile = editor->hierarchy->getProfileByName("Platform");
                       QString parentId = "";
                       if(profile){
                           parentId = QString::fromStdString(profile->ID);
                       }
                       double initLat = 190;
                       double initLon = 0;
                       if (!dialog.isScenarioconfigEnabled()) {
                           QPointF geocords = gislib->canvasToGeo(event->pos());
                           initLat = geocords.y();
                           initLon = geocords.x();
                       }
                       for (int i = 0; i < dialog.getNumber(); ++i) {
                           if (progressDialog.wasCanceled()) {
                               break;
                           }
                           emit editor->treeView->getContextMenu()->addEntityRequested(parentId, dialog.getName() + QString::number(i),
                                                   isProfileParent, dialog.getComponents(), &dialog,"",initLat,
                                                                                       initLon);
                           progressDialog.setValue(i + 1);
                           QApplication::processEvents();
                       }
                       progressDialog.close();
                   }
               }
           });
       }
       contextMenu.addSeparator();
       if (isRuntimeEditor) {
           QAction* addEntityAction = contextMenu.addAction("Add Entity");
           connect(addEntityAction, &QAction::triggered, this, [=]() {
               RuntimeEditor* editor = nullptr;
               QWidget* w = this;
               while (w && !editor) {
                   editor = qobject_cast<RuntimeEditor*>(w);
                   w = w->parentWidget();
               }
               if (editor && editor->hierarchy) {
                   AddItemDialog dialog(AddItemDialog::EntityType,
                                        "Platform",
                                        AddItemDialog::NormalMode,
                                        editor->library,
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
                       ProfileCategaory* profile = editor->hierarchy->getProfileByName("Platform");
                       QString parentId = "";
                       if (profile) {
                           parentId = QString::fromStdString(profile->ID);
                       }
                       // QPointF geocords = gislib->canvasToGeo(event->pos());
                       double initLat = 190;
                       double initLon = 0;
                       for (int i = 0; i < dialog.getNumber(); ++i) {
                           if (progressDialog.wasCanceled()) {
                               break;
                           }
                           emit editor->treeView->getContextMenu()->addEntityRequested(
                               parentId,
                               dialog.getName() + QString::number(i),
                               isProfileParent,
                               dialog.getComponents(),
                               &dialog,
                               "",
                               initLat,
                               initLon
                           );
                           progressDialog.setValue(i + 1);
                           QApplication::processEvents();
                       }
                       progressDialog.close();
                   }
               }
           });
           contextMenu.addSeparator();
       }
       //=================6feb===============
       // Add shape drawing options



            QAction* addLineAction = contextMenu.addAction("Add Line");
            connect(addLineAction, &QAction::triggered, this, [=]() {
                setShapeDrawingMode(true, "Line");
                selectedShape = "Line";
                currentMode = DrawShape;
            });
            contextMenu.addSeparator();
            QAction* addCircleAction = contextMenu.addAction("Add Circle");
            connect(addCircleAction, &QAction::triggered, this, [=]() {
                setShapeDrawingMode(true, "Circle");
                selectedShape = "Circle";
                currentMode = DrawShape;
            });
            contextMenu.addSeparator();
            QAction* addRectangleAction = contextMenu.addAction("Add Rectangle");
            connect(addRectangleAction, &QAction::triggered, this, [=]() {
                setShapeDrawingMode(true, "Rectangle");
                selectedShape = "Rectangle";
                currentMode = DrawShape;
            });
            contextMenu.addSeparator();
            QAction* addPolygonAction = contextMenu.addAction("Add Polygon");
            connect(addPolygonAction, &QAction::triggered, this, [=]() {
                setShapeDrawingMode(true, "Polygon");
                selectedShape = "Polygon";
                currentMode = DrawShape;
            });
            contextMenu.addSeparator();
            QAction* addPointAction = contextMenu.addAction("Add Point");
            connect(addPointAction, &QAction::triggered, this, [=]() {
                setShapeDrawingMode(true, "Points");
                selectedShape = "Points";
                currentMode = DrawShape;
            });
            contextMenu.addSeparator();
       QAction* addTextAction = contextMenu.addAction("Add Text");
       connect(addTextAction, &QAction::triggered, this, [=]() {
           bool ok;
           QString text = QInputDialog::getText(nullptr, "Add Text", "Enter text:",
                                                QLineEdit::Normal, "", &ok);
           if (ok && !text.isEmpty()) {
               QPointF geoPos = gislib->canvasToGeo(event->pos());
               addTextAtGeo(text, geoPos);
           }
       });
       if (m_copiedShape) {
           QAction* pasteAction = contextMenu.addAction("Paste Shape");
           connect(pasteAction, &QAction::triggered, this, [this]() {
               pasteShape();
           });
           contextMenu.addSeparator();
       }

       contextMenu.exec(event->globalPos());
   }
   Refresh();
}

void CanvasWidget::handleTrajectoryRightClick(QMouseEvent *event) {
   int nearestIndex = findNearestWaypoint(event->pos());
   QMenu contextMenu(this);

   // Style sheet to keep text black always
   contextMenu.setStyleSheet(
       "QMenu {"
       "    background-color: white;"
       "    color: black;"
       "    border: 1px solid #cccccc;"
       "}"
       "QMenu::item {"
       "    background-color: white;"
       "    color: black;"
       "    padding: 5px 20px;"
       "}"
       "QMenu::item:selected {"
       "    background-color: #e6e6e6;"
       "    color: black;"
       "}"
       "QMenu::item:hover {"
       "    background-color: #f0f0f0;"
       "    color: black;"
       "}"
   );

   if (nearestIndex >= 0) {
       selectWaypoint(nearestIndex);
       QAction* editWaypointAction = contextMenu.addAction("Edit Waypoint");
       connect(editWaypointAction, &QAction::triggered, this, [=]() {
           if (nearestIndex < 0 || nearestIndex >= (int)currentTrajectory.size()) {
               return;
           }
           Waypoints* wp = currentTrajectory[nearestIndex];
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
contextMenu.addSeparator();
       QAction* deleteWaypointAction = contextMenu.addAction("Delete Waypoint");
       connect(deleteWaypointAction, &QAction::triggered, this, [=]() {
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
       contextMenu.addSeparator();
   }
   if (!currentTrajectory.empty()) {
         QAction* deleteAllWaypointsAction = contextMenu.addAction("Delete All Waypoints");
         connect(deleteAllWaypointsAction, &QAction::triggered, this, [=]() {
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
     }
     contextMenu.addSeparator();
     QAction* saveAction = contextMenu.addAction("Save");
     connect(saveAction, &QAction::triggered, this, [=]() {
         saveTrajectory();
         setTrajectoryDrawingMode(false);
         deselectWaypoint();
     });
     contextMenu.addSeparator();
     QAction* cancelAction = contextMenu.addAction("Cancel");
     connect(cancelAction, &QAction::triggered, this, [=]() {
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
     });
contextMenu.addSeparator();
   if (!contextMenu.actions().isEmpty()) {
       QAction* selectedAction = contextMenu.exec(event->globalPos());
       if (selectedAction) {
       }
   }
}
//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleShapeRightClick(QMouseEvent *event) {
   QPointF geoPos = gislib->canvasToGeo(event->pos());
   QString closestShapeId;
   qreal minDistance = std::numeric_limits<qreal>::max();
   MeshEntry* closestEntry = nullptr;
   auto closestIt = tempMeshes.end();
   for (auto it = tempMeshes.begin(); it != tempMeshes.end(); ++it) {
       MeshEntry& entry = *it;
       if (!entry.position || (!entry.mesh && entry.text.isEmpty() &&
                               !entry.name.startsWith("TempPoint") && entry.bitmapPath.isEmpty())) {
           continue;
       }
       bool isHit = false;
       qreal distance = std::numeric_limits<qreal>::max();
       // TEXT - use text bounds
       if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QPointF entityPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
           QFontMetrics fm(entry.textFont);
           QRect textRect = fm.boundingRect(entry.text);
           // textRect.moveTo(entityPos.x(), entityPos.y());
           textRect.moveTo(entityPos.x(), entityPos.y() - fm.ascent());
           QRect innerRect = textRect.adjusted(
               textRect.width() * 0.125f, textRect.height() * 0.125f,
               -textRect.width() * 0.125f, -textRect.height() * 0.125f
               );
           isHit = innerRect.contains(event->pos());
           distance = QVector2D(event->pos() - entityPos).length();
       }
       // SHAPES & BITMAPS - use rotated polygon
       else {
           QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
           if (rotatedPoly.isEmpty()) continue;
           QPointF centerCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
           // POINT
           if (entry.name.startsWith("TempPoint")) {
               distance = QVector2D(event->pos() - centerCanvas).length();
               isHit = (distance <= 6.0f);
           }
           // POLYLINE
           else if (entry.name.startsWith("TempPolyline")) {
               qreal lineTolerance = 6.0f;
               for (int i = 0; i < rotatedPoly.size() - 1; ++i) {
                   if (isPointNearLineSegment(event->pos(), rotatedPoly[i], rotatedPoly[i+1], lineTolerance)) {
                       isHit = true;
                       distance = QVector2D(event->pos() - centerCanvas).length();
                       break;
                   }
               }
           }
           else {
               QPointF polyCenter(0, 0);
               for (const QPointF& p : rotatedPoly) {
                   polyCenter += p;
               }
               polyCenter /= rotatedPoly.size();
               QPolygonF scaledPoly;
               for (const QPointF& p : rotatedPoly) {
                   QPointF offset = p - polyCenter;
                   scaledPoly << (polyCenter + offset * 0.75f);
               }
               isHit = scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill);
               distance = QVector2D(event->pos() - centerCanvas).length();
           }
       }
       if (isHit && distance < minDistance) {
           minDistance = distance;
           closestShapeId = entry.name;
           closestEntry = &entry;
           closestIt = it;
           break;
       }
   }
   if (!closestShapeId.isEmpty() && closestEntry && closestIt != tempMeshes.end()) {
       QMenu contextMenu(this);
       contextMenu.setWindowFlags(contextMenu.windowFlags() | Qt::Popup);
       contextMenu.setStyleSheet(
           "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
           "QMenu::item { background-color: white; color: black; padding: 5px 20px; }"
           "QMenu::item:selected { background-color: #0078d7; color: white; }"
           );
       // ── Copy ──────────────────────────────────────────────────────────────────
       QAction* copyAction = contextMenu.addAction("Copy Shape");
       connect(copyAction, &QAction::triggered, this, [this, closestShapeId]() {
           m_highlightedShapeId = closestShapeId;
           copySelectedShape();
       });
       if (closestShapeId.startsWith("TempText")) {
           QAction* editTextAction = contextMenu.addAction("Edit Text Properties");
           connect(editTextAction, &QAction::triggered, this, [=]() {
               showTextPropertiesDialog(closestShapeId);
           });
           QAction* deleteAction = contextMenu.addAction("Delete Text");
           connect(deleteAction, &QAction::triggered, this, [=]() {
               deleteText(closestShapeId);
           });
       }
       // SHAPES ONLY
       else if (closestShapeId.startsWith("TempPolyline") ||
                closestShapeId.startsWith("TempCircle") ||
                closestShapeId.startsWith("TempRectangle") ||
                closestShapeId.startsWith("TempPolygon") ||
                closestShapeId.startsWith("TempPoint")) {
           QAction* editAction = contextMenu.addAction("Edit");
           connect(editAction, &QAction::triggered, this, [=]() {
               currentMode = EditShape;
               editingShapeId = closestShapeId;
               setCursor(Qt::SizeAllCursor);
               resizeHandles.clear();
               QPointF centerGeo(closestEntry->position->x(), closestEntry->position->y());
               QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());
               float rotationRad = closestEntry->rotation->z();
               float cosFwd = std::cos(rotationRad);
               float sinFwd = std::sin(rotationRad);
               // RECTANGLE
               if (closestShapeId.startsWith("TempRectangle")) {
                   if (!closestEntry->mesh || closestEntry->mesh->polygen.size() != 4) {
                       return;
                   }
                   for (Vector* v : closestEntry->mesh->polygen) {
                       // Apply rotation to local vertex
                       float worldX = v->x * cosFwd - v->y * sinFwd;
                       float worldY = v->x * sinFwd + v->y * cosFwd;
                       QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                       QPointF vCanvas = gislib->geoToCanvas(vGeo.y(), vGeo.x());
                       resizeHandles.push_back(vCanvas);
                   }
               }
               // CIRCLE
               else if (closestShapeId.startsWith("TempCircle")) {
                   float radius = closestEntry->size->x();
                   float worldX = radius * cosFwd;
                   float worldY = radius * sinFwd;
                   QPointF radiusGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                   QPointF radiusCanvas = gislib->geoToCanvas(radiusGeo.y(), radiusGeo.x());
                   resizeHandles = { radiusCanvas };
               }
               // POLYGON or POLYLINE
               else if (closestShapeId.startsWith("TempPolygon") || closestShapeId.startsWith("TempPolyline")) {
                   if (!closestEntry->mesh || closestEntry->mesh->polygen.empty()) {
                       return;
                   }
                   // Calculate handles from rotated vertices
                   for (Vector* v : closestEntry->mesh->polygen) {
                       // Apply rotation to local vertex
                       float worldX = v->x * cosFwd - v->y * sinFwd;
                       float worldY = v->x * sinFwd + v->y * cosFwd;
                       // Convert to world geo
                       QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                       // Convert to canvas
                       QPointF vCanvas = gislib->geoToCanvas(vGeo.y(), vGeo.x());
                       resizeHandles.push_back(vCanvas);
                   }

               }
           }
                   );

           QAction* propertiesAction = contextMenu.addAction("Shape Properties");
           connect(propertiesAction, &QAction::triggered, this, [=]() {
               showShapePropertiesDialog(closestShapeId);
           });

           if (shapesFeature->hasHistory(closestShapeId)) {
               QAction* historyAction = contextMenu.addAction("History Preview");
               connect(historyAction, &QAction::triggered, this, [=]() {
                   shapesFeature->showHistoryPreview(closestShapeId);
               });

               // Add action to apply the preview (restore to previous state)
               QAction* applyHistoryAction = contextMenu.addAction("History Restore");
               connect(applyHistoryAction, &QAction::triggered, this, [=]() {
                   if (shapesFeature->restorePreviousState(closestShapeId, closestEntry)) {
                       shapesFeature->hideHistoryPreview();
                       Refresh();
                   } else {
                   }
               });
           }
           if (shapesFeature->isShowingPreview() && shapesFeature->getPreviewShapeId() == closestShapeId) {
               QAction* hidePreviewAction = contextMenu.addAction("Hide Preview");
               connect(hidePreviewAction, &QAction::triggered, this, [=]() {
                   shapesFeature->hideHistoryPreview();
               });
           }
           QAction* rotateAction = contextMenu.addAction("Rotate");
           connect(rotateAction, &QAction::triggered, this, [=]() {
               activeRotateId = closestShapeId;
               Refresh();
           });
           QAction* deleteAction = contextMenu.addAction("Delete Shape");
           connect(deleteAction, &QAction::triggered, this, [=]() {
               shapesFeature->clearHistory(closestShapeId);
               delete closestEntry->position;
               delete closestEntry->rotation;
               delete closestEntry->size;
               delete closestEntry->velocity;
               if (closestEntry->mesh) {
                   for (Vector* v : closestEntry->mesh->polygen) delete v;
                   delete closestEntry->mesh->color;
                   delete closestEntry->mesh;
               }
               delete closestEntry->collider;
               delete closestEntry->trajectory;
               tempMeshes.erase(closestIt);

               if (m_layerPanel)
                   m_layerPanel->removeShapeFromLayer(closestShapeId);

               Refresh();
           });
       }
       // BITMAPS ONLY
       else if (!closestEntry->bitmapPath.isEmpty()) {
           QAction* editAction = contextMenu.addAction("Edit");
           connect(editAction, &QAction::triggered, this, [=]() {
               currentMode = EditShape;
               editingShapeId = closestShapeId;
               setCursor(Qt::SizeAllCursor);
               resizeHandles.clear();
               QPointF sizePointGeo(closestEntry->position->x() + closestEntry->size->x(),
                                    closestEntry->position->y() + closestEntry->size->y());
               QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
               QPointF centerCanvas = gislib->geoToCanvas(closestEntry->position->y(), closestEntry->position->x());
               float canvasWidth = qAbs(sizePointCanvas.x() - centerCanvas.x()) * 2;
               float canvasHeight = qAbs(sizePointCanvas.y() - centerCanvas.y()) * 2;
               resizeHandles = {
                   QPointF(centerCanvas.x() - canvasWidth/2, centerCanvas.y() - canvasHeight/2),
                   QPointF(centerCanvas.x() + canvasWidth/2, centerCanvas.y() - canvasHeight/2),
                   QPointF(centerCanvas.x() + canvasWidth/2, centerCanvas.y() + canvasHeight/2),
                   QPointF(centerCanvas.x() - canvasWidth/2, centerCanvas.y() + canvasHeight/2)
               };
               Refresh();
           });
           // Add History option
           if (shapesFeature->hasHistory(closestShapeId)) {
               QAction* historyAction = contextMenu.addAction("Undo Last Change");
               connect(historyAction, &QAction::triggered, this, [=]() {
                   if (shapesFeature->restorePreviousState(closestShapeId, closestEntry)) {
                       Refresh();
                   } else {
                   }
               });
           }
           QAction* rotateAction = contextMenu.addAction("Rotate");
           connect(rotateAction, &QAction::triggered, this, [=]() {
               activeRotateId = closestShapeId;
               Refresh();
           });
           QString deleteText;
           if (closestShapeId.startsWith("TempBitmap")) {
               deleteText = "Delete Bitmap";
           } else if (closestShapeId.startsWith("UserImage_")) {
               deleteText = "Delete Image";
           } else {
               deleteText = "Delete";
           }
           QAction* deleteAction = contextMenu.addAction(deleteText);
           connect(deleteAction, &QAction::triggered, this, [=]() {


               if (m_layerPanel)
                   m_layerPanel->removeShapeFromLayer(closestShapeId);

               delete closestEntry->position;
               delete closestEntry->rotation;
               delete closestEntry->size;
               delete closestEntry->velocity;
               if (closestEntry->mesh) {
                   for (Vector* v : closestEntry->mesh->polygen) delete v;
                   delete closestEntry->mesh->color;
                   delete closestEntry->mesh;
               }
               delete closestEntry->collider;
               delete closestEntry->trajectory;
               tempMeshes.erase(closestIt);
               Refresh();
           });

       }
       contextMenu.exec(event->globalPos());
   } else {
   }
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::isPointInPolygon(const QPointF& point, const std::vector<Vector*>& vertices, const QPointF& centroidGeo, GISlib* gislib) {
   int i, j, n = vertices.size();
   bool inside = false;
   for (i = 0, j = n - 1; i < n; j = i++) {
       // Convert vertices from geo coordinates (relative to centroid) to canvas coordinates
       QPointF vi = gislib->geoToCanvas(vertices[i]->y + centroidGeo.y(), vertices[i]->x + centroidGeo.x());
       QPointF vj = gislib->geoToCanvas(vertices[j]->y + centroidGeo.y(), vertices[j]->x + centroidGeo.x());
       // Ray-casting algorithm
       if (((vi.y() > point.y()) != (vj.y() > point.y())) &&
           (point.x() < (vj.x() - vi.x()) * (point.y() - vi.y()) / (vj.y() - vi.y()) + vi.x())) {
           inside = !inside;
       }
   }
   return inside;
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextMouseMove(QMouseEvent *event) {
   // TEXT DRAGGING - PEHLE CHECK KAREN (HIGHEST PRIORITY)
   if (isEditingText && !editingTextId.isEmpty()) {
       handleTextDragging(event);
       return;
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleBitmapsMouseMove(QMouseEvent *event) {
   if (isRotatingBitmap && !rotatingBitmapId.isEmpty()) {
       for (auto& entry : tempMeshes) {
           if (entry.name == rotatingBitmapId) {
               // Calculate current mouse angle from center
               qreal dx = event->pos().x() - rotateHandleCenter.x();
               qreal dy = event->pos().y() - rotateHandleCenter.y();
               qreal currentAngle = std::atan2(dy, dx);

               // Calculate angle delta
               qreal deltaAngle = currentAngle - initialBitmapAngle;

               // Update rotation (accumulative)
               entry.rotation->setZ(entry.rotation->z() + deltaAngle);

               // Update initial angle for next iteration
               initialBitmapAngle = currentAngle;

               // Show rotation angle in console
               float angleDegrees = entry.rotation->z() * (180.0 / M_PI);
               Console::log(QString("Rotating %1: %2°")
                                .arg(entry.name)
                                .arg(angleDegrees, 0, 'f', 1).toStdString());
               Refresh();
               return;
           }
       }
       // Not found → stop rotation
       isRotatingBitmap = false;
       rotatingBitmapId.clear();
       setCursor(Qt::ArrowCursor);
   }
   if (isDraggingUserImage && !draggingUserImageId.isEmpty()) {
       handleUserImageDragging(event);
       return;
   }
   // Bitmap dragging
   if (isDraggingBitmap && !draggingBitmapId.isEmpty()) {
       handleBitmapDragging(event);
       return;
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleShapesMouseMove(QMouseEvent *event) {
      if (isMultiDrag) return;
   // First check for shape dragging
   if (isDraggingShape && !draggingShapeId.isEmpty()) {
       handleShapeDragging(event);
       return;
   }
   // NEW: Handle shape drag-to-draw preview
   if (shapesFeature && shapesFeature->isDraggingShape()) {
       QPointF currentGeoPos = gislib->canvasToGeo(event->pos());
       shapesFeature->updateDragShape(currentGeoPos);
       return;
   }
   if (currentMode == EditShape && isResizingShape && selectedHandleIndex >= 0 && !editingShapeId.isEmpty()) {
       for (auto& entry : tempMeshes) {
           if (entry.name == editingShapeId) {
               static bool resizeSaved = false;
               if (!resizeSaved) {
                   shapesFeature->saveShapeState(editingShapeId, &entry);
                   resizeSaved = true;
               }
               // *** FIX: Circle ke liye alag check — mesh zaruri nahi ***
                         if (!entry.position || !entry.size) {
                             return;
                         }

                         // *** FIX: TempCircle ke liye mesh check SKIP karo ***
                         if (!entry.name.startsWith("TempCircle") && !entry.mesh) {
                             return;
                         }
               // if (!entry.position || !entry.size || !entry.mesh) {
               //     return;
               // }
               QPointF newPos = event->pos();
               // Get fixed center position
               QPointF centerGeo(entry.position->x(), entry.position->y());
               QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

               // Get current rotation angle in radians
               float rotationRad = entry.rotation->z();

               // ==== POLYGON/POLYLINE VERTEX EDITING ====
               if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
                   if (selectedHandleIndex < (int)entry.mesh->polygen.size()) {
                       Vector* vertex = entry.mesh->polygen[selectedHandleIndex];
                       if (!vertex) {
                           return;
                       }
                       // Step 1: Convert mouse canvas position to geo coordinates
                       QPointF mouseGeo = gislib->canvasToGeo(newPos);

                       // Step 2: Calculate offset from shape center (in geo space)
                       float deltaX = mouseGeo.x() - centerGeo.x();
                       float deltaY = mouseGeo.y() - centerGeo.y();

                       // Step 3: Apply INVERSE rotation to transform from world to local space
                       float cosInv = std::cos(-rotationRad);
                       float sinInv = std::sin(-rotationRad);

                       float localX = deltaX * cosInv - deltaY * sinInv;
                       float localY = deltaX * sinInv + deltaY * cosInv;

                       // Step 4: Update vertex in local space
                       vertex->x = localX;
                       vertex->y = localY;
                       // Recalculate ALL handles with CORRECT rotation
                       resizeHandles.clear();
                       float cosFwd = std::cos(rotationRad);
                       float sinFwd = std::sin(rotationRad);
                       for (Vector* v : entry.mesh->polygen) {
                           if (!v) continue;

                           // Apply FORWARD rotation to transform from local to world space
                           float worldX = v->x * cosFwd - v->y * sinFwd;
                           float worldY = v->x * sinFwd + v->y * cosFwd;

                           // Add to center to get absolute geo coordinates
                           QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);

                           // Convert to canvas for display
                           QPointF vCanvas = gislib->geoToCanvas(vGeo.y(), vGeo.x());
                           resizeHandles.push_back(vCanvas);
                       }
                   }
               }
               // ==== RECTANGLE RESIZING ====
               else if (entry.name.startsWith("TempRectangle")) {
                   // Calculate mouse delta in canvas space
                   QPointF mouseDelta = newPos - centerCanvas;
                   // Apply inverse rotation to get local coordinates
                   float cosInv = std::cos(-rotationRad);
                   float sinInv = std::sin(-rotationRad);
                   float localX = mouseDelta.x() * cosInv - mouseDelta.y() * sinInv;
                   float localY = mouseDelta.x() * sinInv + mouseDelta.y() * cosInv;
                   // Get scale factors (geo to canvas conversion)
                   float currentHalfW = entry.size->x() / 2.0f;
                   float currentHalfH = entry.size->y() / 2.0f;
                   QPointF refPointGeo(centerGeo.x() + currentHalfW, centerGeo.y() + currentHalfH);
                   QPointF refPointCanvas = gislib->geoToCanvas(refPointGeo.y(), refPointGeo.x());
                   float canvasScaleX = std::abs(refPointCanvas.x() - centerCanvas.x()) / currentHalfW;
                   float canvasScaleY = std::abs(refPointCanvas.y() - centerCanvas.y()) / currentHalfH;
                   // Calculate new dimensions
                   float newHalfW = std::abs(localX) / canvasScaleX;
                   float newHalfH = std::abs(localY) / canvasScaleY;
                   const float minGeoSize = 0.0001f;
                   newHalfW = std::max(newHalfW, minGeoSize);
                   newHalfH = std::max(newHalfH, minGeoSize);

                   entry.size->setX(newHalfW * 2.0f);
                   entry.size->setY(newHalfH * 2.0f);

                   // Update vertices in LOCAL space
                   for (Vector* v : entry.mesh->polygen) delete v;
                   entry.mesh->polygen.clear();

                   entry.mesh->polygen = {
                       new Vector(-newHalfW, newHalfH, 0),   // Top-left
                       new Vector(newHalfW, newHalfH, 0),    // Top-right
                       new Vector(newHalfW, -newHalfH, 0),   // Bottom-right
                       new Vector(-newHalfW, -newHalfH, 0)   // Bottom-left
                   };

                   // Recalculate handles with rotation applied
                   resizeHandles.clear();
                   float cosFwd = std::cos(rotationRad);
                   float sinFwd = std::sin(rotationRad);

                   for (Vector* v : entry.mesh->polygen) {
                       // Transform from local to world space with rotation
                       float worldX = v->x * cosFwd - v->y * sinFwd;
                       float worldY = v->x * sinFwd + v->y * cosFwd;

                       // Convert to absolute geo coordinates
                       QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);

                       // Convert to canvas and store
                       resizeHandles.push_back(gislib->geoToCanvas(vGeo.y(), vGeo.x()));
                   }
               }
               // ==== BITMAP RESIZING ====
               else if (!entry.bitmapPath.isEmpty()) {
                   QPointF mouseDelta = newPos - centerCanvas;

                   float cosInv = std::cos(-rotationRad);
                   float sinInv = std::sin(-rotationRad);
                   float localX = mouseDelta.x() * cosInv - mouseDelta.y() * sinInv;
                   float localY = mouseDelta.x() * sinInv + mouseDelta.y() * cosInv;

                   float currentHalfW = entry.size->x() / 2.0f;
                   float currentHalfH = entry.size->y() / 2.0f;

                   QPointF refPointGeo(centerGeo.x() + currentHalfW, centerGeo.y() + currentHalfH);
                   QPointF refPointCanvas = gislib->geoToCanvas(refPointGeo.y(), refPointGeo.x());
                   float canvasScaleX = std::abs(refPointCanvas.x() - centerCanvas.x()) / currentHalfW;
                   float canvasScaleY = std::abs(refPointCanvas.y() - centerCanvas.y()) / currentHalfH;

                   float newHalfW = std::abs(localX) / canvasScaleX;
                   float newHalfH = std::abs(localY) / canvasScaleY;

                   const float minGeoSize = 0.0001f;
                   newHalfW = std::max(newHalfW, minGeoSize);
                   newHalfH = std::max(newHalfH, minGeoSize);

                   entry.size->setX(newHalfW * 2.0f);
                   entry.size->setY(newHalfH * 2.0f);

                   //FIX: Recalculate corner handles with rotation
                   resizeHandles.clear();
                   QVector<QPointF> localCorners = {
                       QPointF(-newHalfW, newHalfH),
                       QPointF(newHalfW, newHalfH),    // Top-right
                       QPointF(newHalfW, -newHalfH),   // Bottom-right
                       QPointF(-newHalfW, -newHalfH)   // Bottom-left
                   };

                   float cosFwd = std::cos(rotationRad);
                   float sinFwd = std::sin(rotationRad);

                   for (const QPointF& corner : localCorners) {
                       // Apply rotation
                       float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                       float worldY = corner.x() * sinFwd + corner.y() * cosFwd;

                       // Convert to geo
                       QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);

                       // Convert to canvas
                       resizeHandles.push_back(gislib->geoToCanvas(cGeo.y(), cGeo.x()));
                   }
               }

               if (m_layerPanel && entry.bitmapPath.isEmpty() == false) {
                   m_layerPanel->updateRasterLayerFromShape(entry.name);
               }
               // ==== CIRCLE RESIZING ====
               else if (entry.name.startsWith("TempCircle")) {
                   float newCanvasRadius = QVector2D(newPos - centerCanvas).length();
                   newCanvasRadius = std::max(10.0f, newCanvasRadius);
                   QPointF radiusPointCanvas = centerCanvas + QPointF(newCanvasRadius, 0);
                   QPointF radiusPointGeo = gislib->canvasToGeo(radiusPointCanvas);
                   QPointF centerGeoCalc = gislib->canvasToGeo(centerCanvas);
                   float newGeoRadius = qAbs(radiusPointGeo.x() - centerGeoCalc.x());
                   entry.size->setX(newGeoRadius);
                   entry.size->setY(newGeoRadius);
                     resizeHandles = { radiusPointCanvas };
               }
               dragStartPos = newPos;
               Refresh();
               return;
           }
       }
       return;
   }
}

//============================================================================
// Written by: Waris
//============================================================================
// Updates resize handle positions based on the current rotation
void CanvasWidget::updateResizeHandlesForRotation(const QString& shapeId) {
   // Iterate through all temporary mesh entries to find the target shape
   for (auto& entry : tempMeshes) {
       if (entry.name == shapeId) {
           // Validate required transform data
           if (!entry.position || !entry.size) return;
           // Shape center in geographic coordinates
           QPointF centerGeo(entry.position->x(), entry.position->y());
           float rotationRad = entry.rotation->z();
           resizeHandles.clear();
           // Handle rectangles and bitmap-based shapes
           if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) {
               // Half dimensions of the shape
               float halfW = entry.size->x() / 2.0f;
               float halfH = entry.size->y() / 2.0f;
               QVector<QPointF> localCorners = {
                   QPointF(-halfW, halfH),
                   QPointF(halfW, halfH),
                   QPointF(halfW, -halfH),
                   QPointF(-halfW, -halfH)
               };
               float cosFwd = std::cos(rotationRad);
               float sinFwd = std::sin(rotationRad);
               for (const QPointF& corner : localCorners) {
                   float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                   float worldY = corner.x() * sinFwd + corner.y() * cosFwd;
                   QPointF cornerGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                   // Convert world coordinates to canvas coordinates
                   resizeHandles.push_back(gislib->geoToCanvas(cornerGeo.y(), cornerGeo.x()));
               }
           }
           // Handle polygons and polylines
           else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
               if (!entry.mesh || entry.mesh->polygen.empty()) {
                   return;
               }
               float cosFwd = std::cos(rotationRad);
               float sinFwd = std::sin(rotationRad);
               for (Vector* v : entry.mesh->polygen) {
                   if (!v) continue;
                   // Transform from local to world space
                   float worldX = v->x * cosFwd - v->y * sinFwd;
                   float worldY = v->x * sinFwd + v->y * cosFwd;
                   QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                   resizeHandles.push_back(gislib->geoToCanvas(vGeo.y(), vGeo.x()));
               }
           }
           else if (entry.name.startsWith("TempCircle")) {
               QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
               resizeHandles = { gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x()) };
           }

           break;
       }
   }
}

// void CanvasWidget::handleMouseMove(QMouseEvent *event)
// {

//     if (isBoxSelecting)
//     {
//         boxCurrentPos = event->pos();
//         Refresh();
//         return;
//     }

//     if (isMultiDrag)
//     {
//         QPointF currentGeo = gislib->canvasToGeo(event->pos());
//         QPointF delta = currentGeo - multiDragStartGeo;
//         moveSelectedItems(delta);
//         multiDragStartGeo = currentGeo;
//         Refresh();
//         return;  // ← ZARURI HAI - baaki kuch mat chalao
//     }

//     if (!gislib) return;

//     checkEntityHover(event->pos());

//     if (boxZoomPending) {
//         QPoint delta = event->pos() - boxZoomStart;
//         if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
//             isBoxZooming = true;
//             boxZoomPending = false;
//         }
//     }

//     if (isBoxZooming) {
//         boxZoomCurrent = event->pos();
//         Refresh();
//         return;
//     }

//     // ↓↓↓ SIRF TAB CHALAO JAB MULTI-DRAG NAHI HAI ↓↓↓
//     if (!isMultiDrag) {
//         handleTextMouseMove(event);
//         handleBitmapsMouseMove(event);
//         handleShapesMouseMove(event);
//     }

//     if (isPanning) {
//         QPoint delta = event->pos() - lastMousePos;
//         canvasOffset += delta;
//         lastMousePos = event->pos();
//         Refresh();
//         return;
//     }

//     if (currentMode == MeasureDistance) return;

//     if (isDrawingTrajectory && isDraggingWaypoint && selectedWaypointIndex >= 0 &&
//         selectedWaypointIndex < (int)currentTrajectory.size()) {
//         QPointF geoPos = gislib->canvasToGeo(event->pos());
//         Waypoints* wp = currentTrajectory[selectedWaypointIndex];
//         wp->position->z = geoPos.x();
//         wp->position->x = geoPos.y();
//         Refresh();
//         return;
//     }

//     if (Meshes.find(selectedEntityId) == Meshes.end()) return;
//     auto& entry = Meshes[selectedEntityId];
//     if (!entry.position || !entry.size || !entry.rotation) return;
//     if (!entry.coreTransform || !entry.coreTransform->geocord) return;

//     float dx = event->pos().x() - entry.coreTransform->getLongitude() - canvasOffset.x();
//     float dy = event->pos().y() - entry.coreTransform->getLatitude() - canvasOffset.y();

//     if (!selectedEntityId.empty() && !activeDragAxis.isEmpty()) {
//         if (isMultiDrag) return;  // extra safety guard

//         if (currentMode == Translate) {
//             QPointF newGeoPos = gislib->canvasToGeo(event->pos());
//             if (activeDragAxis == "x") {
//                 entry.coreTransform->setLongitude(newGeoPos.x());
//             } else if (activeDragAxis == "y") {
//                 entry.coreTransform->setLatitude(newGeoPos.y());
//             } else if (activeDragAxis == "both") {
//                 entry.coreTransform->setGeoCord(newGeoPos.y(), newGeoPos.x());
//             }
//             emit MoveEntity(QString::fromStdString(selectedEntityId));
//         } else if (currentMode == Rotate) {
//             QPointF basePos = gislib->geoToCanvas(
//                 entry.coreTransform->getLatitude(),
//                 entry.coreTransform->getLongitude());
//             qreal angle_new = qAtan2(event->pos().y() - basePos.y(),
//                                      event->pos().x() - basePos.x());
//             qreal angle_old = qAtan2(dragStartPos.y() - basePos.y(),
//                                      dragStartPos.x() - basePos.x());
//             float angle_change = -qRadiansToDegrees(angle_new - angle_old);
//             entry.transform->setRotation(QQuaternion::fromEulerAngles(
//                 QVector3D(0, entry.transform->rotation().toEulerAngles().y() - angle_change, 0)));
//             emit MoveEntity(QString::fromStdString(selectedEntityId));
//         } else if (currentMode == Scale) {
//             QPointF basePos = gislib->geoToCanvas(
//                 entry.coreTransform->getLatitude(),
//                 entry.coreTransform->getLongitude());
//             qreal dist_new = QVector2D(event->pos() - basePos).length();
//             qreal dist_old = QVector2D(dragStartPos - basePos).length();
//             qreal scaleFactor = dist_new / dist_old;
//             if (activeDragAxis == "x") {
//                 QVector3D sc(0, 0, entry.transform->scale3D().z() * scaleFactor);
//                 entry.transform->setScale3D(sc);
//             } else if (activeDragAxis == "y") {
//                 QVector3D sc(entry.transform->scale3D().x() * scaleFactor, 0, 0);
//                 entry.transform->setScale3D(sc);
//             } else if (activeDragAxis == "both") {
//                 QVector3D sc(entry.transform->scale3D().x() * scaleFactor, 0,
//                              entry.transform->scale3D().z() * scaleFactor);
//                 entry.transform->setScale3D(sc);
//             }
//             emit MoveEntity(QString::fromStdString(selectedEntityId));
//         }
//         dragStartPos = event->pos();
//         Refresh();
//     }
// }
void CanvasWidget::handleMouseMove(QMouseEvent *event)
{
    if (isBoxSelecting)
    {
        boxCurrentPos = event->pos();
        Refresh();
        return;
    }

    if (isMultiDrag)
    {
        QPointF currentGeo = gislib->canvasToGeo(event->pos());
        QPointF delta = currentGeo - multiDragStartGeo;
        moveSelectedItems(delta);           // ← Isme trajectory bhi move hogi
        multiDragStartGeo = currentGeo;
        Refresh();
        return;
    }

    if (!gislib) return;

    checkEntityHover(event->pos());

    if (boxZoomPending) {
        QPoint delta = event->pos() - boxZoomStart;
        if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
            isBoxZooming = true;
            boxZoomPending = false;
        }
    }

    if (isBoxZooming) {
        boxZoomCurrent = event->pos();
        Refresh();
        return;
    }

    // ↓↓↓ SIRF TAB CHALAO JAB MULTI-DRAG NAHI HAI ↓↓↓
    if (!isMultiDrag) {
        handleTextMouseMove(event);
        handleBitmapsMouseMove(event);
        handleShapesMouseMove(event);
    }

    if (isPanning) {
        QPoint delta = event->pos() - lastMousePos;
        canvasOffset += delta;
        lastMousePos = event->pos();
        Refresh();
        return;
    }

    if (currentMode == MeasureDistance) return;

    if (isDrawingTrajectory && isDraggingWaypoint && selectedWaypointIndex >= 0 &&
        selectedWaypointIndex < (int)currentTrajectory.size()) {
        QPointF geoPos = gislib->canvasToGeo(event->pos());
        Waypoints* wp = currentTrajectory[selectedWaypointIndex];
        wp->position->z = geoPos.x();
        wp->position->x = geoPos.y();
        Refresh();
        return;
    }

    // ===================== SINGLE ENTITY MOVEMENT =====================
    if (!selectedEntityId.empty() && !activeDragAxis.isEmpty())
    {
        if (isMultiDrag) return;   // Safety

        auto& entry = Meshes[selectedEntityId];
        if (!entry.coreTransform || !entry.coreTransform->geocord) return;

        if (currentMode == Translate)
        {
            QPointF newGeoPos = gislib->canvasToGeo(event->pos());

            if (activeDragAxis == "x") {
                entry.coreTransform->setLongitude(newGeoPos.x());
            }
            else if (activeDragAxis == "y") {
                entry.coreTransform->setLatitude(newGeoPos.y());
            }
            else if (activeDragAxis == "both") {
                // 🔥 IMPORTANT: Single drag mein sirf entity move hogi
                // Trajectory move NHI hogi
                entry.coreTransform->setGeoCord(newGeoPos.y(), newGeoPos.x());
            }
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        }
        else if (currentMode == Rotate) {
            QPointF basePos = gislib->geoToCanvas(
                entry.coreTransform->getLatitude(),
                entry.coreTransform->getLongitude());
            qreal angle_new = qAtan2(event->pos().y() - basePos.y(),
                                     event->pos().x() - basePos.x());
            qreal angle_old = qAtan2(dragStartPos.y() - basePos.y(),
                                     dragStartPos.x() - basePos.x());
            float angle_change = -qRadiansToDegrees(angle_new - angle_old);
            entry.transform->setRotation(QQuaternion::fromEulerAngles(
                QVector3D(0, entry.transform->rotation().toEulerAngles().y() - angle_change, 0)));
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        }
        else if (currentMode == Scale) {
            QPointF basePos = gislib->geoToCanvas(
                entry.coreTransform->getLatitude(),
                entry.coreTransform->getLongitude());
            qreal dist_new = QVector2D(event->pos() - basePos).length();
            qreal dist_old = QVector2D(dragStartPos - basePos).length();
            qreal scaleFactor = dist_new / dist_old;
            if (activeDragAxis == "x") {
                QVector3D sc(0, 0, entry.transform->scale3D().z() * scaleFactor);
                entry.transform->setScale3D(sc);
            } else if (activeDragAxis == "y") {
                QVector3D sc(entry.transform->scale3D().x() * scaleFactor, 0, 0);
                entry.transform->setScale3D(sc);
            } else if (activeDragAxis == "both") {
                QVector3D sc(entry.transform->scale3D().x() * scaleFactor, 0,
                             entry.transform->scale3D().z() * scaleFactor);
                entry.transform->setScale3D(sc);
            }
            emit MoveEntity(QString::fromStdString(selectedEntityId));
        }

        dragStartPos = event->pos();
        Refresh();
    }
}
void CanvasWidget::handleShapesMouseRelease(QMouseEvent *event) {
   if (event->button() == Qt::LeftButton) {
       if (isBoxSelecting)
           {
               isBoxSelecting = false;
               performBoxSelection(boxStartPos, boxCurrentPos);
               Refresh();
               return;
           }

           if (isMultiDrag)
           {
               isMultiDrag = false;
               setCursor(Qt::ArrowCursor);
               return;
           }
       // NEW: Finalize drag-to-draw shape
       if (shapesFeature && shapesFeature->isDraggingShape()) {
           shapesFeature->finalizeDragShape();
           return;
       }

       if (isDraggingShape) {
           stopShapeDragging();
           static QString lastDraggedShape = "";
           lastDraggedShape = "";
       }

       isResizingShape = false;
       selectedHandleIndex = -1;
       static bool resizeSaved = false;
       resizeSaved = false;
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextMouseRelease(QMouseEvent *event) {
   if (event->button() == Qt::LeftButton) {
       // TEXT DRAGGING STOP
       if (isEditingText && !editingTextId.isEmpty()) {
           stopTextDragging();
       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleBitmapsMouseRelease(QMouseEvent *event) {
   if (event->button() == Qt::LeftButton) {
       if (isRotatingBitmap) {


           isRotatingBitmap = false;
           rotatingBitmapId.clear();
           activeRotateId.clear();
           setCursor(Qt::ArrowCursor);
           Refresh();
           return;
       }

       // Dono alag stop karo
       if (isDraggingUserImage) {
           stopUserImageDragging();
       }

       //Stop bitmap dragging (works for both types)
       if (isDraggingBitmap) {
           stopBitmapDragging();
       }
   }
}

void CanvasWidget::handleMouseRelease(QMouseEvent *event) {
   // Box-zoom: Execute zoom on release
   // Box-zoom: Execute zoom on release - EXACT BOX ZOOM
   if (event->button() == Qt::LeftButton && isBoxZooming) {
       QRect zoomBox = QRect(boxZoomStart, boxZoomCurrent).normalized();

       // Minimum box size check - at least 10x10 pixels
       if (zoomBox.width() > 10 && zoomBox.height() > 10) {
           // Convert ALL FOUR CORNERS to geographic coordinates
           QPointF topLeftGeo = gislib->canvasToGeo(zoomBox.topLeft());
           QPointF topRightGeo = gislib->canvasToGeo(zoomBox.topRight());
           QPointF bottomLeftGeo = gislib->canvasToGeo(zoomBox.bottomLeft());
           QPointF bottomRightGeo = gislib->canvasToGeo(zoomBox.bottomRight());

           // Calculate EXACT geographic bounds of the selection box
           // Find min/max longitude (X coordinate in geo)
           double minLon = qMin(qMin(topLeftGeo.x(), topRightGeo.x()),
                               qMin(bottomLeftGeo.x(), bottomRightGeo.x()));
           double maxLon = qMax(qMax(topLeftGeo.x(), topRightGeo.x()),
                               qMax(bottomLeftGeo.x(), bottomRightGeo.x()));

           // Find min/max latitude (Y coordinate in geo)
           double minLat = qMin(qMin(topLeftGeo.y(), topRightGeo.y()),
                               qMin(bottomLeftGeo.y(), bottomRightGeo.y()));
           double maxLat = qMax(qMax(topLeftGeo.y(), topRightGeo.y()),
                               qMax(bottomLeftGeo.y(), bottomRightGeo.y()));

           // IMPORTANT: Use zoomOffset = 0 for EXACT zoom to selected area
           // Negative values zoom in more, positive values zoom out
           // 0 = exactly fit the selected box
           gislib->fitToBounds(minLat, minLon, maxLat, maxLon, 0);

           QString logMsg = QString("Box-zoom: Exact zoom to selected area [Lat: %1 to %2, Lon: %3 to %4]")
                       .arg(minLat, 0, 'f', 6)
                       .arg(maxLat, 0, 'f', 6)
                       .arg(minLon, 0, 'f', 6)
                       .arg(maxLon, 0, 'f', 6);

       } else {
       }
       // Reset box-zoom state
       isBoxZooming = false;
       boxZoomPending = false;
       Refresh();
       return;
   }
   if (boxZoomPending) {
       boxZoomPending = false;
   }
   if (event->button() == Qt::LeftButton) {
       // Call the new bitmap/image handling function
       handleBitmapsMouseRelease(event);

       // Call the new shape handling function
       handleShapesMouseRelease(event);

       // Call the new text handling function
       handleTextMouseRelease(event);

       // Common state cleanup
       activeDragAxis = "";
       isDraggingWaypoint = false;
       selectEntity = false;
       updateTrajectoryData();
   }

   if (currentMode == MeasureDistance) {
       return;
   }
   if (event->button() == Qt::MiddleButton) {
       isPanning = false;
       setCursor(Qt::ArrowCursor);

   }
   Refresh();
}

//============================================================================
// Written by: Waris
// Handles keyboard input for shape editing and shape finalization
//============================================================================
void CanvasWidget::handleShapesKeyPress(QKeyEvent *event) {

   if (event->key() == Qt::Key_Escape && shapesFeature && shapesFeature->isDraggingShape()) {
       shapesFeature->cancelDragShape();
       setShapeDrawingMode(false, "");
       return;
   }
   // ESC key: cancel or finalize current shape operation
   if (event->key() == Qt::Key_Escape) {
       // Exit Edit mode and reset resize stat
       if (currentMode == EditShape) {
           currentMode = Translate;
           editingShapeId = "";
           selectedHandleIndex = -1;
           isResizingShape = false;
           resizeHandles.clear();
           setCursor(Qt::ArrowCursor);
           Refresh();
           return;
       }

       // Handle shape finalization or cancellation in Draw mode
       if (currentMode == DrawShape) {
           // Finalize polygon if minimum vertices are present
           if (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3) {
                // Create and store finalized polygon mesh
               MeshEntry entry;
               entry.name = QString("TempPolygon_%1").arg(shapesFeature->getPolygonCounter());
                // Compute centroid of polygon
               float avgX = 0, avgY = 0;
               for (const Vector* v : tempPolygonVertices) {
                   avgX += v->x;
                   avgY += v->y;
               }
               avgX /= tempPolygonVertices.size();
               avgY /= tempPolygonVertices.size();

               // Initialize transform and mesh data
               entry.position = new QVector3D(avgX, avgY, 0);
               entry.rotation = new QQuaternion();
               float minX = std::numeric_limits<float>::max();
               float maxX = std::numeric_limits<float>::lowest();
               float minY = std::numeric_limits<float>::max();
               float maxY = std::numeric_limits<float>::lowest();
               for (const Vector* v : tempPolygonVertices) {
                   minX = std::min(minX, v->x);
                   maxX = std::max(maxX, v->x);
                   minY = std::min(minY, v->y);
                   maxY = std::max(maxY, v->y);
               }
               entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
               entry.velocity = new QVector3D(0, 0, 0);
               entry.trajectory = nullptr;
               entry.collider = nullptr;
               entry.mesh = new Mesh();
               if (!entry.mesh) {

                   return;
               }
               entry.mesh->color = new QColor(Qt::red);
               if (!entry.mesh->color) {

                   delete entry.mesh;
                   return;
               }
               entry.mesh->lineWidth = 2;
               entry.mesh->closePath = true;
               for (Vector* v : tempPolygonVertices) {
                   entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
               }
               tempMeshes.push_back(entry);
               shapesFeature->addShapeToActiveLayer(entry.name, "Polygon");
               for (Vector* v : tempPolygonVertices) {
                   delete v;
               }
               tempPolygonVertices.clear();
               tempPolygonCanvasPoints.clear();
               QString logMsg = QString("Finalized polygon with %1 vertices via Escape key, centroid: (x: %2, y: %3)")
                                    .arg(entry.mesh->polygen.size())
                                    .arg(avgX)
                                    .arg(avgY);
               setShapeDrawingMode(false, "");
               Refresh();
           }
           // Finalize polyline if minimum vertices are present
           else if (selectedShape == "Line" && tempLineVertices.size() >= 2) {
               static int polylineCounter = 0;
               MeshEntry entry;
               entry.name = QString("TempPolyline_%1").arg(polylineCounter++);
               float avgX = 0, avgY = 0;
               for (const Vector* v : tempLineVertices) {
                   avgX += v->x;
                   avgY += v->y;
               }
               avgX /= tempLineVertices.size();
               avgY /= tempLineVertices.size();
               entry.position = new QVector3D(avgX, avgY, 0);
               entry.rotation = new QQuaternion();
               entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
               entry.velocity = new QVector3D(0, 0, 0);
               entry.trajectory = nullptr;
               entry.collider = nullptr;
               entry.mesh = new Mesh();
               if (!entry.mesh) {
                   return;
               }
               entry.mesh->color = new QColor(Qt::red);
               if (!entry.mesh->color) {

                   delete entry.mesh;
                   return;
               }
               entry.mesh->lineWidth = 2;
               entry.mesh->closePath = false;
               for (Vector* v : tempLineVertices) {
                   entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
               }
               tempMeshes.push_back(entry);
               shapesFeature->addShapeToActiveLayer(entry.name, "Line");
               for (Vector* v : tempLineVertices) {
                   delete v;
               }
               tempLineVertices.clear();
               tempLineCanvasPoints.clear();
               QString logMsg = QString("Finalized polyline %1 with %2 vertices via Escape key")
                                    .arg(entry.name)
                                    .arg(entry.mesh->polygen.size());

               setShapeDrawingMode(false, "");
               Refresh();
           } else if (selectedShape == "Polygon" || selectedShape == "Line") {
               for (Vector* v : tempPolygonVertices) {
                   delete v;
               }
               tempPolygonVertices.clear();
               tempPolygonCanvasPoints.clear();
               for (Vector* v : tempLineVertices) {
                   delete v;
               }
               tempLineVertices.clear();
               tempLineCanvasPoints.clear();
               setShapeDrawingMode(false, "");

               Refresh();
           }
       }
   } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
       if (currentMode == DrawShape && (!tempPolygonCanvasPoints.empty() || !tempLineCanvasPoints.empty())) {
           if (selectedShape == "Polygon" && tempPolygonVertices.size() >= 3) {
               MeshEntry entry;
               // entry.name = "TempPolygon";
               entry.name = QString("TempPolygon_%1").arg(shapesFeature->getPolygonCounter());  //WITH COUNTER
               float avgX = 0, avgY = 0;
               for (const Vector* v : tempPolygonVertices) {
                   avgX += v->x;
                   avgY += v->y;
               }
               avgX /= tempPolygonVertices.size();
               avgY /= tempPolygonVertices.size();
               entry.position = new QVector3D(avgX, avgY, 0);
               entry.rotation = new QQuaternion();
               float minX = std::numeric_limits<float>::max();
               float maxX = std::numeric_limits<float>::lowest();
               float minY = std::numeric_limits<float>::max();
               float maxY = std::numeric_limits<float>::lowest();
               for (const Vector* v : tempPolygonVertices) {
                   minX = std::min(minX, v->x);
                   maxX = std::max(maxX, v->x);
                   minY = std::min(minY, v->y);
                   maxY = std::max(maxY, v->y);
               }
               entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
               entry.velocity = new QVector3D(0, 0, 0);
               entry.trajectory = nullptr;
               entry.collider = nullptr;
               entry.mesh = new Mesh();
               if (!entry.mesh) {
                   return;
               }
               entry.mesh->color = new QColor(Qt::red);
               if (!entry.mesh->color) {
                   delete entry.mesh;
                   return;
               }
               entry.mesh->lineWidth = 2;
               entry.mesh->closePath = true;
               for (Vector* v : tempPolygonVertices) {
                   entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
               }
               tempMeshes.push_back(entry);
               shapesFeature->addShapeToActiveLayer(entry.name, "Polygon");

               for (Vector* v : tempPolygonVertices) {
                   delete v;
               }
               tempPolygonVertices.clear();
               tempPolygonCanvasPoints.clear();
               QString logMsg = QString("Finalized polygon with %1 vertices via Enter key, centroid: (x: %2, y: %3)")
                                    .arg(entry.mesh->polygen.size())
                                    .arg(avgX)
                                    .arg(avgY);
               setShapeDrawingMode(false, "");
               Refresh();
           } else if (selectedShape == "Line" && tempLineVertices.size() >= 2) {
               static int polylineCounter = 0;
               MeshEntry entry;
               entry.name = QString("TempPolyline_%1").arg(polylineCounter++);
               float avgX = 0, avgY = 0;
               for (const Vector* v : tempLineVertices) {
                   avgX += v->x;
                   avgY += v->y;
               }
               avgX /= tempLineVertices.size();
               avgY /= tempLineVertices.size();
               entry.position = new QVector3D(avgX, avgY, 0);
               entry.rotation = new QQuaternion();
               entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
               entry.velocity = new QVector3D(0, 0, 0);
               entry.trajectory = nullptr;
               entry.collider = nullptr;
               entry.mesh = new Mesh();
               if (!entry.mesh) {
                   return;
               }

               entry.mesh->color = new QColor(Qt::red);
               if (!entry.mesh->color) {
                   delete entry.mesh;
                   return;
               }
               entry.mesh->lineWidth = 2;
               entry.mesh->closePath = false;
               for (Vector* v : tempLineVertices) {
                   entry.mesh->polygen.push_back(new Vector(v->x - avgX, v->y - avgY, 0));
               }
               tempMeshes.push_back(entry);
               shapesFeature->addShapeToActiveLayer(entry.name, "Line");

               for (Vector* v : tempLineVertices) {
                   delete v;
               }
               tempLineVertices.clear();
               tempLineCanvasPoints.clear();
               QString logMsg = QString("Finalized polyline %1 with %2 vertices via Enter key")
                                    .arg(entry.name)
                                    .arg(entry.mesh->polygen.size());

               setShapeDrawingMode(false, "");
               Refresh();
           }
       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleBitmapsKeyPress(QKeyEvent *event) {
   // NEW: Cancel bitmap dragging with Escape key (works for both types)
   if (event->key() == Qt::Key_Escape && isDraggingBitmap) {
       stopBitmapDragging();
       return;
   }
   if (event->key() == Qt::Key_Escape) {
       if (!activeRotateId.isEmpty()) {
           activeRotateId.clear();

           Refresh();
           return;
       }
       if (currentMode == PlaceBitmap) {
           isPlacingBitmap = false;
           selectedBitmapType = "";
           setTransformMode(Translate);

       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextKeyPress(QKeyEvent *event) {
}
void CanvasWidget::handleKeyPress(QKeyEvent *event) {
   // Call the new bitmap/image handling function
   handleBitmapsKeyPress(event);

   // Call the new shape handling function
   handleShapesKeyPress(event);

   // Call the new text handling function
   handleTextKeyPress(event);

   // Ctrl+C → copy, Ctrl+V → paste
   if (event->modifiers() & Qt::ControlModifier) {
       if (event->key() == Qt::Key_C) {
           copySelectedShape();
           return;
       }
       if (event->key() == Qt::Key_V) {
           pasteShape();
           return;
       }
   }

   // ── Delete: Entity delete ─────────────────────────────────────────
   if (event->key() == Qt::Key_Delete && !selectedEntityId.empty()
       && currentMode != DrawTrajectory) {
       auto it = Meshes.find(selectedEntityId);
       if (it != Meshes.end() && it->second.entity) {
           QString entityId = QString::fromStdString(selectedEntityId);
           QString parentId = QString::fromStdString(it->second.entity->parentID);
           std::string idToErase = selectedEntityId;

           selectedEntityId.clear();
           selectedEntityIds.clear();

           ScenarioEditor* se = nullptr;
           RuntimeEditor*  re = nullptr;
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

   // ── F2: Rename entity ─────────────────────────────────────────────
   if (event->key() == Qt::Key_F2 && !selectedEntityId.empty()) {
       auto it = Meshes.find(selectedEntityId);
       if (it != Meshes.end() && it->second.entity) {
           QString currentName =
               QString::fromStdString(it->second.entity->Name);

           QInputDialog dialog;
           dialog.setWindowTitle("Rename");
           dialog.setLabelText("Enter new name:");
           dialog.setTextValue(currentName);
           dialog.setWindowModality(Qt::ApplicationModal);
           dialog.setWindowFlags(
               Qt::Dialog |
               Qt::WindowTitleHint |
               Qt::WindowCloseButtonHint);

           if (dialog.exec() == QDialog::Accepted) {
               QString newName = dialog.textValue();
               if (!newName.trimmed().isEmpty() && newName != currentName) {
                   ScenarioEditor* se = nullptr;
                   RuntimeEditor*  re = nullptr;
                   QWidget* w = this;
                   while (w) {
                       if (!se) se = qobject_cast<ScenarioEditor*>(w);
                       if (!re) re = qobject_cast<RuntimeEditor*>(w);
                       w = w->parentWidget();
                   }
                   if (se && se->hierarchy)
                       se->hierarchy->renameEntity(
                           QString::fromStdString(selectedEntityId), newName);
                   else if (re && re->hierarchy)
                       re->hierarchy->renameEntity(
                           QString::fromStdString(selectedEntityId), newName);
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
                if(entry.platform && entry.platform->Active){
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
                if(entry.platform && entry.platform->Active){
                     Platform* entity = entry.platform;
                    if(!entry.radioVisible) return;
                    if (!entity) return;
                    if (!entity->radios) return;
                    bool ok;
                    QString text = QInputDialog::getText(this, tr("Input Box Title"),
                                                         tr("Apna naam likhein:"), QLineEdit::Normal,
                                                         "Default Text", &ok);
                    if (ok && !text.isEmpty()) {
                        for (auto const& pair : *entity->radios->radios) {
                            Radio* s = pair.second;
                            s->sendMsg(text.toStdString());
                        }
                    }
                }
            }
        }
       return;
   }

   if (event->key() == Qt::Key_Escape && currentMode == MeasureDistance) {
       setTransformMode(Translate);
       return;
   }

   if (event->key() == Qt::Key_Escape) {
       if (currentMode == DrawTrajectory && isDrawingTrajectory) {
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
                           newWaypoint->position = new Vector(wp->position->x, wp->position->y, wp->position->z);
                           newWaypoint->speed = wp->speed;
                           currentTrajectory.push_back(newWaypoint);
                       }
                   }
               }
           }
           setTrajectoryDrawingMode(false);
           deselectWaypoint();
           return;
       }
   }
   else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
       if (currentMode == DrawTrajectory && isDrawingTrajectory) {
           saveTrajectory();
           setTrajectoryDrawingMode(false);
           deselectWaypoint();
           return;
       }
   }
   else if (event->key() == Qt::Key_5) {
       setTransformMode(MeasureDistance);
   } else if (event->key() == Qt::Key_1) {
       setTransformMode(Translate);
   } else if (event->key() == Qt::Key_2) {
       setTransformMode(Rotate);
   } else if (event->key() == Qt::Key_3) {
       setTransformMode(Scale);
   } else if (event->key() == Qt::Key_4) {
       simulate = !simulate;
   } else if (event->key() == Qt::Key_Delete && currentMode == DrawTrajectory &&
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

 void CanvasWidget::handleShapesPaint(QPainter& painter){


     if (isBoxSelecting)
         {
             painter.save();
             painter.setPen(QPen(Qt::yellow, 4, Qt::DashLine));   // Red thick border for testing
             painter.setBrush(QColor(255, 0, 0, 80));
             QRectF rect(boxStartPos, boxCurrentPos);
             painter.drawRect(rect.normalized());
             painter.restore();

             // Console mein bhi print karo
             // Console::log("Box Selecting Painting Active!");
             return;   // ← Temporarily sirf debug box dikhao
         }
   // Keep existing temporary preview code unchanged...
   if (!tempPolygonVertices.empty()) {
       QPolygonF previewPolygon;
       for (const Vector* vertex : tempPolygonVertices) {
           QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
           previewPolygon << canvasPoint;
       }
       painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
       painter.setBrush(QColor(255, 0, 0, 50));
       if (previewPolygon.size() > 1) {
           painter.drawPolyline(previewPolygon);
           for (const QPointF& point : previewPolygon) {
               painter.drawEllipse(point, 3, 3);
           }
       }
       if (previewPolygon.size() >= 3) {
           painter.drawPolygon(previewPolygon);
       }
   }

   if (!tempLineVertices.empty()) {
       QPolygonF previewLine;
       for (const Vector* vertex : tempLineVertices) {
           QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
           previewLine << canvasPoint;
       }
       painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
       painter.setBrush(Qt::NoBrush);
       if (previewLine.size() == 1) {
           painter.drawEllipse(previewLine[0], 3, 3);
       } else if (previewLine.size() >= 2) {
           painter.drawPolyline(previewLine);
           for (const QPointF& point : previewLine) {
               painter.drawEllipse(point, 3, 3);
           }
       }
   }

   // MODIFIED: Draw selection outline with visibility check
   if (isDraggingShape && !draggingShapeId.isEmpty()) {
       // NEW: Check visibility before drawing
       if (shouldDrawShape(draggingShapeId)) {
           for (const auto& entry : tempMeshes) {
               if (entry.name == draggingShapeId) {
                   QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
                   if (!rotatedPoly.isEmpty()) {
                       painter.save();
                       QPen pen(Qt::green, 2, Qt::DashLine);
                       painter.setPen(pen);
                       painter.setBrush(Qt::NoBrush);
                       if (entry.name.startsWith("TempPolyline")) {
                           painter.drawPolyline(rotatedPoly);
                       } else {
                           painter.drawPolygon(rotatedPoly);
                       }
                       painter.restore();
                   }
                   break;
               }
           }
       }
   }

   // MODIFIED: Edit mode handles with visibility check
   if (currentMode == EditShape && !editingShapeId.isEmpty()) {
       // NEW: Check visibility before drawing edit handles
       if (shouldDrawShape(editingShapeId)) {
           painter.save();
           for (size_t i = 0; i < resizeHandles.size(); ++i) {
               painter.setPen(QPen(Qt::blue, 2));
               painter.setBrush(Qt::blue);
               painter.drawRect(QRectF(resizeHandles[i] - QPointF(4, 4), QSizeF(8, 8)));

               if (i == selectedHandleIndex) {
                   painter.setPen(QPen(Qt::yellow, 3));
                   painter.setBrush(Qt::NoBrush);
                   painter.drawRect(QRectF(resizeHandles[i] - QPointF(6, 6), QSizeF(12, 12)));
               }
           }
           painter.restore();
       }
   }
   // Keep history preview unchanged
   if (shapesFeature && shapesFeature->isShowingPreview()) {
       shapesFeature->drawHistoryPreview(painter, gislib);
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleBitmapsPaint(QPainter& painter) {
   // Draw rotation handle only for the active rotating shape
   for (const auto& entry : tempMeshes) {
       if (entry.name == activeRotateId) {
           drawRotationHandle(painter, entry);
       }
   }
   // Draw selection outline for dragged bitmap WITH ROTATION
   if (isDraggingBitmap && !draggingBitmapId.isEmpty()) {
       for (const auto& entry : tempMeshes) {
           if (entry.name == draggingBitmapId && !entry.bitmapPath.isEmpty()) {
               QPointF centerGeo(entry.position->x(), entry.position->y());
               QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());
               float rotationRad = entry.rotation->z();

               // Calculate corner positions with rotation
               float halfW = entry.size->x() / 2.0f;
               float halfH = entry.size->y() / 2.0f;

               QVector<QPointF> localCorners = {
                   QPointF(-halfW, -halfH),  // Top-left
                   QPointF(halfW, -halfH),   // Top-right
                   QPointF(halfW, halfH),    // Bottom-right
                   QPointF(-halfW, halfH)    // Bottom-left
               };

               QPolygonF rotatedRect;
               float cosFwd = std::cos(rotationRad);
               float sinFwd = std::sin(rotationRad);

               for (const QPointF& corner : localCorners) {
                   // Apply rotation
                   float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                   float worldY = corner.x() * sinFwd + corner.y() * cosFwd;

                   // Convert to geo
                   QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);

                   // Convert to canvas
                   QPointF cCanvas = gislib->geoToCanvas(cGeo.y(), cGeo.x());
                   rotatedRect << cCanvas;
               }
               painter.save();
               QPen pen(Qt::yellow, 2, Qt::DashLine);
               painter.setPen(pen);
               painter.setBrush(Qt::NoBrush);
               painter.drawPolygon(rotatedRect);
               painter.restore();
               break;
           }
       }
   }

   // Draw selection outline for dragged user image WITH ROTATION
   if (isDraggingUserImage && !draggingUserImageId.isEmpty()) {
       for (const auto& entry : tempMeshes) {
           if (entry.name == draggingUserImageId && !entry.bitmapPath.isEmpty()) {
               QPointF centerGeo(entry.position->x(), entry.position->y());
               QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());
               float rotationRad = entry.rotation->z();
               // Calculate corner positions with rotation
               float halfW = entry.size->x() / 2.0f;
               float halfH = entry.size->y() / 2.0f;
               QVector<QPointF> localCorners = {
                   QPointF(-halfW, -halfH),
                   QPointF(halfW, -halfH),
                   QPointF(halfW, halfH),
                   QPointF(-halfW, halfH)
               };
               QPolygonF rotatedRect;
               float cosFwd = std::cos(rotationRad);
               float sinFwd = std::sin(rotationRad);
               for (const QPointF& corner : localCorners) {
                   float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
                   float worldY = corner.x() * sinFwd + corner.y() * cosFwd;

                   QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
                   QPointF cCanvas = gislib->geoToCanvas(cGeo.y(), cGeo.x());
                   rotatedRect << cCanvas;
               }
               painter.save();
               QPen pen(Qt::magenta, 2, Qt::DashLine);
               painter.setPen(pen);
               painter.setBrush(Qt::NoBrush);
               painter.drawPolygon(rotatedRect);
               painter.restore();
               break;
           }
       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextPaint(QPainter& painter) {
}

void CanvasWidget::handlePaint(QPaintEvent *event)
{
    QElapsedTimer timer;
    timer.start();

    // ── Entity info dialog update ─────────────────────────────────────
    if (!selectedEntityId.empty()) {
        auto it = Meshes.find(selectedEntityId);

        bool valid = (it != Meshes.end())
                  && (it->second.entity       != nullptr)
                  && (it->second.entity->Active)
                  && (it->second.coreTransform != nullptr)
                  && (it->second.coreTransform->geocord != nullptr);

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
        if (!entry.entity
            || !entry.coreTransform
            || !entry.coreTransform->geocord) {
            staleIds.push_back(id);
        }
    }
    for (const auto& sid : staleIds) {
        Meshes.erase(sid);
    }
    for (auto& [id, entry] : Meshes) {
        if (!entry.entity)              continue;
        if (!entry.entity->Active)      continue;
        if (!entry.coreTransform)       continue;
        if (!entry.coreTransform->geocord) continue;

        drawTrajectory      (painter, id, entry);
        drawTrail           (painter, id, entry);
        drawRadar           (painter, id, entry);
        drawRadio           (painter, id, entry);
        drawImage           (painter, id, entry);
        drawCollision       (painter, id, entry);
        drawCollider        (painter, id, entry);
        drawSelectionOutline(painter);
    }

    drawMesh(painter);
    drawSelectionOutline(painter);
    gislib->renderAirbases(painter);

    handleShapesPaint (painter);
    handleBitmapsPaint(painter);
    handleTextPaint   (painter);

    if (shapesFeature && shapesFeature->isDraggingShape()) {
        shapesFeature->drawDragPreview(painter, gislib);
    }

    // ── Measure distance ──────────────────────────────────────────────
    if (currentMode == MeasureDistance && !measurePoints.empty()) {
        painter.save();
        QPen pen(Qt::yellow, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPolygonF poly;
        for (const auto& geo : measurePoints) {
            QPointF canvas = gislib->geoToCanvas(geo.y(), geo.x());
            poly << canvas;
            painter.setBrush(Qt::yellow);
            painter.drawEllipse(canvas, 5, 5);
            painter.setBrush(Qt::NoBrush);
        }
        if (poly.size() > 1) painter.drawPolyline(poly);

        painter.setFont(QFont("Arial", 10));
        painter.setPen(Qt::black);
        double factor = measureDialog
                        ? measureDialog->getCurrentConversionFactor() : 1.0;
        QString unit  = measureDialog
                        ? measureDialog->getCurrentUnitString() : "m";
        for (int i = 1; i < measurePoints.size(); ++i) {
            double dist = gislib->calculateDistance(
                              measurePoints[i-1], measurePoints[i]);
            QPointF mid = (poly[i-1] + poly[i]) / 2.0;
            painter.drawText(mid + QPointF(10, -10),
                QString("%1 %2").arg(dist * factor, 0, 'f', 2).arg(unit));
        }
        painter.restore();
    }

    // ── Trajectory preview ────────────────────────────────────────────
    if (isDrawingTrajectory && !currentTrajectory.empty()) {
        painter.save();
        QColor drawColor = Qt::magenta;
        painter.setPen(QPen(drawColor, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QPolygonF polyline;

        for (size_t i = 0; i < currentTrajectory.size(); ++i) {
            Waypoints* wp = currentTrajectory[i];
            if (!wp || !wp->position) continue;
            QPointF screenPos = gislib->geoToCanvas(
                wp->position->x, wp->position->z);
            polyline << screenPos;
            painter.setBrush(drawColor);
            int ps = ((int)i == selectedWaypointIndex) ? 6 : 4;
            painter.setPen(((int)i == selectedWaypointIndex)
                ? QPen(Qt::yellow, 2) : QPen(drawColor, 1));
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
                QPointF p1    = polyline[i];
                QPointF p2    = polyline[i + 1];
                QPointF mid   = (p1 + p2) / 2.0f;
                float   angle = std::atan2(p2.y()-p1.y(), p2.x()-p1.x());
                QPointF tip   = mid + QPointF(std::cos(angle)*arrowSize,
                                              std::sin(angle)*arrowSize);
                QPointF left  = mid + QPointF(
                    std::cos(angle+M_PI*0.75f)*arrowSize*0.6f,
                    std::sin(angle+M_PI*0.75f)*arrowSize*0.6f);
                QPointF rght  = mid + QPointF(
                    std::cos(angle-M_PI*0.75f)*arrowSize*0.6f,
                    std::sin(angle-M_PI*0.75f)*arrowSize*0.6f);
                QPolygonF arr;
                arr << tip << left << rght;
                painter.drawPolygon(arr);
            }
        }
        painter.restore();
    }
    drawSceneInformation(painter);
    drawEntityInformation(painter);
    drawTransformGizmo(painter);

    // ── Box zoom ─────
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
            painter.drawRect(p.x()-hs/2, p.y()-hs/2, hs, hs);
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
   // --- Data Retrieval and Drawing ---
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
       // Draw text and increment Y offset
       painter.drawText(startPos + QPointF(0, yOffset), text);
       yOffset += 15; // Vertical spacing between lines
   }
   if (showFPS) {
       QString fpsText = QString("FPS: %1").arg(fps*10);
       painter.drawText(QPointF(10, 20), fpsText);
   }
   painter.restore();
}
void CanvasWidget::drawEntityInformation(QPainter& painter) {
   if (!showInformation || simulate || selectedEntityId.empty()) return;
   auto it = Meshes.find(selectedEntityId);
   if (it == Meshes.end()) return;

   painter.save();
   painter.resetTransform();

   QString name = it->second.name;
   auto& pos = it->second.position;
   QString text;

   if (currentMode == Translate || currentMode == DrawTrajectory) {
       text = QString("pos: lon %1, lat %2")
       .arg(pos->x(), 0, 'f', 4) // Longitude
           .arg(pos->y(), 0, 'f', 4); // Latitude
   } else if (currentMode == Rotate) {
       auto& rot = Meshes[selectedEntityId].rotation;
       text = QString("rot: x %1, y %2, z %3")
                  .arg(rot->toEulerAngles().x())
                  .arg(rot->toEulerAngles().y())
                  .arg(rot->toEulerAngles().z());
   } else if (currentMode == Scale) {
       auto& size = Meshes[selectedEntityId].size;
       text = QString("size: x %1, y %2, z %3")
                  .arg(size->x())
                  .arg(size->y())
                  .arg(size->z());
   }
   if (currentMode == DrawTrajectory) {
       text += QString(" | Waypoints: %1").arg(currentTrajectory.size());
   }
   QFont font("Arial", 10, QFont::Bold);
   painter.setFont(font);
   painter.setPen(QColor(255, 255, 255, 200));
   // Fixed screen coordinates ka upyog karein
   painter.drawText(QPointF(10, height() - 50), name);
   painter.drawText(QPointF(10, height() - 30), text);
   painter.drawText(QPointF(10, height() - 10), QString("Mode: %1")
                                                    .arg(currentMode == Translate ? "Translate" : currentMode == Rotate ? "Rotate" : currentMode == Scale ? "Scale" : "Draw Trajectory"));

   painter.restore();
}
void CanvasWidget::drawTransformGizmo(QPainter& painter) {
   if (simulate || currentMode == DrawTrajectory || selectedEntityId.empty()) return;
   auto it = Meshes.find(selectedEntityId);
   if (it == Meshes.end()) return;
   auto& entry = it->second;
   //QPointF base = gislib->geoToCanvas(entry.transform->translation().x(), entry.transform->translation().z());
   if (!entry.coreTransform || !entry.coreTransform->geocord) return;
   QPointF base = gislib->geoToCanvas(
       entry.coreTransform->getLatitude(),
       entry.coreTransform->getLongitude());   QPointF xAxisHandlePos = base + QPointF(50, 0);
   QPointF yAxisHandlePos = base + QPointF(0, -50);
   if (currentMode == Translate) {
       painter.setPen(QPen(Qt::red, (activeDragAxis == "x") ? 4 : 2));
       painter.drawLine(base, xAxisHandlePos);
       painter.setPen(QPen(Qt::blue, (activeDragAxis == "y") ? 4 : 2));
       painter.drawLine(base, yAxisHandlePos);
   } else if (currentMode == Rotate) {
       painter.setPen(QPen(QColor(255, 255, 0, 150), 2, Qt::DashLine));
       painter.setBrush(Qt::NoBrush);
       painter.drawEllipse(base, 40, 40);
       float radius = 40;
       float rad = qDegreesToRadians(entry.transform->rotation().toEulerAngles().y()+90);
       QPointF endpoint(base.x() + radius * sin(rad), base.y() - radius * cos(rad));
       painter.drawLine(base, endpoint);
   } else if (currentMode == Scale) {
       painter.setPen(QPen(Qt::red, 2));
       painter.setBrush(Qt::red);
       painter.drawRect(QRectF(base + QPointF(45, -3), QSizeF(10, 6)));
       painter.setBrush(Qt::green);
       painter.drawRect(QRectF(base + QPointF(-3, 45), QSizeF(6, 10)));
   }
}

void CanvasWidget::drawSelectionOutline(QPainter& painter) {
    if (!showOutline) return;

    // ── 1. ENTITIES ka outline ────────────────────────────────────────
    std::vector<std::string> entitiesToOutline;
    if (!selectedEntityIds.empty()) {
        entitiesToOutline = selectedEntityIds;
    } else if (!selectedEntityId.empty()) {
        entitiesToOutline.push_back(selectedEntityId);
    }

    for (const auto& entityId : entitiesToOutline) {
        if (Meshes.find(entityId) == Meshes.end()) continue;
        auto& entry = Meshes[entityId];
        if (!entry.coreTransform || !entry.coreTransform->geocord) continue;

        QPointF point = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude());

        float w = 55.0f, h = 55.0f;
        if (entry.transform) {
            float sz = entry.transform->scale3D().z();
            float sx = entry.transform->scale3D().x();
            if (sz > 0.001f) w = sz * 55.0f;
            if (sx > 0.001f) h = sx * 55.0f;
        }

        QRectF outlineRect(point.x() - w/2.0f, point.y() - h/2.0f, w, h);
        QPen pen(Qt::red);
        pen.setWidth(2);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(outlineRect);
    }

    // ── 2. SHAPES ka outline ──────────────────────────────────────────
    if (!selectedShapeIds.empty()) {
        QPen shapePen(Qt::red);
        shapePen.setWidth(2);
        shapePen.setStyle(Qt::DashLine);
        painter.setPen(shapePen);
        painter.setBrush(Qt::NoBrush);

        for (const QString& name : selectedShapeIds) {
            for (const auto& entry : tempMeshes) {
                if (entry.name != name) continue;
                if (!shouldDrawShape(entry.name)) continue;

                QPolygonF poly = getRotatedShapePolygon(entry);
                if (poly.isEmpty()) continue;

                painter.save();
                painter.setPen(shapePen);
                painter.setBrush(Qt::NoBrush);

                if (entry.name.startsWith("TempPolyline")) {
                    painter.drawPolyline(poly);
                } else if (entry.name.startsWith("TempPoint")) {
                    QPointF center = gislib->geoToCanvas(
                        entry.position->y(), entry.position->x());
                    painter.drawEllipse(center, 8, 8);
                } else {
                    painter.drawPolygon(poly);
                }
                painter.restore();
                break;
            }
        }
    }
}void CanvasWidget::drawRadar(QPainter& painter, std::string id, MeshEntry entry) {
   Platform* entity = entry.platform;
   if (!entry.detection) return;
   if (!entity) return;
   if (!entity->sensors) return;
   for (auto const& pair : *entity->sensors->sensors) {
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

           float worldLeftEdge  = platformHeading + minAz;   // e.g. 0 + (-60) = -60°
           float worldRightEdge = platformHeading + maxAz;   // e.g. 0 + ( 60) = +60°
           float sectorSpan     = maxAz - minAz;             // always 120°

           // Use left edge to compute pixel radius
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
   if (!entity->radios) return;

   for (auto const& pair : *entity->radios->radios) {
       Radio* s = pair.second;
       QPointF point = gislib->geoToCanvas(entry.coreTransform->getLatitude(), entry.coreTransform->getLongitude());

       float centerX = point.x();
       float centerY = point.y();
       QString  msg = QString::fromStdString(s->msg);
       if(!msg.isEmpty()){
        QFont font("Arial", 5, QFont::Bold);
        painter.setFont(font);
        painter.setPen(QColor(0, 0, 0, 255));
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
           if(s->targets[i].entity && s->targets[i].entity->transform){
                QPointF points2 = gislib->geoToCanvas(s->targets[i].entity->transform->getLatitude(), s->targets[i].entity->transform->getLongitude());
                painter.drawLine(centerX, centerY, points2.x(), points2.y());
           }
           }catch(...){

           }
       }
   }
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

void CanvasWidget::drawCollider(QPainter& painter,std::string id , MeshEntry entry) {
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

}

void CanvasWidget::drawMesh(QPainter& painter) {
    if (!showMesh) return;
    if (!painter.isActive()) return;

    for (const auto& entry : tempMeshes) {
        if (!shouldDrawShape(entry.name)) continue;
        Mesh* mesh = entry.mesh;
        if (!mesh || !mesh->color) continue;

        // ─────────────────────────────────────────────────────────────────
        // 1. TEXT
        // ─────────────────────────────────────────────────────────────────
        if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
            QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            painter.save();

            QFont font = entry.textFont;
            font.setPointSize(entry.textSize);
            painter.setFont(font);
            painter.setPen(entry.textColor);

            if (entry.isTextSelected) {
                QFontMetrics fm(font);
                QRect textRect = fm.boundingRect(entry.text);
                textRect.moveTo(canvasPos.x(), canvasPos.y() - fm.ascent());
                painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(textRect.adjusted(-2, -2, 2, 2));
                drawTextResizeHandles(painter, entry);
            }
            painter.drawText(canvasPos, entry.text);
            painter.restore();

            // Highlight if needed
            if (entry.name == m_highlightedShapeId) {
                painter.save();
                painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                QPolygonF outline = getRotatedShapePolygon(entry);
                if (!outline.isEmpty()) {
                    if (entry.name.startsWith("TempPolyline"))
                        painter.drawPolyline(outline);
                    else
                        painter.drawPolygon(outline);
                }
                painter.restore();
            }
            continue;
        }

        // ─────────────────────────────────────────────────────────────────
        // 2. BITMAP IMAGES
        // ─────────────────────────────────────────────────────────────────
        if (!entry.bitmapPath.isEmpty()) {
            QPixmap img(entry.bitmapPath);
            if (!img.isNull()) {
                QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                QPointF sizePointGeo(entry.position->x() + entry.size->x(), entry.position->y() + entry.size->y());
                QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
                float canvasWidth = qAbs(sizePointCanvas.x() - canvasPos.x()) * 2;
                float canvasHeight = qAbs(sizePointCanvas.y() - canvasPos.y()) * 2;
                QPixmap scaled = img.scaled(canvasWidth, canvasHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                painter.save();
                painter.translate(canvasPos);
                painter.rotate(entry.rotation->z() * (180.0 / M_PI));
                painter.drawPixmap(QPointF(-scaled.width() / 2.0f, -scaled.height() / 2.0f), scaled);
                painter.restore();
            }

            // Highlight if needed
            if (entry.name == m_highlightedShapeId) {
                painter.save();
                painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                QPolygonF outline = getRotatedShapePolygon(entry);
                if (!outline.isEmpty()) {
                    if (entry.name.startsWith("TempPolyline"))
                        painter.drawPolyline(outline);
                    else
                        painter.drawPolygon(outline);
                }
                painter.restore();
            }
            continue;
        }

        // ─────────────────────────────────────────────────────────────────
        // 3. POINT
        // ─────────────────────────────────────────────────────────────────
        if (entry.name.startsWith("TempPoint")) {
            if (mesh->polygen.empty()) continue;
            QPointF canvasPoint = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            painter.setBrush(*mesh->color);
            painter.drawEllipse(canvasPoint, 3, 3);

            if (entry.name == m_highlightedShapeId) {
                 painter.save();
                 painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
                 painter.setBrush(Qt::NoBrush);
                 painter.drawEllipse(canvasPoint, 8, 8);
                 painter.restore();
             }
             continue;
         }
        // ─────────────────────────────────────────────────────────────────
        // 4. CIRCLE
        // ─────────────────────────────────────────────────────────────────
        if (entry.name.startsWith("TempCircle")) {
            QPointF canvasCenter = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
            QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
            float canvasRadius = sqrt(pow(radiusPointCanvas.x() - canvasCenter.x(), 2) +
                                      pow(radiusPointCanvas.y() - canvasCenter.y(), 2));

            if (canvasRadius > 0 && canvasRadius < 10000) {
                painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
                QColor fillColor = *mesh->color;
                fillColor.setAlpha(50);
                painter.setBrush(fillColor);
                painter.drawEllipse(canvasCenter, canvasRadius, canvasRadius);
            }

            if (entry.name == m_highlightedShapeId) {
                painter.save();
                painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                QPolygonF outline = getRotatedShapePolygon(entry);
                if (!outline.isEmpty()) {
                    if (entry.name.startsWith("TempPolyline"))
                        painter.drawPolyline(outline);
                    else
                        painter.drawPolygon(outline);
                }
                painter.restore();
            }
            continue;
        }

        // ─────────────────────────────────────────────────────────────────
        // 5. POLYGON / RECTANGLE / POLYLINE
        // ─────────────────────────────────────────────────────────────────
        QPolygonF polygon;
        QPointF centerGeo(entry.position->x(), entry.position->y());
        QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

        float rotationRad = entry.rotation->z();
        float cosA = std::cos(rotationRad);
        float sinA = std::sin(rotationRad);

        for (Vector* point : mesh->polygen) {
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

        if (!polygon.isEmpty()) {
            painter.setPen(QPen(*mesh->color, mesh->lineWidth, Qt::SolidLine));
            if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempRectangle")) {
                QColor fillColor = *mesh->color;
                fillColor.setAlpha(50);
                painter.setBrush(fillColor);
                painter.drawPolygon(polygon);
            } else {
                painter.setBrush(Qt::NoBrush);
                painter.drawPolyline(polygon);
            }
        }

        // Highlight if needed
        if (entry.name == m_highlightedShapeId) {
            painter.save();
            painter.setPen(QPen(Qt::yellow, 3, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            QPolygonF outline = getRotatedShapePolygon(entry);
            if (!outline.isEmpty()) {
                if (entry.name.startsWith("TempPolyline"))
                    painter.drawPolyline(outline);
                else
                    painter.drawPolygon(outline);
            }
            painter.restore();
        }
    }
    // end for

    // ─────────────────────────────────────────────────────────────────
    // PREVIEW DRAWING (temporary polygons/lines during creation)
    // ─────────────────────────────────────────────────────────────────
    if (!tempPolygonVertices.empty()) {
        QPolygonF previewPolygon;
        for (const Vector* vertex : tempPolygonVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewPolygon << canvasPoint;
        }
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(QColor(255, 0, 0, 50));
        if (previewPolygon.size() > 1) {
            painter.drawPolyline(previewPolygon);
            for (const QPointF& point : previewPolygon) {
                painter.drawEllipse(point, 3, 3);
            }
        }
        if (previewPolygon.size() >= 3) {
            painter.drawPolygon(previewPolygon);
        }
    }

    if (!tempLineVertices.empty()) {
        QPolygonF previewLine;
        for (const Vector* vertex : tempLineVertices) {
            QPointF canvasPoint = gislib->geoToCanvas(vertex->y, vertex->x);
            previewLine << canvasPoint;
        }
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        if (previewLine.size() == 1) {
            painter.drawEllipse(previewLine[0], 3, 3);
        } else if (previewLine.size() >= 2) {
            painter.drawPolyline(previewLine);
            for (const QPointF& point : previewLine) {
                painter.drawEllipse(point, 3, 3);
            }
        }
    }
}
//============================================================================
// Written by: Waris
//============================================================================


void CanvasWidget::drawImage(QPainter& painter, std::string id, MeshEntry entry)
{
    if (!showImage) return;
    if (!entry.entity || !entry.entity->Active) return;
    if (!entry.coreTransform) return;
    if (!entry.coreTransform->geocord) return;
    if (!cacheimg || cacheimg->isNull()) return;

    double lat = entry.coreTransform->geocord->latitude;
    double lon = entry.coreTransform->geocord->longitude;

    QPointF point = gislib->geoToCanvas(lat, lon);
    float angle = 0.0f;
    if (entry.transform) {
        angle = entry.transform->rotation().toEulerAngles().y();
    } else if (entry.coreTransform) {
        angle = entry.coreTransform->geocord->Heading;
    }

    int finalScale = (entry.individualImageSize > 0) ? entry.individualImageSize : ImageScale;

    QPixmap* pixToUse = nullptr;
    if (entry.mesh) {
        pixToUse = entry.mesh->getPixmap(finalScale);
    }
    if (!pixToUse || pixToUse->isNull()) {
        pixToUse = &scaledimg;
    }

    painter.save();
    painter.translate(point.x(), point.y());
    painter.rotate(angle);
    painter.drawPixmap(QPointF(-pixToUse->width() / 2.0f, -pixToUse->height() / 2.0f), *pixToUse);
    painter.restore();
}


void CanvasWidget::drawFormation(QPainter& painter, std::string id, MeshEntry entry) {
   if (!showImage) return;
   if(!entry.platform)return;
   DynamicModel* dynamic = entry.platform->dynamicModel;
   if (dynamic && dynamic->formation) {
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

        for (const auto& pair : *dynamic->formation->formationPositions) {
            Vector offset = *pair.second->Offset;
            QVector3D real = entry.coreTransform->transformPoint(QVector3D(offset.x*30.f,offset.y,offset.z*30.f));
            GeoPos cord = flatXYZToGeo(real.x(),real.y(),real.z());
            QPointF point = gislib->geoToCanvas(cord.lat, cord.lon);
            QVector3D center = entry.coreTransform->translation();
            // painter.drawEllipse(QPoint(x+(offset.x*30.f), y+(offset.z*30.f)), radius, radius);
            painter.drawEllipse(QPoint(point.x(), point.y()), radius, radius);

        }
   }
}

void CanvasWidget::drawCollision(QPainter& painter,std::string id , MeshEntry entry){
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

void CanvasWidget::drawTrajectory(QPainter& painter, const std::string& id, MeshEntry &entry) {
    if (!showTrajectories) return;
   if (!entry.trajectory) {
       if(entry.platform && entry.platform->trajectory){
           entry.trajectory = entry.platform->trajectory;
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
   // bool show = true;
   // if(entry.platform){
   //     show = entry.platform->dynamicModel->followEntity==nullptr;
   // }
   bool show = true;
      if(entry.platform && entry.platform->dynamicModel){
          show = entry.platform->dynamicModel->followEntity==nullptr;
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
   painter.restore();
}
void CanvasWidget::toggleLayerVisibility(const QString& layer, bool visible) {

   if (layer == "ToolTip") {
         showTooltip = visible;
         // If tooltip is disabled, hide any existing tooltip
         if (!visible) {
             m_tooltipTimer.stop();
             QToolTip::hideText();
         }
     }
   else if (layer == "Collider") {
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
   }
   else if (layer == "Sensors") {
       for (auto& [id, entry] : Meshes) {
           entry.detection = visible;
       }
   }
   else if (layer == "Radio") {
       for (auto& [id, entry] : Meshes) {
           entry.radioVisible = visible;
       }
   }
   else if (layer == "Trajectories") {
          showTrajectories = visible;
      }
   Refresh();
}







//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::setShapeDrawingMode(bool enabled, const QString& shapeType)
{
    isDrawingTrajectory = false;
    selectedShape = shapeType;

    if (enabled) {
        // ====================== ADD LAYER CHECK HERE ======================
        if (m_layerPanel) {
            // Check if there is any layer
            if (m_layerPanel->layerItems.isEmpty() && m_layerPanel->rasterLayers.isEmpty()) {
                // Show dialog only for shape drawing
                QMessageBox msgBox(nullptr);

                msgBox.setWindowTitle("No Layer Found");
                msgBox.setText("No layers exist!\n\n"
                               "Please create a layer first to draw shapes.");
                msgBox.setIcon(QMessageBox::Warning);

                QPushButton* createBtn = msgBox.addButton("Create New Layer", QMessageBox::AcceptRole);
                msgBox.addButton("Cancel", QMessageBox::RejectRole);

                msgBox.exec();

                if (msgBox.clickedButton() == createBtn) {
                    m_layerPanel->addLayer();   // Open layer creation dialog
                }
                return;   // ← IMPORTANT: Do not enable shape drawing
            }
        }
        // =================================================================

        // Don't set currentMode to DrawShape if we're in EditShape mode
        if (currentMode != EditShape) {
            currentMode = DrawShape;
        }
        setCursor(Qt::CrossCursor);

        // Clear temporary data only for multi-point shapes
        if (shapeType == "Polygon") {
            for (Vector* v : tempPolygonVertices) delete v;
            tempPolygonVertices.clear();
            tempPolygonCanvasPoints.clear();
        } else if (shapeType == "Line") {
            for (Vector* v : tempLineVertices) delete v;
            tempLineVertices.clear();
            tempLineCanvasPoints.clear();
        }
    }
    else {
        // Only exit DrawShape mode if we're not in EditShape mode
        if (currentMode == DrawShape) {
            currentMode = Translate;
        }
        selectedShape = "";
        setCursor(Qt::ArrowCursor);

        // Clear temporary data
        for (Vector* v : tempPolygonVertices) delete v;
        tempPolygonVertices.clear();
        tempPolygonCanvasPoints.clear();

        for (Vector* v : tempLineVertices) delete v;
        tempLineVertices.clear();
        tempLineCanvasPoints.clear();
    }

    Refresh();
}
//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::onBitmapImageSelected(const QString& filePath) {
   static int userImageCounter = 0;
   MeshEntry entry;
   entry.name = QString("UserImage_%1").arg(userImageCounter++);
   QPointF centerCanvas(width() / 2.0f, height() / 2.0f);
   QPointF geoPos = gislib->canvasToGeo(centerCanvas);
   entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
   entry.rotation = new QQuaternion();
   entry.size = new QVector3D(0.2, 0.2, 1);
   entry.velocity = new QVector3D(0, 0, 0);
   entry.trajectory = nullptr;
   entry.collider = nullptr;
   entry.bitmapPath = filePath;
   entry.mesh = new Mesh();
   if (!entry.mesh) return;
   entry.mesh->color = new QColor(Qt::white);
   entry.mesh->lineWidth = 1;
   entry.mesh->closePath = false;
   tempMeshes.push_back(entry);

   QString logMsg = QString("USER UPLOADED IMAGE: %1 at (lon: %2, lat: %3)")
                        .arg(entry.name)
                        .arg(geoPos.x(), 0, 'f', 6)
                        .arg(geoPos.y(), 0, 'f', 6);
   //Console::log(logMsg.toStdString());

   Refresh();
}

void CanvasWidget::placeBitmapAtGeo(const QPointF& geoPos)
{
   QString bitmapPath = getBitmapImagePath(selectedBitmapType);
   if (bitmapPath.isEmpty()) {
       Console::error("No bitmap path found for type: " + selectedBitmapType.toStdString());
       return;
   }

   static int bitmapCounter = 0;
   MeshEntry entry;
   entry.name = QString("TempBitmap_%1").arg(bitmapCounter++);
   entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
   entry.rotation = new QQuaternion();
   entry.size = new QVector3D(0.5, 0.5, 1);
   entry.velocity = new QVector3D(0, 0, 0);
   entry.trajectory = nullptr;
   entry.collider = nullptr;
   entry.bitmapPath = bitmapPath;
   entry.text = "";
   entry.mesh = new Mesh();
   entry.mesh->color = new QColor(Qt::white);
   entry.mesh->lineWidth = 1;
   entry.mesh->closePath = false;
   tempMeshes.push_back(entry);
   update();
}

// bitmap selected at Geo
void CanvasWidget::onBitmapImageSelectedAtGeo(
   const QString& path,
   const QPointF& geo
   )
{
   // Reuse EXISTING user-image pipeline
   onBitmapImageSelected(path);

   // Force exact geo placement
   placeBitmapAtGeo(geo);

   // Exit placement mode safely
   isPlacingBitmap = false;
   currentMode = PlaceBitmap;

   update();
}

//============================================================================
// Written by: Waris
//============================================================================
QString CanvasWidget::getBitmapImagePath(const QString& bitmapType) {
   QMap<QString, QString> bitmapMap = {
       {"Hospital", ":/icons/images/hospital.png"},
       {"School", ":/icons/images/school.png"},
       {"Forest Area", ":/icons/images/forest-area.png"}
   };
   return bitmapMap.value(bitmapType, "");
}

void CanvasWidget::onBitmapSelected(const QString& bitmapType) {
   selectedBitmapType = bitmapType;
   isPlacingBitmap = true;
   currentMode = PlaceBitmap;
   setCursor(Qt::CrossCursor);
   Refresh();
}

void CanvasWidget::onBitmapSelectedAtGeo(
   const QString& bitmapType,
   const QPointF& geo
   )
{
   selectedBitmapType = bitmapType;
   if (!geo.isNull()) {
       placeBitmapAtGeo(geo);
       isPlacingBitmap = false;
       currentMode = Translate;
       unsetCursor();
       Refresh();
       return;
   }
   isPlacingBitmap = true;
   currentMode = PlaceBitmap;
   setCursor(Qt::CrossCursor);
   Refresh();
}

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
   if (selectedEntityId.empty()) {
       return;
   }
   QJsonArray waypointsArray;
   for (const Waypoints* wp : currentTrajectory) {
       QJsonObject wpObj;
       QJsonObject posObj;
       posObj["type"] = "vector";
       posObj["x"] = wp->position->x;
       posObj["y"] = wp->position->y;
       posObj["z"] = wp->position->z;
       wpObj["position"] = posObj;
       waypointsArray.append(wpObj);
   }
   emit trajectoryUpdated(QString::fromStdString(selectedEntityId), waypointsArray);
   QJsonDocument doc(waypointsArray);
}

QJsonObject CanvasWidget::toJson() const {
   QJsonObject json;

   json["selectedBitmapType"] = selectedBitmapType;
   json["isPlacingBitmap"] = isPlacingBitmap;

   QJsonArray polygonVerticesArray;
   for (const Vector* vertex : tempPolygonVertices) {
       QJsonObject vObj;
       vObj["x"] = vertex->x;
       vObj["y"] = vertex->y;
       vObj["z"] = vertex->z;
       polygonVerticesArray.append(vObj);
   }

   json["tempPolygonVertices"] = polygonVerticesArray;

   QJsonArray polygonCanvasPointsArray;
   for (const QPointF& point : tempPolygonCanvasPoints) {
       QJsonObject pObj;
       pObj["x"] = point.x();
       pObj["y"] = point.y();
       polygonCanvasPointsArray.append(pObj);
   }
   json["tempPolygonCanvasPoints"] = polygonCanvasPointsArray;

   QJsonArray lineVerticesArray;
   for (const Vector* vertex : tempLineVertices) {
       QJsonObject vObj;
       vObj["x"] = vertex->x;
       vObj["y"] = vertex->y;
       vObj["z"] = vertex->z;
       lineVerticesArray.append(vObj);
   }

   json["tempLineVertices"] = lineVerticesArray;

   QJsonArray lineCanvasPointsArray;
   for (const QPointF& point : tempLineCanvasPoints) {
       QJsonObject pObj;
       pObj["x"] = point.x();
       pObj["y"] = point.y();
       lineCanvasPointsArray.append(pObj);
   }
   json["tempLineCanvasPoints"] = lineCanvasPointsArray;
   QJsonArray tempMeshesArray;
   for (const MeshEntry& entry : tempMeshes) {
       QJsonObject meshObj;
       meshObj["name"] = entry.name;
       meshObj["text"] = entry.text;
       if (entry.name.startsWith("TempText")) {
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
   // Trajectory colors save karo
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
   // Save layer panel data
   if (m_layerPanel) {
       QJsonObject layersData = m_layerPanel->toJson();
       json["layers"] = layersData;
       //qDebug() << "✓ CanvasWidget::toJson() - Layer data saved";
   } else {
       //qDebug() << "⚠ CanvasWidget::toJson() - No layer panel to save";
   }
   QJsonArray geoJsonArray;
   for (auto it = geoJsonLayerFilePaths.constBegin();
        it != geoJsonLayerFilePaths.constEnd(); ++it) {
       QJsonObject layerObj;
       layerObj["name"]     = it.key();
       layerObj["filePath"] = it.value();
       layerObj["visible"]  = geoJsonLayers.value(it.key(), true);
       geoJsonArray.append(layerObj);
   }
   json["geoJsonImportedLayers"] = geoJsonArray;
   return json;
}

void CanvasWidget::fromJson(const QJsonObject& json) {
   for (Waypoints* wp : currentTrajectory) {
       if (wp) {
           if (wp->position) {
               delete wp->position;
           }
           delete wp;
       }
   }
   currentTrajectory.clear();
   for (Vector* v : tempPolygonVertices) {
       delete v;
   }
   tempPolygonVertices.clear();
   tempPolygonCanvasPoints.clear();
   for (Vector* v : tempLineVertices) {
       delete v;
   }
   tempLineVertices.clear();
   tempLineCanvasPoints.clear();
   for (MeshEntry& entry : tempMeshes) {
       if (entry.position) delete entry.position;
       if (entry.rotation) delete entry.rotation;
       if (entry.size) delete entry.size;
       if (entry.velocity) delete entry.velocity;
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
   tempMeshes.clear();

   if (json.contains("selectedBitmapType")) {
       selectedBitmapType = json["selectedBitmapType"].toString();
   }

   if (json.contains("isPlacingBitmap")) {
       isPlacingBitmap = json["isPlacingBitmap"].toBool();
   }

   // Restore temporary polygon vertices
   if (json.contains("tempPolygonVertices") && json["tempPolygonVertices"].isArray()) {
       QJsonArray verticesArray = json["tempPolygonVertices"].toArray();
       for (const QJsonValue& val : verticesArray) {
           QJsonObject vObj = val.toObject();
           tempPolygonVertices.push_back(new Vector(
               vObj["x"].toDouble(),
               vObj["y"].toDouble(),
               vObj["z"].toDouble()
               ));
       }
   }

   // Restore temporary polygon canvas points
   if (json.contains("tempPolygonCanvasPoints") && json["tempPolygonCanvasPoints"].isArray()) {
       QJsonArray pointsArray = json["tempPolygonCanvasPoints"].toArray();
       for (const QJsonValue& val : pointsArray) {
           QJsonObject pObj = val.toObject();
           tempPolygonCanvasPoints.push_back(QPointF(
               pObj["x"].toDouble(),
               pObj["y"].toDouble()
               ));
       }
   }

   // Restore temporary line vertices
   if (json.contains("tempLineVertices") && json["tempLineVertices"].isArray()) {
       QJsonArray verticesArray = json["tempLineVertices"].toArray();
       for (const QJsonValue& val : verticesArray) {
           QJsonObject vObj = val.toObject();
           tempLineVertices.push_back(new Vector(
               vObj["x"].toDouble(),
               vObj["y"].toDouble(),
               vObj["z"].toDouble()
               ));
       }
   }

   // Restore temporary line canvas points
   if (json.contains("tempLineCanvasPoints") && json["tempLineCanvasPoints"].isArray()) {
       QJsonArray pointsArray = json["tempLineCanvasPoints"].toArray();
       for (const QJsonValue& val : pointsArray) {
           QJsonObject pObj = val.toObject();
           tempLineCanvasPoints.push_back(QPointF(
               pObj["x"].toDouble(),
               pObj["y"].toDouble()
               ));
       }
   }

   // Restore temporary meshes
   if (json.contains("tempMeshes") && json["tempMeshes"].isArray()) {
       QJsonArray tempMeshesArray = json["tempMeshes"].toArray();
       for (const QJsonValue& val : tempMeshesArray) {
           QJsonObject meshObj = val.toObject();
           MeshEntry entry;

           entry.name = meshObj["name"].toString();
           entry.text = meshObj["text"].toString();

           // Text properties deserialization
           if (entry.name.startsWith("TempText")) {
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

           /* **ROTATION** – read degrees, store as radians */
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

           tempMeshes.push_back(entry);
       }
   }
   // Restore layer panel data - AFTER all meshes are loaded
   if (json.contains("layers") && m_layerPanel) {
       QJsonObject layersData = json["layers"].toObject();
       m_layerPanel->fromJson(layersData);
   }
   // Trajectory colors load karo
   if (json.contains("trajectoryColors") && json["trajectoryColors"].isObject()) {
       QJsonObject colorsObj = json["trajectoryColors"].toObject();
       for (auto it = colorsObj.begin(); it != colorsObj.end(); ++it) {
           std::string entityId = it.key().toStdString();
           auto meshIt = Meshes.find(entityId);
           if (meshIt != Meshes.end()) {
               QJsonObject colorObj = it.value().toObject();
               meshIt->second.trajectoryColor = QColor(
                   colorObj["r"].toInt(),
                   colorObj["g"].toInt(),
                   colorObj["b"].toInt(),
                   colorObj.contains("a") ? colorObj["a"].toInt() : 255
               );
           }
       }
   }
   if (json.contains("geoJsonImportedLayers") && gislib) {
         QJsonArray geoJsonArray = json["geoJsonImportedLayers"].toArray();
         for (const QJsonValue& val : geoJsonArray) {
             QJsonObject layerObj = val.toObject();
             QString layerName = layerObj["name"].toString();
             QString filePath  = layerObj["filePath"].toString();
             bool    visible   = layerObj["visible"].toBool(true);

             if (filePath.isEmpty() || !QFile::exists(filePath)) {
                 //qDebug() << "GeoJSON layer file not found:" << filePath;
                 continue;
             }
             // Re-import without going through the full importGeoJsonLayer
             // signal chain (avoids duplicate layerPanel entries)
             gislib->importGeoJsonLayer(filePath);
             geoJsonLayers[layerName]         = visible;
             geoJsonLayerFilePaths[layerName] = filePath;

             if (!visible)
                 gislib->toggleVectorLayerVisibility(layerName, false);
         }
     }
   Refresh();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::onDistanceMeasured(double distance, QPointF startPoint, QPointF endPoint) {
   double distanceKilometers = distance / 1000.0;
   QString msg = QString("Measured distance: %1 meters (%2 km) from (lon: %3, lat: %4) to (lon: %5, lat: %6)")
                     .arg(distance, 0, 'f', 2)
                     .arg(distanceKilometers, 0, 'f', 2)
                     .arg(startPoint.x(), 0, 'f', 6)
                     .arg(startPoint.y(), 0, 'f', 6)
                     .arg(endPoint.x(), 0, 'f', 6)
                     .arg(endPoint.y(), 0, 'f', 6);
   //Console::log(msg.toStdString());
   if (measureDialog) {
       measureDialog->raise(); // Ensure dialog stays on top
   }
   QMessageBox::information(this, "Distance Measured", msg);
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::onPresetLayerSelected(const QString& preset)
{
   if (preset == "Airbase") {
       gislib->toggleAirbaseLayer();
       update();
   }
}


//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::importGeoJsonLayer(const QString &filePath) {
    if (!gislib) {
        Console::error("GISlib not available for GeoJSON import");
        return;
    }

    gislib->importGeoJsonLayer(filePath);

    QString layerName = QFileInfo(filePath).completeBaseName();
    geoJsonLayers[layerName] = true;
 geoJsonLayerFilePaths[layerName] = filePath;
    if (m_layerPanel) {
        if (!m_layerPanel->layerExists(layerName)) {
            m_layerPanel->addLayerFromScript(layerName);
        }
    }
    //
    loadImportedLayerFeaturesToMeshes(filePath, layerName);

    emit geoJsonLayerAdded(layerName);
    Refresh();
}

// Written by: Waris
//============================================================================
void CanvasWidget::onGeoJsonLayerToggled(const QString &layerName, bool visible) {
   if (gislib) {
       gislib->toggleVectorLayerVisibility(layerName, visible);
       geoJsonLayers[layerName] = visible;
       //Console::log("Toggled GeoJSON layer '" + layerName.toStdString() + "' to " + (visible ? "visible" : "hidden"));
       Refresh();
   } else {
       Console::error("Cannot toggle GeoJSON layer: GISlib not initialized");
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::onMeasurementTypeChanged(bool isEll) {
   gislib->setEllipsoidal(isEll);
   measureDialog->clearMeasurements();
   for (int i = 1; i < measurePoints.size(); ++i) {
       double dist = gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
       QPointF curr = measurePoints[i];
       measureDialog->addMeasurement(curr.x(), curr.y(), dist);
   }
   Refresh();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::startDistanceMeasurement() {
   setTransformMode(MeasureDistance);
   if (measureDialog) measureDialog->clearMeasurements();
   measurePoints.clear();
   //Console::log("Distance measurement mode started");
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::addMeasurePoint(double lon, double lat) {
   if (currentMode != MeasureDistance) startDistanceMeasurement();
   QPointF geo(lon, lat);
   measurePoints.append(geo);

   // Add measurement to dialog
   if (measureDialog && measurePoints.size() >= 2) {
       double dist = gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                               measurePoints.last());
       measureDialog->addMeasurement(lon, lat, dist);
   }
   Refresh();
}

//============================================================================
// Written by: Waris
//============================================================================
double CanvasWidget::getLastSegmentDistance() const {
   if (measurePoints.size() < 2) return 0.0;
   return gislib->calculateDistance(measurePoints[measurePoints.size() - 2],
                                    measurePoints.last());
}

//============================================================================
// Written by: Waris
//============================================================================
double CanvasWidget::getTotalDistance() const {
   double total = 0.0;
   for (int i = 1; i < measurePoints.size(); ++i)
       total += gislib->calculateDistance(measurePoints[i-1], measurePoints[i]);
   return total;
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::clearMeasurementPoints() {
   measurePoints.clear();
   if (measureDialog) measureDialog->clearMeasurements();
   Refresh();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::setMeasurementUnit(const QString &unit) {
   measurementUnit = unit;

   if (unit == "m") conversionFactor = 1.0;
   else if (unit == "km") conversionFactor = 0.001;
   else if (unit == "ft") conversionFactor = 3.28084;
   else if (unit == "mi") conversionFactor = 0.000621371;
   else if (unit == "deg") conversionFactor = 1.0;
   Refresh(); // triggers repaint using the new unit
}

//============================================================================
// Written by: Waris
//============================================================================
// bool CanvasWidget::handleBitmapSelection(QMouseEvent *event) {
//    for (auto& entry : tempMeshes) {
//        // Only preset bitmaps (not user images)
//        if (!entry.name.startsWith("UserImage_") && !entry.bitmapPath.isEmpty()) {
//            // ⭐ Use rotated polygon for hit detection
//            QPolygonF rotatedRect = getRotatedShapePolygon(entry);
//            if (rotatedRect.isEmpty()) continue;
//            // Calculate center of rotated rectangle
//            QPointF center(0, 0);
//            for (const QPointF& p : rotatedRect) {
//                center += p;
//            }
//            center /= rotatedRect.size();
//            // Scale to 75% for selection
//            QPolygonF scaledRect;
//            for (const QPointF& p : rotatedRect) {
//                QPointF offset = p - center;
//                scaledRect << (center + offset * 0.75f);
//            }
//            if (scaledRect.containsPoint(event->pos(), Qt::OddEvenFill)) {
//                isDraggingBitmap = true;
//                draggingBitmapId = entry.name;
//                bitmapDragStartPos = event->pos();
//                setCursor(Qt::ClosedHandCursor);
//                selectedEntityId = "";
//                activeDragAxis = "";
//                isDraggingUserImage = false;
//                m_highlightedShapeId = entry.name;
//                if (m_layerPanel) {
//                    m_layerPanel->selectShapeInPanel(entry.name);
//                }

//                             Refresh();
//                             return true;
//                         }
//                     }
//                 }
//                 return false;
//             }
//============================================================================
// Written by: Waris (Fixed by Grok)
//============================================================================
bool CanvasWidget::handleBitmapSelection(QMouseEvent *event) {
    // Check BOTH: Preset Bitmaps + User Images
    for (auto& entry : tempMeshes) {
        if (entry.bitmapPath.isEmpty()) continue;  // Sirf bitmaps

        // Use rotated polygon for accurate hit detection (best method)
        QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
        if (rotatedPoly.isEmpty()) continue;

        QPointF center(0, 0);
        for (const QPointF& p : rotatedPoly) center += p;
        center /= rotatedPoly.size();

        // Bigger hit area for easier selection (75% to 85%)
        QPolygonF scaledPoly;
        for (const QPointF& p : rotatedPoly) {
            QPointF offset = p - center;
            scaledPoly << (center + offset * 0.85f);
        }

        if (scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill)) {
            // Select this bitmap
            isDraggingBitmap = true;
            draggingBitmapId = entry.name;
            bitmapDragStartPos = event->pos();

            setCursor(Qt::ClosedHandCursor);

            // Clear other selections
            selectedEntityId = "";
            activeDragAxis = "";
            isDraggingUserImage = false;
            isDraggingShape = false;
            draggingShapeId = "";

            m_highlightedShapeId = entry.name;

            if (m_layerPanel) {
                m_layerPanel->selectShapeInPanel(entry.name);
            }

            Refresh();
            return true;
        }
    }
    return false;
}
//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleBitmapDragging(QMouseEvent *event) {
   for (auto& entry : tempMeshes) {
       if (entry.name == draggingBitmapId && !entry.bitmapPath.isEmpty()) {
           // Convert current position and new position to geo coordinates
           QPointF currentGeoPos(entry.position->x(), entry.position->y());
           QPointF currentCanvasPos = gislib->geoToCanvas(currentGeoPos.y(), currentGeoPos.x());

           // Calculate delta in canvas space
           QPointF canvasDelta = event->pos() - bitmapDragStartPos;

           // Apply delta to canvas position
           QPointF newCanvasPos = currentCanvasPos + canvasDelta;

           // Convert back to geo coordinates
           QPointF newGeoPos = gislib->canvasToGeo(newCanvasPos);

           // Update bitmap position
           entry.position->setX(newGeoPos.x()); // Longitude
           entry.position->setY(newGeoPos.y()); // Latitude

           // Update drag start position for next move
           bitmapDragStartPos = event->pos();
           Refresh();
           return;
       }
   }

   // If bitmap not found, stop dragging
   stopBitmapDragging();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::stopBitmapDragging() {
   if (isDraggingBitmap) {
       //Console::log("Stopped dragging bitmap: " + draggingBitmapId.toStdString());
       isDraggingBitmap = false;
       draggingBitmapId = "";
       setCursor(Qt::ArrowCursor);
       Refresh();
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::stopUserImageDragging() {
   if (isDraggingUserImage) {
       //Console::log("Stopped dragging user image: " + draggingUserImageId.toStdString());
       isDraggingUserImage = false;
       draggingUserImageId = "";
       setCursor(Qt::ArrowCursor);
       Refresh();
   }
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleUserImageDragging(QMouseEvent *event) {
   if (!isDraggingUserImage || draggingUserImageId.isEmpty()) return;

   for (auto& entry : tempMeshes) {
       if (entry.name == draggingUserImageId && entry.name.startsWith("UserImage_")) {
           // Convert current position and new position to geo coordinates
           QPointF currentGeoPos(entry.position->x(), entry.position->y());
           QPointF currentCanvasPos = gislib->geoToCanvas(currentGeoPos.y(), currentGeoPos.x());

           // Calculate delta in canvas space
           QPointF canvasDelta = event->pos() - userImageDragStartPos;

           // Apply delta to canvas position
           QPointF newCanvasPos = currentCanvasPos + canvasDelta;

           // Convert back to geo coordinates
           QPointF newGeoPos = gislib->canvasToGeo(newCanvasPos);

           // Update image position
           entry.position->setX(newGeoPos.x()); // Longitude
           entry.position->setY(newGeoPos.y()); // Latitude

           // Update drag start position for next move
           userImageDragStartPos = event->pos();

           QString posInfo = QString("Dragging user image: %1 to (lon: %2, lat: %3)")
                                 .arg(entry.name)
                                 .arg(newGeoPos.x(), 0, 'f', 6)
                                 .arg(newGeoPos.y(), 0, 'f', 6);
           //Console::log(posInfo.toStdString());

           Refresh();
           return;
       }
   }
   stopUserImageDragging();
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::handleUserImageSelection(QMouseEvent *event) {
   // Check user-uploaded images only (starts with "UserImage_")
   for (auto& entry : tempMeshes) {
       if (entry.name.startsWith("UserImage_") && !entry.bitmapPath.isEmpty()) {
           QPointF imageCenterCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());

           // Calculate actual image size in canvas coordinates
           QPointF sizePointGeo(entry.position->x() + entry.size->x(),
                                entry.position->y() + entry.size->y());
           QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());

           float canvasWidth = qAbs(sizePointCanvas.x() - imageCenterCanvas.x()) * 2;
           float canvasHeight = qAbs(sizePointCanvas.y() - imageCenterCanvas.y()) * 2;

           // Use 75% of actual size for selection
           float selectionW = canvasWidth * 0.75f;
           float selectionH = canvasHeight * 0.75f;

           QRectF imageRect(
               imageCenterCanvas.x() - selectionW/2,
               imageCenterCanvas.y() - selectionH/2,
               selectionW,
               selectionH
               );

           if (imageRect.contains(event->pos())) {
               isDraggingUserImage = true;
               draggingUserImageId = entry.name;
               userImageDragStartPos = event->pos();

               setCursor(Qt::ClosedHandCursor);

               // Deselect other entities
               selectedEntityId = "";
               activeDragAxis = "";
               isDraggingBitmap = false;

               Refresh();
               return true;
           }
       }
   }

   return false;
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::handleShapeSelection(QMouseEvent *event) {
    if (currentMode == DrawShape && (selectedShape == "Line" || selectedShape == "Polygon")) {
        return false;
    }

    for (auto& entry : tempMeshes) {
        if (entry.name.startsWith("Temp") && !entry.name.startsWith("TempText") &&
            !entry.name.startsWith("TempBitmap") && entry.bitmapPath.isEmpty()) {

            QPolygonF rotatedPoly = getRotatedShapePolygon(entry);
            if (rotatedPoly.isEmpty()) continue;

            bool isHit = false;

            if (entry.name.startsWith("TempPoint")) {
                QPointF centerCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
                float distance = QVector2D(event->pos() - centerCanvas).length();
                isHit = (distance <= 8.0f);                    // Increased tolerance
            }
            else if (entry.name.startsWith("TempPolyline")) {
                qreal lineTolerance = 8.0f;                    // Increased
                for (int i = 0; i < rotatedPoly.size() - 1; ++i) {
                    if (isPointNearLineSegment(event->pos(), rotatedPoly[i], rotatedPoly[i+1], lineTolerance)) {
                        isHit = true;
                        break;
                    }
                }
            }
            else {
                QPointF center(0, 0);
                for (const QPointF& p : rotatedPoly) center += p;
                center /= rotatedPoly.size();
                QPolygonF scaledPoly;
                for (const QPointF& p : rotatedPoly) {
                    QPointF offset = p - center;
                    scaledPoly << (center + offset * 0.8f);     // Slightly larger hit area
                }
                isHit = scaledPoly.containsPoint(event->pos(), Qt::OddEvenFill);
            }

            if (isHit) {
                emit shapeSelectedFromCanvas(entry.name);

                isDraggingShape   = true;
                draggingShapeId   = entry.name;
                shapesFeature->saveShapeState(draggingShapeId, &entry);
                shapeDragStartPos = event->pos();
                setCursor(Qt::ClosedHandCursor);

                // Clear other selections
                selectedEntityId = "";
                activeDragAxis = "";
                isDraggingBitmap = false;
                isDraggingUserImage = false;

                if (!isMultiSelecting()) {
                    selectedShapeIds.clear();
                    selectedShapeIds.push_back(entry.name);
                    m_highlightedShapeId = entry.name;
                }

                Refresh();
                return true;
            }
        }
    }
    return false;
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleShapeDragging(QMouseEvent *event) {
   if (!isDraggingShape || draggingShapeId.isEmpty()) return;
   for (auto& entry : tempMeshes) {
       if (entry.name == draggingShapeId) {
           static QString lastDraggedShape = "";
           if (lastDraggedShape != draggingShapeId) {
               shapesFeature->saveShapeState(draggingShapeId, &entry);
               lastDraggedShape = draggingShapeId;
           }
           // Calculate drag delta in geographic coordinates
           QPointF currentGeoPos = gislib->canvasToGeo(shapeDragStartPos);
           QPointF newGeoPos = gislib->canvasToGeo(event->pos());
           // Calculate the geographic offset
           double deltaLon = newGeoPos.x() - currentGeoPos.x();
           double deltaLat = newGeoPos.y() - currentGeoPos.y();
           // Update shape position
           entry.position->setX(entry.position->x() + deltaLon);
           entry.position->setY(entry.position->y() + deltaLat);
           // Update drag start position for next move event
           shapeDragStartPos = event->pos();

           QString posInfo = QString("Dragging shape: %1 to (lon: %2, lat: %3)")
                                 .arg(entry.name)
                                 .arg(entry.position->x(), 0, 'f', 6)
                                 .arg(entry.position->y(), 0, 'f', 6);
           //Console::log(posInfo.toStdString());

           Refresh();
           return;
       }
   }
   // If shape not found, stop dragging
   stopShapeDragging();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::stopShapeDragging() {
   if (isDraggingShape) {
       //Console::log("Stopped dragging shape: " + draggingShapeId.toStdString());
       isDraggingShape = false;
       draggingShapeId = "";
       setCursor(Qt::ArrowCursor);
       Refresh();
   }
}

//============================================================================
// Written by: Waris
//============================================================================
qreal CanvasWidget::angleBetweenPoints(const QPointF &center, const QPointF &p1, const QPointF &p2)
{
   QVector2D v1(p1 - center);
   QVector2D v2(p2 - center);
   if (v1.length() < 1e-6 || v2.length() < 1e-6) return 0.0;
   qreal angle = std::atan2(v2.y(), v2.x()) - std::atan2(v1.y(), v1.x());
   return qRadiansToDegrees(angle);
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::drawRotationHandle(QPainter& painter, const MeshEntry& entry)
{
   if (simulate || currentMode != Translate) return;
   if (!entry.position || !entry.size) return;

   // Only draw if this shape is in rotate mode
   if (entry.name != activeRotateId) return;

   bool isRotatable = !entry.bitmapPath.isEmpty() ||
                      entry.name.startsWith("TempRectangle") ||
                      entry.name.startsWith("TempCircle") ||
                      entry.name.startsWith("TempPolygon") ||
                      entry.name.startsWith("TempPolyline");

   if (!isRotatable) return;

   QPointF center = gislib->geoToCanvas(entry.position->y(), entry.position->x());

   // Calculate bounding box based on shape type
   QRectF boundingBox;

   if (entry.name.startsWith("TempCircle")) {
       // For circles, use the radius
       QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
       QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
       float canvasRadius = QVector2D(radiusPointCanvas - center).length();
       boundingBox = QRectF(center.x() - canvasRadius, center.y() - canvasRadius,
                            canvasRadius * 2, canvasRadius * 2);
   }
   else if (entry.name.startsWith("TempRectangle") || !entry.bitmapPath.isEmpty()) {
       // For rectangles and bitmaps
       QPointF sizePointGeo(entry.position->x() + entry.size->x(),
                            entry.position->y() + entry.size->y());
       QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());

       float w = qAbs(sizePointCanvas.x() - center.x()) * 2;
       float h = qAbs(sizePointCanvas.y() - center.y()) * 2;
       boundingBox = QRectF(center.x() - w/2, center.y() - h/2, w, h);
   }
   else if (entry.name.startsWith("TempPolygon") || entry.name.startsWith("TempPolyline")) {
       // For polygons and polylines, use vertex bounds
       QPolygonF poly;
       if (entry.mesh && !entry.mesh->polygen.empty()) {
           for (const Vector* v : entry.mesh->polygen) {
               QPointF geo(v->x + entry.position->x(), v->y + entry.position->y());
               QPointF canvas = gislib->geoToCanvas(geo.y(), geo.x());
               poly << canvas;
           }
           if (!poly.isEmpty()) {
               boundingBox = poly.boundingRect();
           }
       }
   }
   else {
       return;
   }

   // Place rotation handle at bottom-right, extended outward
   QPointF handleOffset(30, 30);
   QPointF handle = boundingBox.bottomRight() + handleOffset;
   painter.save();

   // Draw connection line from center to handle
   painter.setPen(QPen(QColor(100, 200, 255), 1, Qt::DashLine));
   painter.drawLine(center, handle);

   // Draw rotation handle (larger, more visible)
   painter.setPen(QPen(Qt::blue, 2));
   painter.setBrush(QColor(100, 200, 255));
   painter.drawEllipse(handle, 10, 10);

   // Draw inner circle for contrast
   painter.setPen(QPen(Qt::white, 1));
   painter.setBrush(Qt::NoBrush);
   painter.drawEllipse(handle, 6, 6);

   // Draw rotation icon (curved arrow)
   painter.setPen(QPen(Qt::white, 2));
   QRectF arcRect(handle.x() - 4, handle.y() - 4, 8, 8);
   painter.drawArc(arcRect, 30 * 16, 300 * 16);

   // Draw current rotation angle text
   float currentAngle = entry.rotation->z() * (180.0 / M_PI);
   QString angleText = QString("%1°").arg(currentAngle, 0, 'f', 1);
   painter.setFont(QFont("Arial", 9, QFont::Bold));
   painter.setPen(Qt::blue);
   painter.drawText(handle + QPointF(15, 5), angleText);
   painter.restore();
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::handleTextSelection(QMouseEvent *event) {
   const qreal selectionTolerance = 15.0;

   // Only handle text selection on single click, not during dragging
   if (event->type() != QEvent::MouseButtonPress || event->button() != Qt::LeftButton) {
       return false;
   }

   for (auto& entry : tempMeshes) {
       if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QPointF textPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());

           // Text bounding box calculate karen
           QFontMetrics fm(entry.textFont);
           QRect textRect = fm.boundingRect(entry.text);
           // textRect.moveTo(textPos.x(), textPos.y());
           textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());

           // Expand rect for easier selection
           QRectF expandedRect = textRect.adjusted(-5, -5, 5, 5);

           if (expandedRect.contains(event->pos())) {
               // If already selected and dialog is open, just return
               if (entry.isTextSelected && textPropertiesDialog && textPropertiesDialog->isVisible()) {
                   return true;
               }

               // Deselect previous text
               for (auto& otherEntry : tempMeshes) {
                   if (otherEntry.name.startsWith("TempText")) {
                       otherEntry.isTextSelected = false;
                   }
               }

               // Select current text for potential dragging
               entry.isTextSelected = true;
               editingTextId = entry.name;
               isEditingText = true;

               // SET DRAG START POSITION - IMPORTANT
               shapeDragStartPos = event->pos();

               setCursor(Qt::SizeAllCursor);
               //Console::log("Text selected for dragging: " + entry.name.toStdString());

               Refresh();
               return true;
           }
       }
   }

   // If clicked outside text, deselect all texts
   for (auto& entry : tempMeshes) {
       if (entry.name.startsWith("TempText")) {
           entry.isTextSelected = false;
       }
   }
   editingTextId = "";
   isEditingText = false;
   Refresh();
   return false;
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::showTextPropertiesDialog(const QString& textId) {
   // Find the text entry
   MeshEntry* textEntry = nullptr;
   for (auto& entry : tempMeshes) {
       if (entry.name == textId) {
           textEntry = &entry;
           break;
       }
   }
   if (!textEntry) return;
   // Create properties dialog as member variable to prevent multiple instances
   if (!textPropertiesDialog) {
       textPropertiesDialog = new QDialog(this);
       textPropertiesDialog->setWindowTitle("Text Properties");
       textPropertiesDialog->setFixedSize(300, 300);

       // Set dark background and light text for better visibility
       textPropertiesDialog->setStyleSheet(
           "QDialog { background-color: #2b2b2b; color: white; }"
           "QLabel { color: white; font-weight: bold; }"
           "QLineEdit { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
           "QSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
           "QPushButton { background-color: #404040; color: white; border: 1px solid #555; padding: 8px; font-weight: bold; }"
           "QPushButton:hover { background-color: #505050; }"
           "QPushButton:pressed { background-color: #606060; }"
           );

       // Ensure it's deleted when closed
       textPropertiesDialog->setAttribute(Qt::WA_DeleteOnClose);

       connect(textPropertiesDialog, &QDialog::finished, this, [this]() {
           textPropertiesDialog = nullptr;
           stopTextDragging();
       });
   }

   if (textPropertiesDialog->isVisible()) {
       textPropertiesDialog->raise();
       textPropertiesDialog->activateWindow();
       return;
   }

   // Clear previous layout if exists
   if (textPropertiesDialog->layout()) {
       delete textPropertiesDialog->layout();
   }

   QVBoxLayout* layout = new QVBoxLayout(textPropertiesDialog);

   // Text content
   QLabel* contentLabel = new QLabel("Text Content:", textPropertiesDialog);
   QLineEdit* contentEdit = new QLineEdit(textEntry->text, textPropertiesDialog);

   // Text color
   QLabel* colorLabel = new QLabel("Text Color:", textPropertiesDialog);
   QPushButton* colorButton = new QPushButton(textPropertiesDialog);
   colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(textEntry->textColor.name()));
   colorButton->setText("Select Color");
   connect(colorButton, &QPushButton::clicked, this, [=]() {
       QColor newColor = QColorDialog::getColor(textEntry->textColor, textPropertiesDialog, "Select Text Color", QColorDialog::DontUseNativeDialog);
       if (newColor.isValid()) {
           textEntry->textColor = newColor;
           colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(newColor.name()));
           Refresh();
       }
   });

   // Font size
   QLabel* sizeLabel = new QLabel("Font Size:", textPropertiesDialog);
   QSpinBox* sizeSpin = new QSpinBox(textPropertiesDialog);
   sizeSpin->setRange(8, 72);
   sizeSpin->setValue(textEntry->textSize);

   // Buttons
   QHBoxLayout* buttonLayout = new QHBoxLayout();
   QPushButton* applyButton = new QPushButton("Apply", textPropertiesDialog);
   QPushButton* deleteButton = new QPushButton("Delete Text", textPropertiesDialog);
   QPushButton* closeButton = new QPushButton("Close", textPropertiesDialog);

   buttonLayout->addWidget(applyButton);
   buttonLayout->addWidget(deleteButton);
   buttonLayout->addWidget(closeButton);

   // Add widgets to layout
   layout->addWidget(contentLabel);
   layout->addWidget(contentEdit);
   layout->addWidget(colorLabel);
   layout->addWidget(colorButton);
   layout->addWidget(sizeLabel);
   layout->addWidget(sizeSpin);
   layout->addStretch();
   layout->addLayout(buttonLayout);

   // Connect signals
   connect(applyButton, &QPushButton::clicked, this, [=]() {
       // Update text properties
       textEntry->text = contentEdit->text();
       textEntry->textSize = sizeSpin->value();
       textEntry->textFont.setPointSize(textEntry->textSize);

       // Update mesh color
       if (textEntry->mesh && textEntry->mesh->color) {
           *textEntry->mesh->color = textEntry->textColor;
       }
       Refresh();
       if (textPropertiesDialog) {
           textPropertiesDialog->close();
       }
   });

   connect(deleteButton, &QPushButton::clicked, this, [=]() {
       QMessageBox::StandardButton reply = QMessageBox::question(textPropertiesDialog, "Confirm Delete",
                                                                 "Are you sure you want to delete this text?",
                                                                 QMessageBox::Yes | QMessageBox::No);
       if (reply == QMessageBox::Yes) {
           deleteText(textId);
           if (textPropertiesDialog) {
               textPropertiesDialog->close();
           }
       }
   });
   connect(closeButton, &QPushButton::clicked, textPropertiesDialog, &QDialog::close);
   // Show the dialog
   textPropertiesDialog->show();
   textPropertiesDialog->raise();
   textPropertiesDialog->activateWindow();
}


//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::deleteText(const QString& textId) {
   for (auto it = tempMeshes.begin(); it != tempMeshes.end(); ++it) {
       if (it->name == textId) {
           delete it->position;
           delete it->rotation;
           delete it->size;
           delete it->velocity;
           if (it->mesh) {
               delete it->mesh->color;
               delete it->mesh;
           }
           delete it->collider;
           delete it->trajectory;
           tempMeshes.erase(it);
           Refresh();
           break;
       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================

void CanvasWidget::drawTextResizeHandles(QPainter& painter, const MeshEntry& entry) {
   if (!entry.isTextSelected) return;
   QPointF canvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
   QFontMetrics fm(entry.textFont);
   QRect textRect = fm.boundingRect(entry.text);
   textRect.moveTo(canvasPos.x(), canvasPos.y() - fm.ascent());
   painter.save();
   painter.setPen(QPen(Qt::blue, 2));
   painter.setBrush(Qt::blue);
   QVector<QPointF> handles = {
       textRect.topLeft(),
       textRect.topRight(),
       textRect.bottomRight(),
       textRect.bottomLeft()
   };
   for (const QPointF& handle : handles) {
       painter.drawRect(QRectF(handle.x() - 3, handle.y() - 3, 6, 6));
   }
   painter.restore();
}


//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::handleTextDragging(QMouseEvent *event) {
   if (!isEditingText || editingTextId.isEmpty()) return;

   for (auto& entry : tempMeshes) {
       if (entry.name == editingTextId) {
           // Calculate drag delta in canvas coordinates
           QPointF delta = event->pos() - shapeDragStartPos;

           // Convert current text position to canvas coordinates
           QPointF currentCanvasPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());

           // Calculate new canvas position
           QPointF newCanvasPos = currentCanvasPos + delta;

           // Convert back to geographic coordinates
           QPointF newGeoPos = gislib->canvasToGeo(newCanvasPos);

           // Update text position
           entry.position->setX(newGeoPos.x()); // Longitude
           entry.position->setY(newGeoPos.y()); // Latitude

           // Update drag start position for smooth dragging
           shapeDragStartPos = event->pos();

           QString posInfo = QString("Dragging text: %1 to (lon: %2, lat: %3)")
                                 .arg(entry.name)
                                 .arg(newGeoPos.x(), 0, 'f', 6)
                                 .arg(newGeoPos.y(), 0, 'f', 6);
           Refresh();
           return;
       }
   }

   // If text not found, stop dragging
   stopTextDragging();
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::stopTextDragging() {
   if (isEditingText) {
       isEditingText = false;
       editingTextId = "";
       setCursor(Qt::ArrowCursor);
       Refresh();
   }
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::handleTextRightClick(QMouseEvent *event) {
    return true;
   const qreal selectionTolerance = 15.0;

   for (auto& entry : tempMeshes) {
       if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QPointF textPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());

           QFontMetrics fm(entry.textFont);
           QRect textRect = fm.boundingRect(entry.text);
           // textRect.moveTo(textPos.x(), textPos.y());
           textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());

           QRectF expandedRect = textRect.adjusted(-5, -5, 5, 5);

           if (expandedRect.contains(event->pos())) {
               QMenu contextMenu(this);
               contextMenu.setStyleSheet(
                   "QMenu { background-color: white; color: black; border: 1px solid #cccccc; }"
                   "QMenu::item { background-color:    white; color: black; padding: 5px 20px; }"
                   "QMenu::item:selected { background-color: #0078d7; color: white; }"
                   );

               QAction* editTextAction = contextMenu.addAction("Edit Text Properties");
               connect(editTextAction, &QAction::triggered, this, [=]() {
                   showTextPropertiesDialog(entry.name);
               });

               QAction* deleteAction = contextMenu.addAction("Delete Text");
               connect(deleteAction, &QAction::triggered, this, [=]() {
                   deleteText(entry.name);
               });

               contextMenu.exec(event->globalPos());
               return true;
           }
       }
   }
   return false;
}


//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::showShapePropertiesDialog(const QString& shapeId) {
   // Find the shape entry
   MeshEntry* shapeEntry = nullptr;
   for (auto& entry : tempMeshes) {
       if (entry.name == shapeId && (isShape(entry.name) || !entry.bitmapPath.isEmpty())) {
           shapeEntry = &entry;
           break;
       }
   }

   if (!shapeEntry) {
       Console::error("Invalid entry for properties dialog: " + shapeId.toStdString());
       return;
   }

   // Check if it's a bitmap (no mesh color)
   bool isBitmap = !shapeEntry->bitmapPath.isEmpty();

   // Create properties dialog if it doesn't exist
   if (!shapePropertiesDialog) {
       shapePropertiesDialog = new QDialog(this);
       shapePropertiesDialog->setWindowTitle(isBitmap ? "Bitmap Properties" : "Shape Properties");
       shapePropertiesDialog->setFixedSize(300, 200);

       // Set dark theme
       shapePropertiesDialog->setStyleSheet(
           "QDialog { background-color: #2b2b2b; color: white; }"
           "QLabel { color: white; font-weight: bold; }"
           "QSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
           "QDoubleSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
           "QPushButton { background-color: #404040; color: white; border: 1px solid #555; padding: 8px; font-weight: bold; }"
           "QPushButton:hover { background-color: #505050; }"
           "QPushButton:pressed { background-color: #606060; }"
           );

       // Ensure it's deleted when closed
       shapePropertiesDialog->setAttribute(Qt::WA_DeleteOnClose);

       connect(shapePropertiesDialog, &QDialog::finished, this, [this]() {
           shapePropertiesDialog = nullptr;
       });
   }

   // If dialog already exists and is visible, bring it to front
   if (shapePropertiesDialog->isVisible()) {
       shapePropertiesDialog->raise();
       shapePropertiesDialog->activateWindow();
       return;
   }

   // Clear previous layout if exists
   if (shapePropertiesDialog->layout()) {
       QLayoutItem* item;
       while ((item = shapePropertiesDialog->layout()->takeAt(0)) != nullptr) {
           delete item->widget();
           delete item;
       }
       delete shapePropertiesDialog->layout();
   }

   QVBoxLayout *mainLayout = new QVBoxLayout(shapePropertiesDialog);

   // Color selection
   QLabel *colorLabel = new QLabel(isBitmap ? "Background Color:" : "Shape Color:", shapePropertiesDialog);
   QPushButton *colorButton = new QPushButton(shapePropertiesDialog);
   colorButton->setText("Select Color");

   QColor initialColor = isBitmap ? Qt::white : (shapeEntry->mesh && shapeEntry->mesh->color ? *shapeEntry->mesh->color : Qt::red);
   colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(initialColor.name()));
   colorButton->setProperty("selectedColor", initialColor);

   mainLayout->addWidget(colorLabel);
   mainLayout->addWidget(colorButton);

   // Border thickness
   QSpinBox *thicknessSpinBox = nullptr;
   if (!isBitmap) {
       QLabel *thicknessLabel = new QLabel("Border Thickness:", shapePropertiesDialog);
       thicknessSpinBox = new QSpinBox(shapePropertiesDialog);
       thicknessSpinBox->setRange(1, 10);
       thicknessSpinBox->setValue(shapeEntry->mesh ? shapeEntry->mesh->lineWidth : 2);

       mainLayout->addWidget(thicknessLabel);
       mainLayout->addWidget(thicknessSpinBox);
   }

   // Size controls
   QDoubleSpinBox *widthSpinBox = nullptr;
   QDoubleSpinBox *heightSpinBox = nullptr;
   if (isBitmap) {
       QLabel *sizeLabel = new QLabel("Size:", shapePropertiesDialog);
       QHBoxLayout *sizeLayout = new QHBoxLayout();

       widthSpinBox = new QDoubleSpinBox(shapePropertiesDialog);
       heightSpinBox = new QDoubleSpinBox(shapePropertiesDialog);

       widthSpinBox->setRange(0.1, 50.0);
       heightSpinBox->setRange(0.1, 50.0);
       widthSpinBox->setValue(shapeEntry->size->x());
       heightSpinBox->setValue(shapeEntry->size->y());
       widthSpinBox->setSuffix(" units");
       heightSpinBox->setSuffix(" units");

       sizeLayout->addWidget(new QLabel("W:", shapePropertiesDialog));
       sizeLayout->addWidget(widthSpinBox);
       sizeLayout->addWidget(new QLabel("H:", shapePropertiesDialog));
       sizeLayout->addWidget(heightSpinBox);

       mainLayout->addWidget(sizeLabel);
       mainLayout->addLayout(sizeLayout);
   }

   mainLayout->addStretch();

   // Buttons
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *applyButton = new QPushButton("Apply", shapePropertiesDialog);
   QPushButton *cancelButton = new QPushButton("Cancel", shapePropertiesDialog);

   buttonLayout->addWidget(applyButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   // Store current shape ID for the dialog
   QString currentShapeId = shapeId;

   // Connect color button
   connect(colorButton, &QPushButton::clicked, this, [=]() {
       QColor currentColor = colorButton->property("selectedColor").value<QColor>();
       QColor newColor = QColorDialog::getColor(currentColor, shapePropertiesDialog,
                                                "Select Color", QColorDialog::DontUseNativeDialog);
       if (newColor.isValid()) {
           colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(newColor.name()));
           colorButton->setProperty("selectedColor", newColor);
       }
   });

   // Connect apply button
   connect(applyButton, &QPushButton::clicked, this, [=]() {
       QColor selectedColor = colorButton->property("selectedColor").value<QColor>();

       if (isBitmap) {
           // Update bitmap size
           if (widthSpinBox && heightSpinBox) {
               double newWidth = widthSpinBox->value();
               double newHeight = heightSpinBox->value();
               shapeEntry->size->setX(newWidth);
               shapeEntry->size->setY(newHeight);
           }
       } else {
           // Update shape properties
           if (thicknessSpinBox) {
               int newThickness = thicknessSpinBox->value();

               if (selectedColor.isValid() && shapeEntry->mesh && shapeEntry->mesh->color) {
                   *shapeEntry->mesh->color = selectedColor;
               }

               if (shapeEntry->mesh) {
                   shapeEntry->mesh->lineWidth = newThickness;
               }
           }
       }

       Refresh();
       shapePropertiesDialog->accept();
   });

   // Connect cancel button
   connect(cancelButton, &QPushButton::clicked, shapePropertiesDialog, &QDialog::reject);

   // Show the dialog
   shapePropertiesDialog->show();
   shapePropertiesDialog->raise();
   shapePropertiesDialog->activateWindow();

   //Console::log("Opened properties dialog for: " + shapeId.toStdString());
}

//============================================================================
// Written by: Waris
//============================================================================
void CanvasWidget::updateShapeProperties(const QString& shapeId, const QColor& color, int borderThickness) {
   for (auto& entry : tempMeshes) {
       if (entry.name == shapeId && entry.mesh && entry.mesh->color) {
           *entry.mesh->color = color;
           entry.mesh->lineWidth = borderThickness;
           Refresh();
           break;
       }
   }
}

//============================================================================
// Written by: Waris
//============================================================================
bool CanvasWidget::isShape(const QString& shapeId) const {
   return shapeId.startsWith("TempCircle") ||
          shapeId.startsWith("TempRectangle") ||
          shapeId.startsWith("TempPolygon") ||
          shapeId.startsWith("TempPolyline") ||
          shapeId.startsWith("TempPoint") ||
          shapeId.startsWith("TempBitmap") ||
          shapeId.startsWith("UserImage_");
}

void CanvasWidget::showEntityInfo(const QString& entityId)
{
   if (!entityInfoDialog || entityId.isEmpty()) {

       return;
   }
   auto it = Meshes.find(entityId.toStdString());
   if (it == Meshes.end()) {

       hideEntityInfo();
       return;
   }

   MeshEntry& entry = it->second;
   if (!entry.entity || !entry.entity->Active || !entry.coreTransform) {
       hideEntityInfo();
       return;
   }
   try {
       QString entityName = "";
       if (!entry.entity->Name.empty()) {
           entityName = QString::fromStdString(entry.entity->Name);
       }
       else if (!entry.name.isEmpty()) {
           entityName = entry.name;
       }
       entityInfoDialog->setEntityInfo(entityId, entityName, &entry);
       if (entityInfoDialog->isVisible()) {
           entityInfoDialog->raise();
       } else {
           entityInfoDialog->show();
       }
   } catch (const std::exception& e) {
       hideEntityInfo();
   }
}
//========Hide Entity Info========
void CanvasWidget::hideEntityInfo()
{
   if (entityInfoDialog && entityInfoDialog->isVisible()) {
       entityInfoDialog->hide();
   }
}

void CanvasWidget::addTextAtGeo(const QString& text, const QPointF& geoPos)
{
   static int textCounter = 0;

   MeshEntry entry;
   entry.name = QString("TempText_%1").arg(textCounter++);
   entry.position = new QVector3D(geoPos.x(), geoPos.y(), 0);
   entry.rotation = new QQuaternion();
   entry.size = new QVector3D(1.0f, 1.0f, 1.0f);
   entry.velocity = new QVector3D(0, 0, 0);
   entry.trajectory = nullptr;
   entry.collider = nullptr;
   entry.bitmapPath = "";
   entry.text = text;

   // Text properties
   entry.textColor = Qt::black;
   entry.textFont  = QFont("Arial", 12);
   entry.textSize  = 12;
   entry.isTextSelected = false;

   entry.mesh = new Mesh();
   if (!entry.mesh) {
       Console::error("Failed to allocate Mesh for text");
       return;
   }

   entry.mesh->color = new QColor(entry.textColor);
   entry.mesh->lineWidth = 1;
   entry.mesh->closePath = false;
   tempMeshes.push_back(entry);
   Refresh();

}

/////////// move shape by coordinate by amjad///////////////////////////////
bool CanvasWidget::moveShapeByName(const std::string& shapeName,
                                 const QPointF& geoPos)
{
   if (!shapesFeature)
       return false;

   for (auto &entry : tempMeshes) {
       if (entry.name.toStdString() == shapeName) {
           shapesFeature->moveShapeByScript(&entry, geoPos);
           return true;
       }
   }
   return false;
}

// delete shape by amjad
bool CanvasWidget::deleteObjectById(const QString& id)
{
   for (auto it = tempMeshes.begin(); it != tempMeshes.end(); ++it) {
       if (it->name == id) {

           // history clear
           if (shapesFeature)
               shapesFeature->clearHistory(id);

           delete it->position;
           delete it->rotation;
           delete it->size;
           delete it->velocity;

           if (it->mesh) {
               for (Vector* v : it->mesh->polygen)
                   delete v;
               delete it->mesh->color;
               delete it->mesh;
           }

           delete it->collider;
           delete it->trajectory;
           tempMeshes.erase(it);
           //Console::log("Deleted by script: " + id.toStdString());
           Refresh();

           if (m_layerPanel) {
               m_layerPanel->removeShapeFromLayer(id);  // ← ADD THIS
           }
           return true;
       }
   }

   return false;
}

// rotate shape by amjad
QPolygonF CanvasWidget::getRotatedShapePolygon(const MeshEntry& entry) const {
   QPolygonF polygon;

   QPointF centerGeo(entry.position->x(), entry.position->y());
   QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());
   float rotationRad = entry.rotation->z();
   float cosFwd = std::cos(rotationRad);
   float sinFwd = std::sin(rotationRad);

   // CIRCLE - create approximate polygon
   if (entry.name.startsWith("TempCircle")) {
       QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
       QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
       float radius = QVector2D(radiusPointCanvas - centerCanvas).length();

       // Create circle as 32-point polygon
       for (int i = 0; i < 32; ++i) {
           float angle = (i * 2.0f * M_PI) / 32.0f;
           QPointF p = centerCanvas + QPointF(radius * std::cos(angle), radius * std::sin(angle));
           polygon << p;
       }
       return polygon;
   }
   if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QPointF textPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
           QFontMetrics fm(entry.textFont);
           QRect textRect = fm.boundingRect(entry.text);
           textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());

           QRectF expanded = textRect.adjusted(-5, -5, 5, 5);
           QPolygonF poly;
           poly << expanded.topLeft() << expanded.topRight()
                << expanded.bottomRight() << expanded.bottomLeft();
           return poly;
       }
   // RECTANGLE, POLYGON, POLYLINE - use mesh vertices with rotation
   if (entry.mesh && !entry.mesh->polygen.empty()) {
       for (Vector* v : entry.mesh->polygen) {
           // Apply rotation in local space
           float worldX = v->x * cosFwd - v->y * sinFwd;
           float worldY = v->x * sinFwd + v->y * cosFwd;

           // Convert to world geo
           QPointF vGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);

           // Convert to canvas
           QPointF vCanvas = gislib->geoToCanvas(vGeo.y(), vGeo.x());
           polygon << vCanvas;
       }
       return polygon;
   }

   // BITMAP - create rectangle corners with rotation
   if (!entry.bitmapPath.isEmpty()) {
       float halfW = entry.size->x() / 2.0f;
       float halfH = entry.size->y() / 2.0f;

       QVector<QPointF> localCorners = {
           QPointF(-halfW, -halfH),
           QPointF(halfW, -halfH),
           QPointF(halfW, halfH),
           QPointF(-halfW, halfH)
       };

       for (const QPointF& corner : localCorners) {
           float worldX = corner.x() * cosFwd - corner.y() * sinFwd;
           float worldY = corner.x() * sinFwd + corner.y() * cosFwd;

           QPointF cGeo(centerGeo.x() + worldX, centerGeo.y() + worldY);
           QPointF cCanvas = gislib->geoToCanvas(cGeo.y(), cGeo.x());
           polygon << cCanvas;
       }
       return polygon;
   }

   // POINT - small circle
   if (entry.name.startsWith("TempPoint")) {
       for (int i = 0; i < 8; ++i) {
           float angle = (i * 2.0f * M_PI) / 8.0f;
           QPointF p = centerCanvas + QPointF(6 * std::cos(angle), 6 * std::sin(angle));
           polygon << p;
       }
   }
   return polygon;
}
// Add to canvaswidget.cpp:
void CanvasWidget::selectMultipleEntities(const QList<QString>& entityIds) {
   selectedEntityIds.clear();
   selectedEntityId.clear();

   for (const QString& id : entityIds) {
       selectedEntityIds.push_back(id.toStdString());
   }

   // Set first entity as primary selected entity for single-entity operations
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
bool CanvasWidget::isClickOnEmptyCanvas(const QPoint& pos) {
   // Check if clicking on any regular entity
   for (auto& [id, entry] : Meshes) {
       if (!entry.coreTransform) continue;

       QPointF entityPos = gislib->geoToCanvas(
           entry.coreTransform->getLatitude(),
           entry.coreTransform->getLongitude()
       );

       if (QVector2D(pos - entityPos).length() < 20.0f) {
           return false;
       }

       // ALSO CHECK TRAJECTORY - FIXED SCOPE
       if (entry.trajectory && !entry.trajectory->Trajectories.empty()) {
           // Check waypoints
           for (size_t i = 0; i < entry.trajectory->Trajectories.size(); ++i) {
               QPointF wpCanvas = gislib->geoToCanvas(
                   entry.trajectory->Trajectories[i]->position->x,
                   entry.trajectory->Trajectories[i]->position->z
               );
               if (QVector2D(pos - wpCanvas).length() < 10.0f) {
                   return false; // Click is on trajectory waypoint
               }
           }

           // Check trajectory lines
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
                   if (QVector2D(pos - projection).length() < 8.0f) {
                       return false; // Click is on trajectory line
                   }
               }
           }
       }
   }

   // Check tempMeshes for shapes, bitmaps, and text
   for (const auto& tempEntry : tempMeshes) {
       if (!tempEntry.position || !tempEntry.size) continue;

       QPointF centerGeo(tempEntry.position->x(), tempEntry.position->y());
       QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

       // Text
       if (tempEntry.name.startsWith("TempText") && !tempEntry.text.isEmpty()) {
           QFont font = tempEntry.textFont;
           font.setPointSize(tempEntry.textSize);
           QFontMetrics fm(font);
           QRect textRect = fm.boundingRect(tempEntry.text);
           // textRect.moveTo(centerCanvas.x(), centerCanvas.y());
           textRect.moveTo(centerCanvas.x(), centerCanvas.y() - fm.ascent());
           if (textRect.contains(pos)) return false;
       }

       // Bitmaps
       if (!tempEntry.bitmapPath.isEmpty()) {
           QPointF sizePointGeo(tempEntry.position->x() + tempEntry.size->x(),
                               tempEntry.position->y() + tempEntry.size->y());
           QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
           float halfWidth = qAbs(sizePointCanvas.x() - centerCanvas.x());
           float halfHeight = qAbs(sizePointCanvas.y() - centerCanvas.y());

           QRectF bitmapRect(centerCanvas.x() - halfWidth,
                           centerCanvas.y() - halfHeight,
                           halfWidth * 2, halfHeight * 2);
           if (bitmapRect.contains(pos)) return false;
       }

       // Circles
       if (tempEntry.name.startsWith("TempCircle")) {
           QPointF radiusPointGeo(tempEntry.position->x() + tempEntry.size->x(), tempEntry.position->y());
           QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
           float radius = QVector2D(radiusPointCanvas - centerCanvas).length();
           if (QVector2D(pos - centerCanvas).length() <= radius) return false;
       }

       // Shapes (rectangles, polygons, polylines)
       else if (tempEntry.name.startsWith("TempRectangle") ||
                tempEntry.name.startsWith("TempPolygon") ||
                tempEntry.name.startsWith("TempPolyline")) {
           if (tempEntry.mesh && !tempEntry.mesh->polygen.empty()) {
               QPolygonF polygon;
               float rotationRad = tempEntry.rotation->z();
               float cosA = std::cos(rotationRad);
               float sinA = std::sin(rotationRad);

               for (Vector* point : tempEntry.mesh->polygen) {
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

       // Points
       else if (tempEntry.name.startsWith("TempPoint")) {
           QPointF pointCanvas = gislib->geoToCanvas(tempEntry.position->y(), tempEntry.position->x());
           if (QVector2D(pos - pointCanvas).length() <= 5.0f) return false;
       }
   }

   return true;

   // Check tempMeshes for shapes, bitmaps, and text
   for (const auto& entry : tempMeshes) {
       if (!entry.position || !entry.size) continue;

       QPointF centerGeo(entry.position->x(), entry.position->y());
       QPointF centerCanvas = gislib->geoToCanvas(centerGeo.y(), centerGeo.x());

       // Text
       if (entry.name.startsWith("TempText") && !entry.text.isEmpty()) {
           QFont font = entry.textFont;
           font.setPointSize(entry.textSize);
           QFontMetrics fm(font);
           QRect textRect = fm.boundingRect(entry.text);
           // textRect.moveTo(centerCanvas.x(), centerCanvas.y());
           textRect.moveTo(centerCanvas.x(), centerCanvas.y() - fm.ascent());
           if (textRect.contains(pos)) return false;
       }

       // Bitmaps
       if (!entry.bitmapPath.isEmpty()) {
           QPointF sizePointGeo(entry.position->x() + entry.size->x(),
                               entry.position->y() + entry.size->y());
           QPointF sizePointCanvas = gislib->geoToCanvas(sizePointGeo.y(), sizePointGeo.x());
           float halfWidth = qAbs(sizePointCanvas.x() - centerCanvas.x());
           float halfHeight = qAbs(sizePointCanvas.y() - centerCanvas.y());

           QRectF bitmapRect(centerCanvas.x() - halfWidth,
                           centerCanvas.y() - halfHeight,
                           halfWidth * 2, halfHeight * 2);
           if (bitmapRect.contains(pos)) return false;
       }

       // Circles
       if (entry.name.startsWith("TempCircle")) {
           QPointF radiusPointGeo(entry.position->x() + entry.size->x(), entry.position->y());
           QPointF radiusPointCanvas = gislib->geoToCanvas(radiusPointGeo.y(), radiusPointGeo.x());
           float radius = QVector2D(radiusPointCanvas - centerCanvas).length();
           if (QVector2D(pos - centerCanvas).length() <= radius) return false;
       }

       // Shapes (rectangles, polygons, polylines)
       else if (entry.name.startsWith("TempRectangle") ||
                entry.name.startsWith("TempPolygon") ||
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

       // Points
       else if (entry.name.startsWith("TempPoint")) {
           QPointF pointCanvas = gislib->geoToCanvas(entry.position->y(), entry.position->x());
           if (QVector2D(pos - pointCanvas).length() <= 5.0f) return false;
       }
   }

   return true;
}

void CanvasWidget::centerOnEntity(const QString& entityId, bool adjustZoom) {
   // Check if entity exists in Meshes
   auto it = Meshes.find(entityId.toStdString());
   if (it == Meshes.end()) {

       return;
   }
   MeshEntry& entry = it->second;
   if (!entry.coreTransform) {

       return;
   }
   double latitude = entry.coreTransform->getLatitude();
   double longitude = entry.coreTransform->getLongitude();
   if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {

       return;
   }

   if (gislib) {
       gislib->setCenter(latitude, longitude);
       if (adjustZoom) {
           int goodViewingZoom = 9;
           gislib->setZoom(goodViewingZoom);
       }
       update();
   } else {

   }
}

/*
* centerOnEntityWithZoom: Center map on entity with specific zoom level
* Provides explicit zoom control when centering on an entity
*/
void CanvasWidget::centerOnEntityWithZoom(const QString& entityId, int zoomLevel) {
   // Validate zoom level is reasonable (0-18 for most tile servers)
   if (zoomLevel < 0 || zoomLevel > 18) {

       zoomLevel = 10;
   }

   // Check if entity exists
   auto it = Meshes.find(entityId.toStdString());
   if (it == Meshes.end()) {

       return;
   }

   MeshEntry& entry = it->second;
   if (!entry.coreTransform) {

       return;
   }
   double latitude = entry.coreTransform->getLatitude();
   double longitude = entry.coreTransform->getLongitude();
   if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
       return;
   }

   if (gislib) {
       gislib->setCenter(latitude, longitude);
       gislib->setZoom(zoomLevel);
       update();
   } else {

   }
}
QString CanvasWidget::checkEntityHover(const QPoint& mousePos)
{
if (!showTooltip) return QString();
   QString newHoverId;
   for (const auto& [id, entry] : Meshes) {
       if (!entry.coreTransform || !entry.entity || !entry.entity->Active) continue;

       QPointF entityPos = gislib->geoToCanvas(
           entry.coreTransform->getLatitude(),
           entry.coreTransform->getLongitude()
       );

       float imageSize = 25.0f;
       if (entry.individualImageSize > 0 || ImageScale > 0) {
           imageSize = (entry.individualImageSize > 0 ? entry.individualImageSize : ImageScale) / 2.0f;
       }
       imageSize += 5.0f;
       if (QVector2D(mousePos - entityPos).length() < imageSize) {
           newHoverId = QString::fromStdString(id);
           break;
       }
   }
   if (newHoverId != m_hoveredEntityId) {
       m_hoveredEntityId = newHoverId;

       if (m_hoveredEntityId.isEmpty()) {
           QToolTip::hideText();
           m_tooltipTimer.stop();
       } else {
           m_tooltipTimer.start(700);
       }
       if (!m_hoveredEntityId.isEmpty()) {
           setCursor(Qt::PointingHandCursor);
       } else if (currentMode == Translate) {
           setCursor(Qt::ArrowCursor);
       }
   }

   return m_hoveredEntityId;
}

void CanvasWidget::updateHoverTooltip()
{
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
bool CanvasWidget::shouldDrawShape(const QString& shapeId) const {
   if (!m_layerPanel) {
       return true; // No layer panel connected, draw everything
   }
   QString layerName = m_layerPanel->getLayerForShape(shapeId);
   if (layerName.isEmpty()) {
       return true; // Shape not assigned to any layer, draw it
   }
   return m_layerPanel->isLayerVisible(layerName);
}
// Add this helper function in canvaswidget.cpp (around line 100)
double CanvasWidget::calculateTrajectoryCompletionTime(const MeshEntry& entry) const {
   if (!entry.trajectory || entry.trajectory->Trajectories.empty()) {
       return -1.0; // No trajectory
   }

   if (!entry.platform || !entry.platform->dynamicModel) {
       return -1.0; // No dynamic model
   }

   double totalTime = 0.0;
   double currentSpeed = entry.platform->dynamicModel->currentSpeed; // km/h

   if (currentSpeed <= 0) {
       currentSpeed = entry.platform->dynamicModel->moveSpeed; // Default speed
   }

   if (currentSpeed <= 0) {
       return -1.0; // Invalid speed
   }

   // Current position
   QPointF currentPos(entry.coreTransform->getLongitude(),
                     entry.coreTransform->getLatitude());

   // Calculate distance to first waypoint
   if (!entry.trajectory->Trajectories.empty()) {
       Waypoints* firstWp = entry.trajectory->Trajectories[0];
       QPointF wpPos(firstWp->position->z, firstWp->position->x);
       double distanceToFirst = gislib->calculateDistance(currentPos, wpPos) / 1000.0; // km
       totalTime += (distanceToFirst / currentSpeed) * 3600.0; // seconds
   }

   // Calculate time between waypoints
   for (size_t i = 0; i < entry.trajectory->Trajectories.size() - 1; ++i) {
       Waypoints* wp1 = entry.trajectory->Trajectories[i];
       Waypoints* wp2 = entry.trajectory->Trajectories[i + 1];

       QPointF pos1(wp1->position->z, wp1->position->x);
       QPointF pos2(wp2->position->z, wp2->position->x);

       double distance = gislib->calculateDistance(pos1, pos2) / 1000.0; // km
       double speedForSegment = (wp1->speed > 0) ? wp1->speed : currentSpeed;

       totalTime += (distance / speedForSegment) * 3600.0; // seconds
   }

   return totalTime;
}
void CanvasWidget::importLayer(const QString& filePath) {
    if (!gislib) return;

    gislib->importVectorLayer(filePath);

    QString layerName = QFileInfo(filePath).completeBaseName();
    geoJsonLayers[layerName] = true;
    geoJsonLayerFilePaths[layerName] = filePath;

    if (m_layerPanel) {
        if (!m_layerPanel->layerExists(layerName)) {
            m_layerPanel->addLayerFromScript(layerName);
        }
    }

    loadImportedLayerFeaturesToMeshes(filePath, layerName);

    emit geoJsonLayerAdded(layerName);
    Refresh();
}
void CanvasWidget::loadImportedLayerFeaturesToMeshes(const QString& filePath,
                                                      const QString& layerName)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    QJsonArray features;

    if (ext == "geojson" || ext == "json") {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) return;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (!doc.isObject()) return;
        features = doc.object()["features"].toArray();
    } else {
        return;
    }
    if (gislib)
        gislib->toggleVectorLayerVisibility(layerName, false);
    geoJsonLayers[layerName] = false;

    static int importCounter = 0;

    for (const QJsonValue& fval : features) {
        QJsonObject feat  = fval.toObject();
        QJsonObject geom  = feat["geometry"].toObject();
        QJsonObject props = feat["properties"].toObject();

        if (geom.isEmpty()) continue;

        QString geomType = geom["type"].toString();
        QJsonArray coords = geom["coordinates"].toArray();

        // Feature display name
        QString featName = props.value("name").toString();
        if (featName.isEmpty()) featName = props.value("NAME").toString();
        if (featName.isEmpty()) featName = props.value("title").toString();
        if (featName.isEmpty()) featName = props.value("id").toString();


        QColor shapeColor = QColor(255, 0, 0);

        QString fillStr   = props.value("fill").toString();
        QString strokeStr = props.value("stroke").toString();
        QString markerStr = props.value("marker-color").toString();
        QString colorStr  = props.value("color").toString();

        if (!fillStr.isEmpty() && QColor::isValidColor(fillStr))
            shapeColor = QColor(fillStr);
        else if (!strokeStr.isEmpty() && QColor::isValidColor(strokeStr))
            shapeColor = QColor(strokeStr);
        else if (!markerStr.isEmpty() && QColor::isValidColor(markerStr))
            shapeColor = QColor(markerStr);
        else if (!colorStr.isEmpty() && QColor::isValidColor(colorStr))
            shapeColor = QColor(colorStr);

        // Base MeshEntry banana
        auto makeBase = [&]() -> MeshEntry {
            MeshEntry e;
            e.rotation        = new QQuaternion();
            e.velocity        = new QVector3D(0, 0, 0);
            e.trajectory      = nullptr;
            e.collider        = nullptr;
            e.mesh            = new Mesh();
            e.mesh->color     = new QColor(shapeColor);
            e.mesh->lineWidth = 2;
            e.mesh->closePath = false;
            return e;
        };

        // Single ring ko LineString MeshEntry mein convert karo
        auto ringToPolylineEntry = [&](const QJsonArray& ring,
                                       const QString& namePrefix) -> MeshEntry
        {
            MeshEntry entry   = makeBase();
            entry.name        = QString("%1_%2_%3")
                                    .arg(namePrefix)
                                    .arg(layerName)
                                    .arg(importCounter++);
            entry.mesh->closePath = false;

            if (ring.isEmpty()) {
                entry.position = new QVector3D(0, 0, 0);
                entry.size     = new QVector3D(1, 1, 1);
                return entry;
            }

            double avgLon = 0, avgLat = 0;
            int n = ring.size();
            for (const QJsonValue& cv : ring) {
                avgLon += cv.toArray()[0].toDouble();
                avgLat += cv.toArray()[1].toDouble();
            }
            avgLon /= n;
            avgLat /= n;

            entry.position = new QVector3D(avgLon, avgLat, 0);
            entry.size     = new QVector3D(1, 1, 1);

            for (int i = 0; i < n; ++i) {
                QJsonArray pt = ring[i].toArray();
                entry.mesh->polygen.push_back(
                    new Vector(pt[0].toDouble() - avgLon,
                               pt[1].toDouble() - avgLat, 0));
            }
            return entry;
        };

        auto isCirclePolygon = [](const QJsonArray& ring,
                                  double& outLon,
                                  double& outLat,
                                  double& outRadius) -> bool
        {
            int n = ring.size();
            if (n < 6) return false;

            double cx = 0, cy = 0;
            for (int i = 0; i < n; ++i) {
                cx += ring[i].toArray()[0].toDouble();
                cy += ring[i].toArray()[1].toDouble();
            }
            cx /= n;
            cy /= n;

            QVector<double> dists;
            dists.reserve(n);
            for (int i = 0; i < n; ++i) {
                double dx = ring[i].toArray()[0].toDouble() - cx;
                double dy = ring[i].toArray()[1].toDouble() - cy;
                dists.push_back(std::sqrt(dx * dx + dy * dy));
            }

            double minD = *std::min_element(dists.begin(), dists.end());
            double maxD = *std::max_element(dists.begin(), dists.end());

            if (minD < 1e-12) return false;   // degenerate

            double ratio = (maxD - minD) / maxD;
            if (ratio > 0.15) return false;

            outLon    = cx;
            outLat    = cy;
            outRadius = (minD + maxD) / 2.0;
            return true;
        };

        auto isRectanglePolygon = [](const QJsonArray& ring,
                                     double& outAvgLon,
                                     double& outAvgLat,
                                     double& outHalfW,
                                     double& outHalfH) -> bool
        {
            int n = ring.size();
            if (n != 4 && n != 5) return false;

            int limit = (n == 5) ? 4 : 4;

            double minLon =  1e9, maxLon = -1e9;
            double minLat =  1e9, maxLat = -1e9;
            double avgLon = 0,    avgLat = 0;

            for (int i = 0; i < limit; ++i) {
                double lo = ring[i].toArray()[0].toDouble();
                double la = ring[i].toArray()[1].toDouble();
                minLon = qMin(minLon, lo); maxLon = qMax(maxLon, lo);
                minLat = qMin(minLat, la); maxLat = qMax(maxLat, la);
                avgLon += lo;
                avgLat += la;
            }
            avgLon /= limit;
            avgLat /= limit;

            double halfW = (maxLon - minLon) / 2.0;
            double halfH = (maxLat - minLat) / 2.0;

            if (halfW < 1e-12 || halfH < 1e-12) return false;

            outAvgLon = avgLon;
            outAvgLat = avgLat;
            outHalfW  = halfW;
            outHalfH  = halfH;
            return true;
        };

        // ──────────────────────────────────────────────────────────────────
        // Geometry types process karo
        // ──────────────────────────────────────────────────────────────────

        if (geomType == "Point") {
            // ── POINT ─────────────────────────────────────────────────────
            MeshEntry entry = makeBase();
            double lon = coords[0].toDouble();
            double lat = coords[1].toDouble();

            entry.name     = QString("TempPoint_import_%1_%2")
                                 .arg(layerName).arg(importCounter++);
            entry.position = new QVector3D(lon, lat, 0);
            entry.size     = new QVector3D(0.001, 0.001, 1);
            entry.mesh->polygen.push_back(new Vector(lon, lat, 0));

            if (m_layerPanel)
                m_layerPanel->addShapeToLayer(
                    entry.name,
                    featName.isEmpty() ? "Point" : featName,
                    layerName);
            tempMeshes.push_back(entry);

        } else if (geomType == "MultiPoint") {
            // ── MULTIPOINT ────────────────────────────────────────────────
            for (const QJsonValue& ptVal : coords) {
                QJsonArray pt  = ptVal.toArray();
                double lon = pt[0].toDouble();
                double lat = pt[1].toDouble();

                MeshEntry entry = makeBase();
                entry.name     = QString("TempPoint_import_%1_%2")
                                     .arg(layerName).arg(importCounter++);
                entry.position = new QVector3D(lon, lat, 0);
                entry.size     = new QVector3D(0.001, 0.001, 1);
                entry.mesh->polygen.push_back(new Vector(lon, lat, 0));

                if (m_layerPanel)
                    m_layerPanel->addShapeToLayer(
                        entry.name,
                        featName.isEmpty() ? "Point" : featName,
                        layerName);
                tempMeshes.push_back(entry);
            }

        } else if (geomType == "LineString") {
            // ── LINESTRING ────────────────────────────────────────────────
            if (coords.isEmpty()) continue;

            MeshEntry entry = ringToPolylineEntry(coords, "TempPolyline_import");

            if (m_layerPanel)
                m_layerPanel->addShapeToLayer(
                    entry.name,
                    featName.isEmpty() ? "Line" : featName,
                    layerName);
            tempMeshes.push_back(entry);

        } else if (geomType == "MultiLineString") {
            // ── MULTILINESTRING ───────────────────────────────────────────
            for (const QJsonValue& lineVal : coords) {
                QJsonArray pts = lineVal.toArray();
                if (pts.isEmpty()) continue;

                MeshEntry entry = ringToPolylineEntry(pts, "TempPolyline_import");

                if (m_layerPanel)
                    m_layerPanel->addShapeToLayer(
                        entry.name,
                        featName.isEmpty() ? "Line" : featName,
                        layerName);
                tempMeshes.push_back(entry);
            }

        } else if (geomType == "Polygon") {
            // ── POLYGON ───────────────────────────────────────────────────
            if (coords.isEmpty()) continue;
            QJsonArray outerRing = coords[0].toArray();
            if (outerRing.isEmpty()) continue;

            double cLon = 0, cLat = 0, cRadius = 0;
            double rAvgLon = 0, rAvgLat = 0, rHalfW = 0, rHalfH = 0;

            if (isCirclePolygon(outerRing, cLon, cLat, cRadius)) {
                MeshEntry entry = makeBase();
                entry.name     = QString("TempCircle_import_%1_%2")
                                     .arg(layerName).arg(importCounter++);
                entry.position = new QVector3D(cLon, cLat, 0);
                entry.size     = new QVector3D(cRadius, cRadius, 1);
                if (m_layerPanel)
                    m_layerPanel->addShapeToLayer(
                        entry.name,
                        featName.isEmpty() ? "Circle" : featName,
                        layerName);
                tempMeshes.push_back(entry);

            } else if (isRectanglePolygon(outerRing,
                                          rAvgLon, rAvgLat,
                                          rHalfW,  rHalfH)) {
                // ── Rectangle ─────────────────────────────────────────────
                MeshEntry entry = makeBase();
                entry.name      = QString("TempRectangle_import_%1_%2")
                                      .arg(layerName).arg(importCounter++);
                entry.position  = new QVector3D(rAvgLon, rAvgLat, 0);
                entry.size      = new QVector3D(rHalfW * 2.0, rHalfH * 2.0, 1);
                entry.mesh->closePath = true;

                entry.mesh->polygen = {
                    new Vector(-rHalfW,  rHalfH, 0),  // Top-left
                    new Vector( rHalfW,  rHalfH, 0),  // Top-right
                    new Vector( rHalfW, -rHalfH, 0),  // Bottom-right
                    new Vector(-rHalfW, -rHalfH, 0)   // Bottom-left
                };

                if (m_layerPanel)
                    m_layerPanel->addShapeToLayer(
                        entry.name,
                        featName.isEmpty() ? "Rectangle" : featName,
                        layerName);
                tempMeshes.push_back(entry);

            } else {
                double avgLon = 0, avgLat = 0;
                int n = outerRing.size();
                int limit = n;
                if (n > 1) {
                    QJsonArray fi = outerRing[0].toArray();
                    QJsonArray la = outerRing[n-1].toArray();
                    if (qAbs(fi[0].toDouble() - la[0].toDouble()) < 1e-9 &&
                        qAbs(fi[1].toDouble() - la[1].toDouble()) < 1e-9)
                        limit = n - 1;
                }

                for (int i = 0; i < limit; ++i) {
                    avgLon += outerRing[i].toArray()[0].toDouble();
                    avgLat += outerRing[i].toArray()[1].toDouble();
                }
                avgLon /= limit;
                avgLat /= limit;

                MeshEntry entry = makeBase();
                entry.name      = QString("TempPolygon_import_%1_%2")
                                      .arg(layerName).arg(importCounter++);
                entry.position  = new QVector3D(avgLon, avgLat, 0);
                entry.size      = new QVector3D(1, 1, 1);
                entry.mesh->closePath = true;

                for (int i = 0; i < limit; ++i) {
                    QJsonArray pt = outerRing[i].toArray();
                    entry.mesh->polygen.push_back(
                        new Vector(pt[0].toDouble() - avgLon,
                                   pt[1].toDouble() - avgLat, 0));
                }
                if (m_layerPanel)
                    m_layerPanel->addShapeToLayer(
                        entry.name,
                        featName.isEmpty() ? "Polygon" : featName,
                        layerName);
                tempMeshes.push_back(entry);
            }

        } else if (geomType == "MultiPolygon") {
            // ── MULTIPOLYGON ──────────────────────────────────────────────
            for (const QJsonValue& polyVal : coords) {
                QJsonArray outerRing = polyVal.toArray()[0].toArray();
                if (outerRing.isEmpty()) continue;
                double cLon = 0, cLat = 0, cRadius = 0;
                double rAvgLon = 0, rAvgLat = 0, rHalfW = 0, rHalfH = 0;
                if (isCirclePolygon(outerRing, cLon, cLat, cRadius)) {
                    MeshEntry entry = makeBase();
                    entry.name     = QString("TempCircle_import_%1_%2")
                                         .arg(layerName).arg(importCounter++);
                    entry.position = new QVector3D(cLon, cLat, 0);
                    entry.size     = new QVector3D(cRadius, cRadius, 1);
                    if (m_layerPanel)
                        m_layerPanel->addShapeToLayer(
                            entry.name,
                            featName.isEmpty() ? "Circle" : featName,
                            layerName);
                    tempMeshes.push_back(entry);

                } else if (isRectanglePolygon(outerRing,
                                              rAvgLon, rAvgLat,
                                              rHalfW,  rHalfH)) {
                    // Rectangle
                    MeshEntry entry = makeBase();
                    entry.name      = QString("TempRectangle_import_%1_%2")
                                          .arg(layerName).arg(importCounter++);
                    entry.position  = new QVector3D(rAvgLon, rAvgLat, 0);
                    entry.size      = new QVector3D(rHalfW * 2.0, rHalfH * 2.0, 1);
                    entry.mesh->closePath = true;
                    entry.mesh->polygen = {
                        new Vector(-rHalfW,  rHalfH, 0),
                        new Vector( rHalfW,  rHalfH, 0),
                        new Vector( rHalfW, -rHalfH, 0),
                        new Vector(-rHalfW, -rHalfH, 0)
                    };

                    if (m_layerPanel)
                        m_layerPanel->addShapeToLayer(
                            entry.name,
                            featName.isEmpty() ? "Rectangle" : featName,
                            layerName);
                    tempMeshes.push_back(entry);

                } else {
                    // Normal Polygon
                    int n = outerRing.size();
                    int limit = n;
                    if (n > 1) {
                        QJsonArray fi = outerRing[0].toArray();
                        QJsonArray la = outerRing[n-1].toArray();
                        if (qAbs(fi[0].toDouble() - la[0].toDouble()) < 1e-9 &&
                            qAbs(fi[1].toDouble() - la[1].toDouble()) < 1e-9)
                            limit = n - 1;
                    }

                    double avgLon = 0, avgLat = 0;
                    for (int i = 0; i < limit; ++i) {
                        avgLon += outerRing[i].toArray()[0].toDouble();
                        avgLat += outerRing[i].toArray()[1].toDouble();
                    }
                    avgLon /= limit;
                    avgLat /= limit;

                    MeshEntry entry = makeBase();
                    entry.name      = QString("TempPolygon_import_%1_%2")
                                          .arg(layerName).arg(importCounter++);
                    entry.position  = new QVector3D(avgLon, avgLat, 0);
                    entry.size      = new QVector3D(1, 1, 1);
                    entry.mesh->closePath = true;

                    for (int i = 0; i < limit; ++i) {
                        QJsonArray pt = outerRing[i].toArray();
                        entry.mesh->polygen.push_back(
                            new Vector(pt[0].toDouble() - avgLon,
                                       pt[1].toDouble() - avgLat, 0));
                    }

                    if (m_layerPanel)
                        m_layerPanel->addShapeToLayer(
                            entry.name,
                            featName.isEmpty() ? "Polygon" : featName,
                            layerName);
                    tempMeshes.push_back(entry);
                }
            }
        }
    }
}
void CanvasWidget::centerOnShape(const QString& shapeId)
{

    MeshEntry* target = nullptr;
    for (auto& entry : tempMeshes)
    {
        if (entry.name == shapeId)
        {
            target = &entry;
            break;
        }
    }
    if (!target || !target->position || !gislib)
        return;
    double lon = target->position->x();
    double lat = target->position->y();
    gislib->setCenter(lat, lon);

    m_highlightedShapeId = shapeId;

    selectedEntityId.clear();

    update();
}
void CanvasWidget::resetEntityInfoDialog()
{
    if (entityInfoDialog) {
        entityInfoDialog->onHierarchyReset();
    }
}
void CanvasWidget::copySelectedShape()
{
    if (m_copiedShape) {
        if (m_copiedShape->position) delete m_copiedShape->position;
        if (m_copiedShape->rotation) delete m_copiedShape->rotation;
        if (m_copiedShape->size)     delete m_copiedShape->size;
        if (m_copiedShape->velocity) delete m_copiedShape->velocity;
        if (m_copiedShape->mesh) {
            for (Vector* v : m_copiedShape->mesh->polygen) delete v;
            if (m_copiedShape->mesh->color) delete m_copiedShape->mesh->color;
            delete m_copiedShape->mesh;
        }
        delete m_copiedShape;
        m_copiedShape = nullptr;
    }

    MeshEntry* src = nullptr;
    for (auto& entry : tempMeshes) {
        if (entry.name == draggingShapeId ||
            entry.name == m_highlightedShapeId)
        {
            src = &entry;
            break;
        }
    }
    if (!src) {
        Console::warning("No shape selected to copy.");
        return;
    }

    m_copiedShape = new MeshEntry();
    m_copiedShape->name       = src->name;
    m_copiedShape->text       = src->text;
    m_copiedShape->bitmapPath = src->bitmapPath;
    m_copiedShape->textColor  = src->textColor;
    m_copiedShape->textFont   = src->textFont;
    m_copiedShape->textSize   = src->textSize;
    m_copiedShape->isTextSelected = false;

    m_copiedShape->position = src->position
        ? new QVector3D(*src->position) : new QVector3D();
    m_copiedShape->rotation = src->rotation
        ? new QQuaternion(*src->rotation) : new QQuaternion();
    m_copiedShape->size = src->size
        ? new QVector3D(*src->size) : new QVector3D(1,1,1);
    m_copiedShape->velocity = src->velocity
        ? new QVector3D(*src->velocity) : new QVector3D();

    m_copiedShape->trajectory = nullptr;
    m_copiedShape->collider   = nullptr;

    if (src->mesh) {
        m_copiedShape->mesh = new Mesh();
        m_copiedShape->mesh->lineWidth = src->mesh->lineWidth;
        m_copiedShape->mesh->closePath = src->mesh->closePath;
        m_copiedShape->mesh->color = src->mesh->color
            ? new QColor(*src->mesh->color) : new QColor(Qt::red);
        for (Vector* v : src->mesh->polygen)
            m_copiedShape->mesh->polygen.push_back(new Vector(v->x, v->y, v->z));
    } else {
        m_copiedShape->mesh = nullptr;
    }

    Console::log("Shape copied: " + src->name.toStdString());
}

void CanvasWidget::pasteShape()
{
    if (!m_copiedShape) {
        Console::warning("Nothing to paste.");
        return;
    }

    static int pasteCounter = 0;

    QString baseName = m_copiedShape->name;
    static const QRegularExpression reSuffix("(_copy_\\d+|_\\d+)$");
    baseName.remove(reSuffix);

    QString newName;
    do {
        newName = QString("%1_copy_%2").arg(baseName).arg(pasteCounter++);
    } while (std::any_of(tempMeshes.begin(), tempMeshes.end(),
                         [&](const MeshEntry& e){ return e.name == newName; }));

    QPointF origCanvas = gislib->geoToCanvas(
        m_copiedShape->position->y(),
        m_copiedShape->position->x());

    QPointF offsetCanvas(origCanvas.x() + 30.0, origCanvas.y() + 30.0);
    QPointF newGeo = gislib->canvasToGeo(offsetCanvas);

    // ── 3. Deep copy ────────────────────────────────────────────────────
    MeshEntry newEntry;
    newEntry.name           = newName;
    newEntry.text           = m_copiedShape->text;
    newEntry.bitmapPath     = m_copiedShape->bitmapPath;
    newEntry.textColor      = m_copiedShape->textColor;
    newEntry.textFont       = m_copiedShape->textFont;
    newEntry.textSize       = m_copiedShape->textSize;
    newEntry.isTextSelected = false;
    newEntry.trajectory     = nullptr;
    newEntry.collider       = nullptr;

    newEntry.position = new QVector3D(newGeo.x(), newGeo.y(), 0);
    newEntry.rotation = new QQuaternion(*m_copiedShape->rotation);
    newEntry.size     = new QVector3D(*m_copiedShape->size);
    newEntry.velocity = new QVector3D(*m_copiedShape->velocity);

    if (m_copiedShape->mesh) {
        newEntry.mesh            = new Mesh();
        newEntry.mesh->lineWidth = m_copiedShape->mesh->lineWidth;
        newEntry.mesh->closePath = m_copiedShape->mesh->closePath;
        newEntry.mesh->color     = m_copiedShape->mesh->color
            ? new QColor(*m_copiedShape->mesh->color)
            : new QColor(Qt::red);
        for (Vector* v : m_copiedShape->mesh->polygen)
            newEntry.mesh->polygen.push_back(new Vector(v->x, v->y, v->z));
    } else {
        newEntry.mesh = nullptr;
    }

    tempMeshes.push_back(newEntry);

    if (m_layerPanel) {
        QString shapeType;
        if      (newName.contains("Circle",    Qt::CaseInsensitive)) shapeType = "Circle";
        else if (newName.contains("Rectangle", Qt::CaseInsensitive)) shapeType = "Rectangle";
        else if (newName.contains("Polygon",   Qt::CaseInsensitive)) shapeType = "Polygon";
        else if (newName.contains("Polyline",  Qt::CaseInsensitive)) shapeType = "Line";
        else if (newName.contains("Point",     Qt::CaseInsensitive)) shapeType = "Point";
        else if (newName.contains("Text",      Qt::CaseInsensitive)) shapeType = "Text";
        else                                                          shapeType = "Shape";

        m_layerPanel->addShapeToLayer(newName, shapeType);
    }

    m_highlightedShapeId = newName;
    draggingShapeId      = "";
    isDraggingShape      = false;

    Refresh();
}

// ===================== MULTI SELECTION FUNCTIONS =====================

void CanvasWidget::clearMultiSelection()
{
    // Pehle drag states reset karo
    isMultiDrag         = false;
    isDraggingShape     = false;
    draggingShapeId     = "";
    isDraggingBitmap    = false;
    draggingBitmapId    = "";
    isDraggingUserImage = false;
    draggingUserImageId = "";

    selectedEntityIds.clear();
    selectedShapeIds.clear();
    selectedEntityId     = "";
    m_highlightedShapeId = "";

    setCursor(Qt::ArrowCursor);

    if (isBoxSelectionMode && selectedEntityIds.empty() && selectedShapeIds.empty()) {
        isBoxSelectionMode = false;
    }

    Refresh();
}
void CanvasWidget::performBoxSelection(const QPoint& p1, const QPoint& p2)
{
    clearMultiSelection();
    QRectF selRect = QRectF(p1, p2).normalized();

    // Select Entities
    for (auto& [id, entry] : Meshes)
    {
        if (!entry.coreTransform) continue;
        QPointF pos = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude()
        );
        if (selRect.contains(pos))
            selectedEntityIds.push_back(id);
    }

    // Select Shapes, Bitmaps, Text
    for (auto& entry : tempMeshes)
    {
        bool isSelected = false;

        // === TEXT SPECIAL HANDLING ===
        if (entry.name.startsWith("TempText") && !entry.text.isEmpty())
        {
            QPointF textPos = gislib->geoToCanvas(entry.position->y(), entry.position->x());
            QFontMetrics fm(entry.textFont);
            QRect textRect = fm.boundingRect(entry.text);
            textRect.moveTo(textPos.x(), textPos.y() - fm.ascent());

            QRectF expandedRect = textRect.adjusted(-10, -10, 10, 10); // thoda bada tolerance
            if (selRect.intersects(expandedRect))
                isSelected = true;
        }
        // === OTHER SHAPES ===
        else
        {
            QPolygonF poly = getRotatedShapePolygon(entry);
            if (!poly.isEmpty())
            {
                for (const QPointF& pt : poly)
                {
                    if (selRect.contains(pt))
                    {
                        isSelected = true;
                        break;
                    }
                }
            }
        }

        if (isSelected)
        {
            selectedShapeIds.push_back(entry.name);
        }
    }

    if (!selectedEntityIds.empty())
        selectedEntityId = selectedEntityIds[0];

    if (!selectedShapeIds.empty())
        m_highlightedShapeId = selectedShapeIds[0];

    Refresh();
}

void CanvasWidget::moveSelectedItems(const QPointF& deltaGeo)
{
    if (!isMultiDrag) return;

    // 1. Move Entities + Their Trajectories
    for (const std::string& id : selectedEntityIds)
    {
        auto it = Meshes.find(id);
        if (it == Meshes.end()) continue;

        MeshEntry& entry = it->second;

        // Move Main Entity
        if (entry.coreTransform)
        {
            double newLat = entry.coreTransform->getLatitude() + deltaGeo.y();
            double newLon = entry.coreTransform->getLongitude() + deltaGeo.x();
            entry.coreTransform->setGeoCord(newLat, newLon);
        }

        // Move Trajectory Waypoints (Yeh important hai)
        if (entry.trajectory && !entry.trajectory->Trajectories.empty())
        {
            for (Waypoints* wp : entry.trajectory->Trajectories)
            {
                if (wp && wp->position)
                {
                    wp->position->x += deltaGeo.y();   // Latitude
                    wp->position->z += deltaGeo.x();   // Longitude
                }
            }
        }
    }

    // 2. Move Shapes, Bitmaps, Text
    for (const QString& name : selectedShapeIds)
    {
        for (auto& entry : tempMeshes)
        {
            if (entry.name == name && entry.position)
            {
                entry.position->setX(entry.position->x() + deltaGeo.x());
                entry.position->setY(entry.position->y() + deltaGeo.y());
            }
        }
    }

    Refresh();
}

bool CanvasWidget::isClickOnAnySelectedItem(const QPoint& pos)
{
    // Check selected entities
    for (const std::string& id : selectedEntityIds)
    {
        auto it = Meshes.find(id);
        if (it != Meshes.end() && it->second.coreTransform)
        {
            QPointF entityPos = gislib->geoToCanvas(
                it->second.coreTransform->getLatitude(),
                it->second.coreTransform->getLongitude()
            );
            if (QVector2D(pos - entityPos).length() < 25.0f)
                return true;
        }
    }

    // Check selected shapes
    for (const QString& name : selectedShapeIds)
    {
        for (const auto& entry : tempMeshes)
        {
            if (entry.name == name)
            {
                QPolygonF poly = getRotatedShapePolygon(entry);
                if (!poly.isEmpty() && poly.containsPoint(pos, Qt::OddEvenFill))
                    return true;
            }
        }
    }
    return false;
}
