
#ifndef SQLITE_H
#define SQLITE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Recorder/recorder.h"

class Hierarchy;
class Simulation;

// typedef struct{
//     double longitude  ;
//     double latitude   ;
//     double altitude   ;
//     double heading    ;
//     float  turn_radius;
//     float  curr_speed ;
//     float  climb_rate ;
// }entity;

class SQLite : public QObject
{
    Q_OBJECT

public:
    explicit SQLite(Hierarchy* hierarchy,
                    Simulation* simulation,
                    QObject *parent = nullptr);
    ~SQLite();

    void dbInit();
    void dbConnect();
    void close();

    // Frame API (call this per simulation tick)
private:
    // DB lifecycle
    bool open(const QString &dbPath);
    bool exec(const QString &queryStr);
    void applyPragmas();

    // Schema
    void createTables();
    QSqlDatabase database() const;

private:
    QSqlDatabase m_db;
    Hierarchy*   m_hierarchy;
    Simulation*  m_simulation;
// Write Operation Start
public:
    void receivePayLoad(PayLoad m_payLoad);
    bool insertFrameMap(int frameIndex, qint64 timestamp);

    bool insertEntitiesDetails(EntitiesDetailsList m_entitiesDetailsList);
    bool insertEntitiesCreated(int frameIndex, EntitiesCreatedList m_entitiesCreatedList);
    bool insertEntitiesUpdated(int frameIndex, EntitiesUpdatedList m_entitiesUpdatedList);
    bool insertEntitiesDeleted(int frameIndex, EntitiesDeletedList m_entitiesDeletedList);
// Write Operation End

// Read Operation Start
public:
    void setFrameIndexNDuration(int    &maxFrameIndex,
                                qint64 &maxDuration);
    void setPayLoad(PayLoad &payLoad);
    PayLoad createPayLoad();
// get Value through SQLite
    void setTimeStamp(const int &frameIndex,qint64 &timestamp);
    void setEntitiesDetailsList(EntitiesDetailsList &entitiesDetailsList);
    void setEntitiesCreatedList(const int &frameIndex, EntitiesCreatedList &entitiesCreatedList);
    void setEntitiesUpdatedList(const int &frameIndex, EntitiesUpdatedList &entitiesUpdatedList);
    void setEntitiesDeletedList(const int &frameIndex, EntitiesDeletedList &entitiesDeletedList);

signals:
    void sendPayLoad(PayLoad m_payLoad);
// Read Operation End


public:
    enum Options{
        INIT,
        DEINIT
    };
    Q_ENUM(Options);
    Options mode;
    typedef enum{
        CONNECTED = true,
        DISCONNECTED = false
    }DBStatuses;
    Q_ENUM(DBStatuses);

    //DBStatuses* dbStatusPtr = nullptr;
    DBStatuses dbStatus = DISCONNECTED;
    void sendDBStatus(DBStatuses *dbStatusPtr);

    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;

    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b100000000000,
        D_JustPrint       = 0b010000000000,
        D_SetPayLoad      = 0b001000000000,
    }debugSQLite;
    Q_ENUM(debugSQLite)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugSQLite &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                    //| D_SetPayLoad
                    ;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugSQLite &currentdebugType);

    /*------------     Custom Debugger End     ------------*/


//public:
//    std::unordered_map<int,std::pair<std::string, std::string>> getEntities();
//    std::vector<qint64> getFrameMap();
//    void insert_entitiesCreation(    const QString &parentID,
//                                     const QString &id,
//                                     const QString &name,
//                                     const qint64  &created);
//    void insertEntity(std::string id,std::string name,qint64 created);
//    void insertEntityDeletion(std::string id,qint64 deleted);
//    void insertFrame(qint64 timestamp);
// changeNo    INTEGER PRIMARY KEY AUTOINCREMENT,
//     longitude   DOUBLE,
//     latitude    DOUBLE,
//     altitude    DOUBLE,
//     heading     DOUBLE,
//     turn_radius FLOAT,
//     curr_speed  FLOAT,
//     climb_rate  FLOAT,
//     frameIndex  INTEGER,
//     indexNo     INTEGER,
//    std::unordered_map<int , entity> getFrameByFrameIndex(int s_frameIndex);
//    void showFrameByFrameIndex(int s_frameIndex);

// Adjust to your real type
//std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
//    // Inserts
//    void insertEntities();

//    std::unordered_set<std::string> entities;

//    void insertFrames(int frameIndex);

//    // Lookups
//    int getIndexNoFromId(const QString &id);


//    std::vector<std::pair<std::string, int>> *Entities();
//    // FrameMap(qint64 timestamp);
//    // Frames(int frameIndex);

};

#endif // SQLITE_H

