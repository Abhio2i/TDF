#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <QObject>
#include <QString>
#include <vector>
#include <QtGlobal>





struct ProfileCategoriesDetails {
    int       index;
    QString   name;
    QString   ID;
};
using ProfileCategoriesDetailsList = std::vector<ProfileCategoriesDetails>;

enum Operation {
    CREATE = 0b1000,
    READ   = 0b0100,
    UPDATE = 0b0010,
    DELETE = 0b0001
};

struct ProfileCategoriesCRUD {
    int       index;
    Operation operation;
};
using ProfileCategoriesCRUDList = std::vector<ProfileCategoriesCRUD>;

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


/*changeNo    INTEGER PRIMARY KEY AUTOINCREMENT,
 *    longitude   DOUBLE,
 *    latitude    DOUBLE,
 *    altitude    DOUBLE,
 *    heading     DOUBLE,
 *    turn_radius FLOAT,
 *    curr_speed  FLOAT,
 *    climb_rate  FLOAT,
 *    frameIndex  INTEGER,
 *    indexNo     INTEGER,
 *    FOREIGN KEY (indexNo) REFERENCES entities(indexNo),
 *    FOREIGN KEY (frameIndex) REFERENCES frameMap(frameIndex)
 */
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





struct TrajectoryWaypoint {
    int    index;
    double geo_latitude;
    double geo_longitude;
    double geo_altitude;
    double geo_Heading;
    float  vector_x;
    float  vector_y;
    float  vector_z;
    double speed     = 0;
    bool   sensor    = false;
    bool   formation = false;
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

struct PayLoad {
    qint64              timestamp;
    int                 frameIndex;
    EntitiesMeshRenderer2DList     entitiesMeshRenderer2DList;
    EntitiesMeshRenderer2DCRUDList entitiesMeshRenderer2DCRUDList;
    ProfileCategoriesDetailsList profileCategoriesDetailsList;
    ProfileCategoriesCRUDList    profileCategoriesCRUDList;
    EntitiesDetailsList entitiesDetailsList;
    EntitiesCreatedList entitiesCreatedList;
    EntitiesUpdatedList entitiesUpdatedList;
    EntitiesDeletedList entitiesDeletedList;
    EntitiesTrajectoryList     entitiesTrajectoryList;
    EntitiesTrajectoryCRUDList entitiesTrajectoryCRUDList;
};

// Frame


#endif // PAYLOAD_H
