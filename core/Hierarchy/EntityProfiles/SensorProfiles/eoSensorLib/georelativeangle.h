#ifndef GEORELATIVEANGLE_H
#define GEORELATIVEANGLE_H

class GeoRelativeAngle
{
public:
    // Computes the relative angle between sensor heading and target bearing.
    // All angles in degrees.
    // Returns angle in range [-180, +180]
    static float computeRelativeAngle(float sensorLatDeg,
                                       float sensorLonDeg,
                                       float sensorHeadingDeg,
                                       float targetLatDeg,
                                       float targetLonDeg);

private:
    static float degToRad(float deg);
    static float radToDeg(float rad);
    static float normalizeAngle(float angleDeg);
};

#endif // GEORELATIVEANGLE_H
