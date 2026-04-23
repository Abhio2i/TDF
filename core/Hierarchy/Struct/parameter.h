// =============================================================================
// FILE:        parameter.h
// MODULE:      Dynamic Parameter Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Parameter class, which represents a named dynamic
//              parameter with a type (int, float, double, string, bool, char)
//              and a variant value. Used for flexible data exchange and
//              serialization in the simulation framework.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef PARAMETER_H
#define PARAMETER_H

#include <QObject>
#include "./constants.h"
#include <variant>

// =============================================================================
// CLASS: Parameter
//
// DESCRIPTION: Holds a single parameter consisting of a name, a data type,
//              and a variant value. Provides JSON serialization for persistence
//              and network transfer.
// =============================================================================
class Parameter: public QObject
{
    Q_OBJECT
public:
    Parameter();

    // =========================================================================
    // SECTION: Parameter Data
    // DESCRIPTION: Core attributes defining the parameter.
    // =========================================================================
    std::string Name;                           //!< Unique parameter name
    Constants::ParameterType type;              //!< Data type of the value
    std::variant<int, float, double, std::string, bool, char> value; //!< Stored value

    void setValue();    //!< Sets the parameter value (implementation not shown)
    void getValue();    //!< Retrieves the parameter value (implementation not shown)

    // Serialization
    QJsonObject toJson() const;                 //!< Serialises parameter to JSON
    void fromJson(const QJsonObject& obj);      //!< Deserialises parameter from JSON
};

#endif // PARAMETER_H
