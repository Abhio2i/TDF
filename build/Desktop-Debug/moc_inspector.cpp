/****************************************************************************
** Meta object code from reading C++ file 'inspector.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Inspector/inspector.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'inspector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WheelableLineEdit_t {
    QByteArrayData data[1];
    char stringdata0[18];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WheelableLineEdit_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WheelableLineEdit_t qt_meta_stringdata_WheelableLineEdit = {
    {
QT_MOC_LITERAL(0, 0, 17) // "WheelableLineEdit"

    },
    "WheelableLineEdit"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WheelableLineEdit[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void WheelableLineEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject WheelableLineEdit::staticMetaObject = { {
    QMetaObject::SuperData::link<QLineEdit::staticMetaObject>(),
    qt_meta_stringdata_WheelableLineEdit.data,
    qt_meta_data_WheelableLineEdit,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WheelableLineEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WheelableLineEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WheelableLineEdit.stringdata0))
        return static_cast<void*>(this);
    return QLineEdit::qt_metacast(_clname);
}

int WheelableLineEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_Inspector_t {
    QByteArrayData data[28];
    char stringdata0[322];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Inspector_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Inspector_t qt_meta_stringdata_Inspector = {
    {
QT_MOC_LITERAL(0, 0, 9), // "Inspector"
QT_MOC_LITERAL(1, 10, 11), // "foucsEntity"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 2), // "ID"
QT_MOC_LITERAL(4, 26, 12), // "valueChanged"
QT_MOC_LITERAL(5, 39, 4), // "name"
QT_MOC_LITERAL(6, 44, 5), // "delta"
QT_MOC_LITERAL(7, 50, 15), // "addTabRequested"
QT_MOC_LITERAL(8, 66, 16), // "parameterChanged"
QT_MOC_LITERAL(9, 83, 3), // "key"
QT_MOC_LITERAL(10, 87, 13), // "parameterType"
QT_MOC_LITERAL(11, 101, 3), // "add"
QT_MOC_LITERAL(12, 105, 26), // "trajectoryWaypointsChanged"
QT_MOC_LITERAL(13, 132, 8), // "entityId"
QT_MOC_LITERAL(14, 141, 9), // "waypoints"
QT_MOC_LITERAL(15, 151, 4), // "init"
QT_MOC_LITERAL(16, 156, 3), // "obj"
QT_MOC_LITERAL(17, 160, 12), // "addSimpleRow"
QT_MOC_LITERAL(18, 173, 3), // "row"
QT_MOC_LITERAL(19, 177, 5), // "value"
QT_MOC_LITERAL(20, 183, 14), // "setupValueCell"
QT_MOC_LITERAL(21, 198, 7), // "fullKey"
QT_MOC_LITERAL(22, 206, 16), // "updateTrajectory"
QT_MOC_LITERAL(23, 223, 20), // "copyCurrentComponent"
QT_MOC_LITERAL(24, 244, 23), // "pasteToCurrentComponent"
QT_MOC_LITERAL(25, 268, 12), // "handleAddTab"
QT_MOC_LITERAL(26, 281, 18), // "handleAddParameter"
QT_MOC_LITERAL(27, 300, 21) // "handleRemoveParameter"

    },
    "Inspector\0foucsEntity\0\0ID\0valueChanged\0"
    "name\0delta\0addTabRequested\0parameterChanged\0"
    "key\0parameterType\0add\0trajectoryWaypointsChanged\0"
    "entityId\0waypoints\0init\0obj\0addSimpleRow\0"
    "row\0value\0setupValueCell\0fullKey\0"
    "updateTrajectory\0copyCurrentComponent\0"
    "pasteToCurrentComponent\0handleAddTab\0"
    "handleAddParameter\0handleRemoveParameter"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Inspector[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   84,    2, 0x06 /* Public */,
       4,    3,   87,    2, 0x06 /* Public */,
       7,    0,   94,    2, 0x06 /* Public */,
       8,    5,   95,    2, 0x06 /* Public */,
      12,    2,  106,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    3,  111,    2, 0x0a /* Public */,
      17,    3,  118,    2, 0x0a /* Public */,
      20,    3,  125,    2, 0x0a /* Public */,
      22,    2,  132,    2, 0x0a /* Public */,
      23,    0,  137,    2, 0x08 /* Private */,
      24,    0,  138,    2, 0x08 /* Private */,
      25,    0,  139,    2, 0x08 /* Private */,
      26,    0,  140,    2, 0x08 /* Private */,
      27,    0,  141,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QJsonObject,    3,    5,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Bool,    3,    5,    9,   10,   11,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,   13,   14,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QJsonObject,    3,    5,   16,
    QMetaType::Int, QMetaType::Int, QMetaType::QString, QMetaType::QJsonValue,   18,    9,   19,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QJsonValue,   18,   21,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,   13,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Inspector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Inspector *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->foucsEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->valueChanged((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QJsonObject(*)>(_a[3]))); break;
        case 2: _t->addTabRequested(); break;
        case 3: _t->parameterChanged((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< bool(*)>(_a[5]))); break;
        case 4: _t->trajectoryWaypointsChanged((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 5: _t->init((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QJsonObject(*)>(_a[3]))); break;
        case 6: { int _r = _t->addSimpleRow((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QJsonValue(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->setupValueCell((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QJsonValue(*)>(_a[3]))); break;
        case 8: _t->updateTrajectory((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 9: _t->copyCurrentComponent(); break;
        case 10: _t->pasteToCurrentComponent(); break;
        case 11: _t->handleAddTab(); break;
        case 12: _t->handleAddParameter(); break;
        case 13: _t->handleRemoveParameter(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Inspector::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::foucsEntity)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(QString , QString , QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::valueChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Inspector::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::addTabRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(QString , QString , QString , QString , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::parameterChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(QString , QJsonArray );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::trajectoryWaypointsChanged)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Inspector::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_Inspector.data,
    qt_meta_data_Inspector,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Inspector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Inspector::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Inspector.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int Inspector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void Inspector::foucsEntity(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Inspector::valueChanged(QString _t1, QString _t2, QJsonObject _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Inspector::addTabRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Inspector::parameterChanged(QString _t1, QString _t2, QString _t3, QString _t4, bool _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Inspector::trajectoryWaypointsChanged(QString _t1, QJsonArray _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
