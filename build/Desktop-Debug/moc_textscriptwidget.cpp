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
    QByteArrayData data[18];
    char stringdata0[253];
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
QT_MOC_LITERAL(10, 104, 23), // "handleCustomContextMenu"
QT_MOC_LITERAL(11, 128, 3), // "pos"
QT_MOC_LITERAL(12, 132, 18), // "handleRenameAction"
QT_MOC_LITERAL(13, 151, 18), // "handleRemoveAction"
QT_MOC_LITERAL(14, 170, 16), // "handleEditAction"
QT_MOC_LITERAL(15, 187, 17), // "handlePlayClicked"
QT_MOC_LITERAL(16, 205, 18), // "handlePauseClicked"
QT_MOC_LITERAL(17, 224, 28) // "handleAddScriptButtonClicked"

    },
    "TextScriptWidget\0runScriptstring\0\0"
    "code\0runScript\0filePath\0pauseScript\0"
    "renameScript\0newName\0removeScript\0"
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
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   74,    2, 0x06 /* Public */,
       4,    1,   77,    2, 0x06 /* Public */,
       6,    1,   80,    2, 0x06 /* Public */,
       7,    2,   83,    2, 0x06 /* Public */,
       9,    1,   88,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    1,   91,    2, 0x08 /* Private */,
      12,    0,   94,    2, 0x08 /* Private */,
      13,    0,   95,    2, 0x08 /* Private */,
      14,    0,   96,    2, 0x08 /* Private */,
      15,    1,   97,    2, 0x08 /* Private */,
      16,    1,  100,    2, 0x08 /* Private */,
      17,    0,  103,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    5,    8,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,   11,
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
        case 5: _t->handleCustomContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 6: _t->handleRenameAction(); break;
        case 7: _t->handleRemoveAction(); break;
        case 8: _t->handleEditAction(); break;
        case 9: _t->handlePlayClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->handlePauseClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->handleAddScriptButtonClicked(); break;
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
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
