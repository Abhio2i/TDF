#ifndef SONOBUOY_H
#define SONOBUOY_H

#include "core/Hierarchy/EntityProfiles/weapon.h"

class Sonobuoy: public Weapon
{
    Q_OBJECT
public:
    struct SonobuoyOutput{
        uint32_t buoyID;             // Unique identifier for the buoy
        double timestamp;            // Simulation time of transmission
        Platform *entity;
        // Positional Data
        double latitude;         // Current GPS lat (including drift)
        double longitude;        // Current GPS lon
        float depth;             // Hydrophone deployment depth (meters)

        float radius;
        // Acoustic Output
        float bearing;           // Magnetic bearing to detected noise (0-360)
        float signalStrength;    // Signal-to-noise ratio in dB
        float dopplerShift;      // Frequency shift for moving targets
        float* rawSpectrum;      // Pointer to frequency bin data (LOFAR)
        bool isScuttled;             // Status of the buoy (Active/Dead)
    };

    explicit Sonobuoy(Hierarchy* h);
    ~Sonobuoy();
    WeaponType weaponType = WeaponType::Sonobuoy;
    QString weaponTypeName() const override { return "Sonobuoy"; }

    bool drop = false;
    float time = 0.f;//s
    float range = 100;//km
    float TransmissionRange = 100;//km
    float Depth = 500;//m
    float Life = 2.f;//hours

    std::unordered_set<Platform*> detects;
    QVector<SonobuoyOutput> detection;

    void dropped();
    void Update() override;


    QJsonObject toJson()                        const override;
    void        fromJson(const QJsonObject&)          override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

};

#endif // SONOBUOY_H
