#include "eodisplayopengl.h"

#include "core/Hierarchy/Utils/entityutils.h"
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QFont>                                   // For font settings
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>

EODisplayOpenGL::EODisplayOpenGL(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // Set padding
    padding = 40;

    // Enable mouse tracking for hover detection
    setMouseTracking(true);

    //sensor->eoirResolution = sensor->Resolutions::TrueHD;
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize EODisplayOpenGL::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize EODisplayOpenGL::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width and aspect ratio */
int EODisplayOpenGL::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Handle mouse move events for hover detection */
void EODisplayOpenGL::mouseMoveEvent(QMouseEvent *event)
{
    //return;
    lastMousePos = event->pos();

    if (sensor->ewtargets.isEmpty()) {
        hoveredTargetIndex = -1;
        update();
        return;
    }

    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    // Check if mouse is near any target
    int closestIndex = -1;
    float minDistance = 20.0; // Pixel threshold for hover detection

    int i=0;
    for (const Target &t : sensor->ewtargets) {

        // Calculate target position on screen
        float per = t.radius / range;
        if (per < 0.0) per = 0.0;
        if (per > 1.0) per = 1.0;

        float r = per * outerRadius;
        float angleDeg = t.angle;
        float theta = qDegreesToRadians(angleDeg - 90.0);
        int tx = center.x() + int(r * cos(theta));
        int ty = center.y() + int(r * sin(theta));

        // Calculate distance from mouse to target
        float dx = lastMousePos.x() - tx;
        float dy = lastMousePos.y() - ty;
        float distance = sqrt(dx*dx + dy*dy);

        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
        i++;
    }

    if (hoveredTargetIndex != closestIndex) {
        hoveredTargetIndex = closestIndex;
        update(); // Repaint to show/hide labels
    }

    QWidget::mouseMoveEvent(event);
}

/* Handle mouse leave events */
void EODisplayOpenGL::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

EODisplayOpenGL::Angles EODisplayOpenGL::vectorToAngles(
    const float &x,
    const float &y,
    const float &z)
{
    Angles a;
    a.yaw   = std::atan2(y, x) * 180.0 / M_PI;
    a.pitch = std::atan2(z, std::sqrt(x*x + y*y)) * 180.0 / M_PI;
    return a;
}

/* Select entity for display */
/* Select entity for display */
void EODisplayOpenGL::selectEntity(Entity* entit)
{
    entity = nullptr;
    id = "";

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("EO Display (No Platform)");
        update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;
    if (!entity->sensors || !entity->sensors->sensors) {
        update();
        return;
    }
    sensor = nullptr;
    for (auto const& pair :  *entity->sensors->sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::EO) {
            sensor = s;
            setWindowTitle("EO Display (" + QString::fromStdString(entity->Name) + ")");
            break;
        }
    }

    // Reset hover state when entity changes
    hoveredTargetIndex = -1;
    update();
}

/* Remove entity from display */
void EODisplayOpenGL::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
        // Reset window title
        setWindowTitle("EO Display");
        // Reset hover state
        hoveredTargetIndex = -1;
    }
}

/* Update radar display with new data */
void EODisplayOpenGL::updateRadar()
{
    if (entity && sensor) {
        setRange(sensor->range);
        // targets = sensor->ewtargets;
        update();
    } else {
        // Reset targets if no entity/sensor
        targets.clear();
        hoveredTargetIndex = -1;
    }
}

/* Main paint event handler */
void EODisplayOpenGL::paintEvent(QPaintEvent * /*event*/)
{
    QElapsedTimer timer;
    timer.start();  // Start measuring

    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);
    //p.setRenderHint(QPainter::Antialiasing);

    drawBackground(p);
    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    //After Padding Dimension
    QSize freeSpace(width() - (padding*2),height()-(padding*2));


    //drawRadarRing(p, center, outerRadius);
    //Actual Height and Width which is demanded


    //QSize actualScreen(3840,2160);
    if(sensor->eoirResolution != resolution){
        resolution = sensor->eoirResolution;
    }
    QSize actualScreen(sensor->resolutionToSize[sensor->eoirResolution4k].first,
                       sensor->resolutionToSize[sensor->eoirResolution4k].second);
    QString resStr = QString("%1 X %2").arg(
        QString::number(actualScreen.width()),QString::number(actualScreen.height()));
    debug(resStr,D_Resolution);
    // if(resolution != sensor->eoSensor){
    //     resolution = entity->resolution;
    // }


    // Data Type for Dynamic Height and Width
    QSize relativeScreen  = relativeHeightNWidth(actualScreen,freeSpace);

    // Uuse to Increase the size of Object
    float sizeRation       = relativeScreen.width()/(actualScreen.width()+0.0f)*20;

    float relativeRadius = qSqrt(pow(relativeScreen.height(),2) + pow(relativeScreen.width(),2))/2;
    //drawRadarRing(p, center, relativeRadius);

    drawDisplayScreen(p,center,relativeScreen.height(),relativeScreen.width());

    //drawConcentricCircles(p, center, outerRadius);
    //drawTicksAndLabels(p, center, outerRadius);
    drawHorizon(p,center);
    // drawCenterMark(p, center);
    // drawVanishingPoint(p,center);
    //drawTopMarker(p, center, outerRadius);
    if(!sensor)return;
    str = QString("EO Entity: ");

    if(!sensor->eoEntities.isEmpty()){
        str += QString("%1").arg(
            QString::number(sensor->eoEntities.size()));
        auto arr = sensor->eoEntities;
        for(auto entity = sensor->eoEntities.begin();
             entity != sensor->eoEntities.end(); ++entity){

            float vec2const = 100;
            /*
            float posx = center.x() +entity->vec2.x*vec2const*outerRadius;
            float posy = center.y() -entity->vec2.y*vec2const*outerRadius;
            */

            /*
            float posx = center.x() +entity->vec2.x*vec2const*relativeRadius;
            float posy = center.y() -entity->vec2.y*vec2const*relativeRadius;
            */
            float posx = center.x() - (relativeScreen.width()/2)  + (entity->vec2.x/actualScreen.width())*relativeScreen.width();
            float posy = center.y() - (relativeScreen.height()/2) + (entity->vec2.y/actualScreen.height())*relativeScreen.height();
            // If vec2 is normalized (0.0 to 1.0)

            float posxPercent = (posx/relativeScreen.width())*100;
            float posyPercent = (posy/relativeScreen.height())*100;

            str += QString("[Name:%1 Size:%2, Screen:(x:%3,y:%4), View:(x:%5,y:%6,z:%7), "
                           "Heading: %8, Pitch:%9, Imaga Name:%10, Cood.Per:(x:%11,y:%12), Rel Size:%13]").arg(
                           entity->name.c_str(),
                           QString::number(entity->size*100000),
                           QString::number(/*posx*/entity->vec2.x),
                           QString::number(/*posy*/entity->vec2.y),
                           QString::number(entity->vec3.x),
                           QString::number(entity->vec3.y),
                           QString::number(entity->vec3.z),
                           QString::number(entity->relativeHeading),
                           QString::number(entity->relativePitch),
                           imageTypeToName[entity->eoImageType],
                           QString::number(posxPercent),
                           QString::number(posyPercent),
                           QString::number(sizeRation)
                           );
            //Angles angle = vectorToAngles(entity->vec3.x,entity->vec3.y,entity->vec3.z);
            int heading = entity->relativeHeading;
            int pitch   = entity->relativePitch;
            QPoint position(posx,posy);
            drawEntity(p,entity->eoImageType,position,sizeRation*100000*entity->size,heading,pitch);
        }
    }else{
        str += "NA";
    }
    debug(str,D_INIT);
    clearDisplayScreenSurrounding(p,relativeScreen.height(),relativeScreen.width());
    //drawCropRectangle(p,h,w);
    qint64 elapsedMs = timer.elapsed();
    Profiler::currentFrame->csmdisplay = elapsedMs;
}
/* Draw display background */
void EODisplayOpenGL::drawBackground(QPainter &p)
{
    p.save();
    // Fill background
    p.fillRect(rect(), Qt::black);
    // Draw border
    QPen pen(radarGreen, 1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QRect inner(padding, padding, width() - padding*2, height() - padding*2);
    p.drawRect(inner);
    p.restore();
}
// %%% Drawing Methods %%%
/* Draw Vanishing point */
void EODisplayOpenGL::drawVanishingPoint(QPainter &p, const QPoint &center)
{
    p.save();
    p.setPen(QPen(QColor(255, 105, 180)));
    int cross = qMax(15, width()/40);
    p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
    p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);
    // QRect sq(center.x() - 5, center.y() - 5, 10, 10);
    // p.setBrush(QColor(255, 245, 238));
    // p.drawRect(sq);
    p.restore();
}

void EODisplayOpenGL::drawEntity(QPainter &p,const EOImageType &eoImageType, const QPoint &position, float size, int heading =0,int pitch = 0)
{
    p.setPen(QPen(QColor(255, 105, 180)));
    p.setBrush(Qt::NoBrush);

    loadMultiDirectionalImages(p ,heading, pitch);

    QRect rect(position.x() - size/2, position.y() - size/2, size*2, size*2);
    QRectF rectangle(position.x() - size/2, position.y() - size/2, size*2, size*2);
    // if(imgAng == ImageAngles::Top || imgAng == ImageAngles::Bottom){
    //     rect = QRect(position.x() - size/2, position.y() - size/2, size, size*2);
    // }else{
    //     rect = QRect(position.x() - size/2, position.y() - size/2, size*2, size);
    // }
    //p.drawRect(rectangle);
    //QPixmap pixmap(angleImagesPath[static_cast<int>(imgAng)].first);
    QPixmap pixmap(eoImageTypeToFilePath(eoImageType,imgAng));
    if (pixmap.isNull()){
        str += "Image not loaded!";
        return;
    }

    QPoint center = rect.center();
    auto customRectSize = rect.size();
    float x_const = 1 ,y_const =1;
    switch(eoImageType){
    case EOImageType::None:
        //customRectSize /=2;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::One_Engine:
        //customRectSize /=1.8;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::Two_Engine:
        //customRectSize /=2;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::Air_Bus:
        customRectSize *=2;
        x_const = 0.64;
        break;
    }
    QPixmap scaled = pixmap.scaled(customRectSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //QPixmap scaled = pixmap.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // int x = center.x() - (scaled.width()*x_const);
    // int y = center.y() - (scaled.height()*y_const);

    int x = center.x() - scaled.width()  / 2;
    int y = center.y() - scaled.height() / 2;

    QPoint entityPoint(x + (scaled.width()/2), y + (scaled.height()/2));
    p.drawPixmap(x, y, scaled);
    drawTargetDetection(p,entityPoint,rect.size());
}

void EODisplayOpenGL::drawTargetDetection(QPainter &p, const QPoint &center, const QSize &size)
{
    p.setPen(QPen(Qt::red));
    p.setBrush(Qt::NoBrush);

    int semiSide = 7;
    int semiTarget = 2;
    // Top Left
    QPoint TopLeft(center.x()-semiSide,center.y()-semiSide);
    QPoint TopLeftRight(TopLeft.x()+semiTarget,TopLeft.y());
    QPoint TopLeftBottom(TopLeft.x(),TopLeft.y()+semiTarget);
    p.drawLine(TopLeftRight,TopLeft);
    p.drawLine(TopLeftBottom,TopLeft);

    // Top Right
    QPoint TopRight(center.x()+semiSide,center.y()-semiSide);
    QPoint TopRightLeft(TopRight.x()-semiTarget,TopRight.y());
    QPoint TopRightBottom(TopRight.x(),TopRight.y()+semiTarget);
    p.drawLine(TopRightLeft,TopRight);
    p.drawLine(TopRightBottom,TopRight);

    // Bottom Right
    QPoint BottomRight(center.x()+semiSide,center.y()+semiSide);
    QPoint BottomRightLeft(BottomRight.x()-semiTarget,BottomRight.y());
    QPoint BottomRightTop(BottomRight.x(),BottomRight.y()-semiTarget);
    p.drawLine(BottomRightLeft,BottomRight);
    p.drawLine(BottomRightTop,BottomRight);

    // Bottom Left
    QPoint BottomLeft(center.x()-semiSide,center.y()+semiSide);
    QPoint BottomLeftTop(BottomLeft.x(),BottomLeft.y()-semiTarget);
    QPoint BottomLeftRight(BottomLeft.x()+semiTarget,BottomLeft.y());
    p.drawLine(BottomLeftTop,BottomLeft);
    p.drawLine(BottomLeftRight,BottomLeft);
}

void EODisplayOpenGL::drawEntity(QPainter &p, const EOImageType &eoImageType, const QPoint &position, float relativeRadius, float size, int heading, int pitch)
{
    p.setPen(QPen(QColor(255, 105, 180)));
    p.setBrush(Qt::NoBrush);

    loadMultiDirectionalImages(p ,heading, pitch);

    QRect rect(position.x() - size/2, position.y() - size/2, size*2, size*2);
    // if(imgAng == ImageAngles::Top || imgAng == ImageAngles::Bottom){
    //     rect = QRect(position.x() - size/2, position.y() - size/2, size, size*2);
    // }else{
    //     rect = QRect(position.x() - size/2, position.y() - size/2, size*2, size);
    // }

    //QPixmap pixmap(angleImagesPath[static_cast<int>(imgAng)].first);
    QPixmap pixmap(eoImageTypeToFilePath(eoImageType,imgAng));
    if (pixmap.isNull()){
        str += "Image not loaded!";
        return;
    }

    QPoint center = rect.center();
    auto customRectSize = rect.size();
    float x_const = 1 ,y_const =1;
    switch(eoImageType){
    case EOImageType::None:
        //customRectSize /=2;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::One_Engine:
        //customRectSize /=1.8;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::Two_Engine:
        //customRectSize /=2;
        x_const = 0.68;
        y_const = 0.70;
        break;
    case EOImageType::Air_Bus:
        customRectSize *=2;
        x_const = 0.64;
        break;
    }
    QPixmap scaled = pixmap.scaled(customRectSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //QPixmap scaled = pixmap.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = center.x() - (scaled.width()*x_const);
    int y = center.y() - (scaled.height()*y_const);
    /*
    int x = center.x() - scaled.width()  / 2;
    int y = center.y() - scaled.height() / 2;
    */
    p.drawPixmap(x, y, scaled);
}

void EODisplayOpenGL::drawHorizon(QPainter &p, const QPoint &center)
{
    p.save();
    p.setPen(radarGreen);
    p.drawLine(padding, height()/2, width()-padding, height()/2);
    p.drawLine(width()/2, padding, width()/2, height() - padding);
    // int cross = qMax(15, width()/40);
    // p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
    // p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);
    // QRect sq(center.x() - 5, center.y() - 5, 10, 10);
    // p.setBrush(QColor(255, 245, 238));
    // p.drawRect(sq);
    p.restore();
}

void EODisplayOpenGL::loadMultiDirectionalImages(QPainter &p, int heading, int pitch)
{
    if(heading < 45 && heading > -45){
        imgAng = ImageAngles::Back;
    }else if(heading > 45 && heading < 135){
        imgAng = ImageAngles::Right;
    }else if((heading > 135 && heading < 180) || (heading < -135 && heading > -180)){
        imgAng = ImageAngles::Front;
    }else if(heading < -45 && heading > -135){
        imgAng = ImageAngles::Left;
    }
    if(pitch < -45){
        imgAng = ImageAngles::Bottom;
    }else if(pitch > 45){
        imgAng = ImageAngles::Top;
    }

}
/* Draw center cross mark */
void EODisplayOpenGL::drawCenterMark(QPainter &p, const QPoint &center)
{
    p.save();
    p.setPen(QPen(Qt::yellow, 2));
    int cross = qMax(6, width()/80);
    p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
    p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);
    QRect sq(center.x() - 5, center.y() - 5, 10, 10);
    p.setBrush(Qt::yellow);
    p.drawRect(sq);
    p.restore();
}

/* Draw concentric radar circles */
void EODisplayOpenGL::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen pen(radarGreen, 1, Qt::DashLine);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int i = 1; i <= ringCount; ++i) {
        float r = outerRadius * (float(i) / float(ringCount + 1));
        QRectF ring(center.x() - r, center.y() - r, r * 2.0, r * 2.0);
        p.drawEllipse(ring);
    }
    p.restore();
}

/* Draw radar ticks and labels */
void EODisplayOpenGL::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen majorPen(radarGreen, 2);
    QPen minorPen(radarGreen, 1);
    QFont labelFont("Arial", qMax(8, outerRadius/18), QFont::Bold);
    p.setFont(labelFont);
    for (int deg = 0; deg < 360; deg += (majorTickEvery / minorTicksPerMajor)) {
        float angleDeg = deg - 90.0 + ang;
        float theta = qDegreesToRadians(angleDeg);
        bool isMajor = (deg % majorTickEvery == 0);
        int tickOut = outerRadius;
        int tickIn = isMajor ? outerRadius - qMax(16, outerRadius/12) : outerRadius - qMax(6, outerRadius/24);
        int x1 = center.x() + int(tickOut * cos(theta));
        int y1 = center.y() + int(tickOut * sin(theta));
        int x2 = center.x() + int(tickIn * cos(theta));
        int y2 = center.y() + int(tickIn * sin(theta));
        p.setPen(isMajor ? majorPen : minorPen);
        p.drawLine(QPoint(x1, y1), QPoint(x2, y2));
        if (isMajor) {
            // Draw degree labels
            int labelRadius = outerRadius + qMax(16, outerRadius/10);
            int lx = center.x() + int(labelRadius * cos(theta));
            int ly = center.y() + int(labelRadius * sin(theta));
            int heading = (deg % 360);
            QString text = QString("%1").arg(heading, 3, 10, QChar('0'));
            QRect textRect = QFontMetrics(p.font()).boundingRect(text);
            int tx = lx - textRect.width()/2;
            int ty = ly + textRect.height()/2;
            p.setPen(radarGreen);
            p.drawText(tx, ty, text);
        }
    }
    p.restore();
}



/* Draw top marker triangle and label */
void EODisplayOpenGL::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    float theta = qDegreesToRadians(-90.0);
    int triCenterX = center.x() + int((outerRadius + 4) * cos(theta));
    int triCenterY = center.y() + int((outerRadius + 4) * sin(theta));
    int triW = qMax(6, outerRadius/20);
    QPoint pts[3] = {
        QPoint(triCenterX, triCenterY - triW),
        QPoint(triCenterX - triW, triCenterY + (triW/2)),
        QPoint(triCenterX + triW, triCenterY + (triW/2))
    };
    p.setPen(QPen(radarGreen, 1));
    p.setBrush(radarGreen);
    p.drawPolygon(pts, 3);
    QFont f("Arial", qMax(6, outerRadius/25), QFont::Bold);
    p.setFont(f);
    QString txt = "000";
    QRect txtRect = QFontMetrics(f).boundingRect(txt);
    int tx = triCenterX - txtRect.width()/2;
    int ty = triCenterY + triW + txtRect.height() + 1;
    p.setPen(radarGreen);
    p.drawText(tx, ty, txt);
    p.restore();
}

/* Draw targets and their paths */
void EODisplayOpenGL::drawTargetAndPath(QPainter &painter)
{
    int w = width();
    int h = height();
    int centerX = w/2;
    int centerY = h/2;
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;

    if (entity && sensor) {
        // Get entity angle
        ang = entity->transform->toEulerAngles().y();
        painter.setBrush(Qt::red);

        for (const Target &target : sensor->csmtargets) {
            // Calculate target position
            int panelhigh = outerRadius;
            float per = target.radius/range;
            float radius = panelhigh*per;
            float angle = target.angle;
            float targetAngle = (angle + 90) * M_PI / 180;
            float targetRadius = radius;
            int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
            int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));

            // Draw target point
            painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);

            // Draw target labels
            painter.setPen(QPen(Qt::green, 1));
            painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
            painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
        }
    }
}

void EODisplayOpenGL::drawCropRectangle(QPainter &p, const int &h, const int &w)
{
    p.save();
    QRect top   (0,0,w,padding);
    QRect left  (0,0,padding,h);
    QRect bottom(0,h-padding+1,w,padding);
    QRect right (w-padding+1,0,padding,h);
    p.fillRect(top   ,Qt::black);
    p.fillRect(left  ,Qt::black);
    p.fillRect(bottom,Qt::black);
    p.fillRect(right ,Qt::black);
    p.restore();
}

/* Draw outer radar ring */
void EODisplayOpenGL::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

/* Draw Display Rectangle of Display Screen */
void EODisplayOpenGL::drawDisplayScreen(QPainter &p, const QPoint &center, const int &screenHeight, const int &screenWidth)
{
    p.save();

    // Set pen: Color (green), Width (5), Style (Solid)
    p.setPen(QPen(radarGreen, 1, Qt::SolidLine));

    // Set brush to transparent (hollow)
    p.setBrush(QBrush(Qt::transparent));

    // int rectCoodX = ;
    // int rectCoodY = ;
    const QPoint cornerOfRect(center.x() - screenWidth/2, center.y() - screenHeight/2);

    // Draw the rectangle (x, y, width, height)
    p.drawRect(cornerOfRect.x(),cornerOfRect.y(), screenWidth, screenHeight);
    p.restore();
}

void EODisplayOpenGL::clearDisplayScreenSurrounding(QPainter &p, const int &screenHeight, const int &screenWidth)
{
    p.save();
    auto heightPadding = (height()-screenHeight)/2;
    auto widthPadding  = (width()-screenWidth)/2;
    QRect top   (0,0,width(),heightPadding);//(width()-screenWidth)/2
    QRect left  (0,0,widthPadding,height());
    QRect bottom(0,height()-heightPadding+1,width(),heightPadding);
    QRect right (width()-widthPadding+1,0,widthPadding,height());
    p.fillRect(top   ,Qt::black);
    p.fillRect(left  ,Qt::black);
    p.fillRect(bottom,Qt::black);
    p.fillRect(right ,Qt::black);
    p.restore();
}

/* To Get x:y = 1:y in which we get Y float value; */
float EODisplayOpenGL::ratioOfDimension(const QSize &screenDimension)
{
    float ratio = static_cast<float>(screenDimension.height())/static_cast<float>(screenDimension.width());
    return ratio;
}

QSize EODisplayOpenGL::relativeHeightNWidth(
    const QSize &actualScreen,
    const QSize &freeSpace)
{
    float actualScreenRatio = ratioOfDimension(actualScreen);
    float freeSpaceRation   = ratioOfDimension(freeSpace);
    int width  = 0;
    int height = 0;
    // if(actualScreen.height() <= freeSpace.height()&&
    //    actualScreen.width()  <= freeSpace.width()){
    //     goto smallerHeightNWidth;
    // }else if(actualScreen.height() > freeSpace.height()&&
    //          actualScreen.width()  > freeSpace.width()){
    //     goto smallerHeightNWidth;
    // }else if(actualScreen.height() < freeSpace.height()&&
    //          actualScreen.width()  > freeSpace.width()){

    // }
    //smallerHeightNWidth:
    if(actualScreenRatio < freeSpaceRation){
        width  = freeSpace.width();
        height = width*actualScreenRatio;
    }else{
        height = freeSpace.height();
        width  = height/actualScreenRatio;
    }
    return QSize(width,height);
}

void EODisplayOpenGL::debug(const QString &str, const debugEODisplay &currentdebugType)
{
    if(dbgIsAllow(currentdebugType) && (currentdebugType == D_NULL)){
        return;
    }
    if(dbgIsAllow(currentdebugType)){
        qDebug()<<currentdebugType<<str;
    }
}

bool EODisplayOpenGL::dbgIsAllow(const debugEODisplay &currentdebugType)
{
    bool InsideList = ((currentdebugType & debugList) == currentdebugType);
    return InsideList;
}
