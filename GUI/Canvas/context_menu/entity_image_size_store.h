#ifndef ENTITY_IMAGE_SIZE_STORE_H
#define ENTITY_IMAGE_SIZE_STORE_H

#include <QHash>
#include <QString>
#include "core/Hierarchy/ECS/World/world.h"  // ECS::Entity

class EntityImageSizeStore
{
public:
    static EntityImageSizeStore& instance()
    {
        static EntityImageSizeStore s;
        return s;
    }
    void setByHierarchyId(const QString& id, float scale)
    {
        if (!id.isEmpty() && scale > 0.f)
            m_byHierarchyId[id] = scale;
    }
    void setByEntity(ECS::Entity e, float scale)
    {
        if (e != 0 && scale > 0.f)
            m_byEntity[e] = scale;
    }
    float getByHierarchyId(const QString& id, float fallback = 50.f) const
    {
        return m_byHierarchyId.value(id, fallback);
    }

    float getByEntity(ECS::Entity e, float fallback = 50.f) const
    {
        return m_byEntity.value(e, fallback);
    }
    bool hasEntity(ECS::Entity e) const { return m_byEntity.contains(e); }
    void clear()
    {
        m_byHierarchyId.clear();
        m_byEntity.clear();
    }

private:
    QHash<QString, float> m_byHierarchyId;
    QHash<ECS::Entity, float> m_byEntity;
};

#endif
