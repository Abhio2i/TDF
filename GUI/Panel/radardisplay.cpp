#include "radardisplay.h"
#include <cmath>
#include <QDebug>
#include <QJsonParseError>
#include <QHBoxLayout>
#include <QPainter>
#include <core/Debug/console.h>
// radar.cpp
// Aspect Ratio: 9 (width) : 16 (height)
constexpr double ASPECT_RATIO = 16.0 / 9.0;

int RadarDisplay::heightForWidth(int width) const
{
    // height = width * (16 / 9)
    return qRound(width * ASPECT_RATIO);
}

QSize RadarDisplay::sizeHint() const
{
    // एक डिफ़ॉल्ट आकार संकेत (hint) जो अनुपात का सम्मान करता हो
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

QSize RadarDisplay::minimumSize() const
{
    // एक न्यूनतम आकार सेट करें (जैसे 90x160)
    int minWidth = 90;
    return QSize(minWidth, heightForWidth(minWidth));
}

RadarDisplay::RadarDisplay(QWidget *parent) : QWidget(parent)
{
    //etMinimumSize(500, 800);
    setStyleSheet("background-color: black;");
    // Aspect Ratio को बनाए रखने के लिए QSizePolicy सेट करें
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWindowTitle("Radar Display");
    policy.setHeightForWidth(true); // heightForWidth() को प्रभावी बनाने के लिए
    setSizePolicy(policy);
    for(int i=1;i<10;i++){
        Target target;
        target.angle = 9*i;
        target.radius = 10*i;
        targets.append(target);
    }

}

void RadarDisplay::selectEntity(Entity* entit){
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

void RadarDisplay::RemoveEntity(QString ID){
    if(id == ID){
        entity = nullptr;
        sensor = nullptr;
        setWindowTitle("Radar Display");
    }
}


void RadarDisplay::updateRadar(){
    //qDebug()<<entity;
    //qDebug()<<sensor;
    if(entity&&sensor){
        // qDebug()<<"csdvfysgvsgsjkygj";
        setRange(sensor->range);
        setAzimuth(sensor->maxDetectionAngle);
        update();
    }
}


void RadarDisplay::updateFromJson(const QJsonObject &json)
{
    if (json.contains("range") && json["range"].isDouble()) {
        range = json["range"].toInt();
    }
    if (json.contains("azimuth") && json["azimuth"].isDouble()) {
        azimuth = json["azimuth"].toDouble();
    }
    if (json.contains("bar") && json["bar"].isDouble()) {
        bar = json["bar"].toDouble();
    }
    if (json.contains("current_speed") && json["current_speed"].isDouble()) {
        current_speed = json["current_speed"].toInt();
    }
    if (json.contains("max_speed") && json["max_speed"].isDouble()) {
        max_speed = json["max_speed"].toInt();
    }
    if (json.contains("height") && json["height"].isDouble()) {
        radar_height = json["height"].toInt();
    }
    if (json.contains("targets") && json["targets"].isArray()) {
        targets.clear();
        QJsonArray targetArray = json["targets"].toArray();
        for (const QJsonValue &value : targetArray) {
            QJsonObject targetObj = value.toObject();
            if (targetObj.contains("angle") && targetObj.contains("radius")) {
                Target target;
                target.angle = targetObj["angle"].toDouble();
                target.radius = targetObj["radius"].toDouble();
                targets.append(target);
            }
        }
    }
    update();
}

void RadarDisplay::drawBackground(QPainter &painter)
{
    painter.setBrush(Qt::black);
    painter.drawRect(rect());
    painter.setPen(QPen(Qt::green, 1));
    painter.setBrush(Qt::black);
    painter.drawRect(30, 30, QWidget::width() - 60, QWidget::height() - 60);
}

void RadarDisplay::setAzimuth(float value){
    azimuth = value;
}

void RadarDisplay::setRange(float value){
    range = value;
}

void RadarDisplay::drawConcentricCircles(QPainter &painter, int centerX, int centerY, int radius)
{
    painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
    painter.setBrush(Qt::transparent);
    int panelhigh = QWidget::height()-60;
    int offset = 10;//km
    float num = (range<offset?offset:range)/10;


    for (float i = 1; i <= num; i++) {
        float per = (i*offset)/range;
        float radiu = panelhigh*per;
        // qDebug()<<num<<","<<range<<","<<radiu<<","<<per;
        painter.drawEllipse(centerX-(radiu), centerY-(radiu), radiu*2, radiu*2);
    }
}

void RadarDisplay::drawVerticalLine(QPainter &painter, int centerX, int centerY, int radius)
{
    painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
    painter.drawLine(centerX, centerY, centerX, 30);
}

void RadarDisplay::drawCenterSquare(QPainter &painter, int centerX, int centerY)
{
    painter.setBrush(Qt::white);
    painter.drawRect(centerX - 5, centerY - 5, 10, 10);
}

void RadarDisplay::drawAnnotations(QPainter &painter, int centerX, int widgetHeight)
{
    Q_UNUSED(centerX);
    Q_UNUSED(widgetHeight);
    painter.setPen(QPen(Qt::green, 1));
    painter.setFont(QFont("Arial", 9));
    // Top: CRM, ACM, TWS, ATA, AAST spread across full width
    int width = QWidget::width();
    int spacing = width / 6; // Divide width into 6 parts (5 annotations + 1 for padding)
    painter.drawText(spacing * 1 - 20, 15, "CRM");
    painter.drawText(spacing * 2 - 20, 15, "ACM");
    painter.drawText(spacing * 3 - 20, 15, "TWS");
    painter.drawText(spacing * 4 - 20, 15, "ATA");
    painter.drawText(spacing * 5 - 20, 15, "AAST");
    // Next line: Current Speed (left), Max Speed (right)
    painter.drawText(5, 30, QString("%1").arg(range));
    // painter.drawText(width - 100, 30, max_speed != 0 ? QString("Max Speed: %1").arg(max_speed) : "Max Speed: N/A");
    // Left side: 3B below Current Speed
    painter.drawText(5, 45, QString("%1 A").arg(azimuth));

}

void RadarDisplay::drawAzimuth(QPainter &painter, int centerX, int centerY, int radius)
{
    radius *= 2;
    painter.setPen(QPen(Qt::blue, 2));
    double azimuthRad = 90 * M_PI / 180;
    double halfBeamWidth = azimuth / 2.0;
    double leftAngle = azimuthRad - (halfBeamWidth * M_PI / 180);
    double rightAngle = azimuthRad + (halfBeamWidth * M_PI / 180);
    int leftEndX = centerX + static_cast<int>(radius * cos(leftAngle));
    int leftEndY = centerY - static_cast<int>(radius * sin(leftAngle));
    int rightEndX = centerX + static_cast<int>(radius * cos(rightAngle));
    int rightEndY = centerY - static_cast<int>(radius * sin(rightAngle));
    painter.drawLine(centerX, centerY, leftEndX, leftEndY);
    painter.drawLine(centerX, centerY, rightEndX, rightEndY);
}

void RadarDisplay::drawTargetAndPath(QPainter &painter, int centerX, int centerY)
{

    if(entity&&sensor){
    painter.setBrush(Qt::red);
    for (const Target &target : sensor->targets) {
        int panelhigh = QWidget::height()-60;
        float per = target.radius/range;
        float radius = panelhigh*per;
        float angle = target.angle;
        //qDebug()<<radius;
        if(std::abs(angle)>(azimuth/2))continue;
        double targetAngle = (angle+90) * M_PI / 180; // Target at 80°
        double targetRadius = radius;
        int targetX = centerX + static_cast<int>(targetRadius * cos(targetAngle));
        int targetY = centerY - static_cast<int>(targetRadius * sin(targetAngle));
        painter.drawEllipse(targetX - 3, targetY - 3, 6, 6);
        painter.setPen(QPen(Qt::cyan, 1, Qt::DotLine));
        QPointF pathPoints[] = {
            QPointF(centerX, centerY),
            QPointF(centerX, centerY - static_cast<int>(0.4 * radius)),
            QPointF(targetX, targetY)
        };
        painter.drawPolyline(pathPoints, 3);
        painter.setPen(QPen(Qt::green, 1));
        painter.drawText(targetX - 20, targetY - 10, QString("%1").arg(angle));
        painter.drawText(targetX - 20, targetY + 5, QString("%1").arg(radius));
    }
    }
}

// void RadarDisplay::drawInfoText(QPainter &painter, int widgetHeight)
// {
//     Q_UNUSED(painter);
//     Q_UNUSED(widgetHeight);
//     // No text displayed at the bottom
// }

void RadarDisplay::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int centerX = width() / 2;
    int centerY = QWidget::height() - 30;
    int radius = (QWidget::height() - 100) / 2;
    drawBackground(painter);
    drawConcentricCircles(painter, centerX, centerY, radius);
    drawVerticalLine(painter, centerX, centerY, radius);
    drawAnnotations(painter, centerX, QWidget::height());
    drawAzimuth(painter, centerX, centerY, width()*2);
    drawCenterSquare(painter, centerX, centerY);
    drawTargetAndPath(painter, centerX, centerY);
    // drawInfoText(painter, QWidget::height());

    QWidget::paintEvent(event);
}

// Radar::Radar(QWidget *parent) : QWidget(parent)
// {
//     setMinimumSize(400, 600);
//     QHBoxLayout *layout = new QHBoxLayout(this);
//     layout->setContentsMargins(0, 0, 0, 0);
//     radarDisplay = new RadarDisplay(this);
//     layout->addWidget(radarDisplay);
//     setLayout(layout);

//     // JSON with all parameters defined
//     QString jsonString = R"({
//         "radar": {
//             "range": 50,
//             "azimuth": 90,
//             "bar": 3,
//             "current_speed": 40,
//             "max_speed": 50,
//             "height": 433,
//             "targets": [
//                 { "angle": 80, "radius": 0.7 }
//             ]
//         }
//     })";
//     QByteArray jsonData = jsonString.toUtf8();
//     QJsonParseError parseError;
//     QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
//     if (parseError.error != QJsonParseError::NoError) {
//         qDebug() << "JSON parse error:" << parseError.errorString();
//         return;
//     }
//     radarDisplay->updateFromJson(jsonDoc.object()["radar"].toObject());
// }

// void Radar::updateDisplay(const QJsonDocument &jsonDoc)
// {
//     if (radarDisplay) {
//         radarDisplay->updateFromJson(jsonDoc.object()["radar"].toObject());
//     }
// }
