#ifndef STANAG_4347_H
#define STANAG_4347_H

class STANAG_4347
{
public:
    STANAG_4347();

    // Minimum detectable temperature difference
    // minimum_detectable_delta_t
    float MDTD = 0;

    /*===========    Nominal static range    ===========*/
    // Nominal static Detection range in km
    float NSDR;
    // Nominal static Recognition range in km
    float NSRR;
    // Nominal static Identification range in km
    float NSIR;

    /*===============    Target Size    ================*/
    // Target Size in Meters
    struct TargetSize{
        float x = 2.3;
        float y = 2.3;
    };
    TargetSize targetSize;

    /*===============    Temperature    ================*/
    // Target Temperature
    float targetTemp = 0;
    // Background Temperature
    float backgroundTemp = 0;
    // Minimum Delta Temperature
    const float minDeltaTemp = 2;
    // Delta Temperature
    float deltaTemp(float targetTemp, float backgroundTemp);

    /*=========    Atmospheric Transmission    =========*/

    // For Extinction Coefficient
    const float good_transmission_condition = 0.2;
    const float bad_transmission_condition  = 0.1;

    // atmospheric IR-transmission, characterised within the
    // relevant spectral ranges(km)
    float atmTransmission(float extinctionCoeff, float range);

    // Resolution criteria, according to a 50% probability:
    enum class Resolution_Criteria {
        Detection      = 1, // Line Pair  / Target
        Recognition    = 3, // Line Pairs / Target
        Identification = 6, // Line Pairs / Target
    };

    // Change in sensor signal per unit temperature
    float deltaSignalByDeltaTemp = 1;
    // σ = RMS sensor noise
    float rmsNoise = 1;
    //  Noise equivalent temperature difference (NETD)
    float NETD(float rmsNoise, float deltaSignalByDeltaTemp);

    //  MTF (Modulation Transfer Function)
    float outputContrast = 1;
    float inputContrast = 1;
    float MTF(float outputContrast, float inputContrast);

    // Minimum resolvable temperature difference
    // minimum_resolvable_delta_t
    float MRTD(float NETDValue, float MTFValue);
    // MRTD Average
    float MRTDAverage(float MRTDVertical, float MRTDHorizontal);

    // Calculate Nominal Static Ranges (km)
    float calculateNominalStaticRanges(float MRTDAverage , float deltaTemp,
        TargetSize targetSize, Resolution_Criteria resolution_criteria);

    /*===========    Nominal static range    ===========*/


};

#endif // STANAG_4347_H
