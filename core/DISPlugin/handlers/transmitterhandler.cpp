#include "transmitterhandler.h"
#include <dis7/transmitterpdu.h>
#include <dis7/utils/DataStream.h>
#include "../core/DISPlugin/utils/coordconverter.h"

TransmitterHandler::TransmitterHandler(QObject* parent) : QObject(parent) {}
TransmitterHandler::~TransmitterHandler() = default;
void TransmitterHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void TransmitterHandler::onTransmitterReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::TransmitterPdu pdu;
    DIS::DataStream ds(pduBytes.constData(), static_cast<size_t>(pduBytes.size()), DIS::BIG);
    pdu.unmarshal(ds);

    const DIS::EntityID& entityID = pdu.getEntityID();
    const DIS::SimulationAddress& simAddr = entityID.getSimulationAddress();
    uint16_t site = simAddr.getSite(), app = simAddr.getApplication(), entity = entityID.getEntityNumber();

    if (m_mapper && m_mapper->isOwnApplication(site, app)) return;

    std::string engineID = m_mapper ? m_mapper->getEngineID(site, app, entity) : "";
    if (engineID.empty())
        engineID = QString("%1:%2:%3").arg(site).arg(app).arg(entity).toStdString();

    DISIncomingTransmitter data;
    data.transmittingEntityID = engineID;
    data.radioID       = pdu.getRadioID();
    data.transmitState = pdu.getTransmitState();
    data.frequency      = pdu.getFrequency();
    data.power          = pdu.getPower();

    ECEFPosition ecef;
    const DIS::Vector3Double& loc = pdu.getAntennaLocation();
    ecef.x = loc.getX(); ecef.y = loc.getY(); ecef.z = loc.getZ();
    CoordConverter::ecefToGeocord(ecef, data.latitude, data.longitude, data.altitude);

    emit incomingTransmitter(data);
}
