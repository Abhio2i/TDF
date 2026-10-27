#include "setdatahandler.h"
#include <dis7/SetDataPdu.h>
#include <dis7/utils/DataStream.h>

SetDataHandler::SetDataHandler(QObject* parent) : QObject(parent) {}
SetDataHandler::~SetDataHandler() = default;
void SetDataHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void SetDataHandler::onSetDataReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::SetDataPdu pdu;
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
    data.timeInterval  = 0;   // SetDataPdu has no timeInterval field

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

    emit incomingSetData(data);
}
