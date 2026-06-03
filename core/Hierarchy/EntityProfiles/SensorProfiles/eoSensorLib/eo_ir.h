#pragma once

#ifndef EO_IR_H
#define EO_IR_H

#include "payload.h"
#include "eo.h"
#include <cmath>
#include <iostream>
#include "../core/Hierarchy/hierarchy.h"
#include "../core/Hierarchy/profilecategaory.h"


#define M_PI 3.14159265358979323846
const float EARTH_RADIUS_METERS = 6371000.0f;

class EO_IR 
{

/*      Initialization in of EO_IR Start     */
public:
    EO_IR(Hierarchy *hierarchy);
    ~EO_IR();
    bool getIsActive(){ return bool(isActive); }

/*    Initialization of EO Sensor Start      */
    EO *eo = nullptr;
    EO *initEO(eoSensorPayload sensor,
                Surrounding surrounding);
    EO_PayLoad eo_payload;
        eoSensorPayload eo_sensor;
        Surrounding  eo_surrounding;
/*     Initialization of EO Sensor End       */
/*           Projected Area Start            */
    Vec3 toECEF(float latDeg, float lonDeg, float alt);
    Vec3 getViewDir(Coordinate a,Coordinate b);
    Vec3 ecefToENU(const Vec3& d, float latDeg, float lonDeg);
    float getProjectedArea(
        Vec3 viewDir,
        EntityDimension entityDimension);
    ProjectedExtents getProjectedExtents(
        Vec3 viewDir, EntityDimension entityDimension);

        EntityDimension entityDimension;
        Vec3 viewDir;
/*            Projected Area End             */
    IR_PayLoad ir_payload;

private:
    const PayLoad m_payload;
    bool isActive = false;

/*      Initialization in of EO_IR End       */

/*  Preprocessing before sending to EO & IR */
public:
    void preprocessing();
    Hierarchy* m_hierarchy;
    std::unordered_map<std::string, Platform*> m_platform;
    PreProcessEntityList getPreProcessEntityList(){ return *ppel; }
    float getPreProcessEntityDistance(std::string ID){ return (*ppel).at(ID).distanceBtwUser; }
    float getPreProcessEntityAngle   (std::string ID){ return (*ppel).at(ID).angleBtwUser; }
public:
    float toRadians(float degree);
    float distanceBtw(Coordinate p1, Coordinate p2);
    float distanceBtw(float p1latitude ,
                       float p1longitude,
                       float p1altitude ,
                       float p2latitude ,
                       float p2longitude,
                       float p2altitude);
    float calculateAngle(float headingDeg,
                          float pitchDeg,
                          Coordinate entityA,
                          Coordinate entityB);
    float calculateAngle(float headingDeg,
                          float pitchDeg,
                          float p1latitude ,
                          float p1longitude,
                          float p1altitude ,
                          float p2latitude ,
                          float p2longitude,
                          float p2altitude);
    float relativeAngle(float sensorAngle, float targetAngle);
    float viewAngle(float sensorAngle, float targetAngle);
    bool scanVeticalHorizonatalAngles(float veticalAzimuth,
                                      float horizonatalAzimuth,
                                      float veticalAngle,
                                      float horizonatalAngle);
    float getHorizontalTargetAngle(
        Coordinate sensor, Coordinate target,
        float sensorHeadingDeg);
    float getVerticalTargetAngle(float sensorAlt, float targetAlt,
        float horizontalDistance, float sensorPitchDeg);
    float getHorizontalDistance(Coordinate point1, Coordinate point2);
private:
    PreProcessEntityList* ppel;
    Coordinate toCartesian(float lat, float lon, float alt);
    Axis ecefToENU(const Axis& ref, const Axis& target, float latDeg, float lonDeg);
    Axis toVector(float lat, float lon, float alt);
    //float calculateAngle(Axis p1, Coordinate p2);
    //float calculateAngle(Coordinate p1, Coordinate p2);

/*            Main Declartion           */
public:
    void intro();
};

#endif
