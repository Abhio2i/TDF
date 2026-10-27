#ifndef EOIRCOMMONLIB_H
#define EOIRCOMMONLIB_H
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>
#include <functional>
// Inputs
struct Sensor_Physical_Parameters {};
struct Environmental_Conditions {};
struct Target_Parameters {};
struct Platform_Parameters {};
struct Gimbal_N_Mount_Parameters {};
struct Laser_Parameters {};
struct Tracking_Algorithm_Parameters {};
struct ATR_N_AI_Detection_Parameters {};
struct Video_Processing_Parameters {};
struct Calibration_Parameters {};
struct Countermeasures_Parameters {};
struct Operational_Parameters {};
struct Terrain_N_Background_Parameters {
    float measurement_noise_covariance;
};
struct Temporal_N_Dynamic_Parameters {};

constexpr float π = 3.14;
// const double PI = 3.14159265358979323846;
using Complex = std::complex<double>;


class EOIRCommonLib
{
public:
    EOIRCommonLib();
    std::string sensorName = "";

    // Requirements
    // Lens
    float aperture_diameter = 0.10; // 350 mm
        // Sensor Type	Typical Aperture Diameter
        // Small UAV EO camera	20–50 mm
        // Commercial surveillance camera	25–75 mm
        // Vehicle-mounted EO/IR sight	80–200 mm
        // Airborne targeting pod	150–400 mm
        // Long-range military EO/IR system	200–500 mm+
    float optics_transmission = 0.9f; // 0-1
        // Ideal optics (ignore losses)	1.0
        // High-quality coated optics	0.90 – 0.98
        // Typical EO camera optics	0.80 – 0.95
        // Complex multi-element optics	0.70 – 0.90
        // Poor/dirty optics	< 0.70
    float wavelength = 5.0e-6; // (MWIR example) m 5.0e-6
    float pixel_pitch;
    float focal_length;
    float quantum_efficiency = 0.8; // quantum_efficiency	QE	%	Detector quantum efficiency



    // Targets
    float target_cross_sectional_area = 10.09f;
    float target_length = 25;
    float detector_area = 2.25e-10;
    float target_range  = 1000;
    float signal_target = 550;
    float target_temperature = 550.0f;
    float target_temperature_plume = 650;
    float target_temperature_skin  = 310;

    // Environment
    float atmospheric_transmission  = 0.95; // 0-1
    float background_temperature    = 269;
        // Clear atmosphere	0.8 – 0.95
        // Ideal / vacuum / ignored effects	1.0
        // Clear atmosphere	0.8 – 0.95
        // Haze / light fog	0.5 – 0.8
        // Heavy fog / rain	0.1 – 0.5
    float signal_bg = 269;
    // Environmental Extremes Tests
    // Rain and Snow Rate
    float precipitation_rate_rain = 0; // mm/hr
    float precipitation_rate_snow = 0; // mm/hr
    float visibility_range = 500;
    // Sand Storm
    float dust_loading = 0; // C_dust mg/m³	Dust mass concentration
    float sand_particle_size = 12;	// d_sand	μm	Sand particle diameter (mean)

    // Environmental Inputs
    // Sand Storm
    float mass_density_g_m3     = 5.0f;  // g/m^3
    float particle_diameter_m   = 0.001953f;  // meter
    float material_density_g_m3 = 2650000.0f; // Quartz sand
    float q_ext                 = 2.0f;

    // Noise
    float readout_noise      = 5.0f; //  e⁻ RMS
    float thermal_noise      = 2.0f; //  e⁻ RMS
    float temporal_noise_1f  = 1.0f; //  e⁻ RMS

    float integration_time   = 0.01; // second
    std::pair<double,double> sensor_waveband;

    //  Detection Range Tests
    struct SNR_Struct{
        float signal_target  ; //  e⁻
        float signal_bg      ; //  e⁻
        float total_noise    ; //  e⁻ RMS
    };
    float SNR(SNR_Struct snr);
        // For SNR
        struct Total_Noise_Struct
        {
            float readout_noise;
            float photon_shot_noise;
            float thermal_noise;
            float temporal_noise_1f;
        };
        float Total_Noise(Total_Noise_Struct total_noise);

        // Total_Noise
        struct Photon_Shot_Noise_Struct
        {
            float photon_signal_electrons; //  e⁻
        };
        float Photon_Shot_Noise(){
            return 1.0f;
        }
        float Photon_Shot_Noise(Photon_Shot_Noise_Struct photon_shot_noise);

        // Photon_Shot_Noise
        struct Photon_Signal_Electrons_Struct {
            float radiant_flux_at_detector;
            float quantum_efficiency;
            float wavelength;
            float detector_area;
            float integration_time;
        };
        float Photon_Signal_Electrons(Photon_Signal_Electrons_Struct photon_signal_electrons);

        // For Photon_Signal_Electrons
        struct Radiant_Flux_At_Detector_Struct{
            float target_length;
            float optics_area;
            float target_solid_angle;
            float atmospheric_transmission;
            float optics_transmission;
        };
        float Radiant_Flux_At_Detector(Radiant_Flux_At_Detector_Struct radiant_flux_at_detector);

        // For Radiant_Flux_At_Detector
        struct Optics_Area_Struct {float aperture_diameter;};
        float Optics_Area(Optics_Area_Struct optics_area);

        // For Radiant_Flux_At_Detector
        struct Target_Solid_Angle_Struct{
            float target_cross_sectional_area;
            float target_range;
        };
        float Target_Solid_Angle(Target_Solid_Angle_Struct target_solid_angle);




    struct getNETD_Struct{
        float radiant_flux_at_detector;
        float background_temperature;
        std::pair<double,double> sensor_waveband;
        int steps;
    };
    float getNETD(getNETD_Struct netd);
        float NETD(float dL_dT, float radiant_flux_at_detector);
        // Function to calculate dL/dT for a specific wavelength and temperature
        double spectralThermalContrast(double lambda_um, double temp_K);
        // Integrate over the sensor band (e.g., 8 to 12 um for LWIR)
        double calculate_dL_dT(double lambda_start, double lambda_end, double temp_K, int steps);

    struct SNR_LWIR_Struct {
        float target_temperature;
        float background_temperature;
        float atmospheric_transmission;
        float NETD;
    };
    float SNR(SNR_LWIR_Struct snr);

    // Johnson cycles
    float N_Cycles(float target_size, float IFOV, float target_range);
    float IFOV(float pixel_pitch, float focal_length);

    float calculate_tau_MWIR(const std::function<float(float)>& tau_atm, int steps);
    float calculate_tau_LWIR(const std::function<float(float)>& tau_atm, int steps);
    float calculate_tau_SWIR(const std::function<float(float)>& tau_atm, int steps);

    float Spectral_Transmission_Curve(float );
    // flaot Atmospheric_Transmission_Total();

    // Environmental Extremes Tests
    // Rain and Snow Rate
    float Snow_Extinction(float precipitation_rate);
    float L_Snow(float rain_extinction, float target_range); // range km
    float Rain_Extinction(float precipitation_rate);
    float L_Rain(float snow_extinction, float target_range); // range km

    // Fog
    float Fog_Transmission(float visibility_range, float target_range);  // range km

    // Dust
    // σ_dust = C_dust × (π × d_sand² / 4) × Q_ext
    struct DustCondition {
        float visibility_m;          // Visibility in meters
        float mass_density_g_m3;     // Dust mass density (g/m³)
        float range_km;              // Target range (km)
        float particle_diameter_m;   // Mean particle diameter (meters)
        float material_density_g_m3; // Density of dust material (e.g., Quartz ~ 2.65e6 g/m³)
        float q_ext;                 // Extinction efficiency factor
    };

    struct SensorAssessment {
        float number_concentration_m3; // C_dust (particles/m³)
        float sigma_dust_m1;           // Extinction coefficient (m^-1)
        float attenuation_dB_km;       // Specific attenuation (dB/km)
        float total_loss_dB;           // L_dust (dB)
        bool detection_impossible;
        bool window_abrasion_possible;
    };

    float Dust_Extinction(DustCondition dust_condition);

    float R; // measurement_noise_covariance
    float τ_atm(float λ);

    float σ_ext(float λ);
    float σ_mol(float λ);
        float n; // The refractive index of the medium (e.g., air).
        float N; // The number density of the molecules (number of molecules per unit volume).
    float σ_aerosol(float λ);
        double computeMieQext(double x, Complex m);
        double logNormalDistribution(double r, double N0, double r_med, double sigma_g);
        double calculateSigmaAerosol(double lambda, Complex m, double N0, double r_med, double sigma_g);
                         // Refractive index of dust/carbon (real and imaginary part)
        // Aerosol Distribution Parameters
        double N0 = 1e9;                       // 1 billion particles per cubic meter
        double r_med = 0.1e-6;                 // Median radius of 0.1 microns
        double sigma_g = 1.5;                  // Geometric standard deviation
    float σ_water(float λ);
    float σ_rain(float λ);
        float precipitation_rate;
    float σ_dust(float λ);

    float Q_ext();
        float Q_absorption_efficiency;
        float Q_scattering_efficiency;
    // float N_cycles(float Target_Size, float IFOV, float R); // Johnson cycles

    float τ_MWIR(const std::function<float(float)> tauAtm, int steps);
    float computeMwirTransmissionFloat(const std::function<float(float)>& tauAtm, int steps);
    float CEP(float sigmaX, float sigmaY);

    float τ_fog(float V_fog,float R);

};


#endif // EOIRCOMMONLIB_H
