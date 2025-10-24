/****************************************************************************
** Meta object code from reading C++ file 'hierarchytree.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Hierarchytree/hierarchytree.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hierarchytree.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_HierarchyTree_t {
    QByteArrayData data[15];
    char stringdata0[182];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_HierarchyTree_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_HierarchyTree_t qt_meta_stringdata_HierarchyTree = {
    {
QT_MOC_LITERAL(0, 0, 13), // "HierarchyTree"
QT_MOC_LITERAL(1, 14, 12), // "itemSelected"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 4), // "data"
QT_MOC_LITERAL(4, 33, 17), // "copyItemRequested"
QT_MOC_LITERAL(5, 51, 18), // "pasteItemRequested"
QT_MOC_LITERAL(6, 70, 10), // "targetData"
QT_MOC_LITERAL(7, 81, 24), // "removeComponentRequested"
QT_MOC_LITERAL(8, 106, 8), // "entityID"
QT_MOC_LITERAL(9, 115, 13), // "componentName"
QT_MOC_LITERAL(10, 129, 11), // "itemDropped"
QT_MOC_LITERAL(11, 141, 10), // "sourceData"
QT_MOC_LITERAL(12, 152, 14), // "entitySelected"
QT_MOC_LITERAL(13, 167, 7), // "Entity*"
QT_MOC_LITERAL(14, 175, 6) // "entity"

    },
    "HierarchyTree\0itemSelected\0\0data\0"
    "copyItemRequested\0pasteItemRequested\0"
    "targetData\0removeComponentRequested\0"
    "entityID\0componentName\0itemDropped\0"
    "sourceData\0entitySelected\0Entity*\0"
    "entity"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_HierarchyTree[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       4,    1,   47,    2, 0x06 /* Public */,
       5,    1,   50,    2, 0x06 /* Public */,
       7,    2,   53,    2, 0x06 /* Public */,
      10,    2,   58,    2, 0x06 /* Public */,
      12,    2,   63,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QVariantMap,    3,
    QMetaType::Void, QMetaType::QVariantMap,    3,
    QMetaType::Void, QMetaType::QVariantMap,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    8,    9,
    QMetaType::Void, QMetaType::QVariantMap, QMetaType::QVariantMap,   11,    6,
    QMetaType::Void, 0x80000000 | 13, QMetaType::QVariantMap,   14,    3,

       0        // eod
};

void HierarchyTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HierarchyTree *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->itemSelected((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 1: _t->copyItemRequested((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 2: _t->pasteItemRequested((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 3: _t->removeComponentRequested((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 4: _t->itemDropped((*reinterpret_cast< QVariantMap(*)>(_a[1])),(*reinterpret_cast< QVariantMap(*)>(_a[2]))); break;
        case 5: _t->entitySelected((*reinterpret_cast< Entity*(*)>(_a[1])),(*reinterpret_cast< QVariantMap(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entity* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::itemSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::copyItemRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::pasteItemRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::removeComponentRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap , QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::itemDropped)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(Entity * , QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::entitySelected)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject HierarchyTree::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_HierarchyTree.data,
    qt_meta_data_HierarchyTree,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *HierarchyTree::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HierarchyTree::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HierarchyTree.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int HierarchyTree::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void HierarchyTree::itemSelected(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void HierarchyTree::copyItemRequested(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void HierarchyTree::pasteItemRequested(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void HierarchyTree::removeComponentRequested(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void HierarchyTree::itemDropped(QVariantMap _t1, QVariantMap _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void HierarchyTree::entitySelected(Entity * _t1, QVariantMap _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
