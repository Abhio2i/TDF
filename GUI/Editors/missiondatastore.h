/* ========================================================================= */
/* File: missiondatastore.h                                                  */
/* Purpose: Global static store for loaded Mission (.ms) file data           */
/* ========================================================================= */
#ifndef MISSIONDATASTORE_H
#define MISSIONDATASTORE_H

#include <QJsonObject>
#include <QString>

class MissionDataStore
{
public:
    // Loaded .ms file ka complete JSON object
    static QJsonObject missionData;

    // Last loaded .ms file ka path
    static QString     missionFilePath;

    // Helper: specific key ka data lo
    static QJsonObject getDoctrine()
    {
        return missionData.value("doctrine").toObject();
    }

    static QJsonObject getTactical()
    {
        return missionData.value("tactical").toObject();
    }

    // Data set karo (ek jagah se load, har jagah accessible)
    static void setMissionData(const QJsonObject& obj, const QString& filePath = "")
    {
        missionData    = obj;
        missionFilePath = filePath;
    }

    // Clear karo
    static void clear()
    {
        missionData    = QJsonObject();
        missionFilePath = "";
    }

    // Check karo data loaded hai ya nahi
    static bool isLoaded()
    {
        return !missionData.isEmpty();
    }

private:
    MissionDataStore() = delete;
};

#endif // MISSIONDATASTORE_H
