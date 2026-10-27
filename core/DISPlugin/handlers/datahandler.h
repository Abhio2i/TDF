#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingDatum.h"

class DataHandler : public QObject {
    Q_OBJECT
public:
    explicit DataHandler(QObject* parent = nullptr);
    ~DataHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onDataReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingData(DISIncomingDatum data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
