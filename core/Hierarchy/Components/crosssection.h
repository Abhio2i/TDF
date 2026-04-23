// =============================================================================
// FILE:        crosssection.h
// MODULE:      Tactical Simulation Signature Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION:  Defines the CrossSection class, a component responsible for
//               managing the multi-spectral signatures of an entity. It stores
//               Radar Cross Section (RCS), Visual, Infrared, Sonar, and Laser
//               return values used for detection probability calculations.
//
// AUTHOR:       Pankaj Chauhan
// REVIEWED BY:  [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  Nov 2025  Initial implementation for TDF project.
//   Rev 2  Apr 2026  Aligned with DO-178C documentation standards for O2I.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef CROSSSECTION_H
#define CROSSSECTION_H

#include "./component.h"
#include <QObject>
#include <QJsonObject>

// =============================================================================
// CLASS: CrossSection
//
// DESCRIPTION: Component-based signature manager. Defines uniform and
//              modulated values for different sensor spectrums to determine
//              the observability of the parent entity.
// =============================================================================
class CrossSection: public QObject, public Component
{
    Q_OBJECT
public:
    // =========================================================================
    // SECTION: Internal Data Structures
    // DESCRIPTION: Structure to hold signature values for a specific spectrum.
    // =========================================================================
    struct data {
        float uniformedValue = 100.00;
        float modulationValue = 0.00;
    };

    CrossSection();

    // =========================================================================
    // SECTION: Component Identity
    // DESCRIPTION: Runtime type identification for the signature system.
    // =========================================================================
    ComponentType Typo() const override { return ComponentType::CrossSection; }

    // =========================================================================
    // SECTION: Signature Attributes
    // DESCRIPTION: Multi-spectral signature data points.
    // =========================================================================
    data Radar;
    data Visual;
    data Infrared;
    data Sonar;
    data Laser;

    // =========================================================================
    // SECTION: Parameter Management
    // DESCRIPTION: Storage for custom key-value pairs associated with signatures.
    // =========================================================================
    QJsonObject AdditionalParameters;

    // =========================================================================
    // SECTION: Virtual Interface Overrides
    // DESCRIPTION: Mandatory methods for sub-component CRUD and serialization.
    // =========================================================================
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // --- Serialization ---
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // CROSSSECTION_H
