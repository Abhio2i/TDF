/****************************************************************************
** Meta object code from reading C++ file 'layerpanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Tacticaldisplay/Gis/layerpanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'layerpanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LayerPanel_t {
    QByteArrayData data[32];
    char stringdata0[417];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LayerPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LayerPanel_t qt_meta_stringdata_LayerPanel = {
    {
QT_MOC_LITERAL(0, 0, 10), // "LayerPanel"
QT_MOC_LITERAL(1, 11, 18), // "activeLayerChanged"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 12), // "newLayerName"
QT_MOC_LITERAL(4, 44, 10), // "layerAdded"
QT_MOC_LITERAL(5, 55, 9), // "layerName"
QT_MOC_LITERAL(6, 65, 12), // "layerRemoved"
QT_MOC_LITERAL(7, 78, 22), // "layerWithShapesRemoved"
QT_MOC_LITERAL(8, 101, 8), // "shapeIds"
QT_MOC_LITERAL(9, 110, 17), // "shapeMovedToLayer"
QT_MOC_LITERAL(10, 128, 7), // "shapeId"
QT_MOC_LITERAL(11, 136, 9), // "fromLayer"
QT_MOC_LITERAL(12, 146, 7), // "toLayer"
QT_MOC_LITERAL(13, 154, 22), // "layerVisibilityChanged"
QT_MOC_LITERAL(14, 177, 7), // "visible"
QT_MOC_LITERAL(15, 185, 18), // "rasterLayerChanged"
QT_MOC_LITERAL(16, 204, 15), // "showContextMenu"
QT_MOC_LITERAL(17, 220, 3), // "pos"
QT_MOC_LITERAL(18, 224, 8), // "addLayer"
QT_MOC_LITERAL(19, 233, 14), // "addRasterLayer"
QT_MOC_LITERAL(20, 248, 11), // "removeLayer"
QT_MOC_LITERAL(21, 260, 11), // "renameLayer"
QT_MOC_LITERAL(22, 272, 11), // "exportLayer"
QT_MOC_LITERAL(23, 284, 17), // "renameLayerByName"
QT_MOC_LITERAL(24, 302, 10), // "targetName"
QT_MOC_LITERAL(25, 313, 16), // "applyLayerRename"
QT_MOC_LITERAL(26, 330, 7), // "oldName"
QT_MOC_LITERAL(27, 338, 7), // "newName"
QT_MOC_LITERAL(28, 346, 8), // "isRaster"
QT_MOC_LITERAL(29, 355, 23), // "onLayerSelectionChanged"
QT_MOC_LITERAL(30, 379, 25), // "onVisibilityToggleClicked"
QT_MOC_LITERAL(31, 405, 11) // "renameShape"

    },
    "LayerPanel\0activeLayerChanged\0\0"
    "newLayerName\0layerAdded\0layerName\0"
    "layerRemoved\0layerWithShapesRemoved\0"
    "shapeIds\0shapeMovedToLayer\0shapeId\0"
    "fromLayer\0toLayer\0layerVisibilityChanged\0"
    "visible\0rasterLayerChanged\0showContextMenu\0"
    "pos\0addLayer\0addRasterLayer\0removeLayer\0"
    "renameLayer\0exportLayer\0renameLayerByName\0"
    "targetName\0applyLayerRename\0oldName\0"
    "newName\0isRaster\0onLayerSelectionChanged\0"
    "onVisibilityToggleClicked\0renameShape"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LayerPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  104,    2, 0x06 /* Public */,
       4,    1,  107,    2, 0x06 /* Public */,
       6,    1,  110,    2, 0x06 /* Public */,
       7,    1,  113,    2, 0x06 /* Public */,
       9,    3,  116,    2, 0x06 /* Public */,
      13,    2,  123,    2, 0x06 /* Public */,
      15,    0,  128,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      16,    1,  129,    2, 0x08 /* Private */,
      18,    0,  132,    2, 0x08 /* Private */,
      19,    0,  133,    2, 0x08 /* Private */,
      20,    0,  134,    2, 0x08 /* Private */,
      21,    0,  135,    2, 0x08 /* Private */,
      22,    0,  136,    2, 0x08 /* Private */,
      23,    1,  137,    2, 0x08 /* Private */,
      25,    3,  140,    2, 0x08 /* Private */,
      29,    0,  147,    2, 0x08 /* Private */,
      30,    1,  148,    2, 0x08 /* Private */,
      31,    1,  151,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QStringList,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   10,   11,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    5,   14,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   24,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   26,   27,   28,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,   10,

       0        // eod
};

void LayerPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LayerPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->activeLayerChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->layerAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->layerRemoved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->layerWithShapesRemoved((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 4: _t->shapeMovedToLayer((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 5: _t->layerVisibilityChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->rasterLayerChanged(); break;
        case 7: _t->showContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 8: _t->addLayer(); break;
        case 9: _t->addRasterLayer(); break;
        case 10: _t->removeLayer(); break;
        case 11: _t->renameLayer(); break;
        case 12: _t->exportLayer(); break;
        case 13: _t->renameLayerByName((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->applyLayerRename((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 15: _t->onLayerSelectionChanged(); break;
        case 16: _t->onVisibilityToggleClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->renameShape((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LayerPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::activeLayerChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::layerAdded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::layerRemoved)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)(const QStringList & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::layerWithShapesRemoved)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)(const QString & , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::shapeMovedToLayer)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::layerVisibilityChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LayerPanel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::rasterLayerChanged)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject LayerPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_LayerPanel.data,
    qt_meta_data_LayerPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *LayerPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LayerPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LayerPanel.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int LayerPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void LayerPanel::activeLayerChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LayerPanel::layerAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void LayerPanel::layerRemoved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void LayerPanel::layerWithShapesRemoved(const QStringList & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LayerPanel::shapeMovedToLayer(const QString & _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void LayerPanel::layerVisibilityChanged(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void LayerPanel::rasterLayerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
