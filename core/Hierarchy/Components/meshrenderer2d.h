

#ifndef MESHRENDERER2D_H
#define MESHRENDERER2D_H
#include "./component.h"
#include <QObject>
#include <core/Hierarchy/Components/mesh.h>
#include <QJsonObject>
#include <memory>

class MeshRenderer2D : public QObject, public Component
{
    Q_OBJECT
public:
    MeshRenderer2D();
    ComponentType Typo() const override { return ComponentType::MeshRenderer2D; }
    bool Active;
    std::string* Sprite;
    std::string* Texture;

    QColor* color;
    std::shared_ptr<QColor> color2;
    QJsonObject customParameters; // Added to store custom parameters

    std::vector<Mesh*> Meshes;
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
};

#endif // MESHRENDERER2D_H
