/****************************************************************************
** Meta object code from reading C++ file 'sqlite.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/SQLite/sqlite.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sqlite.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SQLite_t {
    QByteArrayData data[17];
    char stringdata0[162];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SQLite_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SQLite_t qt_meta_stringdata_SQLite = {
    {
QT_MOC_LITERAL(0, 0, 6), // "SQLite"
QT_MOC_LITERAL(1, 7, 11), // "sendPayLoad"
QT_MOC_LITERAL(2, 19, 0), // ""
QT_MOC_LITERAL(3, 20, 7), // "PayLoad"
QT_MOC_LITERAL(4, 28, 9), // "m_payLoad"
QT_MOC_LITERAL(5, 38, 7), // "Options"
QT_MOC_LITERAL(6, 46, 4), // "INIT"
QT_MOC_LITERAL(7, 51, 6), // "DEINIT"
QT_MOC_LITERAL(8, 58, 10), // "DBStatuses"
QT_MOC_LITERAL(9, 69, 9), // "CONNECTED"
QT_MOC_LITERAL(10, 79, 12), // "DISCONNECTED"
QT_MOC_LITERAL(11, 92, 11), // "debugSQLite"
QT_MOC_LITERAL(12, 104, 6), // "D_NULL"
QT_MOC_LITERAL(13, 111, 11), // "D_JustPrint"
QT_MOC_LITERAL(14, 123, 12), // "D_GetPayLoad"
QT_MOC_LITERAL(15, 136, 12), // "D_SetPayLoad"
QT_MOC_LITERAL(16, 149, 12) // "D_Trajectory"

    },
    "SQLite\0sendPayLoad\0\0PayLoad\0m_payLoad\0"
    "Options\0INIT\0DEINIT\0DBStatuses\0CONNECTED\0"
    "DISCONNECTED\0debugSQLite\0D_NULL\0"
    "D_JustPrint\0D_GetPayLoad\0D_SetPayLoad\0"
    "D_Trajectory"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SQLite[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       3,   22, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // enums: name, alias, flags, count, data
       5,    5, 0x0,    2,   37,
       8,    8, 0x0,    2,   41,
      11,   11, 0x0,    5,   45,

 // enum data: key, value
       6, uint(SQLite::INIT),
       7, uint(SQLite::DEINIT),
       9, uint(SQLite::CONNECTED),
      10, uint(SQLite::DISCONNECTED),
      12, uint(SQLite::D_NULL),
      13, uint(SQLite::D_JustPrint),
      14, uint(SQLite::D_GetPayLoad),
      15, uint(SQLite::D_SetPayLoad),
      16, uint(SQLite::D_Trajectory),

       0        // eod
};

void SQLite::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SQLite *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sendPayLoad((*reinterpret_cast< PayLoad(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SQLite::*)(PayLoad );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SQLite::sendPayLoad)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SQLite::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SQLite.data,
    qt_meta_data_SQLite,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SQLite::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SQLite::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SQLite.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SQLite::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void SQLite::sendPayLoad(PayLoad _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
