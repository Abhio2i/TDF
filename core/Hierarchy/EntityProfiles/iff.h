#ifndef IFF_H
#define IFF_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <unordered_set>

class IFF : public Entity
{
    Q_OBJECT
public:
    explicit IFF(Hierarchy* h);
    // static std::unordered_set<std::string> iffSeen;
    // IFF attributes
    enum class OperationalMode { Active, Passive, Off, Simulation };
    enum class CodeSystem { NoPulse, FivePulses, SixPulses, TwelvePulses };
    enum class EncryptionType { None, NATO, SecureID };

    struct ModeConfiguration {
        std::string mode1 = "0001"; // Military ID code
        std::string mode2 = "0000"; // Mission code
        std::string mode3A = "1200"; // Civilian squawk code
        std::string mode4 = "ABCD"; // Secure IFF code
        std::string modeC = "0000"; // Altitude code
    };
    // --- add near other public structs / before existing public methods ---
    struct IFFResponse {
        std::string interrogatorId;
        std::string interrogatorName;
        std::string responderId;
        std::string responderName;
        std::string mode;       // e.g., "Mode3A", "ModeC", "Mode4"
        std::string code;       // the code replied (squawk, ID, etc.)
        float distanceMeters;   // distance in meters
        std::string timestamp;  // ISO string
    };
    struct IFFTarget {
        float distance = 0.0f;
        std::string responderId;
        std::string responderName;
        std::string mode;
        std::string code;
        bool ally;
        float radius;
        float angle;
        int status;
        Platform* entity;
    };
    struct Message {
        std::string timeStamp;      // Timestamp of the event
        std::string source;         // Entity name or ID initiating the communication
        std::string destination;    // Entity name or ID receiving it
        std::string content;        // Description of what happened (e.g., interrogation, response, etc.)
    };

    // Interrogation/response API
    void interrogateTargets(Transform* source); // actively interrogate nearby entities
    QJsonObject respondToInterrogation(IFF* interrogator, float distanceMeters); // called when this IFF is interrogated
    Entity* parentEntity = nullptr;
    bool transponder = true;
    float emittingRange = 10.0f; // km
    float emittingFrequency = 0.0f; // MHz
    int code = 1001;
    std::string disType;
    std::string disName;
    OperationalMode operationalMode = OperationalMode::Active;
    ModeConfiguration modeConfiguration;
    CodeSystem codeSystem = CodeSystem::NoPulse;
    EncryptionType encryptionType = EncryptionType::None;
    bool spoofable = true;
    float responseDelay = 50.0f; // ms
    std::string lastInterrogationTime;
    std::vector<Message> messages;
    QVector<IFFTarget> iffTargets;
    void spawn() override;
    void scan();
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;
    // ✅ ADD INSTANCE-SPECIFIC SET
    std::unordered_set<std::string> localIffSeen;
    // Track last known state per responder to detect status/code/mode changes
    std::unordered_set<Platform*> detects;
    QVector<IFFTarget> targets;
    // std::unordered_map<std::string, IFFTarget> lastTargetStates;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
signals:
    void iffContactsUpdated(const QJsonArray& responses); // UI/listeners get QJsonArray of results
private:
    QString operationalModeToString(OperationalMode om) const;
    OperationalMode stringToOperationalMode(const QString& str) const;
    QString codeSystemToString(CodeSystem cs) const;
    CodeSystem stringToCodeSystem(const QString& str) const;
    QString encryptionTypeToString(EncryptionType et) const;
    EncryptionType stringToEncryptionType(const QString& str) const;
    bool interrogationDone = false;   // <-- Add this
};

#endif // IFF_H
