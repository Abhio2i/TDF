/****************************************************************************
** Meta object code from reading C++ file 'gislib.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../GUI/Tacticaldisplay/Gis/gislib.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gislib.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GISlib_t {
    QByteArrayData data[32];
    char stringdata0[332];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GISlib_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GISlib_t qt_meta_stringdata_GISlib = {
    {
QT_MOC_LITERAL(0, 0, 6), // "GISlib"
QT_MOC_LITERAL(1, 7, 10), // "mouseCords"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 3), // "lat"
QT_MOC_LITERAL(4, 23, 3), // "lon"
QT_MOC_LITERAL(5, 27, 5), // "crsId"
QT_MOC_LITERAL(6, 33, 13), // "centerChanged"
QT_MOC_LITERAL(7, 47, 11), // "zoomChanged"
QT_MOC_LITERAL(8, 59, 4), // "zoom"
QT_MOC_LITERAL(9, 64, 10), // "keyPressed"
QT_MOC_LITERAL(10, 75, 10), // "QKeyEvent*"
QT_MOC_LITERAL(11, 86, 5), // "event"
QT_MOC_LITERAL(12, 92, 12), // "mousePressed"
QT_MOC_LITERAL(13, 105, 12), // "QMouseEvent*"
QT_MOC_LITERAL(14, 118, 10), // "mouseMoved"
QT_MOC_LITERAL(15, 129, 13), // "mouseReleased"
QT_MOC_LITERAL(16, 143, 7), // "painted"
QT_MOC_LITERAL(17, 151, 12), // "QPaintEvent*"
QT_MOC_LITERAL(18, 164, 16), // "distanceMeasured"
QT_MOC_LITERAL(19, 181, 8), // "distance"
QT_MOC_LITERAL(20, 190, 10), // "startPoint"
QT_MOC_LITERAL(21, 201, 8), // "endPoint"
QT_MOC_LITERAL(22, 210, 15), // "dragEnterEvents"
QT_MOC_LITERAL(23, 226, 16), // "QDragEnterEvent*"
QT_MOC_LITERAL(24, 243, 14), // "dragMoveEvents"
QT_MOC_LITERAL(25, 258, 15), // "QDragMoveEvent*"
QT_MOC_LITERAL(26, 274, 10), // "dropEvents"
QT_MOC_LITERAL(27, 285, 11), // "QDropEvent*"
QT_MOC_LITERAL(28, 297, 12), // "receiveImage"
QT_MOC_LITERAL(29, 310, 3), // "url"
QT_MOC_LITERAL(30, 314, 4), // "data"
QT_MOC_LITERAL(31, 319, 12) // "receivePlace"

    },
    "GISlib\0mouseCords\0\0lat\0lon\0crsId\0"
    "centerChanged\0zoomChanged\0zoom\0"
    "keyPressed\0QKeyEvent*\0event\0mousePressed\0"
    "QMouseEvent*\0mouseMoved\0mouseReleased\0"
    "painted\0QPaintEvent*\0distanceMeasured\0"
    "distance\0startPoint\0endPoint\0"
    "dragEnterEvents\0QDragEnterEvent*\0"
    "dragMoveEvents\0QDragMoveEvent*\0"
    "dropEvents\0QDropEvent*\0receiveImage\0"
    "url\0data\0receivePlace"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GISlib[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   84,    2, 0x06 /* Public */,
       6,    2,   91,    2, 0x06 /* Public */,
       7,    1,   96,    2, 0x06 /* Public */,
       9,    1,   99,    2, 0x06 /* Public */,
      12,    1,  102,    2, 0x06 /* Public */,
      14,    1,  105,    2, 0x06 /* Public */,
      15,    1,  108,    2, 0x06 /* Public */,
      16,    1,  111,    2, 0x06 /* Public */,
      18,    3,  114,    2, 0x06 /* Public */,
      22,    1,  121,    2, 0x06 /* Public */,
      24,    1,  124,    2, 0x06 /* Public */,
      26,    1,  127,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      28,    2,  130,    2, 0x0a /* Public */,
      31,    2,  135,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::QString,    3,    4,    5,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    3,    4,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, 0x80000000 | 13,   11,
    QMetaType::Void, 0x80000000 | 13,   11,
    QMetaType::Void, 0x80000000 | 13,   11,
    QMetaType::Void, 0x80000000 | 17,   11,
    QMetaType::Void, QMetaType::Double, QMetaType::QPointF, QMetaType::QPointF,   19,   20,   21,
    QMetaType::Void, 0x80000000 | 23,   11,
    QMetaType::Void, 0x80000000 | 25,   11,
    QMetaType::Void, 0x80000000 | 27,   11,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QByteArray,   29,   30,
    QMetaType::Void, QMetaType::QString, QMetaType::QByteArray,   29,   30,

       0        // eod
};

void GISlib::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GISlib *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->mouseCords((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 1: _t->centerChanged((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 2: _t->zoomChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->keyPressed((*reinterpret_cast< QKeyEvent*(*)>(_a[1]))); break;
        case 4: _t->mousePressed((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 5: _t->mouseMoved((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 6: _t->mouseReleased((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 7: _t->painted((*reinterpret_cast< QPaintEvent*(*)>(_a[1]))); break;
        case 8: _t->distanceMeasured((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< QPointF(*)>(_a[2])),(*reinterpret_cast< QPointF(*)>(_a[3]))); break;
        case 9: _t->dragEnterEvents((*reinterpret_cast< QDragEnterEvent*(*)>(_a[1]))); break;
        case 10: _t->dragMoveEvents((*reinterpret_cast< QDragMoveEvent*(*)>(_a[1]))); break;
        case 11: _t->dropEvents((*reinterpret_cast< QDropEvent*(*)>(_a[1]))); break;
        case 12: _t->receiveImage((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QByteArray(*)>(_a[2]))); break;
        case 13: _t->receivePlace((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QByteArray(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QKeyEvent* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMouseEvent* >(); break;
            }
            break;
        case 6:
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
            using _t = void (GISlib::*)(double , double , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::mouseCords)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::centerChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::zoomChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QKeyEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::keyPressed)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QMouseEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::mousePressed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QMouseEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::mouseMoved)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QMouseEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::mouseReleased)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QPaintEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::painted)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(double , QPointF , QPointF );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::distanceMeasured)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QDragEnterEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::dragEnterEvents)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QDragMoveEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::dragMoveEvents)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (GISlib::*)(QDropEvent * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GISlib::dropEvents)) {
                *result = 11;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject GISlib::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_GISlib.data,
    qt_meta_data_GISlib,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *GISlib::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GISlib::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GISlib.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int GISlib::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void GISlib::mouseCords(double _t1, double _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GISlib::centerChanged(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void GISlib::zoomChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void GISlib::keyPressed(QKeyEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void GISlib::mousePressed(QMouseEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void GISlib::mouseMoved(QMouseEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GISlib::mouseReleased(QMouseEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void GISlib::painted(QPaintEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void GISlib::distanceMeasured(double _t1, QPointF _t2, QPointF _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void GISlib::dragEnterEvents(QDragEnterEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void GISlib::dragMoveEvents(QDragMoveEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void GISlib::dropEvents(QDropEvent * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
