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
    QByteArrayData data[17];
    char stringdata0[224];
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
QT_MOC_LITERAL(7, 78, 22), // "layerVisibilityChanged"
QT_MOC_LITERAL(8, 101, 7), // "visible"
QT_MOC_LITERAL(9, 109, 15), // "showContextMenu"
QT_MOC_LITERAL(10, 125, 3), // "pos"
QT_MOC_LITERAL(11, 129, 8), // "addLayer"
QT_MOC_LITERAL(12, 138, 11), // "removeLayer"
QT_MOC_LITERAL(13, 150, 11), // "renameLayer"
QT_MOC_LITERAL(14, 162, 11), // "exportLayer"
QT_MOC_LITERAL(15, 174, 23), // "onLayerSelectionChanged"
QT_MOC_LITERAL(16, 198, 25) // "onVisibilityToggleClicked"

    },
    "LayerPanel\0activeLayerChanged\0\0"
    "newLayerName\0layerAdded\0layerName\0"
    "layerRemoved\0layerVisibilityChanged\0"
    "visible\0showContextMenu\0pos\0addLayer\0"
    "removeLayer\0renameLayer\0exportLayer\0"
    "onLayerSelectionChanged\0"
    "onVisibilityToggleClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LayerPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    1,   72,    2, 0x06 /* Public */,
       6,    1,   75,    2, 0x06 /* Public */,
       7,    2,   78,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    1,   83,    2, 0x08 /* Private */,
      11,    0,   86,    2, 0x08 /* Private */,
      12,    0,   87,    2, 0x08 /* Private */,
      13,    0,   88,    2, 0x08 /* Private */,
      14,    0,   89,    2, 0x08 /* Private */,
      15,    0,   90,    2, 0x08 /* Private */,
      16,    1,   91,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    5,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,

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
        case 3: _t->layerVisibilityChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: _t->showContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 5: _t->addLayer(); break;
        case 6: _t->removeLayer(); break;
        case 7: _t->renameLayer(); break;
        case 8: _t->exportLayer(); break;
        case 9: _t->onLayerSelectionChanged(); break;
        case 10: _t->onVisibilityToggleClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
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
            using _t = void (LayerPanel::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayerPanel::layerVisibilityChanged)) {
                *result = 3;
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
void LayerPanel::layerVisibilityChanged(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
