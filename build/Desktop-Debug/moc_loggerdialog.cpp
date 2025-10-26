/****************************************************************************
** Meta object code from reading C++ file 'loggerdialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Logger/loggerdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'loggerdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TimelineWidget_t {
    QByteArrayData data[1];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TimelineWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TimelineWidget_t qt_meta_stringdata_TimelineWidget = {
    {
QT_MOC_LITERAL(0, 0, 14) // "TimelineWidget"

    },
    "TimelineWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TimelineWidget[] = {

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

void TimelineWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject TimelineWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TimelineWidget.data,
    qt_meta_data_TimelineWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TimelineWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TimelineWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TimelineWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TimelineWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_LoggerDialog_t {
    QByteArrayData data[14];
    char stringdata0[178];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LoggerDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LoggerDialog_t qt_meta_stringdata_LoggerDialog = {
    {
QT_MOC_LITERAL(0, 0, 12), // "LoggerDialog"
QT_MOC_LITERAL(1, 13, 14), // "startRecording"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 13), // "stopRecording"
QT_MOC_LITERAL(4, 43, 13), // "saveRecording"
QT_MOC_LITERAL(5, 57, 13), // "loadRecording"
QT_MOC_LITERAL(6, 71, 8), // "filePath"
QT_MOC_LITERAL(7, 80, 15), // "replayRecording"
QT_MOC_LITERAL(8, 96, 18), // "eventTypesSelected"
QT_MOC_LITERAL(9, 115, 10), // "eventTypes"
QT_MOC_LITERAL(10, 126, 13), // "bookmarkAdded"
QT_MOC_LITERAL(11, 140, 12), // "bookmarkNote"
QT_MOC_LITERAL(12, 153, 16), // "timestampToggled"
QT_MOC_LITERAL(13, 170, 7) // "enabled"

    },
    "LoggerDialog\0startRecording\0\0stopRecording\0"
    "saveRecording\0loadRecording\0filePath\0"
    "replayRecording\0eventTypesSelected\0"
    "eventTypes\0bookmarkAdded\0bookmarkNote\0"
    "timestampToggled\0enabled"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LoggerDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    0,   56,    2, 0x06 /* Public */,
       5,    1,   57,    2, 0x06 /* Public */,
       7,    1,   60,    2, 0x06 /* Public */,
       8,    1,   63,    2, 0x06 /* Public */,
      10,    1,   66,    2, 0x06 /* Public */,
      12,    1,   69,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QStringList,    9,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::Bool,   13,

       0        // eod
};

void LoggerDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LoggerDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->startRecording(); break;
        case 1: _t->stopRecording(); break;
        case 2: _t->saveRecording(); break;
        case 3: _t->loadRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->replayRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->eventTypesSelected((*reinterpret_cast< QStringList(*)>(_a[1]))); break;
        case 6: _t->bookmarkAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->timestampToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::startRecording)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::stopRecording)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecording)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::loadRecording)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayRecording)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(QStringList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::eventTypesSelected)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkAdded)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::timestampToggled)) {
                *result = 7;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject LoggerDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_LoggerDialog.data,
    qt_meta_data_LoggerDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *LoggerDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LoggerDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LoggerDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int LoggerDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void LoggerDialog::startRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LoggerDialog::stopRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LoggerDialog::saveRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void LoggerDialog::loadRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LoggerDialog::replayRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void LoggerDialog::eventTypesSelected(QStringList _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void LoggerDialog::bookmarkAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void LoggerDialog::timestampToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
