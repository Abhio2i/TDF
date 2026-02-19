/****************************************************************************
** Meta object code from reading C++ file 'textscriptwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Testscript/textscriptwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'textscriptwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TextScriptItemWidget_t {
    QByteArrayData data[5];
    char stringdata0[56];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TextScriptItemWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TextScriptItemWidget_t qt_meta_stringdata_TextScriptItemWidget = {
    {
QT_MOC_LITERAL(0, 0, 20), // "TextScriptItemWidget"
QT_MOC_LITERAL(1, 21, 11), // "playClicked"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 8), // "filePath"
QT_MOC_LITERAL(4, 43, 12) // "pauseClicked"

    },
    "TextScriptItemWidget\0playClicked\0\0"
    "filePath\0pauseClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TextScriptItemWidget[] = {

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
       1,    1,   24,    2, 0x06 /* Public */,
       4,    1,   27,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,

       0        // eod
};

void TextScriptItemWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TextScriptItemWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->playClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->pauseClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TextScriptItemWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptItemWidget::playClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TextScriptItemWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptItemWidget::pauseClicked)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TextScriptItemWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TextScriptItemWidget.data,
    qt_meta_data_TextScriptItemWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TextScriptItemWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextScriptItemWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TextScriptItemWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TextScriptItemWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void TextScriptItemWidget::playClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TextScriptItemWidget::pauseClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_TextScriptWidget_t {
    QByteArrayData data[20];
    char stringdata0[283];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TextScriptWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TextScriptWidget_t qt_meta_stringdata_TextScriptWidget = {
    {
QT_MOC_LITERAL(0, 0, 16), // "TextScriptWidget"
QT_MOC_LITERAL(1, 17, 15), // "runScriptstring"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 4), // "code"
QT_MOC_LITERAL(4, 39, 9), // "runScript"
QT_MOC_LITERAL(5, 49, 8), // "filePath"
QT_MOC_LITERAL(6, 58, 11), // "pauseScript"
QT_MOC_LITERAL(7, 70, 12), // "renameScript"
QT_MOC_LITERAL(8, 83, 7), // "newName"
QT_MOC_LITERAL(9, 91, 12), // "removeScript"
QT_MOC_LITERAL(10, 104, 13), // "runScriptFile"
QT_MOC_LITERAL(11, 118, 15), // "pauseScriptFile"
QT_MOC_LITERAL(12, 134, 23), // "handleCustomContextMenu"
QT_MOC_LITERAL(13, 158, 3), // "pos"
QT_MOC_LITERAL(14, 162, 18), // "handleRenameAction"
QT_MOC_LITERAL(15, 181, 18), // "handleRemoveAction"
QT_MOC_LITERAL(16, 200, 16), // "handleEditAction"
QT_MOC_LITERAL(17, 217, 17), // "handlePlayClicked"
QT_MOC_LITERAL(18, 235, 18), // "handlePauseClicked"
QT_MOC_LITERAL(19, 254, 28) // "handleAddScriptButtonClicked"

    },
    "TextScriptWidget\0runScriptstring\0\0"
    "code\0runScript\0filePath\0pauseScript\0"
    "renameScript\0newName\0removeScript\0"
    "runScriptFile\0pauseScriptFile\0"
    "handleCustomContextMenu\0pos\0"
    "handleRenameAction\0handleRemoveAction\0"
    "handleEditAction\0handlePlayClicked\0"
    "handlePauseClicked\0handleAddScriptButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TextScriptWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   84,    2, 0x06 /* Public */,
       4,    1,   87,    2, 0x06 /* Public */,
       6,    1,   90,    2, 0x06 /* Public */,
       7,    2,   93,    2, 0x06 /* Public */,
       9,    1,   98,    2, 0x06 /* Public */,
      10,    1,  101,    2, 0x06 /* Public */,
      11,    1,  104,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,  107,    2, 0x08 /* Private */,
      14,    0,  110,    2, 0x08 /* Private */,
      15,    0,  111,    2, 0x08 /* Private */,
      16,    0,  112,    2, 0x08 /* Private */,
      17,    1,  113,    2, 0x08 /* Private */,
      18,    1,  116,    2, 0x08 /* Private */,
      19,    0,  119,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    5,    8,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,

       0        // eod
};

void TextScriptWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TextScriptWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->runScriptstring((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->runScript((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->pauseScript((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->renameScript((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->removeScript((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->runScriptFile((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->pauseScriptFile((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->handleCustomContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 8: _t->handleRenameAction(); break;
        case 9: _t->handleRemoveAction(); break;
        case 10: _t->handleEditAction(); break;
        case 11: _t->handlePlayClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->handlePauseClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->handleAddScriptButtonClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TextScriptWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::runScriptstring)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::runScript)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::pauseScript)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::renameScript)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::removeScript)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::runScriptFile)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (TextScriptWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TextScriptWidget::pauseScriptFile)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TextScriptWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TextScriptWidget.data,
    qt_meta_data_TextScriptWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TextScriptWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextScriptWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TextScriptWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TextScriptWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void TextScriptWidget::runScriptstring(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TextScriptWidget::runScript(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TextScriptWidget::pauseScript(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TextScriptWidget::renameScript(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void TextScriptWidget::removeScript(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void TextScriptWidget::runScriptFile(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void TextScriptWidget::pauseScriptFile(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
