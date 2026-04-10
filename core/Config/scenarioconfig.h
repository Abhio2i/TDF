#ifndef SCENARIOCONFIG_H
#define SCENARIOCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

class ScenarioConfig : public QObject
{
    Q_OBJECT

public:
    ScenarioConfig(QObject *parent = nullptr);
    ~ScenarioConfig();
    static inline QString software_version = "3.0.94";
    static inline QString file_Version = "";
    void toJson();
    void fromJson();

    // Recent Projects Management
    void addToRecentProjects(const QString &filePath);
    QStringList getRecentProjects() const;
    void clearRecentProjects();
    void saveAppSettings(int fps, const QString &imageSize);
    void saveAppSettings(int fps, int guifps, int simfps, int physicsfps, const QString &imageSize);
    void loadAppSettings();

    // Getters
    int getSavedFPS() const;
    int getSavedGUIFPS() const;
    int getSavedSimulationFPS() const;
    int getSavedPhysicsFPS() const;
    QString getSavedImageSize() const;

    // Configuration properties
    std::string Name;
    QString lastOpenedProject;
    void saveTooltipFields(const QSet<QString>& fields);
    QSet<QString> loadTooltipFields() const;
    void saveDatabaseSettings(bool enabled, const QString& path);
    void loadDatabaseSettings();
    bool getSavedDatabaseEnabled() const;
    QString getSavedDatabasePath() const;


private:
    QSettings *settings;
    int savedFPS;
    int savedGUIFPS;
    int savedSimulationFPS;
    int savedPhysicsFPS;
    QString savedImageSize;
    bool savedDatabaseEnabled;
    QString savedDatabasePath;
};

#endif // SCENARIOCONFIG_H
