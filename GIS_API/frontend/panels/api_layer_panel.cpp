#include "api_layer_panel.h"
#include "backend/api/layer_api.h"
#include "backend/api/shape_api.h"
#include "ui_action_api.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QDebug>
#include <QKeyEvent>
#include <QSet>
#include <QMessageBox>
#include "backend/api/shape_history_api.h"

ApiLayerPanel::ApiLayerPanel(QWidget* parent)
    : QDockWidget(QStringLiteral("Layers"), parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(220);
    setTitleBarWidget(new QWidget(this));
    setupUi();
    m_tree->installEventFilter(this);
    LayerApi* la = LayerApi::instance();
    connect(la, &LayerApi::layerAdded, this, [this](const QString&) { rebuildTree(); });
    connect(la, &LayerApi::layerDeleted, this, [this](const QString&) { rebuildTree(); });
    connect(la, &LayerApi::layerRenamed, this, [this](const QString&, const QString&) { rebuildTree(); });
    connect(la, &LayerApi::layersReset, this, [this]() { rebuildTree(); });
    connect(la, &LayerApi::shapeAssigned, this, [this](const QString&, const QString&) { rebuildTree(); });
    connect(la, &LayerApi::layerEditableChanged, this, [this](const QString&, bool) { rebuildTree(); });
    connect(ShapeApi::instance(), &ShapeApi::shapeAdded, this, [this](const QString&) { rebuildTree(); });
    connect(ShapeApi::instance(), &ShapeApi::shapeDeleted, this, [this](const QString&) { rebuildTree(); });
    connect(ShapeApi::instance(), &ShapeApi::shapeUpdated, this, [this](const QString&) { rebuildTree(); });
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &ApiLayerPanel::onSelectionChanged);
    connect(ShapeApi::instance(), &ShapeApi::allShapesDeleted,
            this, [this]() { rebuildTree(); });
    rebuildTree();
}

ApiLayerPanel::~ApiLayerPanel() = default;

void ApiLayerPanel::setupUi()
{
    QWidget* w = new QWidget(this);
    QVBoxLayout* lay = new QVBoxLayout(w);
    lay->setContentsMargins(4, 4, 4, 4);
    setStyleSheet(R"(
        QDockWidget {
            background-color: #0F2636;
            color: white;
        }
        QDockWidget::title {
            background-color: #0F2636;
            color: white;
            padding: 6px;
            border-bottom: 1px solid #27446d;
            font-weight: bold;
        }
        QWidget {
            background-color: #0F2636;
            color: white;
        }
    )");

    // ★ PEHLE create karo
    m_tree = new QTreeWidget(w);

    // ★ PHIR configure
    m_tree->setHeaderHidden(true);
    m_tree->setColumnCount(2);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setColumnWidth(1, 32);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);

    m_tree->setStyleSheet(R"(
        QTreeWidget {
            background-color: #0F2636;
            color: white;
            border: 2px solid #27446d;
            outline: none;
            show-decoration-selected: 1;
            font-size: 13px;
        }
        QTreeWidget::item {
            background-color: #0F2636;
            height: 26px;
            padding: 4px 2px;
            border: none;
            color: white;
        }
        QTreeWidget::item:selected {
            background-color: #0078D4;
            color: white;
            border-radius: 2px;
        }
        QTreeWidget::item:hover {
            background-color: #1A3652;
            border-radius: 2px;
        }
        QScrollBar:vertical {
            background-color: #0F2636;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: #3A506B;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4A607B;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, &ApiLayerPanel::onContextMenu);
    connect(m_tree, &QTreeWidget::itemClicked,
            this, &ApiLayerPanel::onItemClicked);
    connect(m_tree, &QTreeWidget::itemChanged,
            this, &ApiLayerPanel::onItemChanged);

    lay->addWidget(m_tree);
    setWidget(w);
}
void ApiLayerPanel::rebuildTree()
{
    if (!m_tree)
        return;
    const bool blocked = m_tree->blockSignals(true);
    m_tree->clear();
    m_layerItems.clear();
    m_root = new QTreeWidgetItem(m_tree);
    m_root->setText(0, QStringLiteral("Layers"));
    m_root->setFlags(m_root->flags() & ~Qt::ItemIsSelectable);
    m_root->setExpanded(true);
    LayerApi* la = LayerApi::instance();
    if (la) {
        for (const QString& lid : la->getLayers())
            addLayerItem(lid);
    }
    m_tree->blockSignals(blocked);
}

void ApiLayerPanel::addLayerItem(const QString& layerId)
{
    LayerApi* la = LayerApi::instance();
    QString name = la->getLayerName(layerId);
    bool visible = la->isVisible(layerId);
    const bool editable = la->isEditable(layerId);
    QTreeWidgetItem* item = new QTreeWidgetItem(m_root);

    // Column 0: name only
    item->setText(0, name);
    // Column 1: lock icon (far right)
    item->setText(1, editable ? QStringLiteral("🔓") : QStringLiteral("🔒"));
    item->setTextAlignment(1, Qt::AlignCenter);
    item->setToolTip(1, editable
                            ? QStringLiteral("Editable — click to lock")
                            : QStringLiteral("Locked — click to unlock"));

    item->setData(0, Qt::UserRole, layerId);
    item->setData(0, Qt::UserRole + 1, QStringLiteral("layer"));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    item->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    m_layerItems[layerId] = item;

    if (editable) {
        for (const QString& sid : la->getShapesOfLayer(layerId))
            addShapeItem(item, sid);
        if (item->childCount() > 0)
            item->setExpanded(true);
    }
}
void ApiLayerPanel::addShapeItem(QTreeWidgetItem* parent, const QString& shapeId)
{
    QJsonObject props = ShapeApi::instance()->getShapeProperties(shapeId);
    QString type = props.value(QStringLiteral("type")).toString();
    if (type.compare(QLatin1String("bitmap"), Qt::CaseInsensitive) == 0
        || type.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) == 0)
        return;
    QString text = props.value(QStringLiteral("text")).toString();
    QString label;
    if (!text.isEmpty())
        label = (type == QLatin1String("text")) ? (QStringLiteral("Text: ") + text) : text;
    else
        label = type;
    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, QStringLiteral(" - ") + label);
    item->setData(0, Qt::UserRole, shapeId);
    item->setData(0, Qt::UserRole + 1, QStringLiteral("shape"));
}

void ApiLayerPanel::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_tree->itemAt(pos);

    QList<QTreeWidgetItem*> selectedShapeItems;
    for (QTreeWidgetItem* sel : m_tree->selectedItems()) {
        if (sel->data(0, Qt::UserRole + 1).toString() == QLatin1String("shape"))
            selectedShapeItems.append(sel);
    }

    if (item && !m_tree->selectedItems().contains(item)) {
        m_tree->clearSelection();
        m_tree->setCurrentItem(item);
        item->setSelected(true);
        selectedShapeItems.clear();
        if (item->data(0, Qt::UserRole + 1).toString() == QLatin1String("shape"))
            selectedShapeItems.append(item);
    }

    if (selectedShapeItems.size() > 1) {
        QMenu menu(this);
        menu.setStyleSheet(R"(
            QMenu {
                background-color: #1A3652;
                color: white;
                border: 1px solid #27446d;
                border-radius: 3px;
                padding: 4px 0px;
                font-size: 13px;
            }
            QMenu::item {
                background-color: transparent;
                color: white;
                padding: 8px 28px 8px 20px;
                margin: 2px 4px;
                border-radius: 2px;
            }
            QMenu::item:selected {
                background-color: #0078D4;
                color: white;
            }
        )");
        QAction* removeAction = menu.addAction(
            QStringLiteral("Remove %1 Shapes").arg(selectedShapeItems.size()));
        QAction* selected = menu.exec(m_tree->mapToGlobal(pos));
        if (selected == removeAction)
            deleteSelectedItems();
        return;
    }

    if (item)
        m_tree->setCurrentItem(item);
    const QString kind = item ? item->data(0, Qt::UserRole + 1).toString() : QString();
    const QString id   = item ? item->data(0, Qt::UserRole).toString() : QString();
    UiActionContext ctx;
    ctx.parentWidget = this;
    if (kind == QLatin1String("layer")) {
        ctx.layerId = id;
    } else if (kind == QLatin1String("shape")) {
        ctx.shapeId = id;
        if (item && item->parent()) {
            const QString parentKind = item->parent()->data(0, Qt::UserRole + 1).toString();
            if (parentKind == QLatin1String("layer"))
                ctx.layerId = item->parent()->data(0, Qt::UserRole).toString();
        }
    }

    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background-color: #1A3652;
            color: white;
            border: 1px solid #27446d;
            border-radius: 3px;
            padding: 4px 0px;
            font-size: 13px;
        }
        QMenu::item {
            background-color: transparent;
            color: white;
            padding: 8px 28px 8px 20px;
            margin: 2px 4px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #0078D4;
            color: white;
        }
        QMenu::item:disabled {
            color: #808080;
        }
        QMenu::separator {
            height: 1px;
            background-color: #27446d;
            margin: 4px 10px;
        }
    )");

    UiActionApi* ui = UiActionApi::instance();
    const QVector<UiActionDescriptor> actions =
        ui->actionsFor(UiActionCategory::ContextLayer);
    QString lastGroup;
    bool anyAdded = false;
    for (const UiActionDescriptor& d : actions) {
        if (d.isEnabled && !d.isEnabled(ctx))
            continue;
        if (anyAdded && d.group != lastGroup
            && (!d.group.isEmpty() || !lastGroup.isEmpty())) {
            menu.addSeparator();
        }
        QAction* a = menu.addAction(d.label);
        a->setData(d.id);
        if (d.isEnabled)
            a->setEnabled(d.isEnabled(ctx));
        lastGroup = d.group;
        anyAdded = true;
    }

    // Editable / Lock toggle — ONLY inside this function
    QAction* editableToggleAct = nullptr;
    if (kind == QLatin1String("layer") && !id.isEmpty()) {
        if (anyAdded)
            menu.addSeparator();
        const bool isEd = LayerApi::instance()->isEditable(id);
        editableToggleAct = menu.addAction(
            isEd ? QStringLiteral("🔒 Lock Layer (make non-editable)")
                 : QStringLiteral("🔓 Make Editable"));
        anyAdded = true;
    }

    if (!anyAdded)
        return;

    QAction* selected = menu.exec(m_tree->mapToGlobal(pos));
    if (!selected)
        return;

    if (editableToggleAct && selected == editableToggleAct) {
        const bool currentlyEditable = LayerApi::instance()->isEditable(id);
        LayerApi::instance()->setEditable(id, !currentlyEditable);
        return;
    }

    ui->trigger(selected->data().toString(), ctx);
}

void ApiLayerPanel::onItemClicked(QTreeWidgetItem* item, int column)
{
    if (!item)
        return;

    const QString kind = item->data(0, Qt::UserRole + 1).toString();
    const QString id   = item->data(0, Qt::UserRole).toString();

    if (kind == QLatin1String("layer") && !id.isEmpty()) {
        if (column == 1) {
            // ONLY lock icon column → toggle
            const bool currentlyEditable = LayerApi::instance()->isEditable(id);
            LayerApi::instance()->setEditable(id, !currentlyEditable);
            return;
        }
        // column 0 → normal select / active layer (NO lock toggle)
        LayerApi::instance()->setActiveLayer(id);
        const QStringList shapes = LayerApi::instance()->getShapesOfLayer(id);
        for (const QString& sid : shapes) {
            const QString t = ShapeApi::instance()->getShapeType(sid);
            if (t.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) == 0) {
                emit shapeClicked(sid);
                return;
            }
        }
        return;
    }

    if (kind == QLatin1String("shape")) {
        if (m_tree->selectedItems().size() != 1 || !item->isSelected())
            return;
        emit shapeClicked(id);
    }
}
void ApiLayerPanel::refresh()
{
    rebuildTree();
}

void ApiLayerPanel::selectShape(const QString& shapeId)
{
    if (!m_tree)
        return;

    const bool blocked = m_tree->blockSignals(true);
    m_tree->clearSelection();

    if (shapeId.isEmpty()) {
        m_tree->blockSignals(blocked);
        return;
    }

    {
        const QString type = ShapeApi::instance()->getShapeType(shapeId);
        if (type.compare(QLatin1String("bitmap"), Qt::CaseInsensitive) == 0) {
            m_tree->blockSignals(blocked);
            return;
        }
    }

    for (auto it = m_layerItems.begin(); it != m_layerItems.end(); ++it) {
        QTreeWidgetItem* layerItem = it.value();
        if (!layerItem)
            continue;
        for (int i = 0; i < layerItem->childCount(); ++i) {
            QTreeWidgetItem* child = layerItem->child(i);
            if (child && child->data(0, Qt::UserRole).toString() == shapeId) {
                layerItem->setExpanded(true);
                m_tree->setCurrentItem(child);
                child->setSelected(true);
                m_tree->scrollToItem(child);
                m_tree->blockSignals(blocked);
                return;
            }
        }
    }

    const QString type = ShapeApi::instance()->getShapeType(shapeId);
    if (type.compare(QLatin1String("raster_image"), Qt::CaseInsensitive) == 0) {
        const QString layerId = LayerApi::instance()->getLayerOfShape(shapeId);
        if (!layerId.isEmpty() && m_layerItems.contains(layerId)) {
            QTreeWidgetItem* layerItem = m_layerItems.value(layerId);
            if (layerItem) {
                m_tree->setCurrentItem(layerItem);
                layerItem->setSelected(true);
                m_tree->scrollToItem(layerItem);
            }
        }
    }
    m_tree->blockSignals(blocked);
}

void ApiLayerPanel::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (!item || column != 0)
        return;
    if (item->data(0, Qt::UserRole + 1).toString() != QLatin1String("layer"))
        return;
    const QString layerId = item->data(0, Qt::UserRole).toString();
    if (layerId.isEmpty())
        return;
    const bool visible = (item->checkState(0) == Qt::Checked);
    LayerApi* la = LayerApi::instance();
    if (la->isVisible(layerId) != visible)
        la->setVisibility(layerId, visible);
}

bool ApiLayerPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_tree && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
            deleteSelectedItems();
            return true;
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void ApiLayerPanel::deleteSelectedItems()
{
    QList<QTreeWidgetItem*> selected = m_tree->selectedItems();
    if (selected.isEmpty())
        return;

    struct PendingDelete {
        QString id;
        QString layerId;
    };
    QVector<PendingDelete> shapesToDelete;
    QSet<QString> selectedLayerIds;

    for (QTreeWidgetItem* it : selected) {
        const QString kind = it->data(0, Qt::UserRole + 1).toString();
        const QString id   = it->data(0, Qt::UserRole).toString();
        if (kind == QLatin1String("layer")) {
            selectedLayerIds.insert(id);
        } else if (kind == QLatin1String("shape")) {
            QString layerId;
            if (it->parent() && it->parent()->data(0, Qt::UserRole + 1).toString() == QLatin1String("layer"))
                layerId = it->parent()->data(0, Qt::UserRole).toString();
            shapesToDelete.append({id, layerId});
        }
    }

    QVector<PendingDelete> standaloneShapes;
    for (const PendingDelete& pd : shapesToDelete) {
        if (!pd.layerId.isEmpty() && selectedLayerIds.contains(pd.layerId))
            continue;
        standaloneShapes.append(pd);
    }

    const int totalCount = standaloneShapes.size() + selectedLayerIds.size();
    if (totalCount == 0)
        return;

    const QString msg = (selectedLayerIds.isEmpty())
                            ? QStringLiteral("Are you sure you want to delete %1 selected shape(s)?").arg(standaloneShapes.size())
                            : QStringLiteral("Are you sure you want to delete the selected items (%1 shape(s), %2 layer(s))?")
                                  .arg(standaloneShapes.size()).arg(selectedLayerIds.size());

    if (QMessageBox::question(this, QStringLiteral("Delete Selected"), msg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    for (const PendingDelete& pd : standaloneShapes) {
        ShapeHistoryApi::instance()->clear(pd.id);
        ShapeApi::instance()->deleteShape(pd.id);
    }

    for (const QString& layerId : selectedLayerIds) {
        LayerApi::instance()->deleteLayer(layerId);
    }
}

void ApiLayerPanel::onSelectionChanged()
{
    QStringList shapeIds;
    for (QTreeWidgetItem* it : m_tree->selectedItems()) {
        if (it->data(0, Qt::UserRole + 1).toString() == QLatin1String("shape"))
            shapeIds.append(it->data(0, Qt::UserRole).toString());
    }
    emit shapesSelectionChanged(shapeIds);
}
