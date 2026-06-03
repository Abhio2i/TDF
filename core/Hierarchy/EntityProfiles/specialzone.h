// =============================================================================
// FILE:        specialzone.h
// MODULE:      Tactical Simulation Environment Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the Specialzone class, representing environmental
//               volumes in the simulation. It manages localized weather
//               conditions such as wind, temperature, humidity, and
//               precipitation (rain/fog) within specific altitude bands.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Feb 2026  Added dynamic wind and atmospheric speed calculations.
//   Rev 3  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SPECIALZONE_H
#define SPECIALZONE_H

#include "core/Hierarchy/entity.h"

// =============================================================================
// CLASS: Specialzone
//
// DESCRIPTION: Concrete implementation of an Entity that simulates environmental
//              zones. Used to apply atmospheric effects to platforms entering
//              the zone's defined collider volume.
// =============================================================================
class Specialzone: public Entity
{
    Q_OBJECT
public:
    Specialzone(Hierarchy* h);

    // =========================================================================
    // SECTION: Spatial & Physical Components
    // DESCRIPTION: Pointers to functional blocks for positioning and collision.
    // =========================================================================
    Transform *transform = nullptr;
    Collider *collider = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;

    // =========================================================================
    // SECTION: Atmospheric Parameters
    // DESCRIPTION: Data points defining weather and environmental conditions.
    // =========================================================================
    float direction = 90;        // Wind direction (deg)
    float MinAltitude = 0;       // Lower bound (meters)
    float MaxAltitude = 10000;   // Upper bound (meters)
    float Speed = 70;            // Wind speed (km/h)
    float Temprature = 30;       // Ambient temperature (deg C)
    float humidity = 30;         // Relative humidity (%)
    float rain = 0;              // Precipitation rate (mm/h)
    float fog = 0;               // Visibility reduction (%)
    float AtmoshPhericPressure= 0;
    float AirDensity = 0;
    float gasAttenuation = 0;
    float shadowZone = 0;

    // Environment By Himanshu For EO/IR Sensor
    float absoluteHumidity     = 10.0;  // g/m^3
    float snowfallEquivalent   = 0.0;   // mm/hr
    float backgroundTemp       = 20.0 ; // Celsius
    float aerosolConcentration = 0.05 ; // mg/m^3
    float baseExtinctionCoeff  = 0.15 ; // Base sigma (1/km)
    float ambientIlluminance   = 50000; // lux
    float solarIrradiance      = 800  ; // W/m^2 (Crucial for Glint)

    // =========================================================================
    // SECTION: Simulation Logic
    // DESCRIPTION: Methods for real-time updates and dynamic calculations.
    // =========================================================================
    void Update(float delta);

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Mandatory methods for entity lifecycle and component systems.
    // =========================================================================
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    // =========================================================================
    // SECTION: Serialization
    // DESCRIPTION: Persistence logic for zone configuration and state.
    // =========================================================================
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

private:
    // =========================================================================
    // SECTION: Internal Simulation Helpers
    // DESCRIPTION: Private variables and logic for varying wind/speed over time.
    // =========================================================================
    float time = 0;
    QVector3D getDynamicWind(float baseAngle, float time);
    float getDynamicSpeed(float baseSpeed, float time);
};

#endif // SPECIALZONE_H
