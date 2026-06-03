#include "eosensor.h"
#include <QFileInfo>
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"


EOSensor::EOSensor(Hierarchy* h) : Sensor(h) {
    subType = SubType::EO;
    // azimuth = 360;
    frequency = 10.0f;
    eo_ir = new EO_IR(h);
    m_h = h;
    // range = 50;
    //m_Platforms= h->Platforms;
    m_Platforms= root->Platforms;
    m_Specialzones = h->Specialzones;
    //EO_PayLoad eo_payload;
    //eo = new EO(eo_payload);
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


void EOSensor::scan(){
    if(!Active)return;
    // qDebug() << "[Sensor::ewscan] called for ID:" << QString::fromStdString(id)
    if(!parentEntity) return;
    Transform* source = (root->Platforms)[parentEntity->ID]->transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop

    str = QString();

    auto key = parentEntity->ID;
    auto sensor_platform = m_Platforms.find(key);
    if(sensor_platform != m_Platforms.end()){
        str += sensor_platform->second->Name.c_str();
        str += " ";
    }else{
        str += "NAN";
        return;
    }
    currentEOSensor = key;

    str += QString("Input:[Az:%1, Rg:%2]").arg(
        QString::number(azimuth),
        QString::number(range*1000));
    // minAzimuth = azimuth/2*-1;
    // maxAzimuth = azimuth/2;
    // PoseGeo sensorPoseGeo;
    Transform *sensor_tf;
    if(sensor_platform->second->transform){
        sensor_tf = sensor_platform->second->transform;

        sensorPoseGeo.latitude  = sensor_platform->second->transform->getLatitude();
        sensorPoseGeo.longitude = sensor_platform->second->transform->getLongitude();
        sensorPoseGeo.altitude  = sensor_platform->second->transform->getAltitude()*FEET_TO_METERS;
        sensorPoseGeo.rx = sensor_platform->second->transform->rotation().x();
        sensorPoseGeo.ry = sensor_platform->second->transform->rotation().y();
        sensorPoseGeo.rz = sensor_platform->second->transform->rotation().z();

        sensorGeoCoord.latitude = sensor_platform->second->transform->getLatitude();
        sensorGeoCoord.longitude = sensor_platform->second->transform->getLongitude();
        sensorGeoCoord.altitude = sensor_platform->second->transform->getLatitude();

    }else{
        return;
    }

    /*           Defining the EO Sensor Start            */
    if(eo_ir){

        // Setting Up Surrounding
        auto eo_surrounding = eo_ir->eo_surrounding;
        QString AV[] = {"NA","AV"};
        int AVnum = 0;
        if(m_Specialzones.find(key) != m_Specialzones.end()){
            auto ssz = m_Specialzones.at(key);
            // eo_surrounding = getSurrounding(ssz);
            eo_surrounding = getSurrounding(ssz->collider->CollideRadius,1,ssz->rain,ssz->fog,ssz->humidity);
            AVnum = 1;
        }else{
            eo_surrounding = getSurrounding();
        }
        // Setting Up Sensor

        auto eo_sensor = eo_ir->eo_sensor;
        eo_sensor.fov      = azimuth;
        eo_sensor.maxRange = range;
        // Initializing EO Sensor
        eo = eo_ir->initEO(eo_sensor,eo_surrounding);


        /*    Get & Set Threshold Range Start          */
        float threshold  = eo->getSetThreshold(range*1000,8);
        /*     Get & Set Threshold Range Start         */

        // Display Values
        str += QString("Env(%1) : [A:%2, R:%3, H:%4, F:%5, Th:%6 ]").arg(
            AV[AVnum],
            QString::number(eo_surrounding.k_atm ),
            QString::number(eo_surrounding.k_rain),
            QString::number(eo_surrounding.k_humidity),
            QString::number(eo_surrounding.k_fog ),
            QString::number(threshold ));

    }else{
        return;
    }
    /*            Defining the EO Sensor End             */

    //
    // /*               Dynamic Range Start                 */
    // range = eo->getMaxRange(80)/1000.f;
    // /*                Dynamic Range End                  */
    //

    float sensorLatitude  = source->getLatitude ();
    float sensorLongitude = source->getLongitude();
    float sensorAltitude  = source->getAltitude()*0.3048;
    float sensorHeading   = source->getHeading();
    float sensorPitch     = source->pitch();

    /*     Getting Sensored Entity Coordinates Start     */
    Coordinate sensor_cood =
        {sensorLatitude ,
         sensorLongitude,
         sensorAltitude };
    /*      Getting Sensored Entity Coordinates End      */

    /*             EO Vision of Sensor Start             */

    Vec3 sensorECEF = eoVision.geoToECEF(
        sensorLatitude ,
        sensorLongitude,
        sensorAltitude );

    QString dstr = QString("Sensor : {"
                           "Latitude :%1, "
                           "Longitude :%2, "
                           "Altitude :%3, "
                           "Heading :%4, "
                           "Pitch :%5"
                           "} "
                           ).arg(
                           QString::number(sensorLatitude ),
                           QString::number(sensorLongitude),
                           QString::number(sensorAltitude ),
                           QString::number(sensorHeading  ),
                           QString::number(sensorPitch    )
                           );

    /*              EO Vision of Sensor End              */
    m_Platforms= root->Platforms;
    str += QString("Contains %1 : {").arg(m_Platforms.size());
    mstr = QString("Contains %1 : {").arg(m_Platforms.size());
    dstr += " :{";
    ewtargets.clear();
    eoEntities.clear();
    int gint = 0;
    QString gstr = "{ ";

    /*               Get Resolution Angle Start         */
    eoirResolutionSize = resolutionToSize[eoirResolution];
    horizonatalAngle = azimuth;
    distance         = range*1000;
    verticalAngle    = getVeticalAngle(eoirResolutionSize);
    gstr += QString(
                "[ H A: %1, "
                "V A: %2 ], "
                ).arg(
                    QString::number(horizonatalAngle),
                    QString::number(verticalAngle));
    /*                Get Resolution Angle End          */

    /*                      Sensor Para                 */
    EOIR_Sensor sensor_para = {slantRange,mrtd,sunPhaseAngle,saturationLimit};
    EOIR_Environment environment_para = {
                                         relativeHumidity, absoluteHumidity, rainfallRate,
                                         snowfallEquivalent, ambientTemp, backgroundTemp,
                                         aerosolConcentration, baseExtinctionCoeff, ambientIlluminance,
                                         solarIrradiance};

    ostr = QString();
    ostr += QString(
                "Sensor: ["
                "lat, lon, alt :[%1, %2, %3], "
                "rx, ry, rz:[%4, %5, %6] "
                "], "
                ).arg(
                    QString::number(sensorPoseGeo.latitude  ),
                    QString::number(sensorPoseGeo.longitude ),
                    QString::number(sensorPoseGeo.altitude  ),
                    QString::number(sensorPoseGeo.rx ),
                    QString::number(sensorPoseGeo.ry ),
                    QString::number(sensorPoseGeo.rz )
                    );
    for (auto& [ID, platform] : m_Platforms){
        if(key == ID) continue;
        str += QString(" %1: ").arg(platform->Name.c_str());
        if(Transform *tf = platform->transform){

            // Getting Dimension
            EntityDimension ed;
            ed.length = platform->collider->Length;
            ed.width  = platform->collider->Width;
            ed.heigth = platform->collider->Height;

            // Getting Coordinates
            Coordinate obj_cood =
                {tf->getLatitude(),
                 tf->getLongitude(),
                 tf->getAltitude()};

            // Getting View Direction
            Vec3 viewDir = eo_ir->getViewDir(sensor_cood, obj_cood);

            // Getting Projection Area of Entity to be scanned
            float projectionArea = eo_ir->getProjectedArea(viewDir, ed);

            // Getting Distance Btw Sensor and Entity
            float distance = eo_ir->distanceBtw(sensor_cood,obj_cood);

            // Getting Angle Btw Sensor and Entity
            float angle    = eo_ir->calculateAngle(
                sensor_tf->getHeading(),sensor_tf->pitch(),
                sensor_cood,obj_cood);

            // Getting illumination , glintFactor, from Entity
            float ilm = platform->illumination;
            float glt = platform->glintFactor;

            str += QString("[ DS:%1, AN:%2, VD:(%3,%4,%5), PA:%6, IL:%7, GT:%8 ]").arg(
                QString::number(distance),
                QString::number(angle),
                QString::number(viewDir.x),
                QString::number(viewDir.y),
                QString::number(viewDir.z),
                QString::number(projectionArea));
            /*     Getting Sensored Entity Coordinates Start     */
            //float azimuthRad = azimuth * M_PI / 180.0;
            float displayWidth = 0.02;
            float targetLatitude  = platform->transform->getLatitude ();
            float targetLongitude = platform->transform->getLongitude();
            float targetAltitude  = platform->transform->getAltitude()*0.3048;
            Coordinate target_cood = {targetLatitude,targetLongitude,targetAltitude};
            float targetHeading   = platform->transform->getHeading();
            float targetPitch     = platform->transform->pitch();
            float sensorHeading = source->getHeading();
            float sensorPitch   = source->pitch();
            float baseSize      = 15;
            //QString sprite = platform->meshRenderer2d->Sprite->c_str();
            QFileInfo m_file(platform->meshRenderer2d->Sprite->c_str());
            dstr += QString("Tragets : {"
                            "Latitude :%1, "
                            "Longitude :%2, "
                            "Altitude :%3, "
                            "Heading :%4, "
                            "Pitch :%5, "
                            "Display Width: %6, "
                            "Base Size: %7"
                            "}, "
                            ).arg(
                            QString::number(targetLatitude  ),
                            QString::number(targetLongitude ),
                            QString::number(targetAltitude  ),
                            QString::number(targetHeading   ),
                            QString::number(targetPitch     ),
                            QString::number(displayWidth  ),
                            QString::number(baseSize    )
                            );
            float targetHorizontalDistance = eo_ir->getHorizontalDistance(sensor_cood,target_cood);
            float targetHorizontalTargetAngle =
                eo_ir->getHorizontalTargetAngle(sensor_cood,target_cood,sensorHeading);
            float targetVerticalTargetAngle =
                eo_ir->getVerticalTargetAngle(sensorAltitude,targetAltitude,
                                              targetHorizontalDistance,sensorPitch);

            bool detection = isUnderRangeNAngle(targetHorizontalDistance,targetVerticalTargetAngle,targetHorizontalTargetAngle);
            if(!detection) continue;

            sensor_para.slantRange = eo_ir->distanceBtw(sensor_cood,obj_cood)/1000;

            float result = calculateDetectionProbability(environment_para,{platform->surfaceTemp,platform->specularReflectivity},sensor_para);


            gstr += QString(
                        "T%1.["
                        "HD :%2,"
                        "HTA :%3,"
                        "VTA :%4,"
                        "D :%5"
                        "Pr :%6],"
                        ).arg(
                            QString::number(++gint),
                            QString::number(targetHorizontalDistance),
                            QString::number(targetHorizontalTargetAngle),
                            QString::number(targetVerticalTargetAngle),
                            BoolToStr[detection],
                            QString::number(result)
                            );
            PoseGeo targetPoseGeo;
            targetPoseGeo.latitude  = platform->transform->getLatitude();
            targetPoseGeo.longitude = platform->transform->getLongitude();
            targetPoseGeo.altitude  = platform->transform->getAltitude()*FEET_TO_METERS;
            targetPoseGeo.rx = platform->transform->rotation().x();
            targetPoseGeo.ry = platform->transform->rotation().y();
            targetPoseGeo.rz = platform->transform->rotation().z();

            GeoCoord targetGeoCoord;
            targetGeoCoord.latitude  = platform->transform->getLatitude();
            targetGeoCoord.longitude = platform->transform->getLongitude();
            targetGeoCoord.altitude  = platform->transform->getAltitude()*FEET_TO_METERS;

            CartesianCoord targetCartesianCoord = getRelativeCoordinatesGL(sensorGeoCoord, targetGeoCoord);
            targetCartesianCoord.rx = platform->transform->rotation().x();
            targetCartesianCoord.ry = platform->transform->rotation().y();
            targetCartesianCoord.rz = platform->transform->rotation().z();
            ostr += QString(
                        "Target: ["
                        "Name: %1 ,"
                        "Input: ( "
                        "lat, lon, alt :[%2, %3, %4], "
                        "rx, ry, rz: [%5, %6, %7] "
                        "), Result: ("
                        "x, y, z :[%8, %9, %10], "
                        ") ], "
                        ).arg(
                            platform->Name.c_str(),
                            QString::number(platform->transform->getLatitude()),
                            QString::number(platform->transform->getLongitude()),
                            QString::number(platform->transform->getAltitude()*FEET_TO_METERS),
                            QString::number(platform->transform->rotation().x()),
                            QString::number(platform->transform->rotation().y()),
                            QString::number(platform->transform->rotation().z()),
                            QString::number(targetCartesianCoord.x),
                            QString::number(targetCartesianCoord.y),
                            QString::number(targetCartesianCoord.z)
                            );
            // ostr += QString(
            //             "Target: ["
            //             "lat, lon, alt :[%1, %2, %3], "
            //             "rx, ry, rz:[%4, %5, %6] "
            //             "], "
            //             ).arg(
            //                 QString::number(targetPoseGeo.latitude ),
            //                 QString::number(targetPoseGeo.longitude),
            //                 QString::number(targetPoseGeo.altitude ),
            //                 QString::number(targetPoseGeo.rx),
            //                 QString::number(targetPoseGeo.ry),
            //                 QString::number(targetPoseGeo.rz)
            //                 );

            PoseOpenGL targetPoseOpenGL = CalculateRelativeOpenGLPose(targetPoseGeo,sensorPoseGeo);

            if (result <  0.9 ) continue;
            if (result >  0.9 ){
                //EODetection[ID] = true;
                EODetectionCood[ID] = {true,targetPoseOpenGL};
            }else{
                EODetectionCood[ID] = {false,targetPoseOpenGL};
            }
            EODetectionCoord[ID] = {true,targetCartesianCoord};

            // ostr += QString(
            //     "Result: ["
            //     "Name: %1, "
            //     "Exist: %2, "
            //     "xyz:(%3,%4,%5), "
            //     "rxyz:(%6,%7,%8), "
            //     "], "
            //     ).arg(
            //     platform->Name.c_str(),
            //     QString::number(EODetectionCood[ID].first),
            //     QString::number(EODetectionCood[ID].second.x),
            //     QString::number(EODetectionCood[ID].second.y),
            //     QString::number(EODetectionCood[ID].second.z),
            //     QString::number(EODetectionCood[ID].second.rx),
            //     QString::number(EODetectionCood[ID].second.ry),
            //     QString::number(EODetectionCood[ID].second.rz)
            //     );
        }

    }
    // mstr += " }";
    ostr += QString("Size: ")+QString::number(EODetectionCood.size());
    str  += "}";
    gstr += "}";
    dstr += "}";
    debug(ostr,D_OpenGL);
    // debug(mstr,D_Mesh);
    // debug(dstr,D_Details);
    // debug(str,D_INIT);
    // debug(gstr,D_Geometry);
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

// Main Function: Calculate relative OpenGL coordinates and rotations
PoseOpenGL EOSensor::CalculateRelativeOpenGLPose(const PoseGeo& target, const PoseGeo& sensor) {
    // 1. Convert Sensor to ECEF
    double sx, sy, sz;
    GeodeticToECEF(sensor.latitude, sensor.longitude, sensor.altitude, sx, sy, sz);

    // 2. Convert Target to ECEF
    double tx, ty, tz;
    GeodeticToECEF(target.latitude, target.longitude, target.altitude, tx, ty, tz);

    // 3. ECEF Differences
    double dx = tx - sx;
    double dy = ty - sy;
    double dz = tz - sz;

    // 4. Transform ECEF differences to Sensor's local ENU (East, North, Up) plane
    double lat_rad = deg2rad(sensor.latitude);
    double lon_rad = deg2rad(sensor.longitude);

    double sin_lat = std::sin(lat_rad);
    double cos_lat = std::cos(lat_rad);
    double sin_lon = std::sin(lon_rad);
    double cos_lon = std::cos(lon_rad);

    double east  = -sin_lon * dx + cos_lon * dy;
    double north = -sin_lat * cos_lon * dx - sin_lat * sin_lon * dy + cos_lat * dz;
    double up    =  cos_lat * cos_lon * dx + cos_lat * sin_lon * dy + sin_lat * dz;

    // 5. Map ENU to OpenGL Coordinate System
    // OpenGL uses a right-handed system: X = East, Y = Up, Z = -North
    PoseOpenGL result;
    result.x = static_cast<float>(east);
    result.y = static_cast<float>(up);
    result.z = static_cast<float>(-north);

    // 6. Relative Rotation
    // Assuming rotation axes are local Euler angles (e.g., degrees).
    // To view the target from the sensor's perspective, subtract sensor rotation from target rotation.
    // (Note: For rigorous 3D spatial attitudes, quaternion or rotation matrix multiplication is preferred:
    // R_rel = R_sensor_inv * R_target. Direct subtraction works for simple offset modeling).
    result.rx = target.rx - sensor.rx;
    result.ry = target.ry - sensor.ry;
    result.rz = target.rz - sensor.rz;

    return result;
}

/*
 * Calculates the relative X, Y, and Z coordinates mapped to OpenGL space.
 * +X = East, +Y = Up, -Z = North
 */
CartesianCoord EOSensor::getRelativeCoordinatesGL(GeoCoord sensor, GeoCoord target) {
    // 1. Calculate deltas in degrees, then convert to radians
    float deltaLatRad = (target.latitude - sensor.latitude) * DEG_TO_RAD_F;
    float deltaLonRad = (target.longitude - sensor.longitude) * DEG_TO_RAD_F;

    // 2. Convert sensor latitude to radians for the East-West scaling
    float latSensorRad = sensor.latitude * DEG_TO_RAD_F;

    // 3. Adjust the Earth's radius to the sensor's current altitude level
    float localRadius = EARTH_RADIUS_F + sensor.altitude;

    // 4. Calculate raw distances
    float distanceEast  = localRadius * deltaLonRad * std::cos(latSensorRad);
    float distanceNorth = localRadius * deltaLatRad;
    float distanceUp    = target.altitude - sensor.altitude;

    // 5. Map to OpenGL Right-Handed Coordinate System
    return {
        distanceEast,  // X: Right (East)
        distanceUp,    // Y: Up (Altitude)
        -distanceNorth // Z: Back (South, so North is negative Z)
    };
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

ScreenTarget EOSensor::getTargetScreenBoundingBox(
    Resolutions res,
    float sensorHorizontalFovDeg,
    float sensorVerticalFovDeg,
    float targetHorizontalAngleDeg,
    float targetVerticalAngleDeg,
    float targetDistanceMeters,
    float targetPhysicalWidthMeters,
    float targetPhysicalHeightMeters
    )
{
    // Initialize with float zeroes
    ScreenTarget result = {0.0f, 0.0f, 0.0f, 0.0f, false};

    // auto length = resolutionToSize.at(res).first;
    // auto width  = resolutionToSize.at(res).second;
    //auto it = resolutionToSize.at(res);
    auto it = resolutionToSize.find(res);
    if (it == resolutionToSize.end()) return result;

    // Cast resolution bounds to float once
    // float screenWidth = static_cast<float>(length);
    // float screenHeight = static_cast<float>(width);
    float screenWidth = static_cast<float>(it->second.first);
    float screenHeight = static_cast<float>(it->second.second);

    // 1. Convert FOVs and Angles to radians
    float fovXRad = sensorHorizontalFovDeg * (M_PI / 180.0f);
    float fovYRad = sensorVerticalFovDeg * (M_PI / 180.0f);
    float targetAngleXRad = targetHorizontalAngleDeg * (M_PI / 180.0f);
    float targetAngleYRad = targetVerticalAngleDeg * (M_PI / 180.0f);

    // 2. Calculate Focal Lengths (in pixels)
    float focalLengthX = (screenWidth / 2.0f) / tanf(fovXRad / 2.0f);
    float focalLengthY = (screenHeight / 2.0f) / tanf(fovYRad / 2.0f);

    // 3. Calculate Center Position (X, Y) directly as floats
    float offsetX = focalLengthX * tanf(targetAngleXRad);
    float offsetY = focalLengthY * tanf(targetAngleYRad);

    result.x = (screenWidth / 2.0f) + offsetX;
    result.y = (screenHeight / 2.0f) - offsetY;

    // 4. Calculate Size (Width, Height) directly as floats
    if (targetDistanceMeters > 0.001f) {
        result.width = (targetPhysicalWidthMeters * focalLengthX) / targetDistanceMeters;
        result.height = (targetPhysicalHeightMeters * focalLengthY) / targetDistanceMeters;
    } else {
        result.width = 0.0f;
        result.height = 0.0f;
    }

    // 5. Determine Visibility using float boundaries
    if (std::abs(targetHorizontalAngleDeg) < 90.0f && std::abs(targetVerticalAngleDeg) < 90.0f) {

        float halfW = result.width / 2.0f;
        float halfH = result.height / 2.0f;

        // Target is visible if its right edge is past screen left (0),
        // and its left edge is before screen right (screenWidth).
        bool intersectsX = (result.x + halfW >= 0.0f) && (result.x - halfW < screenWidth);
        bool intersectsY = (result.y + halfH >= 0.0f) && (result.y - halfH < screenHeight);

        if (intersectsX && intersectsY) {
            result.isOnScreen = true;
        }
    }

    return result;
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

    //Sensor
    // defaultObj["slant range"]           = toParm(slantRange,     "km",      0.1 , 50 ); // 5
    defaultObj["min resolvable temp diff"]      = toParm(mrtd,   "deg cel", 0.01, 500); // 0.05
    defaultObj["sun phase angle"]       = toParm(sunPhaseAngle,  "deg",     0,    180); // 90
    defaultObj["saturation limit"]      = toParm(saturationLimit,"deg cel", 10,   100); // 50

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
    this->backgroundTemp = backgroundTemp;
    this->aerosolConcentration = aerosolConcentration;
    this->baseExtinctionCoeff = baseExtinctionCoeff;
    this->ambientIlluminance = ambientIlluminance;
    this->solarIrradiance = solarIrradiance;
}

void EOSensor::setEnvironmentToDefault()
{
    relativeHumidity     = 30   ;       // % (0.0 to 100.0)
    absoluteHumidity     = 10.0 ;       // g/m^3
    rainfallRate         = 0.0  ;       // mm/hr
    snowfallEquivalent   = 0.0  ;       // mm/hr
    ambientTemp          = 30   ;       // Celsius
    backgroundTemp       = 20.0 ;       // Celsius
    aerosolConcentration = 0.05 ;       // mg/m^3
    baseExtinctionCoeff  = 0.15 ;       // Base sigma (1/km)
    ambientIlluminance   = 50000;       // lux
    solarIrradiance      = 800  ;       // W/m^2 (Crucial for Glint)
}
/*------------    Custom Debugger Start    ------------*/

void EOSensor::debug(const QString &str, const debugSQLite &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }
}

bool EOSensor::dbgIsAllow(const debugSQLite &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    return InsideList;
}

/*------------     Custom Debugger End     ------------*/
