#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingEmission.h"

class EmissionHandler : public QObject
{
    Q_OBJECT
public:
    explicit EmissionHandler(QObject* parent = nullptr);
    ~EmissionHandler() override;
    void setMapper(EntityIDMapper* mapper);

public slots:
    void onEmissionReceived(QByteArray pduBytes, QHostAddress sender);

signals:
    void incomingEmission(DISIncomingEmission data);

private:
    EntityIDMapper* m_mapper = nullptr;
};
