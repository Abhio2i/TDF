#ifndef NETWOKTRANSPORT_H
#define NETWOKTRANSPORT_H
#include <QObject>

class NetwokTransport: public QObject
{
    Q_OBJECT
public:
    NetwokTransport();

    std::string ip;
    uint port;

    void toJson();
    void fromJson();
};

#endif // NETWOKTRANSPORT_H
