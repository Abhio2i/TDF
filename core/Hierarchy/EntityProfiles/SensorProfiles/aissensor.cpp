#include "aissensor.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/AIS/AIS.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/AIS_ADSB/RF/RF.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Simulation/simulation.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include <iostream>
#include <functional>

const float RAD2DEG = 180.0f / M_PI;
rf::RfPropagationModel* AISSensor::model = nullptr;

AISSensor::AISSensor(Hierarchy* h) : Sensor(h) {
    subType = SubType::AIS;
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

    // rf::RfPropagationModel model(model_cfg);
    if(model == nullptr){
        model = new rf::RfPropagationModel(model_cfg);
    }

    ais::AisConfig cfg;
    cfg.enabled = true;
    cfg.tx_enabled = true;
    cfg.rx_enabled = true;
    cfg.ais_class = ais::AisClass::CLASS_A;
    cfg.channel_mode = ais::AisChannelMode::DUAL;
    cfg.static_data.mmsi = 419123456;
    cfg.static_data.imo = 9000000 + (419123456 % 1000000);
    cfg.static_data.name = "OWN_SHIP";
    cfg.static_data.callsign = "VT" + std::to_string(419123456 % 10000);
    cfg.static_data.destination = "MUMBAI";
    cfg.static_data.ship_type = 35;
    cfg.static_data.dim_bow_m = 40;
    cfg.static_data.dim_stern_m = 10;
    cfg.static_data.dim_port_m = 6;
    cfg.static_data.dim_starboard_m = 6;
    cfg.static_data.draught_m = 5.5f;

    cfg.dynamic_data.latitude_deg = 18.95000;
    cfg.dynamic_data.longitude_deg = 72.82000;
    cfg.dynamic_data.sog_kn = 12.5;
    cfg.dynamic_data.cog_deg = 90.0;
    cfg.dynamic_data.heading_deg = 90.0;
    cfg.dynamic_data.rot_deg_per_min = 0.0;
    cfg.dynamic_data.position_accuracy = true;
    cfg.dynamic_data.rot_deg_per_min = rot_deg_per_min;
    cfg.dynamic_data.nav_status = ais::AisNavStatus::UNDER_WAY;

    cfg.dynamic_interval_s = 1.0;
    cfg.static_interval_s = 2.0;
    cfg.track_stale_timeout_s = 30.0;


    ownship.configureAis(cfg);

    rf::RfConfig own_rf = ais::makeAisRfConfig(cfg, false, rf::RfMode::TRANSCEIVER);
    own_rf.id = "OWN_AIS";
    own_rf.parent_name = "OWN_PLATFORM";
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
    ownship.attachToModel(model,   {0.0,    0.0,  20.0});
    ownship.setTrackUpdateCallback([](const ais::AisTrack& t) {
        std::cout << "[OWN RECEIVED TRACK UPDATE] MMSI=" << t.mmsi
                  << " Name=" << t.static_data.name
                  << " SNR=" << t.last_report.snr_db << " dB\n";
    });

    ownship.setReceiveCallback([this](const ais::AisReceiveReport& r) {
        rxEvents.append({Simulation::simulationTime, r});
    });


}


void AISSensor::scan(){
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
        ais::AisConfig cfg = ownship.getAisConfiguration();

        // Respect UI-configured values; only refresh runtime-driven pieces here.
        cfg.enabled = enabled && parentEntity->Active;
        cfg.tx_enabled = tx_enabled;
        cfg.rx_enabled = rx_enabled;

        cfg.dynamic_data.heading_deg = source->getHeading();

        ownship.configureAis(cfg);

        rf::RfConfig own_rf = ownship.getRfConfiguration();
        own_rf.id = rf_id.empty() ? parentEntity->ID : rf_id;
        own_rf.parent_name = parentEntity->Name;
        ownship.configureRf(own_rf);
    }


    ownship.tick(Simulation::simulationTime);

    std::vector<rf::RfScanHit> rf_hits = ownship.rfDevice().scan();

    ewtargets.clear();
    if (!rf_hits.empty()) {
        std::cout << " Physical RF Hits:\n";
        for (const auto& h : rf_hits) {

            Target target;
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
            ewtargets.append(target);

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
                << QString("[AIS RX] receiver=%1 sender=%2 mmsi=%3 name=%4 lat=%5 lon=%6 sog=%7 cog=%8 hdg=%9 snr=%10")
                       .arg(QString::fromStdString(parentEntity ? parentEntity->Name : Name))
                       .arg(QString::fromStdString(t.last_report.sender_id))
                       .arg(t.mmsi)
                       .arg(QString::fromStdString(t.static_data.name))
                       .arg(t.dynamic_data.latitude_deg, 0, 'f', 5)
                       .arg(t.dynamic_data.longitude_deg, 0, 'f', 5)
                       .arg(t.dynamic_data.sog_kn, 0, 'f', 2)
                       .arg(t.dynamic_data.cog_deg, 0, 'f', 2)
                       .arg(t.dynamic_data.heading_deg, 0, 'f', 2)
                       .arg(t.last_report.snr_db, 0, 'f', 2);
        }

        rxEvents.clear();
    }
}

QJsonObject AISSensor::toJson() const {

    ais::AisConfig cfg = ownship.getAisConfiguration();
    rf::RfConfig own_rf = ownship.getRfConfiguration();


    QJsonObject obj;
    obj["active"] = Active;
    obj["name"] = QString::fromStdString(Name);
    obj["SensorType"] = "AIS";
    obj["id"] = QString::fromStdString(ID);

    QJsonObject defaultObj;
    defaultObj["type"] = "Section";

    // Existing fields
    // defaultObj["range"] = toParm(range, "km", 0, 500);
    // defaultObj["frequency"] = toParm(frequency, "Ghz", 0.1, 100);
    // defaultObj["azimuth"] = toParm(azimuth, "deg", 0, 360);

    // AIS input fields
    //defaultObj["enabled"] = enabled;
    // defaultObj["tx_enabled"] = tx_enabled;
    // defaultObj["rx_enabled"] = rx_enabled;

    //defaultObj["mmsi"] = QString::number(static_cast<qulonglong>(mmsi));
    //defaultObj["name"] = QString::fromStdString(ais_name);
    //defaultObj["ais_class"] = QString::fromStdString(ais_class);
    defaultObj["channel_mode"] = QString::fromStdString(channel_mode);
    //defaultObj["rot_deg_per_min"] = toParm(rot_deg_per_min, "deg/min", -720.0, 720.0);

    // defaultObj["latitude_deg"] = toParm(latitude_deg, "deg", -90.0, 90.0);
    // defaultObj["longitude_deg"] = toParm(longitude_deg, "deg", -180.0, 180.0);
    // defaultObj["sog_kn"] = toParm(sog_kn, "kn", 0.0, 100.0);
    // defaultObj["cog_deg"] = toParm(cog_deg, "deg", 0.0, 360.0);
    // defaultObj["heading_deg"] = toParm(heading_deg, "deg", 0.0, 360.0);

    // defaultObj["dynamic_interval_s"] = toParm(dynamic_interval_s, "s", 0.1, 600.0);
    // defaultObj["static_interval_s"] = toParm(static_interval_s, "s", 0.1, 3600.0);
    // defaultObj["track_stale_timeout_s"] = toParm(track_stale_timeout_s, "s", 0.0, 3600.0);

    //defaultObj["rf_id"] = QString::fromStdString(rf_id);
    //defaultObj["parent_name"] = QString::fromStdString(parent_name);
    //defaultObj["rf_mode"] = QString::fromStdString(rf_mode);

    defaultObj["enabled"] = cfg.enabled;
    defaultObj["tx_enabled"] = cfg.tx_enabled;
    defaultObj["rx_enabled"] = cfg.rx_enabled;

    defaultObj["mmsi"] = QString::number(static_cast<qulonglong>(cfg.static_data.mmsi));
    defaultObj["name"] = QString::fromStdString(cfg.static_data.name);

    defaultObj["ais_class"] =
        (cfg.ais_class == ais::AisClass::CLASS_A) ? "CLASS_A" : "CLASS_B";

    QString channelMode = "DUAL";
    if (cfg.channel_mode == ais::AisChannelMode::AIS1_ONLY) channelMode = "AIS1_ONLY";
    else if (cfg.channel_mode == ais::AisChannelMode::AIS2_ONLY) channelMode = "AIS2_ONLY";
    defaultObj["channel_mode"] = channelMode;

    //defaultObj["rot_deg_per_min"] = toParm(cfg.dynamic_data.rot_deg_per_min, "deg/min", -720.0, 720.0);

    //defaultObj["latitude_deg"] = toParm(cfg.dynamic_data.latitude_deg, "deg", -90.0, 90.0);
    //defaultObj["longitude_deg"] = toParm(cfg.dynamic_data.longitude_deg, "deg", -180.0, 180.0);
    //defaultObj["sog_kn"] = toParm(cfg.dynamic_data.sog_kn, "kn", 0.0, 100.0);
    //defaultObj["cog_deg"] = toParm(cfg.dynamic_data.cog_deg, "deg", 0.0, 360.0);
    //defaultObj["heading_deg"] = toParm(cfg.dynamic_data.heading_deg, "deg", 0.0, 360.0);

    defaultObj["dynamic_interval_s"] = toParm(cfg.dynamic_interval_s, "s", 0.1, 600.0);
    defaultObj["static_interval_s"] = toParm(cfg.static_interval_s, "s", 0.1, 3600.0);
    defaultObj["track_stale_timeout_s"] = toParm(cfg.track_stale_timeout_s, "s", 0.0, 3600.0);

    //defaultObj["rf_id"] = QString::fromStdString(own_rf.id);
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

    // ownship.configureRf(own_rf);
    return obj;
}

void AISSensor::fromJson(const QJsonObject& obj) {
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
            qulonglong value = s.toULongLong(&ok, 10);
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

        if (defaultObj.contains("range"))
            range = readParmOrDouble(defaultObj["range"], range);

        if (defaultObj.contains("frequency"))
            frequency = readParmOrDouble(defaultObj["frequency"], frequency);

        if (defaultObj.contains("azimuth"))
            azimuth = readParmOrDouble(defaultObj["azimuth"], azimuth);

        if (defaultObj.contains("enabled"))
            enabled = defaultObj["enabled"].toBool();

        if (defaultObj.contains("tx_enabled"))
            tx_enabled = defaultObj["tx_enabled"].toBool();

        if (defaultObj.contains("rx_enabled"))
            rx_enabled = defaultObj["rx_enabled"].toBool();


        if (defaultObj.contains("mmsi"))
            mmsi = readUInt32(defaultObj["mmsi"], mmsi);

        // if (defaultObj.contains("rot_deg_per_min"))
        //     rot_deg_per_min = readParmOrDouble(defaultObj["rot_deg_per_min"], rot_deg_per_min);

        if (defaultObj.contains("name"))
            ais_name = defaultObj["name"].toString().toStdString();


        if (defaultObj.contains("ais_class"))
            ais_class = defaultObj["ais_class"].toString().toStdString();

        if (defaultObj.contains("channel_mode"))
            channel_mode = defaultObj["channel_mode"].toString().toStdString();

        if (defaultObj.contains("latitude_deg"))
            latitude_deg = readParmOrDouble(defaultObj["latitude_deg"], latitude_deg);

        if (defaultObj.contains("longitude_deg"))
            longitude_deg = readParmOrDouble(defaultObj["longitude_deg"], longitude_deg);

        if (defaultObj.contains("sog_kn"))
            sog_kn = readParmOrDouble(defaultObj["sog_kn"], sog_kn);

        if (defaultObj.contains("cog_deg"))
            cog_deg = readParmOrDouble(defaultObj["cog_deg"], cog_deg);

        if (defaultObj.contains("heading_deg"))
            heading_deg = readParmOrDouble(defaultObj["heading_deg"], heading_deg);

        if (defaultObj.contains("dynamic_interval_s"))
            dynamic_interval_s = readParmOrDouble(defaultObj["dynamic_interval_s"], dynamic_interval_s);

        if (defaultObj.contains("static_interval_s"))
            static_interval_s = readParmOrDouble(defaultObj["static_interval_s"], static_interval_s);

        if (defaultObj.contains("track_stale_timeout_s"))
            track_stale_timeout_s = readParmOrDouble(defaultObj["track_stale_timeout_s"], track_stale_timeout_s);

        // if (defaultObj.contains("rf_id"))
        //     rf_id = defaultObj["rf_id"].toString().toStdString();

        // if (defaultObj.contains("parent_name"))
        //     parent_name = defaultObj["parent_name"].toString().toStdString();

        if (defaultObj.contains("rf_mode"))
            rf_mode = defaultObj["rf_mode"].toString().toStdString();
    }

    if (obj.contains("Environmental") && obj["Environmental"].isObject()) {
        QJsonObject Env = obj["Environmental"].toObject();
        if (Env.contains("temperature_c"))
            own_rf.propagation.temperature_c = valueFromParm(Env["temperature_c"].toObject());
        if (Env.contains("pressure_hpa"))
            own_rf.propagation.pressure_hpa = valueFromParm(Env["pressure_hpa"].toObject());
        if (Env.contains("humidity_percent"))
            own_rf.propagation.humidity_percent = valueFromParm(Env["humidity_percent"].toObject());
        // if (Env.contains("gas_attenuation_db_per_km_at_1ghz"))
        // cfg.propagation.gas_attenuation_db_per_km_at_1ghz = valueFromParm(Env["gas_attenuation_db_per_km_at_1ghz"].toObject());
        //if (Env.contains("gas_attenuation_freq_exponent"))
        //    cfg.propagation.gas_attenuation_freq_exponent = valueFromParm(Env["gas_attenuation_freq_exponent"].toObject());
        //if (Env.contains("humidity_attenuation_factor_per_percent"))
        //  cfg.propagation.humidity_attenuation_factor_per_percent = valueFromParm(Env["humidity_attenuation_factor_per_percent"].toObject());
        if (Env.contains("rain_rate_mm_per_hr"))
            own_rf.propagation.rain_rate_mm_per_hr = valueFromParm(Env["rain_rate_mm_per_hr"].toObject());
        //if (Env.contains("rain_attenuation_db_per_km_per_mmhr"))
        //  cfg.propagation.rain_attenuation_db_per_km_per_mmhr = valueFromParm(Env["rain_attenuation_db_per_km_per_mmhr"].toObject());
        if (Env.contains("rain_coverage"))
            own_rf.propagation.rain_coverage = valueFromParm(Env["rain_coverage"].toObject());
        //if (Env.contains("rain_rate_sigma_frac"))
        //  cfg.propagation.rain_rate_sigma_frac = valueFromParm(Env["rain_rate_sigma_frac"].toObject());
        if (Env.contains("wind_speed_mps"))
            own_rf.propagation.wind_speed_mps = valueFromParm(Env["wind_speed_mps"].toObject());
        //if (Env.contains("wind_attenuation_db_per_km_per_mps"))
        //  cfg.propagation.wind_attenuation_db_per_km_per_mps = valueFromParm(Env["wind_attenuation_db_per_km_per_mps"].toObject());
        //if (Env.contains("sea_attenuation_db_per_km"))
        //  cfg.propagation.sea_attenuation_db_per_km = valueFromParm(Env["sea_attenuation_db_per_km"].toObject());


    }
    ais::AisConfig cfg = ownship.getAisConfiguration();

    cfg.enabled = enabled;
    cfg.tx_enabled = tx_enabled;
    cfg.rx_enabled = rx_enabled;

    cfg.static_data.mmsi = mmsi;
    cfg.static_data.name = ais_name;

    if (ais_class == "CLASS_B") {
        cfg.ais_class = ais::AisClass::CLASS_B;
    } else {
        cfg.ais_class = ais::AisClass::CLASS_A;
    }

    if (channel_mode == "AIS1_ONLY") {
        cfg.channel_mode = ais::AisChannelMode::AIS1_ONLY;
    } else if (channel_mode == "AIS2_ONLY") {
        cfg.channel_mode = ais::AisChannelMode::AIS2_ONLY;
    } else {
        cfg.channel_mode = ais::AisChannelMode::DUAL;
    }

    // cfg.dynamic_data.latitude_deg = latitude_deg;
    // cfg.dynamic_data.longitude_deg = longitude_deg;
    // cfg.dynamic_data.sog_kn = sog_kn;
    // cfg.dynamic_data.cog_deg = cog_deg;
    // cfg.dynamic_data.heading_deg = heading_deg;
    // cfg.dynamic_data.rot_deg_per_min = rot_deg_per_min;

    cfg.dynamic_interval_s = dynamic_interval_s;
    cfg.static_interval_s = static_interval_s;
    cfg.track_stale_timeout_s = track_stale_timeout_s;

    ownship.configureAis(cfg);

    ownship.configureRf(own_rf);

    own_rf.id = rf_id.empty() ? ID : rf_id;
    own_rf.parent_name = parent_name;
    own_rf.use_local_propagation_config = true;

    if (rf_mode == "TRANSMITTER_ONLY") {
        own_rf.mode = rf::RfMode::TRANSMITTER_ONLY;
    } else if (rf_mode == "RECEIVER_ONLY") {
        own_rf.mode = rf::RfMode::RECEIVER_ONLY;
    } else {
        own_rf.mode = rf::RfMode::TRANSCEIVER;
    }

    ownship.configureRf(own_rf);

    if (rf_id.empty()) {
        rf_id = ID;
    }

}
