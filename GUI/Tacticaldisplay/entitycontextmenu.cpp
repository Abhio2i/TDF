// entitycontextmenu.cpp
#include "entitycontextmenu.h"
#include "canvaswidget.h"
#include "core/Hierarchy/entity.h"
#include <QDebug>

EntityContextMenu::EntityContextMenu(CanvasWidget* parent)
    : QMenu(parent)
    , m_parent(parent)
    , m_currentEntityId("")
    , m_currentEntry(nullptr)
{
    // Style sheet for menu
    setStyleSheet(
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

    // Create actions
    m_removeAction = new QAction("🗑️ Remove", this);
    m_renameAction = new QAction("✏️ Rename", this);
    m_editTrajectoryAction = new QAction("🛤️ Edit Trajectory", this);
    m_propertiesAction = new QAction("📋 Properties", this);

    // Connect actions to slots
    connect(m_removeAction, &QAction::triggered, this, &EntityContextMenu::onRemoveTriggered);
    connect(m_renameAction, &QAction::triggered, this, &EntityContextMenu::onRenameTriggered);
    connect(m_editTrajectoryAction, &QAction::triggered, this, &EntityContextMenu::onEditTrajectoryTriggered);
    connect(m_propertiesAction, &QAction::triggered, this, &EntityContextMenu::onPropertiesTriggered);
}

void EntityContextMenu::setupMenu(const QString& entityId, MeshEntry& entry)
{
    m_currentEntityId = entityId;
    m_currentEntry = &entry;

    // Clear existing items
    clear();

    // Add actions based on context
    addAction(m_removeAction);
    addAction(m_renameAction);
    addAction(m_editTrajectoryAction);
    addSeparator();

    // Add properties action only if not in Scenario Editor
    if (m_parent && !m_parent->window()->windowTitle().contains("Scenario Editor", Qt::CaseInsensitive)) {
        addAction(m_propertiesAction);
    }

    // Update action texts with entity name
    QString entityName = entry.name.isEmpty() ? entityId : entry.name;
    m_removeAction->setText(QString("🗑️ Remove '%1'").arg(entityName));
    m_renameAction->setText(QString("✏️ Rename '%1'").arg(entityName));
    m_editTrajectoryAction->setText(QString("🛤️ Edit Trajectory for '%1'").arg(entityName));
    m_propertiesAction->setText(QString("📋 Properties of '%1'").arg(entityName));
}

void EntityContextMenu::onRemoveTriggered()
{
    if (m_currentEntityId.isEmpty() || !m_parent) {
        return;
    }

    QString entityName = m_currentEntry && !m_currentEntry->name.isEmpty()
                             ? m_currentEntry->name
                             : m_currentEntityId;

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_parent,
        "Confirm Remove",
        QString("Are you sure you want to remove entity '%1'?").arg(entityName),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        qDebug() << "Remove entity requested:" << m_currentEntityId;
        emit removeEntityRequested(m_currentEntityId);
    }
}

void EntityContextMenu::onRenameTriggered()
{
    if (m_currentEntityId.isEmpty() || !m_currentEntry || !m_parent) {
        return;
    }

    QString currentName = m_currentEntry->name.isEmpty() ? m_currentEntityId : m_currentEntry->name;

    bool ok;
    QString newName = QInputDialog::getText(
        m_parent,
        "Rename Entity",
        QString("Enter new name for '%1':").arg(currentName),
        QLineEdit::Normal,
        currentName,
        &ok
        );

    if (ok && !newName.isEmpty() && newName != currentName) {
        qDebug() << "Rename entity requested:" << m_currentEntityId << "to:" << newName;
        emit renameEntityRequested(m_currentEntityId, newName);
    }
}

void EntityContextMenu::onEditTrajectoryTriggered()
{
    if (m_currentEntityId.isEmpty()) {
        return;
    }

    qDebug() << "Edit trajectory requested for:" << m_currentEntityId;
    emit editTrajectoryRequested(m_currentEntityId);
}

void EntityContextMenu::onPropertiesTriggered()
{
    if (m_currentEntityId.isEmpty()) {
        return;
    }

    qDebug() << "Show properties requested for:" << m_currentEntityId;
    emit showPropertiesRequested(m_currentEntityId);
}
