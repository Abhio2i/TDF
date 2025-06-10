#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>

class Constants: public QObject
{
    Q_OBJECT
public:
    Constants();

    enum ParameterType{
        INT,
        FLOAT,
        STRING,
        LIST,
        CHAR,
        BOOL,
        ENUM
    };
    Q_ENUM(ParameterType)

    enum FormationType{
        Line,
        V,
        Diamond
    };
    Q_ENUM(FormationType)

    enum EntityType{
        Platform,
        Radio,
        Sensor,
        SpecialZone,
        Weapon,
        IFF,
        Supply
    };
    Q_ENUM(EntityType)

    enum ColliderType{
        Sphere,
        Box,
        Cyclinder
    };
    Q_ENUM(ColliderType)

    enum ActionType{
        Landing,
        TakeOFf,
        Fire,
        Destroy,
        Stop,
        On,
        PassiveMode,
        ActiveMode,
        Connect,
        Disconnect,
        TrackCycleOn,
        MissileLaunch,
        GoTo,//excute specific task
    };
    Q_ENUM(ActionType)


    enum State{
        isFly,
        isLand,
        isFollow,
        havePayLoad,
        isOn,
        isConnected,
        isDisconnected,
        isTrackMode,
        isActive,
        isTrigger,
        isEmpty,
        isLossTaget
    };
    Q_ENUM(State)

    enum MissionValue{
        Speed,
        Target,
        Range,
        Bullets,
        DetectEntity,
        EntityIFF,
        Msg,
        Weight,
        Altitude,
        Heading
    };
    Q_ENUM(MissionValue)

    enum OperaterType{
        Equal,
        NotEqual,
        Greater,
        Less,
        GreaterEqual,
        LessEqual
    };
    Q_ENUM(OperaterType)

};

#endif // CONSTANTS_H
