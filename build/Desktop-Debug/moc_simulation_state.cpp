/****************************************************************************
** Meta object code from reading C++ file 'simulation_state.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/Simulation/simulation_state.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'simulation_state.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SimulationStateNS_t {
    QByteArrayData data[9];
    char stringdata0[85];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SimulationStateNS_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SimulationStateNS_t qt_meta_stringdata_SimulationStateNS = {
    {
QT_MOC_LITERAL(0, 0, 17), // "SimulationStateNS"
QT_MOC_LITERAL(1, 18, 5), // "State"
QT_MOC_LITERAL(2, 24, 10), // "INITIALIZE"
QT_MOC_LITERAL(3, 35, 12), // "REINITIALIZE"
QT_MOC_LITERAL(4, 48, 12), // "DEINITIALIZE"
QT_MOC_LITERAL(5, 61, 5), // "START"
QT_MOC_LITERAL(6, 67, 6), // "UPDATE"
QT_MOC_LITERAL(7, 74, 5), // "PAUSE"
QT_MOC_LITERAL(8, 80, 4) // "STOP"

    },
    "SimulationStateNS\0State\0INITIALIZE\0"
    "REINITIALIZE\0DEINITIALIZE\0START\0UPDATE\0"
    "PAUSE\0STOP"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SimulationStateNS[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       1,   14, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // enums: name, alias, flags, count, data
       1,    1, 0x0,    7,   19,

 // enum data: key, value
       2, uint(SimulationStateNS::INITIALIZE),
       3, uint(SimulationStateNS::REINITIALIZE),
       4, uint(SimulationStateNS::DEINITIALIZE),
       5, uint(SimulationStateNS::START),
       6, uint(SimulationStateNS::UPDATE),
       7, uint(SimulationStateNS::PAUSE),
       8, uint(SimulationStateNS::STOP),

       0        // eod
};

QT_INIT_METAOBJECT const QMetaObject SimulationStateNS::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_SimulationStateNS.data,
    qt_meta_data_SimulationStateNS,
    nullptr,
    nullptr,
    nullptr
} };

struct qt_meta_stringdata_SimTypeOfUpdates_t {
    QByteArrayData data[7];
    char stringdata0[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SimTypeOfUpdates_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SimTypeOfUpdates_t qt_meta_stringdata_SimTypeOfUpdates = {
    {
QT_MOC_LITERAL(0, 0, 16), // "SimTypeOfUpdates"
QT_MOC_LITERAL(1, 17, 12), // "TypeOfUpdate"
QT_MOC_LITERAL(2, 30, 14), // "dynamicDynamic"
QT_MOC_LITERAL(3, 45, 13), // "dynamicStatic"
QT_MOC_LITERAL(4, 59, 10), // "trajectory"
QT_MOC_LITERAL(5, 70, 6), // "sensor"
QT_MOC_LITERAL(6, 77, 6) // "entity"

    },
    "SimTypeOfUpdates\0TypeOfUpdate\0"
    "dynamicDynamic\0dynamicStatic\0trajectory\0"
    "sensor\0entity"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SimTypeOfUpdates[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       1,   14, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // enums: name, alias, flags, count, data
       1,    1, 0x0,    5,   19,

 // enum data: key, value
       2, uint(SimTypeOfUpdates::dynamicDynamic),
       3, uint(SimTypeOfUpdates::dynamicStatic),
       4, uint(SimTypeOfUpdates::trajectory),
       5, uint(SimTypeOfUpdates::sensor),
       6, uint(SimTypeOfUpdates::entity),

       0        // eod
};

QT_INIT_METAOBJECT const QMetaObject SimTypeOfUpdates::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_SimTypeOfUpdates.data,
    qt_meta_data_SimTypeOfUpdates,
    nullptr,
    nullptr,
    nullptr
} };

struct qt_meta_stringdata_SimUpdateTypes_t {
    QByteArrayData data[5];
    char stringdata0[48];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SimUpdateTypes_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SimUpdateTypes_t qt_meta_stringdata_SimUpdateTypes = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SimUpdateTypes"
QT_MOC_LITERAL(1, 15, 11), // "UpdateTypes"
QT_MOC_LITERAL(2, 27, 6), // "CREATE"
QT_MOC_LITERAL(3, 34, 6), // "UPDATE"
QT_MOC_LITERAL(4, 41, 6) // "DELETE"

    },
    "SimUpdateTypes\0UpdateTypes\0CREATE\0"
    "UPDATE\0DELETE"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SimUpdateTypes[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       1,   14, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // enums: name, alias, flags, count, data
       1,    1, 0x0,    3,   19,

 // enum data: key, value
       2, uint(SimUpdateTypes::CREATE),
       3, uint(SimUpdateTypes::UPDATE),
       4, uint(SimUpdateTypes::DELETE),

       0        // eod
};

QT_INIT_METAOBJECT const QMetaObject SimUpdateTypes::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_SimUpdateTypes.data,
    qt_meta_data_SimUpdateTypes,
    nullptr,
    nullptr,
    nullptr
} };

QT_WARNING_POP
QT_END_MOC_NAMESPACE
