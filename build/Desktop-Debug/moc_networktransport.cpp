/****************************************************************************
** Meta object code from reading C++ file 'networktransport.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../core/Network/networktransport.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'networktransport.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NetworkTransport_t {
    QByteArrayData data[20];
    char stringdata0[251];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NetworkTransport_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NetworkTransport_t qt_meta_stringdata_NetworkTransport = {
    {
QT_MOC_LITERAL(0, 0, 16), // "NetworkTransport"
QT_MOC_LITERAL(1, 17, 15), // "onNewConnection"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 9), // "onConnect"
QT_MOC_LITERAL(4, 44, 12), // "onDisconnect"
QT_MOC_LITERAL(5, 57, 15), // "onErrorOccurred"
QT_MOC_LITERAL(6, 73, 5), // "error"
QT_MOC_LITERAL(7, 79, 17), // "onReceivedMessage"
QT_MOC_LITERAL(8, 97, 7), // "message"
QT_MOC_LITERAL(9, 105, 15), // "onBinaryMessage"
QT_MOC_LITERAL(10, 121, 11), // "byteMessage"
QT_MOC_LITERAL(11, 133, 12), // "readyUDPRead"
QT_MOC_LITERAL(12, 146, 13), // "NewConnection"
QT_MOC_LITERAL(13, 160, 9), // "Connected"
QT_MOC_LITERAL(14, 170, 12), // "Disconnected"
QT_MOC_LITERAL(15, 183, 13), // "ErrorOccurred"
QT_MOC_LITERAL(16, 197, 16), // "QList<QSslError>"
QT_MOC_LITERAL(17, 214, 6), // "errors"
QT_MOC_LITERAL(18, 221, 15), // "ReceivedMessage"
QT_MOC_LITERAL(19, 237, 13) // "BinaryMessage"

    },
    "NetworkTransport\0onNewConnection\0\0"
    "onConnect\0onDisconnect\0onErrorOccurred\0"
    "error\0onReceivedMessage\0message\0"
    "onBinaryMessage\0byteMessage\0readyUDPRead\0"
    "NewConnection\0Connected\0Disconnected\0"
    "ErrorOccurred\0QList<QSslError>\0errors\0"
    "ReceivedMessage\0BinaryMessage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NetworkTransport[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x06 /* Public */,
       3,    0,   80,    2, 0x06 /* Public */,
       4,    0,   81,    2, 0x06 /* Public */,
       5,    1,   82,    2, 0x06 /* Public */,
       7,    1,   85,    2, 0x06 /* Public */,
       9,    1,   88,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   91,    2, 0x08 /* Private */,
      12,    0,   92,    2, 0x08 /* Private */,
      13,    0,   93,    2, 0x08 /* Private */,
      14,    0,   94,    2, 0x08 /* Private */,
      15,    1,   95,    2, 0x08 /* Private */,
      18,    1,   98,    2, 0x08 /* Private */,
      19,    1,  101,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QByteArray,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16,   17,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QByteArray,   10,

       0        // eod
};

void NetworkTransport::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkTransport *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onNewConnection(); break;
        case 1: _t->onConnect(); break;
        case 2: _t->onDisconnect(); break;
        case 3: _t->onErrorOccurred((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->onReceivedMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->onBinaryMessage((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        case 6: _t->readyUDPRead(); break;
        case 7: _t->NewConnection(); break;
        case 8: _t->Connected(); break;
        case 9: _t->Disconnected(); break;
        case 10: _t->ErrorOccurred((*reinterpret_cast< const QList<QSslError>(*)>(_a[1]))); break;
        case 11: _t->ReceivedMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 12: _t->BinaryMessage((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QSslError> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NetworkTransport::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onNewConnection)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkTransport::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onConnect)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkTransport::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onDisconnect)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkTransport::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onErrorOccurred)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkTransport::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onReceivedMessage)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NetworkTransport::*)(QByteArray );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkTransport::onBinaryMessage)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NetworkTransport::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_NetworkTransport.data,
    qt_meta_data_NetworkTransport,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NetworkTransport::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkTransport::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NetworkTransport.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkTransport::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void NetworkTransport::onNewConnection()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NetworkTransport::onConnect()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkTransport::onDisconnect()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NetworkTransport::onErrorOccurred(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkTransport::onReceivedMessage(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NetworkTransport::onBinaryMessage(QByteArray _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
