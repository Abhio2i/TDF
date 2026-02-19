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
    QByteArrayData data[64];
    char stringdata0[1029];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LoggerDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LoggerDialog_t qt_meta_stringdata_LoggerDialog = {
    {
QT_MOC_LITERAL(0, 0, 12), // "LoggerDialog"
QT_MOC_LITERAL(1, 13, 14), // "loggerModeSend"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 21), // "Recorder::loggerModes"
QT_MOC_LITERAL(4, 51, 12), // "modeOfLogger"
QT_MOC_LITERAL(5, 64, 14), // "recordingStart"
QT_MOC_LITERAL(6, 79, 14), // "recordingPause"
QT_MOC_LITERAL(7, 94, 15), // "recordingResume"
QT_MOC_LITERAL(8, 110, 13), // "recordingStop"
QT_MOC_LITERAL(9, 124, 11), // "replayStart"
QT_MOC_LITERAL(10, 136, 11), // "replayPause"
QT_MOC_LITERAL(11, 148, 12), // "replayResume"
QT_MOC_LITERAL(12, 161, 10), // "replayStop"
QT_MOC_LITERAL(13, 172, 13), // "replayRestart"
QT_MOC_LITERAL(14, 186, 16), // "replayFileLoaded"
QT_MOC_LITERAL(15, 203, 18), // "replayFileUnloaded"
QT_MOC_LITERAL(16, 222, 14), // "startRecording"
QT_MOC_LITERAL(17, 237, 14), // "pauseRecording"
QT_MOC_LITERAL(18, 252, 13), // "stopRecording"
QT_MOC_LITERAL(19, 266, 13), // "saveRecording"
QT_MOC_LITERAL(20, 280, 8), // "filePath"
QT_MOC_LITERAL(21, 289, 13), // "loadRecording"
QT_MOC_LITERAL(22, 303, 19), // "saveRecordingToFile"
QT_MOC_LITERAL(23, 323, 10), // "recordings"
QT_MOC_LITERAL(24, 334, 22), // "saveRecordingRequested"
QT_MOC_LITERAL(25, 357, 15), // "replayRecording"
QT_MOC_LITERAL(26, 373, 11), // "startReplay"
QT_MOC_LITERAL(27, 385, 11), // "pauseReplay"
QT_MOC_LITERAL(28, 397, 12), // "resumeReplay"
QT_MOC_LITERAL(29, 410, 18), // "eventTypesSelected"
QT_MOC_LITERAL(30, 429, 10), // "eventTypes"
QT_MOC_LITERAL(31, 440, 13), // "bookmarkAdded"
QT_MOC_LITERAL(32, 454, 12), // "bookmarkNote"
QT_MOC_LITERAL(33, 467, 16), // "timestampToggled"
QT_MOC_LITERAL(34, 484, 7), // "enabled"
QT_MOC_LITERAL(35, 492, 15), // "bookmarkClicked"
QT_MOC_LITERAL(36, 508, 4), // "note"
QT_MOC_LITERAL(37, 513, 11), // "timestampMs"
QT_MOC_LITERAL(38, 525, 21), // "bookmarkButtonClicked"
QT_MOC_LITERAL(39, 547, 17), // "toggleReplayPause"
QT_MOC_LITERAL(40, 565, 13), // "previousFrame"
QT_MOC_LITERAL(41, 579, 9), // "nextFrame"
QT_MOC_LITERAL(42, 589, 14), // "pressPlayAgain"
QT_MOC_LITERAL(43, 604, 18), // "requestReplayReset"
QT_MOC_LITERAL(44, 623, 19), // "recorderInfoReceive"
QT_MOC_LITERAL(45, 643, 27), // "Recorder::LoggerStatusModes"
QT_MOC_LITERAL(46, 671, 14), // "r_loggerStatus"
QT_MOC_LITERAL(47, 686, 23), // "recorderInfoReceiveOnce"
QT_MOC_LITERAL(48, 710, 20), // "r_recordingStartTime"
QT_MOC_LITERAL(49, 731, 10), // "r_duration"
QT_MOC_LITERAL(50, 742, 31), // "Recorder::SimulationStatusModes"
QT_MOC_LITERAL(51, 774, 18), // "r_simulationStatus"
QT_MOC_LITERAL(52, 793, 24), // "recorderInfoReceiveUsual"
QT_MOC_LITERAL(53, 818, 27), // "recorderInfoReceiveDuration"
QT_MOC_LITERAL(54, 846, 20), // "showBookmarkOnReplay"
QT_MOC_LITERAL(55, 867, 9), // "timestamp"
QT_MOC_LITERAL(56, 877, 22), // "onReplayBookmarkLoaded"
QT_MOC_LITERAL(57, 900, 19), // "setTimelineDuration"
QT_MOC_LITERAL(58, 920, 8), // "duration"
QT_MOC_LITERAL(59, 929, 18), // "replayFromBookmark"
QT_MOC_LITERAL(60, 948, 20), // "updateReplayProgress"
QT_MOC_LITERAL(61, 969, 21), // "switchToRecordingMode"
QT_MOC_LITERAL(62, 991, 18), // "switchToReplayMode"
QT_MOC_LITERAL(63, 1010, 18) // "showBookmarkDialog"

    },
    "LoggerDialog\0loggerModeSend\0\0"
    "Recorder::loggerModes\0modeOfLogger\0"
    "recordingStart\0recordingPause\0"
    "recordingResume\0recordingStop\0replayStart\0"
    "replayPause\0replayResume\0replayStop\0"
    "replayRestart\0replayFileLoaded\0"
    "replayFileUnloaded\0startRecording\0"
    "pauseRecording\0stopRecording\0saveRecording\0"
    "filePath\0loadRecording\0saveRecordingToFile\0"
    "recordings\0saveRecordingRequested\0"
    "replayRecording\0startReplay\0pauseReplay\0"
    "resumeReplay\0eventTypesSelected\0"
    "eventTypes\0bookmarkAdded\0bookmarkNote\0"
    "timestampToggled\0enabled\0bookmarkClicked\0"
    "note\0timestampMs\0bookmarkButtonClicked\0"
    "toggleReplayPause\0previousFrame\0"
    "nextFrame\0pressPlayAgain\0requestReplayReset\0"
    "recorderInfoReceive\0Recorder::LoggerStatusModes\0"
    "r_loggerStatus\0recorderInfoReceiveOnce\0"
    "r_recordingStartTime\0r_duration\0"
    "Recorder::SimulationStatusModes\0"
    "r_simulationStatus\0recorderInfoReceiveUsual\0"
    "recorderInfoReceiveDuration\0"
    "showBookmarkOnReplay\0timestamp\0"
    "onReplayBookmarkLoaded\0setTimelineDuration\0"
    "duration\0replayFromBookmark\0"
    "updateReplayProgress\0switchToRecordingMode\0"
    "switchToReplayMode\0showBookmarkDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LoggerDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      45,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      33,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  239,    2, 0x06 /* Public */,
       5,    0,  242,    2, 0x06 /* Public */,
       6,    0,  243,    2, 0x06 /* Public */,
       7,    0,  244,    2, 0x06 /* Public */,
       8,    0,  245,    2, 0x06 /* Public */,
       9,    0,  246,    2, 0x06 /* Public */,
      10,    0,  247,    2, 0x06 /* Public */,
      11,    0,  248,    2, 0x06 /* Public */,
      12,    0,  249,    2, 0x06 /* Public */,
      13,    0,  250,    2, 0x06 /* Public */,
      14,    0,  251,    2, 0x06 /* Public */,
      15,    0,  252,    2, 0x06 /* Public */,
      16,    0,  253,    2, 0x06 /* Public */,
      17,    0,  254,    2, 0x06 /* Public */,
      18,    0,  255,    2, 0x06 /* Public */,
      19,    1,  256,    2, 0x06 /* Public */,
      21,    1,  259,    2, 0x06 /* Public */,
      22,    1,  262,    2, 0x06 /* Public */,
      24,    0,  265,    2, 0x06 /* Public */,
      25,    1,  266,    2, 0x06 /* Public */,
      26,    0,  269,    2, 0x06 /* Public */,
      27,    0,  270,    2, 0x06 /* Public */,
      28,    0,  271,    2, 0x06 /* Public */,
      29,    1,  272,    2, 0x06 /* Public */,
      31,    1,  275,    2, 0x06 /* Public */,
      33,    1,  278,    2, 0x06 /* Public */,
      35,    2,  281,    2, 0x06 /* Public */,
      38,    2,  286,    2, 0x06 /* Public */,
      39,    0,  291,    2, 0x06 /* Public */,
      40,    0,  292,    2, 0x06 /* Public */,
      41,    0,  293,    2, 0x06 /* Public */,
      42,    0,  294,    2, 0x06 /* Public */,
      43,    0,  295,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      44,    1,  296,    2, 0x0a /* Public */,
      47,    4,  299,    2, 0x0a /* Public */,
      52,    3,  308,    2, 0x0a /* Public */,
      53,    1,  315,    2, 0x0a /* Public */,
      54,    2,  318,    2, 0x0a /* Public */,
      56,    2,  323,    2, 0x0a /* Public */,
      57,    1,  328,    2, 0x0a /* Public */,
      59,    2,  331,    2, 0x0a /* Public */,
      60,    1,  336,    2, 0x0a /* Public */,
      61,    0,  339,    2, 0x0a /* Public */,
      62,    0,  340,    2, 0x0a /* Public */,
      63,    0,  341,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, QMetaType::QJsonObject,   23,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QStringList,   30,
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void, QMetaType::Bool,   34,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   36,   37,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   36,   37,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 45,   46,
    QMetaType::Void, QMetaType::QDateTime, QMetaType::LongLong, 0x80000000 | 45, 0x80000000 | 50,   48,   49,   46,   51,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 45, 0x80000000 | 50,   49,   46,   51,
    QMetaType::Void, QMetaType::LongLong,   49,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   36,   55,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   36,   55,
    QMetaType::Void, QMetaType::LongLong,   58,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   36,   55,
    QMetaType::Void, QMetaType::LongLong,   55,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void LoggerDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LoggerDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->loggerModeSend((*reinterpret_cast< Recorder::loggerModes(*)>(_a[1]))); break;
        case 1: _t->recordingStart(); break;
        case 2: _t->recordingPause(); break;
        case 3: _t->recordingResume(); break;
        case 4: _t->recordingStop(); break;
        case 5: _t->replayStart(); break;
        case 6: _t->replayPause(); break;
        case 7: _t->replayResume(); break;
        case 8: _t->replayStop(); break;
        case 9: _t->replayRestart(); break;
        case 10: _t->replayFileLoaded(); break;
        case 11: _t->replayFileUnloaded(); break;
        case 12: _t->startRecording(); break;
        case 13: _t->pauseRecording(); break;
        case 14: _t->stopRecording(); break;
        case 15: _t->saveRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: _t->loadRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->saveRecordingToFile((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 18: _t->saveRecordingRequested(); break;
        case 19: _t->replayRecording((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->startReplay(); break;
        case 21: _t->pauseReplay(); break;
        case 22: _t->resumeReplay(); break;
        case 23: _t->eventTypesSelected((*reinterpret_cast< QStringList(*)>(_a[1]))); break;
        case 24: _t->bookmarkAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 25: _t->timestampToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 26: _t->bookmarkClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 27: _t->bookmarkButtonClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 28: _t->toggleReplayPause(); break;
        case 29: _t->previousFrame(); break;
        case 30: _t->nextFrame(); break;
        case 31: _t->pressPlayAgain(); break;
        case 32: _t->requestReplayReset(); break;
        case 33: _t->recorderInfoReceive((*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[1]))); break;
        case 34: _t->recorderInfoReceiveOnce((*reinterpret_cast< QDateTime(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2])),(*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[3])),(*reinterpret_cast< Recorder::SimulationStatusModes(*)>(_a[4]))); break;
        case 35: _t->recorderInfoReceiveUsual((*reinterpret_cast< qint64(*)>(_a[1])),(*reinterpret_cast< Recorder::LoggerStatusModes(*)>(_a[2])),(*reinterpret_cast< Recorder::SimulationStatusModes(*)>(_a[3]))); break;
        case 36: _t->recorderInfoReceiveDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 37: _t->showBookmarkOnReplay((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 38: _t->onReplayBookmarkLoaded((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 39: _t->setTimelineDuration((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 40: _t->replayFromBookmark((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< qint64(*)>(_a[2]))); break;
        case 41: _t->updateReplayProgress((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 42: _t->switchToRecordingMode(); break;
        case 43: _t->switchToReplayMode(); break;
        case 44: _t->showBookmarkDialog(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LoggerDialog::*)(Recorder::loggerModes );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::loggerModeSend)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::recordingStart)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::recordingPause)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::recordingResume)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::recordingStop)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayStart)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayPause)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayResume)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayStop)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayRestart)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayFileLoaded)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayFileUnloaded)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::startRecording)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::pauseRecording)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::stopRecording)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecording)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::loadRecording)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecordingToFile)) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::saveRecordingRequested)) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::replayRecording)) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::startReplay)) {
                *result = 20;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::pauseReplay)) {
                *result = 21;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::resumeReplay)) {
                *result = 22;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(QStringList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::eventTypesSelected)) {
                *result = 23;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkAdded)) {
                *result = 24;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::timestampToggled)) {
                *result = 25;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkClicked)) {
                *result = 26;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)(const QString & , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::bookmarkButtonClicked)) {
                *result = 27;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::toggleReplayPause)) {
                *result = 28;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::previousFrame)) {
                *result = 29;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::nextFrame)) {
                *result = 30;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::pressPlayAgain)) {
                *result = 31;
                return;
            }
        }
        {
            using _t = void (LoggerDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LoggerDialog::requestReplayReset)) {
                *result = 32;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject LoggerDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
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
    return QMainWindow::qt_metacast(_clname);
}

int LoggerDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 45)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 45;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 45)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 45;
    }
    return _id;
}

// SIGNAL 0
void LoggerDialog::loggerModeSend(Recorder::loggerModes _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LoggerDialog::recordingStart()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LoggerDialog::recordingPause()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void LoggerDialog::recordingResume()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void LoggerDialog::recordingStop()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void LoggerDialog::replayStart()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void LoggerDialog::replayPause()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void LoggerDialog::replayResume()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void LoggerDialog::replayStop()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void LoggerDialog::replayRestart()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void LoggerDialog::replayFileLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void LoggerDialog::replayFileUnloaded()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void LoggerDialog::startRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void LoggerDialog::pauseRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void LoggerDialog::stopRecording()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void LoggerDialog::saveRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void LoggerDialog::loadRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void LoggerDialog::saveRecordingToFile(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void LoggerDialog::saveRecordingRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void LoggerDialog::replayRecording(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void LoggerDialog::startReplay()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void LoggerDialog::pauseReplay()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void LoggerDialog::resumeReplay()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void LoggerDialog::eventTypesSelected(QStringList _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 23, _a);
}

// SIGNAL 24
void LoggerDialog::bookmarkAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 24, _a);
}

// SIGNAL 25
void LoggerDialog::timestampToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 25, _a);
}

// SIGNAL 26
void LoggerDialog::bookmarkClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 26, _a);
}

// SIGNAL 27
void LoggerDialog::bookmarkButtonClicked(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 27, _a);
}

// SIGNAL 28
void LoggerDialog::toggleReplayPause()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void LoggerDialog::previousFrame()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}

// SIGNAL 30
void LoggerDialog::nextFrame()
{
    QMetaObject::activate(this, &staticMetaObject, 30, nullptr);
}

// SIGNAL 31
void LoggerDialog::pressPlayAgain()
{
    QMetaObject::activate(this, &staticMetaObject, 31, nullptr);
}

// SIGNAL 32
void LoggerDialog::requestReplayReset()
{
    QMetaObject::activate(this, &staticMetaObject, 32, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
