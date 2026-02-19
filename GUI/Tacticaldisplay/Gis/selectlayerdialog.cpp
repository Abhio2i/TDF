/* ========================================================================= */
/* File: selectlayerdialog.cpp                                               */
/* Purpose: Implements layer selection dialog with context menu              */
/* Written by: [Your Name]                                                   */
/* ========================================================================= */

#include "selectlayerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHeaderView>
#include <QApplication>

// %%% Layer Tree Widget Implementation %%%

LayerTreeWidget::LayerTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    // Setup tree widget
    setColumnCount(1);
    setHeaderLabel("Layers");
    setSelectionMode(QTreeWidget::SingleSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Setup context menu
    contextMenu = new QMenu(this);

    addLayerAction = new QAction("Add Layer", this);
    deleteLayerAction = new QAction("Delete Layer", this);
    renameLayerAction = new QAction("Rename Layer", this);

    contextMenu->addAction(addLayerAction);
    contextMenu->addAction(deleteLayerAction);
    contextMenu->addAction(renameLayerAction);

    // Connect actions to slots
    connect(addLayerAction, &QAction::triggered, this, &LayerTreeWidget::onAddLayer);
    connect(deleteLayerAction, &QAction::triggered, this, &LayerTreeWidget::onDeleteLayer);
    connect(renameLayerAction, &QAction::triggered, this, &LayerTreeWidget::onRenameLayer);

    // Set initial layers
    addLayer("Default");
    addLayer("Background");
    addLayer("Foreground");

    // Expand all items by default
    expandAll();
}

void LayerTreeWidget::addLayer(const QString &layerName, const QString &parentLayer)
{
    if (layerName.isEmpty()) {
        return;
    }

    QTreeWidgetItem *parentItem = nullptr;

    if (!parentLayer.isEmpty()) {
        // Find parent item
        QList<QTreeWidgetItem*> items = findItems(parentLayer, Qt::MatchExactly | Qt::MatchRecursive, 0);
        if (!items.isEmpty()) {
            parentItem = items.first();
        }
    }

    // Create new layer item
    QTreeWidgetItem *newItem = new QTreeWidgetItem();
    newItem->setText(0, layerName);
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);

    if (parentItem) {
        parentItem->addChild(newItem);
    } else {
        addTopLevelItem(newItem);
    }

    // Auto-select the new layer
    setCurrentItem(newItem);
    scrollToItem(newItem);

    emit layerAdded(layerName, parentLayer);
}

QString LayerTreeWidget::getSelectedLayer() const
{
    QTreeWidgetItem *item = currentItem();
    if (item) {
        return item->text(0);
    }
    return QString();
}

QTreeWidgetItem* LayerTreeWidget::getSelectedItem() const
{
    return currentItem();
}

void LayerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // Store the position for later use
    lastContextMenuPos = event->pos();

    // Get item at click position
    QTreeWidgetItem *item = itemAt(lastContextMenuPos);

    // Enable/disable actions based on selection
    if (item) {
        setCurrentItem(item);
        deleteLayerAction->setEnabled(true);
        renameLayerAction->setEnabled(true);
    } else {
        deleteLayerAction->setEnabled(false);
        renameLayerAction->setEnabled(false);
    }

    // Show context menu
    contextMenu->exec(event->globalPos());
}

void LayerTreeWidget::onAddLayer()
{
    bool ok;
    QString layerName = QInputDialog::getText(this,
                                              "Add New Layer",
                                              "Enter layer name:",
                                              QLineEdit::Normal,
                                              QString(),
                                              &ok);

    if (ok && !layerName.isEmpty()) {
        // Get parent layer (selected item or root)
        QString parentLayer;
        QTreeWidgetItem *selectedItem = itemAt(lastContextMenuPos);
        if (selectedItem) {
            parentLayer = selectedItem->text(0);
        }

        addLayer(layerName, parentLayer);
    }
}

void LayerTreeWidget::onDeleteLayer()
{
    QTreeWidgetItem *item = getSelectedItem();
    if (!item) {
        return;
    }

    QString layerName = item->text(0);

    // Check if layer has children
    if (item->childCount() > 0) {
        QMessageBox::warning(this,
                             "Cannot Delete Layer",
                             QString("Layer '%1' has child layers. Please delete child layers first.")
                                 .arg(layerName));
        return;
    }

    // Ask for confirmation
    int result = QMessageBox::question(this,
                                       "Delete Layer",
                                       QString("Are you sure you want to delete layer '%1'?")
                                           .arg(layerName),
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        emit layerDeleted(layerName);

        // Remove from tree
        delete item;
    }
}

void LayerTreeWidget::onRenameLayer()
{
    QTreeWidgetItem *item = getSelectedItem();
    if (!item) {
        return;
    }

    QString oldName = item->text(0);

    bool ok;
    QString newName = QInputDialog::getText(this,
                                            "Rename Layer",
                                            "Enter new layer name:",
                                            QLineEdit::Normal,
                                            oldName,
                                            &ok);

    if (ok && !newName.isEmpty() && newName != oldName) {
        // Check if name already exists
        QList<QTreeWidgetItem*> items = findItems(newName, Qt::MatchExactly | Qt::MatchRecursive, 0);
        if (!items.isEmpty()) {
            QMessageBox::warning(this,
                                 "Duplicate Name",
                                 QString("Layer '%1' already exists.").arg(newName));
            return;
        }

        item->setText(0, newName);
        emit layerRenamed(oldName, newName);
    }
}

// %%% Select Layer Dialog Implementation %%%

SelectLayerDialog::SelectLayerDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("Select Layer");
    setMinimumSize(300, 400);
}

void SelectLayerDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);

    // Create layer tree
    layerTree = new LayerTreeWidget(this);
    mainLayout->addWidget(layerTree);

    // Create buttons
    buttonLayout = new QHBoxLayout();

    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals and slots
    connect(okButton, &QPushButton::clicked, this, &SelectLayerDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &SelectLayerDialog::onCancelClicked);
    connect(layerTree, &LayerTreeWidget::layerAdded,
            this, &SelectLayerDialog::onLayerAdded);
}

QString SelectLayerDialog::getSelectedLayer() const
{
    return layerTree->getSelectedLayer();
}

void SelectLayerDialog::setLayers(const QStringList &layers)
{
    layerTree->clear();
    for (const QString &layer : layers) {
        layerTree->addLayer(layer);
    }
}

void SelectLayerDialog::addLayer(const QString &layerName)
{
    layerTree->addLayer(layerName);
}

void SelectLayerDialog::onOkClicked()
{
    QString selectedLayer = layerTree->getSelectedLayer();
    if (!selectedLayer.isEmpty()) {
        emit layerSelected(selectedLayer);
    }
    accept();
}

void SelectLayerDialog::onCancelClicked()
{
    reject();
}

void SelectLayerDialog::onLayerAdded(const QString &layerName, const QString &parentLayer)
{
    emit newLayerCreated(layerName, parentLayer);
}
