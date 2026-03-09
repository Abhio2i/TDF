
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
    /*exec(R"(
        CREATE TABLE entities (
            indexNo  INTEGER PRIMARY KEY AUTOINCREMENT,
            name     TEXT,
            id       TEXT UNIQUE,
            profile  BOOLEAN,
            created  BIGINT,
            deleted  BIGINT
        )
    )");*/

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


    /*exec(R"(
        CREATE TABLE frames (
            changeNo    INTEGER PRIMARY KEY AUTOINCREMENT,
            longitude   DOUBLE,
            latitude    DOUBLE,
            altitude    DOUBLE,
            heading     DOUBLE,
            turn_radius FLOAT,
            curr_speed  FLOAT,
            climb_rate  FLOAT,
            frameIndex  INTEGER,
            indexNo     INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entities(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        )
    )");*/


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

    m_str += "\n  Insert_Entities_Details: ";
    if((m_payLoad.entitiesDetailsList.size() > 0) && (insertEntitiesDetails(m_payLoad.entitiesDetailsList)))                      {
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

    qDebug()<<m_str;
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
bool SQLite::insertEntitiesDetails(EntitiesDetailsList m_entitiesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO entitiesDetails
        (indexNo, name, id, parentID, profile)
        VALUES (:indexNo, :name, :id, :parentID, :profile)
       )");
    for(auto i : m_entitiesDetailsList){
        query.bindValue(":indexNo"  ,i.index);
        query.bindValue(":name"     ,i.name);
        query.bindValue(":id"       ,i.ID);
        query.bindValue(":parentID" ,i.parentID);
        query.bindValue(":profile" ,true);
        // str = QString(
        //           "Index No: %1  "
        //           "Name: %2  "
        //           "ID: %3  "
        //           "Parent ID: %4  "
        //           "Profile: %5  ").arg(
        //         QString::number(i.index),
        //         i.name,
        //         i.ID,
        //         i.parentID,
        //         QString::number(true));

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

