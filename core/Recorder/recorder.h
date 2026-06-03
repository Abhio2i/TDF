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
#include <core/Hierarchy/profilecategaory.h>
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <core/SQLite/sqlite.h>
#include <QSharedPointer>

// Forward declarations
class Hierarchy;
class Simulation;
class Recording;
class Replay;

// ============================================================================
// Recorder class (main coordinator)
// ============================================================================
class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(Hierarchy* hierarchy, Simulation* simulation, QObject *parent = nullptr);

    // ---------- Enums (must be before signals that use them) ----------
    enum loggerModes { RECORDING, REPLAY };
    Q_ENUM(loggerModes)

    enum LoggerStatusModes {
        S_RECORDING_MODE, S_RECORDING, S_RECORDING_PAUSED, S_RECORDING_STOPPED,
        S_REPLAY_MODE, S_REPLAY_LOADED, S_REPLAY_UNLOADED, S_REPLAYING,
        S_REPLAY_PAUSED, S_REPLAY_STOPPED
    };
    Q_ENUM(LoggerStatusModes)

    enum SimulationStatusModes {
        S_SIMULATION_START, S_SIMULATION_PAUSED, S_SIMULATION_STOP, S_SIMULATION_NA
    };
    Q_ENUM(SimulationStatusModes)

signals:
    void setRecorder(Recorder *s_recorder);
    void setSQLite();
    void fileToDB(QString path, bool &isFileSaved);
    void loadToDB(QString path, bool &isFileLoaded);
    void freezeButtonOperation(ButtonNOpsList buttonOperationList);
    void alertViaStr(QString msg);
    void alertViaEnum(Logger_Error err);
    void sendRecorder();
    void recorderInfoSend(Recorder::LoggerStatusModes loggerStatus);
    void recorderInfoSendOnce(QDateTime s_recordingStartTime, qint64 s_duration,
                              Recorder::LoggerStatusModes loggerStatus,
                              Recorder::SimulationStatusModes simulationStatus);
    void recorderInfoSendUsual(qint64 s_duration,
                               Recorder::LoggerStatusModes loggerStatus,
                               Recorder::SimulationStatusModes simulationStatus);
    void recorderInfoSendDuration(qint64 s_duration);

public:
    Recording *m_recording = nullptr;
    Replay    *m_replay    = nullptr;
    loggerModes modeOfLogger;
    QDateTime recordingStartTime;
    qint64 duration = 0;
    LoggerStatusModes loggerStatus;
    SimulationStatusModes simulationStatus;
    qint64* leftTimer = nullptr;
    qint64* rightTimer = nullptr;
    qint64* durationPtr = nullptr;
    QList<QPair<QString, qint64>>* bookmarks = nullptr;
    SQLite *m_sqlite = nullptr;
    QString savedFilePath;
    bool *isFileSaved = new bool(false);
    QString loadedFilePath;
    bool *isFileLoaded = new bool(false);
    Recording* getRecording() const { return m_recording; }
    Replay* getReplay() const { return m_replay; }
    QDateTime startTime() const { return recordingStartTime; }


public slots:
    void loggerModeCheck(loggerModes mode);
    void loggerInfo();
    void update(LoggerStatusModes loggerStatus);

public:
    QString getStringTimer(qint64 time);
    void setLeftRightTimer(qint64 &left, qint64 &right);
    void setDuartionPtr(qint64 &s_durationPtr);
    qint64* getDuartionPtr();
    void setBookmarks(QList<QPair<QString, qint64>> &s_bookmarks);
    DB_Validity sqliteIsValid();
    void saveFile(QString path);
    void loadFile(QString path);

private:
    void recorderInfoUpdateOnce();
    void recorderInfoUpdateUsual();

    QString str;
    typedef enum {
        D_NULL = 0b1000000000000000000000000000000,
        D_JustPrint = 0b0100000000000000000000000000000,
        D_DBOperation = 0b0010000000000000000000000000000,
    } debugRecorder;
    void debug(const QString &str, const debugRecorder &currentdebugType = D_JustPrint);
    int debugList = D_JustPrint | D_DBOperation;
    bool dbgIsAllow(const debugRecorder &currentdebugType);
    Hierarchy* m_hierarchy = nullptr;
    Simulation* m_simulation = nullptr;
};

// ============================================================================
// Recording class – records the simulation state
// ============================================================================
class Recording : public QObject
{
    Q_OBJECT

public:
    explicit Recording(Hierarchy* hierarchy, Simulation* simulation, Recorder *parentRecorder, QObject *parent = nullptr);
    Recorder* getRecorder() const { return m_recorder; }
    enum recordingModes { START, PAUSE, RESUME, STOP };
    Q_ENUM(recordingModes)

public slots:
    void start(Recorder &s_recorder);
    void resume();
    void pause();
    void stop();
    void addBookmark();
    void update();
    void onResultsReady(QSharedPointer<PayLoad> data);

private:
    Recorder*   m_recorder = nullptr;
    Hierarchy*  m_hierarchy = nullptr;
    Simulation* m_simulation = nullptr;

public:
    QDateTime m_startTime;
    QDateTime startTime() const { return m_startTime; }
    bool m_active = false;
    QTimer    *recordingTimer = nullptr;
    QDateTime currentDateTime;
    qint64 recordingPeriod = 100;
    qint64 duration = 0;
    qint64 m_duration = 0;
    recordingModes mode = STOP;

signals:
    void updateUiDuration();
    void started();
    void paused();
    void stopped(qint64);
    void bookmarkRecorded(const QString &note, qint64 timestampMs);
    void sendPayLoad(PayLoad m_payLoad);
    void requestProcessing(QSharedPointer<PayLoad> data);

public:
    void recordingBookmark(const QString &message, qint64 timestampMs);
    qint64 durationShared = 0;
    qint64 leftTimer = 0;
    qint64 rightTimer = 0;
    QList<QPair<QString, qint64>> bookmarks;

    // Profile categories
    ProfileCategoriesDetailsList *m_profileCategoriesDetailsList = nullptr;
    ProfileCategoriesCRUDList    *m_profileCategoriesCRUDList = nullptr;
    int maxProfileIndex = 0;
    QHash<QString, int> profileCategoriesIDIndex;
    void profileCategoriesUpdate(QString ID, QString profileName, Operation operation);
    void profileCategoriesDeleted(QString ID);

    // Entities
    EntitiesDetailsList            *m_entitiesDetailsList = nullptr;
    EntitiesCreatedList            *m_entitiesCreatedList = nullptr;
    EntitiesUpdatedList            *m_entitiesUpdatedList = nullptr;
    EntitiesDeletedList            *m_entitiesDeletedList = nullptr;
    EntitiesMeshRenderer2DList     *m_entitiesMeshRenderer2DList = nullptr;
    EntitiesMeshRenderer2DCRUDList *m_entitiesMeshRenderer2DCRUDList = nullptr;
    EntitiesTrajectoryList         *m_entitiesTrajectoryList = nullptr;
    EntitiesTrajectoryCRUDList     *m_entitiesTrajectoryCRUDList = nullptr;
    QHash<QString, int> entitiesIDIndex;
    int maxIndex = 0;
    int frameIndex = 0;


    void inspectEntitiesIDIndex();
    void entityAddedAllFromStart();
    void entityAddedInBetween(const QString &parentID, const QString &ID, const QString &entityName);
    void entityUpdatesInBetween();
    void entityRemovedInBetween(const QString &ID);
    void meshRenderer2DCRUD(const QString &ID, MeshRenderer2D* meshRenderer2D, Operation operation = CREATE);
    void trajectoryCRUD(const QString &ID, std::vector<Waypoints*> Trajectories, Operation operation = CREATE);
    void framePacket(QSharedPointer<PayLoad> data);
    void framePacketPartialClean(QSharedPointer<PayLoad> payload);
    void beforeChangesPacket(QSharedPointer<PayLoad> data);
    void beforeRecording(QSharedPointer<PayLoad> payload);
    void currentEntities();
    void currentProfile();
    void profileCategoriesUpdate(QSharedPointer<PayLoad> payload, QString ID, QString profileName, Operation operation);
    void profileCategoriesDeleted(QSharedPointer<PayLoad> payload, QString ID);
    void entityAddedInBetween(QSharedPointer<PayLoad> payload, const QString &parentID, const QString &ID, const QString &entityName);
    void entityUpdatesInBetween(QSharedPointer<PayLoad> payload);
    void entityRemovedInBetween(QSharedPointer<PayLoad> payload, const QString &ID);
    void meshRenderer2DCRUD(QSharedPointer<PayLoad> payload, const QString &ID, MeshRenderer2D* meshRenderer2D, Operation operation = CREATE);
    void trajectoryCRUD(QSharedPointer<PayLoad> payload, const QString &ID, std::vector<Waypoints*> Trajectories, Operation operation = CREATE);
    void inspectAll(QSharedPointer<PayLoad> payload);
    QHash<QString, int> sensorIndexMap;

    int maxSensorIndex = 0;
    void sensorAdded(QSharedPointer<PayLoad> payload, const QString &sensorId, Sensor* sensor, Operation op = CREATE);
    void sensorUpdated(QSharedPointer<PayLoad> payload, const QString &sensorId, Sensor* sensor);
    void sensorRemoved(QSharedPointer<PayLoad> payload, const QString &sensorId);
    std::unordered_set<QString> *meshRenderer2DCRUDSet = new std::unordered_set<QString>();

    // recorder.h mein REPLACE karo ye lines:

    // IFF
    QHash<QString, int> iffIndexMap;
    int maxIffIndex = 0;
    void iffAdded(QSharedPointer<PayLoad> payload, const QString &iffId, QObject* iff, Operation op = CREATE);
    void iffUpdated(QSharedPointer<PayLoad> payload, const QString &iffId, QObject* iff);
    void iffRemoved(QSharedPointer<PayLoad> payload, const QString &iffId);

    // Radio
    QHash<QString, int> radioIndexMap;
    int maxRadioIndex = 0;
    void radioAdded(QSharedPointer<PayLoad> payload, const QString &radioId, QObject* radio, Operation op = CREATE);
    void radioUpdated(QSharedPointer<PayLoad> payload, const QString &radioId, QObject* radio);
    void radioRemoved(QSharedPointer<PayLoad> payload, const QString &radioId);

    // Weapon
    QHash<QString, int> weaponIndexMap;
    int maxWeaponIndex = 0;
    void weaponAdded(QSharedPointer<PayLoad> payload, const QString &weaponId, QObject* weapon, Operation op = CREATE);
    void weaponUpdated(QSharedPointer<PayLoad> payload, const QString &weaponId, QObject* weapon);
    void weaponRemoved(QSharedPointer<PayLoad> payload, const QString &weaponId);
    bool filterTrajectories = true;
    bool filterRadio        = true;
    bool filterIFF          = true;
    bool filterFixedPoints  = true;

    bool filterSpecialZone  = true;
    // Sensors master flag
    bool filterSensors      = true;
    // Sensor sub-type flags
    bool filterRadar        = true;
    bool filterAESA         = true;
    bool filterCSM          = true;
    bool filterESM          = true;
    bool filterMAW          = true;
    // bool filterEO           = true;
    bool filterSonar        = true;
    bool filterAIS          = true;
    bool filterADSB         = true;

public:
    QSharedPointer<PayLoad> packet;
    void addPacketData();
    void initiateTask();
    void recordingStart();
    void recordingStop();
    void recordingPauseResume();
    void saveFile();
    QJsonArray getFrameEntitiesData();
    void getSimulationUpdate();
    void changeInHierarchy();
    void insertRecord(const QJsonObject &data);
    void inspectEntitiesUpdatedList();
    bool changeInHierarchyInPause = false;
        std::unordered_map<int, IffData>    iffsIndexMap_replay;
        std::unordered_map<int, RadioData>  radiosIndexMap_replay;
        std::unordered_map<int, WeaponData> weaponsIndexMap_replay;
        bool iffsRestored    = false;
        bool radiosRestored  = false;
        bool weaponsRestored = false;
        void restoreIffsFromPayload   (QSharedPointer<PayLoad> payload);
        void restoreRadiosFromPayload (QSharedPointer<PayLoad> payload);
        void restoreWeaponsFromPayload(QSharedPointer<PayLoad> payload);
        void entityRenamedInBetween(QSharedPointer<PayLoad> payload,
                                    const QString &ID,
                                    const QString &newName);

private:
    void recordingBefore();
    qint64 lastElapsedMs = 0;
    QJsonArray recordedFile;
    QJsonObject recordedData;
    enum DBStatuses { CONNECTED = true, DISCONNECTED = false } dbStatus = DISCONNECTED;
    QString str;

public:
    typedef enum {
        D_NULL = 0b1000000000000000000000000000000,
        D_JustPrint = 0b0100000000000000000000000000000,
        D_Timer = 0b0010000000000000000000000000000,
        D_RecordingStatus = 0b0001000000000000000000000000000,
        D_RecordingUpdate = 0b0000100000000000000000000000000,
        D_BeforeStart = 0b0000010000000000000000000000000,
        D_EntityCreated = 0b0000001000000000000000000000000,
        D_EntityUpdated = 0b0000000100000000000000000000000,
        D_EntityDeleted = 0b0000000010000000000000000000000,
        D_FramePayLoad = 0b0000000001000000000000000000000,
        D_EntitiesIDIndex = 0b0000000000100000000000000000000,
        D_UpdatesInBTW = 0b0000000000010000000000000000000,
        D_ProfileCategories = 0b0000000000001000000000000000000,
        D_TrajectoryCRUD = 0b0000000000000100000000000000000,
        D_MeshRenderer2D = 0b0000000000000010000000000000000,
        D_BeforeRecording = 0b0000000000000001000000000000000,
        D_Inspection = 0b0000000000000000100000000000000,
    } debugOptions;
    void debug(const QString &str, const debugOptions &currentdebugType = D_JustPrint);
    int debugList = D_JustPrint | D_BeforeRecording | D_Inspection;
    bool dbgIsAllow(const debugOptions &currentdebugType);
};

// ============================================================================
// Replay class – replays a recorded simulation
// ============================================================================
class Replay : public QObject
{
    Q_OBJECT

public:
    explicit Replay(Hierarchy* hierarchy, Simulation* simulation, Recorder *parentRecorder, QObject *parent = nullptr);
    Recorder* getRecorder() const { return m_recorder; }
    enum replayModes { START, PAUSE, RESUME, STOP };
    Q_ENUM(replayModes)
    enum jumpOps { FORWORD, BACKWORD, INBETWEEN, BOOKMARK };
    Q_ENUM(jumpOps)
    enum fileStates { NOT_EXIST = false, EXIST = true };
    Q_ENUM(fileStates)
    fileStates fileState = NOT_EXIST;
    void update();
    bool replayLoaded(const QString &filePath);
    void replayBefore();
    void replayStart();
    void connectReplayTimer();

public slots:
    void start();
    void pause();
    void resume();
    void stop();
    void restart();
    void fileLoaded();
    void fileUnloaded();
    void goToNextFrame();
    void goToPreviousFrame();
    void playAgain();
    void startReplayFromTimestamp(qint64 timestampMs);
    void bookmarkReplay(const QString &note, qint64 timestampMs);
    void onResultsReady(QSharedPointer<PayLoad> data);

signals:
    void started(QDateTime);
    void paused();
    void stopped(qint64);
    void setReplayDuration(qint64 duration);
    void replayBookmark(const QString &note, qint64 timestamp);
    void frameLoaded(const QJsonObject &frame);
    void replayFrameLoaded(qint64 timestampMs);
    void updateUiDuration();
    void setMaxDuration(qint64* maxDuration);
    void getPayLoadByFrameIndex(int frameIndex);
    void getPayLoadFromIndex(QSharedPointer<PayLoad> payload, int frameIndex);
    void getMaxFrameIndexNDuration(int* maxFrameIndex, qint64* maxDuration);
    void getPayLoad(QSharedPointer<PayLoad> packet);
    void getCanvas(CanvasWidget *canvas);
    void trajectoryUpdated(QString entityId, QJsonArray waypoints);
    void selectMesh(const QString &ID);
    void requestProcessing(QSharedPointer<PayLoad> data);
    void createProfileCategories(ProfileCategaory *profileCategaory);
    void deleteProfileCategories(QString ID);
    void createEntitiesCreate(QString parentId, QString ID, QString EntityName, bool Profile);
    void updateEntities();
    void deleteEntities(QString parentId, QString ID, bool Profile);
    void render(float deltatime);
    void setCanvasRenderEnabled(bool enabled);

    void entityRecreated(const QString& id);
    void clearCanvasMeshes();

private:
    Recorder*   m_recorder;
    Hierarchy*  m_hierarchy = nullptr;
    Simulation* m_simulation = nullptr;
    bool isPaused = false;
    bool m_active = false;
    qint64 m_duration = 0;
    QVector<QJsonObject> playbackFrames;
    int currentReplayIndex = 0;

public:
    std::unordered_map<std::string, Platform*> *m_Platforms;
    qint64 durationShared = 0;
   qint64 durationLength = 0;
    qint64 leftTimer = 0;
    qint64 rightTimer = 0;
    QList<QPair<QString, qint64>> bookmarks;
    qint64 jumpStep = 5000;
    QTimer *replayTimer = nullptr;
    QDateTime replayDateTime;
    qint64 replayPeriod = 100;
    int frameIndex = 0;
    qint64 duration = 0;
    int maxFrameIndex = 0;
    qint64 maxDuration = 0;
    PayLoad payload;
    QSharedPointer<PayLoad> packet;
    CanvasWidget canvas;
    std::unordered_map<int, EntitiesDetails> entitiesIndexDetails;
    std::unordered_map<int, ProfileCategoriesDetails> profileCategoriesIndexDetails;
    std::unordered_map<int, EntitiesMeshRenderer2D> entitiesMeshRenderer2DIndex;
    std::unordered_map<int, EntitiesTrajectory> entitiesTrajectoryIndex;
    std::unordered_map<int, Sensor*> sensorsIndexMap;
    bool sensorsRestored = false;
    void restoreSensorsFromPayload(QSharedPointer<PayLoad> payload);
    std::unordered_map<int,std::pair<std::string, std::string>> entitiesMap;
    std::unordered_map<int,std::pair<std::string, std::string>> profileCategoriesMap;
    std::vector<qint64> frameMap;
    void createTimer();
    void toggle();
    void loadData();
    void loadFrameEntitiesData();
    void jumpInBetween(qint64 clickedTimestamp);
    void framePayLoadForJumpInBetween(int frameIndex);
    void setEntitiesDetailsInBtw();
    void setEntitiesMeshRenderer2DBtw();
    void setEntitiesDetailsInBtw(QSharedPointer<PayLoad> payload);
    void setEntitiesMeshRenderer2DBtw(QSharedPointer<PayLoad> payload);
    void setEntitiesTrajectoryBtw();
    void cleanHierarchy();
    void setProfileCategoriesDetails();
    void setEntitiesIndexDetails();
    void setEntitiesMeshRenderer2D();
    void crudEntitiesMeshRenderer2D();
    void setEntitiesTrajectory();
    void crudEntitiesTrajectory();
    void crudProfileCategoriesDetails();
    void createEntitiesCreateList();
    void updateEntitiesUpdatedList();
    void deleteEntitiesDeletedList();
    void framePayLoad(QSharedPointer<PayLoad> payload);
    void receivePayLoadFrameIndex(PayLoad payLoad);
    void initiateTask();
    void framePacket(QSharedPointer<PayLoad> payload);
    void beforeReplay(QSharedPointer<PayLoad> payload);
    void setProfileCategoriesDetails(QSharedPointer<PayLoad> payload);
    void setEntitiesIndexDetails(QSharedPointer<PayLoad> payload);
    void setEntitiesMeshRenderer2D(QSharedPointer<PayLoad> payload);
    void crudEntitiesMeshRenderer2D(QSharedPointer<PayLoad> payload);
    void setEntitiesTrajectory(QSharedPointer<PayLoad> payload);
    void setEntitiesTrajectoryByBack(QSharedPointer<PayLoad> payload);
    void crudEntitiesTrajectory(QSharedPointer<PayLoad> payload);
    void crudProfileCategoriesDetails(QSharedPointer<PayLoad> payload);
    void createEntitiesCreateList(QSharedPointer<PayLoad> payload);
    void updateEntitiesUpdatedList(QSharedPointer<PayLoad> payload);
    void deleteEntitiesDeletedList(QSharedPointer<PayLoad> payload);
    replayModes mode;
    // IFF
    bool iffsRestored = false;
    std::unordered_map<int, IffData> iffsIndexMap_replay;
    void restoreIffsFromPayload(QSharedPointer<PayLoad> payload);

    // Radio
    bool radiosRestored = false;
    std::unordered_map<int, RadioData> radiosIndexMap_replay;
    void restoreRadiosFromPayload(QSharedPointer<PayLoad> payload);

    // Weapon
    bool weaponsRestored = false;
    std::unordered_map<int, WeaponData> weaponsIndexMap_replay;
    void restoreWeaponsFromPayload(QSharedPointer<PayLoad> payload);

private:
    QString str;
    typedef enum {
        D_NULL = 0b100000000000000000000,
        D_JustPrint = 0b010000000000000000000,
        D_Timer = 0b001000000000000000000,
        D_MaxFrameIndexNDuration = 0b000100000000000000000,
        D_EntitiesIndexDetails = 0b000010000000000000000,
        D_ProfileCategoriesIndexDetails = 0b000001000000000000000,
        D_EntitiesCreateList = 0b000000100000000000000,
        D_EntitiesDeletedList = 0b000000010000000000000,
        D_PayLoad_Inspect = 0b000000001000000000000,
        D_CleanHierarchy = 0b000000000100000000000,
        D_MeshRenderer = 0b000000000010000000000,
        D_EntitiesTrajectory = 0b000000000001000000000,
        D_BeforeReplay = 0b000000000000100000000,
        D_LoadInBetween = 0b000000000000010000000
    } debugReplay;
    void debug(const QString &str, const debugReplay &currentdebugType = D_JustPrint);
    int debugList = D_JustPrint | D_LoadInBetween | D_EntitiesTrajectory;
    bool dbgIsAllow(const debugReplay &currentdebugType);
};

#endif // RECORDER_H
