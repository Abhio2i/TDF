// =============================================================================
// FILE:        iff.h
// MODULE:      Tactical Simulation Sensor Systems
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the IFF (Identification Friend or Foe) class. This module
//               handles electronic interrogation and response protocols,
//               managing military/civilian modes (Mode 1-4, C) and encryption.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Sep 2025  Initial implementation for TDF project.
//   Rev 2  Mar 2026  Integrated transponder logic and target tracking.
//   Rev 3  Apr 2026  Added DO-178C compliant documentation and contact signals.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef IFF_H
#define IFF_H

#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <string>
#include <unordered_set>

// =============================================================================
// CLASS: IFF
//
// DESCRIPTION: Concrete implementation of a tactical sensor entity. Manages
//              interrogation cycles, response logic for transponders, and
//              maintains a registry of detected friend/foe contacts.
// =============================================================================
class IFF : public Entity
{
    Q_OBJECT
public:
    explicit IFF(Hierarchy* h);

    // =========================================================================
    // SECTION: Enumerations & Data Structures
    // DESCRIPTION: Definitions for operational modes, encryption, and
    //              interrogation response packets.
    // =========================================================================
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

    struct IFFResponse {
        std::string interrogatorId;
        std::string interrogatorName;
        std::string responderId;
        std::string responderName;
        std::string mode;
        std::string code;
        float distanceMeters;
        std::string timestamp;
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
        std::string timeStamp;
        std::string source;
        std::string destination;
        std::string content;
    };

    // =========================================================================
    // SECTION: System Attributes & State
    // DESCRIPTION: Physical and logical parameters governing IFF behavior.
    // =========================================================================
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

    // =========================================================================
    // SECTION: Target & Contact Tracking
    // DESCRIPTION: Containers for detected entities and interrogation logs.
    // =========================================================================
    std::vector<Message> messages;
    QVector<IFFTarget> iffTargets;
    std::unordered_set<std::string> localIffSeen;
    std::unordered_set<Platform*> detects;
    QVector<IFFTarget> targets;

    // =========================================================================
    // SECTION: Interrogation API
    // DESCRIPTION: Methods for active scanning and automated responding.
    // =========================================================================
    void interrogateTargets(Transform* source);
    QJsonObject respondToInterrogation(IFF* interrogator, float distanceMeters);
    void scan();
    int getIFFTargetCount() const;

    bool getIFFTarget(
        int index,
        std::string& outResponderId,
        std::string& outResponderName,
        std::string& outMode,
        std::string& outCode,
        float& outDistance,
        float& outAngle,
        int& outStatus
        ) const;

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Entity lifecycle and component system integration.
    // =========================================================================
    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    // =========================================================================
    // SECTION: Serialization
    // DESCRIPTION: Persistence logic for IFF configuration and state.
    // =========================================================================
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

signals:
    // =========================================================================
    // SECTION: Signals
    // DESCRIPTION: Notifications for UI updates and network synchronization.
    // =========================================================================
    void iffContactsUpdated(const QJsonArray& responses);

private:
    // =========================================================================
    // SECTION: Internal Utilities
    // DESCRIPTION: Helper methods for string conversion and status tracking.
    // =========================================================================
    QString operationalModeToString(OperationalMode om) const;
    OperationalMode stringToOperationalMode(const QString& str) const;
    QString codeSystemToString(CodeSystem cs) const;
    CodeSystem stringToCodeSystem(const QString& str) const;
    QString encryptionTypeToString(EncryptionType et) const;
    EncryptionType stringToEncryptionType(const QString& str) const;
    bool interrogationDone = false;
};

#endif // IFF_H
