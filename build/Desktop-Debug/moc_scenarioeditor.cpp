/****************************************************************************
** Meta object code from reading C++ file 'scenarioeditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Editors/scenarioeditor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scenarioeditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScenarioEditor_t {
    QByteArrayData data[21];
    char stringdata0[339];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScenarioEditor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScenarioEditor_t qt_meta_stringdata_ScenarioEditor = {
    {
QT_MOC_LITERAL(0, 0, 14), // "ScenarioEditor"
QT_MOC_LITERAL(1, 15, 21), // "unsavedChangesChanged"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 10), // "hasChanges"
QT_MOC_LITERAL(4, 49, 9), // "Activated"
QT_MOC_LITERAL(5, 59, 15), // "showProfileInfo"
QT_MOC_LITERAL(6, 75, 21), // "showApplicationDialog"
QT_MOC_LITERAL(7, 97, 14), // "onItemSelected"
QT_MOC_LITERAL(8, 112, 4), // "data"
QT_MOC_LITERAL(9, 117, 21), // "onLibraryItemSelected"
QT_MOC_LITERAL(10, 139, 15), // "addInspectorTab"
QT_MOC_LITERAL(11, 155, 18), // "showFeedbackWindow"
QT_MOC_LITERAL(12, 174, 18), // "markUnsavedChanges"
QT_MOC_LITERAL(13, 193, 23), // "onDockVisibilityChanged"
QT_MOC_LITERAL(14, 217, 7), // "visible"
QT_MOC_LITERAL(15, 225, 11), // "resetLayout"
QT_MOC_LITERAL(16, 237, 24), // "onRecentProjectTriggered"
QT_MOC_LITERAL(17, 262, 17), // "loadRecentProject"
QT_MOC_LITERAL(18, 280, 8), // "filePath"
QT_MOC_LITERAL(19, 289, 24), // "onRecentLibraryTriggered"
QT_MOC_LITERAL(20, 314, 24) // "onRunScriptFileRequested"

    },
    "ScenarioEditor\0unsavedChangesChanged\0"
    "\0hasChanges\0Activated\0showProfileInfo\0"
    "showApplicationDialog\0onItemSelected\0"
    "data\0onLibraryItemSelected\0addInspectorTab\0"
    "showFeedbackWindow\0markUnsavedChanges\0"
    "onDockVisibilityChanged\0visible\0"
    "resetLayout\0onRecentProjectTriggered\0"
    "loadRecentProject\0filePath\0"
    "onRecentLibraryTriggered\0"
    "onRunScriptFileRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScenarioEditor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x06 /* Public */,
       4,    0,   92,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   93,    2, 0x0a /* Public */,
       6,    0,   94,    2, 0x0a /* Public */,
       7,    1,   95,    2, 0x08 /* Private */,
       9,    1,   98,    2, 0x08 /* Private */,
      10,    0,  101,    2, 0x08 /* Private */,
      11,    0,  102,    2, 0x08 /* Private */,
      12,    0,  103,    2, 0x08 /* Private */,
      13,    1,  104,    2, 0x08 /* Private */,
      15,    0,  107,    2, 0x08 /* Private */,
      16,    0,  108,    2, 0x08 /* Private */,
      17,    1,  109,    2, 0x08 /* Private */,
      19,    0,  112,    2, 0x08 /* Private */,
      20,    1,  113,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QVariantMap,    8,
    QMetaType::Void, QMetaType::QVariantMap,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,

       0        // eod
};

void ScenarioEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScenarioEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->unsavedChangesChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->Activated(); break;
        case 2: _t->showProfileInfo(); break;
        case 3: _t->showApplicationDialog(); break;
        case 4: _t->onItemSelected((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 5: _t->onLibraryItemSelected((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 6: _t->addInspectorTab(); break;
        case 7: _t->showFeedbackWindow(); break;
        case 8: _t->markUnsavedChanges(); break;
        case 9: _t->onDockVisibilityChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->resetLayout(); break;
        case 11: _t->onRecentProjectTriggered(); break;
        case 12: _t->loadRecentProject((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->onRecentLibraryTriggered(); break;
        case 14: _t->onRunScriptFileRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScenarioEditor::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScenarioEditor::unsavedChangesChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScenarioEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScenarioEditor::Activated)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ScenarioEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ScenarioEditor.data,
    qt_meta_data_ScenarioEditor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScenarioEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScenarioEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScenarioEditor.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int ScenarioEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
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
void ScenarioEditor::unsavedChangesChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScenarioEditor::Activated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
