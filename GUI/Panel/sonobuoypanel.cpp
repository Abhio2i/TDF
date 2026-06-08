#include "sonobuoypanel.h"
#include "Inspector/inspector.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "qevent.h"
#include "core/Hierarchy/Utils/entityutils.h"
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QSvgRenderer>
#include <QFont>                                   // For font settings
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debug output
#include <core/Debug/console.h>

SonoBuoyPanel::SonoBuoyPanel(QWidget *parent)
    : QWidget(parent), hoveredTargetIndex(-1)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    padding = 40;
    setMouseTracking(true);
    sensorDropdown = new QComboBox(this);
    sensorDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    sensorDropdown->hide();
    connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SonoBuoyPanel::onSensorSelected);

    // --- + and - Buttons Configuration ---
    QString buttonStyle =
        "QPushButton { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-weight: bold; font-size: 16px; "
        "min-width: 10px; min-height: 10px; max-width: 10px; max-height: 10px; "
        "border-radius: 4px; }"
        "QPushButton:hover { background-color: #003300; }"
        "QPushButton:pressed { background-color: #00ff00; color: black; }";

    zoomInButton = new QPushButton("+", this);
    zoomInButton->setStyleSheet(buttonStyle);

    zoomOutButton = new QPushButton("-", this);
    zoomOutButton->setStyleSheet(buttonStyle);

    // --- Layout Management ---
    // Buttons ko horizontal line me lagane ke liye
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    // buttonLayout->addStretch(); // Buttons ko right side me dhakelne ke liye
    buttonLayout->addWidget(zoomInButton);
    buttonLayout->addWidget(zoomOutButton);
    buttonLayout->addStretch();
    buttonLayout->setSpacing(5);

    // Main layout pooray widget ke liye
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch(); // Baaki bacha space niche khali rakhne ke liye
    mainLayout->setContentsMargins(10, 10, 10, 10); // Corner se thoda gap

    // --- Connect Signals to Slots ---
    connect(zoomInButton, &QPushButton::clicked, this, &SonoBuoyPanel::onZoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &SonoBuoyPanel::onZoomOut);

    m_svgRenderer.load(QString(":/svg/images/world.svg"));
}

void SonoBuoyPanel::onZoomOut()
{
    zoomLevel *= 2.f; // 10% zoom in
    if (zoomLevel > 12800.0) zoomLevel = 12800.0; // Max zoom limit
    this->update(); // Paint event ko call karke map re-render karne ke liye
}

void SonoBuoyPanel::onZoomIn()
{
    zoomLevel /= 2.f; // 10% zoom out
    if (zoomLevel < 25) zoomLevel = 25; // Min zoom limit
    this->update();
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize SonoBuoyPanel::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize SonoBuoyPanel::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width and aspect ratio */
int SonoBuoyPanel::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Handle mouse move events for hover detection */
void SonoBuoyPanel::mouseMoveEvent(QMouseEvent *event)
{
    return;
    lastMousePos = event->pos();

    if (sensor->detection.isEmpty()) {
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
    double minDistance = 20.0;

    int i=0;
    for (const Sonobuoy::SonobuoyOutput &t : sensor->detection) {

        // Calculate target position on screen
        double per = t.radius / range;
        if (per < 0.0) per = 0.0;
        if (per > 1.0) per = 1.0;

        double r = per * outerRadius;
        double angleDeg = t.bearing;
        double theta = qDegreesToRadians(angleDeg - 90.0);
        int tx = center.x() + int(r * cos(theta));
        int ty = center.y() + int(r * sin(theta));

        // Calculate distance from mouse to target
        double dx = lastMousePos.x() - tx;
        double dy = lastMousePos.y() - ty;
        double distance = sqrt(dx*dx + dy*dy);

        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
        i++;
    }

    if (hoveredTargetIndex != closestIndex) {
        hoveredTargetIndex = closestIndex;
        update();
    }

    QWidget::mouseMoveEvent(event);
}

/* Handle mouse leave events */
void SonoBuoyPanel::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hoveredTargetIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

/* Select and configure entity for display */
void SonoBuoyPanel::selectEntity(Entity* entit)
{
    entity = nullptr;
    id = "";

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("SonoBuoyPanel (No Platform)");
        update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    sensor = nullptr;
    sensorlist.clear();
    // Remote DIS entities have no weapons component
    if (!entity->weapons || !entity->weapons->weapons) {
        setWindowTitle("SonoBuoyPanel (" +
                       QString::fromStdString(entity->Name) + ")");
        update();
        return;
    }
    for (auto const& pair :  *entity->weapons->weapons) {
        Weapon* s = pair.second;
        if (s ) {

            Sonobuoy* sono = dynamic_cast<Sonobuoy*>(s);
            if(sono){
                if(sensor == nullptr){
                    sensor = sono;
                }
                sensorlist.append(sono);
            }
            setWindowTitle("SonoBuoyPanel (" + QString::fromStdString(entity->Name) + ")");

        }
    }

    // Reset hover state when entity changes
    hoveredTargetIndex = -1;
    updateDropdown();

    update();
}

/* Remove entity if ID matches */
void SonoBuoyPanel::RemoveEntity(QString ID)
{
    if (id == ID) {
        // Clear entity and sensor
        entity = nullptr;
        sensor = nullptr;
        sensorlist.clear();
        // Reset window title
        setWindowTitle("SonoBuoyPanel");
        // Reset hover state
        hoveredTargetIndex = -1;
    }
}

/* Update radar display data */
void SonoBuoyPanel::updateRadar()
{
    if (entity && sensor) {
        // Set radar range and trigger repaint
        setRange(sensor->range);
        update();
    } else {
        // Reset targets if no entity/sensor
        targets.clear();
        if (sensorDropdown) sensorDropdown->hide();
        hoveredTargetIndex = -1;
    }
}

/* Main paint event handler */
void SonoBuoyPanel::paintEvent(QPaintEvent * /*event*/)
{
    QElapsedTimer timer;
    timer.start();  // Start measuring
    if (width() <= 0 || height() <= 0) return;

    // Initialize painter
    QPainter p(this);
    if(!entity) return;
    //p.setRenderHint(QPainter::Antialiasing);
    QVector3D position = entity->transform->translation();
    QPointF pos(position.x(),-position.z());
    // Draw display components
    drawBackground(p);
    int w = width();
    int h = height();

    // SVG ka original size lein
    QSize svgSize = m_svgRenderer.defaultSize();
    p.fillRect(this->rect(), QColor("#1a2634"));

    float mapwidth = 40075.0164;//20037.5082;//111.31949*180.0f;

    // Widget ke size ke hisab se aspect ratio maintain karte hue rect nikalein
    // QRect targetRect = this->rect();
    // float level = mapwidth/zoomLevel;
    // targetRect.setWidth(targetRect.width()*level);
    // targetRect.setHeight(targetRect.height()*level);
    // svgSize.scale(targetRect.size(), Qt::KeepAspectRatio);

    // // Centered bounding box banayein
    // QRect centerRect(0, 0, svgSize.width(), svgSize.height());
    // centerRect.moveCenter(this->rect().center());
    // centerRect.translate((-pos.x()/zoomLevel)*mapwidth, (-pos.y()/zoomLevel)*mapwidth);
    // // Ab centered rect me render karein
    // m_svgRenderer.render(&p, centerRect);

    int outerDiameter = qMin(w - padding*2, h - padding*2);
    int outerRadius = outerDiameter / 2;
    QPoint center(w / 2, h / 2);

    QFont font("Arial", 10, QFont::Bold);
    p.setFont(font);
    p.setPen(QColor(0, 255, 0, 255));
    // Fixed screen coordinates ka upyog karein
    p.drawText(QPointF(0+20, h-20),QString::number(zoomLevel)+"km");
    p.drawLine(5, h-10, w-5, h-10);
    p.drawLine(5, h-20, 5, h-10);
    p.drawLine(w-5, h-20, w-5, h-10);
    p.setBrush(Qt::yellow);
    p.setPen(Qt::NoPen);
    p.drawEllipse(center, 4, 4);

    p.save(); // Current painter state save karein[cite: 2]
    p.translate(center.x(), center.y()); // Painter ko aircraft ke center position par le jayein[cite: 2]
    p.rotate(entity->transform->getHeading()); // Aircraft ko uske heading angle par rotate karein[cite: 2]

    p.setBrush(Qt::blue); // Normally blue[cite: 2]
    p.setPen(QPen(Qt::white, 0.5)); // Patli white outline
    // Aircraft Polygon define karein (origin 0,0 nose ke thoda niche hai)
    QPolygon aircraft;
    aircraft << QPoint(0, -12)   // Nose (Top)
             << QPoint(1, -5)  // Right Fuselage front
             << QPoint(12, 0)    // Right Wingtip front
             << QPoint(12, 2)    // Right Wingtip back
             << QPoint(1, 5)   // Right Fuselage back
             << QPoint(5, 10)    // Right Tail stabilizer
             << QPoint(0, 8)     // Rear center (notch)
             << QPoint(-5, 10)   // Left Tail stabilizer
             << QPoint(-1, 5)  // Left Fuselage back
             << QPoint(-12, 2)   // Left Wingtip back
             << QPoint(-12, 0)   // Left Wingtip front
             << QPoint(-1, -5);// Left Fuselage front
    // Polygon automatically connect back to nose.

    p.drawPolygon(aircraft);// Polygon draw karein
    p.restore(); // Painter state restore karein[cite: 2]
    // qDebug()<<w<<","<<zoomLevel;
    p.setBrush(Qt::green);
    p.setPen(Qt::NoPen);
    QVector<std::string> output;
    for (const Sonobuoy* sensor : sensorlist) {
        if(!sensor || !sensor->transform) continue;
        auto it = entity->weapons->weapons->find(sensor->ID);
        if (it != entity->weapons->weapons->end() && it->second ) {
            QVector3D Sposition = sensor->transform->translation();
            QPointF senpos(Sposition.x(),-Sposition.z());
            QPointF relPos((senpos.x()-pos.x()),(senpos.y()-pos.y()));
            relPos.setX(((relPos.x()/zoomLevel)*center.x())+center.x());
            relPos.setY(((relPos.y()/zoomLevel)*center.x())+center.y());
            float size = 4;
            if(sensor->collider->CollideRadius>5000){
                size = 8.f;
                p.setBrush(Qt::red);
            }else{
                p.setBrush(Qt::green);
            }
            p.drawEllipse(relPos, size, size);
            if (!sensor->detection.isEmpty()) {
                int i=0;
                for (const Sonobuoy::SonobuoyOutput &t : sensor->detection) {
                    if(t.entity && t.entity->transform && !output.contains(t.entity->ID)){
                        output.append(t.entity->ID);

                        QVector3D Subposition = t.entity->transform->translation();
                        QPointF senpos(Subposition.x(),-Subposition.z());
                        // QPointF relPos((senpos.x()-pos.x())+center.x(),(senpos.y()-pos.y())+center.y());
                        QPointF relPos((senpos.x()-pos.x()),(senpos.y()-pos.y()));
                        relPos.setX(((relPos.x()/zoomLevel)*center.x())+center.x());
                        relPos.setY(((relPos.y()/zoomLevel)*center.x())+center.y());

                        p.save(); // Coordinate system save karein
                        p.translate(relPos); // Painter ko ship ki position par le jayein
                        p.rotate(t.entity->transform->getHeading()); // Ship ko uske original angle par rotate karein

                        p.setBrush(Qt::red);
                        p.setPen(QPen(Qt::white, 1)); // Outline ke liye

                        // Ship ka polygon (simple triangle ya pentagon)
                        QPolygon ship;
                        ship << QPoint(0, -12)   // Bow (Front Point)
                             << QPoint(5, -4)    // Front-Right
                             << QPoint(5, 10)    // Back-Right (Stern)
                             << QPoint(-5, 10)   // Back-Left (Stern)
                             << QPoint(-5, -4);  // Front-Left

                        p.drawPolygon(ship);
                        p.restore(); // Coordinate system restore karein

                    }
                }
            }
        }
    }
    return;

    drawRadarRing(p, center, outerRadius);
    drawConcentricCircles(p, center, outerRadius);
    drawTicksAndLabels(p, center, outerRadius);
    drawCenterMark(p, center);
    drawTopMarker(p, center, outerRadius);

    if(!sensor)return;
    // Draw targets with dotted lines and labels
    if (!sensor->detection.isEmpty()) {
        int i=0;
        for (const Sonobuoy::SonobuoyOutput &t : sensor->detection) {
            // const Target &t = targets[i];
            double per = t.radius / range;
            if (per < 0.0) per = 0.0;
            if (per > 1.0) per = 1.0;

            double r = per * outerRadius;
            double angleDeg = t.bearing-90;
            double theta = qDegreesToRadians(angleDeg);
            int tx = center.x() + int(r * cos(theta));
            int ty = center.y() + int(r * sin(theta));

            // Draw dotted line from center to target
            p.setPen(QPen(radarGreen, 1, Qt::DotLine));
            p.drawLine(center, QPoint(tx, ty));

            // Draw target dot - blue normally, red if hovered
            if (i == hoveredTargetIndex) {
                p.setBrush(Qt::red);
            } else {
                p.setBrush(Qt::blue);
            }
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(tx, ty), 4, 4);

            // Draw labels ONLY if this target is hovered
            if (i == hoveredTargetIndex) {
                p.setPen(QPen(Qt::yellow, 1));
                QFont font = p.font();
                font.setPointSize(8);
                p.setFont(font);

                // Show angle and distance for hovered target
                QString angleText = QString("A:%1°").arg(angleDeg, 0, 'f', 1);
                QString distText = QString("D:%1").arg(t.radius, 0, 'f', 1);

                // Draw text at target position
                p.drawText(tx + 6, ty - 6, angleText);
                p.drawText(tx + 6, ty + 12, distText);


                Platform* targetPlatform = dynamic_cast<Platform*>(t.entity);
                if (targetPlatform) {
                    QString nameText = QString::fromStdString(targetPlatform->Name);
                    p.drawText(tx + 6, ty + 30, nameText);
                }
            }
            i++;
        }
    } else if (entity && sensor) {
        // No targets message
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(10);
        p.setFont(font);
        QRect textRect = p.fontMetrics().boundingRect("No ESM Targets Detected");
        p.drawText(center.x() - textRect.width()/2, center.y(), "No ESM Targets Detected");
    }

    qint64 elapsedMs = timer.elapsed();
    // Profiler::currentFrame->SonoBuoyPanel = elapsedMs;
}

// %%% Drawing Methods %%%
/* Draw targets and their paths */
void SonoBuoyPanel::drawTargetAndPath(QPainter &painter)
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

        // for (const Target &target : sensor->esmtargets) {
        //     // Calculate target position
        //     int panelhigh = outerRadius;
        //     float per = target.radius/range;
        //     float radius = panelhigh*per;
        //     float angle = target.angle;
        //     double targetAngle = (angle + 90) * M_PI / 180;
        //     double targetRadius = radius;
        //     int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
        //     int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));

        //     // Draw target point
        //     painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);

        //     // Draw target labels
        //     painter.setPen(QPen(Qt::green, 1));
        //     painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
        //     painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
        // }
    }
}

/* Draw display background */
void SonoBuoyPanel::drawBackground(QPainter &p)
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

/* Draw outer radar ring */
void SonoBuoyPanel::drawRadarRing(QPainter &p, const QPoint &center, int outerRadius)
{
    p.save();
    QPen ringPen(radarGreen, 2);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    QRectF circle(center.x() - outerRadius, center.y() - outerRadius, outerRadius*2.0, outerRadius*2.0);
    p.drawEllipse(circle);
    p.restore();
}

/* Draw concentric radar circles */
void SonoBuoyPanel::drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius)
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

/* Draw radar ticks and labels */
void SonoBuoyPanel::drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius)
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

/* Draw center cross mark */
void SonoBuoyPanel::drawCenterMark(QPainter &p, const QPoint &center)
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

/* Draw top marker triangle and label */
void SonoBuoyPanel::drawTopMarker(QPainter &p, const QPoint &center, int outerRadius)
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
void SonoBuoyPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (sensorDropdown) {
        int dropW = 120;
        int dropH = 22;
        sensorDropdown->setGeometry(width() - dropW - padding, 4, dropW, dropH);
    }
}

void SonoBuoyPanel::updateDropdown()
{
    if (!sensorDropdown) return;

    QSignalBlocker blocker(sensorDropdown);
    sensorDropdown->clear();

    if (sensorlist.isEmpty()) {
        sensorDropdown->hide();
        return;
    }

    for (int i = 0; i < sensorlist.size(); ++i) {
        Sonobuoy* s = sensorlist[i];
        QString name = s ? QString::fromStdString(s->Name) : QString("ESM %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("ESM %1").arg(i + 1);
        sensorDropdown->addItem(name);
    }

    int currentIdx = sensorlist.indexOf(sensor);
    if (currentIdx >= 0)
        sensorDropdown->setCurrentIndex(currentIdx);

    // if (sensorlist.size() > 1)
    //     // sensorDropdown->show();
    // else
    //     sensorDropdown->hide();

    sensorDropdown->setGeometry(width() - 120 - padding, 4, 120, 22);
}

void SonoBuoyPanel::onSensorSelected(int index)
{
    if (index < 0 || index >= sensorlist.size()) return;
    sensor = sensorlist[index];
    hoveredTargetIndex = -1;
    if (sensor)
        setRange(sensor->range);
    update();
}
