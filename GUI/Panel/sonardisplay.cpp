#include "sonardisplay.h"
#include <QPainter>
#include <QtMath>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPointer>
#include <mutex>
std::mutex contactsMutex;

SonarDisplay::SonarDisplay(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");

    connect(&sweepTimer, &QTimer::timeout, this, [=]() {

        qint64 now = QDateTime::currentMSecsSinceEpoch();

        // if no update for some time → simulation paused
        if (now - lastUpdateTime > 200)   // 200ms threshold
        {
            simulationRunning = false;
            sweepTimer.stop();
            return;
        }

        // otherwise continue sweep
        sweepAngle += 2;
        if (sweepAngle >= 360) sweepAngle -= 360;

        update();
    });
}

void SonarDisplay::setSimulation(Simulation* sim)
{
    simulation = sim;
}

void SonarDisplay::setHierarchy(Hierarchy* h) { hierarchy = h; }
void SonarDisplay::selectEntity(Entity* e)    {
    entity = e;
    contacts.clear();
    update();
}
void SonarDisplay::RemoveEntity(QString)      {}
void SonarDisplay::updateRadar()              { update(); }

void SonarDisplay::paintEvent(QPaintEvent *)
{
    updateHeadingFromEntity();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w      = width();
    int h      = height();
    int radius = qMin(w, h) / 2 - 30;
    QPoint center(w / 2, h / 2);

    drawBackground(p);
    drawSonarRing(p, center, radius);
    drawDegreeMarkings(p, center, radius);
    drawBeamCone(p, center, radius);
    drawHeadingLine(p, center, radius);
    drawSweepTrail(p, center, radius);
    drawSweep(p, center, radius);
    drawCenterDot(p, center);
    drawContacts(p, center, radius);
    drawContactLabels(p, center, radius);
    drawRangeButtons(p);
}

void SonarDisplay::drawBackground(QPainter &p)
{
    p.fillRect(rect(), Qt::black);
}

void SonarDisplay::drawSonarRing(QPainter &p, QPoint center, int radius)
{
    QFont font;
    font.setPointSize(7);
    p.setFont(font);

    int intervalKm = maxRange / ringCount;

    for (int i = 1; i <= ringCount; i++)
    {
        int km = intervalKm * i;
        int r  = radius * i / ringCount;

        if (i == ringCount)
        {
            QPen solidPen(QColor(0, 255, 0));
            solidPen.setWidth(2);
            p.setPen(solidPen);
        }
        else
        {
            QPen dottedPen(QColor(0, 255, 0, 130));
            dottedPen.setWidth(1);
            dottedPen.setStyle(Qt::DotLine);
            p.setPen(dottedPen);
        }
        // qDebug() << "[DRAW]"
        //          << QString::fromStdString(c.name)
        //          << "range:" << c.range
        //          << "angle:" << c.angle;
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, r, r);

        p.setPen(QColor(0, 255, 0, 200));
        p.drawText(center.x() + 4, center.y() - r + 12,
                   QString("%1 km").arg(km));
    }
}

void SonarDisplay::drawDegreeMarkings(QPainter &p, QPoint center, int radius)
{
    QFont font;
    font.setPointSize(7);
    p.setFont(font);

    for (int deg = 0; deg < 360; deg += 10)
    {
        double theta = qDegreesToRadians((double)deg - 90);
        double cosT  = cos(theta);
        double sinT  = sin(theta);

        if (deg % 30 == 0)
        {
            int innerR = radius - 12;
            int outerR = radius + 2;

            QPen tickPen(QColor(0, 255, 0, 220));
            tickPen.setWidth(2);
            p.setPen(tickPen);

            p.drawLine(
                QPoint(center.x() + innerR * cosT, center.y() + innerR * sinT),
                QPoint(center.x() + outerR * cosT, center.y() + outerR * sinT)
                );

            int labelR = radius + 20;
            int lx     = center.x() + labelR * cosT;
            int ly     = center.y() + labelR * sinT;

            Qt::Alignment align;
            if      (deg == 0)               align = Qt::AlignHCenter | Qt::AlignBottom;
            else if (deg == 180)             align = Qt::AlignHCenter | Qt::AlignTop;
            else if (deg > 0 && deg < 180)   align = Qt::AlignLeft    | Qt::AlignVCenter;
            else                             align = Qt::AlignRight   | Qt::AlignVCenter;

            p.setPen(QColor(0, 255, 0, 200));
            // p.drawText(QRect(lx - 22, ly - 10, 44, 20), align,
            //            QString("%1°").arg(deg));

            QString text = QString("%1°").arg(deg);

            // text size calculate
            QFontMetrics fm(p.font());
            int textWidth  = fm.horizontalAdvance(text);
            int textHeight = fm.height();

            // position adjust (center properly)
            int tx = lx - textWidth / 2;
            int ty = ly + textHeight / 4;

            p.drawText(tx, ty, text);
        }
        else
        {
            int innerR = radius - 6;
            int outerR = radius + 2;

            QPen tickPen(QColor(0, 255, 0, 120));
            tickPen.setWidth(1);
            p.setPen(tickPen);

            p.drawLine(
                QPoint(center.x() + innerR * cosT, center.y() + innerR * sinT),
                QPoint(center.x() + outerR * cosT, center.y() + outerR * sinT)
                );
        }
    }
}

// ──  Beam cone — sonar ka detection area ──
void SonarDisplay::drawBeamCone(QPainter &p, QPoint center, int radius)
{
    if (m_beamWidth >= 360.0f) return;  // 360° = no cone needed

    float half      = m_beamWidth / 2.0f;
    float startDeg  = - half - 90.0f;
    float spanDeg   = m_beamWidth;

    QRectF rect(center.x() - radius, center.y() - radius,
                radius * 2, radius * 2);

    QPainterPath path;
    path.moveTo(center);
    path.arcTo(rect, startDeg, spanDeg);
    path.closeSubpath();

    p.setBrush(QColor(0, 255, 0, 20));   // faint green fill
    p.setPen(QColor(0, 255, 0, 80));
    p.drawPath(path);
}

// ──  Heading line ──
void SonarDisplay::drawHeadingLine(QPainter &p, QPoint center, int radius)
{
    //double theta = qDegreesToRadians((double)m_heading - 90.0);
    double theta = qDegreesToRadians(-90.0);
    int x = center.x() + radius * cos(theta);
    int y = center.y() + radius * sin(theta);

    QPen pen(QColor(0, 200, 255, 180));  // cyan
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);
    p.drawLine(center, QPoint(x, y));
}

// ── NEW: Sweep trail ──
void SonarDisplay::drawSweepTrail(QPainter &p, QPoint center, int radius)
{
    for (int i = 0; i < m_sweepTrail.size(); i++)
    {
        int    angle = m_sweepTrail[i];
        double theta = qDegreesToRadians((double)angle - 90);
        int    x     = center.x() + radius * cos(theta);
        int    y     = center.y() + radius * sin(theta);

        // Fade — older = more transparent
        float  t     = (float)i / TRAIL_LENGTH;
        int    alpha = (int)(t * 80);

        QPen pen(QColor(0, 255, 0, alpha));
        pen.setWidth(2);
        p.setPen(pen);
        p.drawLine(center, QPoint(x, y));
    }
}

void SonarDisplay::drawSweep(QPainter &p, QPoint center, int radius)
{
    double theta = qDegreesToRadians((double)sweepAngle - 90);
    int x = center.x() + radius * cos(theta);
    int y = center.y() + radius * sin(theta);

    QPen pen(QColor(0, 255, 0, 150));
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(center, QPoint(x, y));
}

void SonarDisplay::drawCenterDot(QPainter &p, QPoint center)
{
    QPen glowPen(QColor(0, 255, 0, 60));
    glowPen.setWidth(6);
    p.setPen(glowPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, 8, 8);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 255, 0, 230));
    p.drawEllipse(center, 4, 4);
}

void SonarDisplay::drawContacts(QPainter &p, QPoint center, int radius)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (auto &c : contacts)
    {
        // Angle normalize (radians → degrees)
        double angle = c.angle;
        if (angle < 6.5)  // radians
        {
            angle = angle * 180.0 / M_PI;
        }

        double theta = qDegreesToRadians(angle - 90);
        double rangeKm = c.range / 1000.0;
        if (rangeKm > maxRange) continue;

        int r = (int)(radius * rangeKm / maxRange);
        int x = center.x() + r * cos(theta);
        int y = center.y() + r * sin(theta);

        // Age based fade
        qint64 age   = now - c.timestamp;
        float  ageRatio   = (float)age / (float)m_pingIntervalMs;
        int    alpha ;

        if (ageRatio < 0.8f)
            alpha = 255;                              // solid
        else
            alpha = qMax(50, (int)(255 * (1.0f - (ageRatio - 0.8f) / 0.2f)));  // last 20% fade

        // Color
        // Strength based on distance (real feel)
        double strength = 1.0 - (c.range / (maxRange * 1000.0));
        strength = std::max(0.0, strength);

        // Angle effect (front strong, side weak, back zero)
        double angleDiff = fabs(c.angle);
        if (angleDiff > 180) angleDiff = 360 - angleDiff;

        // baffle zone
        double angleFactor = 0.0;
        if (angleDiff <= 60)
            angleFactor = 1.0;
        else if (angleDiff <= 120)
            angleFactor = 1.0 - ((angleDiff - 60) / 60.0) * 0.7;
        else
            angleFactor = 0.0;

        // apply
        strength *= angleFactor;

        // convert to intensity
        int intensity = std::min(255, (int)(strength * 255));

        // Color based on type
        QColor dotColor;
        QColor glowColor;

        if (c.type == "submarine")
        {
            dotColor  = QColor(255, intensity / 3, 0, alpha);   // red
            glowColor = QColor(255, 0, 0, alpha / 3);
        }
        else
        {
            dotColor  = QColor(255, 255, intensity / 4, alpha); // yellow
            glowColor = QColor(255, 255, 0, alpha / 3);
        }

        // Glow
        p.setPen(Qt::NoPen);
        p.setBrush(glowColor);
        p.drawEllipse(QPoint(x, y), 8, 8);

        // Dot
        p.setBrush(dotColor);
        p.drawEllipse(QPoint(x, y), 4, 4);
    }
}

// ── NEW: Contact labels ──
void SonarDisplay::drawContactLabels(QPainter &p, QPoint center, int radius)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QFont font;
    font.setPointSize(7);
    p.setFont(font);

    for (auto &c : contacts)
    {
        double angle = c.angle;
        if (angle < 6.5)
        {
            angle = angle * 180.0 / M_PI;
        }

        double theta = qDegreesToRadians(angle - 90);

        double rangeKm = c.range / 1000.0;
        if (rangeKm > maxRange) continue;

        int r = (int)(radius * rangeKm / maxRange);

        int x = center.x() + r * cos(theta);
        int y = center.y() + r * sin(theta);

        qint64 age   = now - c.timestamp;
        int    alpha = qMax(0, (int)(255 - (age / 5000.0) * 255));

        QString label = QString("%1\n%2 km")
                            .arg(QString::fromStdString(c.name))
                            .arg((int)rangeKm);

        p.setPen(QColor(0, 255, 200, alpha));
        p.drawText(QRect(x + 10, y - 10, 80, 30), Qt::AlignLeft, label);
    }
}

void SonarDisplay::drawRangeButtons(QPainter &p)
{
    // ── Position — top right corner ──
    int btnSize = 22;
    int margin  = 8;

    plusRect  = QRect(width() - margin - btnSize,
                     margin,
                     btnSize, btnSize);

    minusRect = QRect(width() - margin - btnSize,
                      margin + btnSize + 4,
                      btnSize, btnSize);

    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    p.setFont(font);

    // + button
    p.setPen(QColor(0, 255, 0, 180));
    p.setBrush(QColor(0, 60, 0, 180));
    p.drawRoundedRect(plusRect, 4, 4);
    p.setPen(QColor(0, 255, 0));
    p.drawText(plusRect, Qt::AlignCenter, "+");

    // - button
    p.setPen(QColor(0, 255, 0, 180));
    p.setBrush(QColor(0, 60, 0, 180));
    p.drawRoundedRect(minusRect, 4, 4);
    p.setPen(QColor(0, 255, 0));
    p.drawText(minusRect, Qt::AlignCenter, "−");

    // Current range label
    QFont rangeFont;
    rangeFont.setPointSize(7);
    p.setFont(rangeFont);
    p.setPen(QColor(0, 255, 0, 180));
    p.drawText(
        QRect(width() - 70, margin + btnSize * 2 + 10, 62, 16),
        Qt::AlignRight | Qt::AlignVCenter,
        QString("Range: %1 km").arg(maxRange)
        );
}

void SonarDisplay::mousePressEvent(QMouseEvent *event)
{
    if (plusRect.contains(event->pos()))
    {
        // + → range badhao (max 100 km)
        if (maxRange < 100)
            maxRange += 20;
        update();
    }
    else if (minusRect.contains(event->pos()))
    {
        // - → range decrease (min 20 km)
        if (maxRange > 20)
            maxRange -= 20;
        update();
    }

    QWidget::mousePressEvent(event);
}

float normalizeRelativeAngle(float bearing, float heading)
{
    float rel = bearing - heading;

    while (rel > 180.0f) rel -= 360.0f;
    while (rel < -180.0f) rel += 360.0f;

    if (rel < 0) rel += 360.0f;

    return rel;
}

void SonarDisplay::updateContacts(const std::vector<DetectionResult>& results)
{

    qDebug() << "[UI UPDATE CONTACTS] incoming:" << results.size();

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // ── Contact tab remove karo jab 2x ping interval ho jaaye ──
    // Real sonar: contact tab tak dikhta hai jab tak next ping nahi aati
    qint64 contactTimeout = m_pingIntervalMs * 2;  // 2 pings ka time

    contacts.erase(
        std::remove_if(contacts.begin(), contacts.end(),
                       [now, contactTimeout](const SonarContact& c) {
                           return (now - c.timestamp) > contactTimeout;
                       }),
        contacts.end());

    for (const auto& r : results)
    {
        if (!r.detected) continue;

        // Debug
        qDebug() << "Contact:" << QString::fromStdString(r.name)
                 << "distance:" << r.distance
                 << "bearing:" << r.bearing;

        bool found = false;
        for (auto& existing : contacts)
        {
            if (existing.name == r.name)
            {
                float relAngle = normalizeRelativeAngle(r.bearing, m_heading);
                existing.angle     = relAngle;
                existing.range     = r.distance;
                existing.timestamp = now;  // ← timestamp refresh
                found = true;
                break;
            }
        }

        if (!found)
        {
            SonarContact c;
            // c.angle     = r.bearing;
            float relAngle = normalizeRelativeAngle(r.bearing, m_heading);

            c.angle     = relAngle;
            c.range     = r.distance;
            c.name      = r.name;
            c.entity    = nullptr;
            c.timestamp = now;

            //  Type detection
            c.type = "surface";
            if (r.name.find("Submarine") != std::string::npos ||
                r.name.find("submarine") != std::string::npos)
            {
                c.type = "submarine";
            }

            contacts.push_back(c);
        }
    }

    // if (!this) return;

    // UI refresh
    update();
}

void SonarDisplay::updateHeadingFromEntity()
{
    if (!entity) return;

    QJsonObject transformJson = entity->getComponent("transform");
    if (transformJson.isEmpty()) return;

    QJsonObject geo = transformJson["geocord"].toObject();

    // IMPORTANT
    m_heading = geo["heading"].toDouble();
}

void SonarDisplay::onSimulationUpdate()
{
    lastUpdateTime = QDateTime::currentMSecsSinceEpoch();

    if (!simulationRunning)
    {
        simulationRunning = true;

        if (!sweepTimer.isActive())
            sweepTimer.start(40);
    }
}

SonarDisplay::~SonarDisplay()
{
    qDebug() << "SonarDisplay destroyed:" << this;

    disconnect();   // 🔥 sab connections tod dega
}
