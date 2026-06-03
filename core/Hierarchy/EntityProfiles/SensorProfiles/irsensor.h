#ifndef IRSENSOR_H
#define IRSENSOR_H

#include "eoSensorLib/eo.h"
#include "eoSensorLib/eo_ir.h"
#include <QObject>

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/profilecategaory.h"
#include "eoSensorLib/eovision.h"
#include "eoSensorLib/georelativeangle.h"
#include "eoSensorLib/georelativeelevation.h"
// #include "eoSensorLib/payload.h"
#define M_PI 3.14159265358979323846



class IRSensor: public Sensor
{
    Q_OBJECT
public:
    Hierarchy* m_h;
    EOVision eoVision;
    GeoRelativeAngle gra;
    GeoRelativeElevation gre;
    explicit IRSensor(Hierarchy* h);
    std::unordered_map<std::string, Platform*> m_Platforms;
    std::unordered_map<std::string, Specialzone*> m_Specialzones;

    Surrounding getSurrounding(float radius   = 10000.0,
                               float atmCoeff = 0.00001,  // %
                               float rainRate = 0.0,      // mm/h
                               float fog      = 10000.0,  // %
                               float humidity = 0.5       // %
                               );
    EO_IR* eo_ir;
    EO* eo = nullptr;

    // Helper: Convert degrees to radians
    double deg2rad(double degrees);
    // Helper function to convert degrees to radians
    double degToRad(double degrees);
    // Helper: Geodetic (Lat, Lon, Alt) to ECEF (X, Y, Z)
    void GeodeticToECEF(double lat, double lon, double alt, double &X, double &Y, double &Z);
    // Main Function: Calculate relative OpenGL coordinates and rotations
    PoseOpenGL CalculateRelativeOpenGLPose(const PoseGeo& target, const PoseGeo& sensor);
    /*
     * Calculates the relative X and Y coordinates of a target given a sensor's position.
     * * @param sensor The latitude and longitude of the sensor (origin)
     * @param target The latitude and longitude of the target
     * @return CartesianCoord containing the relative X and Y distances in meters
     */
    // CartesianCoord getRelativeCoordinates(GeoCoord sensor, GeoCoord target);
    CartesianCoord getRelativeCoordinatesGL(GeoCoord sensor, GeoCoord target);
    /*               Get Resolution Angle Start         */

    float getVeticalAngle(std::pair<int,int> resolution);
    float distance         = 0;
    float verticalAngle    = 0;
    float horizonatalAngle = 0;

    /*                Get Resolution Angle End          */

    /*                Under the parameter or not        */
    bool isUnderRangeNAngle(float t_distance, float t_verticalAngle, float t_horizonatalAngle);

    /*                      Detection                   */
    QString BoolToStr[2] = {"False","True"};

    // ==========================================
    // 2. Detection Probability Formula
    // ==========================================

    float calculateDetectionProbability(const EOIR_Environment& env, const EOIR_Target& tgt, const EOIR_Sensor& sensor);

    ScreenTarget getTargetScreenBoundingBox(
        Resolutions res,
        float sensorHorizontalFovDeg,
        float sensorVerticalFovDeg,
        float targetHorizontalAngleDeg,
        float targetVerticalAngleDeg,
        float targetDistanceMeters,      // NEEDED FOR SIZE
        float targetPhysicalWidthMeters, // NEEDED FOR SIZE
        float targetPhysicalHeightMeters); // NEEDED FOR SIZE
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

    /*---------------   Resolution Start   ------------------*/
    Resolutions currentResolution = Resolutions::FourK;

    /*----------------   Resolution End   -------------------*/


    /*-------------   Selection List Start   ----------------*/
    std::unordered_map<QString,EOImageType> EOImageMap = {
        {"B-21_Raider",                 EOImageType::One_Engine},
        {"BAEHawkmk132",                EOImageType::One_Engine},
        {"BayraktarTB2",                EOImageType::One_Engine},
        {"BoeingX-37B",                 EOImageType::One_Engine},
        {"ChengduJ-10",                 EOImageType::One_Engine},
        {"DassaultMirage2000",          EOImageType::One_Engine},
        {"F-16Falcon",                  EOImageType::One_Engine},
        {"F-16V",                       EOImageType::One_Engine},
        {"F-35Lightning",               EOImageType::One_Engine},
        {"HALHF-24Marut",               EOImageType::One_Engine},
        {"HALHJT-16Kiran",              EOImageType::One_Engine},
        {"HALHJT-36Sitara",             EOImageType::One_Engine},
        {"HALTejas",                    EOImageType::One_Engine},
        {"HALTejasMk2",                 EOImageType::One_Engine},
        {"HawkerHunter",                EOImageType::One_Engine},
        {"JF-17Thunder",                EOImageType::One_Engine},
        {"KAIFA-50",                    EOImageType::One_Engine},
        {"L-39Albatros",                EOImageType::One_Engine},
        {"MIG-21",                      EOImageType::One_Engine},
        {"Mikoyan-GurevichMiG-23",      EOImageType::One_Engine},
        {"MikoyanMiG-27",               EOImageType::One_Engine},
        {"MitsubishiF-2",               EOImageType::One_Engine},
        {"PilatusPC-7",                 EOImageType::One_Engine},
        {"SaabJAS39Gripen",             EOImageType::One_Engine},
        {"SEPECATJaguar",               EOImageType::One_Engine},
        {"T-6Texan",                    EOImageType::One_Engine},
        {"T-38Talon",                   EOImageType::One_Engine},
        {"Yak-130",                     EOImageType::One_Engine},
        {"AntonovAn-32",                EOImageType::Two_Engine},
        {"ATR-72MP",                    EOImageType::Two_Engine},
        {"B-2_Spirit",                  EOImageType::Two_Engine},
        {"BeechcraftKingAir350",        EOImageType::Two_Engine},
        {"ChengduJ-20",                 EOImageType::Two_Engine},
        {"DassaultRafale",              EOImageType::Two_Engine},
        {"Dornier228",                  EOImageType::Two_Engine},
        {"EurofighterTyphoon",          EOImageType::Two_Engine},
        {"F-14Tomcat",                  EOImageType::Two_Engine},
        {"F-15Eagle",                   EOImageType::Two_Engine},
        {"F-18SuperHornet",             EOImageType::Two_Engine},
        {"F-22Raptor",                  EOImageType::Two_Engine},
        {"J-16",                        EOImageType::Two_Engine},
        {"J-20",                        EOImageType::Two_Engine},
        {"J-20MightyDragon",            EOImageType::Two_Engine},
        {"MIG-29",                      EOImageType::Two_Engine},
        {"MikoyanMIG-35",               EOImageType::Two_Engine},
        {"NALSaras",                    EOImageType::Two_Engine},
        {"Netra(AEW&C)",                EOImageType::Two_Engine},
        {"ShenyangJ-16",                EOImageType::Two_Engine},
        {"SR-71_Blackbid",              EOImageType::Two_Engine},
        {"SukhoiSu-25",                 EOImageType::Two_Engine},
        {"SukhoiSu-30MKI",              EOImageType::Two_Engine},
        {"SukhoiSu-34",                 EOImageType::Two_Engine},
        {"SukhoiSu-57",                 EOImageType::Two_Engine},
        {"Tu-160",                      EOImageType::Two_Engine},
        {"A-50Phalcon",                 EOImageType::Air_Bus},
        {"AirbusA330MRTT",              EOImageType::Air_Bus},
        {"AirbusA400MAtlas",            EOImageType::Air_Bus},
        {"AntonovAn-124",               EOImageType::Air_Bus},
        {"AntonovAn-225",               EOImageType::Air_Bus},
        {"AvroLancaster",               EOImageType::Air_Bus},
        {"BoeingC-17GlobemasterIII",    EOImageType::Air_Bus},
        {"BoeingE-3Sentry",             EOImageType::Air_Bus},
        {"BoeingE-7",                   EOImageType::Air_Bus},
        {"BoeingKC-135Stratotanker",    EOImageType::Air_Bus},
        {"BoeingP-8Poseidon",           EOImageType::Air_Bus},
        {"C-130JSuperHercules",         EOImageType::Air_Bus},
        {"DouglasDC-3",                 EOImageType::Air_Bus},
        {"E-2Hawkeye",                  EOImageType::Air_Bus},
        {"E-2KHawkeye(AWACS)",          EOImageType::Air_Bus},
        {"E-3Sentry(AWACS)",            EOImageType::Air_Bus},
        {"EADSCASAC-295",               EOImageType::Air_Bus},
        {"EmbraerKC-390",               EOImageType::Air_Bus},
        {"EnglishElectricCanberra",     EOImageType::Air_Bus},
        {"FairchildC-119FlyingBoxcar",  EOImageType::Air_Bus},
        {"GulfstreamG550",              EOImageType::Air_Bus},
        {"IlyushinIl-76",               EOImageType::Air_Bus},
        {"KJ-500(AWACS)",               EOImageType::Air_Bus},
        {"KJ-2000 ",                    EOImageType::Air_Bus},
        {"LockheedP-3Orion",            EOImageType::Air_Bus},
        {"P-3COrion",                   EOImageType::Air_Bus},
        {"PhalconAWACS",                EOImageType::Air_Bus},
        {"Saab2000Erieye",              EOImageType::Air_Bus},
        {"ShinMaywaUS-2",               EOImageType::Air_Bus},
        {"Tu-95Bomber",                 EOImageType::Air_Bus},

        {"AgustaWestlandAW101",         EOImageType::None},
        {"BellAH-1Cobra",               EOImageType::None},
        {"BoeingAH-64Apache",           EOImageType::None},
        {"BoeingCH-47",                 EOImageType::None},
        {"CAIGWingLoongII",             EOImageType::None},
        {"DRDOGhatak",                  EOImageType::None},
        {"DRDOImperialEagle",           EOImageType::None},
        {"DRDOLakshya"  ,               EOImageType::None},
        {"DRDONishant"  ,               EOImageType::None},
        {"DRDORustom-2" ,               EOImageType::None},
        {"HALDhruv"     ,               EOImageType::None},
        {"HALLightCombatHelicopter",    EOImageType::None},
        {"HALPrachand",                 EOImageType::None},
        {"HALRudra"   ,                 EOImageType::None},
        {"HESAShahed136",               EOImageType::None},
        {"IAIHeron",                    EOImageType::None},
        {"IAISearcher",                 EOImageType::None},
        {"KamovKa-31" ,                 EOImageType::None},
        {"MilMi-17",                    EOImageType::None},
        {"MilMi-24",                    EOImageType::None},
        {"MilMi-26",                    EOImageType::None},
        {"MQ-1Predator"    ,            EOImageType::None},
        {"MQ-9BSeaGuardian",            EOImageType::None},
        {"MQ-9Reaper"      ,            EOImageType::None},
        {"SikorskyUH-60BlackHawk",      EOImageType::None},
        {"TAPAS-BH-201",                EOImageType::None},
        {"WZ-7SoaringDragon",           EOImageType::None},


        /*
        {"AgustaWestlandAW101",         EOImageType::One_Engine},
        {"BellAH-1Cobra",               EOImageType::One_Engine},
        {"BoeingAH-64Apache",           EOImageType::One_Engine},
        {"BoeingCH-47",                 EOImageType::One_Engine},
        {"CAIGWingLoongII",             EOImageType::One_Engine},
        {"DRDOGhatak",                  EOImageType::One_Engine},
        {"DRDOImperialEagle",           EOImageType::One_Engine},
        {"DRDOLakshya"  ,               EOImageType::One_Engine},
        {"DRDONishant"  ,               EOImageType::One_Engine},
        {"DRDORustom-2" ,               EOImageType::One_Engine},
        {"HALDhruv"     ,               EOImageType::One_Engine},
        {"HALLightCombatHelicopter",    EOImageType::One_Engine},
        {"HALPrachand",                 EOImageType::One_Engine},
        {"HALRudra"   ,                 EOImageType::One_Engine},
        {"HESAShahed136",               EOImageType::One_Engine},
        {"IAIHeron",                    EOImageType::One_Engine},
        {"IAISearcher",                 EOImageType::One_Engine},
        {"KamovKa-31" ,                 EOImageType::One_Engine},
        {"MilMi-17",                    EOImageType::One_Engine},
        {"MilMi-24",                    EOImageType::One_Engine},
        {"MilMi-26",                    EOImageType::One_Engine},
        {"MQ-1Predator"    ,            EOImageType::One_Engine},
        {"MQ-9BSeaGuardian",            EOImageType::One_Engine},
        {"MQ-9Reaper"      ,            EOImageType::One_Engine},
        {"SikorskyUH-60BlackHawk",      EOImageType::One_Engine},
        {"TAPAS-BH-201",                EOImageType::One_Engine},
        {"WZ-7SoaringDragon",           EOImageType::One_Engine},
        */
    };
    std::unordered_map<EOImageType,QString>imageTypeToName = {
                                                                {One_Engine,"One Engine"},
                                                                {Two_Engine,"Two Engine"},
                                                                {Air_Bus   ,"Air Bus"},
                                                                {None      ,"Not Available"},
                                                                };

    // Environment

    float relativeHumidity     = 30   ;       // % (0.0 to 100.0)
    float absoluteHumidity     = 10.0 ;       // g/m^3
    float rainfallRate         = 0.0  ;       // mm/hr
    float snowfallEquivalent   = 0.0  ;       // mm/hr
    float ambientTemp          = 30   ;       // Celsius
    float backgroundTemp       = 20.0 ;       // Celsius
    float aerosolConcentration = 0.05 ;       // mg/m^3
    float baseExtinctionCoeff  = 0.15 ;       // Base sigma (1/km)
    float ambientIlluminance   = 50000;       // lux
    float solarIrradiance      = 800  ;       // W/m^2 (Crucial for Glint)

    void setEnvironment(float relativeHumidity,
                        float absoluteHumidity,
                        float rainfallRate,
                        float snowfallEquivalent,
                        float ambientTemp,
                        float backgroundTemp,
                        float aerosolConcentration,
                        float baseExtinctionCoeff,
                        float ambientIlluminance,
                        float solarIrradiance    );

    void setEnvironmentToDefault();
    // Sensor
    float slantRange      = 5;     // km
    float mrtd            = 0.05;  // Minimum Resolvable Temp Difference (Celsius)
    float sunPhaseAngle   = 90;    // Degrees (0 = perfect alignment for direct reflection into sensor)
    float saturationLimit = 50;    // Max apparent delta-T before the sensor blooms/blinds (Celsius)


    /*--------------   Selection List End  ------------------*/
    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;
    QString mstr;
    QString ostr;
    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b10000000000000,
        D_JustPrint       = 0b01000000000000,
        D_INIT            = 0b00100000000000,
        D_Details         = 0b00010000000000,
        D_Mesh            = 0b00001000000000,
        D_Geometry        = 0b00000100000000,
        D_OpenGL          = 0b00000010000000,
    }debugSQLite;
    Q_ENUM(debugSQLite)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugSQLite &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                    // | D_Geometry
                    // | D_Mesh
                    // | D_Details
                    // | D_INIT
                    | D_OpenGL
        ;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugSQLite &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};

#endif // IRSENSOR_H
