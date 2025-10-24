

#ifndef IFF_H
#define IFF_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>

class IFF : public Entity
{
    Q_OBJECT
public:
    explicit IFF(Hierarchy* h);

    // IFF attributes
    enum class OperationalMode { Active, Passive, Off, Simulation };
    enum class CodeSystem { NoPulse, FivePulses, SixPulses, TwelvePulses };
    enum class EncryptionType { None, NATO, SecureID };

    struct ModeConfiguration {
        std::string mode1; // Military ID code
        std::string mode2; // Mission code
        std::string mode3A; // Civilian squawk code
        std::string mode4; // Secure IFF code
        std::string modeC; // Altitude code
    };

    bool transponder = false;
    float emittingRange = 0.0f; // km
    float emittingFrequency = 0.0f; // MHz
    std::string disType;
    std::string disName;
    OperationalMode operationalMode = OperationalMode::Off;
    ModeConfiguration modeConfiguration;
    CodeSystem codeSystem = CodeSystem::NoPulse;
    EncryptionType encryptionType = EncryptionType::None;
    bool spoofable = false;
    float responseDelay = 0.0f; // ms
    std::string lastInterrogationTime;

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

private:
    QString operationalModeToString(OperationalMode om) const;
    OperationalMode stringToOperationalMode(const QString& str) const;
    QString codeSystemToString(CodeSystem cs) const;
    CodeSystem stringToCodeSystem(const QString& str) const;
    QString encryptionTypeToString(EncryptionType et) const;
    EncryptionType stringToEncryptionType(const QString& str) const;
};

#endif // IFF_H
