#ifndef SONARDISPLAY_H
#define SONARDISPLAY_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include <algorithm>
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h"

struct SonarContact
{
    double angle   = 0;
    double range   = 0;
    std::string name;
    std::string type;      //  "submarine", "surface", "unknown"
    Entity* entity = nullptr;
    qint64 timestamp = 0; // when detected
};

class SonarDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit SonarDisplay(QWidget *parent = nullptr);

    void setHierarchy(Hierarchy* h);
    void selectEntity(Entity* entity);
    void RemoveEntity(QString id);
    void updateRadar();

    void updateContacts(const std::vector<DetectionResult>& results);

    // Heading — set from sonar entity
    void setHeading(float degrees) { m_heading = degrees; update(); }

    // Beam width — set from sonar config
    void setBeamWidth(float degrees) { m_beamWidth = degrees; update(); }

    void setPingInterval(float seconds)          // ← ADD
    {
        m_pingIntervalMs = (qint64)(seconds * 1000.0f);
        update();
    }

    Entity* getSelectedEntity() const { return entity; }

protected:
    void paintEvent(QPaintEvent *event)  override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Hierarchy*            hierarchy  = nullptr;
    Entity*               entity     = nullptr;
    QVector<SonarContact> contacts;

    int    maxRange   = 100;  // km
    int    ringCount  = 5;
    int    sweepAngle = 0;
    float  m_heading  = 0.0f;    // ship heading
    float  m_beamWidth = 360.0f; // sonar beam width

    qint64 m_pingIntervalMs = 10000;

    QTimer sweepTimer;
    // Button rects — set in paintEvent
    QRect  plusRect;
    QRect  minusRect;

    // Sweep trail — last N angles
    QVector<int> m_sweepTrail;
    static constexpr int TRAIL_LENGTH = 30;

    void drawBackground(QPainter &p);
    void drawSonarRing(QPainter &p, QPoint center, int radius);
    void drawDegreeMarkings(QPainter &p, QPoint center, int radius);
    void drawBeamCone(QPainter &p, QPoint center, int radius);   // ← NEW
    void drawHeadingLine(QPainter &p, QPoint center, int radius); // ← NEW
    void drawCenterDot(QPainter &p, QPoint center);
    void drawSweep(QPainter &p, QPoint center, int radius);
    void drawSweepTrail(QPainter &p, QPoint center, int radius);  // ← NEW
    void drawContacts(QPainter &p, QPoint center, int radius);
    void drawContactLabels(QPainter &p, QPoint center, int radius); // ← NEW
    void drawRangeButtons(QPainter &p);
};

#endif // SONARDISPLAY_H
