#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/AIS/AIS.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>

namespace ais {

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

    double total_loss = cfg.enable_fspl ? free_space_path_loss(distance, freq_hz) : 0.0;

    if (cfg.enable_los_horizon) {
        double alt_a = std::max(a.altitude, 0.0);
        double alt_b = std::max(b.altitude, 0.0);
        if (alt_a > 0.0 || alt_b > 0.0) {
            double horizon_a = std::sqrt(2.0 * kEarthRadiusM * alt_a);
            double horizon_b = std::sqrt(2.0 * kEarthRadiusM * alt_b);
            out.los_horizon_distance_m = horizon_a + horizon_b;
            if (distance > out.los_horizon_distance_m) {
                total_loss += cfg.los_blocked_loss_db;
            }
        }
    }

    if (cfg.enable_environmental_attenuation) {
        constexpr double L = 0.0065;
        constexpr double g = 9.80665;
        constexpr double R = 287.05;
        constexpr double T_MIN = 216.65;

        double distance_km = distance / 1000.0;
        double freq_ghz = std::max(freq_hz / 1e9, 0.1);

        double T0 = std::max(cfg.temperature_c + 273.15, T_MIN);
        double P0 = std::max(cfg.pressure_hpa * 100.0, 10000.0);

        double h = std::max((a.altitude + b.altitude) * 0.5, 0.0);
        double T = std::max(T0 - L * h, T_MIN);
        double P = P0 * std::pow(T / T0, g / (R * L));
        double density_ratio = (P * T0) / (P0 * T);

        double gas_db_per_km =
            cfg.gas_attenuation_db_per_km_at_1ghz *
            std::pow(freq_ghz, cfg.gas_attenuation_freq_exponent) *
            density_ratio *
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
        lb.noise_floor_dbm = -174.0 + 10.0 * std::log10(rx_bandwidth_hz(rx_cfg)) +
                             rx_cfg.receiver.noise_figure_db + freq_eval.detune_noise_db;
        if (model_cfg.enable_interference) {
            double noise_w = std::pow(10.0, (lb.noise_floor_dbm - 30.0) / 10.0);
            double interference_w = std::pow(10.0, (model_cfg.interference_power_dbm - 30.0) / 10.0);
            lb.noise_floor_dbm = 10.0 * std::log10(noise_w + interference_w) + 30.0;
        }
        lb.snr_db = lb.rx_power_dbm - lb.noise_floor_dbm;
    } else {
        lb.noise_floor_dbm = -174.0;
        lb.snr_db = std::numeric_limits<double>::infinity();
    }

    lb.required_snr_threshold_db = tx_cfg.required_snr_threshold_db;

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

bool payload_to_bits(const std::string& payload, int fill_bits, std::vector<uint8_t>& bits) {
    if (fill_bits < 0 || fill_bits > 5) return false;
    bits.clear();
    bits.reserve(payload.size() * 6);
    for (char ch : payload) {
        uint8_t value = 0;
        if (!ascii_to_sixbit(ch, value)) return false;
        append_unsigned(bits, value, 6);
    }
    if (fill_bits > 0) {
        if (bits.size() < static_cast<size_t>(fill_bits)) return false;
        bits.resize(bits.size() - static_cast<size_t>(fill_bits));
    }
    return true;
}

std::vector<std::string> split(const std::string& text, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

template <typename T>
bool parse_value(const std::string& text, T& out) {
    std::stringstream ss(text);
    ss >> out;
    return !ss.fail();
}

template <>
bool parse_value<bool>(const std::string& text, bool& out) {
    if (text == "1" || text == "true" || text == "TRUE") {
        out = true;
        return true;
    }
    if (text == "0" || text == "false" || text == "FALSE") {
        out = false;
        return true;
    }
    return false;
}

std::vector<std::string> build_nmea_payload_sentences(const std::vector<uint8_t>& bits,
                                                      const std::string& talker) {
    int fill_bits = 0;
    std::string payload = bits_to_payload(bits, fill_bits);
    constexpr size_t kMaxPayloadChars = 60;
    size_t total = std::max<size_t>(1, (payload.size() + kMaxPayloadChars - 1) / kMaxPayloadChars);

    static unsigned seq_id = 0;
    seq_id = (seq_id + 1U) % 10U;
    std::string seq = total > 1 ? std::to_string(seq_id) : "";

    std::vector<std::string> sentences;
    sentences.reserve(total);
    for (size_t i = 0; i < total; ++i) {
        size_t start = i * kMaxPayloadChars;
        std::string fragment = payload.substr(start, kMaxPayloadChars);
        int frag_fill = (i + 1 == total) ? fill_bits : 0;
        std::ostringstream body;
        body << talker << "," << total << "," << (i + 1) << "," << seq << ",A,"
             << fragment << "," << frag_fill;
        sentences.push_back(build_nmea_sentence(body.str()));
    }
    return sentences;
}

std::string join_sentences(const std::vector<std::string>& sentences) {
    std::ostringstream oss;
    for (size_t i = 0; i < sentences.size(); ++i) {
        if (i > 0) oss << "\n";
        oss << sentences[i];
    }
    return oss.str();
}

int clamp_int(int value, int min_value, int max_value) {
    return std::min(std::max(value, min_value), max_value);
}

int encode_rot(double rot_deg_per_min) {
    if (!std::isfinite(rot_deg_per_min)) return -128;
    if (rot_deg_per_min > 708.0) return 127;
    if (rot_deg_per_min < -708.0) return -127;
    double magnitude = 4.733 * std::sqrt(std::fabs(rot_deg_per_min));
    int encoded = static_cast<int>(std::round(magnitude));
    return rot_deg_per_min < 0.0 ? -encoded : encoded;
}

double decode_rot(int encoded_rot) {
    if (encoded_rot == -128) return 0.0;
    double x = static_cast<double>(encoded_rot) / 4.733;
    double rot = x * x;
    return encoded_rot < 0 ? -rot : rot;
}

int32_t encode_lon(double lon_deg) {
    if (!std::isfinite(lon_deg) || lon_deg < -180.0 || lon_deg > 180.0) return 108600000;
    return static_cast<int32_t>(std::llround(lon_deg * 600000.0));
}

int32_t encode_lat(double lat_deg) {
    if (!std::isfinite(lat_deg) || lat_deg < -90.0 || lat_deg > 90.0) return 54600000;
    return static_cast<int32_t>(std::llround(lat_deg * 600000.0));
}

double decode_lon(int32_t lon_raw) {
    return lon_raw == 108600000 ? 0.0 : static_cast<double>(lon_raw) / 600000.0;
}

double decode_lat(int32_t lat_raw) {
    return lat_raw == 54600000 ? 0.0 : static_cast<double>(lat_raw) / 600000.0;
}

std::vector<uint8_t> encode_position_report(const AisConfig& cfg) {
    std::vector<uint8_t> bits;
    bits.reserve(168);
    append_unsigned(bits, 1, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.mmsi, 30);
    append_unsigned(bits, static_cast<uint8_t>(cfg.dynamic_data.nav_status), 4);
    append_signed(bits, encode_rot(cfg.dynamic_data.rot_deg_per_min), 8);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.sog_kn * 10.0)), 0, 1022), 10);
    append_unsigned(bits, cfg.dynamic_data.position_accuracy ? 1U : 0U, 1);
    append_signed(bits, encode_lon(cfg.dynamic_data.longitude_deg), 28);
    append_signed(bits, encode_lat(cfg.dynamic_data.latitude_deg), 27);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.cog_deg * 10.0)), 0, 3599), 12);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.heading_deg)), 0, 359), 9);
    append_unsigned(bits, 60, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, 0, 3);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 19);
    return bits;
}

std::vector<uint8_t> encode_standard_class_b_position_report(const AisConfig& cfg) {
    std::vector<uint8_t> bits;
    bits.reserve(168);
    append_unsigned(bits, 18, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.mmsi, 30);
    append_unsigned(bits, 0, 8);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.sog_kn * 10.0)), 0, 1022), 10);
    append_unsigned(bits, cfg.dynamic_data.position_accuracy ? 1U : 0U, 1);
    append_signed(bits, encode_lon(cfg.dynamic_data.longitude_deg), 28);
    append_signed(bits, encode_lat(cfg.dynamic_data.latitude_deg), 27);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.cog_deg * 10.0)), 0, 3599), 12);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.dynamic_data.heading_deg)), 0, 359), 9);
    append_unsigned(bits, 60, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 1, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 19);
    return bits;
}

std::vector<uint8_t> encode_static_report(const AisConfig& cfg) {
    std::vector<uint8_t> bits;
    bits.reserve(424);
    append_unsigned(bits, 5, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.mmsi, 30);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.imo, 30);
    append_sixbit_text(bits, cfg.static_data.callsign, 7);
    append_sixbit_text(bits, cfg.static_data.name, 20);
    append_unsigned(bits, cfg.static_data.ship_type, 8);
    append_unsigned(bits, cfg.static_data.dim_bow_m, 9);
    append_unsigned(bits, cfg.static_data.dim_stern_m, 9);
    append_unsigned(bits, cfg.static_data.dim_port_m, 6);
    append_unsigned(bits, cfg.static_data.dim_starboard_m, 6);
    append_unsigned(bits, 0, 4);
    append_unsigned(bits, cfg.voyage_data.eta_month, 4);
    append_unsigned(bits, cfg.voyage_data.eta_day, 5);
    append_unsigned(bits, cfg.voyage_data.eta_hour, 5);
    append_unsigned(bits, cfg.voyage_data.eta_minute, 6);
    append_unsigned(bits, clamp_int(static_cast<int>(std::round(cfg.static_data.draught_m * 10.0)), 0, 255), 8);
    append_sixbit_text(bits, cfg.static_data.destination, 20);
    append_unsigned(bits, 0, 1);
    append_unsigned(bits, 0, 1);
    return bits;
}

std::vector<uint8_t> encode_class_b_static_data_part_a(const AisConfig& cfg) {
    std::vector<uint8_t> bits;
    bits.reserve(168);
    append_unsigned(bits, 24, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.mmsi, 30);
    append_unsigned(bits, 0, 2);
    append_sixbit_text(bits, cfg.static_data.name, 20);
    append_unsigned(bits, 0, 8);
    return bits;
}

std::vector<uint8_t> encode_class_b_static_data_part_b(const AisConfig& cfg) {
    std::vector<uint8_t> bits;
    bits.reserve(168);
    append_unsigned(bits, 24, 6);
    append_unsigned(bits, 0, 2);
    append_unsigned(bits, cfg.static_data.mmsi, 30);
    append_unsigned(bits, 1, 2);
    append_unsigned(bits, cfg.static_data.ship_type, 8);
    append_sixbit_text(bits, cfg.static_data.vendor_id.empty() ? std::string("SIM") : cfg.static_data.vendor_id, 3);
    append_sixbit_text(bits, cfg.static_data.callsign, 7);
    append_unsigned(bits, cfg.static_data.dim_bow_m, 9);
    append_unsigned(bits, cfg.static_data.dim_stern_m, 9);
    append_unsigned(bits, cfg.static_data.dim_port_m, 6);
    append_unsigned(bits, cfg.static_data.dim_starboard_m, 6);
    append_unsigned(bits, 0, 6);
    return bits;
}

struct NmeaSentence {
    int fragment_count = 0;
    int fragment_number = 0;
    std::string sequence_id;
    std::string channel;
    std::string payload;
    int fill_bits = 0;
};

bool parse_nmea_sentence(const std::string& line, NmeaSentence& out) {
    if (line.size() < 9 || line.front() != '!') return false;
    size_t star = line.find('*');
    if (star == std::string::npos || star + 2 >= line.size()) return false;
    std::string body = line.substr(1, star - 1);
    unsigned checksum_value = 0;
    std::stringstream ss;
    ss << std::hex << line.substr(star + 1, 2);
    ss >> checksum_value;
    if (ss.fail() || checksum_value != nmea_checksum(body)) return false;

    std::vector<std::string> fields = split(body, ',');
    if (fields.size() != 7 || (fields[0] != "AIVDM" && fields[0] != "AIVDO")) return false;
    if (!parse_value(fields[1], out.fragment_count) ||
        !parse_value(fields[2], out.fragment_number) ||
        !parse_value(fields[6], out.fill_bits)) return false;

    out.sequence_id = fields[3];
    out.channel = fields[4];
    out.payload = fields[5];
    return out.fragment_count > 0 &&
           out.fragment_number > 0 &&
           out.fragment_number <= out.fragment_count;
}

struct FragmentAssembly {
    int fragment_count = 0;
    std::vector<std::string> payloads;
    int fill_bits = 0;
};

double default_dynamic_interval_s(const AisConfig& cfg) {
    if (cfg.dynamic_interval_s > 0.0) {
        return cfg.dynamic_interval_s;
    }
    if (cfg.ais_class == AisClass::CLASS_A) {
        if (cfg.dynamic_data.sog_kn > 23.0) return 2.0;
        if (cfg.dynamic_data.sog_kn > 14.0) return 6.0;
        return 10.0;
    }
    if (cfg.dynamic_data.sog_kn <= 2.0) return 30.0;
    return 15.0;
}

double default_static_interval_s(const AisConfig& cfg) {
    if (cfg.static_interval_s > 0.0) {
        return cfg.static_interval_s;
    }
    return cfg.ais_class == AisClass::CLASS_A ? 360.0 : 180.0;
}

std::string talker_for_config(const AisConfig& cfg) {
    return cfg.prefer_aivdo_for_ownship ? "AIVDO" : "AIVDM";
}

double channel_frequency_hz(const AisConfig& cfg, int channel_index) {
    return channel_index == 2 ? cfg.ais2_frequency_hz : cfg.ais1_frequency_hz;
}

std::string channel_field(int channel_index) {
    return channel_index == 2 ? "B" : "A";
}

std::vector<std::string> build_nmea_payload_sentences_for_channel(const std::vector<uint8_t>& bits,
                                                                  const std::string& talker,
                                                                  const std::string& channel) {
    int fill_bits = 0;
    std::string payload = bits_to_payload(bits, fill_bits);
    constexpr size_t kMaxPayloadChars = 60;
    size_t total = std::max<size_t>(1, (payload.size() + kMaxPayloadChars - 1) / kMaxPayloadChars);

    static unsigned seq_id = 0;
    seq_id = (seq_id + 1U) % 10U;
    std::string seq = total > 1 ? std::to_string(seq_id) : "";

    std::vector<std::string> sentences;
    sentences.reserve(total);
    for (size_t i = 0; i < total; ++i) {
        size_t start = i * kMaxPayloadChars;
        std::string fragment = payload.substr(start, kMaxPayloadChars);
        int frag_fill = (i + 1 == total) ? fill_bits : 0;
        std::ostringstream body;
        body << talker << "," << total << "," << (i + 1) << "," << seq << "," << channel << ","
             << fragment << "," << frag_fill;
        sentences.push_back(build_nmea_sentence(body.str()));
    }
    return sentences;
}

std::vector<uint8_t> build_dynamic_bits(const AisConfig& cfg) {
    return cfg.ais_class == AisClass::CLASS_A
               ? encode_position_report(cfg)
               : encode_standard_class_b_position_report(cfg);
}

constexpr double kAisSlotsPerMinute = 2250.0;
constexpr double kAisSlotDurationS = 60.0 / kAisSlotsPerMinute;

double align_to_slot_boundary(double sim_time_s) {
    if (!std::isfinite(sim_time_s) || sim_time_s < 0.0) return 0.0;
    return std::ceil(sim_time_s / kAisSlotDurationS) * kAisSlotDurationS;
}

double reserve_next_slot_time(double sim_time_s,
                              double interval_s,
                              uint32_t mmsi,
                              int channel_index,
                              bool is_static) {
    double aligned_now = align_to_slot_boundary(sim_time_s);
    int slots_per_interval = std::max(1, static_cast<int>(std::round(interval_s / kAisSlotDurationS)));
    int current_slot = static_cast<int>(std::floor(aligned_now / kAisSlotDurationS));
    int cycle_index = current_slot / slots_per_interval;
    int seed = static_cast<int>(mmsi % static_cast<uint32_t>(slots_per_interval));
    seed = (seed + channel_index * 37 + (is_static ? 911 : 0)) % slots_per_interval;
    int target_slot = (cycle_index + 1) * slots_per_interval + seed;
    return static_cast<double>(target_slot) * kAisSlotDurationS;
}

} // namespace

AisSensor::AisSensor() : device_(std::make_unique<RfDevice>()) {
    device_->setReceiveCallback(
        [this](const std::vector<std::byte>& data, const RfReceiveReport& report) {
            ingest(data, report);
        });
    rebuildRfDefaults();
}

AisSensor::~AisSensor() = default;

void AisSensor::attachToModel(RfPropagationModel* model, const RfPosition& pos) {
    device_->attachToModel(model, pos);
}

void AisSensor::detachFromModel() {
    device_->detachFromModel();
}

void AisSensor::updatePosition(const RfPosition& pos) {
    device_->updatePosition(pos);
}

void AisSensor::configureAis(const AisConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool reset_schedule =
        (cfg.tx_enabled != ais_config_.tx_enabled) ||
        (cfg.rx_enabled != ais_config_.rx_enabled) ||
        (cfg.ais_class != ais_config_.ais_class) ||
        (cfg.channel_mode != ais_config_.channel_mode) ||
        (cfg.ais1_frequency_hz != ais_config_.ais1_frequency_hz) ||
        (cfg.ais2_frequency_hz != ais_config_.ais2_frequency_hz) ||
        (cfg.dynamic_interval_s != ais_config_.dynamic_interval_s) ||
        (cfg.static_interval_s != ais_config_.static_interval_s) ||
        (cfg.static_data.mmsi != ais_config_.static_data.mmsi);
    ais_config_ = cfg;
    rebuildRfDefaults(reset_schedule);
}

AisConfig AisSensor::getAisConfiguration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ais_config_;
}

void AisSensor::configureRf(const RfConfig& cfg) {
    device_->configure(cfg);
}

RfConfig AisSensor::getRfConfiguration() const {
    return device_->getConfiguration();
}

void AisSensor::tick(double sim_time_s) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ais_config_.enabled && ais_config_.tx_enabled) {
        auto select_tx_channel = [this]() {
            if (ais_config_.channel_mode == AisChannelMode::AIS2_ONLY) return 2;
            if (ais_config_.channel_mode == AisChannelMode::DUAL) {
                int selected = next_tx_channel_;
                next_tx_channel_ = (next_tx_channel_ == 1) ? 2 : 1;
                return selected;
            }
            return 1;
        };

        auto transmit_bits_on_channel = [this](const std::vector<uint8_t>& bits, int channel_index) {
            RfConfig rf = device_->getConfiguration();
            rf.frequency_hz = channel_frequency_hz(ais_config_, channel_index);
            device_->configure(rf);
            return device_->transmit(bytes_from_string(
                join_sentences(build_nmea_payload_sentences_for_channel(
                    bits, talker_for_config(ais_config_), channel_field(channel_index)))));
        };

        double dynamic_interval_s = default_dynamic_interval_s(ais_config_);
        double static_interval_s = default_static_interval_s(ais_config_);

        if (!std::isfinite(next_dynamic_tx_s_) || sim_time_s < last_dynamic_tx_s_) {
            int channel_index = select_tx_channel();
            next_dynamic_tx_s_ = reserve_next_slot_time(sim_time_s,
                                                        dynamic_interval_s,
                                                        ais_config_.static_data.mmsi,
                                                        channel_index,
                                                        false);
        }

        if (ais_config_.static_data.mmsi != 0 &&
            std::isfinite(next_dynamic_tx_s_) &&
            sim_time_s + 1e-9 >= next_dynamic_tx_s_) {
            int channel_index = select_tx_channel();
            transmit_bits_on_channel(build_dynamic_bits(ais_config_), channel_index);
            last_dynamic_tx_s_ = next_dynamic_tx_s_;
            next_dynamic_tx_s_ = reserve_next_slot_time(last_dynamic_tx_s_,
                                                        dynamic_interval_s,
                                                        ais_config_.static_data.mmsi,
                                                        channel_index,
                                                        false);
        }

        if (!std::isfinite(next_static_tx_s_) || sim_time_s < last_static_tx_s_) {
            int channel_index = select_tx_channel();
            next_static_tx_s_ = reserve_next_slot_time(sim_time_s,
                                                       static_interval_s,
                                                       ais_config_.static_data.mmsi,
                                                       channel_index,
                                                       true);
        }

        if (ais_config_.static_data.mmsi != 0 &&
            std::isfinite(next_static_tx_s_) &&
            sim_time_s + 1e-9 >= next_static_tx_s_) {
            int channel_index = select_tx_channel();
            if (ais_config_.ais_class == AisClass::CLASS_A) {
                transmit_bits_on_channel(encode_static_report(ais_config_), channel_index);
            } else {
                transmit_bits_on_channel(encode_class_b_static_data_part_a(ais_config_), channel_index);
                int second_channel = select_tx_channel();
                transmit_bits_on_channel(encode_class_b_static_data_part_b(ais_config_), second_channel);
            }
            last_static_tx_s_ = next_static_tx_s_;
            next_static_tx_s_ = reserve_next_slot_time(last_static_tx_s_,
                                                       static_interval_s,
                                                       ais_config_.static_data.mmsi,
                                                       channel_index,
                                                       true);
        }
    }

    if (ais_config_.track_stale_timeout_s <= 0.0) return;
    const uint64_t stale_ms = static_cast<uint64_t>(ais_config_.track_stale_timeout_s * 1000.0);
    const uint64_t current_ms = now_ms();
    for (auto& kv : tracks_) {
        kv.second.stale = (current_ms - kv.second.last_update_ms) > stale_ms;
    }
}

bool AisSensor::transmitDynamicReport() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ais_config_.enabled || !ais_config_.tx_enabled || ais_config_.static_data.mmsi == 0) return false;
    int channel_index = ais_config_.channel_mode == AisChannelMode::AIS2_ONLY ? 2 : next_tx_channel_;
    if (ais_config_.channel_mode == AisChannelMode::DUAL) {
        next_tx_channel_ = (next_tx_channel_ == 1) ? 2 : 1;
    } else {
        channel_index = ais_config_.channel_mode == AisChannelMode::AIS2_ONLY ? 2 : 1;
    }
    RfConfig rf = device_->getConfiguration();
    rf.frequency_hz = channel_frequency_hz(ais_config_, channel_index);
    device_->configure(rf);
    bool ok = device_->transmit(bytes_from_string(
        join_sentences(build_nmea_payload_sentences_for_channel(
            build_dynamic_bits(ais_config_),
            talker_for_config(ais_config_),
            channel_field(channel_index)))));
    if (ok) {
        last_dynamic_tx_s_ = 0.0;
        next_dynamic_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    }
    return ok;
}

bool AisSensor::transmitStaticVoyageReport() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ais_config_.enabled || !ais_config_.tx_enabled || ais_config_.static_data.mmsi == 0) return false;
    auto transmit_bits = [this](const std::vector<uint8_t>& bits, int channel_index) {
        RfConfig rf = device_->getConfiguration();
        rf.frequency_hz = channel_frequency_hz(ais_config_, channel_index);
        device_->configure(rf);
        return device_->transmit(bytes_from_string(
            join_sentences(build_nmea_payload_sentences_for_channel(
                bits, talker_for_config(ais_config_), channel_field(channel_index)))));
    };

    int channel_index = ais_config_.channel_mode == AisChannelMode::AIS2_ONLY ? 2 : 1;
    if (ais_config_.channel_mode == AisChannelMode::DUAL) {
        channel_index = next_tx_channel_;
        next_tx_channel_ = (next_tx_channel_ == 1) ? 2 : 1;
    }

    if (ais_config_.ais_class == AisClass::CLASS_A) {
        bool ok = transmit_bits(encode_static_report(ais_config_), channel_index);
        if (ok) {
            last_static_tx_s_ = 0.0;
            next_static_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        }
        return ok;
    }

    bool part_a_ok = transmit_bits(encode_class_b_static_data_part_a(ais_config_), channel_index);
    int second_channel = ais_config_.channel_mode == AisChannelMode::DUAL ? next_tx_channel_ : channel_index;
    if (ais_config_.channel_mode == AisChannelMode::DUAL) {
        next_tx_channel_ = (next_tx_channel_ == 1) ? 2 : 1;
    }
    bool part_b_ok = transmit_bits(encode_class_b_static_data_part_b(ais_config_), second_channel);
    if (part_a_ok && part_b_ok) {
        last_static_tx_s_ = 0.0;
        next_static_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    }
    return part_a_ok && part_b_ok;
}

std::vector<AisTrack> AisSensor::getTracks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AisTrack> out;
    out.reserve(tracks_.size());
    for (const auto& kv : tracks_) out.push_back(kv.second);
    return out;
}

std::optional<AisTrack> AisSensor::getTrack(uint32_t mmsi) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(mmsi);
    if (it == tracks_.end()) return std::nullopt;
    return it->second;
}

void AisSensor::clearTracks() {
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_.clear();
}

void AisSensor::setTrackUpdateCallback(TrackUpdateCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    update_cb_ = std::move(cb);
}

void AisSensor::setReceiveCallback(ReceiveCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    receive_cb_ = std::move(cb);
}

RfDevice& AisSensor::rfDevice() {
    return *device_;
}

const RfDevice& AisSensor::rfDevice() const {
    return *device_;
}

void AisSensor::ingest(const std::vector<std::byte>& data, const RfReceiveReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ais_config_.enabled || !ais_config_.rx_enabled) return;

    std::string raw = string_from_bytes(data);
    std::vector<std::string> lines = split_lines(raw);
    if (lines.empty() && !raw.empty()) lines.push_back(raw);

    static std::map<std::string, FragmentAssembly> fragment_cache;

    for (const std::string& line : lines) {
        NmeaSentence sentence;
        if (!parse_nmea_sentence(line, sentence)) continue;

        std::string payload;
        int fill_bits = sentence.fill_bits;
        if (sentence.fragment_count == 1) {
            payload = sentence.payload;
        } else {
            std::ostringstream key;
            key << report.sender_id << "|" << sentence.sequence_id << "|" << sentence.fragment_count;
            FragmentAssembly& assembly = fragment_cache[key.str()];
            if (assembly.fragment_count == 0) {
                assembly.fragment_count = sentence.fragment_count;
                assembly.payloads.resize(static_cast<size_t>(sentence.fragment_count));
            }
            assembly.payloads[static_cast<size_t>(sentence.fragment_number - 1)] = sentence.payload;
            if (sentence.fragment_number == sentence.fragment_count) assembly.fill_bits = sentence.fill_bits;

            bool ready = true;
            for (const auto& part : assembly.payloads) {
                if (part.empty()) ready = false;
            }
            if (!ready) continue;

            for (const auto& part : assembly.payloads) payload += part;
            fill_bits = assembly.fill_bits;
            fragment_cache.erase(key.str());
        }

        std::vector<uint8_t> bits;
        if (!payload_to_bits(payload, fill_bits, bits) || bits.size() < 38) continue;
        uint32_t message_type = read_unsigned(bits, 0, 6);
        uint32_t mmsi = read_unsigned(bits, 8, 30);
        AisTrack& track = tracks_[mmsi];
        track.mmsi = mmsi;
        track.static_data.mmsi = mmsi;
        track.last_report = report;
        track.last_update_ms = now_ms();
        track.stale = false;

        if (message_type == 1 || message_type == 2 || message_type == 3) {
            track.last_message_kind = AisMessageKind::POSITION_CLASS_A;
            track.dynamic_data.nav_status = static_cast<AisNavStatus>(read_unsigned(bits, 38, 4));
            track.dynamic_data.rot_deg_per_min = decode_rot(read_signed(bits, 42, 8));
            uint32_t sog_raw = read_unsigned(bits, 50, 10);
            track.dynamic_data.sog_kn = sog_raw >= 1023 ? 0.0 : static_cast<double>(sog_raw) / 10.0;
            track.dynamic_data.position_accuracy = read_unsigned(bits, 60, 1) != 0U;
            track.dynamic_data.longitude_deg = decode_lon(read_signed(bits, 61, 28));
            track.dynamic_data.latitude_deg = decode_lat(read_signed(bits, 89, 27));
            uint32_t cog_raw = read_unsigned(bits, 116, 12);
            track.dynamic_data.cog_deg = cog_raw >= 3600 ? 0.0 : static_cast<double>(cog_raw) / 10.0;
            uint32_t heading_raw = read_unsigned(bits, 128, 9);
            track.dynamic_data.heading_deg = heading_raw >= 360 ? 0.0 : static_cast<double>(heading_raw);
        } else if (message_type == 18 && bits.size() >= 168) {
            track.last_message_kind = AisMessageKind::POSITION_CLASS_B;
            uint32_t sog_raw = read_unsigned(bits, 46, 10);
            track.dynamic_data.sog_kn = sog_raw >= 1023 ? 0.0 : static_cast<double>(sog_raw) / 10.0;
            track.dynamic_data.position_accuracy = read_unsigned(bits, 56, 1) != 0U;
            track.dynamic_data.longitude_deg = decode_lon(read_signed(bits, 57, 28));
            track.dynamic_data.latitude_deg = decode_lat(read_signed(bits, 85, 27));
            uint32_t cog_raw = read_unsigned(bits, 112, 12);
            track.dynamic_data.cog_deg = cog_raw >= 3600 ? 0.0 : static_cast<double>(cog_raw) / 10.0;
            uint32_t heading_raw = read_unsigned(bits, 124, 9);
            track.dynamic_data.heading_deg = heading_raw >= 360 ? 0.0 : static_cast<double>(heading_raw);
            track.dynamic_data.nav_status = AisNavStatus::UNDER_WAY;
        } else if (message_type == 5 && bits.size() >= 424) {
            track.last_message_kind = AisMessageKind::STATIC_AND_VOYAGE;
            track.static_data.imo = read_unsigned(bits, 40, 30);
            track.static_data.callsign = read_sixbit_text(bits, 70, 7);
            track.static_data.name = read_sixbit_text(bits, 112, 20);
            track.static_data.ship_type = static_cast<uint16_t>(read_unsigned(bits, 232, 8));
            track.static_data.dim_bow_m = static_cast<uint16_t>(read_unsigned(bits, 240, 9));
            track.static_data.dim_stern_m = static_cast<uint16_t>(read_unsigned(bits, 249, 9));
            track.static_data.dim_port_m = static_cast<uint16_t>(read_unsigned(bits, 258, 6));
            track.static_data.dim_starboard_m = static_cast<uint16_t>(read_unsigned(bits, 264, 6));
            track.voyage_data.eta_month = static_cast<uint8_t>(read_unsigned(bits, 274, 4));
            track.voyage_data.eta_day = static_cast<uint8_t>(read_unsigned(bits, 278, 5));
            track.voyage_data.eta_hour = static_cast<uint8_t>(read_unsigned(bits, 283, 5));
            track.voyage_data.eta_minute = static_cast<uint8_t>(read_unsigned(bits, 288, 6));
            track.static_data.draught_m = static_cast<float>(read_unsigned(bits, 294, 8)) / 10.0f;
            track.static_data.destination = read_sixbit_text(bits, 302, 20);
        } else if (message_type == 24 && bits.size() >= 144) {
            track.last_message_kind = AisMessageKind::STATIC_DATA_REPORT;
            uint32_t part_number = read_unsigned(bits, 38, 2);
            if (part_number == 0) {
                track.static_data.name = read_sixbit_text(bits, 40, 20);
            } else if (part_number == 1) {
                track.static_data.ship_type = static_cast<uint16_t>(read_unsigned(bits, 40, 8));
                track.static_data.vendor_id = read_sixbit_text(bits, 48, 3);
                track.static_data.callsign = read_sixbit_text(bits, 66, 7);
                track.static_data.dim_bow_m = static_cast<uint16_t>(read_unsigned(bits, 108, 9));
                track.static_data.dim_stern_m = static_cast<uint16_t>(read_unsigned(bits, 117, 9));
                track.static_data.dim_port_m = static_cast<uint16_t>(read_unsigned(bits, 126, 6));
                track.static_data.dim_starboard_m = static_cast<uint16_t>(read_unsigned(bits, 132, 6));
            }
        }

        if (update_cb_) update_cb_(track);
        if (receive_cb_) {
            AisReceiveReport ais_report;
            ais_report.track = track;
            ais_report.raw_sentence = line;
            receive_cb_(ais_report);
        }
    }
}

void AisSensor::rebuildRfDefaults(bool reset_schedule) {
    if (reset_schedule) {
        next_tx_channel_ = ais_config_.channel_mode == AisChannelMode::AIS2_ONLY ? 2 : 1;
        next_dynamic_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        next_static_tx_s_ = std::numeric_limits<double>::quiet_NaN();
    }
    bool use_ais2 = ais_config_.channel_mode == AisChannelMode::AIS2_ONLY;
    RfConfig current = device_->getConfiguration();
    RfConfig rf = makeAisRfConfig(ais_config_, use_ais2,
                                  ais_config_.tx_enabled && ais_config_.rx_enabled
                                      ? RfMode::TRANSCEIVER
                                      : (ais_config_.tx_enabled ? RfMode::TRANSMITTER_ONLY
                                                                : RfMode::RECEIVER_ONLY));
    rf.id = current.id;
    rf.parent_name = current.parent_name;
    rf.max_range_m = current.max_range_m;
    rf.heading_deg = current.heading_deg;
    rf.velocity_mps = current.velocity_mps;
    rf.is_naval = current.is_naval;
    rf.use_local_propagation_config = current.use_local_propagation_config;
    rf.propagation = current.propagation;
    rf.comms_mode = current.comms_mode;
    rf.fixed_path_loss_db = current.fixed_path_loss_db;
    rf.spread_spectrum = current.spread_spectrum;
    rf.processing_gain_db = current.processing_gain_db;
    rf.required_snr_threshold_db = current.required_snr_threshold_db;
    rf.modulation_class = current.modulation_class;
    rf.modulation_scheme = current.modulation_scheme;
    device_->configure(rf);
}

RfConfig makeAisRfConfig(const AisConfig& cfg, bool use_ais2, RfMode mode) {
    RfConfig rf;
    rf.mode = mode;
    rf.protocol = RfProtocol::AIS;
    rf.min_freq_hz = 161.95e6;
    rf.max_freq_hz = 162.05e6;
    rf.frequency_hz = use_ais2 ? cfg.ais2_frequency_hz : cfg.ais1_frequency_hz;
    if (cfg.channel_mode == AisChannelMode::DUAL) {
        rf.receive_frequencies_hz = {cfg.ais1_frequency_hz, cfg.ais2_frequency_hz};
    } else {
        rf.receive_frequencies_hz = {rf.frequency_hz};
    }
    rf.bandwidth_hz = 25e3;
    rf.rx_bandwidth_hz = 25e3;
    rf.tx_power_dbm = cfg.ais_class == AisClass::CLASS_A ? 40.97 : 33.01;
    rf.required_snr_threshold_db = 9.0;
    rf.antenna.gain_dbi = 0.0;
    rf.antenna.beamwidth_deg = 360.0;
    rf.antenna.polarization = RfPolarization::VERTICAL;
    rf.receiver.sensitivity_dbm = -107.0;
    rf.receiver.noise_figure_db = 5.0;
    rf.receiver.squelch_threshold_db = 3.0;
    return rf;
}

} // namespace ais
