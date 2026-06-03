#include "disnetworkbridge.h"
#include <QDebug>

DISNetworkBridge::DISNetworkBridge(QObject* parent)
    : QObject(parent) {}

DISNetworkBridge::~DISNetworkBridge() = default;

void DISNetworkBridge::setMapper(EntityIDMapper* mapper)
{
    m_mapper = mapper;
}

void DISNetworkBridge::configure(const DISConfig& config)
{
    m_config = config;
}

void DISNetworkBridge::registerEntity(const QString& entityID)
{
    if (!m_mapper) return;
    std::string id = entityID.toStdString();
    if (m_mapper->isRegistered(id)) return;

    DISEntityID disID = m_mapper->registerLocalEntity(id);
    {
        QMutexLocker lock(&m_snapshotMutex);
        m_localEntities[entityID] = true;
    }
    qDebug() << "[DISNetworkBridge] Registered entity:" << entityID
             << "→ DIS" << disID.site
             << "/" << disID.application
             << "/" << disID.entity;
}

void DISNetworkBridge::unregisterEntity(const QString& entityID)
{
    QMutexLocker lock(&m_snapshotMutex);
    if (m_mapper)
        m_mapper->unregisterEntity(entityID.toStdString());
    m_localEntities.remove(entityID);
    m_cachedSnapshots.remove(entityID);
    m_destroyedEntities.remove(entityID);
    qDebug() << "[DISNetworkBridge] Unregistered entity:" << entityID;
}

// Called from DIS thread via DISManager::onPushEntitySnapshot
// Data built on main thread, arrives here as a copied value type
void DISNetworkBridge::pushSnapshot(const DISEntitySnapshot& snap)
{
    QMutexLocker lock(&m_snapshotMutex);
    m_cachedSnapshots[QString::fromStdString(snap.entityID)] = snap;
}

// Called from DIS thread via DISManager::onRemoveEntitySnapshot
void DISNetworkBridge::removeSnapshot(const QString& entityID)
{
    QMutexLocker lock(&m_snapshotMutex);
    m_cachedSnapshots.remove(entityID);
    m_localEntities.remove(entityID);
    m_destroyedEntities.remove(entityID);
}

// Called from PDUSender on DIS thread — pure cache read, zero engine access
QList<DISEntitySnapshot> DISNetworkBridge::collectSnapshots()
{
    QMutexLocker lock(&m_snapshotMutex);
    return m_cachedSnapshots.values();
}

uint8_t DISNetworkBridge::teamToForceID(int team)
{
    switch (team) {
    case 0:  return DISForceID::Opposing;
    case 1:  return DISForceID::Friendly;
    case 2:  return DISForceID::Friendly2;
    case 3:  return DISForceID::Neutral;
    case 4:  return DISForceID::Neutral;
    default: return DISForceID::Friendly;
    }
}

void DISNetworkBridge::categoryToDISType(int category,
                                         uint8_t& domain, uint8_t& kind)
{
    kind = DISKind::Platform;
    switch (category) {
    case 0:  domain = DISDomain::Air;     break;
    case 1:  domain = DISDomain::Land;    break;
    case 2:  domain = DISDomain::Surface; break;
    default: domain = DISDomain::Other;   break;
    }
}

uint32_t DISNetworkBridge::healthToAppearance(float health)
{
    uint32_t appearance = 0;
    if      (health <= 0.0f) appearance |= (3u << 3);
    else if (health < 25.0f) appearance |= (3u << 3);
    else if (health < 50.0f) appearance |= (2u << 3);
    else if (health < 75.0f) appearance |= (1u << 3);
    return appearance;
}
uint8_t DISNetworkBridge::subCategoryToDISCategory(int engineCategory,
                                                   int engineSubCategory)
{
    switch (engineCategory) {
    case 0:
        switch (engineSubCategory) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 14;
        default: return 1;
        }
    case 1:
        switch (engineSubCategory) {
        case 0: return 1;
        case 1: return 3;
        case 2: return 4;
        default: return 1;
        }
    case 2:
        switch (engineSubCategory) {
        case 0: return 1;
        case 1: return 1;
        case 2: return 1;
        default: return 1;
        }
    default: return 1;
    }
}   // ← THIS CLOSING BRACE WAS MISSING
