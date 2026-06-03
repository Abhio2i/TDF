#include "recorder.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/eosensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/weapon.h"

/* -------------------------------------------------------
 * Recording Implementation Information Start
 * ------------------------------------------------------*/

// Constructor: Initializes recorder with hierarchy and simulation pointers
Recorder::Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent)
    : QObject(parent), m_hierarchy(hierarchy), m_simulation(simulation)
{
    m_recording = new Recording(m_hierarchy, m_simulation, this, this);
    m_replay    = new Replay   (m_hierarchy, m_simulation, this, this);
    emit sendRecorder();
    emit setSQLite();
    loggerModeCheck(RECORDING);
}

//Common File
void Recorder::loggerModeCheck(loggerModes mode)
{
    recordingStartTime = QDateTime::currentDateTime();
    duration = 0;
    switch(mode){
    case REPLAY:
        modeOfLogger = mode;
        m_replay->update();
        durationPtr = &(m_replay->durationShared);
        setLeftRightTimer(m_replay->leftTimer, m_replay->rightTimer);
        setBookmarks(m_replay->bookmarks);
        break;
    case RECORDING:
        modeOfLogger = mode;
        m_recording->update();
        durationPtr = &(m_recording->durationShared);
        setLeftRightTimer(m_recording->leftTimer, m_recording->rightTimer);
        setBookmarks(m_recording->bookmarks);
        break;
    default:
        break;
    }
}

void Recorder::loggerInfo()
{
    int seconds = duration / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    QString durationText = QString("%1:%2:%3")
                               .arg(hours, 2, 10, QLatin1Char('0'))
                               .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));

}

void Recorder::update(LoggerStatusModes loggerStatus)
{
    emit recorderInfoSend(loggerStatus);
}

QString Recorder::getStringTimer(qint64 time)
{
    qint64 milliseconds = time % 1000;
    qint64 seconds      = (time / 1000) % 60;
    qint64 minutes      = (time / (1000 * 60)) % 60;
    qint64 hours        = (time / (1000 * 60 * 60));
    QString durationText = QString("%1:%2:%3:%4")
                               .arg(hours,        2, 10, QLatin1Char('0'))
                               .arg(minutes,      2, 10, QLatin1Char('0'))
                               .arg(seconds,      2, 10, QLatin1Char('0'))
                               .arg(milliseconds, 3, 10, QLatin1Char('0'));
    return durationText;
}

void Recorder::setLeftRightTimer(qint64 &left, qint64 &right)
{
    rightTimer = &left;
    leftTimer  = &right;
}

void Recorder::setDuartionPtr(qint64 &s_durationPtr)
{
    durationPtr = &s_durationPtr;
}

qint64 *Recorder::getDuartionPtr()
{
    return durationPtr;
}

void Recorder::setBookmarks(QList<QPair<QString, qint64> > &s_bookmarks)
{
    bookmarks = &s_bookmarks;
}

DB_Validity Recorder::sqliteIsValid()
{
    if(m_sqlite == nullptr){
        emit alertViaStr("SQLite instance is not Created Yet!!");
        return DB_Invalid;
    }
    return DB_Valid;
}

void Recorder::saveFile(QString path)
{
     *isFileSaved = false;
    savedFilePath = path;
    fileToDB(savedFilePath,*isFileSaved);
    str = "Convert File to '.db' Logger->Recorder->Sqlite ";
    if(*isFileSaved){
        str += "Succesful";
    }else{
        emit alertViaStr("SQLite DB is Not Saved");
        str += "Failed";
    }
    debug(str,debugRecorder::D_DBOperation);
}

void Recorder::loadFile(QString path)
{
    loadedFilePath = path;
    loadToDB(loadedFilePath,*isFileLoaded);
    str = "Load File '.db' Logger->Recorder->Sqlite ";
    if(*isFileLoaded){
        str += "Succesful";
    }else{
        emit alertViaStr("SQLite DB is Not Loaded");
        str += "Failed";
    }
    debug(str,debugRecorder::D_DBOperation);
}

/*------------    Custom Debugger Start    ------------*/

void Recorder::debug(const QString &str, const debugRecorder &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }
}

bool Recorder::dbgIsAllow(const debugRecorder &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    return InsideList;
}

/*------------     Custom Debugger End     ------------*/
Recording::Recording(
    Hierarchy* hierarchy,
    Simulation* simulation,
    Recorder *parentRecorder,
    QObject *parent) :
    QObject(parent),
    m_hierarchy(hierarchy),
    m_simulation(simulation),
    m_recorder(parentRecorder)
{
    m_recorder = getRecorder();
}

void Recording::update()
{
    m_recorder->recordingStartTime = QDateTime::currentDateTime();
    m_recorder->duration           = 0;
    m_recorder->loggerStatus       = Recorder::S_RECORDING_MODE;
    m_recorder->simulationStatus   = Recorder::S_SIMULATION_NA;
    emit m_recorder->recorderInfoSendOnce(
        m_recorder->recordingStartTime ,
        m_recorder->duration           ,
        m_recorder->loggerStatus       ,
        m_recorder->simulationStatus   );
    m_recorder->loggerInfo();
}


void Recording::start(Recorder &s_recorder)
{
    if( &s_recorder == nullptr){
        emit m_recorder->sendRecorder();
    }
    m_recorder->loggerStatus = Recorder::S_RECORDING;
    m_recorder->update(m_recorder->loggerStatus);
    str = QString(
              "Recording: Information of Logger on Pressing Start Button"
              "\n\tStatus : %1"
              ).arg(m_recorder->loggerStatus);
    debug(QString::number(m_recorder->loggerStatus));
    str = QString();
    rightTimer = 0;
    leftTimer  = 0;
    m_recorder->setLeftRightTimer(leftTimer , rightTimer);
    m_recorder->loggerInfo();
    recordingBefore();
}

void Recording::pause()
{
    m_recorder->loggerStatus = Recorder::S_RECORDING_PAUSED;
    m_recorder->update(m_recorder->loggerStatus);
    str = QString(
              "Recording: Information of Logger on Pressing Pause Button"
              "\n\tStatus : %1"
              ).arg(QString::number(m_recorder->loggerStatus));
    debug(str,D_RecordingStatus);
    str = QString();
    m_recorder->loggerInfo();
    recordingPauseResume();
}

void Recording::resume()
{
    m_recorder->loggerStatus = Recorder::S_RECORDING;
    m_recorder->update(m_recorder->loggerStatus);
    str = QString(
              "Recording: Information of Logger on Pressing Resume Button"
              "\n\tStatus : %1"
              ).arg(QString::number(m_recorder->loggerStatus));
    debug(str,D_RecordingStatus);
    str = QString();
    m_recorder->loggerInfo();
    recordingPauseResume();
}

void Recording::stop()
{
    m_recorder->loggerStatus = Recorder::S_RECORDING_STOPPED;
    m_recorder->update(m_recorder->loggerStatus);
    str = QString(
              "Recording: Information of Logger on Pressing Stop Button"
              "\n\tStatus : %1"
              ).arg(QString::number(m_recorder->loggerStatus));
    debug(str,D_RecordingStatus);
    str = QString();
    m_recorder->loggerInfo();
    rightTimer = 0;
    leftTimer  = 0;
    m_recorder->setLeftRightTimer(leftTimer , rightTimer);
    recordingStop();
}

void Recording::addBookmark()
{
    return;
}

void Recording::insertRecord(const QJsonObject &data)
{
    recordedFile.append(data);
    recordedData = data;
}

void Recording::recordingBefore()
{
    str = "Before Recording is Called \n";
    if(*m_recorder->isFileSaved){
        emit m_recorder->alertViaStr("File is Saved");
        str += "File is Exist";
    }
    m_recorder->freezeButtonOperation(ButtonNOpsList(
        {{std::pair{Recorder_Button ,Freeze  }},
         {std::pair{Recording_Toggle ,Unfreeze}},
         {std::pair{Reocrding_Stop   ,Unfreeze}}}));
    debug(str,D_BeforeRecording);
    recordingStart();
}

void Recording::recordingStart()
{
    // ── RESET ALL STATE for a fresh recording ──────────────────────────
    maxIndex        = 0;
    maxProfileIndex = 0;
    maxSensorIndex  = 0;
    maxIffIndex     = 0;
    maxRadioIndex   = 0;
    maxWeaponIndex  = 0;

    entitiesIDIndex.clear();
    profileCategoriesIDIndex.clear();
    sensorIndexMap.clear();
    iffIndexMap.clear();
    radioIndexMap.clear();
    weaponIndexMap.clear();

    if (meshRenderer2DCRUDSet)
        meshRenderer2DCRUDSet->clear();

    iffsRestored    = false;
    radiosRestored  = false;
    weaponsRestored = false;
    // ── END RESET ──────────────────────────────────────────────────────

    m_recorder = getRecorder();
    mode = START;
    currentDateTime = QDateTime::currentDateTime();
    duration = 0;
    if (recordingTimer == nullptr) {
        recordingTimer = new QTimer(this);
    }
    frameIndex = 0;
    recordingPeriod = 100;
    if (recordingTimer->isActive() == false) {
        recordingTimer->start(recordingPeriod);
    }
    initiateTask();
    beforeChangesPacket(packet);
    for (auto& [id, profile] : m_hierarchy->ProfileCategories) {
        QString qid = QString::fromStdString(id);
        if (!profileCategoriesIDIndex.contains(qid)) {
            profileCategoriesUpdate(packet, qid,
                                    QString::fromStdString(profile->Name),
                                    Operation::CREATE);
        }
    }
    // ========== END OF ADDITION ==========
    inspectAll(packet);
    emit requestProcessing(packet);
    framePacketPartialClean(packet);
    inspectAll(packet);
    connect(recordingTimer, &QTimer::timeout, this, [this]() {
        durationShared += recordingPeriod;
        ++frameIndex;
        packet->timestamp  = durationShared;
        packet->frameIndex = frameIndex;
        entityUpdatesInBetween(packet);
        inspectAll(packet);
        emit requestProcessing(packet);
        framePacketPartialClean(packet);
        emit updateUiDuration();
    });
    emit started();
}
/*---------------- Shared Pointer Write Start --------------*/

void Recording::initiateTask()
{
    packet = QSharedPointer<PayLoad>::create();
    packet->timestamp = 0;
    packet->frameIndex = 0;
    packet->entitiesMeshRenderer2DList = new EntitiesMeshRenderer2DList();
    packet->entitiesMeshRenderer2DCRUDList = new EntitiesMeshRenderer2DCRUDList();
    packet->profileCategoriesDetailsList = new ProfileCategoriesDetailsList();
    packet->profileCategoriesCRUDList = new ProfileCategoriesCRUDList();
    packet->entitiesDetailsList = new EntitiesDetailsList();
    packet->entitiesCreatedList = new EntitiesCreatedList();
    packet->entitiesUpdatedList = new EntitiesUpdatedList();
    packet->entitiesDeletedList = new EntitiesDeletedList();
    packet->entitiesTrajectoryList = new EntitiesTrajectoryList();
    packet->entitiesTrajectoryCRUDList = new EntitiesTrajectoryCRUDList();
    packet->sensorsList = new SensorList();
    packet->sensorsCRUDList = new SensorCRUDList();
    packet->iffList      = new IffList();
    packet->iffCRUDList  = new IffCRUDList();
    packet->radioList    = new RadioList();
    packet->radioCRUDList= new RadioCRUDList();
    packet->weaponList   = new WeaponList();
    packet->weaponCRUDList= new WeaponCRUDList();
}

void Recording::addPacketData()
{

}

void Recording::onResultsReady(QSharedPointer<PayLoad> data)
{

}
/*------------------ Before Recording Start ----------------*/
void Recording::beforeRecording(QSharedPointer<PayLoad> payload)
{
    std::unordered_map<std::string, ProfileCategaory*> m_ProfileCategories
        = m_hierarchy->ProfileCategories;
    for(const auto& profileCategories : m_ProfileCategories){
        profileCategoriesUpdate(payload,profileCategories.first.c_str(),
                                profileCategories.second->Name.c_str(),Operation::CREATE);
    }
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for (const auto& platform : m_Platforms) {
        std::string name      = platform.second->Name.c_str();
        std::string parentID  = platform.second->parentID;
        std::string ID   = platform.first.c_str();
        entityAddedInBetween(payload,parentID.c_str(), ID.c_str(), name.c_str());
        if(platform.second->meshRenderer2d){
            meshRenderer2DCRUD(payload,platform.first.c_str(),platform.second->meshRenderer2d);
        }
        if(platform.second->trajectory){
            trajectoryCRUD(payload,platform.first.c_str(),platform.second->trajectory->Trajectories);
        }
    }
}
void Recording::currentEntities()
{

}

void Recording::currentProfile()
{

}

void Recording::profileCategoriesUpdate
    (QSharedPointer<PayLoad> payload,
     QString ID, QString profileName,
     Operation operation)
{
    if(profileCategoriesIDIndex.contains(ID)){
        str = QString("Already Exist : ");
        str += QString(
                   "True\n"
                   "ID : %1\n"
                   "Entity Name : %2\n"
                   "Operation: %3\n"
                   ).arg( ID, profileName,QString::number(operation));
        debug(str,D_ProfileCategories);

        return;
    }
    profileCategoriesIDIndex[ID] = ++maxProfileIndex;
    ProfileCategoriesDetails profileCategoriesDetails;
    profileCategoriesDetails.index = maxProfileIndex;
    profileCategoriesDetails.ID    = ID;
    profileCategoriesDetails.name  = profileName;
    payload->profileCategoriesDetailsList->push_back(profileCategoriesDetails);
    ProfileCategoriesCRUD profileCategoriesCRUD;
    profileCategoriesCRUD.index = maxProfileIndex;
    profileCategoriesCRUD.operation = operation;
    payload->profileCategoriesCRUDList->push_back(profileCategoriesCRUD);
    str = QString(
              "False => Creating new one\n"
              "ID : %1\n"
              "Entity Name : %2\n"
              "Index: %3\n"
              "Operation: %4\n"
              ).arg(profileCategoriesDetails.ID,
                   profileCategoriesDetails.name,
                   QString::number(profileCategoriesCRUD.index),
                   QString::number(profileCategoriesCRUD.operation));
    debug(str,D_ProfileCategories);
}
void Recording::profileCategoriesDeleted
    (QSharedPointer<PayLoad> payload,
     QString ID)
{
    str = QString("Already Exist : ");
    if(!profileCategoriesIDIndex.contains(ID)){
        str += QString(
                   "FAIL\n"
                   "ID : %1\n"
                   ).arg( ID);
        debug(str,D_ProfileCategories);
        return;
    }
    ProfileCategoriesCRUD profileCategoriesCRUD;
    profileCategoriesCRUD.index = profileCategoriesIDIndex.value(ID);
    profileCategoriesCRUD.operation = Operation::DELETE;
    payload->profileCategoriesCRUDList->push_back(profileCategoriesCRUD);
    str += QString(
               "False => Creating new one\n"
               "ID : %1\n"
               "Index: %2\n"
               "Operation: %3\n"
               ).arg(ID,
                    QString::number(profileCategoriesIDIndex.value(ID)),
                    QString::number(profileCategoriesCRUD.operation));
    profileCategoriesIDIndex.remove(ID);
    debug(str,D_ProfileCategories);
}

void Recording::entityAddedInBetween(
    QSharedPointer<PayLoad> payload,
    const QString &parentID,
    const QString &ID,
    const QString &entityName)
{
    if(entitiesIDIndex.contains(ID)){
        return;
    }
    str = QString(
              "Parent ID   : %1\n"
              "ID          : %2\n"
              "Entity Name : %3\n"
              ).arg(parentID, ID, entityName);

    EntitiesDetails m_entitiesDetails = {
                                         .index    = ++maxIndex,
                                         .name     = entityName,
                                         .parentID = parentID,
                                         .ID = ID};
    payload->entitiesDetailsList->push_back(m_entitiesDetails);
    EntitiesCreated m_createdList = {
        .index = maxIndex
    };
    payload->entitiesCreatedList->push_back(m_createdList);
    entitiesIDIndex[ID] = maxIndex;
    inspectEntitiesIDIndex();
    debug(str,D_EntityCreated);
}

void Recording::entityUpdatesInBetween(QSharedPointer<PayLoad> payload)
{
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    EntitiesUpdated m_entitiesUpdated;
    Platform* platform;
    int indexSize = entitiesIDIndex.size();
    QString m_str = "Entity List Size" +QString(indexSize)+" Updates In Between: ";

    for(auto i = entitiesIDIndex.begin(), end = entitiesIDIndex.end();
         i != end ; ++i){
        QString m_str = i.key();
        if(m_Platforms.find(m_str.toStdString()) == m_Platforms.end()){
            continue;
        }
        platform = m_Platforms.at(m_str.toStdString());
        m_str += "Index :" + QString(i.value());
        if(platform){
            m_entitiesUpdated.index       = i.value();
            m_entitiesUpdated.longitude   = platform->transform->getLongitude();
            m_entitiesUpdated.latitude    = platform->transform->getLatitude();
            m_entitiesUpdated.altitude    = platform->transform->getAltitude();
            m_entitiesUpdated.heading     = platform->transform->getHeading();
            m_entitiesUpdated.turn_radius = platform->dynamicModel->turnRate;
            m_entitiesUpdated.curr_speed  = platform->dynamicModel->currentSpeed;
            m_entitiesUpdated.climb_rate  = platform->dynamicModel->climbRate;
            payload->entitiesUpdatedList->push_back(m_entitiesUpdated);
            m_str += "PASS";
        }else{
            m_str += "Failed";
        }
        m_str += "\n  ";
    }
    debug(m_str,D_UpdatesInBTW);
}
void Recording::entityRemovedInBetween(
    QSharedPointer<PayLoad> payload,
    const QString &ID)
{
    str = QString(
              "ID          : %1"
              ).arg(ID);

    int value;
    if (entitiesIDIndex.contains(ID)) {
        value = entitiesIDIndex.value(ID);
    }else{
        return;
    }
    EntitiesDeleted m_entitiesDeleted ={
        .index = value
    };
    payload->entitiesDeletedList->push_back(m_entitiesDeleted);
    entitiesIDIndex.remove(ID);
    inspectEntitiesIDIndex();
    debug(str,D_EntityDeleted);
}

void Recording::meshRenderer2DCRUD(
    QSharedPointer<PayLoad> payload,
    const QString &ID,
    MeshRenderer2D *meshRenderer2D,
    Operation operation)
{
    auto itr = meshRenderer2DCRUDSet->find(ID);
    if(itr != meshRenderer2DCRUDSet->end()){
        str =  QString("Failed to Add ID: %1").arg(ID);
        debug(str,D_MeshRenderer2D);
        return;
    }
    meshRenderer2DCRUDSet->insert(ID);
    EntitiesMeshRenderer2DCRUD crud;
    crud.index = entitiesIDIndex.value(ID);
    crud.operation = operation;
    payload->entitiesMeshRenderer2DCRUDList->push_back(crud);
    EntitiesMeshRenderer2D entitiesMeshRenderer2D;
    switch(operation){
    case Operation::CREATE:
        entitiesMeshRenderer2D.index   = entitiesIDIndex.value(ID);
        entitiesMeshRenderer2D.Active  = meshRenderer2D->Active;
        entitiesMeshRenderer2D.Sprite  = meshRenderer2D->Sprite->c_str();
        entitiesMeshRenderer2D.Texture = meshRenderer2D->Texture->c_str();
        entitiesMeshRenderer2D.color   = meshRenderer2D->color->name(QColor::HexArgb);
        entitiesMeshRenderer2D.color2  = meshRenderer2D->color2->name(QColor::HexArgb);
        payload->entitiesMeshRenderer2DList->push_back(entitiesMeshRenderer2D);
        break;
    case Operation::UPDATE:
        entitiesMeshRenderer2D.index   = entitiesIDIndex.value(ID);
        entitiesMeshRenderer2D.Active  = meshRenderer2D->Active;
        entitiesMeshRenderer2D.Sprite  = meshRenderer2D->Sprite->c_str();
        entitiesMeshRenderer2D.Texture = meshRenderer2D->Texture->c_str();
        entitiesMeshRenderer2D.color   = meshRenderer2D->color->name(QColor::HexArgb);
        entitiesMeshRenderer2D.color2  = meshRenderer2D->color2->name(QColor::HexArgb);
        payload->entitiesMeshRenderer2DList->push_back(entitiesMeshRenderer2D);
        break;
    default:
        break;
    }
    debug(QString::number(payload->entitiesMeshRenderer2DCRUDList->size()),D_MeshRenderer2D);
}
void Recording::trajectoryCRUD(
    QSharedPointer<PayLoad> payload,
    const QString &ID,
    std::vector<Waypoints *> Trajectories,
    Operation operation)
{
    // ── Trajectory filter ─────────────────────────────────────────────────
    if (!filterTrajectories) return;

    EntitiesTrajectoryCRUD entitiesTrajectoryCRUD;
    entitiesTrajectoryCRUD.index     = entitiesIDIndex.value(ID);
    entitiesTrajectoryCRUD.operation = operation;
    payload->entitiesTrajectoryCRUDList->push_back(entitiesTrajectoryCRUD);

    int i = 0;
    EntitiesTrajectory entitiesTrajectory;
    entitiesTrajectory.index = entitiesIDIndex.value(ID);

    switch(operation){
    case Operation::CREATE:
        for(auto it = Trajectories.begin(); it != Trajectories.end(); ++it){
            TrajectoryWaypoint trajectoryWaypoint;
            trajectoryWaypoint.index         = i++;
            trajectoryWaypoint.geo_latitude  = (*it)->geocord->latitude;
            trajectoryWaypoint.geo_longitude = (*it)->geocord->longitude;
            trajectoryWaypoint.geo_altitude  = (*it)->geocord->altitude;
            trajectoryWaypoint.geo_Heading   = (*it)->geocord->Heading;
            trajectoryWaypoint.vector_x      = (*it)->position->x;
            trajectoryWaypoint.vector_y      = (*it)->position->y;
            trajectoryWaypoint.vector_z      = (*it)->position->z;
            trajectoryWaypoint.speed         = (*it)->speed;
            trajectoryWaypoint.sensor        = (*it)->sensor;
            trajectoryWaypoint.formation     = (*it)->formation;
            entitiesTrajectory.Trajectories.push_back(trajectoryWaypoint);
        }
        payload->entitiesTrajectoryList->push_back(entitiesTrajectory);
        break;

    case Operation::UPDATE:
        for(auto it = Trajectories.begin(); it != Trajectories.end(); ++it){
            TrajectoryWaypoint trajectoryWaypoint;
            trajectoryWaypoint.index         = i++;
            trajectoryWaypoint.geo_latitude  = (*it)->geocord->latitude;
            trajectoryWaypoint.geo_longitude = (*it)->geocord->longitude;
            trajectoryWaypoint.geo_altitude  = (*it)->geocord->altitude;
            trajectoryWaypoint.geo_Heading   = (*it)->geocord->Heading;
            trajectoryWaypoint.vector_x      = (*it)->position->x;
            trajectoryWaypoint.vector_y      = (*it)->position->y;
            trajectoryWaypoint.vector_z      = (*it)->position->z;
            trajectoryWaypoint.speed         = (*it)->speed;
            trajectoryWaypoint.sensor        = (*it)->sensor;
            trajectoryWaypoint.formation     = (*it)->formation;
            entitiesTrajectory.Trajectories.push_back(trajectoryWaypoint);
        }
        payload->entitiesTrajectoryList->push_back(entitiesTrajectory);
        break;

    default:
        break;
    }
    debug(QString::number(Trajectories.size()), D_TrajectoryCRUD);
}

void Recording::inspectAll(QSharedPointer<PayLoad> payload)
{
    str = QString(
              "Timestamp: %1, "
              "FrameIndex: %2, "
              "EntitiesMeshRenderer2DList: %3, "
              "EntitiesMeshRenderer2DCRUDList: %4, "
              "ProfileCategoriesDetailsList: %5, "
              "ProfileCategoriesCRUDList: %6, "
              "EntitiesDetailsList: %7, "
              "EntitiesCreatedList: %8, "
              "EntitiesUpdatedList: %9, "
              "EntitiesDeletedList: %10, "
              "EntitiesTrajectoryList: %11, "
              "EntitiesTrajectoryCRUDList: %12, ").arg(
                  QString::number(payload->timestamp),
                  QString::number(payload->frameIndex),
                  QString::number(payload->entitiesMeshRenderer2DList->size()),
                  QString::number(payload->entitiesMeshRenderer2DCRUDList->size()),
                  QString::number(payload->profileCategoriesDetailsList->size()),
                  QString::number(payload->profileCategoriesCRUDList->size()),
                  QString::number(payload->entitiesDetailsList->size()),
                  QString::number(payload->entitiesCreatedList->size()),
                  QString::number(payload->entitiesUpdatedList->size()),
                  QString::number(payload->entitiesDeletedList->size()),
                  QString::number(payload->entitiesTrajectoryList->size()),
                  QString::number(payload->entitiesTrajectoryCRUDList->size())
                  );
    debug(str , D_Inspection);
}
/*--------------------  Delta Changes End  -----------------*/

void Recording::entityAddedAllFromStart()
{
    std::unordered_map<std::string, ProfileCategaory*> m_ProfileCategories
        = m_hierarchy->ProfileCategories;
    for(const auto& profileCategories : m_ProfileCategories){
        profileCategoriesUpdate(profileCategories.first.c_str(),
                                profileCategories.second->Name.c_str(),Operation::CREATE);
    }
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for (const auto& platform : m_Platforms) {
        std::string name      = platform.second->Name.c_str();
        std::string parentID  = platform.second->parentID;
        std::string ID   = platform.first.c_str();
        entityAddedInBetween(parentID.c_str(), ID.c_str(), name.c_str());
        if(platform.second->meshRenderer2d){
            meshRenderer2DCRUD(platform.first.c_str(),platform.second->meshRenderer2d);
        }
        if(platform.second->trajectory){
            trajectoryCRUD(platform.first.c_str(),platform.second->trajectory->Trajectories);
        }
    }
}
/*---------------- Packet Changes Start ------------------*/

void Recording::framePacket(QSharedPointer<PayLoad> data)
{
    str = QString(
              "Frame PayLoad:=> "
              "Duration: %1  "
              "Frame Index: %2  "
              "Mesh Renderer2D No: %3  "
              "Mesh Renderer 2D CRUD No: %4  "
              "Profile Categories No: %5  "
              "Profile Categories CRUD No: %6  "
              "Entities ID No: %7  "
              "Details List No: %8  "
              "Created List No: %9  "
              "Updated List No: %10  "
              "Deleted List No: %11  "
              ).arg(
                  QString::number(durationShared),
                  QString::number(frameIndex),
                  QString::number(data->entitiesMeshRenderer2DList    ->size()),
                  QString::number(data->entitiesMeshRenderer2DCRUDList->size()),
                  QString::number(data->profileCategoriesDetailsList  ->size()),
                  QString::number(data->profileCategoriesCRUDList     ->size()),
                  QString::number(entitiesIDIndex.size()),
                  QString::number(data->entitiesDetailsList->size()),
                  QString::number(data->entitiesCreatedList->size()),
                  QString::number(data->entitiesUpdatedList->size()),
                  QString::number(data->entitiesDeletedList->size())
                  );
    debug(str,D_FramePayLoad);
}


void Recording::framePacketPartialClean(QSharedPointer<PayLoad> payload)
{
    payload->entitiesMeshRenderer2DList->clear();
    payload->entitiesMeshRenderer2DCRUDList->clear();
    payload->profileCategoriesDetailsList->clear();
    payload->profileCategoriesCRUDList->clear();
    payload->entitiesDetailsList->clear();
    payload->entitiesCreatedList->clear();
    payload->entitiesUpdatedList->clear();
    payload->entitiesDeletedList->clear();
    payload->entitiesTrajectoryList->clear();
    payload->entitiesTrajectoryCRUDList->clear();
    payload->sensorsList->clear();
    payload->sensorsCRUDList->clear();
    payload->iffList->clear();
    payload->iffCRUDList->clear();
    payload->radioList->clear();
    payload->radioCRUDList->clear();
    payload->weaponList->clear();
    payload->weaponCRUDList->clear();
}
/*----------------- Packet Changes End -------------------*/
// Get Changes before recording
void Recording::beforeChangesPacket(QSharedPointer<PayLoad> data)
{
    if (!m_hierarchy) {
        qWarning() << "beforeChangesPacket: m_hierarchy is null!";
        return;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 1: ProfileCategories — filter by profile type
    // ═══════════════════════════════════════════════════════════════════════
    for(const auto& [profId, profile] : m_hierarchy->ProfileCategories){
        if (!profile) continue;

        // ✅ FIX: Profile type ke basis par filter karo
        if (profile->type == Constants::EntityType::Sensor  && !filterSensors) continue;
        if (profile->type == Constants::EntityType::IFF     && !filterIFF)     continue;
        if (profile->type == Constants::EntityType::Radio   && !filterRadio)   continue;

        profileCategoriesUpdate(data,
                                QString::fromStdString(profId),
                                QString::fromStdString(profile->Name),
                                Operation::CREATE);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2: Platforms
    // ═══════════════════════════════════════════════════════════════════════
    for (const auto& platform : m_hierarchy->Platforms) {
        if (!platform.second) continue;
        std::string name     = platform.second->Name;
        std::string parentID = platform.second->parentID;
        std::string ID       = platform.first.c_str();

        entityAddedInBetween(data, parentID.c_str(), ID.c_str(), name.c_str());

        if(platform.second->meshRenderer2d){
            meshRenderer2DCRUD(data, platform.first.c_str(),
                               platform.second->meshRenderer2d);
        }

        // Trajectory filter
        if(filterTrajectories && platform.second->trajectory){
            trajectoryCRUD(data, platform.first.c_str(),
                           platform.second->trajectory->Trajectories);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2b: FixedPoints
    // ═══════════════════════════════════════════════════════════════════════
    if (filterFixedPoints) {
        for (const auto& [id, fp] : m_hierarchy->FixedPointes) {
            if (!fp) continue;
            entityAddedInBetween(data,
                                 QString::fromStdString(fp->parentID),
                                 QString::fromStdString(fp->ID),
                                 QString::fromStdString(fp->Name));
            if(fp->meshRenderer2d){
                meshRenderer2DCRUD(data, QString::fromStdString(fp->ID),
                                   fp->meshRenderer2d);
            }
            // Position save karo replay ke liye
            if (fp->transform && entitiesIDIndex.contains(QString::fromStdString(fp->ID))) {
                EntitiesUpdated eu;
                eu.index       = entitiesIDIndex.value(QString::fromStdString(fp->ID));
                eu.latitude    = fp->transform->getLatitude();
                eu.longitude   = fp->transform->getLongitude();
                eu.altitude    = fp->transform->getAltitude();
                eu.heading     = fp->transform->getHeading();
                eu.turn_radius = 0;
                eu.curr_speed  = 0;
                eu.climb_rate  = 0;
                data->entitiesUpdatedList->push_back(eu);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2c: SpecialZones
    // ═══════════════════════════════════════════════════════════════════════
    if (filterSpecialZone) {
        for (const auto& [id, sz] : m_hierarchy->Specialzones) {
            if (!sz) continue;
            entityAddedInBetween(data,
                                 QString::fromStdString(sz->parentID),
                                 QString::fromStdString(sz->ID),
                                 QString::fromStdString(sz->Name));
            if(sz->meshRenderer2d){
                meshRenderer2DCRUD(data, QString::fromStdString(sz->ID),
                                   sz->meshRenderer2d);
            }
            // Position save karo replay ke liye
            if (sz->transform && entitiesIDIndex.contains(QString::fromStdString(sz->ID))) {
                EntitiesUpdated eu;
                eu.index       = entitiesIDIndex.value(QString::fromStdString(sz->ID));
                eu.latitude    = sz->transform->getLatitude();
                eu.longitude   = sz->transform->getLongitude();
                eu.altitude    = sz->transform->getAltitude();
                eu.heading     = sz->transform->getHeading();
                eu.turn_radius = 0;
                eu.curr_speed  = 0;
                eu.climb_rate  = 0;
                data->entitiesUpdatedList->push_back(eu);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2d: Formations — always record
    // ═══════════════════════════════════════════════════════════════════════
    for (const auto& [id, fm] : m_hierarchy->Formations) {
        if (!fm) continue;
        entityAddedInBetween(data,
                             QString::fromStdString(fm->parentID),
                             QString::fromStdString(fm->ID),
                             QString::fromStdString(fm->Name));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2e: Sensor ENTITIES (hierarchy node)
    // ✅ FIX: filterSensors check add kiya — profile ke andar bhi filter hoga
    // ═══════════════════════════════════════════════════════════════════════
    if (filterSensors) {
        for (const auto& [id, sensor] : m_hierarchy->Sensors) {
            if (!sensor) continue;
            entityAddedInBetween(data,
                                 QString::fromStdString(sensor->parentID),
                                 QString::fromStdString(sensor->ID),
                                 QString::fromStdString(sensor->Name));
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2f: IFF ENTITIES (hierarchy node)
    // ✅ FIX: filterIFF check add kiya — profile ke andar bhi filter hoga
    // ═══════════════════════════════════════════════════════════════════════
    if (filterIFF) {
        for (const auto& [id, iff] : m_hierarchy->Iffs) {
            IFF* iffObj = dynamic_cast<IFF*>(iff);
            if(!iffObj) continue;
            entityAddedInBetween(data,
                                 QString::fromStdString(iffObj->parentID),
                                 QString::fromStdString(iffObj->ID),
                                 QString::fromStdString(iffObj->Name));
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2g: Radio ENTITIES (hierarchy node)
    // ✅ FIX: filterRadio check add kiya — profile ke andar bhi filter hoga
    // ═══════════════════════════════════════════════════════════════════════
    if (filterRadio) {
        for (const auto& [id, radio] : m_hierarchy->Radios) {
            Radio* radioObj = dynamic_cast<Radio*>(radio);
            if(!radioObj) continue;
            entityAddedInBetween(data,
                                 QString::fromStdString(radioObj->parentID),
                                 QString::fromStdString(radioObj->ID),
                                 QString::fromStdString(radioObj->Name));
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 2h: Weapon ENTITIES — always record
    // ═══════════════════════════════════════════════════════════════════════
    for (const auto& [id, weapon] : m_hierarchy->Weapons) {
        Weapon* weaponObj = dynamic_cast<Weapon*>(weapon);
        if(!weaponObj) continue;
        entityAddedInBetween(data,
                             QString::fromStdString(weaponObj->parentID),
                             QString::fromStdString(weaponObj->ID),
                             QString::fromStdString(weaponObj->Name));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 3: Sensor COMPONENT DATA — filter master + subtype
    // ═══════════════════════════════════════════════════════════════════════
    if (filterSensors) {
        for (auto& [id, sensor] : m_hierarchy->Sensors) {
            if (!sensor) continue;
            sensorAdded(data, QString::fromStdString(id), sensor, CREATE);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 4: IFF COMPONENT DATA
    // ═══════════════════════════════════════════════════════════════════════
    if (filterIFF) {
        for (auto& [id, iff] : m_hierarchy->Iffs) {
            if (!iff) continue;
            iffAdded(data, QString::fromStdString(id), iff, CREATE);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 5: Radio COMPONENT DATA
    // ═══════════════════════════════════════════════════════════════════════
    if (filterRadio) {
        for (auto& [id, radio] : m_hierarchy->Radios) {
            if (!radio) continue;
            radioAdded(data, QString::fromStdString(id), radio, CREATE);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 6: Weapon COMPONENT DATA — always record
    // ═══════════════════════════════════════════════════════════════════════
    for (auto& [id, weapon] : m_hierarchy->Weapons) {
        if (!weapon) continue;
        weaponAdded(data, QString::fromStdString(id), weapon, CREATE);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STEP 7: Second pass ProfileCategories (same filter logic)
    // ═══════════════════════════════════════════════════════════════════════
    for(const auto& [profId, profile] : m_hierarchy->ProfileCategories){
        if (!profile) continue;

        // ✅ FIX: Same filter as STEP 1
        if (profile->type == Constants::EntityType::Sensor  && !filterSensors) continue;
        if (profile->type == Constants::EntityType::IFF     && !filterIFF)     continue;
        if (profile->type == Constants::EntityType::Radio   && !filterRadio)   continue;

        profileCategoriesUpdate(data,
                                QString::fromStdString(profId),
                                QString::fromStdString(profile->Name),
                                Operation::CREATE);
    }
}
QJsonArray Recording::getFrameEntitiesData()
{
    QJsonArray frameEntities = QJsonArray();
    return frameEntities;
}

void Recording::recordingPauseResume()
{
    if(!recordingTimer){
        return;
    }
    if(m_recorder->loggerStatus == Recorder::LoggerStatusModes::S_RECORDING){
        recordingTimer->start();
    }else if(m_recorder->loggerStatus == Recorder::LoggerStatusModes::S_RECORDING_PAUSED){
        recordingTimer->stop();
    }
}
void Recording::recordingStop()
{
    durationShared  = 0;
    maxIndex        = 0;
    maxProfileIndex = 0;
    maxSensorIndex  = 0;
    maxIffIndex     = 0;
    maxRadioIndex   = 0;
    maxWeaponIndex  = 0;

    entitiesIDIndex.clear();
    profileCategoriesIDIndex.clear();
    sensorIndexMap.clear();
    iffIndexMap.clear();
    radioIndexMap.clear();
    weaponIndexMap.clear();

    if (meshRenderer2DCRUDSet)
        meshRenderer2DCRUDSet->clear();

    if (!recordingTimer) return;
    if (recordingTimer->isActive())
        recordingTimer->stop();
    recordingTimer->deleteLater();
    recordingTimer = nullptr;
}
void Recording::recordingBookmark(const QString &message, qint64 timestampMs)
{
    bookmarks.append(qMakePair(message, timestampMs));
    emit bookmarkRecorded(message, timestampMs);
}
void Recording::getSimulationUpdate()
{
}
void Recording::changeInHierarchy()
{
}
void Recording::saveFile()
{
}

void Recording::entityAddedInBetween(const QString &parentID,
                                     const QString &ID,
                                     const QString &entityName)
{
    if(entitiesIDIndex.contains(ID)){
        return;
    }
    str = QString(
              "Parent ID   : %1\n"
              "ID          : %2\n"
              "Entity Name : %3\n"
              ).arg(parentID, ID, entityName);

    EntitiesDetails m_entitiesDetails = {
                                         .index    = ++maxIndex,
                                         .name     = entityName,
                                         .parentID = parentID,
                                         .ID = ID};
    m_entitiesDetailsList->push_back(m_entitiesDetails);

    EntitiesCreated m_createdList = {
        .index = maxIndex
    };
    m_entitiesCreatedList->push_back(m_createdList);

    entitiesIDIndex[ID] = maxIndex;
    inspectEntitiesIDIndex();
    debug(str,D_EntityCreated);
}

void Recording::profileCategoriesUpdate(QString ID,
                                        QString profileName, Operation operation)
{
    if(profileCategoriesIDIndex.contains(ID)){
        /* For Debug */
        str = QString("Already Exist : ");
        str += QString(
                   "True\n"
                   "ID : %1\n"
                   "Entity Name : %2\n"
                   "Operation: %3\n"
                   ).arg( ID, profileName,QString::number(operation));
        debug(str,D_ProfileCategories);
        return;
    }

    profileCategoriesIDIndex[ID] = ++maxProfileIndex;
    ProfileCategoriesDetails profileCategoriesDetails;
    profileCategoriesDetails.index = maxProfileIndex;
    profileCategoriesDetails.ID    = ID;
    profileCategoriesDetails.name  = profileName;
    m_profileCategoriesDetailsList->push_back(profileCategoriesDetails);

    /* For Profile Name*/
    ProfileCategoriesCRUD profileCategoriesCRUD;
    profileCategoriesCRUD.index = maxProfileIndex;
    profileCategoriesCRUD.operation = operation;
    m_profileCategoriesCRUDList->push_back(profileCategoriesCRUD);

    /* For Debug */
    str = QString(
              "False => Creating new one\n"
              "ID : %1\n"
              "Entity Name : %2\n"
              "Index: %3\n"
              "Operation: %4\n"
              ).arg(profileCategoriesDetails.ID,
                   profileCategoriesDetails.name,
                   QString::number(profileCategoriesCRUD.index),
                   QString::number(profileCategoriesCRUD.operation));
    debug(str,D_ProfileCategories);
}

void Recording::profileCategoriesDeleted(QString ID)
{
    str = QString("Already Exist : ");
    if(!profileCategoriesIDIndex.contains(ID)){
        /* For Debug */
        str += QString(
                   "FAIL\n"
                   "ID : %1\n"
                   ).arg( ID);
        debug(str,D_ProfileCategories);
        return;
    }

    /* For Profile Name*/
    ProfileCategoriesCRUD profileCategoriesCRUD;
    profileCategoriesCRUD.index = profileCategoriesIDIndex.value(ID);
    profileCategoriesCRUD.operation = Operation::DELETE;
    m_profileCategoriesCRUDList->push_back(profileCategoriesCRUD);

    /* For Debug */
    str += QString(
               "False => Creating new one\n"
               "ID : %1\n"
               "Index: %2\n"
               "Operation: %3\n"
               ).arg(ID,
                    QString::number(profileCategoriesIDIndex.value(ID)),
                    QString::number(profileCategoriesCRUD.operation));
    // Remove In HashMap
    profileCategoriesIDIndex.remove(ID);
    debug(str,D_ProfileCategories);
}

void Recording::entityUpdatesInBetween()
{
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    EntitiesUpdated m_entitiesUpdated;
    Platform* platform;
    int indexSize = entitiesIDIndex.size();
    QString m_str = "Entity List Size" +QString(indexSize)+" Updates In Between: ";

    for(auto i = entitiesIDIndex.begin(), end = entitiesIDIndex.end();
         i != end ; ++i){
        QString m_str = i.key();
        if(m_Platforms.find(m_str.toStdString()) == m_Platforms.end()){
            continue;
        }
        platform = m_Platforms.at(m_str.toStdString());
        m_str += "Index :" + QString(i.value());
        if(platform){
            m_entitiesUpdated.index       = i.value();
            m_entitiesUpdated.longitude   = platform->transform->getLongitude();
            m_entitiesUpdated.latitude    = platform->transform->getLatitude();
            m_entitiesUpdated.altitude    = platform->transform->getAltitude();
            m_entitiesUpdated.heading     = platform->transform->getHeading();
            m_entitiesUpdated.turn_radius = platform->dynamicModel->turnRate;
            m_entitiesUpdated.curr_speed  = platform->dynamicModel->currentSpeed;
            m_entitiesUpdated.climb_rate  = platform->dynamicModel->climbRate;
            m_entitiesUpdatedList->push_back(m_entitiesUpdated);
            m_str += "PASS";
        }else{
            m_str += "Failed";
        }
        m_str += "\n  ";
    }
    debug(m_str,D_UpdatesInBTW);
}

void Recording::entityRemovedInBetween(const QString &ID)
{
    str = QString(
              "ID          : %1"
              ).arg(ID);
    int value;
    if (entitiesIDIndex.contains(ID)) {
        value = entitiesIDIndex.value(ID);
    }else{
        return;
    }
    EntitiesDeleted m_entitiesDeleted ={
        .index = value
    };
    m_entitiesDeletedList->push_back(m_entitiesDeleted);
    entitiesIDIndex.remove(ID);
    inspectEntitiesIDIndex();
    debug(str,D_EntityDeleted);
}

void Recording::meshRenderer2DCRUD(
    const QString &ID,
    MeshRenderer2D* meshRenderer2D,
    Operation operation)
{
    if(!entitiesIDIndex.contains(ID)){
        str =  QString("Failed to Add ID: %1").arg(ID);
        debug(str,D_MeshRenderer2D);
        return;
    }
    EntitiesMeshRenderer2DCRUD entitiesMeshRenderer2DCRUD;
    entitiesMeshRenderer2DCRUD.index     = entitiesIDIndex.value(ID);
    entitiesMeshRenderer2DCRUD.operation = operation;
    m_entitiesMeshRenderer2DCRUDList->push_back(entitiesMeshRenderer2DCRUD);

    EntitiesMeshRenderer2D entitiesMeshRenderer2D;
    switch(operation){
    case Operation::CREATE:
        entitiesMeshRenderer2D.index   = entitiesIDIndex.value(ID);
        entitiesMeshRenderer2D.Active  = meshRenderer2D->Active;
        entitiesMeshRenderer2D.Sprite  = meshRenderer2D->Sprite->c_str();
        entitiesMeshRenderer2D.Texture = meshRenderer2D->Texture->c_str();
        entitiesMeshRenderer2D.color   = meshRenderer2D->color->name(QColor::HexArgb);
        entitiesMeshRenderer2D.color2  = meshRenderer2D->color2->name(QColor::HexArgb);
        m_entitiesMeshRenderer2DList->push_back(entitiesMeshRenderer2D);
        break;
    case Operation::UPDATE:
        entitiesMeshRenderer2D.index   = entitiesIDIndex.value(ID);
        entitiesMeshRenderer2D.Active  = meshRenderer2D->Active;
        entitiesMeshRenderer2D.Sprite  = meshRenderer2D->Sprite->c_str();
        entitiesMeshRenderer2D.Texture = meshRenderer2D->Texture->c_str();
        entitiesMeshRenderer2D.color   = meshRenderer2D->color->name(QColor::HexArgb);
        entitiesMeshRenderer2D.color2  = meshRenderer2D->color2->name(QColor::HexArgb);
        m_entitiesMeshRenderer2DList->push_back(entitiesMeshRenderer2D);
        break;
    default:
        break;
    }

    debug(QString::number(m_entitiesMeshRenderer2DList->size()),D_MeshRenderer2D);
}

void Recording::trajectoryCRUD(const QString &ID,
                               std::vector<Waypoints *> Trajectories,
                               Operation operation)
{
    EntitiesTrajectoryCRUD entitiesTrajectoryCRUD;
    entitiesTrajectoryCRUD.index     = entitiesIDIndex.value(ID);
    entitiesTrajectoryCRUD.operation = operation;
    m_entitiesTrajectoryCRUDList->push_back(entitiesTrajectoryCRUD);

    int i = 0;
    EntitiesTrajectory entitiesTrajectory;
    entitiesTrajectory.index     = entitiesIDIndex.value(ID);
    switch(operation){
    case Operation::CREATE:
        for(auto it = Trajectories.begin();
             it != Trajectories.end(); ++it){
            TrajectoryWaypoint trajectoryWaypoint;
            trajectoryWaypoint.index         = i++;
            trajectoryWaypoint.geo_latitude  = (*it)->geocord->latitude ;
            trajectoryWaypoint.geo_longitude = (*it)->geocord->longitude;
            trajectoryWaypoint.geo_altitude  = (*it)->geocord->altitude ;
            trajectoryWaypoint.geo_Heading   = (*it)->geocord->Heading  ;
            trajectoryWaypoint.vector_x = (*it)->position->x;
            trajectoryWaypoint.vector_y = (*it)->position->y;
            trajectoryWaypoint.vector_z = (*it)->position->z;
            trajectoryWaypoint.speed     = (*it)->speed;
            trajectoryWaypoint.sensor    = (*it)->sensor;
            trajectoryWaypoint.formation = (*it)->formation;
            entitiesTrajectory.Trajectories.push_back(trajectoryWaypoint);
        }
        m_entitiesTrajectoryList->push_back(entitiesTrajectory);
        break;
    case Operation::UPDATE:
        for(auto it = Trajectories.begin();
             it != Trajectories.end(); ++it){
            TrajectoryWaypoint trajectoryWaypoint;
            trajectoryWaypoint.index         = i++;
            trajectoryWaypoint.geo_latitude  = (*it)->geocord->latitude ;
            trajectoryWaypoint.geo_longitude = (*it)->geocord->longitude;
            trajectoryWaypoint.geo_altitude  = (*it)->geocord->altitude ;
            trajectoryWaypoint.geo_Heading   = (*it)->geocord->Heading  ;
            trajectoryWaypoint.vector_x = (*it)->position->x;
            trajectoryWaypoint.vector_y = (*it)->position->y;
            trajectoryWaypoint.vector_z = (*it)->position->z;
            trajectoryWaypoint.speed     = (*it)->speed;
            trajectoryWaypoint.sensor    = (*it)->sensor;
            trajectoryWaypoint.formation = (*it)->formation;
            entitiesTrajectory.Trajectories.push_back(trajectoryWaypoint);
        }
        m_entitiesTrajectoryList->push_back(entitiesTrajectory);
    default:
        break;
    }
    debug(QString::number(Trajectories.size()),D_TrajectoryCRUD);
}
void Recording::inspectEntitiesIDIndex()
{
    std::string m_str = "Inspection of Entities ID Index: \n";
    for(auto i = entitiesIDIndex.begin(), end = entitiesIDIndex.end();
         i != end ; ++i){
        m_str += "\t ID:";
        m_str += i.key().toStdString();
        m_str += " => ";
        m_str += i.value();
        m_str += "\n";
    }
    debug(m_str.c_str(),D_EntitiesIDIndex);
}

void Recording::inspectEntitiesUpdatedList()
{
    QString m_str = "Inpect Entities Update:    \n";
    for(auto i = m_entitiesUpdatedList->begin(),
         end = m_entitiesUpdatedList->end();
         i != end ; ++i){
        m_str += "Index: "      +QString::number(i->index      )+" ";
        m_str += "longitude: "  +QString::number(i->longitude  )+" ";
        m_str += "latitude: "   +QString::number(i->latitude   )+" ";
        m_str += "altitude: "   +QString::number(i->altitude   )+" ";
        m_str += "heading: "    +QString::number(i->heading    )+" ";
        m_str += "turn_radius: "+QString::number(i->turn_radius)+" ";
        m_str += "curr_speed: " +QString::number(i->curr_speed )+" ";
        m_str += "climb_rate: " +QString::number(i->climb_rate )+" ";
        m_str += "\n *********** ";
    }
    debug(m_str, D_UpdatesInBTW);
}

/*------------    Custom Debugger Start    ------------*/

void Recording::debug(const QString &str,const debugOptions &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }
}

bool Recording::dbgIsAllow(const debugOptions &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    // qDebug()<<debugType<<option<<result;
    return InsideList;
}
/*------------     Custom Debugger End     ------------*/
Replay::Replay(
    Hierarchy* hierarchy,
    Simulation* simulation,
    Recorder *parentRecorder,
    QObject *parent) :
    QObject(parent),
    m_hierarchy(hierarchy),
    m_simulation(simulation),
    m_recorder(parentRecorder)
{
    m_recorder = getRecorder();
    emit getCanvas(&canvas);
}
void Replay::update()
{
    m_recorder->duration           = 0;
    m_recorder->loggerStatus       = Recorder::S_REPLAY_MODE;
    m_recorder->simulationStatus   = Recorder::S_SIMULATION_STOP;
    emit m_recorder->recorderInfoSendUsual(
        m_recorder->duration           ,
        m_recorder->loggerStatus       ,
        m_recorder->simulationStatus   );
    m_recorder->loggerInfo();
}

bool Replay::replayLoaded(const QString &filePath)
{
}
void Replay::createTimer()
{
}
void Replay::start()
{
    mode = replayModes::START;
    qDebug()<<mode;
    m_recorder = getRecorder();
    m_Platforms = &m_hierarchy->Platforms;
    m_recorder->loggerStatus = Recorder::S_REPLAYING;
    m_recorder->update(m_recorder->loggerStatus);
    rightTimer = 0;
    leftTimer  = 0;
    m_recorder->loggerInfo();
    m_recorder->setLeftRightTimer(leftTimer , rightTimer);
    qDebug()<<"Replay Mode is Set";
    m_recorder->loggerInfo();
    replayBefore();

}

void Replay::replayBefore()
{
    str = "Before Recording is Called \n";
    if(*m_recorder->isFileLoaded){
        emit m_recorder->alertViaStr("File is Loaded");
        str += "File is Loaded";
    }else{
        emit m_recorder->alertViaStr("File is not Loaded");
        str += "File is not Loaded";
        return;
    }
    ButtonNOpsList bnol =
        {{std::pair{Replay_Start         ,Freeze  }},
         {std::pair{Replay_Toggle        ,Unfreeze}},
         {std::pair{Replay_Jump_Forward  ,Unfreeze}},
         {std::pair{Replay_Jump_Backward ,Unfreeze}},
         {std::pair{Replay_Restart       ,Unfreeze}}};
    m_recorder->freezeButtonOperation(bnol);
    debug(str,D_BeforeReplay);
    replayStart();
}


void Replay::pause()
{
    if(replayTimer){
        mode = replayModes::PAUSE;
        m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
        m_recorder->update(m_recorder->loggerStatus);
        qDebug()<<mode;
        replayTimer->stop();
    }
}


void Replay::resume()
{
    if(replayTimer){
        mode = replayModes::RESUME;
        m_recorder->loggerStatus = Recorder::S_REPLAYING;
        m_recorder->update(m_recorder->loggerStatus);
        qDebug()<<mode;
        replayTimer->start(replayPeriod);
    }
}

void Replay::stop()
{
    if(replayTimer){
        mode = replayModes::STOP;
        m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
        m_recorder->update(m_recorder->loggerStatus);
        qDebug()<<mode;
        replayTimer->stop();
        replayTimer->destroyed();
    }
}
void Replay::replayStart()
{
    emit clearCanvasMeshes();
    m_recorder = getRecorder();
    mode = START;
    replayDateTime = QDateTime::currentDateTime();

    if (replayTimer == nullptr) {
        replayTimer = new QTimer(this);
    }

    replayPeriod = 100;
    disconnect(replayTimer, &QTimer::timeout, nullptr, nullptr);
    if(replayTimer->isActive() == false){
        replayTimer->start(replayPeriod);
    }
    emit getMaxFrameIndexNDuration(&maxFrameIndex, &maxDuration);
    emit setMaxDuration(&maxDuration);
    durationLength = maxDuration;
    duration = maxDuration;
    frameIndex = 0;
    cleanHierarchy();
    initiateTask();

    for (auto &bm : bookmarks) {
        emit replayBookmark(bm.first, bm.second);
    }

    emit setCanvasRenderEnabled(false);
    packet->frameIndex = 0;
    emit getPayLoadFromIndex(packet, frameIndex);

    // Step 1: Profile categories
    if (!profileCategoriesIndexDetails.empty() == false &&
        !packet->profileCategoriesDetailsList->empty()) {
        setProfileCategoriesDetails(packet);
    }
    if (!packet->profileCategoriesCRUDList->empty()) {
        crudProfileCategoriesDetails(packet);
    }

    // Step 2: Entity index details
    if (entitiesIndexDetails.empty() && !packet->entitiesDetailsList->empty()) {
        setEntitiesIndexDetails(packet);
    }

    // Step 3: Entities CREATE
    if (!packet->entitiesCreatedList->empty()) {
        createEntitiesCreateList(packet);
    }

    // ── FIX START ──
    // entitiesCreatedList frameIndex=0 pe empty hoti hai kyunki
    // setPayLoadFromIndex InBtw query use karta hai.
    // createEntitiesCreate signal queued ho sakta hai, isliye
    // processEvents() se pehle process karte hain.
    QCoreApplication::processEvents();

    // FixedPoints ke components add karo aur entityRecreated emit karo
    for (auto& [id, fp] : m_hierarchy->FixedPointes) {
        if (!fp) continue;
        if (!fp->transform)      fp->addComponent("transform");
        if (!fp->collider)       fp->addComponent("collider");
        if (!fp->meshRenderer2d) fp->addComponent("bitmap");
        emit entityRecreated(QString::fromStdString(fp->ID));
    }

    // SpecialZones ke components add karo aur entityRecreated emit karo
    for (auto& [id, sz] : m_hierarchy->Specialzones) {
        if (!sz) continue;
        if (!sz->transform)      sz->addComponent("transform");
        if (!sz->collider)       sz->addComponent("collider");
        if (!sz->meshRenderer2d) sz->addComponent("bitmap");
        emit entityRecreated(QString::fromStdString(sz->ID));
    }

    // Ek aur processEvents taaki entityRecreated se canvas Meshes map
    // update ho jaaye PEHLE MeshRenderer apply ho
    QCoreApplication::processEvents();
    // ── FIX END ──

    // Step 4: MeshRenderer
    if (!packet->entitiesMeshRenderer2DList->empty()) {
        setEntitiesMeshRenderer2D(packet);
    }
    if (!packet->entitiesMeshRenderer2DCRUDList->empty()) {
        crudEntitiesMeshRenderer2D(packet);
    }

    // Step 5: Trajectory
    if (!packet->entitiesTrajectoryList->empty()) {
        setEntitiesTrajectory(packet);
    }

    // Step 6: Sensors, IFF, Radio, Weapon
    auto* tempSensorsList   = packet->sensorsList;
    auto* tempSensorsCRUD   = packet->sensorsCRUDList;
    packet->sensorsList     = new SensorList();
    packet->sensorsCRUDList = new SensorCRUDList();

    if (!sensorsRestored &&
        (!tempSensorsList->empty() || !tempSensorsCRUD->empty())) {
        delete packet->sensorsList;
        delete packet->sensorsCRUDList;
        packet->sensorsList     = tempSensorsList;
        packet->sensorsCRUDList = tempSensorsCRUD;
        restoreSensorsFromPayload(packet);
        packet->sensorsList     = new SensorList();
        packet->sensorsCRUDList = new SensorCRUDList();
    } else {
        delete packet->sensorsList;
        delete packet->sensorsCRUDList;
        packet->sensorsList     = tempSensorsList;
        packet->sensorsCRUDList = tempSensorsCRUD;
    }

    if (!iffsRestored && (!packet->iffList->empty() || !packet->iffCRUDList->empty()))
        restoreIffsFromPayload(packet);
    if (!radiosRestored && (!packet->radioList->empty() || !packet->radioCRUDList->empty()))
        restoreRadiosFromPayload(packet);
    if (!weaponsRestored && (!packet->weaponList->empty() || !packet->weaponCRUDList->empty()))
        restoreWeaponsFromPayload(packet);

    // Step 7: Clear lists
    packet->profileCategoriesCRUDList->clear();
    packet->entitiesCreatedList->clear();
    packet->entitiesUpdatedList->clear();
    packet->entitiesDeletedList->clear();
    packet->entitiesMeshRenderer2DList->clear();
    packet->entitiesMeshRenderer2DCRUDList->clear();
    packet->entitiesTrajectoryList->clear();
    packet->entitiesTrajectoryCRUDList->clear();
    packet->sensorsList->clear();
    packet->sensorsCRUDList->clear();

    for (auto& [id, platform] : m_hierarchy->Platforms) {
        if (platform) platform->update();
    }
    emit setCanvasRenderEnabled(true);
    emit render(0.01f);

    packet->frameIndex = 1;
    connect(replayTimer, &QTimer::timeout, this, [this]() {
        emit requestProcessing(packet);
        packet->frameIndex = frameIndex;
        if(durationShared >= maxDuration || frameIndex >= maxFrameIndex){
            replayTimer->stop();
        } else {
            durationShared += replayPeriod;
            frameIndex++;
            emit getPayLoadFromIndex(packet, frameIndex);
        }
        framePayLoad(packet);
        for (auto& [id, platform] : m_hierarchy->Platforms) {
            if (platform) platform->update();
        }
        Simulation::simulationTime = static_cast<float>(durationShared) / 1000.0f;
        for (auto& [id, sensor] : m_hierarchy->Sensors) {
            if (sensor && sensor->Active) {
                sensor->scan();
            }
        }

        emit updateUiDuration();
        emit render(0.01f);
    });
}

/*---------------- Shared Pointer Write Start --------------*/
void Replay::framePayLoad(QSharedPointer<PayLoad> payload)
{

    str = QString("  Duration : %1  ").arg(durationShared);
    if(profileCategoriesIndexDetails.empty() && !payload->profileCategoriesDetailsList->empty()){
        setProfileCategoriesDetails(payload);
        str += QString("[ Profile Categories Details Update Size: %1 ] ")
                   .arg(payload->profileCategoriesDetailsList->size());
        // ===== CREATE ALL MISSING PROFILES FROM DETAILS =====
        for (const auto &pcd : *payload->profileCategoriesDetailsList) {
            // Check if this profile already exists in the hierarchy
            auto it = m_hierarchy->ProfileCategories.find(pcd.ID.toStdString());
            if (it == m_hierarchy->ProfileCategories.end()) {
                ProfileCategaory* profile = new ProfileCategaory(m_hierarchy);
                profile->Name = pcd.name.toStdString();
                profile->ID   = pcd.ID.toStdString();
                // Set the correct profile type based on name
                QString profileName = pcd.name;
                if (profileName.contains("Sensor", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::Sensor);
                else if (profileName.contains("IFF", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::IFF);
                else if (profileName.contains("Radio", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::Radio);
                else if (profileName.contains("Weapon", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::Weapon);
                else if (profileName.contains("FixedPoint", Qt::CaseInsensitive) ||
                         profileName.contains("Fixed", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::FixedPoint);
                else if (profileName.contains("Zone", Qt::CaseInsensitive) ||
                         profileName.contains("Special", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::SpecialZone);
                else if (profileName.contains("Formation", Qt::CaseInsensitive))
                    profile->setProfileType(Constants::EntityType::Formation);
                else
                    profile->setProfileType(Constants::EntityType::Platform);

                emit createProfileCategories(profile);
            }
        }
    }
    // 2. Profile categories CRUD
    if(!payload->profileCategoriesCRUDList->empty()){
        crudProfileCategoriesDetails(payload);
        str += QString("[ Profile Categories Details CRUD Size: %1 ] ")
                   .arg(payload->profileCategoriesCRUDList->size());
    }

    // 3. Entity index details (sirf pehli baar)
    if(entitiesIndexDetails.empty() && !payload->entitiesDetailsList->empty()){
        setEntitiesIndexDetails(payload);
        str += QString("[ Index Detail Update Size: %1 ] ")
                   .arg(payload->entitiesDetailsList->size());
    }

    // 4. Entities CREATE — PEHLE karo taaki platform ready ho sensors ke liye
    if(!payload->entitiesCreatedList->empty()){
        createEntitiesCreateList(payload);
        str += QString("[ Entities Created Size: %1 ] ")
                   .arg(payload->entitiesCreatedList->size());
    }

    // 5. Mesh renderer data
    if(!payload->entitiesMeshRenderer2DList->empty()){
        str += QString("[ Entities Mesh Renderer Update Size: %1 ] ")
                   .arg(payload->entitiesMeshRenderer2DList->size());
        setEntitiesMeshRenderer2D(payload);
    }

    // 6. Mesh renderer CRUD
    if(!payload->entitiesMeshRenderer2DCRUDList->empty()){
        str += QString("[ Entities Mesh Renderer CRUD Size: %1 ] ")
                   .arg(payload->entitiesMeshRenderer2DCRUDList->size());
        crudEntitiesMeshRenderer2D(payload);
    }

    // 7. Trajectory
    if(!payload->entitiesTrajectoryList->empty()
        || !payload->entitiesTrajectoryCRUDList->empty()){
        setEntitiesTrajectory(payload);
        str += QString("[ Entities Trajectory By Back Update Size: %1 And CRUD Size: %2 ] ")
                   .arg(payload->entitiesTrajectoryList->size(),
                        payload->entitiesTrajectoryCRUDList->size());
    }

    // 8. Entity position updates
    if(!payload->entitiesUpdatedList->empty()){
        updateEntitiesUpdatedList(payload);
        str += QString("[ Entities Updates Size: %1 ] ")
                   .arg(payload->entitiesUpdatedList->size());
    }

    // 9. Entity deletes
    if(!payload->entitiesDeletedList->empty()){
        deleteEntitiesDeletedList(payload);
        str += QString("[ Entities Deleted Size: %1 ] ")
                   .arg(payload->entitiesDeletedList->size());
    }
    if(!sensorsRestored &&
        (!payload->sensorsList->empty() || !payload->sensorsCRUDList->empty())) {
        restoreSensorsFromPayload(payload);
    }
        if (!iffsRestored &&
            (!payload->iffList->empty() || !payload->iffCRUDList->empty())) {
            restoreIffsFromPayload(payload);
        }
        if (!radiosRestored &&
            (!payload->radioList->empty() || !payload->radioCRUDList->empty())) {
            restoreRadiosFromPayload(payload);
        }
        if (!weaponsRestored &&
            (!payload->weaponList->empty() || !payload->weaponCRUDList->empty())) {
            restoreWeaponsFromPayload(payload);
        }
    debug(str, D_PayLoad_Inspect);
    payload->profileCategoriesCRUDList->clear();
    payload->entitiesCreatedList->clear();
    payload->entitiesUpdatedList->clear();
    payload->entitiesDeletedList->clear();
    payload->entitiesMeshRenderer2DList->clear();
    payload->entitiesMeshRenderer2DCRUDList->clear();
    payload->entitiesTrajectoryList->clear();
    payload->entitiesTrajectoryCRUDList->clear();
    payload->sensorsList->clear();
    payload->sensorsCRUDList->clear();
        payload->iffList->clear();
        payload->iffCRUDList->clear();
        payload->radioList->clear();
        payload->radioCRUDList->clear();
        payload->weaponList->clear();
        payload->weaponCRUDList->clear();
}

void Replay::initiateTask()
{
    // Create the shared pointer
    packet = QSharedPointer<PayLoad>::create();
    packet->timestamp = 0;
    packet->frameIndex = 0;
    packet->entitiesMeshRenderer2DList = new EntitiesMeshRenderer2DList();
    packet->entitiesMeshRenderer2DCRUDList = new EntitiesMeshRenderer2DCRUDList();
    packet->profileCategoriesDetailsList = new ProfileCategoriesDetailsList();
    packet->profileCategoriesCRUDList = new ProfileCategoriesCRUDList();
    packet->entitiesDetailsList = new EntitiesDetailsList();
    packet->entitiesCreatedList = new EntitiesCreatedList();
    packet->entitiesUpdatedList = new EntitiesUpdatedList();
    packet->entitiesDeletedList = new EntitiesDeletedList();
    packet->entitiesTrajectoryList = new EntitiesTrajectoryList();
    packet->entitiesTrajectoryCRUDList = new EntitiesTrajectoryCRUDList();
    packet->sensorsList = new SensorList();
    packet->sensorsCRUDList = new SensorCRUDList();
    packet->iffList      = new IffList();
    packet->iffCRUDList  = new IffCRUDList();
    packet->radioList    = new RadioList();
    packet->radioCRUDList= new RadioCRUDList();
    packet->weaponList   = new WeaponList();
    packet->weaponCRUDList= new WeaponCRUDList();
}

void Replay::beforeReplay(QSharedPointer<PayLoad> payload)
{
    payload->frameIndex;
}

void Replay::setProfileCategoriesDetails(
    QSharedPointer<PayLoad> payload)
{
    for(auto pcd = payload->profileCategoriesDetailsList->begin();
         pcd != payload->profileCategoriesDetailsList->end();
         ++pcd)
    {
        profileCategoriesIndexDetails[pcd->index] =*pcd ;
    }
    str = QString(
              "Profile Categories Details Map of Index Details Size: %1   \n")
              .arg(profileCategoriesIndexDetails.size());

    for(auto id = profileCategoriesIndexDetails.begin();
         id != profileCategoriesIndexDetails.end(); ++id)
    {
        str += QString("Index: %1   \n").arg(id->first);
    }
    debug(str,D_ProfileCategoriesIndexDetails);
}

void Replay::setEntitiesIndexDetails(
    QSharedPointer<PayLoad> payload)
{
    for(auto ed = payload->entitiesDetailsList->begin();
         ed != payload->entitiesDetailsList->end();
         ++ed)
    {
        entitiesIndexDetails[ed->index] = *ed;
    }
    str = QString(
              "Entities Map of Index Details Size: %1   \n")
              .arg(entitiesIndexDetails.size());
    for(auto id = entitiesIndexDetails.begin();
         id != entitiesIndexDetails.end(); ++id)
    {
        str += QString("Index: %1   \n").arg(id->first);
    }
    debug(str,D_EntitiesIndexDetails);
}

void Replay::setEntitiesMeshRenderer2D(
    QSharedPointer<PayLoad> payload)
{
    for(auto emr = payload->entitiesMeshRenderer2DList->begin();
         emr != payload->entitiesMeshRenderer2DList->end();
         ++emr)
    {
        entitiesMeshRenderer2DIndex[emr->index] = *emr;
    }
    str = QString(
              "Entities Mesh Renderer 2D Index Size: %1   \n")
              .arg(entitiesMeshRenderer2DIndex.size());
    str += "{  ";
    for(auto id = entitiesMeshRenderer2DIndex.begin();
         id != entitiesMeshRenderer2DIndex.end(); ++id)
    {
        str += QString("[ Index: %1 => %2 ] , ")
                   .arg(QString::number(id->first), id->second.Sprite);
    }
    str += "  }";
    debug(str,D_MeshRenderer);
}

void Replay::crudEntitiesMeshRenderer2D(QSharedPointer<PayLoad> payload)
{
    str = "Mesh CRUD Operation";
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;

    for(auto etc = payload->entitiesMeshRenderer2DCRUDList->begin();
         etc != payload->entitiesMeshRenderer2DCRUDList->end(); ++etc)
    {
        // MeshRenderer2D data check
        if(entitiesMeshRenderer2DIndex.find(etc->index) == entitiesMeshRenderer2DIndex.end()){
            str += " MeshIndex not found: " + QString::number(etc->index);
            continue;
        }

        // EntityDetails check
        if(entitiesIndexDetails.find(etc->index) == entitiesIndexDetails.end()){
            str += " EntityDetails not found: " + QString::number(etc->index);
            continue;
        }

        EntitiesMeshRenderer2D emr = entitiesMeshRenderer2DIndex.at(etc->index);
        EntitiesDetails &ed = entitiesIndexDetails.at(etc->index);
        std::string idStr = ed.ID.toStdString();

        // ✅ Platform check
        auto platIt = m_Platforms.find(idStr);
        if(platIt != m_Platforms.end()){
            if(platIt->second->meshRenderer2d){
                platIt->second->meshRenderer2d->Sprite->clear();
                platIt->second->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
            }
            continue;
        }

        // ✅ FixedPoints check
        auto fpIt = m_hierarchy->FixedPointes.find(idStr);
        if(fpIt != m_hierarchy->FixedPointes.end()){
            if(fpIt->second->meshRenderer2d){
                fpIt->second->meshRenderer2d->Sprite->clear();
                fpIt->second->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
            }
            continue;
        }

        // ✅ SpecialZone check
        auto szIt = m_hierarchy->Specialzones.find(idStr);
        if(szIt != m_hierarchy->Specialzones.end()){
            if(szIt->second->meshRenderer2d){
                szIt->second->meshRenderer2d->Sprite->clear();
                szIt->second->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
            }
            continue;
        }

        str += " Entity not found for ID: " + ed.ID;
    }
    debug(str, D_MeshRenderer);
}
void Replay::setEntitiesTrajectory(QSharedPointer<PayLoad> payload)
{
    if (!payload || !payload->entitiesTrajectoryList) return;

    str = QString("EntitiesTrajectory ");
    std::unordered_map<std::string, Platform*>* m_Platforms = &m_hierarchy->Platforms;

    for (auto et = payload->entitiesTrajectoryList->begin();
         et != payload->entitiesTrajectoryList->end(); ++et)
    {
        str += QString(" [ Index: %1 | Waypoints: %2 ] ,")
                   .arg(et->index)
                   .arg(et->Trajectories.size());

        auto edIt = entitiesIndexDetails.find(et->index);
        if (edIt == entitiesIndexDetails.end()) {
            qDebug() << "❌ entitiesIndexDetails not found for trajectory index:" << et->index;
            continue;
        }

        const EntitiesDetails& ed = edIt->second;
        std::string idStr = ed.ID.toStdString();

        // ✅ Important: Skip if entity was deleted or doesn't exist
        auto platIt = m_Platforms->find(idStr);
        if (platIt == m_Platforms->end() || !platIt->second) {
            qDebug() << "⚠️ Skipping trajectory for deleted entity:" << ed.ID;
            continue;
        }

        entitiesTrajectoryIndex[et->index] = *et;
        Platform* platform = platIt->second;

        // Clear old trajectory safely
        if (platform->trajectory) {
            for (Waypoints* wp : platform->trajectory->Trajectories) {
                if (wp) {
                    delete wp->position; wp->position = nullptr;
                    delete wp->geocord;  wp->geocord = nullptr;
                    delete wp;
                }
            }
            platform->trajectory->Trajectories.clear();
        } else {
            platform->addComponent("trajectory");
        }

        // Add new waypoints
        for (const auto& tw : et->Trajectories) {
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(tw.vector_x, tw.vector_y, tw.vector_z);
            newWaypoint->geocord = new Geocords();
            newWaypoint->geocord->latitude  = tw.geo_latitude;
            newWaypoint->geocord->longitude = tw.geo_longitude;
            newWaypoint->geocord->altitude  = tw.geo_altitude;
            newWaypoint->geocord->Heading   = tw.geo_Heading;
            newWaypoint->speed     = tw.speed;
            newWaypoint->sensor    = tw.sensor;
            newWaypoint->formation = tw.formation;

            platform->trajectory->addTrajectory(newWaypoint);
        }

        emit selectMesh(ed.ID);
    }

    emit render(0.01f);
    debug(str, D_EntitiesTrajectory);
}
void Replay::setEntitiesTrajectoryByBack(QSharedPointer<PayLoad> payload)
{
    str = QString("EntitiesTrajectory Btw ");
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;

    for(auto et = payload->entitiesTrajectoryList->begin();
         et != payload->entitiesTrajectoryList->end();
         ++et)
    {
        entitiesTrajectoryIndex[et->index] = *et;
        // To Debug
        str += QString(
                   " [ Index: %1 and Size: %2 ] ,"
                   ).arg(QString::number(et->index),
                        QString::number(et->Trajectories.size()));
        auto ed = entitiesIndexDetails.find(et->index);
        if(ed == entitiesIndexDetails.end()){
            continue;
        }
        auto platform = m_Platforms.find(ed->second.ID.toStdString());
        if(platform == m_Platforms.end()){
            continue;
        }
        emit selectMesh(platform->first.c_str());
        for (Waypoints* wp : platform->second->trajectory->Trajectories) {
            delete wp->position;
            delete wp;
        }
        platform->second->trajectory->Trajectories.clear();
        for(auto tw = et->Trajectories.begin();
             tw != et->Trajectories.end();
             ++tw){
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(tw->vector_x, tw->vector_y, tw->vector_z);
            newWaypoint->speed = tw->speed;
            newWaypoint->formation = tw->formation;
            newWaypoint->sensor = tw->sensor;
            platform->second->trajectory->addTrajectory(newWaypoint);
        }
    }
    debug(str,D_EntitiesTrajectory);
}

void Replay::crudEntitiesTrajectory(
    QSharedPointer<PayLoad> payload)
{

}

void Replay::crudProfileCategoriesDetails(
    QSharedPointer<PayLoad> payload)
{
    std::unordered_map<std::string, ProfileCategaory*> pc = m_hierarchy->ProfileCategories;
    str = QString();
    for(auto pcc = payload->profileCategoriesCRUDList->cbegin();
         pcc != payload->profileCategoriesCRUDList->cend();
         ++pcc)
    {
        ProfileCategoriesDetails pcd;
        try {
            pcd = profileCategoriesIndexDetails.at(pcc->index);
        }
        catch (std::out_of_range e) {
            str += QString("Failed profileCategoriesIndexDetails Caught: %1")
                       .arg(QString(e.what()));
            continue;
        }

        auto exist = pc.find(pcd.ID.toStdString());

        if(pcc->operation == Operation::CREATE && exist == pc.end()){
            ProfileCategaory* profile = new ProfileCategaory(m_hierarchy);
            profile->Name = pcd.name.toStdString();
            profile->ID   = pcd.ID.toStdString();

            // FIX: Profile ka sahi type set karo name ke basis par.
            // Pehle sirf Platform type set hoti thi (implicitly), ab sab handle karo.
            QString profileName = pcd.name;
            if(profileName.contains("Sensor", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::Sensor);
            } else if(profileName.contains("IFF", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::IFF);
            } else if(profileName.contains("Radio", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::Radio);
            } else if(profileName.contains("Weapon", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::Weapon);
            } else if(profileName.contains("FixedPoint", Qt::CaseInsensitive)
                       || profileName.contains("Fixed", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::FixedPoint);
            } else if(profileName.contains("Zone", Qt::CaseInsensitive)
                       || profileName.contains("Special", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::SpecialZone);
            } else if(profileName.contains("Formation", Qt::CaseInsensitive)){
                profile->setProfileType(Constants::EntityType::Formation);
            } else {
                // Default: Platform
                profile->setProfileType(Constants::EntityType::Platform);
            }

            emit createProfileCategories(profile);

            str += QString(
                       "[ Entity Name: %1 \t"
                       "ID: %2 \t"
                       "Index: %3 \t"
                       "Type: %4 \t ]"
                       ).arg(profile->Name.c_str(),
                            profile->ID.c_str(),
                            QString::number(pcd.index),
                            profileName);
        }

        if(pcc->operation == Operation::DELETE && exist != pc.end()){
            emit deleteProfileCategories(pcd.ID);
        }
    }
    debug(str, D_EntitiesCreateList);
}
void Replay::createEntitiesCreateList(QSharedPointer<PayLoad> payload)
{
    for (auto ec = payload->entitiesCreatedList->begin();
         ec != payload->entitiesCreatedList->end(); ++ec)
    {
        if (entitiesIndexDetails.find(ec->index) == entitiesIndexDetails.end())
            continue;

        EntitiesDetails &ed = entitiesIndexDetails.at(ec->index);
        emit createEntitiesCreate(ed.parentID, ed.ID, ed.name, true);

        std::string idStr = ed.ID.toStdString();

        // Platform
        auto platIt = m_Platforms->find(idStr);
        if (platIt != m_Platforms->end()) {
            Platform* platform = platIt->second;
            if (!platform->transform) platform->addComponent("transform");
            if (platform->transform && !platform->transform->geocord) {
                platform->transform->geocord = new Geocords();
                platform->transform->geocord->latitude  = 0.0;
                platform->transform->geocord->longitude = 0.0;
                platform->transform->geocord->altitude  = 0.0;
                platform->transform->geocord->Heading   = 0.0;
            }
            emit entityRecreated(ed.ID);
            platform->addComponent("crossSection");
            platform->addComponent("trajectory");
            platform->addComponent("rigidbody");
            platform->addComponent("dynamicModel");
            platform->addComponent("bitmap");
            platform->addComponent("collider");
            platform->addComponent("sensors");
            platform->addComponent("iffs");
            platform->addComponent("radios");
            platform->addComponent("weapons");
            continue;
        }

        // ✅ FIX: FixedPoints — components add karo + geocord initialize karo
        auto fpIt = m_hierarchy->FixedPointes.find(idStr);
        if (fpIt != m_hierarchy->FixedPointes.end()) {
            FixedPoints* fp = fpIt->second;
            fp->addComponent("transform");
            fp->addComponent("collider");
            fp->addComponent("bitmap");
            // geocord initialize karo — canvas drawImage() isko check karta hai
            if (fp->transform && !fp->transform->geocord) {
                fp->transform->geocord = new Geocords();
                fp->transform->geocord->latitude  = 0.0;
                fp->transform->geocord->longitude = 0.0;
                fp->transform->geocord->altitude  = 0.0;
                fp->transform->geocord->Heading   = 0.0;
            }
            continue;
        }

        // ✅ FIX: SpecialZone — components add karo + geocord initialize karo
        auto szIt = m_hierarchy->Specialzones.find(idStr);
        if (szIt != m_hierarchy->Specialzones.end()) {
            Specialzone* sz = szIt->second;
            sz->addComponent("transform");
            sz->addComponent("collider");
            sz->addComponent("bitmap");
            // geocord initialize karo — canvas drawImage() isko check karta hai
            if (sz->transform && !sz->transform->geocord) {
                sz->transform->geocord = new Geocords();
                sz->transform->geocord->latitude  = 0.0;
                sz->transform->geocord->longitude = 0.0;
                sz->transform->geocord->altitude  = 0.0;
                sz->transform->geocord->Heading   = 0.0;
            }
            continue;
        }

        // Sensor entity
        if (m_hierarchy->Sensors.find(idStr) != m_hierarchy->Sensors.end()) {
            continue;
        }

        // IFF entity
        if (m_hierarchy->Iffs.find(idStr) != m_hierarchy->Iffs.end()) {
            continue;
        }

        // Radio entity
        if (m_hierarchy->Radios.find(idStr) != m_hierarchy->Radios.end()) {
            continue;
        }

        // Weapon entity
        if (m_hierarchy->Weapons.find(idStr) != m_hierarchy->Weapons.end()) {
            continue;
        }
    }
}
void Replay::updateEntitiesUpdatedList(QSharedPointer<PayLoad> payload)
{
    if (!payload || !payload->entitiesUpdatedList)
        return;

    std::unordered_map<std::string, Platform*>& m_Platforms = m_hierarchy->Platforms;

    for (auto eu = payload->entitiesUpdatedList->begin();
         eu != payload->entitiesUpdatedList->end(); ++eu)
    {
        auto it = entitiesIndexDetails.find(eu->index);
        if (it == entitiesIndexDetails.end())
            continue;

        EntitiesDetails &ed = it->second;
        std::string idStr = ed.ID.toStdString();

        // ── Platform ──────────────────────────────────────────────────────────
        auto platIt = m_Platforms.find(idStr);
        if (platIt != m_Platforms.end())
        {
            Platform* platform = platIt->second;
            if (!platform) continue;

            // Safety checks
            if (!platform->transform) {
                platform->addComponent("transform");
            }
            if (!platform->dynamicModel) {
                platform->addComponent("dynamicModel");
            }

            if (platform->transform) {
                platform->transform->setLongitude(eu->longitude);
                platform->transform->setLatitude(eu->latitude);
                platform->transform->setAltitude(eu->altitude);
                platform->transform->setHeading(eu->heading);
            }

            if (platform->dynamicModel) {
                platform->dynamicModel->turnRate     = eu->turn_radius;
                platform->dynamicModel->currentSpeed = eu->curr_speed;
                platform->dynamicModel->climbRate    = eu->climb_rate;
            }

            if (platform->transform || platform->dynamicModel) {
                platform->update();
            }
            continue;
        }

        // ── FixedPoints ───────────────────────────────────────────────────────
        auto fpIt = m_hierarchy->FixedPointes.find(idStr);
        if (fpIt != m_hierarchy->FixedPointes.end())
        {
            FixedPoints* fp = fpIt->second;
            if (!fp || !fp->transform) continue;

            if (!fp->transform->geocord) {
                fp->transform->geocord = new Geocords();
            }
            fp->transform->geocord->latitude  = eu->latitude;
            fp->transform->geocord->longitude = eu->longitude;
            fp->transform->geocord->altitude  = eu->altitude;
            fp->transform->geocord->Heading   = eu->heading;
            continue;
        }

        // ── SpecialZones ──────────────────────────────────────────────────────
        auto szIt = m_hierarchy->Specialzones.find(idStr);
        if (szIt != m_hierarchy->Specialzones.end())
        {
            Specialzone* sz = szIt->second;
            if (!sz || !sz->transform) continue;

            if (!sz->transform->geocord) {
                sz->transform->geocord = new Geocords();
            }
            sz->transform->geocord->latitude  = eu->latitude;
            sz->transform->geocord->longitude = eu->longitude;
            sz->transform->geocord->altitude  = eu->altitude;
            sz->transform->geocord->Heading   = eu->heading;
            continue;
        }
    }

    emit render(0.01);
}

void Replay::deleteEntitiesDeletedList(
    QSharedPointer<PayLoad> payload)
{
    str = QString();
    for(auto edl = payload->entitiesDeletedList->begin();
         edl != payload->entitiesDeletedList->end();
         ++edl)
    {
        if(entitiesIndexDetails.find(edl->index)
            != entitiesIndexDetails.end()){
            EntitiesDetails &ed = entitiesIndexDetails
                                      .at(edl->index);
            str += QString(
                       "[ ID : %1 ,"
                       "Index : %2 ] ")
                       .arg(ed.ID,ed.index);
            emit deleteEntities(ed.parentID, ed.ID, true);
        }
    }
    debug(str,D_EntitiesDeletedList);
}

void Replay::onResultsReady(QSharedPointer<PayLoad> data)
{

}
/*----------------- Shared Pointer Write End  --------------*/
void Replay::receivePayLoadFrameIndex(PayLoad payLoad)
{
    payload = payLoad;
}
void Replay::cleanHierarchy()
{
    sensorsRestored  = false;
    iffsRestored     = false;
    radiosRestored   = false;
    weaponsRestored  = false;

    sensorsIndexMap.clear();
    iffsIndexMap_replay.clear();
    radiosIndexMap_replay.clear();
    weaponsIndexMap_replay.clear();

    // ← YE ADD KARO
    entitiesIndexDetails.clear();
    profileCategoriesIndexDetails.clear();
    entitiesMeshRenderer2DIndex.clear();
    entitiesTrajectoryIndex.clear();
    durationShared = 0;
    frameIndex     = 0;

    Radio::resetModel();
    m_hierarchy->fromJson(QJsonObject({}));
}
void Replay::setProfileCategoriesDetails()
{
    for(auto pcd = payload.profileCategoriesDetailsList->begin();
         pcd != payload.profileCategoriesDetailsList->end();
         ++pcd)
    {
        profileCategoriesIndexDetails[pcd->index] =*pcd ;
    }
    str = QString(
              "Profile Categories Details Map of Index Details Size: %1   \n")
              .arg(profileCategoriesIndexDetails.size());

    for(auto id = profileCategoriesIndexDetails.begin();
         id != profileCategoriesIndexDetails.end(); ++id)
    {
        str += QString("Index: %1   \n").arg(id->first);
    }
    debug(str,D_ProfileCategoriesIndexDetails);
}

void Replay::setEntitiesIndexDetails()
{
    for(auto ed = payload.entitiesDetailsList->begin();
         ed != payload.entitiesDetailsList->end();
         ++ed)
    {
        entitiesIndexDetails[ed->index] = *ed;
    }
    str = QString(
              "Entities Map of Index Details Size: %1   \n")
              .arg(entitiesIndexDetails.size());
    for(auto id = entitiesIndexDetails.begin();
         id != entitiesIndexDetails.end(); ++id)
    {
        str += QString("Index: %1   \n").arg(id->first);
    }
    debug(str,D_EntitiesIndexDetails);
}

void Replay::setEntitiesMeshRenderer2D()
{
    for(auto emr = payload.entitiesMeshRenderer2DList->begin();
         emr != payload.entitiesMeshRenderer2DList->end();
         ++emr)
    {
        entitiesMeshRenderer2DIndex[emr->index] = *emr;
    }
    str = QString(
              "Entities Mesh Renderer 2D Index Size: %1   \n")
              .arg(entitiesMeshRenderer2DIndex.size());
    str += "{  ";
    for(auto id = entitiesMeshRenderer2DIndex.begin();
         id != entitiesMeshRenderer2DIndex.end(); ++id)
    {
        str += QString("[ Index: %1 => %2 ] , ")
                   .arg(QString::number(id->first), id->second.Sprite);
    }
    str += "  }";
    debug(str,D_MeshRenderer);
}

void Replay::crudEntitiesMeshRenderer2D()
{
    // //For Mesh render
    str = "Mesh CRUD Operation";
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto etc = payload.entitiesMeshRenderer2DCRUDList->begin();
         etc != payload.entitiesMeshRenderer2DCRUDList->end();
         ++etc){
        if(entitiesMeshRenderer2DIndex.find(etc->index)
            == entitiesMeshRenderer2DIndex.end()){
            str += "Failed to Add Mesh For Index: "+QString::number(etc->index);
            continue;
        }
        EntitiesMeshRenderer2D emr = entitiesMeshRenderer2DIndex.at(etc->index);
        EntitiesDetails &ed = entitiesIndexDetails.at(etc->index);
        Platform *platform = m_Platforms.at(ed.ID.toStdString());
        platform->meshRenderer2d->Sprite->clear();
        platform->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
    }
    debug(str, D_MeshRenderer);
}

void Replay::setEntitiesTrajectory()
{
    str = QString("EntitiesTrajectory { ");
    for(auto et = payload.entitiesTrajectoryList->begin();
         et != payload.entitiesTrajectoryList->end();
         ++et)
    {
        // For index of Trajectory
        entitiesTrajectoryIndex[et->index] = *et;
        // To Debug
        str += QString(
                   " [ Index: %1 and Size: %2 ] ,"
                   ).arg(QString::number(et->index),
                        QString::number(et->Trajectories.size()));
    }
    str += QString("  }  \n   ");
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    str += QString("EntitiesCRUD { ");
    for(auto etc = payload.entitiesTrajectoryCRUDList->begin();
         etc != payload.entitiesTrajectoryCRUDList->end();
         ++etc)
    {
        // For index of Trajectory
        EntitiesDetails ed;
        try {
            ed = entitiesIndexDetails.at(etc->index);
        }
        catch (std::out_of_range e) {
            str += QString("Failed entitiesIndexDetails Caught: %1")
                       .arg(QString(e.what()));
            continue;
        }

        Platform* platform = m_Platforms.at(ed.ID.toStdString());
        EntitiesTrajectory et;
        try {
            et = entitiesTrajectoryIndex.at(etc->index);
        }
        catch (std::out_of_range e) {
            str += QString("Failed entitiesTrajectoryIndex Caught: %1")
                       .arg(QString(e.what()));
            continue;
        }
        switch (etc->operation) {
        case Operation::CREATE:
            if(platform->trajectory){
                for (Waypoints* wp : platform->trajectory->Trajectories) {
                    delete wp->position;
                    delete wp;
                }
                platform->trajectory->Trajectories.clear();
                for(auto tw = et.Trajectories.begin();
                     tw != et.Trajectories.end();
                     ++tw){
                    Waypoints* newWaypoint = new Waypoints();
                    newWaypoint->position = new Vector(tw->vector_x, tw->vector_y, tw->vector_z);
                    newWaypoint->speed = tw->speed;
                    newWaypoint->formation = tw->formation;
                    newWaypoint->sensor = tw->sensor;
                    platform->trajectory->addTrajectory(newWaypoint);
                }
            }
            break;
        case Operation::UPDATE:
            if(platform->trajectory){
                for (Waypoints* wp : platform->trajectory->Trajectories) {
                    delete wp->position;
                    delete wp;
                }
                platform->trajectory->Trajectories.clear();
                for(auto tw = et.Trajectories.begin();
                     tw != et.Trajectories.end();
                     ++tw){
                    Waypoints* newWaypoint = new Waypoints();
                    newWaypoint->position = new Vector(tw->vector_x, tw->vector_y, tw->vector_z);
                    newWaypoint->speed = tw->speed;
                    newWaypoint->formation = tw->formation;
                    newWaypoint->sensor = tw->sensor;
                    platform->trajectory->addTrajectory(newWaypoint);
                }
            }
            break;
        case Operation::DELETE:
            if(platform->trajectory){
                for (Waypoints* wp : platform->trajectory->Trajectories) {
                    delete wp->position;
                    delete wp;
                }
                platform->trajectory->Trajectories.clear();
            }
            break;
        default:
            break;
        }
    }
    str += QString("  }");
    debug(str,D_EntitiesTrajectory);
}
void Replay::crudProfileCategoriesDetails()
{
    std::unordered_map<std::string, ProfileCategaory*> pc = m_hierarchy->ProfileCategories;
    str = QString();
    for(auto pcc = payload.profileCategoriesCRUDList->cbegin();
         pcc != payload.profileCategoriesCRUDList->cend();
         ++pcc)
    {
        ProfileCategoriesDetails pcd;
        try {
            pcd = profileCategoriesIndexDetails.at(pcc->index);
        }
        catch (std::out_of_range e) {
            str += QString("Failed profileCategoriesIndexDetails Caught: %1")
                       .arg(QString(e.what()));
            continue;
        }
        auto exist = pc.find(pcd.ID.toStdString());
        if(pcc->operation == Operation::CREATE && exist == pc.end()){
            ProfileCategaory* profile = new ProfileCategaory(m_hierarchy);
            profile->Name = pcd.name.toStdString();
            profile->ID   = pcd.ID.toStdString();
            QJsonObject profileObj;
            profileObj.insert("name",pcd.name);
            profileObj.insert("id",pcd.ID);
            emit createProfileCategories(profile);
            //profile->fromJson(profileObj);
            str += QString(
                       "[ Entity Name: %1 \t"
                       "ID: %2 \t"
                       "Index: %3 \t ]"
                       ).arg(profile->Name.c_str(),
                            profile->ID.c_str(),
                            QString::number(pcd.index));

        }
        if(pcc->operation == Operation::DELETE && exist != pc.end()){
            emit deleteProfileCategories(pcd.ID);
        }
    }
    debug(str,D_EntitiesCreateList);
}

void Replay::createEntitiesCreateList()
{
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    str = QString();
    for(auto ec = payload.entitiesCreatedList->begin();
         ec != payload.entitiesCreatedList->end();
         ++ec)
    {
        if(entitiesIndexDetails.find(ec->index) != entitiesIndexDetails.end()){
            EntitiesDetails &ed = entitiesIndexDetails.at(ec->index);
            str += QString(
                       "Entity Name: %1 \t"
                       "ID: %2 \t"
                       ).arg(ed.name,ed.ID);

            emit createEntitiesCreate(ed.parentID, ed.ID, ed.name, true);
            Platform* platform = m_Platforms.at(ed.ID.toStdString());
            platform->addComponent("transform");
            platform->addComponent("crossSection");
            platform->addComponent("trajectory");
            platform->addComponent("rigidbody");
            platform->addComponent("dynamicModel");
            platform->addComponent("bitmap");
            platform->addComponent("collider");
            platform->addComponent("sensors");
            platform->addComponent("iffs");
            platform->addComponent("radios");
            platform->addComponent("weapons");
        }
    }
    debug(str,D_EntitiesCreateList);
}

void Replay::updateEntitiesUpdatedList()
{
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto eu = payload.entitiesUpdatedList->begin();
         eu != payload.entitiesUpdatedList->end();
         ++eu)
    {
        if(entitiesIndexDetails.find(eu->index) != entitiesIndexDetails.end()){
            EntitiesDetails &ed = entitiesIndexDetails.at(eu->index);

            auto it = m_Platforms.find(ed.ID.toStdString());
            if(it == m_Platforms.end() ){
                continue;
            }else{
                Platform* platform = m_Platforms.at(ed.ID.toStdString());
                platform->transform->setLongitude (eu->longitude);
                platform->transform->setLatitude  (eu->latitude );
                platform->transform->setAltitude  (eu->altitude );
                platform->transform->setHeading   (eu->heading  );
                platform->dynamicModel->turnRate     = eu->turn_radius;
                platform->dynamicModel->currentSpeed = eu->curr_speed ;
                platform->dynamicModel->climbRate    = eu->climb_rate ;
            }
        }
    }
    emit render(0.01);
}

void Replay::deleteEntitiesDeletedList()
{
    str = QString();
    for(auto edl = payload.entitiesDeletedList->begin();
         edl != payload.entitiesDeletedList->end();
         ++edl)
    {
        if(entitiesIndexDetails.find(edl->index)
            != entitiesIndexDetails.end()){
            EntitiesDetails &ed = entitiesIndexDetails
                                      .at(edl->index);
            str += QString(
                       "[ ID : %1 ,"
                       "Index : %2 ] ")
                       .arg(ed.ID,ed.index);
            emit deleteEntities(ed.parentID, ed.ID, true);
        }
    }
    debug(str,D_EntitiesDeletedList);
}


void Replay::jumpInBetween(qint64 clickedTimestamp)
{
    pause();
    int clickedIndex = clickedTimestamp / replayPeriod;
    if(clickedIndex >= 0 && clickedIndex <= maxFrameIndex){
        durationShared = clickedIndex*100;
        frameIndex = clickedIndex;
        framePayLoadForJumpInBetween(clickedIndex);
    }
    Simulation::simulationTime = static_cast<float>(durationShared) / 1000.0f;
    for (auto& [id, sensor] : m_hierarchy->Sensors) {
        if (sensor && sensor->Active) {
            sensor->scan();
        }
    }
    str = QString("JumpInBetween: timestamp:%1 & frameIndex:%2")
              .arg(clickedTimestamp,clickedIndex);
    debug(str,D_LoadInBetween);
    resume();
}

void Replay::framePayLoadForJumpInBetween(int frameIndex)
{
    packet->entitiesDetailsList->clear();
    packet->entitiesMeshRenderer2DList->clear();
    packet->entitiesTrajectoryList->clear();
    packet->entitiesTrajectoryCRUDList->clear();
    emit getPayLoadFromIndex(packet, frameIndex);

    // Step 1: Profile categories
    if (!packet->profileCategoriesDetailsList->empty())
        setProfileCategoriesDetails(packet);
    if (!packet->profileCategoriesCRUDList->empty())
        crudProfileCategoriesDetails(packet);

    // Step 2: Entity details index
    if (entitiesIndexDetails.empty() && !packet->entitiesDetailsList->empty())
        setEntitiesIndexDetails(packet);

    // Step 3: CREATE
    if (!packet->entitiesCreatedList->empty())
        createEntitiesCreateList(packet);

    // ── FIX START: same as replayStart ──
    QCoreApplication::processEvents();

    for (auto& [id, fp] : m_hierarchy->FixedPointes) {
        if (!fp) continue;
        if (!fp->transform)      fp->addComponent("transform");
        if (!fp->collider)       fp->addComponent("collider");
        if (!fp->meshRenderer2d) fp->addComponent("bitmap");
        emit entityRecreated(QString::fromStdString(fp->ID));
    }
    for (auto& [id, sz] : m_hierarchy->Specialzones) {
        if (!sz) continue;
        if (!sz->transform)      sz->addComponent("transform");
        if (!sz->collider)       sz->addComponent("collider");
        if (!sz->meshRenderer2d) sz->addComponent("bitmap");
        emit entityRecreated(QString::fromStdString(sz->ID));
    }

    QCoreApplication::processEvents();
    // ── FIX END ──

    // Step 4: MeshRenderer
    if (!packet->entitiesMeshRenderer2DList->empty())
        setEntitiesMeshRenderer2D(packet);
    if (!packet->entitiesMeshRenderer2DCRUDList->empty())
        crudEntitiesMeshRenderer2D(packet);

    // Step 5: Trajectory
    if (!packet->entitiesTrajectoryList->empty())
        setEntitiesTrajectory(packet);

    // Step 6: Sensors/IFF/Radio/Weapon
    if (!sensorsRestored)
        restoreSensorsFromPayload(packet);
    if (!iffsRestored)
        restoreIffsFromPayload(packet);
    if (!radiosRestored)
        restoreRadiosFromPayload(packet);
    if (!weaponsRestored)
        restoreWeaponsFromPayload(packet);

    // Step 7: Clear
    packet->profileCategoriesCRUDList->clear();
    packet->entitiesCreatedList->clear();
    packet->entitiesUpdatedList->clear();
    packet->entitiesDeletedList->clear();
    packet->entitiesMeshRenderer2DList->clear();
    packet->entitiesMeshRenderer2DCRUDList->clear();
    packet->entitiesTrajectoryList->clear();
    packet->entitiesTrajectoryCRUDList->clear();
    packet->sensorsList->clear();
    packet->sensorsCRUDList->clear();
    packet->iffList->clear();
    packet->iffCRUDList->clear();
    packet->radioList->clear();
    packet->radioCRUDList->clear();
    packet->weaponList->clear();
    packet->weaponCRUDList->clear();
}

void Replay::setEntitiesDetailsInBtw()
{
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    //m_Platforms->clear();
    std::vector<std::pair<std::string,std::string>> toDelete;
    std::pair<std::string,std::string> PidNID;
    for(auto p = m_Platforms.begin();
         p != m_Platforms.end(); ++p){
        PidNID.first  = p->second->parentID;
        PidNID.second = p->second->ID;
        toDelete.push_back(PidNID);
    }
    for(auto td = toDelete.begin();
         td != toDelete.end(); ++td){
        emit deleteEntities(td->first.c_str(),td->second.c_str(), true);
    }
    for(auto ed = payload.entitiesDetailsList->begin();
         ed != payload.entitiesDetailsList->end();
         ++ed)
    {
        emit createEntitiesCreate((*ed).parentID, (*ed).ID, (*ed).name, true);
        Platform* platform = m_Platforms.at((*ed).ID.toStdString());
        platform->addComponent("transform");
        platform->addComponent("crossSection");
        platform->addComponent("trajectory");
        platform->addComponent("rigidbody");
        platform->addComponent("dynamicModel");
        platform->addComponent("bitmap");
        platform->addComponent("collider");
        platform->addComponent("sensors");
        platform->addComponent("iffs");
        platform->addComponent("radios");
    }
}
void Replay::setEntitiesMeshRenderer2DBtw()
{
    str = "Mesh Between Operation";
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto emr = payload.entitiesMeshRenderer2DList->begin();
         emr != payload.entitiesMeshRenderer2DList->end();
         ++emr){
        entitiesMeshRenderer2DIndex[emr->index] = *emr;
    }
    for(auto ed = payload.entitiesDetailsList->begin();
         ed != payload.entitiesDetailsList->end();
         ++ed){
        EntitiesMeshRenderer2D emr = entitiesMeshRenderer2DIndex[ed->index];
        if(m_Platforms.find(ed->ID.toStdString()) == m_Platforms.end()){
            str += "Failed to Add Mesh For Index: "+QString::number(ed->index);
            continue;
        }
        Platform *platform = m_Platforms.at(ed->ID.toStdString());
        platform->meshRenderer2d->Sprite->clear();
        platform->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
    }
    debug(str, D_MeshRenderer);
}

void Replay::setEntitiesDetailsInBtw(QSharedPointer<PayLoad> payload)
{
    str = QString("EntitiesTrajectory Btw ");
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto et = payload->entitiesTrajectoryList->begin();
         et != payload->entitiesTrajectoryList->end();
         ++et)
    {
        entitiesTrajectoryIndex[et->index] = *et;
        str += QString(
                   " [ Index: %1 and Size: %2 ] ,"
                   ).arg(QString::number(et->index),
                        QString::number(et->Trajectories.size()));
        auto ed = entitiesIndexDetails.find(et->index);
        if(ed == entitiesIndexDetails.end()){
            continue;
        }
        auto platform = m_Platforms.find(ed->second.ID.toStdString());
        if(platform == m_Platforms.end()){
            continue;
        }
        for (Waypoints* wp : platform->second->trajectory->Trajectories) {
            delete wp->position;
            delete wp;
        }
        platform->second->trajectory->Trajectories.clear();
        for(auto tw = et->Trajectories.begin();
             tw != et->Trajectories.end();
             ++tw){
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(tw->vector_x, tw->vector_y, tw->vector_z);
            newWaypoint->speed = tw->speed;
            newWaypoint->formation = tw->formation;
            newWaypoint->sensor = tw->sensor;
            platform->second->trajectory->addTrajectory(newWaypoint);
        }
    }
    debug(str,D_EntitiesTrajectory);
}


void Replay::setEntitiesMeshRenderer2DBtw(QSharedPointer<PayLoad> payload)
{
    str = "Mesh Between Operation";
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto emr = payload->entitiesMeshRenderer2DList->begin();
         emr != payload->entitiesMeshRenderer2DList->end();
         ++emr){
        entitiesMeshRenderer2DIndex[emr->index] = *emr;
    }
    for(auto ed = payload->entitiesDetailsList->begin();
         ed != payload->entitiesDetailsList->end();
         ++ed){
        EntitiesMeshRenderer2D emr = entitiesMeshRenderer2DIndex[ed->index];
        if(m_Platforms.find(ed->ID.toStdString()) == m_Platforms.end()){
            str += "Failed to Add Mesh For Index: "+QString::number(ed->index);
            continue;
        }
        Platform *platform = m_Platforms.at(ed->ID.toStdString());
        platform->meshRenderer2d->Sprite->clear();
        platform->meshRenderer2d->Sprite->append(emr.Sprite.toStdString());
    }
    debug(str, D_MeshRenderer);
}

void Replay::setEntitiesTrajectoryBtw()
{
    str = QString("EntitiesTrajectory Btw ");
    std::unordered_map<std::string, Platform*> m_Platforms = m_hierarchy->Platforms;
    for(auto et = payload.entitiesTrajectoryList->begin();
         et != payload.entitiesTrajectoryList->end();
         ++et)
    {
        entitiesTrajectoryIndex[et->index] = *et;
        str += QString(
                   " [ Index: %1 and Size: %2 ] ,"
                   ).arg(QString::number(et->index),
                        QString::number(et->Trajectories.size()));
        auto ed = entitiesIndexDetails.find(et->index);
        if(ed == entitiesIndexDetails.end()){
            continue;
        }
        auto platform = m_Platforms.find(ed->second.ID.toStdString());
        if(platform == m_Platforms.end()){
            continue;
        }
        for (Waypoints* wp : platform->second->trajectory->Trajectories) {
            delete wp->position;
            delete wp;
        }
        platform->second->trajectory->Trajectories.clear();
        for(auto tw = et->Trajectories.begin();
             tw != et->Trajectories.end();
             ++tw){
            Waypoints* newWaypoint = new Waypoints();
            newWaypoint->position = new Vector(tw->vector_x, tw->vector_y, tw->vector_z);
            newWaypoint->speed = tw->speed;
            newWaypoint->formation = tw->formation;
            newWaypoint->sensor = tw->sensor;
            platform->second->trajectory->addTrajectory(newWaypoint);
        }
    }

    debug(str,D_EntitiesTrajectory);
}
void Replay::toggle()
{
}

void Replay::loadData()
{
    std::string entitieList = "Load the entities: \n";
    for(const auto& pair : entitiesMap){
        int key = pair.first;
        std::string id = pair.second.first;
        std::string name = pair.second.second;
        entitieList += "\t Key: "  + std::to_string(key);
        entitieList += "\t ID: "   + QString::fromStdString(id).toStdString();
        entitieList += "\t Name: " + QString::fromStdString(name).toStdString()+"\n";
    }
    int index = 1;
    entitieList += "\n\nLoad Frame Map: \n";
    for(const auto& i: frameMap ){
        entitieList += "\t Index "+std::to_string(index++)+" : "+ std::to_string(i)+"\n";
    }
    qint64 lastTime = *(--frameMap.end());
    entitieList += "\t Last Index :"+std::to_string(lastTime)+"\n";
    qDebug()<<entitieList.c_str();
}
void Replay::restart()
{
    qDebug()<<"Replay Restart";
    durationShared = 0;
}
void Replay::playAgain()
{
    qDebug()<<"Replay Play Again!";
    replayTimer->stop();
    durationShared = 0;
    frameIndex = 0;
    m_recorder->loggerStatus = Recorder::S_REPLAY_MODE;
    m_recorder->update(m_recorder->loggerStatus);
}

void Replay::fileLoaded()
{
    fileState = fileStates::EXIST;
    qDebug()<<fileState;
}

void Replay::fileUnloaded()
{
    fileState = fileStates::NOT_EXIST;
    qDebug()<<fileState;
}

void Replay::goToNextFrame()
{
    if(replayTimer && mode != replayModes::STOP){
        qDebug()<<jumpOps::FORWORD;
        if(durationLength > durationShared+jumpStep && packet->frameIndex <= frameMap.size() - 50 ){
            packet->frameIndex += 50;
        }else{
            qDebug()<<"Not Allowed Duration Length: "<<durationLength<<" < "
                     <<durationShared+jumpStep;
        }
    }else{
        qDebug()<<mode<<"Not Allowed";
    }
}

void Replay::goToPreviousFrame()
{
    if(replayTimer && mode != replayModes::STOP){
        qDebug()<<jumpOps::BACKWORD;
        if(durationShared > jumpStep && packet->frameIndex > 50){
            packet->frameIndex -= 50;
        }else{
        }
    }else{
    }
}
void Replay::startReplayFromTimestamp(qint64 timestampMs)
{
    if(replayTimer && mode != replayModes::STOP){
        qDebug()<<jumpOps::INBETWEEN<<Qt::endl
                 <<"Timer: "<<m_recorder->getStringTimer(timestampMs);
    }else{
        qDebug()<<mode<<"Not Allowed";
    }
}

void Replay::bookmarkReplay(const QString &note, qint64 timestampMs)
{
    if(replayTimer && mode != replayModes::STOP){
        qDebug()<<jumpOps::BOOKMARK<<Qt::endl
                 <<"Bookmark Note: "<<note<<Qt::endl
                 <<"Timer: "<<m_recorder->getStringTimer(timestampMs);
    }else{
        qDebug()<<mode<<"Not Allowed";
    }
}

void Replay::connectReplayTimer()
{
}

/*------------    Custom Debugger Start    ------------*/

void Replay::debug(const QString &str,const debugReplay &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }

}

bool Replay::dbgIsAllow(const debugReplay &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    return InsideList;
}
void Recording::sensorAdded(QSharedPointer<PayLoad> payload,
                            const QString &sensorId,
                            Sensor* sensor, Operation op)
{
    if (!payload || !sensor) return;

    // ── Master sensor filter ───────────────────────────────────────────────
    if (!filterSensors) return;

    // ── Per-subtype filter ─────────────────────────────────────────────────
    // Use the type string so we don't depend on enum internals
    const QString sType = sensor->subTypeToString(sensor->subType);

    if (sType.contains("AESA",  Qt::CaseInsensitive) && !filterAESA)  return;
    if (sType.contains("CSM",   Qt::CaseInsensitive) && !filterCSM)   return;
    if (sType.contains("ESM",   Qt::CaseInsensitive) && !filterESM)   return;
    // if (sType.contains("EO",    Qt::CaseInsensitive) && !filterEO)    return;
    // if (sType.contains("IR",    Qt::CaseInsensitive) && !filterIR)    return;
    if (sType.contains("Sonar", Qt::CaseInsensitive) && !filterSonar) return;
    if (sType.contains("AIS",   Qt::CaseInsensitive) && !filterAIS)   return;
    if (sType.contains("ADSB",  Qt::CaseInsensitive) && !filterADSB)  return;
    // Generic / Radar — check last (broad match)
    if ((sType.contains("Radar",   Qt::CaseInsensitive) ||
         sType.contains("Generic", Qt::CaseInsensitive)) && !filterRadar) return;

    // ── Already recorded? ──────────────────────────────────────────────────
    if (sensorIndexMap.contains(sensorId)) return;

    int idx = ++maxSensorIndex;
    sensorIndexMap[sensorId] = idx;

    SensorData data;
    data.index = idx;
    data.id    = sensorId;
    data.type  = sType;
    data.state = sensor->toJson();
    data.state["_name"]           = QString::fromStdString(sensor->Name);
    data.state["_parentEntityId"] = QString::fromStdString(
        sensor->parentEntity ? sensor->parentEntity->ID : "");

    payload->sensorsList->push_back(data);

    SensorCRUD crud;
    crud.index     = idx;
    crud.operation = op;
    payload->sensorsCRUDList->push_back(crud);
}
void Recording::sensorUpdated(QSharedPointer<PayLoad> payload, const QString &sensorId, Sensor* sensor) {
    if (!sensorIndexMap.contains(sensorId)) return;
    int idx = sensorIndexMap[sensorId];

    SensorData data;
    data.index = idx;
    data.id = sensorId;
    data.type = sensor->subTypeToString(sensor->subType);
    data.state = sensor->toJson();
    payload->sensorsList->push_back(data);

    SensorCRUD crud;
    crud.index = idx;
    crud.operation = UPDATE;
    payload->sensorsCRUDList->push_back(crud);
}

void Recording::sensorRemoved(QSharedPointer<PayLoad> payload, const QString &sensorId) {
    if (!sensorIndexMap.contains(sensorId)) return;
    int idx = sensorIndexMap[sensorId];
    SensorCRUD crud;
    crud.index = idx;
    crud.operation = DELETE;
    payload->sensorsCRUDList->push_back(crud);
    sensorIndexMap.remove(sensorId);
}

void Replay::restoreSensorsFromPayload(QSharedPointer<PayLoad> payload)
{
    if (m_hierarchy->Platforms.empty()) return;
    if (sensorsRestored) return;

    // DELETE pass
    for (auto &crud : *payload->sensorsCRUDList) {
        if (crud.operation == DELETE) {
            auto it = sensorsIndexMap.find(crud.index);
            if (it != sensorsIndexMap.end()) {
                m_hierarchy->Sensors.erase(it->second->ID);
                delete it->second;
                sensorsIndexMap.erase(it);
            }
        }
    }
    bool hasSensorProfile = false;
    for (const auto& [key, profilePtr] : m_hierarchy->ProfileCategories) {
        if (profilePtr && profilePtr->type == Constants::EntityType::Sensor) {
            hasSensorProfile = true;
            break;
        }
    }
    if (!hasSensorProfile) {
        ProfileCategaory* sensorsProfile = m_hierarchy->addProfileCategaory("Sensors");
        if (sensorsProfile)
            sensorsProfile->setProfileType(Constants::EntityType::Sensor);
    }

    bool anyCreated = false;
    for (auto &sdata : *payload->sensorsList) {
        auto it = sensorsIndexMap.find(sdata.index);
        if (it != sensorsIndexMap.end()) {
            it->second->fromJson(sdata.state);
            continue;
        }

        // CREATE
        QString sensorName = sdata.state["_name"].toString();
        if (sensorName.isEmpty()) sensorName = sdata.state["name"].toString();
        if (sensorName.isEmpty()) sensorName = sdata.type;

        QString parentEntityId = sdata.state["_parentEntityId"].toString();
        if (parentEntityId.isEmpty())
            parentEntityId = sdata.state["parentEntityId"].toString();
        if (parentEntityId.isEmpty()) continue;
        auto platIt = m_hierarchy->Platforms.find(parentEntityId.toStdString());
        if (platIt == m_hierarchy->Platforms.end()) continue;
        Platform* platform = platIt->second;
        // sensors component ensure karo
        if (!platform->sensors)
            platform->addComponent("sensors");
        if (!platform->sensors) continue;
        QString compId = QString::fromStdString(platform->sensors->ID);
        m_hierarchy->addSubComponent(compId, sensorName, sdata.type,
                                     sdata.id, sdata.state);
        auto addedIt = m_hierarchy->Sensors.find(sdata.id.toStdString());
        if (addedIt != m_hierarchy->Sensors.end()) {
            sensorsIndexMap[sdata.index] = addedIt->second;
            anyCreated = true;
        }
    }
    sensorsRestored = true;
    emit render(0.01f);
}template<typename T, typename DataT, typename CRUDT, typename ListT, typename CRUDListT>
static void genericAdded(
    QSharedPointer<PayLoad> payload,
    const QString          &id,
    T                      *component,
    Operation               op,
    QHash<QString,int>     &indexMap,
    int                    &maxIndex,
    ListT                  *payloadList,
    CRUDListT              *payloadCRUD)
{
    if (!payload || !component) return;
    if (indexMap.contains(id))  return;
    int idx = ++maxIndex;
    indexMap[id] = idx;
    DataT data;
    data.index = idx;
    data.id    = id;
    data.type  = component->subTypeToString(component->subType);
    data.state = component->toJson();
    data.state["_name"]           = QString::fromStdString(component->Name);
    data.state["_parentEntityId"] = QString::fromStdString(
    component->parentEntity ? component->parentEntity->ID : "");
    payloadList->push_back(data);
    CRUDT crud;
    crud.index     = idx;
    crud.operation = op;
    payloadCRUD->push_back(crud);
}

template<typename T, typename DataT, typename CRUDT, typename ListT, typename CRUDListT>
static void genericUpdated(
    QSharedPointer<PayLoad> payload,
    const QString          &id,
    T                      *component,
    QHash<QString,int>     &indexMap,
    ListT                  *payloadList,
    CRUDListT              *payloadCRUD)
{
    if (!indexMap.contains(id)) return;
    int idx = indexMap[id];
    DataT data;
    data.index = idx;
    data.id    = id;
    data.type  = component->subTypeToString(component->subType);
    data.state = component->toJson();
    payloadList->push_back(data);
    CRUDT crud;  crud.index = idx;  crud.operation = UPDATE;
    payloadCRUD->push_back(crud);
}

template<typename CRUDT, typename CRUDListT>
static void genericRemoved(
    QSharedPointer<PayLoad> payload,
    const QString          &id,
    QHash<QString,int>     &indexMap,
    CRUDListT              *payloadCRUD)
{
    if (!indexMap.contains(id)) return;
    CRUDT crud;  crud.index = indexMap[id];  crud.operation = DELETE;
    payloadCRUD->push_back(crud);
    indexMap.remove(id);
}

// ── IFF ───────────────────────────────────────────────────────────────────

void Recording::iffAdded(QSharedPointer<PayLoad> payload,
                         const QString &iffId,
                         QObject *iff,
                         Operation op)
{
    if (!payload || !iff) return;
    if (iffIndexMap.contains(iffId)) return;
    IFF* iffObj = qobject_cast<IFF*>(iff);
    if (!iffObj) return;
    int idx = ++maxIffIndex;
    iffIndexMap[iffId] = idx;
    IffData data;
    data.index = idx;
    data.id    = iffId;
    data.type  = "IFF";
    data.state = iffObj->toJson();
    data.state["_name"]           = QString::fromStdString(iffObj->Name);
    data.state["_parentEntityId"] = QString::fromStdString(
    iffObj->parentEntity ? iffObj->parentEntity->ID : "");
    payload->iffList->push_back(data);
    IffCRUD crud;
    crud.index     = idx;
    crud.operation = op;
    payload->iffCRUDList->push_back(crud);
}

void Recording::iffUpdated(QSharedPointer<PayLoad> payload,
                           const QString &iffId,
                           QObject *iff)
{
    if (!payload || !iff) return;
    if (!iffIndexMap.contains(iffId)) return;
    IFF* iffObj = qobject_cast<IFF*>(iff);
    if (!iffObj) return;
    int idx = iffIndexMap[iffId];
    IffData data;
    data.index = idx;
    data.id    = iffId;
    data.type  = "IFF";
    data.state = iffObj->toJson();
    payload->iffList->push_back(data);
    IffCRUD crud;
    crud.index     = idx;
    crud.operation = UPDATE;
    payload->iffCRUDList->push_back(crud);
}

void Recording::iffRemoved(QSharedPointer<PayLoad> payload,
                           const QString &iffId)
{
    if (!payload) return;
    if (!iffIndexMap.contains(iffId)) return;
    IffCRUD crud;
    crud.index     = iffIndexMap[iffId];
    crud.operation = DELETE;
    payload->iffCRUDList->push_back(crud);
    iffIndexMap.remove(iffId);
}

// ── Radio ──────────────────────────────────────────────────────────────────
void Recording::radioAdded(QSharedPointer<PayLoad> payload,
                           const QString &radioId,
                           QObject *radio,
                           Operation op)
{
    if (!payload || !radio) return;
    // ── Radio filter ──────────────────────────────────────────────────────
    if (!filterRadio) return;
    if (radioIndexMap.contains(radioId)) return;
    Radio* radioObj = qobject_cast<Radio*>(radio);
    if (!radioObj) return;
    int idx = ++maxRadioIndex;
    radioIndexMap[radioId] = idx;
    RadioData data;
    data.index = idx;
    data.id    = radioId;
    data.type  = "Radio";
    data.state = radioObj->toJson();
    data.state["_name"]           = QString::fromStdString(radioObj->Name);
    data.state["_parentEntityId"] = QString::fromStdString(
    radioObj->parentEntity ? radioObj->parentEntity->ID : "");
    payload->radioList->push_back(data);
    RadioCRUD crud;
    crud.index     = idx;
    crud.operation = op;
    payload->radioCRUDList->push_back(crud);
}

void Recording::radioUpdated(QSharedPointer<PayLoad> payload,
                             const QString &radioId,
                             QObject *radio)
{
    if (!payload || !radio) return;
    if (!radioIndexMap.contains(radioId)) return;
    Radio* radioObj = qobject_cast<Radio*>(radio);
    if (!radioObj) return;
    int idx = radioIndexMap[radioId];
    RadioData data;
    data.index = idx;
    data.id    = radioId;
    data.type  = "Radio";
    data.state = radioObj->toJson();
    payload->radioList->push_back(data);
    RadioCRUD crud;
    crud.index     = idx;
    crud.operation = UPDATE;
    payload->radioCRUDList->push_back(crud);
}

void Recording::radioRemoved(QSharedPointer<PayLoad> payload,
                             const QString &radioId)
{
    if (!payload) return;
    if (!radioIndexMap.contains(radioId)) return;
    RadioCRUD crud;
    crud.index     = radioIndexMap[radioId];
    crud.operation = DELETE;
    payload->radioCRUDList->push_back(crud);
    radioIndexMap.remove(radioId);
}

// ── Weapon ──────────────────
void Recording::weaponAdded(QSharedPointer<PayLoad> payload,
                            const QString &weaponId,
                            QObject *weapon,
                            Operation op)
{
    if (!payload || !weapon) return;
    if (weaponIndexMap.contains(weaponId)) return;
    Weapon* weaponObj = qobject_cast<Weapon*>(weapon);
    if (!weaponObj) return;
    int idx = ++maxWeaponIndex;
    weaponIndexMap[weaponId] = idx;
    WeaponData data;
    data.index = idx;
    data.id    = weaponId;
    data.type  = weaponObj->weaponTypeName();
    data.state = weaponObj->toJson();
    data.state["_name"]           = QString::fromStdString(weaponObj->Name);
    data.state["_parentEntityId"] = QString::fromStdString(
    weaponObj->parentEntity ? weaponObj->parentEntity->ID : "");
    payload->weaponList->push_back(data);
    WeaponCRUD crud;
    crud.index     = idx;
    crud.operation = op;
    payload->weaponCRUDList->push_back(crud);
}

void Recording::weaponUpdated(QSharedPointer<PayLoad> payload,
                              const QString &weaponId,
                              QObject *weapon)
{
    if (!payload || !weapon) return;
    if (!weaponIndexMap.contains(weaponId)) return;
    Weapon* weaponObj = qobject_cast<Weapon*>(weapon);
    if (!weaponObj) return;
    int idx = weaponIndexMap[weaponId];
    WeaponData data;
    data.index = idx;
    data.id    = weaponId;
    data.type  = weaponObj->weaponTypeName();
    data.state = weaponObj->toJson();
    payload->weaponList->push_back(data);
    WeaponCRUD crud;
    crud.index     = idx;
    crud.operation = UPDATE;
    payload->weaponCRUDList->push_back(crud);
}

void Recording::weaponRemoved(QSharedPointer<PayLoad> payload,
                              const QString &weaponId)
{
    if (!payload) return;
    if (!weaponIndexMap.contains(weaponId)) return;
    WeaponCRUD crud;
    crud.index     = weaponIndexMap[weaponId];
    crud.operation = DELETE;
    payload->weaponCRUDList->push_back(crud);
    weaponIndexMap.remove(weaponId);
}

// =========================================================================
// Replay::restoreIffsFromPayload
// =========================================================================

void Replay::restoreIffsFromPayload(QSharedPointer<PayLoad> payload)
{
    if (!payload) return;
    if (m_hierarchy->Platforms.empty()) return;
    if (iffsRestored) return;
    // DELETE
    for (auto &crud : *payload->iffCRUDList) {
        if (crud.operation == DELETE) {
            auto it = iffsIndexMap_replay.find(crud.index);
            if (it != iffsIndexMap_replay.end()) {
                m_hierarchy->Iffs.erase(it->second.id.toStdString());
                iffsIndexMap_replay.erase(it);
            }
        }
    }

    bool anyCreated = false;
    for (auto &idata : *payload->iffList) {
        if (iffsIndexMap_replay.count(idata.index)) {
            // UPDATE
            auto iffIt = m_hierarchy->Iffs.find(idata.id.toStdString());
            if (iffIt != m_hierarchy->Iffs.end())
                iffIt->second->fromJson(idata.state);
            continue;
        }
        iffsIndexMap_replay[idata.index] = idata;
        QString iffName        = idata.state["_name"].toString();
        if (iffName.isEmpty()) iffName = "IFF";
        QString parentEntityId = idata.state["_parentEntityId"].toString();
        if (parentEntityId.isEmpty()) continue;

        auto platIt = m_hierarchy->Platforms.find(parentEntityId.toStdString());
        if (platIt == m_hierarchy->Platforms.end()) continue;

        Platform* platform = platIt->second;
        QString compId;
        if (platform->iffs)
            compId = QString::fromStdString(platform->iffs->ID);
        else {
            platform->addComponent("iffs");
            if (platform->iffs)
                compId = QString::fromStdString(platform->iffs->ID);
            else continue;
        }

        m_hierarchy->addSubComponent(compId, iffName, "IFF", idata.id, idata.state);
    }
    iffsRestored = true;
    emit render(0.01f);
}

// =========================================================================
// Replay::restoreRadiosFromPayload
// =========================================================================

void Replay::restoreRadiosFromPayload(QSharedPointer<PayLoad> payload)
{
    if (!payload) return;
    if (m_hierarchy->Platforms.empty()) return;
    if (radiosRestored) return;

    for (auto &crud : *payload->radioCRUDList) {
        if (crud.operation == DELETE) {
            auto it = radiosIndexMap_replay.find(crud.index);
            if (it != radiosIndexMap_replay.end()) {
                m_hierarchy->Radios.erase(it->second.id.toStdString());
                radiosIndexMap_replay.erase(it);
            }
        }
    }
    bool hasRadioProfile = false;
    for (const auto& [key, profilePtr] : m_hierarchy->ProfileCategories) {
        if (profilePtr && profilePtr->type == Constants::EntityType::Radio) {
            hasRadioProfile = true;
            break;
        }
    }
    if (!hasRadioProfile) {
        ProfileCategaory* radioProfile = m_hierarchy->addProfileCategaory("Radios");
        if (radioProfile)
            radioProfile->setProfileType(Constants::EntityType::Radio);
    }

    bool anyCreated = false;
    for (auto &rdata : *payload->radioList) {
        if (radiosIndexMap_replay.count(rdata.index)) {
            auto radioIt = m_hierarchy->Radios.find(rdata.id.toStdString());
            if (radioIt != m_hierarchy->Radios.end())
                radioIt->second->fromJson(rdata.state);
            continue;
        }
        radiosIndexMap_replay[rdata.index] = rdata;
        QString radioName = rdata.state["_name"].toString();
        if (radioName.isEmpty()) radioName = "Radio";
        QString parentEntityId = rdata.state["_parentEntityId"].toString();
        if (parentEntityId.isEmpty()) continue;
        auto platIt = m_hierarchy->Platforms.find(parentEntityId.toStdString());
        if (platIt == m_hierarchy->Platforms.end()) continue;
        Platform* platform = platIt->second;
        if (!platform->radios)
            platform->addComponent("radios");
        if (!platform->radios) continue;

        QString compId = QString::fromStdString(platform->radios->ID);
        m_hierarchy->addSubComponent(compId, radioName, "Radio", rdata.id, rdata.state);
        anyCreated = true;
    }
    radiosRestored = true;
    emit render(0.01f);
}
// =========================================================================
// Replay::restoreWeaponsFromPayload
// =========================================================================

void Replay::restoreWeaponsFromPayload(QSharedPointer<PayLoad> payload)
{
    if (!payload) return;
    if (m_hierarchy->Platforms.empty()) return;
    if (weaponsRestored) return;

    // DELETE pass
    for (auto &crud : *payload->weaponCRUDList) {
        if (crud.operation == DELETE) {
            auto it = weaponsIndexMap_replay.find(crud.index);
            if (it != weaponsIndexMap_replay.end()) {
                m_hierarchy->Weapons.erase(it->second.id.toStdString());
                weaponsIndexMap_replay.erase(it);
            }
        }
    }

    bool hasWeaponProfile = false;
    for (const auto& [key, profilePtr] : m_hierarchy->ProfileCategories) {
        if (profilePtr && profilePtr->type == Constants::EntityType::Weapon) {
            hasWeaponProfile = true;
            break;
        }
    }

    if (!hasWeaponProfile) {
        ProfileCategaory* weaponProfile = m_hierarchy->addProfileCategaory("Weapons");
        if (weaponProfile)
            weaponProfile->setProfileType(Constants::EntityType::Weapon);
    }

    bool anyCreated = false;
    for (auto &wdata : *payload->weaponList) {
        if (weaponsIndexMap_replay.count(wdata.index)) {
            auto weaponIt = m_hierarchy->Weapons.find(wdata.id.toStdString());
            if (weaponIt != m_hierarchy->Weapons.end())
                weaponIt->second->fromJson(wdata.state);
            continue;
        }
        weaponsIndexMap_replay[wdata.index] = wdata;
        QString weaponName = wdata.state["_name"].toString();
        if (weaponName.isEmpty()) weaponName = "Weapon";
        QString parentEntityId = wdata.state["_parentEntityId"].toString();
        if (parentEntityId.isEmpty()) continue;
        auto platIt = m_hierarchy->Platforms.find(parentEntityId.toStdString());
        if (platIt == m_hierarchy->Platforms.end()) continue;
        Platform* platform = platIt->second;
        if (!platform->weapons)
            platform->addComponent("weapons");
        if (!platform->weapons) continue;
        QString compId = QString::fromStdString(platform->weapons->ID);
        m_hierarchy->addSubComponent(compId, weaponName, wdata.type, wdata.id, wdata.state);
        anyCreated = true;
    }
    weaponsRestored = true;
    emit render(0.01f);
}

void Recording::entityRenamedInBetween(
    QSharedPointer<PayLoad> payload,
    const QString &ID,
    const QString &newName)
{
    if (!entitiesIDIndex.contains(ID)) return;
    int idx = entitiesIDIndex.value(ID);
    EntitiesDetails updated;
    updated.index    = idx;
    updated.name     = newName;
    updated.ID       = ID;
    if (m_hierarchy) {
        auto it = m_hierarchy->Platforms.find(ID.toStdString());
        if (it != m_hierarchy->Platforms.end())
            updated.parentID = QString::fromStdString(it->second->parentID);
    }
    payload->entitiesDetailsList->push_back(updated);
    EntitiesCreated ec;
    ec.index = idx;
    payload->entitiesCreatedList->push_back(ec);
}
