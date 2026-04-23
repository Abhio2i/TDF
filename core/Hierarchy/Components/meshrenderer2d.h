// =============================================================================
// FILE:        meshrenderer2d.h
// MODULE:      2D Mesh Rendering
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the MeshRenderer2D class, which handles 2D rendering
//              of meshes (sprites, textures, polygons) for simulation entities.
//              Supports multiple meshes per renderer and custom JSON parameters.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//   Rev 2  [Date]  Added custom parameters and shared_ptr color support.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef MESHRENDERER2D_H
#define MESHRENDERER2D_H

#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Components/mesh.h>
#include <QJsonObject>
#include <memory>

// =============================================================================
// CLASS: MeshRenderer2D
//
// DESCRIPTION: Renders 2D meshes (sprites, textures, or custom polygons)
//              for an entity. Maintains a collection of Mesh objects and
//              supports activation, coloring, and serialization.
// =============================================================================
class MeshRenderer2D : public QObject, public Component
{
    Q_OBJECT
public:
    MeshRenderer2D();
    ComponentType Typo() const override { return ComponentType::MeshRenderer2D; }

    // =========================================================================
    // SECTION: Rendering Properties
    // DESCRIPTION: Core visual state and texture references.
    // =========================================================================
    bool Active;                    //!< Whether this renderer is active
    std::string* Sprite;            //!< Path to sprite image (if any)
    std::string* Texture;           //!< Path to texture image (if any)

    // =========================================================================
    // SECTION: Color Properties
    // DESCRIPTION: Color overrides for mesh rendering.
    // =========================================================================
    QColor* color;                          //!< Primary color (raw pointer)
    std::shared_ptr<QColor> color2;         //!< Secondary color (shared ownership)

    // =========================================================================
    // SECTION: Custom Parameters & Mesh Collection
    // DESCRIPTION: Extensible storage and mesh list.
    // =========================================================================
    QJsonObject AdditionalParameters;            //!< User-defined key-value parameters
    std::vector<Mesh*> Meshes;              //!< Collection of meshes to render

    // Component interface overrides
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // MESHRENDERER2D_H
