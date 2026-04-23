// =============================================================================
// FILE:        geocords.h
// MODULE:      Geographic Coordinates
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Geocords class, which represents a position on
//              Earth using latitude, longitude, altitude, and heading.
//              Provides conversion between geographic coordinates and local
//              Cartesian vectors (World coordinate system), distance
//              calculation between two points, and JSON serialisation.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef GEOCORDS_H
#define GEOCORDS_H

#include <QObject>
#include "./vector.h"
#include <QJsonObject>

// =============================================================================
// CLASS: Geocords
//
// DESCRIPTION: Geographic coordinate container using latitude/longitude in
//              degrees, altitude in metres (or feet based on context), and
//              heading in degrees clockwise from north. Provides methods to
//              convert to/from local Cartesian coordinates and compute
//              great-circle distances.
// =============================================================================
class Geocords: public QObject
{
    Q_OBJECT
public:
    explicit Geocords(double latitude = 0.0f, double longitude = 0.0f, double altitude = 0.0f, double Heading = 0.0f, QObject* parent = nullptr);

    // =========================================================================
    // SECTION: Coordinate Components
    // DESCRIPTION: Geographic values in degrees (lat/lon/heading) and metres/feet (alt).
    // =========================================================================
    double latitude;    //!< Latitude in degrees (-90 to +90)
    double longitude;   //!< Longitude in degrees (-180 to +180)
    double altitude;    //!< Altitude above reference ellipsoid (units context-dependent)
    double Heading;     //!< Heading in degrees clockwise from north

    // =========================================================================
    // SECTION: Coordinate Conversion
    // DESCRIPTION: Transforms between geographic and local Cartesian coordinates.
    // =========================================================================
    Vector toWorld(Geocords *cordinate);        //!< Converts geographic offset to world vector
    Geocords fromWorld(Vector position);        //!< Converts world vector to geographic coordinates

    double distance(Geocords *cordinate1, Geocords *cordinate2); //!< Distance between two geocoords

    // Serialization
    QJsonObject toJson();                       //!< Serialises geocoords to JSON
    void fromJson(const QJsonObject &obj);      //!< Deserialises geocoords from JSON
};

#endif // GEOCORDS_H
