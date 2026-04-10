#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/ADSB/ADSB.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace adsb {

namespace {

constexpr uint32_t kDf17 = 17U;
constexpr uint32_t kCapability = 5U;
constexpr uint32_t kModeSPoly = 0x1FFF409U;
constexpr double kCprScale = 131072.0;
constexpr double kCprDecodeWindowS = 10.0;

uint64_t now_ms() {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch()).count());
}

bool should_transmit(double sim_time_s, double interval_s, double& last_tx_s) {
    if (interval_s <= 0.0) return false;
    if (!std::isfinite(last_tx_s) || sim_time_s < last_tx_s) {
        last_tx_s = sim_time_s;
        return true;
    }
    if (sim_time_s - last_tx_s >= interval_s) {
        last_tx_s = sim_time_s;
        return true;
    }
    return false;
}

double normalize_deg(double deg) {
    double out = std::fmod(deg, 360.0);
    if (out < 0.0) out += 360.0;
    return out;
}

double wrap_lon_deg(double lon_deg) {
    double out = std::fmod(lon_deg + 180.0, 360.0);
    if (out < 0.0) out += 360.0;
    return out - 180.0;
}

double clamp_lat_deg(double lat_deg) {
    return std::clamp(lat_deg, -89.999, 89.999);
}

double positive_mod(double value, double modulus) {
    double out = std::fmod(value, modulus);
    if (out < 0.0) out += modulus;
    return out;
}

int floor_mod(int value, int modulus) {
    int out = value % modulus;
    if (out < 0) out += modulus;
    return out;
}

void append_unsigned(std::vector<uint8_t>& bits, uint64_t value, int width) {
    for (int i = width - 1; i >= 0; --i) {
        bits.push_back(static_cast<uint8_t>((value >> i) & 1ULL));
    }
}

uint32_t read_unsigned(const std::vector<uint8_t>& bits, size_t offset, size_t width) {
    uint32_t value = 0U;
    for (size_t i = 0; i < width; ++i) {
        value = static_cast<uint32_t>((value << 1U) | (bits[offset + i] & 1U));
    }
    return value;
}

std::vector<std::byte> pack_bits(const std::vector<uint8_t>& bits) {
    std::vector<std::byte> out((bits.size() + 7U) / 8U, std::byte{0});
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) {
            size_t byte_index = i / 8U;
            int bit_index = 7 - static_cast<int>(i % 8U);
            out[byte_index] |= static_cast<std::byte>(1U << bit_index);
        }
    }
    return out;
}

std::vector<uint8_t> unpack_bits(const std::vector<std::byte>& data) {
    std::vector<uint8_t> bits;
    bits.reserve(data.size() * 8U);
    for (std::byte byte : data) {
        uint8_t value = static_cast<uint8_t>(byte);
        for (int bit = 7; bit >= 0; --bit) {
            bits.push_back(static_cast<uint8_t>((value >> bit) & 0x1U));
        }
    }
    return bits;
}

uint32_t compute_crc24_from_88_bits(const std::vector<uint8_t>& first_88_bits) {
    std::vector<uint8_t> work = first_88_bits;
    work.resize(112U, 0U);
    for (size_t i = 0; i < 88U; ++i) {
        if (!work[i]) continue;
        for (size_t j = 0; j < 25U; ++j) {
            uint8_t poly_bit = static_cast<uint8_t>((kModeSPoly >> (24U - j)) & 0x1U);
            work[i + j] ^= poly_bit;
        }
    }
    uint32_t crc = 0U;
    for (size_t i = 88U; i < 112U; ++i) {
        crc = static_cast<uint32_t>((crc << 1U) | work[i]);
    }
    return crc;
}

bool validate_crc24(const std::vector<uint8_t>& full_112_bits) {
    if (full_112_bits.size() != 112U) return false;
    std::vector<uint8_t> work = full_112_bits;
    for (size_t i = 0; i < 88U; ++i) {
        if (!work[i]) continue;
        for (size_t j = 0; j < 25U; ++j) {
            uint8_t poly_bit = static_cast<uint8_t>((kModeSPoly >> (24U - j)) & 0x1U);
            work[i + j] ^= poly_bit;
        }
    }
    for (size_t i = 88U; i < 112U; ++i) {
        if (work[i] != 0U) return false;
    }
    return true;
}

uint8_t encode_callsign_char(char ch) {
    if (ch >= 'A' && ch <= 'Z') return static_cast<uint8_t>(ch - 'A' + 1);
    if (ch >= 'a' && ch <= 'z') return static_cast<uint8_t>(ch - 'a' + 1);
    if (ch >= '0' && ch <= '9') return static_cast<uint8_t>(ch - '0' + 48);
    return 32U;
}

char decode_callsign_char(uint8_t code) {
    if (code >= 1U && code <= 26U) return static_cast<char>('A' + code - 1U);
    if (code >= 48U && code <= 57U) return static_cast<char>('0' + code - 48U);
    return ' ';
}

std::string encode_callsign_string(const std::string& flight_id) {
    std::string out(8, ' ');
    for (size_t i = 0; i < std::min<size_t>(8U, flight_id.size()); ++i) {
        out[i] = flight_id[i];
    }
    return out;
}

std::string decode_callsign_string(const std::vector<uint8_t>& bits, size_t offset) {
    std::string out;
    out.reserve(8);
    for (size_t i = 0; i < 8U; ++i) {
        out.push_back(decode_callsign_char(static_cast<uint8_t>(read_unsigned(bits, offset + i * 6U, 6U))));
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

uint16_t encode_altitude_25ft(double altitude_ft) {
    int n = static_cast<int>(std::llround((altitude_ft + 1000.0) / 25.0));
    n = std::clamp(n, 0, 0x7FF);
    return static_cast<uint16_t>(((n & 0x7F0) << 1) | 0x10 | (n & 0x0F));
}

double decode_altitude_25ft(uint16_t encoded) {
    if ((encoded & 0x10U) == 0U) return 0.0;
    int n = static_cast<int>(((encoded & 0x0FE0U) >> 1) | (encoded & 0x000FU));
    return static_cast<double>(n * 25 - 1000);
}

int cpr_nl(double lat_deg) {
    double lat = std::fabs(lat_deg);
    if (lat < 10.47047130) return 59;
    if (lat < 14.82817437) return 58;
    if (lat < 18.18626357) return 57;
    if (lat < 21.02939493) return 56;
    if (lat < 23.54504487) return 55;
    if (lat < 25.82924707) return 54;
    if (lat < 27.93898710) return 53;
    if (lat < 29.91135686) return 52;
    if (lat < 31.77209708) return 51;
    if (lat < 33.53993436) return 50;
    if (lat < 35.22899598) return 49;
    if (lat < 36.85025108) return 48;
    if (lat < 38.41241892) return 47;
    if (lat < 39.92256684) return 46;
    if (lat < 41.38651832) return 45;
    if (lat < 42.80914012) return 44;
    if (lat < 44.19454951) return 43;
    if (lat < 45.54626723) return 42;
    if (lat < 46.86733252) return 41;
    if (lat < 48.16039128) return 40;
    if (lat < 49.42776439) return 39;
    if (lat < 50.67150166) return 38;
    if (lat < 51.89342469) return 37;
    if (lat < 53.09516153) return 36;
    if (lat < 54.27817472) return 35;
    if (lat < 55.44378444) return 34;
    if (lat < 56.59318756) return 33;
    if (lat < 57.72747354) return 32;
    if (lat < 58.84763776) return 31;
    if (lat < 59.95459277) return 30;
    if (lat < 61.04917774) return 29;
    if (lat < 62.13216659) return 28;
    if (lat < 63.20427479) return 27;
    if (lat < 64.26616523) return 26;
    if (lat < 65.31845310) return 25;
    if (lat < 66.36171008) return 24;
    if (lat < 67.39646774) return 23;
    if (lat < 68.42322022) return 22;
    if (lat < 69.44242631) return 21;
    if (lat < 70.45451075) return 20;
    if (lat < 71.45986473) return 19;
    if (lat < 72.45884545) return 18;
    if (lat < 73.45177442) return 17;
    if (lat < 74.43893416) return 16;
    if (lat < 75.42056257) return 15;
    if (lat < 76.39684391) return 14;
    if (lat < 77.36789461) return 13;
    if (lat < 78.33374083) return 12;
    if (lat < 79.29428225) return 11;
    if (lat < 80.24923213) return 10;
    if (lat < 81.19801349) return 9;
    if (lat < 82.13956981) return 8;
    if (lat < 83.07199445) return 7;
    if (lat < 83.99173563) return 6;
    if (lat < 84.89166191) return 5;
    if (lat < 85.75541621) return 4;
    if (lat < 86.53536998) return 3;
    if (lat < 87.00000000) return 2;
    return 1;
}

struct EncodedCpr {
    uint32_t lat = 0U;
    uint32_t lon = 0U;
};

EncodedCpr encode_cpr(double latitude_deg, double longitude_deg, bool odd) {
    EncodedCpr out;
    const double dlat = odd ? (360.0 / 59.0) : (360.0 / 60.0);
    double lat = clamp_lat_deg(latitude_deg);
    double lon = wrap_lon_deg(longitude_deg);

    out.lat = static_cast<uint32_t>(
                  std::llround((positive_mod(lat, dlat) / dlat) * kCprScale)) & 0x1FFFFU;

    int nl = cpr_nl(lat) - (odd ? 1 : 0);
    if (nl < 1) nl = 1;
    const double dlon = 360.0 / static_cast<double>(nl);
    out.lon = static_cast<uint32_t>(
                  std::llround((positive_mod(lon, dlon) / dlon) * kCprScale)) & 0x1FFFFU;
    return out;
}

bool decode_cpr_global(const AdsbSensor::CprFrame& even_frame,
                       const AdsbSensor::CprFrame& odd_frame,
                       bool use_odd,
                       double& latitude_deg,
                       double& longitude_deg) {
    const double lat_even = static_cast<double>(even_frame.lat_cpr) / kCprScale;
    const double lat_odd = static_cast<double>(odd_frame.lat_cpr) / kCprScale;
    const int j = static_cast<int>(std::floor((59.0 * lat_even - 60.0 * lat_odd) + 0.5));

    double rlat_even = (360.0 / 60.0) * (floor_mod(j, 60) + lat_even);
    double rlat_odd = (360.0 / 59.0) * (floor_mod(j, 59) + lat_odd);
    if (rlat_even >= 270.0) rlat_even -= 360.0;
    if (rlat_odd >= 270.0) rlat_odd -= 360.0;

    const int nl_even = cpr_nl(rlat_even);
    const int nl_odd = cpr_nl(rlat_odd);
    if (nl_even != nl_odd) return false;

    if (use_odd) {
        int ni = std::max(nl_odd - 1, 1);
        int m = static_cast<int>(std::floor(
            ((static_cast<double>(even_frame.lon_cpr) * (nl_odd - 1)) -
             (static_cast<double>(odd_frame.lon_cpr) * nl_odd)) / kCprScale + 0.5));
        double lon = (360.0 / static_cast<double>(ni)) *
                     (floor_mod(m, ni) + static_cast<double>(odd_frame.lon_cpr) / kCprScale);
        if (lon >= 180.0) lon -= 360.0;
        latitude_deg = rlat_odd;
        longitude_deg = lon;
        return true;
    }

    int ni = std::max(nl_even, 1);
    int m = static_cast<int>(std::floor(
        ((static_cast<double>(even_frame.lon_cpr) * (nl_even - 1)) -
         (static_cast<double>(odd_frame.lon_cpr) * nl_even)) / kCprScale + 0.5));
    double lon = (360.0 / static_cast<double>(ni)) *
                 (floor_mod(m, ni) + static_cast<double>(even_frame.lon_cpr) / kCprScale);
    if (lon >= 180.0) lon -= 360.0;
    latitude_deg = rlat_even;
    longitude_deg = lon;
    return true;
}

bool has_valid_reference(double latitude_deg, double longitude_deg) {
    return std::isfinite(latitude_deg) &&
           std::isfinite(longitude_deg) &&
           latitude_deg >= -90.0 && latitude_deg <= 90.0 &&
           longitude_deg >= -180.0 && longitude_deg <= 180.0;
}

bool has_nontrivial_reference(double latitude_deg, double longitude_deg) {
    return has_valid_reference(latitude_deg, longitude_deg) &&
           (std::fabs(latitude_deg) > 1e-6 || std::fabs(longitude_deg) > 1e-6);
}

double lon_distance_deg(double a, double b) {
    return std::fabs(wrap_lon_deg(a - b));
}

double angular_distance_metric(double lat_a, double lon_a, double lat_b, double lon_b) {
    double dlat = lat_a - lat_b;
    double dlon = lon_distance_deg(lon_a, lon_b);
    return dlat * dlat + dlon * dlon;
}

bool decode_cpr_local(const AdsbSensor::CprFrame& frame,
                      double ref_lat_deg,
                      double ref_lon_deg,
                      double& latitude_deg,
                      double& longitude_deg) {
    if (!has_valid_reference(ref_lat_deg, ref_lon_deg)) return false;

    double dlat = frame.odd ? (360.0 / 59.0) : (360.0 / 60.0);
    double yz = static_cast<double>(frame.lat_cpr) / kCprScale;
    int j = static_cast<int>(std::floor(ref_lat_deg / dlat) +
                             std::floor(0.5 + positive_mod(ref_lat_deg, dlat) / dlat - yz));
    double lat = dlat * (static_cast<double>(j) + yz);
    if (lat >= 270.0) lat -= 360.0;
    if (lat < -90.0 || lat > 90.0) return false;

    int nl = cpr_nl(lat) - (frame.odd ? 1 : 0);
    if (nl < 1) nl = 1;
    double dlon = 360.0 / static_cast<double>(nl);
    double xz = static_cast<double>(frame.lon_cpr) / kCprScale;
    int m = static_cast<int>(std::floor(ref_lon_deg / dlon) +
                             std::floor(0.5 + positive_mod(ref_lon_deg, dlon) / dlon - xz));
    double lon = dlon * (static_cast<double>(m) + xz);
    lon = wrap_lon_deg(lon);

    latitude_deg = lat;
    longitude_deg = lon;
    return true;
}

std::vector<std::byte> build_df17_frame(uint32_t icao_address, const std::vector<uint8_t>& me_bits) {
    std::vector<uint8_t> bits;
    bits.reserve(112U);
    append_unsigned(bits, kDf17, 5);
    append_unsigned(bits, kCapability, 3);
    append_unsigned(bits, icao_address & 0xFFFFFFU, 24);
    bits.insert(bits.end(), me_bits.begin(), me_bits.end());
    uint32_t crc = compute_crc24_from_88_bits(bits);
    append_unsigned(bits, crc, 24);
    return pack_bits(bits);
}

bool parse_df17_frame(const std::vector<std::byte>& data,
                      uint32_t& icao_address,
                      std::vector<uint8_t>& me_bits) {
    if (data.size() != 14U) return false;
    std::vector<uint8_t> bits = unpack_bits(data);
    if (bits.size() != 112U) return false;
    if (read_unsigned(bits, 0, 5) != kDf17) return false;
    if (!validate_crc24(bits)) return false;
    icao_address = read_unsigned(bits, 8, 24);
    me_bits.assign(bits.begin() + 32, bits.begin() + 88);
    return true;
}

std::vector<std::byte> encode_identification_frame(const AdsbConfig& cfg) {
    std::vector<uint8_t> me;
    me.reserve(56U);
    uint8_t emitter = cfg.static_data.emitter_category;
    uint8_t tc = static_cast<uint8_t>(1U + std::min<uint8_t>(emitter / 8U, 3U));
    uint8_t category = static_cast<uint8_t>(emitter % 8U);
    append_unsigned(me, tc, 5);
    append_unsigned(me, category, 3);
    std::string callsign = encode_callsign_string(cfg.static_data.flight_id);
    for (char ch : callsign) {
        append_unsigned(me, encode_callsign_char(ch), 6);
    }
    return build_df17_frame(cfg.static_data.icao_address, me);
}

std::vector<std::byte> encode_position_frame(const AdsbConfig& cfg, bool odd) {
    std::vector<uint8_t> me;
    me.reserve(56U);
    append_unsigned(me, 11U, 5);
    append_unsigned(me, 0U, 2);
    append_unsigned(me, cfg.status_data.nic & 0x1U, 1);
    append_unsigned(me, encode_altitude_25ft(cfg.dynamic_data.altitude_baro_ft), 12);
    append_unsigned(me, 0U, 1);
    append_unsigned(me, odd ? 1U : 0U, 1);
    EncodedCpr cpr = encode_cpr(cfg.dynamic_data.latitude_deg, cfg.dynamic_data.longitude_deg, odd);
    append_unsigned(me, cpr.lat, 17);
    append_unsigned(me, cpr.lon, 17);
    return build_df17_frame(cfg.static_data.icao_address, me);
}

std::vector<std::byte> encode_velocity_frame(const AdsbConfig& cfg) {
    std::vector<uint8_t> me;
    me.reserve(56U);
    append_unsigned(me, 19U, 5);
    append_unsigned(me, 1U, 3);
    append_unsigned(me, 0U, 1);
    append_unsigned(me, 0U, 1);
    append_unsigned(me, cfg.status_data.nacp & 0x7U, 3);

    double track_rad = normalize_deg(cfg.dynamic_data.track_angle_deg) * (3.14159265358979323846 / 180.0);
    double east_kt = cfg.dynamic_data.ground_speed_kt * std::sin(track_rad);
    double north_kt = cfg.dynamic_data.ground_speed_kt * std::cos(track_rad);
    bool westbound = east_kt < 0.0;
    bool southbound = north_kt < 0.0;
    uint16_t east_mag = static_cast<uint16_t>(std::clamp<int>(static_cast<int>(std::llround(std::fabs(east_kt))) + 1, 1, 1023));
    uint16_t north_mag = static_cast<uint16_t>(std::clamp<int>(static_cast<int>(std::llround(std::fabs(north_kt))) + 1, 1, 1023));
    bool descending = cfg.dynamic_data.vertical_rate_fpm < 0.0;
    uint16_t vr_mag = static_cast<uint16_t>(std::clamp<int>(static_cast<int>(std::llround(std::fabs(cfg.dynamic_data.vertical_rate_fpm) / 64.0)) + 1, 1, 511));

    append_unsigned(me, westbound ? 1U : 0U, 1);
    append_unsigned(me, east_mag, 10);
    append_unsigned(me, southbound ? 1U : 0U, 1);
    append_unsigned(me, north_mag, 10);
    append_unsigned(me, 0U, 1);
    append_unsigned(me, descending ? 1U : 0U, 1);
    append_unsigned(me, vr_mag, 9);
    append_unsigned(me, 0U, 2);
    append_unsigned(me, 0U, 1);
    append_unsigned(me, 0U, 7);
    return build_df17_frame(cfg.static_data.icao_address, me);
}

uint16_t encode_squawk_code(uint16_t squawk) {
    uint16_t a = static_cast<uint16_t>((squawk / 1000U) % 10U);
    uint16_t b = static_cast<uint16_t>((squawk / 100U) % 10U);
    uint16_t c = static_cast<uint16_t>((squawk / 10U) % 10U);
    uint16_t d = static_cast<uint16_t>(squawk % 10U);
    a &= 0x7U;
    b &= 0x7U;
    c &= 0x7U;
    d &= 0x7U;
    return static_cast<uint16_t>((a << 9U) | (b << 6U) | (c << 3U) | d);
}

uint16_t decode_squawk_code(uint16_t encoded) {
    uint16_t a = static_cast<uint16_t>((encoded >> 9U) & 0x7U);
    uint16_t b = static_cast<uint16_t>((encoded >> 6U) & 0x7U);
    uint16_t c = static_cast<uint16_t>((encoded >> 3U) & 0x7U);
    uint16_t d = static_cast<uint16_t>(encoded & 0x7U);
    return static_cast<uint16_t>(a * 1000U + b * 100U + c * 10U + d);
}

std::vector<std::byte> encode_emergency_status_frame(const AdsbConfig& cfg) {
    std::vector<uint8_t> me;
    me.reserve(56U);
    append_unsigned(me, 28U, 5);
    append_unsigned(me, 1U, 3);
    append_unsigned(me, static_cast<uint32_t>(cfg.status_data.emergency_state) & 0x7U, 3);
    append_unsigned(me, encode_squawk_code(cfg.status_data.squawk) & 0x1FFFU, 13);
    append_unsigned(me, 0U, 32);
    return build_df17_frame(cfg.static_data.icao_address, me);
}

std::vector<std::byte> encode_operational_status_frame(const AdsbConfig& cfg) {
    std::vector<uint8_t> me;
    me.reserve(56U);
    append_unsigned(me, 31U, 5);
    append_unsigned(me, 0U, 3);
    append_unsigned(me, cfg.status_data.capability_class & 0xFFFFU, 16);
    append_unsigned(me, cfg.status_data.operational_mode & 0xFFFFU, 16);
    append_unsigned(me, cfg.status_data.adsb_version & 0x7U, 3);
    append_unsigned(me, cfg.status_data.nic_supplement_a ? 1U : 0U, 1);
    append_unsigned(me, cfg.status_data.nacp & 0xFU, 4);
    append_unsigned(me, cfg.status_data.geometric_vertical_accuracy & 0x3U, 2);
    append_unsigned(me, cfg.status_data.sil & 0x3U, 2);
    append_unsigned(me, cfg.status_data.horizontal_reference_true_north ? 1U : 0U, 1);
    append_unsigned(me, cfg.status_data.sil_supplement ? 1U : 0U, 1);
    append_unsigned(me, cfg.status_data.system_design_assurance & 0x3U, 2);
    return build_df17_frame(cfg.static_data.icao_address, me);
}

bool decode_position_message(const std::vector<uint8_t>& me_bits,
                             AdsbSensor::CprFrame& frame,
                             bool& geometric_altitude) {
    if (me_bits.size() != 56U) return false;
    uint32_t tc = read_unsigned(me_bits, 0, 5);
    if ((tc < 9U || tc > 18U) && (tc < 20U || tc > 22U)) return false;
    frame.valid = true;
    frame.odd = read_unsigned(me_bits, 21, 1) != 0U;
    frame.altitude_ft = decode_altitude_25ft(static_cast<uint16_t>(read_unsigned(me_bits, 8, 12)));
    frame.lat_cpr = read_unsigned(me_bits, 22, 17);
    frame.lon_cpr = read_unsigned(me_bits, 39, 17);
    frame.timestamp_ms = now_ms();
    geometric_altitude = tc >= 20U;
    return true;
}

} // namespace

AdsbSensor::AdsbSensor() : device_(std::make_unique<RfDevice>()) {
    device_->setReceiveCallback(
        [this](const std::vector<std::byte>& data, const RfReceiveReport& report) {
            ingest(data, report);
        });
    rebuildRfDefaults();
}

AdsbSensor::~AdsbSensor() = default;

void AdsbSensor::attachToModel(RfPropagationModel* model, const RfPosition& pos) {
    device_->attachToModel(model, pos);
}

void AdsbSensor::detachFromModel() {
    device_->detachFromModel();
}

void AdsbSensor::updatePosition(const RfPosition& pos) {
    device_->updatePosition(pos);
}

void AdsbSensor::configureAdsb(const AdsbConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool reset_schedule =
        (cfg.enabled != adsb_config_.enabled) ||
        (cfg.tx_enabled != adsb_config_.tx_enabled) ||
        (cfg.rx_enabled != adsb_config_.rx_enabled) ||
        (cfg.use_1090es != adsb_config_.use_1090es) ||
        (cfg.identification_interval_s != adsb_config_.identification_interval_s) ||
        (cfg.position_interval_s != adsb_config_.position_interval_s) ||
        (cfg.velocity_interval_s != adsb_config_.velocity_interval_s) ||
        (cfg.status_interval_s != adsb_config_.status_interval_s) ||
        (cfg.static_data.icao_address != adsb_config_.static_data.icao_address) ||
        (cfg.static_data.flight_id != adsb_config_.static_data.flight_id) ||
        (cfg.static_data.emitter_category != adsb_config_.static_data.emitter_category) ||
        (cfg.static_data.aircraft_length_m != adsb_config_.static_data.aircraft_length_m) ||
        (cfg.static_data.aircraft_width_m != adsb_config_.static_data.aircraft_width_m) ||
        (cfg.dynamic_data.latitude_deg != adsb_config_.dynamic_data.latitude_deg) ||
        (cfg.dynamic_data.longitude_deg != adsb_config_.dynamic_data.longitude_deg) ||
        (cfg.dynamic_data.altitude_baro_ft != adsb_config_.dynamic_data.altitude_baro_ft) ||
        (cfg.dynamic_data.altitude_geometric_ft != adsb_config_.dynamic_data.altitude_geometric_ft) ||
        (cfg.dynamic_data.ground_speed_kt != adsb_config_.dynamic_data.ground_speed_kt) ||
        (cfg.dynamic_data.track_angle_deg != adsb_config_.dynamic_data.track_angle_deg) ||
        (cfg.dynamic_data.vertical_rate_fpm != adsb_config_.dynamic_data.vertical_rate_fpm) ||
        (cfg.status_data.nacp != adsb_config_.status_data.nacp) ||
        (cfg.status_data.nic != adsb_config_.status_data.nic) ||
        (cfg.status_data.sil != adsb_config_.status_data.sil) ||
        (cfg.status_data.adsb_version != adsb_config_.status_data.adsb_version) ||
        (cfg.status_data.geometric_vertical_accuracy != adsb_config_.status_data.geometric_vertical_accuracy) ||
        (cfg.status_data.system_design_assurance != adsb_config_.status_data.system_design_assurance) ||
        (cfg.status_data.capability_class != adsb_config_.status_data.capability_class) ||
        (cfg.status_data.operational_mode != adsb_config_.status_data.operational_mode) ||
        (cfg.status_data.nic_supplement_a != adsb_config_.status_data.nic_supplement_a) ||
        (cfg.status_data.nic_baro != adsb_config_.status_data.nic_baro) ||
        (cfg.status_data.horizontal_reference_true_north != adsb_config_.status_data.horizontal_reference_true_north) ||
        (cfg.status_data.sil_supplement != adsb_config_.status_data.sil_supplement) ||
        (cfg.status_data.emergency_state != adsb_config_.status_data.emergency_state) ||
        (cfg.status_data.squawk != adsb_config_.status_data.squawk);
    adsb_config_ = cfg;
    rebuildRfDefaults(reset_schedule);
    device_->setPowerOn(adsb_config_.enabled &&
                        (adsb_config_.tx_enabled || adsb_config_.rx_enabled));
}

AdsbConfig AdsbSensor::getAdsbConfiguration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adsb_config_;
}

void AdsbSensor::configureRf(const RfConfig& cfg) {
    device_->configure(cfg);
}

RfConfig AdsbSensor::getRfConfiguration() const {
    return device_->getConfiguration();
}

void AdsbSensor::tick(double sim_time_s) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (adsb_config_.enabled && adsb_config_.tx_enabled && adsb_config_.static_data.icao_address != 0U) {
        if (should_transmit(sim_time_s, adsb_config_.identification_interval_s, last_ident_tx_s_)) {
            device_->transmit(encode_identification_frame(adsb_config_));
        }
        if (should_transmit(sim_time_s, adsb_config_.position_interval_s, last_position_tx_s_)) {
            device_->transmit(encode_position_frame(adsb_config_, next_position_odd_));
            next_position_odd_ = !next_position_odd_;
        }
        if (should_transmit(sim_time_s, adsb_config_.velocity_interval_s, last_velocity_tx_s_)) {
            device_->transmit(encode_velocity_frame(adsb_config_));
        }
        if (should_transmit(sim_time_s, adsb_config_.status_interval_s, last_status_tx_s_)) {
            device_->transmit(encode_emergency_status_frame(adsb_config_));
            device_->transmit(encode_operational_status_frame(adsb_config_));
        }
    }

    if (adsb_config_.track_stale_timeout_s <= 0.0) return;
    uint64_t stale_ms = static_cast<uint64_t>(adsb_config_.track_stale_timeout_s * 1000.0);
    uint64_t current_ms = now_ms();
    for (auto& kv : tracks_) {
        kv.second.stale = (current_ms - kv.second.last_update_ms) > stale_ms;
    }
}

bool AdsbSensor::transmitIdentification() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!adsb_config_.enabled || !adsb_config_.tx_enabled || adsb_config_.static_data.icao_address == 0U) return false;
    return device_->transmit(encode_identification_frame(adsb_config_));
}

bool AdsbSensor::transmitPosition() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!adsb_config_.enabled || !adsb_config_.tx_enabled || adsb_config_.static_data.icao_address == 0U) return false;
    bool ok = device_->transmit(encode_position_frame(adsb_config_, next_position_odd_));
    if (ok) next_position_odd_ = !next_position_odd_;
    return ok;
}

bool AdsbSensor::transmitVelocity() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!adsb_config_.enabled || !adsb_config_.tx_enabled || adsb_config_.static_data.icao_address == 0U) return false;
    return device_->transmit(encode_velocity_frame(adsb_config_));
}

bool AdsbSensor::transmitStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!adsb_config_.enabled || !adsb_config_.tx_enabled || adsb_config_.static_data.icao_address == 0U) return false;
    bool emergency_ok = device_->transmit(encode_emergency_status_frame(adsb_config_));
    bool operational_ok = device_->transmit(encode_operational_status_frame(adsb_config_));
    return emergency_ok && operational_ok;
}

std::vector<AdsbTrack> AdsbSensor::getTracks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AdsbTrack> out;
    out.reserve(tracks_.size());
    for (const auto& kv : tracks_) out.push_back(kv.second);
    return out;
}

std::optional<AdsbTrack> AdsbSensor::getTrack(uint32_t icao_address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(icao_address);
    if (it == tracks_.end()) return std::nullopt;
    return it->second;
}

void AdsbSensor::clearTracks() {
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_.clear();
    even_cpr_frames_.clear();
    odd_cpr_frames_.clear();
}

void AdsbSensor::setTrackUpdateCallback(TrackUpdateCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    update_cb_ = std::move(cb);
}

void AdsbSensor::setReceiveCallback(ReceiveCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    receive_cb_ = std::move(cb);
}

RfDevice& AdsbSensor::rfDevice() {
    return *device_;
}

const RfDevice& AdsbSensor::rfDevice() const {
    return *device_;
}

void AdsbSensor::ingest(const std::vector<std::byte>& data, const RfReceiveReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!adsb_config_.enabled || !adsb_config_.rx_enabled) return;

    uint32_t icao_address = 0U;
    std::vector<uint8_t> me_bits;
    if (!parse_df17_frame(data, icao_address, me_bits)) return;
    if (icao_address == 0U) return;

    AdsbTrack& track = tracks_[icao_address];
    track.icao_address = icao_address;
    track.static_data.icao_address = icao_address;
    track.last_report = report;
    track.last_update_ms = now_ms();
    track.stale = false;

    uint32_t type_code = read_unsigned(me_bits, 0, 5);
    if (type_code >= 1U && type_code <= 4U) {
        track.last_message_kind = AdsbMessageKind::IDENTIFICATION;
        track.static_data.emitter_category = static_cast<uint8_t>(((type_code - 1U) * 8U) + read_unsigned(me_bits, 5, 3));
        track.static_data.flight_id = decode_callsign_string(me_bits, 8);
    } else if ((type_code >= 9U && type_code <= 18U) || (type_code >= 20U && type_code <= 22U)) {
        AdsbSensor::CprFrame frame;
        bool geometric_altitude = false;
        if (!decode_position_message(me_bits, frame, geometric_altitude)) return;

        const uint64_t current_ms = track.last_update_ms;
        const uint64_t stale_window_ms = static_cast<uint64_t>(kCprDecodeWindowS * 1000.0);

        if (frame.odd) {
            odd_cpr_frames_[icao_address] = frame;
        } else {
            even_cpr_frames_[icao_address] = frame;
        }

        if (geometric_altitude) {
            track.dynamic_data.altitude_geometric_ft = frame.altitude_ft;
        } else {
            track.dynamic_data.altitude_baro_ft = frame.altitude_ft;
        }
        auto prune_if_stale = [current_ms, stale_window_ms](auto& frames, uint32_t icao) {
            auto it = frames.find(icao);
            if (it != frames.end() && current_ms > it->second.timestamp_ms &&
                (current_ms - it->second.timestamp_ms) > stale_window_ms) {
                frames.erase(it);
            }
        };
        prune_if_stale(even_cpr_frames_, icao_address);
        prune_if_stale(odd_cpr_frames_, icao_address);

        bool have_reference = (track.last_message_kind == AdsbMessageKind::AIRBORNE_POSITION) &&
                              has_nontrivial_reference(track.dynamic_data.latitude_deg,
                                                       track.dynamic_data.longitude_deg);
        double ref_lat = track.dynamic_data.latitude_deg;
        double ref_lon = track.dynamic_data.longitude_deg;
        if (!have_reference &&
            has_nontrivial_reference(adsb_config_.dynamic_data.latitude_deg,
                                     adsb_config_.dynamic_data.longitude_deg)) {
            ref_lat = adsb_config_.dynamic_data.latitude_deg;
            ref_lon = adsb_config_.dynamic_data.longitude_deg;
            have_reference = true;
        }

        bool resolved = false;
        double resolved_lat = track.dynamic_data.latitude_deg;
        double resolved_lon = track.dynamic_data.longitude_deg;
        auto even_it = even_cpr_frames_.find(icao_address);
        auto odd_it = odd_cpr_frames_.find(icao_address);
        if (even_it != even_cpr_frames_.end() && odd_it != odd_cpr_frames_.end()) {
            double age_s = std::fabs(static_cast<double>(even_it->second.timestamp_ms) -
                                     static_cast<double>(odd_it->second.timestamp_ms)) / 1000.0;
            if (age_s <= kCprDecodeWindowS) {
                bool use_odd = odd_it->second.timestamp_ms >= even_it->second.timestamp_ms;
                double global_lat = 0.0;
                double global_lon = 0.0;
                if (decode_cpr_global(even_it->second, odd_it->second, use_odd, global_lat, global_lon)) {
                    resolved_lat = global_lat;
                    resolved_lon = global_lon;
                    resolved = true;

                    if (have_reference) {
                        double local_lat = 0.0;
                        double local_lon = 0.0;
                        const AdsbSensor::CprFrame& freshest = use_odd ? odd_it->second : even_it->second;
                        if (decode_cpr_local(freshest, ref_lat, ref_lon, local_lat, local_lon)) {
                            double global_metric = angular_distance_metric(global_lat, global_lon, ref_lat, ref_lon);
                            double local_metric = angular_distance_metric(local_lat, local_lon, ref_lat, ref_lon);
                            if (local_metric + 1e-9 < global_metric) {
                                resolved_lat = local_lat;
                                resolved_lon = local_lon;
                            }
                        }
                    }
                }
            }
        }

        if (!resolved && have_reference) {
            double local_lat = 0.0;
            double local_lon = 0.0;
            if (decode_cpr_local(frame, ref_lat, ref_lon, local_lat, local_lon)) {
                resolved_lat = local_lat;
                resolved_lon = local_lon;
                resolved = true;
            }
        }

        if (resolved) {
            track.dynamic_data.latitude_deg = resolved_lat;
            track.dynamic_data.longitude_deg = resolved_lon;
        }
        track.last_message_kind = AdsbMessageKind::AIRBORNE_POSITION;
    } else if (type_code == 19U) {
        uint32_t subtype = read_unsigned(me_bits, 5, 3);
        if (subtype == 1U || subtype == 2U) {
            double east_kt = static_cast<double>(read_unsigned(me_bits, 14, 10));
            double north_kt = static_cast<double>(read_unsigned(me_bits, 25, 10));
            if (east_kt > 0.0) east_kt -= 1.0;
            if (north_kt > 0.0) north_kt -= 1.0;
            if (read_unsigned(me_bits, 13, 1) != 0U) east_kt = -east_kt;
            if (read_unsigned(me_bits, 24, 1) != 0U) north_kt = -north_kt;

            double vr_fpm = 0.0;
            uint32_t vr_raw = read_unsigned(me_bits, 37, 9);
            if (vr_raw > 0U) {
                vr_fpm = static_cast<double>((static_cast<int>(vr_raw) - 1) * 64);
                if (read_unsigned(me_bits, 36, 1) != 0U) vr_fpm = -vr_fpm;
            }

            track.dynamic_data.ground_speed_kt = std::sqrt(east_kt * east_kt + north_kt * north_kt);
            track.dynamic_data.track_angle_deg = normalize_deg(std::atan2(east_kt, north_kt) * 180.0 / 3.14159265358979323846);
            track.dynamic_data.vertical_rate_fpm = vr_fpm;
            track.last_message_kind = AdsbMessageKind::AIRBORNE_VELOCITY;
        } else {
            return;
        }
    } else if (type_code == 28U) {
        track.last_message_kind = AdsbMessageKind::STATUS;
        track.status_data.emergency_state = static_cast<AdsbEmergencyState>(read_unsigned(me_bits, 8, 3));
        track.status_data.squawk = decode_squawk_code(static_cast<uint16_t>(read_unsigned(me_bits, 11, 13)));
    } else if (type_code == 31U) {
        track.last_message_kind = AdsbMessageKind::STATUS;
        uint32_t subtype = read_unsigned(me_bits, 5, 3);
        if (subtype != 0U) return;
        track.status_data.capability_class = static_cast<uint16_t>(read_unsigned(me_bits, 8, 16));
        track.status_data.operational_mode = static_cast<uint16_t>(read_unsigned(me_bits, 24, 16));
        track.status_data.adsb_version = static_cast<uint8_t>(read_unsigned(me_bits, 40, 3));
        track.status_data.nic_supplement_a = read_unsigned(me_bits, 43, 1) != 0U;
        track.status_data.nacp = static_cast<uint8_t>(read_unsigned(me_bits, 44, 4));
        track.status_data.geometric_vertical_accuracy = static_cast<uint8_t>(read_unsigned(me_bits, 48, 2));
        track.status_data.sil = static_cast<uint8_t>(read_unsigned(me_bits, 50, 2));
        track.status_data.horizontal_reference_true_north = read_unsigned(me_bits, 52, 1) != 0U;
        track.status_data.sil_supplement = read_unsigned(me_bits, 53, 1) != 0U;
        track.status_data.system_design_assurance = static_cast<uint8_t>(read_unsigned(me_bits, 54, 2));
    } else {
        return;
    }

    if (update_cb_) update_cb_(track);
    if (receive_cb_) {
        AdsbReceiveReport adsb_report;
        adsb_report.track = track;
        adsb_report.raw_frame = data;
        receive_cb_(adsb_report);
    }
}

void AdsbSensor::rebuildRfDefaults(bool reset_schedule) {
    if (reset_schedule) {
        last_ident_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        last_position_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        last_velocity_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        last_status_tx_s_ = std::numeric_limits<double>::quiet_NaN();
        next_position_odd_ = false;
    }
    RfConfig current = device_->getConfiguration();
    RfConfig rf = makeAdsbRfConfig(adsb_config_,
                                   adsb_config_.tx_enabled && adsb_config_.rx_enabled
                                       ? RfMode::TRANSCEIVER
                                       : (adsb_config_.tx_enabled ? RfMode::TRANSMITTER_ONLY
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

RfConfig makeAdsbRfConfig(const AdsbConfig&, RfMode mode) {
    RfConfig rf;
    rf.mode = mode;
    rf.protocol = rf::RfProtocol::ADSB;
    rf.min_freq_hz = 1089e6;
    rf.max_freq_hz = 1091e6;
    rf.frequency_hz = 1090e6;
    rf.receive_frequencies_hz = {1090e6};
    rf.bandwidth_hz = 2e6;
    rf.rx_bandwidth_hz = 2e6;
    rf.tx_power_dbm = 53.0;
    rf.required_snr_threshold_db = 8.0;
    rf.antenna.gain_dbi = 2.0;
    rf.antenna.beamwidth_deg = 360.0;
    rf.receiver.sensitivity_dbm = -85.0;
    rf.receiver.noise_figure_db = 5.0;
    rf.receiver.squelch_threshold_db = 3.0;
    rf.is_naval = false;
    return rf;
}

} // namespace adsb
