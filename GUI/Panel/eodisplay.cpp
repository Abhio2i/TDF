#include "eodisplay.h"
#include "core/Hierarchy/Utils/entityutils.h"
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

    //
    //
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

/* Select entity for display */
void EODisplay::selectEntity(Entity* entit)
{
    entity = nullptr;
    id = "";

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("EO Display (No Platform)");
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;
    if (!entity->sensors || !entity->sensors->sensors) {
        return;
    }

    sensor = nullptr;
    for (auto const& pair :  *entity->sensors->sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::EO) {
            //
            sensor = s;
            glDisplay->setSensorParameter(sensor->sensorPoseGeo,sensor->EODetectionCood);
            glDisplay->setSensorParameter(sensor->sensorPoseGeo,sensor->EODetectionCoord);
            setWindowTitle("EO Display (" + QString::fromStdString(entity->Name) + ")");
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
    if (id == ID) {
        entity = nullptr;
        sensor = nullptr;
        setWindowTitle("EO Display");
    }
}

/* Update radar display with new data */
void EODisplay::updateRadar()
{
    if (entity && sensor) {
        setRange(sensor->range);
        glDisplay->setSensorParameter(sensor->sensorPoseGeo,sensor->EODetectionCood);
        glDisplay->setSensorParameter(sensor->sensorPoseGeo,sensor->EODetectionCoord);
        glDisplay->update();
    }
}

/* Resize the 3D OpenGL Widget while maintaining Aspect Ratio */
void EODisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (glDisplay) {
        // Fallback resolution
        float targetWidth = 3840.0f;
        float targetHeight = 2160.0f;

        if (sensor) {
            targetWidth = sensor->resolutionToSize[sensor->eoirResolution4k].first;
            targetHeight = sensor->resolutionToSize[sensor->eoirResolution4k].second;
        }

        // Calculate aspect ratios
        float targetAspect = targetWidth / targetHeight;
        float windowAspect = static_cast<float>(width()) / static_cast<float>(height());

        int finalWidth, finalHeight;

        // Compare the window's aspect ratio to the target aspect ratio
        if (windowAspect > targetAspect) {
            // Window is too wide (Pillarboxing - black bars on left/right)
            finalHeight = height();
            finalWidth = static_cast<int>(finalHeight * targetAspect);
        } else {
            // Window is too tall (Letterboxing - black bars on top/bottom)
            finalWidth = width();
            finalHeight = static_cast<int>(finalWidth / targetAspect);
        }

        // Center the OpenGL widget perfectly
        int xOffset = (width() - finalWidth) / 2;
        int yOffset = (height() - finalHeight) / 2;

        glDisplay->setGeometry(xOffset, yOffset, finalWidth, finalHeight);
    }
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
