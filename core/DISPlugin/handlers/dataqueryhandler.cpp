#include "dataqueryhandler.h"
#include <dis7/DataQueryPdu.h>
#include <dis7/utils/DataStream.h>

DataQueryHandler::DataQueryHandler(QObject* parent) : QObject(parent) {}
DataQueryHandler::~DataQueryHandler() = default;
void DataQueryHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void DataQueryHandler::onDataQueryReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::DataQueryPdu pdu;
    DIS::DataStream ds(pduBytes.constData(), static_cast<size_t>(pduBytes.size()), DIS::BIG);
    pdu.unmarshal(ds);

    const DIS::EntityID& originID = pdu.getOriginatingEntityID();
    const DIS::SimulationAddress& simAddr = originID.getSimulationAddress();
    uint16_t site = simAddr.getSite(), app = simAddr.getApplication(), entity = originID.getEntityNumber();

    if (m_mapper && m_mapper->isOwnApplication(site, app)) return;

    std::string engineID = m_mapper ? m_mapper->getEngineID(site, app, entity) : "";
    if (engineID.empty())
        engineID = QString("%1:%2:%3").arg(site).arg(app).arg(entity).toStdString();

    DISIncomingDatum data;
    data.originatingEntityID = engineID;
    data.requestID    = pdu.getRequestID();
    data.timeInterval = pdu.getTimeInterval();

    for (const DIS::FixedDatum& fd : pdu.getFixedDatums()) {
        DISFixedDatumEntry entry;
        entry.datumID    = fd.getFixedDatumID();
        entry.datumValue = fd.getFixedDatumValue();
        data.fixedDatums.push_back(entry);
    }
    for (const DIS::VariableDatum& vd : pdu.getVariableDatums()) {
        DISVariableDatumEntry entry;
        entry.datumID = vd.getVariableDatumID();
        std::vector<uint8_t> bytes = vd.getPayloadBytes();
        entry.data = QByteArray(reinterpret_cast<const char*>(bytes.data()),
                                static_cast<int>(bytes.size()));
        data.variableDatums.push_back(entry);
    }

    emit incomingDataQuery(data);
}
