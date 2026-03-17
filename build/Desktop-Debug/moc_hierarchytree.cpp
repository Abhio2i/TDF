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
#include <QtCore/QList>
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
    QByteArrayData data[43];
    char stringdata0[681];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_HierarchyTree_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_HierarchyTree_t qt_meta_stringdata_HierarchyTree = {
    {
QT_MOC_LITERAL(0, 0, 13), // "HierarchyTree"
QT_MOC_LITERAL(1, 14, 18), // "copyItemsRequested"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 18), // "QList<QVariantMap>"
QT_MOC_LITERAL(4, 53, 4), // "data"
QT_MOC_LITERAL(5, 58, 19), // "pasteItemsRequested"
QT_MOC_LITERAL(6, 78, 10), // "targetData"
QT_MOC_LITERAL(7, 89, 12), // "itemsToPaste"
QT_MOC_LITERAL(8, 102, 23), // "removeEntitiesRequested"
QT_MOC_LITERAL(9, 126, 30), // "QList<QPair<QString,QString> >"
QT_MOC_LITERAL(10, 157, 14), // "entityInfoList"
QT_MOC_LITERAL(11, 172, 20), // "profileFilterChanged"
QT_MOC_LITERAL(12, 193, 11), // "profileName"
QT_MOC_LITERAL(13, 205, 19), // "searchFilterChanged"
QT_MOC_LITERAL(14, 225, 10), // "searchText"
QT_MOC_LITERAL(15, 236, 12), // "itemSelected"
QT_MOC_LITERAL(16, 249, 13), // "itemsSelected"
QT_MOC_LITERAL(17, 263, 17), // "copyItemRequested"
QT_MOC_LITERAL(18, 281, 18), // "pasteItemRequested"
QT_MOC_LITERAL(19, 300, 24), // "removeComponentRequested"
QT_MOC_LITERAL(20, 325, 8), // "entityID"
QT_MOC_LITERAL(21, 334, 13), // "componentName"
QT_MOC_LITERAL(22, 348, 11), // "itemDropped"
QT_MOC_LITERAL(23, 360, 10), // "sourceData"
QT_MOC_LITERAL(24, 371, 14), // "entitySelected"
QT_MOC_LITERAL(25, 386, 7), // "Entity*"
QT_MOC_LITERAL(26, 394, 6), // "entity"
QT_MOC_LITERAL(27, 401, 21), // "addFormationRequested"
QT_MOC_LITERAL(28, 423, 16), // "selectedEntities"
QT_MOC_LITERAL(29, 440, 22), // "libraryFileNameChanged"
QT_MOC_LITERAL(30, 463, 8), // "fileName"
QT_MOC_LITERAL(31, 472, 26), // "setEntitiesActiveRequested"
QT_MOC_LITERAL(32, 499, 8), // "entities"
QT_MOC_LITERAL(33, 508, 6), // "active"
QT_MOC_LITERAL(34, 515, 28), // "addWeaponToEntitiesRequested"
QT_MOC_LITERAL(35, 544, 28), // "addSensorToEntitiesRequested"
QT_MOC_LITERAL(36, 573, 26), // "addTeamToEntitiesRequested"
QT_MOC_LITERAL(37, 600, 4), // "team"
QT_MOC_LITERAL(38, 605, 19), // "onSearchTextChanged"
QT_MOC_LITERAL(39, 625, 4), // "text"
QT_MOC_LITERAL(40, 630, 22), // "onProfileFilterChanged"
QT_MOC_LITERAL(41, 653, 5), // "index"
QT_MOC_LITERAL(42, 659, 21) // "updateProfileDropdown"

    },
    "HierarchyTree\0copyItemsRequested\0\0"
    "QList<QVariantMap>\0data\0pasteItemsRequested\0"
    "targetData\0itemsToPaste\0removeEntitiesRequested\0"
    "QList<QPair<QString,QString> >\0"
    "entityInfoList\0profileFilterChanged\0"
    "profileName\0searchFilterChanged\0"
    "searchText\0itemSelected\0itemsSelected\0"
    "copyItemRequested\0pasteItemRequested\0"
    "removeComponentRequested\0entityID\0"
    "componentName\0itemDropped\0sourceData\0"
    "entitySelected\0Entity*\0entity\0"
    "addFormationRequested\0selectedEntities\0"
    "libraryFileNameChanged\0fileName\0"
    "setEntitiesActiveRequested\0entities\0"
    "active\0addWeaponToEntitiesRequested\0"
    "addSensorToEntitiesRequested\0"
    "addTeamToEntitiesRequested\0team\0"
    "onSearchTextChanged\0text\0"
    "onProfileFilterChanged\0index\0"
    "updateProfileDropdown"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_HierarchyTree[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      18,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  119,    2, 0x06 /* Public */,
       5,    2,  122,    2, 0x06 /* Public */,
       8,    1,  127,    2, 0x06 /* Public */,
      11,    1,  130,    2, 0x06 /* Public */,
      13,    1,  133,    2, 0x06 /* Public */,
      15,    1,  136,    2, 0x06 /* Public */,
      16,    1,  139,    2, 0x06 /* Public */,
      17,    1,  142,    2, 0x06 /* Public */,
      18,    1,  145,    2, 0x06 /* Public */,
      19,    2,  148,    2, 0x06 /* Public */,
      22,    2,  153,    2, 0x06 /* Public */,
      24,    2,  158,    2, 0x06 /* Public */,
      27,    1,  163,    2, 0x06 /* Public */,
      29,    1,  166,    2, 0x06 /* Public */,
      31,    2,  169,    2, 0x06 /* Public */,
      34,    1,  174,    2, 0x06 /* Public */,
      35,    1,  177,    2, 0x06 /* Public */,
      36,    2,  180,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      38,    1,  185,    2, 0x08 /* Private */,
      40,    1,  188,    2, 0x08 /* Private */,
      42,    0,  191,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QVariantMap, 0x80000000 | 3,    6,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QVariantMap,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QVariantMap,    4,
    QMetaType::Void, QMetaType::QVariantMap,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   20,   21,
    QMetaType::Void, QMetaType::QVariantMap, QMetaType::QVariantMap,   23,    6,
    QMetaType::Void, 0x80000000 | 25, QMetaType::QVariantMap,   26,    4,
    QMetaType::Void, 0x80000000 | 3,   28,
    QMetaType::Void, QMetaType::QString,   30,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool,   32,   33,
    QMetaType::Void, 0x80000000 | 3,   32,
    QMetaType::Void, 0x80000000 | 3,   32,
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString,   32,   37,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,   39,
    QMetaType::Void, QMetaType::Int,   41,
    QMetaType::Void,

       0        // eod
};

void HierarchyTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HierarchyTree *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->copyItemsRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 1: _t->pasteItemsRequested((*reinterpret_cast< QVariantMap(*)>(_a[1])),(*reinterpret_cast< QList<QVariantMap>(*)>(_a[2]))); break;
        case 2: _t->removeEntitiesRequested((*reinterpret_cast< QList<QPair<QString,QString> >(*)>(_a[1]))); break;
        case 3: _t->profileFilterChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->searchFilterChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->itemSelected((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 6: _t->itemsSelected((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 7: _t->copyItemRequested((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 8: _t->pasteItemRequested((*reinterpret_cast< QVariantMap(*)>(_a[1]))); break;
        case 9: _t->removeComponentRequested((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 10: _t->itemDropped((*reinterpret_cast< QVariantMap(*)>(_a[1])),(*reinterpret_cast< QVariantMap(*)>(_a[2]))); break;
        case 11: _t->entitySelected((*reinterpret_cast< Entity*(*)>(_a[1])),(*reinterpret_cast< QVariantMap(*)>(_a[2]))); break;
        case 12: _t->addFormationRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 13: _t->libraryFileNameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->setEntitiesActiveRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 15: _t->addWeaponToEntitiesRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 16: _t->addSensorToEntitiesRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1]))); break;
        case 17: _t->addTeamToEntitiesRequested((*reinterpret_cast< QList<QVariantMap>(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 18: _t->onSearchTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 19: _t->onProfileFilterChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: _t->updateProfileDropdown(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entity* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 15:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 16:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        case 17:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QVariantMap> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::copyItemsRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap , QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::pasteItemsRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QPair<QString,QString>> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::removeEntitiesRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::profileFilterChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::searchFilterChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::itemSelected)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::itemsSelected)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::copyItemRequested)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::pasteItemRequested)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::removeComponentRequested)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QVariantMap , QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::itemDropped)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(Entity * , QVariantMap );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::entitySelected)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::addFormationRequested)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::libraryFileNameChanged)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::setEntitiesActiveRequested)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::addWeaponToEntitiesRequested)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::addSensorToEntitiesRequested)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (HierarchyTree::*)(QList<QVariantMap> , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HierarchyTree::addTeamToEntitiesRequested)) {
                *result = 17;
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
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void HierarchyTree::copyItemsRequested(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void HierarchyTree::pasteItemsRequested(QVariantMap _t1, QList<QVariantMap> _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void HierarchyTree::removeEntitiesRequested(QList<QPair<QString,QString>> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void HierarchyTree::profileFilterChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void HierarchyTree::searchFilterChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void HierarchyTree::itemSelected(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void HierarchyTree::itemsSelected(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void HierarchyTree::copyItemRequested(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void HierarchyTree::pasteItemRequested(QVariantMap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void HierarchyTree::removeComponentRequested(QString _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void HierarchyTree::itemDropped(QVariantMap _t1, QVariantMap _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void HierarchyTree::entitySelected(Entity * _t1, QVariantMap _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void HierarchyTree::addFormationRequested(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void HierarchyTree::libraryFileNameChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void HierarchyTree::setEntitiesActiveRequested(QList<QVariantMap> _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void HierarchyTree::addWeaponToEntitiesRequested(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void HierarchyTree::addSensorToEntitiesRequested(QList<QVariantMap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void HierarchyTree::addTeamToEntitiesRequested(QList<QVariantMap> _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
