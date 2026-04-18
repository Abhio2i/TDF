#ifndef GEO_RELATIVE_ELEVATION_H
#define GEO_RELATIVE_ELEVATION_H

class GeoRelativeElevation
{
public:
    // Returns relative vertical angle (degrees)
    // Positive => target is above sensor pitch
    // Negative => target is below sensor pitch
    static double computeRelativeElevation(double sensorLatDeg,
                                           double sensorLonDeg,
                                           double sensorAltMeters,
                                           double sensorPitchDeg,
                                           double targetLatDeg,
                                           double targetLonDeg,
                                           double targetAltMeters);

private:
    static double degToRad(double deg);
    static double radToDeg(double rad);
    static double distanceMeters(double lat1, double lon1,
                                 double lat2, double lon2);
};

#endif
