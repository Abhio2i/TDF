#include "eosensor.h"

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
    double radius,
    double atmCoeff,
    double rainRate,
    double fog,
    double humidity)
{
    Surrounding srd;

    //Setting Atmospheric
    srd.k_atm = 0.00001;
    // 0.00001 → very clear air
    // 0.001   → hazy

    //Setting Rain
    double alpha = 0.0001;// This Value is Approx
    double beta  = 0.8;   // This Value is Approx
    srd.k_rain   = alpha * pow(rainRate, beta);

    //Setting Fog (via visibility)
    double visibility = radius*fog;
    srd.k_fog = 3.912 / visibility;

    //Setting Humidity
    double humidityCoeff = 0.0005; // This Value is Approx
    srd.k_humidity  = humidityCoeff * humidity;

    return srd;
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

    str += QString("Input:[Az:%1, Rg:%2]").arg(
        QString::number(azimuth),
        QString::number(range*1000));


    Transform *sensor_tf;
    if(sensor_platform->second->transform){
        sensor_tf = sensor_platform->second->transform;
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
        double threshold  = eo->getSetThreshold(range*1000,8);
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

    double sensorLatitude  = source->getLatitude ();
    double sensorLongitude = source->getLongitude();
    double sensorAltitude  = source->getAltitude()*0.3048;
    double sensorHeading   = source->getHeading();
    double sensorPitch     = source->pitch();

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
    dstr += " :{";
    ewtargets.clear();
    eoEntities.clear();
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
            double projectionArea = eo_ir->getProjectedArea(viewDir, ed);

            // Getting Distance Btw Sensor and Entity
            double distance = eo_ir->distanceBtw(sensor_cood,obj_cood);

            // Getting Angle Btw Sensor and Entity
            double angle    = eo_ir->calculateAngle(
                sensor_tf->getHeading(),sensor_tf->pitch(),
                sensor_cood,obj_cood);

            // Getting illumination , glintFactor, from Entity
            double ilm = platform->illumination;
            double glt = platform->glintFactor;

            str += QString("[ DS:%1, AN:%2, VD:(%3,%4,%5), PA:%6, IL:%7, GT:%8 ]").arg(
                QString::number(distance),
                QString::number(angle),
                QString::number(viewDir.x),
                QString::number(viewDir.y),
                QString::number(viewDir.z),
                QString::number(projectionArea));
            /*     Getting Sensored Entity Coordinates Start     */
            //double azimuthRad = azimuth * M_PI / 180.0;
            double displayWidth = 0.02;

            double targetLatitude  = platform->transform->getLatitude ();
            double targetLongitude = platform->transform->getLongitude();
            double targetAltitude  = platform->transform->getAltitude()*0.3048;
            double targetHeading   = platform->transform->getHeading();
            double targetPitch     = platform->transform->pitch();
            double sensorHeading = source->getHeading();
            double sensorPitch   = source->pitch();
            double baseSize      = 15;
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

            Vec3 targetECEF = eoVision.geoToECEF(targetLatitude, targetLongitude, targetAltitude);
            Vec3 enu = eoVision.ecefToENU(targetECEF, sensorECEF, sensorLatitude, sensorLongitude);
            Vec3 view = eoVision.rotateToSensorFrame(enu, sensorHeading, sensorPitch);

            //if (!(eoVision.isInsideFOV(view, azimuth, azimuth))) continue;
            double distance_ = enu.length();
            if (distance_ < 1.0) {
                dstr += "WARNING: Target too close or same as sensor\n";
            }
            bool isInsideFOV = eoVision.isInsideFOV(view, azimuth, azimuth);
            if(!isInsideFOV) continue;
            double focalLength = (displayWidth / 2) / tan((azimuth * M_PI / 180.0) / 2);
            //double focalLength = (displayWidth / 2) / tan(azimuthRad / 2);
            //double focalLength = (displayWidth / 2) / tan( azimuth/ 2);
            Vec2 screen = eoVision.project(view, focalLength);
            //double size = baseSize * (focalLength / enu.length());
            double size = baseSize * (focalLength / enu.length());
            size = std::max(0.0, size);
            std::string name = platform->Name;
            view.normalized();
            double relativeHeading = eo_ir->relativeAngle(sensorHeading,targetHeading);
            double relativePitch   = eo_ir->relativeAngle(sensorPitch,targetPitch);
            double relativeBearingAngle = gra.computeRelativeAngle(
                sensorLatitude,sensorLongitude,sensorHeading,
                targetLatitude,targetLongitude);
            double elevationAngle = gre.computeRelativeElevation(
                sensorLatitude,sensorLongitude,sensorAltitude,sensorHeading,
                targetLatitude,targetLongitude,targetAltitude);
            // double relativeHeadingII = eo_ir->relativeAngle();
            // double relativePitchII   = eo_ir->relativeAngle();

            EO_Entity eo_entity(name,size,relativeHeading,targetPitch,
                {screen.x,screen.y},{view.x,view.y,view.z});

            eoEntities.push_back(eo_entity);

            dstr += QString("Result of Target : {"
                            "targetECEF : (x:%1,y:%2,z:%3), "
                            "enu : (x:%4,y:%5,z:%6), "
                            "view : (x:%7,y:%8,z:%9), "
                            "focalLength :%10, "
                            "isInsideFOV :%11, "
                            "screen : (x:%12,y:%13), "
                            "size: %14"
                            "}, "
                            ).arg(
            QString::number(targetECEF.x),
            QString::number(targetECEF.y),
            QString::number(targetECEF.z),
            QString::number(enu.x),
            QString::number(enu.y),
            QString::number(enu.z),
            QString::number(view.x),
            QString::number(view.y),
            QString::number(view.z),
            QString::number(focalLength),
            QString::number(isInsideFOV),
            QString::number(screen.x),
            QString::number(screen.y),
            QString::number(size));
            /*      Getting Sensored Entity Coordinates End      */



            // Checking Detection of Entity
            bool isDetected = eo->isDetected(projectionArea,distance,angle,ilm,glt);

            if( isDetected == false){
                str += QString(" D:F],");
                continue;
            }else{
                str += QString(" D:T],");
            }
            Target target;
            target.entity = platform;
            target.angle = angle;//-((h.azimuth_deg+h)+180.f);
            target.radius = distance/1000.f;
            ewtargets.append(target);


        }

    }
    str += " }";
    dstr += "}";
    debug(dstr,D_Details);
    debug(str,D_INIT);

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
    // defaultObj["FOV"] = toParm(azimuth,"deg", 0,    360);
    // defaultObj["max distance"] = toParm(range,"km",0,    500);
    // defaultObj["threshold"] = toParm(range,"%", 0.000000000, 1.000000000);
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

        if (defaultObj.contains("azimuth"))
            azimuth = valueFromParm(defaultObj["azimuth"].toObject());

        // if (defaultObj.contains("FOV"))
        //     azimuth = valueFromParm(defaultObj["FOV"].toObject());
        // if (defaultObj.contains("max distance"))
        //     azimuth = valueFromParm(defaultObj["max distance"].toObject());
        // if (defaultObj.contains("threshold"))
        //     azimuth = valueFromParm(defaultObj["threshold"].toObject());
    }
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
