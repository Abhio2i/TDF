/****************************************************************************
** Meta object code from reading C++ file 'recorder.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/Recorder/recorder.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'recorder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Recorder_t {
    QByteArrayData data[53];
    char stringdata0[803];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Recorder_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Recorder_t qt_meta_stringdata_Recorder = {
    {
QT_MOC_LITERAL(0, 0, 8), // "Recorder"
QT_MOC_LITERAL(1, 9, 11), // "replayFrame"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 5), // "frame"
QT_MOC_LITERAL(4, 28, 13), // "bookmarkAdded"
QT_MOC_LITERAL(5, 42, 4), // "note"
QT_MOC_LITERAL(6, 47, 11), // "timestampMs"
QT_MOC_LITERAL(7, 59, 14), // "replayBookmark"
QT_MOC_LITERAL(8, 74, 9), // "timestamp"
QT_MOC_LITERAL(9, 84, 15), // "bookmarkReached"
QT_MOC_LITERAL(10, 100, 17), // "setReplayDuration"
QT_MOC_LITERAL(11, 118, 8), // "duration"
QT_MOC_LITERAL(12, 127, 15), // "recordingPaused"
QT_MOC_LITERAL(13, 143, 16), // "recordingResumed"
QT_MOC_LITERAL(14, 160, 11), // "frameLoaded"
QT_MOC_LITERAL(15, 172, 17), // "replayFrameLoaded"
QT_MOC_LITERAL(16, 190, 20), // "recordingTimeUpdated"
QT_MOC_LITERAL(17, 211, 2), // "ms"
QT_MOC_LITERAL(18, 214, 16), // "recorderInfoSend"
QT_MOC_LITERAL(19, 231, 27), // "Recorder::LoggerStatusModes"
QT_MOC_LITERAL(20, 259, 12), // "loggerStatus"
QT_MOC_LITERAL(21, 272, 20), // "recorderInfoSendOnce"
QT_MOC_LITERAL(22, 293, 20), // "s_recordingStartTime"
QT_MOC_LITERAL(23, 314, 10), // "s_duration"
QT_MOC_LITERAL(24, 325, 31), // "Recorder::SimulationStatusModes"
QT_MOC_LITERAL(25, 357, 16), // "simulationStatus"
QT_MOC_LITERAL(26, 374, 21), // "recorderInfoSendUsual"
QT_MOC_LITERAL(27, 396, 24), // "recorderInfoSendDuration"
QT_MOC_LITERAL(28, 421, 13), // "playNextFrame"
QT_MOC_LITERAL(29, 435, 11), // "togglePause"
QT_MOC_LITERAL(30, 447, 16), // "resetReplayState"
QT_MOC_LITERAL(31, 464, 15), // "loggerModeCheck"
QT_MOC_LITERAL(32, 480, 11), // "loggerModes"
QT_MOC_LITERAL(33, 492, 4), // "mode"
QT_MOC_LITERAL(34, 497, 14), // "recorderStatus"
QT_MOC_LITERAL(35, 512, 9), // "RECORDING"
QT_MOC_LITERAL(36, 522, 6), // "REPLAY"
QT_MOC_LITERAL(37, 529, 17), // "LoggerStatusModes"
QT_MOC_LITERAL(38, 547, 16), // "S_RECORDING_MODE"
QT_MOC_LITERAL(39, 564, 11), // "S_RECORDING"
QT_MOC_LITERAL(40, 576, 18), // "S_RECORDING_PAUSED"
QT_MOC_LITERAL(41, 595, 19), // "S_RECORDING_STOPPED"
QT_MOC_LITERAL(42, 615, 13), // "S_REPLAY_MODE"
QT_MOC_LITERAL(43, 629, 15), // "S_REPLAY_LOADED"
QT_MOC_LITERAL(44, 645, 17), // "S_REPLAY_UNLOADED"
QT_MOC_LITERAL(45, 663, 11), // "S_REPLAYING"
QT_MOC_LITERAL(46, 675, 15), // "S_REPLAY_PAUSED"
QT_MOC_LITERAL(47, 691, 16), // "S_REPLAY_STOPPED"
QT_MOC_LITERAL(48, 708, 21), // "SimulationStatusModes"
QT_MOC_LITERAL(49, 730, 18), // "S_SIMULATION_START"
QT_MOC_LITERAL(50, 749, 19), // "S_SIMULATION_PAUSED"
QT_MOC_LITERAL(51, 769, 17), // "S_SIMULATION_STOP"
QT_MOC_LITERAL(52, 787, 15) // "S_SIMULATION_NA"

    },
    "Recorder\0replayFrame\0\0frame\0bookmarkAdded\0"
    "note\0timestampMs\0replayBookmark\0"
    "timestamp\0bookmarkReached\0setReplayDuration\0"
    "duration\0recordingPaused\0recordingResumed\0"
    "frameLoaded\0replayFrameLoaded\0"
    "recordingTimeUpdated\0ms\0recorderInfoSend\0"
    "Recorder::LoggerStatusModes\0loggerStatus\0"
    "recorderInfoSendOnce\0s_recordingStartTime\0"
    "s_duration\0Recorder::SimulationStatusModes\0"
    "simulationStatus\0recorderInfoSendUsual\0"
    "recorderInfoSendDuration\0playNextFrame\0"
    "togglePause\0resetReplayState\0"
    "loggerModeCheck\0loggerModes\0mode\0"
    "recorderStatus\0RECORDING\0REPLAY\0"
    "LoggerStatusModes\0S_RECORDING_MODE\0"
    "S_RECORDING\0S_RECORDING_PAUSED\0"
    "S_RECORDING_STOPPED\0S_REPLAY_MODE\0"
    "S_REPLAY_LOADED\0S_REPLAY_UNLOADED\0"
    "S_REPLAYING\0S_REPLAY_PAUSED\0"
    "S_REPLAY_STOPPED\0SimulationStatusModes\0"
    "S_SIMULATION_START\0S_SIMULATION_PAUSED\0"
    "S_SIMULATION_STOP\0S_SIMULATION_NA"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Recorder[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       3,  170, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  109,    2, 0x06 /* Public */,
       4,    2,  112,    2, 0x06 /* Public */,
       7,    2,  117,    2, 0x06 /* Public */,
       9,    2,  122,    2, 0x06 /* Public */,
      10,    1,  127,    2, 0x06 /* Public */,
      12,    0,  130,    2, 0x06 /* Public */,
      13,    0,  131,    2, 0x06 /* Public */,
      14,    1,  132,    2, 0x06 /* Public */,
      15,    1,  135,    2, 0x06 /* Public */,
      16,    1,  138,    2, 0x06 /* Public */,
      18,    1,  141,    2, 0x06 /* Public */,
      21,    4,  144,    2, 0x06 /* Public */,
      26,    3,  153,    2, 0x06 /* Public */,
      27,    1,  160,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      28,    0,  163,    2, 0x08 /* Private */,
      29,    0,  164,    2, 0x0a /* Public */,
      30,    0,  165,    2, 0x0a /* Public */,
      31,    1,  166,    2, 0x0a /* Public */,
      34,    0,  169,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    5,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    5,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    5,    8,
    QMetaType::Void, QMetaType::LongLong,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void, QMetaType::LongLong,    6,
    QMetaType::Void, QMetaType::LongLong,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, QMetaType::QDateTime, QMetaType::LongLong, 0x80000000 | 19, 0x80000000 | 24,   22,   23,   20,   25,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 19, 0x80000000 | 24,   23,   20,   25,
    QMetaType::Void, QMetaType::LongLong,   23,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 32,   33,
    QMetaType::Void,

 // enums: name, alias, flags, count, data
      32,   32, 0x0,    2,  185,
      37,   37, 0x0,   10,  189,
      48,   48, 0x0,    4,  209,

 // enum data: key, value
      35, uint(Recorder::RECORDING),
      36, uint(Recorder::REPLAY),
      38, uint(Recorder::S_RECORDING_MODE),
      39, uint(Recorder::S_RECORDING),
      40, uint(Recorder::S_RECORDING_PAUSED),
      41, uint(Recorder::S_RECORDING_STOPPED),
      42, uint(Recorder::S_REPLAY_MODE),
      43, uint(Recorder::S_REPLAY_LOADED),
      44, uint(Recorder::S_REPLAY_UNLOADED),
      45, uint(Recorder::S_REPLAYING),
      46, uint(Recorder::S_REPLAY_PAUSED),
      47, uint(Recorder::S_REPLAY_STOPPED),
      49, uint(Recorder::S_SIMULATION_START),
      50, uint(Recorder::S_SIMULATION_PAUSED),
      51, uint(Recorder::S_SIMULATION_STOP),
      52, uint(Recorder::S_SIMULATION_NA),

       0        // eod
};

void Recorder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Recorder *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->replayFrame((*reinterpret_cast< QJsonObject(*)>(_a[1]))); break;
        case 1: _t->bookmarkAdded((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 2: _t->replayBookmark((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 3: _t->bookmarkReached((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 4: _t->setReplayDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 5: _t->recordingPaused(); break;
        case 6: _t->recordingResumed(); break;
        case 7: _t->frameLoaded((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 8: _t->replayFrameLoaded((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 9: _t->recordingTimeUpdated((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 10: _t->recorderInfoSend((*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[1]))); break;
        case 11: _t->recorderInfoSendOnce((*reinterpret_cast< QDateTime(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])),(*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[3])),(*reinterpret_cast< Recorder::SimulationStatusModes(*)>(_a[4]))); break;
        case 12: _t->recorderInfoSendUsual((*reinterpret_cast< qint64(*)>(_a[1])),(*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[2])),(*reinterpret_cast< Recorder::SimulationStatusModes(*)>(_a[3]))); break;
        case 13: _t->recorderInfoSendDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 14: _t->playNextFrame(); break;
        case 15: _t->togglePause(); break;
        case 16: _t->resetReplayState(); break;
        case 17: _t->loggerModeCheck((*reinterpret_cast< loggerModes(*)>(_a[1]))); break;
        case 18: _t->recorderStatus(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Recorder::*)(QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::replayFrame)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::bookmarkAdded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::replayBookmark)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::bookmarkReached)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::setReplayDuration)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Recorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recordingPaused)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Recorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recordingResumed)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::frameLoaded)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::replayFrameLoaded)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recordingTimeUpdated)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(Recorder::LoggerStatusModes );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recorderInfoSend)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(QDateTime , qint64 , Recorder::LoggerStatusModes , Recorder::SimulationStatusModes );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recorderInfoSendOnce)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(qint64 , Recorder::LoggerStatusModes , Recorder::SimulationStatusModes );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recorderInfoSendUsual)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (Recorder::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recorder::recorderInfoSendDuration)) {
                *result = 13;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Recorder::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Recorder.data,
    qt_meta_data_Recorder,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Recorder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Recorder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Recorder.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Recorder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void Recorder::replayFrame(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Recorder::bookmarkAdded(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Recorder::replayBookmark(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Recorder::bookmarkReached(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Recorder::setReplayDuration(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Recorder::recordingPaused()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Recorder::recordingResumed()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void Recorder::frameLoaded(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Recorder::replayFrameLoaded(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Recorder::recordingTimeUpdated(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void Recorder::recorderInfoSend(Recorder::LoggerStatusModes _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void Recorder::recorderInfoSendOnce(QDateTime _t1, qint64 _t2, Recorder::LoggerStatusModes _t3, Recorder::SimulationStatusModes _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void Recorder::recorderInfoSendUsual(qint64 _t1, Recorder::LoggerStatusModes _t2, Recorder::SimulationStatusModes _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void Recorder::recorderInfoSendDuration(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
struct qt_meta_stringdata_Recording_t {
    QByteArrayData data[15];
    char stringdata0[109];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Recording_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Recording_t qt_meta_stringdata_Recording = {
    {
QT_MOC_LITERAL(0, 0, 9), // "Recording"
QT_MOC_LITERAL(1, 10, 7), // "started"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 6), // "paused"
QT_MOC_LITERAL(4, 26, 7), // "stopped"
QT_MOC_LITERAL(5, 34, 5), // "start"
QT_MOC_LITERAL(6, 40, 6), // "resume"
QT_MOC_LITERAL(7, 47, 5), // "pause"
QT_MOC_LITERAL(8, 53, 4), // "stop"
QT_MOC_LITERAL(9, 58, 11), // "addBookmark"
QT_MOC_LITERAL(10, 70, 14), // "recordingModes"
QT_MOC_LITERAL(11, 85, 5), // "START"
QT_MOC_LITERAL(12, 91, 5), // "PAUSE"
QT_MOC_LITERAL(13, 97, 6), // "RESUME"
QT_MOC_LITERAL(14, 104, 4) // "STOP"

    },
    "Recording\0started\0\0paused\0stopped\0"
    "start\0resume\0pause\0stop\0addBookmark\0"
    "recordingModes\0START\0PAUSE\0RESUME\0"
    "STOP"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Recording[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       1,   64, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    1,   56,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   59,    2, 0x0a /* Public */,
       6,    0,   60,    2, 0x0a /* Public */,
       7,    0,   61,    2, 0x0a /* Public */,
       8,    0,   62,    2, 0x0a /* Public */,
       9,    0,   63,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // enums: name, alias, flags, count, data
      10,   10, 0x0,    4,   69,

 // enum data: key, value
      11, uint(Recording::START),
      12, uint(Recording::PAUSE),
      13, uint(Recording::RESUME),
      14, uint(Recording::STOP),

       0        // eod
};

void Recording::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Recording *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->started(); break;
        case 1: _t->paused(); break;
        case 2: _t->stopped((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 3: _t->start(); break;
        case 4: _t->resume(); break;
        case 5: _t->pause(); break;
        case 6: _t->stop(); break;
        case 7: _t->addBookmark(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Recording::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recording::started)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Recording::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recording::paused)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Recording::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Recording::stopped)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Recording::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Recording.data,
    qt_meta_data_Recording,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Recording::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Recording::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Recording.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Recording::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void Recording::started()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Recording::paused()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Recording::stopped(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
struct qt_meta_stringdata_Replay_t {
    QByteArrayData data[33];
    char stringdata0[332];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Replay_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Replay_t qt_meta_stringdata_Replay = {
    {
QT_MOC_LITERAL(0, 0, 6), // "Replay"
QT_MOC_LITERAL(1, 7, 7), // "started"
QT_MOC_LITERAL(2, 15, 0), // ""
QT_MOC_LITERAL(3, 16, 6), // "paused"
QT_MOC_LITERAL(4, 23, 7), // "stopped"
QT_MOC_LITERAL(5, 31, 17), // "setReplayDuration"
QT_MOC_LITERAL(6, 49, 8), // "duration"
QT_MOC_LITERAL(7, 58, 14), // "replayBookmark"
QT_MOC_LITERAL(8, 73, 4), // "note"
QT_MOC_LITERAL(9, 78, 9), // "timestamp"
QT_MOC_LITERAL(10, 88, 11), // "frameLoaded"
QT_MOC_LITERAL(11, 100, 5), // "frame"
QT_MOC_LITERAL(12, 106, 17), // "replayFrameLoaded"
QT_MOC_LITERAL(13, 124, 11), // "timestampMs"
QT_MOC_LITERAL(14, 136, 11), // "updateScene"
QT_MOC_LITERAL(15, 148, 9), // "deltaTime"
QT_MOC_LITERAL(16, 158, 5), // "start"
QT_MOC_LITERAL(17, 164, 5), // "pause"
QT_MOC_LITERAL(18, 170, 6), // "resume"
QT_MOC_LITERAL(19, 177, 4), // "stop"
QT_MOC_LITERAL(20, 182, 7), // "restart"
QT_MOC_LITERAL(21, 190, 10), // "fileLoaded"
QT_MOC_LITERAL(22, 201, 12), // "fileUnloaded"
QT_MOC_LITERAL(23, 214, 13), // "goToNextFrame"
QT_MOC_LITERAL(24, 228, 17), // "goToPreviousFrame"
QT_MOC_LITERAL(25, 246, 9), // "playAgain"
QT_MOC_LITERAL(26, 256, 24), // "startReplayFromTimestamp"
QT_MOC_LITERAL(27, 281, 14), // "bookmarkReplay"
QT_MOC_LITERAL(28, 296, 11), // "replayModes"
QT_MOC_LITERAL(29, 308, 5), // "Start"
QT_MOC_LITERAL(30, 314, 5), // "PAUSE"
QT_MOC_LITERAL(31, 320, 6), // "RESUME"
QT_MOC_LITERAL(32, 327, 4) // "STOP"

    },
    "Replay\0started\0\0paused\0stopped\0"
    "setReplayDuration\0duration\0replayBookmark\0"
    "note\0timestamp\0frameLoaded\0frame\0"
    "replayFrameLoaded\0timestampMs\0updateScene\0"
    "deltaTime\0start\0pause\0resume\0stop\0"
    "restart\0fileLoaded\0fileUnloaded\0"
    "goToNextFrame\0goToPreviousFrame\0"
    "playAgain\0startReplayFromTimestamp\0"
    "bookmarkReplay\0replayModes\0Start\0PAUSE\0"
    "RESUME\0STOP"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Replay[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       1,  156, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  114,    2, 0x06 /* Public */,
       3,    0,  117,    2, 0x06 /* Public */,
       4,    1,  118,    2, 0x06 /* Public */,
       5,    1,  121,    2, 0x06 /* Public */,
       7,    2,  124,    2, 0x06 /* Public */,
      10,    1,  129,    2, 0x06 /* Public */,
      12,    1,  132,    2, 0x06 /* Public */,
      14,    1,  135,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      16,    0,  138,    2, 0x0a /* Public */,
      17,    0,  139,    2, 0x0a /* Public */,
      18,    0,  140,    2, 0x0a /* Public */,
      19,    0,  141,    2, 0x0a /* Public */,
      20,    0,  142,    2, 0x0a /* Public */,
      21,    0,  143,    2, 0x0a /* Public */,
      22,    0,  144,    2, 0x0a /* Public */,
      23,    0,  145,    2, 0x0a /* Public */,
      24,    0,  146,    2, 0x0a /* Public */,
      25,    0,  147,    2, 0x0a /* Public */,
      26,    1,  148,    2, 0x0a /* Public */,
      27,    2,  151,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QDateTime,    2,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,    2,
    QMetaType::Void, QMetaType::LongLong,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    8,    9,
    QMetaType::Void, QMetaType::QJsonObject,   11,
    QMetaType::Void, QMetaType::LongLong,   13,
    QMetaType::Void, QMetaType::Float,   15,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,    8,   13,

 // enums: name, alias, flags, count, data
      28,   28, 0x0,    4,  161,

 // enum data: key, value
      29, uint(Replay::Start),
      30, uint(Replay::PAUSE),
      31, uint(Replay::RESUME),
      32, uint(Replay::STOP),

       0        // eod
};

void Replay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Replay *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->started((*reinterpret_cast< QDateTime(*)>(_a[1]))); break;
        case 1: _t->paused(); break;
        case 2: _t->stopped((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 3: _t->setReplayDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 4: _t->replayBookmark((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 5: _t->frameLoaded((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 6: _t->replayFrameLoaded((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 7: _t->updateScene((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 8: _t->start(); break;
        case 9: _t->pause(); break;
        case 10: _t->resume(); break;
        case 11: _t->stop(); break;
        case 12: _t->restart(); break;
        case 13: _t->fileLoaded(); break;
        case 14: _t->fileUnloaded(); break;
        case 15: _t->goToNextFrame(); break;
        case 16: _t->goToPreviousFrame(); break;
        case 17: _t->playAgain(); break;
        case 18: _t->startReplayFromTimestamp((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 19: _t->bookmarkReplay((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Replay::*)(QDateTime );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::started)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Replay::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::paused)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Replay::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::stopped)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Replay::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::setReplayDuration)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Replay::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::replayBookmark)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Replay::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::frameLoaded)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Replay::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::replayFrameLoaded)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Replay::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Replay::updateScene)) {
                *result = 7;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Replay::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Replay.data,
    qt_meta_data_Replay,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Replay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Replay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Replay.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Replay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void Replay::started(QDateTime _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Replay::paused()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Replay::stopped(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Replay::setReplayDuration(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Replay::replayBookmark(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Replay::frameLoaded(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Replay::replayFrameLoaded(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Replay::updateScene(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
