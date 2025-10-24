/****************************************************************************
** Meta object code from reading C++ file 'tacticaldisplay.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Tacticaldisplay/tacticaldisplay.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tacticaldisplay.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TacticalDisplay_t {
    QByteArrayData data[17];
    char stringdata0[162];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TacticalDisplay_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TacticalDisplay_t qt_meta_stringdata_TacticalDisplay = {
    {
QT_MOC_LITERAL(0, 0, 15), // "TacticalDisplay"
QT_MOC_LITERAL(1, 16, 12), // "meshSelected"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 2), // "ID"
QT_MOC_LITERAL(4, 33, 7), // "addMesh"
QT_MOC_LITERAL(5, 41, 8), // "MeshData"
QT_MOC_LITERAL(6, 50, 8), // "meshData"
QT_MOC_LITERAL(7, 59, 10), // "removeMesh"
QT_MOC_LITERAL(8, 70, 12), // "selectedMesh"
QT_MOC_LITERAL(9, 83, 12), // "setMapLayers"
QT_MOC_LITERAL(10, 96, 10), // "layerNames"
QT_MOC_LITERAL(11, 107, 12), // "addCustomMap"
QT_MOC_LITERAL(12, 120, 9), // "layerName"
QT_MOC_LITERAL(13, 130, 7), // "zoomMin"
QT_MOC_LITERAL(14, 138, 7), // "zoomMax"
QT_MOC_LITERAL(15, 146, 7), // "tileUrl"
QT_MOC_LITERAL(16, 154, 7) // "opacity"

    },
    "TacticalDisplay\0meshSelected\0\0ID\0"
    "addMesh\0MeshData\0meshData\0removeMesh\0"
    "selectedMesh\0setMapLayers\0layerNames\0"
    "addCustomMap\0layerName\0zoomMin\0zoomMax\0"
    "tileUrl\0opacity"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TacticalDisplay[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   52,    2, 0x0a /* Public */,
       7,    1,   57,    2, 0x0a /* Public */,
       8,    1,   60,    2, 0x0a /* Public */,
       9,    1,   63,    2, 0x0a /* Public */,
      11,    5,   66,    2, 0x0a /* Public */,
      11,    4,   77,    2, 0x2a /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 5,    3,    6,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QStringList,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::QString, QMetaType::QReal,   12,   13,   14,   15,   16,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::QString,   12,   13,   14,   15,

       0        // eod
};

void TacticalDisplay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TacticalDisplay *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->meshSelected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->addMesh((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< MeshData(*)>(_a[2]))); break;
        case 2: _t->removeMesh((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->selectedMesh((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->setMapLayers((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 5: _t->addCustomMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< qreal(*)>(_a[5]))); break;
        case 6: _t->addCustomMap((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TacticalDisplay::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TacticalDisplay::meshSelected)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TacticalDisplay::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TacticalDisplay.data,
    qt_meta_data_TacticalDisplay,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TacticalDisplay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TacticalDisplay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TacticalDisplay.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TacticalDisplay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void TacticalDisplay::meshSelected(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
