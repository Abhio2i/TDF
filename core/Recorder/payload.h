#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <vector>
#include <QtGlobal>

// =========================================================================
// Enums & basic types
// =========================================================================

enum ButtonFreezeOps {
    Freeze   = true,
    Unfreeze = false
};

enum toggleModes {
    togglePlay,
    togglePause
};

enum LoggerButton {
    Recorder_Button ,
    Recording_Toggle,
    Reocrding_Stop  ,
    Replay_Start        ,
    Replay_Toggle       ,
    Replay_Jump_Forward ,
    Replay_Jump_Backward,
    Replay_Restart      ,
    Size_List           ,
};

using ButtonNOpsList = std::vector<std::pair<LoggerButton, ButtonFreezeOps>>;

enum Operation {
    CREATE = 0b1000,
    READ   = 0b0100,
    UPDATE = 0b0010,
    DELETE = 0b0001
};

// =========================================================================
// ProfileCategories structures
// =========================================================================

struct ProfileCategoriesDetails {
    int       index;
    QString   name;
    QString   ID;
};
using ProfileCategoriesDetailsList = std::vector<ProfileCategoriesDetails>;

struct ProfileCategoriesCRUD {
    int       index;
    Operation operation;
};
using ProfileCategoriesCRUDList = std::vector<ProfileCategoriesCRUD>;

// =========================================================================
// Entities structures
// =========================================================================

struct EntitiesDetails {
    int index;
    QString name;
    QString parentID;
    QString ID;
};
using EntitiesDetailsList = std::vector<EntitiesDetails>;

struct EntitiesCreated {
    int index;
};
using EntitiesCreatedList = std::vector<EntitiesCreated>;

struct EntitiesUpdated {
    int    index       ;
    double longitude   ;
    double latitude    ;
    double altitude    ;
    double heading     ;
    float  turn_radius ;
    float  curr_speed  ;
    float  climb_rate  ;
};
using EntitiesUpdatedList = std::vector<EntitiesUpdated>;

struct EntitiesDeleted {
    int index;
};
using EntitiesDeletedList = std::vector<EntitiesDeleted>;

struct EntitiesMeshRenderer2D {
    int     index;
    bool    Active;
    QString Sprite;
    QString Texture;
    QString color;
    QString color2;
};
using EntitiesMeshRenderer2DList = std::vector<EntitiesMeshRenderer2D>;

struct EntitiesMeshRenderer2DCRUD {
    int       index;
    Operation operation;
};
using EntitiesMeshRenderer2DCRUDList = std::vector<EntitiesMeshRenderer2DCRUD>;

// =========================================================================
// Trajectory structures
// =========================================================================

struct TrajectoryWaypoint {
    int    index;
    double geo_latitude;
    double geo_longitude;
    double geo_altitude;
    double geo_Heading;
    float  vector_x;
    float  vector_y;
    float  vector_z;
    double speed            = 0;
    bool   sensor           = false;
    bool   formation        = false;
};

struct EntitiesTrajectory {
    int index;
    std::vector<TrajectoryWaypoint> Trajectories;
};
using EntitiesTrajectoryList = std::vector<EntitiesTrajectory>;

struct EntitiesTrajectoryCRUD {
    int index;
    Operation operation;
};
using EntitiesTrajectoryCRUDList = std::vector<EntitiesTrajectoryCRUD>;

// =========================================================================
// SENSOR structures (MUST be defined before PayLoad)
// =========================================================================

struct SensorData {
    int         index;       // unique index assigned during recording
    QString     id;          // sensor's unique ID (e.g., "radar_1")
    QString     type;        // "Radar", "AESA", "CSM", "ESM", "EO", "IR", "Sonar", "AIS", "ADSB"
    QJsonObject state;       // full JSON state from sensor->toJson()
};
using SensorList = std::vector<SensorData>;

struct SensorCRUD {
    int       index;
    Operation operation;     // CREATE, UPDATE, DELETE
};
using SensorCRUDList = std::vector<SensorCRUD>;
// ── IFF ──────────────────────────────────────────────────────────────────
struct IffData {
    int         index;
    QString     id;          // unique IFF component/sub-component ID
    QString     type;        // sub-type string
    QJsonObject state;       // full JSON from iff->toJson()
};
using IffList = std::vector<IffData>;

struct IffCRUD {
    int       index;
    Operation operation;
};
using IffCRUDList = std::vector<IffCRUD>;

// ── Radio ─────────────────────────────────────────────────────────────────
struct RadioData {
    int         index;
    QString     id;
    QString     type;
    QJsonObject state;
};
using RadioList = std::vector<RadioData>;

struct RadioCRUD {
    int       index;
    Operation operation;
};
using RadioCRUDList = std::vector<RadioCRUD>;

// ── Weapon ────────────────────────────────────────────────────────────────
struct WeaponData {
    int         index;
    QString     id;
    QString     type;
    QJsonObject state;
};
using WeaponList = std::vector<WeaponData>;

struct WeaponCRUD {
    int       index;
    Operation operation;
};
using WeaponCRUDList = std::vector<WeaponCRUD>;

// =========================================================================
// PayLoad – main data container passed between Recorder, SQLite and Replay
// =========================================================================

struct PayLoad {
    qint64              timestamp;
    int                 frameIndex;
    EntitiesMeshRenderer2DList    * entitiesMeshRenderer2DList;
    EntitiesMeshRenderer2DCRUDList* entitiesMeshRenderer2DCRUDList;
    ProfileCategoriesDetailsList * profileCategoriesDetailsList;
    ProfileCategoriesCRUDList    * profileCategoriesCRUDList;
    EntitiesDetailsList * entitiesDetailsList;
    EntitiesCreatedList * entitiesCreatedList;
    EntitiesUpdatedList * entitiesUpdatedList;
    EntitiesDeletedList * entitiesDeletedList;
    EntitiesTrajectoryList    * entitiesTrajectoryList;
    EntitiesTrajectoryCRUDList* entitiesTrajectoryCRUDList;

    // NEW sensor lists
    SensorList     * sensorsList;
    SensorCRUDList * sensorsCRUDList;
    IffList     * iffList;
    IffCRUDList * iffCRUDList;
    RadioList     * radioList;
    RadioCRUDList * radioCRUDList;
    WeaponList     * weaponList;
    WeaponCRUDList * weaponCRUDList;
};

// =========================================================================
// Legacy / compatibility structures
// =========================================================================

struct Old_PayLoad {
    qint64                         timestamp;
    int                            frameIndex;
    EntitiesMeshRenderer2DList     entitiesMeshRenderer2DList;
    EntitiesMeshRenderer2DCRUDList entitiesMeshRenderer2DCRUDList;
    ProfileCategoriesDetailsList   profileCategoriesDetailsList;
    ProfileCategoriesCRUDList      profileCategoriesCRUDList;
    EntitiesDetailsList            entitiesDetailsList;
    EntitiesCreatedList            entitiesCreatedList;
    EntitiesUpdatedList            entitiesUpdatedList;
    EntitiesDeletedList            entitiesDeletedList;
    EntitiesTrajectoryList         entitiesTrajectoryList;
    EntitiesTrajectoryCRUDList     entitiesTrajectoryCRUDList;
};

struct Old_ProfileCategoriesDetails {
    int       index;
    QString   name;
    QString   ID;
};

// =========================================================================
// SQLite & error enums
// =========================================================================

enum DB_Validity {
    DB_Valid = true,
    DB_Invalid = false
};

enum Logger_Error {
    Err_DB_UNAVAILABLE = 1,
    Err_DB_FILE_NOT_EXIST,
    Err_Undefine_Error,
};

// =========================================================================
// Shared pointer data packet (used in scripting / streaming)
// =========================================================================

struct DataPacket {
    int id;
    int noChange = 0;
    std::vector<double> dynamicValues;
};

#endif // PAYLOAD_H
