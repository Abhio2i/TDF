#include "radio.h"
#include <core/Hierarchy/hierarchy.h> // Include full Hierarchy definition
#include <core/Debug/console.h>
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/propagation_model_config.h"
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_interface.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Simulation/simulation.h"
#include <core/GlobalRegistry.h>
#include <cmath>
#include <QtMath>
#include <QDebug>
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif



const float RAD2DEG = 180.0f / M_PI;
radio::PropagationModel* Radio::model = nullptr;
std::vector<std::byte> bytesFromText(const std::string& text) {
    std::vector<std::byte> data;
    data.reserve(text.size());
    for (char ch : text) {
        data.push_back(static_cast<std::byte>(ch));
    }
    return data;
}

std::string textFromBytes(const std::vector<std::byte>& data) {
    std::string text;
    text.reserve(data.size());
    for (std::byte b : data) {
        text.push_back(static_cast<char>(b));
    }
    return text;
}

Radio::Radio(Hierarchy* h) : Entity(h) {
    type = Constants::EntityType::Radio;
    // Initialize default parameter (similar to Platform and Sensor)
    std::shared_ptr<Parameter> par = std::make_shared<Parameter>();
    par->Name = "radio_param";
    par->type = Constants::ParameterType::FLOAT;
    par->value = 0.0f;
    parameters["radio_param"] = par;

    model_cfg.enable_fspl = true;
    model_cfg.enable_log_distance = false;
    model_cfg.enable_two_ray = false;
    model_cfg.enable_shadowing = false;
    model_cfg.shadowing_sigma_db = 1.5;
    model_cfg.enable_fading =false;
    model_cfg.fading_sigma_db = 1.0;
    model_cfg.enable_polarization_loss = true;
    model_cfg.polarization_mismatch_loss_db = 3.0;
    model_cfg.enable_los_horizon = true;
    model_cfg.enable_comms_mode_losses = true;
    model_cfg.enable_noise_floor = true;
    model_cfg.enable_snr_threshold = true;
    model_cfg.enable_sensitivity = true;
    model_cfg.enable_squelch = true;
    model_cfg.enable_interference = false;
    model_cfg.enable_range_limit = false;
    model_cfg.enable_network_gate_in_scan = true;
    model_cfg.enable_scan_beam = true;
    model_cfg.enable_scan_timing = false;
    model_cfg.enable_doppler = true;

    // Environmental toggles + values
    model_cfg.enable_environmental_attenuation = true;
    model_cfg.temperature_c = 20.0;
    model_cfg.pressure_hpa = 1005.0;
    model_cfg.humidity_percent = 60.0;
    model_cfg.gas_attenuation_db_per_km_at_1ghz = 0.005;
    model_cfg.gas_attenuation_freq_exponent = 1.0;
    model_cfg.humidity_attenuation_factor_per_percent = 0.002;
    model_cfg.rain_rate_mm_per_hr = 2.5;
    model_cfg.rain_attenuation_db_per_km_per_mmhr = 0.004;
    model_cfg.use_itu_rain_model = true;
    model_cfg.rain_coverage = 0.4;
    model_cfg.rain_rate_sigma_frac = 0.15;
    model_cfg.wind_speed_mps = 25.0;
    model_cfg.wind_attenuation_db_per_km_per_mps = 0.0005;
    model_cfg.enable_sea_attenuation = true;
    model_cfg.sea_attenuation_db_per_km = 0.003;

    if(model == nullptr){
       model = radio::createPropagationModelRaw(model_cfg);
    }

        lib_radio = radio::createRadiolibRaw();

        radio::RadioConfig cfg;
        cfg.id = "";
        cfg.parent_platform_name = "PLATFORM_1";
        cfg.mode = radio::RadioMode::TRANSCEIVER;
        cfg.use_local_propagation_config = true;
        cfg.propagation.enable_fspl = true;
        cfg.propagation.enable_log_distance = false;
        cfg.propagation.enable_two_ray = false;
        cfg.propagation.enable_shadowing = false;
        cfg.propagation.shadowing_sigma_db = 1.5;
        cfg.propagation.enable_fading = false;
        cfg.propagation.fading_sigma_db = 1.0;
        cfg.propagation.enable_polarization_loss = true;
        cfg.propagation.polarization_mismatch_loss_db = 3.0;
        cfg.propagation.enable_los_horizon = true;
        cfg.propagation.enable_comms_mode_losses = true;
        cfg.propagation.enable_noise_floor = true;
        cfg.propagation.enable_snr_threshold = true;
        cfg.propagation.enable_sensitivity = true;
        cfg.propagation.enable_squelch = true;
        cfg.propagation.enable_interference = false;
        cfg.propagation.enable_range_limit = false;
        cfg.propagation.enable_network_gate_in_scan = true;
        cfg.propagation.enable_scan_beam = true;
        cfg.propagation.enable_scan_timing = false;
        cfg.propagation.enable_doppler = true;

        // Environmental toggles + values
        cfg.propagation.enable_environmental_attenuation = true;
        cfg.propagation.temperature_c = 20.0;
        cfg.propagation.pressure_hpa = 1005.0;
        cfg.propagation.humidity_percent = 60.0;
        cfg.propagation.gas_attenuation_db_per_km_at_1ghz = 0.005;
        cfg.propagation.gas_attenuation_freq_exponent = 1.0;
        cfg.propagation.humidity_attenuation_factor_per_percent = 0.002;
        cfg.propagation.rain_rate_mm_per_hr = 2.5;
        cfg.propagation.rain_attenuation_db_per_km_per_mmhr = 0.004;
        cfg.propagation.use_itu_rain_model = true;
        cfg.propagation.rain_coverage = 0.4;
        cfg.propagation.rain_rate_sigma_frac = 0.15;
        cfg.propagation.wind_speed_mps = 25.0;
        cfg.propagation.wind_attenuation_db_per_km_per_mps = 0.0005;
        cfg.propagation.enable_sea_attenuation = true;
        cfg.propagation.sea_attenuation_db_per_km = 0.003;
        cfg.comms_mode = radio::CommsMode::LINE_OF_SIGHT;
        cfg.min_freq_hz = 118.5e6;//
        cfg.max_freq_hz = 138.5e6;//
        cfg.frequency_hz = 128.5e6;//
        cfg.bandwidth_hz = 25e3;//
        cfg.tx_power_dbm = 40.0;//m
        cfg.antenna.gain_dbi = 2.0;//m
        cfg.receiver.sensitivity_dbm = -101.0;//m
        cfg.receiver.noise_figure_db = 5.0;//m
        cfg.receiver.squelch_threshold_db = 3.0;//m
        cfg.network_id = 1;

        radio::Position pos;
        pos.x = 0.0;
        pos.y = 0.0;
        pos.altitude = 1000.0;

        radio::attachRadioToModel(lib_radio, model, cfg, pos);
        lib_radio->setPowerOn(true);

        // std::cout << "Radiolib entity created: "
        //           << lib_radio->getConfiguration().id << "\n";

        // radio::destroyRadiolib(lib_radio);
        // radio::destroyPropagationModel(model);

        // Identity
        cfg.is_naval = false;//m
        cfg.rx_bandwidth_hz = 25e3;//m
        cfg.channel = 1;//m
      //  cfg.max_range_m = 100000;//

        // TX / waveform
        //cfg.tx_power_dbm = tx_power_dbm;
        cfg.power_degradation_db = 1.0;//
        cfg.tx_duty_cycle = 0.90;//m
        cfg.modulation_scheme = radio::ModulationScheme::BPSK;
        cfg.required_snr_override = false;//m
        cfg.spread_spectrum = radio::SpreadSpectrum::NONE;
        cfg.processing_gain_db = 0.0;//m

        // Encryption
        cfg.encryption_type = radio::EncryptionType::AES;
        cfg.encryption_key = bytesFromText("0123456789ABCDEF");
        cfg.encryption_iv = bytesFromText("ABCDEF0123456789");

        // Antenna / motion
        cfg.antenna.beamwidth_deg = 360.0;//m
        cfg.antenna.polarization = radio::Polarization::VERTICAL;
        cfg.antenna.scan_type = radio::ScanType::FIXED;
        cfg.heading_deg = 360;//m
        cfg.velocity_mps = 1;//m
        lib_radio->setReceiveCallbackWithMeta(
            [this](const std::vector<std::byte>& data, const radio::ReceiveReport& report) {
                msgTimeStamp = Simulation::simulationTime;
                msg = textFromBytes(data);

                qDebug().noquote()
                    << QString(
                           "\n[RX_1 callback]\n"
                           " sender: %1\n"
                           " msg: %2\n"
                           " rx_power_dbm: %3\n"
                           " noise_floor_dbm: %4\n"
                           " snr_db: %5\n"
                           " path_loss_db: %6\n"
                           " rain_attenuation_db: %7\n"
                           " wind_attenuation_db_per_km: %8\n"
                           " frequency_match: %9\n"
                           " los_horizon_distance_m: %10\n"
                           " polarization_loss_db: %11\n"
                           " range_ok: %12\n"
                           " required_snr_threshold_db: %13\n"
                           " sensitivity_ok: %14\n"
                           " squelch_ok: %15")
                           .arg(QString::fromStdString(report.sender_id))
                           .arg(QString::fromStdString(textFromBytes(data)))
                           .arg(report.rx_power_dbm)
                           .arg(report.noise_floor_dbm)
                           .arg(report.snr_db)
                           .arg(report.path_loss_db)
                           .arg(report.rain_attenuation_db)
                           .arg(report.wind_attenuation_db_per_km)
                           .arg(report.frequency_match ? "true" : "false")
                           .arg(report.los_horizon_distance_m)
                           .arg(report.polarization_loss_db)
                           .arg(report.range_ok ? "true" : "false")
                           .arg(report.required_snr_threshold_db)
                           .arg(report.sensitivity_ok ? "true" : "false")
                           .arg(report.squelch_ok ? "true" : "false");
            });

}


void Radio::sendMsg(std::string msg){
    lib_radio->transmit(bytesFromText(msg));
}


void Radio::spawn() {
    Hierarchy* parent = GlobalRegistry::getParentHierarchy(this);
    emit parent->entityAdded(QString::fromStdString(parentID), QString::fromStdString(ID), QString::fromStdString(Name));
}

std::vector<std::string>Radio:: getSupportedComponents(){
    return std::vector<std::string>{};
}

void Radio::addComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
}

void Radio::removeComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
}

QJsonObject Radio::getComponent(std::string name) {
    Console::error("Radio does not support components: " + name);
    return QJsonObject();
}

void Radio::updateComponent(QString name, const QJsonObject& /*obj*/) {
    Console::error(name.toStdString() + ": Radio does not support components");
}

QJsonObject Radio::toJson() const {
    radio::RadioConfig cfg = lib_radio->getConfiguration();
    if(parentEntity)cfg.id = parentEntity->ID;
    QJsonObject obj;
    obj["name"] = QString::fromStdString(Name);
    obj["branch"] = QString::fromStdString("Entity");
    obj["id"] = QString::fromStdString(ID);
    obj["parent_id"] = QString::fromStdString(parentID);
    obj["active"] = Active;

    // Serialize parameters
    QJsonObject paramMap;
    for (const auto& [key, param] : parameters) {
        if (param) {
            paramMap[QString::fromStdString(key)] = param->toJson();
        }
    }
    QJsonObject parObj;
    parObj["type"] = "parameter";
    parObj["value"] = paramMap;
    obj["parameters"] = parObj;

    QJsonObject RadioTypeObj;
    RadioTypeObj["type"] = "option";
    QJsonArray RadioTypeptionsArray;
    for (const std::string& opt : RadioTypeNames)
        RadioTypeptionsArray.append(QString::fromStdString(opt));
    RadioTypeObj["options"] = RadioTypeptionsArray;
    RadioTypeObj["value"] = QString::fromStdString(RadioTypeNames[static_cast<int>(cfg.mode)]);
    obj["RadioType"] = RadioTypeObj;

    QJsonObject comms_modeObj;
    comms_modeObj["type"] = "option";
    QJsonArray comms_modeptionsArray;
    for (const std::string& opt : CommsModeTypeNames)
        comms_modeptionsArray.append(QString::fromStdString(opt));
    comms_modeObj["options"] = comms_modeptionsArray;
    comms_modeObj["value"] = QString::fromStdString(CommsModeTypeNames[static_cast<int>(cfg.comms_mode)]);
    obj["comms_mode"] = comms_modeObj;

    QJsonObject modulation_schemeObj;
    modulation_schemeObj["type"] = "option";
    QJsonArray modulation_schemeptionsArray;
    for (const std::string& opt : ModulationSchemeTypeNames)
        modulation_schemeptionsArray.append(QString::fromStdString(opt));
    modulation_schemeObj["options"] = modulation_schemeptionsArray;
    modulation_schemeObj["value"] = QString::fromStdString(ModulationSchemeTypeNames[static_cast<int>(cfg.modulation_scheme)]);
    obj["modulation_scheme"] = modulation_schemeObj;

    // QJsonObject spread_spectrumObj;
    // spread_spectrumObj["type"] = "option";
    // QJsonArray spread_spectrumptionsArray;
    // for (const std::string& opt : SpreadSpectrumTypeNames)
    //     spread_spectrumptionsArray.append(QString::fromStdString(opt));
    // spread_spectrumObj["options"] = spread_spectrumptionsArray;
    // spread_spectrumObj["value"] = QString::fromStdString(SpreadSpectrumTypeNames[static_cast<int>(cfg.spread_spectrum)]);
    // obj["spread_spectrum"] = spread_spectrumObj;

    QJsonObject polarizationObj;
    polarizationObj["type"] = "option";
    QJsonArray polarizationptionsArray;
    for (const std::string& opt : PolarizationTypeNames)
        polarizationptionsArray.append(QString::fromStdString(opt));
    polarizationObj["options"] = polarizationptionsArray;
    polarizationObj["value"] = QString::fromStdString(PolarizationTypeNames[static_cast<int>(cfg.antenna.polarization)]);
    obj["polarization"] = polarizationObj;

    QJsonObject scan_typeObj;
    scan_typeObj["type"] = "option";
    QJsonArray scan_typeptionsArray;
    for (const std::string& opt : ScanTypeNames)
        scan_typeptionsArray.append(QString::fromStdString(opt));
    scan_typeObj["options"] = scan_typeptionsArray;
    scan_typeObj["value"] = QString::fromStdString(ScanTypeNames[static_cast<int>(cfg.antenna.scan_type)]);
    obj["scan_type"] = scan_typeObj;

    QJsonObject Transmitter;
    Transmitter["type"] = "Section";
    Transmitter["minFrequency"] = toParm(cfg.min_freq_hz/1000000,"Mhz", 0,    30000);
    Transmitter["maxFrequency"] = toParm(cfg.max_freq_hz/1000000,"Mhz", 0,    30000);
    Transmitter["Frequency"] = toParm(cfg.frequency_hz/1000000,"Mhz", 0,    30000);
    // Transmitter["Range"] = toParm(cfg.max_range_m/1000,"km");
    Transmitter["powerDegradation"] = toParm(cfg.power_degradation_db,"db");
    Transmitter["tx_power_dbm"] = toParm(cfg.tx_power_dbm,"dbm");
    // Transmitter["tx_duty_cycle"] = toParm(cfg.tx_duty_cycle,"");
    Transmitter["is_naval"] = cfg.is_naval;
    obj["Transmitter"] = Transmitter;

    QJsonObject Receiver;
    Receiver["type"] = "Section";
    Receiver["rx_bandwidth_hz"] = toParm(cfg.rx_bandwidth_hz/1000.0f,"Khz", 0, 30000);
    Receiver["sensitivity_dbm"] = toParm(cfg.receiver.sensitivity_dbm,"");
    Receiver["noise_figure_db"] = toParm(cfg.receiver.noise_figure_db,"");
    Receiver["squelch_threshold_db"] = toParm(cfg.receiver.squelch_threshold_db,"");
    Receiver["channel"] = toParm(cfg.channel,"");
    obj["Receiver"] = Receiver;

    // QJsonObject Modulation;
    // Modulation["type"] = "Section";
    // // Modulation["spread_spectrum"] = toParm(spreadSpecturm,"");//option
    // Modulation["required_snr_override"] = cfg.required_snr_override;
    // Modulation["processing_gain_db"] = toParm(cfg.processing_gain_db,"db");
    // obj["Modulation"] = Modulation;

    // QJsonObject Pulse;
    // Pulse["type"] = "Section";
    // Pulse["pulseWidth"] = toParm(cfg.bandwidth_hz/1000.0f,"khz",0,50);
    // obj["Pulse"] = Pulse;

    QJsonObject Antenna;
    Antenna["type"] = "Section";
    Antenna["gain_dbi"] = toParm(cfg.antenna.gain_dbi,"dbi");
    Antenna["beamwidth_deg"] = toParm(cfg.antenna.beamwidth_deg,"deg");
    Antenna["heading_deg"] = toParm(cfg.heading_deg,"deg");
    // Antenna["velocity_mps"] = toParm(cfg.velocity_mps,"mps");
    obj["Antenna"] = Antenna;

    QJsonObject Env;
    Env["type"] = "Section";
    Env["temperature_c"] = toParm(cfg.propagation.temperature_c,"cel");
    Env["pressure_hpa"] = toParm(cfg.propagation.pressure_hpa,"hpa");
    Env["humidity_percent"] = toParm(cfg.propagation.humidity_percent,"%");
    //Env["gas_attenuation_db_per_km_at_1ghz"] = toParm(cfg.propagation.gas_attenuation_db_per_km_at_1ghz,"db/km");
   // Env["gas_attenuation_freq_exponent"] = toParm(cfg.propagation.gas_attenuation_freq_exponent,"");
   // Env["humidity_attenuation_factor_per_percent"] = toParm(cfg.propagation.humidity_attenuation_factor_per_percent,"%");
    Env["rain_rate_mm_per_hr"] = toParm(cfg.propagation.rain_rate_mm_per_hr,"mm/h");
  //  Env["rain_attenuation_db_per_km_per_mmhr"] = toParm(cfg.propagation.rain_attenuation_db_per_km_per_mmhr,"db/km/h");
    Env["rain_coverage"] = toParm(cfg.propagation.rain_coverage,"%");
   // Env["rain_rate_sigma_frac"] = toParm(cfg.propagation.rain_rate_sigma_frac,"mm/h");
    Env["wind_speed_mps"] = toParm(cfg.propagation.wind_speed_mps,"m/s");
   // Env["wind_attenuation_db_per_km_per_mps"] = toParm(cfg.propagation.wind_attenuation_db_per_km_per_mps,"db/km");
   // Env["sea_attenuation_db_per_km"] = toParm(cfg.propagation.sea_attenuation_db_per_km,"db/km");
    obj["Environmental"] = Env;


    return obj;
}

void Radio::fromJson(const QJsonObject& obj) {
    radio::RadioConfig cfg = lib_radio->getConfiguration();
    if(parentEntity)cfg.id = parentEntity->ID;
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("name"))
        Name = obj["name"].toString().toStdString();
    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();
    if (obj.contains("parent_id"))
        parentID = obj["parent_id"].toString().toStdString();

    if (obj.contains("RadioType") && obj["RadioType"].isObject()) {
        QJsonObject RadioTypeObj = obj["RadioType"].toObject();
        if (RadioTypeObj.contains("value")){
            for (int i = 0; i < 3; i++) {
                if (RadioTypeNames[i] == RadioTypeObj["value"].toString().toStdString()) {
                    cfg.mode = (radio::RadioMode)i;
                }
            }
        }
    }

    if (obj.contains("comms_mode") && obj["comms_mode"].isObject()) {
        QJsonObject comms_modeObj = obj["comms_mode"].toObject();
        if (comms_modeObj.contains("value")){
            for (int i = 0; i < 4; i++) {
                if (CommsModeTypeNames[i] == comms_modeObj["value"].toString().toStdString()) {
                    cfg.comms_mode = (radio::CommsMode)i;
                }
            }
        }
    }

    if (obj.contains("modulation_scheme") && obj["modulation_scheme"].isObject()) {
        QJsonObject modulation_schemeObj = obj["modulation_scheme"].toObject();
        if (modulation_schemeObj.contains("value")){
            for (int i = 0; i < 10; i++) {
                if (ModulationSchemeTypeNames[i] == modulation_schemeObj["value"].toString().toStdString()) {
                    cfg.modulation_scheme = (radio::ModulationScheme)i;
                }
            }
        }
    }

    // if (obj.contains("spread_spectrum") && obj["spread_spectrum"].isObject()) {
    //     QJsonObject spread_spectrumObj = obj["spread_spectrum"].toObject();
    //     if (spread_spectrumObj.contains("value")){
    //         for (int i = 0; i < 3; i++) {
    //             if (SpreadSpectrumTypeNames[i] == spread_spectrumObj["value"].toString().toStdString()) {
    //                 cfg.spread_spectrum = (radio::SpreadSpectrum)i;
    //             }
    //         }
    //     }
    // }

    if (obj.contains("polarization") && obj["polarization"].isObject()) {
        QJsonObject polarizationObj = obj["polarization"].toObject();
        if (polarizationObj.contains("value")){
            for (int i = 0; i < 4; i++) {
                if (PolarizationTypeNames[i] == polarizationObj["value"].toString().toStdString()) {
                    cfg.antenna.polarization = (radio::Polarization)i;
                }
            }
        }
    }

    if (obj.contains("scan_type") && obj["scan_type"].isObject()) {
        QJsonObject scan_typeObj = obj["scan_type"].toObject();
        if (scan_typeObj.contains("value")){
            for (int i = 0; i < 3; i++) {
                if (ScanTypeNames[i] == scan_typeObj["value"].toString().toStdString()) {
                    cfg.antenna.scan_type = (radio::ScanType)i;
                }
            }
        }
    }

    if (obj.contains("Transmitter") && obj["Transmitter"].isObject()) {
        QJsonObject Transmitter = obj["Transmitter"].toObject();
        if (Transmitter.contains("minFrequency"))
            cfg.min_freq_hz = valueFromParm(Transmitter["minFrequency"].toObject())*1000000.0f;
        if (Transmitter.contains("maxFrequency"))
            cfg.max_freq_hz = valueFromParm(Transmitter["maxFrequency"].toObject())*1000000.0f;
        if (Transmitter.contains("Frequency"))
            cfg.frequency_hz = valueFromParm(Transmitter["Frequency"].toObject())*1000000.0f;
        // if (Transmitter.contains("Range")){
        //     cfg.max_range_m = valueFromParm(Transmitter["Range"].toObject())*1000.0f;
        //     Range = cfg.max_range_m/1000;
        // }
        if (Transmitter.contains("powerDegradation"))
            cfg.power_degradation_db = valueFromParm(Transmitter["powerDegradation"].toObject());
        if (Transmitter.contains("tx_power_dbm"))
            cfg.tx_power_dbm = valueFromParm(Transmitter["tx_power_dbm"].toObject());
        // if (Transmitter.contains("tx_duty_cycle"))
        //     cfg.tx_duty_cycle = valueFromParm(Transmitter["tx_duty_cycle"].toObject());
        if (Transmitter.contains("is_naval"))
            cfg.is_naval = Transmitter["is_naval"].toBool();
    }

    if (obj.contains("Receiver") && obj["Receiver"].isObject()) {
        QJsonObject Receiver = obj["Receiver"].toObject();
        if (Receiver.contains("rx_bandwidth_hz"))
            cfg.rx_bandwidth_hz = valueFromParm(Receiver["rx_bandwidth_hz"].toObject())*1000.0f;
        if (Receiver.contains("sensitivity_dbm"))
            cfg.receiver.sensitivity_dbm = valueFromParm(Receiver["sensitivity_dbm"].toObject());
        if (Receiver.contains("noise_figure_db"))
            cfg.receiver.noise_figure_db = valueFromParm(Receiver["noise_figure_db"].toObject());
        if (Receiver.contains("squelch_threshold_db"))
            cfg.receiver.squelch_threshold_db = valueFromParm(Receiver["squelch_threshold_db"].toObject());
        if (Receiver.contains("channel"))
            cfg.channel = valueFromParm(Receiver["channel"].toObject());
    }

    // if (obj.contains("Modulation") && obj["Modulation"].isObject()) {
    //     QJsonObject Modulation = obj["Modulation"].toObject();
    //     if (Modulation.contains("required_snr_override"))
    //         cfg.required_snr_override = Modulation["required_snr_override"].toBool();
    //     if (Modulation.contains("processing_gain_db"))
    //         cfg.processing_gain_db = valueFromParm(Modulation["processing_gain_db"].toObject());
    // }

    // if (obj.contains("Pulse") && obj["Pulse"].isObject()) {
    //     QJsonObject Pulse = obj["Pulse"].toObject();
    //     if (Pulse.contains("pulseWidth"))
    //         cfg.bandwidth_hz = valueFromParm(Pulse["pulseWidth"].toObject())*1000.0f;
    // }

    if (obj.contains("Antenna") && obj["Antenna"].isObject()) {
        QJsonObject Antenna = obj["Antenna"].toObject();
        if (Antenna.contains("gain_dbi"))
            cfg.antenna.gain_dbi = valueFromParm(Antenna["gain_dbi"].toObject());
        if (Antenna.contains("beamwidth_deg"))
            cfg.antenna.beamwidth_deg = valueFromParm(Antenna["beamwidth_deg"].toObject());
        if (Antenna.contains("heading_deg"))
            cfg.heading_deg = valueFromParm(Antenna["heading_deg"].toObject());
        // if (Antenna.contains("velocity_mps"))
        //     cfg.velocity_mps = valueFromParm(Antenna["velocity_mps"].toObject());

    }

    if (obj.contains("Environmental") && obj["Environmental"].isObject()) {
        QJsonObject Env = obj["Environmental"].toObject();
        if (Env.contains("temperature_c"))
            cfg.propagation.temperature_c = valueFromParm(Env["temperature_c"].toObject());
        if (Env.contains("pressure_hpa"))
            cfg.propagation.pressure_hpa = valueFromParm(Env["pressure_hpa"].toObject());
        if (Env.contains("humidity_percent"))
            cfg.propagation.humidity_percent = valueFromParm(Env["humidity_percent"].toObject());
       // if (Env.contains("gas_attenuation_db_per_km_at_1ghz"))
           // cfg.propagation.gas_attenuation_db_per_km_at_1ghz = valueFromParm(Env["gas_attenuation_db_per_km_at_1ghz"].toObject());
        //if (Env.contains("gas_attenuation_freq_exponent"))
        //    cfg.propagation.gas_attenuation_freq_exponent = valueFromParm(Env["gas_attenuation_freq_exponent"].toObject());
        //if (Env.contains("humidity_attenuation_factor_per_percent"))
          //  cfg.propagation.humidity_attenuation_factor_per_percent = valueFromParm(Env["humidity_attenuation_factor_per_percent"].toObject());
        if (Env.contains("rain_rate_mm_per_hr"))
            cfg.propagation.rain_rate_mm_per_hr = valueFromParm(Env["rain_rate_mm_per_hr"].toObject());
        //if (Env.contains("rain_attenuation_db_per_km_per_mmhr"))
          //  cfg.propagation.rain_attenuation_db_per_km_per_mmhr = valueFromParm(Env["rain_attenuation_db_per_km_per_mmhr"].toObject());
        if (Env.contains("rain_coverage"))
            cfg.propagation.rain_coverage = valueFromParm(Env["rain_coverage"].toObject());
        //if (Env.contains("rain_rate_sigma_frac"))
          //  cfg.propagation.rain_rate_sigma_frac = valueFromParm(Env["rain_rate_sigma_frac"].toObject());
        if (Env.contains("wind_speed_mps"))
            cfg.propagation.wind_speed_mps = valueFromParm(Env["wind_speed_mps"].toObject());
        //if (Env.contains("wind_attenuation_db_per_km_per_mps"))
          //  cfg.propagation.wind_attenuation_db_per_km_per_mps = valueFromParm(Env["wind_attenuation_db_per_km_per_mps"].toObject());
        //if (Env.contains("sea_attenuation_db_per_km"))
          //  cfg.propagation.sea_attenuation_db_per_km = valueFromParm(Env["sea_attenuation_db_per_km"].toObject());


    }

    // Deserialize parameters
    if (obj.contains("parameters")) {
        QJsonObject parObj = obj["parameters"].toObject();
        if (parObj.contains("value")) {
            QJsonObject paramMap = parObj["value"].toObject();
            for (const QString& key : paramMap.keys()) {
                QJsonObject paramObj = paramMap[key].toObject();
                std::shared_ptr<Parameter> param = std::make_shared<Parameter>();
                param->fromJson(paramObj);
                parameters[key.toStdString()] = param;
            }
        }
    }
    lib_radio->configure(cfg);
}



void Radio::scan(){
    if((Simulation::simulationTime-msgTimeStamp) >= 2.50f){
        msg = "";
    }

    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    float head = 0.f;
    if(!source) return;
    radio::RadioConfig cfg = lib_radio->getConfiguration();
    if(cfg.id == ""||true){
        cfg.id = parentEntity->ID;
        lib_radio->setPowerOn(parentEntity->Active);
        cfg.heading_deg = source->getHeading();
        head = cfg.heading_deg;

        lib_radio->configure(cfg);
    }
    radio::Position pos;
    pos.x = source->translation().z()*1000.f;
    pos.y = source->translation().x()*1000.f;
    pos.altitude = source->translation().y()*1000.f;
    model->updateRadioPosition(lib_radio, pos);

    std::vector<radio::ScanHit> hits = lib_radio->radiolibscan();

    // std::cout << "Scan hit count: " << hits.size() << "\n";
    targets.clear();
    for (std::size_t i = 0; i < hits.size(); ++i) {
        const radio::ScanHit& hit = hits[i];
        RadioTarget target;
        try {
            if ((root->Platforms)[hit.id]) {
                target.entity = (root->Platforms)[hit.id];
            }else{
                target.entity = nullptr;
            }
        } catch (const std::exception& e) {
            // Yahan error handle karein
            qDebug() << "Error accessing platform:" << e.what();
        }
        target.angle = -((hit.azimuth_deg+head)+180.f);
        target.radius = hit.distance_m/1000.f;
        targets.append(target);

        // std::cout << "Hit[" << i << "] "
        //           << "id=" << hit.id
        //           << " platform=" << hit.target_platform_name
        //           << " distance_m=" << hit.distance_m
        //           << " azimuth_deg=" << hit.azimuth_deg
        //           << " rx_power_dbm=" << hit.rx_power_dbm
        //           << " noise_floor_dbm=" << hit.noise_floor_dbm
        //           << " snr_db=" << hit.snr_db
        //           << " path_loss_db=" << hit.path_loss_db
        //           << "\n";
    }




}
int Radio::getRadioTargetCount() const
{
    return targets.size();
}

bool Radio::getRadioTarget(
    int index,
    std::string& outName,
    float& outRadius,
    float& outAngle,
    float& outRange,
    float& outFrequency
    ) const
{
    if (index < 0 || index >= targets.size())
        return false;

    const RadioTarget& t = targets[index];

    outName      = t.name;
    outRadius    = t.radius;
    outAngle     = t.angle;
    outRange     = t.range;
    outFrequency = t.frequency;

    return true;
}

