#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingDatum.h"

class SetDataHandler : public QObject {
    Q_OBJECT
public:
    explicit SetDataHandler(QObject* parent = nullptr);
    ~SetDataHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onSetDataReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingSetData(DISIncomingDatum data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
