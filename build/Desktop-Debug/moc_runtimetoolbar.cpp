/****************************************************************************
** Meta object code from reading C++ file 'runtimetoolbar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Toolbars/runtimetoolbar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'runtimetoolbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RuntimeToolBar_t {
    QByteArrayData data[18];
    char stringdata0[241];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RuntimeToolBar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RuntimeToolBar_t qt_meta_stringdata_RuntimeToolBar = {
    {
QT_MOC_LITERAL(0, 0, 14), // "RuntimeToolBar"
QT_MOC_LITERAL(1, 15, 14), // "startTriggered"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 14), // "pauseTriggered"
QT_MOC_LITERAL(4, 46, 13), // "stopTriggered"
QT_MOC_LITERAL(5, 60, 17), // "nextStepTriggered"
QT_MOC_LITERAL(6, 78, 12), // "speedChanged"
QT_MOC_LITERAL(7, 91, 5), // "speed"
QT_MOC_LITERAL(8, 97, 15), // "replayTriggered"
QT_MOC_LITERAL(9, 113, 15), // "loggerTriggered"
QT_MOC_LITERAL(10, 129, 7), // "checked"
QT_MOC_LITERAL(11, 137, 14), // "startRecording"
QT_MOC_LITERAL(12, 152, 13), // "stopRecording"
QT_MOC_LITERAL(13, 166, 15), // "replayRecording"
QT_MOC_LITERAL(14, 182, 8), // "filePath"
QT_MOC_LITERAL(15, 191, 18), // "eventTypesSelected"
QT_MOC_LITERAL(16, 210, 10), // "eventTypes"
QT_MOC_LITERAL(17, 221, 19) // "radarDisplayToggled"

    },
    "RuntimeToolBar\0startTriggered\0\0"
    "pauseTriggered\0stopTriggered\0"
    "nextStepTriggered\0speedChanged\0speed\0"
    "replayTriggered\0loggerTriggered\0checked\0"
    "startRecording\0stopRecording\0"
    "replayRecording\0filePath\0eventTypesSelected\0"
    "eventTypes\0radarDisplayToggled"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RuntimeToolBar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x06 /* Public */,
       3,    0,   75,    2, 0x06 /* Public */,
       4,    0,   76,    2, 0x06 /* Public */,
       5,    0,   77,    2, 0x06 /* Public */,
       6,    1,   78,    2, 0x06 /* Public */,
       8,    0,   81,    2, 0x06 /* Public */,
       9,    1,   82,    2, 0x06 /* Public */,
      11,    0,   85,    2, 0x06 /* Public */,
      12,    0,   86,    2, 0x06 /* Public */,
      13,    1,   87,    2, 0x06 /* Public */,
      15,    1,   90,    2, 0x06 /* Public */,
      17,    0,   93,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QStringList,   16,
    QMetaType::Void,

       0        // eod
};

void RuntimeToolBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RuntimeToolBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->startTriggered(); break;
        case 1: _t->pauseTriggered(); break;
        case 2: _t->stopTriggered(); break;
        case 3: _t->nextStepTriggered(); break;
        case 4: _t->speedChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->replayTriggered(); break;
        case 6: _t->loggerTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->startRecording(); break;
        case 8: _t->stopRecording(); break;
        case 9: _t->replayRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->eventTypesSelected((*reinterpret_cast< QStringList(*)>(_a[1]))); break;
        case 11: _t->radarDisplayToggled(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::startTriggered)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::pauseTriggered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::stopTriggered)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::nextStepTriggered)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::speedChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::replayTriggered)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::loggerTriggered)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::startRecording)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::stopRecording)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::replayRecording)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)(QStringList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::eventTypesSelected)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (RuntimeToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RuntimeToolBar::radarDisplayToggled)) {
                *result = 11;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RuntimeToolBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolBar::staticMetaObject>(),
    qt_meta_stringdata_RuntimeToolBar.data,
    qt_meta_data_RuntimeToolBar,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RuntimeToolBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RuntimeToolBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RuntimeToolBar.stringdata0))
        return static_cast<void*>(this);
    return QToolBar::qt_metacast(_clname);
}

int RuntimeToolBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QToolBar::qt_metacall(_c, _id, _a);
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
void RuntimeToolBar::startTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RuntimeToolBar::pauseTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void RuntimeToolBar::stopTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void RuntimeToolBar::nextStepTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void RuntimeToolBar::speedChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RuntimeToolBar::replayTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void RuntimeToolBar::loggerTriggered(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void RuntimeToolBar::startRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void RuntimeToolBar::stopRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void RuntimeToolBar::replayRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void RuntimeToolBar::eventTypesSelected(QStringList _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void RuntimeToolBar::radarDisplayToggled()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
