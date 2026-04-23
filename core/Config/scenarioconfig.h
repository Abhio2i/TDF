// =============================================================================
// FILE:        scenarioconfig.h
// MODULE:      Scenario Configuration Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the ScenarioConfig class, which manages application
//              settings, recent projects, FPS limits, database configuration,
//              and tooltip field preferences. Uses QSettings for persistent
//              storage across sessions.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SCENARIOCONFIG_H
#define SCENARIOCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

// =============================================================================
// CLASS: ScenarioConfig
//
// DESCRIPTION: Handles loading and saving of user preferences and scenario
//              configuration. Manages recent projects list, FPS settings for
//              various subsystems (GUI, simulation, physics), database
//              connectivity settings, and tooltip visibility fields.
// =============================================================================
class ScenarioConfig : public QObject
{
    Q_OBJECT

public:
    ScenarioConfig(QObject *parent = nullptr);
    ~ScenarioConfig();

    // =========================================================================
    // SECTION: Version Information
    // DESCRIPTION: Software and file format version strings.
    // =========================================================================
    static inline QString software_version = "4.0.21"; //!< Current software version
    static inline QString file_Version = "";           //!< Loaded file version (if any)

    void toJson();      //!< Serialises configuration to JSON
    void fromJson();    //!< Deserialises configuration from JSON

    // =========================================================================
    // SECTION: Recent Projects Management
    // DESCRIPTION: Maintains list of recently opened project file paths.
    // =========================================================================
    void addToRecentProjects(const QString &filePath);  //!< Adds a path to recent list
    QStringList getRecentProjects() const;              //!< Returns list of recent projects
    void clearRecentProjects();                         //!< Clears the recent projects list

    // =========================================================================
    // SECTION: Application Settings (FPS & Image Size)
    // DESCRIPTION: Save/load frame rate limits and render target size.
    // =========================================================================
    void saveAppSettings(int fps, const QString &imageSize);                       //!< Legacy: saves only main FPS
    void saveAppSettings(int fps, int guifps, int simfps, int physicsfps, const QString &imageSize); //!< Full settings
    void loadAppSettings();                                                       //!< Loads all settings from storage

    // Getters
    int getSavedFPS() const;            //!< Returns main FPS limit
    int getSavedGUIFPS() const;         //!< Returns GUI update FPS limit
    int getSavedSimulationFPS() const;  //!< Returns simulation update FPS limit
    int getSavedPhysicsFPS() const;     //!< Returns physics update FPS limit
    QString getSavedImageSize() const;  //!< Returns image size string (e.g., "1024x768")

    // =========================================================================
    // SECTION: Configuration Properties
    // DESCRIPTION: Current scenario name and last opened project path.
    // =========================================================================
    std::string Name;                   //!< Name of the current scenario
    QString lastOpenedProject;          //!< Path to the most recently opened project

    // =========================================================================
    // SECTION: Tooltip Fields Management
    // DESCRIPTION: Which tooltip fields are currently visible.
    // =========================================================================
    void saveTooltipFields(const QSet<QString>& fields);    //!< Saves set of visible tooltip fields
    QSet<QString> loadTooltipFields() const;                //!< Loads visible tooltip fields

    // =========================================================================
    // SECTION: Database Settings
    // DESCRIPTION: Whether database logging is enabled and the database path.
    // =========================================================================
    void saveDatabaseSettings(bool enabled, const QString& path); //!< Saves DB config
    void loadDatabaseSettings();                                 //!< Loads DB config
    bool getSavedDatabaseEnabled() const;                        //!< Returns DB enabled flag
    QString getSavedDatabasePath() const;                        //!< Returns DB file path

private:
    QSettings *settings;            //!< Persistent settings backend
    int savedFPS;                   //!< Stored main FPS value
    int savedGUIFPS;                //!< Stored GUI FPS value
    int savedSimulationFPS;         //!< Stored simulation FPS value
    int savedPhysicsFPS;            //!< Stored physics FPS value
    QString savedImageSize;         //!< Stored image size string
    bool savedDatabaseEnabled;      //!< Stored database enabled flag
    QString savedDatabasePath;      //!< Stored database file path
};

#endif // SCENARIOCONFIG_H
