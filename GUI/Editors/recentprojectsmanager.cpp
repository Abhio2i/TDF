/* =============================================================================
 * FILE:         recentprojectsmanager.cpp
 * MODULE:       Recent Projects Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the RecentProjectsManager class, a singleton manager
 *               for handling recent projects across different editor types
 *               (ScenarioEditor, MissionEditor, RuntimeEditor, DatabaseEditor,
 *               LibraryData). Provides persistent storage using QSettings,
 *               adding/clearing/retrieving recent project lists, and displaying
 *               a recent projects menu with projectSelected signal emission.
 *
 * REQUIREMENTS: Implements REQ-RECENT-010 through REQ-RECENT-017
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RECENT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "recentprojectsmanager.h"
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileInfo>
#include <QCursor>
#include "tests/recentprojectsmanagertest/recentprojectsmanager_test.h"


// %%% Static Member Initialization %%%
RecentProjectsManager* RecentProjectsManager::m_instance = nullptr;

// %%% Constructor %%%
RecentProjectsManager::RecentProjectsManager(QObject *parent)
    : QObject(parent)
{


}

// %%% Singleton Instance Accessor %%%
RecentProjectsManager* RecentProjectsManager::instance()
{
    if (!m_instance) {
        m_instance = new RecentProjectsManager();
    }
    return m_instance;
}

// %%% Settings Key Generator %%%
QString RecentProjectsManager::getSettingsKey(EditorType editorType) const
{
    switch(editorType) {
    case ScenarioEditor:
        return "recentProjects/ScenarioEditor";
    case RuntimeEditor:
        return "recentProjects/RuntimeEditor";
    case DatabaseEditor:
        return "recentProjects/DatabaseEditor";
    case MissionEditor:
        return "recentProjects/MissionEditor";
    case LibraryData:
        return "recentProjects/Library";
    default:
        return "recentProjects/Default";
    }
}

// %%% Add Project to Recent List %%%
void RecentProjectsManager::addToRecentProjects(const QString &filePath, EditorType editorType)
{
    if (filePath.isEmpty()) return;

    QSettings settings;
    QString key = getSettingsKey(editorType);
    QStringList recentProjects = settings.value(key).toStringList();

    // Remove duplicate entry if exists
    recentProjects.removeAll(filePath);

    // Add to beginning of list
    recentProjects.prepend(filePath);

    // Limit list to last 10 projects
    if (recentProjects.size() > 10) {
        recentProjects = recentProjects.mid(0, 10);
    }

    settings.setValue(key, recentProjects);
}

// %%% Retrieve Recent Projects List %%%
QStringList RecentProjectsManager::getRecentProjects(EditorType editorType) const
{
    QSettings settings;
    QString key = getSettingsKey(editorType);
    return settings.value(key).toStringList();
}

// %%% Clear Recent Projects List %%%
void RecentProjectsManager::clearRecentProjects(EditorType editorType)
{
    QSettings settings;
    QString key = getSettingsKey(editorType);
    settings.remove(key);
}

// %%% Display Recent Projects Menu %%%
void RecentProjectsManager::showRecentProjectsMenu(QWidget *parent, EditorType editorType)
{
    // Retrieve and validate existing projects
    QStringList recentProjects  = getRecentProjects(editorType);
    QStringList existingProjects;

    for (const QString& projectPath : recentProjects) {
        if (QFile::exists(projectPath)) {
            existingProjects << projectPath;
        }
    }

    // Update stored list with only existing projects
    if (existingProjects.size() < recentProjects.size()) {
        for (const QString& projectPath : existingProjects) {
            addToRecentProjects(projectPath, editorType);
        }
    }

    // Handle empty list case
    if (existingProjects.isEmpty()) {
        QString editorName;
        switch(editorType) {
        case ScenarioEditor: editorName = "Scenario"; break;
        case RuntimeEditor:  editorName = "Runtime";  break;
        case DatabaseEditor: editorName = "Database"; break;
        case MissionEditor:  editorName = "Mission";  break;
        default:             editorName = "Unknown";  break;
        }

        QMessageBox::information(parent, "Recent Projects",
                                 QString("No recent %1 projects found!\n\n"
                                         "To see projects here, save or open a %1 project first.")
                                     .arg(editorName));
        return;
    }

    // Create recent projects menu
    QMenu recentMenu(parent);

    // Add header
    QString editorName;
    switch(editorType) {
    case ScenarioEditor: editorName = "Scenario"; break;
    case RuntimeEditor:  editorName = "Runtime";  break;
    case DatabaseEditor: editorName = "Database"; break;
    case MissionEditor:  editorName = "Mission";  break;
    default:             editorName = "Unknown";  break;
    }

    QAction* headerAction = recentMenu.addAction(QString("Recent %1 Projects").arg(editorName));
    headerAction->setEnabled(false);
    recentMenu.addSeparator();

    for (const QString& projectPath : existingProjects) {
        QFileInfo fileInfo(projectPath);
        QString displayText = QString("%1\n%2")
                                  .arg(fileInfo.fileName())
                                  .arg(fileInfo.path());
        QAction* projectAction = recentMenu.addAction(displayText);
        projectAction->setData(projectPath);
        projectAction->setToolTip(projectPath);
    }

    recentMenu.addSeparator();
    QAction* clearAction = recentMenu.addAction("Clear All Recent Projects");

    // Display menu
    QPoint menuPos = QCursor::pos();
    QAction* selectedAction = recentMenu.exec(menuPos);

    // Handle menu selection
    if (selectedAction) {
        if (selectedAction == clearAction) {
            clearRecentProjects(editorType);
        } else if (selectedAction->data().isValid()) {
            QString filePath = selectedAction->data().toString();
            emit projectSelected(filePath, editorType);
        }
    }
}

// %%% Display Recent Libraries Menu %%%
void RecentProjectsManager::showRecentLibraryMenu(QWidget *parent)
{
    QStringList recentLibraries  = getRecentProjects(LibraryData);
    QStringList existingLibraries;

    // Validate existing files
    for (const QString& libPath : recentLibraries) {
        if (QFile::exists(libPath)) {
            existingLibraries << libPath;
        }
    }

    // Update stored list
    if (existingLibraries.size() < recentLibraries.size()) {
        for (const QString& libPath : existingLibraries) {
            addToRecentProjects(libPath, LibraryData);
        }
    }

    // Handle empty list
    if (existingLibraries.isEmpty()) {
        QMessageBox::information(parent, "Recent Libraries",
                                 "No recent library files found!\n\n"
                                 "To see files here, load a library first.");
        return;
    }

    // Create menu
    QMenu recentMenu(parent);
    QAction* headerAction = recentMenu.addAction("Recent Libraries");
    headerAction->setEnabled(false);
    recentMenu.addSeparator();

    // Add library files
    for (const QString& libPath : existingLibraries) {
        QFileInfo fileInfo(libPath);
        QString displayText = QString("%1\n%2")
                                  .arg(fileInfo.fileName())
                                  .arg(fileInfo.path());
        QAction* libAction = recentMenu.addAction(displayText);
        libAction->setData(libPath);
        libAction->setToolTip(libPath);
    }

    recentMenu.addSeparator();
    QAction* clearAction = recentMenu.addAction("Clear All Recent Libraries");

    // Show menu
    QPoint menuPos = QCursor::pos();
    QAction* selectedAction = recentMenu.exec(menuPos);

    // Handle selection
    if (selectedAction) {
        if (selectedAction == clearAction) {
            clearRecentProjects(LibraryData);
        } else if (selectedAction->data().isValid()) {
            QString filePath = selectedAction->data().toString();
            emit projectSelected(filePath, LibraryData);
        }
    }
}

