


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
};

class Sensor : public Entity
{
    Q_OBJECT
public:
    explicit Sensor(Hierarchy* h);

    // Sensor attributes
    enum class Type { Active, Passive };
    enum class Mode { Search, Track, TrackWhileScan, FireControl };

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

    Type type = Type::Active;
    Mode mode = Mode::Search;
    float emissionPower = 0.0f; // Watts
    float emissionFrequency = 0.0f; // MHz or GHz
    float bandwidth = 0.0f; // MHz
    float pulseWidth = 0.0f; // microseconds
    float prf = 0.0f; // Hz (Pulse Repetition Frequency)
    float scanningRate = 0.0f; // degrees/sec
    float beamWidth = 0.0f; // degrees
    float antennaGain = 0.0f; // dBi
    float detectionCapabilities = 0.0f; // Quality Score or Signal Processing Strength
    float maxDetectionAngle = 60.0f; // degrees
    float range = 100.0f; // km or meters
    float ewrange = 100.0f; // km or meters
    float refreshRate = 0.0f; // Hz
    float noiseFigure = 0.0f; // dB
    bool clutterRejection = false;
    bool eccmCapability = false;
    std::vector<Detection> detections;
    std::unordered_set<Platform*> detects;
    QVector<Target> targets;
    std::unordered_set<Platform*> ewdetects;
    QVector<Target> ewtargets;
    void scan(std::string id, Transform *source);
    void ewscan(std::string id , Transform *source);
    bool detectCheck(QVector3D localPos);

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    QString modeToString(Mode m) const;
    Mode stringToMode(const QString& str) const;
};

#endif // SENSOR_H
