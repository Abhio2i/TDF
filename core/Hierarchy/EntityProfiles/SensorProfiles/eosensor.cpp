#include "eosensor.h"
#include <QFileInfo>
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"
#include "EOIRSensorLib/IRSensorLib/irsensorlib.h"
#include <cmath>
#include <algorithm>
#include <QDateTime>
#include <random>
#include <iomanip>
#include <QtMath>
// Constants for WGS84 ellipsoid
constexpr double WGS84_A = 6378137.0; // Semi-major axis in meters
constexpr double WGS84_E2 = 0.00669437999014; // First eccentricity squared
constexpr double PI = 3.14159265358979323846;
constexpr float FEET_TO_METERS = 0.3048f;
// Precompute constants at compile-time using constexpr and 'f' suffix for floats
constexpr float PI_F           = 3.141592653589793f;



EOSensor::EOSensor(Hierarchy* h) : Sensor(h) {
    subType = SubType::EO;
    frequency = 10.0f;
    range     = 40.0f;   // ★ km — long-range detect
    azimuth   = 30.0f;    // ★ deg base HFOV (pod-style)
    eo_ir = new EO_IR(h);
    m_h = h;
    m_Platforms= root->Platforms;
    m_Specialzones = h->Specialzones;
    setEnvironmentToDefault();
}

Surrounding EOSensor::getSurrounding(
    float radius,
    float atmCoeff,
    float rainRate,
    float fog,
    float humidity)
{
    Surrounding srd;

    //Setting Atmospheric
    srd.k_atm = 0.00001;
    // 0.00001 → very clear air
    // 0.001   → hazy

    //Setting Rain
    float alpha = 0.0001;// This Value is Approx
    float beta  = 0.8;   // This Value is Approx
    srd.k_rain   = alpha * pow(rainRate, beta);

    //Setting Fog (via visibility)
    float visibility = radius*fog;
    srd.k_fog = 3.912 / visibility;

    //Setting Humidity
    float humidityCoeff = 0.0005; // This Value is Approx
    srd.k_humidity  = humidityCoeff * humidity;

    return srd;
}

float EOSensor::getVeticalAngle(std::pair<int, int> resolution)
{
    // 1. Extract width and height
    float width = static_cast<float>(resolution.first);
    float height = static_cast<float>(resolution.second);

    // Prevent division by zero if an empty resolution is passed
    if (width <= 0.0f || height <= 0.0f) {
        return 0.0f;
    }

    // 2. Convert horizontal angle (azimuth) from degrees to radians
    // C++ <cmath> functions require radians.
    float azimuthRad = azimuth * (M_PI / 180.0f);

    // 3. Calculate the virtual focal length in pixels
    // Using formula: tan(theta/2) = (width/2) / focal_length
    float halfWidth = width / 2.0f;
    float focalLength = halfWidth / std::tan(azimuthRad / 2.0f);

    // 4. Calculate the vertical angle in radians
    // Using formula: tan(vertical_theta/2) = (height/2) / focal_length
    float halfHeight = height / 2.0f;
    float verticalAngleRad = 2.0f * std::atan(halfHeight / focalLength);

    // 5. Convert the result back to degrees
    float verticalAngleDeg = verticalAngleRad * (180.0f / M_PI);

    return verticalAngleDeg;
}

void EOSensor::scan()
{
    if (!Active)
        return;

    // Long-range EO: zero/near-zero visibility breaks detection
    if (visibility_m < 1000.f)
        visibility_m = 25000.f;

    // Rate limit — frequency (Hz) ke hisaab se
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const float hz = std::max(frequency, 1.0f);
        const qint64 minIntervalMs = static_cast<qint64>(1000.0f / hz);
        if (m_lastScanMs > 0 && (now - m_lastScanMs) < minIntervalMs)
            return;
        m_lastScanMs = now;
    }

    if (!parentEntity || !root)
        return;

    const auto key = parentEntity->ID;

    auto sensor_platform = root->Platforms.find(key);
    if (sensor_platform == root->Platforms.end() || !sensor_platform->second)
        return;

    Platform* sensorPlatform = sensor_platform->second;
    Transform* source = &sensorPlatform->transform;
    Transform* sensor_tf = &sensorPlatform->transform;

    currentEOSensor = key;

    // Sensor pose (GL + model)
    sensorPoseGeo.latitude  = sensorPlatform->transform.getLatitude();
    sensorPoseGeo.longitude = sensorPlatform->transform.getLongitude();
    sensorPoseGeo.altitude  =
        sensorPlatform->transform.getAltitude() * FEET_TO_METERS;

    {
        QVector3D sensorEuler =
            sensorPlatform->transform.rotation().toEulerAngles();
        sensorPoseGeo.rx = sensorEuler.x();
        sensorPoseGeo.ry = -sensorEuler.y();
        sensorPoseGeo.rz = sensorEuler.z();
    }

    if (!eo_ir)
        return;

    // Surrounding / sensor block (side effects on eo_ir if needed)
    {
        auto eo_surrounding = eo_ir->eo_surrounding;
        if (m_Specialzones.find(key) != m_Specialzones.end()) {
            auto ssz = m_Specialzones.at(key);
            eo_surrounding = getSurrounding(
                ssz->collider.CollideRadius,
                1,
                ssz->rain,
                ssz->fog,
                ssz->humidity);
        } else {
            eo_surrounding = getSurrounding();
        }
        auto eo_sensor = eo_ir->eo_sensor;
        eo_sensor.fov      = azimuth;
        eo_sensor.maxRange = range;
    }

    const float sensorLatitude  = source->getLatitude();
    const float sensorLongitude = source->getLongitude();
    const float sensorAltitude  = source->getAltitude() * 0.3048f;
    const float sensorHeading   = source->getHeading();
    const float sensorPitch     = source->pitch();

    const Coordinate sensor_cood = {
        sensorLatitude, sensorLongitude, sensorAltitude
    };

    // FOV resolution angles (once per scan)
    eoirResolutionSize = resolutionToSize[eoirResolution];
    horizonatalAngle   = azimuth;
    distance           = range * 1000.f;
    verticalAngle      = getVeticalAngle(eoirResolutionSize);

    EOIR_Sensor sensor_para = {
        slantRange, mrtd, sunPhaseAngle, saturationLimit
    };
    EOIR_Environment environment_para = {
        relativeHumidity, absoluteHumidity, rainfallRate,
        snowfallEquivalent, ambientTemp, background_temperature,
        aerosolConcentration, baseExtinctionCoeff, ambientIlluminance,
        solarIrradiance
    };

    // Once per scan — not per target
    const float optics_area = ir.Optics_Area({
        .aperture_diameter = aperture_diameter
    });
    (void)optics_area; // keep if you re-enable full IR path below

    ewtargets.clear();

    for (const auto& [ID, platform] : root->Platforms)
    {
        if (key == ID)
            continue;
        if (!platform || platform->ID.empty())
            continue;

        Transform& tf = platform->transform;

        const Coordinate obj_cood = {
            tf.getLatitude(),
            tf.getLongitude(),
            tf.getAltitude() * FEET_TO_METERS
        };

        // --- 1) Cheap horizontal range cull ---
        const float quickDist =
            eo_ir->getHorizontalDistance(sensor_cood, obj_cood);
        if (quickDist > range * 1000.0f * 1.1f) {
            EODetectionCoord[ID] = {false, CartesianCoord{}};
            continue;
        }

        const float targetLatitude  = tf.getLatitude();
        const float targetLongitude = tf.getLongitude();
        const float targetAltitude  = tf.getAltitude() * 0.3048f;
        const Coordinate target_cood = {
            targetLatitude, targetLongitude, targetAltitude
        };
        const float targetHeading = tf.getHeading();

        const float targetHorizontalDistance =
            eo_ir->getHorizontalDistance(sensor_cood, target_cood);
        const float targetHorizontalTargetAngle =
            eo_ir->getHorizontalTargetAngle(
                sensor_cood, target_cood, sensorHeading);
        const float targetVerticalTargetAngle =
            eo_ir->getVerticalTargetAngle(
                sensorAltitude, targetAltitude,
                targetHorizontalDistance, sensorPitch);

        // --- 2) Gimbal-relative FOV (KEEP working signs) ---
        const float hForFov =
            targetHorizontalTargetAngle + gimbalAzimuthOffset;
        const float vForFov =
            targetVerticalTargetAngle - gimbalElevationOffset;
        const bool inFov = isUnderRangeNAngle(
            targetHorizontalDistance, vForFov, hForFov);

        // OpenGL local coords (nose-relative HTA)
        CartesianCoord targetCoord;
        {
            const float horizontalAngleRad =
                qDegreesToRadians(targetHorizontalTargetAngle);
            targetCoord.x =
                targetHorizontalDistance * std::sin(horizontalAngleRad);
            targetCoord.z =
                -targetHorizontalDistance * std::cos(horizontalAngleRad);
            targetCoord.y  = targetAltitude;
            targetCoord.rx = 0.0f;
            targetCoord.ry = -(targetHeading - sensorHeading);
            targetCoord.rz = 0.0f;
        }

        if (!inFov) {
            EODetectionCoord[ID] = {false, targetCoord};
            continue;
        }

        // --- 3) Slant range gate ---
        const float dist3d = eo_ir->distanceBtw(sensor_cood, obj_cood);
        if (dist3d > range * 1000.0f) {
            EODetectionCoord[ID] = {false, targetCoord};
            continue;
        }

        target_range = calculate3DDistance(obj_cood, sensor_cood);
        background_temperature_K =
            celsius_to_kelvin(background_temperature_deg);

        // Pd inputs (no QString / qDebug on hot path)
        EOIR_Target target_para{};
        target_para.surfaceTemp =
            platform->surfaceTemp;
        target_para.specularReflectivity =
            platform->specularReflectivity;

        const float result = calculateDetectionProbability(
            environment_para, target_para, sensor_para);

        float detectThresh = 0.50f;
        if (target_range < 5000.f)
            detectThresh = 0.70f;
        else if (target_range < 15000.f)
            detectThresh = 0.55f;
        else
            detectThresh = 0.40f;

        EODetectionCoord[ID] = {
            (result >= detectThresh),
            targetCoord
        };
    }
}


// Helper: Convert degrees to radians
double EOSensor::deg2rad(double degrees) {
    return degrees * PI / 180.0;
}
// Helper function to convert degrees to radians
double EOSensor::degToRad(double degrees) {
    return degrees * M_PI / 180.0;
}
// Helper: Geodetic (Lat, Lon, Alt) to ECEF (X, Y, Z)
void EOSensor::GeodeticToECEF(double lat, double lon, double alt, double &X, double &Y, double &Z) {
    double lat_rad = deg2rad(lat);
    double lon_rad = deg2rad(lon);

    double sin_lat = std::sin(lat_rad);
    double cos_lat = std::cos(lat_rad);
    double sin_lon = std::sin(lon_rad);
    double cos_lon = std::cos(lon_rad);

    double N = WGS84_A / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);

    X = (N + alt) * cos_lat * cos_lon;
    Y = (N + alt) * cos_lat * sin_lon;
    Z = (N * (1.0 - WGS84_E2) + alt) * sin_lat;
}

bool EOSensor::isUnderRangeNAngle(
    float t_distance, float t_verticalAngle,
    float t_horizonatalAngle)
{
    if(distance < t_distance ) return false;
    if(verticalAngle/2 < abs(t_verticalAngle)) return false;
    if(horizonatalAngle/2 < abs(t_horizonatalAngle)) return false;
    return true;
}
// Function to calculate 3D distance using floats
float EOSensor::calculate3DDistance(const Coordinate& point1, const Coordinate& point2) {
    // 1. Convert to radians
    float lat1Rad = toRadians(point1.latitude);
    float lat2Rad = toRadians(point2.latitude);
    float deltaLat = toRadians(point2.latitude - point1.latitude);
    float deltaLon = toRadians(point2.longitude - point1.longitude);

    // 2. Haversine formula (using float math)
    float a = std::sin(deltaLat / 2.0f) * std::sin(deltaLat / 2.0f) +
              std::cos(lat1Rad) * std::cos(lat2Rad) *
                  std::sin(deltaLon / 2.0f) * std::sin(deltaLon / 2.0f);

    float c = 2.0f * std::atan2(std::sqrt(a), std::sqrt(1.0f - a));

    float horizontalDistance = EARTH_RADIUS * c;

    // 3. Calculate vertical distance
    float verticalDistance = point2.altitude - point1.altitude;

    // 4. Pythagorean theorem for 3D distance
    // Using multiplication instead of std::pow for slight performance gain
    float totalDistance = std::sqrt((horizontalDistance * horizontalDistance) +
                                    (verticalDistance * verticalDistance));

    return totalDistance;
}

float EOSensor::Atmospheric_Transmission(Atmospheric_Mode mode, float target_range)
{
    float R = target_range/1000;
    float value = Atmospheric_Mode_Value[static_cast<int>(mode)];
    auto sigma_ext = [=](float lambda) { return value; }; // Mock extinction
    auto tau = [R, sigma_ext](float lambda) {
        return std::exp(-sigma_ext(lambda) * R);
    };

    return ir.calculate_tau_SWIR(tau, 100);
}


float EOSensor::getProbabilityOfDetection(float current_snr, float snr_thresh, float sigma_snr) const {
    // Prevent division by zero
    if (sigma_snr == 0.0f) return 0.0f;

    float numerator = current_snr - snr_thresh;
    float denominator = std::sqrt(2.0f) * sigma_snr;

    // std::erf is automatically overloaded for float in C++11 and later
    return 0.5f * (1.0f + std::erf(numerator / denominator));
}

float EOSensor::getProbabilityOfFalseAlarm(float snr_thresh, float sigma_noise) const {
    // Prevent division by zero
    if (sigma_noise == 0.0f) return 0.0f;

    float denominator = std::sqrt(2.0f) * sigma_noise;

    return 0.5f * (1.0f - std::erf(snr_thresh / denominator));
}
bool EOSensor::isSignalDetected(float current_snr, float snr_thresh) const {
    // A standard hard detection: True if the signal breaks the threshold
    return current_snr >= snr_thresh;
}
float EOSensor::calculateDetectionProbability(
    const EOIR_Environment &env,
    const EOIR_Target &tgt,
    const EOIR_Sensor &sensor)
{
    // Step 1: Calculate Intrinsic Thermal Contrast
    float deltaT_intrinsic = std::abs(tgt.surfaceTemp - env.backgroundTemp);

    // Step 2: Calculate Solar Glint Factor
    // Glint is highly directional. We use a Gaussian distribution to simulate the reflection cone.
    // A phase angle of 0 degrees means the sun is reflecting perfectly into the sensor.
    // The "5.0f" represents the spread of the reflection; smaller numbers mean a tighter, sharper glint.
    float glintAlignment = std::exp(-std::pow(sensor.sunPhaseAngle / 5.0f, 2.0f));

    // Convert Solar Irradiance (W/m^2) to an apparent Temperature spike (Celsius).
    // The 0.05f is a simulation conversion scalar to map radiant flux to apparent Delta-T for IR.
    float glintApparentTemp = env.solarIrradiance * tgt.specularReflectivity * glintAlignment * 0.05f;

    // Step 3: Calculate Weather Attenuation Penalty
    float weatherPenalty = (env.rainfallRate * 0.25f) + (env.snowfallEquivalent * 0.35f);
    float effectiveSigma = env.baseExtinctionCoeff + weatherPenalty;

    // Step 4: Calculate Atmospheric Transmittance (Beer-Lambert Law)
    float transmittance = std::exp(-effectiveSigma * sensor.slantRange);

    // Step 5: Apply Transmittance to Both Target Signature and Glint
    // The glint also has to travel through the atmosphere to reach the sensor.
    float deltaT_apparent = (deltaT_intrinsic + glintApparentTemp) * transmittance;

    // Simulate severe thermal washout from rain
    if (env.rainfallRate > 5.0f) {
        deltaT_apparent *= 0.6f;
    }

    // Step 6: Sensor Saturation (Blinding) Check
    // If the glint is so powerful that it exceeds the sensor's maximum threshold,
    // the image washes out (blooms), making target detection impossible.
    if (deltaT_apparent > sensor.saturationLimit) {
        return 0.0f;
    }

    // Step 7: Signal-to-Noise Ratio (SNR) Check
    if (deltaT_apparent <= sensor.mrtd) {
        return 0.0f;
    }

    float signalToNoise = deltaT_apparent / sensor.mrtd;

    // Step 8: Calculate Probability of Detection (TTPF)
    const float E = 2.7f;
    float probability = std::pow(signalToNoise, E) / (1.0f + std::pow(signalToNoise, E));

    return std::clamp(probability, 0.0f, 1.0f);
}


QJsonObject EOSensor::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["name"] = QString::fromStdString(Name);
    obj["SensorType"] = "EO";
    obj["id"] = QString::fromStdString(ID);
    QJsonObject defaultObj;
    defaultObj["type"] = "Section";
    defaultObj["range"] = toParm(range,"km",0,    500);
    defaultObj["frequency"] = toParm(frequency,"Ghz", 0.1,  100);
    defaultObj["azimuth"] = toParm(azimuth,"deg", 0,    360);


    defaultObj["photon signal electrons"]  = toParm(photon_signal_electrons, "ghz",  0,100);
    defaultObj["atmospheric transmission"] = toParm(atmospheric_transmission,"τ_atm",0,1);
    defaultObj["radiant flux at detector"] = toParm( radiant_flux_at_detector,"W",   0,100);
    defaultObj["quantum efficiency"]       = toParm(quantum_efficiency,       "QE %",0,100);
    defaultObj["wavelength"]               = toParm(wavelength,                "λ m",0,100000);
    // defaultObj["integration time"]         = toParm(integration_time,   "",     0,100);
    defaultObj["optics area"]              = toParm(optics_area,        "m.sqr",0,10000);
    defaultObj["optics transmission"]      = toParm(optics_transmission,"τ_optics",0,1);
    defaultObj["aperture diameter"]        = toParm(aperture_diameter,  "m",     0,100);
    defaultObj["target size"]  = toParm(target_size, "m", 0,10000);
    defaultObj["pixel pitch"]  = toParm(pixel_pitch, "μm",0,10000);
    defaultObj["focal length"] = toParm(focal_length,"m", 0,10000);
    defaultObj["NETD"]         = toParm(NETD,        "mK",0,10000);
    defaultObj["sensor waveband start"] = toParm(sensor_waveband.first ,"L",0,1000000);
    defaultObj["sensor waveband end"] =   toParm(sensor_waveband.second,"L",0,1000000);
    defaultObj["visibility"]    = toParm(visibility_m         ,"m",0,std::numeric_limits<float>::max());
    defaultObj["mass density"]  = toParm(mass_density_g_m3    ,"g/m3",0,100);
    defaultObj["range km"]      = toParm(range_km             ,"km"  ,0,100);
    defaultObj["particle diameter m"]   = toParm(particle_diameter_m  ,"m",0,100);
    defaultObj["material density g m3"] = toParm(material_density_g_m3,"g/m3",0,100);
    defaultObj["q ext"] = toParm(q_ext, "Q_exe",1,100);

    defaultObj["number concentration m3"] = toParm(number_concentration_m3,"pp/m3",0,100000);
    defaultObj["sigma dust m1"] = toParm(sigma_dust_m1,"PM1",0,1000);
    defaultObj["attenuation dB km"] = toParm(attenuation_dB_km,"dn/km",0,1000);
    defaultObj["total loss dB"] = toParm(total_loss_dB,"db",0,1000);
    // defaultObj["detection impossible"] = toParm(detection_impossible,"",0,1000);
    // defaultObj["window abrasion possible"] = toParm(window_abrasion_possible,"",0,1000);
    defaultObj["snow precipitation rate"] = toParm(snow_precipitation_rate,"mm/hr,",0,1000);
    defaultObj["snow Extinction"] = toParm(snow_Extinction,"γ snow ",0,1000);
    defaultObj["rain precipitation rate"] = toParm(rain_precipitation_rate,"mm/hr",0,1000);
    defaultObj["rain Extinction"] = toParm(rain_Extinction,"σ rain",0,1000);
    defaultObj["air temperature"] = toParm(air_temperature_deg,"deg cel",-30,1000);
    defaultObj["IFOV"] = toParm(IFOV,"μrad",0,10000);
    // Detection
    defaultObj["SNR threshold"] = toParm(SNR_thresh,"db",0,1000);
    defaultObj["relative humidity"] = toParm(relative_humidity,"%",0,100);
    // defaultObj[""] = toParm(,"",1,100);
    // defaultObj[""] = toParm(,"",1,100);

    // Total Noise
    defaultObj["readout noise"] = toParm(readout_noise,"σ read",0,1000);
    defaultObj["photon shot noise"] = toParm(photon_shot_noise,"ghz",  0,100);
    defaultObj["thermal noise"] = toParm(thermal_noise,"σ thermal",0,1000);
    defaultObj["temporal noise 1f"] = toParm(temporal_noise_1f,"σ_1f",0,1000);
    // total_noise),
    // readout_noise),
    // photon_shot_noise),
    // thermal_noise),
    // temporal_noise_1f)
    // Environment
    defaultObj["background temperature"] = toParm(background_temperature_deg,"°C",-100,70);

    // defaultObj
    //Sensor
    // defaultObj["slant range"]           = toParm(slantRange,     "km",      0.1 , 50 ); // 5
    defaultObj["min resolvable temp diff"]      = toParm(mrtd,   "deg cel", 0, 500); // 0.05
    defaultObj["sun phase angle"]       = toParm(sunPhaseAngle,  "deg",     0,    180); // 90
    defaultObj["saturation limit"]      = toParm(saturationLimit,"deg cel", 10,   100); // 50
    //defaultObj["saturation limit"]      = toParm(saturationLimit,"deg cel", 10,   100); // 50
    QJsonObject resolutionsObj;
    resolutionsObj["type"] = "option";
    QJsonArray  optionsArray;
    for (const QString& resolution : ResolutionString)
        optionsArray.append(resolution);
    resolutionsObj["options"] = optionsArray;
    resolutionsObj["value"]   = ResolutionString[static_cast<int>(eoirResolution)];
    defaultObj["resolution"] = resolutionsObj;
    obj["default"] = defaultObj;
    return obj;
}

void EOSensor::fromJson(const QJsonObject& obj) {
    if (obj.contains("id")){
        ID = obj["id"].toString().toStdString();
    }
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    if (obj.contains("default") && obj["default"].isObject()) {
        QJsonObject defaultObj = obj["default"].toObject();
        if (defaultObj.contains("range"))
            range = valueFromParm(defaultObj["range"].toObject());

        if (defaultObj.contains("frequency"))
            frequency = valueFromParm(defaultObj["frequency"].toObject());

        if (defaultObj.contains("azimuth")){
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());
            minAzimuth = azimuth/2*-1;
            maxAzimuth = azimuth/2;
        }
        // if (defaultObj.contains("slant range"             ))
        //     slantRange      = valueFromParm(defaultObj["slant range"             ].toObject());
        if (defaultObj.contains("min resolvable temp diff"))
            mrtd            = valueFromParm(defaultObj["min resolvable temp diff"].toObject());
        if (defaultObj.contains("sun phase angle"         ))
            sunPhaseAngle   = valueFromParm(defaultObj["sun phase angle"         ].toObject());
        if (defaultObj.contains("saturation limit"        ))
            saturationLimit = valueFromParm(defaultObj["saturation limit"        ].toObject());
        if (defaultObj.contains("DetectionCapabilities") && defaultObj["DetectionCapabilities"].isObject()) {
            QJsonObject capabilitiesObj = defaultObj["DetectionCapabilities"].toObject();
            if (capabilitiesObj.contains("value"))
                capabilities = stringTodetectionCapabilities(capabilitiesObj["value"].toString());
        }
        // Environment

        if(defaultObj.contains("photon signal electrons")){
            photon_signal_electrons   = valueFromParm(defaultObj["photon signal electrons"].toObject());
        }
        if(defaultObj.contains("atmospheric transmission")){
            atmospheric_transmission   = valueFromParm(defaultObj["atmospheric transmission"].toObject());
        }

        if(defaultObj.contains("radiant flux at detector")){
            radiant_flux_at_detector   = valueFromParm(defaultObj["radiant flux at detector"].toObject());
        }
        if(defaultObj.contains("quantum efficiency")){
            quantum_efficiency   = valueFromParm(defaultObj["quantum efficiency"].toObject());
        }
        // if(defaultObj.contains("integration time")){
        //     integration_time   = valueFromParm(defaultObj["integration time"].toObject());
        // }
        if(defaultObj.contains("optics area")){
            optics_area   = valueFromParm(defaultObj["optics area"].toObject());
        }
        if(defaultObj.contains("optics transmission")){
            optics_transmission   = valueFromParm(defaultObj["optics transmission"].toObject());
        }
        if(defaultObj.contains("aperture diameter")){
            aperture_diameter   = valueFromParm(defaultObj["aperture diameter"].toObject());
        }
        if(defaultObj.contains("target size")){
            target_size   = valueFromParm(defaultObj["target size"].toObject());
        }
        if(defaultObj.contains("pixel pitch")){
            pixel_pitch   = valueFromParm(defaultObj["pixel pitch"].toObject());
        }
        if(defaultObj.contains("focal length")){
            focal_length   = valueFromParm(defaultObj["focal length"].toObject());
        }
        if(defaultObj.contains("NETD")){
            NETD   = valueFromParm(defaultObj["NETD"].toObject());
        }
        if(defaultObj.contains("sensor waveband start")){
            sensor_waveband.first   = valueFromParm(defaultObj["sensor waveband start"].toObject());
        }
        if(defaultObj.contains("sensor waveband end")){
            sensor_waveband.second   = valueFromParm(defaultObj["sensor waveband end"].toObject());
        }
        if(defaultObj.contains("visibility")){
            visibility_m   = valueFromParm(defaultObj["visibility"].toObject());
        }
        if(defaultObj.contains("mass density")){
            mass_density_g_m3   = valueFromParm(defaultObj["mass density"].toObject());
        }
        if(defaultObj.contains("range km")){
            range_km   = valueFromParm(defaultObj["range km"].toObject());
        }
        if(defaultObj.contains("particle diameter m")){
            particle_diameter_m   = valueFromParm(defaultObj["particle diameter m"].toObject());
        }
        if(defaultObj.contains("material density g m3")){
            material_density_g_m3   = valueFromParm(defaultObj["material density g m3"].toObject());
        }
        if(defaultObj.contains("q ext")){
            q_ext   = valueFromParm(defaultObj["q ext"].toObject());
        }
        if(defaultObj.contains("number concentration m3")){
            number_concentration_m3   = valueFromParm(defaultObj["number concentration m3"].toObject());
        }
        if(defaultObj.contains("sigma dust m1")){
            sigma_dust_m1   = valueFromParm(defaultObj["sigma dust m1"].toObject());
        }
        if(defaultObj.contains("attenuation dB km")){
            attenuation_dB_km   = valueFromParm(defaultObj["attenuation dB km"].toObject());
        }
        if(defaultObj.contains("total loss dB")){
            total_loss_dB   = valueFromParm(defaultObj["total loss dB"].toObject());
        }
        // if(defaultObj.contains("detection impossible")){
        //     detection_impossible   = valueFromParm(defaultObj["detection impossible"].toObject());
        // }
        // if(defaultObj.contains("window abrasion possible")){
        //     window_abrasion_possible   = valueFromParm(defaultObj["window abrasion possible"].toObject());
        // }
        if(defaultObj.contains("relative humidity")){
            relative_humidity   = valueFromParm(defaultObj["relative humidity"].toObject());
        }
        if(defaultObj.contains("snow precipitation rate")){
            snow_precipitation_rate   = valueFromParm(defaultObj["snow precipitation rate"].toObject());
        }
        if(defaultObj.contains("snow Extinction")){
            snow_Extinction   = valueFromParm(defaultObj["snow Extinction"].toObject());
        }
        if(defaultObj.contains("rain precipitation rate")){
            rain_precipitation_rate   = valueFromParm(defaultObj["rain precipitation rate"].toObject());
        }
        if(defaultObj.contains("rain Extinction")){
            rain_Extinction   = valueFromParm(defaultObj["rain Extinction"].toObject());
        }
        // Environment
        if(defaultObj.contains("background temperature")){
            background_temperature_deg  = valueFromParm(defaultObj["background temperature"].toObject());
        }
        if(defaultObj.contains("IFOV")){
            IFOV  = valueFromParm(defaultObj["IFOV"].toObject());
        }
        if(defaultObj.contains("air temperature")){
            air_temperature_deg  = valueFromParm(defaultObj["air temperature"].toObject());
        }

        if(defaultObj.contains("readout noise")){
            readout_noise   = valueFromParm(defaultObj["readout noise"].toObject());
        }
        if(defaultObj.contains("photon shot noise")){
            photon_shot_noise   = valueFromParm(defaultObj["photon shot noise"].toObject());
        }
        if(defaultObj.contains("thermal noise")){
            thermal_noise   = valueFromParm(defaultObj["thermal noise"].toObject());
        }
        if(defaultObj.contains("temporal noise 1f")){
            temporal_noise_1f   = valueFromParm(defaultObj["temporal noise 1f"].toObject());
        }

        if(defaultObj.contains("SNR threshold")){
            SNR_thresh  = valueFromParm(defaultObj["SNR threshold"].toObject());
        }
        if(defaultObj.contains("resolution")){
            QJsonObject resolutionsObj = defaultObj["resolution"].toObject();

            if(!resolutionsObj.contains("value")) return;
            QString value = resolutionsObj["value"].toString();

            // if(!ResolutionString->contains(value)) return;

            if(ResolutionStrToEnum.contains(value)){
                eoirResolution = ResolutionStrToEnum[value];
                //static_cast<Resolutions>(resolution);
            }

        }
    }
}

void EOSensor::setEnvironment(float relativeHumidity, float absoluteHumidity, float rainfallRate, float snowfallEquivalent, float ambientTemp, float backgroundTemp, float aerosolConcentration, float baseExtinctionCoeff, float ambientIlluminance, float solarIrradiance)
{
    this->relativeHumidity = relativeHumidity;
    this->absoluteHumidity = absoluteHumidity;
    this->rainfallRate = rainfallRate;
    this->snowfallEquivalent = snowfallEquivalent;
    this->ambientTemp = ambientTemp;
    this->background_temperature = backgroundTemp;
    this->aerosolConcentration = aerosolConcentration;
    this->baseExtinctionCoeff = baseExtinctionCoeff;
    this->ambientIlluminance = ambientIlluminance;
    this->solarIrradiance = solarIrradiance;
}

void EOSensor::setEnvironmentToDefault()
{
    // relativeHumidity     = 30   ;       // % (0.0 to 100.0)
    // absoluteHumidity     = 10.0 ;       // g/m^3
    // rainfallRate         = 0.0  ;       // mm/hr
    // snowfallEquivalent   = 0.0  ;       // mm/hr
    // ambientTemp          = 30   ;       // Celsius
    // background_temperature        = 20.0 ;       // Celsius
    // aerosolConcentration = 0.05 ;       // mg/m^3
    // baseExtinctionCoeff  = 0.15 ;       // Base sigma (1/km)
    // ambientIlluminance   = 50000;       // lux
    // solarIrradiance      = 800  ;       // W/m^2 (Crucial for Glint)

    relativeHumidity       = 40.0f;
    absoluteHumidity       = 8.0f;
    rainfallRate           = 0.0f;
    snowfallEquivalent     = 0.0f;
    ambientTemp            = 25.0f;
    background_temperature = 20.0f;
    background_temperature_deg = 20.0f;
    aerosolConcentration   = 0.02f;
    baseExtinctionCoeff    = 0.05f;
    ambientIlluminance     = 80000.0f;
    solarIrradiance        = 900.0f;
    atmospheric_transmission = 0.90f;
    visibility_m             = 25000.0f;
    rain_precipitation_rate  = 0.0f;
    snow_precipitation_rate  = 0.0f;
}

