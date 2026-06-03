#include "sqlite.h"
#include <QJsonDocument>
#include <QJsonObject>

SQLite::SQLite(Hierarchy* hierarchy,
               QObject *parent)
    : QObject(parent),
    m_hierarchy(hierarchy)
{

}

SQLite::~SQLite()
{
    dbStatus = DISCONNECTED;
    close();
}

bool SQLite::open(const QString &dbPath)
{
    QDir dir = QFileInfo(dbPath).absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        qCritical() << "Cannot create directory:" << dir.absolutePath();
        return false;
    }
    if (m_db.isValid()) {
        if (m_db.isOpen())
            m_db.close();
        m_db = QSqlDatabase();
    }

    if (QSqlDatabase::contains("main_connection"))
        QSqlDatabase::removeDatabase("main_connection");

   m_db = QSqlDatabase::addDatabase("QSQLITE", "main_connection");
    m_db.setDatabaseName(dbPath);

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
    if (m_db.isOpen()) {
        m_db.close();
    }

    m_db = QSqlDatabase();

    if (QSqlDatabase::contains("main_connection")) {
        QSqlDatabase::removeDatabase("main_connection");
    }

    dbStatus = DISCONNECTED;
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

void SQLite::dbInit()
{
    str = "Step1: Database Init \n";

    QString buildPath = QCoreApplication::applicationDirPath();
    QString dbPath = buildPath + "/simulation.db";
    close();
    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
        str += "Step2: Old DB removed\n";
    }

    if (!open(dbPath))
        return;
    str += "Step3: DB Location"+dbPath;
    debug(str,D_INIT);
    createTables();
}

void SQLite::dbInit(QString path, bool &isFileSaved)
{
    str = "Step1: Database Init \n";
    close();

    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        qCritical() << "Failed to create directory for database:" << dir.absolutePath();
        isFileSaved = false;
        return;
    }

    if (QFile::exists(path)) {
        QFile::remove(path);
        str += "Step2: Old DB removed\n";
    }

    if (!open(path)) {
        isFileSaved = false;
        return;
    }
    str += "Step3: DB Location: " + path;
    debug(str, D_INIT);

    if (!createTables()) {
        qCritical() << "Table creation failed – recording cannot proceed";
        close();
        isFileSaved = false;
        return;
    }

    isFileSaved = true;
}
void SQLite::dbConnect()
{
    str = "Database Connect";
    QString buildPath = QCoreApplication::applicationDirPath();
    QString dbPath = buildPath + "/simulation.db";
    close();
    if (QFile::exists(dbPath)) {
        str += "DB Exist";
    }
    debug(str,D_Connect);
    if (!open(dbPath))
        return;
}

void SQLite::dbConnect(QString path, bool &isFileLoaded)
{
    str = "Database Connect...";
    close();
    if (QFile::exists(path)) {
        str += "DB Exist";
        isFileLoaded = true;
    }
    debug(str,D_Connect);
    if (!open(path)){
        isFileLoaded = false;
        return;
    }
}

SQLite *SQLite::getSQLite()
{
    return this;
}

//
// -------------------- SCHEMA --------------------
//
bool SQLite::createTables()
{
    auto execCreate = [this](const QString &sql) -> bool {
        QSqlQuery query(m_db);
        if (!query.exec(sql)) {
            qCritical() << "Failed to create table:" << sql;
            qCritical() << query.lastError().text();
            return false;
        }
        return true;
    };

    const QStringList statements = {
        R"(CREATE TABLE IF NOT EXISTS frameMap (
            frameIndex INTEGER PRIMARY KEY,
            timestamp  BIGINT
        ))",

        R"(CREATE TABLE IF NOT EXISTS profileCategoriesDetails (
            indexNo  INTEGER PRIMARY KEY,
            name     TEXT,
            id       TEXT UNIQUE,
            creation INTEGER,
            deletion INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS ProfileCategoriesCRUD (
            operation INTEGER,
            indexNo   INTEGER PRIMARY KEY,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES profileCategoriesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesDetails (
            indexNo  INTEGER PRIMARY KEY,
            name     TEXT,
            id       TEXT UNIQUE,
            parentID TEXT,
            profile  BOOLEAN,
            creation INTEGER,
            deletion INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesCreated (
            frameIndex INTEGER,
            indexNo    INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesUpdated (
            frameIndex  INTEGER,
            indexNo     INTEGER,
            longitude   DOUBLE,
            latitude    DOUBLE,
            altitude    DOUBLE,
            heading     DOUBLE,
            turn_radius FLOAT,
            curr_speed  FLOAT,
            climb_rate  FLOAT,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesDeleted (
            frameIndex INTEGER,
            indexNo    INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesMeshRenderer2D (
            indexNo     INTEGER,
            frameIndex  INTEGER,
            Active      BOOLEAN,
            Sprite      TEXT,
            Texture     TEXT,
            color       TEXT,
            color2      TEXT,
            creation    INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesMeshRenderer2DCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS trajectoryWaypoint (
            indexNo        INTEGER,
            waypoint_index INTEGER,
            frameIndex     INTEGER,
            geo_latitude   DOUBLE,
            geo_longitude  DOUBLE,
            geo_altitude   DOUBLE,
            geo_Heading    DOUBLE,
            vector_x       FLOAT,
            vector_y       FLOAT,
            vector_z       FLOAT,
            speed          DOUBLE,
            sensor         BOOLEAN,
            formation      BOOLEAN,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS entitiesTrajectoryCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            creation  INTEGER,
            deletion  INTEGER,
            FOREIGN KEY (indexNo) REFERENCES entitiesDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS bookmarks (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            note      TEXT,
            timestamp BIGINT
        ))",

        R"(CREATE TABLE IF NOT EXISTS sensorsDetails (
            indexNo   INTEGER PRIMARY KEY,
            id        TEXT UNIQUE,
            type      TEXT,
            state     TEXT,
            creation  INTEGER,
            deletion  INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS sensorsCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES sensorsDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS iffDetails (
            indexNo   INTEGER PRIMARY KEY,
            id        TEXT UNIQUE,
            type      TEXT,
            state     TEXT,
            creation  INTEGER,
            deletion  INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS iffCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES iffDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS radioDetails (
            indexNo   INTEGER PRIMARY KEY,
            id        TEXT UNIQUE,
            type      TEXT,
            state     TEXT,
            creation  INTEGER,
            deletion  INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS radioCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES radioDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))",

        R"(CREATE TABLE IF NOT EXISTS weaponDetails (
            indexNo   INTEGER PRIMARY KEY,
            id        TEXT UNIQUE,
            type      TEXT,
            state     TEXT,
            creation  INTEGER,
            deletion  INTEGER
        ))",

        R"(CREATE TABLE IF NOT EXISTS weaponCRUD (
            operation INTEGER,
            indexNo   INTEGER,
            frameIndex INTEGER,
            FOREIGN KEY (indexNo) REFERENCES weaponDetails(indexNo),
            FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
        ))"
    };

    for (const QString &sql : statements) {
        if (!execCreate(sql))
            return false;
    }
    return true;
}

//
// -------------------- INSERT METHODS --------------------
//

bool SQLite::insertFrameMap(int frameIndex, qint64 timestamp)
{
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

bool SQLite::insertProfileCategoriesDetails(int frameIndex,
                                            ProfileCategoriesDetailsList m_profileCategoriesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO profileCategoriesDetails
        (indexNo, name, id, creation)
        VALUES (:indexNo, :name, :id, :creation)
       )");
    for(auto i : m_profileCategoriesDetailsList){
        query.bindValue(":indexNo"  ,i.index);
        query.bindValue(":name"     ,i.name);
        query.bindValue(":id"       ,i.ID);
        query.bindValue(":creation", frameIndex);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Profile Categories Details failed:"
                        << query.lastError().text();
            return false;
        }
    }
    return true;
}

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
        }
    }
    return true;
}

bool SQLite::insertEntitiesDetails(int frameIndex,
                                   EntitiesDetailsList m_entitiesDetailsList)
{
    QSqlQuery insQ(database()), updQ(database());
    insQ.prepare(R"(
        INSERT OR IGNORE INTO entitiesDetails
        (indexNo, name, id, parentID, profile, creation)
        VALUES (:indexNo, :name, :id, :parentID, :profile, :creation)
    )");
    updQ.prepare(R"(
        UPDATE entitiesDetails SET name = :name WHERE indexNo = :indexNo
    )");

    for (EntitiesDetails i : m_entitiesDetailsList) {
        insQ.bindValue(":indexNo",  i.index);
        insQ.bindValue(":name",     i.name);
        insQ.bindValue(":id",       i.ID);
        insQ.bindValue(":parentID", i.parentID);
        insQ.bindValue(":creation", frameIndex);
        insQ.bindValue(":profile",  true);
        if (!insQ.exec()) {
            qCritical() << "insertEntitiesDetails:" << insQ.lastError().text();
            return false;
        }
        if (insQ.numRowsAffected() == 0) {
            updQ.bindValue(":name",    i.name);
            updQ.bindValue(":indexNo", i.index);
            if (!updQ.exec()) {
                qCritical() << "updateEntitiesDetails name:" << updQ.lastError().text();
                return false;
            }
        }
    }
    return true;
}

bool SQLite::insertEntitiesCreated(int frameIndex, EntitiesCreatedList m_entitiesCreatedList)
{
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
        }
    }
    return true;
}

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
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Updation failed:" << query.lastError().text();
            return false;
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
        }
    }

    query.prepare(R"(
        UPDATE entitiesDetails
        SET deletion = :deletion
        WHERE indexNo = :indexNo
        )");
    for(auto i : m_entitiesDeletedList){
        query.bindValue(":deletion"    ,frameIndex);
        query.bindValue(":indexNo"     ,i.index);
        if (!query.exec()) {
            qCritical() << "Insertion Insert Entities Details Deletion failed:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

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
        }
    }
    return true;
}

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
        }
    }
    return true;
}

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
            }
        }
    }
    return true;
}

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
        }
    }
    return true;
}

// ========== SENSOR INSERT METHODS ==========

bool SQLite::insertSensorsDetails(int frameIndex, const SensorList &list)
{
    QSqlQuery insertQuery(database());
    QSqlQuery updateQuery(database());

    insertQuery.prepare(R"(
        INSERT OR IGNORE INTO sensorsDetails (indexNo, id, type, state, creation)
        VALUES (:indexNo, :id, :type, :state, :creation)
    )");

    updateQuery.prepare(R"(
        UPDATE sensorsDetails SET state = :state WHERE id = :id
    )");

    for (const auto &s : list) {
        // Try insert first (will be ignored if id already exists)
        insertQuery.bindValue(":indexNo",   s.index);
        insertQuery.bindValue(":id",        s.id);
        insertQuery.bindValue(":type",      s.type);
        insertQuery.bindValue(":state",     QJsonDocument(s.state).toJson(QJsonDocument::Compact));
        insertQuery.bindValue(":creation",  frameIndex);
        if (!insertQuery.exec()) {
            qCritical() << "Insert sensorsDetails failed:" << insertQuery.lastError().text();
            return false;
        }

        if (insertQuery.numRowsAffected() == 0) {
            updateQuery.bindValue(":state", QJsonDocument(s.state).toJson(QJsonDocument::Compact));
            updateQuery.bindValue(":id",    s.id);
            if (!updateQuery.exec()) {
                qCritical() << "Update sensorsDetails state failed:" << updateQuery.lastError().text();
                return false;
            }
        }
    }
    return true;
}

bool SQLite::insertSensorsCRUD(int frameIndex, const SensorCRUDList &list)
{
    QSqlQuery query(database());
    query.prepare(R"(
        INSERT INTO sensorsCRUD (operation, indexNo, frameIndex)
        VALUES (:operation, :indexNo, :frameIndex)
    )");
    for (const auto &c : list) {
        query.bindValue(":operation", c.operation);
        query.bindValue(":indexNo",   c.index);
        query.bindValue(":frameIndex",frameIndex);
        if (!query.exec()) {
            qCritical() << "Insert sensorsCRUD failed:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

// ========== END SENSOR INSERT METHODS ==========

//
// -------------------- RECEIVE PAYLOAD (WRITE) --------------------
//

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
    }else{
        m_str += f;
    }

    m_str += "\n  Profile_Categories_Details: ";
    if((m_payLoad.profileCategoriesDetailsList->size() > 0) &&
        (insertProfileCategoriesDetails(m_payLoad.frameIndex,*m_payLoad.profileCategoriesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Profile_Categories_CRUD: ";
    if((m_payLoad.profileCategoriesCRUDList->size() > 0) &&
        (insertProfileCategoriesCRUD(m_payLoad.frameIndex,
                                     *m_payLoad.profileCategoriesCRUDList)))
    {
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Details: ";
    if((m_payLoad.entitiesDetailsList->size() > 0) && (insertEntitiesDetails(
            m_payLoad.frameIndex, *m_payLoad.entitiesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Created: ";
    if((m_payLoad.entitiesCreatedList->size() > 0) && (insertEntitiesCreated(m_payLoad.frameIndex, *m_payLoad.entitiesCreatedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Mesh Renderer List: ";
    if((m_payLoad.entitiesMeshRenderer2DList->size() > 0) &&
        (insertEntitiesMeshRenderer2D(m_payLoad.frameIndex,
                                      *m_payLoad.entitiesMeshRenderer2DList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Mesh Renderer CRUD: ";
    if((m_payLoad.entitiesMeshRenderer2DCRUDList->size() > 0) &&
        (insertEntitiesMeshRenderer2DCRUD(m_payLoad.frameIndex,
                                          *m_payLoad.entitiesMeshRenderer2DCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Updated: ";
    if((m_payLoad.entitiesUpdatedList->size() > 0) && (insertEntitiesUpdated(m_payLoad.frameIndex, *m_payLoad.entitiesUpdatedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Deleted: ";
    if((m_payLoad.entitiesDeletedList->size() > 0) && (insertEntitiesDeleted(m_payLoad.frameIndex, *m_payLoad.entitiesDeletedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Trajectory List: ";
    if((m_payLoad.entitiesTrajectoryList->size() > 0) && (insertEntitiesTrajectory(m_payLoad.frameIndex, *m_payLoad.entitiesTrajectoryList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Trajectory CRUD: ";
    if((m_payLoad.entitiesTrajectoryCRUDList->size() > 0) && (insertEntitiesTrajectoryCRUD(m_payLoad.frameIndex, *m_payLoad.entitiesTrajectoryCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }

    // Sensors
    m_str += "\n  Sensors List: ";
    if((m_payLoad.sensorsList->size() > 0) && (insertSensorsDetails(m_payLoad.frameIndex, *m_payLoad.sensorsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Sensors CRUD: ";
    if((m_payLoad.sensorsCRUDList->size() > 0) && (insertSensorsCRUD(m_payLoad.frameIndex, *m_payLoad.sensorsCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }

    debug(m_str,D_GetPayLoad);
}

/*---------------- Shared Pointer Write Start --------------*/
void SQLite::rec_OnProcessRequested(QSharedPointer<PayLoad> data)
{
    if (!data) return;
    QString m_str = QString(
                        "Receive_PayLoad => "
                        "TimeStamo: %1 ,"
                        "Frame Index: %2 ,").arg(
                            QString::number(data->timestamp),
                            QString::number(data->frameIndex));
    QString p = "PASS";
    QString f = "FAIL";

    m_str += "\n  Frame_Map: ";
    if(!insertFrameMap(data->frameIndex,data->timestamp)){
        m_str += p;
    }else{
        m_str += f;
    }
    m_str += "\n  Profile_Categories_Details: ";
    if((data->profileCategoriesDetailsList->size() > 0) &&
        (insertProfileCategoriesDetails(data->frameIndex,*data->profileCategoriesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Profile_Categories_CRUD: ";
    if((data->profileCategoriesCRUDList->size() > 0) &&
        (insertProfileCategoriesCRUD(data->frameIndex,
                                     *data->profileCategoriesCRUDList)))
    {
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Details: ";
    if((data->entitiesDetailsList->size() > 0) && (insertEntitiesDetails(
            data->frameIndex, *data->entitiesDetailsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Created: ";
    if((data->entitiesCreatedList->size() > 0) && (insertEntitiesCreated(data->frameIndex, *data->entitiesCreatedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Mesh Renderer List: ";
    if((data->entitiesMeshRenderer2DList->size() > 0) &&
        (insertEntitiesMeshRenderer2D(data->frameIndex,
                                      *data->entitiesMeshRenderer2DList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Mesh Renderer CRUD: ";
    if((data->entitiesMeshRenderer2DCRUDList->size() > 0) &&
        (insertEntitiesMeshRenderer2DCRUD(data->frameIndex,
                                          *data->entitiesMeshRenderer2DCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Updated: ";
    if((data->entitiesUpdatedList->size() > 0) && (insertEntitiesUpdated(data->frameIndex, *data->entitiesUpdatedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Insert_Entities_Deleted: ";
    if((data->entitiesDeletedList->size() > 0) && (insertEntitiesDeleted(data->frameIndex, *data->entitiesDeletedList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Trajectory List: ";
    if((data->entitiesTrajectoryList->size() > 0) && (insertEntitiesTrajectory(data->frameIndex, *data->entitiesTrajectoryList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Trajectory CRUD: ";
    if((data->entitiesTrajectoryCRUDList->size() > 0) && (insertEntitiesTrajectoryCRUD(data->frameIndex, *data->entitiesTrajectoryCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }

    // ===== SENSORS =====
    m_str += "\n  Sensors List: ";
    if((data->sensorsList->size() > 0) && (insertSensorsDetails(data->frameIndex, *data->sensorsList))){
        m_str += p;
    }else{
        m_str += f;
    }

    m_str += "\n  Sensors CRUD: ";
    if((data->sensorsCRUDList->size() > 0) && (insertSensorsCRUD(data->frameIndex, *data->sensorsCRUDList))){
        m_str += p;
    }else{
        m_str += f;
    }
    // IFF
    if (data->iffList->size() > 0)
        insertIffDetails(data->frameIndex, *data->iffList);
    if (data->iffCRUDList->size() > 0)
        insertIffCRUD(data->frameIndex, *data->iffCRUDList);

    // Radio
    if (data->radioList->size() > 0)
        insertRadioDetails(data->frameIndex, *data->radioList);
    if (data->radioCRUDList->size() > 0)
        insertRadioCRUD(data->frameIndex, *data->radioCRUDList);

    // Weapon
    if (data->weaponList->size() > 0)
        insertWeaponDetails(data->frameIndex, *data->weaponList);
    if (data->weaponCRUDList->size() > 0)
        insertWeaponCRUD(data->frameIndex, *data->weaponCRUDList);
    if (data->iffList && data->iffList->size() > 0)
        insertIffDetails(data->frameIndex, *data->iffList);
    if (data->iffCRUDList && data->iffCRUDList->size() > 0)
        insertIffCRUD(data->frameIndex, *data->iffCRUDList);

    // Radio
    if (data->radioList && data->radioList->size() > 0)
        insertRadioDetails(data->frameIndex, *data->radioList);
    if (data->radioCRUDList && data->radioCRUDList->size() > 0)
        insertRadioCRUD(data->frameIndex, *data->radioCRUDList);

    // Weapon
    if (data->weaponList && data->weaponList->size() > 0)
        insertWeaponDetails(data->frameIndex, *data->weaponList);
    if (data->weaponCRUDList && data->weaponCRUDList->size() > 0)
        insertWeaponCRUD(data->frameIndex, *data->weaponCRUDList);
    debug(m_str,D_GetPayLoad);
    emit rec_ProcessingFinished(data);
}
void SQLite::setPayLoad(PayLoad &payLoad)
{
    payLoad.sensorsList->clear();
    payLoad.sensorsCRUDList->clear();

    // qDebug() << "[SQLITE READ] frameIndex=" << payLoad.frameIndex
    //          << " sensorsList size=" << payLoad.sensorsList->size()
    //          << " sensorsCRUD size=" << payLoad.sensorsCRUDList->size();
    str = QString(
              "Received PayLoad => \n"
              "\t FrameIndex: %1 \n").arg(QString::number(payLoad.frameIndex));

    setTimeStamp(payLoad.frameIndex, payLoad.timestamp);
    str += QString(
               "\t TimeStamp: %1 \n").arg(QString::number(payLoad.timestamp));

    if(payLoad.entitiesDetailsList->empty()){
        setEntitiesDetailsList(*payLoad.entitiesDetailsList);
        str += QString(
                   "\t Entities Details List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesDetailsList->size()));
    }
    if(payLoad.entitiesTrajectoryList->empty()){
        setProfileCategoriesDetails(*payLoad.profileCategoriesDetailsList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryList->size()));
    }
    if(payLoad.profileCategoriesCRUDList->empty()){
        setProfileCategoriesCRUD(payLoad.frameIndex, *payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesCRUDList->size()));
    }
    if(payLoad.profileCategoriesDetailsList->empty()){
        setProfileCategoriesDetails(*payLoad.profileCategoriesDetailsList);
        str += QString(
                   "\t Profile Categories Details List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesDetailsList->size()));
    }
    if(payLoad.profileCategoriesCRUDList->empty()){
        setProfileCategoriesCRUD(payLoad.frameIndex, *payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.profileCategoriesCRUDList->size()));
    }
    if(payLoad.entitiesCreatedList->empty()){
        setEntitiesCreatedList(payLoad.frameIndex, *payLoad.entitiesCreatedList);
        str += QString(
                   "\t Entities Created List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesCreatedList->size()));
    }
    if(payLoad.entitiesUpdatedList->empty()){
        setEntitiesUpdatedList(payLoad.frameIndex, *payLoad.entitiesUpdatedList);
        str += QString(
                   "\t Entities Updated List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesUpdatedList->size()));
    }
    if(payLoad.entitiesDeletedList->empty()){
        setEntitiesDeletedList(payLoad.frameIndex, *payLoad.entitiesDeletedList);
        str += QString(
                   "\t Entities Deleted List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesDeletedList->size()));
    }
    if(payLoad.entitiesMeshRenderer2DList->empty()){
        setEntitiesMeshRenderer2D(payLoad.frameIndex,    *payLoad.entitiesMeshRenderer2DList);
        str += QString(
                   "\t Entities Mesh Renderer List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesMeshRenderer2DList->size()));
    }
    if(payLoad.entitiesMeshRenderer2DCRUDList->empty()){
        setEntitiesMeshRenderer2DCRUD(payLoad.frameIndex, *payLoad.entitiesMeshRenderer2DCRUDList);
        str += QString(
                   "\t Entities Mesh Renderer CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesMeshRenderer2DCRUDList->size()));
    }
    if(payLoad.entitiesTrajectoryList->empty()){
        setEntitiesTrajectory(payLoad.frameIndex, *payLoad.entitiesTrajectoryList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryList->size()));
    }
    if(payLoad.entitiesTrajectoryCRUDList->empty()){
        setEntitiesTrajectoryCRUD(payLoad.frameIndex, *payLoad.entitiesTrajectoryCRUDList);
        str += QString(
                   "\t Entities Trajectory CRUD List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryCRUDList->size()));
    }

    // ===== READ SENSORS — always unconditional =====
    setSensorsDetails(payLoad.frameIndex, *payLoad.sensorsList);
    str += QString(
               "\t Sensors List Size: %1 \n")
               .arg(QString::number(payLoad.sensorsList->size()));

    setSensorsCRUD(payLoad.frameIndex, *payLoad.sensorsCRUDList);
    str += QString(
               "\t Sensors CRUD Size: %1 \n")
               .arg(QString::number(payLoad.sensorsCRUDList->size()));

    debug(str,D_SetPayLoad);
}

void SQLite::receivePayLoadFrameIndex(int frameIndex)
{
    PayLoad m_payLoad;
    // +++ CLEAR SENSORS LISTS +++
    m_payLoad.sensorsList->clear();
    m_payLoad.sensorsCRUDList->clear();
    str = QString(
              "Received PayLoad => \n"
              "\t FrameIndex: %1 \n").arg(QString::number(frameIndex));

    m_payLoad.frameIndex = frameIndex;
    setTimeStamp(m_payLoad.frameIndex, m_payLoad.timestamp);
    str += QString(
               "\t TimeStamp: %1 \n").arg(QString::number(m_payLoad.timestamp));
    if(m_payLoad.entitiesDetailsList->empty()){
        setEntitiesDetailsList(*m_payLoad.entitiesDetailsList);
        str += QString(
                   "\t Entities Details List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesDetailsList->size()));
    }
    if(m_payLoad.profileCategoriesCRUDList->empty()){
        setProfileCategoriesCRUD(m_payLoad.frameIndex, *m_payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(m_payLoad.profileCategoriesCRUDList->size()));
    }
    if(m_payLoad.profileCategoriesDetailsList->empty()){
        setProfileCategoriesDetails(*m_payLoad.profileCategoriesDetailsList);
        str += QString(
                   "\t Profile Categories Details List Size: %1 \n")
                   .arg(QString::number(m_payLoad.profileCategoriesDetailsList->size()));
    }
    if(m_payLoad.profileCategoriesCRUDList->empty()){
        setProfileCategoriesCRUD(m_payLoad.frameIndex, *m_payLoad.profileCategoriesCRUDList);
        str += QString(
                   "\t Profile Categories CRUD List Size: %1 \n")
                   .arg(QString::number(m_payLoad.profileCategoriesCRUDList->size()));
    }
    if(m_payLoad.entitiesCreatedList->empty()){
        setEntitiesCreatedList(m_payLoad.frameIndex, *m_payLoad.entitiesCreatedList);
        str += QString(
                   "\t Entities Created List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesCreatedList->size()));
    }
    if(m_payLoad.entitiesUpdatedList->empty()){
        setEntitiesUpdatedList(m_payLoad.frameIndex, *m_payLoad.entitiesUpdatedList);
        str += QString(
                   "\t Entities Updated List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesUpdatedList->size()));
    }
    if(m_payLoad.entitiesDeletedList->empty()){
        setEntitiesDeletedList(m_payLoad.frameIndex, *m_payLoad.entitiesDeletedList);
        str += QString(
                   "\t Entities Deleted List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesDeletedList->size()));
    }
    if(m_payLoad.entitiesMeshRenderer2DList->empty()){
        setEntitiesMeshRenderer2D(m_payLoad.frameIndex,    *m_payLoad.entitiesMeshRenderer2DList);
        str += QString(
                   "\t Entities Mesh Renderer List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesMeshRenderer2DList->size()));
    }
    if(m_payLoad.entitiesMeshRenderer2DCRUDList->empty()){
        setEntitiesMeshRenderer2DCRUD(m_payLoad.frameIndex, *m_payLoad.entitiesMeshRenderer2DCRUDList);
        str += QString(
                   "\t Entities Mesh Renderer CRUD List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesMeshRenderer2DCRUDList->size()));
    }
    if(m_payLoad.entitiesTrajectoryList->empty()){
        setEntitiesTrajectory(m_payLoad.frameIndex, *m_payLoad.entitiesTrajectoryList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesTrajectoryList->size()));
    }
    if(m_payLoad.entitiesTrajectoryCRUDList->empty()){
        setEntitiesTrajectoryCRUD(m_payLoad.frameIndex, *m_payLoad.entitiesTrajectoryCRUDList);
        str += QString(
                   "\t Entities Trajectory CRUD List Size: %1 \n")
                   .arg(QString::number(m_payLoad.entitiesTrajectoryCRUDList->size()));
    }
    // sensors — always unconditional
    setSensorsDetails(m_payLoad.frameIndex, *m_payLoad.sensorsList);
    setSensorsCRUD(m_payLoad.frameIndex, *m_payLoad.sensorsCRUDList);
    sendPayLoadByFrameIndex(m_payLoad);
    debug(str, D_FrameIndex);
}

void SQLite::setPayLoadFromIndex(PayLoad &payLoad, int frameIndex)
{
    payLoad.sensorsList->clear();
    payLoad.sensorsCRUDList->clear();
    str = QString(
              "Received PayLoad => \n"
              "\t FrameIndex: %1 \n").arg(QString::number(payLoad.frameIndex));

    setTimeStamp(payLoad.frameIndex, payLoad.timestamp);
    str += QString(
               "\t TimeStamp: %1 \n").arg(QString::number(payLoad.timestamp));
    if(payLoad.entitiesDetailsList->empty()){
        setEntitiesDetailsListInBtw(frameIndex,*payLoad.entitiesDetailsList);
        str += QString(
                   "\t Entities Details List Inbetween Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesDetailsList->size()));
    }
    if(payLoad.entitiesMeshRenderer2DList->empty()){
        setEntitiesMeshRenderer2DInBtw(payLoad.frameIndex,    *payLoad.entitiesMeshRenderer2DList);
        str += QString(
                   "\t Entities Mesh Renderer List in Between Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesMeshRenderer2DList->size()));
    }
    if(payLoad.entitiesTrajectoryCRUDList->empty()){
        setEntitiesTrajectoryBtw(payLoad.frameIndex, *payLoad.entitiesTrajectoryList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payLoad.entitiesTrajectoryList->size()));
    }

    // FIX: sensors always read unconditionally (they live at creation=0)
    setSensorsDetails(payLoad.frameIndex, *payLoad.sensorsList);
    str += QString("\t Sensors List Size: %1 \n")
               .arg(QString::number(payLoad.sensorsList->size()));
    setSensorsCRUD(payLoad.frameIndex, *payLoad.sensorsCRUDList);
    str += QString("\t Sensors CRUD Size: %1 \n")
               .arg(QString::number(payLoad.sensorsCRUDList->size()));
    debug(str,D_SetPayLoad);
}

// FIX: sensors now always read unconditionally in setPayLoadFromIndex(QSharedPointer)
void SQLite::setPayLoadFromIndex(QSharedPointer<PayLoad> payload, int frameIndex)
{
    if (!payload) return;
    // +++ CLEAR SENSORS LISTS +++
    payload->sensorsList->clear();
    payload->sensorsCRUDList->clear();
    str = QString(
              "Received PayLoad => \n"
              "\t FrameIndex: %1 \n").arg(QString::number(payload->frameIndex));

    setTimeStamp(payload->frameIndex, payload->timestamp);
    str += QString(
               "\t TimeStamp: %1 \n").arg(QString::number(payload->timestamp));
    if(payload->entitiesDetailsList && payload->entitiesDetailsList->empty()){
        setEntitiesDetailsListInBtw(frameIndex,*payload->entitiesDetailsList);
        str += QString(
                   "\t Entities Details List Inbetween Size: %1 \n")
                   .arg(QString::number(payload->entitiesDetailsList->size()));
    }
    if(payload->entitiesMeshRenderer2DList && payload->entitiesMeshRenderer2DList->empty()){
        setEntitiesMeshRenderer2DInBtw(payload->frameIndex, *payload->entitiesMeshRenderer2DList);
        str += QString(
                   "\t Entities Mesh Renderer List in Between Size: %1 \n")
                   .arg(QString::number(payload->entitiesMeshRenderer2DList->size()));
    }
    if(payload->entitiesTrajectoryCRUDList && payload->entitiesTrajectoryCRUDList->empty()){
        setEntitiesTrajectoryBtw(payload->frameIndex, *payload->entitiesTrajectoryList);
        str += QString(
                   "\t Entities Trajectory List Size: %1 \n")
                   .arg(QString::number(payload->entitiesTrajectoryList->size()));
    }

    if(payload->sensorsList){
        payload->sensorsList->clear();
        setSensorsDetails(payload->frameIndex, *payload->sensorsList);
        str += QString("\t Sensors List Size: %1 \n")
                   .arg(QString::number(payload->sensorsList->size()));
    }
    if(payload->sensorsCRUDList){
        payload->sensorsCRUDList->clear();
        setSensorsCRUD(payload->frameIndex, *payload->sensorsCRUDList);
        str += QString("\t Sensors CRUD Size: %1 \n")
                   .arg(QString::number(payload->sensorsCRUDList->size()));
    }
    if (payload->iffList) {
        payload->iffList->clear();
        setIffDetails(frameIndex, *payload->iffList);
    }
    if (payload->iffCRUDList) {
        payload->iffCRUDList->clear();
        setIffCRUD(frameIndex, *payload->iffCRUDList);
    }
    if (payload->radioList) {
        payload->radioList->clear();
        setRadioDetails(frameIndex, *payload->radioList);
    }
    if (payload->radioCRUDList) {
        payload->radioCRUDList->clear();
        setRadioCRUD(frameIndex, *payload->radioCRUDList);
    }
    if (payload->weaponList) {
        payload->weaponList->clear();
        setWeaponDetails(frameIndex, *payload->weaponList);
    }
    if (payload->weaponCRUDList) {
        payload->weaponCRUDList->clear();
        setWeaponCRUD(frameIndex, *payload->weaponCRUDList);
    }
    debug(str, D_GetPacket);
}

void SQLite::setEntitiesDetailsListInBtw(
    int frameIndex, EntitiesDetailsList &entitiesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo, name, id, parentID, profile
        FROM entitiesDetails
        WHERE creation <= :frameIndex
          AND (deletion IS NULL OR deletion >= :frameIndex);
    )");
    query.bindValue(":frameIndex" ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }
    while (query.next()) {
        EntitiesDetails entitiesDetails;
        entitiesDetails.index    = query.value(0).toInt();
        entitiesDetails.name     = query.value(1).toString();
        entitiesDetails.ID       = query.value(2).toString();
        entitiesDetails.parentID = query.value(3).toString();
        entitiesDetailsList.push_back(entitiesDetails);
    }
}

void SQLite::setEntitiesMeshRenderer2DInBtw(
    int frameIndex,
    EntitiesMeshRenderer2DList &entitiesMeshRenderer2DList)
{
    QSqlQuery query(database());
    query.prepare(R"(
    SELECT t.indexNo, t.Active, t.Sprite, t.Texture, t.color, t.color2
    FROM entitiesMeshRenderer2D t
    JOIN (
        SELECT indexNo, MAX(frameIndex) AS maxFrame
        FROM entitiesMeshRenderer2D
        WHERE frameIndex <= :frameIndex
        GROUP BY indexNo
    ) sub
    ON t.indexNo = sub.indexNo
    AND t.frameIndex = sub.maxFrame;
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
        entitiesMeshRenderer2DList.push_back(emr);
    }
}

void SQLite::setEntitiesTrajectoryBtw(int &frameIndex,
                                      EntitiesTrajectoryList &entitiesTrajectoryList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        WITH latest AS (
            SELECT e.indexNo, MAX(t.frameIndex) AS frameIndex
            FROM entitiesDetails e
            JOIN entitiesTrajectoryCRUD t
                ON e.indexNo = t.indexNo
            WHERE e.creation <= :frameIndex
              AND (e.deletion IS NULL OR e.deletion >= :frameIndex)
              AND t.operation = 8
              AND t.frameIndex <= :frameIndex
            GROUP BY e.indexNo
        )

        SELECT DISTINCT
               w.indexNo,
               w.waypoint_index,
               w.geo_latitude, w.geo_longitude, w.geo_altitude, w.geo_Heading,
               w.vector_x, w.vector_y, w.vector_z,
               w.speed, w.sensor, w.formation
        FROM trajectoryWaypoint w
        JOIN latest l
          ON w.indexNo = l.indexNo
         AND w.frameIndex = l.frameIndex
        ORDER BY w.indexNo ASC, w.waypoint_index ASC;
    )");
    query.bindValue(":frameIndex"  ,frameIndex);
    if (!query.exec()) {
        qCritical() << query.lastError().text();
        return;
    }
    std::unordered_map <int ,EntitiesTrajectory> entitiesTrajectoryIndex;
    while (query.next()) {
        TrajectoryWaypoint tw;
        int index        =  query.value(0).toInt();
        tw.index         =  query.value(1 ).toInt();
        tw.geo_latitude  =  query.value(2 ).toDouble();
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

void SQLite::rep_OnProcessRequested(QSharedPointer<PayLoad> payLoad)
{
    if (!payLoad) return;

    payLoad->sensorsList->clear();
    payLoad->sensorsCRUDList->clear();

    str = QString("Received PayLoad => \n\t FrameIndex: %1 \n")
              .arg(QString::number(payLoad->frameIndex));

    setTimeStamp(payLoad->frameIndex, payLoad->timestamp);
    str += QString("\t TimeStamp: %1 \n")
               .arg(QString::number(payLoad->timestamp));

    if (payLoad->entitiesDetailsList->empty()) {
        setEntitiesDetailsList(*payLoad->entitiesDetailsList);
        str += QString("\t Entities Details List Size: %1 \n")
                   .arg(payLoad->entitiesDetailsList->size());
    }
    if (payLoad->entitiesTrajectoryList->empty()) {
        setProfileCategoriesDetails(*payLoad->profileCategoriesDetailsList);
        str += QString("\t Entities Trajectory List Size: %1 \n")
                   .arg(payLoad->entitiesTrajectoryList->size());
    }
    if (payLoad->profileCategoriesCRUDList->empty()) {
        setProfileCategoriesCRUD(payLoad->frameIndex, *payLoad->profileCategoriesCRUDList);
        str += QString("\t Profile Categories CRUD List Size: %1 \n")
                   .arg(payLoad->profileCategoriesCRUDList->size());
    }
    if (payLoad->profileCategoriesDetailsList->empty()) {
        setProfileCategoriesDetails(*payLoad->profileCategoriesDetailsList);
        str += QString("\t Profile Categories Details List Size: %1 \n")
                   .arg(payLoad->profileCategoriesDetailsList->size());
    }
    if (payLoad->profileCategoriesCRUDList->empty()) {
        setProfileCategoriesCRUD(payLoad->frameIndex, *payLoad->profileCategoriesCRUDList);
        str += QString("\t Profile Categories CRUD List Size: %1 \n")
                   .arg(payLoad->profileCategoriesCRUDList->size());
    }
    if (payLoad->entitiesCreatedList->empty()) {
        setEntitiesCreatedList(payLoad->frameIndex, *payLoad->entitiesCreatedList);
        str += QString("\t Entities Created List Size: %1 \n")
                   .arg(payLoad->entitiesCreatedList->size());
    }
    if (payLoad->entitiesUpdatedList->empty()) {
        setEntitiesUpdatedList(payLoad->frameIndex, *payLoad->entitiesUpdatedList);
        str += QString("\t Entities Updated List Size: %1 \n")
                   .arg(payLoad->entitiesUpdatedList->size());
    }
    if (payLoad->entitiesDeletedList->empty()) {
        setEntitiesDeletedList(payLoad->frameIndex, *payLoad->entitiesDeletedList);
        str += QString("\t Entities Deleted List Size: %1 \n")
                   .arg(payLoad->entitiesDeletedList->size());
    }
    if (payLoad->entitiesMeshRenderer2DList->empty()) {
        setEntitiesMeshRenderer2D(payLoad->frameIndex, *payLoad->entitiesMeshRenderer2DList);
        str += QString("\t Entities Mesh Renderer List Size: %1 \n")
                   .arg(payLoad->entitiesMeshRenderer2DList->size());
    }
    if (payLoad->entitiesMeshRenderer2DCRUDList->empty()) {
        setEntitiesMeshRenderer2DCRUD(payLoad->frameIndex, *payLoad->entitiesMeshRenderer2DCRUDList);
        str += QString("\t Entities Mesh Renderer CRUD List Size: %1 \n")
                   .arg(payLoad->entitiesMeshRenderer2DCRUDList->size());
    }
    if (payLoad->entitiesTrajectoryList->empty()) {
        setEntitiesTrajectory(payLoad->frameIndex, *payLoad->entitiesTrajectoryList);
        str += QString("\t Entities Trajectory List Size: %1 \n")
                   .arg(payLoad->entitiesTrajectoryList->size());
    }
    if (payLoad->entitiesTrajectoryCRUDList->empty()) {
        setEntitiesTrajectoryCRUD(payLoad->frameIndex, *payLoad->entitiesTrajectoryCRUDList);
        str += QString("\t Entities Trajectory CRUD List Size: %1 \n")
                   .arg(payLoad->entitiesTrajectoryCRUDList->size());
    }

    // ===== SENSORS – always read (no guard) =====
    setSensorsDetails(payLoad->frameIndex, *payLoad->sensorsList);
    str += QString("\t Sensors List Size: %1 \n")
               .arg(payLoad->sensorsList->size());

    setSensorsCRUD(payLoad->frameIndex, *payLoad->sensorsCRUDList);
    str += QString("\t Sensors CRUD Size: %1 \n")
               .arg(payLoad->sensorsCRUDList->size());

    if (payLoad->iffList && payLoad->iffList->size() > 0)
        insertIffDetails(payLoad->frameIndex, *payLoad->iffList);
    if (payLoad->iffCRUDList && payLoad->iffCRUDList->size() > 0)
        insertIffCRUD(payLoad->frameIndex, *payLoad->iffCRUDList);

    // Radio
    if (payLoad->radioList && payLoad->radioList->size() > 0)
        insertRadioDetails(payLoad->frameIndex, *payLoad->radioList);
    if (payLoad->radioCRUDList && payLoad->radioCRUDList->size() > 0)
        insertRadioCRUD(payLoad->frameIndex, *payLoad->radioCRUDList);

    // Weapon
    if (payLoad->weaponList && payLoad->weaponList->size() > 0)
        insertWeaponDetails(payLoad->frameIndex, *payLoad->weaponList);
    if (payLoad->weaponCRUDList && payLoad->weaponCRUDList->size() > 0)
        insertWeaponCRUD(payLoad->frameIndex, *payLoad->weaponCRUDList);

    debug(str, D_SetPayLoad);
    emit rep_ProcessingFinished(payLoad);
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
        pcd.ID       = query.value(2).toString();
        profileCategoriesDetailsList.push_back(pcd);
    }
}

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

void SQLite::setEntitiesDetailsList(
    EntitiesDetailsList &entitiesDetailsList)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo, name, id, parentID
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
        entitiesDetails.ID       = query.value(2).toString();
        entitiesDetails.parentID = query.value(3).toString();
        entitiesDetailsList.push_back(entitiesDetails);
    }
}

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
        entitiesMeshRenderer2DList.push_back(emr);
    }
}

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

void SQLite::setEntitiesTrajectory(int &frameIndex,
                                   EntitiesTrajectoryList &entitiesTrajectoryList)
{
    QSqlQuery query(database());
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
    std::unordered_map <int ,EntitiesTrajectory> entitiesTrajectoryIndex;
    while (query.next()) {
        TrajectoryWaypoint tw;
        int index        =  query.value(0).toInt();
        tw.index         =  query.value(1 ).toInt();
        tw.geo_latitude  =  query.value(2 ).toDouble();
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

// ========== SENSOR READ METHODS ==========
void SQLite::setSensorsDetails(int frameIndex, SensorList &list)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT indexNo, id, type, state FROM sensorsDetails
        WHERE creation <= :frameIndex
          AND (deletion IS NULL OR deletion >= :frameIndex)
    )");
    query.bindValue(":frameIndex", frameIndex);
    if (!query.exec()) {
        qCritical() << "setSensorsDetails failed:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        SensorData s;
        s.index = query.value(0).toInt();
        s.id    = query.value(1).toString();
        s.type  = query.value(2).toString();
        s.state = QJsonDocument::fromJson(query.value(3).toByteArray()).object();
        list.push_back(s);
    }

}

void SQLite::setSensorsCRUD(int frameIndex, SensorCRUDList &list)
{
    QSqlQuery query(database());
    query.prepare(R"(
        SELECT operation, indexNo FROM sensorsCRUD
        WHERE frameIndex = :frameIndex
    )");
    query.bindValue(":frameIndex", frameIndex);
    if (!query.exec()) {
        qCritical() << "setSensorsCRUD failed:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        SensorCRUD c;
        c.operation = static_cast<Operation>(query.value(0).toInt());
        c.index     = query.value(1).toInt();
        list.push_back(c);
    }
}

// ========== END SENSOR READ METHODS ==========

QList<QPair<QString, qint64>> SQLite::getBookmarks() const
{
    QList<QPair<QString, qint64>> bookmarks;
    if (dbStatus != CONNECTED) {
        qWarning() << "Database not connected, cannot retrieve bookmarks";
        return bookmarks;
    }
    QSqlQuery query("SELECT note, timestamp FROM bookmarks ORDER BY timestamp", m_db);
    while (query.next()) {
        QString note = query.value(0).toString();
        qint64 ts = query.value(1).toLongLong();
        bookmarks.append(qMakePair(note, ts));
    }
    return bookmarks;
}

void SQLite::saveBookmark(const QString &note, qint64 timestamp)
{
    if (dbStatus != CONNECTED) {
        qWarning() << "Database not connected, cannot save bookmark";
        return;
    }
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO bookmarks (note, timestamp) VALUES (?, ?)");
    query.addBindValue(note);
    query.addBindValue(timestamp);
    if (!query.exec()) {
        qWarning() << "Failed to save bookmark:" << query.lastError().text();
    }
}

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
bool SQLite::insertIffDetails(int frameIndex, const IffList &list)
{
    QSqlQuery ins(database()), upd(database());
    ins.prepare(R"(
        INSERT OR IGNORE INTO iffDetails (indexNo, id, type, state, creation)
        VALUES (:indexNo, :id, :type, :state, :creation)
    )");
    upd.prepare("UPDATE iffDetails SET state = :state WHERE id = :id");
    for (const auto &s : list) {
        ins.bindValue(":indexNo",  s.index);
        ins.bindValue(":id",       s.id);
        ins.bindValue(":type",     s.type);
        ins.bindValue(":state",    QJsonDocument(s.state).toJson(QJsonDocument::Compact));
        ins.bindValue(":creation", frameIndex);
        if (!ins.exec()) { qCritical() << "insertIffDetails:" << ins.lastError(); return false; }
        if (ins.numRowsAffected() == 0) {
            upd.bindValue(":state", QJsonDocument(s.state).toJson(QJsonDocument::Compact));
            upd.bindValue(":id",    s.id);
            if (!upd.exec()) { qCritical() << "updateIffDetails:" << upd.lastError(); return false; }
        }
    }
    return true;
}

bool SQLite::insertIffCRUD(int frameIndex, const IffCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("INSERT INTO iffCRUD (operation,indexNo,frameIndex) VALUES (:op,:idx,:fi)");
    for (const auto &c : list) {
        q.bindValue(":op",  c.operation);
        q.bindValue(":idx", c.index);
        q.bindValue(":fi",  frameIndex);
        if (!q.exec()) { qCritical() << "insertIffCRUD:" << q.lastError(); return false; }
    }
    return true;
}

// ─── IFF READ ────────────────────────────────────────────────────────────

void SQLite::setIffDetails(int frameIndex, IffList &list)
{
    QSqlQuery q(database());
    q.prepare(R"(
        SELECT indexNo, id, type, state FROM iffDetails
        WHERE creation <= :fi AND (deletion IS NULL OR deletion >= :fi)
    )");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setIffDetails:" << q.lastError(); return; }
    while (q.next()) {
        IffData d;
        d.index = q.value(0).toInt();
        d.id    = q.value(1).toString();
        d.type  = q.value(2).toString();
        d.state = QJsonDocument::fromJson(q.value(3).toByteArray()).object();
        list.push_back(d);
    }
}

void SQLite::setIffCRUD(int frameIndex, IffCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("SELECT operation, indexNo FROM iffCRUD WHERE frameIndex = :fi");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setIffCRUD:" << q.lastError(); return; }
    while (q.next()) {
        IffCRUD c;
        c.operation = static_cast<Operation>(q.value(0).toInt());
        c.index     = q.value(1).toInt();
        list.push_back(c);
    }
}

// ─── Radio INSERT ─────────────────────────────────────────────────────────

bool SQLite::insertRadioDetails(int frameIndex, const RadioList &list)
{
    QSqlQuery ins(database()), upd(database());
    ins.prepare(R"(
        INSERT OR IGNORE INTO radioDetails (indexNo, id, type, state, creation)
        VALUES (:indexNo, :id, :type, :state, :creation)
    )");
    upd.prepare("UPDATE radioDetails SET state = :state WHERE id = :id");
    for (const auto &s : list) {
        ins.bindValue(":indexNo",  s.index);
        ins.bindValue(":id",       s.id);
        ins.bindValue(":type",     s.type);
        ins.bindValue(":state",    QJsonDocument(s.state).toJson(QJsonDocument::Compact));
        ins.bindValue(":creation", frameIndex);
        if (!ins.exec()) { qCritical() << "insertRadioDetails:" << ins.lastError(); return false; }
        if (ins.numRowsAffected() == 0) {
            upd.bindValue(":state", QJsonDocument(s.state).toJson(QJsonDocument::Compact));
            upd.bindValue(":id",    s.id);
            if (!upd.exec()) { qCritical() << "updateRadioDetails:" << upd.lastError(); return false; }
        }
    }
    return true;
}

bool SQLite::insertRadioCRUD(int frameIndex, const RadioCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("INSERT INTO radioCRUD (operation,indexNo,frameIndex) VALUES (:op,:idx,:fi)");
    for (const auto &c : list) {
        q.bindValue(":op", c.operation); q.bindValue(":idx", c.index); q.bindValue(":fi", frameIndex);
        if (!q.exec()) { qCritical() << "insertRadioCRUD:" << q.lastError(); return false; }
    }
    return true;
}

// ─── Radio READ ───────────────────────────────────────────────────────────

void SQLite::setRadioDetails(int frameIndex, RadioList &list)
{
    QSqlQuery q(database());
    q.prepare(R"(
        SELECT indexNo, id, type, state FROM radioDetails
        WHERE creation <= :fi AND (deletion IS NULL OR deletion >= :fi)
    )");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setRadioDetails:" << q.lastError(); return; }
    while (q.next()) {
        RadioData d;
        d.index = q.value(0).toInt(); d.id = q.value(1).toString();
        d.type  = q.value(2).toString();
        d.state = QJsonDocument::fromJson(q.value(3).toByteArray()).object();
        list.push_back(d);
    }
}

void SQLite::setRadioCRUD(int frameIndex, RadioCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("SELECT operation, indexNo FROM radioCRUD WHERE frameIndex = :fi");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setRadioCRUD:" << q.lastError(); return; }
    while (q.next()) {
        RadioCRUD c;
        c.operation = static_cast<Operation>(q.value(0).toInt());
        c.index     = q.value(1).toInt();
        list.push_back(c);
    }
}

// ─── Weapon INSERT ────────────────────────────────────────────────────────

bool SQLite::insertWeaponDetails(int frameIndex, const WeaponList &list)
{
    QSqlQuery ins(database()), upd(database());
    ins.prepare(R"(
        INSERT OR IGNORE INTO weaponDetails (indexNo, id, type, state, creation)
        VALUES (:indexNo, :id, :type, :state, :creation)
    )");
    upd.prepare("UPDATE weaponDetails SET state = :state WHERE id = :id");
    for (const auto &s : list) {
        ins.bindValue(":indexNo",  s.index);
        ins.bindValue(":id",       s.id);
        ins.bindValue(":type",     s.type);
        ins.bindValue(":state",    QJsonDocument(s.state).toJson(QJsonDocument::Compact));
        ins.bindValue(":creation", frameIndex);
        if (!ins.exec()) { qCritical() << "insertWeaponDetails:" << ins.lastError(); return false; }
        if (ins.numRowsAffected() == 0) {
            upd.bindValue(":state", QJsonDocument(s.state).toJson(QJsonDocument::Compact));
            upd.bindValue(":id",    s.id);
            if (!upd.exec()) { qCritical() << "updateWeaponDetails:" << upd.lastError(); return false; }
        }
    }
    return true;
}

bool SQLite::insertWeaponCRUD(int frameIndex, const WeaponCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("INSERT INTO weaponCRUD (operation,indexNo,frameIndex) VALUES (:op,:idx,:fi)");
    for (const auto &c : list) {
        q.bindValue(":op", c.operation); q.bindValue(":idx", c.index); q.bindValue(":fi", frameIndex);
        if (!q.exec()) { qCritical() << "insertWeaponCRUD:" << q.lastError(); return false; }
    }
    return true;
}

// ─── Weapon READ ──────────────────────────────────────────────────────────

void SQLite::setWeaponDetails(int frameIndex, WeaponList &list)
{
    QSqlQuery q(database());
    q.prepare(R"(
        SELECT indexNo, id, type, state FROM weaponDetails
        WHERE creation <= :fi AND (deletion IS NULL OR deletion >= :fi)
    )");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setWeaponDetails:" << q.lastError(); return; }
    while (q.next()) {
        WeaponData d;
        d.index = q.value(0).toInt(); d.id = q.value(1).toString();
        d.type  = q.value(2).toString();
        d.state = QJsonDocument::fromJson(q.value(3).toByteArray()).object();
        list.push_back(d);
    }
}

void SQLite::setWeaponCRUD(int frameIndex, WeaponCRUDList &list)
{
    QSqlQuery q(database());
    q.prepare("SELECT operation, indexNo FROM weaponCRUD WHERE frameIndex = :fi");
    q.bindValue(":fi", frameIndex);
    if (!q.exec()) { qCritical() << "setWeaponCRUD:" << q.lastError(); return; }
    while (q.next()) {
        WeaponCRUD c;
        c.operation = static_cast<Operation>(q.value(0).toInt());
        c.index     = q.value(1).toInt();
        list.push_back(c);
    }
}
