#ifndef ENTITYHOVERMANAGER_H
#define ENTITYHOVERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QPoint>
#include <QString>
#include <unordered_map>

class GISlib;
struct MeshEntry;

class EntityHoverManager : public QObject
{
    Q_OBJECT

public:
    explicit EntityHoverManager(QObject *parent = nullptr);
    ~EntityHoverManager();

    // Check if mouse is over any entity and return entity name
    QString checkHover(const QPoint& mousePos,
                       const std::unordered_map<std::string, MeshEntry>& meshes,
                       GISlib* gislib);

    // Reset hover state
    void reset();

    // Get current hovered entity ID
    QString hoveredEntityId() const { return m_hoveredEntityId; }

signals:
    void showTooltip(const QString& entityName, const QPoint& globalPos);
    void hideTooltip();

private slots:
    void onTooltipTimer();

private:
    QString m_hoveredEntityId;
    QPoint m_lastHoverPos;
    QTimer* m_tooltipTimer;
};

#endif // ENTITYHOVERMANAGER_H
