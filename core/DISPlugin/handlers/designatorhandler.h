#pragma once
#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include "../utils/entityidmapper.h"
#include "../interface/DISIncomingDesignator.h"

class DesignatorHandler : public QObject {
    Q_OBJECT
public:
    explicit DesignatorHandler(QObject* parent = nullptr);
    ~DesignatorHandler() override;
    void setMapper(EntityIDMapper* mapper);
public slots:
    void onDesignatorReceived(QByteArray pduBytes, QHostAddress sender);
signals:
    void incomingDesignator(DISIncomingDesignator data);
private:
    EntityIDMapper* m_mapper = nullptr;
};
