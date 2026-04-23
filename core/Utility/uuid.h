// =============================================================================
// FILE:        uuid.h
// MODULE:      Unique Identifier Generation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Uuid class, which provides static methods for
//              generating short unique identifiers (UUID-like). Used for
//              creating unique IDs for entities, components, and other
//              simulation objects.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef UUID_H
#define UUID_H

#include <string>

// =============================================================================
// CLASS: Uuid
//
// DESCRIPTION: Utility class for generating unique identifiers. Currently
//              provides a static method to generate a short unique ID string.
// =============================================================================
class Uuid
{
public:
    Uuid();

    // =========================================================================
    // SECTION: ID Generation
    // DESCRIPTION: Static method to generate a short unique ID string.
    // =========================================================================
    static std::string generateShortUniqueID();   //!< Generates a short unique ID
};

#endif // UUID_H
