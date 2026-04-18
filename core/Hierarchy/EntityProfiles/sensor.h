


#ifndef SENSOR_H
#define SENSOR_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <unordered_set>
#include <vector>

struct Target{
    Platform *entity;
    float radius;
    float angle;
    float speed;
    float direction;
    float altitude;
    float lat;
    float lon;
    float radialVelocity = 0.0f;  // ADD THIS — m/s, + = closing, - = opening

};
// Perspective of User
struct EO_Entity{
    struct Vec2 {
        double x = 0, y = 0;

        Vec2(double x_ = 0, double y_ = 0)
            : x(x_), y(y_) {}

        Vec2(const Vec2& _vec2)
            : x(_vec2.x), y(_vec2.y) {}
    };
    struct Vec3 {
        double x = 0, y = 0, z = 0;

        Vec3(double x_ = 0, double y_ = 0,double z_ = 0)
            : x(x_), y(y_), z(z_) {}

        Vec3(const Vec3& _vec3)
            : x(_vec3.x), y(_vec3.y), z(_vec3.z) {}
    };
    std::string name;
    Vec2 vec2;
    Vec3 vec3;
    double relativeHeading;
    double relativePitch;
    double size;
    EO_Entity(std::string _name = "",
              double _size = 0,
              double _relativeHeading = 0.00f,
              double _relativePitch = 0.00f,
              const Vec2& _vec2 = Vec2(),
              const Vec3& _vec3 = Vec3())
        : name(_name),vec2(_vec2),vec3(_vec3),relativeHeading (_relativeHeading),
        relativePitch (_relativePitch), size(_size){}

    // struct ViewDir{
    //     double x = 0;
    //     double y = 0;
    //     double z = 0;
    //     ViewDir(double _x,double _y,double _z):
    //         x(_x),y(_y),z(_z){};
    // };
    // double screenX = 0;
    // double screenY = 0;
    // double distance      = 0;
    // double angle         = 0;
    // double projectionArea= 0;
    // ViewDir viewDir;
    // EO_Entity(double _distance, double _angle, double _projectionArea, ViewDir _viewDir)
    //     :distance(_distance),angle(_angle),projectionArea(_projectionArea), viewDir(_viewDir){};
};
struct EO_IR_State{
    struct coordinate{
        double x =0;
        double y =0;
        coordinate(double _x,double _y):
            x(_x),y(_y){};
    };
    struct dimension{
        double x = 1;
        double y = 1;
        dimension(double _x,double _y):
            x(_x),y(_y){};
        coordinate center(){
            coordinate coord(x/2, y/2);
            return coord;
        }
    };
    struct vanishingPodouble{
        double x = 0;
        double y = 0;
        vanishingPodouble(double _x,double _y):
            x(_x),y(_y){};
    };
    struct horizonMidPodouble{
        double x = 0;
        double y = 0;
        horizonMidPodouble(double _x,double _y):
            x(_x),y(_y){};
    };

    EO_Entity eo_seneor_entity;
    std::vector<EO_Entity> entityList;

};

// struct Target{
//     Platform *entity;
//     float radius;
//     float angle;
//     float speed;
//     float direction;
//     float altitude;
//     float lat;
//     float lon;
// };

class Sensor : public Entity
{
    Q_OBJECT
public:
    explicit Sensor(Hierarchy* h);

    // Sensor attributes
    enum class Type { Active, Passive };
    enum class Mode { Search, Track, TrackWhileScan, FireControl };
    enum class SubType { Generic, CSM, ESM,EO,Sonar,AIS, ADSB , AESA };
    enum class DetectionCapabilities { All, MovingOnly };
    Q_ENUM(DetectionCapabilities);
    struct Detection {
        struct GeoCoords {
            double latitude;
            double longitude;
            double altitude;
            double heading;
        } geoCoords;
        struct Velocity {
            double x;
            double y;
            double z;
        } velocity;
        std::string entityReference;
        float signalStrength;
        float detectionConfidence;
    };
    struct Message {
        std::string timeStamp;
        std::string source;
        std::string destination;
        std::string content;
    };
    Type sensortype = Type::Active;
    Mode mode = Mode::Search;
    // --- Add below Mode enum ---
    SubType subType = SubType::Generic;
    DetectionCapabilities capabilities = DetectionCapabilities::All;
    Entity* parentEntity = nullptr;
    float frequency = 8;//ghz
    float azimuth = 60;//deg
    bool csmEnabled = false;
    bool esmEnabled = false;
    float emissionPower = 0.0f; // Watts
    float emissionFrequency = 0.0f; // MHz or GHz
    float bandwidth = 0.0f; // MHz
    float pulseWidth = 0.0f; // microseconds
    float prf = 0.0f; // Hz (Pulse Repetition Frequency)
    float scanningRate = 0.0f; // degrees/sec
    float beamWidth = 0.0f; // degrees
    float antennaGain = 0.0f; // dBi
    float maxDetectionAngle = 60.0f; // degrees
    float range = 100.0f; // km or meters
    // float ewrange = 100.0f; // km or meters
    float refreshRate = 0.0f; // Hz
    float noiseFigure = 0.0f; // dB
    bool clutterRejection = false;
    bool eccmCapability = false;
    std::vector<Detection> detections;
    std::unordered_set<Platform*> detects;
    QVector<Target> targets;
    std::unordered_set<Platform*> ewdetects;
    QVector<Target> ewtargets;
    std::vector<Message> messages;
    // --- CSM / ESM Specific ---
    std::unordered_set<Platform*> csmdetects;
    QVector<Target> csmtargets;
    std::unordered_set<Platform*> esmdetects;
    QVector<Target> esmtargets;
    EO_Entity eoSensor;
    QVector<EO_Entity> eoEntities;
    // --- Generic Radar Targets ---
    int getTargetCount() const;                  // Number of Generic targets
    Target getTarget(int index) const;           // Get Generic target by index

    // --- CSM Targets ---
    int getCSMTargetCount() const;               // Number of CSM targets
    Target getCSMTarget(int index) const;        // Get CSM target by index

    // --- ESM Targets ---
    int getESMTargetCount() const;               // Number of ESM targets
    Target getESMTarget(int index) const;        // Get ESM target by index


    float csmrange = 5000.0f;
    float esrange = 8000.0f;
    float csmSensitivity = 1.0f;
    float esmSensitivity = 1.0f;
    bool csmActive = true;
    bool esmActive = true;
    virtual void scan();
    void clearTargets();
    bool detectCheck(QVector3D localPos, float distance, float multi = 1.0f);
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    static SubType getSubTypeFromString(const QString& str) {
        if (str == "CSM") return SubType::CSM;
        if (str == "ESM") return SubType::ESM;
        if (str == "Sonar") return SubType::Sonar;
        if (str == "AESA") return SubType::AESA;
        return SubType::Generic;
    }
    QString detectionCapabilitiesToString(DetectionCapabilities t) const;
    Sensor::DetectionCapabilities stringTodetectionCapabilities(const QString& str) const;
    QString subTypeToString(SubType t) const;
    SubType stringToSubType(const QString& str) const;

signals:
    void availableConnectionsUpdated(const QJsonArray& msgArray);
    void enemyDetected();
    void enemyNotFound();
private:
    bool enemyAvailabel = false;
    QString modeToString(Mode m) const;
    Mode stringToMode(const QString& str) const;
};

#endif // SENSOR_H
QStringList DetectionCapabilitiesTypeOptions();
