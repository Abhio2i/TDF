#ifndef DISNETWORKBRIDGE_H
#define DISNETWORKBRIDGE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>
#include <QSet>

#include "../interface/DISEntitySnapshot.h"
#include "../interface/DISConfig.h"
#include "../utils/entityidmapper.h"

class DISNetworkBridge : public QObject {
    Q_OBJECT

public:
    explicit DISNetworkBridge(QObject* parent = nullptr);
    ~DISNetworkBridge() override;

    void setMapper (EntityIDMapper* mapper);
    void configure (const DISConfig& config);

    // Called from DIS thread via DISManager slots
    void registerEntity  (const QString& entityID);
    void unregisterEntity(const QString& entityID);
    void pushSnapshot    (const DISEntitySnapshot& snap);
    void removeSnapshot  (const QString& entityID);

    // Called from DIS thread by PDUSender — pure cache read, no engine access
    QList<DISEntitySnapshot> collectSnapshots();

    // Static utilities — pure functions, no state, safe from any thread
    static uint8_t  teamToForceID    (int team);
    static void     categoryToDISType(int category,
                                  uint8_t& domain, uint8_t& kind);
    static uint32_t healthToAppearance(float health);
    static uint8_t subCategoryToDISCategory(int engineCategory,
                                            int engineSubCategory);
private:
    EntityIDMapper* m_mapper = nullptr;
    DISConfig       m_config;
    QMutex          m_snapshotMutex;

    QMap<QString, DISEntitySnapshot> m_cachedSnapshots;
    QSet<QString>                    m_destroyedEntities;
    QMap<QString, bool>              m_localEntities;
};

#endif // DISNETWORKBRIDGE_H

