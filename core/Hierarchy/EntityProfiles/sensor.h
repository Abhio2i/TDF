


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
    enum class SubType { Generic, CSM, ESM };
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
    Type type = Type::Active;
    Mode mode = Mode::Search;
    // --- Add below Mode enum ---
    SubType subType = SubType::Generic;
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
    std::vector<Message> messages;
    // --- CSM / ESM Specific ---
    std::unordered_set<Platform*> csmdetects;
    QVector<Target> csmtargets;
    std::unordered_set<Platform*> esmdetects;
    QVector<Target> esmtargets;

    float csmrange = 5000.0f;
    float esrange = 8000.0f;
    float csmSensitivity = 1.0f;
    float esmSensitivity = 1.0f;
    bool csmActive = true;
    bool esmActive = true;
    void scan(std::string id, Transform *source);
    void ewscan(std::string id , Transform *source);
    bool detectCheck(QVector3D localPos);
    void csmScan(std::string id, Transform* source);
    void esmScan(std::string id, Transform* source);
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
        return SubType::Generic;
    }
    QString subTypeToString(SubType t) const;
    SubType stringToSubType(const QString& str) const;
signals:
    void availableConnectionsUpdated(const QJsonArray& msgArray);
private:
    QString modeToString(Mode m) const;
    Mode stringToMode(const QString& str) const;
};

#endif // SENSOR_H
