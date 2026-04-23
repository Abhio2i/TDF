/* =============================================================================
 * FILE:         Setup.h
 * MODULE:       TDF Folder & Resource Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen to Innovation Pvt. Ltd.
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the TDFManager class which manages the TDF (Tool Data
 *               Folder) folder structure and resource paths for aircraft and
 *               city data. Provides singleton access, initialises the TDF
 *               directory structure, copies required resource files (Aircraft.db,
 *               city.json) from application resources to the TDF if they do
 *               not already exist, and returns the appropriate file paths.
 *
 * REQUIREMENTS: REQ-SETUP-010  Singleton TDF manager
 *               REQ-SETUP-011  Initialise TDF folder structure
 *               REQ-SETUP-012  Copy Aircraft.db from resources to TDF if missing
 *               REQ-SETUP-013  Copy city.json from resources to TDF if missing
 *               REQ-SETUP-014  Return path to Aircraft.db (TDF or resource)
 *               REQ-SETUP-015  Return path to city.json (TDF or resource)
 *               REQ-SETUP-016  Check if a file exists
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SETUP-001
 *
 *
 * COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef SETUP_H
#define SETUP_H

#include <QString>
#include <QDir>
#include <QFile>

class TDFManager {
public:
    static TDFManager* instance();

    // Initialize TDF folder structure and copy resources if needed
    void initializeTDFStructure();

    // Get path to Aircraft.db (from TDF if exists, otherwise resource)
    QString getAircraftDbPath();

    // Get path to city.json (from TDF if exists, otherwise resource)
    QString getCityJsonPath();

private:
    TDFManager();
    static TDFManager* s_instance;
    QString m_tdfPath;
    QString m_dbFolderPath;
    QString m_cityDataFolderPath;
    bool m_initialized;
    // Helper methods
    void createFolderStructure();
    void copyResourceIfNeeded(const QString& resourcePath, const QString& destPath);
    bool fileExists(const QString& path);
};

#endif // SETUP_H
