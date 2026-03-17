/****************************************************************************
** Meta object code from reading C++ file 'taskgroup.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../mission/taskgroup.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'taskgroup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TaskGroup_t {
    QByteArrayData data[17];
    char stringdata0[163];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TaskGroup_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TaskGroup_t qt_meta_stringdata_TaskGroup = {
    {
QT_MOC_LITERAL(0, 0, 9), // "TaskGroup"
QT_MOC_LITERAL(1, 10, 6), // "goHome"
QT_MOC_LITERAL(2, 17, 0), // ""
QT_MOC_LITERAL(3, 18, 15), // "activateSensors"
QT_MOC_LITERAL(4, 34, 17), // "deactivateSensors"
QT_MOC_LITERAL(5, 52, 13), // "makeFormation"
QT_MOC_LITERAL(6, 66, 11), // "deformation"
QT_MOC_LITERAL(7, 78, 3), // "run"
QT_MOC_LITERAL(8, 82, 5), // "reset"
QT_MOC_LITERAL(9, 88, 5), // "pause"
QT_MOC_LITERAL(10, 94, 18), // "on_addtask_clicked"
QT_MOC_LITERAL(11, 113, 13), // "on_itemRemove"
QT_MOC_LITERAL(12, 127, 5), // "Task*"
QT_MOC_LITERAL(13, 133, 4), // "task"
QT_MOC_LITERAL(14, 138, 14), // "on_run_clicked"
QT_MOC_LITERAL(15, 153, 7), // "execute"
QT_MOC_LITERAL(16, 161, 1) // "i"

    },
    "TaskGroup\0goHome\0\0activateSensors\0"
    "deactivateSensors\0makeFormation\0"
    "deformation\0run\0reset\0pause\0"
    "on_addtask_clicked\0on_itemRemove\0Task*\0"
    "task\0on_run_clicked\0execute\0i"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TaskGroup[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x06 /* Public */,
       3,    0,   75,    2, 0x06 /* Public */,
       4,    0,   76,    2, 0x06 /* Public */,
       5,    0,   77,    2, 0x06 /* Public */,
       6,    0,   78,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   79,    2, 0x0a /* Public */,
       8,    0,   80,    2, 0x0a /* Public */,
       9,    0,   81,    2, 0x0a /* Public */,
      10,    0,   82,    2, 0x08 /* Private */,
      11,    1,   83,    2, 0x08 /* Private */,
      14,    0,   86,    2, 0x08 /* Private */,
      15,    1,   87,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   16,

       0        // eod
};

void TaskGroup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TaskGroup *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->goHome(); break;
        case 1: _t->activateSensors(); break;
        case 2: _t->deactivateSensors(); break;
        case 3: _t->makeFormation(); break;
        case 4: _t->deformation(); break;
        case 5: _t->run(); break;
        case 6: _t->reset(); break;
        case 7: _t->pause(); break;
        case 8: _t->on_addtask_clicked(); break;
        case 9: _t->on_itemRemove((*reinterpret_cast< Task*(*)>(_a[1]))); break;
        case 10: _t->on_run_clicked(); break;
        case 11: _t->execute((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Task* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TaskGroup::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TaskGroup::goHome)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TaskGroup::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TaskGroup::activateSensors)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TaskGroup::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TaskGroup::deactivateSensors)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TaskGroup::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TaskGroup::makeFormation)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TaskGroup::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TaskGroup::deformation)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TaskGroup::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TaskGroup.data,
    qt_meta_data_TaskGroup,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TaskGroup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TaskGroup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TaskGroup.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TaskGroup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void TaskGroup::goHome()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TaskGroup::activateSensors()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TaskGroup::deactivateSensors()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void TaskGroup::makeFormation()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void TaskGroup::deformation()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
