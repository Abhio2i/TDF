#ifndef SONARDISPLAY_H
#define SONARDISPLAY_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QPointer>
#include <QDateTime>
#include <algorithm>

#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/sonar/active_sonar.h"

class Simulation;

/**
 * @brief Represents a sonar contact displayed on the sonar screen.
 *
 * Contains target information received from the sonar backend
 * and used for rendering contact symbols and labels.
 */
struct SonarContact
{
    double angle = 0;          // Relative bearing (degrees)
    double range = 0;          // Range from ownship (meters)

    std::string name;          // Contact name

    std::string type;          // submarine / surface / unknown

    float intensity = 1.0f;    // Contact intensity (0-1)

    Entity* entity = nullptr;  // Associated hierarchy entity

    qint64 timestamp = 0;      // Detection timestamp
};

/**
 * @brief Sonar tactical display widget.
 *
 * Provides a real-time sonar presentation including:
 *
 *  • Range rings
 *  • Beam cone visualization
 *  • Heading indicator
 *  • Sweep animation
 *  • Contact plotting
 *  • Contact labels
 *  • Zoom controls
 *
 * Receives detection results from the sonar backend
 * and renders them on a PPI-style display.
 */
class SonarDisplay : public QWidget
{
    Q_OBJECT

public:

    /**
     * @brief Constructor.
     */
    explicit SonarDisplay(QWidget *parent = nullptr);

    /**
     * @brief Assign hierarchy reference.
     */
    void setHierarchy(Hierarchy* h);

    /**
     * @brief Select entity whose sonar is displayed.
     */
    void selectEntity(Entity* entity);

    /**
     * @brief Remove selected entity if deleted.
     */
    void RemoveEntity(QString id);

    /**
     * @brief Force display refresh.
     */
    void updateRadar();

    ~SonarDisplay();

    /**
     * @brief Assign simulation reference.
     */
    void setSimulation(Simulation* sim);

    /**
     * @brief Update displayed contacts using
     * latest sonar detection results.
     */
    void updateContacts(
        const std::vector<DetectionResult>& results);

    /**
     * @brief Update ownship heading.
     */
    void setHeading(float degrees)
    {
        m_heading = degrees;
        update();
    }

    /**
     * @brief Update sonar beam width.
     */
    void setBeamWidth(float degrees)
    {
        m_beamWidth = degrees;
        update();
    }

    /**
     * @brief Configure ping interval.
     *
     * Used for contact persistence timing.
     */
    void setPingInterval(float seconds)
    {
        m_pingIntervalMs =
            (qint64)(seconds * 1000.0f);

        update();
    }

    /**
     * @brief Currently selected entity.
     */
    Entity* getSelectedEntity() const
    {
        return entity;
    }

    /**
     * @brief Called every simulation update.
     *
     * Keeps sweep animation synchronized.
     */
    void onSimulationUpdate();

    /**
     * @brief Current heading.
     */
    float getHeading() const
    {
        return m_heading;
    }

    /**
     * @brief Ping interval in milliseconds.
     */
    int getPingInterval() const
    {
        return m_pingIntervalMs;
    }

protected:

    /**
     * @brief Main rendering function.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Handles zoom button clicks.
     */
    void mousePressEvent(QMouseEvent *event) override;

private:

    // =====================================================
    // Simulation references
    // =====================================================

    Hierarchy* hierarchy = nullptr;

    QPointer<Entity> entity = nullptr;

    QString selectedEntityId;

    Simulation* simulation = nullptr;

    // =====================================================
    // Sonar contacts
    // =====================================================

    QVector<SonarContact> contacts;

    // =====================================================
    // Display configuration
    // =====================================================

    int maxRange  = 100;   // Display range (km)

    int ringCount = 5;     // Number of range rings

    int sweepAngle = 0;    // Current sweep angle

    float m_heading = 0.0f;

    float m_beamWidth = 360.0f;

    qint64 m_pingIntervalMs = 10000;

    // =====================================================
    // Simulation state
    // =====================================================

    qint64 lastUpdateTime = 0;

    bool simulationRunning = false;

    // =====================================================
    // Sweep animation
    // =====================================================

    QTimer sweepTimer;

    QVector<int> m_sweepTrail;

    static constexpr int TRAIL_LENGTH = 30;

    // =====================================================
    // UI controls
    // =====================================================

    QRect plusRect;

    QRect minusRect;

    // =====================================================
    // Rendering helpers
    // =====================================================

    /**
     * @brief Draw background.
     */
    void drawBackground(QPainter &p);

    /**
     * @brief Draw sonar range rings.
     */
    void drawSonarRing(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw compass markings.
     */
    void drawDegreeMarkings(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw sonar beam sector.
     */
    void drawBeamCone(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw ownship heading line.
     */
    void drawHeadingLine(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw center marker.
     */
    void drawCenterDot(
        QPainter &p,
        QPoint center);

    /**
     * @brief Draw active sweep line.
     */
    void drawSweep(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw sweep trail effect.
     */
    void drawSweepTrail(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw sonar contacts.
     */
    void drawContacts(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw contact labels.
     */
    void drawContactLabels(
        QPainter &p,
        QPoint center,
        int radius);

    /**
     * @brief Draw range zoom controls.
     */
    void drawRangeButtons(QPainter &p);

    /**
     * @brief Read heading from selected entity.
     */
    void updateHeadingFromEntity();
};

#endif // SONARDISPLAY_H
