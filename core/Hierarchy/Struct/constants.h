
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>
#include <QString>

class Constants : public QObject
{
    Q_OBJECT
public:
    Constants() {}

    enum ParameterType {
        INT,
        FLOAT,
        DOUBLE,
        STRING,
        LIST,
        CHAR,
        BOOL,
        ENUM,
        VECTOR,
        COLOR,
        OPTION,
        Unknown
    };
    Q_ENUM(ParameterType)

    static QString parameterTypeToString(ParameterType type) {
        switch (type) {
        case INT: return "int";
        case FLOAT: return "float";
        case DOUBLE: return "double";
        case STRING: return "string";
        case LIST: return "list";
        case CHAR: return "char";
        case BOOL: return "boolean";
        case ENUM: return "enum";
        case VECTOR: return "vector";
        case COLOR: return "color";
        case OPTION: return "option";
        }
        return "unknown";
    }

    static ParameterType stringToParameterType(const QString& str) {
        if (str == "int") return INT;
        if (str == "float") return FLOAT;
        if (str == "double") return DOUBLE;
        if (str == "string") return STRING;
        if (str == "list") return LIST;
        if (str == "char") return CHAR;
        if (str == "boolean") return BOOL;
        if (str == "enum") return ENUM;
        if (str == "vector") return VECTOR;
        if (str == "color") return COLOR;
        if (str == "option") return OPTION;
        return INT; // Default
    }

    enum FormationType {
        Line,
        V,
        Diamond
    };
    Q_ENUM(FormationType)

    enum EntityType {
        Platform,
        Radio,
        Sensor,
        SpecialZone,
        Weapon,
        IFF,
        Supply,
        FixedPoint,
        Formation
    };
    Q_ENUM(EntityType)

    enum ColliderType {
        Sphere,
        Box,
        Cyclinder
    };
    Q_ENUM(ColliderType)

    enum ActionType {
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
        GoTo
    };
    Q_ENUM(ActionType)

    enum State {
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

    enum MissionValue {
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

    enum OperaterType {
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
