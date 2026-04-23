// =============================================================================
// FILE:        color.h
// MODULE:      Colour Representation
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Color class, which represents an RGB colour with
//              integer components (0-255). Provides JSON serialisation for
//              persistence in simulation assets and UI rendering.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef COLOR_H
#define COLOR_H

#include <qobject.h>

// =============================================================================
// CLASS: Color
//
// DESCRIPTION: Simple RGB colour container. Each component (red, green, blue)
//              is stored as an integer. Provides JSON serialisation methods
//              for saving/loading colour values.
// =============================================================================
class Color: public QObject
{
    Q_OBJECT
public:
    Color();

    int r;          //!< Red component (0-255)
    int g;          //!< Green component (0-255)
    int b;          //!< Blue component (0-255)

    void toJson();  //!< Serialises colour to JSON
    void fromJson();//!< Deserialises colour from JSON
};

#endif // COLOR_H
