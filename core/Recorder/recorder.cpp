#include "recorder.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Simulation/simulation.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

/* -------------------------------------------------------
 * Recording Implementation Information Start
 * ------------------------------------------------------*/

// Constructor: Initializes recorder with hierarchy and simulation pointers
Recorder::Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent)
    : QObject(parent), m_hierarchy(hierarchy), m_simulation(simulation)
{
    // Timer for replaying recorded frames
    //replayTimer = new QTimer(this);

    m_recording = new Recording(m_hierarchy, m_simulation, this, this);  // parented to Recorder
    m_replay    = new Replay   (m_hierarchy, m_simulation, this, this);
    emit sendRecorder();

    // For Default functtion Start
    loggerModeCheck(RECORDING);
    // For Default functtion Start
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
    // qDebug()<<"Recording: Information of Logger on Change:"
    // <<"\n\t Logger Mode  : "<<modeOfLogger
    // <<"\n\t Starting Time: "<<recordingStartTime
    // <<"\n\t Duration     : "<<durationText
    // <<"\n\t Status       : "<<loggerStatus
    // <<"\n\t Simulation   : "<<simulationStatus;
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
                               .arg(milliseconds, 3, 10, QLatin1Char('0')); // 3 digits for ms
    return durationText;
}

void Recorder::setLeftRightTimer(qint64 &left, qint64 &right)
{
    //qDebug()<<"Setting Left:"<<left<<" and Right:"<<right;
    rightTimer = &left;
    leftTimer  = &right;
}

void Recorder::setDuartionPtr(qint64 &s_durationPtr)
{
    qDebug()<<"Setting Duration Pointer:"<<s_durationPtr;
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


/* -------------------------------------------------------
 * Recording Implementation Information End
 * ------------------------------------------------------*/



/* -------------------------------------------------------
 * Recording Implementation Start
 * ------------------------------------------------------*/

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
    //    qDebug()<<"Recording: Information of Logger on Pressing Start Button"
    //    <<"\n\t Status       : "<<m_recorder->loggerStatus;
    rightTimer = 0;
    leftTimer  = 0;
    m_recorder->setLeftRightTimer(leftTimer , rightTimer);
    m_recorder->loggerInfo();
    recordingStart();
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
    //    qDebug()<<"Recording: Information of Logger on Pressing Start Button"
    //    <<"\n\t Status       : "<<m_recorder->loggerStatus;
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
    //    qDebug()<<"Recording: Information of Logger on Pressing Start Button"
    //    <<"\n\t Status       : "<<m_recorder->loggerStatus;
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
    //    qDebug()<<"Recording: Information of Logger on Pressing Start Button"
    //    <<"\n\t Status       : "<<m_recorder->loggerStatus;
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



void Recording::entityAddedAllFromStart()
{
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
    for (const auto& platform : *m_Platforms) {
        std::string name      = platform.second->Name.c_str();
        std::string parentID  = platform.second->parentID;
        std::string ID   = platform.first.c_str();
        entityAddedInBetween(parentID.c_str(), ID.c_str(), name.c_str());
    }
}



void Recording::recordingStart()
{
    m_recorder = getRecorder();
    mode = START;

    currentDateTime = QDateTime::currentDateTime();
    duration      = 0;

    if (recordingTimer == nullptr) {
        recordingTimer = new QTimer(this);
    }
    frameIndex = 0;
    recordingPeriod = 100;
    if(recordingTimer->isActive() == false){
        recordingTimer->start(recordingPeriod);
    }
    //entityAddedAllFromStart();
    connect(recordingTimer, &QTimer::timeout, this, [this]() {
        durationShared += recordingPeriod;
        ++frameIndex;
        entityUpdatesInBetween();
        debug("Timer: "+m_recorder->getStringTimer(durationShared),D_Timer);

        framePayLoad();
        emit updateUiDuration();
    });
    //emit started();
}
void Recording::framePayLoad()
{
    PayLoad payload{
        durationShared,
        frameIndex,
        m_entitiesDetailsList,
        m_entitiesCreatedList,
        m_entitiesUpdatedList,
        m_entitiesDeletedList
    };

    str = QString(
              "Frame PayLoad:=> "
              "Duration: %1  "
              "Frame Index: %2  "
              "Entities ID No: %3  "
              "Details List No: %4  "
              "Created List No: %5  "
              "Updated List No: %6  "
              "Deleted List No: %7  "
              ).arg(
                  QString::number(durationShared),
                  QString::number(frameIndex),
                  QString::number(entitiesIDIndex.size()),
                  QString::number(payload.entitiesDetailsList.size()),
                  QString::number(payload.entitiesCreatedList.size()),
                  QString::number(payload.entitiesUpdatedList.size()),
                  QString::number(payload.entitiesDeletedList.size())
                  );
    debug(str,D_FramePayLoad);
    emit sendPayLoad(payload);
    str = QString();
    m_entitiesDetailsList = EntitiesDetailsList();
    m_entitiesCreatedList = EntitiesCreatedList();
    m_entitiesUpdatedList = EntitiesUpdatedList();
    m_entitiesDeletedList = EntitiesDeletedList();
}


QDateTime Recording::startTime() const
{
    return m_startTime;
}


QJsonArray Recording::getFrameEntitiesData()
{
    //Resetting Frame Entities Array
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
        // emit updateUiDuration();
    }
}
void Recording::recordingStop()
{
    durationShared = 0;
    maxIndex =0;
    if(!recordingTimer){
        return;
    }
    if(!recordingTimer->isActive()){
        recordingTimer->stop();
        recordingTimer->destroyed();
    }
}
void Recording::recordingBookmark(const QString &message, qint64 timestampMs)
{

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

//In Between Running simulation

void Recording::entityAddedInBetween(const QString &parentID, const QString &ID, const QString &entityName)
{
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
    m_entitiesDetailsList.push_back(m_entitiesDetails);

    EntitiesCreated m_createdList = {
        .index = maxIndex
    };
    m_entitiesCreatedList.push_back(m_createdList);

    entitiesIDIndex.insert(ID,maxIndex);
    inspectEntitiesIDIndex();
    debug(str,D_EntityCreated);
    //emit entityCreated(parentID, ID, entityName, durationShared);
}
/*
 *    int    index       ;
 *    double longitude   ;
 *    double latitude    ;
 *    double altitude    ;
 *    double heading     ;
 *    float  turn_radius ;
 *    float  curr_speed  ;
 *    float  climb_rate  ;
 */
void Recording::entityUpdatesInBetween()
{
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
    EntitiesUpdated m_entitiesUpdated;
    Platform* platform;
    int indexSize = entitiesIDIndex.size();
    QString m_str = "Entity List Size" +QString(indexSize)+" Updates In Between: ";

    for(auto i = entitiesIDIndex.begin(), end = entitiesIDIndex.end();
         i != end ; ++i){
        QString m_str = i.key();
        platform = m_Platforms->at(m_str.toStdString());
        m_str += "Index :" + QString(i.value());
        if(platform != nullptr){
            m_entitiesUpdated.index       = i.value();
            m_entitiesUpdated.longitude   = platform->transform->getLongitude();
            m_entitiesUpdated.latitude    = platform->transform->getLatitude();
            m_entitiesUpdated.altitude    = platform->transform->getAltitude();
            m_entitiesUpdated.heading     = platform->transform->getHeading();
            m_entitiesUpdated.turn_radius = platform->dynamicModel->turnRate;
            m_entitiesUpdated.curr_speed  = platform->dynamicModel->currentSpeed;
            m_entitiesUpdated.climb_rate  = platform->dynamicModel->climbRate;
            m_entitiesUpdatedList.push_back(m_entitiesUpdated);
            m_str += "PASS";
        }else{
            m_str += "Failed";
        }
        m_str += "\n  ";
    }
    debug(m_str,D_UpdatesInBTW);
    //inspectEntitiesUpdatedList();
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
    m_entitiesDeletedList.push_back(m_entitiesDeleted);
    entitiesIDIndex.remove(ID);
    inspectEntitiesIDIndex();
    debug(str,D_EntityDeleted);

    //emit entityDeleted(ID,durationShared);
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
    // for(auto i: entitiesIDIndex){
    //     m_str += "\t ID:";
    //     m_str += i.key().toStdString();
    //     m_str += " => ";
    //     m_str += QString(i.second).toStdString();
    //     m_str += "\n";
    // }
    debug(m_str.c_str(),D_EntitiesIDIndex);
}

void Recording::inspectEntitiesUpdatedList()
{
    QString m_str = "Inpect Entities Update:    \n";
    for(auto i = m_entitiesUpdatedList.begin(),
         end = m_entitiesUpdatedList.end();
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



// void Recording::entityRemovedInBetween(std::string ID)
// {
//     str = QString(
//               "ID          : %1"
//               ).arg(ID.c_str());
//     debug(str,D_EntityDeleted);
//     //entityDeleted(id,durationShared);
// }

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


/* -------------------------------------------------------
 * Recording Implementation End
 * ------------------------------------------------------*/




/* -------------------------------------------------------
 * Replay Implementation Start
 * ------------------------------------------------------*/

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
}
void Replay::update()
{
    //m_recorder->recordingStartTime = NULL;
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
    m_Platforms = m_hierarchy->Platforms;

    m_recorder->loggerStatus = Recorder::S_REPLAYING;
    m_recorder->update(m_recorder->loggerStatus);
    rightTimer = 0;
    leftTimer  = 0;
    m_recorder->loggerInfo();
    m_recorder->setLeftRightTimer(leftTimer , rightTimer);
    qDebug()<<"Replay Mode is Set";
    m_recorder->loggerInfo();
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
    m_recorder = getRecorder();
    mode = START;
    //loadData();
    replayDateTime = QDateTime::currentDateTime();


    if (replayTimer == nullptr) {
        replayTimer = new QTimer(this);
    }

    replayPeriod = 100;
    if(replayTimer->isActive() == false){
        replayTimer->start(replayPeriod);
    }

    emit getMaxFrameIndexNDuration(&maxFrameIndex,&maxDuration);
    str = QString("Maximum FrameIndex: %1"
                  "Maximum Duration: %2"
                  "").arg(QString::number(maxFrameIndex), QString::number(maxDuration));
    debug(str, D_MaxFrameIndexNDuration);
    duration      = maxDuration;
    //QTimer::singleShot(durationLength, replayTimer, &QTimer::stop);

    frameIndex = 0;
    payload.entitiesDetailsList = {};
    entitiesIndexDetails = {};

    connect(replayTimer, &QTimer::timeout, this, [this]() {
        if(durationShared >= maxDuration || frameIndex >= maxFrameIndex){
            replayTimer->stop();
        }else{
            durationShared += replayPeriod;
            frameIndex++;
        }
        framePayLoad();
        //emit getFrame(frameIndex);
        //loadFrameEntitiesData();
        emit updateUiDuration();
    });
}

void Replay::framePayLoad()
{
    payload.frameIndex = frameIndex;
    emit getPayLoad(&payload);
    str = QString("  Duration : %1  ").arg(durationShared);
    if(entitiesIndexDetails.empty() && !payload.entitiesDetailsList.empty()){
        setEntitiesIndexDetails();
        str += QString("[ Index Detail Update Size: %1 ] ").arg(payload.entitiesDetailsList.size());
    }
    if(!payload.entitiesCreatedList.empty()){
        createEntitiesCreateList();
        str += QString("[ Entities Created Size: %1 ] ").arg(payload.entitiesCreatedList.size());
    }
    if(!payload.entitiesUpdatedList.empty()){
        updateEntitiesUpdatedList();
        str += QString("[ Entities Updates Size: %1 ] ").arg(payload.entitiesUpdatedList.size());
    }
    if(!payload.entitiesDeletedList.empty()){
        deleteEntitiesDeletedList();
        str += QString("[ Entities Deleted Size: %1 ] ").arg(payload.entitiesDeletedList.size());
    }
    debug(str,D_PayLoad_Inspect);
    payload.entitiesCreatedList = {};
    payload.entitiesUpdatedList = {};
    payload.entitiesDeletedList = {};
}

void Replay::setEntitiesIndexDetails()
{

    for(auto ed = payload.entitiesDetailsList.begin();
         ed != payload.entitiesDetailsList.end();
         ++ed)
    {
        entitiesIndexDetails.insert({ed->index,*ed});
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

void Replay::createEntitiesCreateList()
{
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
    str = QString();
    for(auto ec = payload.entitiesCreatedList.begin();
         ec != payload.entitiesCreatedList.end();
         ++ec)
    {
        EntitiesDetails &ed = entitiesIndexDetails.at(ec->index);
        str += QString(
                   "Entity Name: %1 \t"
                   "ID: %2 \t"
                   ).arg(ed.name,ed.ID);

        emit createEntitiesCreate(ed.parentID, ed.ID, ed.name, true);
        Platform* platform = m_Platforms->at(ed.ID.toStdString());
        platform->addComponent("transform");
        platform->addComponent("crossSection");
        platform->addComponent("trajectory");
        platform->addComponent("rigidbody");
        platform->addComponent("dynamicModel");
        platform->addComponent("collider");
        platform->addComponent("bitmap");
        platform->addComponent("sensors");
        platform->addComponent("iffs");
        platform->addComponent("radios");
    }
    debug(str,D_EntitiesCreateList);
}
/*
 *      struct EntitiesUpdated {
 *          int    index       ;
 *          double longitude   ;
 *          double latitude    ;
 *          double altitude    ;
 *          double heading     ;
 *          float  turn_radius ;
 *          float  curr_speed  ;
 *          float  climb_rate  ;
 *      };
 */
void Replay::updateEntitiesUpdatedList()
{
    std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
    for(auto eu = payload.entitiesUpdatedList.begin();
         eu != payload.entitiesUpdatedList.end();
         ++eu)
    {
        EntitiesDetails &ed = entitiesIndexDetails.at(eu->index);

        auto it = m_Platforms->find(ed.ID.toStdString());
        if(it == m_Platforms->end() ){
            continue;
        }else{
            Platform* platform = m_Platforms->at(ed.ID.toStdString());
            platform->transform->setLongitude (eu->longitude);
            platform->transform->setLatitude  (eu->latitude );
            platform->transform->setAltitude  (eu->altitude );
            platform->transform->setHeading   (eu->heading  );
            platform->dynamicModel->turnRate     = eu->turn_radius;
            platform->dynamicModel->currentSpeed = eu->curr_speed ;
            platform->dynamicModel->climbRate    = eu->climb_rate ;

            // platform->transform->getLongitude();
            // platform->transform->getLatitude();
            // platform->transform->getAltitude();
            // platform->transform->getHeading();
            // platform->dynamicModel->turnRate;
            // platform->dynamicModel->currentSpeed;
            // platform->dynamicModel->climbRate;
        }

        //emit createEntitiesCreate(ed.parentID, ed.ID, ed.name, false);
    }
    emit render(0.01);
}

void Replay::deleteEntitiesDeletedList()
{
    str = QString();
    for(auto edl = payload.entitiesDeletedList.begin();
         edl != payload.entitiesDeletedList.end();
         ++edl)
    {
        EntitiesDetails &ed = entitiesIndexDetails.at(edl->index);
        str += QString(
                   "[ ID : %1 ,"
                   "Index : %2 ] ")
                   .arg(ed.ID,ed.index);
        emit deleteEntities(ed.parentID, ed.ID, true);
    }
    debug(str,D_EntitiesDeletedList);
}


// typedef struct{
//     double longitude  ;
//     double latitude   ;
//     double altitude   ;
//     double heading    ;
//     float  turn_radius;
//     float  curr_speed ;
//     float  climb_rate ;
// }entity;
// std::unordered_map<int , entity> frame;
void Replay::loadFrameEntitiesData()
{
    if(entitiesMap.empty() || frameMap.empty()){
        return;
    }
    for(const auto& i: frame){
        std::string id   =  entitiesMap[i.first].first;
        std::string name =  entitiesMap[i.first].second;

        entity m_entity = i.second;

        if(m_Platforms->find(id) != m_Platforms->end()){
            Platform* platform = (* m_hierarchy->Platforms)[id];
            platform->transform->setAltitude(m_entity.altitude);
            platform->transform->setLatitude(m_entity.latitude);
            platform->transform->setLongitude(m_entity.longitude);
            platform->transform->setHeading(m_entity.heading);
            //platform->transform->setFromEulerAngles(QVector3D(0,m_entity.heading,0));


            platform->dynamicModel->turnRadius   = m_entity.turn_radius ;
            platform->dynamicModel->moveSpeed    = m_entity.curr_speed  ;
            platform->dynamicModel->currentSpeed = m_entity.curr_speed  ;
            emit render(0.02);
            qDebug()<<"********It Exist**********";
        }else{
            qDebug()<<"******It Not Exist*********";
        }
    }
}

void Replay::toggle()
{
    // Case 1: Currently Playing -> PAUSE
    if (replayTimer && replayTimer->isActive()) {
        pause();
    }
    // Case 2: Currently Paused -> RESUME
    else if (isPaused) {
        resume();
    }
    // Case 3: Stopped (Timer is null) -> START
    // This handles the case where it finished, you moved back, and clicked Toggle.
    else {
        start();
    }
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
        if(durationLength > durationShared+jumpStep && frameIndex <= frameMap.size() - 50 ){
            durationShared += jumpStep;
            frameIndex += 50;
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
        if(durationShared > jumpStep && frameIndex > 50){
            durationShared -= jumpStep;
            frameIndex -= 50;
        }else{
            //qDebug()<<"Not Allowed Duration: "<<durationShared<<" < "<<jumpStep;
        }
    }else{
        //qDebug()<<mode<<"Not Allowed";
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

/*------------     Custom Debugger End     ------------*/

/* -------------------------------------------------------
 * Replay Implementation End
 * ------------------------------------------------------*/





// /* -------------------------------------------------------
//  * Recording Implementation Start
//  * ------------------------------------------------------*/

// Recording::Recording(
//     Hierarchy* hierarchy,
//     Simulation* simulation,
//     Recorder *parentRecorder,
//     QObject *parent) :
//     QObject(parent),
//     m_hierarchy(hierarchy),
//     m_simulation(simulation),
//     m_recorder(parentRecorder)

// {
//     m_recorder = getRecorder();
// }


// void Recording::update()
// {
//     m_recorder->recordingStartTime = QDateTime::currentDateTime();
//     m_recorder->duration           = 0;
//     m_recorder->loggerStatus       = Recorder::S_RECORDING_MODE;
//     m_recorder->simulationStatus   = Recorder::S_SIMULATION_NA;
//     emit m_recorder->recorderInfoSendOnce(
//         m_recorder->recordingStartTime ,
//         m_recorder->duration           ,
//         m_recorder->loggerStatus       ,
//         m_recorder->simulationStatus   );
//     // m_recorder->loggerInfo();
// }


// void Recording::start()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_RECORDING;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     // m_recorder->loggerInfo();
//     recordingStart();
// }

// void Recording::pause()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_RECORDING_PAUSED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     recordingPauseResume();
// }

// void Recording::resume()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_RECORDING;
//     m_recorder->update(m_recorder->loggerStatus);
//     // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     recordingPauseResume();
// }

// void Recording::stop()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_RECORDING_STOPPED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Recording: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     recordingStop();
// }

// void Recording::addBookmark()
// {
//     return;
// }

// void Recording::insertRecord(const QJsonObject &data)
// {
//     recordedFile.append(data);
//     recordedData = data;
// }

// void Recording::recordingStart()
// {
//     m_recorder = getRecorder();

//     currentDateTime = QDateTime::currentDateTime();
//     recordingPeriod = 100;

//     // Create timer once (if not already created)
//     if (!recordingTimer) {
//         recordingTimer = new QTimer(this);

//         // Timer lambda must capture this so it sees current members
//         connect(recordingTimer, &QTimer::timeout, this, [this]() {
//             qint64 elapsedMs = 0;

//             if (mode == STOP) {

//             }
//             else if (mode == PAUSE) {

//             }
//             else { // START


//             }
//             if (mode == START) {

//             }
//         });
//     }

//     // Start (or restart) timer
//     if (!recordingTimer->isActive())
//         recordingTimer->start(recordingPeriod);

//     // notify UI / outer systems
//     emit started();
// }
// void Recording::recordingStart()
// {
//     m_recorder = getRecorder();

//     // Ensure consistent state
//     mode = START;
//     recordingStartTime = QDateTime::currentDateTime();
//     pausedOffsetMs = 0;         // accumulated time before current segment
//     lastElapsedMs = 0;
//     isPaused = false;
//     noOfFrame = 1;

//    // qDebug() <<"Recording: Information of Logger "
//              // <<"on Pressing Start Button\n"
//              // <<"Start Recording:";

//     // Reset bookmark array on new recording
//     bookmarks = QJsonArray();
//     if (!m_hierarchy) {
//         qWarning() << "Hierarchy is null. Cannot record.";
//         return;
//     }
//     /* New Type of recording Start */
//     recordedStructure = QJsonObject();
//     rs_hierarchyObj   = QJsonObject();
//     rs_hierarchyObj.insert("timestamp_ms" , lastElapsedMs);
//     rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
//     rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());
//     rs_hierarchy      = QJsonArray();
//     rs_hierarchy.append(rs_hierarchyObj);

//     rs_bookmark = QJsonArray();
//     rs_frame    = QJsonArray();

//     /* New Type of recording End   */

//     // Insert metadata frame (frame 0)
//     QJsonObject metaData;
//     metaData["bookmark"] = QJsonArray();
//     insertRecord(metaData);                // stored at recordedFile[0]

//     currentDateTime = QDateTime::currentDateTime();
//     recordingPeriod = 100;

//     // Create timer once (if not already created)
//     if (!recordingTimer) {
//         recordingTimer = new QTimer(this);

//         // Timer lambda must capture this so it sees current members
//         connect(recordingTimer, &QTimer::timeout, this, [this]() {
//             qint64 elapsedMs = 0;
//             if (mode == STOP) {
//                 // freeze
//                 elapsedMs = lastElapsedMs;
//                 // Addding last value;
//                // qDebug()<<"Stop is called in loop";
//             }
//             else if (mode == PAUSE) {
//                 // when paused, keep the last elapsed value
//                 elapsedMs = lastElapsedMs;
//                 // emit time update for UI if needed, but do not add frames
//                 emit m_recorder->recordingTimeUpdated(elapsedMs);
//                 return;  // do not insert a new snapshot while paused
//             }
//             else { // START
//                 // total time = already-accumulated + current segment
//                 elapsedMs = pausedOffsetMs + recordingStartTime.msecsTo(QDateTime::currentDateTime());
//                 lastElapsedMs = elapsedMs;
//                 emit mapFrame(lastElapsedMs);
//                 //Adding Additional logic

//             }

//             // Emit timeline update
//             emit m_recorder->recordingTimeUpdated(elapsedMs);

//             // Insert a snapshot frame only when recording (not on STOP/PAUSE)
//             if(changeInHierarchyInPause){
//                 //Adding last value;
//                 rs_hierarchyObj   = QJsonObject();
//                 rs_hierarchyObj.insert("timestamp_ms" , elapsedMs);
//                 rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
//                 rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());

//                 changeInHierarchyInPause = false;

//             }
//             if (mode == START) {
//                 // QJsonObject timeEntry;
//                 // timeEntry["timestamp_ms"] = elapsedMs;
//                 // timeEntry["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
//                 // timeEntry["snapshot"] = m_hierarchy->toJson();
//                 // insertRecord(timeEntry);
//                 //// qDebug() <<"\t"<< noOfFrame++ <<". Frame"<< elapsedMs;

//                 //Creating New Frame Data Object
//                 rs_frameObj = QJsonObject();
//                 rs_frameObj.insert("timestamp_ms" , elapsedMs);
//                 rs_frameObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
//                 rs_frameObj.insert("frameEntities",getFrameEntitiesData());

//                 //Adding to frame array
//                 rs_frame.append(rs_frameObj);
//                 //qDebug()<<"\t"<<rs_frameObj;
//             }
//         });
//     }

//     // Start (or restart) timer
//     if (!recordingTimer->isActive())
//         recordingTimer->start(recordingPeriod);

//     // notify UI / outer systems
//     emit started();
// }

// QJsonArray Recording::getFrameEntitiesData()
// {
//     //Resetting Frame Entities Array
//     frameEntities = QJsonArray();
//     std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;
//     for(const auto& platform : *m_Platforms){
//         //Resetting Entity Object
//         entityObj = QJsonObject();
//         Entity* entity = platform.second;
//         //Getting Name
//         const std::string& name = entity->Name.c_str();
//         const std::string& id   = entity->ID.c_str();
//         //Getting Lat & Long and Alt
//         QVector3D     matrix       = platform.second->transform->translation();
//         //Rotation angle
//         QVector3D     angle_matrix = platform.second->transform->toEulerAngles();
//         //Current Speed
//         DynamicModel* dynamicModel = platform.second->dynamicModel;

//         if (entity) {
//             //Setting the name and ID
//             //// qDebug()<<"Entity Name: "<<name.c_str()
//             //         <<", ID: "<<id.c_str();
//             entityObj.insert("name",name.c_str());
//             entityObj.insert("id"  ,id.c_str());

//             //Setting Coorinates
//             //// qDebug()<<"\tCoorinates"
//             //         <<" , "<<matrix.x()
//             //         <<" , "<<matrix.y()
//             //         <<" , "<<matrix.z();
//             entityObj.insert("cord_x",matrix.x());
//             entityObj.insert("cord_y",matrix.y());
//             entityObj.insert("cord_z",matrix.z());

//             //Setting Rotation axies
//             //// qDebug()<<"\tRotation axies"
//             //         <<" : "<<angle_matrix.x()
//             //         <<" , "<<angle_matrix.y()
//             //         <<" , "<<angle_matrix.z();
//             entityObj.insert("axis_x",angle_matrix.x());
//             entityObj.insert("axis_y",angle_matrix.y());
//             entityObj.insert("axis_z",angle_matrix.z());

//             //Setting Dynamic values
//             //// qDebug()<<"\tDynamic Components: "
//             //         <<" Turn Radius "  <<dynamicModel->turnRadius
//             //         <<" Maximum Speed "<<dynamicModel->moveSpeed
//             //         <<" Current Speed "<<dynamicModel->currentSpeed;
//             entityObj.insert("turn_radius",dynamicModel->turnRadius);
//             entityObj.insert("max_speed"  ,dynamicModel->moveSpeed);
//             entityObj.insert("curr_speed" ,dynamicModel->currentSpeed);

//         }
//         //qDebug()<<entityObj;
//         frameEntities.append(entityObj);
//         //End
//     }
//     return frameEntities;
// }

// // QJsonObject Recording::getFrameEntitiesData(Platform *platform)
// // {
// //     QJsonObject jsonEntity;
// //     return jsonEntity;
// // }

// void Recording::recordingPauseResume()
// {
//     if (!recordingTimer) return;

//     if (mode == START)
//     {
//         // Enter pause
//         mode = PAUSE;
//         // accumulate the time we've recorded in this segment
//         if (recordingStartTime.isValid()) {
//             qint64 segmentMs = recordingStartTime.msecsTo(QDateTime::currentDateTime());
//             pausedOffsetMs += segmentMs;
//             lastElapsedMs = pausedOffsetMs; // freeze value
//         }
//         pauseStartTime = QDateTime::currentDateTime();
//         isPaused = true;

//        // qDebug() <<"Recording: Information of Logger "
//                  // <<"on Pressing Pause\n"
//                  // <<"Pause Recording:";
//         emit paused();
//     }
//     else if (mode == PAUSE)
//     {
//         // Resume
//         mode = START;
//         // reset start reference for the new segment
//         recordingStartTime = QDateTime::currentDateTime();
//         isPaused = false;

//        // qDebug() <<"Recording: Information of Logger "
//                  // <<"on Pressing Resume\n"
//                  // <<"Resume Recording:";
//         emit started();
//     }
// }
// void Recording::recordingStop()
// {
//     //Introduction Before Start
//     if (recordingTimer) {
//         recordingTimer->stop();
//         recordingTimer->deleteLater();
//         recordingTimer = nullptr;
//     }
//     // Reset recording start time
//    // qDebug() <<"Recording: Information of Logger "
//              // <<"on Pressing Stop Button\n"
//              // <<"Stop Recording:";
//     m_startTime = QDateTime();
//     noOfFrame = 0;
//     //saveFile();
//     //qDebug() << "Recorder cleanup done.";

//     recordedStructure.insert("rs_frame",rs_frame);
//     recordedStructure.insert("rs_bookmark",rs_bookmark);
//     recordedStructure.insert("rs_hierarchy",rs_hierarchy);
//     saveFile();
//     //    QJsonDocument doc(recordedStructure);
//     //    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
//     //   // qDebug()<<jsonData;

// }
// void Recording::recordingBookmark(const QString &message, qint64 timestampMs)
// {
//     if (rs_frame.isEmpty()) {
//         qWarning() << "Cannot bookmark in empty file";
//         return;
//     }
//     // If caller didn't compute timestampMs, compute using current state:
//     qint64 ts = timestampMs;
//     if (ts < 0) {
//         // compute total elapsed: accumulated + current segment (or lastElapsedMs if paused)
//         if (isPaused) {
//             ts = lastElapsedMs;
//         } else if (recordingStartTime.isValid()) {
//             ts = pausedOffsetMs + recordingStartTime.msecsTo(QDateTime::currentDateTime());
//         } else {
//             ts = pausedOffsetMs; // fallback
//         }
//     }
//     // Bookmark entry
//     rs_bookmarkObj = QJsonObject();
//     rs_bookmarkObj["timestamp_ms"] = ts;
//     rs_bookmarkObj["current_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
//     rs_bookmarkObj["message"]      = message;

//     // Append and write back
//     rs_bookmark.append(rs_bookmarkObj);
//    // qDebug() << "Bookmark Recorded At:" << ts << "Message:" << message<<rs_bookmark;

// }

// void Recording::getSimulationUpdate()
// {

//     //qDebug()<<simUpdate<<" "<<simUpdateType;
// }

// void Recording::changeInHierarchy()
// {
//     rs_hierarchyObj   = QJsonObject();
//     rs_hierarchyObj.insert("timestamp_ms" , lastElapsedMs );
//     rs_hierarchyObj.insert("current_time" , QDateTime::currentDateTime().toString(Qt::ISODate));
//     rs_hierarchyObj.insert("hierarchy"    , m_hierarchy->toJson());
//     rs_hierarchy.append(rs_hierarchyObj);
//     //qDebug()<<"Change In Hierarchy";
// }

// void Recording::saveFile()
// {
//     if (recordedStructure.isEmpty()) {
//         qWarning() << "Recording: Saving Failed"
//                    << "/n/tRecorded File is empty, nothing to save.";
//         return;
//     }
//     // ---- Ask User Where to Save ----
//     QString defaultName =
//         "Recording_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
//     QString filePath = QFileDialog::getSaveFileName(
//         nullptr,
//         tr("Save Recording File"),
//         QDir::homePath() + "/" + defaultName,   // default save location
//         tr("JSON Files (*.json);;All Files (*)")
//         );
//     if (filePath.isEmpty()) {
//        // qDebug() << "User cancelled save dialog.";
//         return;
//     }
//     // ---- Convert QJsonArray → JSON ----
//     QJsonDocument doc(recordedStructure);
//     QByteArray json = doc.toJson(QJsonDocument::Indented);

//     // ---- Save the file ----
//     QFile file(filePath);
//     if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//         qWarning() << "Could not open file for writing:" << filePath;
//         return;
//     }
//     file.write(json);
//     file.close();
//    // qDebug() << "Recording saved successfully at:" << filePath;

// }

// QDateTime Recording::startTime() const
// {
//     return m_startTime;
// }

// qint64 Recording::duration() const
// {
//     return m_duration;
// }
// /* -------------------------------------------------------
//  * Recording Implementation End
//  * ------------------------------------------------------*/




// /* -------------------------------------------------------
//  * Replay Implementation Start
//  * ------------------------------------------------------*/

// Replay::Replay(
//     Hierarchy* hierarchy,
//     Simulation* simulation,
//     Recorder *parentRecorder,
//     QObject *parent) :
//     QObject(parent),
//     m_hierarchy(hierarchy),
//     m_simulation(simulation),
//     m_recorder(parentRecorder)
// {
//     m_recorder = getRecorder();
// }
// void Replay::update()
// {
//     //m_recorder->recordingStartTime = NULL;
//     m_recorder->duration           = 0;
//     m_recorder->loggerStatus       = Recorder::S_REPLAY_MODE;
//     m_recorder->simulationStatus   = Recorder::S_SIMULATION_STOP;
//     emit m_recorder->recorderInfoSendUsual(
//         m_recorder->duration           ,
//         m_recorder->loggerStatus       ,
//         m_recorder->simulationStatus   );
//     m_recorder->loggerInfo();
// }

// bool Replay::replayLoaded(const QString &filePath)
// {
//     // [CRITICAL] Stop any existing replay before touching data
//     stop();

//     QFile loadFile(filePath);
//     if (!loadFile.open(QIODevice::ReadOnly)) {
//         qWarning("Couldn't open replay file.");
//         return false;
//     }

//     QByteArray saveData = loadFile.readAll();
//     loadFile.close();

//     QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
//     if (!loadDoc.isObject()) {
//         qWarning("Replay file format error: Expected JSON Object.");
//         return false;
//     }

//     recordedStructure  = loadDoc.object();
//     rs_frame = recordedStructure["rs_frame"].toArray();

//     playbackFrames.clear();
//     playbackFrames.reserve(rs_frame.size());

//     // Convert JSON array to QVector
//     for (const QJsonValue &val : rs_frame) {
//         if (val.isObject())
//             playbackFrames.append(val.toObject());
//     }

//     qint64 maxTimestamp = 0;

//     // Compute duration
//     for (const QJsonObject &frm : playbackFrames) {
//         if (frm.contains("timestamp_ms")) {
//             maxTimestamp = qMax(maxTimestamp, frm["timestamp_ms"].toVariant().toLongLong());
//         }
//     }
//     emit setReplayDuration(maxTimestamp);

//     // Load bookmarks
//     if (recordedStructure.contains("rs_bookmark") && recordedStructure["rs_bookmark"].isArray()) {
//         QJsonArray arr = recordedStructure["rs_bookmark"].toArray();
//         for (const QJsonValue &v : arr) {
//             if (!v.isObject()) continue;
//             QJsonObject b = v.toObject();
//             QString message = b["message"].toString();
//             qint64 timestamp = b["timestamp_ms"].toVariant().toLongLong();
//             emit replayBookmark(message, timestamp);
//         }
//     }

//     // Load first frame (SKIP metadata at index 0)
//     if (playbackFrames.size() > 0) {
//         currentReplayIndex = 0;

//         // Try to load initial hierarchy if available
//         if(recordedStructure.contains("rs_hierarchy")) {
//             rs_hierarchy = recordedStructure["rs_hierarchy"].toArray();
//             if(!rs_hierarchy.isEmpty()) {
//                 const QJsonObject &firstSnapshot = rs_hierarchy.at(0).toObject();
//                 if (firstSnapshot.contains("hierarchy") && m_hierarchy) {
//                     m_hierarchy->fromJson(firstSnapshot["hierarchy"].toObject());
//                 }
//             }
//         }
//     }
//     else {
//         qWarning() << "Replay: Not enough frames to replay.";
//         return false;
//     }

//     // [CRITICAL] Reset index to 1 (skipping metadata) so it is ready to play
//     currentReplayIndex = 1;

//     // Notify UI that file is loaded
//     fileLoaded();

//     return true;
// }
// void Replay::createTimer()
// {
//     // If timer exists, do nothing (or recreate if you prefer safety)
//     if (replayTimer) return;

//     replayTimer = new QTimer(this);

//     //
//     connect(replayTimer, &QTimer::timeout, this, [this]() {
//         if (currentReplayIndex < playbackFrames.size()) {
//             const QJsonValue &frame = playbackFrames.at(currentReplayIndex);

//             // 1. Update UI Timeline (Blue Line)
//             QJsonObject frameObj = frame.toObject();
//             if(frameObj.contains("timestamp_ms")){
//                 qint64 ts = frameObj["timestamp_ms"].toVariant().toLongLong();
//                 emit replayFrameLoaded(ts);
//             }

//             // 2. Update 3D Scene (Movement)
//             loadFrameEntitiesData(frame);
//             emit updateScene(0.1f);

//             currentReplayIndex++;
//         } else {
//             // End of playback
//             stop();
//         }
//     });
// }

// void Replay::replayStart()
// {

// }

// void Replay::start()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//     if (playbackFrames.isEmpty()) {
//         qWarning() << "Replay: No frames to replay";
//         return;
//     }

//     // --- LOGIC FIX: Don't reset if we are in the middle ---
//     // Only reset to 1 if we are starting fresh or reached the end previously.
//     // If you scrubbed back (goToPreviousFrame), currentReplayIndex will be < size,
//     // so this block is skipped and it plays from where you are.
//     if (!isPaused) {
//         if (currentReplayIndex >= playbackFrames.size() - 1 || currentReplayIndex <= 0) {
//             currentReplayIndex = 1;
//         }
//     }

//     // Ensure timer is created (using the createTimer helper from previous fixes)
//     // If you don't have createTimer, paste the timer creation code here.
//     createTimer();

//     if (!replayTimer->isActive()) {
//         replayTimer->start(100);
//     }

//     isPaused = false;
//    // qDebug() << "Replay STARTED from index" << currentReplayIndex;
// }
// void Replay::loadFrameEntitiesData(const QJsonValue frame)
// {
//     //Get the Hierarchy
//     std::unordered_map<std::string, Platform*> *m_Platforms = m_hierarchy->Platforms;

//     rs_frameObj = QJsonObject();
//     rs_frameObj = frame.toObject();
//     if(rs_frameObj.contains("timestamp_ms")){
//         qint64 ts = rs_frameObj["timestamp_ms"].toVariant().toLongLong();
//         emit replayFrameLoaded(ts);
//     }
//     if(rs_frameObj.contains("frameEntities")){
//         frameEntities = QJsonArray();
//         frameEntities = rs_frameObj["frameEntities"].toArray();
//         for(const QJsonValue &rs_entity : frameEntities){
//             entityObj = QJsonObject();
//             entityObj = rs_entity.toObject();
//             std::string id = entityObj["id"].toString().toStdString();


//             if(m_Platforms->find(id) != m_Platforms->end()){
//                 Platform* platform = (* m_hierarchy->Platforms)[id];

//                 float x = static_cast<float>(entityObj["cord_x"].toDouble());
//                 float y = static_cast<float>(entityObj["cord_y"].toDouble());
//                 float z = static_cast<float>(entityObj["cord_z"].toDouble());
//                 platform->transform->matrix->setTranslation(QVector3D(x,y,z));

//                 float axis_x = static_cast<float>(entityObj["axis_x"].toDouble());
//                 float axis_y = static_cast<float>(entityObj["axis_y"].toDouble());
//                 float axis_z = static_cast<float>(entityObj["axis_z"].toDouble());
//                 platform->transform->setFromEulerAngles(QVector3D(axis_x,axis_y,axis_z));

//                 float turn_radius = static_cast<float>(entityObj["turn_radius"].toDouble());
//                 float max_speed   = static_cast<float>(entityObj["max_speed"].  toDouble());
//                 float curr_speed  = static_cast<float>(entityObj["curr_speed"]. toDouble());
//                 platform->dynamicModel->turnRadius   = turn_radius ;
//                 platform->dynamicModel->moveSpeed    = max_speed   ;
//                 platform->dynamicModel->currentSpeed = curr_speed  ;
//                 //qDebug()<<"********It Exist**********";
//             }
//         }
//     }
// }

// void Replay::toggle()
// {
//     // Case 1: Currently Playing -> PAUSE
//     if (replayTimer && replayTimer->isActive()) {
//         pause();
//     }
//     // Case 2: Currently Paused -> RESUME
//     else if (isPaused) {
//         resume();
//     }
//     // Case 3: Stopped (Timer is null) -> START
//     // This handles the case where it finished, you moved back, and clicked Toggle.
//     else {
//         start();
//     }
// }

// void Replay::pause()
// {
//     m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
//     //New
//     if (replayTimer && replayTimer->isActive()) {
//         replayTimer->stop();
//         isPaused = true;
//         m_recorder->loggerStatus = Recorder::S_REPLAY_PAUSED;
//         m_recorder->update(m_recorder->loggerStatus);
//         m_recorder->loggerInfo();
//        // qDebug() << "Replay PAUSED";
//     }
// }


// void Replay::resume()
// {
//     m_recorder = getRecorder();

//     // 1. Safety Check: If we paused, but the timer was deleted (e.g. after Stop), recreate it.
//     if (!replayTimer) {
//         createTimer();
//     }

//     // 2. Standard Resume Logic
//     if (!isPaused) {
//         qWarning() << "Replay: Resume called but not paused";
//         return;
//     }

//     isPaused = false;

//     // Check active before starting to avoid warnings
//     if (!replayTimer->isActive()) {
//         replayTimer->start(100);
//     }

//     m_recorder->loggerStatus = Recorder::S_REPLAYING;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//    // qDebug() << "Replay RESUMED";
// }

// void Replay::stop()
// {
//     m_recorder = getRecorder(); // Ensure m_recorder is valid
//     m_recorder->loggerStatus = Recorder::S_REPLAY_STOPPED;
//     m_recorder->update(m_recorder->loggerStatus);
//     m_recorder->loggerInfo();

//     if (replayTimer) {
//         replayTimer->stop();
//         replayTimer->deleteLater();
//         replayTimer = nullptr; // Important: set to null so createTimer() knows to recreate it
//     }

//     // Reset to beginning so if user clicks Play, it starts over (optional, or keep current pos)
//     // currentReplayIndex = 1;
//     isPaused = false;

//    // qDebug() << "Replay STOPPED";
// }

// void Replay::restart()
// {
//    // qDebug() << "Replay: Restart Requested";
//     stop(); // Clears timer
//     currentReplayIndex = 1; // Resets to start
//     start(); // Creates new timer and runs
// }
// void Replay::playAgain()
// {
//     restart(); // Just reuse restart logic
// }

// void Replay::fileLoaded()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAY_LOADED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
// }

// void Replay::fileUnloaded()
// {
//     //m_recorder = getRecorder();
//     m_recorder->loggerStatus = Recorder::S_REPLAY_UNLOADED;
//     m_recorder->update(m_recorder->loggerStatus);
//    // qDebug()<<"Replay: Information of Logger on Pressing Start Button"
//              // <<"\n\t Status       : "<<m_recorder->loggerStatus;
//     m_recorder->loggerInfo();
// }

// void Replay::goToNextFrame()
// {
//     if (playbackFrames.isEmpty()) return;

//     if (currentReplayIndex + 50 < playbackFrames.size()) {
//         currentReplayIndex += 50;

//         const QJsonObject &frame = playbackFrames.at(currentReplayIndex);
//         if (frame.contains("hierarchy")) {
//             m_hierarchy->fromJson(frame["hierarchy"].toObject());
//         }

//         emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//     }
// }



// void Replay::goToPreviousFrame()
// {
//     if (playbackFrames.isEmpty()) return;

//     // 1. REMOVE STOP LOGIC
//     // We want the replay to keep running (or start running), so do NOT stop the timer here.

//     // 2. Handle "End of Replay" scenario
//     // If we were stopped/finished, reset index from the end to allow stepping back.
//     if (currentReplayIndex >= playbackFrames.size()) {
//         currentReplayIndex = playbackFrames.size();
//     }

//     // 3. Perform the Step Back
//     if (currentReplayIndex >= 50) {
//         currentReplayIndex -= 50;
//     } else {
//         currentReplayIndex = 0; // Clamp to start
//     }

//     // 4. Update Data and Visuals (Immediate Feedback)
//     if (currentReplayIndex < playbackFrames.size()) {
//         QJsonObject frame = playbackFrames.at(currentReplayIndex);

//         // Update Hierarchy
//         if (frame.contains("hierarchy")) {
//             m_hierarchy->fromJson(frame["hierarchy"].toObject());
//         }

//         // Update 3D Scene
//         loadFrameEntitiesData(frame);
//         emit updateScene(0.1f);

//         // Update Timeline
//         if (frame.contains("timestamp_ms")) {
//             emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//         }
//     }

//     // =========================================================
//     // CRITICAL FIX: AUTO-RESUME / FORCE PLAY
//     // =========================================================
//     // The user wants replay to START (or continue) immediately.

//     // A. Ensure Timer Exists
//     if (!replayTimer) {
//         replayTimer = new QTimer(this);
//         // Re-connect the timer loop logic if you created a new one
//         connect(replayTimer, &QTimer::timeout, this, [this]() {
//             if (currentReplayIndex < playbackFrames.size()) {
//                 const QJsonValue &frame = playbackFrames.at(currentReplayIndex);
//                 loadFrameEntitiesData(frame);
//                 emit updateScene(0.1f);

//                 // Update Timeline
//                 if(frame.toObject().contains("timestamp_ms")){
//                     emit replayFrameLoaded(frame.toObject()["timestamp_ms"].toVariant().toLongLong());
//                 }

//                 currentReplayIndex++;
//             } else {
//                 stop();
//             }
//         });
//     }

//     // B. Set Flag to Playing
//     isPaused = false;

//     // C. Start Timer if not running (e.g., if we were finished/stopped)
//     if (!replayTimer->isActive()) {
//         replayTimer->start(100);
//     }

//     // D. Update UI Status to REPLAYING (So "Pause" button appears)
//     if (!m_recorder) m_recorder = getRecorder();
//     if (m_recorder) {
//         m_recorder->loggerStatus = Recorder::S_REPLAYING;
//         m_recorder->update(m_recorder->loggerStatus);
//         // m_recorder->loggerInfo();
//     }
// }


// void Replay::startReplayFromTimestamp(qint64 timestampMs)
// {
//     if (playbackFrames.isEmpty()) return;

//     // Stop current playback
//     if (replayTimer && replayTimer->isActive()) {
//         replayTimer->stop();
//     }
//     // Note: We don't call full stop() here because we don't want to reset currentReplayIndex to 1

//     // Find correct index
//     for (int i = 0; i < playbackFrames.size(); i++) {
//         qint64 frameTime = playbackFrames[i]["timestamp_ms"].toVariant().toLongLong();
//         if (frameTime >= timestampMs) {
//             currentReplayIndex = i;
//             break;
//         }
//     }

//     // Resume/Start from this new index
//     start();
// }

// void Replay::bookmarkReplay(const QString &note, qint64 timestampMs)
// {
//     // Stop if replay already running
//     if (replayTimer && replayTimer->isActive()) {
//         replayTimer->stop();
//     }

//     // Reset replay state
//     isPaused = false;

//     // Start from timestamp
//     startReplayFromTimestamp(timestampMs);

//    // qDebug() << "Replay jumped to bookmark: " << note << " @ " << timestampMs << " ms";
// }



// void Replay::connectReplayTimer()
// {
//     connect(replayTimer, &QTimer::timeout, this, [this]() {
//         if (currentReplayIndex < playbackFrames.size()) {

//             const QJsonObject &frame = playbackFrames.at(currentReplayIndex);

//             if (frame.contains("hierarchy")) {
//                 m_hierarchy->fromJson(frame["hierarchy"].toObject());
//                 emit replayFrameLoaded(frame["timestamp_ms"].toVariant().toLongLong());
//                 emit frameLoaded(frame);
//             }

//             currentReplayIndex++;

//         } else {
//             stop();
//         }
//     });
// }

// /* -------------------------------------------------------
//  * Replay Implementation End
//  * ------------------------------------------------------*/

