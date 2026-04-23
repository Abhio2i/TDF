// =============================================================================
// FILE:        constants.h
// MODULE:      Global Constants & Enumerations
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines global enumerations and type conversion utilities
//              used throughout the simulation framework. Includes parameter
//              types, formation types, entity types, collider types,
//              action types, state flags, mission values, and comparison
//              operators. Provides string conversion helpers for parameter
//              types.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QObject>
#include <QString>

// =============================================================================
// CLASS: Constants
//
// DESCRIPTION: Container class for all global enumerations and static
//              conversion functions. Not meant to be instantiated.
// =============================================================================
class Constants : public QObject
{
    Q_OBJECT
public:
    Constants() {}

    // =========================================================================
    // ENUM: ParameterType
    // DESCRIPTION: Data types supported for dynamic parameters.
    // =========================================================================
    enum ParameterType {
        INT,        //!< Integer
        FLOAT,      //!< Single-precision float
        DOUBLE,     //!< Double-precision float
        STRING,     //!< Text string
        LIST,       //!< List/array type
        CHAR,       //!< Character
        BOOL,       //!< Boolean (true/false)
        ENUM,       //!< Enumerated value
        VECTOR,     //!< 3D vector
        COLOR,      //!< RGB/RGBA colour
        OPTION,     //!< Option/choice type
        Unknown     //!< Unrecognised or unset type
    };
    Q_ENUM(ParameterType)

    // -------------------------------------------------------------------------
    // SECTION: Parameter Type String Conversion
    // DESCRIPTION: Converts between ParameterType enum and human-readable strings.
    // -------------------------------------------------------------------------
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

    // =========================================================================
    // ENUM: FormationType
    // DESCRIPTION: Tactical formation patterns for groups of entities.
    // =========================================================================
    enum FormationType {
        Line,           //!< Single file line
        V,              //!< V-shape
        Diamond,        //!< Diamond pattern
        Square,         //!< Square box formation
        Column,         //!< Column formation
        EchelonLeft,    //!< Left echelon (staggered)
        EchelonRight,   //!< Right echelon (staggered)
        StaggeredColumn,//!< Alternating left/right offset
        Wedge           //!< Wedge shape
    };
    Q_ENUM(FormationType)

    // =========================================================================
    // ENUM: EntityType
    // DESCRIPTION: Categorisation of simulation entities.
    // =========================================================================
    enum EntityType {
        Platform,       //!< Mobile platform (aircraft, ship, ground vehicle)
        Radio,          //!< Communication radio device
        Sensor,         //!< Sensor (radar, EO, IR)
        SpecialZone,    //!< Area of interest (danger zone, no-fly, etc.)
        Weapon,         //!< Weapon (missile, bomb, torpedo)
        IFF,            //!< Identification Friend or Foe device
        Supply,         //!< Logistics/supply entity
        FixedPoint,     //!< Static point of interest
        Formation       //!< Formation leader/group
    };
    Q_ENUM(EntityType)

    // =========================================================================
    // ENUM: ColliderType
    // DESCRIPTION: Shape types for physics collision detection.
    // =========================================================================
    enum ColliderType {
        Sphere,         //!< Spherical collision volume
        Box,            //!< Axis-aligned or oriented box
        Cyclinder       //!< Cylindrical collision volume (typo: Cyclinder)
    };
    Q_ENUM(ColliderType)

    // =========================================================================
    // ENUM: ActionType
    // DESCRIPTION: Types of actions that can be triggered by entities or scripts.
    // =========================================================================
    enum ActionType {
        Landing,        //!< Entity lands
        TakeOFf,        //!< Entity takes off
        Fire,           //!< Fire weapon
        Destroy,        //!< Entity destroyed
        Stop,           //!< Stop current activity
        On,             //!< Turn on / activate
        PassiveMode,    //!< Switch to passive mode (e.g., radar silent)
        ActiveMode,     //!< Switch to active mode
        Connect,        //!< Establish communication link
        Disconnect,     //!< Break communication link
        TrackCycleOn,   //!< Enable track cycling
        MissileLaunch,  //!< Launch missile
        GoTo            //!< Move to location
    };
    Q_ENUM(ActionType)

    // =========================================================================
    // ENUM: State
    // DESCRIPTION: Boolean state flags for entity condition.
    // =========================================================================
    enum State {
        isFly,          //!< Entity is airborne
        isLand,         //!< Entity is on ground/landed
        isFollow,       //!< Entity is following another
        havePayLoad,    //!< Entity carries weapons/cargo
        isOn,           //!< Entity is powered on
        isConnected,    //!< Communication link established
        isDisconnected, //!< Communication link lost
        isTrackMode,    //!< Sensor in track-while-scan mode
        isActive,       //!< Entity is actively engaged
        isTrigger,      //!< Condition trigger active
        isEmpty,        //!< No payload / empty
        isLossTaget     //!< Target lost (typo: LossTaget)
    };
    Q_ENUM(State)

    // =========================================================================
    // ENUM: MissionValue
    // DESCRIPTION: Key names for mission-related parameters.
    // =========================================================================
    enum MissionValue {
        Speed,          //!< Speed value
        Target,         //!< Target identifier
        Range,          //!< Range/distance value
        Bullets,        //!< Ammunition count
        DetectEntity,   //!< Detected entity ID
        EntityIFF,      //!< IFF code of entity
        Msg,            //!< Message string
        Weight,         //!< Weight/payload value
        Altitude,       //!< Altitude value
        Heading         //!< Heading angle
    };
    Q_ENUM(MissionValue)

    // =========================================================================
    // ENUM: OperaterType
    // DESCRIPTION: Comparison operators for condition evaluation.
    // =========================================================================
    enum OperaterType {
        Equal,          //!< Equality (==)
        NotEqual,       //!< Inequality (!=)
        Greater,        //!< Greater than (>)
        Less,           //!< Less than (<)
        GreaterEqual,   //!< Greater than or equal (>=)
        LessEqual       //!< Less than or equal (<=)
    };
    Q_ENUM(OperaterType)
};

#endif // CONSTANTS_H
