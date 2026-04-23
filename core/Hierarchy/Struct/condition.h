// =============================================================================
// FILE:        condition.h
// MODULE:      Condition Evaluation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Condition class, which represents a logical
//              condition used in mission triggers or behaviour trees.
//              Supports binary comparisons between two operands (int, float,
//              double, string, bool, char) using a specified operator type.
//              Provides AND flag for combining conditions and JSON
//              serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef CONDITION_H
#define CONDITION_H

#include <QObject>
#include "./constants.h"
#include <variant>

// =============================================================================
// CLASS: Condition
//
// DESCRIPTION: Represents a condition that can be evaluated (true/false).
//              Compares two operands using an operator (e.g., less than,
//              equal to, greater than). The AND flag allows logical
//              conjunction with other conditions. Operands can be of
//              multiple types via std::variant.
// =============================================================================
class Condition: public QObject
{
    Q_OBJECT
public:
    Condition();

    // =========================================================================
    // SECTION: Condition Properties
    // DESCRIPTION: Logical and comparison data.
    // =========================================================================
    bool And;       //!< If true, this condition is combined with next using AND
    std::variant<int, float, double, std::string, bool, char> first;   //!< Left operand
    std::variant<int, float, double, std::string, bool, char> second;  //!< Right operand
    Constants::OperaterType type;   //!< Comparison operator (e.g., LESS, EQUAL, GREATER)

    void check();   //!< Evaluates the condition (returns bool, but currently void)

    // Serialization
    void toJson();  //!< Serialises condition to JSON
    void fromJson();//!< Deserialises condition from JSON
};

#endif // CONDITION_H
