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
    QByteArrayData data[47];
    char stringdata0[678];
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
QT_MOC_LITERAL(3, 35, 8), // "entityId"
QT_MOC_LITERAL(4, 44, 17), // "isEntitySelection"
QT_MOC_LITERAL(5, 62, 10), // "MoveEntity"
QT_MOC_LITERAL(6, 73, 2), // "ID"
QT_MOC_LITERAL(7, 76, 17), // "trajectoryUpdated"
QT_MOC_LITERAL(8, 94, 9), // "waypoints"
QT_MOC_LITERAL(9, 104, 26), // "trajectoryUpdatedforLogger"
QT_MOC_LITERAL(10, 131, 23), // "std::vector<Waypoints*>"
QT_MOC_LITERAL(11, 155, 12), // "Trajectories"
QT_MOC_LITERAL(12, 168, 19), // "airbaseLayerToggled"
QT_MOC_LITERAL(13, 188, 7), // "visible"
QT_MOC_LITERAL(14, 196, 17), // "geoJsonLayerAdded"
QT_MOC_LITERAL(15, 214, 9), // "layerName"
QT_MOC_LITERAL(16, 224, 13), // "pointsUpdated"
QT_MOC_LITERAL(17, 238, 14), // "QList<QPointF>"
QT_MOC_LITERAL(18, 253, 6), // "points"
QT_MOC_LITERAL(19, 260, 26), // "requestAddEntityAtPosition"
QT_MOC_LITERAL(20, 287, 9), // "longitude"
QT_MOC_LITERAL(21, 297, 8), // "latitude"
QT_MOC_LITERAL(22, 306, 22), // "entitySelectedOnCanvas"
QT_MOC_LITERAL(23, 329, 6), // "ReInit"
QT_MOC_LITERAL(24, 336, 15), // "onGISKeyPressed"
QT_MOC_LITERAL(25, 352, 10), // "QKeyEvent*"
QT_MOC_LITERAL(26, 363, 5), // "event"
QT_MOC_LITERAL(27, 369, 17), // "onGISMousePressed"
QT_MOC_LITERAL(28, 387, 12), // "QMouseEvent*"
QT_MOC_LITERAL(29, 400, 15), // "onGISMouseMoved"
QT_MOC_LITERAL(30, 416, 18), // "onGISMouseReleased"
QT_MOC_LITERAL(31, 435, 12), // "onGISPainted"
QT_MOC_LITERAL(32, 448, 12), // "QPaintEvent*"
QT_MOC_LITERAL(33, 461, 28), // "updateWaypointsFromInspector"
QT_MOC_LITERAL(34, 490, 18), // "onDistanceMeasured"
QT_MOC_LITERAL(35, 509, 8), // "distance"
QT_MOC_LITERAL(36, 518, 10), // "startPoint"
QT_MOC_LITERAL(37, 529, 8), // "endPoint"
QT_MOC_LITERAL(38, 538, 21), // "onPresetLayerSelected"
QT_MOC_LITERAL(39, 560, 6), // "preset"
QT_MOC_LITERAL(40, 567, 18), // "importGeoJsonLayer"
QT_MOC_LITERAL(41, 586, 8), // "filePath"
QT_MOC_LITERAL(42, 595, 21), // "onGeoJsonLayerToggled"
QT_MOC_LITERAL(43, 617, 24), // "onMeasurementTypeChanged"
QT_MOC_LITERAL(44, 642, 5), // "isEll"
QT_MOC_LITERAL(45, 648, 14), // "showEntityInfo"
QT_MOC_LITERAL(46, 663, 14) // "hideEntityInfo"

    },
    "CanvasWidget\0selectEntitybyCursor\0\0"
    "entityId\0isEntitySelection\0MoveEntity\0"
    "ID\0trajectoryUpdated\0waypoints\0"
    "trajectoryUpdatedforLogger\0"
    "std::vector<Waypoints*>\0Trajectories\0"
    "airbaseLayerToggled\0visible\0"
    "geoJsonLayerAdded\0layerName\0pointsUpdated\0"
    "QList<QPointF>\0points\0requestAddEntityAtPosition\0"
    "longitude\0latitude\0entitySelectedOnCanvas\0"
    "ReInit\0onGISKeyPressed\0QKeyEvent*\0"
    "event\0onGISMousePressed\0QMouseEvent*\0"
    "onGISMouseMoved\0onGISMouseReleased\0"
    "onGISPainted\0QPaintEvent*\0"
    "updateWaypointsFromInspector\0"
    "onDistanceMeasured\0distance\0startPoint\0"
    "endPoint\0onPresetLayerSelected\0preset\0"
    "importGeoJsonLayer\0filePath\0"
    "onGeoJsonLayerToggled\0onMeasurementTypeChanged\0"
    "isEll\0showEntityInfo\0hideEntityInfo"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CanvasWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      24,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,  134,    2, 0x06 /* Public */,
       1,    1,  139,    2, 0x26 /* Public | MethodCloned */,
       5,    1,  142,    2, 0x06 /* Public */,
       7,    2,  145,    2, 0x06 /* Public */,
       9,    2,  150,    2, 0x06 /* Public */,
      12,    1,  155,    2, 0x06 /* Public */,
      14,    1,  158,    2, 0x06 /* Public */,
      16,    1,  161,    2, 0x06 /* Public */,
      19,    2,  164,    2, 0x06 /* Public */,
      22,    1,  169,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      23,    0,  172,    2, 0x0a /* Public */,
      24,    1,  173,    2, 0x0a /* Public */,
      27,    1,  176,    2, 0x0a /* Public */,
      29,    1,  179,    2, 0x0a /* Public */,
      30,    1,  182,    2, 0x0a /* Public */,
      31,    1,  185,    2, 0x0a /* Public */,
      33,    2,  188,    2, 0x0a /* Public */,
      34,    3,  193,    2, 0x0a /* Public */,
      38,    1,  200,    2, 0x0a /* Public */,
      40,    1,  203,    2, 0x0a /* Public */,
      42,    2,  206,    2, 0x0a /* Public */,
      43,    1,  211,    2, 0x08 /* Private */,
      45,    1,  214,    2, 0x0a /* Public */,
      46,    0,  217,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,    3,    8,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 10,    3,   11,
    QMetaType::Void, QMetaType::Bool,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   20,   21,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void, 0x80000000 | 28,   26,
    QMetaType::Void, 0x80000000 | 28,   26,
    QMetaType::Void, 0x80000000 | 28,   26,
    QMetaType::Void, 0x80000000 | 32,   26,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonArray,    3,    8,
    QMetaType::Void, QMetaType::Double, QMetaType::QPointF, QMetaType::QPointF,   35,   36,   37,
    QMetaType::Void, QMetaType::QString,   39,
    QMetaType::Void, QMetaType::QString,   41,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   15,   13,
    QMetaType::Void, QMetaType::Bool,   44,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

       0        // eod
};

void CanvasWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CanvasWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selectEntitybyCursor((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: _t->selectEntitybyCursor((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->MoveEntity((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->trajectoryUpdated((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 4: _t->trajectoryUpdatedforLogger((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< std::vector<Waypoints*>(*)>(_a[2]))); break;
        case 5: _t->airbaseLayerToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->geoJsonLayerAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->pointsUpdated((*reinterpret_cast< const QList<QPointF>(*)>(_a[1]))); break;
        case 8: _t->requestAddEntityAtPosition((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 9: _t->entitySelectedOnCanvas((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->ReInit(); break;
        case 11: _t->onGISKeyPressed((*reinterpret_cast< QKeyEvent*(*)>(_a[1]))); break;
        case 12: _t->onGISMousePressed((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 13: _t->onGISMouseMoved((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 14: _t->onGISMouseReleased((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 15: _t->onGISPainted((*reinterpret_cast< QPaintEvent*(*)>(_a[1]))); break;
        case 16: _t->updateWaypointsFromInspector((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QJsonArray(*)>(_a[2]))); break;
        case 17: _t->onDistanceMeasured((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< QPointF(*)>(_a[2])),(*reinterpret_cast< QPointF(*)>(_a[3]))); break;
        case 18: _t->onPresetLayerSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 19: _t->importGeoJsonLayer((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->onGeoJsonLayerToggled((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 21: _t->onMeasurementTypeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 22: _t->showEntityInfo((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 23: _t->hideEntityInfo(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QPointF> >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QKeyEvent* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 14:
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
            using _t = void (CanvasWidget::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::selectEntitybyCursor)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::MoveEntity)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString , QJsonArray );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::trajectoryUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString , std::vector<Waypoints*> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::trajectoryUpdatedforLogger)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::airbaseLayerToggled)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::geoJsonLayerAdded)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(const QList<QPointF> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::pointsUpdated)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::requestAddEntityAtPosition)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (CanvasWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanvasWidget::entitySelectedOnCanvas)) {
                *result = 9;
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
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    }
    return _id;
}

// SIGNAL 0
void CanvasWidget::selectEntitybyCursor(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void CanvasWidget::MoveEntity(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CanvasWidget::trajectoryUpdated(QString _t1, QJsonArray _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CanvasWidget::trajectoryUpdatedforLogger(QString _t1, std::vector<Waypoints*> _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CanvasWidget::airbaseLayerToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CanvasWidget::geoJsonLayerAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void CanvasWidget::pointsUpdated(const QList<QPointF> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void CanvasWidget::requestAddEntityAtPosition(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void CanvasWidget::entitySelectedOnCanvas(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
