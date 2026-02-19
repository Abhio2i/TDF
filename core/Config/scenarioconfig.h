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
    static inline QString software_version = "2.0.92";
    static inline QString file_Version = "";
    void toJson();
    void fromJson();

    // Recent Projects Management
    void addToRecentProjects(const QString &filePath);
    QStringList getRecentProjects() const;
    void clearRecentProjects();

    // Application Settings Management

    void saveAppSettings(int fps, const QString &imageSize);

    // New method with 5 parameters for all settings
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

private:
    QSettings *settings;
    int savedFPS;
    int savedGUIFPS;
    int savedSimulationFPS;
    int savedPhysicsFPS;
    QString savedImageSize;
};

#endif // SCENARIOCONFIG_H
