// Propagation model configuration (feature toggles + tuning knobs).
// This controls which equations are active inside src/propagation_model.cpp.
#pragma once

namespace radio {

// by codex: model-level configuration shared by all radios in a simulation.
struct PropagationModelConfig {
    // Core feature toggles
    bool enable_los_horizon = true;
    bool enable_comms_mode_losses = true;
    bool enable_polarization_loss = true;
    bool enable_fixed_path_loss_override = true;
    bool enable_sensitivity = true;
    bool enable_squelch = true;
    bool enable_fspl = true;
    bool enable_log_distance = false;
    bool enable_two_ray = false;
    bool enable_shadowing = false;
    bool enable_fading = false;
    bool enable_noise_floor = true;
    bool enable_snr_threshold = true;
    bool enable_interference = false;
    bool enable_scan_beam = false;          // apply antenna beam/sector gating
    bool enable_scan_timing = false;        // time-varying scan angle
    bool enable_range_limit = false;
    bool enable_network_gate_in_scan = true;
    bool enable_doppler = false;

    // Optional higher-fidelity features (placeholders for now)
    bool enable_delay = false;
    bool enable_ber = false;
    bool enable_obstacles = false;
    bool enable_terrain = false;

    // Tunable constants for simplified models
    double los_blocked_loss_db = 200.0;
    double bLOS_diffraction_db_per_m = 0.1;
    double satcom_extra_loss_db = 150.0;
    double troposcatter_log_loss_factor_db = 40.0;
    double polarization_mismatch_loss_db = 3.0;

    // Path loss model parameters
    double log_distance_path_loss_exp = 2.0;
    double log_distance_ref_distance_m = 1.0;

    // Shadowing/fading parameters (log-normal)
    double shadowing_sigma_db = 3.0;
    double fading_sigma_db = 2.0;

    // Interference power (dBm) used as extra noise floor when enabled
    double interference_power_dbm = -120.0;

    // Environmental attenuation (simplified ISA-based model)
    bool enable_environmental_attenuation = true;
    bool enable_sea_attenuation = true;
    // Standard sea-level conditions (ISA)
    double temperature_c = 15.0;
    double pressure_hpa = 1013.25;
    double humidity_percent = 50.0;
    // Gas attenuation baseline at 1 GHz (scaled by frequency and air density)
    double gas_attenuation_db_per_km_at_1ghz = 0.005;
    double gas_attenuation_freq_exponent = 1.0;
    double humidity_attenuation_factor_per_percent = 0.002;
    // Rain attenuation (ITU-R P.838-3 power-law by default)
    double rain_rate_mm_per_hr = 0.0;
    double rain_attenuation_db_per_km_per_mmhr = 0.004;
    bool use_itu_rain_model = true;
    // Optional rain variability/coverage (0 = none). Coverage is fraction of path in rain.
    double rain_coverage = 1.0;
    double rain_rate_sigma_frac = 0.0;
    // Wind attenuation (very small; used as a placeholder)
    double wind_speed_mps = 0.0;
    double wind_attenuation_db_per_km_per_mps = 0.0005;
    // Sea surface attenuation (applied if either endpoint is naval)
    double sea_attenuation_db_per_km = 0.003;
};

} // namespace radio
