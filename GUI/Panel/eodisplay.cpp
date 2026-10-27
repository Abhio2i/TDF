#include "eodisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/eosensor.h"
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>
#include <QCoreApplication>

EODisplay::EODisplay(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set background color for the letterbox/pillarbox areas
    setStyleSheet("background-color: black;");

    // Set size policy
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    // Set padding to 0 since we want to utilize the whole screen
    padding = 0;

    // Enable mouse tracking
    setMouseTracking(true);

    glDisplay = new EOOpenGLDisplay(this);
    // Initialize the OpenGL subwidget

}

// %%% Size Management %%%
QSize EODisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize EODisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

int EODisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

void EODisplay::mouseMoveEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
    QWidget::mouseMoveEvent(event);
}

void EODisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    QWidget::leaveEvent(event);
}

/* =========  Per-entity camera state  ========= */
void EODisplay::saveCameraState(const std::string &entityId)
{
    if (!glDisplay || entityId.empty()) return;
    m_cameraStates[entityId] = {
        glDisplay->getCurrentAzimuth(),
        glDisplay->getCurrentElevation(),
        glDisplay->getZoom()
    };
}

void EODisplay::restoreCameraState(const std::string &entityId)
{
    if (!glDisplay) return;

    auto it = m_cameraStates.find(entityId);
    if (it != m_cameraStates.end()) {
        // Entity visited before — resume exactly where we left it
        const CameraState &s = it->second;
        glDisplay->setCommandedLOS(s.az, s.el);
        // Jump instantly rather than slewing from 0,0 —
        // use the internal setters directly so there's no visible sweep
        glDisplay->setZoom(s.zoom);
    } else {
        // Brand-new entity — start at boresight, zoom 1
        glDisplay->setCommandedLOS(0.f, 0.f);
        glDisplay->setZoom(1.f);
    }
}

/* Select entity for display */
void EODisplay::selectEntity(Entity* entit)
{
    // 1. Save gimbal az/el/zoom for the entity we're LEAVING, so we can
    //    restore it when the operator comes back to the same entity.
    if (!id.isEmpty())
        saveCameraState(id.toStdString());

    entity = nullptr;
    sensor = nullptr;
    id.clear();
    sensorId.clear();


    Platform* platform = dynamic_cast<Platform*>(entit);

    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("EO Display (No Platform)");
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    // 2. Restore the camera to wherever the operator left it for THIS
    //    entity (or boresight/zoom-1 if it's never been visited before).
    restoreCameraState(id.toStdString());

    if (entity->sensors.sensors.empty()) {
        return;
    }

    sensor = nullptr;

    for (auto const& pair : entity->sensors.sensors) {
        Sensor* s = pair.second;

        if (s && s->subType == Sensor::SubType::EO) {
            sensor = s;
            sensorId = QString::fromStdString(s->ID);

            glDisplay->setSensorParameter(
                sensor->sensorPoseGeo,
                sensor->azimuth,
                sensor->EODetectionCoord,
                sensor->range * 1000.0f // range is stored in km — see eosensor.cpp
                );

            setWindowTitle(
                "EO Display (" +
                QString::fromStdString(entity->Name) +
                ")"
                );

            break;
        }
    }

    // Trigger a resize event to immediately recalculate the 3D widget ratio
    QResizeEvent* resizeEv = new QResizeEvent(size(), size());
    QCoreApplication::postEvent(this, resizeEv);
}

/* Remove entity from display */
void EODisplay::RemoveEntity(QString ID)
{

    if (id != ID)
        return;

    entity = nullptr;
    sensor = nullptr;

    id.clear();
    sensorId.clear();

    setWindowTitle("EO Display");

}

void EODisplay::RemoveSensor(const QString& sensorID)
{
    if (sensorId != sensorID)
        return;

    sensor = nullptr;
    sensorId.clear();

    setWindowTitle("EO Display (Sensor Removed)");
}

/* Update radar display with new data */
void EODisplay::updateRadar()
{
    if (!hierarchy)
        return;

    if (id.isEmpty() || sensorId.isEmpty())
        return;

    Entity* currentEntity =
        hierarchy->getEntityById(id);

    if (!currentEntity)
    {
        entity = nullptr;
        sensor = nullptr;
        id.clear();
        sensorId.clear();
        return;
    }

    Platform* platform =
        dynamic_cast<Platform*>(currentEntity);

    if (!platform)
    {
        entity = nullptr;
        sensor = nullptr;
        id.clear();
        sensorId.clear();
        return;
    }

    Sensor* currentSensor =
        platform->sensors.getSensor(
            sensorId.toStdString()
            );

    if (!currentSensor)
    {
        sensor = nullptr;
        sensorId.clear();

        setWindowTitle("EO Display (Sensor Removed)");
        return;
    }

    entity = platform;
    sensor = currentSensor;

    setRange(currentSensor->range);

    glDisplay->setSensorParameter(
        currentSensor->sensorPoseGeo,
        currentSensor->azimuth,
        currentSensor->EODetectionCoord,
        currentSensor->range * 1000.0f // range is stored in km — see eosensor.cpp
        );

    // GIMBAL-AWARE DETECTION: push the operator's current gimbal pointing
    // direction (set via mouse-drag/arrow-keys on glDisplay) into the
    // sensor's own simulation state. eosensor.cpp's scan() reads this each
    // time it runs, so detection is gated on where the turret is ACTUALLY
    // pointing right now, not just the aircraft's raw nose direction.
    // dynamic_cast guards against currentSensor being some other Sensor
    // subtype (e.g. IR) that doesn't have gimbal offset members — in that
    // case this safely does nothing rather than crashing.
    if (EOSensor* eoSensor = dynamic_cast<EOSensor*>(currentSensor)) {
        eoSensor->gimbalAzimuthOffset   = glDisplay->getCurrentAzimuth();
        eoSensor->gimbalElevationOffset = glDisplay->getCurrentElevation();
    }

    glDisplay->update();
}

/* Resize the 3D OpenGL Widget while maintaining Aspect Ratio */
void EODisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!glDisplay)
        return;

    if (width() <= 0 || height() <= 0)
        return;

    // EO/IR display aspect ratio: SQUARE, matching real EO/IR operator
    // console convention — the sensor feed is kept square (1:1) so it never
    // distorts as the gimbal/platform rotates, and the surrounding letterbox
    // area is reserved for HUD symbology instead of stretching the image
    // (see EOOpenGLDisplay's HUD overlay, drawn inside this square area).
    constexpr float targetWidth  = 1.0f;
    constexpr float targetHeight = 1.0f;

    const float targetAspect =
        targetWidth / targetHeight;

    const float windowAspect =
        static_cast<float>(width()) /
        static_cast<float>(height());

    int finalWidth;
    int finalHeight;

    if (windowAspect > targetAspect)
    {
        // Pillarbox
        finalHeight = height();
        finalWidth =
            static_cast<int>(finalHeight * targetAspect);
    }
    else
    {
        // Letterbox
        finalWidth = width();
        finalHeight =
            static_cast<int>(finalWidth / targetAspect);
    }

    const int xOffset =
        (width() - finalWidth) / 2;

    const int yOffset =
        (height() - finalHeight) / 2;

    glDisplay->setGeometry(
        xOffset,
        yOffset,
        finalWidth,
        finalHeight
        );
}

/* Main paint event handler - Now only draws the black background for letterboxing */
void EODisplay::paintEvent(QPaintEvent * /*event*/)
{
    QElapsedTimer timer;
    timer.start();

    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);

    // Fill the background to ensure letterbox borders remain black
    p.fillRect(rect(), Qt::black);

    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->csmdisplay = elapsedMs;
}

// %%% Debug Helpers %%%
void EODisplay::debug(const QString &str, const debugEODisplay &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug() << currentdebugType << str;
    }
}

bool EODisplay::dbgIsAllow(const debugEODisplay &currentdebugType)
{
    return ((currentdebugType & debugList) == currentdebugType);
}
