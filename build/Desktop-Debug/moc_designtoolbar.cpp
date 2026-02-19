/****************************************************************************
** Meta object code from reading C++ file 'designtoolbar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Toolbars/designtoolbar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QSet>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'designtoolbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StayOpenMenu_t {
    QByteArrayData data[1];
    char stringdata0[13];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StayOpenMenu_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StayOpenMenu_t qt_meta_stringdata_StayOpenMenu = {
    {
QT_MOC_LITERAL(0, 0, 12) // "StayOpenMenu"

    },
    "StayOpenMenu"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StayOpenMenu[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void StayOpenMenu::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject StayOpenMenu::staticMetaObject = { {
    QMetaObject::SuperData::link<QMenu::staticMetaObject>(),
    qt_meta_stringdata_StayOpenMenu.data,
    qt_meta_data_StayOpenMenu,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StayOpenMenu::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StayOpenMenu::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StayOpenMenu.stringdata0))
        return static_cast<void*>(this);
    return QMenu::qt_metacast(_clname);
}

int StayOpenMenu::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMenu::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_DesignToolBar_t {
    QByteArrayData data[75];
    char stringdata0[1158];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DesignToolBar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DesignToolBar_t qt_meta_stringdata_DesignToolBar = {
    {
QT_MOC_LITERAL(0, 0, 13), // "DesignToolBar"
QT_MOC_LITERAL(1, 14, 13), // "viewTriggered"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 13), // "moveTriggered"
QT_MOC_LITERAL(4, 43, 15), // "rotateTriggered"
QT_MOC_LITERAL(5, 59, 14), // "scaleTriggered"
QT_MOC_LITERAL(6, 74, 15), // "zoomInTriggered"
QT_MOC_LITERAL(7, 90, 16), // "zoomOutTriggered"
QT_MOC_LITERAL(8, 107, 21), // "gridVisibilityToggled"
QT_MOC_LITERAL(9, 129, 19), // "gridSnappingToggled"
QT_MOC_LITERAL(10, 149, 20), // "layerSelectTriggered"
QT_MOC_LITERAL(11, 170, 24), // "measureDistanceTriggered"
QT_MOC_LITERAL(12, 195, 17), // "gridPlaneXToggled"
QT_MOC_LITERAL(13, 213, 17), // "gridPlaneYToggled"
QT_MOC_LITERAL(14, 231, 17), // "gridPlaneZToggled"
QT_MOC_LITERAL(15, 249, 18), // "gridOpacityChanged"
QT_MOC_LITERAL(16, 268, 20), // "snapGridSizeXToggled"
QT_MOC_LITERAL(17, 289, 20), // "snapGridSizeYToggled"
QT_MOC_LITERAL(18, 310, 20), // "snapGridSizeZToggled"
QT_MOC_LITERAL(19, 331, 17), // "snapRotateToggled"
QT_MOC_LITERAL(20, 349, 16), // "snapScaleToggled"
QT_MOC_LITERAL(21, 366, 11), // "modeChanged"
QT_MOC_LITERAL(22, 378, 13), // "TransformMode"
QT_MOC_LITERAL(23, 392, 4), // "mode"
QT_MOC_LITERAL(24, 397, 20), // "transformModeChanged"
QT_MOC_LITERAL(25, 418, 9), // "Transform"
QT_MOC_LITERAL(26, 428, 24), // "gridPlaneXOpacityChanged"
QT_MOC_LITERAL(27, 453, 5), // "value"
QT_MOC_LITERAL(28, 459, 24), // "gridPlaneYOpacityChanged"
QT_MOC_LITERAL(29, 484, 24), // "gridPlaneZOpacityChanged"
QT_MOC_LITERAL(30, 509, 18), // "layerOptionToggled"
QT_MOC_LITERAL(31, 528, 6), // "option"
QT_MOC_LITERAL(32, 535, 7), // "checked"
QT_MOC_LITERAL(33, 543, 20), // "searchPlaceTriggered"
QT_MOC_LITERAL(34, 564, 8), // "location"
QT_MOC_LITERAL(35, 573, 25), // "searchCoordinateTriggered"
QT_MOC_LITERAL(36, 599, 3), // "lat"
QT_MOC_LITERAL(37, 603, 3), // "lon"
QT_MOC_LITERAL(38, 607, 15), // "mapLayerChanged"
QT_MOC_LITERAL(39, 623, 5), // "layer"
QT_MOC_LITERAL(40, 629, 21), // "selectCenterTriggered"
QT_MOC_LITERAL(41, 651, 14), // "customMapAdded"
QT_MOC_LITERAL(42, 666, 9), // "layerName"
QT_MOC_LITERAL(43, 676, 7), // "zoomMin"
QT_MOC_LITERAL(44, 684, 7), // "zoomMax"
QT_MOC_LITERAL(45, 692, 7), // "tileUrl"
QT_MOC_LITERAL(46, 700, 7), // "opacity"
QT_MOC_LITERAL(47, 708, 13), // "shapeSelected"
QT_MOC_LITERAL(48, 722, 5), // "shape"
QT_MOC_LITERAL(49, 728, 14), // "bitmapSelected"
QT_MOC_LITERAL(50, 743, 6), // "bitmap"
QT_MOC_LITERAL(51, 750, 19), // "bitmapImageSelected"
QT_MOC_LITERAL(52, 770, 8), // "filePath"
QT_MOC_LITERAL(53, 779, 23), // "editTrajectoryTriggered"
QT_MOC_LITERAL(54, 803, 19), // "presetLayerSelected"
QT_MOC_LITERAL(55, 823, 6), // "preset"
QT_MOC_LITERAL(56, 830, 22), // "importGeoJsonTriggered"
QT_MOC_LITERAL(57, 853, 19), // "geoJsonLayerToggled"
QT_MOC_LITERAL(58, 873, 7), // "visible"
QT_MOC_LITERAL(59, 881, 26), // "searchCoordinatesTriggered"
QT_MOC_LITERAL(60, 908, 8), // "latitude"
QT_MOC_LITERAL(61, 917, 9), // "longitude"
QT_MOC_LITERAL(62, 927, 22), // "addTrajectoryTriggered"
QT_MOC_LITERAL(63, 950, 23), // "coordinateSystemChanged"
QT_MOC_LITERAL(64, 974, 5), // "crsId"
QT_MOC_LITERAL(65, 980, 21), // "tooltipOptionsChanged"
QT_MOC_LITERAL(66, 1002, 13), // "QSet<QString>"
QT_MOC_LITERAL(67, 1016, 13), // "activeOptions"
QT_MOC_LITERAL(68, 1030, 13), // "onModeChanged"
QT_MOC_LITERAL(69, 1044, 26), // "onMeasureDistanceTriggered"
QT_MOC_LITERAL(70, 1071, 19), // "onGeoJsonLayerAdded"
QT_MOC_LITERAL(71, 1091, 13), // "importGeoJson"
QT_MOC_LITERAL(72, 1105, 24), // "onAddBaseLayerToSelected"
QT_MOC_LITERAL(73, 1130, 7), // "layerId"
QT_MOC_LITERAL(74, 1138, 19) // "updateMapLayersMenu"

    },
    "DesignToolBar\0viewTriggered\0\0moveTriggered\0"
    "rotateTriggered\0scaleTriggered\0"
    "zoomInTriggered\0zoomOutTriggered\0"
    "gridVisibilityToggled\0gridSnappingToggled\0"
    "layerSelectTriggered\0measureDistanceTriggered\0"
    "gridPlaneXToggled\0gridPlaneYToggled\0"
    "gridPlaneZToggled\0gridOpacityChanged\0"
    "snapGridSizeXToggled\0snapGridSizeYToggled\0"
    "snapGridSizeZToggled\0snapRotateToggled\0"
    "snapScaleToggled\0modeChanged\0TransformMode\0"
    "mode\0transformModeChanged\0Transform\0"
    "gridPlaneXOpacityChanged\0value\0"
    "gridPlaneYOpacityChanged\0"
    "gridPlaneZOpacityChanged\0layerOptionToggled\0"
    "option\0checked\0searchPlaceTriggered\0"
    "location\0searchCoordinateTriggered\0"
    "lat\0lon\0mapLayerChanged\0layer\0"
    "selectCenterTriggered\0customMapAdded\0"
    "layerName\0zoomMin\0zoomMax\0tileUrl\0"
    "opacity\0shapeSelected\0shape\0bitmapSelected\0"
    "bitmap\0bitmapImageSelected\0filePath\0"
    "editTrajectoryTriggered\0presetLayerSelected\0"
    "preset\0importGeoJsonTriggered\0"
    "geoJsonLayerToggled\0visible\0"
    "searchCoordinatesTriggered\0latitude\0"
    "longitude\0addTrajectoryTriggered\0"
    "coordinateSystemChanged\0crsId\0"
    "tooltipOptionsChanged\0QSet<QString>\0"
    "activeOptions\0onModeChanged\0"
    "onMeasureDistanceTriggered\0"
    "onGeoJsonLayerAdded\0importGeoJson\0"
    "onAddBaseLayerToSelected\0layerId\0"
    "updateMapLayersMenu"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DesignToolBar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      47,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      41,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  249,    2, 0x06 /* Public */,
       3,    0,  250,    2, 0x06 /* Public */,
       4,    0,  251,    2, 0x06 /* Public */,
       5,    0,  252,    2, 0x06 /* Public */,
       6,    0,  253,    2, 0x06 /* Public */,
       7,    0,  254,    2, 0x06 /* Public */,
       8,    1,  255,    2, 0x06 /* Public */,
       9,    1,  258,    2, 0x06 /* Public */,
      10,    0,  261,    2, 0x06 /* Public */,
      11,    0,  262,    2, 0x06 /* Public */,
      12,    1,  263,    2, 0x06 /* Public */,
      13,    1,  266,    2, 0x06 /* Public */,
      14,    1,  269,    2, 0x06 /* Public */,
      15,    1,  272,    2, 0x06 /* Public */,
      16,    1,  275,    2, 0x06 /* Public */,
      17,    1,  278,    2, 0x06 /* Public */,
      18,    1,  281,    2, 0x06 /* Public */,
      19,    1,  284,    2, 0x06 /* Public */,
      20,    1,  287,    2, 0x06 /* Public */,
      21,    1,  290,    2, 0x06 /* Public */,
      24,    1,  293,    2, 0x06 /* Public */,
      26,    1,  296,    2, 0x06 /* Public */,
      28,    1,  299,    2, 0x06 /* Public */,
      29,    1,  302,    2, 0x06 /* Public */,
      30,    2,  305,    2, 0x06 /* Public */,
      33,    1,  310,    2, 0x06 /* Public */,
      35,    2,  313,    2, 0x06 /* Public */,
      38,    1,  318,    2, 0x06 /* Public */,
      40,    0,  321,    2, 0x06 /* Public */,
      41,    5,  322,    2, 0x06 /* Public */,
      47,    1,  333,    2, 0x06 /* Public */,
      49,    1,  336,    2, 0x06 /* Public */,
      51,    1,  339,    2, 0x06 /* Public */,
      53,    0,  342,    2, 0x06 /* Public */,
      54,    1,  343,    2, 0x06 /* Public */,
      56,    1,  346,    2, 0x06 /* Public */,
      57,    2,  349,    2, 0x06 /* Public */,
      59,    2,  354,    2, 0x06 /* Public */,
      62,    0,  359,    2, 0x06 /* Public */,
      63,    1,  360,    2, 0x06 /* Public */,
      65,    1,  363,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      68,    1,  366,    2, 0x0a /* Public */,
      69,    0,  369,    2, 0x0a /* Public */,
      70,    1,  370,    2, 0x0a /* Public */,
      71,    0,  373,    2, 0x08 /* Private */,
      72,    1,  374,    2, 0x08 /* Private */,
      74,    0,  377,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, 0x80000000 | 25,   23,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   31,   32,
    QMetaType::Void, QMetaType::QString,   34,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   36,   37,
    QMetaType::Void, QMetaType::QString,   39,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::QString, QMetaType::QReal,   42,   43,   44,   45,   46,
    QMetaType::Void, QMetaType::QString,   48,
    QMetaType::Void, QMetaType::QString,   50,
    QMetaType::Void, QMetaType::QString,   52,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   55,
    QMetaType::Void, QMetaType::QString,   52,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   42,   58,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   60,   61,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   64,
    QMetaType::Void, 0x80000000 | 66,   67,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   42,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   73,
    QMetaType::Void,

       0        // eod
};

void DesignToolBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DesignToolBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->viewTriggered(); break;
        case 1: _t->moveTriggered(); break;
        case 2: _t->rotateTriggered(); break;
        case 3: _t->scaleTriggered(); break;
        case 4: _t->zoomInTriggered(); break;
        case 5: _t->zoomOutTriggered(); break;
        case 6: _t->gridVisibilityToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->gridSnappingToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->layerSelectTriggered(); break;
        case 9: _t->measureDistanceTriggered(); break;
        case 10: _t->gridPlaneXToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->gridPlaneYToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->gridPlaneZToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->gridOpacityChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->snapGridSizeXToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->snapGridSizeYToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 16: _t->snapGridSizeZToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 17: _t->snapRotateToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->snapScaleToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 19: _t->modeChanged((*reinterpret_cast< TransformMode(*)>(_a[1]))); break;
        case 20: _t->transformModeChanged((*reinterpret_cast< const Transform(*)>(_a[1]))); break;
        case 21: _t->gridPlaneXOpacityChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 22: _t->gridPlaneYOpacityChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 23: _t->gridPlaneZOpacityChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 24: _t->layerOptionToggled((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 25: _t->searchPlaceTriggered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 26: _t->searchCoordinateTriggered((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 27: _t->mapLayerChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 28: _t->selectCenterTriggered(); break;
        case 29: _t->customMapAdded((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< qreal(*)>(_a[5]))); break;
        case 30: _t->shapeSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 31: _t->bitmapSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 32: _t->bitmapImageSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 33: _t->editTrajectoryTriggered(); break;
        case 34: _t->presetLayerSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 35: _t->importGeoJsonTriggered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 36: _t->geoJsonLayerToggled((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 37: _t->searchCoordinatesTriggered((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 38: _t->addTrajectoryTriggered(); break;
        case 39: _t->coordinateSystemChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 40: _t->tooltipOptionsChanged((*reinterpret_cast< const QSet<QString>(*)>(_a[1]))); break;
        case 41: _t->onModeChanged((*reinterpret_cast< TransformMode(*)>(_a[1]))); break;
        case 42: _t->onMeasureDistanceTriggered(); break;
        case 43: _t->onGeoJsonLayerAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 44: _t->importGeoJson(); break;
        case 45: _t->onAddBaseLayerToSelected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 46: _t->updateMapLayersMenu(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 40:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSet<QString> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::viewTriggered)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::moveTriggered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::rotateTriggered)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::scaleTriggered)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::zoomInTriggered)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::zoomOutTriggered)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridVisibilityToggled)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridSnappingToggled)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::layerSelectTriggered)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::measureDistanceTriggered)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneXToggled)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneYToggled)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneZToggled)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridOpacityChanged)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::snapGridSizeXToggled)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::snapGridSizeYToggled)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::snapGridSizeZToggled)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::snapRotateToggled)) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::snapScaleToggled)) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(TransformMode );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::modeChanged)) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const Transform & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::transformModeChanged)) {
                *result = 20;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneXOpacityChanged)) {
                *result = 21;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneYOpacityChanged)) {
                *result = 22;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::gridPlaneZOpacityChanged)) {
                *result = 23;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::layerOptionToggled)) {
                *result = 24;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::searchPlaceTriggered)) {
                *result = 25;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::searchCoordinateTriggered)) {
                *result = 26;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::mapLayerChanged)) {
                *result = 27;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::selectCenterTriggered)) {
                *result = 28;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & , int , int , const QString & , qreal );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::customMapAdded)) {
                *result = 29;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::shapeSelected)) {
                *result = 30;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::bitmapSelected)) {
                *result = 31;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::bitmapImageSelected)) {
                *result = 32;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::editTrajectoryTriggered)) {
                *result = 33;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::presetLayerSelected)) {
                *result = 34;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::importGeoJsonTriggered)) {
                *result = 35;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::geoJsonLayerToggled)) {
                *result = 36;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::searchCoordinatesTriggered)) {
                *result = 37;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::addTrajectoryTriggered)) {
                *result = 38;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::coordinateSystemChanged)) {
                *result = 39;
                return;
            }
        }
        {
            using _t = void (DesignToolBar::*)(const QSet<QString> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DesignToolBar::tooltipOptionsChanged)) {
                *result = 40;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DesignToolBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolBar::staticMetaObject>(),
    qt_meta_stringdata_DesignToolBar.data,
    qt_meta_data_DesignToolBar,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DesignToolBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DesignToolBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DesignToolBar.stringdata0))
        return static_cast<void*>(this);
    return QToolBar::qt_metacast(_clname);
}

int DesignToolBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QToolBar::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    }
    return _id;
}

// SIGNAL 0
void DesignToolBar::viewTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DesignToolBar::moveTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DesignToolBar::rotateTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void DesignToolBar::scaleTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void DesignToolBar::zoomInTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DesignToolBar::zoomOutTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DesignToolBar::gridVisibilityToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void DesignToolBar::gridSnappingToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void DesignToolBar::layerSelectTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void DesignToolBar::measureDistanceTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void DesignToolBar::gridPlaneXToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void DesignToolBar::gridPlaneYToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void DesignToolBar::gridPlaneZToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void DesignToolBar::gridOpacityChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void DesignToolBar::snapGridSizeXToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void DesignToolBar::snapGridSizeYToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void DesignToolBar::snapGridSizeZToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void DesignToolBar::snapRotateToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void DesignToolBar::snapScaleToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void DesignToolBar::modeChanged(TransformMode _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void DesignToolBar::transformModeChanged(const Transform & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 21
void DesignToolBar::gridPlaneXOpacityChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}

// SIGNAL 22
void DesignToolBar::gridPlaneYOpacityChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 23
void DesignToolBar::gridPlaneZOpacityChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 23, _a);
}

// SIGNAL 24
void DesignToolBar::layerOptionToggled(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 24, _a);
}

// SIGNAL 25
void DesignToolBar::searchPlaceTriggered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 25, _a);
}

// SIGNAL 26
void DesignToolBar::searchCoordinateTriggered(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 26, _a);
}

// SIGNAL 27
void DesignToolBar::mapLayerChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 27, _a);
}

// SIGNAL 28
void DesignToolBar::selectCenterTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void DesignToolBar::customMapAdded(const QString & _t1, int _t2, int _t3, const QString & _t4, qreal _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 29, _a);
}

// SIGNAL 30
void DesignToolBar::shapeSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 30, _a);
}

// SIGNAL 31
void DesignToolBar::bitmapSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 31, _a);
}

// SIGNAL 32
void DesignToolBar::bitmapImageSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 32, _a);
}

// SIGNAL 33
void DesignToolBar::editTrajectoryTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 33, nullptr);
}

// SIGNAL 34
void DesignToolBar::presetLayerSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 34, _a);
}

// SIGNAL 35
void DesignToolBar::importGeoJsonTriggered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 35, _a);
}

// SIGNAL 36
void DesignToolBar::geoJsonLayerToggled(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 36, _a);
}

// SIGNAL 37
void DesignToolBar::searchCoordinatesTriggered(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 37, _a);
}

// SIGNAL 38
void DesignToolBar::addTrajectoryTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 38, nullptr);
}

// SIGNAL 39
void DesignToolBar::coordinateSystemChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 39, _a);
}

// SIGNAL 40
void DesignToolBar::tooltipOptionsChanged(const QSet<QString> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 40, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
