/* ========================================================================= */
/* File: selectlayerdialog.h                                                 */
/* Purpose: Dialog for selecting and managing layers                         */
/* Written by: [Your Name]                                                   */
/* ========================================================================= */

#ifndef SELECTLAYERDIALOG_H
#define SELECTLAYERDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QInputDialog>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
QT_END_NAMESPACE

// %%% Layer Tree Widget %%%
/* Custom tree widget for displaying layers with context menu */
class LayerTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit LayerTreeWidget(QWidget *parent = nullptr);

    // Add a new layer to the tree
    void addLayer(const QString &layerName, const QString &parentLayer = QString());

    // Get selected layer name
    QString getSelectedLayer() const;

    // Get selected layer item
    QTreeWidgetItem* getSelectedItem() const;

protected:
    // Handle right-click context menu
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    // Handle adding new layer
    void onAddLayer();
    // Handle deleting layer
    void onDeleteLayer();
    // Handle renaming layer
    void onRenameLayer();

signals:
    // Signal when layer is added
    void layerAdded(const QString &layerName, const QString &parentLayer);
    // Signal when layer is deleted
    void layerDeleted(const QString &layerName);
    // Signal when layer is renamed
    void layerRenamed(const QString &oldName, const QString &newName);

private:
    // Context menu for layer operations
    QMenu *contextMenu;
    // Action for adding layer
    QAction *addLayerAction;
    // Action for deleting layer
    QAction *deleteLayerAction;
    // Action for renaming layer
    QAction *renameLayerAction;
    // Store last clicked position
    QPoint lastContextMenuPos;
};

// %%% Select Layer Dialog %%%
/* Dialog for selecting and managing layers */
class SelectLayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectLayerDialog(QWidget *parent = nullptr);

    // Get selected layer name
    QString getSelectedLayer() const;

    // Set available layers
    void setLayers(const QStringList &layers);

    // Add a single layer
    void addLayer(const QString &layerName);

private slots:
    // Handle OK button click
    void onOkClicked();
    // Handle Cancel button click
    void onCancelClicked();
    // Handle layer addition
    void onLayerAdded(const QString &layerName, const QString &parentLayer);

signals:
    // Signal when layer is selected
    void layerSelected(const QString &layerName);
    // Signal when new layer is created
    void newLayerCreated(const QString &layerName, const QString &parentLayer);

private:
    // Setup UI components
    void setupUI();

    // %%% UI Components %%%
    // Tree widget for layers
    LayerTreeWidget *layerTree;
    // OK button
    QPushButton *okButton;
    // Cancel button
    QPushButton *cancelButton;
    // Main layout
    QVBoxLayout *mainLayout;
    // Button layout
    QHBoxLayout *buttonLayout;
};

#endif // SELECTLAYERDIALOG_H
