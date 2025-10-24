
#include "ewdisplay.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QtMath>
#include <QDebug>
#include <core/Debug/console.h>

EWDisplay::EWDisplay(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    padding = 40;
}

QSize EWDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize EWDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

int EWDisplay::heightForWidth(int width) const
{

    return qRound(width * ASPECT_RATIO);
}

void EWDisplay::selectEntity(Entity* entit){
    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        return;
    }
    qDebug()<<"csdvfdsagdsb";
    id = QString::fromStdString(platform->ID);
    entity = platform;
    for (Sensor* s : entity->sensorList) {
        if (s) {
            sensor = s;
            setWindowTitle("Radar Display (" + QString::fromStdString(entity->Name)+")");
            qDebug()<<"csdvfyjkygj";
            break;
        }
    }

}

void EWDisplay::RemoveEntity(QString ID){
    if(id == ID){
        entity = nullptr;
        sensor = nullptr;
        setWindowTitle("Radar Display");
    }
}


void EWDisplay::updateRadar(){
    //qDebug()<<entity;
    //qDebug()<<sensor;
    if(entity&&sensor){
        // qDebug()<<"csdvfysgvsgsjkygj";
        setRange(sensor->ewrange);
        update();
    }
}


void EWDisplay::paintEvent(QPaintEvent * /*event*/)
{
    //qDebug()<<sensor;
    if (width() <= 0 || height() <= 0) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawBackground(p);

    int w = width();
    int h = height();
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);
    drawRadarRing(p, center, outerRadius);
    drawConcentricCircles(p, center, outerRadius);
    drawTicksAndLabels(p, center, outerRadius);
    drawCenterMark(p, center);
    drawTopMarker(p, center, outerRadius);
    drawTargetAndPath(p);
    // Draw target points (if any)
    if (!targets.isEmpty()) {
         qDebug()<<entity;
        p.setPen(QPen(radarGreen, 1, Qt::DotLine));
        for (const Target &t : targets) {
            double per = qBound(0.0, t.radius / 100.0, 1.0);
            double r = per * outerRadius;
            double angleDeg = t.angle;
            double theta = qDegreesToRadians(angleDeg - 90.0);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));
            p.setBrush(Qt::red);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(tx, ty), 4, 4);
            p.setPen(QPen(radarGreen, 1, Qt::DotLine));
            p.drawLine(center, QPoint(tx, ty));
        }
    }
}

void EWDisplay::drawTargetAndPath(QPainter &painter)
{
    int w = width();
    int h = height();
    int centerX = w/2;
    int centerY = h/2;
    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    if(entity&&sensor){
        ang = entity->transform->toEulerAngles().y();
        painter.setBrush(Qt::red);
        for (const Target &target : sensor->ewtargets) {

            int panelhigh = outerRadius;//QWidget::height()-60;
            float per = target.radius/range;
            float radius = panelhigh*per;
            float angle = target.angle;
            //qDebug()<<radius;
            double targetAngle = (angle+90) * M_PI / 180; // Target at 80°
            double targetRadius = radius;
            int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
            int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));
            painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);
            // painter.setPen(QPen(Qt::cyan, 1, Qt::DotLine));
            // QPointF pathPoints[] = {
            //     QPointF(centerX, centerY),
            //     QPointF(centerX, centerY - static_cast<int>(0.4 * radius)),
            //     QPointF(targetX, targetY)
            // };
            // painter.drawPolyline(pathPoints, 3);
            painter.setPen(QPen(Qt::green, 1));
            painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
            painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
        }
    }
}


void EWDisplay::drawBackground(QPainter &p)
{
    p.save();
    p.fillRect(rect(), Qt::black);
    QPen pen(radarGreen, 1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QRect inner(padding, padding, width() - padding*2, height() - padding*2);
    p.drawRect(inner);
    p.restore();
}

void EWDisplay::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

void EWDisplay::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen pen(radarGreen, 1, Qt::DashLine);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int i = 1; i <= ringCount; ++i) {
        double r = outerRadius * (double(i) / double(ringCount + 1));
        QRectF ring(center.x() - r, center.y() - r, r * 2.0, r * 2.0);
        p.drawEllipse(ring);
    }
    p.restore();
}

void EWDisplay::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen majorPen(radarGreen, 2);
    QPen minorPen(radarGreen, 1);
    QFont labelFont("Arial", qMax(8, outerRadius/18), QFont::Bold);
    p.setFont(labelFont);
    for (int deg = 0; deg < 360; deg += (majorTickEvery / minorTicksPerMajor)) {
        double angleDeg = deg - 90.0 + ang;
        double theta = qDegreesToRadians(angleDeg);
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

void EWDisplay::drawCenterMark(QPainter &p, const QPoint &center)
{
    p.save();
    p.setPen(QPen(radarGreen, 2));
    int cross = qMax(6, width()/80);
    p.drawLine(center.x() - cross, center.y(), center.x() + cross, center.y());
    p.drawLine(center.x(), center.y() - cross, center.x(), center.y() + cross);
    QRect sq(center.x() - 5, center.y() - 5, 10, 10);
    p.setBrush(radarGreen);
    p.drawRect(sq);
    p.restore();
}

void EWDisplay::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    double theta = qDegreesToRadians(-90.0);
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
