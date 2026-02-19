#ifndef RADAR_H
#define RADAR_H

#include "core/Hierarchy/EntityProfiles/sensor.h"

class Radar : public Sensor
{
    Q_OBJECT
public:
    explicit Radar(Hierarchy* h);
    bool on = false;

    ///Emmison
    float power = 100;//kw
    float frequency = 8.00;//ghz

    //Envolope
    float minAzimuth = 0;//deg
    float maxAzimuth = 60;//deg
    float minElevation = 0;//deg
    float maxElevation = 60;//deg

    //Scanning
    float rate = 1;
    float hits = 2;

    //Sensor Antenna
    float AntennaGain = 1;
    float AntennaBandwidth = 1;
    float beamWidth = 1;
    int scanType = 0;
    int scanTime1 = 0;
    int scanTime2 = 0;
    float peakSideLobLevel = 1;
    float avgSideLobLevel = 1;

    //Sensor Pulse
    float pulseWidth = 1;




    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
};

#endif // RADAR_H
