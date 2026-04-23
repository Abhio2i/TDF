// =============================================================================
// FILE:        mesh.h
// MODULE:      Graphical Mesh Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Mesh class, which represents a graphical shape
//              or sprite used for rendering simulation entities. Supports
//              polygons, sprite textures, colors, and image caching for
//              efficient 2D/3D rendering.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added polygon support and image scaling.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef MESH_H
#define MESH_H

#include "qpixmap.h"
#include <QObject>
#include <core/Hierarchy/Struct/color.h>
#include <core/Hierarchy/Struct/vector.h>
#include <QColor>

// =============================================================================
// CLASS: Mesh
//
// DESCRIPTION: Represents a visual mesh that can be a sprite, texture, or
//              polygon shape. Provides methods for point manipulation and
//              pixmap caching to optimize rendering performance.
// =============================================================================
class Mesh : public QObject
{
    Q_OBJECT
public:
    Mesh();

    // =========================================================================
    // SECTION: Mesh Properties
    // DESCRIPTION: Core visual and identification attributes.
    // =========================================================================
    bool Active;                    //!< Whether this mesh is currently active
    std::string ID;                 //!< Unique identifier for the mesh
    std::string* Sprite;            //!< Path to sprite image (if any)
    std::string* Texture;           //!< Path to texture image (if any)
    std::string lastpath;           //!< Last loaded file path (for caching)
    QPixmap* cacheimg;              //!< Cached pixmap for fast rendering
    QPixmap scaledimg;              //!< Scaled version of the pixmap
    int ImageScale = 0;             //!< Scale factor for image rendering

    // Color attributes
    QColor *color;                  //!< Outline color of the mesh
    QColor *fillColor;              //!< Fill color for polygons

    // =========================================================================
    // SECTION: Geometry Properties
    // DESCRIPTION: Line styling and polygon vertex data.
    // =========================================================================
    float lineWidth;                //!< Width of polygon outline
    bool closePath;                 //!< Whether to close the polygon path
    std::vector<Vector*> polygen;   //!< List of vertices defining the polygon

    // =========================================================================
    // SECTION: Public Methods
    // DESCRIPTION: Operations for pixmap retrieval and geometry manipulation.
    // =========================================================================
    QPixmap *getPixmap(int size);   //!< Returns scaled/cached pixmap for given size
    void addPoint(Vector* point);   //!< Adds a vertex to the polygon
    void removePoint();             //!< Removes the last vertex
    void clear();                   //!< Clears all vertices

    // Serialization methods
    void toJson();                  //!< Converts mesh to JSON representation
    void fromJson();                //!< Populates mesh from JSON
};

#endif // MESH_H
