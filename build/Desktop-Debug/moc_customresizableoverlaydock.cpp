/****************************************************************************
** Meta object code from reading C++ file 'customresizableoverlaydock.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Editors/customresizableoverlaydock.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'customresizableoverlaydock.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CustomResizableOverlayDock_t {
    QByteArrayData data[8];
    char stringdata0[72];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CustomResizableOverlayDock_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CustomResizableOverlayDock_t qt_meta_stringdata_CustomResizableOverlayDock = {
    {
QT_MOC_LITERAL(0, 0, 26), // "CustomResizableOverlayDock"
QT_MOC_LITERAL(1, 27, 5), // "moved"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 6), // "oldPos"
QT_MOC_LITERAL(4, 41, 6), // "newPos"
QT_MOC_LITERAL(5, 48, 7), // "resized"
QT_MOC_LITERAL(6, 56, 7), // "oldSize"
QT_MOC_LITERAL(7, 64, 7) // "newSize"

    },
    "CustomResizableOverlayDock\0moved\0\0"
    "oldPos\0newPos\0resized\0oldSize\0newSize"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CustomResizableOverlayDock[] = {

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
    QMetaType::Void, QMetaType::QPoint, QMetaType::QPoint,    3,    4,
    QMetaType::Void, QMetaType::QSize, QMetaType::QSize,    6,    7,

       0        // eod
};

void CustomResizableOverlayDock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CustomResizableOverlayDock *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->moved((*reinterpret_cast< QPoint(*)>(_a[1])),(*reinterpret_cast< QPoint(*)>(_a[2]))); break;
        case 1: _t->resized((*reinterpret_cast< QSize(*)>(_a[1])),(*reinterpret_cast< QSize(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CustomResizableOverlayDock::*)(QPoint , QPoint );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomResizableOverlayDock::moved)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CustomResizableOverlayDock::*)(QSize , QSize );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomResizableOverlayDock::resized)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CustomResizableOverlayDock::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_CustomResizableOverlayDock.data,
    qt_meta_data_CustomResizableOverlayDock,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CustomResizableOverlayDock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CustomResizableOverlayDock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CustomResizableOverlayDock.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int CustomResizableOverlayDock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
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
void CustomResizableOverlayDock::moved(QPoint _t1, QPoint _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CustomResizableOverlayDock::resized(QSize _t1, QSize _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
