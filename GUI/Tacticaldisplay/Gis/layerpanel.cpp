/* ========================================================================= */
/* File: layerpanel.cpp                                                     */
/* Written by: Waris Ali                                                   */
/* ========================================================================= */

#include "layerpanel.h"
#include "layerpanel-styles.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
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
#include "GUI/Tacticaldisplay/Gis/gislib.h"
// #include "tests/layerpaneltest/layerpanel_test.h"
// #include "GUI/mainwindow.h"
// #include <QTimer>
// #include "tests/gui_test_control.h"
static RasterLayer makeDefaultExtents(const RasterLayer& rl, CanvasWidget* canvas);



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
    // runUnitTestsOnce();

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
    layerTree->setColumnCount(3);
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

    // Column sizing: col0 stretches, col1 and col2 are fixed 28px
    layerTree->setColumnCount(2);
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
    connect(removeLayerAction, &QAction::triggered, this, &LayerPanel::removeLayer);
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
        // For shape items - show rename option
        else if (selectedItem->parent() &&
                 selectedItem->parent()->text(0) != "Layers") {
            // Shape item - show rename action
            QString shapeId = selectedItem->data(0, Qt::UserRole).toString();
            QAction* renameShapeAction = new QAction("Rename Shape", this);
            connect(renameShapeAction, &QAction::triggered, this, [this, shapeId]() {
                renameShape(shapeId);
            });
            contextMenu->addAction(renameShapeAction);
            showMenu = true;
        }
    } else {
        // Empty area click - no menu
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
    // Get layer name from user
    bool ok;
    QString layerName = QInputDialog::getText(this,
                                              "Add Layer",
                                              "Layer Name:",
                                              QLineEdit::Normal,
                                              "New Layer",
                                              &ok);

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

// %%% Remove Layer %%%
/* Remove selected layer — works for both vector and raster layers */
void LayerPanel::removeLayer()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();

    if (!selectedItem) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Selection");
        msgBox.setText("Please select a layer to remove!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    // Resolve name from UserRole (works for both vector and raster)
    QString layerName = selectedItem->data(0, Qt::UserRole).toString();
    if (layerName.isEmpty())
        layerName = getFullLayerName(selectedItem);

    // Determine type
    const bool isRaster = rasterLayers.contains(layerName);
    const bool isVector = layerItems.contains(layerName);

    if (!isRaster && !isVector) {
        // Not a layer row (e.g. shape child or root) — nothing to do
        return;
    }

    // Build confirmation message
    int shapeCount = isVector ? layerShapes.value(layerName).count() : 0;
    QString message = QString("Are you sure you want to remove layer '%1'?").arg(layerName);
    if (shapeCount > 0)
        message += QString("\n\nThis will also permanently delete %1 shape(s).").arg(shapeCount);
    if (isRaster)
        message += "\n\nThe raster image file will NOT be deleted from disk.";

    QMessageBox msgBox(this);
    msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
    msgBox.setWindowTitle("Remove Layer");
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox.exec() != QMessageBox::Yes)
        return;

    // ── Remove from tree ──────────────────────────────────────────────────
    QTreeWidgetItem* parent = selectedItem->parent();
    if (parent)
        parent->removeChild(selectedItem);
    else
        layerTree->takeTopLevelItem(layerTree->indexOfTopLevelItem(selectedItem));
    delete selectedItem;

    if (isVector) {
        // ── Vector layer cleanup ──────────────────────────────────────────
        QStringList shapes = layerShapes.value(layerName);

        // Notify canvas to erase shapes from tempMeshes BEFORE clearing maps
        emit layerWithShapesRemoved(shapes);

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
        // ── Raster layer cleanup ──────────────────────────────────────────
        rasterLayers.remove(layerName);
        rasterLayerItems.remove(layerName);
        rasterLayerOrder.removeAll(layerName);
        rasterNameLabels.remove(layerName);
        rasterExpandButtons.remove(layerName);

        if (rasterVisibilityWidgets.contains(layerName)) {
            delete rasterVisibilityWidgets.take(layerName);
        }

        // Trigger canvas repaint so the raster is no longer drawn
        emit rasterLayerChanged();
        emit layerRemoved(layerName);
    }
}

// %%% Rename Layer %%%
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

        // Re-key all raster maps
        if (rasterLayerItems.contains(oldName))
            rasterLayerItems[newName] = rasterLayerItems.take(oldName);
        if (rasterVisibilityWidgets.contains(oldName))
            rasterVisibilityWidgets[newName] = rasterVisibilityWidgets.take(oldName);
        if (rasterExpandButtons.contains(oldName))
            rasterExpandButtons[newName] = rasterExpandButtons.take(oldName);

        // Re-key name label map + update displayed text
        if (rasterNameLabels.contains(oldName)) {
            QLabel* lbl = rasterNameLabels.take(oldName);
            lbl->setText(newName);
            // Update the property used by eventFilter for future double-clicks
            lbl->setProperty("rasterLayerName", newName);
            rasterNameLabels[newName] = lbl;
        }

        // Update layerOrder list
        int idx = rasterLayerOrder.indexOf(oldName);
        if (idx >= 0) rasterLayerOrder[idx] = newName;

        // Update tree item UserRole
        if (rasterLayerItems.contains(newName))
            rasterLayerItems[newName]->setData(0, Qt::UserRole, newName);

        // Update the col0Widget property used by the eventFilter
        if (rasterLayerItems.contains(newName)) {
            QWidget* w = layerTree->itemWidget(rasterLayerItems[newName], 0);
            if (w) w->setProperty("rasterLayerName", newName);
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

    qDebug() << "Layer renamed:" << oldName << "→" << newName;
}


// %%% Layer Selection Changed %%%
/* Handle layer selection change */
void LayerPanel::onLayerSelectionChanged()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();

    if (!selectedItem) {
        return;
    }

    // Only set active layer if it's a layer item (not a shape)
    QString text = selectedItem->text(0);
    if (text != "Layers" && selectedItem->parent() && selectedItem->parent()->text(0) == "Layers") {
        QString layerName = getFullLayerName(selectedItem);
        setActiveLayer(layerName);
    }
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

    if (targetLayer.isEmpty() || !layerExists(targetLayer)) {
        // No active layer - show message to user
        QMessageBox msgBox(nullptr);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Active Layer");
        msgBox.setText("Please create and select a layer before drawing shapes!\n\n"
                       "Right-click in the Layers Panel and select 'Add Layer'.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    // Add to layer's shape list
    if (!layerShapes[targetLayer].contains(shapeId)) {
        layerShapes[targetLayer].append(shapeId);
    }

    // Add reverse lookup
    shapeToLayer[shapeId] = targetLayer;

    // Add visual representation in tree
    addShapeItemToTree(targetLayer, shapeId, shapeType);

    // Update shape count
    updateLayerShapeCount(targetLayer);
}

/* Remove shape from layer */
void LayerPanel::removeShapeFromLayer(const QString& shapeId)
{
    QString layerName = shapeToLayer.value(shapeId);

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

    qDebug() << "Shape" << shapeId << "moved from layer"
             << sourceLayerName << "→" << targetLayerName;
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
/* Highlight the shape row in the tree that matches shapeId */
void LayerPanel::selectShapeInPanel(const QString& shapeId)
{
    if (shapeId.isEmpty()) return;

    for (QTreeWidgetItem* layerItem : layerItems.values()) {
        for (int i = 0; i < layerItem->childCount(); ++i) {
            QTreeWidgetItem* child = layerItem->child(i);
            if (child->data(0, Qt::UserRole).toString() == shapeId) {
                // Make sure the parent layer is expanded
                layerItem->setExpanded(true);
                // Select and scroll to the item
                layerTree->setCurrentItem(child);
                layerTree->scrollToItem(child, QAbstractItemView::EnsureVisible);
                return;
            }
        }
    }
}

// %%% Layer Queries %%%
/* Check if layer exists */
bool LayerPanel::layerExists(const QString& layerName) const
{
    return layerItems.contains(layerName);
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
    if (!layerItem) {
        return;
    }

    QString shapeSuffix;
    int underscoreIdx = shapeId.lastIndexOf('_');
    if (underscoreIdx >= 0)
        shapeSuffix = shapeId.mid(underscoreIdx);   // includes the "_", e.g. "_1"

    QString displayName = shapeDisplayNames.contains(shapeId)
                              ? shapeDisplayNames[shapeId]
                              : QString("%1%2").arg(shapeType, shapeSuffix);

    // Create the tree item (no visible text — widget handles display)
    QTreeWidgetItem *shapeItem = new QTreeWidgetItem(layerItem);
    shapeItem->setData(0, Qt::UserRole, shapeId);
    shapeItem->setFlags(shapeItem->flags() & ~Qt::ItemIsEditable);
    shapeItem->setText(0, "");   // widget takes over

    // Build a widget with explicit left-padding so the row looks indented
    // relative to the layer header above it.
    QWidget* shapeWidget = new QWidget();
    shapeWidget->setStyleSheet("background: transparent;");
    QHBoxLayout* shapeLayout = new QHBoxLayout(shapeWidget);
    // 28 px left margin creates the visible indent beyond the tree indentation
    shapeLayout->setContentsMargins(8, 0, 2, 0);
    shapeLayout->setSpacing(4);

    // Small neutral dash indent marker
    QLabel* dotLabel = new QLabel("–");
    dotLabel->setFixedWidth(12);
    dotLabel->setStyleSheet("color: #7A8FA6; background: transparent; font-size: 10px;");

    // Shape name label — plain white, no type-based colour
    QLabel* nameLabel = new QLabel(displayName);
    nameLabel->setStyleSheet("color: #C8D6E0; background: transparent; font-size: 12px;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // Store shapeId so rename can update this label later
    nameLabel->setProperty("shapeId", shapeId);

    shapeLayout->addWidget(dotLabel);
    shapeLayout->addWidget(nameLabel);
    shapeLayout->addStretch();

    layerTree->setItemWidget(shapeItem, 0, shapeWidget);

    // Expand layer to show new shape
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
        "  border-radius: 3px;"
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

    // Check if canvas widget is available
    if (!m_canvasWidget) {
        QMessageBox::critical(this, "Export Error",
                              "Canvas widget not available for export!");
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
    bool success = false;

    if (selectedFormat.contains("GeoJSON")) {
        success = exportToGeoJSON(layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("KML")) {
        success = exportToKML(layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("GML")) {
        success = exportToGML(layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("Shapefile")) {
        success = exportToShapefile(layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("FlatGeobuf")) {
        success = exportToFlatGeobuf(layerName, shapesInLayer, filePath);
    } else if (selectedFormat.contains("CSV")) {
        success = exportToCSV(layerName, shapesInLayer, filePath);
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

/* Export layer to GeoJSON format */
bool LayerPanel::exportToGeoJSON(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    QJsonObject rootObject;
    rootObject["type"] = "FeatureCollection";

    QJsonObject crs;
    QJsonObject crsProperties;
    crsProperties["name"] = "EPSG:4326";  // WGS84
    crs["type"] = "name";
    crs["properties"] = crsProperties;
    rootObject["crs"] = crs;

    QJsonArray features;

    for (const auto& entry : m_canvasWidget->tempMeshes) {
        QString entryName = entry.name;

        // Check if this shape is in our shapeIds list
        bool belongsToLayer = false;
        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName) {
                belongsToLayer = true;
                break;
            }
        }

        if (!belongsToLayer) {
            continue;
        }

        // Create GeoJSON feature
        QJsonObject feature = createGeoJSONFeature(entry, entryName);

        if (!feature.isEmpty()) {
            features.append(feature);
        }
    }

    rootObject["features"] = features;

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        // qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(rootObject);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

/* Export layer to KML format */
bool LayerPanel::exportToKML(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // KML Header
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    out << "  <Document>\n";
    out << "    <name>" << layerName << "</name>\n";  // FIXED: was <n>
    out << "    <description>Exported from Tactical Display</description>\n";

    // ========== DEBUG CODE START ==========
    qDebug() << "==========================================";
    qDebug() << "KML EXPORT DEBUG";
    qDebug() << "==========================================";
    qDebug() << "Layer Name:" << layerName;
    qDebug() << "Shapes to export (from layer panel):" << shapeIds;
    qDebug() << "Number of shapes:" << shapeIds.size();

    if (!m_canvasWidget) {
        qDebug() << "ERROR: m_canvasWidget is NULL!";
        out << "  </Document>\n</kml>\n";
        file.close();
        return true;
    }

    qDebug() << "Canvas widget exists: YES";
    qDebug() << "Total entries in Meshes:" << m_canvasWidget->Meshes.size();
    qDebug() << "Total entries in tempMeshes:" << m_canvasWidget->tempMeshes.size();

    // List all mesh keys
    qDebug() << "\n--- ALL MESHES IN CANVAS ---";
    for (const auto& pair : m_canvasWidget->Meshes) {
        QString meshKey = QString::fromStdString(pair.first);
        QString entryName = pair.second.name;
        qDebug() << "  Key:" << meshKey << "| Name:" << entryName;
    }

    // List all tempMeshes
    qDebug() << "\n--- ALL TEMP MESHES IN CANVAS ---";
    for (size_t i = 0; i < m_canvasWidget->tempMeshes.size(); i++) {
        QString entryName = m_canvasWidget->tempMeshes[i].name;
        qDebug() << "  [" << i << "] Name:" << entryName;
    }

    qDebug() << "\n--- CHECKING MATCHES IN MESHES ---";
    // ========== DEBUG CODE END ==========

    int exportedCount = 0;

    // Add placemarks for each shape - CHECK MESHES FIRST
    for (auto& pair : m_canvasWidget->Meshes) {
        MeshEntry& entry = pair.second;
        QString entryName = entry.name;
        QString meshKey = QString::fromStdString(pair.first);

        qDebug() << "Checking Mesh:" << meshKey << "(" << entryName << ")";

        // Check if this shape is in our shapeIds list
        bool belongsToLayer = false;
        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName || shapeId == meshKey) {
                belongsToLayer = true;
                qDebug() << "  ✓ MATCH FOUND with:" << shapeId;
                break;
            }
        }

        if (!belongsToLayer) {
            qDebug() << "  ✗ Not in layer - skipping";
            continue;
        }

        qDebug() << "  → Creating placemark...";
        QString kmlPlacemark = createKMLPlacemark(entry, entryName);

        if (!kmlPlacemark.isEmpty()) {
            out << kmlPlacemark;
            exportedCount++;
            qDebug() << "  ✓ Exported successfully";
        } else {
            qDebug() << "  ✗ Placemark generation returned empty!";
        }
    }

    // ========== ALSO CHECK TEMP MESHES ==========
    qDebug() << "\n--- CHECKING MATCHES IN TEMP MESHES ---";

    for (const MeshEntry& entry : m_canvasWidget->tempMeshes) {
        QString entryName = entry.name;

        qDebug() << "Checking tempMesh:" << entryName;

        // Check if this shape is in our shapeIds list
        bool belongsToLayer = false;
        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName) {
                belongsToLayer = true;
                qDebug() << "  ✓ MATCH FOUND with:" << shapeId;
                break;
            }
        }

        if (!belongsToLayer) {
            qDebug() << "  ✗ Not in layer - skipping";
            continue;
        }

        qDebug() << "  → Creating placemark...";
        QString kmlPlacemark = createKMLPlacemark(entry, entryName);

        if (!kmlPlacemark.isEmpty()) {
            out << kmlPlacemark;
            exportedCount++;
            qDebug() << "  ✓ Exported successfully from tempMeshes";
        } else {
            qDebug() << "  ✗ Placemark generation returned empty!";
        }
    }
    // ========== END TEMP MESHES CHECK ==========

    qDebug() << "\n--- EXPORT SUMMARY ---";
    qDebug() << "Total shapes exported:" << exportedCount;
    qDebug() << "==========================================\n";

    // KML Footer
    out << "  </Document>\n";
    out << "</kml>\n";

    file.close();
    return true;
}

/* Export layer to GML format - COMPLETE FIXED VERSION WITH DEBUG */
bool LayerPanel::exportToGML(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // GML Header
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<gml:FeatureCollection\n";
    out << "  xmlns:gml=\"http://www.opengis.net/gml\"\n";
    out << "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
    out << "  <gml:name>" << layerName << "</gml:name>\n";

    // ========== DEBUG CODE START ==========
    qDebug() << "==========================================";
    qDebug() << "GML EXPORT DEBUG";
    qDebug() << "==========================================";
    qDebug() << "Layer Name:" << layerName;
    qDebug() << "Shapes to export:" << shapeIds;
    qDebug() << "Number of shapes:" << shapeIds.size();

    if (!m_canvasWidget) {
        qDebug() << "ERROR: m_canvasWidget is NULL!";
        out << "</gml:FeatureCollection>\n";
        file.close();
        return true;
    }

    qDebug() << "Canvas widget exists: YES";
    qDebug() << "Total entries in Meshes:" << m_canvasWidget->Meshes.size();
    qDebug() << "Total entries in tempMeshes:" << m_canvasWidget->tempMeshes.size();
    qDebug() << "\n--- CHECKING MATCHES IN MESHES ---";
    // ========== DEBUG CODE END ==========

    int exportedCount = 0;

    // Add features for each shape - CHECK MESHES FIRST
    for (auto& pair : m_canvasWidget->Meshes) {
        MeshEntry& entry = pair.second;
        QString entryName = entry.name;
        QString meshKey = QString::fromStdString(pair.first);

        qDebug() << "Checking Mesh:" << meshKey << "(" << entryName << ")";

        // Check if this shape is in our shapeIds list
        bool belongsToLayer = false;
        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName || shapeId == meshKey) {
                belongsToLayer = true;
                qDebug() << "  ✓ MATCH FOUND with:" << shapeId;
                break;
            }
        }

        if (!belongsToLayer) {
            qDebug() << "  ✗ Not in layer - skipping";
            continue;
        }

        qDebug() << "  → Creating GML feature...";
        QString gmlFeature = createGMLFeature(entry, entryName);

        if (!gmlFeature.isEmpty()) {
            out << gmlFeature;
            exportedCount++;
            qDebug() << "  ✓ Exported successfully";
        } else {
            qDebug() << "  ✗ Feature generation returned empty!";
        }
    }

    // ========== ALSO CHECK TEMP MESHES ==========
    qDebug() << "\n--- CHECKING MATCHES IN TEMP MESHES ---";

    for (const MeshEntry& entry : m_canvasWidget->tempMeshes) {
        QString entryName = entry.name;

        qDebug() << "Checking tempMesh:" << entryName;

        // Check if this shape is in our shapeIds list
        bool belongsToLayer = false;
        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName) {
                belongsToLayer = true;
                qDebug() << "  ✓ MATCH FOUND with:" << shapeId;
                break;
            }
        }

        if (!belongsToLayer) {
            qDebug() << "  ✗ Not in layer - skipping";
            continue;
        }

        qDebug() << "  → Creating GML feature...";
        QString gmlFeature = createGMLFeature(entry, entryName);

        if (!gmlFeature.isEmpty()) {
            out << gmlFeature;
            exportedCount++;
            qDebug() << "  ✓ Exported successfully from tempMeshes";
        } else {
            qDebug() << "  ✗ Feature generation returned empty!";
        }
    }
    // ========== END TEMP MESHES CHECK ==========

    qDebug() << "\n--- EXPORT SUMMARY ---";
    qDebug() << "Total shapes exported:" << exportedCount;
    qDebug() << "==========================================\n";

    // GML Footer
    out << "</gml:FeatureCollection>\n";

    file.close();
    return true;
}

/* Export layer to Shapefile format */
bool LayerPanel::exportToShapefile(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    qDebug() << "==========================================";
    qDebug() << "SHAPEFILE EXPORT DEBUG";
    qDebug() << "==========================================";
    qDebug() << "Layer Name:" << layerName;
    qDebug() << "Shapes to export:" << shapeIds;

    if (!m_canvasWidget) {
        qDebug() << "ERROR: m_canvasWidget is NULL!";
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    // Separate shapes by geometry type
    QVector<const MeshEntry*> polygonShapes;
    QVector<const MeshEntry*> lineShapes;
    QVector<const MeshEntry*> pointShapes;

    // Check Meshes
    for (const auto& pair : m_canvasWidget->Meshes) {
        const MeshEntry& entry = pair.second;
        QString entryName = entry.name;
        QString meshKey = QString::fromStdString(pair.first);

        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName || shapeId == meshKey) {
                // Classify by geometry type
                if (entryName.contains("Polyline", Qt::CaseInsensitive) ||
                    entryName.contains("Line", Qt::CaseInsensitive)) {
                    lineShapes.append(&entry);
                    qDebug() << "  Found LINE in Meshes:" << entryName;
                } else if (entryName.contains("Point", Qt::CaseInsensitive)) {
                    pointShapes.append(&entry);
                    qDebug() << "  Found POINT in Meshes:" << entryName;
                } else {
                    // Everything else is polygon (Circle, Rectangle, Polygon)
                    polygonShapes.append(&entry);
                    qDebug() << "  Found POLYGON in Meshes:" << entryName;
                }
                break;
            }
        }
    }

    // Check tempMeshes
    for (const MeshEntry& entry : m_canvasWidget->tempMeshes) {
        QString entryName = entry.name;

        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName) {
                // Classify by geometry type
                if (entryName.contains("Polyline", Qt::CaseInsensitive) ||
                    entryName.contains("Line", Qt::CaseInsensitive)) {
                    lineShapes.append(&entry);
                    qDebug() << "  Found LINE in tempMeshes:" << entryName;
                } else if (entryName.contains("Point", Qt::CaseInsensitive)) {
                    pointShapes.append(&entry);
                    qDebug() << "  Found POINT in tempMeshes:" << entryName;
                } else {
                    // Everything else is polygon (Circle, Rectangle, Polygon)
                    polygonShapes.append(&entry);
                    qDebug() << "  Found POLYGON in tempMeshes:" << entryName;
                }
                break;
            }
        }
    }

    qDebug() << "\nGeometry type summary:";
    qDebug() << "  Polygons:" << polygonShapes.size();
    qDebug() << "  Lines:" << lineShapes.size();
    qDebug() << "  Points:" << pointShapes.size();

    if (polygonShapes.isEmpty() && lineShapes.isEmpty() && pointShapes.isEmpty()) {
        qDebug() << "ERROR: No shapes to export!";
        return false;
    }

    bool success = true;
    int filesCreated = 0;

    // Export polygons (circles, rectangles, polygons)
    if (!polygonShapes.isEmpty()) {
        QString polyPath = dirPath + "/" + baseName + "_polygons.shp";
        qDebug() << "\n--- Exporting POLYGONS to:" << polyPath;
        if (exportShapesByType(polygonShapes, polyPath, layerName + " (Polygons)", 5)) {
            filesCreated++;
            qDebug() << "✓ Polygon shapefile created successfully";
        } else {
            qDebug() << "✗ Failed to create polygon shapefile";
            success = false;
        }
    }

    // Export lines
    if (!lineShapes.isEmpty()) {
        QString linePath = dirPath + "/" + baseName + "_lines.shp";
        qDebug() << "\n--- Exporting LINES to:" << linePath;
        if (exportShapesByType(lineShapes, linePath, layerName + " (Lines)", 3)) {
            filesCreated++;
            qDebug() << "✓ Line shapefile created successfully";
        } else {
            qDebug() << "✗ Failed to create line shapefile";
            success = false;
        }
    }

    // Export points
    if (!pointShapes.isEmpty()) {
        QString pointPath = dirPath + "/" + baseName + "_points.shp";
        qDebug() << "\n--- Exporting POINTS to:" << pointPath;
        if (exportShapesByType(pointShapes, pointPath, layerName + " (Points)", 1)) {
            filesCreated++;
            qDebug() << "✓ Point shapefile created successfully";
        } else {
            qDebug() << "✗ Failed to create point shapefile";
            success = false;
        }
    }

    qDebug() << "==========================================";
    qDebug() << "SHAPEFILE EXPORT COMPLETE!";
    qDebug() << "Total shapefile sets created:" << filesCreated;
    qDebug() << "==========================================\n";

    return success;
}
bool LayerPanel::exportShapesByType(const QVector<const MeshEntry*>& shapes,
                                    const QString& filePath,
                                    const QString& layerName,
                                    int shapeType)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    QString shpPath = dirPath + "/" + baseName + ".shp";
    QString shxPath = dirPath + "/" + baseName + ".shx";
    QString dbfPath = dirPath + "/" + baseName + ".dbf";
    QString prjPath = dirPath + "/" + baseName + ".prj";

    try {
        QFile shpFile(shpPath);
        if (!shpFile.open(QIODevice::WriteOnly)) {
            qDebug() << "  ERROR: Cannot create .shp file:" << shpPath;
            return false;
        }
        QDataStream shpStream(&shpFile);
        shpStream.setByteOrder(QDataStream::BigEndian);

        QFile shxFile(shxPath);
        if (!shxFile.open(QIODevice::WriteOnly)) {
            qDebug() << "  ERROR: Cannot create .shx file!";
            shpFile.close();
            return false;
        }
        QDataStream shxStream(&shxFile);
        shxStream.setByteOrder(QDataStream::BigEndian);

        // Calculate bounding box
        double minX = 180.0, minY = 90.0, maxX = -180.0, maxY = -90.0;

        QVector<QByteArray> shapeRecords;
        QVector<qint32> recordOffsets;
        qint32 currentOffset = 50;  // Header = 100 bytes = 50 words

        for (int i = 0; i < shapes.size(); i++) {
            const MeshEntry* entry = shapes[i];

            if (!entry->position || !entry->rotation || !entry->mesh) {
                qDebug() << "    Skipping" << entry->name << "- missing data";
                continue;
            }

            QByteArray record = createShapefileRecord(entry, shapeType, minX, minY, maxX, maxY);

            if (!record.isEmpty()) {
                recordOffsets.append(currentOffset);
                shapeRecords.append(record);

                // FIXED: Account for 8-byte record header (4 words) + content
                currentOffset += 4 + (record.size() / 2);

                qDebug() << "    Created record for:" << entry->name << "(" << record.size() << "bytes)";
            }
        }

        // File length in words
        qint32 shpFileLength = currentOffset;
        qint32 shxFileLength = 50 + (shapeRecords.size() * 4);

        // Write headers
        writeShapefileHeader(shpStream, shpFileLength, shapeType, minX, minY, maxX, maxY);
        writeShapefileHeader(shxStream, shxFileLength, shapeType, minX, minY, maxX, maxY);

        // Write shape records WITH HEADERS
        for (int i = 0; i < shapeRecords.size(); i++) {
            // CRITICAL: Write record header (8 bytes, big endian)
            shpStream << (qint32)(i + 1);  // Record number (starts at 1)
            shpStream << (qint32)(shapeRecords[i].size() / 2);  // Content length in WORDS

            // Write geometry data
            shpStream.writeRawData(shapeRecords[i].constData(), shapeRecords[i].size());
        }

        // Write index records
        for (int i = 0; i < recordOffsets.size(); i++) {
            shxStream << recordOffsets[i];
            shxStream << (qint32)(shapeRecords[i].size() / 2);
        }

        shpFile.close();
        shxFile.close();

        // Write DBF
        if (!writeDBF(dbfPath, shapes)) {
            qDebug() << "    WARNING: Failed to create .dbf file";
        }

        // Write PRJ
        QFile prjFile(prjPath);
        if (prjFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream prjOut(&prjFile);
            prjOut << "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\","
                   << "SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],"
                   << "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]]";
            prjFile.close();
        }

        return true;

    } catch (...) {
        qDebug() << "  ERROR: Exception during shapefile creation!";
        return false;
    }
}

void LayerPanel::writeShapefileHeader(QDataStream& stream, qint32 fileLength,
                                      int shapeType, double minX, double minY,
                                      double maxX, double maxY)
{
    // File code (big endian)
    stream << (qint32)9994;

    // Unused (5 * 4 bytes)
    for (int i = 0; i < 5; i++) {
        stream << (qint32)0;
    }

    // File length in words (big endian)
    stream << fileLength;

    // Switch to little endian for rest of header
    stream.setByteOrder(QDataStream::LittleEndian);

    // Version
    stream << (qint32)1000;

    // Shape type
    stream << (qint32)shapeType;

    // Bounding box (8 doubles = 64 bytes)
    stream << minX << minY << maxX << maxY;
    stream << (double)0.0 << (double)0.0;  // Z range
    stream << (double)0.0 << (double)0.0;  // M range

    // Switch back to big endian for record headers
    stream.setByteOrder(QDataStream::BigEndian);
}

// Helper function to create a shape record
QByteArray LayerPanel::createShapefileRecord(const MeshEntry* entry, int shapeType,
                                             double& minX, double& minY,
                                             double& maxX, double& maxY)
{
    QByteArray record;
    QDataStream stream(&record, QIODevice::WriteOnly);

    QString shapeName = entry->name;

    // CRITICAL FIX: Use geographic coordinates directly from entry->position
    // entry->position already contains lon/lat, NOT canvas pixels!
    QVector3D center = *entry->position;

    qDebug() << "    [createShapefileRecord]" << shapeName;
    qDebug() << "      Center (geographic):" << center.x() << "," << center.y();

    // Get rotation
    float rotation = entry->rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    // Collect all points for this shape IN GEOGRAPHIC COORDINATES
    QVector<QPointF> points;

    if (shapeName.startsWith("TempCircle")) {
        // Create circle as 36-point polygon
        double radius = entry->size ? entry->size->x() : 0.001;
        const int numPoints = 36;

        qDebug() << "      Circle radius:" << radius << "degrees";

        for (int i = 0; i <= numPoints; i++) {
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            // FIXED: Add radius to geographic coordinates
            double lat = center.y() + radius * qSin(angle);
            double lon = center.x() + radius * qCos(angle);
            points.append(QPointF(lon, lat));
        }
    } else if (shapeName.startsWith("TempPoint")) {
        // Single point - use geographic coordinates directly
        points.append(QPointF(center.x(), center.y()));
        qDebug() << "      Point at:" << center.x() << "," << center.y();
    } else {
        // Polygon or polyline - vertices are OFFSETS in geographic units
        for (Vector* v : entry->mesh->polygen) {
            // Apply rotation
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            // FIXED: Add offsets to geographic center (NOT canvas conversion!)
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points.append(QPointF(lon, lat));
        }

        // Close polygon
        if (shapeType == 5 && !points.isEmpty()) {
            points.append(points.first());
        }

        qDebug() << "      Polygon/Line vertices:" << points.size();
    }

    if (points.isEmpty()) {
        qDebug() << "      WARNING: No points generated!";
        return QByteArray();
    }

    // Debug: Print first point to verify coordinates are reasonable
    qDebug() << "      First point:" << points.first().x() << "," << points.first().y();

    // Update bounding box
    for (const QPointF& pt : points) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    qDebug() << "      Bounding box: (" << minX << "," << minY << ") to (" << maxX << "," << maxY << ")";

    // Write record (content only, header written separately)
    stream.setByteOrder(QDataStream::LittleEndian);

    // Shape type
    stream << (qint32)shapeType;

    if (shapeType == 1) {
        // Point
        stream << points[0].x() << points[0].y();
    } else if (shapeType == 3 || shapeType == 5) {
        // PolyLine or Polygon
        // Bounding box for this record
        double recMinX = 180.0, recMinY = 90.0, recMaxX = -180.0, recMaxY = -90.0;
        for (const QPointF& pt : points) {
            recMinX = qMin(recMinX, pt.x());
            recMaxX = qMax(recMaxX, pt.x());
            recMinY = qMin(recMinY, pt.y());
            recMaxY = qMax(recMaxY, pt.y());
        }
        stream << recMinX << recMinY << recMaxX << recMaxY;

        // Number of parts and points
        stream << (qint32)1;  // 1 part
        stream << (qint32)points.size();

        // Part index (starts at 0)
        stream << (qint32)0;

        // Points - write as lon, lat
        for (const QPointF& pt : points) {
            stream << pt.x() << pt.y();
        }
    }

    return record;
}

// Helper function to write DBF attribute file
bool LayerPanel::writeDBF(const QString& filePath, const QVector<const MeshEntry*>& shapes)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // DBF header
    stream << (quint8)0x03;  // Version (dBASE III)

    // Date (YY MM DD)
    QDate today = QDate::currentDate();
    stream << (quint8)(today.year() - 1900);
    stream << (quint8)today.month();
    stream << (quint8)today.day();

    // Number of records
    stream << (quint32)shapes.size();

    // Header size (32 bytes header + 32 bytes per field + 1 terminator)
    quint16 headerSize = 32 + (2 * 32) + 1;  // 2 fields: NAME and TYPE
    stream << headerSize;

    // Record size (1 delete flag + field sizes)
    quint16 recordSize = 1 + 50 + 20;  // NAME(50) + TYPE(20)
    stream << recordSize;

    // Reserved (20 bytes)
    for (int i = 0; i < 20; i++) {
        stream << (quint8)0;
    }

    // Field descriptors
    // Field 1: NAME
    writeDBFFieldDescriptor(stream, "NAME", 'C', 50);

    // Field 2: TYPE
    writeDBFFieldDescriptor(stream, "TYPE", 'C', 20);

    // Header terminator
    stream << (quint8)0x0D;

    // Records
    for (const MeshEntry* entry : shapes) {
        // Delete flag (space = not deleted)
        stream << (quint8)0x20;

        // NAME field (50 chars)
        QString name = entry->name.leftJustified(50, ' ', true);
        stream.writeRawData(name.toLatin1().constData(), 50);

        // TYPE field (20 chars)
        QString type = getShapeType(*entry).leftJustified(20, ' ', true);
        stream.writeRawData(type.toLatin1().constData(), 20);
    }

    // End of file marker
    stream << (quint8)0x1A;

    file.close();
    return true;
}

void LayerPanel::writeDBFFieldDescriptor(QDataStream& stream, const QString& name,
                                         char type, int length)
{
    // Field name (11 bytes, null-terminated)
    QByteArray fieldName = name.toLatin1().leftJustified(11, '\0', true);
    stream.writeRawData(fieldName.constData(), 11);

    // Field type (C=Character, N=Numeric, L=Logical, D=Date)
    stream << (quint8)type;

    // Reserved (4 bytes)
    stream << (quint32)0;

    // Field length
    stream << (quint8)length;

    // Decimal count
    stream << (quint8)0;

    // Reserved (14 bytes)
    for (int i = 0; i < 14; i++) {
        stream << (quint8)0;
    }
}


/* Create GeoJSON feature from MeshEntry */
QJsonObject LayerPanel::createGeoJSONFeature(const MeshEntry& entry, const QString& shapeId)
{
    QJsonObject feature;
    feature["type"] = "Feature";

    // Properties
    QJsonObject properties;
    properties["name"] = entry.name;
    properties["id"] = shapeId;
    properties["type"] = getShapeType(entry);
    feature["properties"] = properties;

    // Geometry
    QJsonObject geometry = getGeometryAsGeoJSON(entry);
    if (!geometry.isEmpty()) {
        feature["geometry"] = geometry;
    }

    return feature;
}

/* Create KML placemark from MeshEntry */
QString LayerPanel::createKMLPlacemark(const MeshEntry& entry, const QString& shapeId)
{
    QString kml;
    kml += "    <Placemark>\n";
    kml += "      <n>" + entry.name + "</n>\n";  // FIXED: changed from <n> to <n>
    kml += "      <description>Type: " + getShapeType(entry) + ", ID: " + shapeId + "</description>\n";

    QString geometry = getGeometryAsKML(entry);
    if (!geometry.isEmpty()) {
        kml += geometry;
    } else {
        qDebug() << "      WARNING: getGeometryAsKML returned empty for" << entry.name;
    }

    kml += "    </Placemark>\n";
    return kml;
}

/* Create GML feature from MeshEntry */
QString LayerPanel::createGMLFeature(const MeshEntry& entry, const QString& shapeId)
{
    QString gml;
    gml += "  <gml:featureMember>\n";
    gml += "    <Feature>\n";
    gml += "      <gml:name>" + entry.name + "</gml:name>\n";
    gml += "      <gml:description>Type: " + getShapeType(entry) + ", ID: " + shapeId + "</gml:description>\n";

    QString geometry = getGeometryAsGML(entry);
    if (!geometry.isEmpty()) {
        gml += geometry;
    }

    gml += "    </Feature>\n";
    gml += "  </gml:featureMember>\n";
    return gml;
}

/* Get geometry as GeoJSON object */
QJsonObject LayerPanel::getGeometryAsGeoJSON(const MeshEntry& entry)
{
    QJsonObject geometry;

    if (!entry.position || !entry.rotation || !entry.mesh) {
        return geometry;
    }

    QString shapeName = entry.name;

    // Center position is already in geographic coordinates
    QVector3D center = *entry.position;

    // Rotation
    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle")) {
        // Export circle as Polygon with multiple points forming a circle
        geometry["type"] = "Polygon";

        // Get radius from size (in same units as center coordinates - degrees)
        double radius = entry.size ? entry.size->x() : 0.001;  // Default ~111m if no size

        // If radius is already in degrees (your case), use directly:
        double radiusLat = radius;
        double radiusLon = radius;

        // Create circle as polygon with 36 points
        const int numPoints = 36;
        QJsonArray ring;

        for (int i = 0; i <= numPoints; i++) {  // +1 to close the polygon
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            double lat = center.y() + radiusLat * qSin(angle);
            double lon = center.x() + radiusLon * qCos(angle);

            QJsonArray point;
            point.append(lon);  // Longitude first (GeoJSON standard)
            point.append(lat);  // Latitude second
            ring.append(point);
        }

        QJsonArray coordinates;
        coordinates.append(ring);
        geometry["coordinates"] = coordinates;

    } else if (shapeName.startsWith("TempRectangle") ||
               shapeName.startsWith("TempPolygon")) {
        geometry["type"] = "Polygon";

        if (!entry.mesh->polygen.empty()) {
            QJsonArray coordinates;
            QJsonArray ring;

            for (Vector* v : entry.mesh->polygen) {
                // CRITICAL: Apply rotation
                float worldX = v->x * cosFwd - v->y * sinFwd;
                float worldY = v->x * sinFwd + v->y * cosFwd;

                // CRITICAL: Add to center (vertices are OFFSETS in same units as center)
                double lon = center.x() + worldX;
                double lat = center.y() + worldY;

                QJsonArray point;
                point.append(lon);
                point.append(lat);
                ring.append(point);
            }

            // Close ring
            if (!ring.isEmpty()) {
                ring.append(ring.first());
            }

            coordinates.append(ring);
            geometry["coordinates"] = coordinates;
        }

    } else if (shapeName.startsWith("TempPolyline")) {
        geometry["type"] = "LineString";

        if (!entry.mesh->polygen.empty()) {
            QJsonArray coordinates;

            for (Vector* v : entry.mesh->polygen) {
                float worldX = v->x * cosFwd - v->y * sinFwd;
                float worldY = v->x * sinFwd + v->y * cosFwd;

                double lon = center.x() + worldX;
                double lat = center.y() + worldY;

                QJsonArray point;
                point.append(lon);
                point.append(lat);
                coordinates.append(point);
            }

            geometry["coordinates"] = coordinates;
        }

    } else if (shapeName.startsWith("TempPoint")) {
        geometry["type"] = "Point";
        QJsonArray coordinates;
        coordinates.append(center.x());
        coordinates.append(center.y());
        geometry["coordinates"] = coordinates;
    }

    return geometry;
}

/* Get geometry as KML string - COMPLETE FIXED VERSION */
QString LayerPanel::getGeometryAsKML(const MeshEntry& entry)
{
    QString kml;

    qDebug() << "      [getGeometryAsKML] Processing:" << entry.name;

    if (!entry.position) {
        qDebug() << "        ERROR: position is NULL!";
        return kml;
    }
    if (!entry.rotation) {
        qDebug() << "        ERROR: rotation is NULL!";
        return kml;
    }
    if (!entry.mesh) {
        qDebug() << "        ERROR: mesh is NULL!";
        return kml;
    }

    QString shapeName = entry.name;

    // Center position is already in geographic coordinates (lon, lat)
    QVector3D center = *entry.position;

    qDebug() << "        Position:" << center.x() << "," << center.y();
    qDebug() << "        Mesh vertices:" << entry.mesh->polygen.size();

    // Rotation
    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle")) {
        qDebug() << "        Exporting as Circle (Polygon)";

        // Export circle as Polygon with multiple points forming a circle
        kml += "      <Polygon>\n";
        kml += "        <outerBoundaryIs>\n";
        kml += "          <LinearRing>\n";
        kml += "            <coordinates>\n";

        // Get radius from size (in same units as center coordinates - degrees)
        double radius = entry.size ? entry.size->x() : 0.001;
        double radiusLat = radius;
        double radiusLon = radius;

        qDebug() << "        Circle radius:" << radius;

        // Create circle as polygon with 36 points
        const int numPoints = 36;
        for (int i = 0; i <= numPoints; i++) {  // +1 to close the polygon
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            double lat = center.y() + radiusLat * qSin(angle);
            double lon = center.x() + radiusLon * qCos(angle);

            kml += "              " + QString::number(lon, 'f', 8) + "," +
                   QString::number(lat, 'f', 8) + ",0\n";
        }

        kml += "            </coordinates>\n";
        kml += "          </LinearRing>\n";
        kml += "        </outerBoundaryIs>\n";
        kml += "      </Polygon>\n";

    } else if (shapeName.startsWith("TempPoint")) {
        qDebug() << "        Exporting as Point";

        // Point
        kml += "      <Point>\n";
        kml += "        <coordinates>" + QString::number(center.x(), 'f', 8) + "," +
               QString::number(center.y(), 'f', 8) + ",0</coordinates>\n";
        kml += "      </Point>\n";

    } else if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line")) {
        qDebug() << "        Exporting as LineString";

        // LineString
        kml += "      <LineString>\n";
        kml += "        <coordinates>\n";

        for (Vector* v : entry.mesh->polygen) {
            // Apply rotation
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            // Add to center (vertices are OFFSETS in same units as center)
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "          " + QString::number(lon, 'f', 8) + "," +
                   QString::number(lat, 'f', 8) + ",0\n";
        }

        kml += "        </coordinates>\n";
        kml += "      </LineString>\n";

    } else if (shapeName.startsWith("TempRectangle") ||
               shapeName.startsWith("TempPolygon")) {
        qDebug() << "        Exporting as Polygon";

        // Polygon
        kml += "      <Polygon>\n";
        kml += "        <outerBoundaryIs>\n";
        kml += "          <LinearRing>\n";
        kml += "            <coordinates>\n";

        for (Vector* v : entry.mesh->polygen) {
            // Apply rotation
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            // Add to center (vertices are OFFSETS in same units as center)
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "              " + QString::number(lon, 'f', 8) + "," +
                   QString::number(lat, 'f', 8) + ",0\n";
        }

        // Close the ring
        if (!entry.mesh->polygen.empty()) {
            Vector* first = entry.mesh->polygen[0];
            float worldX = first->x * cosFwd - first->y * sinFwd;
            float worldY = first->x * sinFwd + first->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            kml += "              " + QString::number(lon, 'f', 8) + "," +
                   QString::number(lat, 'f', 8) + ",0\n";
        }

        kml += "            </coordinates>\n";
        kml += "          </LinearRing>\n";
        kml += "        </outerBoundaryIs>\n";
        kml += "      </Polygon>\n";

    } else {
        qDebug() << "        WARNING: Unknown shape type:" << shapeName;
    }

    qDebug() << "        Generated KML length:" << kml.length() << "characters";
    return kml;
}

/* Get geometry as GML string */
QString LayerPanel::getGeometryAsGML(const MeshEntry& entry)
{
    QString gml;

    qDebug() << "      [getGeometryAsGML] Processing:" << entry.name;

    if (!entry.position) {
        qDebug() << "        ERROR: position is NULL!";
        return gml;
    }
    if (!entry.rotation) {
        qDebug() << "        ERROR: rotation is NULL!";
        return gml;
    }
    if (!entry.mesh) {
        qDebug() << "        ERROR: mesh is NULL!";
        return gml;
    }

    QString shapeName = entry.name;

    // Center position is already in geographic coordinates (lon, lat)
    QVector3D center = *entry.position;

    qDebug() << "        Position:" << center.x() << "," << center.y();
    qDebug() << "        Mesh vertices:" << entry.mesh->polygen.size();

    // Rotation
    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle")) {
        qDebug() << "        Exporting as Circle (Polygon)";

        // Export circle as Polygon with multiple points forming a circle
        gml += "      <gml:Polygon>\n";
        gml += "        <gml:outerBoundaryIs>\n";
        gml += "          <gml:LinearRing>\n";
        gml += "            <gml:coordinates>\n";

        // Get radius from size (in same units as center coordinates - degrees)
        double radius = entry.size ? entry.size->x() : 0.001;
        double radiusLat = radius;
        double radiusLon = radius;

        qDebug() << "        Circle radius:" << radius;

        // Create circle as polygon with 36 points
        const int numPoints = 36;
        QStringList coords;
        for (int i = 0; i <= numPoints; i++) {  // +1 to close the polygon
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            double lat = center.y() + radiusLat * qSin(angle);
            double lon = center.x() + radiusLon * qCos(angle);

            coords.append(QString::number(lon, 'f', 8) + "," + QString::number(lat, 'f', 8));
        }

        gml += "              " + coords.join(" ") + "\n";
        gml += "            </gml:coordinates>\n";
        gml += "          </gml:LinearRing>\n";
        gml += "        </gml:outerBoundaryIs>\n";
        gml += "      </gml:Polygon>\n";

    } else if (shapeName.startsWith("TempPoint")) {
        qDebug() << "        Exporting as Point";

        // Point
        gml += "      <gml:Point>\n";
        gml += "        <gml:coordinates>" + QString::number(center.x(), 'f', 8) + "," +
               QString::number(center.y(), 'f', 8) + "</gml:coordinates>\n";
        gml += "      </gml:Point>\n";

    } else if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line")) {
        qDebug() << "        Exporting as LineString";

        // LineString
        gml += "      <gml:LineString>\n";
        gml += "        <gml:coordinates>\n";

        QStringList coords;
        for (Vector* v : entry.mesh->polygen) {
            // Apply rotation
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            // Add to center (vertices are OFFSETS in same units as center)
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            coords.append(QString::number(lon, 'f', 8) + "," + QString::number(lat, 'f', 8));
        }

        gml += "          " + coords.join(" ") + "\n";
        gml += "        </gml:coordinates>\n";
        gml += "      </gml:LineString>\n";

    } else if (shapeName.startsWith("TempRectangle") ||
               shapeName.startsWith("TempPolygon")) {
        qDebug() << "        Exporting as Polygon";

        // Polygon
        gml += "      <gml:Polygon>\n";
        gml += "        <gml:outerBoundaryIs>\n";
        gml += "          <gml:LinearRing>\n";
        gml += "            <gml:coordinates>\n";

        QStringList coords;
        for (Vector* v : entry.mesh->polygen) {
            // Apply rotation
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;

            // Add to center (vertices are OFFSETS in same units as center)
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            coords.append(QString::number(lon, 'f', 8) + "," + QString::number(lat, 'f', 8));
        }

        // Close the ring
        if (!entry.mesh->polygen.empty()) {
            Vector* first = entry.mesh->polygen[0];
            float worldX = first->x * cosFwd - first->y * sinFwd;
            float worldY = first->x * sinFwd + first->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;

            coords.append(QString::number(lon, 'f', 8) + "," + QString::number(lat, 'f', 8));
        }

        gml += "              " + coords.join(" ") + "\n";
        gml += "            </gml:coordinates>\n";
        gml += "          </gml:LinearRing>\n";
        gml += "        </gml:outerBoundaryIs>\n";
        gml += "      </gml:Polygon>\n";

    } else {
        qDebug() << "        WARNING: Unknown shape type:" << shapeName;
    }

    qDebug() << "        Generated GML length:" << gml.length() << "characters";
    return gml;
}

/* Helper to get shape type from mesh entry */
QString LayerPanel::getShapeType(const MeshEntry& entry)
{
    QString shapeName = entry.name;  // entry.name is already QString

    if (shapeName.startsWith("Circle")) return "Circle";
    if (shapeName.startsWith("Rectangle")) return "Rectangle";
    if (shapeName.startsWith("Polygon")) return "Polygon";
    if (shapeName.startsWith("Polyline") || shapeName.startsWith("Line")) return "Line";
    if (shapeName.startsWith("Point")) return "Point";

    return "Unknown";
}
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

    // qDebug() << "✓ LayerPanel::toJson() - Saved" << layersArray.size() << "layers";

    return json;
}

/* Deserialize layer data from JSON */
void LayerPanel::fromJson(const QJsonObject& json)
{
    // qDebug() << "✓ LayerPanel::fromJson() - Starting layer restoration";

    // Clear existing layer data (but keep UI structure)
    layerShapes.clear();
    shapeToLayer.clear();
    layerVisibility.clear();

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

            // Support both old format (plain string) and new format (object with id + displayName)
            if (shapeVal.isString()) {
                shapeId = shapeVal.toString();
            } else if (shapeVal.isObject()) {
                QJsonObject shapeObj = shapeVal.toObject();
                shapeId = shapeObj["id"].toString();
                displayName = shapeObj["displayName"].toString();
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

    // qDebug() << "✓ LayerPanel::fromJson() - Restored" << layerItems.size() << "layers";
    // qDebug() << "  Active layer:" << activeLayerName;
    // qDebug() << "  Total shapes restored:" << shapeToLayer.size();

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

    return "Shape";  // Default fallback
}

/* Export layer to FlatGeobuf format */
bool LayerPanel::exportToFlatGeobuf(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    qDebug() << "==========================================";
    qDebug() << "FLATGEOBUF EXPORT DEBUG";
    qDebug() << "==========================================";
    qDebug() << "Layer Name:" << layerName;
    qDebug() << "Shapes to export:" << shapeIds;

    // FlatGeobuf is a binary format using FlatBuffers
    // For now, we'll export as GeoJSON and provide conversion instructions
    // A full implementation would require the FlatGeobuf library

    QFileInfo fileInfo(filePath);
    QString geoJsonPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_temp.geojson";

    qDebug() << "FlatGeobuf export: Converting via GeoJSON...";

    // First export as GeoJSON
    bool success = exportToGeoJSON(layerName, shapeIds, geoJsonPath);

    if (!success) {
        qDebug() << "ERROR: Failed to create temporary GeoJSON!";
        return false;
    }

    // Try to use ogr2ogr if available (GDAL tool)
    QProcess process;
    QStringList arguments;
    arguments << "-f" << "FlatGeobuf" << filePath << geoJsonPath;

    process.start("ogr2ogr", arguments);
    bool conversionSuccess = process.waitForFinished(30000); // 30 second timeout

    if (conversionSuccess && process.exitCode() == 0) {
        qDebug() << "✓ FlatGeobuf created successfully using ogr2ogr";

        // Clean up temporary GeoJSON
        QFile::remove(geoJsonPath);

        qDebug() << "==========================================";
        qDebug() << "FLATGEOBUF EXPORT SUCCESSFUL!";
        qDebug() << "File:" << filePath;
        qDebug() << "==========================================\n";

        return true;
    } else {
        qDebug() << "WARNING: ogr2ogr not available or conversion failed";
        qDebug() << "Creating instruction file instead...";

        // Create instruction file
        QString instructionPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_README.txt";
        QFile readmeFile(instructionPath);
        if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream readme(&readmeFile);
            readme << "FlatGeobuf Export Instructions\n";
            readme << "================================\n\n";
            readme << "Layer: " << layerName << "\n\n";
            readme << "FlatGeobuf requires GDAL/OGR for conversion.\n\n";
            readme << "GeoJSON file created: " << QFileInfo(geoJsonPath).fileName() << "\n\n";
            readme << "To convert to FlatGeobuf:\n";
            readme << "1. Install GDAL: https://gdal.org/download.html\n";
            readme << "2. Run: ogr2ogr -f FlatGeobuf output.fgb " << QFileInfo(geoJsonPath).fileName() << "\n\n";
            readme << "Or use QGIS:\n";
            readme << "1. Load the .geojson file\n";
            readme << "2. Right-click layer > Export > Save Features As\n";
            readme << "3. Select 'FlatGeobuf' format\n";
            readmeFile.close();
        }

        qDebug() << "==========================================";
        qDebug() << "FLATGEOBUF: GeoJSON created, manual conversion needed";
        qDebug() << "Files created:";
        qDebug() << "  " << geoJsonPath;
        qDebug() << "  " << instructionPath;
        qDebug() << "==========================================\n";

        return true; // Return true since GeoJSON was created
    }
}

/* Export layer to CSV format */
/* Export layer to CSV format - FIXED with CSVT for QGIS auto-detection */
bool LayerPanel::exportToCSV(const QString& layerName, const QStringList& shapeIds, const QString& filePath)
{
    qDebug() << "==========================================";
    qDebug() << "CSV EXPORT DEBUG";
    qDebug() << "==========================================";
    qDebug() << "Layer Name:" << layerName;
    qDebug() << "Shapes to export:" << shapeIds;

    if (!m_canvasWidget) {
        qDebug() << "ERROR: m_canvasWidget is NULL!";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "ERROR: Cannot create CSV file!";
        return false;
    }

    QTextStream out(&file);

    // CSV Header
    out << "Name,Type,Longitude,Latitude,Geometry_WKT\n";

    int exportedCount = 0;

    // Check Meshes
    for (const auto& pair : m_canvasWidget->Meshes) {
        const MeshEntry& entry = pair.second;
        QString entryName = entry.name;
        QString meshKey = QString::fromStdString(pair.first);

        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName || shapeId == meshKey) {
                QString csvLine = createCSVLine(entry);
                if (!csvLine.isEmpty()) {
                    out << csvLine << "\n";
                    exportedCount++;
                    qDebug() << "  Exported from Meshes:" << entryName;
                }
                break;
            }
        }
    }

    // Check tempMeshes
    for (const MeshEntry& entry : m_canvasWidget->tempMeshes) {
        QString entryName = entry.name;

        for (const QString& shapeId : shapeIds) {
            if (shapeId == entryName) {
                QString csvLine = createCSVLine(entry);
                if (!csvLine.isEmpty()) {
                    out << csvLine << "\n";
                    exportedCount++;
                    qDebug() << "  Exported from tempMeshes:" << entryName;
                }
                break;
            }
        }
    }

    file.close();

    // ========== CREATE CSVT FILE FOR QGIS AUTO-DETECTION ==========
    // CSVT file tells QGIS the data types of each column
    QFileInfo csvFileInfo(filePath);
    QString csvtPath = csvFileInfo.absolutePath() + "/" + csvFileInfo.completeBaseName() + ".csvt";

    QFile csvtFile(csvtPath);
    if (csvtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream csvtOut(&csvtFile);
        // Define column types: String, String, Real, Real, WKT
        csvtOut << "\"String\",\"String\",\"Real\",\"Real\",\"WKT\"\n";
        csvtFile.close();
        qDebug() << "  ✓ Created CSVT file for QGIS geometry detection";
    }

    // ========== CREATE README WITH IMPORT INSTRUCTIONS ==========
    QString readmePath = csvFileInfo.absolutePath() + "/" + csvFileInfo.completeBaseName() + "_IMPORT_INSTRUCTIONS.txt";
    QFile readmeFile(readmePath);
    if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream readme(&readmeFile);
        readme << "CSV + WKT Geometry Import Instructions for QGIS\n";
        readme << "=================================================\n\n";
        readme << "Layer: " << layerName << "\n";
        readme << "Total features: " << exportedCount << "\n\n";
        readme << "FILES CREATED:\n";
        readme << "  1. " << csvFileInfo.fileName() << " - CSV data file\n";
        readme << "  2. " << QFileInfo(csvtPath).fileName() << " - Column type definitions\n\n";
        readme << "IMPORTING IN QGIS:\n";
        readme << "==================\n\n";
        readme << "METHOD 1: Drag and Drop (Easiest)\n";
        readme << "  1. Simply drag the .csv file into QGIS\n";
        readme << "  2. QGIS will auto-detect geometry from WKT column (using .csvt file)\n";
        readme << "  3. All shapes should display on map\n\n";
        readme << "METHOD 2: Add Delimited Text Layer\n";
        readme << "  1. Layer → Add Layer → Add Delimited Text Layer\n";
        readme << "  2. Select: " << csvFileInfo.fileName() << "\n";
        readme << "  3. File Format: CSV\n";
        readme << "  4. Geometry Definition: Well Known Text (WKT)\n";
        readme << "  5. Geometry field: Geometry_WKT\n";
        readme << "  6. Geometry CRS: EPSG:4326 (WGS 84)\n";
        readme << "  7. Click Add\n\n";
        readme << "TROUBLESHOOTING:\n";
        readme << "================\n";
        readme << "If geometries don't show:\n";
        readme << "  - Verify the .csvt file is in the same folder as .csv\n";
        readme << "  - Manually select 'WKT' as geometry type in import dialog\n";
        readme << "  - Check that CRS is set to EPSG:4326\n\n";
        readme << "ALTERNATIVE: Use GeoJSON Instead\n";
        readme << "=================================\n";
        readme << "For easier import, export as GeoJSON format:\n";
        readme << "  - GeoJSON is automatically recognized by QGIS\n";
        readme << "  - No manual configuration needed\n";
        readme << "  - Just drag and drop - it works!\n";
        readmeFile.close();
        qDebug() << "  ✓ Created import instructions file";
    }

    qDebug() << "==========================================";
    qDebug() << "CSV EXPORT SUCCESSFUL!";
    qDebug() << "Total shapes exported:" << exportedCount;
    qDebug() << "Files created:";
    qDebug() << "  - " << filePath;
    qDebug() << "  - " << csvtPath;
    qDebug() << "  - " << readmePath;
    qDebug() << "==========================================\n";

    return exportedCount > 0;
}

/* Create CSV line for a shape */
QString LayerPanel::createCSVLine(const MeshEntry& entry)
{
    if (!entry.position || !entry.rotation || !entry.mesh) {
        return QString();
    }

    QStringList fields;

    // Name (escaped for CSV)
    QString name = entry.name;
    if (name.contains(',') || name.contains('"')) {
        name = "\"" + name.replace("\"", "\"\"") + "\"";
    }
    fields << name;

    // Type
    fields << getShapeType(entry);

    // Center coordinates
    QVector3D center = *entry.position;
    fields << QString::number(center.x(), 'f', 8);
    fields << QString::number(center.y(), 'f', 8);

    // Geometry as WKT (Well-Known Text)
    QString wkt = createWKTGeometry(entry);
    if (wkt.contains(',') || wkt.contains('"')) {
        wkt = "\"" + wkt.replace("\"", "\"\"") + "\"";
    }
    fields << wkt;

    return fields.join(",");
}

/* Create WKT (Well-Known Text) geometry */
QString LayerPanel::createWKTGeometry(const MeshEntry& entry)
{
    QString shapeName = entry.name;
    QVector3D center = *entry.position;

    float rotation = entry.rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    if (shapeName.startsWith("TempCircle")) {
        // Polygon
        double radius = entry.size ? entry.size->x() : 0.001;
        const int numPoints = 36;

        QStringList points;
        for (int i = 0; i <= numPoints; i++) {
            double angle = (i * 360.0 / numPoints) * M_PI / 180.0;
            double lat = center.y() + radius * qSin(angle);
            double lon = center.x() + radius * qCos(angle);
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("POLYGON((%1))").arg(points.join(","));

    } else if (shapeName.startsWith("TempPoint")) {
        // Point
        return QString("POINT(%1 %2)")
            .arg(center.x(), 0, 'f', 8)
            .arg(center.y(), 0, 'f', 8);

    } else if (shapeName.startsWith("TempPolyline") || shapeName.startsWith("Line")) {
        // LineString
        QStringList points;
        for (Vector* v : entry.mesh->polygen) {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("LINESTRING(%1)").arg(points.join(","));

    } else {
        // Polygon (Rectangle, Polygon)
        QStringList points;
        for (Vector* v : entry.mesh->polygen) {
            float worldX = v->x * cosFwd - v->y * sinFwd;
            float worldY = v->x * sinFwd + v->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        // Close the ring
        if (!entry.mesh->polygen.empty()) {
            Vector* first = entry.mesh->polygen[0];
            float worldX = first->x * cosFwd - first->y * sinFwd;
            float worldY = first->x * sinFwd + first->y * cosFwd;
            double lon = center.x() + worldX;
            double lat = center.y() + worldY;
            points << QString("%1 %2").arg(lon, 0, 'f', 8).arg(lat, 0, 'f', 8);
        }

        return QString("POLYGON((%1))").arg(points.join(","));
    }
}
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
        rl = makeDefaultExtents(rl, m_canvasWidget);
    }

    // ── 6. Register ───────────────────────────────────────────────────────
    rasterLayers[layerName]   = rl;
    rasterLayerOrder.append(layerName);

    // ── 7. Create tree item + toggle ──────────────────────────────────────
    createRasterLayerItem(layerName);

    // ── 8. Notify canvas ─────────────────────────────────────────────────
    emit rasterLayerChanged();

    if (m_canvasWidget && m_canvasWidget->gislib) {
        m_canvasWidget->gislib->fitToBounds(
            rl.minLat, rl.minLon,
            rl.maxLat, rl.maxLon,
            0   // zoomOffset = 0 (use the auto-computed zoom level)
            );
    }

    qDebug() << "✓ Raster layer added:" << layerName
             << "| georef:" << (georefFound ? "yes" : "no (default)")
             << "| extents: lon[" << rl.minLon << "," << rl.maxLon << "]"
             << "lat[" << rl.minLat << "," << rl.maxLat << "]"
             << "| size:" << pixmap.width() << "x" << pixmap.height();
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

    updateRasterVisibilityIcon(layerName, newVisible);

    // Reuse layerVisibilityChanged so canvas auto-repaints (same slot already
    // connected in CanvasWidget::setLayerPanel).
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

void LayerPanel::drawRasterLayers(QPainter& painter, GISlib* gislib) const
{
    if (!gislib) return;

    painter.save();

    for (const QString& name : rasterLayerOrder) {
        if (!rasterLayers.contains(name)) continue;
        const RasterLayer& rl = rasterLayers[name];

        if (!rl.visible)        continue;
        if (rl.pixmap.isNull()) continue;

        // geoToCanvas(lat, lon) — latitude first
        QPointF topLeft  = gislib->geoToCanvas(rl.maxLat, rl.minLon);
        QPointF botRight = gislib->geoToCanvas(rl.minLat, rl.maxLon);

        QRectF destRect(topLeft, botRight);

        if (destRect.width() < 1.0 || destRect.height() < 1.0)
            continue;

        painter.setOpacity(rl.opacity);
        painter.drawPixmap(destRect, rl.pixmap,
                           QRectF(0, 0, rl.pixmap.width(), rl.pixmap.height()));
    }

    painter.setOpacity(1.0);
    painter.restore();
}


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

        qDebug() << "  ✓ World file:" << (base + "." + wext)
                 << "lon[" << minLon << "," << maxLon << "]"
                 << "lat[" << minLat << "," << maxLat << "]";
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
        qDebug() << "  ✗ GeoTIFF: missing scale or tiepoint tags in" << imagePath;
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

    qDebug() << "  GeoTIFF native bbox: UL(" << ulX << "," << ulY
             << ")  LR(" << lrX << "," << lrY << ")";
    qDebug() << "  GeoTIFF scale: X=" << scaleX << " Y=" << scaleY;
    qDebug() << "  Image size:" << imgW << "x" << imgH;

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

    qDebug() << "  GeoTIFF GeoKeys: modelType=" << modelType << " epsgCode=" << epsgCode;

    // ── Determine if reprojection is needed ───────────────────────────────

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
        qDebug() << "  GeoTIFF: no GeoKey modelType — auto-detect needsReproject="
                 << needsReproject;
    }

    // ── If EPSG not found, guess from coordinate ranges ───────────────────
    if (needsReproject && epsgCode <= 0) {
        qDebug() << "  ✗ GeoTIFF: projected CRS but EPSG code unknown — cannot reproject";
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
            qDebug() << "  ✗ GeoTIFF: cannot create CRS for" << srcEpsgStr;
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
                qDebug() << "    Native(" << c[0] << "," << c[1]
                         << ") → WGS84(" << geo.x() << "," << geo.y() << ")";
            } catch (QgsCsException& e) {
                qDebug() << "  ✗ GeoTIFF reprojection failed:" << e.what();
                return false;
            }
        }

        // qDebug() << "  ✓ Reprojected" << srcEpsgStr << "→ EPSG:4326";
    }

    // ── Final sanity check ────────────────────────────────────────────────
    if (minLon < -180.1 || maxLon > 180.1) {
        qDebug() << "  ✗ GeoTIFF lon out of range:" << minLon << maxLon;
        return false;
    }
    if (minLat < -90.1 || maxLat > 90.1) {
        qDebug() << "  ✗ GeoTIFF lat out of range:" << minLat << maxLat;
        return false;
    }
    if (maxLon - minLon < 1e-9 || maxLat - minLat < 1e-9) {
        qDebug() << "  ✗ GeoTIFF zero-size extent";
        return false;
    }

    // ── Write to out only after all validation passes ─────────────────────
    out.minLon = minLon;
    out.maxLon = maxLon;
    out.minLat = minLat;
    out.maxLat = maxLat;

    // qDebug() << "  ✓ GeoTIFF final extents: lon[" << minLon << "," << maxLon
    //          << "]  lat[" << minLat << "," << maxLat << "]";
    return true;
}

/* =========================================================================
   HELPER — compute a sensible default bounding box when no georef exists.
   ========================================================================= */
static RasterLayer makeDefaultExtents(const RasterLayer& rl, CanvasWidget* canvas)
{
    RasterLayer out = rl;

    const int imgW = rl.pixmap.width();
    const int imgH = rl.pixmap.height();

    double centreLon = 0.0;
    double centreLat = 20.0;  // sensible world-view default
    double degPerPixLon = 0.1;
    double degPerPixLat = 0.1;

    if (canvas && canvas->gislib) {
        GISlib* gis = canvas->gislib;

        // Get the map centre in geographic coordinates
        QPointF centreCanvas(gis->width() / 2.0, gis->height() / 2.0);
        QPointF centreGeo = gis->canvasToGeo(centreCanvas);
        centreLon = centreGeo.x();
        centreLat = centreGeo.y();

        // Compute degrees-per-pixel by sampling two pixels apart
        // This uses the actual geoToCanvas/canvasToGeo math — no zoom access needed
        QPointF p1 = gis->canvasToGeo(QPointF(gis->width()/2.0,       gis->height()/2.0));
        QPointF p2 = gis->canvasToGeo(QPointF(gis->width()/2.0 + 1.0, gis->height()/2.0));
        QPointF p3 = gis->canvasToGeo(QPointF(gis->width()/2.0,       gis->height()/2.0 + 1.0));

        // How many degrees does 1 canvas pixel span in each direction?
        degPerPixLon = std::abs(p2.x() - p1.x());
        degPerPixLat = std::abs(p3.y() - p1.y());

        // Guard against degenerate values
        if (degPerPixLon < 1e-12) degPerPixLon = 0.001;
        if (degPerPixLat < 1e-12) degPerPixLat = 0.001;
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

    // qDebug() << "  ℹ No georef — pixel-scale default:"
    //          << "lon[" << out.minLon << "," << out.maxLon << "]"
    //          << "lat[" << out.minLat << "," << out.maxLat << "]"
    //          << "| deg/px: lon=" << degPerPixLon << " lat=" << degPerPixLat;

    return out;
}
// void LayerPanel::runUnitTestsOnce()
// {
//            if (!GuiTestControl::isEnabled()) return;
//     static bool testsRun = false;
//     if (testsRun) return;
//     testsRun = true;

//     QTimer::singleShot(0, []() {
//         Console* console = nullptr;
//         MainWindow* mw = MainWindow::instance();
//         if (mw && mw->databaseEditor && mw->databaseEditor->console) {
//             console = mw->databaseEditor->console;
//         }
//         if (!console) {
//             qDebug() << "LayerPanel: console not available, cannot run tests";
//             return;
//         }

//         // Create a temporary LayerPanel (no parent, won't show)
//         LayerPanel* testPanel = new LayerPanel(nullptr);
//         runLayerPanelTests(testPanel, console);
//         testPanel->deleteLater();
//     });
// }
