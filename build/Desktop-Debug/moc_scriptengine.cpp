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
    QByteArrayData data[58];
    char stringdata0[676];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScriptEngine_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScriptEngine_t qt_meta_stringdata_ScriptEngine = {
    {
QT_MOC_LITERAL(0, 0, 12), // "ScriptEngine"
QT_MOC_LITERAL(1, 13, 11), // "ScriptSleep"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 12), // "milliseconds"
QT_MOC_LITERAL(4, 39, 11), // "addProfiles"
QT_MOC_LITERAL(5, 51, 17), // "ProfileCategaory*"
QT_MOC_LITERAL(6, 69, 11), // "std::string"
QT_MOC_LITERAL(7, 81, 4), // "name"
QT_MOC_LITERAL(8, 86, 9), // "addEntity"
QT_MOC_LITERAL(9, 96, 9), // "Platform*"
QT_MOC_LITERAL(10, 106, 2), // "Id"
QT_MOC_LITERAL(11, 109, 5), // "bool&"
QT_MOC_LITERAL(12, 115, 7), // "profile"
QT_MOC_LITERAL(13, 123, 9), // "addFolder"
QT_MOC_LITERAL(14, 133, 7), // "Folder*"
QT_MOC_LITERAL(15, 141, 12), // "addComponent"
QT_MOC_LITERAL(16, 154, 12), // "renameEntity"
QT_MOC_LITERAL(17, 167, 2), // "id"
QT_MOC_LITERAL(18, 170, 7), // "newName"
QT_MOC_LITERAL(19, 178, 13), // "renameProfile"
QT_MOC_LITERAL(20, 192, 9), // "profileID"
QT_MOC_LITERAL(21, 202, 12), // "renameFolder"
QT_MOC_LITERAL(22, 215, 8), // "folderID"
QT_MOC_LITERAL(23, 224, 13), // "removeProfile"
QT_MOC_LITERAL(24, 238, 12), // "removeFolder"
QT_MOC_LITERAL(25, 251, 8), // "parentId"
QT_MOC_LITERAL(26, 260, 12), // "removeEntity"
QT_MOC_LITERAL(27, 273, 8), // "entityID"
QT_MOC_LITERAL(28, 282, 13), // "getEntityById"
QT_MOC_LITERAL(29, 296, 7), // "Entity*"
QT_MOC_LITERAL(30, 304, 18), // "findEntitiesByType"
QT_MOC_LITERAL(31, 323, 13), // "CScriptArray*"
QT_MOC_LITERAL(32, 337, 4), // "type"
QT_MOC_LITERAL(33, 342, 14), // "getAllEntities"
QT_MOC_LITERAL(34, 357, 15), // "getAllEntityIds"
QT_MOC_LITERAL(35, 373, 11), // "renderscene"
QT_MOC_LITERAL(36, 385, 22), // "setCanvasSelectedShape"
QT_MOC_LITERAL(37, 408, 9), // "shapeName"
QT_MOC_LITERAL(38, 418, 13), // "canvasAddLine"
QT_MOC_LITERAL(39, 432, 6), // "points"
QT_MOC_LITERAL(40, 439, 16), // "canvasAddPolygon"
QT_MOC_LITERAL(41, 456, 18), // "canvasAddRectangle"
QT_MOC_LITERAL(42, 475, 1), // "x"
QT_MOC_LITERAL(43, 477, 1), // "y"
QT_MOC_LITERAL(44, 479, 5), // "width"
QT_MOC_LITERAL(45, 485, 6), // "height"
QT_MOC_LITERAL(46, 492, 15), // "canvasAddCircle"
QT_MOC_LITERAL(47, 508, 6), // "radius"
QT_MOC_LITERAL(48, 515, 14), // "canvasAddPoint"
QT_MOC_LITERAL(49, 530, 16), // "onBitmapSelected"
QT_MOC_LITERAL(50, 547, 10), // "bitmapType"
QT_MOC_LITERAL(51, 558, 18), // "getBitmapImagePath"
QT_MOC_LITERAL(52, 577, 21), // "onBitmapImageSelected"
QT_MOC_LITERAL(53, 599, 8), // "filePath"
QT_MOC_LITERAL(54, 608, 24), // "canvasImportGeoJsonLayer"
QT_MOC_LITERAL(55, 633, 24), // "canvasToggleGeoJsonLayer"
QT_MOC_LITERAL(56, 658, 9), // "layerName"
QT_MOC_LITERAL(57, 668, 7) // "visible"

    },
    "ScriptEngine\0ScriptSleep\0\0milliseconds\0"
    "addProfiles\0ProfileCategaory*\0std::string\0"
    "name\0addEntity\0Platform*\0Id\0bool&\0"
    "profile\0addFolder\0Folder*\0addComponent\0"
    "renameEntity\0id\0newName\0renameProfile\0"
    "profileID\0renameFolder\0folderID\0"
    "removeProfile\0removeFolder\0parentId\0"
    "removeEntity\0entityID\0getEntityById\0"
    "Entity*\0findEntitiesByType\0CScriptArray*\0"
    "type\0getAllEntities\0getAllEntityIds\0"
    "renderscene\0setCanvasSelectedShape\0"
    "shapeName\0canvasAddLine\0points\0"
    "canvasAddPolygon\0canvasAddRectangle\0"
    "x\0y\0width\0height\0canvasAddCircle\0"
    "radius\0canvasAddPoint\0onBitmapSelected\0"
    "bitmapType\0getBitmapImagePath\0"
    "onBitmapImageSelected\0filePath\0"
    "canvasImportGeoJsonLayer\0"
    "canvasToggleGeoJsonLayer\0layerName\0"
    "visible"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScriptEngine[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,  149,    2, 0x0a /* Public */,
       4,    1,  152,    2, 0x0a /* Public */,
       8,    3,  155,    2, 0x0a /* Public */,
      13,    3,  162,    2, 0x0a /* Public */,
      15,    2,  169,    2, 0x0a /* Public */,
      16,    2,  174,    2, 0x0a /* Public */,
      19,    2,  179,    2, 0x0a /* Public */,
      21,    2,  184,    2, 0x0a /* Public */,
      23,    1,  189,    2, 0x0a /* Public */,
      24,    2,  192,    2, 0x0a /* Public */,
      26,    2,  197,    2, 0x0a /* Public */,
      28,    1,  202,    2, 0x0a /* Public */,
      30,    1,  205,    2, 0x0a /* Public */,
      33,    0,  208,    2, 0x0a /* Public */,
      34,    0,  209,    2, 0x0a /* Public */,
      35,    0,  210,    2, 0x0a /* Public */,

 // methods: name, argc, parameters, tag, flags
      36,    1,  211,    2, 0x02 /* Public */,
      38,    2,  214,    2, 0x02 /* Public */,
      40,    2,  219,    2, 0x02 /* Public */,
      41,    5,  224,    2, 0x02 /* Public */,
      46,    4,  235,    2, 0x02 /* Public */,
      48,    3,  244,    2, 0x02 /* Public */,
      49,    1,  251,    2, 0x02 /* Public */,
      51,    1,  254,    2, 0x02 /* Public */,
      52,    1,  257,    2, 0x02 /* Public */,
      54,    1,  260,    2, 0x02 /* Public */,
      55,    2,  263,    2, 0x02 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    0x80000000 | 5, 0x80000000 | 6,    7,
    0x80000000 | 9, 0x80000000 | 6, 0x80000000 | 6, 0x80000000 | 11,   10,    7,   12,
    0x80000000 | 14, 0x80000000 | 6, 0x80000000 | 6, 0x80000000 | 11,   10,    7,   12,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   10,    7,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   17,   18,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   20,   18,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   22,   18,
    QMetaType::Void, 0x80000000 | 6,   20,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   25,   22,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   25,   27,
    0x80000000 | 29, 0x80000000 | 6,   17,
    0x80000000 | 31, QMetaType::Int,   32,
    0x80000000 | 31,
    0x80000000 | 31,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, 0x80000000 | 6,   37,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 31,    7,   39,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 31,    7,   39,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float,    7,   42,   43,   44,   45,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Float, QMetaType::Float, QMetaType::Float,    7,   42,   43,   47,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Float, QMetaType::Float,    7,   42,   43,
    QMetaType::Void, 0x80000000 | 6,   50,
    0x80000000 | 6, 0x80000000 | 6,   50,
    QMetaType::Void, 0x80000000 | 6,   53,
    QMetaType::Void, 0x80000000 | 6,   53,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Bool,   56,   57,

       0        // eod
};

void ScriptEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScriptEngine *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ScriptSleep((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: { ProfileCategaory* _r = _t->addProfiles((*reinterpret_cast< const std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< ProfileCategaory**>(_a[0]) = std::move(_r); }  break;
        case 2: { Platform* _r = _t->addEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< Platform**>(_a[0]) = std::move(_r); }  break;
        case 3: { Folder* _r = _t->addFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< Folder**>(_a[0]) = std::move(_r); }  break;
        case 4: _t->addComponent((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 5: _t->renameEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 6: _t->renameProfile((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 7: _t->renameFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 8: _t->removeProfile((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 9: _t->removeFolder((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 10: _t->removeEntity((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 11: { Entity* _r = _t->getEntityById((*reinterpret_cast< const std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< Entity**>(_a[0]) = std::move(_r); }  break;
        case 12: { CScriptArray* _r = _t->findEntitiesByType((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 13: { CScriptArray* _r = _t->getAllEntities();
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 14: { CScriptArray* _r = _t->getAllEntityIds();
            if (_a[0]) *reinterpret_cast< CScriptArray**>(_a[0]) = std::move(_r); }  break;
        case 15: _t->renderscene(); break;
        case 16: _t->setCanvasSelectedShape((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 17: _t->canvasAddLine((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< CScriptArray*(*)>(_a[2]))); break;
        case 18: _t->canvasAddPolygon((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< CScriptArray*(*)>(_a[2]))); break;
        case 19: _t->canvasAddRectangle((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5]))); break;
        case 20: _t->canvasAddCircle((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4]))); break;
        case 21: _t->canvasAddPoint((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 22: _t->onBitmapSelected((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 23: { std::string _r = _t->getBitmapImagePath((*reinterpret_cast< const std::string(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 24: _t->onBitmapImageSelected((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 25: _t->canvasImportGeoJsonLayer((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 26: _t->canvasToggleGeoJsonLayer((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
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
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
