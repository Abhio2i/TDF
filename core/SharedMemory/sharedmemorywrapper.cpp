#include "sharedmemorywrapper.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/adsbsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "qapplication.h"
#include "qthread.h"
#include "sharedMemory.h"
#include "EntityHandler.h"
#include <csignal>
#include <iostream>
#include <core/Debug/console.h>

SharedMemoryWrapper::SharedMemoryWrapper() {
    shm.create();
    handler = shm.entityHandler;
    // handler->addEntity(SharedStructs::Entity()); // Ek dummy entity add karein
    // std::cout << "Program running... Press Ctrl+C to stop." << std::endl;

    // while(true) {
    //     if(handler->getEntityByIndex(0) != nullptr) {
    //         SharedStructs::Entity* entities = handler->getEntityByIndex(0);
    //         entities->dynamicsData.speed += 1.0f;
    //         std::cout << "Current Speed: " << entities->dynamicsData.speed << "\r" << std::flush;
    //         if(entities->dynamicsData.speed > 10){
    //             break;
    //         }
    //     }
    //     usleep(100000); // 100ms sleep taaki CPU 100% na ho jaye
    // }

}

SharedMemoryWrapper::~SharedMemoryWrapper() {
    std::cout << "sharedmemory delete";
    Console::log("sharedmemory delete");
    shm.cleanup();
    sharedComponent.clear();
}

void SharedMemoryWrapper::entityAdded(QString /*parentID*/, Entity* entity) {

    if (entity->type == Constants::EntityType::Platform) {
        Platform* platform = dynamic_cast<Platform*>(entity);
        // Remote DIS entities have no dynamicModel/trajectory/sensors.
        // SharedMemory is only for local simulation entities.
        // if (platform->isRemoteDISEntity) return;//by Aman
        SharedComponent component;
        component.name = platform->Name;
        component.base = entity;
        component.platform = platform;
        component.transform = &platform->transform;
        component.rigidbody = &platform->rigidbody;
        component.collider = &platform->collider;
        component.dynamicModel = &platform->dynamicModel;
        component.trajectory = &platform->trajectory;
        sharedComponent[platform->ID] = component;
        SharedStructs::Entity sharedEntity{};
        sharedEntity.setID(entity->ID.c_str());
        sharedEntity.setName(entity->Name.c_str());
        handler->addEntity(sharedEntity);
    }
}

void SharedMemoryWrapper::entityRemoved(QString ID) {
    std::string key = ID.toStdString();
    auto physIt = sharedComponent.find(key);
    if (physIt != sharedComponent.end()) {
        handler->removeEntityByID(ID.toStdString());
        sharedComponent.erase(physIt);
    }
}

void SharedMemoryWrapper::SimulationUpdate(float deltaTime){
    return;
    for (auto& [id, comp] : sharedComponent) {
        SharedStructs::Entity* entity = handler->getEntityByID(id);
        updateTransformData(comp,entity);
        updateDynamicData(comp,entity);
        updateTrajectoryData(comp,entity);
        updateSensorData(comp,entity);
        updateRadioData(comp,entity);
    }
}

void SharedMemoryWrapper::updateTransformData(SharedComponent comp,SharedStructs::Entity* entity){
    QVector3D position = comp.transform->translation();
    QVector3D rotation = comp.transform->toEulerAngles();
    entity->transformData.position.set(position.x(),position.y(),position.z());
    entity->transformData.rotation.set(rotation.x(),rotation.y(),rotation.z());
    entity->transformData.geoCords.set(comp.transform->getLatitude(),
                                       comp.transform->getLongitude(),
                                       comp.transform->getAltitude(),
                                       comp.transform->getHeading()
                                       );
}

void SharedMemoryWrapper::updateDynamicData(SharedComponent comp,SharedStructs::Entity* entity){
    //Maximums
    entity->dynamicsData.setMinSpeed(comp.dynamicModel->minSpeed);
    entity->dynamicsData.setMaxSpeed(comp.dynamicModel->maxSpeed);
    entity->dynamicsData.setMaxAcceleration(comp.dynamicModel->Acceleration);
    entity->dynamicsData.setMaxDeceleration(comp.dynamicModel->Decceleration);
    entity->dynamicsData.setMaxAltitude(comp.dynamicModel->maxAltitude);
    entity->dynamicsData.setClimbRate(comp.dynamicModel->climbRate);

    entity->dynamicsData.setSpeed(comp.dynamicModel->currentSpeed);
    entity->dynamicsData.setAcceleration(comp.dynamicModel->currentAcceralation);
    entity->dynamicsData.setCurrentHeightAboveGround(comp.dynamicModel->currentHeightAboveGround);
    entity->dynamicsData.setPitch(comp.dynamicModel->pitch);
    entity->dynamicsData.setRoll(comp.dynamicModel->roll);
    entity->dynamicsData.setYaw(comp.dynamicModel->yaw);
    entity->dynamicsData.setTrueHeading(comp.dynamicModel->TrueHeading);
    entity->dynamicsData.setPitchRate(comp.dynamicModel->Pitchrate);
    entity->dynamicsData.setRollRate(comp.dynamicModel->Rollrate);
    entity->dynamicsData.setYawRate(comp.dynamicModel->Yawrate);
    entity->dynamicsData.setHorizonPitchRate(comp.dynamicModel->horizonPitchRate);
    entity->dynamicsData.setHorizonRollRate(comp.dynamicModel->horizonRollRate);
    entity->dynamicsData.setTurnRate(comp.dynamicModel->angleOfAttack);
    entity->dynamicsData.setAngleOfAttack(comp.dynamicModel->sideslipAngle);
    entity->dynamicsData.setFlightPathAngle(comp.dynamicModel->flightPathAngle);
    entity->dynamicsData.setTrueAirSpeed(comp.dynamicModel->TrueAirSpeed);
    entity->dynamicsData.setGroundVelocity(comp.dynamicModel->GroundVelocity);
    entity->dynamicsData.setEastVelocity(comp.dynamicModel->EastVelocity);
    entity->dynamicsData.setVerticalVelocity(comp.dynamicModel->VerticalVelocity);
    entity->dynamicsData.setNorthVelocity(comp.dynamicModel->NorthVelocity);
    entity->dynamicsData.setDriftAngle(comp.dynamicModel->DriftAngle);


    QVector3D& groundVelocity = comp.dynamicModel->groundVelocity;
    entity->dynamicsData.groundVelocity.set(groundVelocity.x(),groundVelocity.y(),groundVelocity.z());

    QVector3D& airVelocity = comp.dynamicModel->airVelocity;
    entity->dynamicsData.airVelocity.set(airVelocity.x(),airVelocity.y(),airVelocity.z());

    QVector3D& bodyAirVelocity = comp.dynamicModel->bodyAirVelocity;
    entity->dynamicsData.bodyAirVelocity.set(bodyAirVelocity.x(),bodyAirVelocity.y(),bodyAirVelocity.z());

    QVector3D& bodyGroundVelocity = comp.dynamicModel->bodyGroundVelocity;
    entity->dynamicsData.bodyGroundVelocity.set(bodyGroundVelocity.x(),bodyGroundVelocity.y(),bodyGroundVelocity.z());

    QVector3D& localVelocity = comp.dynamicModel->localVelocity;
    entity->dynamicsData.localVelocity.set(localVelocity.x(),localVelocity.y(),localVelocity.z());

    QVector3D& acceleration = comp.dynamicModel->acceleration;
    entity->dynamicsData.acceleration.set(acceleration.x(),acceleration.y(),acceleration.z());

    QVector3D& bodyAcceleration = comp.dynamicModel->bodyAcceleration;
    entity->dynamicsData.bodyAcceleration.set(bodyAcceleration.x(),bodyAcceleration.y(),bodyAcceleration.z());

    QVector3D& velocity = comp.dynamicModel->velocity;
    entity->dynamicsData.velocity.set(velocity.x(),velocity.y(),velocity.z());

    QVector3D& angularVelocity = comp.dynamicModel->angularVelocity;
    entity->dynamicsData.angularVelocity.set(angularVelocity.x(),angularVelocity.y(),angularVelocity.z());


    // int i =0;
    // for (auto it = comp.dynamicModel->AdditionalParameters.begin(); it != comp.dynamicModel->AdditionalParameters.end(); ++it) {
    //     SharedStructs::parameter* param = new SharedStructs::parameter();
    //     param->setName(it.key().toStdString().c_str());
    //     param->setValue(it.value().toString().toStdString());
    //     entity->dynamicsData.updateCustomParameter(i,*param);
    //     i++;
    // }
    // entity->dynamicsData.setParamCount(i);

}

void SharedMemoryWrapper::updateTrajectoryData(SharedComponent comp,SharedStructs::Entity* entity){
    if(comp.trajectory->Trajectories.size() != entity->trajectoryData.getWaypointCount()){
        if(comp.trajectory->Trajectories.size()<99){
            int i = 0;
            for (const Waypoints* waypoint : comp.trajectory->Trajectories) {
                if (waypoint) {
                    SharedStructs::waypoint* wp = new SharedStructs::waypoint();
                    wp->position.set(waypoint->position->x,waypoint->position->y,waypoint->position->z);
                    wp->geoCords.set(waypoint->geocord->latitude,
                                     waypoint->geocord->longitude,
                                     waypoint->geocord->altitude,
                                     waypoint->geocord->Heading);
                    wp->setspeed(waypoint->speed);
                    entity->trajectoryData.addWaypoint(*wp,i);
                    i++;
                }
            }
            entity->trajectoryData.setWaypointCount(i);
        }else{
            entity->trajectoryData.setWaypointCount(0);
        }
    }
}

void SharedMemoryWrapper::updateSensorData(SharedComponent comp, SharedStructs::Entity* entity)
{
    int aisDataCount = 0;
    int adsbDataCount = 0;

    for (auto const& pair :comp.platform->sensors.sensors)
    {
        Sensor* s = pair.second;
        if (!s) continue;

        if (s->subType == Sensor::SubType::AIS )
        {
            if (aisDataCount >= 100) // Actual radio array capacity ke according
                continue;
            AISSensor* r = dynamic_cast<AISSensor*>(s);
            if (!r) {  continue; }   // guard: skip if cast fails

            for(auto const& ais : r->detect ){
                entity->ais[aisDataCount].mmsi = ais.mmsi;
                entity->ais[aisDataCount].imo_number = ais.imo_number;
                entity->ais[aisDataCount].setName(ais.name);
                entity->ais[aisDataCount].setCallSign(ais.call_sign);
                entity->ais[aisDataCount].setDestination(ais.destination);
                entity->ais[aisDataCount].dims.length = ais.dims.length;
                entity->ais[aisDataCount].dims.beam = ais.dims.beam;
                entity->ais[aisDataCount].dims.draught = ais.dims.draught;
                entity->ais[aisDataCount].ship_type = static_cast<decltype(entity->ais[aisDataCount].ship_type)>(
                    ais.ship_type
                    );;
                entity->ais[aisDataCount].pos.latitude = ais.pos.latitude;
                entity->ais[aisDataCount].pos.longitude = ais.pos.longitude;
                entity->ais[aisDataCount].pos.timestamp = ais.pos.timestamp;
                entity->ais[aisDataCount].sog = ais.sog;
                entity->ais[aisDataCount].cog = ais.cog;
                entity->ais[aisDataCount].heading = ais.heading;
                entity->ais[aisDataCount].rot = ais.rot;
                entity->ais[aisDataCount].nav_status = static_cast<decltype(entity->ais[aisDataCount].nav_status)>(
                    ais.nav_status
                    );
                entity->ais[aisDataCount].eta.month = ais.eta.month;
                entity->ais[aisDataCount].eta.day = ais.eta.day;
                entity->ais[aisDataCount].eta.hour = ais.eta.hour;
                entity->ais[aisDataCount].eta.minute = ais.eta.minute;
                entity->ais[aisDataCount].repeat_indicator = ais.repeat_indicator;
                entity->ais[aisDataCount].ais_channel = ais.ais_channel;
                entity->ais[aisDataCount].is_verified = ais.is_verified;

                aisDataCount++;
            }
        }
        else
            if (s->subType == Sensor::SubType::ADSB )
            {
                if (adsbDataCount >= 100) // Actual radio array capacity ke according
                continue;
                ADSBSensor* r = dynamic_cast<ADSBSensor*>(s);
                if (!r) {  continue; }   // guard: skip if cast fails

                for (const auto& adsb : r->detect)
                {
                    auto& dst = entity->adsb[adsbDataCount];

                    dst.setMessageType(adsb.messageType.c_str());
                    dst.setTransmissionType(adsb.transmissiontype.c_str());
                    dst.setSessionId(adsb.session_id.c_str());
                    dst.setAircraftId(adsb.aircraft_id.c_str());
                    dst.setHexIdent(adsb.hex_ident.c_str());
                    dst.setFlightId(adsb.fligh_Id.c_str());

                    dst.setGeneratedDate(adsb.generated_date.c_str());
                    dst.setGeneratedTime(adsb.generated_time.c_str());
                    dst.setLoggedDate(adsb.logged_date.c_str());
                    dst.setLoggedTime(adsb.logged_time.c_str());

                    dst.setCallSign(adsb.call_sign.c_str());
                    dst.setSquawk(adsb.squawk.c_str());

                    dst.altitude         = adsb.altitude;
                    dst.ground_speed     = adsb.ground_speed;
                    dst.track            = adsb.track;
                    dst.lat              = adsb.lat;
                    dst.lon              = adsb.lon;
                    dst.climb_rate       = adsb.climb_rate;

                    dst.alert            = adsb.alert;
                    dst.emergency        = adsb.emergency;
                    dst.spi              = adsb.spi;
                    dst.is_on_ground     = adsb.is_on_ground;

                    adsbDataCount++;
                }
            }

    }
    entity->aisCount = aisDataCount;
    entity->adsbCount = adsbDataCount;

}

void SharedMemoryWrapper::updateRadioData(SharedComponent comp, SharedStructs::Entity* entity)
{
    int radioCount = 0;


    for (auto const& pair :comp.platform->radios.radios)
    {
        if (radioCount >= 100) // Actual radio array capacity ke according
            break;
        Radio* s = pair.second;
        if (!s) continue;

        Radio* r = dynamic_cast<Radio*>(s);
        if (!r) {  continue; }   // guard: skip if cast fails

        for(auto const& radio : r->targets ){
            auto& dst = entity->radios[radioCount];
            // Geometry
            dst.distance_m = radio.distance_m;
            dst.radius_m = radio.radius_m;
            dst.azimuth_deg = radio.azimuth_deg;

            // Scanner
            dst.scanner_max_range_m = radio.scanner_max_range_m;
            dst.scanner_frequency_hz = radio.scanner_frequency_hz;

            // Signal parameters
            dst.path_loss_db = radio.path_loss_db;
            dst.rx_power_dbm = radio.rx_power_dbm;
            dst.noise_floor_dbm = radio.noise_floor_dbm;
            dst.snr_db = radio.snr_db;
            dst.rain_attenuation_db = radio.rain_attenuation_db;
            dst.wind_attenuation_db_per_km = radio.wind_attenuation_db_per_km;
            dst.los_horizon_distance_m = radio.los_horizon_distance_m;
            dst.polarization_loss_db = radio.polarization_loss_db;
            dst.required_snr_threshold_db = radio.required_snr_threshold_db;

            // Detection flags
            dst.network_match = radio.network_match;
            dst.frequency_match = radio.frequency_match;
            dst.range_ok = radio.range_ok;
            dst.beam_ok = radio.beam_ok;
            dst.sensitivity_ok = radio.sensitivity_ok;
            dst.squelch_ok = radio.squelch_ok;
            dst.snr_ok = radio.snr_ok;

            // Footprint
            dst.is_footprint = radio.is_footprint;
            dst.heading_deg = radio.heading_deg;
            dst.beamwidth_deg = radio.beamwidth_deg;

            // Messages
            radioCount++;

        }


    }
    entity->radioCount = radioCount;

}


// void SharedMemoryWrapper::updateSensorData(SharedComponent comp, SharedStructs::Entity* entity)
// {
//     int i           = 0;
//     int sensorcount = 0;

//     for (auto const& pair :comp.platform->sensors.sensors)
//     {
//         Sensor* s = pair.second;
//         if (!s) continue;

//         if (s->subType == Sensor::SubType::Generic && i < 4)
//         {
//             sensorcount++;

//             Radar* r = dynamic_cast<Radar*>(s);
//             if (!r) { i++; continue; }   // guard: skip if cast fails

//             // ------------------------------------------------------------------
//             // Radar configuration — read from RadarConfig (single source of truth)
//             //
//             // ALIGNMENT NOTE (radarmodel refactor):
//             //   RadarAttributes  →  RadarConfig  (type rename only)
//             //
//             //   Three fields that existed in RadarAttributes were removed from
//             //   RadarConfig because they were never read by any physics function:
//             //     scanningNumHits  →  not in RadarConfig, send 0 (harmless default)
//             //     scanTime0        →  not in RadarConfig, send 0.0f
//             //     scanTime1        →  not in RadarConfig, send 0.0f
//             //
//             //   All other fields map 1-to-1 with identical names and types.
//             // ------------------------------------------------------------------
//             RadarConfig cfg = r->getRadarConfig();

//             entity->sensors[i].sensorType       = 0;
//             entity->sensors[i].power            = static_cast<float>(cfg.emissionPower_kW);
//             entity->sensors[i].frequency        = static_cast<float>(cfg.frequency_Hz / 1e9); // Hz → GHz
//             entity->sensors[i].minAzimuth       = cfg.minAzimuth;
//             entity->sensors[i].maxAzimuth       = cfg.maxAzimuth;
//             entity->sensors[i].minElevation     = cfg.minElevation;
//             entity->sensors[i].maxElevation     = cfg.maxElevation;
//             entity->sensors[i].rate             = cfg.scanningRate_RPM;
//             entity->sensors[i].hits             = 0.0f;   // scanningNumHits removed from RadarConfig
//             entity->sensors[i].AntennaGain      = cfg.antennaGain;
//             entity->sensors[i].AntennaBandwidth = static_cast<float>(cfg.antennaBandwidth);
//             entity->sensors[i].beamWidth        = cfg.beamWidth;
//             entity->sensors[i].scanType         = static_cast<int>(cfg.scanType);
//             entity->sensors[i].scanTime1        = 0.0f;   // scanTime0 removed from RadarConfig
//             entity->sensors[i].scanTime2        = 0.0f;   // scanTime1 removed from RadarConfig
//             entity->sensors[i].peakSideLobLevel = cfg.peakSidelobeLevel;
//             entity->sensors[i].avgSideLobLevel  = cfg.avgSidelobeLevel;
//             entity->sensors[i].pulseWidth       = cfg.pulseWidth;

//             // ------------------------------------------------------------------
//             // Tracked targets — capped at 19
//             // ------------------------------------------------------------------
//             int trgtcount = static_cast<int>(r->targets.size());
//             trgtcount     = trgtcount > 19 ? 19 : trgtcount;

//             for (int k = 0; k < trgtcount; ++k)
//             {
//                 const Target& tgt = r->targets[k];

//                 // Guard: skip if entity or transform is null
//                 if (!tgt.entity /*|| !tgt.entity->transform*/) continue;

//                 entity->sensors[i].trackedTargets[k].latitude  = tgt.entity->transform.getLatitude();
//                 entity->sensors[i].trackedTargets[k].longitude = tgt.entity->transform.getLongitude();
//                 entity->sensors[i].trackedTargets[k].altitude  = tgt.entity->transform.getAltitude();
//                 entity->sensors[i].trackedTargets[k].speed     = tgt.speed;
//                 entity->sensors[i].trackedTargets[k].distance  = tgt.radius;
//                 entity->sensors[i].trackedTargets[k].bearing   = tgt.angle;
//             }

//             entity->sensors[i].trackedTargetCount = trgtcount;
//             i++;
//         }
//     }

//     sensorcount         = sensorcount > 4 ? 4 : sensorcount;
//     entity->sensorCount = sensorcount;
// }
