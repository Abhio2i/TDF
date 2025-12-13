
#include "recentprojectsmanager.h"
#include "qdebug.h"
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileInfo>
#include <QCursor>

RecentProjectsManager* RecentProjectsManager::m_instance = nullptr;

RecentProjectsManager::RecentProjectsManager(QObject *parent)
    : QObject(parent)
{
}

RecentProjectsManager* RecentProjectsManager::instance()
{
    if (!m_instance) {
        m_instance = new RecentProjectsManager();
    }
    return m_instance;
}

QString RecentProjectsManager::getSettingsKey(EditorType editorType) const
{
    switch(editorType) {
    case ScenarioEditor:
        return "recentProjects/ScenarioEditor";
    case RuntimeEditor:
        return "recentProjects/RuntimeEditor";
    case DatabaseEditor:
        return "recentProjects/DatabaseEditor";
    default:
        return "recentProjects/Default";
    }
}

void RecentProjectsManager::addToRecentProjects(const QString &filePath, EditorType editorType)
{
    if (filePath.isEmpty()) return;

    QSettings settings;
    QString key = getSettingsKey(editorType);
    QStringList recentProjects = settings.value(key).toStringList();

    // Remove if already exists
    recentProjects.removeAll(filePath);
    // Add to beginning
    recentProjects.prepend(filePath);
    // Keep only last 10 projects
    while (recentProjects.size() > 10) {
        recentProjects.removeLast();
    }

    settings.setValue(key, recentProjects);
    qDebug() << "✅ Added to" << key << "recent projects:" << filePath;
}

QStringList RecentProjectsManager::getRecentProjects(EditorType editorType) const
{
    QSettings settings;
    QString key = getSettingsKey(editorType);
    return settings.value(key).toStringList();
}

void RecentProjectsManager::clearRecentProjects(EditorType editorType)
{
    QSettings settings;
    QString key = getSettingsKey(editorType);
    settings.remove(key);
    qDebug() << "🗑️ Recent projects list cleared for" << key;
}

void RecentProjectsManager::showRecentProjectsMenu(QWidget *parent, EditorType editorType)
{
    qDebug() << "Recent Project menu clicked for editor type:" << editorType;

    // Get editor-specific recent projects
    QStringList recentProjects = getRecentProjects(editorType);

    // Filter only existing files
    QStringList existingProjects;
    for (const QString& projectPath : recentProjects) {
        if (QFile::exists(projectPath)) {
            existingProjects << projectPath;
        }
    }

    // Update list with only existing projects
    if (existingProjects.size() < recentProjects.size()) {
        for (const QString& projectPath : existingProjects) {
            addToRecentProjects(projectPath, editorType);
        }
    }

    if (existingProjects.isEmpty()) {
        QString editorName;
        switch(editorType) {
        case ScenarioEditor: editorName = "Scenario"; break;
        case RuntimeEditor: editorName = "Runtime"; break;
        case DatabaseEditor: editorName = "Database"; break;
        }

        QMessageBox::information(parent, "Recent Projects",
                                 QString("📂 No recent %1 projects found!\n\n"
                                         "To see projects here, save or open a %1 project first.")
                                     .arg(editorName));
        return;
    }

    // Create recent projects menu
    QMenu recentMenu(parent);

    // Add header with editor name
    QString editorName;
    switch(editorType) {
    case ScenarioEditor: editorName = "Scenario"; break;
    case RuntimeEditor: editorName = "Runtime"; break;
    case DatabaseEditor: editorName = "Database"; break;
    }

    QAction* headerAction = recentMenu.addAction(QString("📋 Recent %1 Projects").arg(editorName));
    headerAction->setEnabled(false);
    recentMenu.addSeparator();

    // Add recent projects to menu
    for (const QString& projectPath : existingProjects) {
        QFileInfo fileInfo(projectPath);
        QString displayText = QString("📄 %1\n 📍 %2")
                                  .arg(fileInfo.fileName())
                                  .arg(fileInfo.path());
        QAction* projectAction = recentMenu.addAction(displayText);
        projectAction->setData(projectPath);
        projectAction->setToolTip(projectPath);
    }

    recentMenu.addSeparator();

    // Connect clear action
    QAction* clearAction = recentMenu.addAction("🗑️ Clear All Recent Projects");

    // Show menu at cursor position
    QPoint menuPos = QCursor::pos();
    QAction* selectedAction = recentMenu.exec(menuPos);

    if (selectedAction) {
        if (selectedAction == clearAction) {
            clearRecentProjects(editorType);
        } else if (selectedAction->data().isValid()) {
            QString filePath = selectedAction->data().toString();
            emit projectSelected(filePath, editorType);
        }
    }
}
