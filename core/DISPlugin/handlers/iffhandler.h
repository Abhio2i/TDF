// =============================================================================
// FILE:        iffhandler.h
// MODULE:      DIS Network Plugin — Handlers
// =============================================================================
#pragma once
#include <QObject>
#include "../interface/disincomingdata.h"
#include "../utils/entityidmapper.h"
#include <QHostAddress>
class IFFHandler : public QObject {
    Q_OBJECT
public:
    explicit IFFHandler(QObject* parent = nullptr);
    void setMapper(EntityIDMapper* mapper);

public slots:
    void onIFFReceived(const QByteArray& data,
                       const QHostAddress& sender,
                       quint16 senderPort);

signals:
    void incomingIFF(DISIncomingIFF iff);

private:
    EntityIDMapper* m_mapper = nullptr;
};
