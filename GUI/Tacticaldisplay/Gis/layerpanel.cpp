
/* ========================================================================= */
/* File: layerpanel.cpp                                                     */
/* Purpose: Enhanced layer panel with visibility toggle                    */
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
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <QDir>
#include <core/Hierarchy/Struct/vector.h>

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

    // Create 3-column tree
    layerTree = new QTreeWidget(mainWidget);
    layerTree->setColumnCount(3);
    layerTree->setHeaderHidden(true);
    layerTree->setContextMenuPolicy(Qt::CustomContextMenu);
    layerTree->setSelectionMode(QAbstractItemView::SingleSelection);
    layerTree->setDragEnabled(true);
    layerTree->setAcceptDrops(true);
    layerTree->setDropIndicatorShown(true);
    layerTree->setDragDropMode(QAbstractItemView::InternalMove);
    layerTree->setRootIsDecorated(false);
    layerTree->setIndentation(12);

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
    addLayerAction = new QAction("Add Layer", this);
    // removeLayerAction = new QAction("Remove Layer", this);
    renameLayerAction = new QAction("Rename Layer", this);
    exportLayerAction = new QAction("Export Layer", this);

    // Connect actions
    connect(addLayerAction, &QAction::triggered, this, &LayerPanel::addLayer);
    // connect(removeLayerAction, &QAction::triggered, this, &LayerPanel::removeLayer);
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
            showMenu = true;
        }
        // For actual layer items (not root, not shapes)
        else if (selectedItem->parent() &&
                 selectedItem->parent()->text(0) == "Layers") {
            // This is a layer - show layer-specific actions
            contextMenu->addAction(removeLayerAction);
            contextMenu->addSeparator();
            contextMenu->addAction(renameLayerAction);
            contextMenu->addSeparator();
            contextMenu->addAction(exportLayerAction);
            showMenu = true;
        }
        // For shape items - optionally show shape-specific actions
        // or don't show menu at all
        else if (selectedItem->parent() &&
                 selectedItem->parent()->text(0) != "Layers") {
            // Shape item - no menu for shapes
            return;
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

// %%% Remove Layer %%%
/* Remove selected layer and its children */
void LayerPanel::removeLayer()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();

    if (!selectedItem || selectedItem->text(0) == "Layers") {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Selection");
        msgBox.setText("Please select a layer to remove!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    QString layerName = getFullLayerName(selectedItem);
    int shapeCount = layerShapes.value(layerName).count();
    int childCount = selectedItem->childCount();

    // Confirm deletion
    QString message = QString("Are you sure you want to remove layer '%1'?")
                          .arg(layerName);
    if (shapeCount > 0) {
        message += QString("\n\nThis will also remove %1 shape(s).").arg(shapeCount);
    }
    if (childCount > 0) {
        message += QString("\n\nThis will also remove %1 child layer(s).").arg(childCount);
    }

    QMessageBox msgBox(this);
    msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
    msgBox.setWindowTitle("Remove Layer");
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        // Remove from maps
        layerItems.remove(layerName);
        layerVisibility.remove(layerName);

        // Remove all shapes in this layer
        QStringList shapes = layerShapes.value(layerName);
        for (const QString& shapeId : shapes) {
            shapeToLayer.remove(shapeId);
        }

        layerShapes.remove(layerName);

        // Remove visibility toggle widget
        if (visibilityToggleWidgets.contains(layerName)) {
            QWidget* toggleWidget = visibilityToggleWidgets.value(layerName);
            visibilityToggleWidgets.remove(layerName);
            if (toggleWidget) {
                delete toggleWidget;
            }
        }

        // Remove from tree
        QTreeWidgetItem *parent = selectedItem->parent();
        if (parent) {
            parent->removeChild(selectedItem);
        } else {
            int index = layerTree->indexOfTopLevelItem(selectedItem);
            layerTree->takeTopLevelItem(index);
        }
        delete selectedItem;

        // Clear active layer if it was removed
        if (activeLayerName == layerName) {
            activeLayerName.clear();
            clearActiveLayerVisual();

            // Try to set another layer as active
            if (!layerItems.isEmpty()) {
                setActiveLayer(layerItems.firstKey());
            }
        }

        // Emit signal
        emit layerRemoved(layerName);
    }
}

// %%% Rename Layer %%%
/* Rename selected layer */
void LayerPanel::renameLayer()
{
    QTreeWidgetItem *selectedItem = layerTree->currentItem();

    if (!selectedItem || selectedItem->text(0) == "Layers") {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("No Selection");
        msgBox.setText("Please select a layer to rename!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    QString oldName = getFullLayerName(selectedItem);

    // Get new layer name from user
    bool ok;
    QString newName = QInputDialog::getText(this,
                                            "Rename Layer",
                                            "New Layer Name:",
                                            QLineEdit::Normal,
                                            oldName,
                                            &ok);

    if (!ok || newName.isEmpty()) {
        if (ok && newName.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
            msgBox.setWindowTitle("Invalid Name");
            msgBox.setText("Layer name cannot be empty!");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
        }
        return;
    }

    // Check if new name is different
    if (newName == oldName) {
        return;
    }

    // Check if layer name already exists
    if (layerExists(newName)) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(LayerPanelStyles::MessageBox);
        msgBox.setWindowTitle("Duplicate Name");
        msgBox.setText("A layer with this name already exists!");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    // Update maps
    layerItems[newName] = layerItems.take(oldName);
    layerShapes[newName] = layerShapes.take(oldName);
    layerVisibility[newName] = layerVisibility.take(oldName);

    // Update reverse lookup for shapes
    QStringList shapes = layerShapes.value(newName);
    for (const QString& shapeId : shapes) {
        shapeToLayer[shapeId] = newName;
    }

    // Update active layer name if needed
    if (activeLayerName == oldName) {
        activeLayerName = newName;
    }

    // Update visibility toggle widget map
    if (visibilityToggleWidgets.contains(oldName)) {
        QWidget* toggleWidget = visibilityToggleWidgets.take(oldName);
        visibilityToggleWidgets[newName] = toggleWidget;
    }

    // Update tree item
    selectedItem->setData(0, Qt::UserRole, newName);
    selectedItem->setText(0, newName);

    // Update shape count
    updateLayerShapeCount(newName);
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

    // Update shape count
    updateLayerShapeCount(layerName);
}

/* Get layer name for a shape */
QString LayerPanel::getLayerForShape(const QString& shapeId) const
{
    return shapeToLayer.value(shapeId);
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

    // Create shape item
    QTreeWidgetItem *shapeItem = new QTreeWidgetItem(layerItem);
    shapeItem->setText(0, QString("%1 - %2").arg(shapeType, shapeId));
    shapeItem->setData(0, Qt::UserRole, shapeId);
    shapeItem->setFlags(shapeItem->flags() & ~Qt::ItemIsEditable);

    // Set colors for dark background
    if (shapeType == "Circle") {
        shapeItem->setForeground(0, QBrush(QColor(255, 165, 0)));      // Orange
    } else if (shapeType == "Rectangle") {
        shapeItem->setForeground(0, QBrush(QColor(77, 166, 255)));     // Light Blue
    } else if (shapeType == "Polygon") {
        shapeItem->setForeground(0, QBrush(QColor(111, 207, 151)));    // Green
    } else if (shapeType == "Line") {
        shapeItem->setForeground(0, QBrush(QColor(255, 107, 107)));    // Red
    } else if (shapeType == "Point") {
        shapeItem->setForeground(0, QBrush(QColor(255, 217, 102)));    // Yellow
    }

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

    // Store label so we can update name on rename
    // (optional: store in a map if needed)
    layerTree->setItemWidget(item, 0, col0Widget);
    item->setText(0, "");  // Clear text since widget handles display

    // Store expand button reference
    expandButtons[layerName] = expandBtn;

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
    connect(visBtn, &QPushButton::clicked, this, [this, layerName]() {
        onVisibilityToggleClicked(layerName);
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

        // Convert radius from degrees to proper lat/lon offsets
        // If radius is in meters, convert to degrees:
        // Uncomment these lines if radius is in meters:
        // double radiusLat = radius / 111320.0;
        // double radiusLon = radius / (111320.0 * qCos(qDegreesToRadians(center.y())));

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
            shapesArray.append(shapeId);
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
            QString shapeId = shapeVal.toString();
            shapes.append(shapeId);
            shapeToLayer[shapeId] = layerName;

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
