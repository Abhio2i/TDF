#ifndef FIXEDPOINTS_H
#define FIXEDPOINTS_H
#include <core/Hierarchy/entity.h>
#include <QObject>
#include <QJsonObject>

class FixedPoints: public Entity
{
    Q_OBJECT
public:
    FixedPoints(Hierarchy* h);
    Transform *transform = nullptr;
    Collider *collider = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

};

#endif // FIXEDPOINTS_H
