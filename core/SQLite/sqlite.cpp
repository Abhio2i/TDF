
#include "sqlite.h"

SQLite::SQLite(Hierarchy* hierarchy,
               Simulation* simulation,
               QObject *parent)
    : QObject(parent),
    m_hierarchy(hierarchy),
    m_simulation(simulation)
{

}

SQLite::~SQLite()
{
    dbStatus = DISCONNECTED;
    close();
}

bool SQLite::open(const QString &dbPath)
{
    if (QSqlDatabase::contains("main_connection")) {
        m_db = QSqlDatabase::database("main_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "main_connection");
        m_db.setDatabaseName(dbPath);
    }

    if (!m_db.open()) {
        qCritical() << "SQLite open failed:" << m_db.lastError().text();
        dbStatus = DISCONNECTED;
        return false;
    }

    applyPragmas();
    dbStatus = CONNECTED;
    return true;
}

void SQLite::close()
{
    if (m_db.isValid() && m_db.isOpen()){
        m_db.close();
        dbStatus = DISCONNECTED;
    }
}

bool SQLite::exec(const QString &queryStr)
{
    QSqlQuery query(m_db);
    if (!query.exec(queryStr)) {
        qCritical() << query.lastError().text()
        << "\nQuery:" << queryStr;
        return false;
    }
    return true;
}

QSqlDatabase SQLite::database() const
{
    return m_db;
}

void SQLite::sendDBStatus(DBStatuses *dbStatusPtr)
{
    *dbStatusPtr = dbStatus;
}



void SQLite::applyPragmas()
{
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA synchronous=NORMAL;");
    exec("PRAGMA foreign_keys=ON;");
}

//
// -------------------- INIT --------------------
//
// void SQLite::dbInit()
// {
//     qDebug() << "Database Init";
//     QString dbPath =
//         QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
//         + "/simulation.db";
//     QDir().mkpath(QFileInfo(dbPath).path());
//     close();
//     if (QFile::exists(dbPath)) {
//         QFile::remove(dbPath);
//         qDebug() << "Old DB removed";
//     }
//     if (!open(dbPath))
//         return;
//     qDebug()<<"DB Location"<<dbPath;
//     createTables();
// }
void SQLite::dbInit()
{
    qDebug() << "Database Init";

    QString buildPath = QCoreApplication::applicationDirPath();
    QString dbPath = buildPath + "/simulation.db";

    close();

    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
        qDebug() << "Old DB removed";
    }

    if (!open(dbPath))
        return;
    qDebug()<<"DB Location"<<dbPath;
    createTables();
    //insertEntities();
}

void SQLite::dbConnect()
{
    qDebug() << "Database Connect";

    QString buildPath = QCoreApplication::applicationDirPath();
    QString dbPath = buildPath + "/simulation.db";

    close();

    if (QFile::exists(dbPath)) {
        qDebug() << "DB Exist";
    }

    if (!open(dbPath))
        return;
}

//
// -------------------- SCHEMA --------------------
//
void SQLite::createTables()
{
    /*
*       struct ProfileCategoriesDetails {
*           int       index;
*           QString   name;
*           QString   ID;
*       };
*       using ProfileCategoriesDetailsList = std::vector<ProfileCategoriesDetails>;
*/

    exec(R"(
        CREATE TABLE profileCategoriesDetails  (
            indexNo  INTEGER PRIMARY KEY,
            name     TEXT,
            id       TEXT UNIQUE
        )
    )");
    /*
 *
 *        enum Operation {
*           CREATE = true,
*           DELETE = false,
*       };
*
*       struct ProfileCategoriesCRUD {
*           int       index;
*           Operation operation;
*       };
*       using ProfileCategoriesCRUDList = std::vector<ProfileCategoriesCRUD>;
*
*/
    exec(R"(
        CREATE TABLE ProfileCategoriesCRUD  (
            operation   INTEGER,
            indexNo     INTEGER PRIMARY KEY,
            frameIndex  INTEGER,
            FOREIGN KEY (indexNo)    REFERENCES profileCategoriesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");



    /*
*
*
*       struct EntitiesDetails {
*           int index;
*           QString name;
*           QString parentID;
*           QString ID;
*       };
*       using EntitiesDetailsList = std::vector<EntitiesDetails>;
*/

    exec(R"(
        CREATE TABLE   frameMap (
            frameIndex INTEGER PRIMARY KEY,
            timestamp  BIGINT
        )
    )");
    /*struct entitiesDetails {
    int index;
    QString name;
    QString parentID;
    QString ID;
};*/
    exec(R"(
        CREATE TABLE entitiesDetails  (
            indexNo  INTEGER PRIMARY KEY,
            name     TEXT,
            id       TEXT UNIQUE,
            parentID TEXT,
            profile  BOOLEAN
        )
    )");
    exec(R"(
        CREATE TABLE entitiesCreated  (
            frameIndex  INTEGER,
            indexNo     INTEGER,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");
    exec(R"(
        CREATE TABLE entitiesUpdated  (
            frameIndex  INTEGER,
            indexNo     INTEGER,
            longitude   DOUBLE,
            latitude    DOUBLE,
            altitude    DOUBLE,
            heading     DOUBLE,
            turn_radius FLOAT,
            curr_speed  FLOAT,
            climb_rate  FLOAT,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");
    exec(R"(
        CREATE TABLE entitiesDeleted  (
            frameIndex  INTEGER,
            indexNo     INTEGER,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");

    /*
 *       struct EntitiesMeshRenderer2D {
 *           int index;
 *           bool Active;
 *           QString Sprite;
 *           QString Texture;
 *           QString color;
 *           QString color2;
 *       };
 *       using EntitiesMeshRenderer2DList = std::vector<EntitiesMeshRenderer2D>;
 */
    exec(R"(
        CREATE TABLE entitiesMeshRenderer2D  (
            indexNo     INTEGER,
            frameIndex  INTEGER,
            Active      BOOLEAN,
            Sprite      TEXT,
            Texture     TEXT,
            color       TEXT,
            color2      TEXT,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");
    /*
 *
 *       struct EntitiesMeshRenderer2DCRUD {
 *           int       index;
 *           Operation operation;
 *       };
 *       using EntitiesMeshRenderer2DCRUDList = std::vector<EntitiesMeshRenderer2DCRUD>;
 */
    exec(R"(
        CREATE TABLE entitiesMeshRenderer2DCRUD  (
            operation   INTEGER,
            indexNo     INTEGER,
            frameIndex  INTEGER,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");

    /*
 *      struct TrajectoryWaypoint {
 *          int    index;
 *          double geo_latitude;
 *          double geo_longitude;
 *          double geo_altitude;
 *          double geo_Heading;
 *          float  vector_x;
 *          float  vector_y;
 *          float  vector_z;
 *          double speed     = 0;
 *          bool   sensor    = false;
 *          bool   formation = false;
 *      };
 */
    exec(R"(
        CREATE TABLE trajectoryWaypoint  (
            indexNo        INTEGER,
            waypoint_index INTEGER,
            frameIndex    INTEGER,
            geo_latitude  DOUBLE,
            geo_longitude DOUBLE,
            geo_altitude  DOUBLE,
            geo_Heading   DOUBLE,
            vector_x      FLOAT,
            vector_y      FLOAT,
            vector_z      FLOAT,
            speed         DOUBLE,
            sensor        BOOLEAN,
            formation     BOOLEAN,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");

    /*
 *      struct EntitiesTrajectoryCRUD {
 *          int index;
 *          Operation operation;
 *      };
 */

    exec(R"(
        CREATE TABLE entitiesTrajectoryCRUD  (
            operation   INTEGER,
            indexNo     INTEGER,
            frameIndex  INTEGER,
            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");

}

//
// -------------------- ENTITIES --------------------

/*   qint64              timestamp;
 *   int                 frameIndex;
 *   EntitiesDetailsList entitiesDetailsList;
 *   EntitiesCreatedList entitiesCreatedList;
 *   EntitiesUpdatedList entitiesUpdatedList;
 *   EntitiesDeletedList entitiesDeletedList;
 */
void SQLite::receivePayLoad(PayLoad m_payLoad){
    QString m_str = QString(
                        "Receive_PayLoad => "
                        "TimeStamo: %1 ,"
                        "Frame Index: %2 ,").arg(
                            QString::number(m_payLoad.timestamp),
                            QString::number(m_payLoad.frameIndex));
    QString p = "PASS";
    QString f = "FAIL";

    m_str += "\n  Frame_Map: ";
    if(!insertFrameMap(m_payLoad.frameIndex,m_payLoad.timestamp)){
        m_str += p;
        //return;
    }else{
        m_str += f;
    }

    m_str += "\n  Profile_Categories_Details: ";
    if((m_payLoad.profileCategoriesDetailsList.size() > 0) &&
        (insertProfileCategoriesDetails(m_payLoad.profileCategoriesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Profile_Categories_CRUD: ";
    if((m_payLoad.profileCategoriesCRUDList.size() > 0) &&
        (insertProfileCategoriesCRUD(m_payLoad.frameIndex,
                                     m_payLoad.profileCategoriesCRUDList)))
    {
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Details: ";
    if((m_payLoad.entitiesDetailsList.size() > 0) && (insertEntitiesDetails(m_payLoad.entitiesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Created: ";
    if((m_payLoad.entitiesCreatedList.size() > 0) && (insertEntitiesCreated(m_payLoad.frameIndex, m_payLoad.entitiesCreatedList))){
        m_str += p;
    }else{
        m_str += f;
    }
    /*
 *      using EntitiesMeshRenderer2DCRUDList = std::vector<EntitiesMeshRenderer2DCRUD>;
 *      using EntitiesMeshRenderer2DList = std::vector<EntitiesMeshRenderer2D>;
 */
    m_str += "\n  Mesh Renderer List: ";
    if((m_payLoad.entitiesMeshRenderer2DList.size() > 0) &&
        (insertEntitiesMeshRenderer2D(m_payLoad.frameIndex,
                                      m_payLoad.entitiesMeshRenderer2DList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Mesh Renderer CRUD: ";
    if((m_payLoad.entitiesMeshRenderer2DCRUDList.size() > 0) &&
        (insertEntitiesMeshRenderer2DCRUD(m_payLoad.frameIndex,
                                          m_payLoad.entitiesMeshRenderer2DCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }



    m_str += "\n  Insert_Entities_Updated: ";
    if((m_payLoad.entitiesUpdatedList.size() > 0) && (insertEntitiesUpdated(m_payLoad.frameIndex, m_payLoad.entitiesUpdatedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Deleted: ";
    if((m_payLoad.entitiesDeletedList.size() > 0) && (insertEntitiesDeleted(m_payLoad.frameIndex, m_payLoad.entitiesDeletedList))){
        m_str += p;
    }else{
        m_str += f;
    }


    m_str += "\n  Trajectory List: ";
    if((m_payLoad.entitiesTrajectoryList.size() > 0) && (insertEntitiesTrajectory(m_payLoad.frameIndex, m_payLoad.entitiesTrajectoryList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Trajectory CRUD: ";
    if((m_payLoad.entitiesTrajectoryCRUDList.size() > 0) && (insertEntitiesTrajectoryCRUD(m_payLoad.frameIndex, m_payLoad.entitiesTrajectoryCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }


    debug(m_str,D_GetPayLoad);
}

bool SQLite::insertFrameMap(int frameIndex, qint64 timestamp)
{
    /*CREATE TABLE   frameMap (
     *    frameIndex INTEGER PRIMARY KEY,
     *    timestamp  BIGINT
     *)
     */
    QSqlQuery query(database());
    query.prepare(
        "INSERT INTO frameMap (frameIndex, timestamp) "
        "VALUES (:frameIndex, :timestamp)"
        );

    query.bindValue(":frameIndex", frameIndex);
    query.bindValue(":timestamp", timestamp);

    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return false;
    }
    return true;
}

/*
*       CREATE TABLE profileCategoriesDetails  (
*                   indexNo  INTEGER PRIMARY KEY,
*                   name     TEXT,
*                   id       TEXT UNIQUE
*               )
*/
bool SQLite::insertProfileCategoriesDetails(
    ProfileCategoriesDetailsList m_profileCategoriesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO profileCategoriesDetails
        (indexNo, name, id)
        VALUES (:indexNo, :name, :id)
       )");
    for(auto i : m_profileCategoriesDetailsList){
        query.bindValue(":indexNo"  ,i.index);
        query.bindValue(":name"     ,i.name);
        query.bindValue(":id"       ,i.ID);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Profile Categories Details failed:"
                        << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Profile Categories Details successful!";
        }
    }
    return true;
}
/*
*        CREATE TABLE ProfileCategoriesCRUD  (
*            operation   INTEGER,
*            indexNo     INTEGER PRIMARY KEY,
*            frameIndex  INTEGER,
*            FOREIGN KEY (indexNo)    REFERENCES profileCategoriesDetails(indexNo),
*            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
*        )
*/
bool SQLite::insertProfileCategoriesCRUD(int frameIndex,
                                         ProfileCategoriesCRUDList m_profileCategoriesCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO ProfileCategoriesCRUD
        ( operation, frameIndex, indexNo)
        VALUES ( :operation, :frameIndex, :indexNo)
        )");
    for(auto i : m_profileCategoriesCRUDList){
        query.bindValue(":operation"   ,i.operation);
        query.bindValue(":frameIndex"  ,frameIndex);
        query.bindValue(":indexNo"     ,i.index);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Creation failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Entities Creation successful!";
        }
    }
    return true;
}

bool SQLite::insertEntitiesDetails(EntitiesDetailsList m_entitiesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesDetails
        (indexNo, name, id, parentID, profile)
        VALUES (:indexNo, :name, :id, :parentID, :profile)
       )");
    for(EntitiesDetails i : m_entitiesDetailsList){
        query.bindValue(":indexNo"  ,i.index);
        query.bindValue(":name"     ,i.name);
        query.bindValue(":id"       ,i.ID);
        query.bindValue(":parentID" ,i.parentID);
        query.bindValue(":profile" ,true);

        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Details failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Entities Details successful!";
        }
    }
    return true;
}

bool SQLite::insertEntitiesCreated(int frameIndex, EntitiesCreatedList m_entitiesCreatedList)
{
    /*
     *  CREATE TABLE entitiesCreated  (
     *      frameIndex  INTEGER,
     *      indexNo     INTEGER,
     *      FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
     *      FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
     *  )
     */
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesCreated
        ( frameIndex, indexNo)
        VALUES ( :frameIndex, :indexNo)
        )");
    for(auto i : m_entitiesCreatedList){
        query.bindValue(":frameIndex"  ,frameIndex);
        query.bindValue(":indexNo"     ,i.index);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Creation failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Entities Creation successful!";
        }
    }
    return true;
}
/*
 *      CREATE TABLE entitiesUpdated  (
 *          frameIndex  INTEGER,
 *          indexNo     INTEGER,
 *          longitude   DOUBLE,
 *          latitude    DOUBLE,
 *          altitude    DOUBLE,
 *          heading     DOUBLE,
 *          turn_radius FLOAT,
 *          curr_speed  FLOAT,
 *          climb_rate  FLOAT,
 *          FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *          FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *      )
 */
bool SQLite::insertEntitiesUpdated(int frameIndex, EntitiesUpdatedList m_entitiesUpdatedList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesUpdated
        (frameIndex, indexNo, longitude, latitude, altitude,
         heading, turn_radius, curr_speed, climb_rate)
        VALUES
        (:frameIndex, :indexNo, :longitude, :latitude, :altitude,
         :heading, :turn_radius, :curr_speed, :climb_rate)
    )");
    for(auto entity : m_entitiesUpdatedList){
        query.bindValue(":frameIndex",  frameIndex);
        query.bindValue(":indexNo",     entity.index);
        query.bindValue(":longitude",   entity.longitude);
        query.bindValue(":latitude",    entity.latitude);
        query.bindValue(":altitude",    entity.altitude);
        query.bindValue(":heading",     entity.heading);
        query.bindValue(":turn_radius", entity.turn_radius);
        query.bindValue(":curr_speed",  entity.curr_speed);
        query.bindValue(":climb_rate",  entity.climb_rate);
        qDebug() << query.boundValues();
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Updation failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Entities Updation successful!";
        }
    }

    return true;
}

bool SQLite::insertEntitiesDeleted(int frameIndex, EntitiesDeletedList m_entitiesDeletedList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesDeleted
        ( frameIndex, indexNo)
        VALUES ( :frameIndex, :indexNo)
        )");
    for(auto i : m_entitiesDeletedList){
        query.bindValue(":frameIndex"  ,frameIndex);
        query.bindValue(":indexNo"     ,i.index);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Deletion failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Insert Entities Deletion successful!";
        }
    }
    return true;
}

/*
 *        CREATE TABLE entitiesMeshRenderer2DCRUD  (
 *            operation   INTEGER,
 *            indexNo     INTEGER,
 *            frameIndex  INTEGER,
 *            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *        )
 */
bool SQLite::insertEntitiesMeshRenderer2DCRUD(int frameIndex,
                                              EntitiesMeshRenderer2DCRUDList m_entitiesMeshRenderer2DCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesMeshRenderer2DCRUD
        (operation, indexNo, frameIndex)
        VALUES (:operation, :indexNo, :frameIndex)
        )");
    for(auto i : m_entitiesMeshRenderer2DCRUDList){
        query.bindValue(":operation"   ,i.operation);
        query.bindValue(":indexNo"     ,i.index);
        query.bindValue(":frameIndex"  ,frameIndex);
        if (!query.exec()) {
            qCritical() << "Insertion Trajectory CRUD failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Trajectory CRUD successful!";
        }
    }
    return true;
}


/*
 *      CREATE TABLE entitiesMeshRenderer2D  (
 *                  indexNo     INTEGER,
 *                  frameIndex  INTEGER,
 *                  Active      BOOLEAN,
 *                  Sprite      TEXT,
 *                  Texture     TEXT,
 *                  color       TEXT,
 *                  color2      TEXT,
 *                  FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *                  FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *              )
 */
bool SQLite::insertEntitiesMeshRenderer2D(int frameIndex,
                                          EntitiesMeshRenderer2DList m_entitiesMeshRenderer2DList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesMeshRenderer2D
        (indexNo, frameIndex,
        Active, Sprite, Texture, color, color2)
        VALUES (:indexNo , :frameIndex,
        :Active, :Sprite, :Texture, :color, :color2)
        )");
    for(auto emr = m_entitiesMeshRenderer2DList.begin();
         emr != m_entitiesMeshRenderer2DList.end();
         ++emr){
        query.bindValue(":indexNo"   ,(*emr).index  );
        query.bindValue(":frameIndex",frameIndex );
        query.bindValue(":Active"    ,(*emr).Active );
        query.bindValue(":Sprite"    ,(*emr).Sprite );
        query.bindValue(":Texture"   ,(*emr).Texture);
        query.bindValue(":color"     ,(*emr).color  );
        query.bindValue(":color2"    ,(*emr).color2 );
        if (!query.exec()) {
            qCritical() << "Insertion Trajectory CRUD failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Trajectory CRUD successful!";
        }
    }
    return true;
}




/*
 *  CREATE TABLE trajectoryWaypoint  (
 *      indexNo       INTEGER,
 *      frameIndex    INTEGER,
 *      geo_latitude  DOUBLE,
 *      geo_longitude DOUBLE,
 *      geo_altitude  DOUBLE,
 *      geo_Heading   DOUBLE,
 *      vector_x      FLOAT,
 *      vector_y      FLOAT,
 *      vector_z      FLOAT,
 *      speed         DOUBLE,
 *      sensor        BOOLEAN,
 *      formation     BOOLEAN,
 *      FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *      FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *  )
 *
 *
 */
bool SQLite::insertEntitiesTrajectory
    (int frameIndex,EntitiesTrajectoryList m_entitiesTrajectoryList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO trajectoryWaypoint
        (indexNo, waypoint_index, frameIndex,
        geo_latitude, geo_longitude,
        geo_altitude, geo_Heading,
        vector_x, vector_y, vector_z,
        speed, sensor, formation
        )
        VALUES (:indexNo, :index,:frameIndex,
        :geo_latitude, :geo_longitude,
        :geo_altitude, :geo_Heading,
        :vector_x, :vector_y, :vector_z,
        :speed, :sensor, :formation)
       )");
    for(auto etl  = m_entitiesTrajectoryList.begin();
         etl !=  m_entitiesTrajectoryList.end(); ++etl){
        for(auto tw : (*etl).Trajectories){
            query.bindValue(":indexNo"      ,(*etl).index);
            query.bindValue(":index"      ,  tw.index);
            query.bindValue(":frameIndex"   ,frameIndex);
            query.bindValue(":geo_latitude" ,tw.geo_latitude);
            query.bindValue(":geo_longitude",tw.geo_longitude);
            query.bindValue(":geo_altitude" ,tw.geo_altitude);
            query.bindValue(":geo_Heading"  ,tw.geo_Heading);
            query.bindValue(":vector_x"     ,tw.vector_x);
            query.bindValue(":vector_y"     ,tw.vector_y);
            query.bindValue(":vector_z"     ,tw.vector_z);
            query.bindValue(":speed"        ,tw.speed);
            query.bindValue(":sensor"       ,tw.sensor);
            query.bindValue(":formation    ",tw.formation);
            if (!query.exec()) {
                qCritical() << "Insertion Trajectory Waypoint failed:"
                            << query.lastError().text();
                return false;
            } else {
                qDebug() << "Insertion Trajectory Waypoint successful!";
            }
        }
    }
    return true;
}
/*
 *        CREATE TABLE entitiesTrajectoryCRUD  (
 *            operation   INTEGER,
 *            indexNo     INTEGER,
 *            frameIndex  INTEGER,
 *            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *        )
 */
bool SQLite::insertEntitiesTrajectoryCRUD
    (int frameIndex,
     EntitiesTrajectoryCRUDList m_entitiesTrajectoryCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesTrajectoryCRUD
        (operation, indexNo, frameIndex)
        VALUES (:operation, :indexNo, :frameIndex)
        )");
    for(auto i : m_entitiesTrajectoryCRUDList){
        query.bindValue(":operation"   ,i.operation);
        query.bindValue(":indexNo"     ,i.index);
        query.bindValue(":frameIndex"  ,frameIndex);
        if (!query.exec()) {
            qCritical() << "Insertion Trajectory CRUD failed:" << query.lastError().text();
            return false;
        } else {
            qDebug() << "Insertion Trajectory CRUD successful!";
        }
    }
    return true;
}





/*******************************************
 *          Read Operation Start           *
 *******************************************/


void SQLite::setPayLoad(PayLoad &payLoad)
{
    //payLoad.frameIndex
    str = QString(
              "Received PayLoad => \n"
              "\t FrameIndex: %1 \n").arg(QString::number(payLoad.frameIndex));

    setTimeStamp(payLoad.frameIndex, payLoad.timestamp);
    str += QString(
               "\t TimeStamp: %1 \n").arg(QString::number(payLoad.timestamp));
    if(payLoad.entitiesDetailsList.empty()){
        setEntitiesDetailsList(payLoad.entitiesDetailsList);
        str += QString(
                   "\t Entities Details List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesDetailsList.size()));
    }
    if(payLoad.entitiesTrajectoryList.empty()){
        setProfileCategoriesDetails(payLoad.profileCategoriesDetailsList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryList.size()));
    }
    if(payLoad.profileCategoriesCRUDList.empty()){
        setProfileCategoriesCRUD(payLoad.frameIndex, payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesCRUDList.size()));
    }
    if(payLoad.profileCategoriesDetailsList.empty()){
        setProfileCategoriesDetails(payLoad.profileCategoriesDetailsList);
        str += QString(
                   "\t Profile Categories Details List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesDetailsList.size()));
    }
    if(payLoad.profileCategoriesCRUDList.empty()){
        setProfileCategoriesCRUD(payLoad.frameIndex, payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesCRUDList.size()));
    }
    if(payLoad.entitiesCreatedList.empty()){
        setEntitiesCreatedList(payLoad.frameIndex, payLoad.entitiesCreatedList);
        str += QString(
                   "\t Entities Created List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesCreatedList.size()));
    }
    if(payLoad.entitiesUpdatedList.empty()){
        setEntitiesUpdatedList(payLoad.frameIndex, payLoad.entitiesUpdatedList);
        str += QString(
                   "\t Entities Updated List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesUpdatedList.size()));
    }
    if(payLoad.entitiesDeletedList.empty()){
        setEntitiesDeletedList(payLoad.frameIndex, payLoad.entitiesDeletedList);
        str += QString(
                   "\t Entities Deleted List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesDeletedList.size()));
    }
    if(payLoad.entitiesMeshRenderer2DList.empty()){
        setEntitiesMeshRenderer2D(payLoad.frameIndex,    payLoad.entitiesMeshRenderer2DList);
        str += QString(
                   "\t Entities Mesh Renderer List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesMeshRenderer2DList.size()));
    }
    if(payLoad.entitiesMeshRenderer2DCRUDList.empty()){
        setEntitiesMeshRenderer2DCRUD(payLoad.frameIndex, payLoad.entitiesMeshRenderer2DCRUDList);
        str += QString(
                   "\t Entities Mesh Renderer CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesMeshRenderer2DCRUDList.size()));
    }
    if(payLoad.entitiesTrajectoryList.empty()){
        setEntitiesTrajectory(payLoad.frameIndex, payLoad.entitiesTrajectoryList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryList.size()));
    }
    if(payLoad.entitiesTrajectoryCRUDList.empty()){
        setEntitiesTrajectoryCRUD(payLoad.frameIndex, payLoad.entitiesTrajectoryCRUDList);
        str += QString(
                   "\t Entities Trajectory CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryCRUDList.size()));
    }
    debug(str,D_SetPayLoad);
}


PayLoad SQLite::createPayLoad()
{
    PayLoad m_payLoad;
    return m_payLoad;
}

void SQLite::setTimeStamp(const int &frameIndex, qint64 &timestamp)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT timestamp FROM frameMap
        WHERE frameIndex = :frameIndex
        )");
    query.bindValue(":frameIndex"  ,frameIndex);

    if (!query.exec()) {
        qCritical() << query.lastError().text();
        timestamp = 0;
        return;
    }
    if (query.next()) {
        timestamp = query.value(0).toInt();
        return;
    }
    timestamp = 0;
    return;
}


void SQLite::setFrameIndexNDuration(int &maxFrameIndex, qint64 &maxDuration)
{
    QSqlQuery query(database());
    if (!query.exec(R"(
        SELECT frameIndex, timestamp
        FROM frameMap
        ORDER BY frameIndex DESC
        LIMIT 1
    )")) {
        qCritical() << query.lastError().text();
    }
    if (query.next()) {
        maxFrameIndex = query.value(0).toInt();
        maxDuration   = query.value(1).toLongLong();
    }
}
/*
*       CREATE TABLE profileCategoriesDetails  (
*                   indexNo  INTEGER PRIMARY KEY,
*                   name     TEXT,
*                   id       TEXT UNIQUE
*               )
*/
void SQLite::setProfileCategoriesDetails(
    ProfileCategoriesDetailsList &profileCategoriesDetailsList)
{
    QSqlQuery query(database());

    query.prepare(R"(
        SELECT indexNo, name, id
        FROM profileCategoriesDetails
    )");

    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        ProfileCategoriesDetails pcd;

        pcd.index    = query.value(0).toInt();
        pcd.name     = query.value(1).toString();
        pcd.ID       = query.value(2).toString();   // id column
        profileCategoriesDetailsList.push_back(pcd);
    }
}

/*
*        CREATE TABLE ProfileCategoriesCRUD  (
*            operation   INTEGER,
*            indexNo     INTEGER PRIMARY KEY,
*            frameIndex  INTEGER,
*            FOREIGN KEY (indexNo)    REFERENCES profileCategoriesDetails(indexNo),
*            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
*        )
*/
void SQLite::setProfileCategoriesCRUD(int &frameIndex,
                                      ProfileCategoriesCRUDList &profileCategoriesCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT operation, indexNo FROM ProfileCategoriesCRUD
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        ProfileCategoriesCRUD pcc;
        pcc.operation    = static_cast<Operation>(query.value(0).toInt());
        pcc.index        = query.value(1).toInt();
        profileCategoriesCRUDList.push_back(pcc);
    }
}




/*
    CREATE TABLE entitiesDetails  (
        indexNo  INTEGER PRIMARY KEY,
        name     TEXT,
        id       TEXT UNIQUE,
        parentID TEXT,
        profile  BOOLEAN
    )
*/
void SQLite::setEntitiesDetailsList(EntitiesDetailsList &entitiesDetailsList)

{
    QSqlQuery query(database());

    query.prepare(R"(
        SELECT indexNo, name, id, parentID, profile
        FROM entitiesDetails
    )");

    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesDetails entitiesDetails;

        entitiesDetails.index    = query.value(0).toInt();
        entitiesDetails.name     = query.value(1).toString();
        entitiesDetails.ID       = query.value(2).toString();   // id column
        entitiesDetails.parentID = query.value(3).toString();
        // profile = query.value(4).toBool(); // if needed later

        entitiesDetailsList.push_back(entitiesDetails);
    }

}
/*
 *CREATE TABLE entitiesCreated  (
 *    frameIndex  INTEGER,
 *    indexNo     INTEGER,
 *    FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *    FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 */

void SQLite::setEntitiesCreatedList(
    const int &frameIndex,
    EntitiesCreatedList &entitiesCreatedList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo FROM entitiesCreated
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesCreated entitiesCreated;
        entitiesCreated.index    = query.value(0).toInt();
        entitiesCreatedList.push_back(entitiesCreated);
    }
}

/*
 *     CREATE TABLE entitiesUpdated  (
 *         frameIndex  INTEGER,
 *         indexNo     INTEGER,
 *         longitude   DOUBLE,
 *         latitude    DOUBLE,
 *         altitude    DOUBLE,
 *         heading     DOUBLE,
 *         turn_radius FLOAT,
 *         curr_speed  FLOAT,
 *         climb_rate  FLOAT,
 *         FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *         FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *     )
 */
void SQLite::setEntitiesUpdatedList(
    const int &frameIndex,
    EntitiesUpdatedList &entitiesUpdatedList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo, longitude, latitude, altitude, heading,
                turn_radius, curr_speed, climb_rate
        FROM entitiesUpdated WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }
    while (query.next()) {
        EntitiesUpdated entitiesUpdated;
        entitiesUpdated.index          = query.value(0).toInt();
        entitiesUpdated.longitude      = query.value(1).toDouble();
        entitiesUpdated.latitude       = query.value(2).toDouble();
        entitiesUpdated.altitude       = query.value(3).toDouble();
        entitiesUpdated.heading        = query.value(4).toDouble();
        entitiesUpdated.turn_radius    = query.value(5).toFloat();
        entitiesUpdated.curr_speed     = query.value(6).toFloat();
        entitiesUpdated.climb_rate     = query.value(7).toFloat();
        entitiesUpdatedList.push_back(entitiesUpdated);
    }
}

/*
 *      CREATE TABLE entitiesDeleted  (
 *          frameIndex  INTEGER,
 *          indexNo     INTEGER,
 *          FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *          FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *      )
 */
void SQLite::setEntitiesDeletedList(
    const int &frameIndex,
    EntitiesDeletedList &entitiesDeletedList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo FROM entitiesDeleted
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesDeleted entitiesDeleted;
        entitiesDeleted.index    = query.value(0).toInt();
        entitiesDeletedList.push_back(entitiesDeleted);
    }
}

/*
 *      CREATE TABLE entitiesMeshRenderer2D  (
 *                  indexNo     INTEGER,
 *                  frameIndex  INTEGER,
 *                  Active      BOOLEAN,
 *                  Sprite      TEXT,
 *                  Texture     TEXT,
 *                  color       TEXT,
 *                  color2      TEXT,
 *                  FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *                  FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *              )
 */

void SQLite::setEntitiesMeshRenderer2D(const int &frameIndex,
                                       EntitiesMeshRenderer2DList &entitiesMeshRenderer2DList)
{
    QSqlQuery query(database());

    query.prepare(R"(
        SELECT indexNo, Active, Sprite, Texture, color, color2
        FROM entitiesMeshRenderer2D
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);

    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesMeshRenderer2D emr;
        emr.index    = query.value(0).toInt();
        emr.Active   = query.value(1).toBool();
        emr.Sprite   = query.value(2).toString();
        emr.Texture  = query.value(3).toString();
        emr.color    = query.value(4).toString();
        emr.color2   = query.value(5).toString();
        // emr.name     = query.value(1).toString();
        // emr.ID       = query.value(2).toString();   // id column
        entitiesMeshRenderer2DList.push_back(emr);
    }
}

/*
 *        CREATE TABLE entitiesMeshRenderer2DCRUD  (
 *            operation   INTEGER,
 *            indexNo     INTEGER,
 *            frameIndex  INTEGER,
 *            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *        )
 */

void SQLite::setEntitiesMeshRenderer2DCRUD(const int &frameIndex,
                                           EntitiesMeshRenderer2DCRUDList &entitiesMeshRenderer2DCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT operation, indexNo FROM entitiesMeshRenderer2DCRUD
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesMeshRenderer2DCRUD emrc;
        emrc.operation    = static_cast<Operation>(query.value(0).toInt());
        emrc.index        = query.value(1).toInt();
        entitiesMeshRenderer2DCRUDList.push_back(emrc);
    }
}

/*
 *  CREATE TABLE trajectoryWaypoint  (
 *      indexNo       INTEGER,
 *      frameIndex    INTEGER,
 *      geo_latitude  DOUBLE,
 *      geo_longitude DOUBLE,
 *      geo_altitude  DOUBLE,
 *      geo_Heading   DOUBLE,
 *      vector_x      FLOAT,
 *      vector_y      FLOAT,
 *      vector_z      FLOAT,
 *      speed         DOUBLE,
 *      sensor        BOOLEAN,
 *      formation     BOOLEAN,
 *      FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *      FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *  )
 */

void SQLite::setEntitiesTrajectory(int &frameIndex,
                                   EntitiesTrajectoryList &entitiesTrajectoryList)
{
    QSqlQuery query(database());

    // query.prepare(R"(
    //     SELECT indexNo,
    //     geo_latitude, geo_longitude, geo_altitude, geo_Heading,
    //     vector_x, vector_y, vector_z,
    //     speed, sensor, formation
    //     FROM trajectoryWaypoint
    //     WHERE frameIndex = :frameIndex
    //     ORDER BY indexNo ASC, first_name DESC
    // )");
    query.prepare(R"(
            SELECT indexNo, waypoint_index,
            geo_latitude, geo_longitude, geo_altitude, geo_Heading,
            vector_x, vector_y, vector_z,
            speed, sensor, formation
            FROM trajectoryWaypoint
            WHERE frameIndex = :frameIndex
            ORDER BY indexNo ASC, waypoint_index ASC
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }
    EntitiesTrajectory et = EntitiesTrajectory();
    //int prevIndex = -1;
    std::unordered_map <int ,EntitiesTrajectory> entitiesTrajectoryIndex;
    while (query.next()) {
        TrajectoryWaypoint tw;
        int index        =  query.value(0).toInt();
        tw.index         =  query.value(1 ).toInt();
        tw.geo_latitude  =  query.value(2 ).toDouble();   // id column
        tw.geo_longitude =  query.value(3 ).toDouble();
        tw.geo_altitude  =  query.value(4 ).toDouble();
        tw.geo_Heading   =  query.value(5 ).toDouble();
        tw.vector_x      =  query.value(6 ).toFloat();
        tw.vector_y      =  query.value(7 ).toFloat();
        tw.vector_z      =  query.value(8 ).toFloat();
        tw.speed         =  query.value(9 ).toDouble();
        tw.sensor        =  query.value(10).toBool();
        tw.formation     =  query.value(11).toBool();
        entitiesTrajectoryIndex[index].Trajectories.push_back(tw);
    }

    for(auto eti = entitiesTrajectoryIndex.begin();
         eti != entitiesTrajectoryIndex.end();
         ++eti){
        EntitiesTrajectory et;
        et       = (*eti).second;
        et.index = (*eti).first;
        entitiesTrajectoryList.push_back(et);
    }
}
/*
 *        CREATE TABLE entitiesTrajectoryCRUD  (
 *            operation   INTEGER,
 *            indexNo     INTEGER,
 *            frameIndex  INTEGER,
 *            FOREIGN KEY (indexNo)    REFERENCES entitiesDetails(indexNo),
 *            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 *        )
 */
void SQLite::setEntitiesTrajectoryCRUD(int &frameIndex, EntitiesTrajectoryCRUDList &entitiesTrajectoryCRUDList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT operation, indexNo FROM entitiesTrajectoryCRUD
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }

    while (query.next()) {
        EntitiesTrajectoryCRUD etc;
        etc.operation    = static_cast<Operation>(query.value(0).toInt());
        etc.index        = query.value(1).toInt();
        entitiesTrajectoryCRUDList.push_back(etc);
    }
}
/*******************************************
 *          Read Operation End           *
 *******************************************/


/*------------    Custom Debugger Start    ------------*/

void SQLite::debug(const QString &str, const debugSQLite &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }
}

bool SQLite::dbgIsAllow(const debugSQLite &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    return InsideList;
}

/*------------     Custom Debugger End     ------------*/




//bool SQLite::insertFrameMap(int frameIndex, qint64 timestamp)
//{

//    QSqlQuery query(database());
//    query.prepare(
//        "INSERT INTO frameMap (frameIndex,timestamp,) VALUES (:frameIndex, :timestamp)"
//        );
//    query.bindValue(":frameIndex", frameIndex);
//    query.bindValue(":timestamp" , timestamp);

//    if (!query.exec()) {
//        qCritical() << query.lastError().text();
//        return false;
//    }

//    return true;
//}
//
//void SQLite::insert_entitiesCreation(
//    const QString &parentID,
//    const QString &id,
//    const QString &name,
//    const qint64  &created)
//{
//    if (!m_db.isOpen()) {
//        qCritical() << "Database is not open!";
//        return;
//    }
//    QSqlQuery query(m_db);   // USE YOUR CONNECTION
//    query.prepare(R"(
//        INSERT INTO entitiesDetails
//        ( name,  id,  parentID,  profile) VALUES
//        (:name, :id, :parentID, :profile)
//    )");
//    query.bindValue(":name",     name);
//    query.bindValue(":id",       id);
//    query.bindValue(":parentID", parentID);
//    query.bindValue(":profile",  true);

//    if (!query.exec()) {
//        qDebug() << "Insert failed:" << query.lastError().text();
//        return;
//    }
//    QVariant insertedId = query.lastInsertId();
//    int currentIndexNo = insertedId.isValid() ? insertedId.toInt() : -1;
//    if(currentIndexNo == -1){
//        return;
//    }

//    query.prepare(R"(
//        SELECT frameIndex FROM frameMap
//        WHERE timestamp = :timestamp
//    )");
//    query.bindValue(":timestamp",  created);

//    if (!query.exec()) {
//        qDebug() << "Query failed to extract frameIndex:" << query.lastError().text();
//        return;
//    }

//    int currentFrameIndex = -1;

//    if (query.next()) {
//        currentFrameIndex = query.value(0).toInt();
//    }

//    query.prepare(R"(
//        INSERT INTO entitiesCreated
//        ( creation, frameIndex, indexNo   ) VALUES
//        (:creation, :frameIndex, :indexNo)
//    )");

//    query.bindValue(":creation",   created);
//    query.bindValue(":frameIndex", currentFrameIndex);
//    query.bindValue(":indexNo",    currentIndexNo);

//    if (!query.exec()) {
//        qDebug() << "Insert failed:" << query.lastError().text();
//        return;
//    }
//}

//void SQLite::insertEntities()
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "INSERT INTO entities (name, id, created) VALUES (:name, :id, :created)"
//        );

//    for (const auto& platform : *m_Platforms) {
//        query.bindValue(":name", platform.second->Name.c_str());
//        query.bindValue(":id",   platform.first.c_str());
//        query.bindValue(":created", 0);
//        if (!query.exec()) {
//            qCritical() << "Insertion failed:" << query.lastError().text();
//        } else {
//            entities.insert(platform.first);
//            qDebug() << "Insertion successful!";
//        }
//    }
//}

//void SQLite::insertEntity(std::string id, std::string name, qint64 created)
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "INSERT INTO entities (name, id, created) VALUES (:name, :id, :created)"
//        );
//    query.bindValue(":name",   id.c_str());
//    query.bindValue(":id",     name.c_str());
//    query.bindValue(":created",created);
//    if (!query.exec()) {
//        qCritical() << "Insertion Creation Time failed:" << query.lastError().text();
//    } else {
//        qDebug() << "Insertion Creation Time successful!";
//    }
//}

//void SQLite::insertEntityDeletion(std::string id, qint64 deleted)
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "INSERT INTO entities (deleted) VALUES (:deleted) WHERE id = :id"
//        );
//    query.bindValue(":name",   id.c_str());
//    query.bindValue(":deleted",deleted);
//    if (!query.exec()) {
//        qCritical() << "Insertion Deletion Time failed:" << query.lastError().text();
//    } else {
//        qDebug() << "Insertion Deletion Time successful!";
//    }
//}

//std::vector<std::pair<std::string, int> > *SQLite::Entities()
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "SELECT * FROM entities"
//        );
//    if (query.exec() && query.next()){
//    }
//        //return query.value(0).toInt();

//    return nullptr;
//}



//
// -------------------- FRAMES --------------------
//


//void SQLite::insertFrames(int frameIndex)
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "INSERT INTO frames ("
//        "longitude, latitude, altitude, heading,"
//        "turn_radius, curr_speed, climb_rate,"
//        "frameIndex, indexNo) "
//        "VALUES ("
//        ":longitude, :latitude, :altitude, :heading,"
//        ":turn_radius, :curr_speed, :climb_rate,"
//        ":frameIndex, :indexNo)"
//        );

//    for (const auto& platform : *m_Platforms) {

//        int indexNo = getIndexNoFromId(
//            QString::fromStdString(platform.first)
//            );

//        if (indexNo == -1)
//            continue;

//        query.bindValue(":longitude", platform.second->transform->getLongitude());
//        query.bindValue(":latitude",  platform.second->transform->getLatitude());
//        query.bindValue(":altitude",  platform.second->transform->getAltitude());
//        query.bindValue(":heading",   platform.second->transform->getHeading());
//        query.bindValue(":turn_radius", platform.second->dynamicModel->turnRate);
//        query.bindValue(":curr_speed",  platform.second->dynamicModel->currentSpeed);
//        query.bindValue(":climb_rate",  platform.second->dynamicModel->climbRate);
//        query.bindValue(":frameIndex",  frameIndex);
//        query.bindValue(":indexNo",     indexNo);

//        query.exec();
//    }
//}
//int SQLite::getIndexNoFromId(const QString &id)
//{
//    QSqlQuery query(database());
//    query.prepare(
//        "SELECT indexNo FROM entities WHERE id = :id"
//        );
//    query.bindValue(":id", id);

//    if (query.exec() && query.next())
//        return query.value(0).toInt();

//    return -1;
//}

//std::unordered_map<int,std::pair<std::string, std::string>> SQLite::getEntities()
//{
//    std::unordered_map<int,std::pair<std::string, std::string>> entitiesMap;
//    //std::vector<std::pair<std::string, int>> result;

//    if (!m_db.isOpen()) {
//        qCritical() << "Database is not open!";
//        return entitiesMap;
//    }

//    QSqlQuery query(m_db);   // ✅ USE YOUR CONNECTION

//    if (!query.exec("SELECT name, indexNo, id FROM entities")) {
//        qCritical() << "Query failed:" << query.lastError().text();
//        return entitiesMap;
//    }

//    while (query.next()) {
//        std::string name = query.value(0).toString().toStdString();
//        int indexNo      = query.value(1).toInt();
//        std::string id   = query.value(2).toString().toStdString();
//        std::pair<std::string, std::string> temp;
//        temp.first  = id;
//        temp.second = name;
//        //result.emplace_back(name, indexNo);
//        entitiesMap.insert({indexNo, temp});
//    }

//    return entitiesMap;
//}

//std::vector<qint64> SQLite::getFrameMap()
//{
//    std::vector<qint64> frameMap;

//    if (!m_db.isOpen()) {
//        qCritical() << "Database is not open!";
//        return frameMap;
//    }

//    QSqlQuery query(m_db);   // ✅ USE YOUR CONNECTION

//    if (!query.exec("SELECT timestamp FROM frameMap")) {
//        qCritical() << "Query failed:" << query.lastError().text();
//        return frameMap;
//    }

//    while (query.next()) {
//        qint64 timestamp   = query.value(0).toLongLong();
//        frameMap.push_back(timestamp);
//    }
//    return frameMap;
//}


//std::unordered_map<int, entity> SQLite::getFrameByFrameIndex(int s_frameIndex)
//{
//    std::unordered_map<int, entity> frame;
//    if (!m_db.isOpen()) {
//        qCritical() << "Database is not open!";
//        return frame;
//    }
//    QSqlQuery query(m_db);   // ✅ USE YOUR CONNECTION
//    query.prepare(R"(
//        SELECT indexNo,
//               longitude, latitude, altitude, heading,
//               turn_radius, curr_speed, climb_rate
//        FROM frames
//        WHERE frameIndex = :s_frameIndex
//    )");

//    query.bindValue(":s_frameIndex", s_frameIndex);

//    if (!query.exec()) {
//        qCritical() << "Query failed:" << query.lastError().text();
//        return frame;
//    }
//    int i = 0;
//    while (query.next()) {
//        int indexNo       = query.value(0).toInt();
//        entity m_entity;
//        m_entity.longitude = query.value(1).toDouble();
//        m_entity.latitude  = query.value(2).toDouble();
//        m_entity.altitude  = query.value(3).toDouble();
//        m_entity.heading   = query.value(4).toDouble();
//        m_entity.turn_radius  = query.value(5).toFloat();
//        m_entity.curr_speed   = query.value(6).toFloat();
//        m_entity.climb_rate   = query.value(7).toFloat();
//        frame.insert({indexNo, m_entity});
//        i++;
//    }
//    //qDebug()<<"Frame is send No of Entity:"<<i;
//    return frame;
//}

//void SQLite::showFrameByFrameIndex(int s_frameIndex)
//{
//    std::unordered_map<int, entity> frame;
//    if (!m_db.isOpen()) {
//        qCritical() << "Database is not open!";
//        return ;
//    }
//    QSqlQuery query(m_db);   // ✅ USE YOUR CONNECTION
//    query.prepare(R"(
//        SELECT indexNo,
//               longitude, latitude, altitude, heading,
//               turn_radius, curr_speed, climb_rate
//        FROM frames
//        WHERE frameIndex = :s_frameIndex
//    )");

//    query.bindValue(":s_frameIndex", s_frameIndex);

//    if (!query.exec()) {
//        qCritical() << "Query failed:" << query.lastError().text();
//        return ;
//    }
//    int i = 0;
//    while (query.next()) {
//        int indexNo           = query.value(0).toInt();
//        entity m_entity;
//        m_entity.longitude    = query.value(1).toDouble();
//        m_entity.latitude     = query.value(2).toDouble();
//        m_entity.altitude     = query.value(3).toDouble();
//        m_entity.heading      = query.value(4).toDouble();
//        m_entity.turn_radius  = query.value(5).toFloat();
//        m_entity.curr_speed   = query.value(6).toFloat();
//        m_entity.climb_rate   = query.value(7).toFloat();
//        frame.insert({indexNo, m_entity});
//        i++;
//    }
//    qDebug()<<"Frame is send No of Entity:"<<i;
//    return ;
//}



//
// -------------------- PUBLIC API --------------------
//
//void SQLite::insertFrame(qint64 timestamp)
//{
//    return;
//}

// -------------------- Replay----------------------------
// Start
// End

