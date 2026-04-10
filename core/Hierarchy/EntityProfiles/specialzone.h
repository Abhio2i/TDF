#ifndef SPECIALZONE_H
#define SPECIALZONE_H

#include "core/Hierarchy/entity.h"
class Specialzone: public Entity
{
    Q_OBJECT
public:
    Specialzone(Hierarchy* h);
    Transform *transform = nullptr;
    Collider *collider = nullptr;
    MeshRenderer2D *meshRenderer2d = nullptr;
    float direction = 90;
    float MinAltitude = 0;
    float MaxAltitude = 10000;
    float Speed = 70;//km/h
    float Temprature = 30;//deg
    float humidity = 30;//%
    float rain = 0;//mm/h
    float fog = 0;//%

    void Update(float delta);

    void spawn() override;
    std::vector<std::string> getSupportedComponents() override;
    void addComponent(std::string name) override;
    void removeComponent(std::string name) override;
    QJsonObject getComponent(std::string name) override;
    void updateComponent(QString name, const QJsonObject& obj) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;
private:
    float time = 0;
    QVector3D getDynamicWind(float baseAngle, float time);
    float getDynamicSpeed(float baseSpeed, float time);
};

#endif // SPECIALZONE_H
