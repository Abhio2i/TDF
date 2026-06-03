#ifndef GEO_RELATIVE_ELEVATION_H
#define GEO_RELATIVE_ELEVATION_H

class GeoRelativeElevation
{
public:
    // Returns relative vertical angle (degrees)
    // Positive => target is above sensor pitch
    // Negative => target is below sensor pitch
    static float computeRelativeElevation(float sensorLatDeg,
                                           float sensorLonDeg,
                                           float sensorAltMeters,
                                           float sensorPitchDeg,
                                           float targetLatDeg,
                                           float targetLonDeg,
                                           float targetAltMeters);

private:
    static float degToRad(float deg);
    static float radToDeg(float rad);
    static float distanceMeters(float lat1, float lon1,
                                 float lat2, float lon2);
};

#endif
