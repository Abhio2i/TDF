#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/RF/RF.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>

namespace rf {

namespace {

constexpr double kEarthRadiusM = 6371000.0;
constexpr double kPi = 3.14159265358979323846;

uint64_t now_ms() {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch()).count());
}

std::mt19937& rng() {
    static thread_local std::mt19937 gen(42);
    return gen;
}

double sanitize_to_zero_or_range(double value,
                                 double min_value,
                                 double max_value,
                                 const char* field_name) {
    if (std::isfinite(value) && value >= min_value && value <= max_value) {
        return value;
    }

    double replacement = (0.0 >= min_value && 0.0 <= max_value)
                             ? 0.0
                             : std::clamp(value, min_value, max_value);
    if (!std::isfinite(value)) {
        replacement = (0.0 >= min_value && 0.0 <= max_value) ? 0.0 : min_value;
    }

    std::cerr << "[rf] Invalid propagation config '" << field_name
              << "'=" << value
              << " outside [" << min_value << ", " << max_value << "]"
              << "; clamping to " << replacement << ".\n";
    return replacement;
}

double sanitize_positive_or_default(double value,
                                    double min_value,
                                    double max_value,
                                    double replacement,
                                    const char* field_name) {
    if (std::isfinite(value) && value >= min_value && value <= max_value) {
        return value;
    }

    std::cerr << "[rf] Invalid RF config '" << field_name
              << "'=" << value
              << " outside [" << min_value << ", " << max_value << "]"
              << "; clamping to " << replacement << ".\n";
    return replacement;
}

double sanitize_nonnegative_or_default(double value,
                                       double max_value,
                                       double replacement,
                                       const char* field_name) {
    if (std::isfinite(value) && value >= 0.0 && value <= max_value) {
        return value;
    }

    std::cerr << "[rf] Invalid RF config '" << field_name
              << "'=" << value
              << " outside [0, " << max_value << "]"
              << "; clamping to " << replacement << ".\n";
    return replacement;
}

RfPropagationConfig sanitize_model_config(const RfPropagationConfig& input) {
    RfPropagationConfig cfg = input;

    cfg.bLOS_diffraction_db_per_m = sanitize_to_zero_or_range(
        cfg.bLOS_diffraction_db_per_m, 0.0, 10.0, "bLOS_diffraction_db_per_m");
    cfg.satcom_extra_loss_db = sanitize_to_zero_or_range(
        cfg.satcom_extra_loss_db, 0.0, 300.0, "satcom_extra_loss_db");
    cfg.troposcatter_log_loss_factor_db = sanitize_to_zero_or_range(
        cfg.troposcatter_log_loss_factor_db, 0.0, 200.0, "troposcatter_log_loss_factor_db");
    cfg.log_distance_path_loss_exp = sanitize_to_zero_or_range(
        cfg.log_distance_path_loss_exp, 1.0, 8.0, "log_distance_path_loss_exp");
    cfg.log_distance_ref_distance_m = sanitize_to_zero_or_range(
        cfg.log_distance_ref_distance_m, 1.0, 1000.0, "log_distance_ref_distance_m");
    cfg.temperature_c = sanitize_to_zero_or_range(cfg.temperature_c, -80.0, 60.0, "temperature_c");
    cfg.pressure_hpa = sanitize_to_zero_or_range(cfg.pressure_hpa, 300.0, 1100.0, "pressure_hpa");
    cfg.humidity_percent = sanitize_to_zero_or_range(cfg.humidity_percent, 0.0, 100.0, "humidity_percent");
    cfg.gas_attenuation_db_per_km_at_1ghz = sanitize_to_zero_or_range(
        cfg.gas_attenuation_db_per_km_at_1ghz, 0.0, 0.1, "gas_attenuation_db_per_km_at_1ghz");
    cfg.gas_attenuation_freq_exponent = sanitize_to_zero_or_range(
        cfg.gas_attenuation_freq_exponent, 0.0, 3.0, "gas_attenuation_freq_exponent");
    cfg.humidity_attenuation_factor_per_percent = sanitize_to_zero_or_range(
        cfg.humidity_attenuation_factor_per_percent, 0.0, 0.02, "humidity_attenuation_factor_per_percent");
    cfg.rain_rate_mm_per_hr = sanitize_to_zero_or_range(
        cfg.rain_rate_mm_per_hr, 0.0, 200.0, "rain_rate_mm_per_hr");
    cfg.rain_attenuation_db_per_km_per_mmhr = sanitize_to_zero_or_range(
        cfg.rain_attenuation_db_per_km_per_mmhr, 0.0, 0.1, "rain_attenuation_db_per_km_per_mmhr");
    cfg.rain_coverage = sanitize_to_zero_or_range(cfg.rain_coverage, 0.0, 1.0, "rain_coverage");
    cfg.rain_rate_sigma_frac = sanitize_to_zero_or_range(
        cfg.rain_rate_sigma_frac, 0.0, 0.5, "rain_rate_sigma_frac");
    cfg.wind_speed_mps = sanitize_to_zero_or_range(cfg.wind_speed_mps, 0.0, 80.0, "wind_speed_mps");
    cfg.wind_attenuation_db_per_km_per_mps = sanitize_to_zero_or_range(
        cfg.wind_attenuation_db_per_km_per_mps, 0.0, 0.01, "wind_attenuation_db_per_km_per_mps");
    cfg.sea_attenuation_db_per_km = sanitize_to_zero_or_range(
        cfg.sea_attenuation_db_per_km, 0.0, 0.05, "sea_attenuation_db_per_km");
    cfg.interference_power_dbm = sanitize_to_zero_or_range(
        cfg.interference_power_dbm, -200.0, 0.0, "interference_power_dbm");
    cfg.shadowing_sigma_db = sanitize_to_zero_or_range(cfg.shadowing_sigma_db, 0.0, 12.0, "shadowing_sigma_db");
    cfg.fading_sigma_db = sanitize_to_zero_or_range(cfg.fading_sigma_db, 0.0, 10.0, "fading_sigma_db");
    cfg.los_blocked_loss_db = sanitize_to_zero_or_range(cfg.los_blocked_loss_db, 0.0, 300.0, "los_blocked_loss_db");

    return cfg;
}

RfConfig sanitize_rf_config(const RfConfig& input) {
    RfConfig cfg = input;
    cfg.bandwidth_hz = sanitize_positive_or_default(
        cfg.bandwidth_hz, 1.0, 100e6, 1.0, "bandwidth_hz");
    cfg.rx_bandwidth_hz = sanitize_nonnegative_or_default(
        cfg.rx_bandwidth_hz, 100e6, 0.0, "rx_bandwidth_hz");
    cfg.receiver.noise_figure_db = sanitize_nonnegative_or_default(
        cfg.receiver.noise_figure_db, 50.0, 0.0, "receiver.noise_figure_db");
    cfg.processing_gain_db = sanitize_nonnegative_or_default(
        cfg.processing_gain_db, 100.0, 0.0, "processing_gain_db");
    cfg.fixed_path_loss_db = sanitize_nonnegative_or_default(
        cfg.fixed_path_loss_db, 400.0, 0.0, "fixed_path_loss_db");
    cfg.propagation = sanitize_model_config(cfg.propagation);
    return cfg;
}

double blend_average(double a, double b) {
    return 0.5 * (a + b);
}

RfPropagationConfig blend_propagation_configs(const RfPropagationConfig& a,
                                              const RfPropagationConfig& b) {
    RfPropagationConfig out = a;

    out.enable_fspl = a.enable_fspl || b.enable_fspl;
    out.enable_log_distance = a.enable_log_distance || b.enable_log_distance;
    out.enable_two_ray = a.enable_two_ray || b.enable_two_ray;
    out.enable_los_horizon = a.enable_los_horizon || b.enable_los_horizon;
    out.enable_comms_mode_losses = a.enable_comms_mode_losses || b.enable_comms_mode_losses;
    out.enable_fixed_path_loss_override = a.enable_fixed_path_loss_override || b.enable_fixed_path_loss_override;
    out.enable_scan_beam = a.enable_scan_beam || b.enable_scan_beam;
    out.enable_scan_timing = a.enable_scan_timing || b.enable_scan_timing;
    out.enable_range_limit = a.enable_range_limit || b.enable_range_limit;
    out.enable_noise_floor = a.enable_noise_floor || b.enable_noise_floor;
    out.enable_snr_threshold = a.enable_snr_threshold || b.enable_snr_threshold;
    out.enable_sensitivity = a.enable_sensitivity || b.enable_sensitivity;
    out.enable_squelch = a.enable_squelch || b.enable_squelch;
    out.enable_interference = a.enable_interference || b.enable_interference;
    out.enable_shadowing = a.enable_shadowing || b.enable_shadowing;
    out.enable_fading = a.enable_fading || b.enable_fading;
    out.enable_doppler = a.enable_doppler || b.enable_doppler;
    out.enable_environmental_attenuation = a.enable_environmental_attenuation || b.enable_environmental_attenuation;
    out.enable_sea_attenuation = a.enable_sea_attenuation || b.enable_sea_attenuation;
    out.enable_polarization_loss = a.enable_polarization_loss || b.enable_polarization_loss;
    out.use_itu_rain_model = a.use_itu_rain_model || b.use_itu_rain_model;

    out.los_blocked_loss_db = blend_average(a.los_blocked_loss_db, b.los_blocked_loss_db);
    out.bLOS_diffraction_db_per_m = blend_average(a.bLOS_diffraction_db_per_m, b.bLOS_diffraction_db_per_m);
    out.satcom_extra_loss_db = blend_average(a.satcom_extra_loss_db, b.satcom_extra_loss_db);
    out.troposcatter_log_loss_factor_db =
        blend_average(a.troposcatter_log_loss_factor_db, b.troposcatter_log_loss_factor_db);
    out.log_distance_path_loss_exp =
        blend_average(a.log_distance_path_loss_exp, b.log_distance_path_loss_exp);
    out.log_distance_ref_distance_m =
        blend_average(a.log_distance_ref_distance_m, b.log_distance_ref_distance_m);
    out.shadowing_sigma_db = blend_average(a.shadowing_sigma_db, b.shadowing_sigma_db);
    out.fading_sigma_db = blend_average(a.fading_sigma_db, b.fading_sigma_db);
    out.polarization_mismatch_loss_db = blend_average(a.polarization_mismatch_loss_db, b.polarization_mismatch_loss_db);
    out.temperature_c = blend_average(a.temperature_c, b.temperature_c);
    out.pressure_hpa = blend_average(a.pressure_hpa, b.pressure_hpa);
    out.humidity_percent = blend_average(a.humidity_percent, b.humidity_percent);
    out.gas_attenuation_db_per_km_at_1ghz = blend_average(a.gas_attenuation_db_per_km_at_1ghz, b.gas_attenuation_db_per_km_at_1ghz);
    out.gas_attenuation_freq_exponent = blend_average(a.gas_attenuation_freq_exponent, b.gas_attenuation_freq_exponent);
    out.humidity_attenuation_factor_per_percent = blend_average(a.humidity_attenuation_factor_per_percent, b.humidity_attenuation_factor_per_percent);
    out.rain_rate_mm_per_hr = blend_average(a.rain_rate_mm_per_hr, b.rain_rate_mm_per_hr);
    out.rain_attenuation_db_per_km_per_mmhr = blend_average(a.rain_attenuation_db_per_km_per_mmhr, b.rain_attenuation_db_per_km_per_mmhr);
    out.rain_coverage = blend_average(a.rain_coverage, b.rain_coverage);
    out.rain_rate_sigma_frac = blend_average(a.rain_rate_sigma_frac, b.rain_rate_sigma_frac);
    out.wind_speed_mps = blend_average(a.wind_speed_mps, b.wind_speed_mps);
    out.wind_attenuation_db_per_km_per_mps = blend_average(a.wind_attenuation_db_per_km_per_mps, b.wind_attenuation_db_per_km_per_mps);
    out.sea_attenuation_db_per_km = blend_average(a.sea_attenuation_db_per_km, b.sea_attenuation_db_per_km);
    out.interference_power_dbm = blend_average(a.interference_power_dbm, b.interference_power_dbm);

    return sanitize_model_config(out);
}

RfPropagationConfig resolve_propagation_config(const RfConfig& tx_cfg,
                                               const RfConfig& rx_cfg,
                                               const RfPropagationConfig& fallback) {
    const bool tx_local = tx_cfg.use_local_propagation_config;
    const bool rx_local = rx_cfg.use_local_propagation_config;
    if (!tx_local && !rx_local) return fallback;

    if (tx_local && rx_local) {
        return blend_propagation_configs(sanitize_model_config(tx_cfg.propagation),
                                         sanitize_model_config(rx_cfg.propagation));
    }

    return tx_local ? sanitize_model_config(tx_cfg.propagation)
                    : sanitize_model_config(rx_cfg.propagation);
}

double sample_normal_db(double sigma_db) {
    if (sigma_db <= 0.0) return 0.0;
    std::normal_distribution<double> dist(0.0, sigma_db);
    return dist(rng());
}

double normalize_deg(double deg) {
    double d = std::fmod(deg, 360.0);
    if (d < 0.0) d += 360.0;
    return d;
}

double angle_diff_deg(double a, double b) {
    double diff = std::fabs(normalize_deg(a) - normalize_deg(b));
    return diff > 180.0 ? 360.0 - diff : diff;
}

double distance_2d(const RfPosition& a, const RfPosition& b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

double azimuth_deg(const RfPosition& from, const RfPosition& to) {
    return normalize_deg(std::atan2(to.y - from.y, to.x - from.x) * 180.0 / kPi);
}

double free_space_path_loss(double distance_m, double freq_hz) {
    return 20.0 * std::log10(std::max(distance_m, 1.0)) +
           20.0 * std::log10(std::max(freq_hz, 1.0)) - 147.55;
}

double rx_bandwidth_hz(const RfConfig& cfg) {
    double bw = cfg.rx_bandwidth_hz > 0.0 ? cfg.rx_bandwidth_hz : cfg.bandwidth_hz;
    return bw > 0.0 ? bw : 1.0;
}

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

Vec2 velocity_vector(const RfConfig& cfg) {
    double heading_rad = normalize_deg(cfg.heading_deg) * (kPi / 180.0);
    return {cfg.velocity_mps * std::cos(heading_rad),
            cfg.velocity_mps * std::sin(heading_rad)};
}

double doppler_shift_hz(const RfPosition& tx_pos,
                        const RfPosition& rx_pos,
                        const RfConfig& tx_cfg,
                        const RfConfig& rx_cfg) {
    double dist = distance_2d(tx_pos, rx_pos);
    if (dist <= 0.0) return 0.0;

    Vec2 tx_v = velocity_vector(tx_cfg);
    Vec2 rx_v = velocity_vector(rx_cfg);
    Vec2 rel_v{tx_v.x - rx_v.x, tx_v.y - rx_v.y};

    double ux = (rx_pos.x - tx_pos.x) / dist;
    double uy = (rx_pos.y - tx_pos.y) / dist;
    double v_rel = rel_v.x * ux + rel_v.y * uy;
    constexpr double kSpeedOfLight = 299792458.0;
    return (v_rel / kSpeedOfLight) * tx_cfg.frequency_hz;
}

double default_required_snr_db(RfModulationScheme modulation_scheme,
                               double bandwidth_hz) {
    switch (modulation_scheme) {
    case RfModulationScheme::AM:
        return 14.0;
    case RfModulationScheme::FM:
        return bandwidth_hz > 25e3 ? 13.5 : 11.5;
    case RfModulationScheme::BPSK:
        return 9.6;
    case RfModulationScheme::QPSK:
        return 10.5;
    case RfModulationScheme::PSK8:
        return 14.0;
    case RfModulationScheme::QAM16:
        return 16.0;
    case RfModulationScheme::QAM64:
        return 22.0;
    case RfModulationScheme::FSK2:
        return 11.0;
    case RfModulationScheme::FSK4:
        return 13.0;
    case RfModulationScheme::GMSK:
        return 9.0;
    case RfModulationScheme::OFDM_BPSK:
        return 10.5;
    case RfModulationScheme::OFDM_QPSK:
        return 12.0;
    case RfModulationScheme::OFDM_QAM16:
        return 18.0;
    case RfModulationScheme::OFDM_QAM64:
        return 24.0;
    }
    return 10.0;
}

double default_required_snr_db(RfModulationClass /*modulation_class*/,
                               RfModulationScheme modulation_scheme,
                               double bandwidth_hz) {
    return default_required_snr_db(modulation_scheme, bandwidth_hz);
}

uint64_t now_period_ms() {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch()).count());
}

double sector_span_deg(double min_deg, double max_deg) {
    double min_n = normalize_deg(min_deg);
    double max_n = normalize_deg(max_deg);
    if (min_n == max_n) return 360.0;
    if (min_n < max_n) return max_n - min_n;
    return (360.0 - min_n) + max_n;
}

bool angle_in_sector(double az, double min_deg, double max_deg) {
    double az_n = normalize_deg(az);
    double min_n = normalize_deg(min_deg);
    double max_n = normalize_deg(max_deg);
    if (min_n == max_n) return true;
    if (min_n < max_n) return az_n >= min_n && az_n <= max_n;
    return az_n >= min_n || az_n <= max_n;
}

double sweep_angle(double min_deg, double max_deg, uint32_t period_ms) {
    if (period_ms == 0) return normalize_deg(min_deg);
    double span = sector_span_deg(min_deg, max_deg);
    double phase = std::fmod(static_cast<double>(now_period_ms()),
                             static_cast<double>(period_ms)) /
                   static_cast<double>(period_ms);
    return normalize_deg(min_deg + span * phase);
}

bool beam_allows(const RfPosition& from,
                 const RfPosition& to,
                 const RfConfig& cfg,
                 const RfPropagationConfig& model_cfg) {
    if (!model_cfg.enable_scan_beam) return true;
    double bw = cfg.antenna.beamwidth_deg;
    if (bw <= 0.0 || bw >= 360.0) return true;
    double target_az = azimuth_deg(from, to);
    double boresight = cfg.heading_deg;

    if (cfg.antenna.scan_type == RfScanType::SECTOR_SCAN) {
        if (!angle_in_sector(target_az, cfg.antenna.azimuth_min_deg, cfg.antenna.azimuth_max_deg)) {
            return false;
        }
        if (model_cfg.enable_scan_timing && cfg.antenna.scan_period_ms > 0) {
            boresight = sweep_angle(cfg.antenna.azimuth_min_deg,
                                    cfg.antenna.azimuth_max_deg,
                                    cfg.antenna.scan_period_ms);
        } else {
            boresight = normalize_deg((cfg.antenna.azimuth_min_deg + cfg.antenna.azimuth_max_deg) * 0.5);
        }
    } else if (cfg.antenna.scan_type == RfScanType::CONICAL_SCAN) {
        if (model_cfg.enable_scan_timing && cfg.antenna.scan_period_ms > 0) {
            boresight = sweep_angle(0.0, 360.0, cfg.antenna.scan_period_ms);
        } else {
            return true;
        }
    }

    return angle_diff_deg(target_az, boresight) <= (bw * 0.5);
}

struct IturCoeffs {
    double a;
    double b;
    double c;
};

double itur_log10_k(const IturCoeffs* coeffs, size_t n, double m_k, double c_k, double log10f) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double num = (log10f - coeffs[i].b) / coeffs[i].c;
        sum += coeffs[i].a * std::exp(-(num * num));
    }
    return sum + m_k * log10f + c_k;
}

double itur_alpha(const IturCoeffs* coeffs, size_t n, double m_a, double c_a, double log10f) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double num = (log10f - coeffs[i].b) / coeffs[i].c;
        sum += coeffs[i].a * std::exp(-(num * num));
    }
    return sum + m_a * log10f + c_a;
}

void itur_rain_coeffs(double freq_ghz,
                      RfPolarization pol,
                      double elevation_rad,
                      double& k_out,
                      double& alpha_out) {
    static const IturCoeffs kH[] = {
        {-5.33980, -0.10008, 1.13098},
        {-0.35351,  1.26970, 0.45400},
        {-0.23789,  0.86036, 0.15354},
        {-0.94158,  0.64552, 0.16817}
    };
    static const IturCoeffs kV[] = {
        {-3.80595,  0.56934, 0.81061},
        {-3.44965, -0.22911, 0.51059},
        {-0.39902,  0.73042, 0.11899},
        { 0.50167,  1.07319, 0.27195}
    };
    static const IturCoeffs aH[] = {
        {-0.14318,  1.82442, -0.55187},
        { 0.29591,  0.77564,  0.19822},
        { 0.32177,  0.63773,  0.13164},
        {-5.37610, -0.96230,  1.47828},
        {16.1721,  -3.29980,  3.43990}
    };
    static const IturCoeffs aV[] = {
        {-0.07771,  2.33840, -0.76284},
        { 0.56727,  0.95545,  0.54039},
        {-0.20238,  1.14520,  0.26809},
        {-48.2991,  0.791669, 0.116226},
        { 48.5833,  0.791459, 0.116479}
    };

    double f = std::clamp(freq_ghz, 1.0, 1000.0);
    double log10f = std::log10(f);

    double log10_kH = itur_log10_k(kH, 4, -0.18961, 0.71147, log10f);
    double log10_kV = itur_log10_k(kV, 4, -0.16398, 0.63297, log10f);
    double kH_val = std::pow(10.0, log10_kH);
    double kV_val = std::pow(10.0, log10_kV);

    double alphaH = itur_alpha(aH, 5, 0.67849, -1.95537, log10f);
    double alphaV = itur_alpha(aV, 5, -0.053739, 0.83433, log10f);

    double tau_deg = 45.0;
    switch (pol) {
    case RfPolarization::HORIZONTAL:
        tau_deg = 0.0;
        break;
    case RfPolarization::VERTICAL:
        tau_deg = 90.0;
        break;
    case RfPolarization::CIRCULAR_LEFT:
    case RfPolarization::CIRCULAR_RIGHT:
        tau_deg = 45.0;
        break;
    }

    double tau = tau_deg * (kPi / 180.0);
    double cos2tau = std::cos(2.0 * tau);
    double cos2theta = std::cos(elevation_rad);
    cos2theta *= cos2theta;

    double k = (kH_val + kV_val + (kH_val - kV_val) * cos2theta * cos2tau) * 0.5;
    double alpha = (kH_val * alphaH + kV_val * alphaV +
                    (kH_val * alphaH - kV_val * alphaV) * cos2theta * cos2tau) /
                   (2.0 * k);

    k_out = k;
    alpha_out = alpha;
}

struct FrequencyEval {
    bool match = false;
    double matched_rx_frequency_hz = 0.0;
    double detune_hz = 0.0;
    double detune_noise_db = 0.0;
};

std::vector<double> active_receive_frequencies(const RfConfig& cfg) {
    if (!cfg.receive_frequencies_hz.empty()) {
        return cfg.receive_frequencies_hz;
    }
    return {cfg.frequency_hz};
}

FrequencyEval evaluate_frequency_capture(const RfConfig& tx_cfg,
                                         const RfConfig& rx_cfg,
                                         double tx_freq_hz) {
    FrequencyEval fe;
    if (tx_freq_hz < tx_cfg.min_freq_hz || tx_freq_hz > tx_cfg.max_freq_hz) return fe;

    double tx_bw = tx_cfg.bandwidth_hz > 0.0 ? tx_cfg.bandwidth_hz : 1.0;
    double rx_bw = rx_bandwidth_hz(rx_cfg);
    FrequencyEval best;
    best.detune_hz = std::numeric_limits<double>::infinity();

    for (double rx_freq_hz : active_receive_frequencies(rx_cfg)) {
        if (rx_freq_hz < rx_cfg.min_freq_hz || rx_freq_hz > rx_cfg.max_freq_hz) {
            continue;
        }

        double detune_hz = std::fabs(tx_freq_hz - rx_freq_hz);
        double clean_hz = std::max(0.5 * (tx_bw + rx_bw), rx_cfg.receiver.frequency_tolerance_hz);
        double cut_hz = std::max(clean_hz, rx_cfg.receiver.frequency_disconnect_hz);

        FrequencyEval candidate;
        candidate.detune_hz = detune_hz;
        candidate.matched_rx_frequency_hz = rx_freq_hz;

        if (detune_hz <= clean_hz) {
            candidate.match = true;
        } else if (detune_hz < cut_hz) {
            double t = (detune_hz - clean_hz) / std::max(cut_hz - clean_hz, 1.0);
            candidate.match = true;
            candidate.detune_noise_db = t * std::max(0.0, rx_cfg.receiver.detune_noise_max_db);
        }

        if (candidate.match && (!best.match || candidate.detune_hz < best.detune_hz)) {
            best = candidate;
        } else if (!best.match && candidate.detune_hz < best.detune_hz) {
            best = candidate;
        }
    }

    if (best.match) {
        return best;
    }
    if (best.detune_hz != std::numeric_limits<double>::infinity()) {
        fe.detune_hz = best.detune_hz;
        fe.matched_rx_frequency_hz = best.matched_rx_frequency_hz;
    }
    return fe;
}

struct PathLossBreakdown {
    double total_loss_db = 0.0;
    double rain_attenuation_db = 0.0;
    double wind_attenuation_db_per_km = 0.0;
    double los_horizon_distance_m = 0.0;
};

struct LinkBudget {
    double distance_m = 0.0;
    double path_loss_db = 0.0;
    double rx_power_dbm = 0.0;
    double noise_floor_dbm = 0.0;
    double snr_db = 0.0;
    double effective_frequency_hz = 0.0;
    double rain_attenuation_db = 0.0;
    double wind_attenuation_db_per_km = 0.0;
    double los_horizon_distance_m = 0.0;
    double polarization_loss_db = 0.0;
    double required_snr_threshold_db = 0.0;
    bool protocol_match = false;
    bool frequency_match = false;
    bool range_ok = false;
    bool beam_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
    bool snr_ok = false;
    bool link_ok = false;
};

PathLossBreakdown compute_path_loss(const RfPosition& a,
                                    const RfPosition& b,
                                    const RfConfig& tx_cfg,
                                    const RfConfig& rx_cfg,
                                    const RfPropagationConfig& cfg) {
    PathLossBreakdown out;
    double distance = std::max(distance_2d(a, b), 1.0);
    double freq_hz = tx_cfg.frequency_hz > 0.0 ? tx_cfg.frequency_hz : 1.0;

    double fspl = free_space_path_loss(distance, freq_hz);
    double base_loss = 0.0;

    if (cfg.enable_two_ray) {
        double ht = std::max(a.altitude, 0.0);
        double hr = std::max(b.altitude, 0.0);
        if (ht > 0.0 && hr > 0.0) {
            double two_ray_loss = 40.0 * std::log10(distance) -
                                  20.0 * std::log10(ht) -
                                  20.0 * std::log10(hr);
            base_loss = std::max(fspl, two_ray_loss);
        } else if (cfg.enable_log_distance) {
            double ref_d = std::max(cfg.log_distance_ref_distance_m, 1.0);
            double ref_loss = free_space_path_loss(ref_d, freq_hz);
            base_loss = ref_loss +
                        10.0 * cfg.log_distance_path_loss_exp * std::log10(distance / ref_d);
        } else if (cfg.enable_fspl) {
            base_loss = fspl;
        }
    } else if (cfg.enable_log_distance) {
        double ref_d = std::max(cfg.log_distance_ref_distance_m, 1.0);
        double ref_loss = free_space_path_loss(ref_d, freq_hz);
        base_loss = ref_loss +
                    10.0 * cfg.log_distance_path_loss_exp * std::log10(distance / ref_d);
    } else if (cfg.enable_fspl) {
        base_loss = fspl;
    }

    double extra_loss = 0.0;
    if (cfg.enable_los_horizon && cfg.enable_comms_mode_losses) {
        double alt_a = std::max(a.altitude, 0.0);
        double alt_b = std::max(b.altitude, 0.0);
        if (alt_a > 0.0 || alt_b > 0.0) {
            double horizon_a = std::sqrt(2.0 * kEarthRadiusM * alt_a);
            double horizon_b = std::sqrt(2.0 * kEarthRadiusM * alt_b);
            out.los_horizon_distance_m = horizon_a + horizon_b;
            if (distance > out.los_horizon_distance_m) {
                switch (tx_cfg.comms_mode) {
                case RfCommsMode::LINE_OF_SIGHT:
                    extra_loss = cfg.los_blocked_loss_db;
                    break;
                case RfCommsMode::BEYOND_LINE_OF_SIGHT:
                    extra_loss = cfg.bLOS_diffraction_db_per_m *
                                 (distance - out.los_horizon_distance_m);
                    break;
                case RfCommsMode::SATCOM:
                    extra_loss = cfg.satcom_extra_loss_db;
                    break;
                case RfCommsMode::TROPOSCATTER:
                    if (out.los_horizon_distance_m > 0.0) {
                        extra_loss = cfg.troposcatter_log_loss_factor_db *
                                     std::log10(distance / out.los_horizon_distance_m);
                    }
                    break;
                }
            }
        }
    }

    double total_loss = base_loss + extra_loss;

    if (cfg.enable_environmental_attenuation) {
        constexpr double L = 0.0065;
        constexpr double g = 9.80665;
        constexpr double R = 287.05;
        constexpr double T_MIN = 216.65;
        constexpr double P_REF = 101325.0;
        constexpr double T_REF = 288.15;
        constexpr double GAS_TEMP_EXP = 0.75;

        double distance_km = distance / 1000.0;
        double freq_ghz = std::max(freq_hz / 1e9, 0.1);

        double T0 = std::max(cfg.temperature_c + 273.15, T_MIN);
        double P0 = std::max(cfg.pressure_hpa * 100.0, 10000.0);

        double h = std::max((a.altitude + b.altitude) * 0.5, 0.0);
        double T = std::max(T0 - L * h, T_MIN);
        double P = P0 * std::pow(T / T0, g / (R * L));
        double density_ratio = (P / T) / (P_REF / T_REF);
        double temperature_response = std::pow(T_REF / T, GAS_TEMP_EXP);

        double gas_db_per_km =
            cfg.gas_attenuation_db_per_km_at_1ghz *
            std::pow(freq_ghz, cfg.gas_attenuation_freq_exponent) *
            density_ratio *
            temperature_response *
            (1.0 + cfg.humidity_percent * cfg.humidity_attenuation_factor_per_percent);

        double rain_db_per_km = 0.0;
        double rain_rate = cfg.rain_rate_mm_per_hr;
        if (cfg.rain_rate_sigma_frac > 0.0) {
            std::normal_distribution<double> dist(0.0, cfg.rain_rate_sigma_frac);
            rain_rate = std::max(0.0, rain_rate * (1.0 + dist(rng())));
        }

        double rain_coverage = std::clamp(cfg.rain_coverage, 0.0, 1.0);
        if (rain_rate > 0.0) {
            if (cfg.use_itu_rain_model) {
                double horiz = std::max(distance_2d(a, b), 1.0);
                double elevation = std::atan2(std::fabs(b.altitude - a.altitude), horiz);
                double k = 0.0;
                double alpha = 0.0;
                itur_rain_coeffs(freq_ghz, tx_cfg.antenna.polarization, elevation, k, alpha);
                rain_db_per_km = k * std::pow(rain_rate, alpha);
            } else {
                rain_db_per_km = rain_rate * cfg.rain_attenuation_db_per_km_per_mmhr;
            }
        }
        rain_db_per_km *= rain_coverage;
        out.rain_attenuation_db = rain_db_per_km * distance_km;

        out.wind_attenuation_db_per_km =
            cfg.wind_speed_mps * cfg.wind_attenuation_db_per_km_per_mps;
        double wind_loss_db = out.wind_attenuation_db_per_km * distance_km;

        double sea_db_per_km = 0.0;
        if (cfg.enable_sea_attenuation && (tx_cfg.is_naval || rx_cfg.is_naval)) {
            sea_db_per_km = cfg.sea_attenuation_db_per_km;
        }

        total_loss += (gas_db_per_km + rain_db_per_km + out.wind_attenuation_db_per_km + sea_db_per_km) * distance_km;
        (void)wind_loss_db;
    }

    if (cfg.enable_shadowing) total_loss += sample_normal_db(cfg.shadowing_sigma_db);
    if (cfg.enable_fading) total_loss += sample_normal_db(cfg.fading_sigma_db);

    out.total_loss_db = total_loss;
    return out;
}

LinkBudget evaluate_link(const RfPosition& sender_pos,
                         const RfPosition& receiver_pos,
                         const RfConfig& tx_cfg,
                         const RfConfig& rx_cfg,
                         const RfPropagationConfig& model_cfg) {
    LinkBudget lb;
    lb.distance_m = distance_2d(sender_pos, receiver_pos);
    lb.protocol_match = (tx_cfg.protocol == rx_cfg.protocol);

    double doppler_hz = 0.0;
    if (model_cfg.enable_doppler &&
        (tx_cfg.velocity_mps != 0.0 || rx_cfg.velocity_mps != 0.0)) {
        doppler_hz = doppler_shift_hz(sender_pos, receiver_pos, tx_cfg, rx_cfg);
    }

    FrequencyEval freq_eval = evaluate_frequency_capture(tx_cfg, rx_cfg, tx_cfg.frequency_hz + doppler_hz);
    lb.frequency_match = freq_eval.match;
    lb.effective_frequency_hz = tx_cfg.frequency_hz + doppler_hz;

    if (model_cfg.enable_range_limit) {
        bool tx_ok = tx_cfg.max_range_m <= 0.0 || lb.distance_m <= tx_cfg.max_range_m;
        bool rx_ok = rx_cfg.max_range_m <= 0.0 || lb.distance_m <= rx_cfg.max_range_m;
        lb.range_ok = tx_ok && rx_ok;
    } else {
        lb.range_ok = true;
    }

    lb.beam_ok = beam_allows(sender_pos, receiver_pos, tx_cfg, model_cfg) &&
                 beam_allows(receiver_pos, sender_pos, rx_cfg, model_cfg);

    PathLossBreakdown loss = compute_path_loss(sender_pos, receiver_pos, tx_cfg, rx_cfg, model_cfg);
    lb.path_loss_db = loss.total_loss_db;
    lb.rain_attenuation_db = loss.rain_attenuation_db;
    lb.wind_attenuation_db_per_km = loss.wind_attenuation_db_per_km;
    lb.los_horizon_distance_m = loss.los_horizon_distance_m;
    if (model_cfg.enable_fixed_path_loss_override && tx_cfg.fixed_path_loss_db > 0.0) {
        lb.path_loss_db = tx_cfg.fixed_path_loss_db;
    }

    double tx_power_dbm = tx_cfg.tx_power_dbm - tx_cfg.power_degradation_db;
    if (tx_cfg.tx_duty_cycle > 0.0 && tx_cfg.tx_duty_cycle < 1.0) {
        tx_power_dbm += 10.0 * std::log10(tx_cfg.tx_duty_cycle);
    }

    double pol_loss_db = 0.0;
    if (model_cfg.enable_polarization_loss &&
        tx_cfg.antenna.polarization != rx_cfg.antenna.polarization) {
        pol_loss_db = model_cfg.polarization_mismatch_loss_db;
    }
    lb.polarization_loss_db = pol_loss_db;

    lb.rx_power_dbm = tx_power_dbm + tx_cfg.antenna.gain_dbi + rx_cfg.antenna.gain_dbi -
                      lb.path_loss_db - pol_loss_db;

    if (model_cfg.enable_noise_floor) {
        double receiver_temp_k = std::clamp(model_cfg.temperature_c + 273.15, 173.15, 373.15);
        double thermal_offset_db = 10.0 * std::log10(receiver_temp_k / 290.0);
        lb.noise_floor_dbm = -174.0 + thermal_offset_db + 10.0 * std::log10(rx_bandwidth_hz(rx_cfg)) +
                             rx_cfg.receiver.noise_figure_db + freq_eval.detune_noise_db;
        if (model_cfg.enable_interference) {
            double noise_w = std::pow(10.0, (lb.noise_floor_dbm - 30.0) / 10.0);
            double interference_w = std::pow(10.0, (model_cfg.interference_power_dbm - 30.0) / 10.0);
            lb.noise_floor_dbm = 10.0 * std::log10(noise_w + interference_w) + 30.0;
        }
        lb.snr_db = lb.rx_power_dbm - lb.noise_floor_dbm;
        if (tx_cfg.spread_spectrum != RfSpreadSpectrum::NONE) {
            lb.snr_db += tx_cfg.processing_gain_db;
        }
    } else {
        lb.noise_floor_dbm = -174.0;
        lb.snr_db = std::numeric_limits<double>::infinity();
    }

    lb.required_snr_threshold_db =
        tx_cfg.required_snr_threshold_db > 0.0
            ? tx_cfg.required_snr_threshold_db
            : default_required_snr_db(tx_cfg.modulation_scheme,
                                      tx_cfg.bandwidth_hz > 0.0
                                          ? tx_cfg.bandwidth_hz
                                          : rx_bandwidth_hz(tx_cfg));

    lb.sensitivity_ok = !model_cfg.enable_sensitivity ||
                        (lb.rx_power_dbm >= rx_cfg.receiver.sensitivity_dbm);
    lb.squelch_ok = !model_cfg.enable_squelch ||
                    (lb.rx_power_dbm >=
                     (rx_cfg.receiver.sensitivity_dbm + rx_cfg.receiver.squelch_threshold_db));
    lb.snr_ok = !model_cfg.enable_snr_threshold || !model_cfg.enable_noise_floor ||
                (lb.snr_db >= lb.required_snr_threshold_db);

    lb.link_ok = lb.protocol_match && lb.frequency_match && lb.range_ok && lb.beam_ok &&
                 lb.sensitivity_ok && lb.squelch_ok && lb.snr_ok;
    return lb;
}

std::vector<std::byte> bytes_from_string(const std::string& text) {
    std::vector<std::byte> out;
    out.reserve(text.size());
    for (char ch : text) out.push_back(static_cast<std::byte>(ch));
    return out;
}

std::string string_from_bytes(const std::vector<std::byte>& data) {
    std::string text;
    text.reserve(data.size());
    for (std::byte b : data) text.push_back(static_cast<char>(b));
    return text;
}

uint8_t nmea_checksum(const std::string& body) {
    uint8_t value = 0;
    for (char ch : body) value ^= static_cast<uint8_t>(ch);
    return value;
}

std::string build_nmea_sentence(const std::string& body) {
    std::ostringstream oss;
    oss << "!" << body << "*"
        << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(nmea_checksum(body));
    return oss.str();
}

char sixbit_to_ascii(uint8_t value) {
    char out = static_cast<char>(value + 48);
    if (out > 87) out = static_cast<char>(out + 8);
    return out;
}

bool ascii_to_sixbit(char ch, uint8_t& value) {
    unsigned char uc = static_cast<unsigned char>(ch);
    if (uc < 48 || uc > 119 || (uc > 87 && uc < 96)) return false;
    value = static_cast<uint8_t>(uc - 48);
    if (value > 40) value = static_cast<uint8_t>(value - 8);
    return value <= 63;
}

void append_unsigned(std::vector<uint8_t>& bits, uint64_t value, int width) {
    for (int i = width - 1; i >= 0; --i) {
        bits.push_back(static_cast<uint8_t>((value >> i) & 1ULL));
    }
}

void append_signed(std::vector<uint8_t>& bits, int32_t value, int width) {
    uint64_t masked = value < 0 ? (1ULL << width) + static_cast<int64_t>(value)
                                : static_cast<uint64_t>(value);
    append_unsigned(bits, masked & ((1ULL << width) - 1ULL), width);
}

uint32_t read_unsigned(const std::vector<uint8_t>& bits, size_t offset, int width) {
    uint32_t value = 0;
    for (int i = 0; i < width; ++i) {
        value = static_cast<uint32_t>((value << 1U) | bits[offset + static_cast<size_t>(i)]);
    }
    return value;
}

int32_t read_signed(const std::vector<uint8_t>& bits, size_t offset, int width) {
    uint32_t value = read_unsigned(bits, offset, width);
    uint32_t sign_bit = 1U << (width - 1);
    if ((value & sign_bit) == 0U) return static_cast<int32_t>(value);
    return static_cast<int32_t>(value - (1U << width));
}

uint8_t encode_text_char(char ch) {
    unsigned char uc = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(ch)));
    if (uc >= '@' && uc <= '_') return static_cast<uint8_t>(uc - 64);
    if (uc >= ' ' && uc <= '?') return static_cast<uint8_t>(uc);
    return 32U;
}

char decode_text_char(uint8_t value) {
    if (value < 32) return static_cast<char>(value + 64);
    return static_cast<char>(value);
}

void append_sixbit_text(std::vector<uint8_t>& bits, const std::string& text, size_t chars) {
    for (size_t i = 0; i < chars; ++i) {
        uint8_t value = i < text.size() ? encode_text_char(text[i]) : 32U;
        append_unsigned(bits, value, 6);
    }
}

std::string read_sixbit_text(const std::vector<uint8_t>& bits, size_t offset, size_t chars) {
    std::string out;
    out.reserve(chars);
    for (size_t i = 0; i < chars; ++i) {
        out.push_back(decode_text_char(static_cast<uint8_t>(read_unsigned(bits, offset + i * 6, 6))));
    }
    while (!out.empty() && (out.back() == '@' || out.back() == ' ')) out.pop_back();
    return out;
}

std::string bits_to_payload(const std::vector<uint8_t>& bits, int& fill_bits) {
    fill_bits = static_cast<int>((6 - (bits.size() % 6)) % 6);
    std::vector<uint8_t> padded = bits;
    padded.insert(padded.end(), static_cast<size_t>(fill_bits), 0U);

    std::string payload;
    payload.reserve(padded.size() / 6);
    for (size_t i = 0; i < padded.size(); i += 6) {
        uint8_t value = 0;
        for (size_t j = 0; j < 6; ++j) {
            value = static_cast<uint8_t>((value << 1U) | padded[i + j]);
        }
        payload.push_back(sixbit_to_ascii(value));
    }
    return payload;
}


} // namespace

RfDevice::RfDevice() = default;
RfDevice::~RfDevice() {
    detachFromModel();
}

void RfDevice::configure(const RfConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = sanitize_rf_config(config);
}

RfConfig RfDevice::getConfiguration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void RfDevice::setPowerOn(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    powered_on_ = on;
}

bool RfDevice::isPoweredOn() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return powered_on_;
}

void RfDevice::setReceiveCallback(ReceiveCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    receive_cb_ = std::move(cb);
}

void RfDevice::attachToModel(RfPropagationModel* model, const RfPosition& pos) {
    detachFromModel();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        model_ = model;
    }
    if (model_) model_->addDevice(this, pos);
}

void RfDevice::detachFromModel() {
    RfPropagationModel* model = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        model = model_;
        model_ = nullptr;
    }
    if (model) model->removeDevice(this);
}

void RfDevice::updatePosition(const RfPosition& pos) {
    RfPropagationModel* model = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        model = model_;
    }
    if (model) model->updatePosition(this, pos);
}

bool RfDevice::transmit(const std::vector<std::byte>& data) {
    RfPropagationModel* model = nullptr;
    bool can_tx = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        model = model_;
        can_tx = powered_on_ && model_ && config_.mode != RfMode::RECEIVER_ONLY;
    }
    if (!can_tx || !model) return false;
    model->transmit(this, data);
    return true;
}

std::vector<RfScanHit> RfDevice::scan() const {
    RfPropagationModel* model = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        model = model_;
    }
    return model ? model->scan(this) : std::vector<RfScanHit>{};
}

void RfDevice::deliver(const std::vector<std::byte>& data, const RfReceiveReport& report) {
    ReceiveCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!powered_on_ || config_.mode == RfMode::TRANSMITTER_ONLY || !receive_cb_) return;
        cb = receive_cb_;
    }
    cb(data, report);
}

RfPropagationModel::RfPropagationModel(const RfPropagationConfig& cfg) : config_(cfg) {}
RfPropagationModel::~RfPropagationModel() = default;

void RfPropagationModel::addDevice(RfDevice* device, const RfPosition& pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_[device] = DeviceEntry{pos};
}

void RfPropagationModel::removeDevice(RfDevice* device) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_.erase(device);
}

void RfPropagationModel::updatePosition(RfDevice* device, const RfPosition& pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device);
    if (it != devices_.end()) it->second.pos = pos;
}

std::vector<RfScanHit> RfPropagationModel::scan(const RfDevice* scanner) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RfScanHit> hits;
    auto it = devices_.find(scanner);
    if (it == devices_.end()) return hits;
    RfConfig rx_cfg = scanner->getConfiguration();
    if (!scanner->isPoweredOn() || rx_cfg.mode == RfMode::TRANSMITTER_ONLY) return hits;

    for (const auto& kv : devices_) {
        const RfDevice* other = kv.first;
        if (other == scanner || !other->isPoweredOn()) continue;
        RfConfig tx_cfg = other->getConfiguration();
        if (tx_cfg.mode == RfMode::RECEIVER_ONLY) continue;

        RfPropagationConfig link_cfg = resolve_propagation_config(tx_cfg, rx_cfg, config_);
        LinkBudget lb = evaluate_link(kv.second.pos, it->second.pos, tx_cfg, rx_cfg, link_cfg);
        if (!lb.link_ok) continue;

        RfScanHit hit;
        hit.id = tx_cfg.id;
        hit.target_name = tx_cfg.parent_name;
        hit.distance_m = lb.distance_m;
        hit.azimuth_deg = azimuth_deg(it->second.pos, kv.second.pos);
        hit.path_loss_db = lb.path_loss_db;
        hit.rx_power_dbm = lb.rx_power_dbm;
        hit.noise_floor_dbm = lb.noise_floor_dbm;
        hit.snr_db = lb.snr_db;
        hit.frequency_hz = lb.effective_frequency_hz;
        hit.rain_attenuation_db = lb.rain_attenuation_db;
        hit.wind_attenuation_db_per_km = lb.wind_attenuation_db_per_km;
        hit.los_horizon_distance_m = lb.los_horizon_distance_m;
        hit.polarization_loss_db = lb.polarization_loss_db;
        hit.required_snr_threshold_db = lb.required_snr_threshold_db;
        hit.protocol_match = lb.protocol_match;
        hit.frequency_match = lb.frequency_match;
        hit.range_ok = lb.range_ok;
        hit.beam_ok = lb.beam_ok;
        hit.sensitivity_ok = lb.sensitivity_ok;
        hit.squelch_ok = lb.squelch_ok;
        hit.snr_ok = lb.snr_ok;
        hit.link_ok = lb.link_ok;
        hits.push_back(hit);
    }

    return hits;
}

void RfPropagationModel::transmit(RfDevice* sender, const std::vector<std::byte>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto sender_it = devices_.find(sender);
    if (sender_it == devices_.end()) return;
    RfConfig tx_cfg = sender->getConfiguration();
    if (tx_cfg.mode == RfMode::RECEIVER_ONLY) return;

    for (const auto& kv : devices_) {
        RfDevice* receiver = const_cast<RfDevice*>(kv.first);
        if (receiver == sender || !receiver->isPoweredOn()) continue;
        RfConfig rx_cfg = receiver->getConfiguration();
        if (rx_cfg.mode == RfMode::TRANSMITTER_ONLY) continue;

        RfPropagationConfig link_cfg = resolve_propagation_config(tx_cfg, rx_cfg, config_);
        LinkBudget lb = evaluate_link(sender_it->second.pos, kv.second.pos, tx_cfg, rx_cfg, link_cfg);
        if (!lb.link_ok) continue;

        RfReceiveReport report;
        report.sender_id = tx_cfg.id;
        report.distance_m = lb.distance_m;
        report.azimuth_deg = azimuth_deg(kv.second.pos, sender_it->second.pos);
        report.path_loss_db = lb.path_loss_db;
        report.rx_power_dbm = lb.rx_power_dbm;
        report.noise_floor_dbm = lb.noise_floor_dbm;
        report.snr_db = lb.snr_db;
        report.frequency_hz = lb.effective_frequency_hz;
        report.rain_attenuation_db = lb.rain_attenuation_db;
        report.wind_attenuation_db_per_km = lb.wind_attenuation_db_per_km;
        report.los_horizon_distance_m = lb.los_horizon_distance_m;
        report.polarization_loss_db = lb.polarization_loss_db;
        report.required_snr_threshold_db = lb.required_snr_threshold_db;
        report.protocol_match = lb.protocol_match;
        report.frequency_match = lb.frequency_match;
        report.range_ok = lb.range_ok;
        report.beam_ok = lb.beam_ok;
        report.sensitivity_ok = lb.sensitivity_ok;
        report.squelch_ok = lb.squelch_ok;
        report.snr_ok = lb.snr_ok;
        receiver->deliver(data, report);
    }
}

void RfPropagationModel::setConfig(const RfPropagationConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = sanitize_model_config(cfg);
}

RfPropagationConfig RfPropagationModel::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

} // namespace rf
