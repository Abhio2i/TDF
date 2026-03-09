
#include "scenarioconfig.h"
#include <QDebug>
#include <QCoreApplication>

ScenarioConfig::ScenarioConfig(QObject *parent)
    : QObject(parent),
    savedFPS(60),
    savedGUIFPS(60),
    savedSimulationFPS(60),
    savedPhysicsFPS(60),
    savedImageSize("100px"),
    savedDatabaseEnabled(false),
    savedDatabasePath(QString())
{

    // Use organization and application name
    QString orgName = QCoreApplication::organizationName();
    QString appName = QCoreApplication::applicationName();

    if (orgName.isEmpty()) orgName = "YourCompany";
    if (appName.isEmpty()) appName = "YourApplication";

    settings = new QSettings(orgName, appName, this);

    // Load saved settings
    loadAppSettings();
     loadDatabaseSettings();
}

ScenarioConfig::~ScenarioConfig()
{
    // Save settings before closing
    saveAppSettings(savedFPS, savedGUIFPS, savedSimulationFPS, savedPhysicsFPS, savedImageSize);
}

// Existing method - 2 parameters (for backward compatibility)
void ScenarioConfig::saveAppSettings(int fps, const QString &imageSize)
{
    if (fps >= 1 && fps <= 1000) {
        settings->setValue("appSettings/fps", fps);
        savedFPS = fps;
    }

    if (!imageSize.isEmpty()) {
        settings->setValue("appSettings/imageSize", imageSize);
        savedImageSize = imageSize;
    }

    // Force save to disk
    settings->sync();


}


void ScenarioConfig::saveAppSettings(int fps, int guifps, int simfps, int physicsfps, const QString &imageSize)
{
    // Save Main FPS
    if (fps >= 1 && fps <= 1000) {
        settings->setValue("appSettings/fps", fps);
        savedFPS = fps;
    }

    // Save GUI FPS
    if (guifps >= 1 && guifps <= 1000) {
        settings->setValue("appSettings/guifps", guifps);
        savedGUIFPS = guifps;
    }

    // Save Simulation FPS
    if (simfps >= 1 && simfps <= 1000) {
        settings->setValue("appSettings/simulationfps", simfps);
        savedSimulationFPS = simfps;
    }

    // Save Physics FPS
    if (physicsfps >= 1 && physicsfps <= 1000) {
        settings->setValue("appSettings/physicsfps", physicsfps);
        savedPhysicsFPS = physicsfps;
    }

    // Save Image Size
    if (!imageSize.isEmpty()) {
        settings->setValue("appSettings/imageSize", imageSize);
        savedImageSize = imageSize;
    }

    // Force save to disk
    settings->sync();

    qDebug() << "💾 All settings saved:";
    qDebug() << "  Main FPS:" << fps;
    qDebug() << "  GUI FPS:" << guifps;
    qDebug() << "  Simulation FPS:" << simfps;
    qDebug() << "  Physics FPS:" << physicsfps;
    qDebug() << "  Image Size:" << imageSize;
}

// Load application settings
void ScenarioConfig::loadAppSettings()
{
    // Load Main FPS with default value 60
    savedFPS = settings->value("appSettings/fps", 60).toInt();

    // Load GUI FPS with default value 60
    savedGUIFPS = settings->value("appSettings/guifps", 60).toInt();

    // Load Simulation FPS with default value 60
    savedSimulationFPS = settings->value("appSettings/simulationfps", 60).toInt();

    // Load Physics FPS with default value 60
    savedPhysicsFPS = settings->value("appSettings/physicsfps", 60).toInt();

    // Load Image Size with default value "100px"
    savedImageSize = settings->value("appSettings/imageSize", "100px").toString();

    qDebug() << "Application settings loaded:";
    qDebug() << "  Main FPS:" << savedFPS;
    qDebug() << "  GUI FPS:" << savedGUIFPS;
    qDebug() << "  Simulation FPS:" << savedSimulationFPS;
    qDebug() << "  Physics FPS:" << savedPhysicsFPS;
    qDebug() << "  Image Size:" << savedImageSize;
}

// Get saved FPS
int ScenarioConfig::getSavedFPS() const
{
    return savedFPS;
}

// Get saved GUI FPS
int ScenarioConfig::getSavedGUIFPS() const
{
    return savedGUIFPS;
}

// Get saved Simulation FPS
int ScenarioConfig::getSavedSimulationFPS() const
{
    return savedSimulationFPS;
}

// Get saved Physics FPS
int ScenarioConfig::getSavedPhysicsFPS() const
{
    return savedPhysicsFPS;
}

// Get saved Image Size
QString ScenarioConfig::getSavedImageSize() const
{
    return savedImageSize;
}

// Add file to recent projects list
void ScenarioConfig::addToRecentProjects(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    QStringList recentProjects = settings->value("recentProjects").toStringList();

    // Remove if already exists
    recentProjects.removeAll(filePath);

    // Add to beginning
    recentProjects.prepend(filePath);

    // Keep only last 10 projects
    while (recentProjects.size() > 10) {
        recentProjects.removeLast();
    }

    settings->setValue("recentProjects", recentProjects);

    // Update last opened project
    lastOpenedProject = filePath;
    settings->setValue("lastOpenedProject", filePath);

    qDebug() << "✅ Added to recent projects:" << filePath;
}

// Get recent projects list
QStringList ScenarioConfig::getRecentProjects() const
{
    return settings->value("recentProjects").toStringList();
}

// Clear recent projects list
void ScenarioConfig::clearRecentProjects()
{
    settings->remove("recentProjects");
    qDebug() << "Recent projects list cleared";
}

// Optional: JSON methods (if needed)
void ScenarioConfig::toJson()
{
    // Implement if needed
}

void ScenarioConfig::fromJson()
{
    // Implement if needed
}
// Save tooltip field selections
void ScenarioConfig::saveTooltipFields(const QSet<QString>& fields)
{
    QStringList fieldList = fields.values();
    settings->setValue("tooltipSettings/selectedFields", fieldList);
    settings->sync();

    qDebug() << "💾 Tooltip fields saved:" << fieldList;
}

// Load tooltip field selections
QSet<QString> ScenarioConfig::loadTooltipFields() const
{
    QStringList fieldList = settings->value("tooltipSettings/selectedFields").toStringList();

    // Default fields if nothing saved
    if (fieldList.isEmpty()) {
        fieldList = QStringList{"Name", "Speed", "Altitude", "Latitude", "Longitude"};
    }

    QSet<QString> fields;
    for (const QString& field : fieldList) {
        fields.insert(field);
    }

    qDebug() << "📖 Tooltip fields loaded:" << fieldList;
    return fields;
}
void ScenarioConfig::saveDatabaseSettings(bool enabled, const QString& path)
{
    settings->setValue("databaseSettings/enabled", enabled);
    settings->setValue("databaseSettings/path",    path);
    savedDatabaseEnabled = enabled;
    savedDatabasePath    = path;
    settings->sync();

}

void ScenarioConfig::loadDatabaseSettings()
{
    savedDatabaseEnabled = settings->value("databaseSettings/enabled", false).toBool();
    savedDatabasePath    = settings->value("databaseSettings/path",    QString()).toString();

}

bool ScenarioConfig::getSavedDatabaseEnabled() const
{
    return savedDatabaseEnabled;
}

QString ScenarioConfig::getSavedDatabasePath() const
{
    return savedDatabasePath;
}
