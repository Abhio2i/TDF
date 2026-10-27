#ifndef SHAPE_TYPE_REGISTRY_H
#define SHAPE_TYPE_REGISTRY_H

#include "backend/shapes/ishape_type.h"
#include <QMap>
#include <QStringList>
#include <QObject>

class ShapeTypeRegistry : public QObject
{
    Q_OBJECT
public:
    static ShapeTypeRegistry* instance();

    // Ownership transfer
    void registerType(IShapeType* type);
    void unregisterType(const QString& typeId);

    IShapeType* type(const QString& typeId) const;
    QList<IShapeType*> allTypes() const;
    QStringList typeIds() const;
    bool hasType(const QString& typeId) const;

signals:
    void typeRegistered(const QString& typeId);
    void typeUnregistered(const QString& typeId);

private:
    explicit ShapeTypeRegistry(QObject* parent = nullptr);
    static ShapeTypeRegistry* m_instance;

    QMap<QString, IShapeType*> m_types;
};

#endif // SHAPE_TYPE_REGISTRY_H
