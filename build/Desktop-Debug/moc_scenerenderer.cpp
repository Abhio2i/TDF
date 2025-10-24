/****************************************************************************
** Meta object code from reading C++ file 'scenerenderer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/Render/scenerenderer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scenerenderer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SceneRenderer_t {
    QByteArrayData data[15];
    char stringdata0[139];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SceneRenderer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SceneRenderer_t qt_meta_stringdata_SceneRenderer = {
    {
QT_MOC_LITERAL(0, 0, 13), // "SceneRenderer"
QT_MOC_LITERAL(1, 14, 6), // "Render"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 9), // "deltaTime"
QT_MOC_LITERAL(4, 32, 16), // "DeltaTimeUpdated"
QT_MOC_LITERAL(5, 49, 7), // "addMesh"
QT_MOC_LITERAL(6, 57, 2), // "ID"
QT_MOC_LITERAL(7, 60, 8), // "MeshData"
QT_MOC_LITERAL(8, 69, 8), // "meshData"
QT_MOC_LITERAL(9, 78, 10), // "removeMesh"
QT_MOC_LITERAL(10, 89, 11), // "entityAdded"
QT_MOC_LITERAL(11, 101, 8), // "parentID"
QT_MOC_LITERAL(12, 110, 7), // "Entity*"
QT_MOC_LITERAL(13, 118, 6), // "entity"
QT_MOC_LITERAL(14, 125, 13) // "entityRemoved"

    },
    "SceneRenderer\0Render\0\0deltaTime\0"
    "DeltaTimeUpdated\0addMesh\0ID\0MeshData\0"
    "meshData\0removeMesh\0entityAdded\0"
    "parentID\0Entity*\0entity\0entityRemoved"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SceneRenderer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       4,    1,   47,    2, 0x06 /* Public */,
       5,    2,   50,    2, 0x06 /* Public */,
       9,    1,   55,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    2,   58,    2, 0x0a /* Public */,
      14,    1,   63,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Float,    3,
    QMetaType::Void, QMetaType::Float,    3,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 7,    6,    8,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 12,   11,   13,
    QMetaType::Void, QMetaType::QString,    6,

       0        // eod
};

void SceneRenderer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SceneRenderer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Render((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 1: _t->DeltaTimeUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 2: _t->addMesh((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< MeshData(*)>(_a[2]))); break;
        case 3: _t->removeMesh((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->entityAdded((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< Entity*(*)>(_a[2]))); break;
        case 5: _t->entityRemoved((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entity* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SceneRenderer::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneRenderer::Render)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SceneRenderer::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneRenderer::DeltaTimeUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SceneRenderer::*)(QString , MeshData );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneRenderer::addMesh)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SceneRenderer::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneRenderer::removeMesh)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SceneRenderer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SceneRenderer.data,
    qt_meta_data_SceneRenderer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SceneRenderer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SceneRenderer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SceneRenderer.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SceneRenderer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void SceneRenderer::Render(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SceneRenderer::DeltaTimeUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SceneRenderer::addMesh(QString _t1, MeshData _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SceneRenderer::removeMesh(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
