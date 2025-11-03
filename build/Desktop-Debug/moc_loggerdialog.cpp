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
    QByteArrayData data[6];
    char stringdata0[71];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TimelineWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TimelineWidget_t qt_meta_stringdata_TimelineWidget = {
    {
QT_MOC_LITERAL(0, 0, 14), // "TimelineWidget"
QT_MOC_LITERAL(1, 15, 21), // "bookmarkButtonClicked"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 4), // "note"
QT_MOC_LITERAL(4, 43, 11), // "timestampMs"
QT_MOC_LITERAL(5, 55, 15) // "bookmarkClicked"

    },
    "TimelineWidget\0bookmarkButtonClicked\0"
    "\0note\0timestampMs\0bookmarkClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TimelineWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   24,    2, 0x06 /* Public */,
       5,    2,   29,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    3,    4,

       0        // eod
};

void TimelineWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TimelineWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->bookmarkButtonClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 1: _t->bookmarkClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TimelineWidget::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimelineWidget::bookmarkButtonClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TimelineWidget::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimelineWidget::bookmarkClicked)) {
                *result = 1;
                return;
            }
        }
    }
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
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void TimelineWidget::bookmarkButtonClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TimelineWidget::bookmarkClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_LoggerDialog_t {
    QByteArrayData data[26];
    char stringdata0[366];
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
QT_MOC_LITERAL(5, 57, 8), // "filePath"
QT_MOC_LITERAL(6, 66, 13), // "loadRecording"
QT_MOC_LITERAL(7, 80, 19), // "saveRecordingToFile"
QT_MOC_LITERAL(8, 100, 10), // "recordings"
QT_MOC_LITERAL(9, 111, 22), // "saveRecordingRequested"
QT_MOC_LITERAL(10, 134, 15), // "replayRecording"
QT_MOC_LITERAL(11, 150, 18), // "eventTypesSelected"
QT_MOC_LITERAL(12, 169, 10), // "eventTypes"
QT_MOC_LITERAL(13, 180, 13), // "bookmarkAdded"
QT_MOC_LITERAL(14, 194, 12), // "bookmarkNote"
QT_MOC_LITERAL(15, 207, 16), // "timestampToggled"
QT_MOC_LITERAL(16, 224, 7), // "enabled"
QT_MOC_LITERAL(17, 232, 15), // "bookmarkClicked"
QT_MOC_LITERAL(18, 248, 4), // "note"
QT_MOC_LITERAL(19, 253, 11), // "timestampMs"
QT_MOC_LITERAL(20, 265, 21), // "bookmarkButtonClicked"
QT_MOC_LITERAL(21, 287, 20), // "showBookmarkOnReplay"
QT_MOC_LITERAL(22, 308, 9), // "timestamp"
QT_MOC_LITERAL(23, 318, 19), // "setTimelineDuration"
QT_MOC_LITERAL(24, 338, 8), // "duration"
QT_MOC_LITERAL(25, 347, 18) // "replayFromBookmark"

    },
    "LoggerDialog\0startRecording\0\0stopRecording\0"
    "saveRecording\0filePath\0loadRecording\0"
    "saveRecordingToFile\0recordings\0"
    "saveRecordingRequested\0replayRecording\0"
    "eventTypesSelected\0eventTypes\0"
    "bookmarkAdded\0bookmarkNote\0timestampToggled\0"
    "enabled\0bookmarkClicked\0note\0timestampMs\0"
    "bookmarkButtonClicked\0showBookmarkOnReplay\0"
    "timestamp\0setTimelineDuration\0duration\0"
    "replayFromBookmark"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LoggerDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x06 /* Public */,
       3,    0,   90,    2, 0x06 /* Public */,
       4,    1,   91,    2, 0x06 /* Public */,
       6,    1,   94,    2, 0x06 /* Public */,
       7,    1,   97,    2, 0x06 /* Public */,
       9,    0,  100,    2, 0x06 /* Public */,
      10,    1,  101,    2, 0x06 /* Public */,
      11,    1,  104,    2, 0x06 /* Public */,
      13,    1,  107,    2, 0x06 /* Public */,
      15,    1,  110,    2, 0x06 /* Public */,
      17,    2,  113,    2, 0x06 /* Public */,
      20,    2,  118,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      21,    2,  123,    2, 0x0a /* Public */,
      23,    1,  128,    2, 0x0a /* Public */,
      25,    2,  131,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QJsonObject,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QStringList,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::Bool,   16,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   18,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   18,   19,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   18,   22,
    QMetaType::Void, QMetaType::LongLong,   24,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   18,   22,

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
        case 2: _t->saveRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->loadRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->saveRecordingToFile((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 5: _t->saveRecordingRequested(); break;
        case 6: _t->replayRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->eventTypesSelected((*reinterpret_cast< QStringList(*)>(_a[1]))); break;
        case 8: _t->bookmarkAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->timestampToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->bookmarkClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 11: _t->bookmarkButtonClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 12: _t->showBookmarkOnReplay((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 13: _t->setTimelineDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 14: _t->replayFromBookmark((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
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
            using _t = void (LoggerDialog::*)(const QString & );
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
            using _t = void (LoggerDialog::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecordingToFile)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecordingRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayRecording)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(QStringList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::eventTypesSelected)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkAdded)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::timestampToggled)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkClicked)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkButtonClicked)) {
                *result = 11;
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
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
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
void LoggerDialog::saveRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void LoggerDialog::loadRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LoggerDialog::saveRecordingToFile(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void LoggerDialog::saveRecordingRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void LoggerDialog::replayRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void LoggerDialog::eventTypesSelected(QStringList _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void LoggerDialog::bookmarkAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void LoggerDialog::timestampToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void LoggerDialog::bookmarkClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void LoggerDialog::bookmarkButtonClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
