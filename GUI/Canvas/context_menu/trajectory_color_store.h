#ifndef TRAJECTORY_COLOR_STORE_H
#define TRAJECTORY_COLOR_STORE_H

#include <QHash>
#include <QColor>
#include <QString>
#include "core/Hierarchy/ECS/World/world.h"

class TrajectoryColorStore
{
public:
    static TrajectoryColorStore& instance()
    {
        static TrajectoryColorStore s;
        return s;
    }

    void setByHierarchyId(const QString& id, const QColor& c)
    {
        if (!id.isEmpty() && c.isValid())
            m_byHierarchyId[id] = c;
    }

    void setByEntity(ECS::Entity e, const QColor& c)
    {
        if (e != 0 && c.isValid())
            m_byEntity[e] = c;
    }

    QColor getByHierarchyId(const QString& id,
                            const QColor& fallback = QColor(0, 100, 255)) const
    {
        return m_byHierarchyId.value(id, fallback);
    }

    QColor getByEntity(ECS::Entity e,
                       const QColor& fallback = QColor(0, 100, 255)) const
    {
        return m_byEntity.value(e, fallback);
    }

    bool hasEntity(ECS::Entity e) const { return m_byEntity.contains(e); }

private:
    QHash<QString, QColor> m_byHierarchyId;
    QHash<ECS::Entity, QColor> m_byEntity;
};

#endif
