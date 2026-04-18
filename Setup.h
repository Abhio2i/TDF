/* ========================================================================= */
/* File: Setup.h                                                             */
/* Purpose: Manages TDF folder structure and resource paths for aircraft     */
/*          and city data                                                    */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
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
