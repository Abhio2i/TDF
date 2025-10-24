/****************************************************************************
** Meta object code from reading C++ file 'simulation.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/Simulation/simulation.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'simulation.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Simulation_t {
    QByteArrayData data[20];
    char stringdata0[181];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Simulation_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Simulation_t qt_meta_stringdata_Simulation = {
    {
QT_MOC_LITERAL(0, 0, 10), // "Simulation"
QT_MOC_LITERAL(1, 11, 5), // "Awake"
QT_MOC_LITERAL(2, 17, 0), // ""
QT_MOC_LITERAL(3, 18, 5), // "Begin"
QT_MOC_LITERAL(4, 24, 6), // "Update"
QT_MOC_LITERAL(5, 31, 7), // "Physics"
QT_MOC_LITERAL(6, 39, 15), // "HierarchyUpdate"
QT_MOC_LITERAL(7, 55, 6), // "Render"
QT_MOC_LITERAL(8, 62, 9), // "deltaTime"
QT_MOC_LITERAL(9, 72, 12), // "speedUpdated"
QT_MOC_LITERAL(10, 85, 5), // "speed"
QT_MOC_LITERAL(11, 91, 11), // "entityAdded"
QT_MOC_LITERAL(12, 103, 8), // "parentID"
QT_MOC_LITERAL(13, 112, 7), // "Entity*"
QT_MOC_LITERAL(14, 120, 6), // "entity"
QT_MOC_LITERAL(15, 127, 13), // "entityRemoved"
QT_MOC_LITERAL(16, 141, 2), // "ID"
QT_MOC_LITERAL(17, 144, 12), // "entityUpdate"
QT_MOC_LITERAL(18, 157, 17), // "handleReplayFrame"
QT_MOC_LITERAL(19, 175, 5) // "frame"

    },
    "Simulation\0Awake\0\0Begin\0Update\0Physics\0"
    "HierarchyUpdate\0Render\0deltaTime\0"
    "speedUpdated\0speed\0entityAdded\0parentID\0"
    "Entity*\0entity\0entityRemoved\0ID\0"
    "entityUpdate\0handleReplayFrame\0frame"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Simulation[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x06 /* Public */,
       3,    0,   70,    2, 0x06 /* Public */,
       4,    0,   71,    2, 0x06 /* Public */,
       5,    0,   72,    2, 0x06 /* Public */,
       6,    0,   73,    2, 0x06 /* Public */,
       7,    1,   74,    2, 0x06 /* Public */,
       9,    1,   77,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    2,   80,    2, 0x0a /* Public */,
      15,    1,   85,    2, 0x0a /* Public */,
      17,    1,   88,    2, 0x0a /* Public */,
      18,    1,   91,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float,    8,
    QMetaType::Void, QMetaType::Float,   10,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 13,   12,   14,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::QJsonObject,   19,

       0        // eod
};

void Simulation::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Simulation *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Awake(); break;
        case 1: _t->Begin(); break;
        case 2: _t->Update(); break;
        case 3: _t->Physics(); break;
        case 4: _t->HierarchyUpdate(); break;
        case 5: _t->Render((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 6: _t->speedUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 7: _t->entityAdded((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< Entity*(*)>(_a[2]))); break;
        case 8: _t->entityRemoved((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->entityUpdate((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->handleReplayFrame((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entity* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Simulation::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::Awake)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Simulation::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::Begin)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Simulation::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::Update)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Simulation::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::Physics)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Simulation::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::HierarchyUpdate)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Simulation::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::Render)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Simulation::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Simulation::speedUpdated)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Simulation::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Simulation.data,
    qt_meta_data_Simulation,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Simulation::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Simulation::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Simulation.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Simulation::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void Simulation::Awake()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Simulation::Begin()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Simulation::Update()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Simulation::Physics()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Simulation::HierarchyUpdate()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Simulation::Render(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Simulation::speedUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
