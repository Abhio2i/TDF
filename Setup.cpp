#include "Setup.h"
#include <QDebug>
#include <QStandardPaths>

TDFManager* TDFManager::s_instance = nullptr;

TDFManager::TDFManager()
    : m_initialized(false)
{
    QString homeDir = QDir::homePath();
    m_tdfPath = homeDir + "/TDF";
    m_dbFolderPath = m_tdfPath + "/Data";
    m_cityDataFolderPath = m_tdfPath + "/CityData";
}

TDFManager* TDFManager::instance()
{
    if (!s_instance) {
        s_instance = new TDFManager();
    }
    return s_instance;
}

void TDFManager::initializeTDFStructure()
{
    if (m_initialized) {
        return;
    }

    // Create folder structure
    createFolderStructure();

    // Copy Aircraft.db if it doesn't exist in TDF/DB
    QString aircraftDbDest = m_dbFolderPath + "/Aircraft.db";
    copyResourceIfNeeded(":/DB/DB/Aircraft.db", aircraftDbDest);

    // Copy city.json if it doesn't exist in TDF/CityData
    QString cityJsonDest = m_cityDataFolderPath + "/city.json";
    copyResourceIfNeeded(":/jsonData/DB/jsonData/city.json", cityJsonDest);
    m_initialized = true;

}

void TDFManager::createFolderStructure()
{
    QDir dir;

    // Create TDF folder
    if (!dir.exists(m_tdfPath)) {
        if (dir.mkpath(m_tdfPath)) {

        } else {
            qWarning() << "Failed to create TDF folder:" << m_tdfPath;
        }
    }

    // Create DB subfolder
    if (!dir.exists(m_dbFolderPath)) {
        if (dir.mkpath(m_dbFolderPath)) {

        } else {
            qWarning() << "Failed to create DB folder:" << m_dbFolderPath;
        }
    }

    // Create CityData subfolder
    if (!dir.exists(m_cityDataFolderPath)) {
        if (dir.mkpath(m_cityDataFolderPath)) {

        } else {
            qWarning() << "Failed to create CityData folder:" << m_cityDataFolderPath;
        }
    }
}

void TDFManager::copyResourceIfNeeded(const QString& resourcePath, const QString& destPath)
{
    // Check if file already exists in destination
    if (fileExists(destPath)) {

        return;
    }

    // Copy from resource to destination
    QFile resourceFile(resourcePath);
    if (!resourceFile.exists()) {
        qWarning() << "Resource file not found:" << resourcePath;
        return;
    }

    if (resourceFile.copy(destPath)) {


        // Set file permissions to read/write
        QFile::setPermissions(destPath, QFile::ReadOwner | QFile::WriteOwner |
                                            QFile::ReadGroup | QFile::ReadOther);
    } else {
        qWarning() << "Failed to copy resource:" << resourcePath
                   << "Error:" << resourceFile.errorString();
    }
}

bool TDFManager::fileExists(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile();
}

QString TDFManager::getAircraftDbPath()
{
    QString tdfPath = m_dbFolderPath + "/Aircraft.db";

    // If file exists in TDF, return that path
    if (fileExists(tdfPath)) {
        return tdfPath;
    }

    // Otherwise return resource path
    return ":/DB/DB/Aircraft.db";
}

QString TDFManager::getCityJsonPath()
{
    QString tdfPath = m_cityDataFolderPath + "/city.json";

    // If file exists in TDF, return that path
    if (fileExists(tdfPath)) {
        return tdfPath;
    }
    // Otherwise return resource path
    return ":/jsonData/DB/jsonData/city.json";
}


