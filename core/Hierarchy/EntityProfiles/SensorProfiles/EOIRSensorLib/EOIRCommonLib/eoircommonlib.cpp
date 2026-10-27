#include "eoircommonlib.h"

// Define constants using float literals
constexpr float PI = 3.14159265f;
constexpr float DB_CONVERSION_FACTOR = 4.34294481f; // 10 / ln(10)



EOIRCommonLib::EOIRCommonLib()
{

}



// Struct to store look-up data for the spectral absorption profile of pure water
struct WaterAbsorptionData {
    double wavelength; // in nanometers (nm)
    double a_w;        // absorption coefficient (m^-1)
};

class WaterOpticalPropertiesSolver {
private:
    // Lookup table populated with benchmark Pope & Fry (1997) data points
    // This covers critical milestones across the visible light spectrum
    std::vector<WaterAbsorptionData> absorption_lut = {
        {400.0, 0.00663}, {410.0, 0.00473}, {420.0, 0.00454}, {430.0, 0.00495},
        {440.0, 0.00635}, {450.0, 0.00921}, {460.0, 0.00979}, {470.0, 0.0106},
        {480.0, 0.0127},  {490.0, 0.0150},  {500.0, 0.0204},  {510.0, 0.0325},
        {520.0, 0.0477},  {530.0, 0.0507},  {540.0, 0.0558},  {550.0, 0.0638},
        {560.0, 0.0708},  {570.0, 0.0799},  {580.0, 0.108},   {590.0, 0.157},
        {600.0, 0.244},   {610.0, 0.289},   {620.0, 0.309},   {630.0, 0.318},
        {640.0, 0.329},   {650.0, 0.340},   {660.0, 0.410},   {670.0, 0.430},
        {680.0, 0.465},   {690.0, 0.510},   {700.0, 0.650}
    };

    // Linear interpolation to resolve intermediate wavelengths within the lookup table
    double interpolateAbsorption(double wavelength_nm) {
        // Handle boundary overruns gracefully
        if (wavelength_nm <= absorption_lut.front().wavelength) return absorption_lut.front().a_w;
        if (wavelength_nm >= absorption_lut.back().wavelength) return absorption_lut.back().a_w;

        // Find the upper bounding iterator
        auto it = std::upper_bound(absorption_lut.begin(), absorption_lut.end(), wavelength_nm,
                                   [](double val, const WaterAbsorptionData& data) {
                                       return val < data.wavelength;
                                   });

        auto prev_it = it - 1;

        // Compute structural slope weight
        double fraction = (wavelength_nm - prev_it->wavelength) / (it->wavelength - prev_it->wavelength);
        return prev_it->a_w + fraction * (it->a_w - prev_it->a_w);
    }

public:
    // Pure Water Scattering Component b_w(λ) using fluctuation theory power law
    double calculatePureWaterScattering(double wavelength_nm) {
        const double b_ref = 0.00288; // Reference scattering at 500nm in m^-1
        const double lambda_ref = 500.0;

        // Spectral dependency exponent modeled for density variations in pure liquid medium
        return b_ref * std::pow(lambda_ref / wavelength_nm, 4.32);
    }

    // Calculates the total attenuation cross-section: sigma_water = a_w + b_w
    double calculateSigmaWater(double wavelength_nm) {
        double a_w = interpolateAbsorption(wavelength_nm);
        double b_w = calculatePureWaterScattering(wavelength_nm);

        return a_w + b_w;
    }
};

float EOIRCommonLib::Photon_Shot_Noise(Photon_Shot_Noise_Struct photon_shot_noise)
{
    return std::sqrt(photon_shot_noise.photon_signal_electrons);
}

float EOIRCommonLib::Photon_Signal_Electrons(
    Photon_Signal_Electrons_Struct photon_signal_electrons)
{
    constexpr double h = 6.62607015e-34;
    constexpr double c = 2.99792458e8;

    return   photon_signal_electrons.radiant_flux_at_detector
           * photon_signal_electrons.quantum_efficiency *
            (photon_signal_electrons.wavelength / (h * c)) *
             photon_signal_electrons.detector_area *
             photon_signal_electrons.integration_time;
}

float EOIRCommonLib::Radiant_Flux_At_Detector(
    Radiant_Flux_At_Detector_Struct radiant_flux_at_detector)
{
    return  radiant_flux_at_detector.target_length
           *radiant_flux_at_detector.optics_area
           *radiant_flux_at_detector.target_solid_angle
           *radiant_flux_at_detector.atmospheric_transmission
           *radiant_flux_at_detector.optics_transmission;
}

float EOIRCommonLib::Optics_Area(
    Optics_Area_Struct optics_area)
{
    return PI*pow((optics_area.aperture_diameter/2),2);
}

float EOIRCommonLib::Target_Solid_Angle(Target_Solid_Angle_Struct target_solid_angle)
{
    return target_solid_angle.target_cross_sectional_area/
           (pow(target_solid_angle.target_range,2));
}

//  Detection Range Tests
float EOIRCommonLib::SNR(SNR_Struct snr)
{
    return (snr.signal_target - snr.signal_bg) / snr.total_noise;
}

float EOIRCommonLib::Total_Noise(
    Total_Noise_Struct total_noise)
{
    return sqrt(pow(total_noise.readout_noise, 2) +
                pow(total_noise.photon_shot_noise, 2) +
                pow(total_noise.thermal_noise, 2) +
                pow(total_noise.temporal_noise_1f, 2));
}


float EOIRCommonLib::τ_atm(float λ){
    return exp(-1*σ_ext(λ) * R);
}

float EOIRCommonLib::σ_ext(float λ)
{
    return σ_mol(λ) + σ_aerosol(λ) + σ_water(λ) + σ_rain(λ) + σ_dust(λ);
}
float EOIRCommonLib::σ_mol(float λ)
{
    return (24 *pow(π, 3) *pow((pow(n, 2) -1), 2)) / (pow(λ, 4) *pow(N, 2) *pow((pow(n, 2) + 2), 2));
}
float EOIRCommonLib::σ_aerosol(float λ)
{
    // Complex m(1.5, -0.01);

    return 0;// calculateSigmaAerosol(λ, m, N0, r_med, sigma_g);
}
float EOIRCommonLib::σ_water(float λ)
{
    WaterOpticalPropertiesSolver solver;
    // double a_w = solver.calculateSigmaWater(λ) - solver.calculatePureWaterScattering(λ);
    // double b_w = solver.calculatePureWaterScattering(λ);
    float sigma = solver.calculateSigmaWater(λ);
    return sigma;
}
float EOIRCommonLib::σ_rain(float λ)
{
    return 0.2*pow(precipitation_rate,0.63);
}
float EOIRCommonLib::σ_dust(float λ)
{
    return dust_loading * (PI * pow(sand_particle_size,2) / 4) * Q_ext();
}
float EOIRCommonLib::Q_ext(){
    return Q_absorption_efficiency + Q_scattering_efficiency;
}


float EOIRCommonLib::N_Cycles(float target_size, float IFOV_urad, float target_range)
{
    // 1. Convert IFOV from microradians to radians (assuming 50 means 50 urad)
    float IFOV_rad = IFOV_urad * 1e-6f;

    // 2. Calculate the physical size of one pixel at the given range
    float projected_pixel_size = IFOV_rad * target_range;

    // 3. Prevent division by zero just like we did in the Canvas code
    if (projected_pixel_size <= 0.0f) {
        return 0.0f;
    }

    // 4. Calculate total pixels across target
    float pixels_across_target = target_size / projected_pixel_size;

    // 5. Convert pixels to cycles (2 pixels = 1 cycle)
    return pixels_across_target / 2.0f;
}

float EOIRCommonLib::IFOV(float pixel_pitch, float focal_length) {
    // Prevent division by zero
    if (focal_length <= 0.0f) {
        return -1.0f;
    }
    // IFOV = p / f
    return pixel_pitch / focal_length;
}


float EOIRCommonLib::calculate_tau_MWIR(const std::function<float(float)>& tau_atm, int steps = 100) {
    float start = 3.0f, end = 5.0f;
    float delta = (end - start) / static_cast<float>(steps);
    float sum = 0.0f;
    for (int i = 0; i < steps; ++i) {
        sum += 0.5f * (tau_atm(start + i * delta) + tau_atm(start + (i + 1) * delta)) * delta;
    }
    return sum / 2.0f;
}

float EOIRCommonLib::calculate_tau_LWIR(const std::function<float(float)>& tau_atm, int steps = 100) {
    float start = 8.0f, end = 12.0f;
    float delta = (end - start) / static_cast<float>(steps);
    float sum = 0.0f;
    for (int i = 0; i < steps; ++i) {
        sum += 0.5f * (tau_atm(start + i * delta) + tau_atm(start + (i + 1) * delta)) * delta;
    }
    return sum / 4.0f;
}

float EOIRCommonLib::calculate_tau_SWIR(const std::function<float(float)>& tau_atm, int steps = 100) {
    float start = 0.9f, end = 1.7f;
    float delta = (end - start) / static_cast<float>(steps);
    float sum = 0.0f;
    for (int i = 0; i < steps; ++i) {
        sum += 0.5f * (tau_atm(start + i * delta) + tau_atm(start + (i + 1) * delta)) * delta;
    }
    return sum / 0.8f;
}

float EOIRCommonLib::τ_MWIR(const std::function<float(float)> tauAtm, int steps)
{
    return computeMwirTransmissionFloat(tauAtm,steps);
}

float EOIRCommonLib::CEP(float sigmaX, float sigmaY) {
    return 0.59f * (sigmaX + sigmaY);
}

float EOIRCommonLib::τ_fog(float V_fog,float R) {
    return  exp(-(3.912 / V_fog) * R);
}


// Mock function using float for atmospheric transmission
float getAtmosphericTransmissionFloat(float lambda) {
    if (lambda >= 4.2f && lambda <= 4.4f) return 0.05f; // CO2 absorption band
    return 0.85f; // High transmission window
}

// Computes the average MWIR band transmission using float
float EOIRCommonLib::computeMwirTransmissionFloat(const std::function<float(float)>& tauAtm, int steps) {
    float startWavelength = 3.0f;
    float endWavelength = 5.0f;
    float deltaLambda = (endWavelength - startWavelength) / static_cast<float>(steps);

    float integralSum = 0.0f;

    // Trapezoidal numerical integration
    for (int i = 0; i < steps; ++i) {
        float lambda1 = startWavelength + static_cast<float>(i) * deltaLambda;
        float lambda2 = lambda1 + deltaLambda;

        // Area of the current trapezoid
        integralSum += 0.5f * (tauAtm(lambda1) + tauAtm(lambda2)) * deltaLambda;
    }

    // Final formula calculation (divided by 2)
    return integralSum / 2.0f;
}

float EOIRCommonLib::getNETD(getNETD_Struct netd){
    double dL_dT =  calculate_dL_dT(netd.sensor_waveband.first,
                        netd.sensor_waveband.second,
                        netd.background_temperature,
                        netd.steps);
    return NETD(dL_dT,netd.radiant_flux_at_detector);

}

float EOIRCommonLib::NETD(float dL_dT, float radiant_flux_at_detector){
    return radiant_flux_at_detector/dL_dT;
}

float EOIRCommonLib::SNR(SNR_LWIR_Struct snr){
    return (std::abs(snr.atmospheric_transmission- snr.background_temperature)
            *snr.atmospheric_transmission)/snr.NETD;
}

// Constants for Planck's Law derivative
const double c1 = 1.191042e8; // W * um^4 / (m^2 * sr)
const double c2 = 14387.752;  // um * K

// Function to calculate dL/dT for a specific wavelength and temperature
double EOIRCommonLib::spectralThermalContrast(double lambda_um, double temp_K) {
    double exponent = c2 / (lambda_um * temp_K);
    double exp_term = std::exp(exponent);

    // Protect against division by zero or extreme limits
    if (std::isinf(exp_term)) return 0.0;

    double term1 = c1 / (std::pow(lambda_um, 5) * std::pow(exp_term - 1.0, 2));
    double term2 = (c2 * exp_term) / (lambda_um * std::pow(temp_K, 2));

    return term1 * term2;
}

// Integrate over the sensor band (e.g., 8 to 12 um for LWIR)
double EOIRCommonLib::calculate_dL_dT(double lambda_start, double lambda_end, double temp_K, int steps) {
    double deltaLambda = (lambda_end - lambda_start) / steps;
    double integralSum = 0.0;

    for (int i = 0; i < steps; ++i) {
        double lambda1 = lambda_start + i * deltaLambda;
        double lambda2 = lambda1 + deltaLambda;

        integralSum += 0.5 * (spectralThermalContrast(lambda1, temp_K) +
                              spectralThermalContrast(lambda2, temp_K)) * deltaLambda;
    }
    return integralSum;
}

// Environmental Extremes Tests
// Rain and Snow Rate
float EOIRCommonLib::Rain_Extinction(float precipitation_rate)
{
    return 0.2*pow(precipitation_rate,0.63);
}
float EOIRCommonLib::L_Rain(float rain_extinction, float target_range)
{
    return 2*rain_extinction*target_range;
}

float EOIRCommonLib::Snow_Extinction(float precipitation_rate)
{
    return 0.1*pow(precipitation_rate,0.7);
}
float EOIRCommonLib::L_Snow(float snow_extinction, float target_range)
{
    return 2*snow_extinction*target_range;
}

float EOIRCommonLib::Fog_Transmission(float visibility_range, float target_range)
{
    return exp(-1*((3.912/visibility_range)*target_range));
}

/****************** SnadStorm Start ********************/

// ---------------------------------------------------------
// 1. Core Physics Constants
// ---------------------------------------------------------
namespace AtmosphericConstants {
    constexpr float PI = 3.14159265f;
    constexpr float DB_CONVERSION_FACTOR = 4.34294481f; // 10 / ln(10)
    constexpr float OPTICAL_FAILURE_THRESHOLD_DB = 70.0f;
    constexpr float ABRASION_THRESHOLD_G_M3 = 2.0f;
}

// ---------------------------------------------------------
// 2. Pure Micro-Physics Functions
// ---------------------------------------------------------
namespace DustPhysics {

    inline float calculateParticleMass(float diameter_m, float material_density_g_m3) {
        float radius = diameter_m / 2.0f;
        float volume = (4.0f / 3.0f) * AtmosphericConstants::PI * std::pow(radius, 3.0f);
        return volume * material_density_g_m3;
    }

    inline float calculateNumberConcentration(float mass_density_g_m3, float single_particle_mass_g) {
        return mass_density_g_m3 / single_particle_mass_g;
    }

    inline float calculateExtinctionCoefficient(float concentration_m3, float diameter_m, float q_ext) {
        float cross_section = (AtmosphericConstants::PI * diameter_m * diameter_m) / 4.0f;
        return concentration_m3 * cross_section * q_ext;
    }

    inline float convertExtinctionToAttenuation(float sigma_m1) {
        return sigma_m1 * 1000.0f * AtmosphericConstants::DB_CONVERSION_FACTOR;
    }

    inline float calculateTotalPathLoss(float attenuation_db_km, float range_km) {
        return attenuation_db_km * range_km;
    }
}

// ---------------------------------------------------------
// 3. Operational Evaluation Functions
// ---------------------------------------------------------

/****************** SnadStorm End *********************/

namespace SensorEvaluation {

    inline bool isDetectionImpossible(float total_loss_db) {
        return total_loss_db >= AtmosphericConstants::OPTICAL_FAILURE_THRESHOLD_DB;
    }

    inline bool isAbrasionRisk(float mass_density_g_m3) {
        return mass_density_g_m3 > AtmosphericConstants::ABRASION_THRESHOLD_G_M3;
    }
}

float EOIRCommonLib::Dust_Extinction(EOIRCommonLib::DustCondition dust_condition){
    //return dust_loading * (PI * pow(sand_particle_size,2) / 4) * Q_ext();
    // --- Pipeline Execution ---

    // Step 1: Derive the mass of a single particle
    float particle_mass = DustPhysics::calculateParticleMass(dust_condition.particle_diameter_m, dust_condition.material_density_g_m3);

    // Step 2: Determine C_dust (particles/m³)
    float c_dust = DustPhysics::calculateNumberConcentration(dust_condition.mass_density_g_m3, particle_mass);

    // Step 3: Calculate extinction (m^-1)
    float sigma_dust = DustPhysics::calculateExtinctionCoefficient(c_dust, dust_condition.particle_diameter_m, dust_condition.q_ext);

    // Step 4: Convert to dB/km
    float attenuation_db_km = DustPhysics::convertExtinctionToAttenuation(sigma_dust);

    // Step 5: Total Path Loss over range
    float total_loss_db = DustPhysics::calculateTotalPathLoss(attenuation_db_km, dust_condition.range_km);

    return total_loss_db;
    // Step 6: Hardware & Operational Assessment
    // bool detection_failed = SensorEvaluation::isDetectionImpossible(total_loss_db);
    // bool hardware_at_risk = SensorEvaluation::isAbrasionRisk(mass_density_g_m3);
}

// 1. Log-Normal Distribution for Particle Size n(r)
// N0: Total particle density, r_med: Median radius, sigma_g: Geometric standard deviation
double logNormalDistribution(double r, double N0, double r_med, double sigma_g) {
    if (r <= 0.0) return 0.0;
    double log_r_ratio = std::log(r / r_med);
    double log_sigma = std::log(sigma_g);
    double exponent = - (log_r_ratio * log_r_ratio) / (2.0 * log_sigma * log_sigma);
    double denominator = r * log_sigma * std::sqrt(2.0 * PI);
    return (N0 / denominator) * std::exp(exponent);
}

// 2. Bohren-Huffman Mie Theory Solver for Q_ext
// x: size parameter (2*pi*r/lambda), m: relative complex refractive index
double computeMieQext(double x, Complex m) {
    if (x < 1e-9) return 0.0;

    // Determine the number of terms required for convergence (Wiscombe criterion)
    int nmax = static_cast<int>(x + 4.0 * std::pow(x, 1.0 / 3.0) + 2.0);

    // Downward recurrence for the logarithmic derivative of Bessel function D(z)
    Complex z = m * x;
    std::vector<Complex> D(nmax + 1, Complex(0.0, 0.0));
    for (int n = nmax - 1; n >= 0; --n) {
        Complex n_c(n + 1, 0.0);
        D[n] = n_c / z - 1.0 / (D[n + 1] + n_c / z);
    }

    // Upward recurrence for Riccati-Bessel functions psi and chi
    double psi_prev = std::sin(x);
    double psi_curr = psi_prev / x - std::cos(x);
    double chi_prev = -std::cos(x);
    double chi_curr = chi_prev / x - std::sin(x);
    Complex xi_curr(psi_curr, -chi_curr);

    double qext = 0.0;

    for (int n = 1; n <= nmax; ++n) {
        double fn = static_cast<double>(n);
        double psi_next = (2.0 * fn + 1.0) * psi_curr / x - psi_prev;
        double chi_next = (2.0 * fn + 1.0) * chi_curr / x - chi_prev;
        Complex xi_next(psi_next, -chi_next);

        // Derivatives of Riccati-Bessel functions
        Complex psi_prime = psi_next - fn * psi_curr / x;
        Complex xi_prime = xi_next - fn * xi_curr / x;

        // Mie Coefficients an and bn
        Complex an_num = D[n] * psi_curr - m * psi_prime;
        Complex an_den = D[n] * xi_curr - m * xi_prime;
        Complex an = an_num / an_den;

        Complex bn_num = m * D[n] * psi_curr - psi_prime;
        Complex bn_den = m * D[n] * xi_curr - xi_prime;
        Complex bn = bn_num / bn_den;

        // Add to the summation series
        qext += (2.0 * fn + 1.0) * (an.real() + bn.real());

        // Step forward in the recurrence loop
        psi_prev = psi_curr;
        psi_curr = psi_next;
        chi_prev = chi_curr;
        chi_curr = chi_next;
        xi_curr = xi_next;
    }

    return (2.0 / (x * x)) * qext;
}


// 3. Main Integration Routine for sigma_aerosol
double calculateSigmaAerosol(double lambda, Complex m, double N0, double r_med, double sigma_g) {
    double sigma_aerosol = 0.0;

    // Integration bounds for standard atmospheric aerosols (in meters)
    double r_min = 1e-9;       // 1 nanometer
    double r_max = 10e-6;      // 10 micrometers
    int num_steps = 1000;      // Resolution steps
    double dr = (r_max - r_min) / num_steps;

    // Discretized Integration loop using the Trapezoidal Rule
    for (int i = 0; i < num_steps; ++i) {
        double r = r_min + i * dr;

        // Calculate Mie Size Parameter
        double x = (2.0 * PI * r) / lambda;

        // Get components for the integrand
        double Q_ext = computeMieQext(x, m);
        double nr = logNormalDistribution(r, N0, r_med, sigma_g);

        // Integrand = pi * r^2 * Q_ext * n(r)
        double integrand = PI * r * r * Q_ext * nr;

        // Summing up the areas (dr)
        sigma_aerosol += integrand * dr;
    }

    return sigma_aerosol;
}

