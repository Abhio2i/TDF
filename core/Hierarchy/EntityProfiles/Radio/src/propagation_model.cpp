#include "propagation_model_impl.h" // include dependency
// Propagation model implementation: link budget, path loss, and delivery logic.
// Key dependencies:
// - RadioConfig / ReceiverConfig / AntennaConfig in include/radio/radio_config.h
// - PropagationModelConfig toggles in include/radio/propagation_model_config.h
// - ScanHit and ReceiveReport in include/radio/radio_types.h
#include "radio_impl.h" // include dependency
#include "utils/math_utils.h" // include dependency
#include <algorithm> // include dependency
#include <chrono> // include dependency
#include <cmath> // include dependency
#include <cstdint> // include dependency
#include <limits> // include dependency
#include <random> // include dependency

namespace radio { // namespace scope

namespace { // namespace scope

constexpr double EARTH_RADIUS = 6371000.0; // meters
constexpr double kPi = 3.14159265358979323846; // assign or declare
constexpr double SPEED_OF_LIGHT = 299792458.0; // m/s

struct LinkBudget { // type definition
    double distance_m = 0.0; // assign or declare
    double path_loss_db = 0.0; // assign or declare
    double rx_power_dbm = 0.0; // assign or declare
    double noise_floor_dbm = std::numeric_limits<double>::quiet_NaN(); // assign or declare
    double snr_db = std::numeric_limits<double>::infinity(); // assign or declare
    double doppler_hz = 0.0; // assign or declare
    bool network_match = false; // assign or declare
    bool frequency_match = false; // assign or declare
    bool range_ok = false; // assign or declare
    bool beam_ok = false; // assign or declare
    bool sensitivity_ok = false; // assign or declare
    bool squelch_ok = false; // assign or declare
    bool snr_ok = false; // assign or declare
    bool link_ok = false; // assign or declare
}; // statement

double normalize_deg(double deg) { // function start
    double d = std::fmod(deg, 360.0); // assign or declare
    if (d < 0.0) d += 360.0; // condition check
    return d; // return value
} // block end

double angle_diff_deg(double a, double b) { // function start
    double diff = std::fabs(normalize_deg(a) - normalize_deg(b)); // assign or declare
    return diff > 180.0 ? 360.0 - diff : diff; // return value
} // block end

bool angle_in_sector(double az, double min_deg, double max_deg) { // function start
    double az_n = normalize_deg(az); // assign or declare
    double min_n = normalize_deg(min_deg); // assign or declare
    double max_n = normalize_deg(max_deg); // assign or declare
    if (min_n == max_n) return true; // full circle
    if (min_n < max_n) return az_n >= min_n && az_n <= max_n; // condition check
    return az_n >= min_n || az_n <= max_n; // return value
} // block end

double sector_span_deg(double min_deg, double max_deg) { // function start
    double min_n = normalize_deg(min_deg); // assign or declare
    double max_n = normalize_deg(max_deg); // assign or declare
    if (min_n == max_n) return 360.0; // condition check
    if (min_n < max_n) return max_n - min_n; // condition check
    return (360.0 - min_n) + max_n; // return value
} // block end

double azimuth_deg(const Position& from, const Position& to) { // function start
    double dx = to.x - from.x; // assign or declare
    double dy = to.y - from.y; // assign or declare
    double angle = std::atan2(dy, dx) * 180.0 / kPi; // assign or declare
    return normalize_deg(angle); // return value
} // block end

double distance_2d(const Position& a, const Position& b) { // function start
    double dx = b.x - a.x; // assign or declare
    double dy = b.y - a.y; // assign or declare
    return std::sqrt(dx * dx + dy * dy); // return value
} // block end

struct Vec2 { // type definition
    double x = 0.0; // assign or declare
    double y = 0.0; // assign or declare
}; // statement

Vec2 velocity_vector(const RadioConfig& cfg) { // function start
    double heading_rad = normalize_deg(cfg.heading_deg) * (kPi / 180.0); // assign or declare
    return {cfg.velocity_mps * std::cos(heading_rad), // return value
            cfg.velocity_mps * std::sin(heading_rad)}; // statement
} // block end

double doppler_shift_hz(const Position& tx_pos, const Position& rx_pos, // statement
                        const RadioConfig& tx_config, const RadioConfig& rx_config) { // statement
    double dist = distance_2d(tx_pos, rx_pos); // assign or declare
    if (dist <= 0.0) return 0.0; // condition check

    Vec2 tx_v = velocity_vector(tx_config); // assign or declare
    Vec2 rx_v = velocity_vector(rx_config); // assign or declare
    Vec2 rel_v{tx_v.x - rx_v.x, tx_v.y - rx_v.y}; // statement

    double ux = (rx_pos.x - tx_pos.x) / dist; // assign or declare
    double uy = (rx_pos.y - tx_pos.y) / dist; // assign or declare
    double v_rel = rel_v.x * ux + rel_v.y * uy; // assign or declare

    return (v_rel / SPEED_OF_LIGHT) * tx_config.frequency_hz; // return value
} // block end

struct IturCoeffs { // type definition
    double a; // statement
    double b; // statement
    double c; // statement
}; // statement

double itur_log10_k(const IturCoeffs* coeffs, size_t n, double m_k, double c_k, double log10f) { // function start
    double sum = 0.0; // assign or declare
    for (size_t i = 0; i < n; ++i) { // loop over range
        double num = (log10f - coeffs[i].b) / coeffs[i].c; // assign or declare
        sum += coeffs[i].a * std::exp(-(num * num)); // assign or declare
    } // block end
    return sum + m_k * log10f + c_k; // return value
} // block end

double itur_alpha(const IturCoeffs* coeffs, size_t n, double m_a, double c_a, double log10f) { // function start
    double sum = 0.0; // assign or declare
    for (size_t i = 0; i < n; ++i) { // loop over range
        double num = (log10f - coeffs[i].b) / coeffs[i].c; // assign or declare
        sum += coeffs[i].a * std::exp(-(num * num)); // assign or declare
    } // block end
    return sum + m_a * log10f + c_a; // return value
} // block end

void itur_rain_coeffs(double freq_ghz, Polarization pol, double elevation_rad, // statement
                      double& k_out, double& alpha_out) { // statement
    // ITU-R P.838-3 coefficients
    static const IturCoeffs kH[] = { // statement
        {-5.33980, -0.10008, 1.13098}, // statement
        {-0.35351,  1.26970, 0.45400}, // statement
        {-0.23789,  0.86036, 0.15354}, // statement
        {-0.94158,  0.64552, 0.16817} // statement
    }; // statement
    static const IturCoeffs kV[] = { // statement
        {-3.80595,  0.56934, 0.81061}, // statement
        {-3.44965, -0.22911, 0.51059}, // statement
        {-0.39902,  0.73042, 0.11899}, // statement
        { 0.50167,  1.07319, 0.27195} // statement
    }; // statement
    static const IturCoeffs aH[] = { // statement
        {-0.14318,  1.82442, -0.55187}, // statement
        { 0.29591,  0.77564,  0.19822}, // statement
        { 0.32177,  0.63773,  0.13164}, // statement
        {-5.37610, -0.96230,  1.47828}, // statement
        {16.1721,  -3.29980,  3.43990} // statement
    }; // statement
    static const IturCoeffs aV[] = { // statement
        {-0.07771,  2.33840, -0.76284}, // statement
        { 0.56727,  0.95545,  0.54039}, // statement
        {-0.20238,  1.14520,  0.26809}, // statement
        {-48.2991,  0.791669, 0.116226}, // statement
        { 48.5833,  0.791459, 0.116479} // statement
    }; // statement

    double f = std::clamp(freq_ghz, 1.0, 1000.0); // assign or declare
    double log10f = std::log10(f); // assign or declare

    double log10_kH = itur_log10_k(kH, 4, -0.18961, 0.71147, log10f); // assign or declare
    double log10_kV = itur_log10_k(kV, 4, -0.16398, 0.63297, log10f); // assign or declare
    double kH_val = std::pow(10.0, log10_kH); // assign or declare
    double kV_val = std::pow(10.0, log10_kV); // assign or declare

    double alphaH = itur_alpha(aH, 5, 0.67849, -1.95537, log10f); // assign or declare
    double alphaV = itur_alpha(aV, 5, -0.053739, 0.83433, log10f); // assign or declare

    double tau_deg = 45.0; // assign or declare
    switch (pol) { // switch on value
    case Polarization::HORIZONTAL: // case label
        tau_deg = 0.0; // assign or declare
        break; // break out
    case Polarization::VERTICAL: // case label
        tau_deg = 90.0; // assign or declare
        break; // break out
    case Polarization::CIRCULAR_LEFT: // case label
    case Polarization::CIRCULAR_RIGHT: // case label
        tau_deg = 45.0; // assign or declare
        break; // break out
    } // block end

    double tau = tau_deg * (kPi / 180.0); // assign or declare
    double cos2tau = std::cos(2.0 * tau); // assign or declare
    double cos2theta = std::cos(elevation_rad); // assign or declare
    cos2theta *= cos2theta; // assign or declare

    double k = (kH_val + kV_val + (kH_val - kV_val) * cos2theta * cos2tau) * 0.5; // assign or declare
    double alpha = (kH_val * alphaH + kV_val * alphaV + // statement
                    (kH_val * alphaH - kV_val * alphaV) * cos2theta * cos2tau) / // statement
                   (2.0 * k); // statement

    k_out = k; // assign or declare
    alpha_out = alpha; // assign or declare
} // block end

uint64_t now_ms() { // function start
    using clock = std::chrono::steady_clock; // type alias
    return static_cast<uint64_t>( // return value
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count()); // statement
} // block end

double sweep_angle(double min_deg, double max_deg, uint32_t period_ms) { // function start
    if (period_ms == 0) return normalize_deg(min_deg); // condition check
    double span = sector_span_deg(min_deg, max_deg); // assign or declare
    double phase = std::fmod(static_cast<double>(now_ms()), static_cast<double>(period_ms)) / // statement
                   static_cast<double>(period_ms); // statement
    return normalize_deg(min_deg + span * phase); // return value
} // block end

bool beam_allows(const Position& from, const Position& to, // statement
                 const RadioConfig& cfg, // statement
                 const PropagationModelConfig& model_cfg, // statement
                 bool use_scan_timing) { // statement
    if (!model_cfg.enable_scan_beam) return true; // condition check

    double bw = cfg.antenna.beamwidth_deg; // assign or declare
    if (bw <= 0.0 || bw >= 360.0) return true; // condition check

    double target_az = azimuth_deg(from, to); // assign or declare
    double boresight = cfg.heading_deg; // assign or declare

    if (cfg.antenna.scan_type == ScanType::SECTOR_SCAN) { // condition check
        if (!angle_in_sector(target_az, cfg.antenna.azimuth_min_deg, cfg.antenna.azimuth_max_deg)) { // condition check
            return false; // return value
        } // block end
        if (use_scan_timing && cfg.antenna.scan_time1_ms > 0) { // condition check
            boresight = sweep_angle(cfg.antenna.azimuth_min_deg, // statement
                                    cfg.antenna.azimuth_max_deg, // statement
                                    cfg.antenna.scan_time1_ms); // statement
        } else { // statement
            boresight = normalize_deg((cfg.antenna.azimuth_min_deg + cfg.antenna.azimuth_max_deg) * 0.5); // assign or declare
        } // block end
    } else if (cfg.antenna.scan_type == ScanType::CONICAL_SCAN) { // function start
        if (use_scan_timing && cfg.antenna.scan_time1_ms > 0) { // condition check
            boresight = sweep_angle(0.0, 360.0, cfg.antenna.scan_time1_ms); // assign or declare
        } else { // statement
            return true; // full coverage when timing is disabled
        } // block end
    } // block end

    return angle_diff_deg(target_az, boresight) <= (bw * 0.5); // return value
} // block end

double rx_bandwidth_hz(const RadioConfig& cfg) { // function start
    double bw = cfg.rx_bandwidth_hz > 0.0 ? cfg.rx_bandwidth_hz : cfg.bandwidth_hz; // assign or declare
    if (bw <= 0.0) return 1.0; // condition check
    return bw; // return value
} // block end

struct FrequencyEval {
    bool match = false;
    double detune_hz = 0.0;
    double detune_noise_db = 0.0;
};

FrequencyEval evaluate_frequency_capture(const RadioConfig& tx_config,
                                         const RadioConfig& rx_config,
                                         double tx_freq_hz) {
    FrequencyEval fe;

    if (tx_freq_hz < tx_config.min_freq_hz || tx_freq_hz > tx_config.max_freq_hz) {
        return fe;
    }
    if (rx_config.frequency_hz < rx_config.min_freq_hz || rx_config.frequency_hz > rx_config.max_freq_hz) {
        return fe;
    }

    double tx_bw = tx_config.bandwidth_hz > 0.0 ? tx_config.bandwidth_hz : 1.0;
    double rx_bw = rx_bandwidth_hz(rx_config);

    fe.detune_hz = std::fabs(tx_freq_hz - rx_config.frequency_hz);

    // Clean-capture window:
    // use either passband overlap scale or the receiver's explicit tolerance, whichever is larger.
    double clean_capture_hz =
        std::max(0.5 * (tx_bw + rx_bw), rx_config.receiver.frequency_tolerance_hz);

    double disconnect_hz =
        std::max(clean_capture_hz, rx_config.receiver.frequency_disconnect_hz);

    if (fe.detune_hz <= clean_capture_hz) {
        fe.match = true;
        fe.detune_noise_db = 0.0;
        return fe;
    }

    if (fe.detune_hz >= disconnect_hz) {
        fe.match = false;
        fe.detune_noise_db = rx_config.receiver.detune_noise_max_db;
        return fe;
    }

    double t = (fe.detune_hz - clean_capture_hz) /
               std::max(disconnect_hz - clean_capture_hz, 1.0);
    t = std::clamp(t, 0.0, 1.0);

    fe.match = true;
    fe.detune_noise_db = t * std::max(0.0, rx_config.receiver.detune_noise_max_db);
    return fe;
}

double default_required_snr_db(ModulationScheme scheme) { // function start
    switch (scheme) { // switch on value
    case ModulationScheme::BPSK: // case label
        return 9.6; // return value
    case ModulationScheme::QPSK: // case label
        return 10.5; // return value
    case ModulationScheme::QAM16: // case label
        return 16.0; // return value
    case ModulationScheme::QAM64: // case label
        return 22.0; // return value
    case ModulationScheme::GMSK: // case label
        return 9.0; // return value
    } // block end
    return 10.0; // return value
} // block end

std::mt19937& rng() { // function start
    static thread_local std::mt19937 gen(42); // statement
    return gen; // return value
} // block end

double sample_normal_db(double sigma_db) { // function start
    if (sigma_db <= 0.0) return 0.0; // condition check
    std::normal_distribution<double> dist(0.0, sigma_db); // statement
    return dist(rng()); // return value
} // block end

// Compute total path loss (dB). Combines:
// - Selected propagation model (FSPL/log-distance/two-ray)
// - Optional comms-mode losses (LOS/BLOS/SATCOM/TROPOSCATTER)
// - Environmental attenuation (rain, humidity, gas, wind, sea)
// - Optional shadowing/fading
double compute_path_loss(const Position& a, const Position& b, // statement
                         const RadioConfig& tx_config, // statement
                         const RadioConfig& rx_config, // statement
                         const PropagationModelConfig& config) { // statement
    (void)rx_config; // statement
    double distance = std::max(distance_2d(a, b), 1.0); // assign or declare
    double freq_hz = tx_config.frequency_hz > 0.0 ? tx_config.frequency_hz : 1.0; // assign or declare

    double fspl = utils::freeSpacePathLoss(distance, freq_hz); // assign or declare
    double base_loss = 0.0; // assign or declare

    if (config.enable_two_ray) { // condition check
        double ht = std::max(a.altitude, 0.0); // assign or declare
        double hr = std::max(b.altitude, 0.0); // assign or declare
        if (ht > 0.0 && hr > 0.0) { // condition check
            double two_ray_loss = 40.0 * std::log10(distance) - 20.0 * std::log10(ht) - // statement
                                  20.0 * std::log10(hr); // statement
            base_loss = std::max(fspl, two_ray_loss); // assign or declare
        } else if (config.enable_log_distance) { // function start
            double ref_d = std::max(config.log_distance_ref_distance_m, 1.0); // assign or declare
            double ref_loss = utils::freeSpacePathLoss(ref_d, freq_hz); // assign or declare
            base_loss = ref_loss + 10.0 * config.log_distance_path_loss_exp * // statement
                                       std::log10(distance / ref_d); // statement
        } else if (config.enable_fspl) { // function start
            base_loss = fspl; // assign or declare
        } // block end
    } else if (config.enable_log_distance) { // function start
        double ref_d = std::max(config.log_distance_ref_distance_m, 1.0); // assign or declare
        double ref_loss = utils::freeSpacePathLoss(ref_d, freq_hz); // assign or declare
        base_loss = ref_loss + 10.0 * config.log_distance_path_loss_exp * // statement
                                   std::log10(distance / ref_d); // statement
    } else if (config.enable_fspl) { // function start
        base_loss = fspl; // assign or declare
    } // block end

    double extra_loss = 0.0; // assign or declare
    if (config.enable_los_horizon && config.enable_comms_mode_losses) { // condition check
        double alt_a = std::max(a.altitude, 0.0); // assign or declare
        double alt_b = std::max(b.altitude, 0.0); // assign or declare
        if (alt_a > 0.0 || alt_b > 0.0) { // condition check
            double horizon_a = std::sqrt(2.0 * EARTH_RADIUS * alt_a); // assign or declare
            double horizon_b = std::sqrt(2.0 * EARTH_RADIUS * alt_b); // assign or declare
            double max_los_dist = horizon_a + horizon_b; // assign or declare

            if (distance > max_los_dist) { // condition check
                switch (tx_config.comms_mode) { // switch on value
                case CommsMode::LINE_OF_SIGHT: // case label
                    extra_loss = config.los_blocked_loss_db; // assign or declare
                    break; // break out
                case CommsMode::BEYOND_LINE_OF_SIGHT: // case label
                    extra_loss = config.bLOS_diffraction_db_per_m * (distance - max_los_dist); // assign or declare
                    break; // break out
                case CommsMode::SATCOM: // case label
                    extra_loss = config.satcom_extra_loss_db; // assign or declare
                    break; // break out
                case CommsMode::TROPOSCATTER: // case label
                    if (max_los_dist > 0) { // condition check
                        double ratio = distance / max_los_dist; // assign or declare
                        extra_loss = config.troposcatter_log_loss_factor_db * std::log10(ratio); // assign or declare
                    } // block end
                    break; // break out
                } // block end
            } // block end
        } // block end
    } // block end

    double total_loss = base_loss + extra_loss; // assign or declare

    if (config.enable_environmental_attenuation) { // condition check
        // Standard atmosphere approximation (ISA)
        constexpr double L = 0.0065;        // K/m (temperature lapse rate)
        constexpr double g = 9.80665;       // m/s^2
        constexpr double R = 287.05;        // J/(kg*K)
        constexpr double T_MIN = 216.65;    // K (lower stratosphere clamp)

        double distance_km = distance / 1000.0; // assign or declare
        double freq_ghz = std::max(freq_hz / 1e9, 0.1); // assign or declare

        double T0 = std::max(config.temperature_c + 273.15, T_MIN); // assign or declare
        double P0 = std::max(config.pressure_hpa * 100.0, 10000.0); // assign or declare

        double h = std::max((a.altitude + b.altitude) * 0.5, 0.0); // assign or declare
        double T = std::max(T0 - L * h, T_MIN); // assign or declare
        double P = P0 * std::pow(T / T0, g / (R * L)); // assign or declare
        double density_ratio = (P * T0) / (P0 * T); // assign or declare

        double gas_db_per_km = config.gas_attenuation_db_per_km_at_1ghz * // statement
                               std::pow(freq_ghz, config.gas_attenuation_freq_exponent) * // statement
                               density_ratio * // statement
                               (1.0 + config.humidity_percent * // statement
                                          config.humidity_attenuation_factor_per_percent); // statement

        double rain_db_per_km = 0.0; // assign or declare
        double rain_rate = config.rain_rate_mm_per_hr; // assign or declare
        if (config.rain_rate_sigma_frac > 0.0) { // condition check
            std::normal_distribution<double> dist(0.0, config.rain_rate_sigma_frac); // statement
            double jitter = 1.0 + dist(rng()); // assign or declare
            rain_rate = std::max(0.0, rain_rate * jitter); // assign or declare
        } // block end
        double rain_coverage = std::clamp(config.rain_coverage, 0.0, 1.0); // assign or declare
        if (rain_rate > 0.0) { // condition check
            if (config.use_itu_rain_model) { // condition check
                double horiz = std::max(distance_2d(a, b), 1.0); // assign or declare
                double elevation = std::atan2(std::fabs(b.altitude - a.altitude), horiz); // assign or declare
                double k = 0.0; // assign or declare
                double alpha = 0.0; // assign or declare
                itur_rain_coeffs(freq_ghz, tx_config.antenna.polarization, elevation, k, alpha); // statement
                rain_db_per_km = k * std::pow(rain_rate, alpha); // assign or declare
            } else { // statement
                rain_db_per_km = rain_rate * // statement
                                 config.rain_attenuation_db_per_km_per_mmhr; // statement
            } // block end
        } // block end
        rain_db_per_km *= rain_coverage; // assign or declare

        double wind_db_per_km = config.wind_speed_mps * // statement
                                config.wind_attenuation_db_per_km_per_mps; // statement

        double sea_db_per_km = 0.0; // assign or declare
        if (config.enable_sea_attenuation && // condition check
            (tx_config.is_naval || rx_config.is_naval)) { // function start
            sea_db_per_km = config.sea_attenuation_db_per_km; // assign or declare
        } // block end

        double env_db_per_km = gas_db_per_km + rain_db_per_km + wind_db_per_km + sea_db_per_km; // assign or declare
        total_loss += env_db_per_km * distance_km; // assign or declare
    } // block end

    if (config.enable_shadowing) { // condition check
        total_loss += sample_normal_db(config.shadowing_sigma_db); // assign or declare
    } // block end
    if (config.enable_fading) { // condition check
        total_loss += sample_normal_db(config.fading_sigma_db); // assign or declare
    } // block end

    return total_loss; // return value
} // block end

// Evaluate a full link budget between sender and receiver.
// Used by both transmit() and radiolibscan() to ensure consistent behavior.
LinkBudget evaluate_link(const Position& sender_pos, const Position& receiver_pos, // statement
                         const RadioConfig& tx_config, const RadioConfig& rx_config, // statement
                         const PropagationModelConfig& config, // statement
                         bool use_scan_timing) { // statement
    LinkBudget lb; // statement
    lb.distance_m = distance_2d(sender_pos, receiver_pos); // assign or declare

    lb.network_match = (tx_config.network_id == rx_config.network_id) &&
                       (tx_config.channel == rx_config.channel);

    double doppler_hz = 0.0;
    if (config.enable_doppler && (tx_config.velocity_mps != 0.0 || rx_config.velocity_mps != 0.0)) {
        doppler_hz = doppler_shift_hz(sender_pos, receiver_pos, tx_config, rx_config);
    }
    lb.doppler_hz = doppler_hz;

    FrequencyEval freq_eval =
        evaluate_frequency_capture(tx_config, rx_config, tx_config.frequency_hz + doppler_hz);

    lb.frequency_match = freq_eval.match;

    if (config.enable_range_limit) { // condition check
        bool tx_ok = (tx_config.max_range_m <= 0.0) || (lb.distance_m <= tx_config.max_range_m); // assign or declare
        bool rx_ok = (rx_config.max_range_m <= 0.0) || (lb.distance_m <= rx_config.max_range_m); // assign or declare
        lb.range_ok = tx_ok && rx_ok; // assign or declare
    } else { // statement
        lb.range_ok = true; // assign or declare
    } // block end

    lb.beam_ok = beam_allows(sender_pos, receiver_pos, tx_config, config, use_scan_timing) && // statement
                 beam_allows(receiver_pos, sender_pos, rx_config, config, use_scan_timing); // statement

    lb.path_loss_db = compute_path_loss(sender_pos, receiver_pos, tx_config, rx_config, config); // assign or declare
    if (config.enable_fixed_path_loss_override && tx_config.fixed_path_loss_db > 0.0) { // condition check
        lb.path_loss_db = tx_config.fixed_path_loss_db; // assign or declare
    } // block end

    double pol_loss_db = 0.0; // assign or declare
    if (config.enable_polarization_loss && // condition check
        tx_config.antenna.polarization != rx_config.antenna.polarization) { // statement
        pol_loss_db = config.polarization_mismatch_loss_db; // assign or declare
    } // block end

    double tx_power_dbm = tx_config.tx_power_dbm - tx_config.power_degradation_db; // assign or declare
    if (tx_config.tx_duty_cycle > 0.0 && tx_config.tx_duty_cycle < 1.0) { // condition check
        tx_power_dbm += 10.0 * std::log10(tx_config.tx_duty_cycle); // assign or declare
    } // block end

    lb.rx_power_dbm = tx_power_dbm + tx_config.antenna.gain_dbi + rx_config.antenna.gain_dbi - // statement
                      lb.path_loss_db - pol_loss_db; // statement

    double effective_sensitivity = rx_config.receiver.sensitivity_dbm; // assign or declare
    if (tx_config.lpi_enabled || tx_config.lpd_enabled) { // condition check
        // Placeholder: adjust effective sensitivity for LPI/LPD if desired.
    } // block end
    if (tx_config.aj_enabled) { // condition check
        // Placeholder: anti-jamming effects.
    } // block end

    if (config.enable_sensitivity) { // condition check
        lb.sensitivity_ok = lb.rx_power_dbm >= effective_sensitivity; // assign or declare
    } else { // statement
        lb.sensitivity_ok = true; // assign or declare
    } // block end

    if (config.enable_squelch) { // condition check
        double squelch_level = rx_config.receiver.sensitivity_dbm + rx_config.receiver.squelch_threshold_db; // assign or declare
        lb.squelch_ok = lb.rx_power_dbm >= squelch_level; // assign or declare
    } else { // statement
        lb.squelch_ok = true; // assign or declare
    } // block end

    if (config.enable_noise_floor) {
        double bw = rx_bandwidth_hz(rx_config);
        lb.noise_floor_dbm =
            -174.0 + 10.0 * std::log10(bw) + rx_config.receiver.noise_figure_db;

        // Soft detuning increases effective noise as frequency offset grows.
        lb.noise_floor_dbm += freq_eval.detune_noise_db;

        if (config.enable_interference) {
            double noise_w = utils::dbmToWatt(lb.noise_floor_dbm);
            double interference_w = utils::dbmToWatt(config.interference_power_dbm);
            lb.noise_floor_dbm = utils::wattToDbm(noise_w + interference_w);
        }

        lb.snr_db = lb.rx_power_dbm - lb.noise_floor_dbm;

        if (tx_config.spread_spectrum != SpreadSpectrum::NONE) {
            lb.snr_db += tx_config.processing_gain_db;
        }
    }


    if (config.enable_snr_threshold && config.enable_noise_floor) { // condition check
        double required_snr = tx_config.required_snr_override // statement
                                  ? tx_config.required_snr_db // statement
                                  : default_required_snr_db(tx_config.modulation_scheme); // statement
        lb.snr_ok = lb.snr_db >= required_snr; // assign or declare
    } else { // statement
        lb.snr_ok = true; // assign or declare
    } // block end

    lb.link_ok = lb.network_match && lb.frequency_match && lb.range_ok && lb.beam_ok && // statement
                 lb.sensitivity_ok && lb.squelch_ok && lb.snr_ok; // statement

    return lb; // return value
} // block end

} // namespace

void PropagationModelImpl::addRadio(Radiolib* radio, const Position& pos) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    radios_[radio] = RadioEntry{pos}; // assign or declare
} // block end

void PropagationModelImpl::removeRadio(Radiolib* radio) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    radios_.erase(radio); // statement
} // block end

void PropagationModelImpl::updateRadioPosition(Radiolib* radio, const Position& new_pos) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    auto it = radios_.find(radio); // assign or declare
    if (it != radios_.end()) { // condition check
        it->second.pos = new_pos; // assign or declare
    } // block end
} // block end

// by codex: configure model behavior/features.
void PropagationModelImpl::setConfig(const PropagationModelConfig& config) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    config_ = config; // assign or declare
} // block end

// by codex: read current model config.
PropagationModelConfig PropagationModelImpl::getConfig() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return config_; // return value
} // block end

// Transmit path: evaluate link budget from sender to every other radio and
// deliver payloads to those that pass all checks.
void PropagationModelImpl::transmit(Radiolib* sender, const std::vector<std::byte>& data,
                                    const RadioConfig& tx_config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto sender_it = radios_.find(sender);
    if (sender_it == radios_.end()) return;
    if (tx_config.mode == RadioMode::RECEIVER_ONLY) return;

    const Position& sender_pos = sender_it->second.pos;

    for (auto& [receiver, entry] : radios_) {
        if (receiver == sender) continue;
        if (!receiver->isPoweredOn()) continue;

        RadioConfig rx_config = receiver->getConfiguration();
        if (rx_config.mode == RadioMode::TRANSMITTER_ONLY) continue;

        LinkBudget lb = evaluate_link(sender_pos, entry.pos, tx_config, rx_config, config_, false);
        if (!lb.link_ok) {
            continue;
        }

        RadioImpl* rx_impl = dynamic_cast<RadioImpl*>(receiver);
        if (rx_impl) {
            ReceiveReport report;
            report.sender = sender;
            report.sender_id = tx_config.id;
            report.sender_platform_name = tx_config.parent_platform_name;
            report.distance_m = lb.distance_m;
            report.azimuth_deg = azimuth_deg(entry.pos, sender_pos);
            report.path_loss_db = lb.path_loss_db;
            report.rx_power_dbm = lb.rx_power_dbm;
            report.noise_floor_dbm = lb.noise_floor_dbm;
            report.snr_db = lb.snr_db;
            report.frequency_hz = tx_config.frequency_hz + lb.doppler_hz;
            rx_impl->receive(data, report);
        }
    }
}


// Scan path: evaluate link budget from scanner to all other radios and emit ScanHit records.
std::vector<ScanHit> PropagationModelImpl::radiolibscan(Radiolib* scanner) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ScanHit> results;
    auto scanner_it = radios_.find(scanner);
    if (scanner_it == radios_.end()) return results;
    if (!scanner->isPoweredOn()) return results;

    const Position& scanner_pos = scanner_it->second.pos;
    RadioConfig scanner_cfg = scanner->getConfiguration();

    // TRANSMITTER_ONLY:
    // return a single synthetic footprint record instead of receive-style hits.
    if (scanner_cfg.mode == RadioMode::TRANSMITTER_ONLY) {
        ScanHit hit;
        hit.radio = nullptr;
        hit.id = scanner_cfg.id;
        hit.target_platform_name = scanner_cfg.parent_platform_name;
        hit.distance_m = scanner_cfg.max_range_m;
        hit.radius_m = scanner_cfg.max_range_m;
        hit.azimuth_deg = normalize_deg(scanner_cfg.heading_deg);
        hit.scanner_max_range_m = scanner_cfg.max_range_m;
        hit.scanner_frequency_hz = scanner_cfg.frequency_hz;

        hit.network_match = true;
        hit.frequency_match = true;
        hit.range_ok = true;
        hit.beam_ok = true;
        hit.sensitivity_ok = true;
        hit.squelch_ok = true;
        hit.snr_ok = true;

        hit.is_footprint = true;
        hit.heading_deg = normalize_deg(scanner_cfg.heading_deg);
        hit.beamwidth_deg = scanner_cfg.antenna.beamwidth_deg;

        results.push_back(hit);
        return results;
    }

    // RECEIVER_ONLY / TRANSCEIVER:
    // receiver-oriented scan = "which radios can this scanner receive from?"
    for (auto& [candidate_tx, entry] : radios_) {
        if (candidate_tx == scanner) continue;
        if (!candidate_tx->isPoweredOn()) continue;

        RadioConfig candidate_tx_cfg = candidate_tx->getConfiguration();
        if (candidate_tx_cfg.mode == RadioMode::RECEIVER_ONLY) continue;

        LinkBudget lb = evaluate_link(entry.pos, scanner_pos,
                                      candidate_tx_cfg, scanner_cfg,
                                      config_, config_.enable_scan_timing);

        if (!lb.frequency_match) continue;
        if (config_.enable_network_gate_in_scan && !lb.network_match) continue;
        if (!lb.range_ok || !lb.beam_ok || !lb.sensitivity_ok || !lb.squelch_ok) continue;
        if (config_.enable_snr_threshold && config_.enable_noise_floor && !lb.snr_ok) continue;

        ScanHit hit;
        hit.radio = candidate_tx;
        hit.id = candidate_tx_cfg.id;
        hit.target_platform_name = candidate_tx_cfg.parent_platform_name;
        hit.distance_m = lb.distance_m;
        hit.radius_m = lb.distance_m;
        hit.azimuth_deg = azimuth_deg(scanner_pos, entry.pos);

        // For receiver-oriented scan, these refer to the candidate transmitter.
        hit.scanner_max_range_m = candidate_tx_cfg.max_range_m;
        hit.scanner_frequency_hz = candidate_tx_cfg.frequency_hz;

        hit.path_loss_db = lb.path_loss_db;
        hit.rx_power_dbm = lb.rx_power_dbm;
        hit.noise_floor_dbm = lb.noise_floor_dbm;
        hit.snr_db = lb.snr_db;

        hit.network_match = lb.network_match;
        hit.frequency_match = lb.frequency_match;
        hit.range_ok = lb.range_ok;
        hit.beam_ok = lb.beam_ok;
        hit.sensitivity_ok = lb.sensitivity_ok;
        hit.squelch_ok = lb.squelch_ok;
        hit.snr_ok = lb.snr_ok;

        hit.is_footprint = false;
        hit.heading_deg = candidate_tx_cfg.heading_deg;
        hit.beamwidth_deg = candidate_tx_cfg.antenna.beamwidth_deg;

        results.push_back(hit);
    }

    return results;
}

bool PropagationModelImpl::canCommunicate(const Position& sender_pos, const Position& receiver_pos, // statement
                                          const RadioConfig& tx_config, // statement
                                          const RadioConfig& rx_config) const { // statement
    LinkBudget lb = evaluate_link(sender_pos, receiver_pos, tx_config, rx_config, config_, false); // assign or declare
    return lb.link_ok; // return value
} // block end

double PropagationModelImpl::computePathLoss(const Position& a, const Position& b, // statement
                                             const RadioConfig& tx_config, // statement
                                             const RadioConfig& rx_config) const { // statement
    return compute_path_loss(a, b, tx_config, rx_config, config_); // return value
} // block end

// Factory
std::unique_ptr<PropagationModel> createPropagationModel() { // function start
    return std::make_unique<PropagationModelImpl>(); // return value
} // block end

// by codex: factory overload to create a propagation model with explicit config.
std::unique_ptr<PropagationModel> createPropagationModel(const PropagationModelConfig& config) { // function start
    auto model = std::make_unique<PropagationModelImpl>(); // assign or declare
    model->setConfig(config); // statement
    return model; // return value
} // block end

PropagationModel* createPropagationModelRaw() { // function start
    return new PropagationModelImpl(); // return value
} // block end

PropagationModel* createPropagationModelRaw(const PropagationModelConfig& config) { // function start
    PropagationModelImpl* model = new PropagationModelImpl(); // assign or declare
    model->setConfig(config); // statement
    return model; // return value
} // block end

void destroyPropagationModel(PropagationModel* model) { // function start
    delete model; // statement
} // block end

} // namespace radio
