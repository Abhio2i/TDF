/****************************************************************************
** Meta object code from reading C++ file 'applicationdialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Settings/applicationdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'applicationdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ApplicationDialog_t {
    QByteArrayData data[17];
    char stringdata0[237];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ApplicationDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ApplicationDialog_t qt_meta_stringdata_ApplicationDialog = {
    {
QT_MOC_LITERAL(0, 0, 17), // "ApplicationDialog"
QT_MOC_LITERAL(1, 18, 8), // "fpsState"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 5), // "value"
QT_MOC_LITERAL(4, 34, 11), // "guiFPSState"
QT_MOC_LITERAL(5, 46, 18), // "simulationFPSState"
QT_MOC_LITERAL(6, 65, 15), // "physicsFPSState"
QT_MOC_LITERAL(7, 81, 15), // "canvasIconState"
QT_MOC_LITERAL(8, 97, 18), // "developerModeState"
QT_MOC_LITERAL(9, 116, 7), // "enabled"
QT_MOC_LITERAL(10, 124, 23), // "databaseSettingsChanged"
QT_MOC_LITERAL(11, 148, 4), // "path"
QT_MOC_LITERAL(12, 153, 11), // "onOkClicked"
QT_MOC_LITERAL(13, 165, 15), // "onCancelClicked"
QT_MOC_LITERAL(14, 181, 14), // "validateInputs"
QT_MOC_LITERAL(15, 196, 19), // "onResetDatabasePath"
QT_MOC_LITERAL(16, 216, 20) // "onBrowseDatabasePath"

    },
    "ApplicationDialog\0fpsState\0\0value\0"
    "guiFPSState\0simulationFPSState\0"
    "physicsFPSState\0canvasIconState\0"
    "developerModeState\0enabled\0"
    "databaseSettingsChanged\0path\0onOkClicked\0"
    "onCancelClicked\0validateInputs\0"
    "onResetDatabasePath\0onBrowseDatabasePath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ApplicationDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   74,    2, 0x06 /* Public */,
       4,    1,   77,    2, 0x06 /* Public */,
       5,    1,   80,    2, 0x06 /* Public */,
       6,    1,   83,    2, 0x06 /* Public */,
       7,    1,   86,    2, 0x06 /* Public */,
       8,    1,   89,    2, 0x06 /* Public */,
      10,    2,   92,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,   97,    2, 0x08 /* Private */,
      13,    0,   98,    2, 0x08 /* Private */,
      14,    0,   99,    2, 0x08 /* Private */,
      15,    0,  100,    2, 0x08 /* Private */,
      16,    0,  101,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    9,   11,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ApplicationDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ApplicationDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->fpsState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->guiFPSState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->simulationFPSState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->physicsFPSState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->canvasIconState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->developerModeState((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->databaseSettingsChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->onOkClicked(); break;
        case 8: _t->onCancelClicked(); break;
        case 9: _t->validateInputs(); break;
        case 10: _t->onResetDatabasePath(); break;
        case 11: _t->onBrowseDatabasePath(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ApplicationDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::fpsState)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::guiFPSState)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::simulationFPSState)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::physicsFPSState)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::canvasIconState)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::developerModeState)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ApplicationDialog::*)(bool , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationDialog::databaseSettingsChanged)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ApplicationDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ApplicationDialog.data,
    qt_meta_data_ApplicationDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ApplicationDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ApplicationDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ApplicationDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ApplicationDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void ApplicationDialog::fpsState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ApplicationDialog::guiFPSState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ApplicationDialog::simulationFPSState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ApplicationDialog::physicsFPSState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ApplicationDialog::canvasIconState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ApplicationDialog::developerModeState(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ApplicationDialog::databaseSettingsChanged(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
