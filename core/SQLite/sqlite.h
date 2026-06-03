
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
#include "core/Recorder/payload.h"
#include <QSharedPointer>

class Hierarchy;
class SQLite : public QObject
{
    Q_OBJECT

public:
    explicit SQLite(Hierarchy* hierarchy,
                    QObject *parent = nullptr);
    ~SQLite();

    void dbInit();
    void dbInit(QString path,bool &isFileSaved);
    void dbConnect();
    void dbConnect(QString path, bool &isFileLoaded);
    void setSQLite(SQLite &s_SQLite);
    SQLite *getSQLite();
    void close();

private:
    bool open(const QString &dbPath);
    bool exec(const QString &queryStr);
    void applyPragmas();
    // void createTables();
    bool createTables();
    QSqlDatabase database() const;

private:
    QSqlDatabase m_db;
    Hierarchy*   m_hierarchy;
public:
    void receivePayLoad(PayLoad m_payLoad);
    bool insertFrameMap(int frameIndex, qint64 timestamp);

    bool insertProfileCategoriesDetails(int frameIndex,
        ProfileCategoriesDetailsList m_profileCategoriesDetailsList);
    bool insertProfileCategoriesCRUD(int frameIndex,
        ProfileCategoriesCRUDList m_profileCategoriesCRUDList);
    bool insertEntitiesDetails(int frameIndex,EntitiesDetailsList m_entitiesDetailsList);
    bool insertEntitiesCreated(int frameIndex, EntitiesCreatedList m_entitiesCreatedList);
    bool insertEntitiesUpdated(int frameIndex, EntitiesUpdatedList m_entitiesUpdatedList);
    bool insertEntitiesDeleted(int frameIndex, EntitiesDeletedList m_entitiesDeletedList);
    bool insertEntitiesMeshRenderer2DCRUD(int frameIndex,
                                          EntitiesMeshRenderer2DCRUDList m_entitiesMeshRenderer2DCRUDList);
    bool insertEntitiesMeshRenderer2D(int frameIndex,
                                   EntitiesMeshRenderer2DList m_entitiesMeshRenderer2DList);

    bool insertEntitiesTrajectory(int frameIndex,
                                  EntitiesTrajectoryList m_entitiesTrajectoryList);
    bool insertEntitiesTrajectoryCRUD(int frameIndex,
                                      EntitiesTrajectoryCRUDList m_entitiesTrajectoryCRUDList);
public:
    void setFrameIndexNDuration(int    &maxFrameIndex,
                                qint64 &maxDuration);
    void setPayLoad(PayLoad &payload);
    void receivePayLoadFrameIndex(int frameIndex);
    PayLoad createPayLoad();
    void setTimeStamp(const int &frameIndex,qint64 &timestamp);

    void setProfileCategoriesDetails(
        ProfileCategoriesDetailsList &profileCategoriesDetailsList);
    void setProfileCategoriesCRUD(int &frameIndex,
                                  ProfileCategoriesCRUDList &profileCategoriesCRUDList);
    void setEntitiesDetailsList(
        EntitiesDetailsList &entitiesDetailsList);
    void setEntitiesCreatedList(const int &frameIndex,
                                EntitiesCreatedList &entitiesCreatedList);
    void setEntitiesUpdatedList(const int &frameIndex,
                                EntitiesUpdatedList &entitiesUpdatedList);
    void setEntitiesDeletedList(const int &frameIndex,
                                EntitiesDeletedList &entitiesDeletedList);

    void setEntitiesMeshRenderer2D(const int &frameIndex,
                                   EntitiesMeshRenderer2DList &entitiesMeshRenderer2DList);
    void setEntitiesMeshRenderer2DCRUD(const int &frameIndex,
                                       EntitiesMeshRenderer2DCRUDList &entitiesMeshRenderer2DCRUDList);

    void setEntitiesTrajectory(int &frameIndex,
                               EntitiesTrajectoryList &entitiesTrajectoryList);
    void setEntitiesTrajectoryCRUD(int &frameIndex,
                                   EntitiesTrajectoryCRUDList &entitiesTrajectoryCRUDList);

    void setPayLoadFromIndex(PayLoad &payLoad,int frameIndex);
    void setPayLoadFromIndex(QSharedPointer<PayLoad> payload,int frameIndex);
    void setEntitiesDetailsListInBtw(int frameIndex,
        EntitiesDetailsList &entitiesDetailsList);
    void setEntitiesDeletedListInBtw(int frameIndex,
        EntitiesDetailsList &entitiesDetailsList);
    void setEntitiesMeshRenderer2DInBtw(int frameIndex,
        EntitiesMeshRenderer2DList &entitiesMeshRenderer2DList);
    void setEntitiesTrajectoryBtw(int &frameIndex,
        EntitiesTrajectoryList &entitiesTrajectoryList);
    QList<QPair<QString, qint64>> getBookmarks() const;
    void saveBookmark(const QString &note, qint64 timestamp);
    // ---- IFF ----
    bool insertIffDetails (int frameIndex, const IffList     &list);
    bool insertIffCRUD    (int frameIndex, const IffCRUDList &list);
    void setIffDetails    (int frameIndex, IffList     &list);
    void setIffCRUD       (int frameIndex, IffCRUDList &list);

    // ---- Radio ----
    bool insertRadioDetails (int frameIndex, const RadioList     &list);
    bool insertRadioCRUD    (int frameIndex, const RadioCRUDList &list);
    void setRadioDetails    (int frameIndex, RadioList     &list);
    void setRadioCRUD       (int frameIndex, RadioCRUDList &list);

    // ---- Weapon ----
    bool insertWeaponDetails (int frameIndex, const WeaponList     &list);
    bool insertWeaponCRUD    (int frameIndex, const WeaponCRUDList &list);
    void setWeaponDetails    (int frameIndex, WeaponList     &list);
    void setWeaponCRUD       (int frameIndex, WeaponCRUDList &list);


signals:
    void sendPayLoad(PayLoad m_payLoad);
    void sendPayLoadByFrameIndex(PayLoad m_payLoad);
public slots:
    void rec_OnProcessRequested(QSharedPointer<PayLoad> data);
signals:
    void rec_ProcessingFinished(QSharedPointer<PayLoad> data);
public slots:
    void rep_OnProcessRequested(QSharedPointer<PayLoad> payLoad);
signals:
    void rep_ProcessingFinished(QSharedPointer<PayLoad> data);
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

    DBStatuses dbStatus = DISCONNECTED;
    void sendDBStatus(DBStatuses *dbStatusPtr);
    bool insertSensorsDetails(int frameIndex, const SensorList &list);
    bool insertSensorsCRUD(int frameIndex, const SensorCRUDList &list);
    void setSensorsDetails(int frameIndex, SensorList &list);
    void setSensorsCRUD(int frameIndex, SensorCRUDList &list);

private:
    QString str;
public:
    typedef enum {
        D_NULL            = 0b10000000000000,
        D_JustPrint       = 0b01000000000000,
        D_INIT            = 0b00100000000000,
        D_Connect         = 0b00010000000000,
        D_GetPayLoad      = 0b00001000000000,
        D_SetPayLoad      = 0b00000100000000,
        D_Trajectory      = 0b00000010000000,
        D_LoadInBetween   = 0b00000001000000,
        D_FrameIndex      = 0b00000000100000,
        D_GetPacket       = 0b00000000010000,
    }debugSQLite;
    Q_ENUM(debugSQLite)

private:
    void debug(const QString &str,const debugSQLite &currentdebugType = D_JustPrint);

    int debugList = D_JustPrint
        | D_INIT
        | D_FrameIndex
        | D_GetPacket
        ;
    bool dbgIsAllow(const debugSQLite &currentdebugType);
};

#endif // SQLITE_H

