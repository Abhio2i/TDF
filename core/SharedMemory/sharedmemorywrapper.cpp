#include "sharedmemorywrapper.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/radar.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
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

        SharedComponent component;
        component.name = platform->Name;
        component.base = entity;
        component.platform = platform;
        component.transform = platform->transform;
        component.rigidbody = platform->rigidbody;
        component.collider = platform->collider;
        component.dynamicModel = platform->dynamicModel;
        component.trajectory = platform->trajectory;
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
    for (auto& [id, comp] : sharedComponent) {
        SharedStructs::Entity* entity = handler->getEntityByID(id);
        updateTransformData(comp,entity);
        updateDynamicData(comp,entity);
        updateTrajectoryData(comp,entity);
        updateSensorData(comp,entity);
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
    QVector3D velocity = comp.dynamicModel->velocity;
    entity->dynamicsData.velocity.set(velocity.x(),velocity.y(),velocity.z());
    entity->dynamicsData.setMaxAcceleration(comp.dynamicModel->Acceleration);
    entity->dynamicsData.setMaxDeceleration(comp.dynamicModel->Decceleration);
    entity->dynamicsData.setMaxAltitude(comp.dynamicModel->maxAltitude);
    entity->dynamicsData.setMinSpeed(comp.dynamicModel->minSpeed);
    entity->dynamicsData.setMaxSpeed(comp.dynamicModel->maxSpeed);
    entity->dynamicsData.setSpeed(comp.dynamicModel->currentSpeed);
    entity->dynamicsData.setTurnRate(comp.dynamicModel->turnRate);
    entity->dynamicsData.setClimbRate(comp.dynamicModel->climbRate);
    entity->dynamicsData.setPitch(comp.dynamicModel->pitch);
    entity->dynamicsData.setRoll(comp.dynamicModel->roll);
    entity->dynamicsData.setYaw(comp.dynamicModel->yaw);
    entity->dynamicsData.setRollRate(comp.dynamicModel->Rollrate);
    entity->dynamicsData.setPitchRate(comp.dynamicModel->Pitchrate);
    entity->dynamicsData.setYawRate(comp.dynamicModel->Yawrate);
    entity->dynamicsData.setDriftAngle(comp.dynamicModel->DriftAngle);
    entity->dynamicsData.setTrueHeading(comp.dynamicModel->TrueHeading);
    entity->dynamicsData.setTrueAirSpeed(comp.dynamicModel->TrueAirSpeed);
    entity->dynamicsData.setNorthVelocity(comp.dynamicModel->NorthVelocity);
    entity->dynamicsData.setEastVelocity(comp.dynamicModel->EastVelocity);
    entity->dynamicsData.setVerticalVelocity(comp.dynamicModel->VerticalVelocity);
    entity->dynamicsData.setGroundVelocity(comp.dynamicModel->GroundVelocity);

    int i =0;
    for (auto it = comp.dynamicModel->customParameters.begin(); it != comp.dynamicModel->customParameters.end(); ++it) {
        SharedStructs::parameter* param = new SharedStructs::parameter();
        param->setName(it.key().toStdString().c_str());
        param->setValue(it.value().toString().toStdString());
        entity->dynamicsData.updateCustomParameter(i,*param);
        i++;
    }
    entity->dynamicsData.setParamCount(i);

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
    int i           = 0;
    int sensorcount = 0;

    for (auto const& pair : *comp.platform->sensors->sensors)
    {
        Sensor* s = pair.second;
        if (!s) continue;

        if (s->subType == Sensor::SubType::Generic && i < 4)
        {
            sensorcount++;

            Radar* r = dynamic_cast<Radar*>(s);
            if (!r) { i++; continue; }   // guard: skip if cast fails

            // ------------------------------------------------------------------
            // Radar configuration — read from RadarConfig (single source of truth)
            //
            // ALIGNMENT NOTE (radarmodel refactor):
            //   RadarAttributes  →  RadarConfig  (type rename only)
            //
            //   Three fields that existed in RadarAttributes were removed from
            //   RadarConfig because they were never read by any physics function:
            //     scanningNumHits  →  not in RadarConfig, send 0 (harmless default)
            //     scanTime0        →  not in RadarConfig, send 0.0f
            //     scanTime1        →  not in RadarConfig, send 0.0f
            //
            //   All other fields map 1-to-1 with identical names and types.
            // ------------------------------------------------------------------
            RadarConfig cfg = r->getRadarConfig();

            entity->sensors[i].sensorType       = 0;
            entity->sensors[i].power            = static_cast<float>(cfg.emissionPower_kW);
            entity->sensors[i].frequency        = static_cast<float>(cfg.frequency_Hz / 1e9); // Hz → GHz
            entity->sensors[i].minAzimuth       = cfg.minAzimuth;
            entity->sensors[i].maxAzimuth       = cfg.maxAzimuth;
            entity->sensors[i].minElevation     = cfg.minElevation;
            entity->sensors[i].maxElevation     = cfg.maxElevation;
            entity->sensors[i].rate             = cfg.scanningRate_RPM;
            entity->sensors[i].hits             = 0.0f;   // scanningNumHits removed from RadarConfig
            entity->sensors[i].AntennaGain      = cfg.antennaGain;
            entity->sensors[i].AntennaBandwidth = static_cast<float>(cfg.antennaBandwidth);
            entity->sensors[i].beamWidth        = cfg.beamWidth;
            entity->sensors[i].scanType         = static_cast<int>(cfg.scanType);
            entity->sensors[i].scanTime1        = 0.0f;   // scanTime0 removed from RadarConfig
            entity->sensors[i].scanTime2        = 0.0f;   // scanTime1 removed from RadarConfig
            entity->sensors[i].peakSideLobLevel = cfg.peakSidelobeLevel;
            entity->sensors[i].avgSideLobLevel  = cfg.avgSidelobeLevel;
            entity->sensors[i].pulseWidth       = cfg.pulseWidth;

            // ------------------------------------------------------------------
            // Tracked targets — capped at 19
            // ------------------------------------------------------------------
            int trgtcount = static_cast<int>(r->targets.size());
            trgtcount     = trgtcount > 19 ? 19 : trgtcount;

            for (int k = 0; k < trgtcount; ++k)
            {
                const Target& tgt = r->targets[k];

                // Guard: skip if entity or transform is null
                if (!tgt.entity || !tgt.entity->transform) continue;

                entity->sensors[i].trackedTargets[k].latitude  = tgt.entity->transform->getLatitude();
                entity->sensors[i].trackedTargets[k].longitude = tgt.entity->transform->getLongitude();
                entity->sensors[i].trackedTargets[k].altitude  = tgt.entity->transform->getAltitude();
                entity->sensors[i].trackedTargets[k].speed     = tgt.speed;
                entity->sensors[i].trackedTargets[k].distance  = tgt.radius;
                entity->sensors[i].trackedTargets[k].bearing   = tgt.angle;
            }

            entity->sensors[i].trackedTargetCount = trgtcount;
            i++;
        }
    }

    sensorcount         = sensorcount > 4 ? 4 : sensorcount;
    entity->sensorCount = sensorcount;
}
// void SharedMemoryWrapper::updateSensorData(SharedComponent comp,SharedStructs::Entity* entity){
//     // int count = comp.platform->sensors->sensors->size();
//     int i =0;
//     int sensorcount = 0;
//     for (auto const& pair :*comp.platform->sensors->sensors) {
//         Sensor* s = pair.second;
//         if(!s)continue;

//         if(s->subType == Sensor::SubType::Generic && i <4){
//             sensorcount++;
//             Radar* r = dynamic_cast<Radar*>(s);
//             entity->sensors[i].sensorType = 0;
//             entity->sensors[i].power = r->power;
//             entity->sensors[i].frequency = r->frequency;

//             entity->sensors[i].minAzimuth = r->minAzimuth;
//             entity->sensors[i].maxAzimuth = r->maxAzimuth;
//             entity->sensors[i].minElevation = r->minElevation;
//             entity->sensors[i].maxElevation = r->maxElevation;

//             entity->sensors[i].rate = r->rate;
//             entity->sensors[i].hits = r->hits;

//             entity->sensors[i].AntennaGain = r->AntennaGain;
//             entity->sensors[i].AntennaBandwidth = r->AntennaBandwidth;
//             entity->sensors[i].beamWidth = r->beamWidth;
//             entity->sensors[i].scanType = r->scanType;
//             entity->sensors[i].scanTime1 = r->scanTime1;
//             entity->sensors[i].scanTime2 = r->scanTime2;
//             entity->sensors[i].peakSideLobLevel = r->peakSideLobLevel;
//             entity->sensors[i].avgSideLobLevel = r->avgSideLobLevel;

//             entity->sensors[i].pulseWidth = r->pulseWidth;

//             for (int k = 0; k < r->targets.size(); ++k) {
//                 if(k<19){
//                     entity->sensors[i].trackedTargets[k].latitude = r->targets[k].entity->transform->getLatitude();
//                     entity->sensors[i].trackedTargets[k].longitude = r->targets[k].entity->transform->getLongitude();
//                     entity->sensors[i].trackedTargets[k].altitude = r->targets[k].entity->transform->getAltitude();
//                     entity->sensors[i].trackedTargets[k].speed = r->targets[k].speed;
//                     entity->sensors[i].trackedTargets[k].distance = r->targets[k].radius;
//                     entity->sensors[i].trackedTargets[k].bearing = r->targets[k].angle;
//                 }
//             }
//             int trgtcount = r->targets.size();
//             trgtcount = trgtcount>19?19:trgtcount;
//             entity->sensors[i].trackedTargetCount = trgtcount;

//             i++;
//         }
//     }

//     sensorcount = sensorcount >4?4 : sensorcount;
//     entity->sensorCount = sensorcount;

// }
