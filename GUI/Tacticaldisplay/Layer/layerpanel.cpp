/* ========================================================================= */
/* File: layerpanel.cpp                                                     */
/* Written by: Waris Ali                                                   */
/* ========================================================================= */

#include "layerpanel.h"
#include "layerpanel-styles.h"
// CHAITANYA JAIN CHANGE:
// Why: Included the ILayerHost interface definition from the new Layer directory.
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
// CHAITANYA JAIN CHANGE:
// Why: Included new GIS export headers.
#include "GUI/Tacticaldisplay/Layer/geojson.h"
#include "GUI/Tacticaldisplay/Layer/kml.h"
#include "GUI/Tacticaldisplay/Layer/gml.h"
#include "GUI/Tacticaldisplay/Layer/shapefile.h"
#include "GUI/Tacticaldisplay/Layer/flatgeobuf.h"
#include "GUI/Tacticaldisplay/Layer/csv.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <cstring>
#include <cmath>
#include <QDir>
#include <qgscoordinatetransform.h>
#include <qgsproject.h>
#include <core/Hierarchy/Struct/vector.h>
// CHAITANYA JAIN CHANGE:
// Why: Included mesh.h to resolve incomplete type errors when accessing Mesh properties.
#include <core/Hierarchy/Components/mesh.h>
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include <QDialogButtonBox>

// CHAITANYA JAIN CHANGE:
// Why: Decoupled makeDefaultExtents function signature from CanvasWidget by changing parameter to ILayerHost*.
// static RasterLayer makeDefaultExtents(const RasterLayer& rl, CanvasWidget* canvas);
static RasterLayer makeDefaultExtents(const RasterLayer& rl, ILayerHost* host);



// %%% Constructor %%%
/* Initialize layer panel with tree view and context menu */
LayerPanel::LayerPanel(QWidget *parent)
    : QDockWidget(parent)
{
    // Apply dock style
    setStyleSheet(LayerPanelStyles::LayerPanelDock);
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetMovable |
                QDockWidget::DockWidgetFloatable);
    setMinimumWidth(200);
    setTitleBarWidget(new QWidget());
    setupUI();
    setupContextMenu();
}

// %%% Destructor %%%
/* Clean up layer panel resources */
LayerPanel::~LayerPanel()
{
    // Cleanup handled by Qt parent-child relationship
}

void LayerPanel::setupUI()
{
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setStyleSheet("background-color: #0F2636;");
    QVBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(0);
    // Create tree widget for layer management
    layerTree = new QTreeWidget(mainWidget);
    layerTree->setColumnCount(2);
    layerTree->setHeaderHidden(true);
    layerTree->setContextMenuPolicy(Qt::CustomContextMenu);
    layerTree->setSelectionMode(QAbstractItemView::SingleSelection);
    layerTree->setDragEnabled(true);
    layerTree->setAcceptDrops(true);
    layerTree->setDropIndicatorShown(true);
    // DragDrop mode: our eventFilter intercepts drops before Qt reparents items
    layerTree->setDragDropMode(QAbstractItemView::DragDrop);
    layerTree->setRootIsDecorated(false);
    layerTree->setIndentation(20);
    // Install event filter on the viewport so LayerPanel::eventFilter() handles shape->layer drops
    layerTree->viewport()->installEventFilter(this);

    // Column sizing: col0 stretches, col1 is fixed 28px
    layerTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    layerTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    layerTree->header()->resizeSection(1, 28);

    layerTree->setStyleSheet(
        "QTreeWidget {"
        "  background-color: #0F2636;"
        "  color: white;"
        "  border: 2px solid #27446d;"
        "  outline: none;"
        "  font-size: 13px;"
        "}"
        "QTreeWidget::item {"
        "  color: white;"
        "  background-color: transparent;"
        "  height: 26px;"
        "  padding: 2px 4px;"
        "  border: none;"
        "}"
        "QTreeWidget::item:selected {"
        "  background-color: #1C5FAE;"
        "  color: white;"
        "}"
        "QTreeWidget::item:hover:!selected {"
        "  background-color: #1A3652;"
        "}"
        "QTreeWidget::branch {"
        "  background-color: #0F2636;"
        "  image: none;"
        "  border-image: none;"
        "}"
        "QTreeWidget::branch:selected {"
        "  background-color: #1C5FAE;"
        "}"
        "QScrollBar:vertical {"
        "  background-color: #0F2636; width: 8px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: #3A506B; min-height: 20px; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover { background-color: #4A607B; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  border: none; background: none; height: 0px;"
        "}"
        );

    layout->addWidget(layerTree);
    setWidget(mainWidget);
    // Root "Layers" item (spans all columns, non-selectable)
    rootLayersItem = new QTreeWidgetItem(layerTree);
    rootLayersItem->setText(0, "Layers");
    rootLayersItem->setFlags(rootLayersItem->flags() & ~Qt::ItemIsSelectable);
    rootLayersItem->setExpanded(true);

    QFont rootFont = rootLayersItem->font(0);
    rootFont.setBold(true);
    rootLayersItem->setFont(0, rootFont);
    rootLayersItem->setForeground(0, QBrush(QColor(204, 204, 204)));

    connect(layerTree, &QTreeWidget::customContextMenuRequested,
            this, &LayerPanel::showContextMenu);
    connect(layerTree, &QTreeWidget::itemSelectionChanged,
            this, &LayerPanel::onLayerSelectionChanged);
    connect(layerTree, &QTreeWidget::itemClicked,
            this, &LayerPanel::onShapeItemClicked);
    // ── Shortcut Keys ─────────────────────────────────────────────────────
    // Delete: Remove selected layer or shape
    QShortcut* deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), layerTree);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, [this]() {
        requestDeleteSelectedItem();
    });

    // F2: Rename selected layer or shape
    QShortcut* renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), layerTree);
    renameShortcut->setContext(Qt::WidgetShortcut);
    connect(renameShortcut, &QShortcut::activated, this, [this]() {
        QTreeWidgetItem* selected = layerTree->currentItem();
        if (!selected) return;

        // Shape item
        QString shapeId = selected->data(0, Qt::UserRole).toString();
        if (!shapeId.isEmpty() && shapeToLayer.contains(shapeId)) {
            renameShape(shapeId);
            return;
        }

        // Vector or Raster layer
        QString layerName = selected->data(0, Qt::UserRole).toString();
        if (!layerName.isEmpty() &&
            (layerItems.contains(layerName) || rasterLayers.contains(layerName))) {
            renameLayerByName(layerName);
            return;
        }
    });
}

// %%% Context Menu Setup %%%
/* Initialize context menu with actions */
void LayerPanel::setupContextMenu()
{
    contextMenu = new QMenu(this);
    contextMenu->setStyleSheet(LayerPanelStyles::ContextMenu);

    // Create actions
    addLayerAction = new QAction("Add Vector Layer", this);
    addRasterLayerAction = new QAction("Add Raster Layer", this);
    removeLayerAction = new QAction("Remove Layer", this);
    renameLayerAction = new QAction("Rename Layer", this);
    exportLayerAction = new QAction("Export Layer", this);

    // Connect actions
    connect(addLayerAction, &QAction::triggered, this, &LayerPanel::addLayer);
    connect(addRasterLayerAction, &QAction::triggered, this, &LayerPanel::addRasterLayer);
    connect(removeLayerAction, &QAction::triggered, this, &LayerPanel::requestDeleteSelectedItem);
    connect(renameLayerAction, &QAction::triggered, this, &LayerPanel::renameLayer);
    connect(exportLayerAction, &QAction::triggered, this, &LayerPanel::exportLayer);
}

// %%% Context Menu Display %%%
/* Show context menu on right-click with dynamic menu building */
void LayerPanel::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *selectedItem = layerTree->itemAt(pos);
    // Clear existing menu items
    contextMenu->clear();

    bool showMenu = false;

    if (selectedItem) {
        QString itemText = selectedItem->text(0);
        // ONLY show "Add Layer" on the root "Layers" item
        if (itemText == "Layers") {
            contextMenu->addAction(addLayerAction);
            contextMenu->addAction(addRasterLayerAction);
            showMenu = true;
        }
        // For actual layer items (vector or raster) — identified via maps, not text
        else if (isLayerItem(selectedItem) ||
                 rasterLayerItems.values().contains(selectedItem)) {
            contextMenu->addAction(removeLayerAction);
            contextMenu->addSeparator();
            contextMenu->addAction(renameLayerAction);
            // Export only makes sense for vector layers
            if (isLayerItem(selectedItem)) {
                contextMenu->addSeparator();
                contextMenu->addAction(exportLayerAction);
            }
            showMenu = true;
        }
        else if (selectedItem->parent() &&
                 selectedItem->parent()->text(0) != "Layers") {
            // Shape item - show rename and remove actions
            QString shapeId = selectedItem->data(0, Qt::UserRole).toString();
            QAction* renameShapeAction = new QAction("Rename Shape", this);
            connect(renameShapeAction, &QAction::triggered, this, [this, shapeId]() {
                renameShape(shapeId);
            });
            contextMenu->addAction(renameShapeAction);
            contextMenu->addSeparator();
            QAction* removeShapeAction = new QAction("Remove Shape", this);
            connect(removeShapeAction, &QAction::triggered, this, &LayerPanel::requestDeleteSelectedItem);
            contextMenu->addAction(removeShapeAction);
            showMenu = true;
        }
    } else {
        return;
    }
    if (showMenu && !contextMenu->actions().isEmpty()) {
        contextMenu->exec(layerTree->mapToGlobal(pos));
    }
}

// %%% Add Layer %%%
/* Add new layer with visibility toggle */
void LayerPanel::addLayer()
{

    QDialog inputDlg(this);
    inputDlg.setWindowTitle("Add Layer");
    inputDlg.setMinimumWidth(300);
    inputDlg.setStyleSheet(
        LayerPanelStyles::MessageBox +
        " QDialog { border: 2px solid #27446d; background-color: #0F2636; }"
        );

    QVBoxLayout* layout = new QVBoxLayout(&inputDlg);
    layout->setSpacing(8);
    layout->setContentsMargins(15, 15, 15, 10);

    QLabel* lbl = new QLabel("Layer Name:", &inputDlg);
    lbl->setStyleSheet("color: white;");
    layout->addWidget(lbl);

    QLineEdit* lineEdit = new QLineEdit("New Layer", &inputDlg);
    lineEdit->setStyleSheet(
        "color: white; background-color: #1A3652;"
        "border: 1px solid #27446d; padding: 4px;");
    lineEdit->selectAll();
    layout->addWidget(lineEdit);

    QDialogButtonBox* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &inputDlg);
    btns->setStyleSheet(LayerPanelStyles::MessageBox);
    layout->addWidget(btns);

    QObject::connect(btns, &QDialogButtonBox::accepted, &inputDlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &inputDlg, &QDialog::reject);

    bool ok = (inputDlg.exec() == QDialog::Accepted);
    QString layerName = lineEdit->text().trimmed();

    if (!ok || layerName.isEmpty()) {
        if (ok && layerName.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
            msgBox.setWindowTitle("Invalid Name");
            msgBox.setText("Layer name cannot be empty!");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
        }
        return;
    }
    // Check if layer name already exists
    if (layerExists(layerName)) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("Duplicate Name");
        msgBox.setText("A layer with this name already exists!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }
    QTreeWidgetItem *selectedItem = layerTree->currentItem();
    QTreeWidgetItem *newLayer = new QTreeWidgetItem();
    newLayer->setText(0, layerName);
    newLayer->setFlags(newLayer->flags() | Qt::ItemIsEditable);
    // Always add under the root "Layers" item
    if (rootLayersItem) {
        rootLayersItem->addChild(newLayer);
    } else {
        // Fallback if root item not found
        layerTree->addTopLevelItem(newLayer);
    }

    // Register in maps
    layerItems[layerName] = newLayer;
    layerShapes[layerName] = QStringList();

    // Initialize layer as visible by default
    layerVisibility[layerName] = true;

    // Create visibility toggle for this layer
    createVisibilityToggle(layerName, newLayer);

    // Expand parent and select new layer
    if (newLayer->parent()) {
        newLayer->parent()->setExpanded(true);
    }
    layerTree->setCurrentItem(newLayer);

    // Set as active layer (first layer becomes active automatically)
    setActiveLayer(layerName);

    // Emit signal
    emit layerAdded(layerName);
}

// Add layer via script by amjad
void LayerPanel::addLayerFromScript(const QString& name)
{
    if (layerExists(name))
        return;
    if (!rootLayersItem)
        return;
    QTreeWidgetItem *newLayer = new QTreeWidgetItem();
    newLayer->setText(0, name);
    newLayer->setFlags(newLayer->flags() | Qt::ItemIsEditable);
    rootLayersItem->addChild(newLayer);
    layerItems[name] = newLayer;
    layerShapes[name] = QStringList();
    layerVisibility[name] = true;
    createVisibilityToggle(name, newLayer);
    setActiveLayer(name);
    // refresh tree UI
    QTreeWidget* tree = rootLayersItem->treeWidget();
    if (tree) {
        tree->expandItem(rootLayersItem);
        tree->setCurrentItem(newLayer);
        tree->viewport()->update();
    }
}


void LayerPanel::requestDeleteSelectedItem()
{
    QTreeWidgetItem* selected = layerTree->currentItem();
    if (!selected) return;

    QString itemData = selected->data(0, Qt::UserRole).toString();
    if (itemData.isEmpty()) {
        itemData = getFullLayerName(selected);
    }

    bool isShape = !itemData.isEmpty() && shapeToLayer.contains(itemData);
    bool isRaster = rasterLayers.contains(itemData);
    bool isVector = layerItems.contains(itemData);

    if (!isShape && !isRaster && !isVector) return;

    QString msg;
    if (isShape) {
        msg = QString("Are you sure you want to delete \"%1\"?").arg(shapeDisplayNames.value(itemData, itemData));
    } else if (isRaster) {
        msg = QString("Are you sure you want to delete \"%1\"?").arg(itemData);
    } else if (isVector) {
        msg = QString("Are you sure you want to delete \"%1\" and all of its contents?").arg(itemData);
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Confirm Deletion");
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    // Process deletion after confirmation
    if (isShape) {

        if (m_layerHost) {
            m_layerHost->deleteObjectFromCanvas(itemData);
        } else {
            removeShapeFromLayer(itemData);
        }
    } else if (isRaster) {
        removeRasterLayer(itemData);
    } else if (isVector) {
        removeLayer();
    }
}

// %%% Remove Layer %%%
/* Remove selected layer — works for both vector and raster layers */
void LayerPanel::removeLayer()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();
    if (!selectedItem) {
        // show message and return
        return;
    }

    QString layerName = selectedItem->data(0, Qt::UserRole).toString();
    if (layerName.isEmpty())
        layerName = getFullLayerName(selectedItem);

    const bool isRaster = rasterLayers.contains(layerName);
    const bool isVector = layerItems.contains(layerName);

    if (!isRaster && !isVector) return;

    // confirmation dialog (same as before) ...

    if (isVector) {
        // ── Vector layer cleanup ──────────────────────────────────────────
        // First remove from tree (vector layers have no shape that calls back)
        QTreeWidgetItem* parent = selectedItem->parent();
        if (parent)
            parent->removeChild(selectedItem);
        else
            layerTree->takeTopLevelItem(layerTree->indexOfTopLevelItem(selectedItem));
        delete selectedItem;

        QStringList shapes = layerShapes.value(layerName);
        emit layerWithShapesRemoved(shapes);
        emit geoJsonLayerRemoved(layerName);

        for (const QString& shapeId : shapes) {
            shapeToLayer.remove(shapeId);
            shapeDisplayNames.remove(shapeId);
        }

        layerItems.remove(layerName);
        layerShapes.remove(layerName);
        layerVisibility.remove(layerName);
        layerNameLabels.remove(layerName);
        expandButtons.remove(layerName);

        if (visibilityToggleWidgets.contains(layerName)) {
            delete visibilityToggleWidgets.take(layerName);
        }

        // Update active layer
        if (activeLayerName == layerName) {
            activeLayerName.clear();
            clearActiveLayerVisual();
            if (!layerItems.isEmpty())
                setActiveLayer(layerItems.firstKey());
        }
        emit layerRemoved(layerName);

    } else {
        // CHAITANYA JAIN CHANGE:
        // Why: Refactored complete raster-layer deletion logic to delegate to removeRasterLayer(layerName).
        /*
        // Delete the shape from canvas directly
        QString shapeId;
        for (auto it = shapeToRasterLayer.constBegin(); it != shapeToRasterLayer.constEnd(); ++it) {
            if (it.value() == layerName) {
                shapeId = it.key();
                break;
            }
        }

        if (m_canvasWidget && !shapeId.isEmpty()) {
            for (auto it = m_canvasWidget->getTempMeshes().begin(); it != m_canvasWidget->getTempMeshes().end(); ++it) {
                if (it->name == shapeId) {
                    m_canvasWidget->getTempMeshes().erase(it);
                    m_canvasWidget->update();
                    break;
                }
            }
        }
        */
        removeRasterLayer(layerName);
    }
}// %%% Rename Layer %%%
/* Rename selected layer */
bool LayerPanel::eventFilter(QObject* obj, QEvent* event)
{
    // ── Shape-to-layer drag-and-drop (intercepted on layerTree->viewport()) ──
    if (obj == layerTree->viewport()) {

        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent* de = static_cast<QDragEnterEvent*>(event);
            // Only accept drags that started inside the tree itself
            if (de->source() == layerTree) {
                de->acceptProposedAction();
                return true;
            }
            de->ignore();
            return true;
        }

        if (event->type() == QEvent::DragMove) {
            QDragMoveEvent* dm = static_cast<QDragMoveEvent*>(event);
            if (dm->source() != layerTree) { dm->ignore(); return true; }

            QTreeWidgetItem* dragged = layerTree->currentItem();
            // Use resolveDropTargetLayer so empty layers (no visible child rows)
            // are found even when itemAt() returns nullptr over the blank area.
            QTreeWidgetItem* target  = resolveDropTargetLayer(dm->pos());
            if (isValidShapeToLayerDrop(dragged, target))
                dm->acceptProposedAction();
            else
                dm->ignore();
            return true;
        }

        if (event->type() == QEvent::Drop) {
            QDropEvent* de = static_cast<QDropEvent*>(event);
            if (de->source() != layerTree) { de->ignore(); return true; }

            QTreeWidgetItem* dragged = layerTree->currentItem();
            // Same fix: resolve through empty space onto the layer row above.
            QTreeWidgetItem* target  = resolveDropTargetLayer(de->pos());

            if (!isValidShapeToLayerDrop(dragged, target)) {
                de->ignore();
                return true;
            }

            // Resolve target layer name from UserRole (set in createVisibilityToggle)
            QString targetLayerName = target->data(0, Qt::UserRole).toString();
            if (targetLayerName.isEmpty()) {
                targetLayerName = target->text(0);
                int idx = targetLayerName.indexOf(" (");
                if (idx > 0) targetLayerName = targetLayerName.left(idx);
            }

            QString shapeId = dragged->data(0, Qt::UserRole).toString();

            de->acceptProposedAction();   // consume BEFORE moveShapeToLayer
            moveShapeToLayer(shapeId, targetLayerName);
            return true;
        }
    }

    // ── Double-click on layer name label -> inline rename ─────────────────
    if (event->type() == QEvent::MouseButtonDblClick) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (w) {
            // Check for vector layer name
            QVariant vProp = w->property("layerName");
            if (vProp.isValid()) {
                renameLayerByName(vProp.toString());
                return true;   // consume event
            }
            // Check for raster layer name
            QVariant rProp = w->property("rasterLayerName");
            if (rProp.isValid()) {
                renameLayerByName(rProp.toString());
                return true;
            }
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

bool LayerPanel::isLayerItem(QTreeWidgetItem* item) const
{
    if (!item) return false;
    return layerItems.values().contains(item);
}

bool LayerPanel::isShapeItem(QTreeWidgetItem* item) const
{
    if (!item) return false;
    QString id = item->data(0, Qt::UserRole).toString();
    return !id.isEmpty() && shapeToLayer.contains(id);
}

bool LayerPanel::isValidShapeToLayerDrop(QTreeWidgetItem* dragged,
                                         QTreeWidgetItem* target) const
{
    if (!isShapeItem(dragged) || !isLayerItem(target)) return false;

    // Disallow dropping onto the layer the shape already belongs to
    QString shapeId   = dragged->data(0, Qt::UserRole).toString();
    QString curLayer  = shapeToLayer.value(shapeId);
    QString tgtLayer  = target->data(0, Qt::UserRole).toString();

    if (tgtLayer.isEmpty()) {
        for (auto it = layerItems.constBegin(); it != layerItems.constEnd(); ++it) {
            if (it.value() == target) { tgtLayer = it.key(); break; }
        }
    }

    return !tgtLayer.isEmpty() && tgtLayer != curLayer;
}

QTreeWidgetItem* LayerPanel::resolveDropTargetLayer(const QPoint& viewportPos) const
{
    QTreeWidgetItem* item = layerTree->itemAt(viewportPos);

    // Direct hit on a known layer row
    if (isLayerItem(item)) return item;

    // Hit on a shape row — return its parent layer
    if (isShapeItem(item) && isLayerItem(item->parent())) return item->parent();

    // No item hit (empty space) — scan upward row-by-row
    const int rowH = (layerTree->sizeHintForRow(0) > 4) ? layerTree->sizeHintForRow(0) : 26;
    for (int y = viewportPos.y() - 1; y >= 0; y -= rowH) {
        QTreeWidgetItem* candidate = layerTree->itemAt(QPoint(viewportPos.x(), y));
        if (!candidate) continue;
        if (isLayerItem(candidate))              return candidate;
        if (isShapeItem(candidate) &&
            isLayerItem(candidate->parent()))    return candidate->parent();
    }

    return nullptr;
}

void LayerPanel::renameLayer()
{
    renameLayerByName(QString());   // delegates to the shared implementation
}

/* ─────────────────────────────────────────────────────────────────────────────
 * renameLayerByName()
 *
 * Shared rename implementation called from:
 * ───────────────────────────────────────────────────────────────────────── */
void LayerPanel::renameLayerByName(const QString& targetName)
{
    // ── 1. Resolve which layer to rename ──────────────────────────────────
    QString oldName = targetName;

    if (oldName.isEmpty()) {
        QTreeWidgetItem* sel = layerTree->currentItem();
        if (!sel || sel->text(0) == "Layers") {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
            msgBox.setWindowTitle("No Selection");
            msgBox.setText("Please select a layer to rename!");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            return;
        }
        oldName = getFullLayerName(sel);
        // For items whose text was replaced by a widget, fall back to UserRole
        if (oldName.isEmpty())
            oldName = sel->data(0, Qt::UserRole).toString();
    }

    if (oldName.isEmpty()) return;

    const bool isRaster = rasterLayers.contains(oldName);
    const bool isVector = layerItems.contains(oldName);
    if (!isRaster && !isVector) return;

    // ── 2. Inline QLineEdit placed over the name label ────────────────────
    // Find the name label widget so we can position the editor over it.
    QLabel* nameLabel = isRaster
                            ? rasterNameLabels.value(oldName, nullptr)
                            :  layerNameLabels.value(oldName, nullptr);

    // If the label is available, show an in-place QLineEdit over it.
    // Otherwise fall back to a QInputDialog.
    if (nameLabel && nameLabel->isVisible()) {
        // Create a QLineEdit child of the label's parent widget
        QLineEdit* editor = new QLineEdit(nameLabel->parentWidget());
        editor->setText(oldName);
        editor->selectAll();
        editor->setGeometry(nameLabel->geometry());
        editor->setStyleSheet(
            "QLineEdit {"
            "  background: #1A3A5C;"
            "  color: white;"
            "  border: 1px solid #4A90D9;"
            "  border-radius: 3px;"
            "  font-size: 13px;"
            "  padding: 0 2px;"
            "}");
        editor->show();
        editor->setFocus();
        // Guard flag — prevents editingFinished firing a second time after
        // returnPressed or Escape already committed/cancelled the edit.
        bool* committed = new bool(false);
        // Lambda that commits the rename — runs at most once.
        auto commit = [this, editor, oldName, nameLabel, isRaster, committed](bool apply) {
            if (*committed) return;   // ← swallow the duplicate signal
            *committed = true;
            const QString newName = editor->text().trimmed();
            editor->hide();
            nameLabel->show();
            editor->deleteLater();
            delete committed;
            if (!apply || newName.isEmpty() || newName == oldName) return;
            // Duplicate check — exclude oldName itself (it's still in the map
            const bool existsElsewhere =
                (layerItems.contains(newName)   && newName != oldName) ||
                (rasterLayers.contains(newName) && newName != oldName);
            if (existsElsewhere) {
                QMessageBox msgBox(this);
                msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
                msgBox.setWindowTitle("Duplicate Name");
                msgBox.setText("A layer named \"" + newName + "\" already exists!");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.exec();
                return;
            }
            applyLayerRename(oldName, newName, isRaster);
        };
        // Hide label while editing so they don't overlap
        nameLabel->hide();
        // editingFinished fires on both Return and focus-loss — we use it as
        connect(editor, &QLineEdit::editingFinished, this,
                [commit]() { commit(true); });
        // Escape key cancels without renaming
        QShortcut* esc = new QShortcut(QKeySequence(Qt::Key_Escape), editor);
        connect(esc, &QShortcut::activated, this,
                [commit, editor]() {
                    editor->clearFocus();   // triggers editingFinished → commit(false)
                    // But committed guard means only this path fires
                    commit(false);
                });
        return;
    }

    // ── 3. Fallback: QInputDialog ─────────────────────────────────────────
    bool ok;
    QString newName = QInputDialog::getText(this,
                                            "Rename Layer",
                                            "New Layer Name:",
                                            QLineEdit::Normal,
                                            oldName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName) return;
    newName = newName.trimmed();

    if (layerItems.contains(newName) || rasterLayers.contains(newName)) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("Duplicate Name");
        msgBox.setText("A layer named \"" + newName + "\" already exists!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    applyLayerRename(oldName, newName, isRaster);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * applyLayerRename()
 * Performs all map-key renames and UI updates after the new name is confirmed.
 * ───────────────────────────────────────────────────────────────────────── */
void LayerPanel::applyLayerRename(const QString& oldName, const QString& newName,
                                  bool isRaster)
{
    if (isRaster) {
        // ── Raster layer rename ───────────────────────────────────────────
        if (!rasterLayers.contains(oldName)) return;

        // Update the RasterLayer struct's own name field
        RasterLayer rl = rasterLayers.take(oldName);
        rl.name = newName;
        rasterLayers[newName] = rl;

        if (rasterLayerItems.contains(oldName))
            rasterLayerItems[newName] = rasterLayerItems.take(oldName);
        if (rasterVisibilityWidgets.contains(oldName))
            rasterVisibilityWidgets[newName] = rasterVisibilityWidgets.take(oldName);
        if (rasterExpandButtons.contains(oldName))
            rasterExpandButtons[newName] = rasterExpandButtons.take(oldName);
        if (rasterNameLabels.contains(oldName)) {
            QLabel* lbl = rasterNameLabels.take(oldName);
            lbl->setText(newName);
            lbl->setProperty("rasterLayerName", newName);
            rasterNameLabels[newName] = lbl;
        }
        int idx = rasterLayerOrder.indexOf(oldName);
        if (idx >= 0) rasterLayerOrder[idx] = newName;
        if (rasterLayerItems.contains(newName))
            rasterLayerItems[newName]->setData(0, Qt::UserRole, newName);
        if (rasterLayerItems.contains(newName)) {
            QWidget* w = layerTree->itemWidget(rasterLayerItems[newName], 0);
            if (w) w->setProperty("rasterLayerName", newName);
        }

        // ── FIX: re-key the maps that were missing ─────────────────────

        // 1. layerVisibility: toggle reads/writes by name, must track new name
        if (layerVisibility.contains(oldName))
            layerVisibility[newName] = layerVisibility.take(oldName);

        // 2. layerShapes: addShapeToLayer wrote the shapeId under oldName
        if (layerShapes.contains(oldName))
            layerShapes[newName] = layerShapes.take(oldName);

        // 3. shapeToLayer: shouldDrawShape() calls getLayerForShape() which
        //    reads this map — it must return the new name so isLayerVisible()
        //    looks up the right key
        for (auto it = shapeToLayer.begin(); it != shapeToLayer.end(); ++it) {
            if (it.value() == oldName)
                it.value() = newName;
        }

        // 4. shapeToRasterLayer: used by removeRasterLayer / selectRasterInPanel
        for (auto it = shapeToRasterLayer.begin(); it != shapeToRasterLayer.end(); ++it) {
            if (it.value() == oldName)
                it.value() = newName;
        }

    } else {
        // ── Vector layer rename ───────────────────────────────────────────
        if (!layerItems.contains(oldName)) return;

        // Re-key all vector maps
        layerItems[newName]          = layerItems.take(oldName);
        layerShapes[newName]         = layerShapes.take(oldName);
        layerVisibility[newName]     = layerVisibility.take(oldName);

        // Update reverse shape→layer lookup
        for (const QString& shapeId : layerShapes.value(newName))
            shapeToLayer[shapeId] = newName;

        // Active layer
        if (activeLayerName == oldName)
            activeLayerName = newName;

        // Visibility toggle widget map
        if (visibilityToggleWidgets.contains(oldName))
            visibilityToggleWidgets[newName] = visibilityToggleWidgets.take(oldName);

        // Expand button map
        if (expandButtons.contains(oldName))
            expandButtons[newName] = expandButtons.take(oldName);

        // Re-key name label map + update displayed text
        if (layerNameLabels.contains(oldName)) {
            QLabel* lbl = layerNameLabels.take(oldName);
            lbl->setText(newName);
            lbl->setProperty("layerName", newName);
            layerNameLabels[newName] = lbl;
        }

        // Update tree item
        layerItems[newName]->setData(0, Qt::UserRole, newName);

        // Update the col0Widget property
        {
            QWidget* w = layerTree->itemWidget(layerItems[newName], 0);
            if (w) w->setProperty("layerName", newName);
        }

        updateLayerShapeCount(newName);
    }

    //qDebug() << "Layer renamed:" << oldName << "→" << newName;
}

void LayerPanel::onLayerSelectionChanged()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();
    if (!selectedItem) return;

    // // ----- 1. RASTER LAYERS (must be checked first) -----
    // if (rasterLayerItems.values().contains(selectedItem)) {
    //     if (m_suppressCenter) return;   // ← skip when flag is set

    //     QString layerName = getFullLayerName(selectedItem);
    //     for (auto it = shapeToRasterLayer.constBegin(); it != shapeToRasterLayer.constEnd(); ++it) {
    //         if (it.value() == layerName) {
    //             emit shapeClicked(it.key());   // triggers center + highlight
    //             break;
    //         }
    //     }
    //     return;
    // }
    if (rasterLayerItems.values().contains(selectedItem)) {
        if (m_suppressCenter) return;   // ← already present, works correctly

        QString layerName = getFullLayerName(selectedItem);
        for (auto it = shapeToRasterLayer.constBegin(); it != shapeToRasterLayer.constEnd(); ++it) {
            if (it.value() == layerName) {
                emit shapeClicked(it.key());
                break;
            }
        }
        return;
    }
    // ----- 2. VECTOR LAYERS -----
    QString text = selectedItem->text(0);
    if (text != "Layers" && selectedItem->parent() &&
        selectedItem->parent()->text(0) == "Layers") {
        QString layerName = getFullLayerName(selectedItem);
        setActiveLayer(layerName);
        return;
    }

    // ----- 3. SHAPE ITEMS (handled by itemClicked, not here) -----
}
// %%% Active Layer Management %%%
/* Set active layer by name */
void LayerPanel::setActiveLayer(const QString& layerName)
{
    if (layerName.isEmpty() || !layerExists(layerName)) {
        return;
    }

    // Clear previous active layer visual
    clearActiveLayerVisual();

    // Set new active layer
    activeLayerName = layerName;

    // Update visual indicator
    QTreeWidgetItem *item = findLayerItem(layerName);
    if (item) {
        updateActiveLayerVisual(item);

        // Emit signal
        emit activeLayerChanged(layerName);
    }
}

/* Get active layer tree widget item */
QTreeWidgetItem* LayerPanel::getActiveLayerItem() const
{
    return layerItems.value(activeLayerName, nullptr);
}

// %%% Shape Management %%%
/* Add shape to layer */
void LayerPanel::addShapeToLayer(const QString& shapeId, const QString& shapeType,
                                 const QString& layerName)
{
    QString targetLayer = layerName.isEmpty() ? activeLayerName : layerName;

    if (targetLayer.isEmpty() && !layerItems.isEmpty()) {
        targetLayer = layerItems.firstKey();
        activeLayerName = targetLayer;
    }

    if (!layerShapes.contains(targetLayer))
        layerShapes[targetLayer] = QStringList();

    if (!layerShapes[targetLayer].contains(shapeId)) {
        layerShapes[targetLayer].append(shapeId);
    }

    shapeToLayer[shapeId] = targetLayer;


    if (!shapeDisplayNames.contains(shapeId)) {
        QString baseType = !shapeType.isEmpty() ? shapeType : getShapeTypeFromId(shapeId);
        shapeDisplayNames[shapeId] = generateFriendlyShapeName(shapeId, baseType);
    }

    addShapeItemToTree(targetLayer, shapeId, shapeType);
    updateLayerShapeCount(targetLayer);
}
/* Remove shape from layer */
void LayerPanel::removeShapeFromLayer(const QString& shapeId)
{
    // Check if this shape belongs to a raster layer
    if (shapeToRasterLayer.contains(shapeId)) {
        QString layerName = shapeToRasterLayer[shapeId];
        // Remove the whole raster layer (including its tree item)
        removeRasterLayer(layerName);
        return;
    }  QString layerName = shapeToLayer.value(shapeId);
    if (layerName.isEmpty()) {
        return;
    }

    // Remove from layer's shape list
    layerShapes[layerName].removeAll(shapeId);

    // Remove reverse lookup
    shapeToLayer.remove(shapeId);

    // Remove from tree
    removeShapeItemFromTree(shapeId);

    // Clean up display name
    shapeDisplayNames.remove(shapeId);

    // Update shape count
    updateLayerShapeCount(layerName);

    // Notify canvas to remove shape
    emit shapeRemovedFromLayer(shapeId);
}

// ============================================================================
// moveShapeToLayer
// Transfers a shape from its current layer to targetLayerName.
// ============================================================================
void LayerPanel::moveShapeToLayer(const QString& shapeId,
                                  const QString& targetLayerName)
{
    // ── 1. Validate ──────────────────────────────────────────────────────
    if (shapeId.isEmpty() || targetLayerName.isEmpty()) return;

    const QString sourceLayerName = shapeToLayer.value(shapeId);
    if (sourceLayerName.isEmpty()) {
        qWarning() << "moveShapeToLayer: shapeId" << shapeId << "has no source layer";
        return;
    }
    if (sourceLayerName == targetLayerName) return;   // already there
    if (!layerExists(targetLayerName)) {
        qWarning() << "moveShapeToLayer: target layer" << targetLayerName << "does not exist";
        return;
    }

    // ── 2. Determine shape type (needed to re-create the tree item) ──────
    const QString shapeType = getShapeTypeFromId(shapeId);

    // Preserve any custom display name the user may have set
    const QString displayName = shapeDisplayNames.value(shapeId, QString());

    // ── 3. Remove from source layer's data structures ────────────────────
    layerShapes[sourceLayerName].removeAll(shapeId);
    shapeToLayer.remove(shapeId);
    removeShapeItemFromTree(shapeId);          // deletes the QTreeWidgetItem
    updateLayerShapeCount(sourceLayerName);

    // ── 4. Add to target layer's data structures ─────────────────────────
    if (!layerShapes[targetLayerName].contains(shapeId))
        layerShapes[targetLayerName].append(shapeId);
    shapeToLayer[shapeId] = targetLayerName;

    // Restore display name before calling addShapeItemToTree so it picks it up
    if (!displayName.isEmpty())
        shapeDisplayNames[shapeId] = displayName;

    addShapeItemToTree(targetLayerName, shapeId, shapeType);
    updateLayerShapeCount(targetLayerName);

    // ── 5. Expand the target layer so the user sees the moved shape ───────
    QTreeWidgetItem* targetItem = layerItems.value(targetLayerName, nullptr);
    if (targetItem) targetItem->setExpanded(true);

    // ── 6. Notify canvas to re-render ─────────────────────────────────────
    emit layerVisibilityChanged(sourceLayerName,
                                layerVisibility.value(sourceLayerName, true));
    emit layerVisibilityChanged(targetLayerName,
                                layerVisibility.value(targetLayerName, true));

    // Dedicated signal for any other interested observers
    emit shapeMovedToLayer(shapeId, sourceLayerName, targetLayerName);

    //qDebug() << "Shape" << shapeId << "moved from layer"
    // << sourceLayerName << "→" << targetLayerName;
}

/* Get layer name for a shape */
QString LayerPanel::getLayerForShape(const QString& shapeId) const
{
    return shapeToLayer.value(shapeId);
}

/* Get custom display name for a shape */
QString LayerPanel::getShapeDisplayName(const QString& shapeId) const
{
    return shapeDisplayNames.value(shapeId, shapeId);
}

/* Set custom display name for a shape and update tree UI */
void LayerPanel::setShapeDisplayName(const QString& shapeId, const QString& displayName)
{
    shapeDisplayNames[shapeId] = displayName;

    // Update the name label inside the shape row widget
    for (QTreeWidgetItem *layerItem : layerItems.values()) {
        for (int i = 0; i < layerItem->childCount(); ++i) {
            QTreeWidgetItem *child = layerItem->child(i);
            if (child->data(0, Qt::UserRole).toString() == shapeId) {
                // Find the QLabel that holds the display name inside the widget
                QWidget* w = layerTree->itemWidget(child, 0);
                if (w) {
                    QLabel* lbl = w->findChild<QLabel*>("", Qt::FindDirectChildrenOnly);
                    // The name label is the second QLabel (after the dot icon)
                    QList<QLabel*> labels = w->findChildren<QLabel*>();
                    if (labels.size() >= 2)
                        labels[1]->setText(displayName);
                    else if (labels.size() == 1)
                        labels[0]->setText(displayName);
                }
                return;
            }
        }
    }
}

/* Rename a shape via dialog */
void LayerPanel::renameShape(const QString& shapeId)
{
    // Get current display name
    QString currentName = shapeDisplayNames.contains(shapeId)
                              ? shapeDisplayNames[shapeId]
                              : shapeId;

    QInputDialog dlg(this);
    dlg.setStyleSheet(LayerPanelStyles::InputDialog);
    dlg.setWindowTitle("Rename Shape");
    dlg.setLabelText("Enter new name for this shape:");
    dlg.setTextValue(currentName);
    dlg.resize(350, 120);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString newName = dlg.textValue().trimmed();
    if (newName.isEmpty() || newName == currentName)
        return;

    setShapeDisplayName(shapeId, newName);
}


void LayerPanel::updateLayerShapeCount(const QString& layerName)
{
    QTreeWidgetItem *item = findLayerItem(layerName);
    if (!item) return;

    int shapeCount = layerShapes.value(layerName).count();
    QString displayName = shapeCount > 0
                              ? QString("%1 (%2)").arg(layerName).arg(shapeCount)
                              : layerName;

    // Update label in col0 widget
    QWidget* col0Widget = layerTree->itemWidget(item, 0);
    if (col0Widget) {
        QLabel* label = col0Widget->findChild<QLabel*>();
        if (label) {
            label->setText(displayName);
        }
    }

    // Still store name in UserData for getFullLayerName()
    item->setData(0, Qt::UserRole, layerName);
}
void LayerPanel::selectShapeInPanel(const QString& shapeId)
{
    if (shapeId.isEmpty()) return;

    m_suppressCenter = true;
    layerTree->blockSignals(true);

    // ============================================================
    // IMPORTANT:
    // Clear previous selection before selecting the new item.
    // This prevents multiple shapes/text items staying highlighted
    // in the Layer Panel.
    // ============================================================
    layerTree->clearSelection();

    // ============================================================
    // RASTER LAYER
    // ============================================================
    if (shapeToRasterLayer.contains(shapeId)) {

        QString layerName = shapeToRasterLayer[shapeId];

        QTreeWidgetItem* rasterItem =
            rasterLayerItems.value(layerName, nullptr);

        if (rasterItem) {

            if (rootLayersItem)
                rootLayersItem->setExpanded(true);

            // Select only this item
            layerTree->setCurrentItem(rasterItem);
            rasterItem->setSelected(true);

            layerTree->scrollToItem(
                rasterItem,
                QAbstractItemView::EnsureVisible
                );
        }

        layerTree->blockSignals(false);
        m_suppressCenter = false;
        return;
    }

    // ============================================================
    // VECTOR SHAPES / TEXT / POINT / CIRCLE / POLYGON ETC.
    // ============================================================
    for (QTreeWidgetItem* layerItem : layerItems.values()) {

        if (!layerItem)
            continue;

        for (int i = 0; i < layerItem->childCount(); ++i) {

            QTreeWidgetItem* child = layerItem->child(i);

            if (!child)
                continue;

            QString itemShapeId =
                child->data(0, Qt::UserRole).toString();

            if (itemShapeId == shapeId) {

                // Expand parent layer
                layerItem->setExpanded(true);

                // Select only newly clicked shape/text
                layerTree->setCurrentItem(child);
                child->setSelected(true);

                // Bring item into visible area
                layerTree->scrollToItem(
                    child,
                    QAbstractItemView::EnsureVisible
                    );

                layerTree->blockSignals(false);
                m_suppressCenter = false;
                return;
            }
        }
    }

    // Shape wasn't found, restore state safely
    layerTree->blockSignals(false);
    m_suppressCenter = false;
}
// %%% Layer Queries %%%
/* Check if layer exists */
bool LayerPanel::layerExists(const QString& layerName) const
{
    return layerItems.contains(layerName) || rasterLayers.contains(layerName);
}

/* Get all shapes in a layer */
QStringList LayerPanel::getShapesInLayer(const QString& layerName) const
{
    return layerShapes.value(layerName);
}

// %%% Visibility Management %%%
/* Check if layer is visible */
bool LayerPanel::isLayerVisible(const QString& layerName) const
{
    return layerVisibility.value(layerName, true);
}

/* Set layer visibility */
void LayerPanel::setLayerVisibility(const QString& layerName, bool visible)
{
    if (!layerExists(layerName)) {
        return;
    }

    layerVisibility[layerName] = visible;

    // Update toggle icon
    updateVisibilityToggleIcon(layerName, visible);

    // Emit signal
    emit layerVisibilityChanged(layerName, visible);
}

/* Handle visibility toggle clicked */
void LayerPanel::onVisibilityToggleClicked(const QString& layerName)
{
    bool currentVisibility = isLayerVisible(layerName);
    setLayerVisibility(layerName, !currentVisibility);
}

// %%% Helper Methods %%%
/* Find layer item by name */
QTreeWidgetItem* LayerPanel::findLayerItem(const QString& layerName) const
{
    return layerItems.value(layerName, nullptr);
}

/* Get full layer name */
QString LayerPanel::getFullLayerName(QTreeWidgetItem* item) const
{
    if (!item) {
        return QString();
    }
    // Check if stored in user data
    QString storedName = item->data(0, Qt::UserRole).toString();
    if (!storedName.isEmpty()) {
        return storedName;
    }
    // Extract from display text (remove count if present)
    QString text = item->text(0);
    int countIndex = text.indexOf(" (");
    if (countIndex > 0) {
        return text.left(countIndex);
    }
    return text;
}

/* Update visual indicator for active layer */
void LayerPanel::updateActiveLayerVisual(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }
    // Make it bold
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);
    // Set background color for dark theme
    item->setBackground(0, QBrush(QColor(39, 68, 109)));  // #27446d
    item->setForeground(0, QBrush(QColor(255, 255, 255)));
}

/* Clear active layer visual indicator */
void LayerPanel::clearActiveLayerVisual()
{
    QTreeWidgetItem *item = getActiveLayerItem();
    if (!item) {
        return;
    }
    // Reset font
    QFont font = item->font(0);
    font.setBold(false);
    item->setFont(0, font);
    // Clear background
    item->setBackground(0, QBrush(QColor(15, 38, 54)));  // #0F2636
}

/* Add shape item under layer in tree */
void LayerPanel::addShapeItemToTree(const QString& layerName, const QString& shapeId,
                                    const QString& shapeType)
{
    QTreeWidgetItem *layerItem = findLayerItem(layerName);
    if (!layerItem) return;

    // === STRONG FIX: Imported name ko bilkul same rakho ===
    QString displayName = shapeDisplayNames.value(shapeId);
    if (displayName.isEmpty()) {
        displayName = shapeId;           // exact name from GeoJSON
        shapeDisplayNames[shapeId] = displayName;
    }

    QTreeWidgetItem *shapeItem = new QTreeWidgetItem(layerItem);
    shapeItem->setData(0, Qt::UserRole, shapeId);
    shapeItem->setFlags(shapeItem->flags() & ~Qt::ItemIsEditable);
    shapeItem->setText(0, "");

    QWidget* shapeWidget = new QWidget();
    shapeWidget->setStyleSheet("background: transparent;");
    QHBoxLayout* shapeLayout = new QHBoxLayout(shapeWidget);
    shapeLayout->setContentsMargins(8, 0, 2, 0);
    shapeLayout->setSpacing(4);

    QLabel* dotLabel = new QLabel("–");
    dotLabel->setFixedWidth(12);
    dotLabel->setStyleSheet("color: #7A8FA6; background: transparent; font-size: 10px;");

    QLabel* nameLabel = new QLabel(displayName);
    nameLabel->setStyleSheet("color: #C8D6E0; background: transparent; font-size: 12px;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setProperty("shapeId", shapeId);

    shapeLayout->addWidget(dotLabel);
    shapeLayout->addWidget(nameLabel);
    shapeLayout->addStretch();

    layerTree->setItemWidget(shapeItem, 0, shapeWidget);
    layerItem->setExpanded(true);
}
/* Remove shape item from tree */
void LayerPanel::removeShapeItemFromTree(const QString& shapeId)
{
    // Search through all layer items
    for (QTreeWidgetItem *layerItem : layerItems.values()) {
        for (int i = 0; i < layerItem->childCount(); ++i) {
            QTreeWidgetItem *child = layerItem->child(i);
            if (child->data(0, Qt::UserRole).toString() == shapeId) {
                layerItem->removeChild(child);
                delete child;
                return;
            }
        }
    }
}

void LayerPanel::createVisibilityToggle(const QString& layerName, QTreeWidgetItem* item)
{
    // ── Column 0: [▶] + LayerName label widget ────────────────────────
    QWidget* col0Widget = new QWidget();
    col0Widget->setStyleSheet("background: transparent;");
    QHBoxLayout* col0Layout = new QHBoxLayout(col0Widget);
    col0Layout->setContentsMargins(4, 0, 0, 0);
    col0Layout->setSpacing(4);

    // Expand button FIRST (left-most)
    QPushButton* expandBtn = new QPushButton("▶");
    expandBtn->setFixedSize(16, 16);
    expandBtn->setFlat(true);
    expandBtn->setCursor(Qt::PointingHandCursor);
    expandBtn->setToolTip("Expand / Collapse");
    expandBtn->setStyleSheet(
        "QPushButton {"
        "  border: none; background: transparent;"
        "  color: #AAAAAA; font-size: 10px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  color: #FFFFFF;"
        "  background: rgba(255,255,255,0.15);"
        "  border-radius: 0px;"
        "}"
        );

    // Layer name label AFTER the button
    QLabel* nameLabel = new QLabel(layerName);
    nameLabel->setStyleSheet("color: white; background: transparent; font-size: 13px;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    col0Layout->addWidget(expandBtn);
    col0Layout->addWidget(nameLabel);
    col0Layout->addStretch();

    // Store label so renameLayer() can update the displayed text directly
    layerNameLabels[layerName] = nameLabel;

    layerTree->setItemWidget(item, 0, col0Widget);
    item->setText(0, "");  // Clear text since widget handles display
    // Set UserRole immediately so isLayerItem / resolveDropTargetLayer can
    item->setData(0, Qt::UserRole, layerName);

    // Store expand button reference
    expandButtons[layerName] = expandBtn;

    // ── Double-click on the label → inline rename ─────────────────────
    nameLabel->installEventFilter(this);
    nameLabel->setProperty("layerName", layerName);
    nameLabel->setCursor(Qt::IBeamCursor);
    nameLabel->setToolTip("Double-click to rename");
    connect(nameLabel, &QLabel::linkActivated, this, [this](){});  // dummy — keeps label alive
    // Use a lambda via a small helper QObject to catch mouseDoubleClickEvent
    col0Widget->installEventFilter(this);
    col0Widget->setProperty("layerName", layerName);

    // ── Column 1: ✓/✗ Visibility button ──────────────────────────────
    QPushButton* visBtn = new QPushButton("✓");
    visBtn->setFixedSize(22, 22);
    visBtn->setFlat(true);
    visBtn->setCursor(Qt::PointingHandCursor);
    visBtn->setToolTip("Show / Hide layer");
    visBtn->setStyleSheet(
        "QPushButton {"
        "  border: none; background: transparent;"
        "  color: #4CAF50; font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255,255,255,0.15); border-radius: 3px;"
        "}"
        );

    QWidget* col1Widget = new QWidget();
    col1Widget->setStyleSheet("background: transparent;");
    QHBoxLayout* col1Layout = new QHBoxLayout(col1Widget);
    col1Layout->setContentsMargins(3, 0, 3, 0);
    col1Layout->setSpacing(0);
    col1Layout->addWidget(visBtn);
    layerTree->setItemWidget(item, 1, col1Widget);

    // Store visibility widget reference
    visibilityToggleWidgets[layerName] = col1Widget;

    // ── Connect expand button ─────────────────────────────────────────
    connect(expandBtn, &QPushButton::clicked, this, [this, item, expandBtn]() {
        item->setExpanded(!item->isExpanded());
        expandBtn->setText(item->isExpanded() ? "▼" : "▶");
    });

    connect(layerTree, &QTreeWidget::itemExpanded,
            this, [expandBtn, item](QTreeWidgetItem* changed) {
                if (changed == item) expandBtn->setText("▼");
            });
    connect(layerTree, &QTreeWidget::itemCollapsed,
            this, [expandBtn, item](QTreeWidgetItem* changed) {
                if (changed == item) expandBtn->setText("▶");
            });

    // ── Connect visibility button ─────────────────────────────────────
    connect(visBtn, &QPushButton::clicked, this, [this, col1Widget]() {
        // Capture col1Widget not layerName: layerName is stale after rename.
        for (auto it = visibilityToggleWidgets.constBegin();
             it != visibilityToggleWidgets.constEnd(); ++it) {
            if (it.value() == col1Widget) {
                onVisibilityToggleClicked(it.key());
                return;
            }
        }
    });
}
// =====================================================================
// STEP 4: Replace updateVisibilityToggleIcon() completely
// =====================================================================

void LayerPanel::updateVisibilityToggleIcon(const QString& layerName, bool visible)
{
    QWidget* w = visibilityToggleWidgets.value(layerName, nullptr);
    if (!w) return;

    QPushButton* visBtn = w->findChild<QPushButton*>();
    if (!visBtn) return;

    if (visible) {
        visBtn->setText("✓");
        visBtn->setStyleSheet(
            "QPushButton {"
            "  border: none; background: transparent;"
            "  color: #4CAF50; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 3px; }"
            );
        visBtn->setToolTip("Click to hide layer");
    } else {
        visBtn->setText("✗");
        visBtn->setStyleSheet(
            "QPushButton {"
            "  border: none; background: transparent;"
            "  color: #F44336; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 3px; }"
            );
        visBtn->setToolTip("Click to show layer");
    }
}

// %%% Export Layer %%%
/* Export selected layer to various GIS formats */
void LayerPanel::exportLayer()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();

    if (!selectedItem || selectedItem->text(0) == "Layers") {
        QMessageBox::warning(this, "No Selection",
                             "Please select a layer to export!");
        return;
    }

    QString layerName = getFullLayerName(selectedItem);
    QStringList shapesInLayer = getShapesInLayer(layerName);
    if (shapesInLayer.isEmpty()) {
        QMessageBox::information(this, "Empty Layer",
                                 QString("Layer '%1' contains no shapes to export.").arg(layerName));
        return;
    }
    // CHAITANYA JAIN CHANGE:
    // Why: Check if layer host is available instead of canvas widget.
    // if (!m_canvasWidget) { QMessageBox::critical(this, "Export Error", "Canvas widget not available for export!"); return; }
    if (!m_layerHost) {
        QMessageBox::critical(this, "Export Error",
                              "Layer host not available for export!");
        return;
    }
    // Create format selection dialog
    QStringList formats;
    formats << "GeoJSON (*.geojson)"
            << "KML (*.kml)"
            << "GML (*.gml)"
            << "Shapefile (*.shp)"
            << "FlatGeobuf (*.fgb)"
            << "CSV (*.csv)";

    bool ok;
    QString selectedFormat = QInputDialog::getItem(this,
                                                   "Select Export Format",
                                                   "Choose export format for layer '" + layerName + "':",
                                                   formats, 0, false, &ok);

    if (!ok) {
        return;  // User cancelled
    }

    // Get save file path
    QString filter;
    QString defaultExt;

    if (selectedFormat.contains("GeoJSON")) {
        filter = "GeoJSON Files (*.geojson)";
        defaultExt = ".geojson";
    } else if (selectedFormat.contains("KML")) {
        filter = "KML Files (*.kml)";
        defaultExt = ".kml";
    } else if (selectedFormat.contains("GML")) {
        filter = "GML Files (*.gml)";
        defaultExt = ".gml";
    } else if (selectedFormat.contains("Shapefile")) {
        filter = "Shapefiles (*.shp)";
        defaultExt = ".shp";
    } else if (selectedFormat.contains("FlatGeobuf")) {
        filter = "FlatGeobuf Files (*.fgb)";
        defaultExt = ".fgb";
    } else if (selectedFormat.contains("CSV")) {
        filter = "CSV Files (*.csv)";
        defaultExt = ".csv";
    }

    QString defaultFileName = layerName + defaultExt;
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Export Layer - " + layerName,
                                                    QDir::homePath() + "/" + defaultFileName,
                                                    filter);

    if (filePath.isEmpty()) {
        return;  // User cancelled
    }

    // Ensure correct extension
    if (!filePath.endsWith(defaultExt, Qt::CaseInsensitive)) {
        filePath += defaultExt;
    }

    // Export based on format
    // CHAITANYA JAIN CHANGE:
    // Why: Delegated export handling to the new decoupled export classes.
    bool success = false;
    if (selectedFormat.contains("GeoJSON")) {
        success = GeoJSON::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("KML")) {
        success = KML::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("GML")) {
        success = GML::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("Shapefile")) {
        success = Shapefile::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("FlatGeobuf")) {
        success = FlatGeobuf::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("CSV")) {
        success = CSV::exportLayer(m_layerHost, layerName, shapesInLayer, filePath);
    }
    if (success) {
        QMessageBox::information(this, "Export Successful",
                                 QString("Layer '%1' exported successfully to:\n%2")
                                     .arg(layerName, filePath));
    } else {
        QMessageBox::critical(this, "Export Failed",
                              QString("Failed to export layer '%1'.").arg(layerName));
    }
}

// CHAITANYA JAIN CHANGE:
// Why: Removed createGeoJSONFeature, createKMLPlacemark, createGMLFeature, getGeometryAsGeoJSON, getGeometryAsKML, getGeometryAsGML, and getShapeType helper functions.
// These have been moved to standalone classes/modules inside the Layer folder.
/* Serialize all layer data to JSON */
QJsonObject LayerPanel::toJson() const
{
    QJsonObject json;

    // Save active layer
    json["activeLayer"] = activeLayerName;

    // Save layer structure
    QJsonArray layersArray;

    // Iterate through all layer items
    for (auto it = layerItems.constBegin(); it != layerItems.constEnd(); ++it) {
        QString layerName = it.key();
        QTreeWidgetItem* item = it.value();

        if (!item) continue;

        QJsonObject layerObj;
        layerObj["name"] = layerName;

        // Save visibility state
        layerObj["visible"] = layerVisibility.value(layerName, true);

        // Save shapes in this layer
        QJsonArray shapesArray;
        QStringList shapes = layerShapes.value(layerName);
        for (const QString& shapeId : shapes) {
            QJsonObject shapeObj;
            shapeObj["id"] = shapeId;
            shapeObj["shapeType"] = getShapeTypeFromId(shapeId);

            if (shapeDisplayNames.contains(shapeId))
                shapeObj["displayName"] = shapeDisplayNames[shapeId];
            shapesArray.append(shapeObj);
        }
        layerObj["shapes"] = shapesArray;

        // Save parent layer name (for hierarchical structure)
        if (item->parent() && item->parent() != rootLayersItem) {
            QString parentName = getFullLayerName(item->parent());
            layerObj["parent"] = parentName;
        } else {
            layerObj["parent"] = "";  // Top-level layer
        }

        layersArray.append(layerObj);
    }

    json["layers"] = layersArray;

    // //qDebug() << "✓ LayerPanel::toJson() - Saved" << layersArray.size() << "layers";

    return json;
}

/* Deserialize layer data from JSON */
void LayerPanel::fromJson(const QJsonObject& json)
{
    // //qDebug() << "✓ LayerPanel::fromJson() - Starting layer restoration";

    // Clear existing layer data (but keep UI structure)
    layerShapes.clear();
    shapeToLayer.clear();
    layerVisibility.clear();

    // CHAITANYA JAIN CHANGE:
    // Reset document-scoped GIS import state during New File.
    // Reason: the previous document's imported-file registry was causing false
    // "already imported" results in a completely new document.
    rasterLayers.clear();

    // Remove all layer items except root
    if (layerTree && rootLayersItem) {
        QList<QTreeWidgetItem*> itemsToRemove;
        for (int i = 0; i < rootLayersItem->childCount(); ++i) {
            itemsToRemove.append(rootLayersItem->child(i));
        }
        for (QTreeWidgetItem* item : itemsToRemove) {
            delete item;
        }
    }
    layerItems.clear();

    // Restore layers
    QJsonArray layersArray = json["layers"].toArray();

    // First pass: Create all layers (without parent relationships)
    QMap<QString, QJsonObject> layerDataMap;
    for (const QJsonValue& val : layersArray) {
        QJsonObject layerObj = val.toObject();
        QString layerName = layerObj["name"].toString();
        layerDataMap[layerName] = layerObj;
    }

    // Second pass: Create layers in correct hierarchy
    // First create top-level layers
    for (const QString& layerName : layerDataMap.keys()) {
        QJsonObject layerObj = layerDataMap[layerName];
        QString parentName = layerObj["parent"].toString();

        if (parentName.isEmpty()) {
            // Top-level layer
            QTreeWidgetItem* layerItem = new QTreeWidgetItem(rootLayersItem);
            layerItem->setText(0, layerName);
            layerItem->setFlags(layerItem->flags() | Qt::ItemIsEditable);
            layerItems[layerName] = layerItem;

            // Create visibility toggle
            createVisibilityToggle(layerName, layerItem);
        }
    }

    // Then create child layers
    for (const QString& layerName : layerDataMap.keys()) {
        QJsonObject layerObj = layerDataMap[layerName];
        QString parentName = layerObj["parent"].toString();

        if (!parentName.isEmpty() && layerItems.contains(parentName)) {
            // Child layer
            QTreeWidgetItem* parentItem = layerItems[parentName];
            QTreeWidgetItem* layerItem = new QTreeWidgetItem(parentItem);
            layerItem->setText(0, layerName);
            layerItem->setFlags(layerItem->flags() | Qt::ItemIsEditable);
            layerItems[layerName] = layerItem;

            // Create visibility toggle
            createVisibilityToggle(layerName, layerItem);
        }
    }

    // Third pass: Restore layer properties and shapes
    for (const QString& layerName : layerDataMap.keys()) {
        QJsonObject layerObj = layerDataMap[layerName];

        // Restore visibility
        bool visible = layerObj["visible"].toBool();
        layerVisibility[layerName] = visible;
        updateVisibilityToggleIcon(layerName, visible);

        // Restore shapes
        QJsonArray shapesArray = layerObj["shapes"].toArray();
        QStringList shapes;
        for (const QJsonValue& shapeVal : shapesArray) {
            QString shapeId;
            QString displayName;
            QString savedShapeType;

            // Support both old format (plain string) and new format (object with id + displayName)
            if (shapeVal.isString()) {
                shapeId = shapeVal.toString();
            } else if (shapeVal.isObject()) {
                QJsonObject shapeObj = shapeVal.toObject();
                shapeId = shapeObj["id"].toString();
                displayName = shapeObj["displayName"].toString();
                savedShapeType = shapeObj["shapeType"].toString();
            }

            if (shapeId.isEmpty()) continue;

            shapes.append(shapeId);
            shapeToLayer[shapeId] = layerName;

            if (!displayName.isEmpty())
                shapeDisplayNames[shapeId] = displayName;

            // *** CRITICAL FIX: Add shape item to tree ***
            QString shapeType = getShapeTypeFromId(shapeId);
            addShapeItemToTree(layerName, shapeId, shapeType);
        }
        layerShapes[layerName] = shapes;

        // Update shape count in tree
        updateLayerShapeCount(layerName);
    }

    // Restore active layer
    QString savedActiveLayer = json["activeLayer"].toString();
    if (!savedActiveLayer.isEmpty() && layerItems.contains(savedActiveLayer)) {
        setActiveLayer(savedActiveLayer);
    } else if (!layerItems.isEmpty()) {
        // If saved active layer doesn't exist, set first layer as active
        setActiveLayer(layerItems.firstKey());
    }

    // Expand all layer items to show shapes
    if (rootLayersItem) {
        rootLayersItem->setExpanded(true);

        // Expand each layer item so shapes are visible
        for (QTreeWidgetItem* layerItem : layerItems.values()) {
            if (layerItem) {
                layerItem->setExpanded(true);
            }
        }
    }

    // //qDebug() << "✓ LayerPanel::fromJson() - Restored" << layerItems.size() << "layers";
    // //qDebug() << "  Active layer:" << activeLayerName;
    // //qDebug() << "  Total shapes restored:" << shapeToLayer.size();

    // Emit signals to notify that layers have been restored
    for (const QString& layerName : layerItems.keys()) {
        emit layerAdded(layerName);
    }
}
QString LayerPanel::getShapeTypeFromId(const QString& shapeId) const
{
    // Shape IDs follow patterns like: "Circle_1", "Rectangle_2", "Polyline_3"
    if (shapeId.startsWith("Circle") || shapeId.startsWith("TempCircle")) {
        return "Circle";
    } else if (shapeId.startsWith("Rectangle") || shapeId.startsWith("TempRectangle")) {
        return "Rectangle";
    } else if (shapeId.startsWith("Polygon") || shapeId.startsWith("TempPolygon")) {
        return "Polygon";
    } else if (shapeId.startsWith("Polyline") || shapeId.startsWith("Line") ||
               shapeId.startsWith("TempPolyline")) {
        return "Line";
    } else if (shapeId.startsWith("Point")) {
        return "Point";
    } else if (shapeId.startsWith("Text") || shapeId.startsWith("TempText")) {
        return "Text";
    }

    else if (shapeId.startsWith("Hospital") ||
             shapeId.startsWith("School") ||
             shapeId.startsWith("Forest") ||
             shapeId.startsWith("TempBitmap") ||
             shapeId.startsWith("Bitmap")) {
        return "Bitmap";
    }
    return "Shape";  // Default fallback
}

// CHAITANYA JAIN CHANGE:
// Why: Removed exportToFlatGeobuf, exportToCSV, createCSVLine, and createWKTGeometry helper functions.
// These have been moved to standalone classes/modules inside the Layer folder.
// %%% Add Raster Layer %%%
/* Open a file dialog, load the selected raster image and add it to the panel */
void LayerPanel::addRasterLayer()
{
    // ── 1. File dialog ────────────────────────────────────────────────────
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Raster Layer",
        QDir::homePath(),
        "Raster Images (*.tif *.tiff *.jpg *.jpeg *.png *.bmp *.gif *.webp);;"
        "GeoTIFF (*.tif *.tiff);;"
        "JPEG (*.jpg *.jpeg);;"
        "PNG (*.png);;"
        "BMP (*.bmp);;"
        "GIF (*.gif);;"
        "WebP (*.webp);;"
        "All Files (*)");

    if (filePath.isEmpty())
        return;

    // ── 2. Load image ─────────────────────────────────────────────────────
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("Load Error");
        msgBox.setText(QString("Could not load image:\n%1\n\n"
                               "Make sure the file is a valid raster format.")
                           .arg(filePath));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return;
    }

    // ── 3. Derive a unique display name ───────────────────────────────────
    QString baseName  = QFileInfo(filePath).baseName();
    QString layerName = baseName;
    int suffix = 1;
    while (layerExists(layerName) || rasterLayers.contains(layerName))
        layerName = QString("%1_%2").arg(baseName).arg(suffix++);

    // ── 4. Build a preliminary RasterLayer with the pixmap ────────────────
    RasterLayer rl;
    rl.name     = layerName;
    rl.filePath = filePath;
    rl.pixmap   = pixmap;
    rl.visible  = true;
    rl.opacity  = 1.0;
    // extents left at default (0,0,0,0) — will be filled below

    // ── 5. Determine geographic extents ───────────────────────────────────
    bool georefFound = false;

    // Priority 1: world-file (.tfw, .jgw, .pgw, …)
    if (!georefFound)
        georefFound = readWorldFile(filePath, rl);

    // Priority 2: embedded GeoTIFF tags (TIFF files only)
    if (!georefFound)
        georefFound = readGeoTiffExtents(filePath, rl);

    // Priority 3: sensible view-centred default
    if (!georefFound) {
        // CHAITANYA JAIN CHANGE:
        // Why: Calculates default extents using the ILayerHost interface.
        // rl = makeDefaultExtents(rl, m_canvasWidget);
        rl = makeDefaultExtents(rl, m_layerHost);
    }

    // ── 6. Register ───────────────────────────────────────────────────────
    rasterLayers[layerName] = rl;
    rasterLayerOrder.append(layerName);
    createRasterLayerItem(layerName);

    // --- Create editable shape for the raster ---
    layerVisibility[layerName] = true;
    if (m_layerHost) {
        QString shapeId = QString("RasterImage_%1").arg(layerName);
        MeshEntry entry;
        entry.name = shapeId;
        entry.position = new QVector3D((rl.minLon + rl.maxLon) / 2.0,
                                       (rl.minLat + rl.maxLat) / 2.0, 0);
        entry.rotation = new QQuaternion();
        entry.size = new QVector3D((rl.maxLon - rl.minLon) / 2.0,
                                   (rl.maxLat - rl.minLat) / 2.0, 1);
        entry.velocity = new QVector3D(0, 0, 0);
        entry.bitmapPath = rl.filePath;
        entry.mesh = new Mesh();
        entry.mesh->color = new QColor(Qt::white);
        entry.mesh->lineWidth = 1;
        entry.mesh->closePath = false;

        // CHAITANYA JAIN CHANGE:
        // Why: Adds the raster shape's MeshEntry to the canvas via ILayerHost.
        // m_canvasWidget->getTempMeshes().push_back(entry); m_canvasWidget->update();
        m_layerHost->addRasterMesh(entry);
        addShapeToLayer(shapeId, "RasterImage", layerName);

        rl.shapeId = shapeId;
        rasterLayers[layerName] = rl;   // update with shapeId
        shapeToRasterLayer[shapeId] = layerName;

        // CHAITANYA JAIN CHANGE:
        // Why: Zooms/centers the canvas map to the raster coordinates using ILayerHost.
        // m_canvasWidget->gislib->fitToBounds(...); m_canvasWidget->update();
        m_layerHost->fitMapBounds(rl.minLon, rl.minLat, rl.maxLon, rl.maxLat);
    }
    emit rasterLayerChanged();

}

/* Create a tree widget item for a raster layer, including its visibility toggle */
void LayerPanel::createRasterLayerItem(const QString& layerName)
{
    // ── Tree item ─────────────────────────────────────────────────────────
    QTreeWidgetItem* item = new QTreeWidgetItem(rootLayersItem);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(0, Qt::UserRole, layerName);
    rasterLayerItems[layerName] = item;

    // ── Column 0: [▶] + icon + name label ────────────────────────────────
    QWidget* col0Widget = new QWidget();
    col0Widget->setStyleSheet("background: transparent;");
    QHBoxLayout* col0Layout = new QHBoxLayout(col0Widget);
    col0Layout->setContentsMargins(4, 0, 0, 0);
    col0Layout->setSpacing(4);

    // Expand button (kept consistent with vector layer style)
    QPushButton* expandBtn = new QPushButton("▶");
    expandBtn->setFixedSize(16, 16);
    expandBtn->setFlat(true);
    expandBtn->setCursor(Qt::PointingHandCursor);
    expandBtn->setToolTip("Expand / Collapse");
    expandBtn->setStyleSheet(
        "QPushButton {"
        "  border: none; background: transparent;"
        "  color: #AAAAAA; font-size: 10px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  color: #FFFFFF;"
        "  background: rgba(255,255,255,0.15);"
        "  border-radius: 3px;"
        "}"
        );

    // Small raster icon (🗺 or a coloured square to distinguish from vector)
    QLabel* iconLabel = new QLabel("🗺");
    iconLabel->setFixedWidth(18);
    iconLabel->setStyleSheet("color: #F0A500; background: transparent; font-size: 12px;");

    // Layer name label
    QLabel* nameLabel = new QLabel(layerName);
    nameLabel->setStyleSheet(
        "color: #F0D090;"             // Warm amber — visually distinct from vector (white)
        "background: transparent;"
        "font-size: 13px;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    col0Layout->addWidget(expandBtn);
    col0Layout->addWidget(iconLabel);
    col0Layout->addWidget(nameLabel);
    col0Layout->addStretch();

    layerTree->setItemWidget(item, 0, col0Widget);
    item->setText(0, "");   // widget handles display
    item->setData(0, Qt::UserRole, layerName);

    rasterExpandButtons[layerName] = expandBtn;

    // Store name label for rename updates
    rasterNameLabels[layerName] = nameLabel;

    // Double-click on the name label or its container → rename
    nameLabel->installEventFilter(this);
    nameLabel->setProperty("rasterLayerName", layerName);
    nameLabel->setCursor(Qt::IBeamCursor);
    nameLabel->setToolTip("Double-click to rename");
    col0Widget->installEventFilter(this);
    col0Widget->setProperty("rasterLayerName", layerName);

    // ── Column 1: ✓/✗ visibility button ──────────────────────────────────
    QPushButton* visBtn = new QPushButton("✓");
    visBtn->setFixedSize(22, 22);
    visBtn->setFlat(true);
    visBtn->setCursor(Qt::PointingHandCursor);
    visBtn->setToolTip("Show / Hide raster layer");
    visBtn->setStyleSheet(
        "QPushButton {"
        "  border: none; background: transparent;"
        "  color: #4CAF50; font-size: 13px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 3px; }"
        );

    QWidget* col1Widget = new QWidget();
    col1Widget->setStyleSheet("background: transparent;");
    QHBoxLayout* col1Layout = new QHBoxLayout(col1Widget);
    col1Layout->setContentsMargins(3, 0, 3, 0);
    col1Layout->setSpacing(0);
    col1Layout->addWidget(visBtn);

    layerTree->setItemWidget(item, 1, col1Widget);
    rasterVisibilityWidgets[layerName] = col1Widget;

    // ── Connect expand button ─────────────────────────────────────────────
    connect(expandBtn, &QPushButton::clicked, this, [this, item, expandBtn]() {
        item->setExpanded(!item->isExpanded());
        expandBtn->setText(item->isExpanded() ? "▼" : "▶");
    });
    connect(layerTree, &QTreeWidget::itemExpanded,
            this, [expandBtn, item](QTreeWidgetItem* changed) {
                if (changed == item) expandBtn->setText("▼");
            });
    connect(layerTree, &QTreeWidget::itemCollapsed,
            this, [expandBtn, item](QTreeWidgetItem* changed) {
                if (changed == item) expandBtn->setText("▶");
            });

    // ── Connect visibility button ─────────────────────────────────────────
    connect(visBtn, &QPushButton::clicked, this,
            [this, col1Widget]() {
                // Capture col1Widget not layerName: stale after rename.
                for (auto it = rasterVisibilityWidgets.constBegin();
                     it != rasterVisibilityWidgets.constEnd(); ++it) {
                    if (it.value() == col1Widget) {
                        onRasterVisibilityToggleClicked(it.key());
                        return;
                    }
                }
            });

    // Expand the root so the new entry is immediately visible
    if (rootLayersItem) {
        rootLayersItem->setExpanded(true);
    }
}
/* Handle raster visibility toggle button click */
void LayerPanel::onRasterVisibilityToggleClicked(const QString& layerName)
{
    if (!rasterLayers.contains(layerName)) return;

    bool newVisible = !rasterLayers[layerName].visible;
    rasterLayers[layerName].visible = newVisible;
    layerVisibility[layerName] = newVisible;   // keep in sync

    updateRasterVisibilityIcon(layerName, newVisible);
    emit layerVisibilityChanged(layerName, newVisible);
    emit rasterLayerChanged();
}
/* Update the raster visibility toggle button appearance */
void LayerPanel::updateRasterVisibilityIcon(const QString& layerName, bool visible)
{
    QWidget* w = rasterVisibilityWidgets.value(layerName, nullptr);
    if (!w) return;

    QPushButton* visBtn = w->findChild<QPushButton*>();
    if (!visBtn) return;

    if (visible) {
        visBtn->setText("✓");
        visBtn->setStyleSheet(
            "QPushButton {"
            "  border: none; background: transparent;"
            "  color: #4CAF50; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 3px; }"
            );
        visBtn->setToolTip("Click to hide raster layer");
    } else {
        visBtn->setText("✗");
        visBtn->setStyleSheet(
            "QPushButton {"
            "  border: none; background: transparent;"
            "  color: #F44336; font-size: 13px; font-weight: bold;"
            "}"
            "QPushButton:hover { background: rgba(255,255,255,0.15); border-radius: 3px; }"
            );
        visBtn->setToolTip("Click to show raster layer");
    }
}

// Dead drawRasterLayers function removed.


/* =========================================================================
   readWorldFile()
   Reads a plain-text world-file sidecar for geographic extents.
   Writes ONLY to local variables; copies to `out` only on verified success.
   ========================================================================= */
bool LayerPanel::readWorldFile(const QString& imagePath, RasterLayer& out)
{
    QFileInfo fi(imagePath);
    QString base = fi.absolutePath() + "/" + fi.completeBaseName();
    QString ext  = fi.suffix().toLower();

    QStringList candidates;
    if      (ext == "tif"  || ext == "tiff")  candidates << "tfw" << "tifw" << "wld";
    else if (ext == "jpg"  || ext == "jpeg")  candidates << "jgw" << "jpgw" << "wld";
    else if (ext == "png")                    candidates << "pgw" << "pngw" << "wld";
    else if (ext == "bmp")                    candidates << "bpw" << "wld";
    else if (ext == "gif")                    candidates << "gfw" << "wld";
    else if (ext == "webp")                   candidates << "wld";
    else                                      candidates << "wld";

    for (const QString& wext : candidates) {
        QFile wf(base + "." + wext);
        if (!wf.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream ts(&wf);
        QStringList lines;
        while (!ts.atEnd()) {
            QString line = ts.readLine().trimmed();
            if (!line.isEmpty()) lines << line;
        }
        wf.close();

        if (lines.size() < 6) continue;
        bool ok1, ok4, ok5, ok6;
        double pixW  = lines[0].toDouble(&ok1);
        double pixH  = lines[3].toDouble(&ok4);   // negative for north-up
        double ulLon = lines[4].toDouble(&ok5);
        double ulLat = lines[5].toDouble(&ok6);

        if (!(ok1 && ok4 && ok5 && ok6)) continue;
        if (pixW == 0.0 || pixH == 0.0)  continue;

        int imgW = out.pixmap.width();
        int imgH = out.pixmap.height();
        if (imgW <= 0 || imgH <= 0)      continue;

        // ── Compute extents into LOCAL variables first ────────────────────
        // World-file gives the centre of the upper-left pixel.
        double minLon = ulLon - pixW * 0.5;
        double maxLat = ulLat - pixH * 0.5;      // pixH < 0 → maxLat > ulLat
        double maxLon = ulLon + pixW * (imgW - 0.5);
        double minLat = ulLat + pixH * (imgH - 0.5);  // pixH < 0 → minLat < ulLat

        // Ensure min < max (handle non-standard sign conventions)
        if (minLon > maxLon) std::swap(minLon, maxLon);
        if (minLat > maxLat) std::swap(minLat, maxLat);

        // ── Strict geographic sanity check ────────────────────────────────
        if (minLon < -180.1 || maxLon > 180.1)  continue;
        if (minLat < -90.1  || maxLat > 90.1)   continue;
        if (maxLon - minLon < 1e-9)              continue;
        if (maxLat - minLat < 1e-9)              continue;

        // ── Only now write to `out` ───────────────────────────────────────
        out.minLon = minLon;
        out.maxLon = maxLon;
        out.minLat = minLat;
        out.maxLat = maxLat;

        //qDebug() << "  ✓ World file:" << (base + "." + wext)
        // << "lon[" << minLon << "," << maxLon << "]"
        // << "lat[" << minLat << "," << maxLat << "]";
        return true;
    }
    return false;
}

/*
 * readGeoTiffExtents
 */
bool LayerPanel::readGeoTiffExtents(const QString& imagePath, RasterLayer& out)
{
    const QString ext = QFileInfo(imagePath).suffix().toLower();
    if (ext != "tif" && ext != "tiff")
        return false;

    QFile f(imagePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.read(4 * 1024 * 1024);  // 4 MB
    f.close();

    if (data.size() < 8) return false;

    // ── Byte order ────────────────────────────────────────────────────────
    bool le;
    if      (data[0]=='I' && data[1]=='I') le = true;
    else if (data[0]=='M' && data[1]=='M') le = false;
    else return false;

    auto u16 = [&](int o) -> quint16 {
        if (o+1 >= data.size()) return 0;
        quint8 a=quint8(data[o]), b=quint8(data[o+1]);
        return le ? quint16(quint16(b)<<8|a) : quint16(quint16(a)<<8|b);
    };
    auto u32 = [&](int o) -> quint32 {
        if (o+3 >= data.size()) return 0;
        quint8 a=quint8(data[o]),b=quint8(data[o+1]),
            c=quint8(data[o+2]),d=quint8(data[o+3]);
        return le ? (quint32(d)<<24|quint32(c)<<16|quint32(b)<<8|a)
                  : (quint32(a)<<24|quint32(b)<<16|quint32(c)<<8|d);
    };
    auto f64 = [&](int o) -> double {
        if (o+7 >= data.size()) return 0.0;
        quint8 buf[8];
        if (le) for(int i=0;i<8;i++) buf[i]=quint8(data[o+i]);
        else    for(int i=0;i<8;i++) buf[i]=quint8(data[o+7-i]);
        double v; memcpy(&v,buf,8); return v;
    };
    auto u16at = [&](int o) -> quint16 {  // alias for clarity
        return u16(o);
    };

    if (u16(2) != 42) return false;  // not classic TIFF

    quint32 ifdOff = u32(4);
    if (!ifdOff || ifdOff+2 >= (quint32)data.size()) return false;

    const int nTags = u16(ifdOff);
    const int tagBase = ifdOff + 2;

    // ── TIFF type sizes ───────────────────────────────────────────────────
    // We need: SHORT(3)=2bytes, LONG(4)=4bytes, DOUBLE(12)=8bytes

    // ── Tags we care about ────────────────────────────────────────────────
    constexpr quint16 TAG_PIXEL_SCALE = 33550;
    constexpr quint16 TAG_TIEPOINT   = 33922;
    constexpr quint16 TAG_GEOKEY_DIR = 34735;  // SHORT array: GeoKey directory
    constexpr quint16 TAG_GEODOUBLE  = 34736;  // DOUBLE array: GeoKey double values
    // 34737 = GeoAsciiParamsTag (strings, not needed here)

    double scaleX = 0, scaleY = 0;
    double tiePixI=0, tiePixJ=0, tieGeoX=0, tieGeoY=0;
    bool hasScale=false, hasTie=false;

    QVector<quint16> geoKeys;
    QVector<double>  geoDoubles;

    for (int i = 0; i < nTags; i++) {
        const int eb = tagBase + i*12;
        if (eb+11 >= data.size()) break;

        const quint16 tag   = u16(eb);
        const quint16 type  = u16(eb+2);
        const quint32 count = u32(eb+4);
        const quint32 voff  = u32(eb+8);  // value or offset to value

        // ── ModelPixelScaleTag (33550) — 3 doubles: ScaleX, ScaleY, ScaleZ ──
        if (tag == TAG_PIXEL_SCALE && type == 12 && count >= 2) {
            // Always at a file offset (3*8=24 bytes > 4)
            if (voff+15 < (quint32)data.size()) {
                double sx = f64(voff);
                double sy = f64(voff+8);
                if (sx > 0 && sy > 0) {
                    scaleX = sx;
                    scaleY = sy;
                    hasScale = true;
                }
            }
        }
        // ── ModelTiepointTag (33922) — N*6 doubles ──────────────────────────
        else if (tag == TAG_TIEPOINT && type == 12 && count >= 6) {
            if (voff+47 < (quint32)data.size()) {
                tiePixI = f64(voff);      // pixel I (col)
                tiePixJ = f64(voff+8);    // pixel J (row)
                // f64(voff+16) = K, skip
                tieGeoX = f64(voff+24);   // X: lon (geographic) or Easting (projected)
                tieGeoY = f64(voff+32);   // Y: lat (geographic) or Northing (projected)
                hasTie  = true;
            }
        }
        // ── GeoKeyDirectoryTag (34735) — SHORT array ─────────────────────────
        else if (tag == TAG_GEOKEY_DIR && type == 3) {
            const quint32 bytes = count * 2;
            if (bytes <= 4) {
                // Packed in the offset field
                for (quint32 k=0; k<count; k++)
                    geoKeys.append(u16at(eb+8 + k*2));
            } else if (voff + bytes <= (quint32)data.size()) {
                for (quint32 k=0; k<count; k++)
                    geoKeys.append(u16at(voff + k*2));
            }
        }
        // ── GeoDoubleParamsTag (34736) — DOUBLE array ────────────────────────
        else if (tag == TAG_GEODOUBLE && type == 12) {
            const quint32 bytes = count * 8;
            if (voff + bytes <= (quint32)data.size()) {
                for (quint32 k=0; k<count; k++)
                    geoDoubles.append(f64(voff + k*8));
            }
        }
    }

    if (!hasScale || !hasTie) {
        //qDebug() << "  ✗ GeoTIFF: missing scale or tiepoint tags in" << imagePath;
        return false;
    }

    const int imgW = out.pixmap.width();
    const int imgH = out.pixmap.height();
    if (imgW <= 0 || imgH <= 0) return false;

    // ── Compute bounding box corners in native CRS ────────────────────────
    // Tie-point maps pixel(tiePixI, tiePixJ) → native(tieGeoX, tieGeoY)
    // scaleX/Y are always positive in GeoTIFF (Y increases northward → subtract for rows)
    const double ulX = tieGeoX - tiePixI * scaleX;  // upper-left
    const double ulY = tieGeoY + tiePixJ * scaleY;
    const double lrX = ulX + imgW * scaleX;          // lower-right
    const double lrY = ulY - imgH * scaleY;

    //qDebug() << "  GeoTIFF native bbox: UL(" << ulX << "," << ulY
    // << ")  LR(" << lrX << "," << lrY << ")";
    //qDebug() << "  GeoTIFF scale: X=" << scaleX << " Y=" << scaleY;
    //qDebug() << "  Image size:" << imgW << "x" << imgH;

    // ── Parse GeoKey directory to determine CRS ───────────────────────────
    int modelType  = -1;
    int epsgCode   = -1;

    if (geoKeys.size() >= 4) {
        const int numKeys = geoKeys[3];
        for (int k = 0; k < numKeys; k++) {
            const int b = 4 + k*4;
            if (b+3 >= geoKeys.size()) break;

            const quint16 keyId  = geoKeys[b];
            const quint16 tagLoc = geoKeys[b+1];
            const quint16 kcount = geoKeys[b+2];
            const quint16 valOff = geoKeys[b+3];

            // For inline SHORT values: tagLoc==0, Count==1, Value is valOff itself
            if (tagLoc == 0 && kcount == 1) {
                if (keyId == 1024) modelType = valOff;  // GTModelTypeGeoKey
                if (keyId == 2048) epsgCode  = valOff;  // GeographicTypeGeoKey
                if (keyId == 3072) epsgCode  = valOff;  // ProjectedCSTypeGeoKey
            }
            // For values stored in GeoDoubleParams: tagLoc==34736
            // (not needed for CRS identification — EPSG codes are always short ints)
        }
    }
    bool needsReproject = false;
    if (modelType == 1) {
        needsReproject = true;
    } else if (modelType == 2) {
        needsReproject = false;
    } else {
        // No GeoKeys — auto-detect by coordinate range
        const double maxAbsX = qMax(std::abs(ulX), std::abs(lrX));
        const double maxAbsY = qMax(std::abs(ulY), std::abs(lrY));
        needsReproject = (maxAbsX > 180.0 || maxAbsY > 90.0);
        //qDebug() << "  GeoTIFF: no GeoKey modelType — auto-detect needsReproject="
        // << needsReproject;
    }

    // ── If EPSG not found, guess from coordinate ranges ───────────────────
    if (needsReproject && epsgCode <= 0) {
        //qDebug() << "  ✗ GeoTIFF: projected CRS but EPSG code unknown — cannot reproject";
        return false;
    }

    double minLon, maxLon, minLat, maxLat;

    if (!needsReproject) {
        // ── Geographic CRS — values are already degrees ───────────────────
        minLon = qMin(ulX, lrX);
        maxLon = qMax(ulX, lrX);
        minLat = qMin(ulY, lrY);
        maxLat = qMax(ulY, lrY);

    } else {
        // ── Projected CRS — reproject all 4 corners to WGS84 ─────────────
        const QString srcEpsgStr = QString("EPSG:%1").arg(epsgCode);
        QgsCoordinateReferenceSystem srcCrs(srcEpsgStr);
        QgsCoordinateReferenceSystem wgs84("EPSG:4326");

        if (!srcCrs.isValid()) {
            //qDebug() << "  ✗ GeoTIFF: cannot create CRS for" << srcEpsgStr;
            return false;
        }

        QgsCoordinateTransform xform(srcCrs, wgs84, QgsProject::instance());

        // All 4 corners — handles rotated/skewed projections correctly
        const double corners[4][2] = {
            { ulX, ulY },   // upper-left
            { lrX, ulY },   // upper-right
            { ulX, lrY },   // lower-left
            { lrX, lrY },   // lower-right
        };

        minLon =  1e18; maxLon = -1e18;
        minLat =  1e18; maxLat = -1e18;

        for (const auto& c : corners) {
            try {
                QgsPointXY geo = xform.transform(QgsPointXY(c[0], c[1]));
                minLon = qMin(minLon, geo.x());
                maxLon = qMax(maxLon, geo.x());
                minLat = qMin(minLat, geo.y());
                maxLat = qMax(maxLat, geo.y());
                //qDebug() << "    Native(" << c[0] << "," << c[1]
                // << ") → WGS84(" << geo.x() << "," << geo.y() << ")";
            } catch (QgsCsException& e) {
                //qDebug() << "  ✗ GeoTIFF reprojection failed:" << e.what();
                return false;
            }
        }

        // //qDebug() << "  ✓ Reprojected" << srcEpsgStr << "→ EPSG:4326";
    }

    // ── Final sanity check ────────────────────────────────────────────────
    if (minLon < -180.1 || maxLon > 180.1) {
        //qDebug() << "  ✗ GeoTIFF lon out of range:" << minLon << maxLon;
        return false;
    }
    if (minLat < -90.1 || maxLat > 90.1) {
        //qDebug() << "  ✗ GeoTIFF lat out of range:" << minLat << maxLat;
        return false;
    }
    if (maxLon - minLon < 1e-9 || maxLat - minLat < 1e-9) {
        //qDebug() << "  ✗ GeoTIFF zero-size extent";
        return false;
    }

    // ── Write to out only after all validation passes ─────────────────────
    out.minLon = minLon;
    out.maxLon = maxLon;
    out.minLat = minLat;
    out.maxLat = maxLat;

    // //qDebug() << "  ✓ GeoTIFF final extents: lon[" << minLon << "," << maxLon
    //          << "]  lat[" << minLat << "," << maxLat << "]";
    return true;
}

/* =========================================================================
   HELPER — compute a sensible default bounding box when no georef exists.
   ========================================================================= */
// CHAITANYA JAIN CHANGE:
// Why: Decoupled makeDefaultExtents definition from CanvasWidget by changing parameter to ILayerHost*.
// static RasterLayer makeDefaultExtents(const RasterLayer& rl, CanvasWidget* canvas)
static RasterLayer makeDefaultExtents(const RasterLayer& rl, ILayerHost* host)
{
    RasterLayer out = rl;

    const int imgW = rl.pixmap.width();
    const int imgH = rl.pixmap.height();

    double centreLon = 0.0;
    double centreLat = 20.0;  // sensible world-view default
    double degPerPixLon = 0.1;
    double degPerPixLat = 0.1;

    if (host) {
        ViewportInfo info = host->getViewportInfo();
        centreLon = info.centerGeo.x();
        centreLat = info.centerGeo.y();
        degPerPixLon = info.degPerPixLon;
        degPerPixLat = info.degPerPixLat;
    }

    // Image geographic span = pixel count × degrees-per-pixel
    const double spanLon = imgW * degPerPixLon;
    const double spanLat = imgH * degPerPixLat;

    out.minLon = centreLon - spanLon / 2.0;
    out.maxLon = centreLon + spanLon / 2.0;
    out.minLat = centreLat - spanLat / 2.0;
    out.maxLat = centreLat + spanLat / 2.0;

    // Clamp to valid geo range
    out.minLon = std::max(out.minLon, -180.0);
    out.maxLon = std::min(out.maxLon,  180.0);
    out.minLat = std::max(out.minLat, -85.0511);
    out.maxLat = std::min(out.maxLat,  85.0511);
    return out;
}
QString LayerPanel::generateFriendlyShapeName(const QString& shapeId, const QString& shapeType) const
{
    // CHAITANYA JAIN CHANGE:
    // Why: Fixed Shape Name Numbering issue so that drawn/pasted shapes are named sequentially (e.g. "Circle 1", "Polygon 2") instead of exposing internal Temp IDs.
    static QMap<QString, int> typeCounters;
    QString baseType = shapeType.isEmpty() ? "Shape" : shapeType;
    int count = ++typeCounters[baseType];
    return QString("%1 %2").arg(baseType).arg(count);
}
void LayerPanel::onShapeItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    if (m_suppressCenter) return;

    // Check if this is a shape item (has shapeId stored)
    QString shapeId = item->data(0, Qt::UserRole).toString();
    if (!shapeId.isEmpty() && shapeToLayer.contains(shapeId))
    {
        emit shapeClicked(shapeId);
    }
}
void LayerPanel::updateRasterLayerFromShape(const QString& shapeId)
{
    if (!shapeToRasterLayer.contains(shapeId)) return;
    QString layerName = shapeToRasterLayer[shapeId];
    if (!rasterLayers.contains(layerName)) return;
    // CHAITANYA JAIN CHANGE:
    // Why: Checks validity of m_layerHost and retrieves modified raster shape properties from ILayerHost interface.
    // if (!m_canvasWidget) return;
    if (!m_layerHost) return;

    std::vector<MeshEntry> matches = m_layerHost->getMeshEntriesForShapes(QStringList() << shapeId);
    if (matches.empty()) return;

    const MeshEntry* entry = &matches[0];
    if (!entry || !entry->position || !entry->size) return;

    double cx = entry->position->x();
    double cy = entry->position->y();
    double hw = entry->size->x();
    double hh = entry->size->y();
    rasterLayers[layerName].minLon = cx - hw;
    rasterLayers[layerName].maxLon = cx + hw;
    rasterLayers[layerName].minLat = cy - hh;
    rasterLayers[layerName].maxLat = cy + hh;
    emit rasterLayerChanged();
}

void LayerPanel::removeRasterLayer(const QString& layerName)
{
    if (!rasterLayers.contains(layerName)) return;

    // Find shape ID for this layer (for cleaning shape maps)
    QString shapeId;
    for (auto it = shapeToRasterLayer.constBegin(); it != shapeToRasterLayer.constEnd(); ++it) {
        if (it.value() == layerName) {
            shapeId = it.key();
            break;
        }
    }

    // CHAITANYA JAIN CHANGE:
    // Why: Decoupled LayerPanel from CanvasWidget and fixed complete raster-layer deletion lifecycle using ILayerHost.
    if (m_layerHost && !shapeId.isEmpty()) {
        // Remove from shapeToRasterLayer first to prevent infinite recursion
        // when deleteObjectById calls removeShapeFromLayer.
        shapeToRasterLayer.remove(shapeId);
        m_layerHost->deleteObjectFromCanvas(shapeId);
    }

    // Remove from shape maps
    if (!shapeId.isEmpty()) {
        shapeToLayer.remove(shapeId);
        shapeToRasterLayer.remove(shapeId);
        shapeDisplayNames.remove(shapeId);
    }

    // Remove from raster-specific maps
    rasterLayers.remove(layerName);
    if (rasterLayerItems.contains(layerName)) {
        delete rasterLayerItems.take(layerName);
    }

    rasterLayerOrder.removeAll(layerName);
    rasterNameLabels.remove(layerName);
    rasterExpandButtons.remove(layerName);
    if (rasterVisibilityWidgets.contains(layerName)) {
        delete rasterVisibilityWidgets.take(layerName);
    }
    // Remove from general visibility map
    layerVisibility.remove(layerName);
    // Remove the tree item (direct child of rootLayersItem)
    if (rootLayersItem) {
        for (int i = 0; i < rootLayersItem->childCount(); ++i) {
            QTreeWidgetItem* child = rootLayersItem->child(i);
            if (child->data(0, Qt::UserRole).toString() == layerName) {
                delete child;
                break;
            }
        }
    }

    emit rasterLayerChanged();
    emit layerRemoved(layerName);
}
void LayerPanel::selectRasterInPanel(const QString& shapeId)
{
    QString layerName = shapeToRasterLayer.value(shapeId);
    if (layerName.isEmpty()) return;
    QTreeWidgetItem* item = rasterLayerItems.value(layerName, nullptr);
    if (!item) return;
    m_suppressCenter = true;
    // Block ALL signals from layerTree, not just selection changed
    layerTree->blockSignals(true);
    layerTree->setCurrentItem(item);
    layerTree->scrollToItem(item, QAbstractItemView::EnsureVisible);
    layerTree->blockSignals(false);

    m_suppressCenter = false;

    layerTree->viewport()->update();
}