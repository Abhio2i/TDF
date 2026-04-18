#ifndef GEORELATIVEANGLE_H
#define GEORELATIVEANGLE_H

class GeoRelativeAngle
{
public:
    // Computes the relative angle between sensor heading and target bearing.
    // All angles in degrees.
    // Returns angle in range [-180, +180]
    static double computeRelativeAngle(double sensorLatDeg,
                                       double sensorLonDeg,
                                       double sensorHeadingDeg,
                                       double targetLatDeg,
                                       double targetLonDeg);

private:
    static double degToRad(double deg);
    static double radToDeg(double rad);
    static double normalizeAngle(double angleDeg);
};

#endif // GEORELATIVEANGLE_H
