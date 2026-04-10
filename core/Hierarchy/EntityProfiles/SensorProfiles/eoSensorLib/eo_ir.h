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

class EO_IR 
{

/*      Initialization in of EO_IR Start     */
public:
    EO_IR(Hierarchy *hierarchy);
    ~EO_IR();
    bool getIsActive(){ return bool(isActive); }

/*    Initialization of EO Sensor Start      */
    EO *eo = nullptr;
    EO *initEO(CustomSensor sensor,
                Surrounding surrounding);
    EO_PayLoad eo_payload;
        CustomSensor eo_sensor;
        Surrounding  eo_surrounding;
/*     Initialization of EO Sensor End       */
/*           Projected Area Start            */
    Vec3 toECEF(double latDeg, double lonDeg, double alt);
    Vec3 getViewDir(Coordinate a,Coordinate b);
    Vec3 ecefToENU(const Vec3& d, double latDeg, double lonDeg);
    double getProjectedArea(
        Vec3 viewDir,
        EntityDimension entityDimension);

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
    std::unordered_map<std::string, Platform*> *m_platform;
    PreProcessEntityList getPreProcessEntityList(){ return *ppel; }
    double getPreProcessEntityDistance(std::string ID){ return (*ppel).at(ID).distanceBtwUser; }
    double getPreProcessEntityAngle   (std::string ID){ return (*ppel).at(ID).angleBtwUser; }
public:
    double toRadians(double degree);
    double distanceBtw(Coordinate p1, Coordinate p2);
    double distanceBtw(double p1latitude ,
                       double p1longitude,
                       double p1altitude ,
                       double p2latitude ,
                       double p2longitude,
                       double p2altitude);
    double calculateAngle(double headingDeg,
                          double pitchDeg,
                          Coordinate entityA,
                          Coordinate entityB);
    double calculateAngle(double headingDeg,
                          double pitchDeg,
                          double p1latitude ,
                          double p1longitude,
                          double p1altitude ,
                          double p2latitude ,
                          double p2longitude,
                          double p2altitude);
private:
    PreProcessEntityList* ppel;
    Coordinate toCartesian(double lat, double lon, double alt);
    Axis ecefToENU(const Axis& ref, const Axis& target, double latDeg, double lonDeg);
    Axis toVector(double lat, double lon, double alt);
    //double calculateAngle(Axis p1, Coordinate p2);
    //double calculateAngle(Coordinate p1, Coordinate p2);

/*            Main Declartion           */
public:
    void intro();
};

#endif
