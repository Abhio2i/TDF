#include "adsbsensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/ADSB/ADSB.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/RF/RF.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Simulation/simulation.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include <iostream>
#include <functional>
#include <QJsonDocument>


rf::RfPropagationModel* ADSBSensor::model = nullptr;
const float RAD2DEG = 180.0f / M_PI;
ADSBSensor::ADSBSensor(Hierarchy* h) : Sensor(h) {
    subType = SubType::ADSB;
    azimuth = 360;
    frequency = 10.0f;


    rf::RfPropagationConfig model_cfg;
    model_cfg.enable_fspl = true;
    model_cfg.enable_noise_floor = true;
    model_cfg.enable_sensitivity = true;
    model_cfg.enable_squelch = true;
    model_cfg.enable_range_limit = false;
    model_cfg.enable_scan_beam = false;
    model_cfg.enable_shadowing = false;
    model_cfg.enable_fading = false;
    model_cfg.enable_doppler = true;
    model_cfg.enable_environmental_attenuation = true;
    model_cfg.enable_polarization_loss = true;

    // rf::RfPropagationModel model(model_cfg);
    if(model == nullptr){
        model = new rf::RfPropagationModel(model_cfg);
    }

    adsb::AdsbConfig cfg;
    cfg.enabled = true;
    cfg.tx_enabled = true;
    cfg.rx_enabled = true;

    cfg.identification_interval_s = 2.0;
    cfg.position_interval_s = 1.0;
    cfg.velocity_interval_s = 1.0;
    cfg.status_interval_s = 5.0;
    cfg.track_stale_timeout_s = 30.0;

    cfg.static_data.icao_address = 0xABC001;
    cfg.static_data.flight_id = "OWN123";
    cfg.static_data.emitter_category = 12;
    cfg.static_data.aircraft_length_m = 38;
    cfg.static_data.aircraft_width_m = 35;

    cfg.dynamic_data.latitude_deg = 18.95000;
    cfg.dynamic_data.longitude_deg = 72.82000;
    cfg.dynamic_data.altitude_baro_ft = 32000.0;
    cfg.dynamic_data.altitude_geometric_ft = 32000.0 + 75.0;
    cfg.dynamic_data.ground_speed_kt = 440.00;
    cfg.dynamic_data.track_angle_deg = 90.00;
    cfg.dynamic_data.vertical_rate_fpm = 0.00;

    cfg.status_data.nacp = 9;
    cfg.status_data.nic = 8;
    cfg.status_data.sil = 3;
    cfg.status_data.adsb_version = 2;
    cfg.status_data.geometric_vertical_accuracy = 2;
    cfg.status_data.system_design_assurance = 2;
    cfg.status_data.capability_class = 0xA55A;
    cfg.status_data.operational_mode = 0x0F0F;
    cfg.status_data.nic_supplement_a = true;
    cfg.status_data.horizontal_reference_true_north = true;
    cfg.status_data.sil_supplement = true;
    cfg.status_data.emergency_state = adsb::AdsbEmergencyState::NONE;
    cfg.status_data.squawk = 1200;
    cfg.status_data.utc_second = 0;

    ownship.configureAdsb(cfg);
    rf::RfConfig own_rf = adsb::makeAdsbRfConfig(cfg, rf::RfMode::TRANSCEIVER);
    own_rf.id = "OWN_ADSB";
    own_rf.parent_name = "OWN_AIRCRAFT";
    own_rf.use_local_propagation_config = true;
    own_rf.propagation.enable_fspl = true;
    own_rf.propagation.enable_log_distance = false;
    own_rf.propagation.enable_two_ray = false;
    own_rf.propagation.enable_shadowing = false;
    own_rf.propagation.shadowing_sigma_db = 1.5;
    own_rf.propagation.enable_fading = false;
    own_rf.propagation.fading_sigma_db = 1.0;
    own_rf.propagation.enable_polarization_loss = true;
    own_rf.propagation.polarization_mismatch_loss_db = 3.0;
    own_rf.propagation.enable_los_horizon = true;
    own_rf.propagation.enable_comms_mode_losses = true;
    own_rf.propagation.enable_noise_floor = true;
    own_rf.propagation.enable_snr_threshold = true;
    own_rf.propagation.enable_sensitivity = true;
    own_rf.propagation.enable_squelch = true;
    own_rf.propagation.enable_interference = false;
    own_rf.propagation.enable_range_limit = false;
    own_rf.propagation.enable_scan_beam = true;
    own_rf.propagation.enable_scan_timing = false;
    own_rf.propagation.enable_doppler = true;

    // Environmental toggles + values
    own_rf.propagation.enable_environmental_attenuation = true;
    own_rf.propagation.temperature_c = 20.0;
    own_rf.propagation.pressure_hpa = 1005.0;
    own_rf.propagation.humidity_percent = 60.0;
    own_rf.propagation.gas_attenuation_db_per_km_at_1ghz = 0.005;
    own_rf.propagation.gas_attenuation_freq_exponent = 1.0;
    own_rf.propagation.humidity_attenuation_factor_per_percent = 0.002;
    own_rf.propagation.rain_rate_mm_per_hr = 2.5;
    own_rf.propagation.rain_attenuation_db_per_km_per_mmhr = 0.004;
    own_rf.propagation.use_itu_rain_model = true;
    own_rf.propagation.rain_coverage = 0.4;
    own_rf.propagation.rain_rate_sigma_frac = 0.15;
    own_rf.propagation.wind_speed_mps = 25.0;
    own_rf.propagation.wind_attenuation_db_per_km_per_mps = 0.0005;
    own_rf.propagation.enable_sea_attenuation = true;
    own_rf.propagation.sea_attenuation_db_per_km = 0.003;
    ownship.configureRf(own_rf);
    ownship.attachToModel(model,  {0.0,     0.0,   10000.0});

    ownship.setTrackUpdateCallback([](const adsb::AdsbTrack& t) {
        std::cout << "[OWN RECEIVED TRACK UPDATE] ICAO=0x"
                  << std::hex << std::uppercase << t.icao_address << std::dec
                  << " Flight=" << t.static_data.flight_id
                  << " SNR=" << t.last_report.snr_db << " dB\n";
    });

    ownship.setReceiveCallback([this](const adsb::AdsbReceiveReport& r) {
        rxEvents.append({Simulation::simulationTime, r});
    });
}

void ADSBSensor::scan(){
    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;



    rf::RfPosition pos;
    pos.x = source->translation().z()*1000.f;
    pos.y = source->translation().x()*1000.f;
    pos.altitude = source->translation().y()*1000.f;
    ownship.updatePosition(pos);


    {
        rf::RfConfig own_rf = ownship.getRfConfiguration();
        own_rf.id = rf_id.empty() ? parentEntity->ID : rf_id;
        own_rf.parent_name = parentEntity->Name;
        own_rf.use_local_propagation_config = true;


        adsb::AdsbConfig cfg = ownship.getAdsbConfiguration();
        cfg.enabled = enabled && parentEntity->Active;
        cfg.status_data.utc_second = static_cast<uint32_t>(Simulation::simulationTime);
        cfg.static_data.flight_id = parentEntity->Name;
        ownship.configureAdsb(cfg);
        ownship.configureRf(own_rf);
    }

    ownship.tick(Simulation::simulationTime);

    std::vector<rf::RfScanHit> rf_hits = ownship.rfDevice().scan();

    // std::cout << "Frame " << frame
    //           << " | sim_time=" << sim_time_s << "s"
    //           << " | rf_hits=" << rf_hits.size()
    //           << " | adsb_tracks=" << ownship.getTracks().size()
    //           << "\n";

    detect.clear();
    if (!rf_hits.empty()) {
        std::cout << " Physical RF Hits:\n";
        for (const auto& h : rf_hits) {
            ADSBTarget target;
            try {
                if ((root->Platforms)[h.id]) {
                    target.entity = (root->Platforms)[h.id];
                }else{
                    target.entity = nullptr;
                }
            } catch (const std::exception& e) {
                // Yahan error handle karein
                qDebug() << "Error accessing platform:" << e.what();
            }
            target.angle = h.azimuth_deg;//-((h.azimuth_deg+h)+180.f);
            target.radius = h.distance_m/1000.f;
            detect.append(target);

            // std::cout << "  ID=" << h.id
            //           << " Dist=" << h.distance_m
            //           << "m Rx=" << h.rx_power_dbm
            //           << "dBm SNR=" << h.snr_db
            //           << "dB\n";
        }
    }

    if ((Simulation::simulationTime - lastPrintTime) >= 1.0f) {
        lastPrintTime = Simulation::simulationTime;

        for (const auto& e : rxEvents) {
            const auto& t = e.report.track;

            if (parentEntity && t.last_report.sender_id == parentEntity->ID) {
                continue;
            }

            qDebug().noquote()
                << QString("[ADSB RX] receiver=%1 sender=%2 icao=%3 flight=%4 lat=%5 lon=%6 alt_ft=%7 gs=%8 track=%9 vr=%10 snr=%11")
                       .arg(QString::fromStdString(parentEntity ? parentEntity->Name : Name))
                       .arg(QString::fromStdString(t.last_report.sender_id))
                       .arg(t.icao_address)
                       .arg(QString::fromStdString(t.static_data.flight_id))
                       .arg(t.dynamic_data.latitude_deg, 0, 'f', 5)
                       .arg(t.dynamic_data.longitude_deg, 0, 'f', 5)
                       .arg(t.dynamic_data.altitude_baro_ft, 0, 'f', 1)
                       .arg(t.dynamic_data.ground_speed_kt, 0, 'f', 2)
                       .arg(t.dynamic_data.track_angle_deg, 0, 'f', 2)
                       .arg(t.dynamic_data.vertical_rate_fpm, 0, 'f', 1)
                       .arg(t.last_report.snr_db, 0, 'f', 2);
        }

        rxEvents.clear();
    }
}


QJsonObject ADSBSensor::toJson() const {

    adsb::AdsbConfig cfg = ownship.getAdsbConfiguration();
    rf::RfConfig own_rf = ownship.getRfConfiguration();
    QJsonObject obj;
    obj["active"] = Active;
    obj["name"] = QString::fromStdString(Name);
    obj["SensorType"] = "ADSB";
    obj["id"] = QString::fromStdString(ID);
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";

    defaultObj["enabled"] = cfg.enabled;
    defaultObj["tx_enabled"] = cfg.tx_enabled;
    defaultObj["rx_enabled"] = cfg.rx_enabled;

    defaultObj["icao_address"] = QString::number(static_cast<qulonglong>(cfg.static_data.icao_address));
    defaultObj["flight_id"] = QString::fromStdString(cfg.static_data.flight_id);

    // defaultObj["latitude_deg"] = toParm(cfg.dynamic_data.latitude_deg, "deg", -90.0, 90.0);
    // defaultObj["longitude_deg"] = toParm(cfg.dynamic_data.longitude_deg, "deg", -180.0, 180.0);
    // defaultObj["altitude_baro_ft"] = toParm(cfg.dynamic_data.altitude_baro_ft, "ft", -2000.0, 100000.0);
    // defaultObj["ground_speed_kt"] = toParm(cfg.dynamic_data.ground_speed_kt, "kt", 0.0, 1200.0);
    // defaultObj["track_angle_deg"] = toParm(cfg.dynamic_data.track_angle_deg, "deg", 0.0, 360.0);
    // defaultObj["vertical_rate_fpm"] = toParm(cfg.dynamic_data.vertical_rate_fpm, "fpm", -10000.0, 10000.0);


    defaultObj["identification_interval_s"] = toParm(cfg.identification_interval_s, "s", 0.1, 600.0);
    defaultObj["position_interval_s"] = toParm(cfg.position_interval_s, "s", 0.1, 600.0);
    defaultObj["velocity_interval_s"] = toParm(cfg.velocity_interval_s, "s", 0.1, 600.0);
    defaultObj["status_interval_s"] = toParm(cfg.status_interval_s, "s", 0.1, 600.0);
    defaultObj["track_stale_timeout_s"] = toParm(cfg.track_stale_timeout_s, "s", 0.0, 3600.0);

    // defaultObj["rf_id"] = QString::fromStdString(own_rf.id);
    // defaultObj["parent_name"] = QString::fromStdString(own_rf.parent_name);

    QString rfMode = "TRANSCEIVER";
    if (own_rf.mode == rf::RfMode::TRANSMITTER_ONLY) rfMode = "TRANSMITTER_ONLY";
    else if (own_rf.mode == rf::RfMode::RECEIVER_ONLY) rfMode = "RECEIVER_ONLY";
    defaultObj["rf_mode"] = rfMode;

    obj["default"] = defaultObj;


    QJsonObject Env;
    Env["type"] = "Section";
    Env["temperature_c"] = toParm(own_rf.propagation.temperature_c,"cel");
    Env["pressure_hpa"] = toParm(own_rf.propagation.pressure_hpa,"hpa");
    Env["humidity_percent"] = toParm(own_rf.propagation.humidity_percent,"%");
    //Env["gas_attenuation_db_per_km_at_1ghz"] = toParm(own_rf.propagation.gas_attenuation_db_per_km_at_1ghz,"db/km");
    // Env["gas_attenuation_freq_exponent"] = toParm(own_rf.propagation.gas_attenuation_freq_exponent,"");
    // Env["humidity_attenuation_factor_per_percent"] = toParm(own_rf.propagation.humidity_attenuation_factor_per_percent,"%");
    Env["rain_rate_mm_per_hr"] = toParm(own_rf.propagation.rain_rate_mm_per_hr,"mm/h");
    //  Env["rain_attenuation_db_per_km_per_mmhr"] = toParm(cfg.propagation.rain_attenuation_db_per_km_per_mmhr,"db/km/h");
    Env["rain_coverage"] = toParm(own_rf.propagation.rain_coverage,"%");
    // Env["rain_rate_sigma_frac"] = toParm(cfg.propagation.rain_rate_sigma_frac,"mm/h");
    Env["wind_speed_mps"] = toParm(own_rf.propagation.wind_speed_mps,"m/s");
    // Env["wind_attenuation_db_per_km_per_mps"] = toParm(cfg.propagation.wind_attenuation_db_per_km_per_mps,"db/km");
    // Env["sea_attenuation_db_per_km"] = toParm(cfg.propagation.sea_attenuation_db_per_km,"db/km");
    obj["Environmental"] = Env;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    return obj;
}
void ADSBSensor::fromJson(const QJsonObject& obj) {
    adsb::AdsbConfig cfg = ownship.getAdsbConfiguration();
    rf::RfConfig own_rf = ownship.getRfConfiguration();

    auto readParmOrDouble = [this](const QJsonValue& v, double currentValue) -> double {
        if (v.isObject()) return valueFromParm(v.toObject());
        if (v.isDouble()) return v.toDouble();
        return currentValue;
    };

    auto readUInt32 = [](const QJsonValue& v, uint32_t currentValue) -> uint32_t {
        bool ok = false;

        if (v.isString()) {
            QString s = v.toString().trimmed();
            qulonglong value = 0;

            if (s.startsWith("0x", Qt::CaseInsensitive)) {
                value = s.mid(2).toULongLong(&ok, 16);
            } else {
                value = s.toULongLong(&ok, 10);
            }

            if (ok) return static_cast<uint32_t>(value);
            return currentValue;
        }

        qulonglong value = v.toVariant().toULongLong(&ok);
        if (ok) return static_cast<uint32_t>(value);

        return currentValue;
    };

    if (obj.contains("id")) {
        ID = obj["id"].toString().toStdString();
    }

    if (obj.contains("active")) {
        Active = obj["active"].toBool();
    }

    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();

        // Existing fields
        if (defaultObj.contains("range"))
            range = readParmOrDouble(defaultObj["range"], range);

        if (defaultObj.contains("frequency"))
            frequency = readParmOrDouble(defaultObj["frequency"], frequency);

        if (defaultObj.contains("azimuth"))
            azimuth = readParmOrDouble(defaultObj["azimuth"], azimuth);

        // ADS-B input fields
        if (defaultObj.contains("enabled"))
            cfg.enabled = defaultObj["enabled"].toBool();

        if (defaultObj.contains("tx_enabled"))
            cfg.tx_enabled = defaultObj["tx_enabled"].toBool();

        if (defaultObj.contains("rx_enabled"))
            cfg.rx_enabled = defaultObj["rx_enabled"].toBool();

        if (defaultObj.contains("icao_address"))
            cfg.static_data.icao_address = readUInt32(defaultObj["icao_address"], icao_address);

        if (defaultObj.contains("flight_id"))
            cfg.static_data.flight_id = defaultObj["flight_id"].toString().toStdString();

        // if (defaultObj.contains("latitude_deg"))
        //     latitude_deg = readParmOrDouble(defaultObj["latitude_deg"], latitude_deg);

        // if (defaultObj.contains("longitude_deg"))
        //     longitude_deg = readParmOrDouble(defaultObj["longitude_deg"], longitude_deg);

        // if (defaultObj.contains("altitude_baro_ft"))
        //     altitude_baro_ft = readParmOrDouble(defaultObj["altitude_baro_ft"], altitude_baro_ft);

        // if (defaultObj.contains("ground_speed_kt"))
        //     ground_speed_kt = readParmOrDouble(defaultObj["ground_speed_kt"], ground_speed_kt);

        // if (defaultObj.contains("track_angle_deg"))
        //     track_angle_deg = readParmOrDouble(defaultObj["track_angle_deg"], track_angle_deg);

        // if (defaultObj.contains("vertical_rate_fpm"))
        //     vertical_rate_fpm = readParmOrDouble(defaultObj["vertical_rate_fpm"], vertical_rate_fpm);

        if (defaultObj.contains("identification_interval_s"))
            cfg.identification_interval_s = readParmOrDouble(defaultObj["identification_interval_s"], identification_interval_s);

        if (defaultObj.contains("position_interval_s"))
            cfg.position_interval_s = readParmOrDouble(defaultObj["position_interval_s"], position_interval_s);

        if (defaultObj.contains("velocity_interval_s"))
            cfg.velocity_interval_s = readParmOrDouble(defaultObj["velocity_interval_s"], velocity_interval_s);

        if (defaultObj.contains("status_interval_s"))
            cfg.status_interval_s = readParmOrDouble(defaultObj["status_interval_s"], status_interval_s);

        if (defaultObj.contains("track_stale_timeout_s"))
            cfg.track_stale_timeout_s = readParmOrDouble(defaultObj["track_stale_timeout_s"], track_stale_timeout_s);

        // if (defaultObj.contains("rf_id"))
        //     cfg.rf_id = defaultObj["rf_id"].toString().toStdString();

        // if (defaultObj.contains("parent_name"))
        //     cfg.parent_name = defaultObj["parent_name"].toString().toStdString();

        // if (defaultObj.contains("rf_mode"))
        //     own_rf.mode = defaultObj["rf_mode"].toString().toStdString();
        // }
    }


    if (obj.contains("Environmental") && obj["Environmental"].isObject()) {
        QJsonObject Env = obj["Environmental"].toObject();
        if (Env.contains("temperature_c"))
            own_rf.propagation.temperature_c = valueFromParm(Env["temperature_c"].toObject());
        if (Env.contains("pressure_hpa"))
            own_rf.propagation.pressure_hpa = valueFromParm(Env["pressure_hpa"].toObject());
        if (Env.contains("humidity_percent"))
            own_rf.propagation.humidity_percent = valueFromParm(Env["humidity_percent"].toObject());
        if (Env.contains("rain_rate_mm_per_hr"))
            own_rf.propagation.rain_rate_mm_per_hr = valueFromParm(Env["rain_rate_mm_per_hr"].toObject());
        if (Env.contains("rain_coverage"))
            own_rf.propagation.rain_coverage = valueFromParm(Env["rain_coverage"].toObject());
        if (Env.contains("wind_speed_mps"))
            own_rf.propagation.wind_speed_mps = valueFromParm(Env["wind_speed_mps"].toObject());
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }
    ownship.configureAdsb(cfg);
    ownship.configureRf(own_rf);

    // if (rf_id.empty()) {
    //     rf_id = ID;
    // }
}




