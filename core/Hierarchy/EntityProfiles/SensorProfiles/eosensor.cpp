#include "eosensor.h"

#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/hierarchy.h"


EOSensor::EOSensor(Hierarchy* h) : Sensor(h) {
    subType = SubType::EO;
    azimuth = 360;
    frequency = 10.0f;
    eo_ir = new EO_IR(h);
    m_h = h;
    range = 50;
    m_Platforms= &h->Platforms;
    m_Specialzones = &h->Specialzones;
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
    Transform* source = root->Platforms[parentEntity->ID]->transform;
    if(!source) return;
    // C# foreach (Transform tr in targets) -> C++ range-based for loop

    str = QString();
    auto key = parentEntity->ID;
    auto sensor_platform = m_Platforms->find(key);
    if(sensor_platform != m_Platforms->end()){
        str += sensor_platform->second->Name.c_str();
        str += " ";
    }else{
        str += "NAN";
        return;
    }
    // if(m_Specialzones->find(key) != m_Specialzones->end()){
    //     auto ssz = m_Specialzones->at(key);
    //     auto srd = eo_ir->eo_surrounding;
    //     srd.atmCoeff      = 1;
    //     srd.rainCoeff     = ssz->rain;
    //     srd.fogCoeff      = ssz->fog;
    //     srd.humidityCoeff = ssz->humidity;
    //     eo_ir->eo->setEnvironment(srd);
    // }else{
    //     eo_ir->eo->setEnvironment({1,1,1,1});
    // }

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
        if(m_Specialzones->find(key) != m_Specialzones->end()){
            auto ssz = m_Specialzones->at(key);
            // eo_surrounding = getSurrounding(ssz);
            eo_surrounding = getSurrounding(ssz->collider->CollideRadius,1,ssz->rain,ssz->fog,ssz->humidity);
        }else{
            eo_surrounding = getSurrounding();
        }
        // Display Values
        str += QString("Env: [A:%1, R:%2, H:%3, F:%4 ]").arg(
            QString::number(eo_surrounding.k_atm ),
            QString::number(eo_surrounding.k_rain),
            QString::number(eo_surrounding.k_fog ),
            QString::number(eo_surrounding.k_fog ));

        // Setting Up Sensor
        auto eo_sensor      = eo_ir->eo_sensor;
        // Initializing EO Sensor
        eo = eo_ir->initEO(eo_sensor,eo_surrounding);

    }else{
        return;
    }
    /*            Defining the EO Sensor End             */


    /*               Dynamic Range Start                 */
    range = eo->getMaxRange(80)/1000.f;
    /*                Dynamic Range End                  */

    /*     Getting Sensored Entity Coordinates Start     */
    Coordinate sensor_cood =
        {source->getLatitude(),
         source->getLongitude(),
         source->getAltitude()};
    /*      Getting Sensored Entity Coordinates End      */

    str += "Contains: {";
    ewtargets.clear();
    for (auto& [ID, platform] : *m_Platforms){
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

    debug(str,D_INIT);


    // // C# foreach (Transform tr in targets) -> C++ range-based for loop
    // for (auto& [key, entity] : *root->Radios)
    // {
    //     if(!entity || !entity->parentEntity || !entity->parentEntity->Active) continue;
    //     auto it = root->Platforms->find(entity->parentEntity->ID);
    //     if (it != root->Platforms->end()) {
    //         Platform* platform = it->second;
    //         // qDebug() << "[Sensor::ewscan] iterating entity:" << QString::fromStdString(key);
    //         if(platform->ID == parentEntity->ID || !platform || !platform->transform) continue;
    //         QVector3D localPos = source->inverseTransformPoint(platform->transform->matrix->translation());
    //         //float distance = localPos.length();
    //         float metredis = distanceBetween(source->getLatitude(),source->getLongitude(),platform->transform->getLatitude(),platform->transform->getLongitude())/1000;

    //         // horizontal angle (Y axis) : x vs z
    //         float yAngle = std::atan2(localPos.x(), localPos.z()) * RAD2DEG;
    //         float fre1 = entity->minFrequency;
    //         float fre2 = entity->maxFrequency;
    //         // qDebug()<<yAngle<<","<<detectCheck(localPos,metredis)<<","<<(fre1 < frequency && fre2> frequency);
    //         if (entity->Active && detectCheck(localPos,metredis) && fre1 < frequency && fre2> frequency)  // .position() is assumed
    //         {
    //             //qDebug()<< "detect";
    //             if (ewdetects.count(platform) == 0)
    //             {
    //                 ewdetects.insert(platform);
    //                 Target target;
    //                 target.entity = platform;
    //                 target.angle = yAngle;
    //                 target.radius = metredis;
    //                 ewtargets.append(target);
    //             }else{
    //                 for (int i = 0; i < ewtargets.size(); ++i) {
    //                     if (ewtargets.at(i).entity == platform) {
    //                         ewtargets[i].angle = yAngle;
    //                         ewtargets[i].radius = metredis;
    //                         break;
    //                     }
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             if (ewdetects.count(platform) > 0)
    //             {
    //                 for (int i = 0; i < ewtargets.size(); ++i) {
    //                     if (ewtargets.at(i).entity == platform) {
    //                         ewtargets.removeAt(i);
    //                         break;
    //                     }
    //                 }
    //                 ewdetects.erase(platform);
    //             }
    //         }
    //     }
    // }
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
