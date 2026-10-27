#include "shape_type_registry.h"
#include <QDebug>

ShapeTypeRegistry* ShapeTypeRegistry::m_instance = nullptr;

ShapeTypeRegistry* ShapeTypeRegistry::instance()
{
    if (!m_instance)
        m_instance = new ShapeTypeRegistry();
    return m_instance;
}

ShapeTypeRegistry::ShapeTypeRegistry(QObject* parent)
    : QObject(parent)
{
}

void ShapeTypeRegistry::registerType(IShapeType* type)
{
    if (!type || type->typeId().isEmpty()) {
        qWarning() << "ShapeTypeRegistry: invalid type";
        return;
    }

    const QString id = type->typeId();
    if (m_types.contains(id)) {
        qWarning() << "ShapeTypeRegistry: type already registered:" << id;
        delete type;
        return;
    }

    m_types.insert(id, type);
    emit typeRegistered(id);
}

void ShapeTypeRegistry::unregisterType(const QString& typeId)
{
    if (!m_types.contains(typeId))
        return;

    delete m_types.take(typeId);
    emit typeUnregistered(typeId);
}

IShapeType* ShapeTypeRegistry::type(const QString& typeId) const
{
    return m_types.value(typeId, nullptr);
}

QList<IShapeType*> ShapeTypeRegistry::allTypes() const
{
    return m_types.values();
}

QStringList ShapeTypeRegistry::typeIds() const
{
    return m_types.keys();
}

bool ShapeTypeRegistry::hasType(const QString& typeId) const
{
    return m_types.contains(typeId);
}
