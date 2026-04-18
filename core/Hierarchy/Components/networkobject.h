#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H
#include <QObject>
#include "./transform.h"
#include <core/Hierarchy/Struct/parameter.h>


class NetworkObject: public QObject, public Component
{
    Q_OBJECT
public:
    NetworkObject();
    ComponentType Typo() const override { return ComponentType::NetworkObject; }
    bool Active;
    bool isOwner;
    bool isOwnByServer;
    bool isServer;
    bool isClient;
    bool isSpawn;
    bool isUpdate;
    std::string lastUpdateStamp;

    Transform *transform;
    std::unordered_map<std::string, Parameter> *parameters;

    void recieveUpdate();
    void addSubComponent(std::string name, QString data1 = "", QString data2 = "", QJsonObject data3 = QJsonObject()) override;
    void removeSubComponent(std::string ID) override;
    QJsonObject getsubComponentData(std::string ID) const override;
    void updateSubComponent(std::string ID, const QJsonObject& obj) override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &obj) override;

signals:
    void netWorkSignal();
};

#endif // NETWORKOBJECT_H
