#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <QString>
#include <vector>
#include <QtGlobal>

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

struct PayLoad {
    qint64              timestamp;
    int                 frameIndex;
    EntitiesDetailsList entitiesDetailsList;
    EntitiesCreatedList entitiesCreatedList;
    EntitiesUpdatedList entitiesUpdatedList;
    EntitiesDeletedList entitiesDeletedList;
};

// Frame


#endif // PAYLOAD_H
