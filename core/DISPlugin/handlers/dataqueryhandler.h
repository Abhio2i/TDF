#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingDatum.h"

class DataQueryHandler : public QObject {
    Q_OBJECT
public:
    explicit DataQueryHandler(QObject* parent = nullptr);
    ~DataQueryHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onDataQueryReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingDataQuery(DISIncomingDatum data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
