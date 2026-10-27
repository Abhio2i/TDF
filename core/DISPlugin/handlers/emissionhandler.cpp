#include "emissionhandler.h"
#include <dis7/ElectromagneticEmissionsPdu.h>
#include <dis7/utils/DataStream.h>
#include "core/DISPlugin/utils/coordconverter.h"

EmissionHandler::EmissionHandler(QObject* parent) : QObject(parent) {}
EmissionHandler::~EmissionHandler() = default;
void EmissionHandler::setMapper(EntityIDMapper* mapper) { m_mapper = mapper; }

void EmissionHandler::onEmissionReceived(QByteArray pduBytes, QHostAddress sender)
{
    Q_UNUSED(sender)
    DIS::ElectromagneticEmissionsPdu pdu;
    DIS::DataStream ds(pduBytes.constData(), static_cast<size_t>(pduBytes.size()), DIS::BIG);
    pdu.unmarshal(ds);

    const DIS::EntityID& entityID = pdu.getEmittingEntityID();
    const DIS::SimulationAddress& simAddr = entityID.getSimulationAddress();
    uint16_t site = simAddr.getSite(), app = simAddr.getApplication(), entity = entityID.getEntityNumber();


    if (m_mapper && m_mapper->isOwnApplication(site, app)) return;

    std::string engineID = m_mapper ? m_mapper->getEngineID(site, app, entity) : "";
    if (engineID.empty())
        engineID = QString("%1:%2:%3").arg(site).arg(app).arg(entity).toStdString();

    const std::vector<DIS::ElectromagneticEmissionSystemData>& systems = pdu.getSystems();
    if (systems.empty()) return;

    const DIS::ElectromagneticEmissionSystemData& sys = systems[0];

    DISIncomingEmission data;
    data.emittingEntityID = engineID;
    data.emitterFunction  = sys.getEmitterSystem().getFunction();
    data.systemID         = sys.getEmitterSystem().getEmitterIdNumber();

    for (const DIS::ElectromagneticEmissionBeamData& beam : sys.getBeamDataRecords()) {
        DISIncomingEmissionBeam b;
        b.beamID       = beam.getBeamIDNumber();
        b.frequency    = beam.getFundamentalParameterData().getFrequency();
        b.freqRange    = beam.getFundamentalParameterData().getFrequencyRange();
        b.erpPower     = beam.getFundamentalParameterData().getEffectiveRadiatedPower();
        b.prf          = beam.getFundamentalParameterData().getPulseRepetitionFrequency();
        b.pulseWidth   = beam.getFundamentalParameterData().getPulseWidth();
        b.beamAzCenter = static_cast<float>(beam.getBeamData().getBeamAzimuthCenter()   * CoordConverter::RAD_TO_DEG);
        b.beamAzSweep  = static_cast<float>(beam.getBeamData().getBeamAzimuthSweep()    * CoordConverter::RAD_TO_DEG);
        b.beamElCenter = static_cast<float>(beam.getBeamData().getBeamElevationCenter() * CoordConverter::RAD_TO_DEG);
        b.beamElSweep  = static_cast<float>(beam.getBeamData().getBeamElevationSweep()  * CoordConverter::RAD_TO_DEG);
        b.beamFunction = beam.getBeamFunction();
        b.active       = (beam.getBeamStatus() == 0);
        data.beams.push_back(b);
    }
    emit incomingEmission(data);
}
