/****************************************************************************
** Meta object code from reading C++ file 'scene3dwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/scene3dwidget/scene3dwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scene3dwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Scene3DWidget_t {
    QByteArrayData data[13];
    char stringdata0[161];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Scene3DWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Scene3DWidget_t qt_meta_stringdata_Scene3DWidget = {
    {
QT_MOC_LITERAL(0, 0, 13), // "Scene3DWidget"
QT_MOC_LITERAL(1, 14, 20), // "selectEntityByCursor"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 2), // "ID"
QT_MOC_LITERAL(4, 39, 11), // "focusEntity"
QT_MOC_LITERAL(5, 51, 14), // "updateEntities"
QT_MOC_LITERAL(6, 66, 9), // "addEntity"
QT_MOC_LITERAL(7, 76, 11), // "Mesh3dEntry"
QT_MOC_LITERAL(8, 88, 12), // "removeEntity"
QT_MOC_LITERAL(9, 101, 10), // "MoveEntity"
QT_MOC_LITERAL(10, 112, 14), // "selectedEntity"
QT_MOC_LITERAL(11, 127, 12), // "updateCamera"
QT_MOC_LITERAL(12, 140, 20) // "updateCameraRotation"

    },
    "Scene3DWidget\0selectEntityByCursor\0\0"
    "ID\0focusEntity\0updateEntities\0addEntity\0"
    "Mesh3dEntry\0removeEntity\0MoveEntity\0"
    "selectedEntity\0updateCamera\0"
    "updateCameraRotation"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Scene3DWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   62,    2, 0x0a /* Public */,
       5,    0,   65,    2, 0x0a /* Public */,
       6,    2,   66,    2, 0x0a /* Public */,
       8,    1,   71,    2, 0x0a /* Public */,
       9,    1,   74,    2, 0x0a /* Public */,
      10,    1,   77,    2, 0x0a /* Public */,
      11,    0,   80,    2, 0x0a /* Public */,
      12,    0,   81,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 7,    3,    2,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Scene3DWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Scene3DWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selectEntityByCursor((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->focusEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->updateEntities(); break;
        case 3: _t->addEntity((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< Mesh3dEntry(*)>(_a[2]))); break;
        case 4: _t->removeEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->MoveEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 6: _t->selectedEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->updateCamera(); break;
        case 8: _t->updateCameraRotation(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Scene3DWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Scene3DWidget::selectEntityByCursor)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Scene3DWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_Scene3DWidget.data,
    qt_meta_data_Scene3DWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Scene3DWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Scene3DWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Scene3DWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Scene3DWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void Scene3DWidget::selectEntityByCursor(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
