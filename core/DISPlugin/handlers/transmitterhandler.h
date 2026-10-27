#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingTransmitter.h"

class TransmitterHandler : public QObject {
    Q_OBJECT
public:
    explicit TransmitterHandler(QObject* parent = nullptr);
    ~TransmitterHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onTransmitterReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingTransmitter(DISIncomingTransmitter data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
