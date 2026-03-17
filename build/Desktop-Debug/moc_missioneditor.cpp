/****************************************************************************
** Meta object code from reading C++ file 'missioneditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Editors/missioneditor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'missioneditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MissionEditor_t {
    QByteArrayData data[18];
    char stringdata0[265];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MissionEditor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MissionEditor_t qt_meta_stringdata_MissionEditor = {
    {
QT_MOC_LITERAL(0, 0, 13), // "MissionEditor"
QT_MOC_LITERAL(1, 14, 21), // "unsavedChangesChanged"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 10), // "hasChanges"
QT_MOC_LITERAL(4, 48, 9), // "Activated"
QT_MOC_LITERAL(5, 58, 15), // "hierarchyLoaded"
QT_MOC_LITERAL(6, 74, 13), // "hierarchyData"
QT_MOC_LITERAL(7, 88, 15), // "showProfileInfo"
QT_MOC_LITERAL(8, 104, 21), // "showApplicationDialog"
QT_MOC_LITERAL(9, 126, 18), // "showFeedbackWindow"
QT_MOC_LITERAL(10, 145, 23), // "onDockVisibilityChanged"
QT_MOC_LITERAL(11, 169, 7), // "visible"
QT_MOC_LITERAL(12, 177, 11), // "resetLayout"
QT_MOC_LITERAL(13, 189, 24), // "onRecentProjectTriggered"
QT_MOC_LITERAL(14, 214, 17), // "loadRecentProject"
QT_MOC_LITERAL(15, 232, 8), // "filePath"
QT_MOC_LITERAL(16, 241, 18), // "onTreeItemSelected"
QT_MOC_LITERAL(17, 260, 4) // "data"

    },
    "MissionEditor\0unsavedChangesChanged\0"
    "\0hasChanges\0Activated\0hierarchyLoaded\0"
    "hierarchyData\0showProfileInfo\0"
    "showApplicationDialog\0showFeedbackWindow\0"
    "onDockVisibilityChanged\0visible\0"
    "resetLayout\0onRecentProjectTriggered\0"
    "loadRecentProject\0filePath\0"
    "onTreeItemSelected\0data"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MissionEditor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    0,   72,    2, 0x06 /* Public */,
       5,    1,   73,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   76,    2, 0x0a /* Public */,
       8,    0,   77,    2, 0x0a /* Public */,
       9,    0,   78,    2, 0x08 /* Private */,
      10,    1,   79,    2, 0x08 /* Private */,
      12,    0,   82,    2, 0x08 /* Private */,
      13,    0,   83,    2, 0x08 /* Private */,
      14,    1,   84,    2, 0x08 /* Private */,
      16,    1,   87,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::QVariantMap,   17,

       0        // eod
};

void MissionEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MissionEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->unsavedChangesChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->Activated(); break;
        case 2: _t->hierarchyLoaded((*reinterpret_cast< QJsonObject(*)>(_a[1]))); break;
        case 3: _t->showProfileInfo(); break;
        case 4: _t->showApplicationDialog(); break;
        case 5: _t->showFeedbackWindow(); break;
        case 6: _t->onDockVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->resetLayout(); break;
        case 8: _t->onRecentProjectTriggered(); break;
        case 9: _t->loadRecentProject((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->onTreeItemSelected((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MissionEditor::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MissionEditor::unsavedChangesChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MissionEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MissionEditor::Activated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MissionEditor::*)(QJsonObject );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MissionEditor::hierarchyLoaded)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MissionEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MissionEditor.data,
    qt_meta_data_MissionEditor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MissionEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MissionEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MissionEditor.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MissionEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void MissionEditor::unsavedChangesChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MissionEditor::Activated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MissionEditor::hierarchyLoaded(QJsonObject _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
