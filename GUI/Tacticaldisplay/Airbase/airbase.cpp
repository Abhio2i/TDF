#include "airbase.h"
#include "../Gis/gislib.h"
#include "Setup.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <core/Debug/console.h>

// ============================================================================
// CHAITANYA JAIN CHANGE
//
// Type:
// Safe Refactoring
//
// Objective:
// Extract Airbase functionality from GISlib into a dedicated module.
//
// Root Cause:
// Airbase logic was tightly coupled with GISlib, reducing modularity and
// maintainability.
//
// Legacy Behaviour:
// Airbase rendering and management resided entirely inside GISlib.
//
// New Behaviour:
// GISlib delegates all Airbase responsibilities to the new Airbase module.
// Runtime behavior remains identical.
//
// Why this is safe:
// This is a structural refactor only. No rendering logic, calculations,
// coordinate handling, event flow, or user-visible behavior has been modified.
//
// ============================================================================

Airbase::Airbase() : showAirbases(false), airbaseIconPath(":/icons/images/airbase.png") {
}

void Airbase::initialize() {
    airbasePixmap = QPixmap(airbaseIconPath);
    if (airbasePixmap.isNull()) {
        Console::error("Failed to load airbase icon: " + airbaseIconPath.toStdString());
    }
}

void Airbase::loadAirbaseData() {
    if (!airbasePositions.empty()) return;
    QString airbaseJsonPath = TDFManager::instance()->getAirbaseJsonPath();
    QFile file(airbaseJsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    QJsonObject root = doc.object();
    if (!root.contains("airbases") || !root["airbases"].isArray()) {
        return;
    }
    QJsonArray airbaseArray = root["airbases"].toArray();
    airbasePositions.clear();
    airbasePositions.reserve(airbaseArray.size());
    for (const QJsonValue& val : airbaseArray) {
        QJsonObject obj = val.toObject();
        double lon = obj["lon"].toDouble();
        double lat = obj["lat"].toDouble();
        airbasePositions.emplace_back(lon, lat);
    }
}

void Airbase::toggleVisibility() {
    if (!showAirbases) {
        loadAirbaseData();
        showAirbases = true;
    } else {
        showAirbases = false;
    }
}

void Airbase::render(QPainter& painter, GISlib* gis, int zoom) {
    Q_UNUSED(zoom);
    if (!showAirbases || airbasePositions.empty() || airbasePixmap.isNull())
        return;

    const int iconSize = 20;
    static QPixmap cachedScaledIcon =
        airbasePixmap.scaled(iconSize, iconSize,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);

    for (const auto& pos : airbasePositions) {
        QPointF canvasPos = gis->geoToCanvas(pos.second, pos.first);
        painter.drawPixmap(
            canvasPos.x() - iconSize / 2,
            canvasPos.y() - iconSize / 2,
            cachedScaledIcon
            );
    }
}