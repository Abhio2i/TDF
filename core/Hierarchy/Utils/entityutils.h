// =============================================================================
// FILE:        entityutils.h
// MODULE:      Entity Utilities & Coordinate Conversion
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Provides utility functions for entity management, coordinate
//              transformations (ECEF, flat Earth, geographic), unit conversions,
//              distance calculations, and enum-to-string conversions for
//              entity types and formation types.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef ENTITYUTILS_H
#define ENTITYUTILS_H

#include <QString>
#include <QList>
#include "core/Hierarchy/Struct/constants.h"
#include <QStringList>
#include <cmath>

// =============================================================================
// SECTION: Coordinate System Structures
// DESCRIPTION: ECEF (Earth-Centered Earth-Fixed), Flat Earth (local tangent),
//              and geographic (lat/lon/alt) representations.
// =============================================================================

struct ECEF { double x, y, z; };        //!< Earth-Centered Earth-Fixed coordinates (meters)
struct FlatXYZ { double x, y, z; };     //!< Local tangent plane coordinates (meters)
struct GeoPos { double lat, lon, alt; }; //!< Geographic coordinates (degrees, degrees, meters)

// =============================================================================
// SECTION: Physical Constants
// DESCRIPTION: Standard values for physics and unit conversions.
// =============================================================================

const double G_ACCELERATION = 9.8f;              //!< Gravitational acceleration (m/s²)
const double FTtoKM = 1/3281.0f;                //!< Feet to kilometers conversion factor
const double KMtoFT = 3281.0f;                  //!< Kilometers to feet conversion factor
const double FTminToKMs = 1/196900.0f;          //!< Feet per minute to km per second conversion factor
#define EARTH_RADIUS 6371000.0                  //!< Mean Earth radius (meters)

// =============================================================================
// SECTION: Parameter Helpers
// DESCRIPTION: JSON serialisation helpers for numeric parameters with units.
// =============================================================================

QJsonObject toParm(float value, QString unit, float min = 0.0f, float max = 0.0f, QString description = "");
float valueFromParm(const QJsonObject& parm);

// =============================================================================
// SECTION: Angle Conversion
// DESCRIPTION: Degree to radian conversion.
// =============================================================================

double toRadians(double degree);                //!< Converts degrees to radians

// =============================================================================
// SECTION: Entity Type String Conversions
// DESCRIPTION: Convert between Constants::EntityType enum and string.
// =============================================================================

QString entityTypeToString(Constants::EntityType type);
Constants::EntityType stringToEntityType(const QString& str);
QStringList entityTypeOptions();                //!< Returns list of all entity type names

// =============================================================================
// SECTION: Formation Type String Conversions
// DESCRIPTION: Convert between Constants::FormationType enum and string.
// =============================================================================

QString formationTypeToString(Constants::FormationType type);
Constants::FormationType stringToFormationType(const QString& str);
QStringList formationTypeOptions();             //!< Returns list of all formation type names

// =============================================================================
// SECTION: Geographic Distance
// DESCRIPTION: Great-circle distance between two points (Haversine formula).
// =============================================================================

double distanceBetween(double lat1, double lon1, double lat2, double lon2); //!< Distance in meters

// =============================================================================
// SECTION: Coordinate Transformations
// DESCRIPTION: Conversions between geographic, flat Earth, and ECEF.
// =============================================================================

FlatXYZ geoToFlatXYZ(double lat, double lon, double alt);   //!< Geographic to local tangent plane
GeoPos flatXYZToGeo(double x, double y, double z);          //!< Local tangent plane to geographic
ECEF geoToXYZ(double lat, double lon, double alt);          //!< Geographic to ECEF
GeoPos xyzToGeo(double x, double y, double z);              //!< ECEF to geographic

// =============================================================================
// SECTION: Navigation Utilities
// DESCRIPTION: Compute new position given starting point, heading, and distance.
// =============================================================================

std::tuple<double, double> calculateNewLatLong(double lat1, double lon1, double heading, double DISTANCE_KM);
double convertToClockwise360(double lon180);                //!< Converts -180..180 longitude to 0..360

#endif // ENTITYUTILS_H
