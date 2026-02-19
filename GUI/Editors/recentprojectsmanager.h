/* ========================================================================= */
/* File: recentprojectsmanager.h                                             */
/* Purpose: Manages recent projects list for various editor types            */
// Written by   : Arti Rajpoot
/* ========================================================================= */

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
