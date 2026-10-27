#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingSignal.h"

class SignalHandler : public QObject {
    Q_OBJECT
public:
    explicit SignalHandler(QObject* parent = nullptr);
    ~SignalHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onSignalReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingSignal(DISIncomingSignal data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
