/****************************************************************************
** Meta object code from reading C++ file 'scriptengine.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/ScriptEngine/scriptengine.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scriptengine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScriptEngine_t {
    QByteArrayData data[98];
    char stringdata0[1237];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScriptEngine_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScriptEngine_t qt_meta_stringdata_ScriptEngine = {
    {
QT_MOC_LITERAL(0, 0, 12), // "ScriptEngine"
QT_MOC_LITERAL(1, 13, 18), // "requestSidebarView"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 8), // "viewName"
QT_MOC_LITERAL(4, 42, 17), // "requestDisplayTab"
QT_MOC_LITERAL(5, 60, 7), // "tabName"
QT_MOC_LITERAL(6, 68, 19), // "requestSelectEntity"
QT_MOC_LITERAL(7, 88, 8), // "entityId"
QT_MOC_LITERAL(8, 97, 23), // "requestSensorScreenshot"
QT_MOC_LITERAL(9, 121, 8), // "filePath"
QT_MOC_LITERAL(10, 130, 11), // "ScriptSleep"
QT_MOC_LITERAL(11, 142, 12), // "milliseconds"
QT_MOC_LITERAL(12, 155, 11), // "addProfiles"
QT_MOC_LITERAL(13, 167, 17), // "ProfileCategaory*"
QT_MOC_LITERAL(14, 185, 11), // "std::string"
QT_MOC_LITERAL(15, 197, 4), // "name"
QT_MOC_LITERAL(16, 202, 9), // "addEntity"
QT_MOC_LITERAL(17, 212, 9), // "Platform*"
QT_MOC_LITERAL(18, 222, 2), // "Id"
QT_MOC_LITERAL(19, 225, 5), // "bool&"
QT_MOC_LITERAL(20, 231, 7), // "profile"
QT_MOC_LITERAL(21, 239, 9), // "addFolder"
QT_MOC_LITERAL(22, 249, 7), // "Folder*"
QT_MOC_LITERAL(23, 257, 12), // "addComponent"
QT_MOC_LITERAL(24, 270, 12), // "renameEntity"
QT_MOC_LITERAL(25, 283, 2), // "id"
QT_MOC_LITERAL(26, 286, 7), // "newName"
QT_MOC_LITERAL(27, 294, 13), // "renameProfile"
QT_MOC_LITERAL(28, 308, 9), // "profileID"
QT_MOC_LITERAL(29, 318, 12), // "renameFolder"
QT_MOC_LITERAL(30, 331, 8), // "folderID"
QT_MOC_LITERAL(31, 340, 13), // "removeProfile"
QT_MOC_LITERAL(32, 354, 12), // "removeFolder"
QT_MOC_LITERAL(33, 367, 8), // "parentId"
QT_MOC_LITERAL(34, 376, 12), // "removeEntity"
QT_MOC_LITERAL(35, 389, 8), // "entityID"
QT_MOC_LITERAL(36, 398, 13), // "getEntityById"
QT_MOC_LITERAL(37, 412, 7), // "Entity*"
QT_MOC_LITERAL(38, 420, 18), // "findEntitiesByType"
QT_MOC_LITERAL(39, 439, 13), // "CScriptArray*"
QT_MOC_LITERAL(40, 453, 4), // "type"
QT_MOC_LITERAL(41, 458, 14), // "getAllEntities"
QT_MOC_LITERAL(42, 473, 15), // "getAllEntityIds"
QT_MOC_LITERAL(43, 489, 11), // "renderscene"
QT_MOC_LITERAL(44, 501, 23), // "canvasCreateVectorLayer"
QT_MOC_LITERAL(45, 525, 9), // "layerName"
QT_MOC_LITERAL(46, 535, 17), // "canvasSelectLayer"
QT_MOC_LITERAL(47, 553, 15), // "canvasAddCircle"
QT_MOC_LITERAL(48, 569, 6), // "radius"
QT_MOC_LITERAL(49, 576, 18), // "canvasAddRectangle"
QT_MOC_LITERAL(50, 595, 1), // "w"
QT_MOC_LITERAL(51, 597, 1), // "h"
QT_MOC_LITERAL(52, 599, 16), // "canvasAddPolygon"
QT_MOC_LITERAL(53, 616, 3), // "pts"
QT_MOC_LITERAL(54, 620, 15), // "canvasStartLine"
QT_MOC_LITERAL(55, 636, 18), // "canvasAddLinePoint"
QT_MOC_LITERAL(56, 655, 3), // "lon"
QT_MOC_LITERAL(57, 659, 3), // "lat"
QT_MOC_LITERAL(58, 663, 16), // "canvasFinishLine"
QT_MOC_LITERAL(59, 680, 14), // "canvasAddPoint"
QT_MOC_LITERAL(60, 695, 1), // "x"
QT_MOC_LITERAL(61, 697, 1), // "y"
QT_MOC_LITERAL(62, 699, 16), // "onBitmapSelected"
QT_MOC_LITERAL(63, 716, 10), // "bitmapType"
QT_MOC_LITERAL(64, 727, 18), // "getBitmapImagePath"
QT_MOC_LITERAL(65, 746, 21), // "onBitmapImageSelected"
QT_MOC_LITERAL(66, 768, 20), // "canvasToggleAirbases"
QT_MOC_LITERAL(67, 789, 24), // "canvasImportGeoJsonLayer"
QT_MOC_LITERAL(68, 814, 24), // "canvasToggleGeoJsonLayer"
QT_MOC_LITERAL(69, 839, 7), // "visible"
QT_MOC_LITERAL(70, 847, 30), // "canvasStartDistanceMeasurement"
QT_MOC_LITERAL(71, 878, 21), // "canvasAddMeasurePoint"
QT_MOC_LITERAL(72, 900, 28), // "canvasGetLastSegmentDistance"
QT_MOC_LITERAL(73, 929, 22), // "canvasGetTotalDistance"
QT_MOC_LITERAL(74, 952, 24), // "canvasSetMeasurementUnit"
QT_MOC_LITERAL(75, 977, 4), // "unit"
QT_MOC_LITERAL(76, 982, 15), // "canvasSwitchMap"
QT_MOC_LITERAL(77, 998, 7), // "mapName"
QT_MOC_LITERAL(78, 1006, 22), // "switchCoordinateSystem"
QT_MOC_LITERAL(79, 1029, 6), // "system"
QT_MOC_LITERAL(80, 1036, 9), // "moveShape"
QT_MOC_LITERAL(81, 1046, 9), // "shapeName"
QT_MOC_LITERAL(82, 1056, 11), // "rotateShape"
QT_MOC_LITERAL(83, 1068, 8), // "angleDeg"
QT_MOC_LITERAL(84, 1077, 16), // "showShapeHistory"
QT_MOC_LITERAL(85, 1094, 16), // "hideShapeHistory"
QT_MOC_LITERAL(86, 1111, 19), // "restoreShapeHistory"
QT_MOC_LITERAL(87, 1131, 7), // "addText"
QT_MOC_LITERAL(88, 1139, 4), // "text"
QT_MOC_LITERAL(89, 1144, 18), // "addShapeProperties"
QT_MOC_LITERAL(90, 1163, 1), // "r"
QT_MOC_LITERAL(91, 1165, 1), // "g"
QT_MOC_LITERAL(92, 1167, 1), // "b"
QT_MOC_LITERAL(93, 1169, 15), // "borderThickness"
QT_MOC_LITERAL(94, 1185, 11), // "deleteshape"
QT_MOC_LITERAL(95, 1197, 22), // "setCanvasSelectedShape"
QT_MOC_LITERAL(96, 1220, 7), // "useCity"
QT_MOC_LITERAL(97, 1228, 8) // "cityName"

    },
    "ScriptEngine\0requestSidebarView\0\0"
    "viewName\0requestDisplayTab\0tabName\0"
    "requestSelectEntity\0entityId\0"
    "requestSensorScreenshot\0filePath\0"
    "ScriptSleep\0milliseconds\0addProfiles\0"
    "ProfileCategaory*\0std::string\0name\0"
    "addEntity\0Platform*\0Id\0bool&\0profile\0"
    "addFolder\0Folder*\0addComponent\0"
    "renameEntity\0id\0newName\0renameProfile\0"
    "profileID\0renameFolder\0folderID\0"
    "removeProfile\0removeFolder\0parentId\0"
    "removeEntity\0entityID\0getEntityById\0"
    "Entity*\0findEntitiesByType\0CScriptArray*\0"
    "type\0getAllEntities\0getAllEntityIds\0"
    "renderscene\0canvasCreateVectorLayer\0"
    "layerName\0canvasSelectLayer\0canvasAddCircle\0"
    "radius\0canvasAddRectangle\0w\0h\0"
    "canvasAddPolygon\0pts\0canvasStartLine\0"
    "canvasAddLinePoint\0lon\0lat\0canvasFinishLine\0"
    "canvasAddPoint\0x\0y\0onBitmapSelected\0"
    "bitmapType\0getBitmapImagePath\0"
    "onBitmapImageSelected\0canvasToggleAirbases\0"
    "canvasImportGeoJsonLayer\0"
    "canvasToggleGeoJsonLayer\0visible\0"
    "canvasStartDistanceMeasurement\0"
    "canvasAddMeasurePoint\0"
    "canvasGetLastSegmentDistance\0"
    "canvasGetTotalDistance\0canvasSetMeasurementUnit\0"
    "unit\0canvasSwitchMap\0mapName\0"
    "switchCoordinateSystem\0system\0moveShape\0"
    "shapeName\0rotateShape\0angleDeg\0"
    "showShapeHistory\0hideShapeHistory\0"
    "restoreShapeHistory\0addText\0text\0"
    "addShapeProperties\0r\0g\0b\0borderThickness\0"
    "deleteshape\0setCanvasSelectedShape\0"
    "useCity\0cityName"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScriptEngine[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      52,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  274,    2, 0x06 /* Public */,
       4,    1,  277,    2, 0x06 /* Public */,
       6,    1,  280,    2, 0x06 /* Public */,
       8,    1,  283,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    1,  286,    2, 0x0a /* Public */,
      12,    1,  289,    2, 0x0a /* Public */,
      16,    3,  292,    2, 0x0a /* Public */,
      21,    3,  299,    2, 0x0a /* Public */,
      23,    2,  306,    2, 0x0a /* Public */,
      24,    2,  311,    2, 0x0a /* Public */,
      27,    2,  316,    2, 0x0a /* Public */,
      29,    2,  321,    2, 0x0a /* Public */,
      31,    1,  326,    2, 0x0a /* Public */,
      32,    2,  329,    2, 0x0a /* Public */,
      34,    2,  334,    2, 0x0a /* Public */,
      36,    1,  339,    2, 0x0a /* Public */,
      38,    1,  342,    2, 0x0a /* Public */,
      41,    0,  345,    2, 0x0a /* Public */,
      42,    0,  346,    2, 0x0a /* Public */,
      43,    0,  347,    2, 0x0a /* Public */,
      44,    1,  348,    2, 0x0a /* Public */,
      46,    1,  351,    2, 0x0a /* Public */,
      47,    2,  354,    2, 0x0a /* Public */,
      49,    3,  359,    2, 0x0a /* Public */,
      52,    2,  366,    2, 0x0a /* Public */,
      54,    0,  371,    2, 0x0a /* Public */,
      55,    2,  372,    2, 0x0a /* Public */,
      58,    0,  377,    2, 0x0a /* Public */,
      59,    3,  378,    2, 0x0a /* Public */,
      62,    3,  385,    2, 0x0a /* Public */,
      64,    1,  392,    2, 0x0a /* Public */,
      65,    3,  395,    2, 0x0a /* Public */,
      66,    0,  402,    2, 0x0a /* Public */,
      67,    1,  403,    2, 0x0a /* Public */,
      68,    2,  406,    2, 0x0a /* Public */,
      70,    0,  411,    2, 0x0a /* Public */,
      71,    2,  412,    2, 0x0a /* Public */,
      72,    0,  417,    2, 0x0a /* Public */,
      73,    0,  418,    2, 0x0a /* Public */,
      74,    1,  419,    2, 0x0a /* Public */,
      76,    1,  422,    2, 0x0a /* Public */,
      78,    1,  425,    2, 0x0a /* Public */,
      80,    3,  428,    2, 0x0a /* Public */,
      82,    2,  435,    2, 0x0a /* Public */,
      84,    1,  440,    2, 0x0a /* Public */,
      85,    0,  443,    2, 0x0a /* Public */,
      86,    1,  444,    2, 0x0a /* Public */,
      87,    3,  447,    2, 0x0a /* Public */,
      89,    5,  454,    2, 0x0a /* Public */,
      94,    1,  465,    2, 0x0a /* Public */,

 // methods: name, argc, parameters, tag, flags
      95,    1,  468,    2, 0x02 /* Public */,
      96,    1,  471,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,   11,
    0x80000000 | 13, 0x80000000 | 14,   15,
    0x80000000 | 17, 0x80000000 | 14, 0x80000000 | 14, 0x80000000 | 19,   18,   15,   20,
    0x80000000 | 22, 0x80000000 | 14, 0x80000000 | 14, 0x80000000 | 19,   18,   15,   20,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   18,   15,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   25,   26,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   28,   26,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   30,   26,
    QMetaType::Void, 0x80000000 | 14,   28,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   33,   30,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 14,   33,   35,
    0x80000000 | 37, 0x80000000 | 14,   25,
    0x80000000 | 39, QMetaType::Int,   40,
    0x80000000 | 39,
    0x80000000 | 39,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14,   45,
    QMetaType::Void, 0x80000000 | 14,   45,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Float,   15,   48,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Float, QMetaType::Float,   15,   50,   51,
    QMetaType::Void, 0x80000000 | 14, 0x80000000 | 39,   15,   53,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   56,   57,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Float, QMetaType::Float,   15,   60,   61,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Float, QMetaType::Float,   63,   60,   61,
    QMetaType::Void, 0x80000000 | 14,   63,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Float, QMetaType::Float,    9,   56,   57,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14,    9,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Bool,   45,   69,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   56,   57,
    QMetaType::Double,
    QMetaType::Double,
    QMetaType::Void, 0x80000000 | 14,   75,
    QMetaType::Void, 0x80000000 | 14,   77,
    QMetaType::Void, 0x80000000 | 14,   79,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Double, QMetaType::Double,   81,   56,   57,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Double,   81,   83,
    QMetaType::Void, 0x80000000 | 14,   81,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14,   81,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Double, QMetaType::Double,   88,   56,   57,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   81,   90,   91,   92,   93,
    QMetaType::Void, 0x80000000 | 14,   25,

 // methods: parameters
    QMetaType::Void, 0x80000000 | 14,   81,
    QMetaType::Void, 0x80000000 | 14,   97,

       0        // eod
};

void ScriptEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScriptEngine *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->requestSidebarView((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->requestDisplayTab((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->requestSelectEntity((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->requestSensorScreenshot((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->ScriptSleep((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: { ProfileCategaory* _r = _t->addProfiles((*reinterpret_cast< const std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< ProfileCategaory**>(_a[0]) = std::move(_r); }  break;
        case 6: { Platform* _r = _t->addEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< Platform**>(_a[0]) = std::move(_r); }  break;
        case 7: { Folder* _r = _t->addFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< Folder**>(_a[0]) = std::move(_r); }  break;
        case 8: _t->addComponent((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 9: _t->renameEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 10: _t->renameProfile((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 11: _t->renameFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 12: _t->removeProfile((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 13: _t->removeFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 14: _t->removeEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 15: { Entity* _r = _t->getEntityById((*reinterpret_cast< const std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< Entity**>(_a[0]) = std::move(_r); }  break;
        case 16: { CScriptArray* _r = _t->findEntitiesByType((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 17: { CScriptArray* _r = _t->getAllEntities();
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 18: { CScriptArray* _r = _t->getAllEntityIds();
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 19: _t->renderscene(); break;
        case 20: _t->canvasCreateVectorLayer((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 21: _t->canvasSelectLayer((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 22: _t->canvasAddCircle((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 23: _t->canvasAddRectangle((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 24: _t->canvasAddPolygon((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< CScriptArray*(*)>(_a[2]))); break;
        case 25: _t->canvasStartLine(); break;
        case 26: _t->canvasAddLinePoint((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 27: _t->canvasFinishLine(); break;
        case 28: _t->canvasAddPoint((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 29: _t->onBitmapSelected((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 30: _t->getBitmapImagePath((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 31: _t->onBitmapImageSelected((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 32: _t->canvasToggleAirbases(); break;
        case 33: _t->canvasImportGeoJsonLayer((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 34: _t->canvasToggleGeoJsonLayer((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 35: _t->canvasStartDistanceMeasurement(); break;
        case 36: _t->canvasAddMeasurePoint((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 37: { double _r = _t->canvasGetLastSegmentDistance();
            if (_a[0]) *reinterpret_cast< double*>(_a[0]) = std::move(_r); }  break;
        case 38: { double _r = _t->canvasGetTotalDistance();
            if (_a[0]) *reinterpret_cast< double*>(_a[0]) = std::move(_r); }  break;
        case 39: _t->canvasSetMeasurementUnit((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 40: _t->canvasSwitchMap((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 41: _t->switchCoordinateSystem((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 42: _t->moveShape((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 43: _t->rotateShape((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 44: _t->showShapeHistory((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 45: _t->hideShapeHistory(); break;
        case 46: _t->restoreShapeHistory((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 47: _t->addText((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 48: _t->addShapeProperties((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 49: _t->deleteshape((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 50: _t->setCanvasSelectedShape((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 51: _t->useCity((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScriptEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptEngine::requestSidebarView)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScriptEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptEngine::requestDisplayTab)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScriptEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptEngine::requestSelectEntity)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ScriptEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptEngine::requestSensorScreenshot)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ScriptEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ScriptEngine.data,
    qt_meta_data_ScriptEngine,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScriptEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScriptEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScriptEngine.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ScriptEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 52)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 52;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 52)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 52;
    }
    return _id;
}

// SIGNAL 0
void ScriptEngine::requestSidebarView(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScriptEngine::requestDisplayTab(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScriptEngine::requestSelectEntity(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ScriptEngine::requestSensorScreenshot(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
