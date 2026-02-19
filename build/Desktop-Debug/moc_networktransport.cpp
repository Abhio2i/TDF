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
    QByteArrayData data[29];
    char stringdata0[330];
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
QT_MOC_LITERAL(3, 34, 11), // "QWebSocket*"
QT_MOC_LITERAL(4, 46, 7), // "pSocket"
QT_MOC_LITERAL(5, 54, 9), // "onConnect"
QT_MOC_LITERAL(6, 64, 12), // "onDisconnect"
QT_MOC_LITERAL(7, 77, 15), // "onErrorOccurred"
QT_MOC_LITERAL(8, 93, 5), // "error"
QT_MOC_LITERAL(9, 99, 17), // "onReceivedMessage"
QT_MOC_LITERAL(10, 117, 7), // "message"
QT_MOC_LITERAL(11, 125, 15), // "onBinaryMessage"
QT_MOC_LITERAL(12, 141, 11), // "byteMessage"
QT_MOC_LITERAL(13, 153, 12), // "readyUDPRead"
QT_MOC_LITERAL(14, 166, 13), // "NewConnection"
QT_MOC_LITERAL(15, 180, 9), // "Connected"
QT_MOC_LITERAL(16, 190, 12), // "Disconnected"
QT_MOC_LITERAL(17, 203, 13), // "ErrorOccurred"
QT_MOC_LITERAL(18, 217, 16), // "QList<QSslError>"
QT_MOC_LITERAL(19, 234, 6), // "errors"
QT_MOC_LITERAL(20, 241, 15), // "ReceivedMessage"
QT_MOC_LITERAL(21, 257, 13), // "BinaryMessage"
QT_MOC_LITERAL(22, 271, 4), // "init"
QT_MOC_LITERAL(23, 276, 2), // "ip"
QT_MOC_LITERAL(24, 279, 4), // "port"
QT_MOC_LITERAL(25, 284, 5), // "start"
QT_MOC_LITERAL(26, 290, 6), // "server"
QT_MOC_LITERAL(27, 297, 11), // "sendMessage"
QT_MOC_LITERAL(28, 309, 20) // "sendBinaryUDPMessage"

    },
    "NetworkTransport\0onNewConnection\0\0"
    "QWebSocket*\0pSocket\0onConnect\0"
    "onDisconnect\0onErrorOccurred\0error\0"
    "onReceivedMessage\0message\0onBinaryMessage\0"
    "byteMessage\0readyUDPRead\0NewConnection\0"
    "Connected\0Disconnected\0ErrorOccurred\0"
    "QList<QSslError>\0errors\0ReceivedMessage\0"
    "BinaryMessage\0init\0ip\0port\0start\0"
    "server\0sendMessage\0sendBinaryUDPMessage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NetworkTransport[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   99,    2, 0x06 /* Public */,
       5,    0,  102,    2, 0x06 /* Public */,
       6,    0,  103,    2, 0x06 /* Public */,
       7,    1,  104,    2, 0x06 /* Public */,
       9,    1,  107,    2, 0x06 /* Public */,
      11,    1,  110,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,  113,    2, 0x08 /* Private */,
      14,    0,  114,    2, 0x08 /* Private */,
      15,    0,  115,    2, 0x08 /* Private */,
      16,    0,  116,    2, 0x08 /* Private */,
      17,    1,  117,    2, 0x08 /* Private */,
      20,    1,  120,    2, 0x08 /* Private */,
      21,    1,  123,    2, 0x08 /* Private */,
      22,    2,  126,    2, 0x0a /* Public */,
      25,    1,  131,    2, 0x0a /* Public */,
      27,    1,  134,    2, 0x0a /* Public */,
      28,    1,  137,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QByteArray,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 18,   19,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QByteArray,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   23,   24,
    QMetaType::Void, QMetaType::Bool,   26,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QByteArray,   12,

       0        // eod
};

void NetworkTransport::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkTransport *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onNewConnection((*reinterpret_cast< QWebSocket*(*)>(_a[1]))); break;
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
        case 13: _t->init((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 14: _t->start((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->sendMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: _t->sendBinaryUDPMessage((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QWebSocket* >(); break;
            }
            break;
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
            using _t = void (NetworkTransport::*)(QWebSocket * );
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
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void NetworkTransport::onNewConnection(QWebSocket * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
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
