#ifndef MESHRENDERER2D_H
#define MESHRENDERER2D_H
#include "./component.h"
#include<qobject.h>
#include<core/Hierarchy/Components/mesh.h>
#include <QJsonObject>
#include <memory>

class MeshRenderer2D : public QObject, public Component
{
    Q_OBJECT
public:
    MeshRenderer2D();
    ComponentType Typo() const override { return ComponentType::MeshRenderer2D; }
    bool Active;
    std::string ID;
    std::string* Sprite;
    std::string* Texture;

    QColor* color;
    std::shared_ptr<QColor> color2;


    std::vector<Mesh*> Meshes;

    QJsonObject toJson()const override;
    void fromJson(const QJsonObject &obj) override;

};

#endif // MESHRENDERER2D_H
