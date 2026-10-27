#ifndef IRSENSOR_H
#define IRSENSOR_H

#include "eoSensorLib/eo.h"
#include "eoSensorLib/eo_ir.h"
#include <QObject>

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/profilecategaory.h"
#include "eoSensorLib/eovision.h"
#include "eoSensorLib/georelativeangle.h"
#include "eoSensorLib/georelativeelevation.h"
#include "EOIRSensorLib/IRSensorLib/irsensorlib.h"
// #include "eoSensorLib/payload.h"
#define M_PI 3.14159265358979323846


class IRSensor: public Sensor
{
    Q_OBJECT
public:
    Hierarchy* m_h;
    EOVision eoVision;
    GeoRelativeAngle gra;
    GeoRelativeElevation gre;
    explicit IRSensor(Hierarchy* h);
    std::unordered_map<std::string, Platform*> m_Platforms;
    std::unordered_map<std::string, Specialzone*> m_Specialzones;

    Surrounding getSurrounding(float radius   = 10000.0,
                               float atmCoeff = 0.00001,  // %
                               float rainRate = 0.0,      // mm/h
                               float fog      = 10000.0,  // %
                               float humidity = 0.5       // %
                               );
    EO_IR* eo_ir;

    // For DO178C
    IRSensorLib ir;

    // Helper: Convert degrees to radians
    double deg2rad(double degrees);
    // Helper function to convert degrees to radians
    double degToRad(double degrees);
    // Helper: Geodetic (Lat, Lon, Alt) to ECEF (X, Y, Z)
    void GeodeticToECEF(double lat, double lon, double alt, double &X, double &Y, double &Z);
    // Main Function: Calculate relative OpenGL coordinates and rotations

    /*               Get Resolution Angle Start         */

    float getVeticalAngle(std::pair<int,int> resolution);
    float distance         = 0;
    float verticalAngle    = 0;
    float horizonatalAngle = 0;

    /*                Get Resolution Angle End          */

    /*                Under the parameter or not        */
    bool isUnderRangeNAngle(float t_distance, float t_verticalAngle, float t_horizonatalAngle);

    float gimbalAzimuthOffset   = 0.0f;
    float gimbalElevationOffset = 0.0f;

    /*                      Detection                   */
    QString BoolToStr[2] = {"False","True"};

    // ==========================================
    // 2. Detection Probability Formula
    // ==========================================

    float calculateDetectionProbability(const EOIR_Environment& env, const EOIR_Target& tgt, const EOIR_Sensor& sensor);

    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

    /*---------------   Resolution Start   ------------------*/
    Resolutions currentResolution = Resolutions::FourK;

    /*----------------   Resolution End   -------------------*/

    //******************* Inputs *******************//
    // Unit convertion formula
    float celsius_to_kelvin(float celsius){
        return celsius + 273.15f;
    }

    // Sensor
    // Light Emittion
    float wavelength = 5.0e-6; // (MWIR example) m 5.0e-6

    // for target range
    float calculate3DDistance(const Coordinate& point1, const Coordinate& point2);

    // Target
    // Temperature
    float target_temperature       = 273 ;
    float target_temperature_plume = 650;
    float target_temperature_skin  = 310;
    // Dimension
    float target_range  = 0;
    float target_length = 25;
    float target_cross_sectional_area = 12.0f;
    float detector_area = 2.25e-10;

    // Environment
    float atmospheric_transmission   = 0.95;    // 0-1 τ_atm
    float background_temperature     = 20.0 ;   // Celsius
    float background_temperature_deg = 35.0 ;   // Celsius
    float background_temperature_K   = 35.0+273;   // Celsius
    float air_temperature_deg = 15;
    float air_temperature_k = 288.15;

    float readout_noise      = 5.0f; //  e⁻ RMS
    float thermal_noise      = 2.0f; //  e⁻ RMS
    float temporal_noise_1f  = 1.0f; //  e⁻ RMS

    float relative_humidity = 0;

    //******************* Inputs *******************//

    //***************** Derivative *****************//
    float photon_shot_noise = 1;
    float photon_signal_electrons = 1;

    float radiant_flux_at_detector;
    float quantum_efficiency = 0.8;
    // float wavelength;
    // float detector_area;
    // float integration_time;
    // For radiant_flux_at_detector
    // float target_length;
    float optics_area;
    float target_solid_angle;
    // float atmospheric_transmission;
    float optics_transmission = 0.9f; // 0-1;
    // For optics_area
    float aperture_diameter = 0.10; // m;
    // float target_cross_sectional_area;
    // float target_range;
    // For N_Cycles
    float target_size;
    float IFOV = 50;
    // float target_range;
    // For IFOV
    float pixel_pitch;
    float focal_length;

    // SNR for LWIR
    // float target_temperature;
    // float background_temperature;
    // float atmospheric_transmission;
    float NETD     = 24;
    // float radiant_flux_at_detector;
    // float background_temperature;
    std::pair<double,double> sensor_waveband;
    int steps;

    // for dust_extinction
    float visibility_m = 25000.0f;          // Visibility in meters
    float mass_density_g_m3 = 5;     // Dust mass density (g/m³)
    float range_km;              // Target range (km)
    float particle_diameter_m  = 0.001953f;   // Mean particle diameter (meters)
    float material_density_g_m3 = 2650000.0f; // Density of dust material (e.g., Quartz ~ 2.65e6 g/m³)
    float q_ext = 2;                 // Extinction efficiency factor

    // for SensorAssessment
    float number_concentration_m3; // C_dust (particles/m³)
    float sigma_dust_m1;           // Extinction coefficient (m^-1)
    float attenuation_dB_km;       // Specific attenuation (dB/km)
    float total_loss_dB;           // L_dust (dB)
    // bool  detection_impossible;
    // bool  window_abrasion_possible;
    float snow_precipitation_rate;
    float snow_Extinction;
    float rain_precipitation_rate;
    float rain_Extinction;

    // Fog
    float visibility_range;
    // Atmospheric_Transmission_Mode
    enum class Atmospheric_Mode {
        MWIR_Clear_Air = 0,
        MWIR_High_Altitude,
        MWIR_Light_Haze,
        MWIR_Moderate_Haze,
        LWIR_Clear_Air,
        SWIR_Clear_Air
    };
    float Atmospheric_Mode_Value[6] = {
        0.00812f,
        0.00166f,
        0.02377f,
        0.06931f,
        0.00639f,
        0.00828f
    };
    float Atmospheric_Transmission(Atmospheric_Mode mode, float target_range);
    //***************** Derivative *****************//


    // Output
    float SNR_MWIR = 25;

    float SNR_LWIR = 25;
    // Johnson cycles
    float N_Cycles = 12;
    float tau_MWIR = 12;
    float tau_LWIR = 12;
    float tau_SWIR = 12;

    float L_Snow = 0;
    float L_Rain = 0;

    float dust_extinction ;
    float fog_transmission;

    // Detection Function :
    float SNR_thresh = 3.0;


    // float calculatePd(float SNR, float SNR_thresh, float sigma_SNR);
    // float calculatePfa(float SNR_thresh, float sigma_noise);
    // float calculatePfaOptimized(float SNR_thresh, float sigma_noise);
    // float calculateSigmaSNR(float target_expected_SNR);
    // --- Sensor & Environment Configuration ---
    // --- Sensor & Environment Configuration ---
    struct SensorEnvironmentConfig {
        float NEI;
        float extinction_coeff;
        float sigma_noise;
        bool deterministic_mode; // Toggle between realistic statistical noise and consistent arcade-style results
    };
    /**
     * Calculates the Probability of Detection (Pd)
     * P_d = 0.5 * [1 + erf((SNR - SNR_thresh) / (sqrt(2) * sigma_SNR))]
     * * @param current_snr The currently observed Signal-to-Noise Ratio
     * @param snr_thresh  The Signal-to-Noise Ratio threshold
     * @param sigma_snr   The standard deviation of the SNR
     * @return Probability between 0.0f and 1.0f
     */
    float getProbabilityOfDetection(float current_snr, float snr_thresh, float sigma_snr) const;

    /**
     * Calculates the Probability of False Alarm (Pfa)
     * P_fa = 0.5 * [1 - erf(SNR_thresh / (sqrt(2) * sigma_noise))]
     * * @param snr_thresh  The Signal-to-Noise Ratio threshold
     * @param sigma_noise The standard deviation of the noise
     * @return Probability between 0.0f and 1.0f
     */
    float getProbabilityOfFalseAlarm(float snr_thresh, float sigma_noise) const;

    /**
     * Determines if a signal is definitively detected based on the threshold.
     * @param current_snr The currently observed Signal-to-Noise Ratio
     * @param snr_thresh  The Signal-to-Noise Ratio threshold
     * @return true if current SNR is greater than or equal to the threshold, false otherwise
     */
    bool isSignalDetected(float current_snr, float snr_thresh) const;

    float relativeHumidity     = 40   ;       // % (0.0 to 100.0)
    float absoluteHumidity     = 8.0 ;       // g/m^3
    float rainfallRate         = 0.0  ;       // mm/hr
    float snowfallEquivalent   = 0.0  ;       // mm/hr
    float ambientTemp          = 30   ;       // Celsius

    float aerosolConcentration = 0.05 ;       // mg/m^3
    float baseExtinctionCoeff  = 0.05 ;       // Base sigma (1/km)
    float ambientIlluminance   = 80000;       // lux
    float solarIrradiance      = 900  ;       // W/m^2 (Crucial for Glint)

    void setEnvironment(float relativeHumidity,
                        float absoluteHumidity,
                        float rainfallRate,
                        float snowfallEquivalent,
                        float ambientTemp,
                        float backgroundTemp,
                        float aerosolConcentration,
                        float baseExtinctionCoeff,
                        float ambientIlluminance,
                        float solarIrradiance    );

    void setEnvironmentToDefault();
    // Sensor
    float slantRange      = 40;     // km
    float mrtd            = 0.08;  // Minimum Resolvable Temp Difference (Celsius)
    float sunPhaseAngle   = 90;    // Degrees (0 = perfect alignment for direct reflection into sensor)
    float saturationLimit = 50;    // Max apparent delta-T before the sensor blooms/blinds (Celsius)

    /*--------------   Selection List End  ------------------*/
    /*------------    Custom Debugger Start    ------------*/
private:
    qint64 m_lastScanMs = 0;

public:
    typedef enum {
        D_NULL            = 0b10000000000000,
        D_JustPrint       = 0b01000000000000,
        D_INIT            = 0b00100000000000,
        D_Details         = 0b00010000000000,
        D_Mesh            = 0b00001000000000,
        D_Geometry        = 0b00000100000000,
        D_OpenGL          = 0b00000010000000,
    }debugSQLite;
    Q_ENUM(debugSQLite)
};

#endif // IRSENSOR_H
