#include "designatorhandler.h"
#include <dis7/DesignatorPdu.h>
#include <dis7/utils/DataStream.h>
#include "core/DISPlugin/utils/coordconverter.h"

DesignatorHandler::DesignatorHandler(QObject* parent) : QObject(parent) {}
DesignatorHandler::~DesignatorHandler() = default;
void DesignatorHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void DesignatorHandler::onDesignatorReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::DesignatorPdu pdu;
    DIS::DataStream ds(pduBytes.constData(), static_cast<size_t>(pduBytes.size()), DIS::BIG);
    pdu.unmarshal(ds);

    const DIS::EntityID& entityID = pdu.getDesignatingEntityID();
    const DIS::SimulationAddress& simAddr = entityID.getSimulationAddress();
    uint16_t site = simAddr.getSite(), app = simAddr.getApplication(), entity = entityID.getEntityNumber();

    if (m_mapper && m_mapper->isOwnApplication(site, app)) return;

    std::string engineID = m_mapper ? m_mapper->getEngineID(site, app, entity) : "";
    if (engineID.empty())
        engineID = QString("%1:%2:%3").arg(site).arg(app).arg(entity).toStdString();

    DISIncomingDesignator data;
    data.designatingEntityID  = engineID;
    data.codeName              = pdu.getCodeName();
    data.designatorCode        = pdu.getDesignatorCode();
    data.designatorPower       = pdu.getDesignatorPower();
    data.designatorWavelength  = pdu.getDesignatorWavelength();

    const DIS::EntityID& targetID = pdu.getDesignatedEntityID();
    if (m_mapper) {
        std::string targetEngineID = m_mapper->getEngineID(
            targetID.getSimulationAddress().getSite(),
            targetID.getSimulationAddress().getApplication(),
            targetID.getEntityNumber());
        if (!targetEngineID.empty())
            data.designatedEntityID = targetEngineID;
    }

    ECEFPosition ecef;
    const DIS::Vector3Double& loc = pdu.getDesignatorSpotLocation();
    ecef.x = loc.getX(); ecef.y = loc.getY(); ecef.z = loc.getZ();
    CoordConverter::ecefToGeocord(ecef, data.latitude, data.longitude, data.altitude);

    emit incomingDesignator(data);
}
