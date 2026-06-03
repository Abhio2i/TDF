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
 *               application resources to the TDF if they do not already exist
 *               or if the bundled version is newer, and returns the appropriate
 *               file paths.
 *
 * REQUIREMENTS: Implements REQ-SETUP-010 through REQ-SETUP-016
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SETUP-001
 *
 * COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "Setup.h"
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

TDFManager* TDFManager::s_instance = nullptr;

TDFManager::TDFManager()
    : m_initialized(false)
{
    QString homeDir = QDir::homePath();
    m_tdfPath       = homeDir + "/TDF";
    m_dbFolderPath  = m_tdfPath + "/Data";
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
    createFolderStructure();
    // Aircraft.db — version check karo, agar bundled newer hai toh overwrite
    QString aircraftDbDest = m_dbFolderPath + "/Aircraft.db";
    copyJsonIfOutdated(":/DB/DB/Aircraft.db", aircraftDbDest);
    // city.json — sirf copy if missing (file_Version nahi hai isme)
    QString cityJsonDest = m_cityDataFolderPath + "/city.json";
    copyResourceIfNeeded(":/jsonData/DB/jsonData/city.json", cityJsonDest);
    m_initialized = true;
}
// -----------------------------------------------------------------------------
// createFolderStructure
// -----------------------------------------------------------------------------
void TDFManager::createFolderStructure()
{
    QDir dir;
    if (!dir.exists(m_tdfPath)) {
        if (!dir.mkpath(m_tdfPath)) {
            qWarning() << "Failed to create TDF folder:" << m_tdfPath;
        }
    }
    if (!dir.exists(m_dbFolderPath)) {
        if (!dir.mkpath(m_dbFolderPath)) {
            qWarning() << "Failed to create Data folder:" << m_dbFolderPath;
        }
    }
    if (!dir.exists(m_cityDataFolderPath)) {
        if (!dir.mkpath(m_cityDataFolderPath)) {
            qWarning() << "Failed to create CityData folder:" << m_cityDataFolderPath;
        }
    }
}

// -----------------------------------------------------------------------------
// copyResourceIfNeeded  (used for non-JSON binary resources e.g. Aircraft.db)
// -----------------------------------------------------------------------------
void TDFManager::copyResourceIfNeeded(const QString& resourcePath,
                                      const QString& destPath)
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
        QFile::setPermissions(destPath,
                              QFile::ReadOwner | QFile::WriteOwner |
                                  QFile::ReadGroup | QFile::ReadOther);
    } else {
        qWarning() << "Failed to copy resource:" << resourcePath
                   << "Error:" << resourceFile.errorString();
    }
}

// -----------------------------------------------------------------------------
// copyJsonIfOutdated
// Copies the bundled JSON to destPath when:
//   (a) destPath does not yet exist, OR
//   (b) the bundled file_Version is strictly greater than the existing one.
// and compares component-by-component (major → minor → patch).
// -----------------------------------------------------------------------------
void TDFManager::copyJsonIfOutdated(const QString& resourcePath,
                                    const QString& destPath)
{
    // -- Read bundled (resource) version --
    QString bundledVersion = readJsonVersion(resourcePath);
    if (bundledVersion.isEmpty()) {
        qWarning() << "Cannot read version from bundled resource:" << resourcePath;
        copyResourceIfNeeded(resourcePath, destPath);
        return;
    }
    if (!fileExists(destPath)) {
        forceCopyResource(resourcePath, destPath);
        return;
    }
    QString existingVersion = readJsonVersion(destPath);
    if (existingVersion.isEmpty()) {
        qWarning() << "Cannot read version from TDF file:" << destPath
                   << "— replacing with bundled copy.";
        forceCopyResource(resourcePath, destPath);
        return;
    }
    if (isVersionNewer(bundledVersion, existingVersion)) {
        forceCopyResource(resourcePath, destPath);
    } else {
        qDebug() << "TDF JSON is up-to-date (version" << existingVersion << "). No update needed.";
    }
}

// -----------------------------------------------------------------------------
// forceCopyResource  — overwrites destPath even if it already exists
// -----------------------------------------------------------------------------
void TDFManager::forceCopyResource(const QString& resourcePath,
                                   const QString& destPath)
{
    QFile resourceFile(resourcePath);
    if (!resourceFile.exists()) {
        qWarning() << "Resource file not found:" << resourcePath;
        return;
    }
    if (fileExists(destPath)) {
        if (!QFile::remove(destPath)) {
            qWarning() << "Failed to remove outdated file:" << destPath;
            return;
        }
    }
    if (resourceFile.copy(destPath)) {
        QFile::setPermissions(destPath,
                              QFile::ReadOwner | QFile::WriteOwner |
                                  QFile::ReadGroup | QFile::ReadOther);
    } else {
        qWarning() << "Failed to copy resource:" << resourcePath
                   << "Error:" << resourceFile.errorString();
    }
}

// -----------------------------------------------------------------------------
// readJsonVersion
// Opens the JSON file at 'path' (resource or filesystem), parses it, and
// returns the value of hierarchy.file_Version as a QString.
// Returns an empty string on any error.
// -----------------------------------------------------------------------------
QString TDFManager::readJsonVersion(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open JSON for version read:" << path;
        return QString();
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in" << path << ":" << parseError.errorString();
        return QString();
    }
    QJsonObject root      = doc.object();
    QJsonObject hierarchy = root.value("hierarchy").toObject();
    QString version       = hierarchy.value("file_Version").toString();
    if (version.isEmpty()) {
        qWarning() << "file_Version key missing or empty in:" << path;
    }
    return version;
}

// -----------------------------------------------------------------------------
// parseVersion
// Non-numeric tokens are treated as 0.
// -----------------------------------------------------------------------------

QVector<int> TDFManager::parseVersion(const QString& versionStr)
{
    QVector<int> parts;
    const QStringList tokens = versionStr.split('.', Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        bool ok = false;
        int val = token.trimmed().toInt(&ok);
        parts.append(ok ? val : 0);
    }
    return parts;
}

// -----------------------------------------------------------------------------
// isVersionNewer
// Returns true if 'candidate' is strictly greater than 'existing'.
// Comparison is major → minor → patch (left-to-right, shorter vector padded
// with zeros).
// -----------------------------------------------------------------------------
bool TDFManager::isVersionNewer(const QString& candidate,
                                const QString& existing)
{
    QVector<int> vCandidate = parseVersion(candidate);
    QVector<int> vExisting  = parseVersion(existing);

    int maxLen = qMax(vCandidate.size(), vExisting.size());
    while (vCandidate.size() < maxLen) vCandidate.append(0);
    while (vExisting.size()  < maxLen) vExisting.append(0);

    for (int i = 0; i < maxLen; ++i) {
        if (vCandidate[i] > vExisting[i]) return true;
        if (vCandidate[i] < vExisting[i]) return false;
    }
    return false;   // equal versions — no update needed
}

// -----------------------------------------------------------------------------
// fileExists
// -----------------------------------------------------------------------------
bool TDFManager::fileExists(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.exists() && fileInfo.isFile();
}

// -----------------------------------------------------------------------------
// getAircraftDbPath
// -----------------------------------------------------------------------------
QString TDFManager::getAircraftDbPath()
{
    QString tdfPath = m_dbFolderPath + "/Aircraft.db";
    if (fileExists(tdfPath)) {
        return tdfPath;
    }
    return ":/DB/DB/Aircraft.db";
}

// -----------------------------------------------------------------------------
// getCityJsonPath
// -----------------------------------------------------------------------------
QString TDFManager::getCityJsonPath()
{
    QString tdfPath = m_cityDataFolderPath + "/city.json";
    if (fileExists(tdfPath)) {
        return tdfPath;
    }
    return ":/jsonData/DB/jsonData/city.json";
}
