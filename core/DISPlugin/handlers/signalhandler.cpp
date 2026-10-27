#include "signalhandler.h"
#include <dis7/signalpdu.h>
#include <dis7/utils/DataStream.h>

SignalHandler::SignalHandler(QObject* parent) : QObject(parent) {}
SignalHandler::~SignalHandler() = default;
void SignalHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void SignalHandler::onSignalReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::SignalPdu pdu;
    DIS::DataStream ds(pduBytes.constData(), static_cast<size_t>(pduBytes.size()), DIS::BIG);
    pdu.unmarshal(ds);

    const DIS::EntityID& entityID = pdu.getEntityID();
    const DIS::SimulationAddress& simAddr = entityID.getSimulationAddress();
    uint16_t site = simAddr.getSite(), app = simAddr.getApplication(), entity = entityID.getEntityNumber();

    if (m_mapper && m_mapper->isOwnApplication(site, app)) return;

    std::string engineID = m_mapper ? m_mapper->getEngineID(site, app, entity) : "";
    if (engineID.empty())
        engineID = QString("%1:%2:%3").arg(site).arg(app).arg(entity).toStdString();

    DISIncomingSignal data;
    data.transmittingEntityID = engineID;
    data.radioID        = pdu.getRadioID();
    data.encodingScheme = pdu.getEncodingScheme();
    data.sampleRate      = pdu.getSampleRate();
    data.samples          = pdu.getSamples();
    data.data              = pdu.getData();

    emit incomingSignal(data);
}
