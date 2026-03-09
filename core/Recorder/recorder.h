
#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QTimer>
#include <QJsonValue>
#include <QString>
#include <core/Simulation/simulation_state.h>
#include <core/Hierarchy/entity.h>
#include <core/Hierarchy/EntityProfiles/platform.h>
#include <cmath>
#include "payload.h"


// Forward declarations to avoid circular includes
class Hierarchy;
class Simulation;
class Recording;
class Replay;

class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);


public:
    // Constructor:Accepts hierarchy and simulation to pull state and recording speed
    //explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);

private:
    Hierarchy* m_hierarchy = nullptr;  // Used for extracting structure snapshot
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed

signals:
    void setRecorder(Recorder *s_recorder);
    /* -------------------------------------------------------
 * Shared Object Start
 * ------------------------------------------------------*/

public:
    //For Seleting Modes
    enum loggerModes{
        RECORDING,
        REPLAY
    };
    Q_ENUM(loggerModes)

public:
    //String for Debug
    Recording *m_recording = nullptr;
    Replay    *m_replay    = nullptr;
    QString    debugString;
    loggerModes modeOfLogger;

public:
    Recording* getRecording() const { return m_recording; }
    Replay* getReplay()    const { return m_replay;    }

public slots:
    void loggerModeCheck(loggerModes mode);

    //Recorder Information Start
    //Enum for Logger Status of:
    //S Stands for Status:
public:
    enum LoggerStatusModes{
        //For Recording
        S_RECORDING_MODE,   //0
        S_RECORDING,        //1
        S_RECORDING_PAUSED, //2
        S_RECORDING_STOPPED,//3
        //For REPLAY
        S_REPLAY_MODE,      //4
        S_REPLAY_LOADED,    //5
        S_REPLAY_UNLOADED,  //6
        S_REPLAYING,        //7
        S_REPLAY_PAUSED,    //8
        S_REPLAY_STOPPED,   //9
    };
    Q_ENUM(LoggerStatusModes)

    //Enum for Simulation Status:
    //S Stands for Status:
    enum SimulationStatusModes{
        S_SIMULATION_START, //0
        S_SIMULATION_PAUSED,//1
        S_SIMULATION_STOP,  //2
        S_SIMULATION_NA,    //3
    };
    Q_ENUM(SimulationStatusModes)
    QDateTime                       recordingStartTime;
    qint64                          duration;
    Recorder::LoggerStatusModes     loggerStatus    ;
    Recorder::SimulationStatusModes simulationStatus;
    // Recorder* recorder;
    // Recorder* getRecorder() const { return recorder; }
    QDateTime   startTime() const;


public:
    //To Display input data
    void loggerInfo();
    void update(Recorder::LoggerStatusModes     loggerStatus);
private:
    //Use to Update Recorder Information only first time
    void recorderInfoUpdateOnce();
    //Use to Update Recorder Information after first
    void recorderInfoUpdateUsual();



signals:
    //Use to Send Recorder Information only first time
    void sendRecorder();
    //For Testing
    void recorderInfoSend(
        Recorder::LoggerStatusModes     loggerStatus);

    //void update(Recorder::SimulationStatusModes simulationStatus);
    void recorderInfoSendOnce(
        QDateTime       s_recordingStartTime,
        qint64          s_duration,
        Recorder::LoggerStatusModes     loggerStatus,
        Recorder::SimulationStatusModes simulationStatus);

    //Use to Send Recorder Information after first
    void recorderInfoSendUsual(
        qint64           s_duration,
        Recorder::LoggerStatusModes     loggerStatus,
        Recorder::SimulationStatusModes simulationStatus);

    //Use to Send Recorder Information only duration
    void recorderInfoSendDuration(qint64 s_duration);

    //Recorder Information End
public:
    QString getStringTimer(qint64 time);


    //TimeLine Widget Start
public:
    qint64* leftTimer  = nullptr;
    qint64* rightTimer = nullptr;
    void setLeftRightTimer(qint64 &left, qint64 &right);

    qint64* durationPtr = nullptr;
    void setDuartionPtr(qint64 &s_durationPtr);
    qint64* getDuartionPtr();
    QList<QPair<QString, qint64>>* bookmarks;
    void setBookmarks(QList<QPair<QString, qint64>> &s_bookmarks);

    //TimeLine Wideget End
};

/* -------------------------------------------------------
 * Shared Object End
 * ------------------------------------------------------*/


/* -------------------------------------------------------
 * Recording QObject Start
 * ------------------------------------------------------*/

class Recording : public QObject
{
    Q_OBJECT

public:
    explicit Recording(Hierarchy* hierarchy, Simulation* simulation,Recorder *parentRecorder, QObject *parent = nullptr);

public:
    Recording *m_recording;
    Recorder* getRecorder() const { return m_recorder; }

    //For Recording States
    enum recordingModes{
        START,
        PAUSE,
        RESUME,
        STOP
    };
    Q_ENUM(recordingModes)

public slots:
    void start(Recorder &s_recorder);
    void resume();
    void pause();
    void stop();
    void addBookmark();
    void update();

private:
    Recorder*   m_recorder   = nullptr;
    Hierarchy*  m_hierarchy  = nullptr;  // Used for extracting structure snapshot
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed

public:

    QDateTime   m_startTime;
    QDateTime   startTime() const;
    bool        m_active { false };

    //TimeLine Widget Start
public:
    QTimer    *recordingTimer = nullptr;
    QDateTime currentDateTime;
    qint64    recordingPeriod;
    qint64    duration;

    qint64    m_duration;
signals:
    void updateUiDuration();
    //TimeLine Wideget End

private:
    QDateTime recordingStartTime;
    qint64 pausedOffsetMs = 0;   // public earlier, that's ok
    bool   isPaused = false;
    int    noOfFrame = 1;

    QDateTime pauseStartTime;
    //Recording Start Recording Start

    //Recording Start Recording Start
public:
    void recordingBookmark(const QString &message, qint64 timestampMs);

    /*------------ New Type of recording Start ------------*/

    // public:
    //     //Definig essential enums
    //     enum typeOfUpdate{
    //         dynamicDynamic,
    //         dynamicStatic,
    //         trajectory,
    //         sensor,
    //         entity,
    //     };

    //     enum updateTypes{
    //         CREATE,
    //         UPDATE,
    //         DELETE
    //     };

private:
    SimUpdateTypes::UpdateTypes     simUpdate     = SimUpdateTypes::UPDATE;
    SimTypeOfUpdates::TypeOfUpdate  simUpdateType = SimTypeOfUpdates::dynamicDynamic;

    QJsonArray    getFrameEntitiesData();

public:
    void getSimulationUpdate();
    void changeInHierarchy();
    bool changeInHierarchyInPause = false;
    qint64 lastElapsedMs = 0;
    recordingModes mode = STOP;


    //Recording Main Container
    QJsonArray recordedFile;
    //Meta Data essential for recording
    //   Contain static components of recording
    //QJsonObject metaData; //Add later


public slots:
    //void getSimulationUpdate();
    /*------------ New Type of recording End   ------------*/

private:
    void recordingStart();
    void recordingStop();
    void recordingPauseResume();
    void saveFile();



    //  Contain snap in data entry
    //QJsonObject timeEntry;
    QJsonObject recordedData;

    //Recording Container Insert Method;
    void insertRecord(const QJsonObject &data);


    //Recording Constants
    //RecordinDynamic components of recording
public:
    qint64 durationShared = 0;
    qint64 leftTimer      = 0;
    qint64 rightTimer     = 0;
    QList<QPair<QString, qint64>> bookmarks = {
        qMakePair(QString("Recording BookMark A"), 41000),
        qMakePair(QString("Recording BookMark B"), 42000),
        qMakePair(QString("Recording BookMark C"), 43000),
        qMakePair(QString("Recording BookMark D"), 44000)
    };

signals:
    void started();
    void paused();
    void stopped(qint64);
    void insertEntity(QString ID, bool Profile);
    void mapFrame(const qint64 &s_duration);
    //In Between Running simulation
private:
    enum HierarchyOps{
        Success = true,
        Failed = false,
        EntityAdded,
        EntityRemoved,

    };
    //Q_ENUM(HierarchyOps)
public:
    std::unordered_set<std::string>     entityIDList;

    // Index & ID Start
    int maxIndex = 0;
    //std::unordered_map<QString,int> entitiesIDIndex;
    EntitiesDetailsList m_entitiesDetailsList;
    EntitiesCreatedList m_entitiesCreatedList;
    EntitiesUpdatedList m_entitiesUpdatedList;
    EntitiesDeletedList m_entitiesDeletedList;
    QHash<QString, int> entitiesIDIndex;
    void inspectEntitiesIDIndex();
    void inspectEntitiesUpdatedList();
    // Index & ID End

    // Frame Index Start
    int frameIndex = 0;
    // Frame Index End


    // To Insert All entity from Start of Recording
    void entityAddedAllFromStart();
    void entityAddedInBetween(const QString &parentID, const QString &ID, const QString &entityName);
    void entityUpdatesInBetween();
    void entityRemovedInBetween(const QString &ID);
    void framePayLoad();
signals:
    void entityCreated(const QString &parentID, const QString &id, const QString &name, const qint64 &created);
    void entityDeleted(const QString &id,const qint64  &deleted);
    void sendPayLoad(PayLoad m_payLoad);
private:
    // enum DBStatuses{
    //     CONNECTED = true,
    //     DISCONNECTED = false
    // };
    enum DBStatuses {
        CONNECTED = true,
        DISCONNECTED = false
    }dbStatus = DISCONNECTED;


    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;

    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b100000000000,
        D_JustPrint       = 0b010000000000,
        D_Timer           = 0b001000000000,
        D_RecordingStatus = 0b000100000000,
        D_RecordingUpdate = 0b000010000000,
        D_BeforeStart     = 0b000001000000,
        D_EntityCreated   = 0b000000100000,
        D_EntityUpdated   = 0b000000010000,
        D_EntityDeleted   = 0b000000001000,
        D_FramePayLoad    = 0b000000000100,
        D_EntitiesIDIndex = 0b000000000010,
        D_UpdatesInBTW    = 0b000000000001
    }debugOptions;
    Q_ENUM(debugOptions)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugOptions &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                    | D_RecordingStatus
                    | D_RecordingUpdate
                    | D_BeforeStart
                    | D_EntityCreated
                    | D_EntityUpdated
                    | D_EntitiesIDIndex
                    | D_EntityDeleted
                    | D_UpdatesInBTW;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugOptions &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};

/* -------------------------------------------------------
 * Recording QObject End
 * ------------------------------------------------------*/


/* -------------------------------------------------------
 * Replay QObject Start
 * ------------------------------------------------------*/

typedef struct{
    double longitude  ;
    double latitude   ;
    double altitude   ;
    double heading    ;
    float  turn_radius;
    float  curr_speed ;
    float  climb_rate ;
}entity;


class Replay : public QObject
{
    Q_OBJECT

public:
    explicit Replay(Hierarchy* hierarchy, Simulation* simulation,Recorder *parentRecorder, QObject *parent = nullptr);

    //Recorder* recorder;
    //Recording *m_recording = nullptr;
    // Recorder* getRecorder() const { return m_recorder; }
    Recorder* getRecorder() const { return m_recorder; }
    Replay *m_replay = nullptr;

    qint64 pausedTimestamp = 0;

    //For Replay
    enum replayModes{
        START,
        PAUSE,
        RESUME,
        STOP
    };
    Q_ENUM(replayModes)
    replayModes mode;

    enum jumpOps{
        FORWORD,
        BACKWORD,
        INBETWEEN,
        BOOKMARK
    };
    Q_ENUM(jumpOps)

    enum fileStates{
        NOT_EXIST = false,
        EXIST = true
    };
    Q_ENUM(fileStates)
    fileStates fileState = NOT_EXIST;

    void update() ;
    bool replayLoaded(const QString &filePath);
    void replayStart();
    void connectReplayTimer();
signals:
    void started(QDateTime);
    void paused();
    void stopped(qint64);
    void setReplayDuration(qint64 duration);
    void replayBookmark(const QString &note, qint64 timestamp);
    void frameLoaded(const QJsonObject &frame);

    void replayFrameLoaded(qint64 timestampMs);
    void updateUiDuration();
public slots:
    void start()  ;
    void pause()  ;
    void resume() ;
    void stop()   ;
    void restart();
    void fileLoaded();
    void fileUnloaded();
    void goToNextFrame();
    void goToPreviousFrame();
    void playAgain();
    void startReplayFromTimestamp(qint64 timestampMs);
    void bookmarkReplay(const QString &note, qint64 timestampMs);



private:
    Recorder*   m_recorder;
    Hierarchy*  m_hierarchy = nullptr;  // Used for extracting structure snapshot
    Simulation* m_simulation = nullptr;  // Used for getting simulation speed
    QDateTime   m_startTime;
    qint64      m_duration;
    bool        m_active { false };

    bool isPaused = false;
    QVector<QJsonObject> playbackFrames;
    //int currentReplayIndex = 1;
    int currentReplayIndex = 0;

public:

    void createTimer();
    void toggle();
    void loadData();

signals:
    void updateScene(float deltaTime);
public:
    std::unordered_map<std::string, Platform*> *m_Platforms;
public:
    qint64 durationShared = 0;
    qint64 durationLength = 1000*60*2;
    qint64 leftTimer  =     0;
    qint64 rightTimer =     0;
    QList<QPair<QString, qint64>> bookmarks = {
        qMakePair(QString("Replay BookMark A"), 41000),
        qMakePair(QString("Replay BookMark B"), 42000),
        qMakePair(QString("Replay BookMark C"), 43000),
        qMakePair(QString("Replay BookMark D"), 44000)
    };
    qint64 jumpStep = 5000;

public:
    QTimer    *replayTimer = nullptr;
    QDateTime replayDateTime;
    qint64    replayPeriod;

    int       frameIndex;
    qint64    duration;

    int       maxFrameIndex;
    qint64    maxDuration;



    /*-------------- Fetching Data Start --------------*/
public:
    PayLoad payload;
    void framePayLoad();
signals:
    void getPayLoad(PayLoad* payload);
    void getMaxFrameIndexNDuration(
        int*    maxFrameIndex,
        qint64* maxDuration);
    /*--------------  Fetching Data End  --------------*/


    /*---------  Update Data on Canvas Start  ---------*/

    /*
 *      EntitiesDetailsList
 *      EntitiesCreatedList
 *      EntitiesUpdatedList
 *      EntitiesDeletedList
 */

public:
    std::unordered_map<int, EntitiesDetails> entitiesIndexDetails;
    void setEntitiesIndexDetails();

    void createEntitiesCreateList();
    void updateEntitiesUpdatedList();
    void deleteEntitiesDeletedList();
signals:
    void createEntitiesCreate(QString parentId,QString ID,QString EntityName,bool Profile);
    void updateEntities();
    void deleteEntities(QString parentId, QString ID, bool Profile);
    /*---------   Update Data on Canvas End   ---------*/
public:
    std::unordered_map<int,std::pair<std::string, std::string>> entitiesMap;
    std::vector<qint64> frameMap;

    void loadFrameEntitiesData();
    std::unordered_map<int , entity> frame;

signals:
    void getEntities();
    void getFrameMap();
    void getFrame(int s_frameIndex);
    void render(float deltatime);

    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;

    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b100000000000,
        D_JustPrint       = 0b010000000000,
        D_Timer           = 0b001000000000,
        D_MaxFrameIndexNDuration = 0b000100000000,
        D_EntitiesIndexDetails   = 0b000010000000,
        D_EntitiesCreateList     = 0b000001000000,
        D_EntitiesDeletedList    = 0b000000100000,
        D_PayLoad_Inspect        = 0b000000010000
    }debugReplay;
    Q_ENUM(debugReplay)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugReplay &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                    | D_EntitiesDeletedList
                    | D_PayLoad_Inspect
        //| D_Timer
        //| D_EntitiesCreateList
        //| D_MaxFrameIndexNDuration
        //| D_EntitiesIndexDetails
        ;

    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugReplay &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};
/* -------------------------------------------------------
 * Replay QObject End
 * -------------------------------------------------------*/




//
// /*-------------------------------------------------------
//  * Recording QObject Start
//  * ------------------------------------------------------*/

// class Recording : public QObject
// {
//     Q_OBJECT

// public:
//     explicit Recording(Hierarchy* hierarchy, Simulation* simulation,Recorder *parentRecorder, QObject *parent = nullptr);
// public:
//     //Recorder *m_recorder;
//     // Recorder* getRecorder() const { return m_recorder; }
//     Recording *m_recording;
//     //For Recording
//     enum recordingModes{
//         START,
//         PAUSE,
//         RESUME,
//         STOP
//     };

//     Q_ENUM(recordingModes)
//     Recorder* getRecorder() const { return m_recorder; }

//     void update();
//     QDateTime startTime() const;
//     qint64     duration()  const;

// public slots:
//     void start();
//     void resume();
//     void pause();
//     void stop();
//     void addBookmark();
// private:
//     Recorder*   m_recorder   = nullptr;
//     Hierarchy*  m_hierarchy  = nullptr;  // Used for extracting structure snapshot
//     Simulation* m_simulation = nullptr;  // Used for getting simulation speed
//     QDateTime   m_startTime;
//     qint64      m_duration;
//     bool        m_active { false };

//     //Recording Start Recording Start
// public:
//     void recordingBookmark(const QString &message, qint64 timestampMs);

//     /*------------ New Type of recording Start ------------*/

//     // public:
//     //     //Definig essential enums
//     //     enum typeOfUpdate{
//     //         dynamicDynamic,
//     //         dynamicStatic,
//     //         trajectory,
//     //         sensor,
//     //         entity,
//     //     };

//     //     enum updateTypes{
//     //         CREATE,
//     //         UPDATE,
//     //         DELETE
//     //     };

// private:
//     SimUpdateTypes::UpdateTypes     simUpdate     = SimUpdateTypes::UPDATE;
//     SimTypeOfUpdates::TypeOfUpdate  simUpdateType = SimTypeOfUpdates::dynamicDynamic;
//     QJsonObject recordedStructure;
//     /*├──►*/QJsonArray   rs_hierarchy;
//     /*│---*/QJsonObject    rs_hierarchyObj;
//     /*├──►*/QJsonArray   rs_bookmark ;
//     /*│---*/QJsonObject    rs_bookmarkObj;
//     /*└─┬►*/QJsonArray   rs_frame    ;
//     /*  │---*/QJsonObject  rs_frameObj;
//     /*  └─┬►*/QJsonArray     frameEntities;
//     /*    ├───►*/ QJsonObject  entityObj;
//     /*    ├───►*/ //QJsonObject CurrentSpeed;
//     /*    └───►*/
//     QJsonArray    getFrameEntitiesData();

// public:
//     void getSimulationUpdate();
//     void changeInHierarchy();
//     bool changeInHierarchyInPause = false;
//     qint64 lastElapsedMs = 0;

// public slots:
//     //void getSimulationUpdate();
//     /*------------ New Type of recording End   ------------*/

// private:
//     void recordingStart();
//     void recordingStop();
//     void recordingPauseResume();
//     void saveFile();


//     //Recording Main Container
//     QJsonArray recordedFile;
//     //Meta Data essential for recording
//     //   Contain static components of recording
//     //QJsonObject metaData; //Add later
//     QDateTime   currentDateTime;
//     QJsonArray  bookmarks;
//     //  Contain snap in data entry
//     //QJsonObject timeEntry;
//     QJsonObject recordedData;

//     //Recording Container Insert Method;
//     void insertRecord(const QJsonObject &data);


//     //Recording Constants
//     //RecordinDynamic components of recording
// public:
//     recordingModes mode = STOP;
//     //for logger
//     //QTimer *recordingTimer = nullptr;
//     //qint64 lastElapsedMs;
//     //qint64 pausedOffsetMs = 0;
//     //Remove
//     qint64 recordingPeriod;
// private:
//     QTimer *recordingTimer = nullptr;
//     QDateTime recordingStartTime;
//     qint64 pausedOffsetMs = 0;   // public earlier, that's ok

//     bool isPaused = false;
//     int noOfFrame = 1;

//     QDateTime pauseStartTime;
//     //Recording Start Recording Start
// signals:
//     void started();
//     void paused();
//     void stopped(qint64);
// signals:
//     void mapFrame(const qint64 &s_duration);

// };
// /* -------------------------------------------------------
//  * Recording QObject End
//  * ------------------------------------------------------*/
//



#endif // RECORDER_H
