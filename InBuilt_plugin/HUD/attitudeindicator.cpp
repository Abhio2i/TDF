#include "attitudeindicator.h"
#include "qwidget.h"
#include <cmath>

AttitudeIndicator::AttitudeIndicator(QWidget *parent)
    : QWidget(parent), pitch(0.0f), roll(0.0f), speed(100.0f), altitude(2500.0f), heading(135.0f) {

    setWindowTitle("Advanced Primary Flight Display (PFD)");
    resize(600, 500); // UI elements ke liye thoda bada size
    setFocusPolicy(Qt::StrongFocus);

    // setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    // 🌟 Close Button ko remove karne aur Always on Top rakhne ka exact tarika:
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowStaysOnTopHint;             // Always on top lagaya
    flags &= ~Qt::WindowCloseButtonHint;           // Close button ko flag se nikal diya (Clear)
    flags |= Qt::CustomizeWindowHint;              // Customization enable ki

    setWindowFlags(flags);
}

void AttitudeIndicator::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    QPoint center(w / 2, h / 2);

    // Background base
    painter.fillRect(rect(), QColor(15, 15, 15));

    // 1. Center Artificial Horizon (Pitch & Roll)
    drawHorizon(painter, center, w, h);

    // 2. Left Side: Speed Tape (Ladder moves up/down)
    drawSpeedTape(painter, h);

    // 3. Right Side: Altitude Tape (Ladder moves up/down)
    drawAltitudeTape(painter, w, h);

    // 4. Bottom Side: Heading Tape (Bar moves left/right)
    drawHeadingTape(painter, w, h);

    // 5. Text Readouts: Pitch & Roll numeric text
    drawDataReadouts(painter, w, h);
}

// ---- 1. HORIZON DRAWING ----
void AttitudeIndicator::drawHorizon(QPainter &painter, const QPoint &center, int w, int h) {
    painter.save();

    // Sirf center area me horizon dikhane ke liye clip path lagaya
    painter.setClipRect(100, 50, w - 200, h - 130);

    painter.translate(center.x(), center.y() - 20);
    painter.rotate(roll);
    painter.translate(0, pitch * 3.0f); // 3 pixels per degree

    // Sky & Ground
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(100, 120, 200));
    painter.drawRect(-w, -h * 2, w * 2, h * 2);
    painter.setBrush(QColor(140, 50, 40));
    painter.drawRect(-w, 0, w * 2, h * 2);

    // Pitch Ladder Lines
    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    for (int i = -40; i <= 40; i += 10) {
        if (i == 0) continue;
        int yOffset = -i * 3.0f;
        painter.drawLine(-40, yOffset, 40, yOffset);
        // Numbers on ladder
        painter.drawText(-65, yOffset + 5, QString::number(std::abs(i)));
        painter.drawText(45, yOffset + 5, QString::number(std::abs(i)));
    }
    painter.restore();

    // Static Aircraft Symbol (Fixed in Center)
    painter.setPen(QPen(QColor(240, 200, 30), 5));
    painter.drawLine(center.x() - 60, center.y() - 20, center.x() - 20, center.y() - 20);
    painter.drawLine(center.x() + 20, center.y() - 20, center.x() + 60, center.y() - 20);
    painter.setBrush(QColor(240, 200, 30));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(center.x(), center.y() - 20), 4, 4);
}

// ---- 2. SPEED TAPE (MOVING LADDER) ----
void AttitudeIndicator::drawSpeedTape(QPainter &painter, int h) {
    int tapeX = 30;
    int tapeW = 60;
    int visibleHeight = h - 130;
    int centerY = 50 + visibleHeight / 2;

    painter.fillRect(tapeX, 50, tapeW, visibleHeight, QColor(30, 30, 30, 200));
    painter.save();
    painter.setClipRect(tapeX, 50, tapeW, visibleHeight);

    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 10));

    // Dynamic ladder movement logic
    int startSpeed = ((int)speed / 10) * 10 - 40;
    for (int s = startSpeed; s <= speed + 40; s += 10) {
        if (s < 0) continue;
        int yOffset = centerY - (s - speed) * 2.0f; // 2 pixels per knot
        painter.drawLine(tapeX + tapeW - 15, yOffset, tapeX + tapeW, yOffset);
        painter.drawText(tapeX + 5, yOffset + 5, QString::number(s));
    }
    painter.restore();

    // Current Speed Indicator Box (Fixed Pointer)
    painter.fillRect(tapeX, centerY - 12, tapeW + 5, 24, QColor(0, 0, 0));
    painter.setPen(QPen(Qt::green, 2));
    painter.drawRect(tapeX, centerY - 12, tapeW + 5, 24);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(tapeX + 10, centerY + 6, QString::number((int)speed));
}

// ---- 3. ALTITUDE TAPE (MOVING LADDER) ----
void AttitudeIndicator::drawAltitudeTape(QPainter &painter, int w, int h) {
    int tapeW = 70;
    int tapeX = w - 100;
    int visibleHeight = h - 130;
    int centerY = 50 + visibleHeight / 2;

    painter.fillRect(tapeX, 50, tapeW, visibleHeight, QColor(30, 30, 30, 200));
    painter.save();
    painter.setClipRect(tapeX, 50, tapeW, visibleHeight);

    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 10));

    // Dynamic ladder movement logic (Har 100 feet par mark)
    int startAlt = ((int)altitude / 100) * 100 - 300;
    for (int a = startAlt; a <= altitude + 300; a += 100) {
        if (a < 0) continue;
        int yOffset = centerY - (a - altitude) * 0.3f; // 0.3 pixels per foot
        painter.drawLine(tapeX, yOffset, tapeX + 15, yOffset);
        painter.drawText(tapeX + 20, yOffset + 5, QString::number(a));
    }
    painter.restore();

    // Current Altitude Indicator Box
    painter.fillRect(tapeX - 5, centerY - 12, tapeW + 5, 24, QColor(0, 0, 0));
    painter.setPen(QPen(Qt::green, 2));
    painter.drawRect(tapeX - 5, centerY - 12, tapeW + 5, 24);
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(tapeX + 8, centerY + 6, QString::number((int)altitude));
}

// ---- 4. HEADING TAPE (MOVING BAR) ----
void AttitudeIndicator::drawHeadingTape(QPainter &painter, int w, int h) {
    int tapeY = h - 60;
    int tapeH = 40;
    int tapeW = w - 200;
    int tapeX = 100;
    int centerX = tapeX + tapeW / 2;

    painter.fillRect(tapeX, tapeY, tapeW, tapeH, QColor(25, 25, 25));
    painter.save();
    painter.setClipRect(tapeX, tapeY, tapeW, tapeH);

    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 10));

    // Heading movement logic (0 to 360 degrees rotation wrapper)
    int startHeading = ((int)heading / 5) * 5 - 20;
    for (int hg = startHeading; hg <= heading + 20; hg += 5) {
        int normalizedHeading = (hg + 360) % 360;
        int xOffset = centerX + (hg - heading) * 4.0f; // 4 pixels per degree

        painter.drawLine(xOffset, tapeY, xOffset, tapeY + 10);

        if (normalizedHeading % 10 == 0) {
            QString label = QString::number(normalizedHeading / 10);
            if (normalizedHeading == 0) label = "N";
            else if (normalizedHeading == 90) label = "E";
            else if (normalizedHeading == 180) label = "S";
            else if (normalizedHeading == 270) label = "W";

            painter.drawText(xOffset - 8, tapeY + 28, label);
        }
    }
    painter.restore();

    // Center Heading Pointer Index
    painter.setPen(QPen(Qt::magenta, 2));
    painter.drawLine(centerX, tapeY, centerX, tapeY + tapeH);
    // Top box digital heading show karne ke liye
    painter.fillRect(centerX - 20, tapeY - 18, 40, 16, QColor(0, 0, 0));
    painter.setPen(QPen(Qt::magenta, 1));
    painter.drawRect(centerX - 20, tapeY - 18, 40, 16);
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(centerX - 13, tapeY - 5, QString("%1").arg((int)heading, 3, 10, QChar('0')));
}

// ---- 5. TEXT READOUTS (PITCH & ROLL LABELS) ----
void AttitudeIndicator::drawDataReadouts(QPainter &painter, int w, int h) {
    Q_UNUSED(w);
    // Top right aur left par digital text formats likhna
    painter.setPen(Qt::white);
    painter.setFont(QFont("Consolas", 11, QFont::Bold));

    painter.drawText(110, 35, QString("PITCH: %1°").arg(pitch, 0, 'f', 1));
    painter.drawText(400, 35, QString("ROLL:  %1°").arg(roll, 0, 'f', 1));
}


