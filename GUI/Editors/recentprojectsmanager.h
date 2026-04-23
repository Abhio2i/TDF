/* =============================================================================
 * FILE:         recentprojectsmanager.h
 * MODULE:       Recent Projects Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the RecentProjectsManager class, a singleton manager
 *               for handling recent projects across different editor types
 *               (ScenarioEditor, MissionEditor, RuntimeEditor, DatabaseEditor,
 *               LibraryData). Provides persistent storage using QSettings,
 *               adding/clearing/retrieving recent project lists, and displaying
 *               a recent projects menu with projectSelected signal emission.
 *
 * REQUIREMENTS: REQ-RECENT-010  Singleton manager for recent projects
 *               REQ-RECENT-011  Support multiple editor types
 *               REQ-RECENT-012  Persistent storage of recent projects
 *               REQ-RECENT-013  Add file path to recent projects list
 *               REQ-RECENT-014  Retrieve recent projects list
 *               REQ-RECENT-015  Clear recent projects list for an editor type
 *               REQ-RECENT-016  Display recent projects menu
 *               REQ-RECENT-017  Emit projectSelected signal on menu selection
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RECENT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef RECENTPROJECTSMANAGER_H
#define RECENTPROJECTSMANAGER_H

#include <QObject>       // For QObject base class and Qt signal/slot system
#include <QString>       // For string handling
#include <QStringList>   // For list of strings
#include <QSettings>     // For persistent storage of recent projects

// %%% Class Definition %%%
/* Singleton manager class for handling recent projects across different editors */
class RecentProjectsManager : public QObject
{
    Q_OBJECT

public:
    // %%% Editor Types Enumeration %%%
    /* Defines the types of editors that use recent projects */
    enum EditorType {
        ScenarioEditor,   // For scenario editing applications
         MissionEditor,
        RuntimeEditor,    // For runtime editing applications
        DatabaseEditor,   // For database editing applications
         LibraryData
    };

    // %%% Singleton Access Method %%%
    /* Returns the single instance of RecentProjectsManager */
    static RecentProjectsManager* instance();

    // %%% Recent Projects Management Methods %%%
    /* Add a file path to recent projects list for specific editor type */
    void addToRecentProjects(const QString &filePath, EditorType editorType);

    /* Get list of recent projects for specific editor type */
    QStringList getRecentProjects(EditorType editorType) const;

    /* Clear recent projects list for specific editor type */
    void clearRecentProjects(EditorType editorType);

    /* Display recent projects menu in parent widget for specific editor type */
    void showRecentProjectsMenu(QWidget *parent, EditorType editorType);
void showRecentLibraryMenu(QWidget *parent);
     // static void runUnitTestsOnce();
signals:
    // %%% Signals %%%
    /* Emitted when a project is selected from recent projects menu */
    void projectSelected(const QString& filePath, EditorType editorType);

private:
    // %%% Private Members %%%
    /* Private constructor for singleton pattern */
    RecentProjectsManager(QObject *parent = nullptr);

    /* Static instance pointer for singleton pattern */
    static RecentProjectsManager* m_instance;

    /* Get settings key for specific editor type */
    QString getSettingsKey(EditorType editorType) const;
};

#endif /* RECENTPROJECTSMANAGER_H */
