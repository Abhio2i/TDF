#include "irdisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>
#include <QCoreApplication>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/irsensor.h"


IRDisplay::IRDisplay(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{

    qDebug() << "IRDisplay constructor START";

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
    glDisplay = new IROpenGLDisplay(this);
    // Initialize the OpenGL subwidget

}

// %%% Size Management %%%
QSize IRDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize IRDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

int IRDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

void IRDisplay::mouseMoveEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
    QWidget::mouseMoveEvent(event);
}

void IRDisplay::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    QWidget::leaveEvent(event);
}

/* =========  Per-entity camera state  ========= */
void IRDisplay::saveCameraState(const std::string &entityId)
{
    if (!glDisplay || entityId.empty()) return;
    m_cameraStates[entityId] = {
        glDisplay->getCurrentAzimuth(),
        glDisplay->getCurrentElevation(),
        glDisplay->getZoom()
    };
}

void IRDisplay::restoreCameraState(const std::string &entityId)
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
void IRDisplay::selectEntity(Entity* entit)
{

    // 1. Save gimbal az/el/zoom for the entity we're LEAVING, so we can
    //    restore it when the operator comes back to the same entity.
    if (!id.isEmpty())
        saveCameraState(id.toStdString());

    // Clear previous state
    entity = nullptr;
    sensor = nullptr;
    id.clear();
    sensorId.clear();

    Platform* platform = dynamic_cast<Platform*>(entit);

    if (!platform)
    {
        Console::error("Entity is not a Platform");
        setWindowTitle("IR Display (No Platform)");
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    // 2. Restore the camera to wherever the operator left it for THIS
    //    entity (or boresight/zoom-1 if it's never been visited before).
    restoreCameraState(id.toStdString());

    if (platform->sensors.sensors.empty())
    {
        setWindowTitle(
            "IR Display (" +
            QString::fromStdString(platform->Name) +
            ")"
            );
        return;
    }

    for (const auto& pair : platform->sensors.sensors)
    {
        Sensor* s = pair.second;

        if (!s)
            continue;

        if (s->subType != Sensor::SubType::IR)
            continue;

        // Store ID, not lifetime ownership
        sensorId = QString::fromStdString(s->ID);

        // Only use sensor while it is known to be valid
        sensor = s;

        glDisplay->setSensorParameter(
            s->sensorPoseGeo,
            s->azimuth,
            s->IRDetectionCoord
            );

        qDebug() << "IRDetectionCoord Size:"
                 << s->IRDetectionCoord.size();

        setWindowTitle(
            "IR Display (" +
            QString::fromStdString(platform->Name) +
            ")"
            );

        break;
    }

    QResizeEvent* resizeEv =
        new QResizeEvent(size(), size());

    QCoreApplication::postEvent(this, resizeEv);
}

/* Remove entity from display */
void IRDisplay::RemoveEntity(QString ID)
{
    if (id != ID)
        return;

    entity = nullptr;
    sensor = nullptr;

    id.clear();
    sensorId.clear();

    setWindowTitle("IR Display");
}

void IRDisplay::RemoveSensor(const QString& removedSensorID)
{
    if (sensorId != removedSensorID)
        return;

    sensor = nullptr;
    sensorId.clear();

    setWindowTitle("IR Display (Sensor Removed)");
}


void IRDisplay::updateRadar()
{
    if (!hierarchy) return;
    if (id.isEmpty() || sensorId.isEmpty()) return;

    Entity* currentEntity = hierarchy->getEntityById(id);
    if (!currentEntity) {
        entity = nullptr; sensor = nullptr;
        id.clear(); sensorId.clear();
        return;
    }

    Platform* platform = dynamic_cast<Platform*>(currentEntity);
    if (!platform) {
        entity = nullptr; sensor = nullptr;
        id.clear(); sensorId.clear();
        return;
    }

    Sensor* currentSensor =
        platform->sensors.getSensor(sensorId.toStdString());
    if (!currentSensor) {
        sensor = nullptr; sensorId.clear();
        setWindowTitle("IR Display (Sensor Removed)");
        return;
    }

    entity = platform;
    sensor = currentSensor;
    setRange(currentSensor->range);

    //  1) GIMBAL PEHLE — scan isse pehle yeh values use kare
    if (IRSensor* ir = dynamic_cast<IRSensor*>(currentSensor)) {
        ir->gimbalAzimuthOffset   = glDisplay->getCurrentAzimuth();
        ir->gimbalElevationOffset = glDisplay->getCurrentElevation();
    }

    // 2) Fresh coords display ko
    glDisplay->setSensorParameter(
        currentSensor->sensorPoseGeo,
        currentSensor->azimuth,
        currentSensor->IRDetectionCoord,
        currentSensor->range * 1000.0f // range stored in km — see irsensor.cpp
        );

    // 3) Paint — track follow
    glDisplay->update();
}

/* Resize the 3D OpenGL Widget while maintaining Aspect Ratio */
void IRDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!glDisplay)
        return;

    if (width() <= 0 || height() <= 0)
        return;

    constexpr float targetWidth = 3840.0f;
    constexpr float targetHeight = 2160.0f;

    const float targetAspect =
        targetWidth / targetHeight;

    const float windowAspect =
        static_cast<float>(width()) /
        static_cast<float>(height());

    int finalWidth;
    int finalHeight;

    if (windowAspect > targetAspect)
    {
        finalHeight = height();
        finalWidth =
            static_cast<int>(finalHeight * targetAspect);
    }
    else
    {
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
void IRDisplay::paintEvent(QPaintEvent * /*event*/)
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
void IRDisplay::debug(const QString &str, const debugIRDisplay &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug() << currentdebugType << str;
    }
}

bool IRDisplay::dbgIsAllow(const debugIRDisplay &currentdebugType)
{
    return ((currentdebugType & debugList) == currentdebugType);
}

IRDisplay::~IRDisplay()
{
}
