/****************************************************************************
** Meta object code from reading C++ file 'canvaswidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Tacticaldisplay/canvaswidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canvaswidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CanvasWidget_t {
    QByteArrayData data[36];
    char stringdata0[490];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CanvasWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CanvasWidget_t qt_meta_stringdata_CanvasWidget = {
    {
QT_MOC_LITERAL(0, 0, 12), // "CanvasWidget"
QT_MOC_LITERAL(1, 13, 20), // "selectEntitybyCursor"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 2), // "ID"
QT_MOC_LITERAL(4, 38, 10), // "MoveEntity"
QT_MOC_LITERAL(5, 49, 17), // "trajectoryUpdated"
QT_MOC_LITERAL(6, 67, 8), // "entityId"
QT_MOC_LITERAL(7, 76, 9), // "waypoints"
QT_MOC_LITERAL(8, 86, 19), // "airbaseLayerToggled"
QT_MOC_LITERAL(9, 106, 7), // "visible"
QT_MOC_LITERAL(10, 114, 17), // "geoJsonLayerAdded"
QT_MOC_LITERAL(11, 132, 9), // "layerName"
QT_MOC_LITERAL(12, 142, 13), // "pointsUpdated"
QT_MOC_LITERAL(13, 156, 14), // "QList<QPointF>"
QT_MOC_LITERAL(14, 171, 6), // "points"
QT_MOC_LITERAL(15, 178, 15), // "onGISKeyPressed"
QT_MOC_LITERAL(16, 194, 10), // "QKeyEvent*"
QT_MOC_LITERAL(17, 205, 5), // "event"
QT_MOC_LITERAL(18, 211, 17), // "onGISMousePressed"
QT_MOC_LITERAL(19, 229, 12), // "QMouseEvent*"
QT_MOC_LITERAL(20, 242, 15), // "onGISMouseMoved"
QT_MOC_LITERAL(21, 258, 18), // "onGISMouseReleased"
QT_MOC_LITERAL(22, 277, 12), // "onGISPainted"
QT_MOC_LITERAL(23, 290, 12), // "QPaintEvent*"
QT_MOC_LITERAL(24, 303, 28), // "updateWaypointsFromInspector"
QT_MOC_LITERAL(25, 332, 18), // "onDistanceMeasured"
QT_MOC_LITERAL(26, 351, 8), // "distance"
QT_MOC_LITERAL(27, 360, 10), // "startPoint"
QT_MOC_LITERAL(28, 371, 8), // "endPoint"
QT_MOC_LITERAL(29, 380, 21), // "onPresetLayerSelected"
QT_MOC_LITERAL(30, 402, 6), // "preset"
QT_MOC_LITERAL(31, 409, 18), // "importGeoJsonLayer"
QT_MOC_LITERAL(32, 428, 8), // "filePath"
QT_MOC_LITERAL(33, 437, 21), // "onGeoJsonLayerToggled"
QT_MOC_LITERAL(34, 459, 24), // "onMeasurementTypeChanged"
QT_MOC_LITERAL(35, 484, 5) // "isEll"

    },
    "CanvasWidget\0selectEntitybyCursor\0\0"
    "ID\0MoveEntity\0trajectoryUpdated\0"
    "entityId\0waypoints\0airbaseLayerToggled\0"
    "visible\0geoJsonLayerAdded\0layerName\0"
    "pointsUpdated\0QList<QPointF>\0points\0"
    "onGISKeyPressed\0QKeyEvent*\0event\0"
    "onGISMousePressed\0QMouseEvent*\0"
    "onGISMouseMoved\0onGISMouseReleased\0"
    "onGISPainted\0QPaintEvent*\0"
    "updateWaypointsFromInspector\0"
    "onDistanceMeasured\0distance\0startPoint\0"
    "endPoint\0onPresetLayerSelected\0preset\0"
    "importGeoJsonLayer\0filePath\0"
    "onGeoJsonLayerToggled\0onMeasurementTypeChanged\0"
    "isEll"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CanvasWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   99,    2, 0x06 /* Public */,
       4,    1,  102,    2, 0x06 /* Public */,
       5,    2,  105,    2, 0x06 /* Public */,
       8,    1,  110,    2, 0x06 /* Public */,
      10,    1,  113,    2, 0x06 /* Public */,
      12,    1,  116,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    1,  119,    2, 0x0a /* Public */,
      18,    1,  122,    2, 0x0a /* Public */,
      20,    1,  125,    2, 0x0a /* Public */,
      21,    1,  128,    2, 0x0a /* Public */,
      22,    1,  131,    2, 0x0a /* Public */,
      24,    2,  134,    2, 0x0a /* Public */,
      25,    3,  139,    2, 0x0a /* Public */,
      29,    1,  146,    2, 0x0a /* Public */,
      31,    1,  149,    2, 0x0a /* Public */,
      33,    2,  152,    2, 0x0a /* Public */,
      34,    1,  157,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,    6,    7,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, 0x80000000 | 13,   14,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 16,   17,
    QMetaType::Void, 0x80000000 | 19,   17,
    QMetaType::Void, 0x80000000 | 19,   17,
    QMetaType::Void, 0x80000000 | 19,   17,
    QMetaType::Void, 0x80000000 | 23,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,    6,    7,
    QMetaType::Void, QMetaType::Double, QMetaType::QPointF, QMetaType::QPointF,   26,   27,   28,
    QMetaType::Void, QMetaType::QString,   30,
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   11,    9,
    QMetaType::Void, QMetaType::Bool,   35,

       0        // eod
};

void CanvasWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CanvasWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selectEntitybyCursor((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->MoveEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->trajectoryUpdated((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 3: _t->airbaseLayerToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->geoJsonLayerAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->pointsUpdated((*reinterpret_cast< const QList<QPointF>(*)>(_a[1]))); break;
        case 6: _t->onGISKeyPressed((*reinterpret_cast< QKeyEvent*(*)>(_a[1]))); break;
        case 7: _t->onGISMousePressed((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 8: _t->onGISMouseMoved((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 9: _t->onGISMouseReleased((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 10: _t->onGISPainted((*reinterpret_cast< QPaintEvent*(*)>(_a[1]))); break;
        case 11: _t->updateWaypointsFromInspector((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 12: _t->onDistanceMeasured((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< QPointF(*)>(_a[2])),(*reinterpret_cast< QPointF(*)>(_a[3]))); break;
        case 13: _t->onPresetLayerSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->importGeoJsonLayer((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 15: _t->onGeoJsonLayerToggled((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 16: _t->onMeasurementTypeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QPointF> >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QKeyEvent* >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CanvasWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::selectEntitybyCursor)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::MoveEntity)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString , QJsonArray );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::trajectoryUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::airbaseLayerToggled)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::geoJsonLayerAdded)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(const QList<QPointF> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::pointsUpdated)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CanvasWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CanvasWidget.data,
    qt_meta_data_CanvasWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CanvasWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CanvasWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CanvasWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CanvasWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void CanvasWidget::selectEntitybyCursor(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CanvasWidget::MoveEntity(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CanvasWidget::trajectoryUpdated(QString _t1, QJsonArray _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CanvasWidget::airbaseLayerToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CanvasWidget::geoJsonLayerAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CanvasWidget::pointsUpdated(const QList<QPointF> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
