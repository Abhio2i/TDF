/* =============================================================================
 * FILE:         Setup.cpp
 * MODULE:       TDF Folder & Resource Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen to Innovation Pvt. Ltd.
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the TDFManager class which manages the TDF (Tool Data
 *               Folder) folder structure and resource paths for aircraft and
 *               city data. Provides singleton access, initialises the TDF
 *               directory structure, copies required resource files from
 *               application resources to the TDF if they do not already exist,
 *               and returns the appropriate file paths.
 *
 * REQUIREMENTS: Implements REQ-SETUP-010 through REQ-SETUP-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SETUP-001
 *
 *
 * COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
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
    if (fileExists(destPath)) {

        return;
    }
    QFile resourceFile(resourcePath);
    if (!resourceFile.exists()) {
        qWarning() << "Resource file not found:" << resourcePath;
        return;
    }
    if (resourceFile.copy(destPath)) {
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
    if (fileExists(tdfPath)) {
        return tdfPath;
    }
    return ":/DB/DB/Aircraft.db";
}

QString TDFManager::getCityJsonPath()
{
    QString tdfPath = m_cityDataFolderPath + "/city.json";
    if (fileExists(tdfPath)) {
        return tdfPath;
    }
    return ":/jsonData/DB/jsonData/city.json";
}


