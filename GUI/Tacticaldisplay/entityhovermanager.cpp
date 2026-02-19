#include "EntityHoverManager.h"
#include "GUI/Tacticaldisplay/Gis/gislib.h"
#include "core/Hierarchy/entity.h"

EntityHoverManager::EntityHoverManager(QObject *parent)
    : QObject(parent)
    , m_tooltipTimer(new QTimer(this))
{
    m_tooltipTimer->setSingleShot(true);
    m_tooltipTimer->setInterval(300); // 300ms delay
    connect(m_tooltipTimer, &QTimer::timeout, this, &EntityHoverManager::onTooltipTimer);
}

EntityHoverManager::~EntityHoverManager()
{
    m_tooltipTimer->stop();
}

QString EntityHoverManager::checkHover(const QPoint& mousePos,
                                       const std::unordered_map<std::string, MeshEntry>& meshes,
                                       GISlib* gislib)
{
    QString newHoverId;

    for (const auto& [id, entry] : meshes) {
        if (!entry.coreTransform || !entry.entity || !entry.entity->Active)
            continue;

        QPointF entityPos = gislib->geoToCanvas(
            entry.coreTransform->getLatitude(),
            entry.coreTransform->getLongitude()
            );

        // Calculate hover area based on image size
        float hoverRadius = 25.0f; // Fixed hover area
        if (entry.individualImageSize > 0) {
            hoverRadius = entry.individualImageSize / 2.0f;
        }

        if (QVector2D(mousePos - entityPos).length() < hoverRadius) {
            newHoverId = QString::fromStdString(id);
            break;
        }
    }

    // Handle hover state change
    if (newHoverId != m_hoveredEntityId) {
        m_hoveredEntityId = newHoverId;

        if (m_hoveredEntityId.isEmpty()) {
            emit hideTooltip();
            m_tooltipTimer->stop();
        } else {
            m_lastHoverPos = mousePos;
            m_tooltipTimer->start();
        }
    }

    return m_hoveredEntityId;
}

void EntityHoverManager::reset()
{
    m_hoveredEntityId.clear();
    m_tooltipTimer->stop();
    emit hideTooltip();
}

void EntityHoverManager::onTooltipTimer()
{
    if (m_hoveredEntityId.isEmpty()) return;
    emit showTooltip(m_hoveredEntityId, m_lastHoverPos);
}
